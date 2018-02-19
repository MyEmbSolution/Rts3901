/*
 * Realtek Semiconductor Corp.
 *
 * bsp/rtl8168g.c:
 *     bsp Ethernet(10/100) initialization code
 *
 * Copyright (C) 2014	Hurray Niu (hurray_niu@realsil.com.cn)
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
#include <linux/r8168_platform.h>

#include "bspchip.h"

static struct resource bsp_ethernet_resource[] = {
	[0] = {
		.start = BSP_ETHERNET_PADDR,
		.end = BSP_ETHERNET_PADDR + BSP_ETHERNET_PSIZE - 1,
		.flags = IORESOURCE_MEM, },
	[1] = {
		.start = BSP_IRQ_ETHERNET,
		.end = BSP_IRQ_ETHERNET,
		.flags = IORESOURCE_IRQ, }
};

static u64 bsp_ethernet_dmamask = 0xFFFFFFFFUL;

static struct rtl8168_platform_data rtl8168_pdata = {
	.led_config[0] = {
		.led_pin = -1,
	},
	.led_config[1] = {
		.led_pin = 1,
		.link_mode = 100,
		.act_full = 1,
		.act_high_active = 0,
	},
	.led_config[2] = {
		.led_pin = -1,
	}
};

struct platform_device bsp_ethernet_device = {
	.name = "rtl8168",
	.id = -1,
	.num_resources = ARRAY_SIZE(bsp_ethernet_resource),
	.resource = bsp_ethernet_resource,
	.dev = {
		.dma_mask = &bsp_ethernet_dmamask,
		.coherent_dma_mask = 0xffffffffUL,
		.platform_data = &rtl8168_pdata,
	}
};

static struct platform_device *bsp_ethernet_devs[] __initdata = {
	&bsp_ethernet_device,
};

static int __init bsp_ethernet_init(void)
{
	u32 retval;
	printk("INFO: initializing ethernet devices ...\n");

	retval = platform_add_devices(bsp_ethernet_devs, ARRAY_SIZE(bsp_ethernet_devs));
	if (retval < 0) {
		printk("ERROR: unable to add devices\n");
		return retval;
	}

	return retval;
}
arch_initcall(bsp_ethernet_init);
