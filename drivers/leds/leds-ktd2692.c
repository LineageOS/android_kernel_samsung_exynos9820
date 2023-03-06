/*
 * LED driver - leds-ktd2692.c
 *
 * Copyright (C) 2020 Sunggeun Yim <sunggeun.yim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/pwm.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/leds-ktd2692.h>
#include <linux/time.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif

//#define DEBUG_LED_TIME

extern struct class *camera_class; /*sys/class/camera*/
struct device *ktd2692_dev;

struct ktd2692_platform_data *global_ktd2692data = NULL;
struct device *global_dev;

void ktd2692_setGpio(int onoff)
{
	if (onoff) {
		__gpio_set_value(global_ktd2692data->flash_control, 1);
	} else {
		__gpio_set_value(global_ktd2692data->flash_control, 0);
	}
}

void ktd2692_set_low_bit(void)
{
#ifdef DEBUG_LED_TIME
	struct timeval start_low, end_low, start_high, end_high;
	unsigned long time_low, time_high;
#endif

#ifdef DEBUG_LED_TIME
	do_gettimeofday(&start_low);
#endif

	__gpio_set_value(global_ktd2692data->flash_control, 0);
	udelay(T_L_LB);

#ifdef DEBUG_LED_TIME
	do_gettimeofday(&end_low);

	do_gettimeofday(&start_high);
#endif

	__gpio_set_value(global_ktd2692data->flash_control, 1);
	udelay(T_H_LB);

#ifdef DEBUG_LED_TIME
	do_gettimeofday(&end_high);

	time_low = (end_low.tv_sec - start_low.tv_sec) * 1000000 + (end_low.tv_usec - start_low.tv_usec);
	time_high = (end_high.tv_sec - start_high.tv_sec) * 1000000 + (end_high.tv_usec - start_high.tv_usec);

	LED_INFO("[ta] LOW BIT: time_low(%lu) / time_high(%lu)\n", time_low, time_high);
	if (time_low <= (time_high*2)) 
		LED_ERROR("[ta] Low Bit: high pulse too long\n");
#endif
}

void ktd2692_set_high_bit(void)
{
#ifdef DEBUG_LED_TIME
	struct timeval start_low, end_low, start_high, end_high;
	unsigned long time_low, time_high;
#endif

#ifdef DEBUG_LED_TIME
	do_gettimeofday(&start_low);
#endif

	__gpio_set_value(global_ktd2692data->flash_control, 0);
	udelay(T_L_HB);

#ifdef DEBUG_LED_TIME
	do_gettimeofday(&end_low);

	do_gettimeofday(&start_high);
#endif

	__gpio_set_value(global_ktd2692data->flash_control, 1);
	udelay(T_H_HB);

#ifdef DEBUG_LED_TIME
	do_gettimeofday(&end_high);

	time_low = (end_low.tv_sec - start_low.tv_sec) * 1000000 + (end_low.tv_usec - start_low.tv_usec);
	time_high = (end_high.tv_sec - start_high.tv_sec) * 1000000 + (end_high.tv_usec - start_high.tv_usec);

	LED_INFO("[ta] HIGHT BIT: time_low(%lu) / time_high(%lu)\n", time_low, time_high);
	if ((time_low*2) >= time_high) 
		LED_ERROR("[ta] HIGH BIT: low pulse too long\n");
#endif
}

static int ktd2692_set_bit(unsigned int bit)
{
	if (bit) {
		ktd2692_set_high_bit();
	} else {
		ktd2692_set_low_bit();
	}
	return 0;
}

static int ktd2692_write_data(unsigned data)
{
	int err = 0;
	unsigned int bit = 0;

	/* Data Start Condition */
	__gpio_set_value(global_ktd2692data->flash_control, 1);
	udelay(T_SOD);

	/* BIT 7*/
	bit = ((data>> 7) & 0x01);
	ktd2692_set_bit(bit);

	/* BIT 6 */
	bit = ((data>> 6) & 0x01);
	ktd2692_set_bit(bit);

	/* BIT 5*/
	bit = ((data>> 5) & 0x01);
	ktd2692_set_bit(bit);

	/* BIT 4 */
	bit = ((data>> 4) & 0x01);
	ktd2692_set_bit(bit);

	/* BIT 3*/
	bit = ((data>> 3) & 0x01);
	ktd2692_set_bit(bit);

	/* BIT 2 */
	bit = ((data>> 2) & 0x01);
	ktd2692_set_bit(bit);

	/* BIT 1*/
	bit = ((data>> 1) & 0x01);
	ktd2692_set_bit(bit);

	/* BIT 0 */
	bit = ((data>> 0) & 0x01);
	ktd2692_set_bit(bit);

	 __gpio_set_value(global_ktd2692data->flash_control, 0);
	udelay(T_EOD_L);

	/* Data End Condition */
	__gpio_set_value(global_ktd2692data->flash_control, 1);
	udelay(T_EOD_H);

	return err;
}

ssize_t ktd2692_store(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	int value = 0;
	int brightness_value = 0;
	int ret = 0;
	unsigned long flags = 0;
	int torch_intensity = -1;
	struct pinctrl *pinctrl;

	if ((buf == NULL) || kstrtouint(buf, 10, &value)) {
		return -1;
	}

	if (global_ktd2692data == NULL) {
		LED_ERROR("KTD2692(%s) global_ktd2692data is not initialized.\n", __func__);
		return -EFAULT;
	}

	global_ktd2692data->sysfs_input_data = value;

	if (value <= 0) {
		ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
		if (ret) {
			LED_ERROR("Failed to requeset ktd2692_led_control\n");
		} else {
			LED_INFO("KTD2692-TORCH OFF. : E(%d)\n", value);

			global_ktd2692data->mode_status = KTD2692_DISABLES_MOVIE_FLASH_MODE;
			spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
			ktd2692_write_data(global_ktd2692data->mode_status|
								KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
			spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);

			ktd2692_setGpio(0);
			gpio_free(global_ktd2692data->flash_control);
			global_ktd2692data->is_torch_enable = false;
			LED_INFO("KTD2692-TORCH OFF. : X(%d)\n", value);
		}

		pinctrl = devm_pinctrl_get_select(global_dev, "is");
		if (IS_ERR(pinctrl))
			pr_err("%s: flash %s pins are not configured\n", __func__, "is");

	} else {
		pinctrl = devm_pinctrl_get_select(global_dev, "host");
		if (IS_ERR(pinctrl))
			pr_err("%s: flash %s pins are not configured\n", __func__, "host");

		ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
		if (ret) {
			LED_ERROR("Failed to requeset ktd2692_led_control\n");
		} else {
			LED_INFO("KTD2692-TORCH ON. : E(%d)\n", value);

			global_ktd2692data->mode_status = KTD2692_ENABLE_MOVIE_MODE;
			global_ktd2692data->is_torch_enable = true;
			spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
			ktd2692_write_data(global_ktd2692data->LVP_Voltage|
								KTD2692_ADDR_LVP_SETTING);
#if 0	/* use the internel defualt setting */
			ktd2692_write_data(global_ktd2692data->flash_timeout|
								KTD2692_ADDR_FLASH_TIMEOUT_SETTING);
#endif
			if (value == 100) {
				ktd2692_write_data(global_ktd2692data->factory_current_value|
									KTD2692_ADDR_MOVIE_CURRENT_SETTING);
			} else if (1001 <= value && value <= 1010) {
				brightness_value = value - 1001;
				if (global_ktd2692data->torch_table[brightness_value] != 0) {
					torch_intensity = KTD2692_CAL_MOVIE_CURRENT(KTD2692_TORCH_STEP_LEVEL_CURRENT(global_ktd2692data->torch_table[brightness_value], global_ktd2692data->max_current),
						global_ktd2692data->max_current);
				}
				if (torch_intensity < 0) {
					LED_INFO("KTD2692-force to set as default : %d\n", global_ktd2692data->torch_current_value);
					torch_intensity = global_ktd2692data->torch_current_value;
				}
				ktd2692_write_data(torch_intensity|
									KTD2692_ADDR_MOVIE_CURRENT_SETTING);
			} else {
				ktd2692_write_data(global_ktd2692data->torch_current_value|
									KTD2692_ADDR_MOVIE_CURRENT_SETTING);
			}
			ktd2692_write_data(global_ktd2692data->mode_status|
								KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
			spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);

			gpio_free(global_ktd2692data->flash_control);
			LED_INFO("KTD2692-TORCH ON. : X(%d)\n", value);
		}
	}

	if (!IS_ERR(pinctrl))
		devm_pinctrl_put(pinctrl);

	return count;
}

EXPORT_SYMBOL(ktd2692_led_mode_ctrl);

int32_t ktd2692_led_mode_ctrl(int state, u32 intensity)
{
	int ret = 0;
	unsigned long flags = 0;
	struct pinctrl *pinctrl;

	if (global_ktd2692data == NULL) {
		LED_ERROR("KTD2692(%s) global_ktd2692data is not initialized.\n", __func__);
		return -EFAULT;
	}

	switch(state) {
		case 1:
			/* FlashLight Mode OFF */
			ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
			if (ret) {
				LED_ERROR("Failed to request ktd2692_led_mode_ctrl\n");
			} else {
				LED_INFO("KTD2692-FLASH OFF E(%d)\n", state);
				global_ktd2692data->mode_status = KTD2692_DISABLES_MOVIE_FLASH_MODE;
				spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
				ktd2692_write_data(global_ktd2692data->mode_status|
									KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
				spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);

				ktd2692_setGpio(0);
				gpio_free(global_ktd2692data->flash_control);
				global_ktd2692data->is_torch_enable = false;
				LED_INFO("KTD2692-FLASH OFF X(%d)\n", state);
			}
			break;
		case 2:
			/* FlashLight Mode Flash */
			pinctrl = devm_pinctrl_get_select(global_dev, "host");
			if (IS_ERR(pinctrl))
				pr_err("%s: flash %s pins are not configured\n", __func__, "host");

			ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
			if (ret) {
				LED_ERROR("Failed to request ktd2692_led_mode_ctrl\n");
			} else {
				LED_INFO("KTD2692-FLASH ON E(%d)\n", state);
				global_ktd2692data->mode_status = KTD2692_ENABLE_FLASH_MODE;
				spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
				ktd2692_write_data(global_ktd2692data->LVP_Voltage|
									KTD2692_ADDR_LVP_SETTING);
				if (intensity > 0) {
					ktd2692_write_data((uint8_t)(KTD2692_FLASH_CURRENT(intensity, KTD2692_MAX_CURRENT))|
										KTD2692_ADDR_FLASH_CURRENT_SETTING);
					LED_INFO("[%s] : Intensity(%u) Current-Index (%d)\n", __func__, intensity,
							 KTD2692_FLASH_CURRENT(intensity, KTD2692_MAX_CURRENT));
				} else {
					ktd2692_write_data(global_ktd2692data->flash_current_value|
										KTD2692_ADDR_FLASH_CURRENT_SETTING);
				}

				ktd2692_write_data(global_ktd2692data->mode_status|
									KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
				spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);

				gpio_free(global_ktd2692data->flash_control);
				LED_INFO("KTD2692-FLASH ON X(%d)\n", state);
			}
			break;
		case 3:
			/* FlashLight Mode TORCH */
			pinctrl = devm_pinctrl_get_select(global_dev, "host");
			if (IS_ERR(pinctrl))
				pr_err("%s: flash %s pins are not configured\n", __func__, "host");

			if (global_ktd2692data->is_torch_enable == true) {
				LED_INFO("KTD2692-TORCH is already ON\n");
				return 0;
			}

			ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
			if (ret) {
				LED_ERROR("Failed to request ktd2692_led_mode_ctrl\n");
			} else {
				LED_INFO("KTD2692-TORCH ON E(%d) movie current [%d] \n", state, global_ktd2692data->movie_current_value);
				global_ktd2692data->mode_status = KTD2692_ENABLE_MOVIE_MODE;
				spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
				ktd2692_write_data(global_ktd2692data->LVP_Voltage|
									KTD2692_ADDR_LVP_SETTING);
				if (intensity > 0) {
					ktd2692_write_data((uint8_t)(KTD2692_MOVIE_CURRENT(intensity, KTD2692_MAX_CURRENT)) |
										KTD2692_ADDR_MOVIE_CURRENT_SETTING);
					LED_INFO("[%s] :  Intensity(%u) Current-Index (%d)\n", __func__, intensity,
							 KTD2692_MOVIE_CURRENT(intensity, KTD2692_MAX_CURRENT));
				} else {
					ktd2692_write_data(global_ktd2692data->movie_current_value|
										KTD2692_ADDR_MOVIE_CURRENT_SETTING);
				}

				ktd2692_write_data(global_ktd2692data->mode_status|
									KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
				spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);

				gpio_free(global_ktd2692data->flash_control);
				LED_INFO("KTD2692-TORCH ON X(%d)\n", state);
			}
			break;
		default:
			/* FlashLight Mode OFF */
			ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
			if (ret) {
				LED_ERROR("Failed to request ktd2692_led_mode_ctrl\n");
			} else {
				LED_INFO("KTD2692-FLASH OFF E(%d)\n", state);
				global_ktd2692data->mode_status = KTD2692_DISABLES_MOVIE_FLASH_MODE;
				spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
				ktd2692_write_data(global_ktd2692data->mode_status|
									KTD2692_ADDR_MOVIE_FLASHMODE_CONTROL);
				spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);

				ktd2692_setGpio(0);
				gpio_free(global_ktd2692data->flash_control);
				LED_INFO("KTD2692-FLASH OFF X(%d)\n", state);
			}
			break;
	}

	return ret;
}

ssize_t ktd2692_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", global_ktd2692data->sysfs_input_data);
}

static DEVICE_ATTR(rear_flash, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
	ktd2692_show, ktd2692_store);
static DEVICE_ATTR(rear_torch_flash, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
	ktd2692_show, ktd2692_store);

static int ktd2692_parse_dt(struct device *dev,
                                struct ktd2692_platform_data *pdata)
{
	struct device_node *dnode = dev->of_node;
	u32 buffer = 0;
	int ret = 0;
	u32 torch_table_enable = 0;

	/* Defulat Value */
	pdata->LVP_Voltage = KTD2692_DISABLE_LVP;
	pdata->flash_timeout = KTD2692_TIMER_1049ms;	/* default */
	pdata->min_current_value = KTD2692_MIN_CURRENT_240mA;
	pdata->flash_current_value = KTD2692_FLASH_CURRENT(KTD2692_FLASH_DEFAULT_CURRENT, KTD2692_MAX_CURRENT);
	pdata->movie_current_value = KTD2692_MOVIE_CURRENT(KTD2692_MOVIE_DEFAULT_CURRENT, KTD2692_MAX_CURRENT);
	pdata->factory_current_value = KTD2692_MOVIE_CURRENT(KTD2692_FACTORY_DEFAULT_CURRENT, KTD2692_MAX_CURRENT);
	pdata->torch_current_value = KTD2692_MOVIE_CURRENT(KTD2692_TORCH_DEFAULT_CURRENT, KTD2692_MAX_CURRENT);
	pdata->mode_status = KTD2692_DISABLES_MOVIE_FLASH_MODE;

	/* get gpio */
	pdata->flash_control = of_get_named_gpio(dnode, "flash_control", 0);
	if (!gpio_is_valid(pdata->flash_control)) {
		dev_err(dev, "failed to get flash_control\n");
		return -1;
	}

	/* get max current value */
	if (of_property_read_u32(dnode, "max_current", &buffer) == 0) {
		dev_info(dev, "max_current = <%d>\n",
			buffer);
		pdata->max_current = buffer;
	}

	/* get flash current value */
	if (of_property_read_u32(dnode, "flash_current", &buffer) == 0) {
		dev_info(dev, "flash_current = <%d><%d>\n",
			buffer, KTD2692_FLASH_CURRENT(buffer, pdata->max_current));
		pdata->flash_current_value = KTD2692_FLASH_CURRENT(buffer, pdata->max_current);
	}

	/* get movie current value */
	if (of_property_read_u32(dnode, "movie_current", &buffer) == 0) {
		dev_info(dev, "movie_current = <%d><%d>\n",
			buffer, KTD2692_MOVIE_CURRENT(buffer, pdata->max_current));
		pdata->movie_current_value = KTD2692_MOVIE_CURRENT(buffer, pdata->max_current);
	}

	/* get factory current value */
	if (of_property_read_u32(dnode, "factory_current", &buffer) == 0) {
		dev_info(dev, "factory_current = <%d><%d>\n",
			buffer, KTD2692_MOVIE_CURRENT(buffer, pdata->max_current));
		pdata->factory_current_value = KTD2692_MOVIE_CURRENT(buffer, pdata->max_current);
	}

	/* get torch current value */
	if (of_property_read_u32(dnode, "torch_current", &buffer) == 0) {
		dev_info(dev, "torch_current = <%d><%d>\n",
			buffer, KTD2692_MOVIE_CURRENT(buffer, pdata->max_current));
		pdata->torch_current_value = KTD2692_MOVIE_CURRENT(buffer, pdata->max_current);
	}

	ret = of_property_read_u32(dnode, "torch_table_enable", &torch_table_enable);
	if (ret) {
		pr_info("%s failed to get a torch_table_enable\n", __func__);
	}
	if (torch_table_enable == 1) {
		pdata->torch_table_enable = torch_table_enable;
		ret = of_property_read_u32_array(dnode, "torch_table", pdata->torch_table, TORCH_STEP);
	} else {
		pdata->torch_table_enable = 0;
	}

	return ret;
}

static int ktd2692_probe(struct platform_device *pdev)
{
	struct ktd2692_platform_data *pdata;
	int ret = 0;

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
		ret = ktd2692_parse_dt(&pdev->dev, pdata);
		if (ret < 0) {
			return -EFAULT;
		}
	} else {
	pdata = pdev->dev.platform_data;
		if (pdata == NULL) {
			return -EFAULT;
		}
	}

	global_ktd2692data = pdata;
	global_dev = &pdev->dev;

	LED_INFO("KTD2692_LED Probe\n");

	global_ktd2692data->is_torch_enable = false;

	ktd2692_dev = device_create(camera_class, NULL, 0, NULL, "flash");
	if (IS_ERR(ktd2692_dev)) {
		LED_ERROR("Failed to create device(flash)!\n");
	}

	if (device_create_file(ktd2692_dev, &dev_attr_rear_flash) < 0) {
		LED_ERROR("failed to create device file, %s\n",
				dev_attr_rear_flash.attr.name);
	}
	if (device_create_file(ktd2692_dev, &dev_attr_rear_torch_flash) < 0) {
		LED_ERROR("failed to create device file, %s\n",
				dev_attr_rear_torch_flash.attr.name);
	}

	spin_lock_init(&pdata->int_lock);

	return 0;
}
static int ktd2692_remove(struct platform_device *pdev)
{

	device_remove_file(ktd2692_dev, &dev_attr_rear_flash);
	device_remove_file(ktd2692_dev, &dev_attr_rear_torch_flash);
	device_destroy(camera_class, 0);
	class_destroy(camera_class);

	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id ktd2692_dt_ids[] = {
	{ .compatible = "ktd2692",},
	{},
};
/*MODULE_DEVICE_TABLE(of, ktd2692_dt_ids);*/
#endif

static struct platform_driver ktd2692_driver = {
	.driver = {
		   .name = ktd2692_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = ktd2692_dt_ids,
#endif
		   },
	.probe = ktd2692_probe,
	.remove = ktd2692_remove,
};

static int __init ktd2692_init(void)
{
	return platform_driver_register(&ktd2692_driver);
}

static void __exit ktd2692_exit(void)
{
	platform_driver_unregister(&ktd2692_driver);
}

module_init(ktd2692_init);
module_exit(ktd2692_exit);

MODULE_AUTHOR("sunggeun yim <sunggeun.yim@samsung.com.com>");
MODULE_DESCRIPTION("KTD2692 driver");
MODULE_LICENSE("GPL");


