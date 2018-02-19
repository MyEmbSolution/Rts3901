/*
 * Realtek Semiconductor Corp.
 *
 * bsp/pinctrl.c:
 *     bsp pinctrl initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 *               2014      Peter Sun (peter_sun@realsil.com.cn)
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
#include <linux/pinctrl/machine.h>

#include "bspchip.h"

/* pinctrl Master Controller */

static const struct pinctrl_map rts_map[] = {
	PIN_MAP_DUMMY_STATE("pwm_platform", PINCTRL_STATE_SLEEP),
	PIN_MAP_MUX_GROUP_DEFAULT("pwm_platform", "pinctrl_platform",
				  "pwm0grp", "pwmfunc"),
	PIN_MAP_MUX_GROUP_DEFAULT("pwm_platform", "pinctrl_platform",
				  "pwm1grp", "pwmfunc"),
	PIN_MAP_MUX_GROUP_DEFAULT("pwm_platform", "pinctrl_platform",
				  "pwm2grp", "pwmfunc"),
	PIN_MAP_MUX_GROUP_DEFAULT("pwm_platform", "pinctrl_platform",
				  "pwm3grp", "pwmfunc"),

	PIN_MAP_DUMMY_STATE("serial8250", PINCTRL_STATE_SLEEP),
	PIN_MAP_MUX_GROUP_DEFAULT("serial8250", "pinctrl_platform",
				  "uart1grp", "uartfunc"),

	PIN_MAP_DUMMY_STATE("audio-platform", PINCTRL_STATE_SLEEP),
	PIN_MAP_MUX_GROUP_DEFAULT("audio-platform", "pinctrl_platform",
				  "dmicgrp", "dmicfunc"),

	PIN_MAP_DUMMY_STATE("usbphy-platform", PINCTRL_STATE_SLEEP),
	PIN_MAP_MUX_GROUP_DEFAULT("usbphy-platform", "pinctrl_platform",
				  "usbdgrp", "usbdfunc"),
	PIN_MAP_MUX_GROUP_DEFAULT("rtl8168", "pinctrl_platform",
				  "etnled1grp", "etnledfunc"),
};

static struct resource bsp_pinctrl_resource[] = {
	[0] = {
		.start	= BSP_GPIO_PADDR,
		.end	= BSP_GPIO_PADDR + BSP_GPIO_PSIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= BSP_IRQ_OTHERS,
		.end	= BSP_IRQ_OTHERS,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device bsp_pinctrl_device = {
	.name          = "pinctrl_platform",
	.id            = -1,
	.num_resources = ARRAY_SIZE(bsp_pinctrl_resource),
	.resource      = bsp_pinctrl_resource,
};

static struct platform_device *pinctrl_devs[] __initdata = {
	&bsp_pinctrl_device,
};

static int __init pinctrl_device_init(void)
{
	int ret;

	/* core init */
	printk("INFO: initializing pinctrl device ...\n");

	ret = platform_add_devices(pinctrl_devs, ARRAY_SIZE(pinctrl_devs));
	if (ret < 0) {
		printk("ERROR: unable to add pinctrl device\n");
		return ret;
	}

	pinctrl_register_mappings(rts_map, ARRAY_SIZE(rts_map));

	return 0;
}
arch_initcall(pinctrl_device_init);

