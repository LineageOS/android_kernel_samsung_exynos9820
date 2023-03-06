/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3fa9/s6e3fa9_m62_panel_aod_dimming.h
 *
 * Header file for S6E3FA9 Dimming Driver
 *
 * Copyright (c) 2017 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3FA9_M62_PANEL_AOD_DIMMING_H__
#define __S6E3FA9_M62_PANEL_AOD_DIMMING_H__
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3fa9_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3FA9
 * PANEL : M62
 */
static unsigned int m62_aod_brt_tbl[S6E3FA9_M62_AOD_NR_LUMINANCE] = {
	BRT_LT(13), BRT_LT(32), BRT_LT(55), BRT(255),
};

static unsigned int m62_aod_lum_tbl[S6E3FA9_M62_AOD_NR_LUMINANCE] = {
	2, 10, 30, 60,
};

static struct brightness_table s6e3fa9_m62_panel_aod_brightness_table = {
	.brt = m62_aod_brt_tbl,
	.sz_brt = ARRAY_SIZE(m62_aod_brt_tbl),
	.lum = m62_aod_lum_tbl,
	.sz_lum = ARRAY_SIZE(m62_aod_lum_tbl),
	.brt_to_step = m62_aod_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(m62_aod_brt_tbl),
};

static struct panel_dimming_info s6e3fa9_m62_panel_aod_dimming_info = {
	.name = "s6e3fa9_m62_aod",
	.target_luminance = S6E3FA9_M62_AOD_TARGET_LUMINANCE,
	.nr_luminance = S6E3FA9_M62_AOD_NR_LUMINANCE,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3fa9_m62_panel_aod_brightness_table,
};
#endif /* __S6E3FA9_M62_PANEL_AOD_DIMMING_H__ */
