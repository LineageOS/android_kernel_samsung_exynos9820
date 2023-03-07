/*
 * Samsung Exynos SoC series Sensor driver
 *
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

#include "fimc-is-config.h"
#include "fimc-is-vender-caminfo.h"
#include "fimc-is-vender-specific.h"
#include "fimc-is-sec-define.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-sysfs.h"

static int fimc_is_vender_caminfo_open(struct inode *inode, struct file *file)
{
	fimc_is_vender_caminfo *p_vender_caminfo;

	p_vender_caminfo = vzalloc(sizeof(fimc_is_vender_caminfo));
	if(p_vender_caminfo == NULL) {
		err("failed to allocate memory for fimc_is_vender_caminfo");
		return -ENOMEM;
	}

	mutex_init(&p_vender_caminfo->mlock);

	file->private_data = p_vender_caminfo;

	return 0;
}

static int fimc_is_vender_caminfo_release(struct inode *inode, struct file *file)
{
	fimc_is_vender_caminfo *p_vender_caminfo = (fimc_is_vender_caminfo *)file->private_data;

	if (p_vender_caminfo)
		vfree(p_vender_caminfo);

	return 0;
}

static int fimc_is_vender_caminfo_cmd_get_factory_supported_id(void __user *user_data)
{
	int i;
	caminfo_supported_list supported_list;
	struct fimc_is_common_cam_info *common_camera_infos;

	fimc_is_get_common_cam_info(&common_camera_infos);
	if (!common_camera_infos) {
		err("common_camera_infos is NULL");
		return -EFAULT;
	}

	supported_list.size = common_camera_infos->max_supported_camera;

	for (i = 0; i < supported_list.size; i++) {
		supported_list.data[i] = common_camera_infos->supported_camera_ids[i];
	}

	if (copy_to_user(user_data, (void *)&supported_list, sizeof(caminfo_supported_list))) {
		err("%s : failed to copy data to user", __func__);
		return -EINVAL;
	}

	return 0;
}

static int fimc_is_vender_caminfo_cmd_get_rom_data_by_position(void __user *user_data)
{
	int ret = 0;
	int rom_id;
	char *cal_buf;
	struct fimc_is_rom_info *finfo;
	caminfo_romdata romdata;

	if (copy_from_user((void *)&romdata, user_data, sizeof(caminfo_romdata))) {
		err("%s : failed to copy data from user", __func__);
		ret = -EINVAL;
		goto EXIT;
	}

	rom_id = fimc_is_vendor_get_rom_id_from_position(romdata.cam_position);

	if (rom_id == ROM_ID_NOTHING) {
		err("%s : invalid camera position (%d)", __func__, romdata.cam_position);
		ret = -EINVAL;
		goto EXIT;
	}

	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);
	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	romdata.rom_size = finfo->rom_size;

	if (copy_to_user(user_data, &romdata, sizeof(caminfo_romdata))) {
		err("%s : failed to copy data to user", __func__);
		ret = -EINVAL;
		goto EXIT;
	}

	if (romdata.buf_size >= romdata.rom_size) {
		if (copy_to_user(romdata.buf, cal_buf, romdata.rom_size)) {
			err("%s : failed to copy data to user", __func__);
			ret = -EINVAL;
			goto EXIT;
		}
	} else {
		err("%s : wrong buf size : buf size must be bigger than cal buf size", __func__);
		ret = -EINVAL;
	}

EXIT:
	return ret;
}


static int fimc_is_vender_caminfo_compat_cmd_get_rom_data_by_position(compat_uptr_t user_data)
{
	int ret = 0;
	int rom_id;
	char *cal_buf;
	struct fimc_is_rom_info *finfo;
	compat_caminfo_romdata romdata;

	if (copy_from_user((void *)&romdata, compat_ptr(user_data), sizeof(compat_caminfo_romdata))) {
		err("%s : failed to copy data from user", __func__);
		ret = -EINVAL;
		goto EXIT;
	}
	rom_id = fimc_is_vendor_get_rom_id_from_position(romdata.cam_position);
	info("%s : rom_id:%d", __func__, rom_id);

	if (rom_id == ROM_ID_NOTHING) {
		err("%s : invalid camera position (%d)", __func__, romdata.cam_position);
		ret = -EINVAL;
		goto EXIT;
	}

	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);
	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	romdata.rom_size = finfo->rom_size;

	if (copy_to_user(compat_ptr(user_data), &romdata, sizeof(compat_caminfo_romdata))) {
		err("%s : failed to copy data to user", __func__);
		ret = -EINVAL;
		goto EXIT;
	}

	if (romdata.buf_size >= romdata.rom_size) {
		if (copy_to_user(compat_ptr(romdata.buf), cal_buf, romdata.rom_size)) {
			err("%s : failed to copy data to user", __func__);
			ret = -EINVAL;
			goto EXIT;
		}
	} else {
		err("%s : wrong buf size : buf size must be bigger than cal buf size", __func__);
		ret = -EINVAL;
	}

EXIT:
	return ret;
}

static long fimc_is_vender_caminfo_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	fimc_is_vender_caminfo *p_vender_caminfo;
	caminfo_ioctl_cmd ioctl_cmd;

	BUG_ON(!file->private_data);

	p_vender_caminfo = (fimc_is_vender_caminfo *)file->private_data;

	mutex_lock(&p_vender_caminfo->mlock);

	if (cmd != FIMC_IS_CAMINFO_IOCTL_COMMAND) {
		err("%s : not support cmd:%x, arg:%x", __func__, cmd, arg);
		ret = -EINVAL;
		goto EXIT;
	}

	if (copy_from_user((void *)&ioctl_cmd, (const void *)arg, sizeof(caminfo_ioctl_cmd))) {
		err("%s : failed to copy data from user", __func__);
		ret = -EINVAL;
		goto EXIT;
	}
	info("%s : cmd number:%u, arg:%x", __func__, ioctl_cmd.cmd, arg);

	switch (ioctl_cmd.cmd) {
		case CAMINFO_CMD_ID_GET_FACTORY_SUPPORTED_ID:
			ret = fimc_is_vender_caminfo_cmd_get_factory_supported_id(ioctl_cmd.data);
			break;
		case CAMINFO_CMD_ID_GET_ROM_DATA_BY_POSITION:
			ret = fimc_is_vender_caminfo_cmd_get_rom_data_by_position(ioctl_cmd.data);
			break;
		default:
			err("%s : not support cmd number:%u, arg:%x", __func__, ioctl_cmd.cmd, arg);
			ret = -EINVAL;
			break;
	}

EXIT:
	mutex_unlock(&p_vender_caminfo->mlock);

	return ret;
}

static long fimc_is_vender_caminfo_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	fimc_is_vender_caminfo *p_vender_caminfo;
	compat_caminfo_ioctl_cmd compat_ioctl_cmd;

	BUG_ON(!file->private_data);

	p_vender_caminfo = (fimc_is_vender_caminfo *)file->private_data;

	mutex_lock(&p_vender_caminfo->mlock);

	if (cmd != FIMC_IS_CAMINFO_COMPAT_IOCTL_COMMAND) {
		err("%s : not support cmd:%x, arg:%x", __func__, cmd, arg);
		ret = -EINVAL;
		goto EXIT;
	}

	if (compat_ptr(arg) == NULL)
		return ret;

	if (copy_from_user((void *)&compat_ioctl_cmd, compat_ptr(arg), sizeof(compat_ioctl_cmd))) {
		err("%s : failed to copy data from user", __func__);
		ret = -EINVAL;
		goto EXIT;
	}
	info("%s : cmd number:%u, arg:%x", __func__, compat_ioctl_cmd.cmd, compat_ptr(arg));

	switch (compat_ioctl_cmd.cmd) {
		case CAMINFO_CMD_ID_GET_FACTORY_SUPPORTED_ID:
			ret = fimc_is_vender_caminfo_cmd_get_factory_supported_id(compat_ptr(compat_ioctl_cmd.data));
			break;
		case CAMINFO_CMD_ID_GET_ROM_DATA_BY_POSITION:
			ret = fimc_is_vender_caminfo_compat_cmd_get_rom_data_by_position(compat_ioctl_cmd.data);
			break;
		default:
			err("%s : not support cmd number:%u, arg:%x", __func__, compat_ioctl_cmd.cmd, compat_ptr(arg));
			ret = -EINVAL;
			break;
	}

EXIT:
	mutex_unlock(&p_vender_caminfo->mlock);
	return ret;
}

static struct file_operations fimc_is_vender_caminfo_fops =
{
	.owner = THIS_MODULE,
	.open = fimc_is_vender_caminfo_open,
	.release = fimc_is_vender_caminfo_release,
	.unlocked_ioctl = fimc_is_vender_caminfo_ioctl,
	.compat_ioctl = fimc_is_vender_caminfo_compat_ioctl,
};

struct miscdevice fimc_is_vender_caminfo_driver =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = "caminfo",
	.fops = &fimc_is_vender_caminfo_fops,
};

#ifndef MODULE
static int fimc_is_vender_caminfo_init(void)
{
	info("%s\n", __func__);

	return misc_register(&fimc_is_vender_caminfo_driver);
}

static void fimc_is_vender_caminfo_exit(void)
{
	info("%s\n", __func__);

	misc_deregister(&fimc_is_vender_caminfo_driver);

}

module_init(fimc_is_vender_caminfo_init);
module_exit(fimc_is_vender_caminfo_exit);
#endif

MODULE_DESCRIPTION("Exynos Caminfo driver");
MODULE_LICENSE("GPL v2");
