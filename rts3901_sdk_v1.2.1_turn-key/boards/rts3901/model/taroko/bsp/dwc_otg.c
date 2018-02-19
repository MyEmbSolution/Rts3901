/*
 * Realtek Semiconductor Corp.
 *
 * bsp/dwc_otg.c:
 * bsp dwc_otg initialization code
 *
 * Copyright (C) 2014 Lei Wang (lei_wang@realsil.com.cn)
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

static struct resource bsp_dwc_otg_resource[] = {
	[0] = {
			.start	= BSP_DWC_OTG_BASE,
			.end	= BSP_DWC_OTG_BASE + BSP_DWC_OTG_SIZE - 1,
			.flags	= IORESOURCE_MEM,
	},
	[1] = {
			.start	= BSP_IRQ_USBDEV,
			.end	= BSP_IRQ_USBDEV,
			.flags	= IORESOURCE_IRQ,
	},

};

static struct platform_device bsp_dwc_otg_device = {
	.name		= "dwc_otg",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bsp_dwc_otg_resource),
	.resource	= bsp_dwc_otg_resource,
};

static struct platform_device *bsp_dwc_otg_devs[] __initdata = {
	&bsp_dwc_otg_device,
};

static int __init bsp_dwc_otg_init(void)
{
	int ret;
	printk("INFO: initializing dwc_otg devices ...\n");

	ret = platform_add_devices(bsp_dwc_otg_devs,
		ARRAY_SIZE(bsp_dwc_otg_devs));
	if (ret < 0) {
		printk("ERROR: unable to add dwc_otg device\n");
		return ret;
	}

	return 0;
}
arch_initcall(bsp_dwc_otg_init);
