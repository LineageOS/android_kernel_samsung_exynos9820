/*
 * Exynos FMP test driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/miscdevice.h>
#include <linux/crypto.h>
#include <linux/buffer_head.h>
#include <linux/genhd.h>
#include <linux/delay.h>

#include <crypto/authenc.h>
#include <crypto/fmp.h>

#if defined(CONFIG_EXYNOS_FMP_FIPS_FUNC_TEST)
#include "fmp_fips_func_test.h"
#endif
#include "fmp_fips_main.h"
#include "fmp_fips_fops.h"
#include "fmp_fips_selftest.h"
#include "fmp_fips_integrity.h"
#include "fmp_test.h"

enum fips_state {
	FMP_FIPS_INIT_STATE,
	FMP_FIPS_PROGRESS_STATE,
	FMP_FIPS_ERR_STATE,
	FMP_FIPS_SUCCESS_STATE
};

static enum fips_state fmp_fips_state = FMP_FIPS_INIT_STATE;

bool in_fmp_fips_err(void)
{
	if (fmp_fips_state == FMP_FIPS_INIT_STATE ||
	    fmp_fips_state == FMP_FIPS_ERR_STATE)
		return true;
	return false;
}

bool in_fmp_fips_init(void)
{
	if (fmp_fips_state == FMP_FIPS_INIT_STATE)
		return true;
	return false;
}

static void set_fmp_fips_state(uint32_t val)
{
	fmp_fips_state = val;
}

void exynos_fmp_fips_test(struct exynos_fmp *fmp)
{
#if defined(CONFIG_EXYNOS_FMP_FIPS_FUNC_TEST)
	exynos_fmp_func_test_KAT_case(fmp);
#endif
	exynos_fmp_fips_init(fmp);
}

int exynos_fmp_fips_init(struct exynos_fmp *fmp)
{
	int ret = 0;

	set_fmp_fips_state(FMP_FIPS_PROGRESS_STATE);
	dev_info(fmp->dev, "%s: Started!!\n", __func__);

	if (!fmp || !fmp->dev) {
		pr_err("%s: Invalid exynos fmp dev\n", __func__);
		goto err;
	}

	fmp->test_data = fmp_test_init(fmp);
	if (!fmp->test_data) {
		dev_err(fmp->dev,
			"%s: fails to initialize fips test.\n", __func__);
		goto err;
	}

	ret = do_fmp_selftest(fmp);
	if (ret) {
		dev_err(fmp->dev, "%s: self-tests for FMP failed\n", __func__);
		goto err_data;
	} else {
		dev_info(fmp->dev, "%s: self-tests for FMP passed\n", __func__);
	}

	set_fmp_fips_state(FMP_FIPS_SUCCESS_STATE);
	fmp->result.overall = 1;
	fmp_test_exit(fmp->test_data);
	return 0;
err_data:
#if defined(CONFIG_NODE_FOR_SELFTEST_FAIL)
	set_fmp_fips_state(FMP_FIPS_ERR_STATE);
	fmp->result.overall = 0;
	fmp_test_exit(fmp->test_data);
	return 0;
#else
	// return 0 if KAT function test mode
	if (fmp->test_vops) {
		set_fmp_fips_state(FMP_FIPS_ERR_STATE);
		fmp->result.overall = 0;
		fmp_test_exit(fmp->test_data);
		return 0;
	}
	else {
		panic("%s: Panic due to FMP self test for FIPS KAT", __func__);
	}
#endif
err:
	set_fmp_fips_state(FMP_FIPS_ERR_STATE);
	return -EINVAL;
}

int exynos_fmp_fips_integrity(struct exynos_fmp *fmp)
{
	int ret = 0;

	dev_info(fmp->dev, "%s: Started!!\n", __func__);

	if (!fmp || !fmp->dev) {
		pr_err("%s: Invalid exynos fmp dev\n", __func__);
		goto err;
	}

	ret = do_fmp_integrity_check(fmp);
	if (ret) {
		dev_err(fmp->dev, "%s: integrity check for FMP failed\n", __func__);
		fmp->result.integrity = 0;
		goto err_data;
	} else {
		dev_info(fmp->dev, "%s: integrity check for FMP passed\n", __func__);
		fmp->result.integrity = 1;
	}

	set_fmp_fips_state(FMP_FIPS_PROGRESS_STATE);
	return 0;

err_data:
#if defined(CONFIG_NODE_FOR_SELFTEST_FAIL)
	set_fmp_fips_state(FMP_FIPS_ERR_STATE);
	fmp->result.overall = 0;
	fmp_test_exit(fmp->test_data);
	return 0;
#else
	panic("%s: Panic due to FMP integrity", __func__);
#endif

err:
	set_fmp_fips_state(FMP_FIPS_ERR_STATE);
	return -EINVAL;
}
