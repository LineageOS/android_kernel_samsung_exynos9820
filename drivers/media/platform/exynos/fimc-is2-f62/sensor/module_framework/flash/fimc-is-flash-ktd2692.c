/*
 * Samsung Exynos5 SoC series Flash driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>

#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>

#ifndef CONFIG_LEDS_KTD2692
#include "fimc-is-flash.h"
#endif

#include "fimc-is-flash-ktd2692.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-core.h"

#if defined(CONFIG_LEDS_KTD2692)
#include <linux/leds-ktd2692.h>
extern int32_t ktd2692_led_mode_ctrl(int state, u32 intensity);
#endif

static int flash_ktd2692_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	struct fimc_is_flash *flash;

	BUG_ON(!subdev);

	flash = (struct fimc_is_flash *)v4l2_get_subdevdata(subdev);

	BUG_ON(!flash);

	/* TODO: init flash driver */
	flash->flash_data.mode = CAM2_FLASH_MODE_OFF;
	flash->flash_data.intensity = 100; /* TODO: Need to figure out min/max range */
	flash->flash_data.firing_time_us = 1 * 1000 * 1000; /* Max firing time is 1sec */
	flash->flash_data.flash_fired = false;

#if defined(CONFIG_LEDS_KTD2692)
#else
	gpio_request_one(flash->flash_gpio, GPIOF_OUT_INIT_LOW, "CAM_FLASH_GPIO_OUTPUT");
	gpio_free(flash->flash_gpio);
#endif

	return ret;
}

static int sensor_ktd2692_flash_control(struct v4l2_subdev *subdev, enum flash_mode mode, u32 intensity)
{
	int ret = 0;
	struct fimc_is_flash *flash = NULL;

	BUG_ON(!subdev);

	flash = (struct fimc_is_flash *)v4l2_get_subdevdata(subdev);
	BUG_ON(!flash);

	dbg_flash("%s : mode = %s, intensity = %d\n", __func__,
		mode == CAM2_FLASH_MODE_OFF ? "OFF" :
		mode == CAM2_FLASH_MODE_SINGLE ? "FLASH" : "TORCH",
		intensity);

	if (mode == CAM2_FLASH_MODE_OFF) {
#if defined(CONFIG_LEDS_KTD2692)
		ret = ktd2692_led_mode_ctrl(CAM2_FLASH_MODE_OFF, 0);
#else
		ret = control_flash_gpio(flash->flash_gpio, 0);
#endif
		if (ret)
			err("capture flash off fail");
	} else if (mode == CAM2_FLASH_MODE_SINGLE) {
#if defined(CONFIG_LEDS_KTD2692)
#ifdef CONFIG_FLASH_CURRENT_CHANGE_SUPPORT
		if (intensity > 500) {
			ret = ktd2692_led_mode_ctrl(CAM2_FLASH_MODE_SINGLE, intensity);
			if (ret)
				err("capture flash on fail");
		} else {
			ret = ktd2692_led_mode_ctrl(CAM2_FLASH_MODE_TORCH, intensity);
			if (ret)
				err("capture flash on fail");
		}
#else
		ret = ktd2692_led_mode_ctrl(CAM2_FLASH_MODE_SINGLE, 0);
#endif
#else
		ret = control_flash_gpio(flash->flash_gpio, intensity);
#endif
		if (ret)
			err("capture flash on fail");
	} else if (mode == CAM2_FLASH_MODE_TORCH) {
#if defined(CONFIG_LEDS_KTD2692)
		ret = ktd2692_led_mode_ctrl(CAM2_FLASH_MODE_TORCH, 0);
#else
		ret = control_flash_gpio(flash->torch_gpio, intensity);
#endif
		if (ret)
			err("torch flash on fail");
	} else {
		err("Invalid flash mode");
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

int flash_ktd2692_s_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;
	struct fimc_is_flash *flash = NULL;

	BUG_ON(!subdev);

	flash = (struct fimc_is_flash *)v4l2_get_subdevdata(subdev);
	BUG_ON(!flash);

	switch(ctrl->id) {
	case V4L2_CID_FLASH_SET_INTENSITY:
		/* TODO : Check min/max intensity */
		if (ctrl->value < 0) {
			err("failed to flash set intensity: %d\n", ctrl->value);
			ret = -EINVAL;
			goto p_err;
		}
		flash->flash_data.intensity = ctrl->value;
		break;
	case V4L2_CID_FLASH_SET_FIRING_TIME:
		/* TODO : Check min/max firing time */
		if (ctrl->value < 0) {
			err("failed to flash set firing time: %d\n", ctrl->value);
			ret = -EINVAL;
			goto p_err;
		}
		flash->flash_data.firing_time_us = ctrl->value;
		break;
	case V4L2_CID_FLASH_SET_FIRE:
		ret =  sensor_ktd2692_flash_control(subdev, flash->flash_data.mode, ctrl->value);
		if (ret) {
			err("sensor_ktd2692_flash_control(mode:%d, val:%d) is fail(%d)",
					(int)flash->flash_data.mode, ctrl->value, ret);
			goto p_err;
		}
		break;
	default:
		err("err!!! Unknown CID(%#x)", ctrl->id);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = flash_ktd2692_init,
	.s_ctrl = flash_ktd2692_s_ctrl,
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
};

int flash_ktd2692_probe(struct device *dev, struct i2c_client *client)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct v4l2_subdev *subdev_flash;
	struct fimc_is_device_sensor *device;
	u32 sensor_id[FIMC_IS_STREAM_COUNT] = {0, };
	u32 place = 0;
	struct fimc_is_flash *flash;
	struct device_node *dnode;
	const u32 *sensor_id_spec;
	u32 sensor_id_len;
	int i = 0;

	BUG_ON(!fimc_is_dev);
	BUG_ON(!dev);
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

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_info("core device is not yet probed");
		ret = -EPROBE_DEFER;
		goto p_err;
	}

	for (i = 0; i < sensor_id_len; i++) {
		ret = of_property_read_u32(dnode, "place", &place);
		if (ret) {
			pr_info("place read is fail(%d)", ret);
			place = 0;
			ret = 0;
		}

		probe_info("%s sensor_id(%d) flash_place(%d)\n", __func__, sensor_id[i], place);
		device = &core->sensor[sensor_id[i]];
		if (!device) {
			err("sensor device is NULL");
			ret = -ENOMEM;
			goto p_err;
		}

		flash = kzalloc(sizeof(struct fimc_is_flash), GFP_KERNEL);
		if (!flash) {
			err("flash is NULL");
			ret = -ENOMEM;
			goto p_err;
		}

		subdev_flash = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
		if (!subdev_flash) {
			err("subdev_flash is NULL");
			ret = -ENOMEM;
			kfree(flash);
			goto p_err;
		}

		flash->id = FLADRV_NAME_KTD2692;
		flash->subdev = subdev_flash;
		flash->client = client;
		flash->device = sensor_id[i];
		device->subdev_flash = subdev_flash;
		device->flash = flash;
		//core->client3 = client;

#if defined(CONFIG_LEDS_KTD2692)
#else
		flash->flash_gpio = of_get_named_gpio(dnode, "flash-gpio", 0);
		if (!gpio_is_valid(flash->flash_gpio)) {
			dev_err(dev, "failed to get PIN_RESET\n");
			return -EINVAL;
		}
#endif

		flash->flash_data.mode = CAM2_FLASH_MODE_OFF;
		flash->flash_data.intensity = 100; /* TODO: Need to figure out min/max range */
		flash->flash_data.firing_time_us = 1 * 1000 * 1000; /* Max firing time is 1sec */

		v4l2_subdev_init(subdev_flash, &subdev_ops);

		v4l2_set_subdevdata(subdev_flash, flash);
		v4l2_set_subdev_hostdata(subdev_flash, device);
		snprintf(subdev_flash->name, V4L2_SUBDEV_NAME_SIZE, "flash-subdev.%d", flash->id);

	};
p_err:
	return ret;
}

int flash_ktd2692_platform_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev;

	BUG_ON(!pdev);

	dev = &pdev->dev;

	ret = flash_ktd2692_probe(dev, NULL);
	if (ret < 0) {
		probe_err("flash gpio probe fail(%d)\n", ret);
		goto p_err;
	}

	probe_info("%s done\n", __func__);

p_err:
	return ret;
}

static int flash_ktd2692_platform_remove(struct platform_device *pdev)
{
		int ret = 0;

		info("%s\n", __func__);

		return ret;
}

static const struct of_device_id exynos_fimc_is_sensor_flash_ktd2692_match[] = {
	{
		.compatible = "samsung,sensor-flash-ktd2692",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_sensor_flash_ktd2692_match);

/* register platform driver */
static struct platform_driver sensor_flash_ktd2692_platform_driver = {
	.probe  = flash_ktd2692_platform_probe,
	.remove = flash_ktd2692_platform_remove,
	.driver = {
		.name   = "FIMC-IS-SENSOR-FLASH-KTD2692-PLATFORM",
		.owner  = THIS_MODULE,
		.of_match_table = exynos_fimc_is_sensor_flash_ktd2692_match,
	}
};
module_platform_driver(sensor_flash_ktd2692_platform_driver);
