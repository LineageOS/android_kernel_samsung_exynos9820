/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3fa9/s6e3fa9.h
 *
 * Header file for S6E3FA9 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3FA9_H__
#define __S6E3FA9_H__

#include <linux/types.h>
#include <linux/kernel.h>
#ifdef CONFIG_SUPPORT_DDI_FLASH
#include "../panel_poc.h"
#endif

/*
 * OFFSET ==> OFS means N-param - 1
 * <example>
 * XXX 1st param => S6E3FA9_XXX_OFS (0)
 * XXX 2nd param => S6E3FA9_XXX_OFS (1)
 * XXX 36th param => S6E3FA9_XXX_OFS (35)
 */

#define AID_INTERPOLATION
#define S6E3FA9_GAMMA_CMD_CNT (35)

#define S6E3FA9_IRC_ONOFF_OFS	(0)
#define S6E3FA9_IRC_VALUE_OFS	(10)
#define S6E3FA9_IRC_VALUE_LEN	(33)

#define S6E3FA9_ADDR_OFS	(0)
#define S6E3FA9_ADDR_LEN	(1)
#define S6E3FA9_DATA_OFS	(S6E3FA9_ADDR_OFS + S6E3FA9_ADDR_LEN)

#define S6E3FA9_MTP_REG				0xC8
#define S6E3FA9_MTP_OFS				0
#define S6E3FA9_MTP_LEN				34

#define S6E3FA9_DATE_REG			0xA1
#define S6E3FA9_DATE_OFS			4
#define S6E3FA9_DATE_LEN			(PANEL_DATE_LEN)

#define S6E3FA9_COORDINATE_REG		0xA1
#define S6E3FA9_COORDINATE_OFS		0
#define S6E3FA9_COORDINATE_LEN		(PANEL_COORD_LEN)

#define S6E3FA9_ID_REG				0x04
#define S6E3FA9_ID_OFS				0
#define S6E3FA9_ID_LEN				(PANEL_ID_LEN)

#define S6E3FA9_CODE_REG			0xD6
#define S6E3FA9_CODE_OFS			0
#define S6E3FA9_CODE_LEN			(PANEL_CODE_LEN)

#define S6E3FA9_ELVSS_REG			0xB5
#define S6E3FA9_ELVSS_OFS			0
#define S6E3FA9_ELVSS_LEN			23

#define S6E3FA9_ELVSS_TEMP_0_REG		0xB5
#define S6E3FA9_ELVSS_TEMP_0_OFS		74
#define S6E3FA9_ELVSS_TEMP_0_LEN		1

#define S6E3FA9_ELVSS_TEMP_1_REG		0xB5
#define S6E3FA9_ELVSS_TEMP_1_OFS		75
#define S6E3FA9_ELVSS_TEMP_1_LEN		1

#define S6E3FA9_GAMMA_INTER_REG			0xB1
#define S6E3FA9_GAMMA_INTER_OFS			0x5D
#define S6E3FA9_GAMMA_INTER_LEN			6

#define S6E3FA9_OCTA_ID_REG			0xA1
#define S6E3FA9_OCTA_ID_OFS			11
#define S6E3FA9_OCTA_ID_LEN			(PANEL_OCTA_ID_LEN)

#define S6E3FA9_COPR_SPI_REG			0x5A
#define S6E3FA9_COPR_SPI_OFS			0
#define S6E3FA9_COPR_SPI_LEN			(41)

#define S6E3FA9_COPR_DSI_REG			0x5A
#define S6E3FA9_COPR_DSI_OFS			0
#define S6E3FA9_COPR_DSI_LEN			(41)

#define S6E3FA9_CHIP_ID_REG			0xD6
#define S6E3FA9_CHIP_ID_OFS			0
#define S6E3FA9_CHIP_ID_LEN			5

/* for brightness debugging */
#define S6E3FA9_GAMMA_REG			0xCA
#define S6E3FA9_GAMMA_OFS			0
#define S6E3FA9_GAMMA_LEN			34

#define S6E3FA9_AOR_REG			0xB1
#define S6E3FA9_AOR_OFS			0
#define S6E3FA9_AOR_LEN			2

#define S6E3FA9_VINT_REG			0xF4
#define S6E3FA9_VINT_OFS			4
#define S6E3FA9_VINT_LEN			1

#define S6E3FA9_ELVSS_T_REG			0xB5
#define S6E3FA9_ELVSS_T_OFS			2
#define S6E3FA9_ELVSS_T_LEN			1

#define S6E3FA9_IRC_REG			(0x92)
#define S6E3FA9_IRC_OFS			(S6E3FA9_IRC_VALUE_OFS)
#define S6E3FA9_IRC_LEN			(S6E3FA9_IRC_VALUE_LEN)

/* for panel dump */
#define S6E3FA9_RDDPM_REG			0x0A
#define S6E3FA9_RDDPM_OFS			0
#define S6E3FA9_RDDPM_LEN			(PANEL_RDDPM_LEN)

#define S6E3FA9_RDDSM_REG			0x0E
#define S6E3FA9_RDDSM_OFS			0
#define S6E3FA9_RDDSM_LEN			(PANEL_RDDSM_LEN)

#define S6E3FA9_ERR_REG				0xEA
#define S6E3FA9_ERR_OFS				0
#define S6E3FA9_ERR_LEN				5

#define S6E3FA9_ERR_FG_REG			0xEE
#define S6E3FA9_ERR_FG_OFS			0
#define S6E3FA9_ERR_FG_LEN			1

#define S6E3FA9_DSI_ERR_REG			0x05
#define S6E3FA9_DSI_ERR_OFS			0
#define S6E3FA9_DSI_ERR_LEN			1

#define S6E3FA9_SELF_DIAG_REG			0x0F
#define S6E3FA9_SELF_DIAG_OFS			0
#define S6E3FA9_SELF_DIAG_LEN			1

#define S6E3FA9_SELF_MASK_CHECKSUM_REG		0xFB
#define S6E3FA9_SELF_MASK_CHECKSUM_OFS		15
#define S6E3FA9_SELF_MASK_CHECKSUM_LEN		2

#define S6E3FA9_SELF_MASK_CRC_REG		0x7F
#define S6E3FA9_SELF_MASK_CRC_OFS	6
#define S6E3FA9_SELF_MASK_CRC_LEN		4

#ifdef CONFIG_SUPPORT_DDI_CMDLOG
#define S6E3FA9_CMDLOG_REG			0x9C
#define S6E3FA9_CMDLOG_OFS			0
#define S6E3FA9_CMDLOG_LEN			0x80
#endif

#ifdef CONFIG_SUPPORT_CCD_TEST
#define S6E3FA9_CCD_STATE_REG				0xCD
#define S6E3FA9_CCD_STATE_OFS				14
#define S6E3FA9_CCD_STATE_LEN				1
#endif

#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
#define S6E3FA9_GRAYSPOT_CAL_REG			0xB5
#define S6E3FA9_GRAYSPOT_CAL_OFS			3
#define S6E3FA9_GRAYSPOT_CAL_LEN			1
#endif

#ifdef CONFIG_SUPPORT_POC_SPI
#define S6E3FA9_POC_SPI_READ_REG (0x03)
#define S6E3FA9_POC_SPI_READ_LEN (2048)
#define S6E3FA9_POC_SPI_STATUS1_REG (0x05)
#define S6E3FA9_POC_SPI_STATUS1_LEN (1)
#define S6E3FA9_POC_SPI_STATUS2_REG (0x35)
#define S6E3FA9_POC_SPI_STATUS2_LEN (1)
#endif

#ifdef CONFIG_SUPPORT_POC_FLASH
#define S6E3FA9_POC_MCA_CHKSUM_REG		(0x91)
#define S6E3FA9_POC_MCA_CHKSUM_OFS		(0)
#define S6E3FA9_POC_MCA_CHKSUM_LEN		(94)
#endif

#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
#define NR_S6E3FA9_MDNIE_REG	(3)

#define S6E3FA9_MDNIE_0_REG		(0xDF)
#define S6E3FA9_MDNIE_0_OFS		(0)
#define S6E3FA9_MDNIE_0_LEN		(55)

#define S6E3FA9_MDNIE_1_REG		(0xDE)
#define S6E3FA9_MDNIE_1_OFS		(S6E3FA9_MDNIE_0_OFS + S6E3FA9_MDNIE_0_LEN)
#define S6E3FA9_MDNIE_1_LEN		(182)

#define S6E3FA9_MDNIE_2_REG		(0xDD)
#define S6E3FA9_MDNIE_2_OFS		(S6E3FA9_MDNIE_1_OFS + S6E3FA9_MDNIE_1_LEN)
#define S6E3FA9_MDNIE_2_LEN		(19)
#define S6E3FA9_MDNIE_LEN		(S6E3FA9_MDNIE_0_LEN + S6E3FA9_MDNIE_1_LEN + S6E3FA9_MDNIE_2_LEN)

#define S6E3FA9_SCR_CR_OFS	(31)
#define S6E3FA9_SCR_WR_OFS	(49)
#define S6E3FA9_SCR_WG_OFS	(51)
#define S6E3FA9_SCR_WB_OFS	(53)
#define S6E3FA9_NIGHT_MODE_OFS	(S6E3FA9_SCR_CR_OFS)
#define S6E3FA9_NIGHT_MODE_LEN	(24)
#define S6E3FA9_COLOR_LENS_OFS	(S6E3FA9_SCR_CR_OFS)
#define S6E3FA9_COLOR_LENS_LEN	(24)

#define S6E3FA9_TRANS_MODE_OFS	(16)
#define S6E3FA9_TRANS_MODE_LEN	(1)
#endif /* CONFIG_EXYNOS_DECON_MDNIE_LITE */


#ifdef CONFIG_DYNAMIC_FREQ
#define S6E3FA9_MAX_MIPI_FREQ			4
#define S6E3FA9_DEFAULT_MIPI_FREQ		0
enum {
	S6E3FA9_OSC_DEFAULT,
	MAX_S6E3FA9_OSC,
};
#endif


enum {
	GAMMA_MAPTBL,
	GAMMA_MODE2_MAPTBL,
	VBIAS_MAPTBL,
	AOR_MAPTBL,
	MPS_MAPTBL,
	TSET_MAPTBL,
	ELVSS_MAPTBL,
	ELVSS_TEMP_MAPTBL,
#ifdef CONFIG_SUPPORT_XTALK_MODE
	VGH_MAPTBL,
#endif
	VINT_MAPTBL,
	VINT_VRR_120HZ_MAPTBL,
	ACL_ONOFF_MAPTBL,
	ACL_FRAME_AVG_MAPTBL,
	ACL_START_POINT_MAPTBL,
	ACL_OPR_MAPTBL,
	IRC_MAPTBL,
	IRC_MODE_MAPTBL,
#ifdef CONFIG_SUPPORT_HMD
	/* HMD MAPTBL */
	HMD_GAMMA_MAPTBL,
	HMD_AOR_MAPTBL,
	HMD_ELVSS_MAPTBL,
#endif /* CONFIG_SUPPORT_HMD */
	DSC_MAPTBL,
	PPS_MAPTBL,
	SCALER_MAPTBL,
	CASET_MAPTBL,
	PASET_MAPTBL,
	SSD_IMPROVE_MAPTBL,
	AID_MAPTBL,
	OSC_MAPTBL,
	VFP_NM_MAPTBL,
	VFP_HS_MAPTBL,
	PWR_GEN_MAPTBL,
	SRC_AMP_MAPTBL,
	MTP_MAPTBL,
	FPS_MAPTBL,
	LPM_NIT_MAPTBL,
	LPM_MODE_MAPTBL,
	LPM_DYN_VLIN_MAPTBL,
	LPM_ON_MAPTBL,
	LPM_AOR_OFF_MAPTBL,
#ifdef CONFIG_SUPPORT_DDI_FLASH
	POC_ON_MAPTBL,
	POC_WR_ADDR_MAPTBL,
	POC_RD_ADDR_MAPTBL,
	POC_WR_DATA_MAPTBL,
#endif
#ifdef CONFIG_SUPPORT_POC_FLASH
	POC_ER_ADDR_MAPTBL,
#endif
#ifdef CONFIG_SUPPORT_POC_SPI
	POC_SPI_READ_ADDR_MAPTBL,
	POC_SPI_WRITE_ADDR_MAPTBL,
	POC_SPI_WRITE_DATA_MAPTBL,
	POC_SPI_ERASE_ADDR_MAPTBL,
#endif
	POC_ONOFF_MAPTBL,
#ifdef CONFIG_SUPPORT_TDMB_TUNE
	TDMB_TUNE_MAPTBL,
#endif
#ifdef CONFIG_DYNAMIC_FREQ
	DYN_FFC_MAPTBL,
#endif

#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
	GRAYSPOT_CAL_MAPTBL,
#endif
	GAMMA_INTER_CONTROL_MAPTBL,
	POC_COMP_MAPTBL,
	DBV_MAPTBL,
	HBM_CYCLE_MAPTBL,
	HBM_ONOFF_MAPTBL,
	DIA_ONOFF_MAPTBL,
#if defined(CONFIG_SUPPORT_FAST_DISCHARGE)
	FAST_DISCHARGE_MAPTBL,
#endif
	MAX_MAPTBL,
};

enum {
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
	READ_COPR_SPI,
	READ_COPR_DSI,
#endif
	READ_ID,
	READ_COORDINATE,
	READ_CODE,
	READ_ELVSS,
	READ_ELVSS_TEMP_0,
	READ_ELVSS_TEMP_1,
	READ_MTP,
	READ_DATE,
	READ_OCTA_ID,
	READ_CHIP_ID,
	/* for brightness debugging */
	READ_AOR,
	READ_VINT,
	READ_ELVSS_T,
	READ_IRC,

	READ_RDDPM,
	READ_RDDSM,
	READ_ERR,
	READ_ERR_FG,
	READ_DSI_ERR,
	READ_SELF_DIAG,
	READ_SELF_MASK_CHECKSUM,
	READ_SELF_MASK_CRC,
#ifdef CONFIG_SUPPORT_POC_SPI
	READ_POC_SPI_READ,
	READ_POC_SPI_STATUS1,
	READ_POC_SPI_STATUS2,
#endif
#ifdef CONFIG_SUPPORT_POC_FLASH
	READ_POC_MCA_CHKSUM,
#endif
#ifdef CONFIG_SUPPORT_DDI_CMDLOG
	READ_CMDLOG,
#endif
#ifdef CONFIG_SUPPORT_DDI_CMDLOG
	READ_CMDLOG,
#endif
#ifdef CONFIG_SUPPORT_CCD_TEST
	READ_CCD_STATE,
#endif
#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
	READ_GRAYSPOT_CAL,
#endif
};

enum {
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
	RES_COPR_SPI,
	RES_COPR_DSI,
#endif
	RES_ID,
	RES_COORDINATE,
	RES_CODE,
	RES_ELVSS,
	RES_ELVSS_TEMP_0,
	RES_ELVSS_TEMP_1,
	RES_MTP,
	RES_DATE,
	RES_OCTA_ID,
	RES_CHIP_ID,
	/* for brightness debugging */
	RES_AOR,
	RES_VINT,
	RES_ELVSS_T,
	RES_IRC,
	RES_RDDPM,
	RES_RDDSM,
	RES_ERR,
	RES_ERR_FG,
	RES_DSI_ERR,
	RES_SELF_DIAG,
#ifdef CONFIG_SUPPORT_DDI_CMDLOG
	RES_CMDLOG,
#endif
#ifdef CONFIG_SUPPORT_POC_SPI
	RES_POC_SPI_READ,
	RES_POC_SPI_STATUS1,
	RES_POC_SPI_STATUS2,
#endif
#ifdef CONFIG_SUPPORT_POC_FLASH
	RES_POC_MCA_CHKSUM,
#endif
#ifdef CONFIG_SUPPORT_CCD_TEST
	RES_CCD_STATE,
	RES_CCD_CHKSUM_FAIL,
#endif
#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
	RES_GRAYSPOT_CAL,
#endif
	RES_SELF_MASK_CHECKSUM,
	RES_SELF_MASK_CRC,
};

enum {
	HLPM_ON_LOW,
	HLPM_ON,
	MAX_HLPM_ON
};

static u8 S6E3FA9_ID[S6E3FA9_ID_LEN];
static u8 S6E3FA9_COORDINATE[S6E3FA9_COORDINATE_LEN];
static u8 S6E3FA9_CODE[S6E3FA9_CODE_LEN];
static u8 S6E3FA9_ELVSS[S6E3FA9_ELVSS_LEN];
static u8 S6E3FA9_ELVSS_TEMP_0[S6E3FA9_ELVSS_TEMP_0_LEN];
static u8 S6E3FA9_ELVSS_TEMP_1[S6E3FA9_ELVSS_TEMP_1_LEN];
static u8 S6E3FA9_MTP[S6E3FA9_MTP_LEN];
static u8 S6E3FA9_DATE[S6E3FA9_DATE_LEN];
static u8 S6E3FA9_OCTA_ID[S6E3FA9_OCTA_ID_LEN];
/* for brightness debugging */
static u8 S6E3FA9_AOR[S6E3FA9_AOR_LEN];
static u8 S6E3FA9_VINT[S6E3FA9_VINT_LEN];
static u8 S6E3FA9_ELVSS_T[S6E3FA9_ELVSS_T_LEN];
static u8 S6E3FA9_IRC[S6E3FA9_IRC_LEN];
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
static u8 S6E3FA9_COPR_SPI[S6E3FA9_COPR_SPI_LEN];
static u8 S6E3FA9_COPR_DSI[S6E3FA9_COPR_DSI_LEN];
#endif
static u8 S6E3FA9_CHIP_ID[S6E3FA9_CHIP_ID_LEN];
static u8 S6E3FA9_RDDPM[S6E3FA9_RDDPM_LEN];
static u8 S6E3FA9_RDDSM[S6E3FA9_RDDSM_LEN];
static u8 S6E3FA9_ERR[S6E3FA9_ERR_LEN];
static u8 S6E3FA9_ERR_FG[S6E3FA9_ERR_FG_LEN];
static u8 S6E3FA9_DSI_ERR[S6E3FA9_DSI_ERR_LEN];
static u8 S6E3FA9_SELF_DIAG[S6E3FA9_SELF_DIAG_LEN];
#ifdef CONFIG_SUPPORT_DDI_CMDLOG
static u8 S6E3FA9_CMDLOG[S6E3FA9_CMDLOG_LEN];
#endif
#ifdef CONFIG_SUPPORT_POC_SPI
static u8 S6E3FA9_POC_SPI_READ[S6E3FA9_POC_SPI_READ_LEN];
static u8 S6E3FA9_POC_SPI_STATUS1[S6E3FA9_POC_SPI_STATUS1_LEN];
static u8 S6E3FA9_POC_SPI_STATUS2[S6E3FA9_POC_SPI_STATUS2_LEN];
#endif
#ifdef CONFIG_SUPPORT_POC_FLASH
static u8 S6E3FA9_POC_MCA_CHKSUM[S6E3FA9_POC_MCA_CHKSUM_LEN];
#endif
#ifdef CONFIG_SUPPORT_CCD_TEST
static u8 S6E3FA9_CCD_STATE[S6E3FA9_CCD_STATE_LEN];
static u8 S6E3FA9_CCD_CHKSUM_FAIL[S6E3FA9_CCD_STATE_LEN] = { 0x04 };
#endif
#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
static u8 S6E3FA9_GRAYSPOT_CAL[S6E3FA9_GRAYSPOT_CAL_LEN];
#endif
static u8 S6E3FA9_SELF_MASK_CHECKSUM[S6E3FA9_SELF_MASK_CHECKSUM_LEN];
static u8 S6E3FA9_SELF_MASK_CRC[S6E3FA9_SELF_MASK_CRC_LEN];

static struct rdinfo s6e3fa9_rditbl[] = {
	[READ_ID] = RDINFO_INIT(id, DSI_PKT_TYPE_RD, S6E3FA9_ID_REG, S6E3FA9_ID_OFS, S6E3FA9_ID_LEN),
	[READ_COORDINATE] = RDINFO_INIT(coordinate, DSI_PKT_TYPE_RD, S6E3FA9_COORDINATE_REG, S6E3FA9_COORDINATE_OFS, S6E3FA9_COORDINATE_LEN),
	[READ_CODE] = RDINFO_INIT(code, DSI_PKT_TYPE_RD, S6E3FA9_CODE_REG, S6E3FA9_CODE_OFS, S6E3FA9_CODE_LEN),
	[READ_ELVSS] = RDINFO_INIT(elvss, DSI_PKT_TYPE_RD, S6E3FA9_ELVSS_REG, S6E3FA9_ELVSS_OFS, S6E3FA9_ELVSS_LEN),
	[READ_ELVSS_TEMP_0] = RDINFO_INIT(elvss_temp_0, DSI_PKT_TYPE_RD, S6E3FA9_ELVSS_TEMP_0_REG, S6E3FA9_ELVSS_TEMP_0_OFS, S6E3FA9_ELVSS_TEMP_0_LEN),
	[READ_ELVSS_TEMP_1] = RDINFO_INIT(elvss_temp_1, DSI_PKT_TYPE_RD, S6E3FA9_ELVSS_TEMP_1_REG, S6E3FA9_ELVSS_TEMP_1_OFS, S6E3FA9_ELVSS_TEMP_1_LEN),
	[READ_MTP] = RDINFO_INIT(mtp, DSI_PKT_TYPE_RD, S6E3FA9_MTP_REG, S6E3FA9_MTP_OFS, S6E3FA9_MTP_LEN),
	[READ_DATE] = RDINFO_INIT(date, DSI_PKT_TYPE_RD, S6E3FA9_DATE_REG, S6E3FA9_DATE_OFS, S6E3FA9_DATE_LEN),
	[READ_OCTA_ID] = RDINFO_INIT(octa_id, DSI_PKT_TYPE_RD, S6E3FA9_OCTA_ID_REG, S6E3FA9_OCTA_ID_OFS, S6E3FA9_OCTA_ID_LEN),
	/* for brightness debugging */
	[READ_AOR] = RDINFO_INIT(aor, DSI_PKT_TYPE_RD, S6E3FA9_AOR_REG, S6E3FA9_AOR_OFS, S6E3FA9_AOR_LEN),
	[READ_VINT] = RDINFO_INIT(vint, DSI_PKT_TYPE_RD, S6E3FA9_VINT_REG, S6E3FA9_VINT_OFS, S6E3FA9_VINT_LEN),
	[READ_ELVSS_T] = RDINFO_INIT(elvss_t, DSI_PKT_TYPE_RD, S6E3FA9_ELVSS_T_REG, S6E3FA9_ELVSS_T_OFS, S6E3FA9_ELVSS_T_LEN),
	[READ_IRC] = RDINFO_INIT(irc, DSI_PKT_TYPE_RD, S6E3FA9_IRC_REG, S6E3FA9_IRC_OFS, S6E3FA9_IRC_LEN),
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
	[READ_COPR_SPI] = RDINFO_INIT(copr_spi, SPI_PKT_TYPE_RD, S6E3FA9_COPR_SPI_REG, S6E3FA9_COPR_SPI_OFS, S6E3FA9_COPR_SPI_LEN),
	[READ_COPR_DSI] = RDINFO_INIT(copr_dsi, DSI_PKT_TYPE_RD, S6E3FA9_COPR_DSI_REG, S6E3FA9_COPR_DSI_OFS, S6E3FA9_COPR_DSI_LEN),
#endif
	[READ_CHIP_ID] = RDINFO_INIT(chip_id, DSI_PKT_TYPE_RD, S6E3FA9_CHIP_ID_REG, S6E3FA9_CHIP_ID_OFS, S6E3FA9_CHIP_ID_LEN),
	[READ_RDDPM] = RDINFO_INIT(rddpm, DSI_PKT_TYPE_RD, S6E3FA9_RDDPM_REG, S6E3FA9_RDDPM_OFS, S6E3FA9_RDDPM_LEN),
	[READ_RDDSM] = RDINFO_INIT(rddsm, DSI_PKT_TYPE_RD, S6E3FA9_RDDSM_REG, S6E3FA9_RDDSM_OFS, S6E3FA9_RDDSM_LEN),
	[READ_ERR] = RDINFO_INIT(err, DSI_PKT_TYPE_RD, S6E3FA9_ERR_REG, S6E3FA9_ERR_OFS, S6E3FA9_ERR_LEN),
	[READ_ERR_FG] = RDINFO_INIT(err_fg, DSI_PKT_TYPE_RD, S6E3FA9_ERR_FG_REG, S6E3FA9_ERR_FG_OFS, S6E3FA9_ERR_FG_LEN),
	[READ_DSI_ERR] = RDINFO_INIT(dsi_err, DSI_PKT_TYPE_RD, S6E3FA9_DSI_ERR_REG, S6E3FA9_DSI_ERR_OFS, S6E3FA9_DSI_ERR_LEN),
	[READ_SELF_DIAG] = RDINFO_INIT(self_diag, DSI_PKT_TYPE_RD, S6E3FA9_SELF_DIAG_REG, S6E3FA9_SELF_DIAG_OFS, S6E3FA9_SELF_DIAG_LEN),
#ifdef CONFIG_SUPPORT_DDI_CMDLOG
	[READ_CMDLOG] = RDINFO_INIT(cmdlog, DSI_PKT_TYPE_RD, S6E3FA9_CMDLOG_REG, S6E3FA9_CMDLOG_OFS, S6E3FA9_CMDLOG_LEN),
#endif
#ifdef CONFIG_SUPPORT_POC_SPI
	[READ_POC_SPI_READ] = RDINFO_INIT(poc_spi_read, SPI_PKT_TYPE_RD, S6E3FA9_POC_SPI_READ_REG, 0, S6E3FA9_POC_SPI_READ_LEN),
	[READ_POC_SPI_STATUS1] = RDINFO_INIT(poc_spi_status1, SPI_PKT_TYPE_RD, S6E3FA9_POC_SPI_STATUS1_REG, 0, S6E3FA9_POC_SPI_STATUS1_LEN),
	[READ_POC_SPI_STATUS2] = RDINFO_INIT(poc_spi_status2, SPI_PKT_TYPE_RD, S6E3FA9_POC_SPI_STATUS2_REG, 0, S6E3FA9_POC_SPI_STATUS2_LEN),
#endif
#ifdef CONFIG_SUPPORT_POC_FLASH
	[READ_POC_MCA_CHKSUM] = RDINFO_INIT(poc_mca_chksum, DSI_PKT_TYPE_RD, S6E3FA9_POC_MCA_CHKSUM_REG, S6E3FA9_POC_MCA_CHKSUM_OFS, S6E3FA9_POC_MCA_CHKSUM_LEN),
#endif
#ifdef CONFIG_SUPPORT_CCD_TEST
	[READ_CCD_STATE] = RDINFO_INIT(ccd_state, DSI_PKT_TYPE_RD, S6E3FA9_CCD_STATE_REG, S6E3FA9_CCD_STATE_OFS, S6E3FA9_CCD_STATE_LEN),
#endif
#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
	[READ_GRAYSPOT_CAL] = RDINFO_INIT(grayspot_cal, DSI_PKT_TYPE_RD, S6E3FA9_GRAYSPOT_CAL_REG, S6E3FA9_GRAYSPOT_CAL_OFS, S6E3FA9_GRAYSPOT_CAL_LEN),
#endif
	[READ_SELF_MASK_CHECKSUM] = RDINFO_INIT(self_mask_checksum, DSI_PKT_TYPE_RD, S6E3FA9_SELF_MASK_CHECKSUM_REG, S6E3FA9_SELF_MASK_CHECKSUM_OFS, S6E3FA9_SELF_MASK_CHECKSUM_LEN),
	[READ_SELF_MASK_CRC] = RDINFO_INIT(self_mask_crc, DSI_PKT_TYPE_RD, S6E3FA9_SELF_MASK_CRC_REG, S6E3FA9_SELF_MASK_CRC_OFS, S6E3FA9_SELF_MASK_CRC_LEN),
};

static DEFINE_RESUI(id, &s6e3fa9_rditbl[READ_ID], 0);
static DEFINE_RESUI(coordinate, &s6e3fa9_rditbl[READ_COORDINATE], 0);
static DEFINE_RESUI(code, &s6e3fa9_rditbl[READ_CODE], 0);
static DEFINE_RESUI(elvss, &s6e3fa9_rditbl[READ_ELVSS], 0);
static DEFINE_RESUI(elvss_temp_0, &s6e3fa9_rditbl[READ_ELVSS_TEMP_0], 0);
static DEFINE_RESUI(elvss_temp_1, &s6e3fa9_rditbl[READ_ELVSS_TEMP_1], 0);
static DEFINE_RESUI(mtp, &s6e3fa9_rditbl[READ_MTP], 0);
static DEFINE_RESUI(date, &s6e3fa9_rditbl[READ_DATE], 0);
static DEFINE_RESUI(octa_id, &s6e3fa9_rditbl[READ_OCTA_ID], 0);
/* for brightness debugging */
static DEFINE_RESUI(aor, &s6e3fa9_rditbl[READ_AOR], 0);
static DEFINE_RESUI(vint, &s6e3fa9_rditbl[READ_VINT], 0);
static DEFINE_RESUI(elvss_t, &s6e3fa9_rditbl[READ_ELVSS_T], 0);
static DEFINE_RESUI(irc, &s6e3fa9_rditbl[READ_IRC], 0);
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
static DEFINE_RESUI(copr_spi, &s6e3fa9_rditbl[READ_COPR_SPI], 0);
static DEFINE_RESUI(copr_dsi, &s6e3fa9_rditbl[READ_COPR_DSI], 0);
#endif
static DEFINE_RESUI(chip_id, &s6e3fa9_rditbl[READ_CHIP_ID], 0);
static DEFINE_RESUI(rddpm, &s6e3fa9_rditbl[READ_RDDPM], 0);
static DEFINE_RESUI(rddsm, &s6e3fa9_rditbl[READ_RDDSM], 0);
static DEFINE_RESUI(err, &s6e3fa9_rditbl[READ_ERR], 0);
static DEFINE_RESUI(err_fg, &s6e3fa9_rditbl[READ_ERR_FG], 0);
static DEFINE_RESUI(dsi_err, &s6e3fa9_rditbl[READ_DSI_ERR], 0);
static DEFINE_RESUI(self_diag, &s6e3fa9_rditbl[READ_SELF_DIAG], 0);

#ifdef CONFIG_SUPPORT_DDI_CMDLOG
static DEFINE_RESUI(cmdlog, &s6e3fa9_rditbl[READ_CMDLOG], 0);
#endif
#ifdef CONFIG_SUPPORT_POC_SPI
static DEFINE_RESUI(poc_spi_read, &s6e3fa9_rditbl[READ_POC_SPI_READ], 0);
static DEFINE_RESUI(poc_spi_status1, &s6e3fa9_rditbl[READ_POC_SPI_STATUS1], 0);
static DEFINE_RESUI(poc_spi_status2, &s6e3fa9_rditbl[READ_POC_SPI_STATUS2], 0);
#endif
#ifdef CONFIG_SUPPORT_POC_FLASH
static DEFINE_RESUI(poc_mca_chksum, &s6e3fa9_rditbl[READ_POC_MCA_CHKSUM], 0);
#endif
#ifdef CONFIG_SUPPORT_CCD_TEST
static DEFINE_RESUI(ccd_state, &s6e3fa9_rditbl[READ_CCD_STATE], 0);
#endif
#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
static DEFINE_RESUI(grayspot_cal, &s6e3fa9_rditbl[READ_GRAYSPOT_CAL], 0);
#endif

static DEFINE_RESUI(self_mask_checksum, &s6e3fa9_rditbl[READ_SELF_MASK_CHECKSUM], 0);
static DEFINE_RESUI(self_mask_crc, &s6e3fa9_rditbl[READ_SELF_MASK_CRC], 0);

static struct resinfo s6e3fa9_restbl[] = {
	[RES_ID] = RESINFO_INIT(id, S6E3FA9_ID, RESUI(id)),
	[RES_COORDINATE] = RESINFO_INIT(coordinate, S6E3FA9_COORDINATE, RESUI(coordinate)),
	[RES_CODE] = RESINFO_INIT(code, S6E3FA9_CODE, RESUI(code)),
	[RES_ELVSS] = RESINFO_INIT(elvss, S6E3FA9_ELVSS, RESUI(elvss)),
	[RES_ELVSS_TEMP_0] = RESINFO_INIT(elvss_temp_0, S6E3FA9_ELVSS_TEMP_0, RESUI(elvss_temp_0)),
	[RES_ELVSS_TEMP_1] = RESINFO_INIT(elvss_temp_1, S6E3FA9_ELVSS_TEMP_1, RESUI(elvss_temp_1)),
	[RES_MTP] = RESINFO_INIT(mtp, S6E3FA9_MTP, RESUI(mtp)),
	[RES_DATE] = RESINFO_INIT(date, S6E3FA9_DATE, RESUI(date)),
	[RES_OCTA_ID] = RESINFO_INIT(octa_id, S6E3FA9_OCTA_ID, RESUI(octa_id)),
	[RES_AOR] = RESINFO_INIT(aor, S6E3FA9_AOR, RESUI(aor)),
	[RES_VINT] = RESINFO_INIT(vint, S6E3FA9_VINT, RESUI(vint)),
	[RES_ELVSS_T] = RESINFO_INIT(elvss_t, S6E3FA9_ELVSS_T, RESUI(elvss_t)),
	[RES_IRC] = RESINFO_INIT(irc, S6E3FA9_IRC, RESUI(irc)),
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
	[RES_COPR_SPI] = RESINFO_INIT(copr_spi, S6E3FA9_COPR_SPI, RESUI(copr_spi)),
	[RES_COPR_DSI] = RESINFO_INIT(copr_dsi, S6E3FA9_COPR_DSI, RESUI(copr_dsi)),
#endif
	[RES_CHIP_ID] = RESINFO_INIT(chip_id, S6E3FA9_CHIP_ID, RESUI(chip_id)),
	[RES_RDDPM] = RESINFO_INIT(rddpm, S6E3FA9_RDDPM, RESUI(rddpm)),
	[RES_RDDSM] = RESINFO_INIT(rddsm, S6E3FA9_RDDSM, RESUI(rddsm)),
	[RES_ERR] = RESINFO_INIT(err, S6E3FA9_ERR, RESUI(err)),
	[RES_ERR_FG] = RESINFO_INIT(err_fg, S6E3FA9_ERR_FG, RESUI(err_fg)),
	[RES_DSI_ERR] = RESINFO_INIT(dsi_err, S6E3FA9_DSI_ERR, RESUI(dsi_err)),
	[RES_SELF_DIAG] = RESINFO_INIT(self_diag, S6E3FA9_SELF_DIAG, RESUI(self_diag)),
#ifdef CONFIG_SUPPORT_DDI_CMDLOG
	[RES_CMDLOG] = RESINFO_INIT(cmdlog, S6E3FA9_CMDLOG, RESUI(cmdlog)),
#endif
#ifdef CONFIG_SUPPORT_POC_SPI
	[RES_POC_SPI_READ] = RESINFO_INIT(poc_spi_read, S6E3FA9_POC_SPI_READ, RESUI(poc_spi_read)),
	[RES_POC_SPI_STATUS1] = RESINFO_INIT(poc_spi_status1, S6E3FA9_POC_SPI_STATUS1, RESUI(poc_spi_status1)),
	[RES_POC_SPI_STATUS2] = RESINFO_INIT(poc_spi_status2, S6E3FA9_POC_SPI_STATUS2, RESUI(poc_spi_status2)),
#endif
#ifdef CONFIG_SUPPORT_POC_FLASH
	[RES_POC_MCA_CHKSUM] = RESINFO_INIT(poc_mca_chksum, S6E3FA9_POC_MCA_CHKSUM, RESUI(poc_mca_chksum)),
#endif
#ifdef CONFIG_SUPPORT_CCD_TEST
	[RES_CCD_STATE] = RESINFO_INIT(ccd_state, S6E3FA9_CCD_STATE, RESUI(ccd_state)),
	[RES_CCD_CHKSUM_FAIL] = {
		.name = "ccd_chksum_fail",
		.type = CMD_TYPE_RES,
		.state = RES_UNINITIALIZED,
		.data = S6E3FA9_CCD_CHKSUM_FAIL,
		.dlen = ARRAY_SIZE(S6E3FA9_CCD_CHKSUM_FAIL),
		.resui = NULL,
		.nr_resui = 0,
	},
#endif
#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
	[RES_GRAYSPOT_CAL] = RESINFO_INIT(grayspot_cal, S6E3FA9_GRAYSPOT_CAL, RESUI(grayspot_cal)),
#endif
	[RES_SELF_MASK_CHECKSUM] = RESINFO_INIT(self_mask_checksum, S6E3FA9_SELF_MASK_CHECKSUM, RESUI(self_mask_checksum)),
	[RES_SELF_MASK_CRC] = RESINFO_INIT(self_mask_crc, S6E3FA9_SELF_MASK_CRC, RESUI(self_mask_crc)),
};

enum {
	DUMP_RDDPM = 0,
	DUMP_RDDSM,
	DUMP_ERR,
	DUMP_ERR_FG,
	DUMP_DSI_ERR,
	DUMP_SELF_DIAG,
	DUMP_SELF_MASK_CRC,
#ifdef CONFIG_SUPPORT_DDI_CMDLOG
	DUMP_CMDLOG,
#endif
};

static void show_rddpm(struct dumpinfo *info);
static void show_rddsm(struct dumpinfo *info);
static void show_err(struct dumpinfo *info);
static void show_err_fg(struct dumpinfo *info);
static void show_dsi_err(struct dumpinfo *info);
static void show_self_diag(struct dumpinfo *info);
#ifdef CONFIG_SUPPORT_DDI_CMDLOG
static void show_cmdlog(struct dumpinfo *info);
#endif
static void show_self_mask_crc(struct dumpinfo *info);

static struct dumpinfo s6e3fa9_dmptbl[] = {
	[DUMP_RDDPM] = DUMPINFO_INIT(rddpm, &s6e3fa9_restbl[RES_RDDPM], show_rddpm),
	[DUMP_RDDSM] = DUMPINFO_INIT(rddsm, &s6e3fa9_restbl[RES_RDDSM], show_rddsm),
	[DUMP_ERR] = DUMPINFO_INIT(err, &s6e3fa9_restbl[RES_ERR], show_err),
	[DUMP_ERR_FG] = DUMPINFO_INIT(err_fg, &s6e3fa9_restbl[RES_ERR_FG], show_err_fg),
	[DUMP_DSI_ERR] = DUMPINFO_INIT(dsi_err, &s6e3fa9_restbl[RES_DSI_ERR], show_dsi_err),
	[DUMP_SELF_DIAG] = DUMPINFO_INIT(self_diag, &s6e3fa9_restbl[RES_SELF_DIAG], show_self_diag),
#ifdef CONFIG_SUPPORT_DDI_CMDLOG
	[DUMP_CMDLOG] = DUMPINFO_INIT(cmdlog, &s6e3fa9_restbl[RES_CMDLOG], show_cmdlog),
#endif
	[DUMP_SELF_MASK_CRC] = DUMPINFO_INIT(self_mask_crc, &s6e3fa9_restbl[RES_SELF_MASK_CRC], show_self_mask_crc),
};

static int init_common_table(struct maptbl *tbl);
#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
static int getidx_common_maptbl(struct maptbl *tbl);
#endif
static int init_gamma_mode2_brt_table(struct maptbl *tbl);
static int getidx_gamma_mode2_brt_table(struct maptbl *tbl);
static int getidx_gm2_elvss_table(struct maptbl *tbl);
static int getidx_hbm_transition_table(struct maptbl *tbl);
static int getidx_acl_opr_table(struct maptbl *tbl);

static int init_lpm_brt_table(struct maptbl *tbl);
static int getidx_lpm_brt_table(struct maptbl *tbl);
static int getidx_lpm_on_table(struct maptbl *tbl);

#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
static void copy_dummy_maptbl(struct maptbl *tbl, u8 *dst);
#endif
static void copy_common_maptbl(struct maptbl *tbl, u8 *dst);
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
static void copy_copr_maptbl(struct maptbl *tbl, u8 *dst);
#endif
static void copy_tset_maptbl(struct maptbl *tbl, u8 *dst);
static void copy_grayspot_cal_maptbl(struct maptbl *tbl, u8 *dst);
#ifdef CONFIG_SUPPORT_XTALK_MODE
static int getidx_vgh_table(struct maptbl *tbl);
#endif
#if defined(CONFIG_SUPPORT_FAST_DISCHARGE)
static int getidx_fast_discharge_table(struct maptbl *tbl);
#endif
#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
static int init_color_blind_table(struct maptbl *tbl);
static int getidx_mdnie_scenario_maptbl(struct maptbl *tbl);
#ifdef CONFIG_SUPPORT_HMD
static int getidx_mdnie_hmd_maptbl(struct maptbl *tbl);
#endif
static int getidx_mdnie_hdr_maptbl(struct maptbl *tbl);
static int getidx_mdnie_trans_mode_maptbl(struct maptbl *tbl);
static int init_mdnie_night_mode_table(struct maptbl *tbl);
static int getidx_mdnie_night_mode_maptbl(struct maptbl *tbl);
static int init_mdnie_color_lens_table(struct maptbl *tbl);
static int getidx_color_lens_maptbl(struct maptbl *tbl);
static int init_color_coordinate_table(struct maptbl *tbl);
static int init_sensor_rgb_table(struct maptbl *tbl);
static int getidx_adjust_ldu_maptbl(struct maptbl *tbl);
static int getidx_color_coordinate_maptbl(struct maptbl *tbl);
static void copy_color_coordinate_maptbl(struct maptbl *tbl, u8 *dst);
static void copy_scr_white_maptbl(struct maptbl *tbl, u8 *dst);
static void copy_adjust_ldu_maptbl(struct maptbl *tbl, u8 *dst);
static int getidx_trans_maptbl(struct pkt_update_info *pktui);
static int getidx_mdnie_0_maptbl(struct pkt_update_info *pktui);
static int getidx_mdnie_1_maptbl(struct pkt_update_info *pktui);
static int getidx_mdnie_2_maptbl(struct pkt_update_info *pktui);
static int getidx_mdnie_scr_white_maptbl(struct pkt_update_info *pktui);
static void update_current_scr_white(struct maptbl *tbl, u8 *dst);
#endif /* CONFIG_EXYNOS_DECON_MDNIE_LITE */
#ifdef CONFIG_DYNAMIC_FREQ
static int getidx_dyn_ffc_table(struct maptbl *tbl);
#endif
#endif /* __S6E3FA9_H__ */
