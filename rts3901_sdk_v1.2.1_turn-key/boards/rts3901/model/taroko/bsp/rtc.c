/*
 * Realtek Semiconductor Corp.
 *
 * bsp/rtc.c:
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
#include "xb2.h"

static struct resource bsp_rtc_resource[2] = {
	[0] = {
		.start	= BSP_RTC_PADDR,
		.end	= BSP_RTC_PADDR + BSP_RTC_PSIZE - 1,
		.flags	= IORESOURCE_MEM,
	}
};

struct platform_device bsp_rtc_device = {
	.name          = "rts-rtc",
	.id            = 0,
	.num_resources = ARRAY_SIZE(bsp_rtc_resource),
	.resource      = bsp_rtc_resource,
};

static struct platform_device *bsp_rtc_devs[] __initdata = {
	&bsp_rtc_device,
};

static int __init rtc_device_init(void)
{
	int ret;

	/* core init */
	printk("INFO: initializing RTC ...\n");

	bsp_rtc_resource[1].start = rts_xb2_to_irq(RTC_ALARM3_INT_IRQ);
	bsp_rtc_resource[1].end = rts_xb2_to_irq(RTC_ALARM3_INT_IRQ);
	bsp_rtc_resource[1].flags = IORESOURCE_IRQ;

	ret = platform_add_devices(bsp_rtc_devs, ARRAY_SIZE(bsp_rtc_devs));
	if (ret < 0) {
		printk("ERROR: unable to add RTC\n");
		return ret;
	}

	return 0;
}
arch_initcall(rtc_device_init);

