/*
 * Realtek Semiconductor Corp.
 *
 * bsp/usb.c:
 *     bsp USB initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 *               2014      Wei WANG (wei_wang@realsil.com.cn)
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


/* USB Host Controller */

static struct resource bsp_ehci_resource[] = {
	[0] = {
		.start = BSP_EHCI_PADDR,
		.end   = BSP_EHCI_PADDR + BSP_EHCI_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BSP_IRQ_USBHOST,
		.end   = BSP_IRQ_USBHOST,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 bsp_usb_dmamask = 0xFFFFFFFFUL;

struct platform_device bsp_ehci_device = {
	.name          = "ehci-platform",
	.id            = -1,
	.num_resources = ARRAY_SIZE(bsp_ehci_resource),
	.resource      = bsp_ehci_resource,
	.dev           = {
		.dma_mask = &bsp_usb_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};

static struct resource bsp_ohci_resource[] = {
	[0] = {
		.start = BSP_OHCI_PADDR,
		.end   = BSP_OHCI_PADDR + BSP_OHCI_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BSP_IRQ_USBHOST,
		.end   = BSP_IRQ_USBHOST,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device bsp_ohci_device = {
	.name          = "ohci-platform",
	.id            = -1,
	.num_resources = ARRAY_SIZE(bsp_ohci_resource),
	.resource      = bsp_ohci_resource,
	.dev           = {
		.dma_mask = &bsp_usb_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};

static struct platform_device *bsp_usb_devs[] __initdata = {
	&bsp_ehci_device,
	&bsp_ohci_device,
};

static int __init bsp_usb_init(void)
{
	int ret;

	/* core init */
	printk("INFO: initializing USB host ...\n");

	ret = platform_add_devices(bsp_usb_devs, ARRAY_SIZE(bsp_usb_devs));
	if (ret < 0) {
		printk("ERROR: unable to add usb host\n");
		return ret;
	}

	return 0;
}
arch_initcall(bsp_usb_init);
