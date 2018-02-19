/*
 * Realtek Semiconductor Corp.
 *
 * bsp/spi.c:
 *     bsp spi initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 *               2014      Darcy Lu (darcy_lu@realsil.com.cn)
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
#include <linux/mtd/partitions.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>

#include "bspchip.h"

static struct flash_platform_data rts_spiflash_data = {
	.name		= "m25p80",
	.type		= "mx25l12845e",
};

static struct spi_board_info rts_spi_board_info[] __initdata = {
	{
		.modalias		= "m25p80",
		.platform_data		= &rts_spiflash_data,
		.mode			= SPI_MODE_3,
		.max_speed_hz		= 30000000,
		.bus_num		= 0,
		.chip_select		= 0,
		.irq				= BSP_IRQ_SPI,
	},
};

/* spi Host Controller */

static struct resource bsp_spic_resource[] = {
	[0] = {
		.start = BSP_SPIC_PADDR,
		.end   = BSP_SPIC_PADDR + BSP_SPIC_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BSP_IRQ_SPI,
		.end   = BSP_IRQ_SPI,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 bsp_spi_dmamask = 0xFFFFFFFFUL;

struct platform_device bsp_spic_device = {
	.name          = "spic-platform",
	.id            = 0,
	.num_resources = ARRAY_SIZE(bsp_spic_resource),
	.resource      = bsp_spic_resource,
	.dev           = {
		.dma_mask = &bsp_spi_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};

static int __init bsp_spi_init(void)
{
	int ret;

	/* core init */
	printk("INFO: initializing spi host ...%x\n", rts_spi_board_info[0].bus_num);

	ret = platform_device_register(&bsp_spic_device);

	spi_register_board_info(rts_spi_board_info,
				ARRAY_SIZE(rts_spi_board_info));

	printk("spi platform id is %x\n", bsp_spic_device.id);

	if (ret < 0) {
		printk("ERROR: unable to add spi host\n");
		return ret;
	}

	return 0;
}
arch_initcall(bsp_spi_init);

