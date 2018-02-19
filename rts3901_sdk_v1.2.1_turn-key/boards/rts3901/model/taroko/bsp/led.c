/*
 * Realtek Semiconductor Corp.
 *
 * bsp/led.c:
 *     bsp led initialization code
 *
 * Copyright (C)  2014	Peter Sun (peter_sun@realsil.com.cn)
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/leds.h>

#include "bspchip.h"

#define LEDS_WIFI_NUM	CONFIG_LEDS_WIFI_NUM

static struct gpio_led gpio_leds[] = {
	{
		.name			= "wifi_led",
		.gpio			= LEDS_WIFI_NUM,
		.active_low		= 1,
		.default_trigger	= "none",
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
	},
};

static struct gpio_led_platform_data gpio_led_info = {
	.leds		= gpio_leds,
	.num_leds	= ARRAY_SIZE(gpio_leds),
};

static struct platform_device leds = {
	.name	= "leds-gpio",
	.id	= -1,
	.dev	= {
		.platform_data	= &gpio_led_info,
	}
};

static struct platform_device *bsp_leds_devs[] __initdata = {
	&leds,
};

static int __init led_device_init(void)
{
	int ret;

	printk("INFO: initializing led device - %d...\n", LEDS_WIFI_NUM);

	ret = platform_add_devices(bsp_leds_devs, ARRAY_SIZE(bsp_leds_devs));
	if (ret < 0) {
		printk("ERROR: unable to add led device\n");
		return ret;
	}

	return 0;
}
arch_initcall(led_device_init);

