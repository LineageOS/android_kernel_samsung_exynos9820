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
#include <linux/of_gpio.h>
#include <linux/syscalls.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <exynos-fimc-is-sensor.h>
#include "fimc-is-hw.h"
#include "fimc-is-core.h"
#include "fimc-is-param.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-dt.h"
#include "fimc-is-cis-imx682.h"
#include "fimc-is-cis-imx682-setA.h"
#include "fimc-is-cis-imx682-setB.h"

#include "fimc-is-helper-i2c.h"
#include "fimc-is-sec-define.h"

#include "interface/fimc-is-interface-library.h"
#include "fimc-is-interface-sensor.h"
#define SENSOR_NAME "IMX682"

static u8 coarse_integ_step_value;

static const u32 *sensor_imx682_global;
static u32 sensor_imx682_global_size;
static const u32 **sensor_imx682_setfiles;
static const u32 *sensor_imx682_setfile_sizes;
static u32 sensor_imx682_max_setfile_num;
static const struct sensor_pll_info_compact **sensor_imx682_pllinfos;
#ifdef DISABLE_PDAF
static const u32 *sensor_imx682_pdaf_off_setfile;
static u32 sensor_imx682_pdaf_off_size;
#endif
static bool sensor_imx682_cal_write_flag;
static bool sensor_imx682_stream_on_state;

extern struct fimc_is_lib_support gPtr_lib_support;

int sensor_imx682_cis_check_rev(struct fimc_is_cis *cis)
{
	int ret = 0;
	u8 status = 0;
	u8 rev = 0;
	struct i2c_client *client;

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);
	/* Specify OTP Page Address for READ - Page127(dec) */
	fimc_is_sensor_write8(client, SENSOR_IMX682_OTP_PAGE_SETUP_ADDR, 0x7F);

	/* Turn ON OTP Read MODE */
	fimc_is_sensor_write8(client, SENSOR_IMX682_OTP_READ_TRANSFER_MODE_ADDR,  0x01);

	/* Check status - 0x01 : read ready*/
	fimc_is_sensor_read8(client, SENSOR_IMX682_OTP_STATUS_REGISTER_ADDR,  &status);
	if ((status & 0x1) == false)
		err("status fail, (%d)", status);

	/* CHIP REV 0x0018 */
	ret = fimc_is_sensor_read8(client, SENSOR_IMX682_OTP_CHIP_REVISION_ADDR, &rev);
	if (ret < 0) {
		err("fimc_is_sensor_read8 fail (ret %d)", ret);
		I2C_MUTEX_UNLOCK(cis->i2c_lock);
		return ret;
	}
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	cis->cis_data->cis_rev = rev;

	probe_info("imx682 rev:%#x", rev);

	return 0;
}

static void sensor_imx682_set_integration_max_margin(u32 mode, cis_shared_data *cis_data)
{
	FIMC_BUG_VOID(!cis_data);

	cis_data->max_margin_coarse_integration_time = SENSOR_IMX682_COARSE_INTEGRATION_TIME_MAX_MARGIN;
	dbg_sensor(1, "max_margin_coarse_integration_time(%d)\n",
		cis_data->max_margin_coarse_integration_time);
}

static void sensor_imx682_set_integration_min(u32 mode, cis_shared_data *cis_data)
{
	FIMC_BUG_VOID(!cis_data);

	switch (mode) {
	case SENSOR_IMX682_REMOSAIC_FULL_9248x6932_12FPS:
	case SENSOR_IMX682_REMOSAIC_CROP_4624x3468_30FPS:
	case SENSOR_IMX682_REMOSAIC_CROP_4624x2604_30FPS:
		cis_data->min_coarse_integration_time = SENSOR_IMX682_COARSE_INTEGRATION_TIME_MIN;
		dbg_sensor(1, "min_coarse_integration_time(%d)\n",
			cis_data->min_coarse_integration_time);
		break;
	case SENSOR_IMX682_2X2BIN_FULL_4264X3468_30FPS:
	case SENSOR_IMX682_2X2BIN_FULL_4264X2604_30FPS:
	case SENSOR_IMX682_2X2BIN_FULL_4000X3000_30FPS:
	case SENSOR_IMX682_2X2BIN_FULL_4264X2604_60FPS:
		cis_data->min_coarse_integration_time = SENSOR_IMX682_COARSE_INTEGRATION_TIME_MIN_FOR_PDAF;
		dbg_sensor(1, "min_coarse_integration_time(%d)\n",
			cis_data->min_coarse_integration_time);
		break;
	case SENSOR_IMX682_2X2BIN_V2H2_2304X1728_120FPS:
	case SENSOR_IMX682_2X2BIN_V2H2_2000X1128_240FPS:
		cis_data->min_coarse_integration_time = SENSOR_IMX682_COARSE_INTEGRATION_TIME_MIN_FOR_V2H2;
		dbg_sensor(1, "min_coarse_integration_time(%d)\n",
			cis_data->min_coarse_integration_time);
		break;
	default:
		err("[%s] Unsupport imx682 sensor mode\n", __func__);
		cis_data->min_coarse_integration_time = SENSOR_IMX682_COARSE_INTEGRATION_TIME_MIN;
		dbg_sensor(1, "min_coarse_integration_time(%d)\n",
			cis_data->min_coarse_integration_time);
		break;
	}
}

static void sensor_imx682_set_integration_step_value(u32 mode)
{
	switch (mode) {
	case SENSOR_IMX682_REMOSAIC_FULL_9248x6932_12FPS:
	case SENSOR_IMX682_REMOSAIC_CROP_4624x3468_30FPS:
	case SENSOR_IMX682_REMOSAIC_CROP_4624x2604_30FPS:
	case SENSOR_IMX682_2X2BIN_FULL_4264X3468_30FPS:
	case SENSOR_IMX682_2X2BIN_FULL_4264X2604_30FPS:
	case SENSOR_IMX682_2X2BIN_FULL_4000X3000_30FPS:
	case SENSOR_IMX682_2X2BIN_FULL_4264X2604_60FPS:
		coarse_integ_step_value = 1;
		dbg_sensor(1, "coarse_integration step value(%d)\n",
			coarse_integ_step_value);
		break;
	case SENSOR_IMX682_2X2BIN_V2H2_2304X1728_120FPS:
	case SENSOR_IMX682_2X2BIN_V2H2_2000X1128_240FPS:
		coarse_integ_step_value = 2;
		dbg_sensor(1, "coarse_integration step value(%d)\n",
			coarse_integ_step_value);
		break;
	default:
		err("[%s] Unsupport imx682 sensor mode\n", __func__);
		coarse_integ_step_value = 1;
		dbg_sensor(1, "coarse_integration step value(%d)\n",
			coarse_integ_step_value);
		break;
	}
}

static void sensor_imx682_cis_data_calculation(const struct sensor_pll_info_compact *pll_info, cis_shared_data *cis_data)
{
	u32 total_pixels = 0;
	u32 pixel_rate = 0;
	u32 frame_rate = 0, max_fps = 0, frame_valid_us = 0;

	FIMC_BUG_VOID(!pll_info);

	/* 1. get pclk value from pll info */
	pixel_rate = pll_info->pclk * TOTAL_NUM_OF_IVTPX_CHANNEL;
	total_pixels = pll_info->frame_length_lines * pll_info->line_length_pck;

	/* 2. FPS calculation */
	frame_rate = pixel_rate / total_pixels;

	/* 3. the time of processing one frame calculation (us) */
	cis_data->min_frame_us_time = (1 * 1000 * 1000) / frame_rate;
	cis_data->cur_frame_us_time = cis_data->min_frame_us_time;
#ifdef CAMERA_REAR2
	cis_data->min_sync_frame_us_time = cis_data->min_frame_us_time;
#endif

	dbg_sensor(1, "frame_duration(%d) - frame_rate (%d) = pixel_rate(%u) / "
		KERN_CONT "(pll_info->frame_length_lines(%d) * pll_info->line_length_pck(%d))\n",
		cis_data->min_frame_us_time, frame_rate, pixel_rate, pll_info->frame_length_lines, pll_info->line_length_pck);

	/* calculate max fps */
	max_fps = (pixel_rate * 10) / (pll_info->frame_length_lines * pll_info->line_length_pck);
	max_fps = (max_fps % 10 >= 5 ? frame_rate + 1 : frame_rate);

	cis_data->pclk = pixel_rate;
	cis_data->max_fps = max_fps;
	cis_data->frame_length_lines = pll_info->frame_length_lines;
	cis_data->line_length_pck = pll_info->line_length_pck;
	cis_data->line_readOut_time = sensor_cis_do_div64((u64)cis_data->line_length_pck * (u64)(1000 * 1000 * 1000), cis_data->pclk);
	cis_data->rolling_shutter_skew = (cis_data->cur_height - 1) * cis_data->line_readOut_time;
	cis_data->max_coarse_integration_time =
		SENSOR_IMX682_MAX_COARSE_INTEG_WITH_FRM_LENGTH_CTRL - SENSOR_IMX682_COARSE_INTEGRATION_TIME_MAX_MARGIN;

	if(!sensor_imx682_stream_on_state) {
		cis_data->stream_on = false;
	}

	/* Frame valid time calculation */
	frame_valid_us = sensor_cis_do_div64((u64)cis_data->cur_height * (u64)cis_data->line_length_pck * (u64)(1000 * 1000), cis_data->pclk);
	cis_data->frame_valid_us_time = (int)frame_valid_us;

	dbg_sensor(1, "%s\n", __func__);
	dbg_sensor(1, "Sensor size(%d x %d) setting : SUCCESS!\n", cis_data->cur_width, cis_data->cur_height);
	dbg_sensor(1, "Frame Valid(%d us)\n", frame_valid_us);
	dbg_sensor(1, "rolling_shutter_skew : %lld\n", cis_data->rolling_shutter_skew);

	dbg_sensor(1, "Fps : %d, max fps(%d)\n", frame_rate, cis_data->max_fps);
	dbg_sensor(1, "min_frame_time(%d us)\n", cis_data->min_frame_us_time);
	dbg_sensor(1, "Pixel rate(%d Khz)\n", cis_data->pclk / 1000);

	/* Frame period calculation */
	cis_data->frame_time = (cis_data->line_readOut_time * cis_data->cur_height / 1000);
	cis_data->rolling_shutter_skew = (cis_data->cur_height - 1) * cis_data->line_readOut_time;

	dbg_sensor(1, "[%s] frame_time(%d), rolling_shutter_skew(%lld)\n", __func__,
	cis_data->frame_time, cis_data->rolling_shutter_skew);

	/* Constant values */
	cis_data->min_fine_integration_time = SENSOR_IMX682_FINE_INTEGRATION_TIME;
	cis_data->max_fine_integration_time = SENSOR_IMX682_FINE_INTEGRATION_TIME;
	info("%s: done", __func__);
}

#if SENSOR_IMX682_CAL_DEBUG
int sensor_imx682_cis_cal_dump(char* name, char *buf, size_t size)
{
	int ret = 0;

	struct file *fp;
	ssize_t tx = -ENOENT;
	int fd, old_mask;
	loff_t pos = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	old_mask = sys_umask(0);

	sys_rmdir(name);
	fd = sys_open(name, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0666);
	if (fd < 0) {
		err("open file error(%d) : %s", fd, name);
		sys_umask(old_mask);
		set_fs(old_fs);
		ret = -EINVAL;
		goto p_err;
	}

	fp = fget(fd);
	if (fp) {
		tx = vfs_write(fp, buf, size, &pos);
		if (tx != size) {
			err("fail to write %s. ret %zd", name, tx);
			ret = -ENOENT;
		}

		vfs_fsync(fp, 0);
		fput(fp);
	} else {
		err("fail to get file *: %s", name);
	}

	sys_close(fd);
	sys_umask(old_mask);
	set_fs(old_fs);

p_err:
	return ret;
}
#endif

#if SENSOR_IMX682_WRITE_PDAF_CAL
int sensor_imx682_cis_LRC_write(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	int position;
	u16 start_addr1;
	u16 start_addr2;
	u16 data_size;

	ulong cal_addr;
	u8 cal_data[SENSOR_IMX682_LRC_CAL_SIZE] = {0, };

#ifdef CONFIG_VENDER_MCD
	char *rom_cal_buf = NULL;
#else
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
#endif

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	FIMC_BUG(!sensor_peri);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	position = sensor_peri->module->position;

#ifdef CONFIG_VENDER_MCD
	ret = fimc_is_sec_get_cal_buf(&rom_cal_buf, position);

	if (ret < 0) {
		goto p_err;
	}

	cal_addr = (ulong)rom_cal_buf;
	if (position == SENSOR_POSITION_REAR) {
		cal_addr += SENSOR_IMX682_LRC_CAL_BASE_REAR;
	} else {
		err("cis_imx682 position(%d) is invalid!\n", position);
		goto p_err;
	}
#else
	if (position == SENSOR_POSITION_REAR){
		cal_addr = lib->minfo->kvaddr_cal[position] + SENSOR_IMX682_LRC_CAL_BASE_REAR;
	}else {
		err("cis_imx682 position(%d) is invalid!\n", position);
		goto p_err;
	}
#endif

	memcpy(cal_data, (u16 *)cal_addr, SENSOR_IMX682_LRC_CAL_SIZE);

#if SENSOR_IMX682_CAL_DEBUG
	ret = sensor_imx682_cis_cal_dump(SENSOR_IMX682_LRC_DUMP_NAME, (char *)cal_data, (size_t)SENSOR_IMX682_LRC_CAL_SIZE);
	if (ret < 0) {
		err("cis_imx682 LRC Cal dump fail(%d)!\n", ret);
		goto p_err;
	}
#endif

	start_addr1 = SENSOR_IMX682_LRC_REG_ADDR1;
	start_addr2 = SENSOR_IMX682_LRC_REG_ADDR2;
	data_size = (SENSOR_IMX682_LRC_CAL_SIZE / 2);

	I2C_MUTEX_LOCK(cis->i2c_lock);

	/* LRC table is written in two parts. Refer PDAF Manual. */
	ret = fimc_is_sensor_write8_sequential(client, start_addr1, cal_data, data_size);
	if (ret < 0) {
		err("cis_imx682 LRC write Error(%d)\n", ret);
	}

	ret = fimc_is_sensor_write8_sequential(client, start_addr2, cal_data + data_size, data_size);
	if (ret < 0) {
		err("cis_imx682 LRC write Error(%d)\n", ret);
	}

	I2C_MUTEX_UNLOCK(cis->i2c_lock);

p_err:
	return ret;
}
#endif

#if SENSOR_IMX682_WRITE_SENSOR_CAL
int sensor_imx682_cis_QuadSensCal_write(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	int position;
	u16 start_addr;
	u16 data_size;

	ulong cal_addr;
	u8 cal_data[SENSOR_IMX682_QUAD_SENS_CAL_SIZE] = {0, };

#ifdef CONFIG_VENDER_MCD
	char *rom_cal_buf = NULL;
#else
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
#endif

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	FIMC_BUG(!sensor_peri);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	position = sensor_peri->module->position;

#ifdef CONFIG_VENDER_MCD
	ret = fimc_is_sec_get_cal_buf(&rom_cal_buf, position);
	if (ret < 0) {
		goto p_err;
	}

	cal_addr = (ulong)rom_cal_buf;
	if (position == SENSOR_POSITION_REAR) {
		cal_addr += SENSOR_IMX682_QUAD_SENS_CAL_BASE_REAR;
	} else {
		err("cis_imx682 position(%d) is invalid!\n", position);
		goto p_err;
	}
#else
	if (position == SENSOR_POSITION_REAR){
		cal_addr = lib->minfo->kvaddr_cal[position] + SENSOR_IMX682_QUAD_SENS_CAL_BASE_REAR;
	}else {
		err("cis_imx682 position(%d) is invalid!\n", position);
		goto p_err;
	}
#endif

	memcpy(cal_data, (u16 *)cal_addr, SENSOR_IMX682_QUAD_SENS_CAL_SIZE);

#if SENSOR_IMX682_CAL_DEBUG
	ret = sensor_imx682_cis_cal_dump(SENSOR_IMX682_QSC_DUMP_NAME, (char *)cal_data, (size_t)SENSOR_IMX682_QUAD_SENS_CAL_SIZE);
	if (ret < 0) {
		err("cis_imx682 QSC Cal dump fail(%d)!\n", ret);
		goto p_err;
	}
#endif

	start_addr = SENSOR_IMX682_QUAD_SENS_REG_ADDR;
	data_size = SENSOR_IMX682_QUAD_SENS_CAL_SIZE;

	ret = fimc_is_sensor_write8_sequential(client, start_addr, cal_data, data_size);
	if (ret < 0) {
		err("cis_imx682 QSC write Error(%d)\n", ret);
	}

p_err:
	return ret;
}
#endif

static void sensor_imx682_cis_set_paf_stat_enable(u32 mode, cis_shared_data *cis_data)
{
	WARN_ON(!cis_data);

	switch (mode) {
	case SENSOR_IMX682_REMOSAIC_FULL_9248x6932_12FPS:
	case SENSOR_IMX682_2X2BIN_FULL_4264X3468_30FPS:
	case SENSOR_IMX682_2X2BIN_FULL_4264X2604_30FPS:
	case SENSOR_IMX682_2X2BIN_FULL_4000X3000_30FPS:
		cis_data->is_data.paf_stat_enable = true;
		break;
	default:
		cis_data->is_data.paf_stat_enable = false;
		break;
	}
#ifdef DISABLE_PDAF
	cis_data->is_data.paf_stat_enable = false;
#endif
}

/*************************************************
 *  [IMX682 Analog gain formular]
 *
 *  m0: [0x008c:0x008d] fixed to 0
 *  m1: [0x0090:0x0091] fixed to -1
 *  c0: [0x008e:0x008f] fixed to 1024
 *  c1: [0x0092:0x0093] fixed to 1024
 *  X : [0x0204:0x0205] Analog gain setting value
 *
 *  Analog Gain = (m0 * X + c0) / (m1 * X + c1)
 *              = 1024 / (1024 - X)
 *
 *  Analog Gain Range = 112 to 1008 (1dB to 26dB)
 *
 *************************************************/

u32 sensor_imx682_cis_calc_again_code(u32 permille)
{
	return 1024 - (1024000 / permille);
}

u32 sensor_imx682_cis_calc_again_permile(u32 code)
{
	return 1024000 / (1024 - code);
}

u32 sensor_imx682_cis_calc_dgain_code(u32 permile)
{
	u8 buf[2] = {0, 0};
	buf[0] = (permile / 1000) & 0x0F;
	buf[1] = (((permile - (buf[0] * 1000)) * 256) / 1000);

	return (buf[0] << 8 | buf[1]);
}

u32 sensor_imx682_cis_calc_dgain_permile(u32 code)
{
	return (((code & 0x0F00) >> 8) * 1000) + ((code & 0x00FF) * 1000 / 256);
}

u32 sensor_imx682_get_fine_integration_value(struct fimc_is_cis *cis,  u16 *fine_integ)
{
	u32 ret = 0;
	u16 fine_integ_time = 0;
	struct i2c_client *client;

	FIMC_BUG(!cis);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);
	/* FINE_INTEG_TIME ADDR [0x0200:0x0201] */
	ret = fimc_is_sensor_read16(client, SENSOR_IMX682_FINE_INTEG_TIME_ADDR, &fine_integ_time);
	if (ret < 0) {
		err("fimc_is_sensor_read16 fail (ret %d)", ret);
		I2C_MUTEX_UNLOCK(cis->i2c_lock);
		return ret;
	}
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	*fine_integ = fine_integ_time;

	info("%s: read fine_integ_time = %#x\n", __func__, fine_integ_time);

	return ret;
}

int sensor_imx682_cis_set_seamless_mode_change_param(struct fimc_is_cis *cis)
{
	int ret = 0;
	cis_shared_data *cis_data;
	struct i2c_client *client;

	u32 frame_duration = 0;
	u32 exp_val = 0;
	u32 analog_gain = 0;
	u32 digital_gain = 0;

	u64 pix_rate_freq_khz = 0;
	u32 line_length_pck = 0;
	u32 frame_length_lines = 0;
	u16 coarse_int = 0;
	u32 min_fine_int = 0;
	u8 dgains[2] = {0, 0};

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -ENODEV;
		goto p_err;
	}

	exp_val = cis_data->auto_exposure[NEXT_FRAME].exposure;
	analog_gain = cis_data->cur_analog_gain;
	digital_gain = cis_data->cur_digital_gain;

	pix_rate_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;

	{
		// frame duration
		coarse_int = (u32)(((pix_rate_freq_khz * exp_val) / line_length_pck) / 1000);
		frame_length_lines = coarse_int + cis_data->max_margin_coarse_integration_time;
		frame_duration = (u32)((u64)(frame_length_lines * line_length_pck) / pix_rate_freq_khz) * 1000;
		frame_duration = MAX(frame_duration, cis_data->min_frame_us_time);
		frame_length_lines = (u16)((pix_rate_freq_khz * frame_duration) / (line_length_pck * 1000));

		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d): pix_rate_freq_khz (%d),"
			KERN_CONT "line_length_pck(%d), frame_length_lines(%#x)\n", cis->id, __func__,
			cis_data->sen_vsync_count, pix_rate_freq_khz, line_length_pck, cis_data->frame_length_lines);
		dbg_sensor(1, "[%s] calculated frame_duration(%d)\n", __func__, frame_duration);

		ret = fimc_is_sensor_write16(client, SENSOR_IMX682_FRAME_LENGTH_LINE_ADDR, frame_length_lines);
		if (ret < 0)
			goto p_err;

		cis_data->cur_frame_us_time = frame_duration;
		cis_data->frame_length_lines = frame_length_lines;
		cis_data->max_coarse_integration_time = SENSOR_IMX682_MAX_COARSE_INTEG_WITH_FRM_LENGTH_CTRL - cis_data->max_margin_coarse_integration_time;
	}

	{
		// exposure
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), target(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, exp_val);

		min_fine_int = cis_data->min_fine_integration_time;

		coarse_int = (u16)(((exp_val * pix_rate_freq_khz) - min_fine_int) / (line_length_pck * 1000));

		if (coarse_int%coarse_integ_step_value)
			coarse_int -= 1;

		if (coarse_int > cis_data->max_coarse_integration_time) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), coarse(%d) max(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, coarse_int, cis_data->max_coarse_integration_time);
			coarse_int = cis_data->max_coarse_integration_time;
		}

		if (coarse_int < cis_data->min_coarse_integration_time) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), coarse(%d) min(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, coarse_int, cis_data->min_coarse_integration_time);
			coarse_int = cis_data->min_coarse_integration_time;
		}

		if (cis_data->min_coarse_integration_time == SENSOR_IMX682_COARSE_INTEGRATION_TIME_MIN
			&& coarse_int < 16
			&& coarse_int%2 == 0) {
			coarse_int -= 1;
		}

		ret = fimc_is_sensor_write16(client, SENSOR_IMX682_COARSE_INTEG_TIME_ADDR, coarse_int);
		if (ret < 0)
			goto p_err;

		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d): coarse_integration_time (%#x)\n",
			cis->id, __func__, cis_data->sen_vsync_count, coarse_int);
	}

	{
		// Analog gain
		dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d), analog_gain(%#x)\n",
			cis->id, __func__, cis->cis_data->sen_vsync_count, analog_gain);

		ret = fimc_is_sensor_write16(client, SENSOR_IMX682_ANALOG_GAIN_ADDR, analog_gain);
		if (ret < 0)
			goto p_err;
	}

	{
		// Digital gain
		dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d), idgain_code(%#x)\n",
			cis->id, __func__, cis->cis_data->sen_vsync_count, digital_gain);

		dgains[0] = (u8)((digital_gain & 0x0F00) >> 8);
		dgains[1] = (u8)(digital_gain & 0xFF);

		ret = fimc_is_sensor_write8_array(client, SENSOR_IMX682_DIG_GAIN_ADDR, dgains, 2);
		if (ret < 0)
			goto p_err;
	}

p_err:
	return ret;
}

/* CIS OPS */
int sensor_imx682_cis_init(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	u32 setfile_index = 0;
	u8 cis_rev;
	cis_setting_info setinfo;
#ifdef USE_CAMERA_HW_BIG_DATA
	struct cam_hw_param *hw_param = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
#endif

	setinfo.param = NULL;
	setinfo.return_value = 0;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	FIMC_BUG(!cis->cis_data);
	memset(cis->cis_data, 0, sizeof(cis_shared_data));

	probe_info("%s imx682 init\n", __func__);
	cis->rev_flag = false;

/***********************************************************************
***** Check that QSC Cal is written for Remosaic Capture.
***** false : Not yet write the QSC
***** true  : Written the QSC Or Skip
***********************************************************************/
	sensor_imx682_cal_write_flag = false;

	ret = sensor_imx682_cis_check_rev(cis);
	if (ret < 0) {
#ifdef USE_CAMERA_HW_BIG_DATA
		sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
		if (sensor_peri)
			fimc_is_sec_get_hw_param(&hw_param, sensor_peri->module->position);
		if (hw_param)
			hw_param->i2c_sensor_err_cnt++;
#endif
		warn("sensor_imx682_check_rev is fail when cis init");
		cis->rev_flag = true;
		ret = 0;
	}

	cis_rev = cis->cis_data->cis_rev;
	/* Check if don't care condition is satisfied */
	if ((cis_rev & 0xF0) ==  (SENSOR_IMX682_CHIP_ID_VER_3_X_0x1X & 0xF0)) {
		cis_rev = SENSOR_IMX682_CHIP_ID_VER_3_X_0x1X;
	}

	switch (cis_rev) {
		case SENSOR_IMX682_CHIP_ID_VER_3_X_0x01:
		case SENSOR_IMX682_CHIP_ID_VER_3_X_0x02:
		case SENSOR_IMX682_CHIP_ID_VER_3_X_0x1X:
			probe_info("%s setfile_B\n", __func__);
			sensor_imx682_global = sensor_imx682_setfile_B_Global;
			sensor_imx682_global_size = ARRAY_SIZE(sensor_imx682_setfile_B_Global);
			sensor_imx682_setfiles = sensor_imx682_setfiles_B;
			sensor_imx682_setfile_sizes = sensor_imx682_setfile_B_sizes;
			sensor_imx682_pllinfos = sensor_imx682_pllinfos_B;
			sensor_imx682_max_setfile_num = ARRAY_SIZE(sensor_imx682_setfiles_B);
			break;
		default:
			probe_info("%s chip_rev(%d) is wrong! setfile_B for MP (default)\n", __func__, cis->cis_data->cis_rev);
			sensor_imx682_global = sensor_imx682_setfile_B_Global;
			sensor_imx682_global_size = ARRAY_SIZE(sensor_imx682_setfile_B_Global);
			sensor_imx682_setfiles = sensor_imx682_setfiles_B;
			sensor_imx682_setfile_sizes = sensor_imx682_setfile_B_sizes;
			sensor_imx682_pllinfos = sensor_imx682_pllinfos_B;
			sensor_imx682_max_setfile_num = ARRAY_SIZE(sensor_imx682_setfiles_B);
			break;
	}
#ifdef DISABLE_PDAF
	sensor_imx682_pdaf_off_setfile = sensor_imx682_pdaf_off_setfile_B;
	sensor_imx682_pdaf_off_size = ARRAY_SIZE(sensor_imx682_pdaf_off_setfile_B);
#endif
	cis->cis_data->product_name = cis->id;
	cis->cis_data->cur_width = SENSOR_IMX682_MAX_WIDTH;
	cis->cis_data->cur_height = SENSOR_IMX682_MAX_HEIGHT;
	cis->cis_data->low_expo_start = 33000;
	cis->need_mode_change = false;
	cis->long_term_mode.sen_strm_off_on_step = 0;
	cis->long_term_mode.sen_strm_off_on_enable = false;
	sensor_imx682_stream_on_state = false;

	sensor_imx682_cis_data_calculation(sensor_imx682_pllinfos[setfile_index], cis->cis_data);
	sensor_imx682_set_integration_max_margin(setfile_index, cis->cis_data);
	sensor_imx682_set_integration_min(setfile_index, cis->cis_data);
	sensor_imx682_set_integration_step_value(setfile_index);

#if SENSOR_IMX682_DEBUG_INFO
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_min_exposure_time, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] min exposure time : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_max_exposure_time, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] max exposure time : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_min_analog_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] min again : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_max_analog_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] max again : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_min_digital_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] min dgain : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_max_digital_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] max dgain : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
#endif

#if SENSOR_IMX682_WRITE_PDAF_CAL
	ret = sensor_imx682_cis_LRC_write(subdev);
	if (ret < 0) {
		err("sensor_imx682_LRC_Cal_write fail!! (%d)", ret);
		goto p_err;
	}
#endif

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_imx682_cis_log_status(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client = NULL;
	u8 data8 = 0;
	u16 data16 = 0;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -ENODEV;
		goto p_err;
	}

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -ENODEV;
		goto p_err;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);

	pr_info("[%s] *******************************\n", __func__);

	ret = fimc_is_sensor_read16(client, SENSOR_IMX682_MODEL_ID_ADDR, &data16);
	if (unlikely(!ret)) pr_info("[SEN:INFO]model_id(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read8(client, SENSOR_IMX682_REVISION_NUM_ADDR, &data8);
	if (unlikely(!ret)) pr_info("[SEN:INFO]revision_number(0x%x)\n", data8);
	else goto i2c_err;
	ret = fimc_is_sensor_read8(client, SENSOR_IMX682_FRAME_COUNT_ADDR, &data8);
	if (unlikely(!ret)) pr_info("[SEN:INFO]frame_count(0x%x)\n", data8);
	else goto i2c_err;
	ret = fimc_is_sensor_read8(client, SENSOR_IMX682_SETUP_MODE_SELECT_ADDR, &data8);
	if (unlikely(!ret)) pr_info("[SEN:INFO]setup_mode(streaming)(0x%x)\n", data8);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, SENSOR_IMX682_COARSE_INTEG_TIME_ADDR, &data16);
	if (unlikely(!ret)) pr_info("[SEN:INFO]coarse_integration_time(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, SENSOR_IMX682_ANALOG_GAIN_ADDR, &data16);
	if (unlikely(!ret)) pr_info("[SEN:INFO]gain_code_global(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, SENSOR_IMX682_FRAME_LENGTH_LINE_ADDR, &data16);
	if (unlikely(!ret)) pr_info("[SEN:INFO]frame_length_line(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, SENSOR_IMX682_LINE_LENGTH_PCK_ADDR, &data16);
	if (unlikely(!ret)) pr_info("[SEN:INFO]line_length_pck(0x%x)\n", data16);
	else goto i2c_err;

	sensor_cis_dump_registers(subdev, sensor_imx682_setfiles[0], sensor_imx682_setfile_sizes[0]);

i2c_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	pr_err("[SEN:DUMP] *******************************\n");

p_err:
	return ret;
}

static int sensor_imx682_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
{
#if USE_GROUP_PARAM_HOLD
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	struct i2c_client *client = NULL;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (hold == cis->cis_data->group_param_hold) {
		pr_debug("already group_param_hold (%d)\n", cis->cis_data->group_param_hold);
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, SENSOR_IMX682_GROUP_PARAM_HOLD_ADDR, hold);
	if (ret < 0) {
		err("register write fail!!");
		goto p_err;
	}

	cis->cis_data->group_param_hold = hold;
	ret = 1;

p_err:
	return ret;
#else

	return 0;
#endif
}

/* Input
 *	hold : true - hold, flase - no hold
 * Output
 *      return: 0 - no effect(already hold or no hold)
 *		positive - setted by request
 *		negative - ERROR value
 */
int sensor_imx682_cis_group_param_hold(struct v4l2_subdev *subdev, bool hold)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	ret = sensor_imx682_cis_group_param_hold_func(subdev, hold);
	if (ret < 0) {
		err("sensor_imx682_cis_group_param_hold_func fail!!");
		goto p_err;
	}

p_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);
	return ret;
}

int sensor_imx682_cis_set_global_setting(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);

	I2C_MUTEX_LOCK(cis->i2c_lock);

	/* setfile global setting is at camera entrance */
	info("[%s] global setting start\n", __func__);
	ret = sensor_cis_set_registers(subdev, sensor_imx682_global, sensor_imx682_global_size);
	if (ret < 0) {
		err("sensor_imx682_set_registers fail!!");
		goto p_err;
	}

	dbg_sensor(1, "[%s] global setting done\n", __func__);

p_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	// Check that QSC and DPC Cal is written for Remosaic Capture.
	// false : Not yet write the QSC and DPC
	// true  : Written the QSC and DPC
	sensor_imx682_cal_write_flag = false;
	return ret;
}

int sensor_imx682_cis_mode_change(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	u16 fine_integ = 0;
	struct fimc_is_cis *cis = NULL;
	struct i2c_client *client;
	cis_shared_data *cis_data = NULL;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (mode > sensor_imx682_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		ret = -EINVAL;
		goto p_err;
	}

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	/* If check_rev(Sensor ID in OTP) of IMX682 fail when cis_init, one more check_rev in mode_change */
	if (cis->rev_flag == true) {
		cis->rev_flag = false;
		ret = sensor_imx682_cis_check_rev(cis);
		if (ret < 0) {
			err("sensor_imx682_check_rev is fail");
			goto p_err;
		}
		info("[%s] cis_rev=%#x\n", __func__, cis->cis_data->cis_rev);
	}

	sensor_imx682_cis_set_paf_stat_enable(mode, cis->cis_data);

	I2C_MUTEX_LOCK(cis->i2c_lock);

	info("[%s] mode=%d, mode change setting start\n", __func__, mode);
	ret = sensor_cis_set_registers(subdev, sensor_imx682_setfiles[mode], sensor_imx682_setfile_sizes[mode]);
	if (ret < 0) {
		err("sensor_imx682_set_registers fail!!");
		goto p_i2c_err;
	}

#ifdef DISABLE_PDAF
	ret = sensor_cis_set_registers(subdev, sensor_imx682_pdaf_off_setfile, sensor_imx682_pdaf_off_size);
	if (ret < 0) {
		err("sensor_imx682_pdaf_off_setfile fail!!");
		goto p_i2c_err;
	}
#endif
	/* Disable Embedded Data Line */
	fimc_is_sensor_write8(client, SENSOR_IMX682_EBD_CONTROL_ADDR, 0x00);

	if (cis_data->stream_on) {
		info("[%s] current mode (%d) is stream on\n", __func__, mode);
		if (IS_SEAMLESS_MODE_CHANGE(cis)) {
			ret = sensor_imx682_cis_set_seamless_mode_change_param(cis);
			if (ret < 0)
				goto p_i2c_err;
		}
	}
	dbg_sensor(1, "[%s] mode changed(%d)\n", __func__, mode);

#if SENSOR_IMX682_WRITE_SENSOR_CAL
	if (mode >= IMX682_MODE_REMOSAIC_START
		&& mode <= IMX682_MODE_REMOSAIC_END
		&& sensor_imx682_cal_write_flag == false) {
		sensor_imx682_cal_write_flag = true;

		info("[%s] %d mode is QBC Remosaic Mode! Write QSC data.\n", __func__, mode);

		ret = sensor_imx682_cis_QuadSensCal_write(subdev);
		if (ret < 0) {
			err("sensor_imx576_Quad_Sens_Cal_write fail!! (%d)", ret);
			goto p_err;
		}
	}
#endif

p_i2c_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	msleep(10);

	sensor_imx682_set_integration_max_margin(mode, cis->cis_data);
	sensor_imx682_set_integration_min(mode, cis->cis_data);
	sensor_imx682_set_integration_step_value(mode);
	sensor_imx682_get_fine_integration_value(cis, &fine_integ);

	/* Constant values */
	cis->cis_data->min_fine_integration_time = fine_integ;
	cis->cis_data->max_fine_integration_time = fine_integ;

p_err:
	return ret;
}

int sensor_imx682_cis_set_size(struct v4l2_subdev *subdev, cis_shared_data *cis_data)
{
	int ret = 0;
	struct i2c_client *client = NULL;
	struct fimc_is_cis *cis = NULL;
#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif
	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	if (unlikely(!cis_data)) {
		err("cis data is NULL");
		if (unlikely(!cis->cis_data)) {
			ret = -EINVAL;
			goto p_err;
		} else {
			cis_data = cis->cis_data;
		}
	}

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec) * 1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:

	return ret;
}

int sensor_imx682_cis_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	FIMC_BUG(!sensor_peri);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	sensor_imx682_cis_group_param_hold(subdev, 0x01);

#ifdef SENSOR_IMX682_DEBUG_INFO
	{
		u16 pll;
		fimc_is_sensor_read16(client, 0x0300, &pll);
		dbg_sensor(1, "______ vt_pix_clk_div(%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0302, &pll);
		dbg_sensor(1, "______ vt_sys_clk_div(%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0304, &pll);
		dbg_sensor(1, "______ pre_pll_clk_div(%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0306, &pll);
		dbg_sensor(1, "______ pll_multiplier(%x)\n", pll);
		fimc_is_sensor_read16(client, 0x030a, &pll);
		dbg_sensor(1, "______ op_sys_clk_div(%x)\n", pll);
		fimc_is_sensor_read16(client, 0x030c, &pll);
		dbg_sensor(1, "______ op_prepllck_div(%x)\n", pll);
		fimc_is_sensor_read16(client, 0x030e, &pll);
		dbg_sensor(1, "______ op_pll_multiplier(%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0310, &pll);
		dbg_sensor(1, "______ pll_mult_driv(%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0340, &pll);
		dbg_sensor(1, "______ frame_length_lines(%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0342, &pll);
		dbg_sensor(1, "______ line_length_pck(%x)\n", pll);
	}
#endif

	I2C_MUTEX_LOCK(cis->i2c_lock);

#ifdef DISABLE_DUAL_SYNC
	info("[%s] start (single mode)\n", __func__);
#else
	info("[%s] start (dual master mode)\n", __func__);

	fimc_is_sensor_write8(client, 0x3020, 0x1);
	fimc_is_sensor_write8(client, 0x3031, 0x1);
#endif

	/* Sensor stream on */
	fimc_is_sensor_write8(client, SENSOR_IMX682_SETUP_MODE_SELECT_ADDR, 0x01);
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	sensor_imx682_cis_group_param_hold(subdev, 0x00);

	cis_data->stream_on = true;
	sensor_imx682_stream_on_state = true;

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_imx682_cis_stream_off(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	sensor_imx682_cis_group_param_hold(subdev, 0x00);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	fimc_is_sensor_write8(client, SENSOR_IMX682_SETUP_MODE_SELECT_ADDR, 0x00);
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	cis_data->stream_on = false;
	sensor_imx682_stream_on_state = false;

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_imx682_cis_adjust_frame_duration(struct v4l2_subdev *subdev,
						u32 input_exposure_time,
						u32 *target_duration)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_shared_data *cis_data;

	u64 pix_rate_freq_khz = 0;
	u32 line_length_pck = 0;
	u32 coarse_integ_time = 0;
	u32 frame_length_lines = 0;
	u32 frame_duration = 0;
	u32 max_frame_us_time = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!target_duration);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	pix_rate_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;
	coarse_integ_time = (u32)(((pix_rate_freq_khz * input_exposure_time)
							- cis_data->min_fine_integration_time) / (line_length_pck * 1000));

	if (cis->min_fps == cis->max_fps) {
		dbg_sensor(1, "[%s] requested min_fps(%d), max_fps(%d) from HAL\n", __func__, cis->min_fps, cis->max_fps);

		if (coarse_integ_time > cis_data->max_coarse_integration_time) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), coarse(%u) max(%u)\n", cis->id, __func__,
				cis_data->sen_vsync_count, coarse_integ_time, cis_data->max_coarse_integration_time);
			coarse_integ_time = cis_data->max_coarse_integration_time;
		}

		if (coarse_integ_time < cis_data->min_coarse_integration_time) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), coarse(%u) min(%u)\n", cis->id, __func__,
				cis_data->sen_vsync_count, coarse_integ_time, cis_data->min_coarse_integration_time);
			coarse_integ_time = cis_data->min_coarse_integration_time;
		}
	}

	frame_length_lines = coarse_integ_time + cis_data->max_margin_coarse_integration_time;

	frame_duration = (u32)((u64)(frame_length_lines * line_length_pck) / pix_rate_freq_khz) * 1000;
	max_frame_us_time = 1000000/cis->min_fps;

	dbg_sensor(1, "[%s](vsync cnt = %d) input exp(%d), calculated frame duraion(%d), min_frame_us(%d), max_frame_us_time(%d)\n",
			__func__, cis_data->sen_vsync_count, input_exposure_time, frame_duration, cis_data->min_frame_us_time, max_frame_us_time);

	if (input_exposure_time <= 0)
		*target_duration = cis_data->min_frame_us_time;
	else
		*target_duration = MAX(frame_duration, cis_data->min_frame_us_time);
	
	/*
	 * For recording with fixed fps (>= 10fps).
	 * If input exposure is setted as 33333us@30fps from ddk,
	 * then calculated frame duration is larger than 33333us because of CIT MARGIN.
	 */
	if ((cis_data->min_frame_us_time <= 100000) && (cis->min_fps == cis->max_fps)) {
		*target_duration = MIN(frame_duration, max_frame_us_time);
	}

	dbg_sensor(1, "[%s] calculated frame_duration(%d), adjusted frame_duration(%d)\n", __func__, frame_duration, *target_duration);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

	return ret;
}

int sensor_imx682_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 frame_duration)
{
	int ret = 0;
	int hold = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;
	struct fimc_is_long_term_expo_mode *lte_mode;

	u64 pix_rate_freq_khz = 0;
	u32 line_length_pck = 0;
	u16 frame_length_lines = 0;
	u64 max_fll_frame_time;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;
	lte_mode = &cis->long_term_mode;

	if (frame_duration < cis_data->min_frame_us_time) {
		dbg_sensor(1, "frame duration(%d) is less than min(%d)\n", frame_duration, cis_data->min_frame_us_time);
		frame_duration = cis_data->min_frame_us_time;
	}

	pix_rate_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;

	max_fll_frame_time = (65535 * (u64)line_length_pck * 1000) / (u64)pix_rate_freq_khz;

	/*
	 * For Long Exposure Mode without stream on_off. (ex. Night HyperLapse)
	 * If frame duration over than 1sec, then it has to be applied CIT shift.
	 * In this case, frame_duration is set in set_exposure_time with CIT shift.
	 */
	if (lte_mode->sen_strm_off_on_enable == false && cis_data->min_frame_us_time > max_fll_frame_time) {
		cis_data->cur_frame_us_time = frame_duration;
		dbg_sensor(1, "[MOD:D:%d][%s] Skip set frame duration(%d) for CIT SHIFT.\n",
			cis->id, __func__, frame_duration);
		return ret;
	}

	frame_length_lines = (u16)((pix_rate_freq_khz * frame_duration) / (line_length_pck * 1000));
	dbg_sensor(1, "[MOD:D:%d] %s, pix_rate_freq_khz(%zu) frame_duration(%d us), llp(%d), fll(%d)\n",
			cis->id, __func__, pix_rate_freq_khz, frame_duration, line_length_pck, frame_length_lines);

	/* When SM/SSM is setted to fixed 240fps */
	if (frame_duration == SENSOR_IMX682_240FPS_FRAME_DURATION_US) {
		frame_length_lines = cis_data->frame_length_lines;

		dbg_sensor(1, "[MOD:D:%d] %s, frame_duration(%d us), llp(%d), fll(%d): fixed frame rate for 240FPS\n",
				cis->id, __func__, frame_duration, line_length_pck, frame_length_lines);
	}

	hold = sensor_imx682_cis_group_param_hold(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);
	
	if (lte_mode->sen_strm_off_on_enable == false && cis_data->frame_length_lines_shifter > 0) {
		cis_data->frame_length_lines_shifter = 0;
		ret = fimc_is_sensor_write8(client, SENSOR_IMX682_CIT_LSHIFT_ADDR, 0);
		if (ret < 0)
			goto p_i2c_err;
	}

	ret = fimc_is_sensor_write16(client, SENSOR_IMX682_FRAME_LENGTH_LINE_ADDR, frame_length_lines);
	if (ret < 0) {
		goto p_i2c_err;
	}

	cis_data->cur_frame_us_time = frame_duration;
	cis_data->frame_length_lines = frame_length_lines;
	if (lte_mode->sen_strm_off_on_enable) {
		cis_data->max_coarse_integration_time =
			SENSOR_IMX682_MAX_COARSE_INTEG_WITH_FRM_LENGTH_CTRL - cis_data->max_margin_coarse_integration_time;
	} else {
		cis_data->max_coarse_integration_time = cis_data->frame_length_lines - cis_data->max_margin_coarse_integration_time;
	}

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_i2c_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

p_err:
	if (hold > 0) {
		hold = sensor_imx682_cis_group_param_hold(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	return ret;
}

int sensor_imx682_cis_set_frame_rate(struct v4l2_subdev *subdev, u32 min_fps)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_shared_data *cis_data;

	u32 frame_duration = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	if (min_fps > cis_data->max_fps) {
		err("[MOD:D:%d] %s, request FPS is too high(%d), set to max_fps(%d)\n",
			cis->id, __func__, min_fps, cis_data->max_fps);
		min_fps = cis_data->max_fps;
	}

	if (min_fps == 0) {
		err("[MOD:D:%d] %s, request FPS is 0, set to min FPS(1)\n", cis->id, __func__);
		min_fps = 1;
	}

	frame_duration = (1 * 1000 * 1000) / min_fps;

	dbg_sensor(1, "[MOD:D:%d] %s, set FPS(%d), frame duration(%d)\n",
			cis->id, __func__, min_fps, frame_duration);

	ret = sensor_imx682_cis_set_frame_duration(subdev, frame_duration);
	if (ret < 0) {
		err("[MOD:D:%d] %s, set frame duration is fail(%d)\n",
			cis->id, __func__, ret);
		goto p_err;
	}

	cis_data->min_frame_us_time = frame_duration;

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:

	return ret;
}

int sensor_imx682_cis_set_exposure_time(struct v4l2_subdev *subdev, struct ae_param *target_exposure)
{
	int ret = 0;
	int hold = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;
	struct fimc_is_long_term_expo_mode *lte_mode;

	u64 pix_rate_freq_khz = 0;
	u16 coarse_int = 0;
	u32 line_length_pck = 0;
	u32 min_fine_int = 0;
	u32 target_exp = 0;
	u32 target_frame_duration = 0;
	u16 frame_length_lines = 0;
	u64 max_fll_frame_time;

	unsigned char cit_lshift_val = 0;
	int cit_lshift_count = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!target_exposure);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if ((target_exposure->long_val <= 0) || (target_exposure->short_val <= 0)) {
		err("[%s] invalid target exposure(%d, %d)\n", __func__,
				target_exposure->long_val, target_exposure->short_val);
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;
	lte_mode = &cis->long_term_mode;

	dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), target long(%d), short(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, target_exposure->long_val, target_exposure->short_val);

	target_exp = target_exposure->val;
	pix_rate_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;
	min_fine_int = cis_data->min_fine_integration_time;

	max_fll_frame_time = (65535 * (u64)line_length_pck * 1000) / (u64)pix_rate_freq_khz;

	/*
	 * For Long Exposure Mode without stream on_off. (ex. Night Hyper Laps: min exp. is 1.5sec)
	 * If frame duration over than 1sec, then sequence is same as below
	 * 1. set CIT_LSHFIT
	 * 2. set COARSE_INTEGRATION_TIME
	 * 3. set FRM_LENGTH_LINES
	 */	 
	if (cis_data->is_data.scene_mode == AA_SCENE_MODE_SUPER_NIGHT || cis_data->is_data.scene_mode == AA_SCENE_MODE_HYPERLAPSE) {
		if (lte_mode->sen_strm_off_on_enable == false && cis_data->cur_frame_us_time > max_fll_frame_time) {
			target_frame_duration = cis_data->cur_frame_us_time;
			dbg_sensor(1, "[MOD:D:%d] %s, input frame duration(%d) for CIT SHIFT \n",
				cis->id, __func__, target_frame_duration);

			if (target_frame_duration > 100000) {
				cit_lshift_val = (unsigned char)(target_frame_duration / 100000);
				while (cit_lshift_val > 1) {
					cit_lshift_val /= 2;
					target_frame_duration /= 2;
					target_exp /= 2;
					cit_lshift_count ++;
				}

				if (cit_lshift_count > SENSOR_IMX682_MAX_CIT_LSHIFT_VALUE)
				cit_lshift_count = SENSOR_IMX682_MAX_CIT_LSHIFT_VALUE;
			}

			frame_length_lines = (u16)((pix_rate_freq_khz * target_frame_duration) / (line_length_pck * 1000));

			cis_data->frame_length_lines = frame_length_lines;
			cis_data->frame_length_lines_shifter = cit_lshift_count;
			cis_data->max_coarse_integration_time =
				frame_length_lines - cis_data->max_margin_coarse_integration_time;

			dbg_sensor(1, "[MOD:D:%d] %s, target_frame_duration(%d), frame_length_line(%d), cit_lshift_count(%d)\n",
				cis->id, __func__, target_frame_duration, frame_length_lines, cit_lshift_count);
		}
	}

	coarse_int = (u16)((((target_exp * pix_rate_freq_khz) / 1000)- min_fine_int) / line_length_pck);

	if (coarse_int%coarse_integ_step_value)
		coarse_int -= 1;

	if (coarse_int > cis_data->max_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, coarse_int(%d) max(%d)\n", cis->id, __func__,
			coarse_int, cis_data->max_coarse_integration_time);
		coarse_int = cis_data->max_coarse_integration_time;
	}

	if (coarse_int < cis_data->min_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, coarse_int(%d) min(%d)\n", cis->id, __func__,
			coarse_int, cis_data->min_coarse_integration_time);
		coarse_int = cis_data->min_coarse_integration_time;
	}

	if (cis_data->min_coarse_integration_time == SENSOR_IMX682_COARSE_INTEGRATION_TIME_MIN
		&& coarse_int < 16
		&& coarse_int%2 == 0) {
		coarse_int -= 1;
	}

	cis_data->cur_exposure_coarse = coarse_int;
	cis_data->cur_long_exposure_coarse = coarse_int;
	cis_data->cur_short_exposure_coarse = coarse_int;

	hold = sensor_imx682_cis_group_param_hold(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);

	if (lte_mode->sen_strm_off_on_enable == false && cis_data->min_frame_us_time > max_fll_frame_time) {
		if (cit_lshift_count > 0) {
			ret = fimc_is_sensor_write8(client, SENSOR_IMX682_CIT_LSHIFT_ADDR, cit_lshift_count);
			if (ret < 0)
				goto p_i2c_err;
		}
	}

	ret = fimc_is_sensor_write16(client, SENSOR_IMX682_COARSE_INTEG_TIME_ADDR, coarse_int);
	if (ret < 0)
		goto p_i2c_err;

	if (lte_mode->sen_strm_off_on_enable == false && cis_data->min_frame_us_time > max_fll_frame_time) {
		ret = fimc_is_sensor_write16(client, SENSOR_IMX682_FRAME_LENGTH_LINE_ADDR, frame_length_lines);
		if (ret < 0)
			goto p_i2c_err;
	}

	dbg_sensor(1, "[MOD:D:%d] %s, pixelrate(%zu), llp(%d), fll(%d), min_fine_int(%d) cit(%d)\n",  cis->id, __func__,
		pix_rate_freq_khz, line_length_pck, cis_data->frame_length_lines, min_fine_int, coarse_int);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_i2c_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

p_err:
	if (hold > 0) {
		hold = sensor_imx682_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	return ret;
}

int sensor_imx682_cis_get_min_exposure_time(struct v4l2_subdev *subdev, u32 *min_expo)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	cis_shared_data *cis_data = NULL;
	u32 min_integration_time = 0;
	u32 min_coarse = 0;
	u32 min_fine = 0;
	u64 pix_rate_freq_khz = 0;
	u32 line_length_pck = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!min_expo);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	pix_rate_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;
	min_coarse = cis_data->min_coarse_integration_time;
	min_fine = cis_data->min_fine_integration_time;

	min_integration_time = (u32)((u64)((line_length_pck * min_coarse) + min_fine) * 1000 / pix_rate_freq_khz);
	*min_expo = min_integration_time;

	dbg_sensor(1, "[%s] min integration time %d\n", __func__, min_integration_time);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

	return ret;
}

int sensor_imx682_cis_get_max_exposure_time(struct v4l2_subdev *subdev, u32 *max_expo)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_shared_data *cis_data;
	u32 max_integration_time = 0;
	u32 max_coarse_margin = 0;
	u32 max_coarse = 0;
	u32 max_fine = 0;
	u64 pix_rate_freq_khz = 0;
	u32 line_length_pck = 0;
	u32 frame_length_lines = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!max_expo);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	pix_rate_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;
	frame_length_lines = cis_data->frame_length_lines;
	max_coarse_margin = cis_data->max_margin_coarse_integration_time;
	max_coarse = frame_length_lines - max_coarse_margin;
	max_fine = cis_data->max_fine_integration_time;

	max_integration_time = (u32)((u64)((line_length_pck * max_coarse) + max_fine) * 1000 / pix_rate_freq_khz);

	*max_expo = max_integration_time;

	/* To Do: is udating this value right hear? */
	cis_data->max_coarse_integration_time = max_coarse;


	dbg_sensor(1, "[%s] frame_length_lines %d, line_length_pck %d\n",
			__func__, frame_length_lines, line_length_pck);

	dbg_sensor(1, "[%s] max integration time %d, max coarse integration %d\n",
			__func__, max_integration_time, cis_data->max_coarse_integration_time);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

	return ret;
}

int sensor_imx682_cis_adjust_analog_gain(struct v4l2_subdev *subdev, u32 input_again, u32 *target_permile)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_shared_data *cis_data;

	u32 again_code = 0;
	u32 again_permile = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!target_permile);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	again_code = sensor_imx682_cis_calc_again_code(input_again);

	if (again_code > cis_data->max_analog_gain[0]) {
		again_code = cis_data->max_analog_gain[0];
	} else if (again_code < cis_data->min_analog_gain[0]) {
		again_code = cis_data->min_analog_gain[0];
	}

	again_permile = sensor_imx682_cis_calc_again_permile(again_code);

	dbg_sensor(1, "[%s] min again(%d), max(%d), input_again(%d), code(%d), permile(%d)\n", __func__,
			cis_data->max_analog_gain[0],
			cis_data->min_analog_gain[0],
			input_again,
			again_code,
			again_permile);

	*target_permile = again_permile;

	return ret;
}

int sensor_imx682_cis_set_analog_gain(struct v4l2_subdev *subdev, struct ae_param *again)
{
	int ret = 0;
	int hold = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;

	u16 analog_gain = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!again);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	analog_gain = (u16)sensor_imx682_cis_calc_again_code(again->val);

	if (analog_gain < cis->cis_data->min_analog_gain[0]) {
		analog_gain = cis->cis_data->min_analog_gain[0];
	}

	if (analog_gain > cis->cis_data->max_analog_gain[0]) {
		analog_gain = cis->cis_data->max_analog_gain[0];
	}

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d), input_again = %d us, analog_gain(%#x)\n",
		cis->id, __func__, cis->cis_data->sen_vsync_count, again->val, analog_gain);

	hold = sensor_imx682_cis_group_param_hold(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	// the address of analog_gain is [9:0] from 0x0204 to 0x0205
	analog_gain &= 0x03FF;

	// Analog gain
	I2C_MUTEX_LOCK(cis->i2c_lock);
	ret = fimc_is_sensor_write16(client, SENSOR_IMX682_ANALOG_GAIN_ADDR, analog_gain);
	if (ret < 0)
		goto p_i2c_err;

	cis_data->cur_analog_gain = analog_gain;
	cis_data->cur_analog_gain_permille = again->val;

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_i2c_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

p_err:
	if (hold > 0) {
		hold = sensor_imx682_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	return ret;
}

int sensor_imx682_cis_get_analog_gain(struct v4l2_subdev *subdev, u32 *again)
{
	int ret = 0;
	int hold = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;

	u16 analog_gain = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!again);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	hold = sensor_imx682_cis_group_param_hold(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);
	ret = fimc_is_sensor_read16(client, SENSOR_IMX682_ANALOG_GAIN_ADDR, &analog_gain);
	if (ret < 0)
		goto p_i2c_err;

	analog_gain &= 0x03FF;
	*again = sensor_imx682_cis_calc_again_permile(analog_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, cur_again = %d us, analog_gain(%#x)\n",
			cis->id, __func__, *again, analog_gain);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_i2c_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

p_err:
	if (hold > 0) {
		hold = sensor_imx682_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	return ret;
}

int sensor_imx682_cis_get_min_analog_gain(struct v4l2_subdev *subdev, u32 *min_again)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_shared_data *cis_data;
	u16 min_again_code = SENSOR_IMX682_MIN_ANALOG_GAIN_SET_VALUE;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!min_again);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	cis_data->min_analog_gain[0] = min_again_code;
	cis_data->min_analog_gain[1] = sensor_imx682_cis_calc_again_permile(min_again_code);
	*min_again = cis_data->min_analog_gain[1];

	dbg_sensor(1, "[%s] min_again_code %d, main_again_permile %d\n", __func__,
		cis_data->min_analog_gain[0], cis_data->min_analog_gain[1]);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

	return ret;
}

int sensor_imx682_cis_get_max_analog_gain(struct v4l2_subdev *subdev, u32 *max_again)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_shared_data *cis_data;
	u16 max_again_code = SENSOR_IMX682_MAX_ANALOG_GAIN_SET_VALUE;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!max_again);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	cis_data->max_analog_gain[0] = max_again_code;
	cis_data->max_analog_gain[1] = sensor_imx682_cis_calc_again_permile(max_again_code);
	*max_again = cis_data->max_analog_gain[1];

	dbg_sensor(1, "[%s] max_again_code %d, max_again_permile %d\n", __func__,
		cis_data->max_analog_gain[0], cis_data->max_analog_gain[1]);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

	return ret;
}

int sensor_imx682_cis_set_digital_gain(struct v4l2_subdev *subdev, struct ae_param *dgain)
{
	int ret = 0;
	int hold = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;

	u16 dgain_code = 0;
	u8 dgains[2] = {0};

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!dgain);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dgain_code = (u16)sensor_cis_calc_dgain_code(dgain->val);

	if (dgain_code < cis->cis_data->min_digital_gain[0]) {
		dgain_code = cis->cis_data->min_digital_gain[0];
	}
	if (dgain_code > cis->cis_data->max_digital_gain[0]) {
		dgain_code = cis->cis_data->max_digital_gain[0];
	}

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d), input_dgain = %d, dgain_code(%#x)\n",
			cis->id, __func__, cis->cis_data->sen_vsync_count, dgain->val, dgain_code);

	hold = sensor_imx682_cis_group_param_hold(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	/*
	 * 0x020E [15:8] - range 1d to 15d
	 * 0x020F [7:0] - range 0d to 255d
	 */
	dgains[0] = (u8)((dgain_code & 0x0F00) >> 8);
	dgains[1] = (u8)(dgain_code & 0xFF);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	ret = fimc_is_sensor_write8_array(client, SENSOR_IMX682_DIG_GAIN_ADDR, dgains, 2);
	if (ret < 0) {
		goto p_i2c_err;
	}

	cis_data->cur_digital_gain = dgain_code;
	cis_data->cur_digital_gain_permille = dgain->val;

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_i2c_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

p_err:
	if (hold > 0) {
		hold = sensor_imx682_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	return ret;
}

int sensor_imx682_cis_get_digital_gain(struct v4l2_subdev *subdev, u32 *dgain)
{
	int ret = 0;
	int hold = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;

	u16 digital_gain = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!dgain);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	hold = sensor_imx682_cis_group_param_hold(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);
	ret = fimc_is_sensor_read16(client, SENSOR_IMX682_DIG_GAIN_ADDR, &digital_gain);
	if (ret < 0)
		goto p_i2c_err;

	*dgain = sensor_cis_calc_dgain_permile(digital_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, dgain_permile = %d, dgain_code(%#x)\n",
			cis->id, __func__, *dgain, digital_gain);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_i2c_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

p_err:
	if (hold > 0) {
		hold = sensor_imx682_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	return ret;
}

int sensor_imx682_cis_get_min_digital_gain(struct v4l2_subdev *subdev, u32 *min_dgain)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_shared_data *cis_data;
	u16 min_dgain_code = SENSOR_IMX682_MIN_DIGITAL_GAIN_SET_VALUE;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!min_dgain);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	cis_data->min_digital_gain[0] = min_dgain_code;
	cis_data->min_digital_gain[1] = sensor_imx682_cis_calc_dgain_permile(min_dgain_code);

	*min_dgain = cis_data->min_digital_gain[1];

	dbg_sensor(1, "[%s] min_dgain_code %d, min_dgain_permile %d\n", __func__,
		cis_data->min_digital_gain[0], cis_data->min_digital_gain[1]);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

	return ret;
}

int sensor_imx682_cis_get_max_digital_gain(struct v4l2_subdev *subdev, u32 *max_dgain)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_shared_data *cis_data;
	u16 max_dgain_code = SENSOR_IMX682_MAX_DIGITAL_GAIN_SET_VALUE;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!max_dgain);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	cis_data->max_digital_gain[0] = max_dgain_code;
	cis_data->max_digital_gain[1] = sensor_imx682_cis_calc_dgain_permile(max_dgain_code);

	*max_dgain = cis_data->max_digital_gain[1];

	dbg_sensor(1, "[%s] max_dgain_code %d, max_dgain_permile %d\n", __func__,
		cis_data->max_digital_gain[0], cis_data->max_digital_gain[1]);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

	return ret;
}

int sensor_imx682_cis_long_term_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct fimc_is_long_term_expo_mode *lte_mode;
	unsigned char cit_lshift_val = 0;
	int cit_lshift_count = 0;
	u32 target_exp = 0;
	int hold = 0;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	lte_mode = &cis->long_term_mode;

	hold = sensor_imx682_cis_group_param_hold(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);
	/* LTE mode or normal mode set */
	if (lte_mode->sen_strm_off_on_enable) {
		target_exp = lte_mode->expo[0];
		if (target_exp >= 125000) {
			cit_lshift_val = (unsigned char)(target_exp / 125000);
			while (cit_lshift_val > 1) {
				cit_lshift_val /= 2;
				target_exp /= 2;
				cit_lshift_count ++;
			}

			lte_mode->expo[0] = target_exp;

			if (cit_lshift_count > SENSOR_IMX682_MAX_CIT_LSHIFT_VALUE)
				cit_lshift_count = SENSOR_IMX682_MAX_CIT_LSHIFT_VALUE;

			ret = fimc_is_sensor_write8(cis->client, SENSOR_IMX682_CIT_LSHIFT_ADDR, cit_lshift_count);
		}
	} else {
		cit_lshift_count = 0;
		ret = fimc_is_sensor_write8(cis->client, SENSOR_IMX682_CIT_LSHIFT_ADDR, cit_lshift_count);
	}

	I2C_MUTEX_UNLOCK(cis->i2c_lock);

p_err:
	if (hold > 0) {
		hold = sensor_imx682_cis_group_param_hold(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	info("[%s] sen_strm_enable(%d), cit_lshift_count (%d), target_exp(%d)", __func__,
		lte_mode->sen_strm_off_on_enable, cit_lshift_count, lte_mode->expo[0]);

	if (ret < 0)
		pr_err("ERR[%s]: LTE register setting fail\n", __func__);

	return ret;
}

int sensor_imx682_cis_set_wb_gain(struct v4l2_subdev *subdev, struct wb_gains wb_gains)
{
#define ABS_WB_GAIN_NUM	4
	int ret = 0;
	int hold = 0;
	int mode = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	u16 abs_gains[ABS_WB_GAIN_NUM] = {0, };	//[0]=gr, [1]=r, [2]=b, [3]=gb

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (!cis->use_wb_gain)
		return ret;

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	mode = cis->cis_data->sens_config_index_cur;

	if (!IS_REMOSAIC_MODE(cis))
		return 0;

	dbg_sensor(1, "[SEN:%d]%s:DDK vlaue: wb_gain_gr(%d), wb_gain_r(%d), wb_gain_b(%d), wb_gain_gb(%d)\n",
		cis->id, __func__, wb_gains.gr, wb_gains.r, wb_gains.b, wb_gains.gb);

	abs_gains[0] = (u16)((wb_gains.gr / 4) & 0xFFFF);
	abs_gains[1] = (u16)((wb_gains.r / 4) & 0xFFFF);
	abs_gains[2] = (u16)((wb_gains.b / 4) & 0xFFFF);
	abs_gains[3] = (u16)((wb_gains.gb / 4) & 0xFFFF);

	dbg_sensor(1, "[SEN:%d]%s, abs_gain_gr(0x%4X), abs_gain_r(0x%4X), abs_gain_b(0x%4X), abs_gain_gb(0x%4X)\n",
		cis->id, __func__, abs_gains[0], abs_gains[1], abs_gains[2], abs_gains[3]);

	hold = sensor_imx682_cis_group_param_hold(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);
	ret = fimc_is_sensor_write16_array(client, SENSOR_IMX682_ABS_GAIN_GR_SET_ADDR, abs_gains, ABS_WB_GAIN_NUM);
	if (ret < 0)
		goto p_i2c_err;

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_i2c_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

p_err:
	if (hold > 0) {
		hold = sensor_imx682_cis_group_param_hold(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	return ret;
}

void sensor_imx682_cis_data_calc(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;

	FIMC_BUG_VOID(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG_VOID(!cis);
	FIMC_BUG_VOID(!cis->cis_data);

	if (mode > sensor_imx682_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		return;
	}

	/* If check_rev fail when cis_init, one more check_rev in mode_change */
	if (cis->rev_flag == true) {
		cis->rev_flag = false;
		ret = sensor_imx682_cis_check_rev(cis);
		if (ret < 0) {
			err("sensor_imx682_check_rev is fail: ret(%d)", ret);
			return;
		}
	}

	sensor_imx682_cis_data_calculation(sensor_imx682_pllinfos[mode], cis->cis_data);
}

int sensor_imx682_cis_compensate_gain_for_extremely_br(struct v4l2_subdev *subdev, u32 expo, u32 *again, u32 *dgain)
{
	struct fimc_is_cis *cis = NULL;
	int ret = 0;

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);

	/* Gains are controlled by user only in pro mode. Kernel layer is not involved in gains */
	if (cis->cis_data->is_data.scene_mode == AA_SCENE_MODE_PRO_MODE)
		return ret;

	ret = sensor_cis_compensate_gain_for_extremely_br(subdev, expo, again, dgain);
	return ret;
}

static struct fimc_is_cis_ops cis_ops_imx682 = {
	.cis_init = sensor_imx682_cis_init,
	.cis_log_status = sensor_imx682_cis_log_status,
	.cis_group_param_hold = sensor_imx682_cis_group_param_hold,
	.cis_set_global_setting = sensor_imx682_cis_set_global_setting,
	.cis_set_size = sensor_imx682_cis_set_size,
	.cis_mode_change = sensor_imx682_cis_mode_change,
	.cis_stream_on = sensor_imx682_cis_stream_on,
	.cis_stream_off = sensor_imx682_cis_stream_off,
	.cis_wait_streamon = sensor_cis_wait_streamon,
	.cis_wait_streamoff = sensor_cis_wait_streamoff,
	.cis_adjust_frame_duration = sensor_imx682_cis_adjust_frame_duration,
	.cis_set_frame_duration = sensor_imx682_cis_set_frame_duration,
	.cis_set_frame_rate = sensor_imx682_cis_set_frame_rate,
	.cis_set_exposure_time = sensor_imx682_cis_set_exposure_time,
	.cis_get_min_exposure_time = sensor_imx682_cis_get_min_exposure_time,
	.cis_get_max_exposure_time = sensor_imx682_cis_get_max_exposure_time,
	.cis_adjust_analog_gain = sensor_imx682_cis_adjust_analog_gain,
	.cis_set_analog_gain = sensor_imx682_cis_set_analog_gain,
	.cis_get_analog_gain = sensor_imx682_cis_get_analog_gain,
	.cis_get_min_analog_gain = sensor_imx682_cis_get_min_analog_gain,
	.cis_get_max_analog_gain = sensor_imx682_cis_get_max_analog_gain,
	.cis_set_digital_gain = sensor_imx682_cis_set_digital_gain,
	.cis_get_digital_gain = sensor_imx682_cis_get_digital_gain,
	.cis_get_min_digital_gain = sensor_imx682_cis_get_min_digital_gain,
	.cis_get_max_digital_gain = sensor_imx682_cis_get_max_digital_gain,
	.cis_compensate_gain_for_extremely_br = sensor_imx682_cis_compensate_gain_for_extremely_br,
	.cis_set_long_term_exposure = sensor_imx682_cis_long_term_exposure,
	.cis_set_wb_gains = sensor_imx682_cis_set_wb_gain,
	.cis_data_calculation = sensor_imx682_cis_data_calc,
};

int cis_imx682_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	bool use_pdaf = false;
	struct fimc_is_core *core = NULL;
	struct v4l2_subdev *subdev_cis = NULL;
	struct fimc_is_cis *cis = NULL;
	struct fimc_is_device_sensor *device = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	u32 sensor_id[FIMC_IS_STREAM_COUNT] = {0, };
	u32 sensor_id_len;
	const u32 *sensor_id_spec;
	struct device *dev;
	struct device_node *dnode;
	int i;

	FIMC_BUG(!client);
	FIMC_BUG(!fimc_is_dev);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_info("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	dev = &client->dev;
	dnode = dev->of_node;

	if (of_property_read_bool(dnode, "use_pdaf")) {
		use_pdaf = true;
	}

	sensor_id_spec = of_get_property(dnode, "id", &sensor_id_len);
	if (!sensor_id_spec) {
		err("sensor_id num read is fail(%d)", ret);
		goto p_err;
	}

	sensor_id_len /= sizeof(*sensor_id_spec);

	probe_info("%s sensor_id_spec %d, sensor_id_len %d\n", __func__,
			*sensor_id_spec, sensor_id_len);

	ret = of_property_read_u32_array(dnode, "id", sensor_id, sensor_id_len);
	if (ret) {
		err("sensor_id read is fail(%d)", ret);
		goto p_err;
	}

	for (i = 0; i < sensor_id_len; i++) {
		probe_info("%s sensor_id %d\n", __func__, sensor_id[i]);
		device = &core->sensor[sensor_id[i]];

		sensor_peri = find_peri_by_cis_id(device, SENSOR_NAME_IMX682);
		if (!sensor_peri) {
			probe_info("sensor peri is not yet probed");
			return -EPROBE_DEFER;
		}

		cis = &sensor_peri->cis;
		subdev_cis = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
		if (!subdev_cis) {
			probe_err("subdev_cis is NULL");
			ret = -ENOMEM;
			goto p_err;
		}

		sensor_peri->subdev_cis = subdev_cis;

		cis->id = SENSOR_NAME_IMX682;
		cis->subdev = subdev_cis;
		cis->device = sensor_id[i];
		cis->client = client;
		sensor_peri->module->client = cis->client;
		cis->i2c_lock = NULL;
		cis->ctrl_delay = N_PLUS_TWO_FRAME;

		cis->cis_data = kzalloc(sizeof(cis_shared_data), GFP_KERNEL);
		if (!cis->cis_data) {
			err("cis_data is NULL");
			ret = -ENOMEM;
			goto p_err;
		}

		cis->cis_ops = &cis_ops_imx682;

		/* belows are depend on sensor cis. MUST check sensor spec */
		cis->bayer_order = OTF_INPUT_ORDER_BAYER_RG_GB;

		if (of_property_read_bool(dnode, "sensor_f_number")) {
			ret = of_property_read_u32(dnode, "sensor_f_number", &cis->aperture_num);
			if (ret) {
				warn("f-number read is fail(%d)", ret);
			}
		} else {
			cis->aperture_num = F2_2;
		}

		probe_info("%s f-number %d\n", __func__, cis->aperture_num);

		cis->use_dgain = true;
		cis->hdr_ctrl_by_again = false;
		cis->use_wb_gain = true;
		cis->use_pdaf = use_pdaf;

		v4l2_set_subdevdata(subdev_cis, cis);
		v4l2_set_subdev_hostdata(subdev_cis, device);
		snprintf(subdev_cis->name, V4L2_SUBDEV_NAME_SIZE, "cis-subdev.%d", cis->id);
	}

	probe_info("%s done\n", __func__);

p_err:
	return ret;
}

static const struct of_device_id sensor_cis_imx682_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-cis-imx682",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_cis_imx682_match);

static const struct i2c_device_id sensor_cis_imx682_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_cis_imx682_driver = {
	.probe	= cis_imx682_probe,
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_cis_imx682_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_cis_imx682_idt
};

static int __init sensor_cis_imx682_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_cis_imx682_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_cis_imx682_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_cis_imx682_init);
