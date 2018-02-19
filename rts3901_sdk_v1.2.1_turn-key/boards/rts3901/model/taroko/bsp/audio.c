/*
 * Realtek Semiconductor Corp.
 *
 * bsp/snd.c:
 *     bsp SND initialization code
 *
 * Copyright (C) 2014      Wind HAN (wind_han@realsil.com.cn)
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
#include <linux/i2c.h>
#include <linux/dma-mapping.h>
#include <linux/sizes.h>

#include "bspchip.h"

static struct resource bsp_i2s_resource[] = {
	[0] = {
		.start = BSP_DAI_PADDR,
		.end = BSP_DAI_PADDR + BSP_DAI_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BSP_DAIP_PADDR,
		.end = BSP_DAIP_PADDR + BSP_DAIP_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct resource bsp_dma_resource[] = {
	[0] = {
		.start = BSP_AUDIO_PADDR,
		.end = BSP_AUDIO_PADDR + BSP_AUDIO_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BSP_IRQ_I2S,
		.end = BSP_IRQ_I2S,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource bsp_codec_resource[] = {
	[0] = {
		.start = BSP_CODEC_PADDR,
		.end = BSP_CODEC_PADDR + BSP_CODEC_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

static u64 bsp_snd_dmamask = 0xFFFFFFFFUL;

struct platform_device bsp_i2s_device = {
#ifdef CONFIG_SOC_FPGA_CODE
	.name = "rts3901-fpga-adai",
#else
	.name = "rts3901-adai",
#endif
	.id = -1,
	.num_resources = ARRAY_SIZE(bsp_i2s_resource),
	.resource = bsp_i2s_resource,
	.dev = {
		.dma_mask = &bsp_snd_dmamask,
		.coherent_dma_mask = 0xffffffffUL,
	}
};

struct platform_device bsp_dma_device = {
#ifdef CONFIG_SOC_FPGA_CODE
	.name = "rts3901-fpga-adma",
#else
	.name = "rts3901-adma",
#endif
	.id = -1,
	.num_resources = ARRAY_SIZE(bsp_dma_resource),
	.resource = bsp_dma_resource,
	.dev = {
		.init_name = "audio-platform",
		.dma_mask = &bsp_snd_dmamask,
		.coherent_dma_mask = 0xffffffffUL,
	}
};

struct platform_device bsp_codec_device = {
	.name = "rlx-codec",
	.id = -1,
	.num_resources = ARRAY_SIZE(bsp_codec_resource),
	.resource = bsp_codec_resource,
	.dev = {
		.dma_mask = &bsp_snd_dmamask,
		.coherent_dma_mask = 0xffffffffUL,
	}
};

static struct platform_device *bsp_snd_devs[] __initdata = {
	&bsp_dma_device,
	&bsp_i2s_device,
	&bsp_codec_device,
};

static struct i2c_board_info i2c_device[] = {
	{ I2C_BOARD_INFO("rt5651", 0x1A), },
	{ I2C_BOARD_INFO("rt5658", 0x1B), }
};

static int __init bsp_snd_init(void)
{
	int ret;
#ifdef CONFIG_SOC_RESV_CONTIG_MEM
	int dma;
	extern phys_addr_t resvd_contig_mem_base;
	phys_addr_t snd_resvd_mem_base;
	size_t snd_resvd_mem_size;
#endif

	/* core init */
	printk("INFO: initializing snd device ...\n");

#ifdef CONFIG_SOC_RESV_CONTIG_MEM
	snd_resvd_mem_base = resvd_contig_mem_base + AUDIO_RESVD_MEM_OFFSET;
	snd_resvd_mem_size = AUDIO_RESVD_MEM_SIZE;
	printk(KERN_INFO"snd resvd mem size : %d\n", snd_resvd_mem_size);
	dma = dma_declare_coherent_memory(&bsp_dma_device.dev,
			snd_resvd_mem_base, snd_resvd_mem_base, snd_resvd_mem_size,
			DMA_MEMORY_MAP | DMA_MEMORY_EXCLUSIVE);
	if (!(dma & DMA_MEMORY_MAP))
		return -ENOMEM;
#endif

	ret = platform_add_devices(bsp_snd_devs, ARRAY_SIZE(bsp_snd_devs));
	if (ret < 0) {
		printk("ERROR: unable to add snd device");
		return ret;
	}

	i2c_register_board_info(0, i2c_device, ARRAY_SIZE(i2c_device));

	return 0;
}
arch_initcall(bsp_snd_init);
