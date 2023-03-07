/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_CIS_IMX682_H
#define FIMC_IS_CIS_IMX682_H

#include "fimc-is-cis.h"

#define EXT_CLK_Mhz (26)
#define SENSOR_IMX682_240FPS_FRAME_DURATION_US    (4166)

#define SENSOR_IMX682_MAX_WIDTH          (9248 + 0)
#define SENSOR_IMX682_MAX_HEIGHT         (6936 + 0)

/* Related Sensor Parameter */
#define USE_GROUP_PARAM_HOLD                      (1)
#define TOTAL_NUM_OF_IVTPX_CHANNEL                (8)

#define SENSOR_IMX682_FINE_INTEGRATION_TIME                    (6320)    //FINE_INTEG_TIME is a fixed value (0x0200: 16bit - read value is 0x18b0)
#define SENSOR_IMX682_COARSE_INTEGRATION_TIME_MIN              (9)
#define SENSOR_IMX682_COARSE_INTEGRATION_TIME_MIN_FOR_PDAF     (6)
#define SENSOR_IMX682_COARSE_INTEGRATION_TIME_MIN_FOR_V2H2     (8)
#define SENSOR_IMX682_COARSE_INTEGRATION_TIME_MAX_MARGIN       (48)
#define SENSOR_IMX682_MAX_COARSE_INTEG_WITH_FRM_LENGTH_CTRL    (65503)
#define SENSOR_IMX682_MAX_CIT_LSHIFT_VALUE                     (0x7)

#define SENSOR_IMX682_OTP_PAGE_SETUP_ADDR         (0x0A02)
#define SENSOR_IMX682_OTP_READ_TRANSFER_MODE_ADDR (0x0A00)
#define SENSOR_IMX682_OTP_STATUS_REGISTER_ADDR    (0x0A01)
#define SENSOR_IMX682_OTP_CHIP_REVISION_ADDR      (0x0018)

#define SENSOR_IMX682_MODEL_ID_ADDR               (0x0000)
#define SENSOR_IMX682_REVISION_NUM_ADDR           (0x0002)
#define SENSOR_IMX682_FRAME_COUNT_ADDR            (0x0005)
#define SENSOR_IMX682_SETUP_MODE_SELECT_ADDR      (0x0100)
#define SENSOR_IMX682_GROUP_PARAM_HOLD_ADDR       (0x0104)

#define SENSOR_IMX682_CIT_LSHIFT_ADDR             (0x3100)
#define SENSOR_IMX682_FRAME_LENGTH_LINE_ADDR      (0x0340)
#define SENSOR_IMX682_LINE_LENGTH_PCK_ADDR        (0x0342)
#define SENSOR_IMX682_FINE_INTEG_TIME_ADDR        (0x0200)
#define SENSOR_IMX682_COARSE_INTEG_TIME_ADDR      (0x0202)
#define SENSOR_IMX682_ANALOG_GAIN_ADDR            (0x0204)
#define SENSOR_IMX682_DIG_GAIN_ADDR               (0x020E)

#define SENSOR_IMX682_MIN_ANALOG_GAIN_SET_VALUE   (0)
#define SENSOR_IMX682_MAX_ANALOG_GAIN_SET_VALUE   (1008)
#define SENSOR_IMX682_MIN_DIGITAL_GAIN_SET_VALUE  (0x0100)
#define SENSOR_IMX682_MAX_DIGITAL_GAIN_SET_VALUE  (0x0FFF)

#define SENSOR_IMX682_ABS_GAIN_GR_SET_ADDR        (0x0B8E)
#define SENSOR_IMX682_ABS_GAIN_R_SET_ADDR         (0x0B90)
#define SENSOR_IMX682_ABS_GAIN_B_SET_ADDR         (0x0B92)
#define SENSOR_IMX682_ABS_GAIN_GB_SET_ADDR        (0x0B94)

/* Related EEPROM CAL */
#define SENSOR_IMX682_LRC_CAL_BASE_REAR           (0x34D4)
#define SENSOR_IMX682_LRC_CAL_SIZE                (504)
#define SENSOR_IMX682_LRC_REG_ADDR1               (0x7b00)
#define SENSOR_IMX682_LRC_REG_ADDR2               (0x7c00)

#define SENSOR_IMX682_QUAD_SENS_CAL_BASE_REAR     (0x2900)
#define SENSOR_IMX682_QUAD_SENS_CAL_SIZE          (3024)
#define SENSOR_IMX682_QUAD_SENS_REG_ADDR          (0xCA00)

#define SENSOR_IMX682_EBD_CONTROL_ADDR            (0x41D0)

#define SENSOR_IMX682_LRC_DUMP_NAME               "/data/vendor/camera/IMX682_LRC_DUMP.bin"
#define SENSOR_IMX682_QSC_DUMP_NAME               "/data/vendor/camera/IMX682_QSC_DUMP.bin"

/* Related Function Option */
#define SENSOR_IMX682_WRITE_PDAF_CAL              (1)
#define SENSOR_IMX682_WRITE_SENSOR_CAL            (1)
#define SENSOR_IMX682_CAL_DEBUG                   (0)
#define SENSOR_IMX682_DEBUG_INFO                  (0)

/*
 * [Mode Information]
 *
 * Reference File : IMX682_SEC-DPHY-26MHz_RegisterSetting_ver4.00-4.00_b4_MP_200706.xlsx
 * Update Data    : 2020-07-07
 * Author         : abhishek.77
 *
 * - Global Setting -
 * 
 * - 2x2 BIN For Still Preview / Capture -
 *    [ 0 ] 2Bin_A         : 2x2 Binning mode A 4624x3468 30fps       : Single Still Preview/Capture (4:3)    ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2262
 *    [ 1 ] 2Bin_B         : 2x2 Binning mode B 4624x2604 30fps       : Single Still Preview/Capture (16:9)   ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2262
 *    [ 2 ] 2Bin_D         : 2x2 Binning mode D 4000x3000 30fps       : Single Still Preview/Capture (4:3)    ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2262
 * 
 * - 2x2 BIN V2H2 For HighSpeed Recording/FastAE-
 *    [ 3 ] V2H2_FAE       : 2x2 Binning mode V2H2 2304X1728 120fps   : FAST AE (4:3)                         ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2262
 *
 * - 2x2 BIN V2H2 For HighSpeed Recording -
 *    [ 4 ] V2H2_SSL_3      : 2x2 Binning mode V2H2 2000X1128 240fps   : High Speed Recording (16:9)           ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2262
 * 
 * - 2x2 BIN FHD Recording
 *    [ 5 ] 2Bin_C          : 2x2 Binning V2H2 4624x2604  60fps        : FHD Recording (16:9)                  ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2262
 *
 * - Remosaic For Single Still Remosaic Capture -
 *    [ 6 ] FULL            : Remosaic Full 9248x6936 12fps            : Single Still Remosaic Capture (4:3)   ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2262
 *    [ 7 ] FULL            : Remosaic Crop 4624x3468 30fps            : Single Still Remosaic Capture (4:3)   ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2262
 *    [ 8 ] FULL            : Remosaic Crop 4624x2604 30fps            : Single Still Remosaic Capture (16:9)  ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2262
 */

enum sensor_imx682_mode_enum {

	/* 2x2 Binning 30Fps */
	SENSOR_IMX682_2X2BIN_FULL_4264X3468_30FPS,
	SENSOR_IMX682_2X2BIN_FULL_4264X2604_30FPS,
	SENSOR_IMX682_2X2BIN_FULL_4000X3000_30FPS,

	/* 2X2 Binning V2H2 120FPS */
	SENSOR_IMX682_2X2BIN_V2H2_2304X1728_120FPS,

	/* 2X2 Binning V2H2 240FPS */
	SENSOR_IMX682_2X2BIN_V2H2_2000X1128_240FPS,
	
	/* 2X2 Binning FHD Recording*/
	SENSOR_IMX682_2X2BIN_FULL_4264X2604_60FPS,

	/* Remosaic */
	IMX682_MODE_REMOSAIC_START,
	SENSOR_IMX682_REMOSAIC_FULL_9248x6932_12FPS = IMX682_MODE_REMOSAIC_START,
	SENSOR_IMX682_REMOSAIC_CROP_4624x3468_30FPS,
	SENSOR_IMX682_REMOSAIC_CROP_4624x2604_30FPS,
	IMX682_MODE_REMOSAIC_END = SENSOR_IMX682_REMOSAIC_CROP_4624x2604_30FPS,
};

/*
 *    CHIP Revision Confirmation
 *
 *                            Fab1                            Fab2            
 *****************************************************************************
 *    I2C Address             0x01h    0x02h   0x1Xh          0x02h    0x1Xh  
 *    0x001  
 *****************************************************************************
 *    Register Ver            Ver3.0   Ver3.0  Ver3.0         Ver3.0   Ver3.0 
 */

enum sensor_imx682_chip_id_ver_enum {
	SENSOR_IMX682_CHIP_ID_VER_3_X_0x01 = 0x01,
	SENSOR_IMX682_CHIP_ID_VER_3_X_0x02 = 0x02,
	SENSOR_IMX682_CHIP_ID_VER_3_X_0x1X = 0x10,
};

#endif


/* IS_REMOSAIC_MODE(struct fimc_is_cis *cis) */
#define IS_REMOSAIC(mode) ({						\
	typecheck(u32, mode) && (mode >= IMX682_MODE_REMOSAIC_START) &&	\
	(mode <= IMX682_MODE_REMOSAIC_END);				\
})

#define IS_REMOSAIC_MODE(cis) ({					\
	u32 m;								\
	typecheck(struct fimc_is_cis *, cis);				\
	m = cis->cis_data->sens_config_index_cur;			\
	(m >= IMX682_MODE_REMOSAIC_START) && (m <= IMX682_MODE_REMOSAIC_END); \
})

#define IS_SEAMLESS_MODE_CHANGE(cis) ({					\
	u32 m;								\
	typecheck(struct fimc_is_cis *, cis);				\
	m = cis->cis_data->sens_config_index_cur;			\
	(m == SENSOR_IMX682_REMOSAIC_CROP_4624x3468_30FPS		\
	|| m == SENSOR_IMX682_REMOSAIC_CROP_4624x2604_30FPS);		\
})
