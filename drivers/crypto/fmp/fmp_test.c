/*
 * Exynos FMP cipher driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 * Authors: Boojin Kim <boojin.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/crypto.h>
#include <linux/buffer_head.h>
#include <linux/genhd.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/blk_types.h>
#include <crypto/fmp.h>
#include <linux/mm_types.h>
#include "fmp_test.h"

#define MAX_RETRY_COUNT (0x100)
#define FIPS_BLOCK_NAME	"/dev/block/by-name/fmp_selftest"

/* test block device init for fmp test */
struct fmp_test_data *fmp_test_init(struct exynos_fmp *fmp)
{
	struct fmp_test_data *data;
	struct device *dev;
	struct inode *inode;
	struct super_block *sb;
	unsigned long blocksize;
	unsigned char blocksize_bits;
	uint32_t count = 0;

	fmode_t fmode = FMODE_WRITE | FMODE_READ;

	if (!fmp) {
		pr_err("%s: Invalid exynos fmp struct\n", __func__);
		goto err;
	}

	dev = fmp->dev;
	data = kmalloc(sizeof(struct fmp_test_data), GFP_KERNEL);
	if (!data)
		goto err;
	do {
		data->bdev = blkdev_get_by_path(FIPS_BLOCK_NAME, fmode, NULL);
		if (IS_ERR(data->bdev)) {
			dev_warn(dev, "retry blkdev_get_by_path = %d\n", count);
			mdelay(100);
			count++;
			continue;
		} else {
			dev_info(dev, "FOUND fmp fips block = %d\n", count);
			break;
		}
	} while (count < MAX_RETRY_COUNT);

	if (IS_ERR(data->bdev)) {
		dev_err(dev, "%s: Fail to open block device\n", __func__);
		goto err_data;
	}
	data->test_block_offset = 1;

	inode = data->bdev->bd_inode;
	sb = inode->i_sb;
	blocksize = sb->s_blocksize;
	blocksize_bits = sb->s_blocksize_bits;
	data->sector =
	    (i_size_read(inode) -
	     (blocksize * data->test_block_offset)) >> blocksize_bits;

	return data;
err_data:
	kzfree(data);
err:
	return NULL;
}

int fmp_cipher_run(struct exynos_fmp *fmp, struct fmp_test_data *fdata,
		uint8_t *data, uint32_t len, bool bypass, uint32_t write,
		void *priv, struct fmp_crypto_info *ci)
{
	int ret = 0;
	struct device *dev;
	static struct buffer_head *bh;
	u32 org_algo_mode;

	if (!fmp || !fdata || !ci) {
		pr_err("%s: Invalid fmp struct(fmp, fdata, ci)\n", __func__);
		return -EINVAL;
	}
	dev = fmp->dev;

	bh = __getblk(fdata->bdev, fdata->sector, FMP_BLK_SIZE);
	if (!bh) {
		dev_err(dev, "%s: Fail to get block from bdev\n", __func__);
		return -ENODEV;
	}

	/* set algo_mode for test */
	org_algo_mode = ci->algo_mode;
	if (bypass)
		ci->algo_mode = EXYNOS_FMP_BYPASS_MODE;
	ci->algo_mode |= EXYNOS_FMP_ALGO_MODE_TEST;

	get_bh(bh);

	/* priv is diskc for crypto test. */
	if (!priv) {
		/* ci is fmp_test_data->ci */
		fmp->test_data = fdata;
		ci->ctx = fmp;
		ci->use_diskc = 0;
		ci->enc_mode = EXYNOS_FMP_FILE_ENC;
		bh->b_private = fmp;
		fmp->bh = bh;
		fmp->fips_run++;
	} else {
		/* ci is crypto_tfm_ctx(tfm) */
		bh->b_private = priv;
	}

	if (write == WRITE_MODE) {
		memcpy(bh->b_data, data, len);
		set_buffer_dirty(bh);
		ret = __sync_dirty_buffer(bh, REQ_SYNC | REQ_FUA);
		if (ret) {
			dev_err(dev, "%s: IO error syncing for write mode\n",
				__func__);
			ret = -EIO;
			goto out;
		}
		memset(bh->b_data, 0, FMP_BLK_SIZE);
	} else {
		lock_buffer(bh);
		fmp_set_bh_compl_handler(bh);
		submit_bh(REQ_OP_READ, REQ_SYNC | REQ_PRIO, bh);
		wait_on_buffer(bh);
		if (unlikely(!buffer_uptodate(bh))) {
			ret = -EIO;
			goto out;
		}
		memcpy(data, bh->b_data, len);
	}
out:
	if (ci)
		ci->algo_mode = org_algo_mode;
	put_bh(bh);

	fmp->bh = NULL;
	return ret;
}

int fmp_test_crypt(struct exynos_fmp *fmp, struct fmp_test_data *fdata,
		uint8_t *src, uint8_t *dst, uint32_t len, uint32_t enc,
		void *priv, struct fmp_crypto_info *ci)
{
	int ret = 0;

	if (!fdata) {
		pr_err("%s: Invalid exynos fmp struct\n", __func__);
		return -1;
	}

	if (enc == ENCRYPT) {
		ret = fmp_cipher_run(fmp, fdata, src, len, 0,
				WRITE_MODE, priv, ci);
		if (ret) {
			pr_err("Fail to run fmp cipher ret(%d)\n",
				ret);
			goto err;
		}
		ret = fmp_cipher_run(fmp, fdata, dst, len, 1,
				READ_MODE, priv, ci);
		if (ret) {
			pr_err("Fail to run fmp cipher ret(%d)\n",
				ret);
			goto err;
		}
	} else if (enc == DECRYPT) {
		ret = fmp_cipher_run(fmp, fdata, src, len, 1,
				WRITE_MODE, priv, ci);
		if (ret) {
			pr_err("Fail to run fmp cipher ret(%d)\n",
				ret);
			goto err;
		}
		ret = fmp_cipher_run(fmp, fdata, dst, len, 0,
				READ_MODE, priv, ci);
		if (ret) {
			pr_err("Fail to run fmp cipher ret(%d)\n",
				ret);
			goto err;
		}
	} else {
		pr_err("%s: Invalid enc %d mode\n", __func__, enc);
		goto err;
	}

	return 0;
err:
	return -EINVAL;
}

/* test block device release for fmp test */
void fmp_test_exit(struct fmp_test_data *fdata)
{
	fmode_t fmode = FMODE_WRITE | FMODE_READ;

	if (!fdata) {
		pr_err("%s: Invalid exynos fmp struct\n", __func__);
		return;
	}
	if (fdata->bdev)
		blkdev_put(fdata->bdev, fmode);
	kzfree(fdata);
}
