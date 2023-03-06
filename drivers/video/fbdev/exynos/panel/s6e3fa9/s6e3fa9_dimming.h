/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3fa9/s6e3fa9_dimming.h
 *
 * Header file for S6E3FA9 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3FA9_DIMMING_H__
#define __S6E3FA9_DIMMING_H__
#include <linux/types.h>
#include <linux/kernel.h>
#include "../dimming.h"
#include "s6e3fa9.h"

#define S6E3FA9_NR_TP (12)

#define S6E3FA9_CANVAS1_NR_LUMINANCE (256)
#define S6E3FA9_CANVAS1_TARGET_LUMINANCE (420)
#define S6E3FA9_CANVAS1_NR_HBM_LUMINANCE (170)
#define S6E3FA9_CANVAS1_TARGET_HBM_LUMINANCE (700)

#ifdef CONFIG_SUPPORT_AOD_BL
#define S6E3FA9_CANVAS1_AOD_NR_LUMINANCE (4)
#define S6E3FA9_CANVAS1_AOD_TARGET_LUMINANCE (60)
#endif

#define S6E3FA9_CANVAS1_TOTAL_NR_LUMINANCE (S6E3FA9_CANVAS1_NR_LUMINANCE + S6E3FA9_CANVAS1_NR_HBM_LUMINANCE)

#define S6E3FA9_M62_NR_LUMINANCE (256)
#define S6E3FA9_M62_TARGET_LUMINANCE (420)
#define S6E3FA9_M62_NR_HBM_LUMINANCE (170)
#define S6E3FA9_M62_TARGET_HBM_LUMINANCE (700)

#ifdef CONFIG_SUPPORT_AOD_BL
#define S6E3FA9_M62_AOD_NR_LUMINANCE (4)
#define S6E3FA9_M62_AOD_TARGET_LUMINANCE (60)
#endif

#define S6E3FA9_M62_TOTAL_NR_LUMINANCE (S6E3FA9_M62_NR_LUMINANCE + S6E3FA9_M62_NR_HBM_LUMINANCE)

#endif /* __S6E3FA9_DIMMING_H__ */
