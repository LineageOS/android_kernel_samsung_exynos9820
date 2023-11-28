/*
 * Exynos FMP driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
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
#include <linux/buffer_head.h>
#include <crypto/fmp.h>
#ifdef CONFIG_EXYNOS_FMP_FIPS
#include "fmp/fmp_fips_fops.h"
#include "fmp/fmp_fips_main.h"
#include "fmp/fmp_test.h"
#endif

#ifdef CONFIG_EXYNOS_FMP_FIPS
void fmp_set_bh_compl_handler(struct buffer_head *bh)
{
	bh->b_end_io = end_buffer_read_sync;
}

static const char pass[] = "passed";
static const char fail[] = "failed";

static ssize_t fmp_fips_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct exynos_fmp *fmp = dev_get_drvdata(dev);

	return snprintf(buf, sizeof(pass), "%s\n", fmp->result.overall ? pass : fail);
}

static ssize_t aes_xts_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct exynos_fmp *fmp = dev_get_drvdata(dev);

	return snprintf(buf, sizeof(pass), "%s\n", fmp->result.aes_xts ? pass : fail);
}

static ssize_t aes_cbc_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct exynos_fmp *fmp = dev_get_drvdata(dev);

	return snprintf(buf, sizeof(pass), "%s\n", fmp->result.aes_cbc ? pass : fail);
}

static ssize_t sha256_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct exynos_fmp *fmp = dev_get_drvdata(dev);

	return snprintf(buf, sizeof(pass), "%s\n", fmp->result.sha256 ? pass : fail);
}

static ssize_t hmac_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct exynos_fmp *fmp = dev_get_drvdata(dev);

	return snprintf(buf, sizeof(pass), "%s\n", fmp->result.hmac ? pass : fail);
}

static ssize_t integrity_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct exynos_fmp *fmp = dev_get_drvdata(dev);

	return snprintf(buf, sizeof(pass), "%s\n", fmp->result.integrity ? pass : fail);
}

static ssize_t fmp_fips_run_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	static bool run;

	if (!run) {
		struct exynos_fmp *fmp = dev_get_drvdata(dev);
		exynos_fmp_fips_test(fmp);
		run = 1;
	}
	return count;
}

static DEVICE_ATTR_RO(fmp_fips_status);
static DEVICE_ATTR_RO(aes_xts_status);
static DEVICE_ATTR_RO(aes_cbc_status);
static DEVICE_ATTR_RO(sha256_status);
static DEVICE_ATTR_RO(hmac_status);
static DEVICE_ATTR_RO(integrity_status);
static DEVICE_ATTR_WO(fmp_fips_run);

static struct attribute *fmp_fips_attr[] = {
	&dev_attr_fmp_fips_status.attr,
	&dev_attr_aes_xts_status.attr,
	&dev_attr_aes_cbc_status.attr,
	&dev_attr_sha256_status.attr,
	&dev_attr_hmac_status.attr,
	&dev_attr_integrity_status.attr,
	&dev_attr_fmp_fips_run.attr,
	NULL,
};

static struct attribute_group fmp_fips_attr_group = {
	.name	= "fmp-fips",
	.attrs	= fmp_fips_attr,
};

static int __nocfi fmp_fips_fops_open(struct inode *inode, struct file *file)
{
	return fmp_fips_open(inode, file);
}

static int __nocfi fmp_fips_fops_release(struct inode *inode, struct file *file)
{
	return fmp_fips_release(inode, file);
}

static long __nocfi fmp_fips_fops_ioctl(struct file *file, unsigned int cmd, unsigned long arg_)
{
	return fmp_fips_ioctl(file, cmd, arg_);
}

static long __nocfi fmp_fips_fops_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg_)
{
	return fmp_fips_compat_ioctl(file, cmd, arg_);
}

static const struct file_operations fmp_fips_fops = {
	.owner		= THIS_MODULE,
	.open		= fmp_fips_fops_open,
	.release	= fmp_fips_fops_release,
	.unlocked_ioctl = fmp_fips_fops_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= fmp_fips_fops_compat_ioctl,
#endif
};

int exynos_fmp_fips_register(struct exynos_fmp *fmp)
{
	int ret;

	if (!fmp || !fmp->dev) {
		pr_err("%s: Invalid exynos fmp dev\n", __func__);
		goto err;
	}

	fmp->miscdev.minor = MISC_DYNAMIC_MINOR;
	fmp->miscdev.name = "fmp";
	fmp->miscdev.fops = &fmp_fips_fops;
	ret = misc_register(&fmp->miscdev);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to register misc device. ret(%d)\n",
				__func__, ret);
		goto err;
	}

	ret = sysfs_create_group(&fmp->dev->kobj, &fmp_fips_attr_group);
	if (ret) {
		dev_err(fmp->dev, "%s: Fail to create sysfs. ret(%d)\n",
				__func__, ret);
		goto err_misc;
	}

	dev_info(fmp->dev, "%s: FMP register misc device. ret(%d)\n",
			__func__, ret);
	return 0;

err_misc:
	misc_deregister(&fmp->miscdev);
err:
	return -EINVAL;
}

void exynos_fmp_fips_deregister(struct exynos_fmp *fmp)
{
	sysfs_remove_group(&fmp->dev->kobj, &fmp_fips_attr_group);
	misc_deregister(&fmp->miscdev);
}
#endif

static int exynos_fmp_probe(struct platform_device *pdev)
{
	struct exynos_fmp *fmp_ctx = exynos_fmp_init(pdev);

	if (!fmp_ctx) {
		dev_err(&pdev->dev,
			"%s: Fail to get fmp_ctx\n", __func__);
		return -EINVAL;
	}
	dev_set_drvdata(&pdev->dev, fmp_ctx);

	return 0;
}

static int exynos_fmp_remove(struct platform_device *pdev)
{
	void *drv_data = dev_get_drvdata(&pdev->dev);

	if (!drv_data) {
		pr_err("%s: Fail to get drvdata\n", __func__);
		return 0;
	}
	exynos_fmp_exit(drv_data);
	return 0;
}

static const struct of_device_id exynos_fmp_match[] = {
	{ .compatible = "samsung,exynos-fmp" },
	{},
};

static struct platform_driver exynos_fmp_driver = {
	.driver = {
		   .name = "exynos-fmp",
			.owner = THIS_MODULE,
			.pm = NULL,
		   .of_match_table = exynos_fmp_match,
		   },
	.probe = exynos_fmp_probe,
	.remove = exynos_fmp_remove,
};

static int __init fmp_init(void)
{
	return platform_driver_register(&exynos_fmp_driver);
}
subsys_initcall(fmp_init);

static void __exit fmp_exit(void)
{
	platform_driver_unregister(&exynos_fmp_driver);
}
module_exit(fmp_exit);

MODULE_DESCRIPTION("FMP driver");
MODULE_AUTHOR("Boojin Kim <boojin.kim@samsung.com>");
MODULE_LICENSE("GPL");
