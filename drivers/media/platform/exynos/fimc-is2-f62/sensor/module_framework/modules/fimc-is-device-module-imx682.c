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

#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <exynos-fimc-is-sensor.h>
#include "fimc-is-hw.h"
#include "fimc-is-core.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-dt.h"
#include "fimc-is-device-module-base.h"

#define SENSOR_IMX682_NAME		SENSOR_NAME_IMX682

#ifndef USE_VENDOR_PWR_PIN_NAME
#define RCAM_AF_VDD		"VDD_2.8V_AF"	 /* RCAM1_AFVDD_2P8 */
#define IMX682_IOVDD		"VDDIO_1.8V_CAM" /* CAM_VDDIO_1P8 : CAM_VLDO3 is used for all camera commonly */
#define IMX682_AVDD1		"VDDA_2.9V_CAM"	 /* RCAM1_AVDD1_2P9 */
#define IMX682_AVDD2		"gpio_ldo_en"	 /* RCAM1_AVDD2_1P8 */
#define IMX682_DVDD		"VDDD_1.1V_CAM"	 /* RCAM1_DVDD_1P1 */
#endif

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
#ifndef DISABLE_PDAF
static struct fimc_is_sensor_cfg config_imx682[] = {
	/* 2x2 BIN For Single Still Preview / Capture */
	FIMC_IS_SENSOR_CFG_EX(4624, 3468, 30, 0, 0, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE, EX_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 4624, 3468),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 1152,  1720),   VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF,  1152, 1720),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),

	/* 2x2 BIN For Single Still Preview / Capture */
	FIMC_IS_SENSOR_CFG_EX(4624, 2604, 30, 0, 1, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE, EX_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 4624, 2604),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 1152,  1288),   VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF,  1152, 1288),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),

	/* 2x2 BIN For Single Still Preview / Capture */
	FIMC_IS_SENSOR_CFG_EX(4000, 3000, 30, 0, 2, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE, EX_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 4000, 3000),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 1000,  1496),   VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF,  1000, 1496),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),

	/* 2x2 BIN V2H2 For HighSpeed Recording/FastAE */
	FIMC_IS_SENSOR_CFG_EX(2304, 1728, 120, 0, 3, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE, EX_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 2304, 1728),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),

	/* 2x2 BIN V2H2 For HighSpeed Recording */
	FIMC_IS_SENSOR_CFG_EX(2000, 1128, 240, 0, 4, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE, EX_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 2000, 1128),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),

	/* 2x2 BIN For Single Still Preview / Capture */
	FIMC_IS_SENSOR_CFG_EX(4624, 2604, 60, 0, 5, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE, EX_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 4624, 2604),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),

	/* Remosaic Full For Single Still Remosaic Capture */
	FIMC_IS_SENSOR_CFG_EX(9248, 6936, 12, 0, 6, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE, EX_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 9248, 6936),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 1152,  1720),   VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF,  1152, 1720),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),

	FIMC_IS_SENSOR_CFG_EX(4624, 3468, 30, 0, 7, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE, EX_LOW_RES_TETRA,
		VC_IN(0, HW_FORMAT_RAW10, 4624, 3468),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 576,  856),    VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF,  576, 856),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),

	FIMC_IS_SENSOR_CFG_EX(4624, 2604, 30, 0, 8, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE, EX_LOW_RES_TETRA,
		VC_IN(0, HW_FORMAT_RAW10, 4624, 2604),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 576, 648),     VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF,  576, 648),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
};
#else
static struct fimc_is_sensor_cfg config_imx682[] = {
	/* 2x2 BIN For Single Still Preview / Capture */
	FIMC_IS_SENSOR_CFG(4624, 3468, 30, 0, 0, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 4624, 3468),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),

	/* 2x2 BIN For Single Still Preview / Capture */
	FIMC_IS_SENSOR_CFG(4624, 2604, 30, 0, 1, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 4624, 2604),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),

	/* 2x2 BIN For Single Still Preview / Capture */
	FIMC_IS_SENSOR_CFG(4000, 3000, 30, 0, 2, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 4000, 3000),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),

	/* 2x2 BIN V2H2 For HighSpeed Recording/FastAE */
	FIMC_IS_SENSOR_CFG(2304, 1728, 120, 0, 3, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 2304, 1728),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),

	/* 2x2 BIN V2H2 For HighSpeed Recording */
	FIMC_IS_SENSOR_CFG(2000, 1128, 240, 0, 4, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 2000, 1128),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),

	/* 2x2 BIN For Single Still Preview / Capture */
	FIMC_IS_SENSOR_CFG(4624, 2604, 60, 0, 5, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 4624, 2604),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),

	/* Remosaic Full For Single Still Remosaic Capture */
	FIMC_IS_SENSOR_CFG(9248, 6936, 12, 0, 6, CSI_DATA_LANES_4, 2262, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 9248, 6936),   VC_OUT(HW_FORMAT_RAW10, VC_NOTHING,  0, 0),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0),       VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
};
#endif

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_module_init,
	.g_ctrl = sensor_module_g_ctrl,
	.s_ctrl = sensor_module_s_ctrl,
	.g_ext_ctrls = sensor_module_g_ext_ctrls,
	.s_ext_ctrls = sensor_module_s_ext_ctrls,
	.ioctl = sensor_module_ioctl,
	.log_status = sensor_module_log_status,
};

static const struct v4l2_subdev_video_ops video_ops = {
	.s_routing = sensor_module_s_routing,
	.s_stream = sensor_module_s_stream,
	.s_parm = sensor_module_s_param
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = sensor_module_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

static int sensor_imx682_power_setpin(struct device *dev,
		struct exynos_platform_fimc_is_module *pdata)
{
	struct fimc_is_core *core;
	struct device_node *dnode = dev->of_node;
	int gpio_mclk = 0;
	int gpio_reset = 0;
	int gpio_none = 0;
	int gpio_ldo_en = 0;

	FIMC_BUG(!dev);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core is NULL");
		return -EINVAL;
	}

	dev_info(dev, "%s E v4\n", __func__);

	gpio_mclk = of_get_named_gpio(dnode, "gpio_mclk", 0);
	if (!gpio_is_valid(gpio_mclk)) {
		dev_err(dev, "failed to get gpio_mclk\n");
		return -EINVAL;
	} else {
		gpio_request_one(gpio_mclk, GPIOF_OUT_INIT_LOW, "CAM_MCLK_OUTPUT_LOW");
		gpio_free(gpio_mclk);
	}

	gpio_reset = of_get_named_gpio(dnode, "gpio_reset", 0);
	if (!gpio_is_valid(gpio_reset)) {
		dev_err(dev, "failed to get PIN_RESET\n");
		return -EINVAL;
	}else{
		gpio_request_one(gpio_reset, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
		gpio_free(gpio_reset);
	}

	gpio_ldo_en = of_get_named_gpio(dnode, "gpio_ldo_en", 0);
	if (!gpio_is_valid(gpio_ldo_en)) {
		dev_warn(dev, "failed to %s\n", "gpio_ldo_en");
		return -EINVAL;
	} else {
		gpio_request_one(gpio_ldo_en, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
		gpio_free(gpio_ldo_en);
	}

	SET_PIN_INIT(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_ON);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_OFF);

	/* Normal on */
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON,  gpio_reset, SENSOR_RESET_LOW,  PIN_OUTPUT,    0, 500);

	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON,  gpio_none,  IMX682_AVDD1,      PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON,  gpio_ldo_en, IMX682_AVDD2,     PIN_OUTPUT, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON,  gpio_none,  IMX682_IOVDD,      PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON,  gpio_none,  IMX682_DVDD,       PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON,  gpio_none,  RCAM_AF_VDD,       PIN_REGULATOR, 1, 500);

	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON,  gpio_none,  "pin",   PIN_FUNCTION,  2, 500);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON,  gpio_none,  "MCLK",   PIN_MCLK,  1, 1500);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON,  gpio_none,  "on_i2c", PIN_I2C, 1, 0);
	
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON,  gpio_reset, SENSOR_RESET_HIGH, PIN_OUTPUT,    1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON,  gpio_none,  SENSOR_SET_DELAY,  PIN_NONE,      0, 15000);

	/* Normal off */
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none,  SENSOR_SET_DELAY,  PIN_NONE,      0, 5000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_reset, SENSOR_RESET_LOW,  PIN_OUTPUT,    0, 0);
	
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none,  "off_i2c", PIN_I2C, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none,  "MCLK",   PIN_MCLK,  0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none,  "pin",   PIN_FUNCTION,  0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none,  "pin",   PIN_FUNCTION,  1, 500);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none,  "pin",   PIN_FUNCTION,  0, 0);

	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none,  RCAM_AF_VDD,       PIN_REGULATOR, 0, 500);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none,  IMX682_AVDD1,      PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_ldo_en, IMX682_AVDD2,     PIN_OUTPUT, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none,  IMX682_IOVDD,      PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none,  IMX682_DVDD,       PIN_REGULATOR, 0, 0);

	/* READ_ROM - POWER ON */
	SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_ON,	gpio_none, IMX682_IOVDD,	  PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_ON,	gpio_none, RCAM_AF_VDD,		  PIN_REGULATOR, 1, 500);
	SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_ON,	gpio_none, "on_i2c", PIN_I2C, 1, 10);
	SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_ON,	gpio_none, SENSOR_SET_DELAY,  PIN_NONE, 0, 15000);
	/* READ_ROM - POWER OFF */
	SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_OFF, gpio_none, "off_i2c", PIN_I2C, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_OFF, gpio_none, RCAM_AF_VDD, 	  PIN_REGULATOR, 0, 500);
	SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_OFF, gpio_none, IMX682_IOVDD,	  PIN_REGULATOR, 0, 20000);
	dev_info(dev, "%s X v4\n", __func__);

	return 0;
}

static int __init sensor_module_imx682_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor *device;
	struct sensor_open_extended *ext;
	struct exynos_platform_fimc_is_module *pdata;
	struct device *dev;
	struct pinctrl_state *s;
	int t;

	FIMC_BUG(!fimc_is_dev);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_info("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	dev = &pdev->dev;

	fimc_is_module_parse_dt(dev, sensor_imx682_power_setpin);

	pdata = dev_get_platdata(dev);
	device = &core->sensor[pdata->id];

	subdev_module = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_module) {
		probe_err("subdev_module is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	probe_info("%s pdta->id(%d), module_enum id = %d \n", __func__, pdata->id, atomic_read(&device->module_count));
	module = &device->module_enum[atomic_read(&device->module_count)];
	atomic_inc(&device->module_count);
	clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
	module->pdata = pdata;
	module->dev = dev;
	module->sensor_id = SENSOR_NAME_IMX682;
	module->subdev = subdev_module;
	module->device = pdata->id;
	module->client = NULL;
	module->active_width = 9248;
	module->active_height = 6936;
	module->margin_left = 0;
	module->margin_right = 0;
	module->margin_top = 0;
	module->margin_bottom = 0;
	module->pixel_width = module->active_width + 0;
	module->pixel_height = module->active_height + 0;
	module->max_framerate = 247;
	module->position = pdata->position;
	module->bitwidth = 10;
	module->sensor_maker = "SONY";
	module->sensor_name = "IMX682";
	module->setfile_name = "setfile_imx682.bin";
	module->cfgs = sizeof(config_imx682) / sizeof(config_imx682[0]);
	module->cfg = config_imx682;
	module->ops = NULL;

#if 1
	for (t = VC_BUF_DATA_TYPE_SENSOR_STAT1; t < VC_BUF_DATA_TYPE_MAX; t++) {
		module->vc_extra_info[t].stat_type = VC_STAT_TYPE_INVALID;
		module->vc_extra_info[t].sensor_mode = VC_SENSOR_MODE_INVALID;
		module->vc_extra_info[t].max_width = 0;
		module->vc_extra_info[t].max_height = 0;
		module->vc_extra_info[t].max_element = 0;

/* HACK: disable tail mode with PDP */
#if 0
		if (IS_ENABLED(CONFIG_CAMERA_PDP)) {
			switch (t) {
			case VC_BUF_DATA_TYPE_SENSOR_STAT1:
				if (!IS_ENABLED(CONFIG_SOC_EXYNOS9820) ||
						IS_ENABLED(CONFIG_SOC_EXYNOS9820_EVT0)) {
					module->vc_extra_info[t].stat_type
						= VC_STAT_TYPE_TAIL_FOR_SW_PDAF;
					module->vc_extra_info[t].sensor_mode
						= VC_SENSOR_MODE_MSPD_GLOBAL_TAIL;
					module->vc_extra_info[t].max_width = 128;
					module->vc_extra_info[t].max_height = 736;
					module->vc_extra_info[t].max_element = 2;
				}
				break;
			case VC_BUF_DATA_TYPE_GENERAL_STAT1:
				/* PDP STAT0: SFR, Size: 1756(1756)Bytes */
				if (IS_ENABLED(CONFIG_SOC_EXYNOS9820) &&
						!IS_ENABLED(CONFIG_SOC_EXYNOS9820_EVT0)) {
					module->vc_extra_info[t].stat_type
						= VC_STAT_TYPE_PDP_1_1_PDAF_STAT0;
					module->vc_extra_info[t].sensor_mode
						= VC_SENSOR_MODE_MSPD_GLOBAL_TAIL;
					module->vc_extra_info[t].max_width = 1756;
					module->vc_extra_info[t].max_height = 1;
					module->vc_extra_info[t].max_element = 1;
				}
				break;
			case VC_BUF_DATA_TYPE_GENERAL_STAT2:
				/* PDP STAT1/2: DMA, Size: 17920+21504(17556+21168)Bytes */
				if (IS_ENABLED(CONFIG_SOC_EXYNOS9820) &&
						!IS_ENABLED(CONFIG_SOC_EXYNOS9820_EVT0)) {
					module->vc_extra_info[t].stat_type
						= VC_STAT_TYPE_PDP_1_1_PDAF_STAT1;
					module->vc_extra_info[t].sensor_mode
						= VC_SENSOR_MODE_MSPD_GLOBAL_TAIL;
					module->vc_extra_info[t].max_width = 39424;
					module->vc_extra_info[t].max_height = 1;
					module->vc_extra_info[t].max_element = 1;
				}
				break;
			}
		} else {
			switch (t) {
			case VC_BUF_DATA_TYPE_SENSOR_STAT1:
				module->vc_extra_info[t].stat_type
					= VC_STAT_TYPE_TAIL_FOR_SW_PDAF;
				module->vc_extra_info[t].sensor_mode
					= VC_SENSOR_MODE_MSPD_GLOBAL_TAIL;
				module->vc_extra_info[t].max_width = 128;
				module->vc_extra_info[t].max_height = 736;
				module->vc_extra_info[t].max_element = 2;
				break;
			}
		}
#else
		switch (t) {
			case VC_BUF_DATA_TYPE_SENSOR_STAT1:
				module->vc_extra_info[t].stat_type
					= VC_STAT_TYPE_TAIL_FOR_SW_PDAF;
				module->vc_extra_info[t].sensor_mode
					= VC_SENSOR_MODE_MSPD_GLOBAL_TAIL;
				module->vc_extra_info[t].max_width = 1152;
				module->vc_extra_info[t].max_height = 1720;
				module->vc_extra_info[t].max_element = 2;
				break;
		}
#endif
	}
#endif

	/* Sensor peri */
	module->private_data = kzalloc(sizeof(struct fimc_is_device_sensor_peri), GFP_KERNEL);
	if (!module->private_data) {
		probe_err("fimc_is_device_sensor_peri is NULL");
		ret = -ENOMEM;
		goto p_err;
	}
	fimc_is_sensor_peri_probe((struct fimc_is_device_sensor_peri *)module->private_data);
	PERI_SET_MODULE(module);

	ext = &module->ext;
	ext->sensor_con.product_name = module->sensor_id /*SENSOR_NAME_IMX682*/;
	ext->sensor_con.peri_type = SE_I2C;
	ext->sensor_con.peri_setting.i2c.channel = pdata->sensor_i2c_ch;
	ext->sensor_con.peri_setting.i2c.slave_address = pdata->sensor_i2c_addr;
	ext->sensor_con.peri_setting.i2c.speed = 1000000;

	ext->actuator_con.product_name = ACTUATOR_NAME_NOTHING;
	ext->flash_con.product_name = FLADRV_NAME_NOTHING;
	ext->from_con.product_name = FROMDRV_NAME_NOTHING;
	ext->preprocessor_con.product_name = PREPROCESSOR_NAME_NOTHING;
	ext->ois_con.product_name = OIS_NAME_NOTHING;

	if (pdata->af_product_name !=  ACTUATOR_NAME_NOTHING) {
		ext->actuator_con.product_name = pdata->af_product_name;
		ext->actuator_con.peri_type = SE_I2C;
		ext->actuator_con.peri_setting.i2c.channel = pdata->af_i2c_ch;
		ext->actuator_con.peri_setting.i2c.slave_address = pdata->af_i2c_addr;
		ext->actuator_con.peri_setting.i2c.speed = 400000;
	}

	if (pdata->flash_product_name != FLADRV_NAME_NOTHING) {
		ext->flash_con.product_name = pdata->flash_product_name;
		ext->flash_con.peri_type = SE_GPIO;
		ext->flash_con.peri_setting.gpio.first_gpio_port_no = pdata->flash_first_gpio;
		ext->flash_con.peri_setting.gpio.second_gpio_port_no = pdata->flash_second_gpio;
	}

	if (pdata->preprocessor_product_name != PREPROCESSOR_NAME_NOTHING) {
		ext->preprocessor_con.product_name = pdata->preprocessor_product_name;
		ext->preprocessor_con.peri_info0.valid = true;
		ext->preprocessor_con.peri_info0.peri_type = SE_SPI;
		ext->preprocessor_con.peri_info0.peri_setting.spi.channel = pdata->preprocessor_spi_channel;
		ext->preprocessor_con.peri_info1.valid = true;
		ext->preprocessor_con.peri_info1.peri_type = SE_I2C;
		ext->preprocessor_con.peri_info1.peri_setting.i2c.channel = pdata->preprocessor_i2c_ch;
		ext->preprocessor_con.peri_info1.peri_setting.i2c.slave_address = pdata->preprocessor_i2c_addr;
		ext->preprocessor_con.peri_info1.peri_setting.i2c.speed = 400000;
		ext->preprocessor_con.peri_info2.valid = true;
		ext->preprocessor_con.peri_info2.peri_type = SE_DMA;
		if (pdata->preprocessor_dma_channel == DMA_CH_NOT_DEFINED)
			ext->preprocessor_con.peri_info2.peri_setting.dma.channel = FLITE_ID_D;
		else
			ext->preprocessor_con.peri_info2.peri_setting.dma.channel = pdata->preprocessor_dma_channel;
	}

	if (pdata->ois_product_name != OIS_NAME_NOTHING) {
		ext->ois_con.product_name = pdata->ois_product_name;
		ext->ois_con.peri_type = SE_I2C;
		ext->ois_con.peri_setting.i2c.channel = pdata->ois_i2c_ch;
		ext->ois_con.peri_setting.i2c.slave_address = pdata->ois_i2c_addr;
		ext->ois_con.peri_setting.i2c.speed = 400000;
	} else {
		ext->ois_con.product_name = pdata->ois_product_name;
		ext->ois_con.peri_type = SE_NULL;
	}

	v4l2_subdev_init(subdev_module, &subdev_ops);

	v4l2_set_subdevdata(subdev_module, module);
	v4l2_set_subdev_hostdata(subdev_module, device);
	snprintf(subdev_module->name, V4L2_SUBDEV_NAME_SIZE, "sensor-subdev.%d", module->sensor_id);

	s = pinctrl_lookup_state(pdata->pinctrl, "release");

	if (pinctrl_select_state(pdata->pinctrl, s) < 0) {
		probe_err("pinctrl_select_state is fail\n");
		goto p_err;
	}
p_err:
	probe_info("%s(%d)\n", __func__, ret);
	return ret;
}

static const struct of_device_id exynos_fimc_is_sensor_module_imx682_match[] = {
	{
		.compatible = "samsung,sensor-module-imx682",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_sensor_module_imx682_match);

static struct platform_driver sensor_module_imx682_driver = {
	.driver = {
		.name   = "FIMC-IS-SENSOR-MODULE-IMX682",
		.owner  = THIS_MODULE,
		.of_match_table = exynos_fimc_is_sensor_module_imx682_match,
	}
};

static int __init fimc_is_sensor_module_imx682_init(void)
{
	int ret;

	ret = platform_driver_probe(&sensor_module_imx682_driver,
				sensor_module_imx682_probe);
	if (ret)
		err("failed to probe %s driver: %d\n",
			sensor_module_imx682_driver.driver.name, ret);

	return ret;
}
late_initcall(fimc_is_sensor_module_imx682_init);

