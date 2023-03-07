/*
 * Samsung Exynos5 SoC series Actuator driver
 *
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>

#include "fimc-is-actuator-dw9818.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-core.h"
#include "fimc-is-helper-actuator-i2c.h"

#include "interface/fimc-is-interface-library.h"

#define ACTUATOR_NAME		"DW9818"

#define REG_CONTROL     0x02 // Default: 0x00, R/W, [1] = RING, [0] = PD(Power Down mode)
#define REG_VCM_MSB     0x03 // Default: 0x00, R/W, [1:0] = Pos[9:8]
#define REG_VCM_LSB     0x04 // Default: 0x00, R/W, [7:0] = Pos[7:0]
#define REG_STATUS      0x05 // Default: 0x00, R,   [1] = MBUSY(eFlash busy), [0] = VBUSY(VCM busy)
#define REG_MODE        0x06 // Default: 0x00, R/W, [3] = RING [1:0] = (SAC1~0)
#define REG_PRESCALE    0x07 // Default: 0x60, R/W, [1:0] = PRESC
#define REG_SACTIMING   0x08 // Default: 0x60, R/W, [5:0] = SACT
#define REG_PRESET      0x0A // Default: 0x60, R/W, [5:0] = SACT
#define REG_NRC_EN      0x0B // Default: 0x60, R/W, [5:0] = SACT
#define REG_NRC_STEP    0x0C // Default: 0x60, R/W, [5:0] = SACT
#define REG_MPK         0x10 // Default: 0x60, R/W, [5:0] = SACT

#define DEF_DW9818_FIRST_POSITION		512
#define DEF_DW9818_FIRST_DELAY			20
#define DEF_DW9818_SECOND_DELAY			10

#define DEF_DW9818_OFFSET_POSITION_MAX 60
#define DEF_DW9818_AF_PAN 450
#define DEF_DW9818_PRESET_MAX 255

#define REAR_CAL_AF_PAN_ADDR 0x2634 /* AF Far position */

extern struct fimc_is_lib_support gPtr_lib_support;
extern struct fimc_is_sysfs_actuator sysfs_actuator;

int sensor_dw9818_init(struct i2c_client *client, struct fimc_is_caldata_list_dw9818 *cal_data)
{
	int ret = 0;
	u8 i2c_data[2];
	u32 control_mode, pre_scale, sac_time;

	probe_info("%s start\n", __func__);

	if (!cal_data) {
		/* PD(Power Down) mode enable */
		i2c_data[0] = REG_CONTROL;
		i2c_data[1] = 0x01;
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;

		/* PD disable(normal operation) */
		i2c_data[0] = REG_CONTROL;
		i2c_data[1] = 0x00;
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;

		/* wait 5ms after power-on for DW9818 */
		usleep_range(PWR_ON_DELAY, PWR_ON_DELAY);

		/* Ring mode enable & SAC write
		 * RING mode [3] SAC[1:0] setting */
		i2c_data[0] = REG_MODE;
		i2c_data[1] = ((0x01 << 3) | (0x03 << 5));
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;

		/*
		 * PRESC[1:0] mode setting
		 */
		pre_scale = 0x01;
		i2c_data[0] = REG_PRESCALE;
		i2c_data[1] = (pre_scale & 0xff);
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;

		/*
		 * SAC TIMING[6:0] mode setting
		 */
		i2c_data[0] = REG_SACTIMING;
		i2c_data[1] = 0x36;
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;
		usleep_range(INIT_DELAY, INIT_DELAY);
	} else {
		/* PD(Power Down) mode enable */
		i2c_data[0] = REG_CONTROL;
		i2c_data[1] = 0x01;
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;

		/* PD disable(normal operation) */
		i2c_data[0] = REG_CONTROL;
		i2c_data[1] = 0x00;
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;

		/* wait 5ms after power-on for DW9818 */
		usleep_range(PWR_ON_DELAY, PWR_ON_DELAY);

		control_mode = cal_data->control_mode;
		pre_scale = cal_data->prescale;
		sac_time = cal_data->resonance;

		/* If SAC data is NULL, set SAC default values as camera group requested */
		if ((control_mode == 0xFF && pre_scale == 0xFF && sac_time == 0xFF)
			|| (control_mode == 0x00 && pre_scale == 0x00 && sac_time == 0x00)) {
			dbg_actuator("[%s] Set default SAC data (SAC data from cal : %02X %02X %02X)",
				__func__, control_mode, pre_scale, sac_time);
			control_mode = 0x0A;
			pre_scale = 0x01;
			sac_time = 0x49;
		}

		dbg_actuator("[%s]AF Cal data: control_mode=0x%2x, pre_scale=0x%2x\n", __func__, control_mode, pre_scale);
		dbg_actuator("[%s]AF Cal data: sac_time=0x%2x\n", __func__, sac_time);

		/* Ring mode enable & SAC write
		 * RING mode [3] SAC[1:0] setting */
		i2c_data[0] = REG_MODE;
		i2c_data[1] = ((0x01 << 3) |((control_mode & 0xff)) );
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;

		/*
		 * PRESC[1:0] mode setting
		 */
		i2c_data[0] = REG_PRESCALE;
		i2c_data[1] = (pre_scale & 0xff);
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;

		/*
		 * SAC TIMING[6:0] mode setting
		 */
		i2c_data[0] = REG_SACTIMING;
		i2c_data[1] = sac_time;
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;
		usleep_range(INIT_DELAY, INIT_DELAY);
	}

p_err:
	return ret;
}

static int sensor_dw9818_write_position(struct i2c_client *client, u32 val)
{
	int ret = 0;
	u8 val_high = 0, val_low = 0;

	BUG_ON(!client);

	if (!client->adapter) {
		err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	if (val > DW9818_POS_MAX_SIZE) {
		err("Invalid af position(position : %d, Max : %d).\n",
					val, DW9818_POS_MAX_SIZE);
		ret = -EINVAL;
		goto p_err;
	}

	/*
	 * val_high is position VCM_MSB[9:8],
	 * val_low is position VCM_LSB[7:0]
	 */
	val_high = (val & 0x300) >> 8;
	val_low = (val & 0x00FF);

	ret = fimc_is_sensor_addr_data_write16(client, REG_VCM_MSB, val_high, val_low);

p_err:
	return ret;
}

/*
static int sensor_dw9818_valid_check(struct i2c_client * client)
{
	int i;

	BUG_ON(!client);

	if (sysfs_actuator.init_step > 0) {
		for (i = 0; i < sysfs_actuator.init_step; i++) {
			if (sysfs_actuator.init_positions[i] < 0) {
				warn("invalid position value, default setting to position");
				return 0;
			} else if (sysfs_actuator.init_delays[i] < 0) {
				warn("invalid delay value, default setting to delay");
				return 0;
			}
		}
	} else
		return 0;

	return sysfs_actuator.init_step;
}

static void sensor_dw9818_print_log(int step)
{
	int i;

	if (step > 0) {
		dbg_actuator("initial position ");
		for (i = 0; i < step; i++) {
			dbg_actuator(" %d", sysfs_actuator.init_positions[i]);
		}
		dbg_actuator(" setting");
	}
}
*/
static int sensor_dw9818_init_position(struct i2c_client *client,
		struct fimc_is_actuator *actuator, int pos)
{
	int ret = 0;

	ret = sensor_dw9818_write_position(client, DEF_DW9818_FIRST_POSITION);
	if (ret < 0)
		goto p_err;

	msleep(DEF_DW9818_FIRST_DELAY);

	dbg_actuator("First initial position %d, setting\n", DEF_DW9818_FIRST_POSITION);

	if (pos <= 0 || pos > 1023) {
		actuator->position = DEF_DW9818_FIRST_POSITION;
		dbg_actuator("Second initial position (pan value on cal) is invalid (%d)", pos);
	} else {
		ret = sensor_dw9818_write_position(client, pos);
		if (ret < 0)
			goto p_err;

		msleep(DEF_DW9818_SECOND_DELAY);
		actuator->position = pos;
		dbg_actuator("Second initial position %d, setting\n", pos);
	}

p_err:
	return ret;
}

int sensor_dw9818_actuator_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	struct fimc_is_actuator *actuator;
	struct fimc_is_caldata_list_dw9818 *cal_data = NULL;
	struct i2c_client *client = NULL;
	long cal_addr;

#ifdef USE_CAMERA_HW_BIG_DATA
	struct cam_hw_param *hw_param = NULL;
	struct fimc_is_device_sensor *device = NULL;
#endif

#ifdef DEBUG_ACTUATOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	BUG_ON(!subdev);

	dbg_actuator("%s\n", __func__);

	actuator = (struct fimc_is_actuator *)v4l2_get_subdevdata(subdev);
	if (!actuator) {
		err("actuator is not detect!\n");
		goto p_err;
	}

	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/* EEPROM AF calData address */
	if (gPtr_lib_support.binary_load_flg) {
		cal_addr = gPtr_lib_support.minfo->kvaddr_cal[SENSOR_POSITION_REAR] + EEPROM_OEM_BASE;
		cal_data = (struct fimc_is_caldata_list_dw9818 *)(cal_addr);
	} else {
		cal_data = NULL;
	}

	/* Read into EEPROM data or default setting */
	ret = sensor_dw9818_init(client, cal_data);
	if (ret < 0){
#ifdef USE_CAMERA_HW_BIG_DATA
		device = v4l2_get_subdev_hostdata(subdev);
		if (device)
			fimc_is_sec_get_hw_param(&hw_param, device->position);
		if (hw_param)
			hw_param->i2c_af_err_cnt++;
#endif
		goto p_err;
	}

	if (gPtr_lib_support.binary_load_flg) {
		int pos_pan;

		cal_addr = gPtr_lib_support.minfo->kvaddr_cal[SENSOR_POSITION_REAR] + REAR_CAL_AF_PAN_ADDR;
		pos_pan = *((int *)cal_addr) * 2;	/* Pan value in cal is 9 bit, so it should be doubled */

		ret = sensor_dw9818_init_position(client, actuator, pos_pan);
		if (ret < 0)
			goto p_err;
	}

#ifdef DEBUG_ACTUATOR_TIME
	do_gettimeofday(&end);
	pr_info("[%s] time %lu us", __func__, (end.tv_sec - st.tv_sec) * 1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_dw9818_actuator_get_status(struct v4l2_subdev *subdev, u32 *info)
{
	int ret = 0;
	u8 val = 0;
	struct fimc_is_actuator *actuator = NULL;
	struct i2c_client *client = NULL;
#ifdef DEBUG_ACTUATOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	dbg_actuator("%s\n", __func__);

	BUG_ON(!subdev);
	BUG_ON(!info);

	actuator = (struct fimc_is_actuator *)v4l2_get_subdevdata(subdev);
	BUG_ON(!actuator);

	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/* If you need check the position value, use this */
#if 0
	/* Read VCM_MSB(0x03) pos[9:8] and VCM_LSB(0x04) pos[7:0] */
	ret = fimc_is_sensor_addr8_read8(client, REG_VCM_MSB, &val);
	data = (val & 0x0300);
	ret = fimc_is_sensor_addr8_read8(client, REG_VCM_LSB, &val);
	data |= (val & 0x00ff);
#endif

	ret = fimc_is_sensor_addr8_read8(client, REG_STATUS, &val);
	if (ret < 0)
		return ret;

	/* If data is 1 of 0x1 and 0x2 bit, will have to actuator not move */
	*info = ((val & 0x3) == 0) ? ACTUATOR_STATUS_NO_BUSY : ACTUATOR_STATUS_BUSY;

#ifdef DEBUG_ACTUATOR_TIME
	do_gettimeofday(&end);
	pr_info("[%s] time %lu us", __func__, (end.tv_sec - st.tv_sec) * 1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

#ifdef USE_CAMERA_ACT_DRIVER_SOFT_LANDING
int sensor_dw9818_actuator_wait_busy(struct v4l2_subdev *subdev)
{
	u32 info;
	int count = 0;
	msleep(5);
	do {
		sensor_dw9818_actuator_get_status(subdev, &info);
		if (info == ACTUATOR_STATUS_BUSY) {
			msleep(10);
		}
		count += 1;
	}while (info == ACTUATOR_STATUS_BUSY && count < 15);
	return 0;
}

int sensor_dw9818_actuator_soft_landing(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u8 val = 0;
	int interval = 0;
	int position = 0;
	struct fimc_is_actuator *actuator;
	struct i2c_client *client;
	int i = 0;

#ifdef DEBUG_ACTUATOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	BUG_ON(!subdev);
	actuator = (struct fimc_is_actuator *)v4l2_get_subdevdata(subdev);
	BUG_ON(!actuator);
	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}
	sensor_dw9818_actuator_wait_busy(subdev);

	/* Read VCM_MSB(0x03) pos[9:8] and VCM_LSB(0x04) pos[7:0] */
	ret = fimc_is_sensor_addr8_read8(client, REG_VCM_MSB, &val);
	if (ret < 0)
		goto p_err;
	position = (val & 0x03) << 8;
	ret = fimc_is_sensor_addr8_read8(client, REG_VCM_LSB, &val);
	if (ret < 0)
		goto p_err;
	position |= (val & 0x00ff);

	interval = (DEF_DW9818_FIRST_POSITION - position) / 3;

	for(i = 0; i < 3; i++)
	{
		position += interval;
		ret = sensor_dw9818_write_position(client, position);
		if (ret < 0)
			goto p_err;
		sensor_dw9818_actuator_wait_busy(subdev);
	}

	pr_info("[%s] Softlanding Successful, final position: [%x]\n",__func__, DEF_DW9818_FIRST_POSITION);
	return ret;
p_err:
	err("[%s] Actuator Softlanding Failed \n", __func__);
	return ret;
}
#endif
int sensor_dw9818_actuator_set_position(struct v4l2_subdev *subdev, u32 *info)
{
	int ret = 0;
	struct fimc_is_actuator *actuator;
	struct i2c_client *client;
	u32 position = 0;
#ifdef DEBUG_ACTUATOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	BUG_ON(!subdev);
	BUG_ON(!info);

	actuator = (struct fimc_is_actuator *)v4l2_get_subdevdata(subdev);
	BUG_ON(!actuator);

	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	position = *info;
	if (position > DW9818_POS_MAX_SIZE) {
		err("Invalid af position(position : %d, Max : %d).\n",
					position, DW9818_POS_MAX_SIZE);
		ret = -EINVAL;
		goto p_err;
	}

	/* debug option : fixed position testing */
	if (sysfs_actuator.enable_fixed)
		position = sysfs_actuator.fixed_position;

	/* position Set */
	ret = sensor_dw9818_write_position(client, position);
	if (ret <0)
		goto p_err;
	actuator->position = position;

	dbg_actuator("%s: position(%d)\n", __func__, position);

#ifdef DEBUG_ACTUATOR_TIME
	do_gettimeofday(&end);
	pr_info("[%s] time %lu us", __func__, (end.tv_sec - st.tv_sec) * 1000000 + (end.tv_usec - st.tv_usec));
#endif
p_err:
	return ret;
}

static int sensor_dw9818_actuator_g_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;
	u32 val = 0;

	switch(ctrl->id) {
	case V4L2_CID_ACTUATOR_GET_STATUS:
		ret = sensor_dw9818_actuator_get_status(subdev, &val);
		if (ret < 0) {
			err("err!!! ret(%d), actuator status(%d)", ret, val);
			ret = -EINVAL;
			goto p_err;
		}
		break;
	default:
		err("err!!! Unknown CID(%#x)", ctrl->id);
		ret = -EINVAL;
		goto p_err;
	}

	ctrl->value = val;

p_err:
	return ret;
}

static int sensor_dw9818_actuator_s_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;

	switch(ctrl->id) {
	case V4L2_CID_ACTUATOR_SET_POSITION:
		ret = sensor_dw9818_actuator_set_position(subdev, &ctrl->value);
		if (ret) {
			err("failed to actuator set position: %d, (%d)\n", ctrl->value, ret);
			ret = -EINVAL;
			goto p_err;
		}
		break;
#ifdef USE_CAMERA_ACT_DRIVER_SOFT_LANDING
	case V4L2_CID_ACTUATOR_SOFT_LANDING:
		ret = sensor_dw9818_actuator_soft_landing(subdev);
		if(ret == HW_SOFTLANDING_FAIL) {
			err("[%s] NRC Softlanding Failed \n",__func__);
			goto p_err;
		}
		if (ret) {
			err("[%s] Actuator Softlanding Failed  \n", __func__);
			ret = -EINVAL;
			goto p_err;
		}
		break;
#endif
	default:
		err("err!!! Unknown CID(%#x)", ctrl->id);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_dw9818_actuator_init,
	.g_ctrl = sensor_dw9818_actuator_g_ctrl,
	.s_ctrl = sensor_dw9818_actuator_s_ctrl,
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
};

int sensor_dw9818_actuator_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = 0;
	struct fimc_is_core *core= NULL;
	struct v4l2_subdev *subdev_actuator = NULL;
	struct fimc_is_actuator *actuator = NULL;
	struct fimc_is_device_sensor *device = NULL;
	u32 sensor_id[FIMC_IS_STREAM_COUNT] = {0, };
	u32 place = 0;
	struct device *dev;
	struct device_node *dnode;
	const u32 *sensor_id_spec;
	u32 sensor_id_len;
	int i = 0;

	BUG_ON(!fimc_is_dev);
	BUG_ON(!client);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core device is not yet probed");
		ret = -EPROBE_DEFER;
		goto p_err;
	}

	dev = &client->dev;
	dnode = dev->of_node;

	sensor_id_spec = of_get_property(dnode, "id", &sensor_id_len);
	if (!sensor_id_spec) {
		err("sensor_id num read is fail(%d)", ret);
		goto p_err;
	}

	sensor_id_len /= sizeof(*sensor_id_spec);

	ret = of_property_read_u32_array(dnode, "id", sensor_id, sensor_id_len);
	if (ret) {
		err("sensor_id read is fail(%d)", ret);
		goto p_err;
	}

	for (i = 0; i < sensor_id_len; i++) {
		ret = of_property_read_u32(dnode, "place", &place);
		if (ret) {
			pr_info("place read is fail(%d)", ret);
			place = 0;
		}
		probe_info("%s sensor_id(%d) actuator_place(%d)\n", __func__, sensor_id[i], place);

		device = &core->sensor[sensor_id[i]];

		actuator = kzalloc(sizeof(struct fimc_is_actuator), GFP_KERNEL);
		if (!actuator) {
			err("actuator is NULL");
			ret = -ENOMEM;
			goto p_err;
		}

		subdev_actuator = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
		if (!subdev_actuator) {
			err("subdev_actuator is NULL");
			ret = -ENOMEM;
			kfree(actuator);
			goto p_err;
		}

		/* This name must is match to sensor_open_extended actuator name */
		actuator->id = ACTUATOR_NAME_DW9818;
		actuator->subdev = subdev_actuator;
		actuator->device = sensor_id[i];
		actuator->client = client;
		actuator->position = 0;
		actuator->max_position = DW9818_POS_MAX_SIZE;
		actuator->pos_size_bit = DW9818_POS_SIZE_BIT;
		actuator->pos_direction = DW9818_POS_DIRECTION;
		actuator->i2c_lock = NULL;

		device->subdev_actuator[place] = subdev_actuator;
		device->actuator[place] = actuator;

		v4l2_i2c_subdev_init(subdev_actuator, client, &subdev_ops);
		v4l2_set_subdevdata(subdev_actuator, actuator);
		v4l2_set_subdev_hostdata(subdev_actuator, device);

		snprintf(subdev_actuator->name, V4L2_SUBDEV_NAME_SIZE, "actuator-subdev.%d", actuator->id);
	}

	probe_info("%s done\n", __func__);
	return ret;
p_err:
	if (subdev_actuator)
		kzfree(subdev_actuator);

	return ret;
}

static int sensor_dw9818_actuator_remove(struct i2c_client *client)
{
	int ret = 0;

	return ret;
}

static const struct of_device_id exynos_is_dw9818_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-actuator-dw9818",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_is_dw9818_match);

static const struct i2c_device_id actuator_dw9818_idt[] = {
	{ ACTUATOR_NAME, 0 },
	{},
};

static struct i2c_driver actuator_dw9818_driver = {
	.driver = {
		.name	= ACTUATOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = exynos_is_dw9818_match
	},
	.probe	= sensor_dw9818_actuator_probe,
	.remove	= sensor_dw9818_actuator_remove,
	.id_table = actuator_dw9818_idt
};
module_i2c_driver(actuator_dw9818_driver);
