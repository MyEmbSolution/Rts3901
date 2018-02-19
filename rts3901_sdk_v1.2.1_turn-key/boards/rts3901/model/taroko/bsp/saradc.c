/*
 * Realtek Semiconductor Corp.
 *
 * bsp/saradc.c:
 *     bsp adc initialization code
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

#include "bspchip.h"

static struct resource bsp_adc_resource[] = {
	[0] = {
		.start	= BSP_ADC_PADDR,
		.end	= BSP_ADC_PADDR + BSP_ADC_PSIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

struct platform_device bsp_adc_device = {
	.name          = "rts_saradc",
	.id            = 0,
	.num_resources = ARRAY_SIZE(bsp_adc_resource),
	.resource      = bsp_adc_resource,
};

static struct platform_device *bsp_adc_devs[] __initdata = {
	&bsp_adc_device,
};

static int __init adc_device_init(void)
{
	int ret;

	/* core init */
	printk("INFO: initializing adc ...\n");

	ret = platform_add_devices(bsp_adc_devs, ARRAY_SIZE(bsp_adc_devs));
	if (ret < 0) {
		printk("ERROR: unable to add adc\n");
		return ret;
	}

	return 0;
}
arch_initcall(adc_device_init);

