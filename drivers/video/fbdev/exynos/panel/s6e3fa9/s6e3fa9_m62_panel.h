/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3fa9/s6e3fa9_m62_panel.h
 *
 * Header file for S6E3FA9 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3FA9_M62_A3_S1_PANEL_H__
#define __S6E3FA9_M62_A3_S1_PANEL_H__

#include "../panel.h"
#include "../panel_drv.h"
#include "s6e3fa9.h"
#include "s6e3fa9_dimming.h"
#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
#include "s6e3fa9_m62_panel_mdnie.h"
#endif
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
#include "s6e3fa9_m62_panel_copr.h"
#endif
#ifdef CONFIG_SUPPORT_DDI_FLASH
#include "s6e3fa9_m62_panel_poc.h"
#endif
#include "s6e3fa9_m62_panel_dimming.h"
#ifdef CONFIG_SUPPORT_AOD_BL
#include "s6e3fa9_m62_panel_aod_dimming.h"
#endif
#ifdef CONFIG_EXTEND_LIVE_CLOCK
#include "s6e3fa9_m62_aod_panel.h"
#include "../aod/aod_drv.h"
#endif

#ifdef CONFIG_SUPPORT_DISPLAY_PROFILER
#include "s6e3fa9_profiler_panel.h"
#include "../display_profiler/display_profiler.h"
#endif
#ifdef CONFIG_SUPPORT_POC_SPI
#include "../spi/w25q80_panel_spi.h"
#include "../spi/mx25r4035_panel_spi.h"
#endif
#ifdef CONFIG_DYNAMIC_FREQ
#include "s6e3fa9_m62_df_tbl.h"
#endif

#undef __pn_name__
#define __pn_name__	m62

#undef __PN_NAME__
#define __PN_NAME__	M62

/* ===================================================================================== */
/* ============================= [S6E3FA9 READ INFO TABLE] ============================= */
/* ===================================================================================== */
/* <READINFO TABLE> IS DEPENDENT ON LDI. IF YOU NEED, DEFINE PANEL's RESOURCE TABLE */


/* ===================================================================================== */
/* ============================= [S6E3FA9 RESOURCE TABLE] ============================== */
/* ===================================================================================== */
/* <RESOURCE TABLE> IS DEPENDENT ON LDI. IF YOU NEED, DEFINE PANEL's RESOURCE TABLE */


/* ===================================================================================== */
/* ============================== [S6E3FA9 MAPPING TABLE] ============================== */
/* ===================================================================================== */

static u8 m62_brt_table[S6E3FA9_TOTAL_STEP][2] = {
	/* Normal 5x51+1 */
	{ 0x00, 0x03},	{ 0x00, 0x06},	{ 0x00, 0x09},	{ 0x00, 0x0C},	{ 0x00, 0x0F},
	{ 0x00, 0x12},	{ 0x00, 0x15},	{ 0x00, 0x18},	{ 0x00, 0x1B},	{ 0x00, 0x1E},
	{ 0x00, 0x21},	{ 0x00, 0x24},	{ 0x00, 0x27},	{ 0x00, 0x2A},	{ 0x00, 0x2D},
	{ 0x00, 0x30},	{ 0x00, 0x34},	{ 0x00, 0x37},	{ 0x00, 0x3B},	{ 0x00, 0x3E},
	{ 0x00, 0x42},	{ 0x00, 0x46},	{ 0x00, 0x49},	{ 0x00, 0x4D},	{ 0x00, 0x50},
	{ 0x00, 0x54},	{ 0x00, 0x58},	{ 0x00, 0x5B},	{ 0x00, 0x5F},	{ 0x00, 0x62},
	{ 0x00, 0x66},	{ 0x00, 0x6A},	{ 0x00, 0x6D},	{ 0x00, 0x71},	{ 0x00, 0x74},
	{ 0x00, 0x78},	{ 0x00, 0x7C},	{ 0x00, 0x7F},	{ 0x00, 0x83},	{ 0x00, 0x86},
	{ 0x00, 0x8A},	{ 0x00, 0x8E},	{ 0x00, 0x91},	{ 0x00, 0x95},	{ 0x00, 0x98},
	{ 0x00, 0x9C},	{ 0x00, 0xA0},	{ 0x00, 0xA3},	{ 0x00, 0xA7},	{ 0x00, 0xAA},
	{ 0x00, 0xAE},	{ 0x00, 0xB2},	{ 0x00, 0xB5},	{ 0x00, 0xB9},	{ 0x00, 0xBC},
	{ 0x00, 0xC0},	{ 0x00, 0xC4},	{ 0x00, 0xC7},	{ 0x00, 0xCB},	{ 0x00, 0xCE},
	{ 0x00, 0xD2},	{ 0x00, 0xD6},	{ 0x00, 0xD9},	{ 0x00, 0xDD},	{ 0x00, 0xE0},
	{ 0x00, 0xE4},	{ 0x00, 0xE8},	{ 0x00, 0xEB},	{ 0x00, 0xEF},	{ 0x00, 0xF2},
	{ 0x00, 0xF6},	{ 0x00, 0xFA},	{ 0x00, 0xFD},	{ 0x01, 0x01},	{ 0x01, 0x04},
	{ 0x01, 0x08},	{ 0x01, 0x0C},	{ 0x01, 0x0F},	{ 0x01, 0x13},	{ 0x01, 0x16},
	{ 0x01, 0x1A},	{ 0x01, 0x1E},	{ 0x01, 0x21},	{ 0x01, 0x25},	{ 0x01, 0x29},
	{ 0x01, 0x2C},	{ 0x01, 0x30},	{ 0x01, 0x33},	{ 0x01, 0x37},	{ 0x01, 0x3B},
	{ 0x01, 0x3E},	{ 0x01, 0x42},	{ 0x01, 0x45},	{ 0x01, 0x49},	{ 0x01, 0x4D},
	{ 0x01, 0x50},	{ 0x01, 0x54},	{ 0x01, 0x57},	{ 0x01, 0x5B},	{ 0x01, 0x5F},
	{ 0x01, 0x62},	{ 0x01, 0x66},	{ 0x01, 0x69},	{ 0x01, 0x6D},	{ 0x01, 0x71},
	{ 0x01, 0x74},	{ 0x01, 0x78},	{ 0x01, 0x7B},	{ 0x01, 0x7F},	{ 0x01, 0x83},
	{ 0x01, 0x86},	{ 0x01, 0x8A},	{ 0x01, 0x8D},	{ 0x01, 0x91},	{ 0x01, 0x95},
	{ 0x01, 0x98},	{ 0x01, 0x9C},	{ 0x01, 0x9F},	{ 0x01, 0xA3},	{ 0x01, 0xA7},
	{ 0x01, 0xAA},	{ 0x01, 0xAE},	{ 0x01, 0xB1},	{ 0x01, 0xB5},	{ 0x01, 0xB9},
	{ 0x01, 0xBC},	{ 0x01, 0xC0},	{ 0x01, 0xC3},	{ 0x01, 0xC7},	{ 0x01, 0xCB},
	{ 0x01, 0xD0},	{ 0x01, 0xD4},	{ 0x01, 0xD9},	{ 0x01, 0xDD},	{ 0x01, 0xE2},
	{ 0x01, 0xE6},	{ 0x01, 0xEB},	{ 0x01, 0xEF},	{ 0x01, 0xF4},	{ 0x01, 0xF8},
	{ 0x01, 0xFD},	{ 0x02, 0x01},	{ 0x02, 0x06},	{ 0x02, 0x0A},	{ 0x02, 0x0F},
	{ 0x02, 0x13},	{ 0x02, 0x17},	{ 0x02, 0x1C},	{ 0x02, 0x20},	{ 0x02, 0x25},
	{ 0x02, 0x29},	{ 0x02, 0x2E},	{ 0x02, 0x32},	{ 0x02, 0x37},	{ 0x02, 0x3B},
	{ 0x02, 0x40},	{ 0x02, 0x44},	{ 0x02, 0x49},	{ 0x02, 0x4D},	{ 0x02, 0x52},
	{ 0x02, 0x56},	{ 0x02, 0x5B},	{ 0x02, 0x5F},	{ 0x02, 0x64},	{ 0x02, 0x68},
	{ 0x02, 0x6C},	{ 0x02, 0x71},	{ 0x02, 0x75},	{ 0x02, 0x7A},	{ 0x02, 0x7E},
	{ 0x02, 0x83},	{ 0x02, 0x87},	{ 0x02, 0x8C},	{ 0x02, 0x90},	{ 0x02, 0x95},
	{ 0x02, 0x99},	{ 0x02, 0x9E},	{ 0x02, 0xA2},	{ 0x02, 0xA7},	{ 0x02, 0xAB},
	{ 0x02, 0xB0},	{ 0x02, 0xB4},	{ 0x02, 0xB8},	{ 0x02, 0xBD},	{ 0x02, 0xC1},
	{ 0x02, 0xC6},	{ 0x02, 0xCA},	{ 0x02, 0xCF},	{ 0x02, 0xD3},	{ 0x02, 0xD8},
	{ 0x02, 0xDC},	{ 0x02, 0xE1},	{ 0x02, 0xE5},	{ 0x02, 0xEA},	{ 0x02, 0xEE},
	{ 0x02, 0xF3},	{ 0x02, 0xF7},	{ 0x02, 0xFC},	{ 0x03, 0x00},	{ 0x03, 0x05},
	{ 0x03, 0x09},	{ 0x03, 0x0D},	{ 0x03, 0x12},	{ 0x03, 0x16},	{ 0x03, 0x1B},
	{ 0x03, 0x1F},	{ 0x03, 0x24},	{ 0x03, 0x28},	{ 0x03, 0x2D},	{ 0x03, 0x31},
	{ 0x03, 0x36},	{ 0x03, 0x3A},	{ 0x03, 0x3F},	{ 0x03, 0x43},	{ 0x03, 0x48},
	{ 0x03, 0x4C},	{ 0x03, 0x51},	{ 0x03, 0x55},	{ 0x03, 0x5A},	{ 0x03, 0x5E},
	{ 0x03, 0x62},	{ 0x03, 0x67},	{ 0x03, 0x6B},	{ 0x03, 0x70},	{ 0x03, 0x74},
	{ 0x03, 0x79},	{ 0x03, 0x7D},	{ 0x03, 0x82},	{ 0x03, 0x86},	{ 0x03, 0x8B},
	{ 0x03, 0x8F},	{ 0x03, 0x94},	{ 0x03, 0x98},	{ 0x03, 0x9D},	{ 0x03, 0xA1},
	{ 0x03, 0xA6},	{ 0x03, 0xAA},	{ 0x03, 0xAE},	{ 0x03, 0xB3},	{ 0x03, 0xB7},
	{ 0x03, 0xBC},	{ 0x03, 0xC0},	{ 0x03, 0xC5},	{ 0x03, 0xC9},	{ 0x03, 0xCE},
	{ 0x03, 0xD2},	{ 0x03, 0xD7},	{ 0x03, 0xDB},	{ 0x03, 0xE0},	{ 0x03, 0xE4},
	{ 0x03, 0xE9},	{ 0x03, 0xED},	{ 0x03, 0xF2},	{ 0x03, 0xF6},	{ 0x03, 0xFB},
	{ 0x03, 0xFF},

	/* HBM 5x34 */
	{ 0x00, 0x03},	{ 0x00, 0x05},	{ 0x00, 0x07},	{ 0x00, 0x09},	{ 0x00, 0x0C},
	{ 0x00, 0x0E},	{ 0x00, 0x11},	{ 0x00, 0x13},	{ 0x00, 0x15},	{ 0x00, 0x18},
	{ 0x00, 0x1A},	{ 0x00, 0x1D},	{ 0x00, 0x1F},	{ 0x00, 0x21},	{ 0x00, 0x24},
	{ 0x00, 0x26},	{ 0x00, 0x28},	{ 0x00, 0x2B},	{ 0x00, 0x2D},	{ 0x00, 0x30},
	{ 0x00, 0x32},	{ 0x00, 0x34},	{ 0x00, 0x37},	{ 0x00, 0x39},	{ 0x00, 0x3B},
	{ 0x00, 0x3E},	{ 0x00, 0x40},	{ 0x00, 0x43},	{ 0x00, 0x45},	{ 0x00, 0x47},
	{ 0x00, 0x4A},	{ 0x00, 0x4C},	{ 0x00, 0x4E},	{ 0x00, 0x51},	{ 0x00, 0x53},
	{ 0x00, 0x56},	{ 0x00, 0x58},	{ 0x00, 0x5A},	{ 0x00, 0x5D},	{ 0x00, 0x5F},
	{ 0x00, 0x61},	{ 0x00, 0x64},	{ 0x00, 0x66},	{ 0x00, 0x69},	{ 0x00, 0x6B},
	{ 0x00, 0x6D},	{ 0x00, 0x70},	{ 0x00, 0x72},	{ 0x00, 0x74},	{ 0x00, 0x77},
	{ 0x00, 0x79},	{ 0x00, 0x7C},	{ 0x00, 0x7E},	{ 0x00, 0x80},	{ 0x00, 0x83},
	{ 0x00, 0x85},	{ 0x00, 0x87},	{ 0x00, 0x8A},	{ 0x00, 0x8C},	{ 0x00, 0x8F},
	{ 0x00, 0x91},	{ 0x00, 0x93},	{ 0x00, 0x96},	{ 0x00, 0x98},	{ 0x00, 0x9A},
	{ 0x00, 0x9D},	{ 0x00, 0x9F},	{ 0x00, 0xA2},	{ 0x00, 0xA4},	{ 0x00, 0xA6},
	{ 0x00, 0xA9},	{ 0x00, 0xAB},	{ 0x00, 0xAD},	{ 0x00, 0xB0},	{ 0x00, 0xB2},
	{ 0x00, 0xB5},	{ 0x00, 0xB7},	{ 0x00, 0xB9},	{ 0x00, 0xBC},	{ 0x00, 0xBE},
	{ 0x00, 0xC0},	{ 0x00, 0xC3},	{ 0x00, 0xC5},	{ 0x00, 0xC8},	{ 0x00, 0xCA},
	{ 0x00, 0xCC},	{ 0x00, 0xCF},	{ 0x00, 0xD1},	{ 0x00, 0xD3},	{ 0x00, 0xD6},
	{ 0x00, 0xD8},	{ 0x00, 0xDB},	{ 0x00, 0xDD},	{ 0x00, 0xDF},	{ 0x00, 0xE2},
	{ 0x00, 0xE4},	{ 0x00, 0xE7},	{ 0x00, 0xE9},	{ 0x00, 0xEB},	{ 0x00, 0xEE},
	{ 0x00, 0xF0},	{ 0x00, 0xF2},	{ 0x00, 0xF5},	{ 0x00, 0xF7},	{ 0x00, 0xFA},
	{ 0x00, 0xFC},	{ 0x00, 0xFE},	{ 0x01, 0x01},	{ 0x01, 0x03},	{ 0x01, 0x05},
	{ 0x01, 0x08},	{ 0x01, 0x0A},	{ 0x01, 0x0D},	{ 0x01, 0x0F},	{ 0x01, 0x11},
	{ 0x01, 0x14},	{ 0x01, 0x16},	{ 0x01, 0x18},	{ 0x01, 0x1B},	{ 0x01, 0x1D},
	{ 0x01, 0x20},	{ 0x01, 0x22},	{ 0x01, 0x24},	{ 0x01, 0x27},	{ 0x01, 0x29},
	{ 0x01, 0x2B},	{ 0x01, 0x2E},	{ 0x01, 0x30},	{ 0x01, 0x33},	{ 0x01, 0x35},
	{ 0x01, 0x37},	{ 0x01, 0x3A},	{ 0x01, 0x3C},	{ 0x01, 0x3E},	{ 0x01, 0x41},
	{ 0x01, 0x43},	{ 0x01, 0x46},	{ 0x01, 0x48},	{ 0x01, 0x4A},	{ 0x01, 0x4D},
	{ 0x01, 0x4F},	{ 0x01, 0x51},	{ 0x01, 0x54},	{ 0x01, 0x56},	{ 0x01, 0x59},
	{ 0x01, 0x5B},	{ 0x01, 0x5D},	{ 0x01, 0x60},	{ 0x01, 0x62},	{ 0x01, 0x64},
	{ 0x01, 0x67},	{ 0x01, 0x69},	{ 0x01, 0x6C},	{ 0x01, 0x6E},	{ 0x01, 0x70},
	{ 0x01, 0x73},	{ 0x01, 0x75},	{ 0x01, 0x77},	{ 0x01, 0x7A},	{ 0x01, 0x7C},
	{ 0x01, 0x7F},	{ 0x01, 0x81},	{ 0x01, 0x83},	{ 0x01, 0x86},	{ 0x01, 0x88},
	{ 0x01, 0x8A},	{ 0x01, 0x8D},	{ 0x01, 0x8F},	{ 0x01, 0x92},	{ 0x01, 0x94},
};

static u8 m62_elvss_table[S6E3FA9_TOTAL_STEP][1] = {
	/* Normal  8x32 */
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	/* HBM 8x21+2 */
	{ 0x1E }, { 0x1D },	{ 0x1D }, { 0x1D },	{ 0x1D }, { 0x1D }, { 0x1D }, { 0x1D },
	{ 0x1D }, { 0x1D }, { 0x1D }, { 0x1C }, { 0x1C }, { 0x1C }, { 0x1C }, { 0x1C },
	{ 0x1C }, { 0x1C },	{ 0x1C }, { 0x1C },	{ 0x1C }, { 0x1C }, { 0x1C }, { 0x1B },
	{ 0x1B }, { 0x1B },	{ 0x1B }, { 0x1B },	{ 0x1B }, { 0x1B },	{ 0x1B }, { 0x1B },
	{ 0x1B }, { 0x1B }, { 0x1B }, { 0x1A }, { 0x1A }, { 0x1A },	{ 0x1A }, { 0x1A },
	{ 0x1A }, { 0x1A }, { 0x1A }, { 0x1A },	{ 0x1A }, { 0x1A },	{ 0x1A }, { 0x1A },
	{ 0x19 }, { 0x19 }, { 0x19 }, { 0x19 }, { 0x19 }, { 0x19 },	{ 0x19 }, { 0x19 },
	{ 0x19 }, { 0x19 }, { 0x19 }, { 0x19 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x18 },
	{ 0x18 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x18 },
	{ 0x17 }, { 0x17 }, { 0x17 }, { 0x17 }, { 0x17 }, { 0x17 },	{ 0x17 }, { 0x17 },
	{ 0x17 }, { 0x17 }, { 0x17 }, { 0x17 }, { 0x16 }, { 0x16 }, { 0x16 }, { 0x16 },
	{ 0x16 }, { 0x16 }, { 0x16 }, { 0x16 }, { 0x16 }, { 0x16 },	{ 0x16 }, { 0x16 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 },	{ 0x15 }, { 0x15 },	{ 0x15 }, { 0x15 },
	{ 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x15 }, { 0x14 },	{ 0x14 }, { 0x14 },
	{ 0x14 }, { 0x14 },	{ 0x14 }, { 0x14 },	{ 0x14 }, { 0x14 },	{ 0x14 }, { 0x14 },
	{ 0x14 }, { 0x13 },	{ 0x13 }, { 0x13 },	{ 0x13 }, { 0x13 },	{ 0x13 }, { 0x13 },
	{ 0x13 }, { 0x13 },	{ 0x13 }, { 0x13 },	{ 0x13 }, { 0x12 }, { 0x12 }, { 0x12 },
	{ 0x12 }, { 0x12 }, { 0x12 }, { 0x12 },	{ 0x12 }, { 0x12 },	{ 0x12 }, { 0x12 },
	{ 0x12 }, { 0x11 }, { 0x11 }, { 0x11 },	{ 0x11 }, { 0x11 }, { 0x11 }, { 0x11 },
	{ 0x11 }, { 0x11 }, { 0x11 }, { 0x11 },	{ 0x11 }, { 0x11 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 }, { 0x10 },
	{ 0x10 }, { 0x10 },
};

static u8 m62_hbm_transition_table[MAX_PANEL_HBM][SMOOTH_TRANS_MAX][1] = {
	/* HBM off */
	{
		/* Normal */
		{ 0x20 },
		/* Smooth */
		{ 0x28 },
	},
	/* HBM on */
	{
		/* Normal */
		{ 0xE0 },
		/* Smooth */
		{ 0xE0 },
	}
};

static u8 m62_acl_opr_table[ACL_OPR_MAX][1] = {
	{ 0x00 }, /* ACL OFF OPR */
	{ 0x01 }, /* ACL ON OPR_3 */
	{ 0x01 }, /* ACL ON OPR_6 */
	{ 0x01 }, /* ACL ON OPR_8 */
	{ 0x02 }, /* ACL ON OPR_12 */
	{ 0x02 }, /* ACL ON OPR_15 */
};

static u8 m62_lpm_nit_table[4][2] = {
	/* LPM 2NIT: No effort because of already otp programmed by 53h to 0x23 */
	{ 0x89, 0x07 },
	/* LPM 10NIT */
	{ 0x89, 0x07 },
	/* LPM 30NIT */
	{ 0x59, 0x0C },
	/* LPM 60NIT */
	{ 0x09, 0x18 },
};

static u8 m62_lpm_on_table[MAX_HLPM_ON][1] = {
	[HLPM_ON_LOW] = { 0x23 },
	[HLPM_ON] = { 0x22 },
};

static u8 m62_vgh_table[][1] = {
	{ 0xCC },
	{ 0xC4 },
};

#if defined(CONFIG_SUPPORT_FAST_DISCHARGE)
static u8 m62_fast_discharge_table[][1] = {
	{ 0x80 },	//fd 0ff
	{ 0x40 },	//fd on
};
#endif

#ifdef CONFIG_DYNAMIC_FREQ
static u8 m62_dyn_ffc_table[S6E3FA9_MAX_MIPI_FREQ][2] = {
	/* FFC for Default OSC (96.5) */
	{0x1E, 0xAF}, /* 1176.5 */
	{0x1E, 0x32}, /* 1195.5 */
	{0x1F, 0x61}, /* 1150.5 */
	{0x1F, 0x34}, /* 1157 */
};
#endif

static struct maptbl m62_maptbl[MAX_MAPTBL] = {
	[GAMMA_MODE2_MAPTBL] = DEFINE_2D_MAPTBL(m62_brt_table, init_gamma_mode2_brt_table, getidx_gamma_mode2_brt_table, copy_common_maptbl),
	[HBM_ONOFF_MAPTBL] = DEFINE_3D_MAPTBL(m62_hbm_transition_table, init_common_table, getidx_hbm_transition_table, copy_common_maptbl),
	[ACL_OPR_MAPTBL] = DEFINE_2D_MAPTBL(m62_acl_opr_table, init_common_table, getidx_acl_opr_table, copy_common_maptbl),
	[TSET_MAPTBL] = DEFINE_0D_MAPTBL(m62_tset_table, init_common_table, NULL, copy_tset_maptbl),
	[ELVSS_MAPTBL] = DEFINE_2D_MAPTBL(m62_elvss_table, init_common_table, getidx_gm2_elvss_table, copy_common_maptbl),
	[LPM_NIT_MAPTBL] = DEFINE_2D_MAPTBL(m62_lpm_nit_table, init_lpm_brt_table, getidx_lpm_brt_table, copy_common_maptbl),
	[LPM_ON_MAPTBL] = DEFINE_2D_MAPTBL(m62_lpm_on_table, init_common_table, getidx_lpm_on_table, copy_common_maptbl),
#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
	[GRAYSPOT_CAL_MAPTBL] = DEFINE_0D_MAPTBL(m62_grayspot_cal_table, init_common_table, NULL, copy_grayspot_cal_maptbl),
#endif
#ifdef CONFIG_SUPPORT_XTALK_MODE
	[VGH_MAPTBL] = DEFINE_2D_MAPTBL(m62_vgh_table, init_common_table, getidx_vgh_table, copy_common_maptbl),
#endif
#ifdef CONFIG_DYNAMIC_FREQ
	[DYN_FFC_MAPTBL] = DEFINE_2D_MAPTBL(m62_dyn_ffc_table, init_common_table, getidx_dyn_ffc_table, copy_common_maptbl),
#endif
#if defined(CONFIG_SUPPORT_FAST_DISCHARGE)
	[FAST_DISCHARGE_MAPTBL] = DEFINE_2D_MAPTBL(m62_fast_discharge_table, init_common_table, getidx_fast_discharge_table, copy_common_maptbl),
#endif
};

/* ===================================================================================== */
/* ============================== [S6E3FA9 COMMAND TABLE] ============================== */
/* ===================================================================================== */
static u8 M62_A3_S1_KEY1_ENABLE[] = { 0x9F, 0xA5, 0xA5 };
static u8 M62_A3_S1_KEY2_ENABLE[] = { 0xF0, 0x5A, 0x5A };
static u8 M62_A3_S1_KEY3_ENABLE[] = { 0xFC, 0x5A, 0x5A };
static u8 M62_A3_S1_KEY1_DISABLE[] = { 0x9F, 0x5A, 0x5A };
static u8 M62_A3_S1_KEY2_DISABLE[] = { 0xF0, 0xA5, 0xA5 };
static u8 M62_A3_S1_KEY3_DISABLE[] = { 0xFC, 0xA5, 0xA5 };
static u8 M62_A3_S1_SLEEP_OUT[] = { 0x11 };
static u8 M62_A3_S1_SLEEP_IN[] = { 0x10 };
static u8 M62_A3_S1_DISPLAY_OFF[] = { 0x28 };
static u8 M62_A3_S1_DISPLAY_ON[] = { 0x29 };

static u8 M62_A3_S1_TE_ON[] = { 0x35, 0x00 };

static u8 M62_A3_S1_TSP_HSYNC[] = {
	0xB9,
	0x01, 0x80, 0xE8, 0x09, 0x00, 0x00, 0x00, 0x11,
	0x03
};

static u8 M62_A3_S1_ERR_FG_ENABLE[] = {
	0xE5,
	0x13,
};

static u8 M62_A3_S1_ERR_FG_SETTING[] = {
	0xED,
	0x00, 0x4C,
};

static u8 M62_A3_S1_ACL_DEFAULT_1[] = {
	0xB4,
	0x00, 0x55, 0x40, 0xFF
};

static u8 M62_A3_S1_ACL_DEFAULT_2[] = {
	0xB8,
	0x02, 0x00
};

static u8 M62_A3_S1_ELVSS[] = {
	0xB5,
	0x23, 0x8D, 0x16,
};

static u8 M62_A3_S1_SMOOTH_DIMMING_INIT[] = {
	0xB6, 0x01,
};

static u8 M62_A3_S1_SMOOTH_DIMMING[] = {
	0xB6, 0x08,
};

static u8 M62_A3_S1_FFC_SET[] = {
	0xC5,
	0x0D, 0x10, 0x64, 0x1E, 0xAF, 0x05, 0x00, 0x26, 0xB0
};

static u8 M62_A3_S1_DIA[] = {
	0xC6, 0x02,
};

static u8 M62_A3_S1_GAMMA_SETTING_1[] = {
	0xB7,
	0x00, 0x8D, 0xBB, 0x9B
};

static u8 M62_A3_S1_GAMMA_SETTING_2[] = {
	0xB7,
	0x00, 0x9A, 0xCC, 0xAC
};

static u8 M62_A3_S1_GAMMA_SETTING_3[] = {
	0xB6,
	0x00, 0xBB, 0xE6, 0xC4
};

static u8 M62_A3_S1_GAMMA_SETTING_4[] = {
	0xB6,
	0x04, 0xEA, 0x0A, 0xEA
};

static u8 M62_A3_S1_GAMMA_SETTING_5[] = {
	0xB6,
	0x04, 0xEA, 0x0A, 0xEA
};

static u8 M62_A3_S1_GAMMA_SETTING_6[] = {
	0xB6,
	0x04, 0xEA, 0x0A, 0xEA
};


#ifdef CONFIG_SUPPORT_DDI_CMDLOG
static u8 M62_A3_S1_CMDLOG_ENABLE[] = { 0xF7, 0x80 };
static u8 M62_A3_S1_CMDLOG_DISABLE[] = { 0xF7, 0x00 };
static u8 M62_A3_S1_GAMMA_UPDATE_ENABLE[] = { 0xF7, 0x8F };
#else
static u8 M62_A3_S1_GAMMA_UPDATE_ENABLE[] = { 0xF7, 0x03 };
#endif

static u8 M62_A3_S1_LPM_ON[] = { 0x53, 0x22 };
static u8 M62_A3_S1_LPM_NIT[] = { 0xBB, 0x89, 0x07 };

#ifdef CONFIG_SUPPORT_DYNAMIC_HLPM
static u8 M62_A3_S1_DYNAMIC_HLPM_ENABLE[] = {
	0x85,
	0x03, 0x0B, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x26, 0x02, 0xB2, 0x07, 0xBC, 0x09, 0xEB
};

static u8 M62_A3_S1_DYNAMIC_HLPM_DISABLE[] = {
	0x85,
	0x00
};
#endif

#ifdef CONFIG_DYNAMIC_FREQ
static u8 M62_A3_S1_FFC[] = {
	0xC5, /* default 1176 */
	0x0D, 0x10, 0x64, 0x1E, 0xAF, 0x05, 0x00, 0x26,
	0xB0
};

static DEFINE_PKTUI(m62_ffc, &m62_maptbl[DYN_FFC_MAPTBL], 4);
static DEFINE_VARIABLE_PACKET(m62_ffc, DSI_PKT_TYPE_WR, M62_A3_S1_FFC, 0);
#endif

static DEFINE_STATIC_PACKET(m62_level1_key_enable, DSI_PKT_TYPE_WR, M62_A3_S1_KEY1_ENABLE, 0);
static DEFINE_STATIC_PACKET(m62_level2_key_enable, DSI_PKT_TYPE_WR, M62_A3_S1_KEY2_ENABLE, 0);
static DEFINE_STATIC_PACKET(m62_level3_key_enable, DSI_PKT_TYPE_WR, M62_A3_S1_KEY3_ENABLE, 0);
static DEFINE_STATIC_PACKET(m62_level1_key_disable, DSI_PKT_TYPE_WR, M62_A3_S1_KEY1_DISABLE, 0);
static DEFINE_STATIC_PACKET(m62_level2_key_disable, DSI_PKT_TYPE_WR, M62_A3_S1_KEY2_DISABLE, 0);
static DEFINE_STATIC_PACKET(m62_level3_key_disable, DSI_PKT_TYPE_WR, M62_A3_S1_KEY3_DISABLE, 0);

static DEFINE_STATIC_PACKET(m62_sleep_out, DSI_PKT_TYPE_WR, M62_A3_S1_SLEEP_OUT, 0);
static DEFINE_STATIC_PACKET(m62_sleep_in, DSI_PKT_TYPE_WR, M62_A3_S1_SLEEP_IN, 0);
static DEFINE_STATIC_PACKET(m62_display_on, DSI_PKT_TYPE_WR, M62_A3_S1_DISPLAY_ON, 0);
static DEFINE_STATIC_PACKET(m62_display_off, DSI_PKT_TYPE_WR, M62_A3_S1_DISPLAY_OFF, 0);

static DEFINE_STATIC_PACKET(m62_te_on, DSI_PKT_TYPE_WR, M62_A3_S1_TE_ON, 0);
static DEFINE_STATIC_PACKET(m62_tsp_hsync, DSI_PKT_TYPE_WR, M62_A3_S1_TSP_HSYNC, 0);

static DEFINE_STATIC_PACKET(m62_err_fg_enable, DSI_PKT_TYPE_WR, M62_A3_S1_ERR_FG_ENABLE, 0);
static DEFINE_STATIC_PACKET(m62_err_fg_setting, DSI_PKT_TYPE_WR, M62_A3_S1_ERR_FG_SETTING, 0);
static DEFINE_STATIC_PACKET(m62_acl_default_1, DSI_PKT_TYPE_WR, M62_A3_S1_ACL_DEFAULT_1, 0);
static DEFINE_STATIC_PACKET(m62_acl_default_2, DSI_PKT_TYPE_WR, M62_A3_S1_ACL_DEFAULT_2, 0x51);
static DEFINE_PKTUI(m62_lpm_on, &m62_maptbl[LPM_ON_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(m62_lpm_on, DSI_PKT_TYPE_WR, M62_A3_S1_LPM_ON, 0);
static DEFINE_PKTUI(m62_lpm_nit, &m62_maptbl[LPM_NIT_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(m62_lpm_nit, DSI_PKT_TYPE_WR, M62_A3_S1_LPM_NIT, 0x02);

#ifdef CONFIG_SUPPORT_DYNAMIC_HLPM
static DEFINE_STATIC_PACKET(m62_dynamic_hlpm_enable, DSI_PKT_TYPE_WR, M62_A3_S1_DYNAMIC_HLPM_ENABLE, 0);
static DEFINE_STATIC_PACKET(m62_dynamic_hlpm_disable, DSI_PKT_TYPE_WR, M62_A3_S1_DYNAMIC_HLPM_DISABLE, 0);
#endif

static DECLARE_PKTUI(m62_elvss) = {
	{ .offset = 1, .maptbl = &m62_maptbl[TSET_MAPTBL] },
	{ .offset = 3, .maptbl = &m62_maptbl[ELVSS_MAPTBL] },
};
//static DEFINE_PKTUI(m62_elvss, &m62_maptbl[ELVSS_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(m62_elvss, DSI_PKT_TYPE_WR, M62_A3_S1_ELVSS, 0);
static DEFINE_STATIC_PACKET(m62_smooth_dimming_init, DSI_PKT_TYPE_WR, M62_A3_S1_SMOOTH_DIMMING_INIT, 0x04);
static DEFINE_STATIC_PACKET(m62_smooth_dimming, DSI_PKT_TYPE_WR, M62_A3_S1_SMOOTH_DIMMING, 0x04);
static DEFINE_STATIC_PACKET(m62_ffc_set, DSI_PKT_TYPE_WR, M62_A3_S1_FFC_SET, 0x00);

#ifdef CONFIG_SUPPORT_DDI_CMDLOG
static DEFINE_STATIC_PACKET(m62_cmdlog_enable, DSI_PKT_TYPE_WR, M62_A3_S1_CMDLOG_ENABLE, 0);
static DEFINE_STATIC_PACKET(m62_cmdlog_disable, DSI_PKT_TYPE_WR, M62_A3_S1_CMDLOG_DISABLE, 0);
#endif
static DEFINE_STATIC_PACKET(m62_gamma_update_enable, DSI_PKT_TYPE_WR, M62_A3_S1_GAMMA_UPDATE_ENABLE, 0);

static DEFINE_PANEL_MDELAY(m62_wait_1msec, 1);
static DEFINE_PANEL_MDELAY(m62_wait_10msec, 10);
static DEFINE_PANEL_UDELAY(m62_wait_33msec, 33400);
static DEFINE_PANEL_UDELAY(m62_wait_16msec, 16700);

static DEFINE_PANEL_MDELAY(m62_wait_100msec, 100);
static DEFINE_PANEL_MDELAY(m62_wait_120msec, 120);
static DEFINE_PANEL_MDELAY(m62_wait_sleep_out_110msec, 110);
static DEFINE_PANEL_MDELAY(m62_wait_sleep_in, 120);
static DEFINE_PANEL_UDELAY(m62_wait_1usec, 1);

static DEFINE_PANEL_KEY(m62_level1_key_enable, CMD_LEVEL_1, KEY_ENABLE, &PKTINFO(m62_level1_key_enable));
static DEFINE_PANEL_KEY(m62_level2_key_enable, CMD_LEVEL_2, KEY_ENABLE, &PKTINFO(m62_level2_key_enable));
static DEFINE_PANEL_KEY(m62_level3_key_enable, CMD_LEVEL_3, KEY_ENABLE, &PKTINFO(m62_level3_key_enable));
static DEFINE_PANEL_KEY(m62_level1_key_disable, CMD_LEVEL_1, KEY_DISABLE, &PKTINFO(m62_level1_key_disable));
static DEFINE_PANEL_KEY(m62_level2_key_disable, CMD_LEVEL_2, KEY_DISABLE, &PKTINFO(m62_level2_key_disable));
static DEFINE_PANEL_KEY(m62_level3_key_disable, CMD_LEVEL_3, KEY_DISABLE, &PKTINFO(m62_level3_key_disable));

static u8 M62_A3_S1_HBM_TRANSITION[] = {
	0x53, 0x20
};

static DEFINE_PKTUI(m62_hbm_transition, &m62_maptbl[HBM_ONOFF_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(m62_hbm_transition, DSI_PKT_TYPE_WR, M62_A3_S1_HBM_TRANSITION, 0);

static u8 M62_A3_S1_ACL[] = {
	0x55, 0x02
};

static DEFINE_PKTUI(m62_acl_control, &m62_maptbl[ACL_OPR_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(m62_acl_control, DSI_PKT_TYPE_WR, M62_A3_S1_ACL, 0);

static u8 M62_A3_S1_WRDISBV[] = {
	0x51, 0x03, 0xFF
};
static DEFINE_PKTUI(m62_wrdisbv, &m62_maptbl[GAMMA_MODE2_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(m62_wrdisbv, DSI_PKT_TYPE_WR, M62_A3_S1_WRDISBV, 0);


static u8 M62_A3_S1_CASET[] = { 0x2A, 0x00, 0x00, 0x04, 0x37 };
static u8 M62_A3_S1_PASET[] = { 0x2B, 0x00, 0x00, 0x09, 0x5F };

static DEFINE_STATIC_PACKET(m62_caset, DSI_PKT_TYPE_WR, M62_A3_S1_CASET, 0);
static DEFINE_STATIC_PACKET(m62_paset, DSI_PKT_TYPE_WR, M62_A3_S1_PASET, 0);

static DEFINE_STATIC_PACKET(m62_gamma_setting_1, DSI_PKT_TYPE_WR, M62_A3_S1_GAMMA_SETTING_1, 0x38);
static DEFINE_STATIC_PACKET(m62_gamma_setting_2, DSI_PKT_TYPE_WR, M62_A3_S1_GAMMA_SETTING_2, 0x0C);
static DEFINE_STATIC_PACKET(m62_gamma_setting_3, DSI_PKT_TYPE_WR, M62_A3_S1_GAMMA_SETTING_3, 0xDF);
static DEFINE_STATIC_PACKET(m62_gamma_setting_4, DSI_PKT_TYPE_WR, M62_A3_S1_GAMMA_SETTING_4, 0xB3);
static DEFINE_STATIC_PACKET(m62_gamma_setting_5, DSI_PKT_TYPE_WR, M62_A3_S1_GAMMA_SETTING_5, 0x87);
static DEFINE_STATIC_PACKET(m62_gamma_setting_6, DSI_PKT_TYPE_WR, M62_A3_S1_GAMMA_SETTING_6, 0x5B);

#if defined(CONFIG_SUPPORT_FAST_DISCHARGE)
static u8 M62_A3_S1_FAST_DISCHARGE[] = { 0xB5, 0x40 };
static DEFINE_PKTUI(m62_fast_discharge, &m62_maptbl[FAST_DISCHARGE_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(m62_fast_discharge, DSI_PKT_TYPE_WR, M62_A3_S1_FAST_DISCHARGE, 0x0B);
#endif

static u8 M62_A3_S1_MCD_ON_00[] = { 0xF4, 0xCE };
static u8 M62_A3_S1_MCD_ON_01[] = {
	0xCB,
	0x00, 0x00, 0x42, 0x0F, 0x00, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x50, 0x00, 0x00, 0x40, 0x47, 0x00,
	0x4D, 0x00, 0x00, 0x00, 0x20, 0x07, 0x28, 0x38,
	0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0,
	0x0B, 0x46
};
static u8 M62_A3_S1_MCD_ON_02[] = { 0xF6, 0x00 };
static u8 M62_A3_S1_MCD_ON_03[] = { 0xCC, 0x12 };

static u8 M62_A3_S1_MCD_OFF_00[] = { 0xF4, 0xCC };
static u8 M62_A3_S1_MCD_OFF_01[] = {
	0xCB,
	0x00, 0x00, 0x42, 0x0B, 0x00, 0x06, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x50, 0x00, 0x00, 0x40, 0x47, 0x00,
	0x4D, 0x00, 0x00, 0x00, 0x20, 0x07, 0x28, 0x38,
	0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0,
	0x00, 0x00
};
static u8 M62_A3_S1_MCD_OFF_02[] = { 0xF6, 0x90 };
static u8 M62_A3_S1_MCD_OFF_03[] = { 0xCC, 0x00 };

static DEFINE_STATIC_PACKET(m62_mcd_on_00, DSI_PKT_TYPE_WR, M62_A3_S1_MCD_ON_00, 0);
static DEFINE_STATIC_PACKET(m62_mcd_on_01, DSI_PKT_TYPE_WR, M62_A3_S1_MCD_ON_01, 0);
static DEFINE_STATIC_PACKET(m62_mcd_on_02, DSI_PKT_TYPE_WR, M62_A3_S1_MCD_ON_02, 0x05);
static DEFINE_STATIC_PACKET(m62_mcd_on_03, DSI_PKT_TYPE_WR, M62_A3_S1_MCD_ON_03, 0x04);

static DEFINE_STATIC_PACKET(m62_mcd_off_00, DSI_PKT_TYPE_WR, M62_A3_S1_MCD_OFF_00, 0);
static DEFINE_STATIC_PACKET(m62_mcd_off_01, DSI_PKT_TYPE_WR, M62_A3_S1_MCD_OFF_01, 0);
static DEFINE_STATIC_PACKET(m62_mcd_off_02, DSI_PKT_TYPE_WR, M62_A3_S1_MCD_OFF_02, 0x05);
static DEFINE_STATIC_PACKET(m62_mcd_off_03, DSI_PKT_TYPE_WR, M62_A3_S1_MCD_OFF_03, 0x04);


#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
static u8 M62_A3_S1_GRAYSPOT_ON_01[] = {
	0xF6, 0x00
};
static u8 M62_A3_S1_GRAYSPOT_ON_02[] = {
	0xF2, 0x02, 0x00, 0x58, 0x38, 0xD0
};
static u8 M62_A3_S1_GRAYSPOT_ON_03[] = {
	0xF4, 0x1E
};
static u8 M62_A3_S1_GRAYSPOT_ON_04[] = {
	0xB5, 0x19, 0x8D, 0x35, 0x00
};

static u8 M62_A3_S1_GRAYSPOT_OFF_01[] = {
	0xF6, 0x90
};
static u8 M62_A3_S1_GRAYSPOT_OFF_02[] = {
	0xF2, 0x02, 0x0E, 0x58, 0x38, 0x50
};
static u8 M62_A3_S1_GRAYSPOT_OFF_03[] = {
	0xF4, 0x1E
};
/* send elvss, 4th only */
static u8 M62_A3_S1_GRAYSPOT_OFF_04[] = {
	0xB5, 0x00
};

static DEFINE_STATIC_PACKET(m62_grayspot_on_01, DSI_PKT_TYPE_WR, M62_A3_S1_GRAYSPOT_ON_01, 0x05);
static DEFINE_STATIC_PACKET(m62_grayspot_on_02, DSI_PKT_TYPE_WR, M62_A3_S1_GRAYSPOT_ON_02, 0x07);
static DEFINE_STATIC_PACKET(m62_grayspot_on_03, DSI_PKT_TYPE_WR, M62_A3_S1_GRAYSPOT_ON_03, 0x11);
static DEFINE_STATIC_PACKET(m62_grayspot_on_04, DSI_PKT_TYPE_WR, M62_A3_S1_GRAYSPOT_ON_04, 0);

static DEFINE_STATIC_PACKET(m62_grayspot_off_01, DSI_PKT_TYPE_WR, M62_A3_S1_GRAYSPOT_OFF_01, 0x05);
static DEFINE_STATIC_PACKET(m62_grayspot_off_02, DSI_PKT_TYPE_WR, M62_A3_S1_GRAYSPOT_OFF_02, 0x07);
static DEFINE_STATIC_PACKET(m62_grayspot_off_03, DSI_PKT_TYPE_WR, M62_A3_S1_GRAYSPOT_OFF_03, 0x11);
static DEFINE_PKTUI(m62_grayspot_off_04, &m62_maptbl[GRAYSPOT_CAL_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(m62_grayspot_off_04, DSI_PKT_TYPE_WR, M62_A3_S1_GRAYSPOT_OFF_04, 0x03);
#endif

#ifdef CONFIG_SUPPORT_MST
static u8 M62_A3_S1_MST_ON_01[] = {
	0xF6,
	0x3B, 0x00, 0x78
};
static u8 M62_A3_S1_MST_ON_02[] = { 0xBF, 0x33, 0x25 };

static u8 M62_A3_S1_MST_OFF_01[] = {
	0xF6,
	0x00, 0x00, 0x90
};
static u8 M62_A3_S1_MST_OFF_02[] = { 0xBF, 0x00, 0x07 };

static DEFINE_STATIC_PACKET(m62_mst_on_01, DSI_PKT_TYPE_WR, M62_A3_S1_MST_ON_01, 0x03);
static DEFINE_STATIC_PACKET(m62_mst_on_02, DSI_PKT_TYPE_WR, M62_A3_S1_MST_ON_02, 0);

static DEFINE_STATIC_PACKET(m62_mst_off_01, DSI_PKT_TYPE_WR, M62_A3_S1_MST_OFF_01, 0x03);
static DEFINE_STATIC_PACKET(m62_mst_off_02, DSI_PKT_TYPE_WR, M62_A3_S1_MST_OFF_02, 0);
#endif

#ifdef CONFIG_SUPPORT_XTALK_MODE
static u8 M62_A3_S1_XTALK_MODE[] = { 0xF4, 0xCC };
static DEFINE_PKTUI(m62_xtalk_mode, &m62_maptbl[VGH_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(m62_xtalk_mode, DSI_PKT_TYPE_WR, M62_A3_S1_XTALK_MODE, 0);
#endif

#ifdef CONFIG_SUPPORT_CCD_TEST
static u8 M62_A3_S1_CCD_ENABLE[] = { 0xCC, 0x01 };
static u8 M62_A3_S1_CCD_DISABLE[] = { 0xCC, 0x00 };
static DEFINE_STATIC_PACKET(m62_ccd_test_enable, DSI_PKT_TYPE_WR, M62_A3_S1_CCD_ENABLE, 0x02);
static DEFINE_STATIC_PACKET(m62_ccd_test_disable, DSI_PKT_TYPE_WR, M62_A3_S1_CCD_DISABLE, 0x02);
#endif

static struct seqinfo SEQINFO(m62_set_bl_param_seq);

static void *m62_init_cmdtbl[] = {
	&DLYINFO(m62_wait_10msec),
	&KEYINFO(m62_level1_key_enable),
	&KEYINFO(m62_level2_key_enable),
	&KEYINFO(m62_level3_key_enable),
	&PKTINFO(m62_sleep_out),
	&DLYINFO(m62_wait_10msec),
	&PKTINFO(m62_caset),
	&PKTINFO(m62_paset),
	&PKTINFO(m62_te_on),
	&PKTINFO(m62_tsp_hsync),
#ifdef CONFIG_DYNAMIC_FREQ
	&PKTINFO(m62_ffc),
#else
	&PKTINFO(m62_ffc_set),
#endif
	&PKTINFO(m62_err_fg_enable),
	&PKTINFO(m62_err_fg_setting),
	&PKTINFO(m62_acl_default_1),
	&PKTINFO(m62_acl_default_2),
	&PKTINFO(m62_smooth_dimming),
#if defined(CONFIG_SUPPORT_FAST_DISCHARGE)
	&PKTINFO(m62_fast_discharge),
#endif
	&SEQINFO(m62_set_bl_param_seq),
	&KEYINFO(m62_level3_key_disable),
	&KEYINFO(m62_level2_key_disable),
	&KEYINFO(m62_level1_key_disable),
	&DLYINFO(m62_wait_sleep_out_110msec),
};

static void *m62_res_init_cmdtbl[] = {
	&KEYINFO(m62_level1_key_enable),
	&KEYINFO(m62_level2_key_enable),
	&KEYINFO(m62_level3_key_enable),
	&s6e3fa9_restbl[RES_COORDINATE],
	&s6e3fa9_restbl[RES_CODE],
	&s6e3fa9_restbl[RES_DATE],
	&s6e3fa9_restbl[RES_OCTA_ID],
#ifdef CONFIG_DISPLAY_USE_INFO
	&s6e3fa9_restbl[RES_CHIP_ID],
	&s6e3fa9_restbl[RES_SELF_DIAG],
	&s6e3fa9_restbl[RES_ERR_FG],
	&s6e3fa9_restbl[RES_DSI_ERR],
#endif
	&KEYINFO(m62_level3_key_disable),
	&KEYINFO(m62_level2_key_disable),
	&KEYINFO(m62_level1_key_disable),
};

static void *m62_set_bl_param_cmdtbl[] = {
	&PKTINFO(m62_hbm_transition),
	&PKTINFO(m62_acl_control),
	&PKTINFO(m62_elvss),
	&PKTINFO(m62_wrdisbv),
#ifdef CONFIG_SUPPORT_XTALK_MODE
	&PKTINFO(m62_xtalk_mode),
#endif
};

static DEFINE_SEQINFO(m62_set_bl_param_seq, m62_set_bl_param_cmdtbl);

static void *m62_set_bl_cmdtbl[] = {
	&KEYINFO(m62_level1_key_enable),
	&KEYINFO(m62_level2_key_enable),
	&SEQINFO(m62_set_bl_param_seq),
	&PKTINFO(m62_gamma_update_enable),
	&KEYINFO(m62_level2_key_disable),
	&KEYINFO(m62_level1_key_disable),
};

static void *m62_display_on_cmdtbl[] = {
	&KEYINFO(m62_level1_key_enable),
	&PKTINFO(m62_display_on),
	&KEYINFO(m62_level1_key_disable),
};

static void *m62_display_off_cmdtbl[] = {
	&KEYINFO(m62_level1_key_enable),
	&PKTINFO(m62_display_off),
	&KEYINFO(m62_level1_key_disable),
};

static void *m62_exit_cmdtbl[] = {
 	&KEYINFO(m62_level1_key_enable),
	&s6e3fa9_dmptbl[DUMP_RDDPM],
#ifdef CONFIG_DISPLAY_USE_INFO
	&KEYINFO(m62_level2_key_enable),
	&s6e3fa9_dmptbl[DUMP_ERR_FG],
	&KEYINFO(m62_level2_key_disable),
	&s6e3fa9_dmptbl[DUMP_DSI_ERR],
	&s6e3fa9_dmptbl[DUMP_SELF_DIAG],
#endif
	&PKTINFO(m62_sleep_in),
	&KEYINFO(m62_level1_key_disable),
	&DLYINFO(m62_wait_sleep_in),
};

static void *m62_check_condition_cmdtbl[] = {
};

static void *m62_alpm_enter_cmdtbl[] = {
	&KEYINFO(m62_level1_key_enable),
	&KEYINFO(m62_level2_key_enable),
	&PKTINFO(m62_lpm_nit),
	&PKTINFO(m62_lpm_on),
	&DLYINFO(m62_wait_16msec),
	&KEYINFO(m62_level2_key_disable),
	&KEYINFO(m62_level1_key_disable),
};

static void *m62_alpm_exit_cmdtbl[] = {
	&DLYINFO(m62_wait_1usec),
};

static void *m62_alpm_enter_delay_cmdtbl[] = {
	&DLYINFO(m62_wait_33msec),
};

#ifdef CONFIG_SUPPORT_DDI_CMDLOG
static void *m62_cmdlog_dump_cmdtbl[] = {
	&KEYINFO(m62_level2_key_enable),
	&s6e3fa9_dmptbl[DUMP_CMDLOG],
	&KEYINFO(m62_level2_key_disable),
};
#endif

static void *m62_dump_cmdtbl[] = {
	&KEYINFO(m62_level1_key_enable),
	&KEYINFO(m62_level2_key_enable),
	&KEYINFO(m62_level3_key_enable),
	&s6e3fa9_dmptbl[DUMP_RDDPM],
	&s6e3fa9_dmptbl[DUMP_RDDSM],
	&s6e3fa9_dmptbl[DUMP_ERR],
	&s6e3fa9_dmptbl[DUMP_ERR_FG],
	&s6e3fa9_dmptbl[DUMP_DSI_ERR],
	&s6e3fa9_dmptbl[DUMP_SELF_DIAG],
	&KEYINFO(m62_level3_key_disable),
	&KEYINFO(m62_level2_key_disable),
	&KEYINFO(m62_level1_key_disable),
};

static void *m62_mcd_on_cmdtbl[] = {
	&KEYINFO(m62_level2_key_enable),
	&PKTINFO(m62_mcd_on_00),
 	&PKTINFO(m62_mcd_on_01),
	&PKTINFO(m62_gamma_update_enable),
	&PKTINFO(m62_mcd_on_02),
	&PKTINFO(m62_mcd_on_03),
 	&PKTINFO(m62_gamma_update_enable),
	&DLYINFO(m62_wait_100msec),
	&KEYINFO(m62_level2_key_disable),
};

static void *m62_mcd_off_cmdtbl[] = {
	&KEYINFO(m62_level2_key_enable),
	&PKTINFO(m62_mcd_off_00),
 	&PKTINFO(m62_mcd_off_01),
	&PKTINFO(m62_mcd_off_02),
	&PKTINFO(m62_mcd_off_03),
 	&PKTINFO(m62_gamma_update_enable),
	&DLYINFO(m62_wait_100msec),
	&KEYINFO(m62_level2_key_disable),
};

#ifdef CONFIG_SUPPORT_MST
static void *m62_mst_on_cmdtbl[] = {
	&KEYINFO(m62_level2_key_enable),
	&PKTINFO(m62_mst_on_01),
	&PKTINFO(m62_mst_on_02),
	&KEYINFO(m62_level2_key_disable),
};

static void *m62_mst_off_cmdtbl[] = {
	&KEYINFO(m62_level2_key_enable),
	&PKTINFO(m62_mst_off_01),
	&PKTINFO(m62_mst_off_02),
	&KEYINFO(m62_level2_key_disable),
};
#endif

#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
static void *m62_grayspot_on_cmdtbl[] = {
	&KEYINFO(m62_level2_key_enable),
	&PKTINFO(m62_grayspot_on_01),
	&PKTINFO(m62_grayspot_on_02),
	&PKTINFO(m62_grayspot_on_03),
	&PKTINFO(m62_grayspot_on_04),
	&PKTINFO(m62_gamma_update_enable),
	&DLYINFO(m62_wait_100msec),
	&KEYINFO(m62_level2_key_disable),
};

static void *m62_grayspot_off_cmdtbl[] = {
	&KEYINFO(m62_level2_key_enable),
	&PKTINFO(m62_grayspot_off_01),
	&PKTINFO(m62_grayspot_off_02),
	&PKTINFO(m62_grayspot_off_03),
	&PKTINFO(m62_elvss),
	&PKTINFO(m62_grayspot_off_04),
	&PKTINFO(m62_gamma_update_enable),
	&DLYINFO(m62_wait_100msec),
	&KEYINFO(m62_level2_key_disable),
};
#endif

#ifdef CONFIG_SUPPORT_CCD_TEST
static void *m62_ccd_test_cmdtbl[] = {
	&KEYINFO(m62_level2_key_enable),
	&PKTINFO(m62_ccd_test_enable),
	&DLYINFO(m62_wait_1msec),
	&s6e3fa9_restbl[RES_CCD_STATE],
	&PKTINFO(m62_ccd_test_disable),
	&KEYINFO(m62_level2_key_disable),
};
#endif

#if defined(CONFIG_SUPPORT_FAST_DISCHARGE)
static void *m62_fast_discharge_cmdtbl[] = {
	&KEYINFO(m62_level2_key_enable),
	&PKTINFO(m62_fast_discharge),
	&KEYINFO(m62_level2_key_disable),
	&DLYINFO(m62_wait_120msec),
};
#endif

#ifdef CONFIG_DYNAMIC_FREQ
static void *m62_dynamic_ffc_cmdtbl[] = {
	&KEYINFO(m62_level2_key_enable),
	&KEYINFO(m62_level3_key_enable),
	&PKTINFO(m62_ffc),
	&KEYINFO(m62_level3_key_disable),
	&KEYINFO(m62_level2_key_disable),
};
#endif

static void *m62_dummy_cmdtbl[] = {
	NULL,
};

static struct seqinfo m62_seqtbl[MAX_PANEL_SEQ] = {
	[PANEL_INIT_SEQ] = SEQINFO_INIT("init-seq", m62_init_cmdtbl),
	[PANEL_RES_INIT_SEQ] = SEQINFO_INIT("resource-init-seq", m62_res_init_cmdtbl),
	[PANEL_SET_BL_SEQ] = SEQINFO_INIT("set-bl-seq", m62_set_bl_cmdtbl),
	[PANEL_DISPLAY_ON_SEQ] = SEQINFO_INIT("display-on-seq", m62_display_on_cmdtbl),
	[PANEL_DISPLAY_OFF_SEQ] = SEQINFO_INIT("display-off-seq", m62_display_off_cmdtbl),
	[PANEL_EXIT_SEQ] = SEQINFO_INIT("exit-seq", m62_exit_cmdtbl),
	[PANEL_ALPM_ENTER_SEQ] = SEQINFO_INIT("alpm-enter-seq", m62_alpm_enter_cmdtbl),
	[PANEL_ALPM_DELAY_SEQ] = SEQINFO_INIT("alpm-enter-delay-seq", m62_alpm_enter_delay_cmdtbl),
	[PANEL_ALPM_EXIT_SEQ] = SEQINFO_INIT("alpm-exit-seq", m62_alpm_exit_cmdtbl),
	[PANEL_MCD_ON_SEQ] = SEQINFO_INIT("mcd-on-seq", m62_mcd_on_cmdtbl),
	[PANEL_MCD_OFF_SEQ] = SEQINFO_INIT("mcd-off-seq", m62_mcd_off_cmdtbl),
#ifdef CONFIG_SUPPORT_MST
	[PANEL_MST_ON_SEQ] = SEQINFO_INIT("mst-on-seq", m62_mst_on_cmdtbl),
	[PANEL_MST_OFF_SEQ] = SEQINFO_INIT("mst-off-seq", m62_mst_off_cmdtbl),
#endif
#ifdef CONFIG_SUPPORT_CCD_TEST
	[PANEL_CCD_TEST_SEQ] = SEQINFO_INIT("ccd-test-seq", m62_ccd_test_cmdtbl),
#endif
#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
	[PANEL_GRAYSPOT_ON_SEQ] = SEQINFO_INIT("grayspot-on-seq", m62_grayspot_on_cmdtbl),
	[PANEL_GRAYSPOT_OFF_SEQ] = SEQINFO_INIT("grayspot-off-seq", m62_grayspot_off_cmdtbl),
#endif
	[PANEL_DUMP_SEQ] = SEQINFO_INIT("dump-seq", m62_dump_cmdtbl),
#ifdef CONFIG_SUPPORT_DDI_CMDLOG
	[PANEL_CMDLOG_DUMP_SEQ] = SEQINFO_INIT("cmdlog-dump-seq", m62_cmdlog_dump_cmdtbl),
#endif
#ifdef CONFIG_DYNAMIC_FREQ
	[PANEL_DYNAMIC_FFC_SEQ] = SEQINFO_INIT("dynamic-ffc-seq", m62_dynamic_ffc_cmdtbl),
#endif
#if defined(CONFIG_SUPPORT_FAST_DISCHARGE)
	[PANEL_FD_SEQ] = SEQINFO_INIT("fast-discharge-seq", m62_fast_discharge_cmdtbl),
#endif
	[PANEL_CHECK_CONDITION_SEQ] = SEQINFO_INIT("check-condition-seq", m62_check_condition_cmdtbl),
	[PANEL_DUMMY_SEQ] = SEQINFO_INIT("dummy-seq", m62_dummy_cmdtbl),
};

struct common_panel_info s6e3fa9_m62_panel_info = {
	.ldi_name = "s6e3fa9",
	.name = "s6e3fa9_m62_default",
	.model = "AMB675TG01",
	.vendor = "SDC",
	.id = 0x814001,
	.rev = 0,
	.ddi_props = {
		.gpara = (DDI_SUPPORT_WRITE_GPARA |
				DDI_SUPPORT_READ_GPARA | DDI_SUPPORT_POINT_GPARA),
//		.err_fg_recovery = false,
	},
	.maptbl = m62_maptbl,
	.nr_maptbl = ARRAY_SIZE(m62_maptbl),
	.seqtbl = m62_seqtbl,
	.nr_seqtbl = ARRAY_SIZE(m62_seqtbl),
	.rditbl = s6e3fa9_rditbl,
	.nr_rditbl = ARRAY_SIZE(s6e3fa9_rditbl),
	.restbl = s6e3fa9_restbl,
	.nr_restbl = ARRAY_SIZE(s6e3fa9_restbl),
	.dumpinfo = s6e3fa9_dmptbl,
	.nr_dumpinfo = ARRAY_SIZE(s6e3fa9_dmptbl),
#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
	.mdnie_tune = &s6e3fa9_m62_mdnie_tune,
#endif
	.panel_dim_info = {
		[PANEL_BL_SUBDEV_TYPE_DISP] = &s6e3fa9_m62_panel_dimming_info,
#ifdef CONFIG_SUPPORT_AOD_BL
		[PANEL_BL_SUBDEV_TYPE_AOD] = &s6e3fa9_m62_panel_aod_dimming_info,
#endif
	},
#ifdef CONFIG_EXTEND_LIVE_CLOCK
	.aod_tune = &s6e3fa9_m62_aod,
#endif
#ifdef CONFIG_SUPPORT_DISPLAY_PROFILER
	.profile_tune = &fa9_profiler_tune,
#endif
#ifdef CONFIG_DYNAMIC_FREQ
	.df_freq_tbl = m62_dynamic_freq_set,
#endif

};

static int __init s6e3fa9_m62_panel_init(void)
{
	register_common_panel(&s6e3fa9_m62_panel_info);

	return 0;
}
arch_initcall(s6e3fa9_m62_panel_init)
#endif /* __S6E3FA9_M62_A3_S1_PANEL_H__ */
