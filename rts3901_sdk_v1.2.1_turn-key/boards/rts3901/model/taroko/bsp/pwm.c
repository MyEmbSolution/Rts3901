/*
 * Realtek Semiconductor Corp.
 *
 * bsp/sd.c:
 *     bsp SD controller initialization code
 *
 * Copyright (C) 2014      Wei WANG (wei_wang@realsil.com.cn)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/sizes.h>

#include "bspchip.h"

static struct resource bsp_pwm_resource[] = {
	[0] = {
		.start = BSP_PWM_PADDR,
		.end   = BSP_PWM_PADDR + BSP_PWM_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device bsp_pwm_device = {
	.name          = "pwm_platform",
	.id            = -1,
	.num_resources = ARRAY_SIZE(bsp_pwm_resource),
	.resource      = bsp_pwm_resource,
};

static struct platform_device *bsp_pwm_devs[] __initdata = {
	&bsp_pwm_device,
};

static int __init bsp_pwm_init(void)
{
	int ret;

	/* core init */
	printk("INFO: initializing PWM controller ...\n");

	ret = platform_add_devices(bsp_pwm_devs, ARRAY_SIZE(bsp_pwm_devs));
	if (ret < 0) {
		printk("ERROR: unable to add PWM controller\n");
		return ret;
	}

	return 0;
}
arch_initcall(bsp_pwm_init);
MODULE_LICENSE("GPL");
