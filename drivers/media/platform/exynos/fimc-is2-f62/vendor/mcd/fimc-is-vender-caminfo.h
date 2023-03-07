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

#ifndef FIMC_IS_VENDER_CAMINFO_H
#define FIMC_IS_VENDER_CAMINFO_H

#include <linux/vmalloc.h>
#include <asm/compat.h>
#include "fimc-is-core.h"

#ifndef _LINUX_TYPES_H
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
#endif

#define FIMC_IS_CAMINFO_IOCTL_MAGIC 			0xFB
#define CAM_MAX_SUPPORTED_LIST			20

#define FIMC_IS_CAMINFO_IOCTL_COMMAND		_IOWR(FIMC_IS_CAMINFO_IOCTL_MAGIC, 0x01, caminfo_ioctl_cmd *)
#define FIMC_IS_CAMINFO_COMPAT_IOCTL_COMMAND	_IOWR(FIMC_IS_CAMINFO_IOCTL_MAGIC, 0x01, compat_uptr_t)

typedef struct
{
	uint32_t cmd;
	void *data;
} caminfo_ioctl_cmd;

typedef struct
{
	uint32_t cmd;
	compat_uptr_t data; // void * (32-bit)
} compat_caminfo_ioctl_cmd;

enum caminfo_cmd_id
{
	CAMINFO_CMD_ID_GET_FACTORY_SUPPORTED_ID = 0,
	CAMINFO_CMD_ID_GET_ROM_DATA_BY_POSITION = 1,
};

typedef struct
{
	uint32_t cam_position;
	uint32_t buf_size;
	uint8_t *buf;
	uint32_t rom_size;
} caminfo_romdata;

typedef struct
{
	uint32_t cam_position;
	uint32_t buf_size;
	compat_uptr_t buf; // uint8_t * (32-bit)
	uint32_t rom_size;
} compat_caminfo_romdata;

typedef struct
{
	uint32_t size;
	uint32_t data[CAM_MAX_SUPPORTED_LIST];
} caminfo_supported_list;

typedef struct
{
	struct mutex mlock;
} fimc_is_vender_caminfo;

#endif /* FIMC_IS_VENDER_CAMINFO_H */
