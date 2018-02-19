/*
 * Realtek Semiconductor Corp.
 *
 * bsp/i2c.c:
 *     bsp I2C initialization code
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


/* I2C Master Controller */

static struct resource bsp_i2c_resource[] = {
	[0] = {
		.start	= BSP_I2C0_PADDR,
		.end	= BSP_I2C0_PADDR + BSP_I2C0_PSIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= BSP_IRQ_OTHERS,
		.end   = BSP_IRQ_OTHERS,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device bsp_i2c_device = {
	.name          = "i2c_designware",
	.id            = 0,
	.num_resources = ARRAY_SIZE(bsp_i2c_resource),
	.resource      = bsp_i2c_resource,
};

static struct platform_device *bsp_i2c_devs[] __initdata = {
	&bsp_i2c_device,
};

static int __init i2c_designware_device_init(void)
{
	int ret;

	/* core init */
	printk("INFO: initializing I2C master ...\n");

	ret = platform_add_devices(bsp_i2c_devs, ARRAY_SIZE(bsp_i2c_devs));
	if (ret < 0) {
		printk("ERROR: unable to add I2C master\n");
		return ret;
	}

	return 0;
}
arch_initcall(i2c_designware_device_init);

