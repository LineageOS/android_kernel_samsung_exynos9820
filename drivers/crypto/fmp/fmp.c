/*
 * Exynos FMP driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 * Authors: Boojin Kim <boojin.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/smc.h>
#include <crypto/fmp.h>

#include "fmp_fips_main.h"
#ifdef CONFIG_EXYNOS_FMP_FIPS
#include "fmp_test.h"
#include "fmp_fips_info.h"
#include "fmp_fips_func_test.h"
#endif

#include <linux/genhd.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>

#define WORD_SIZE 4
#define FMP_IV_MAX_IDX (FMP_IV_SIZE_16 / WORD_SIZE)

#define byte2word(b0, b1, b2, b3)       \
			(((unsigned int)(b0) << 24) | \
			((unsigned int)(b1) << 16) | \
			((unsigned int)(b2) << 8) | (b3))
#define get_word(x, c)  byte2word(((unsigned char *)(x) + 4 * (c))[0], \
				((unsigned char *)(x) + 4 * (c))[1], \
				((unsigned char *)(x) + 4 * (c))[2], \
				((unsigned char *)(x) + 4 * (c))[3])

static struct device *fmp_dev;

static inline struct exynos_fmp *get_fmp(void)
{
	return dev_get_drvdata(fmp_dev);
}

static inline void dump_ci(struct fmp_crypto_info *c)
{
	if (c) {
		pr_info
		    ("%s: algo:%d enc:%d key_size:%d\n",
		     __func__, c->algo_mode, c->enc_mode,c->key_size);
		if (c->enc_mode == EXYNOS_FMP_FILE_ENC)
			print_hex_dump(KERN_CONT, "key:",
				       DUMP_PREFIX_OFFSET, 16, 1, c->key,
				       sizeof(c->key), false);
	}
}

static inline void dump_table(struct fmp_table_setting *table)
{
	print_hex_dump(KERN_CONT, "dump:", DUMP_PREFIX_OFFSET, 16, 1,
		       table, sizeof(struct fmp_table_setting), false);
}

static inline int is_set_fmp_disk_key(struct exynos_fmp *fmp)
{
	return (fmp->status_disk_key == KEY_SET) ? TRUE : FALSE;
}

static inline int is_stored_fmp_disk_key(struct exynos_fmp *fmp)
{
	return (fmp->status_disk_key == KEY_STORED) ? TRUE : FALSE;
}

static inline int is_supported_ivsize(u32 ivlen)
{
	if (ivlen && (ivlen <= FMP_IV_SIZE_16))
		return TRUE;
	else
		return FALSE;
}

static inline int check_aes_xts_size(struct fmp_table_setting *table,
				     bool cmdq_enabled)
{
	int size;

	if (cmdq_enabled)
		size = GET_CMDQ_LENGTH(table);
	else
		size = GET_LENGTH(table);
	return (size > MAX_AES_XTS_TRANSFER_SIZE) ? size : 0;
}

/* check for fips that no allow same keys */
static inline int check_aes_xts_key(char *key,
				    enum fmp_crypto_key_size key_size)
{
	char *enckey, *twkey;

	enckey = key;
	twkey = key + key_size;
	return (memcmp(enckey, twkey, key_size)) ? 0 : -1;
}

int fmplib_set_algo_mode(struct fmp_table_setting *table,
			 struct fmp_crypto_info *crypto, bool cmdq_enabled)
{
	int ret;
	enum fmp_crypto_algo_mode algo_mode = crypto->algo_mode & EXYNOS_FMP_ALGO_MODE_MASK;

	if (algo_mode == EXYNOS_FMP_ALGO_MODE_AES_XTS) {
		ret = check_aes_xts_size(table, cmdq_enabled);
		if (ret) {
			pr_err("%s: Fail FMP XTS due to invalid size(%d), cmdq:%d\n",
			       __func__, ret, cmdq_enabled);
			return -EINVAL;
		}
	}

	switch (crypto->enc_mode) {
	case EXYNOS_FMP_FILE_ENC:
		if (cmdq_enabled)
			SET_CMDQ_FAS(table, algo_mode);
		else
			SET_FAS(table, algo_mode);
		break;
	case EXYNOS_FMP_DISK_ENC:
		if (cmdq_enabled)
			SET_CMDQ_DAS(table, algo_mode);
		else
			SET_DAS(table, algo_mode);
		break;
	default:
		pr_err("%s: Invalid fmp enc mode %d\n", __func__,
		       crypto->enc_mode);
		return -EINVAL;
	}
	return 0;
}

static int fmplib_set_file_key(struct fmp_table_setting *table,
			struct fmp_crypto_info *crypto)
{
	enum fmp_crypto_algo_mode algo_mode = crypto->algo_mode & EXYNOS_FMP_ALGO_MODE_MASK;
	enum fmp_crypto_key_size key_size = crypto->fmp_key_size;
	char *key = crypto->key;
	int idx, max;

	if (!key || (crypto->enc_mode != EXYNOS_FMP_FILE_ENC) ||
		((key_size != EXYNOS_FMP_KEY_SIZE_16) &&
		 (key_size != EXYNOS_FMP_KEY_SIZE_32))) {
		pr_err("%s: Invalid key_size:%d enc_mode:%d\n",
		       __func__, key_size, crypto->enc_mode);
		return -EINVAL;
	}

	if ((algo_mode == EXYNOS_FMP_ALGO_MODE_AES_XTS)
	    && check_aes_xts_key(key, key_size)) {
		pr_err("%s: Fail FMP XTS due to the same enc and twkey\n",
		       __func__);
		return -EINVAL;
	}

	if (algo_mode == EXYNOS_FMP_ALGO_MODE_AES_CBC) {
		max = key_size / WORD_SIZE;
		for (idx = 0; idx < max; idx++)
			*(&table->file_enckey0 + idx) =
			    get_word(key, max - (idx + 1));
	} else if (algo_mode == EXYNOS_FMP_ALGO_MODE_AES_XTS) {
		key_size *= 2;
		max = key_size / WORD_SIZE;
		for (idx = 0; idx < (max / 2); idx++)
			*(&table->file_enckey0 + idx) =
			    get_word(key, (max / 2) - (idx + 1));
		for (idx = 0; idx < (max / 2); idx++)
			*(&table->file_twkey0 + idx) =
			    get_word(key, max - (idx + 1));
	}
	return 0;
}

static int fmplib_set_key_size(struct fmp_table_setting *table,
			struct fmp_crypto_info *crypto, bool cmdq_enabled)
{
	enum fmp_crypto_key_size key_size;

	key_size = crypto->fmp_key_size;

	if ((key_size != EXYNOS_FMP_KEY_SIZE_16) &&
		(key_size != EXYNOS_FMP_KEY_SIZE_32)) {
		pr_err("%s: Invalid keysize %d\n", __func__, key_size);
		return -EINVAL;
	}

	switch (crypto->enc_mode) {
	case EXYNOS_FMP_FILE_ENC:
		if (cmdq_enabled)
			SET_CMDQ_KEYLEN(table,
					(key_size ==
					 FMP_KEY_SIZE_32) ? FKL_CMDQ : 0);
		else
			SET_KEYLEN(table,
				   (key_size == FMP_KEY_SIZE_32) ? FKL : 0);
		break;
	case EXYNOS_FMP_DISK_ENC:
		if (cmdq_enabled)
			SET_CMDQ_KEYLEN(table,
					(key_size ==
					 FMP_KEY_SIZE_32) ? DKL_CMDQ : 0);
		else
			SET_KEYLEN(table,
				   (key_size == FMP_KEY_SIZE_32) ? DKL : 0);
		break;
	default:
		pr_err("%s: Invalid fmp enc mode %d\n", __func__,
		       crypto->enc_mode);
		return -EINVAL;
	}
	return 0;
}

static int fmplib_set_disk_key(struct exynos_fmp *fmp, u8 *key, u32 key_size)
{
	u64 ret;

	/* TODO: only set for host0 */
	// hack: mark on 5.4  __flush_dcache_area(key, (size_t) FMP_MAX_KEY_SIZE);
	ret =
	    exynos_smc(SMC_CMD_FMP_DISK_KEY_STORED, 0, virt_to_phys(key),
		       key_size);
	if (ret) {
		pr_err("%s: Fail to set FMP disk key. ret = %d\n", __func__,
		       ret);
		fmp->status_disk_key = KEY_ERROR;
		return -EINVAL;
	}
	fmp->status_disk_key = KEY_STORED;
	return 0;
}

static int fmplib_clear_disk_key(struct exynos_fmp *fmp)
{
	u64 ret;

	ret = exynos_smc(SMC_CMD_FMP_DISK_KEY_CLEAR, 0, 0, 0);
	if (ret) {
		pr_err("%s: Fail to clear FMP disk key. ret = %d\n",
		       __func__, ret);
		fmp->status_disk_key = KEY_ERROR;
		return -EPERM;
	}

	fmp->status_disk_key = KEY_CLEAR;
	return 0;
}

static int fmplib_set_iv(struct fmp_table_setting *table,
		  struct fmp_crypto_info *crypto, u8 *iv)
{
	int idx;

	switch (crypto->enc_mode) {
	case EXYNOS_FMP_FILE_ENC:
		for (idx = 0; idx < FMP_IV_MAX_IDX; idx++)
			*(&table->file_iv0 + idx) =
			    get_word(iv, FMP_IV_MAX_IDX - (idx + 1));
		break;
	case EXYNOS_FMP_DISK_ENC:
		for (idx = 0; idx < FMP_IV_MAX_IDX; idx++)
			*(&table->disk_iv0 + idx) =
			    get_word(iv, FMP_IV_MAX_IDX - (idx + 1));
		break;
	default:
		pr_err("%s: Invalid fmp enc mode %d\n", __func__,
		       crypto->enc_mode);
		return -EINVAL;
	}
	return 0;
}

static void fmp_dumpsfr(void)
{
	u64 fmp_sec = exynos_smc(SMC_CMD_FMP_SMU_DUMP, 0, 0, 0x10);
	u64 fmp_ver = exynos_smc(SMC_CMD_FMP_SMU_DUMP, 0, 0, 0x1c);

	pr_info("%s: sec:%x, ver:%x\n", __func__, fmp_sec, fmp_ver);
}

int gnum3;
int exynos_fmp_crypt(struct fmp_crypto_info *fmp_ci, void *priv)
{
	struct exynos_fmp *fmp = get_fmp();
	struct fmp_request *r = priv;
	struct fmp_crypto_info *ci = fmp_ci;
	u64 ret = 0;
	u8 iv[FMP_IV_SIZE_16];
	bool fips = 0;
	enum fmp_crypto_algo_mode algo_mode;
	struct exynos_fmp_fips_test_vops *test_vops = NULL;

	if (!r || !fmp) {
		pr_err("%s: invalid req or fmp\n", __func__);
		return -EINVAL;
	}

#ifdef CONFIG_EXYNOS_FMP_FIPS
	test_vops = fmp->test_vops;
#endif

	if (unlikely(in_fmp_fips_err())) {
#if defined(CONFIG_NODE_FOR_SELFTEST_FAIL)
		pr_err("%s: Fail to work fmp config due to fips in error.\n",
			__func__);
		return -EINVAL;
#else
	if (test_vops)
		pr_err("%s: Fail to work fmp config due to fips in error.\n",
			__func__);
	else
		panic("%s: Fail to work fmp config due to fips in error\n",
			__func__);
	return -EINVAL;
#endif
	}

	/* check fips test mode */
	if (r->fips) {
		if (!fmp->test_data) {
			pr_err("%s: no test_data for test mode\n", __func__);
			goto out;
		}

		ci = &fmp->test_data->ci;
		if (!(ci->algo_mode & EXYNOS_FMP_ALGO_MODE_TEST)) {
			pr_err("%s: no test_data for algo mode\n", __func__);
			goto out;
		}
	}
	algo_mode = ci->algo_mode & EXYNOS_FMP_ALGO_MODE_MASK;

	/* check test mode */
	if (ci->algo_mode & EXYNOS_FMP_ALGO_MODE_TEST) {
		if (!algo_mode)
			return 0;

		/* use test manager's iv instead of host driver's iv */
		r->iv = fmp->test_data->iv;
		r->ivsize = sizeof(fmp->test_data->iv);
		dev_dbg(fmp->dev, "%s: fips crypt: ivsize:%d, key_size: %d, %d\n",
				__func__, r->ivsize, ci->key_size, ci->fmp_key_size);
		fips = true;
	}

#ifdef CONFIG_EXYNOS_FMP_FIPS
	if (fips == false && fmp->result.overall == 0) {
#if defined(CONFIG_NODE_FOR_SELFTEST_FAIL)
		pr_err("%s: Fail to work fmp config due to all fips test is not done.\n",
			__func__);
#else
		panic("%s: Fail to work fmp config due to all fips test is not done\n",
			__func__);
#endif
		return -EINVAL;
	}
#endif

	/* check crypto info & input param */
	if (!algo_mode || !is_supported_ivsize(r->ivsize) ||
			!r->table || !r->iv) {
		dev_err(fmp->dev, "%s: invalid mode:%d ivsize:%d\n",
			__func__, algo_mode, r->ivsize);
		ret = -EINVAL;
		goto out;
	}

	/* set algo & enc mode into table */
	ret = fmplib_set_algo_mode(r->table, ci, r->cmdq_enabled);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP encryption mode\n", __func__);

		ret = -EINVAL;
		goto out;
	}

	/* set key size into table */
	switch (ci->enc_mode) {
	case EXYNOS_FMP_FILE_ENC:
		ret = fmplib_set_file_key(r->table, ci);
		if (ret) {
			dev_err(fmp->dev, "%s: Fail to set FMP key\n",
				__func__);
			ret = -EINVAL;
			goto out;
		}
		break;
	case EXYNOS_FMP_DISK_ENC:
		if (is_stored_fmp_disk_key(fmp)) {
			ret = exynos_smc(SMC_CMD_FMP_DISK_KEY_SET, 0, 0, 0);
			if (ret) {
				dev_err(fmp->dev,
					"%s: Fail to set disk key\n", __func__);
				goto out;
			}
			fmp->status_disk_key = KEY_SET;
		} else if (!is_set_fmp_disk_key(fmp)) {
			dev_err(fmp->dev,
				"%s: Fail because disk key is clear\n",
				__func__);
			ret = -EINVAL;
			goto out;
		}
		break;
	default:
		dev_err(fmp->dev, "%s: Invalid fmp enc mode %d\n", __func__,
			ci->enc_mode);
		ret = -EINVAL;
		goto out;
	}

	/* set key size into table */
	ret = fmplib_set_key_size(r->table, ci, r->cmdq_enabled);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP key size\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	/* set iv */
	memset(iv, 0, FMP_IV_SIZE_16);
	memcpy(iv, r->iv, r->ivsize);
	ret = fmplib_set_iv(r->table, ci, iv);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to set FMP IV\n", __func__);
		ret = -EINVAL;
		goto out;
	}
out:
	if (ret) {
		dump_ci(ci);
		if (r && r->table)
			dump_table(r->table);
	}
	return ret;
}
EXPORT_SYMBOL(exynos_fmp_crypt);

static inline void fmplib_clear_file_key(struct fmp_table_setting *table)
{
	memset(&table->file_iv0, 0, sizeof(__le32) * 24);
}

int exynos_fmp_clear(struct fmp_crypto_info *ci, void *priv)
{
	struct fmp_request *r = priv;
#ifdef CONFIG_EXYNOS_FMP_FIPS
	struct exynos_fmp *fmp = get_fmp();
	struct exynos_fmp_fips_test_vops *test_vops = fmp->test_vops;
	int ret;
#endif

	if (!r) {
		pr_err("%s: Invalid input\n", __func__);
		return -EINVAL;
	}

	if (!r->table) {
		pr_err("%s: Invalid input table\n", __func__);
		return -EINVAL;
	}

#ifdef CONFIG_EXYNOS_FMP_FIPS
	if (test_vops) {
		ret = test_vops->zeroization(r->table, "before");
		if (ret)
			dev_err(fmp->dev,
				"%s: Fail to check zeroization(before)\n",
				__func__);
	}
#endif
	fmplib_clear_file_key(r->table);

#ifdef CONFIG_EXYNOS_FMP_FIPS
	if (test_vops) {
		ret = test_vops->zeroization(r->table, "after");
		if (ret)
			dev_err(fmp->dev,
				"%s: Fail to check zeroization(after)\n",
				__func__);
	}
#endif

	return 0;
}
EXPORT_SYMBOL(exynos_fmp_clear);

int exynos_fmp_setkey(struct fmp_crypto_info *ci, u8 *in_key, u32 keylen,
		      bool persistent)
{
	struct exynos_fmp *fmp = get_fmp();
	int ret = 0;
	int keylen_org = keylen;
	enum fmp_crypto_algo_mode algo_mode = ci->algo_mode & EXYNOS_FMP_ALGO_MODE_MASK;

	if (!fmp || !in_key) {
		pr_err("%s: invalid input param\n", __func__);
		return -EINVAL;
	}

	/* set key_size & fmp_key_size */
	if (algo_mode == EXYNOS_FMP_ALGO_MODE_AES_XTS)
		keylen = keylen >> 1;
	switch (keylen) {
	case FMP_KEY_SIZE_16:
		ci->fmp_key_size = EXYNOS_FMP_KEY_SIZE_16;
		break;
	case FMP_KEY_SIZE_32:
		ci->fmp_key_size = EXYNOS_FMP_KEY_SIZE_32;
		break;
	default:
		pr_err("%s: FMP doesn't support key size %d\n", __func__,
		       keylen);
		return -ENOKEY;
	}
	ci->key_size = keylen_org;

	/* set key */
	if (persistent) {
		ci->enc_mode = EXYNOS_FMP_DISK_ENC;
		ret = fmplib_set_disk_key(fmp, in_key, ci->key_size);
		if (ret)
			pr_err("%s: Fail to set FMP disk key\n", __func__);
	} else {
		ci->enc_mode = EXYNOS_FMP_FILE_ENC;
		memset(ci->key, 0, sizeof(ci->key));
		memcpy(ci->key, in_key, ci->key_size);
	}
	return ret;
}
EXPORT_SYMBOL(exynos_fmp_setkey);

int exynos_fmp_clearkey(struct fmp_crypto_info *ci)
{
	struct exynos_fmp *fmp = get_fmp();
	int ret = 0;

	if (!fmp) {
		pr_err("%s: invalid input param\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	if (ci->enc_mode == EXYNOS_FMP_DISK_ENC) {
		ret = fmplib_clear_disk_key(fmp);
		if (ret)
			pr_err("%s: fail to clear FMP disk key\n", __func__);
	} else if (ci->enc_mode == EXYNOS_FMP_FILE_ENC) {
		memset(ci->key, 0, sizeof(ci->key));
		ci->key_size = 0;
	} else {
		pr_err("%s: invalid algo mode:%d\n", __func__, ci->enc_mode);
		ret = -EINVAL;
	}
out:
	return ret;
}

int exynos_fmp_smu_abort(int id)
{
	u64 ret = 0;

	if (id == SMU_ID_MAX)
		return 0;

	ret = exynos_smc(SMC_CMD_SMU, SMU_ABORT, id, 0);
	if (ret)
		pr_err("%s: Fail smc call. ret(%d)\n", __func__, ret);

	return ret;
}

static void fmplib_bypass(void *desc, bool cmdq_enabled)
{
	if (cmdq_enabled) {
		SET_CMDQ_FAS((struct fmp_table_setting *)desc, 0);
		SET_CMDQ_DAS((struct fmp_table_setting *)desc, 0);
	} else {
		SET_FAS((struct fmp_table_setting *)desc, 0);
		SET_DAS((struct fmp_table_setting *)desc, 0);
	}
}

#ifdef CONFIG_EXYNOS_FMP_FIPS
static bool fmp_check_fips(struct bio *bio, struct exynos_fmp *fmp)
{
	struct page *page;
	struct buffer_head *bh;
	bool find = false;
	struct fmp_crypto_info *ci;

	if (bio->bi_io_vec)  {
		page = bio->bi_io_vec[0].bv_page;
		if (page && !PageAnon(page) && page_has_buffers(page)) {
			bh = (void *)page->private;
			if (bh && ((void *)bh->b_private == (void *)fmp)) {
				find = true;
				if ((void *)page->private != (void *)fmp->bh) {
					dev_err(fmp->dev, "%s: invalid fips bh\n", __func__);
					return false;
				}
			}
		}
	}

	if (find) {
		fmp->fips_run--;
		fmp->fips_fin++;
		ci = &fmp->test_data->ci;
		dev_dbg(fmp->dev, "%s: find fips run(%d) fin(%d)with algo:%d, enc:%d, key_size:%d\n",
			__func__, fmp->fips_run, fmp->fips_fin, ci->algo_mode, ci->enc_mode, ci->key_size);
		if (ci->algo_mode == (EXYNOS_FMP_ALGO_MODE_TEST | EXYNOS_FMP_BYPASS_MODE)) {
			dev_dbg(fmp->dev, "%s: find fips for bypass mode\n", __func__);
			return 0;
		}
	}
	return find;
}
#endif

int exynos_fmp_fips(struct bio *bio)
{
	bool find = false;
#ifdef CONFIG_EXYNOS_FMP_FIPS
	struct exynos_fmp *fmp = get_fmp();
	struct page *page;
	struct buffer_head *bh;
	struct fmp_crypto_info *ci;

	if (fmp->fips_fin) {
		if (bio->bi_io_vec)  {
			page = bio->bi_io_vec[0].bv_page;
			if (page && !PageAnon(page) && page_has_buffers(page)) {
				bh = (void *)page->private;
				if (bh && ((void *)bh->b_private == (void *)fmp)) {
					find = true;
					if ((void *)page->private != (void *)fmp->bh) {
						dev_err(fmp->dev, "%s: invalid fips bh\n", __func__);
						return false;
					}
				}
			}
		}

		if (find) {
			fmp->fips_fin--;
			ci = &fmp->test_data->ci;
			dev_dbg(fmp->dev, "%s: find fips run(%d) fin(%d)with algo:%d, enc:%d, key_size:%d\n",
			__func__, fmp->fips_run, fmp->fips_fin, ci->algo_mode, ci->enc_mode, ci->key_size);
			if (ci->algo_mode == (EXYNOS_FMP_ALGO_MODE_TEST | EXYNOS_FMP_BYPASS_MODE)) {
				dev_dbg(fmp->dev, "%s: find fips for bypass mode\n", __func__);
				return 0;
			}
		}
	}
#endif
	return find;
}
EXPORT_SYMBOL(exynos_fmp_fips);

int exynos_fmp_bypass(struct fmp_request *req, struct bio *bio)
{
	int i;
	void *prd;
#ifdef CONFIG_EXYNOS_FMP_FIPS
	struct exynos_fmp *fmp = get_fmp();

	if (fmp->fips_run) {
		if (fmp_check_fips(bio, fmp)) {
			dev_dbg(fmp->dev, "%s: find fips\n", __func__);
			return -EINVAL;
		}
	}
#endif
	prd = req->table;
	for (i = 0; i < req->prdt_cnt; i++) {
		fmplib_bypass(prd, 0);
		prd = (void *)prd + req->prdt_off;
	}
	return 0;
}
EXPORT_SYMBOL(exynos_fmp_bypass);

#define CFG_DESCTYPE_3 0x3
int exynos_fmp_sec_cfg(int fmp_id, int smu_id, bool init)
{
	u64 ret = 0;

	/* configure fmp */
	if (fmp_id != SMU_ID_MAX) {
		ret = exynos_smc(SMC_CMD_FMP_SECURITY, 0, fmp_id, CFG_DESCTYPE_3);
		if (ret)
			pr_err("%s: Fail smc call for FMP_SECURITY. ret(%d)\n",
					__func__, ret);
	}

	/* configure smu */
	if (smu_id != SMU_ID_MAX) {
		if (init)
			ret = exynos_smc(SMC_CMD_SMU, SMU_INIT, smu_id, 0);
		else
			ret = exynos_smc(SMC_CMD_FMP_SMU_RESUME, 0, smu_id, 0);

		if (ret)
			pr_err("%s: Fail smc call for SMU_INIT/RESUME. ret(%d)\n",
					__func__, ret);
	}
	if (ret)
		fmp_dumpsfr();
	return ret;
}
EXPORT_SYMBOL(exynos_fmp_sec_cfg);

void *exynos_fmp_init(struct platform_device *pdev)
{
	struct exynos_fmp *fmp;
#ifdef CONFIG_EXYNOS_FMP_FIPS
	int ret = 0;
#endif

	if (!pdev) {
		pr_err("%s: Invalid platform_device.\n", __func__);
		goto err;
	}

	fmp = devm_kzalloc(&pdev->dev, sizeof(struct exynos_fmp), GFP_KERNEL);
	if (!fmp)
		goto err;

	fmp->dev = &pdev->dev;
	if (!fmp->dev) {
		pr_err("%s: Invalid device.\n", __func__);
		goto err_dev;
	}

	/* init disk key status */
	fmp->status_disk_key = KEY_CLEAR;

	dev_info(fmp->dev, "Exynos FMP Version: %s\n", FMP_DRV_VERSION);

#ifdef CONFIG_EXYNOS_FMP_FIPS
	atomic_set(&fmp->fips_start, 0);

	ret = exynos_fmp_fips_register(fmp);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to exynos_fmp_fips_register. ret(0x%x)",
				__func__, ret);
		goto err_dev;
	}

	ret = exynos_fmp_fips_integrity(fmp);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to exynos_fmp_fips_integrity. ret(0x%x)",
				__func__, ret);
		goto err_dev;
	}
#endif

	fmp_dev = &pdev->dev;

	dev_info(fmp->dev, "Exynos FMP driver is initialized\n");
	return fmp;

err_dev:
	devm_kfree(&pdev->dev, fmp);
err:
	return NULL;
}

void exynos_fmp_exit(struct platform_device *pdev)
{
	struct exynos_fmp *fmp = dev_get_drvdata(&pdev->dev);

#ifdef CONFIG_EXYNOS_FMP_FIPS
	exynos_fmp_fips_deregister(fmp);
#endif
	devm_kfree(&pdev->dev, fmp);
}
