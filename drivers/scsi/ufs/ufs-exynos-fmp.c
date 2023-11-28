/*
 * Exynos FMP UFS crypto interface
 *
 * Copyright (C) 2020 Samsung Electronics Co., Ltd.
 * Authors: Boojin Kim <boojin.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/keyslot-manager.h>
#include <crypto/fmp.h>
#include "ufshcd.h"
#include "ufshcd-crypto.h"
#include "ufs-exynos.h"
#include <crypto/fmp.h>

#ifdef CONFIG_SCSI_UFS_EXYNOS_FMP
enum fmp_crypto_api {
	api_init,
	api_setup_rq_keyslot_manager,
	api_destroy_rq_keyslot_manager,
	api_hba_init_crypto,
	api_suspend,
	api_resume,
	api_prepare_lrbp_crypto,
	api_prepare_lrbp_crypto_loop,
	api_complete_lrbp_crypto,
	api_lrbp_crypto_err,
	api_max,
};

#ifdef FMP_DEBUG
static u32 fmp_dcnt[api_max];
static void fmp_crypto_debug(enum fmp_crypto_api api, bool dump, void *table)
{
	bool need_dump = 0;

	fmp_dcnt[api]++;

	if (dump) {
		if (api == api_lrbp_crypto_err)
			need_dump = 1;
		else if (fmp_dcnt[api] % 30 == 0)
			need_dump = 1;

		if (need_dump) {
			pr_info("%s(%d): init:%d,%d keyslot:%d,%d s2r:%d,%d crypt:%d,%d,%d,%d\n",
				__func__,
				api,
				fmp_dcnt[api_init],
				fmp_dcnt[api_hba_init_crypto],
				fmp_dcnt[api_setup_rq_keyslot_manager],
				fmp_dcnt[api_destroy_rq_keyslot_manager],
				fmp_dcnt[api_suspend],
				fmp_dcnt[api_resume],
				fmp_dcnt[api_prepare_lrbp_crypto],
				fmp_dcnt[api_prepare_lrbp_crypto_loop],
				fmp_dcnt[api_complete_lrbp_crypto],
				fmp_dcnt[api_lrbp_crypto_err]);
			print_hex_dump(KERN_CONT, "fmp:", DUMP_PREFIX_OFFSET,
					16, 1, table, 64, false);
		}
	}
}
#else
static void fmp_crypto_debug(enum fmp_crypto_api api, bool dump, void *table)
{
	if (api == api_lrbp_crypto_err)
		print_hex_dump(KERN_CONT, "fmp:", DUMP_PREFIX_OFFSET,
					16, 1, table, 64, false);
}
#endif

static void fmp_ufshcd_crypto_setup_rq_keyslot_manager(struct ufs_hba *hba,
						 struct request_queue *q)
{
	q->ksm = hba->ksm;
	fmp_crypto_debug(api_setup_rq_keyslot_manager, 0, NULL);
}

static int fmp_ufshcd_prepare_lrbp_crypto(struct ufs_hba *hba,
			       struct scsi_cmnd *cmd,
			       struct ufshcd_lrb *lrbp)
{
	return 0;
}

static int fmp_ufshcd_map_sg_crypto(struct ufs_hba *hba,
			       struct ufshcd_lrb *lrbp)
{
	struct scsi_cmnd *cmd = lrbp->cmd;
	struct bio *bio = cmd->request->bio;
	struct request_queue *q = cmd->request->q;
	int sg_segments = scsi_sg_count(lrbp->cmd);
	struct scatterlist *sg;
	struct fmp_crypto_info fmp_info;
	struct fmp_request req;
	int ret = 0;
	int idx = 0;
	u64 iv = 0;
	struct ufshcd_sg_entry *prd = (struct ufshcd_sg_entry *)lrbp->ucd_prdt_ptr;

	if (!bio || !q)
		return 0;

	if (!q->ksm || !bio_crypt_should_process(cmd->request)) {
		req.table = prd;
		req.prdt_off = hba->sg_entry_size;
		req.prdt_cnt = sg_segments;
		ret = exynos_fmp_bypass(&req, bio);
		if (ret) {
			pr_debug("%s: find fips\n", __func__);
			req.fips = true;
			goto encrypt;
		}
		return 0;
	}
	fmp_info.enc_mode = EXYNOS_FMP_FILE_ENC;
	fmp_info.algo_mode = EXYNOS_FMP_ALGO_MODE_AES_XTS;

	ret = exynos_fmp_setkey(&fmp_info,
		(u8 *)bio->bi_crypt_context->bc_key->raw,
		bio->bi_crypt_context->bc_key->size, 0);
	if (ret) {
		pr_err("%s: fails to set fmp key. ret:%d\n", __func__, ret);
		fmp_crypto_debug(api_lrbp_crypto_err, 1, req.table);
		return ret;
	}

	req.iv = &iv;
	req.ivsize = sizeof(iv);
	req.fips = false;
encrypt:
	req.cmdq_enabled = 0;
	scsi_for_each_sg(lrbp->cmd, sg, sg_segments, idx) {
		if (!req.fips)
			iv = bio->bi_crypt_context->bc_dun[0] + idx;
		req.table = prd;
		ret = exynos_fmp_crypt(&fmp_info, &req);
		if (ret) {
			pr_err("%s: fails to crypt fmp. ret:%d\n", __func__, ret);
			fmp_crypto_debug(api_lrbp_crypto_err, 1, req.table);
			return ret;
		}
		prd = (void *)prd + hba->sg_entry_size;
		fmp_crypto_debug(api_prepare_lrbp_crypto_loop, 0, NULL);
	}
	fmp_crypto_debug(api_prepare_lrbp_crypto, 1, &lrbp->ucd_prdt_ptr[0]);
	return 0;
}

static int fmp_ufshcd_complete_lrbp_crypto(struct ufs_hba *hba,
				struct scsi_cmnd *cmd,
				struct ufshcd_lrb *lrbp)
{
	struct bio *bio = cmd->request->bio;
	struct request_queue *q = cmd->request->q;
	int sg_segments = scsi_sg_count(lrbp->cmd);
	struct scatterlist *sg;
	struct fmp_crypto_info fmp_info;
	struct fmp_request req;
	int ret = 0;
	int idx = 0;
	struct ufshcd_sg_entry *prd;

	if (!bio || !q)
		return 0;

	if (!q->ksm || !bio_crypt_should_process(cmd->request)) {
		ret = exynos_fmp_fips(bio);
		if (ret) {
			pr_debug("%s: find fips\n", __func__);
			req.fips = true;
		} else {
			return 0;
		}
	}

	prd = (struct ufshcd_sg_entry *)lrbp->ucd_prdt_ptr;
	scsi_for_each_sg(lrbp->cmd, sg, sg_segments, idx) {
		req.table = prd;
		ret = exynos_fmp_clear(&fmp_info, &req);
		if (ret) {
			pr_warn("%s: fails to clear fips\n", __func__);
			break;
		}
		prd = (void *)prd + hba->sg_entry_size;
	}
	fmp_crypto_debug(api_complete_lrbp_crypto, 1, &lrbp->ucd_prdt_ptr[0]);
	return 0;
}

static const struct keyslot_mgmt_ll_ops fmp_ksm_ops = {
};

static int fmp_ufshcd_hba_init_crypto(struct ufs_hba *hba,
					const struct keyslot_mgmt_ll_ops *ksm_ops)
{
	unsigned int crypto_modes_supported[BLK_ENCRYPTION_MODE_MAX] = {
		[BLK_ENCRYPTION_MODE_AES_256_XTS] = 4096,
	};

	fmp_crypto_debug(api_hba_init_crypto, 0, NULL);
	hba->ksm = keyslot_manager_create_passthrough(NULL, &fmp_ksm_ops,
					BLK_CRYPTO_FEATURE_STANDARD_KEYS,
					crypto_modes_supported, NULL);
	if (!hba->ksm) {
		pr_info("%s fails to get keyslot manager\n", __func__);
		return -EINVAL;
	}
	return 0;
}

static struct ufs_hba_crypto_variant_ops exynos_ufs_fmp_ops = {
	.setup_rq_keyslot_manager = fmp_ufshcd_crypto_setup_rq_keyslot_manager,
	.hba_init_crypto = fmp_ufshcd_hba_init_crypto,
	.map_sg_crypto = fmp_ufshcd_map_sg_crypto,
	.prepare_lrbp_crypto = fmp_ufshcd_prepare_lrbp_crypto,
	.complete_lrbp_crypto = fmp_ufshcd_complete_lrbp_crypto,
};

void exynos_ufs_fmp_config(struct ufs_hba *hba, bool init)
{
	if (init) {
		fmp_crypto_debug(api_init, 0, NULL);
		hba->sg_entry_size = sizeof(struct fmp_table_setting);
		hba->crypto_vops = &exynos_ufs_fmp_ops;
	}
	exynos_fmp_sec_cfg(0, 0, init);
}
#else
void exynos_ufs_fmp_config(struct ufs_hba *hba, bool init)
{
}
#endif
