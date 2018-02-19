/*
 * Realtek Semiconductor Corp.
 *
 * bsp/thermal.c:
 *     bsp thermal sensor initialization code
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


/* thermal sensor */

static struct resource bsp_thermal_resource[] = {
	[0] = {
		.start	= BSP_THERMAL_PADDR,
		.end	= BSP_THERMAL_PADDR + BSP_THERMAL_PSIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= BSP_IRQ_OTHERS,
		.end   = BSP_IRQ_OTHERS,
		.flags	= IORESOURCE_IRQ,
	}
};

static u32 thermal_k[] = {
	26687, 27078, 27581, 27925, 28465, 28906, 29350, 29778,
	30184, 30646, 31040, 31493, 31934, 32353, 32790, 33212,
	33675, 34134
};

struct platform_device bsp_thermal_device = {
	.name          = "rts-thermal",
	.id            = 0,
	.num_resources = ARRAY_SIZE(bsp_thermal_resource),
	.resource      = bsp_thermal_resource,
	.dev	= {
		.platform_data	= thermal_k,
	}
};

static struct platform_device *bsp_thermal_devs[] __initdata = {
	&bsp_thermal_device,
};

static int __init thermal_device_init(void)
{
	int ret;

	/* core init */
	printk("INFO: initializing thermal sensor ...\n");

	ret = platform_add_devices(bsp_thermal_devs,
		ARRAY_SIZE(bsp_thermal_devs));
	if (ret < 0) {
		printk("ERROR: unable to add thermal sensor\n");
		return ret;
	}

	return 0;
}
arch_initcall(thermal_device_init);

