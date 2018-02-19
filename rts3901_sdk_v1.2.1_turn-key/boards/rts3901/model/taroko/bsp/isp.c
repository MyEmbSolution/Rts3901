/*
 * Realtek Semiconductor Corp.
 *
 * bsp/isp.c:
 *     bsp ISP initialization code
 *
 * Copyright (C) 2014      Wei WANG (wei_wang@realsil.com.cn)
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
#include <linux/dma-mapping.h>
#include <linux/sizes.h>
#include <linux/platform_data/camera-rtsoc.h>

#include "bspchip.h"

static struct resource bsp_isp_resource[] = {
	[0] = {
		.start = BSP_ISP_PADDR,
		.end   = BSP_ISP_PADDR + BSP_ISP_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BSP_IRQ_MCU8051,
		.end   = BSP_IRQ_MCU8051,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 bsp_isp_dmamask = 0xFFFFFFFFUL;

static struct rtscam_soc_pdata rts_soc_pdata = {
	.capibility	= RTSCAM_SOC_CAP_FLAG_TDNR |
		RTSCAM_SOC_CAP_FLAG_LDC |
		RTSCAM_SOC_CAP_FLAG_DEHAZE,
};

static struct platform_device rts_soc_camera = {
#ifdef CONFIG_SOC_FPGA_CODE
	.name		= "rts3901-fpga-isp",
#else
	.name		= "rts3901-isp",
#endif
	.id		= -1,
	.num_resources = ARRAY_SIZE(bsp_isp_resource),
	.resource	= bsp_isp_resource,
	.dev		= {
		.init_name = "rts_soc_camera",
		.dma_mask = &bsp_isp_dmamask,
		.coherent_dma_mask = 0xffffffffUL,
		.platform_data = &rts_soc_pdata,
	}
};

static struct resource bsp_h264_resource[] = {
	[0] = {
		.start = BSP_H264_PADDR,
		.end   = BSP_H264_PADDR + 1200 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BSP_IRQ_H264,
		.end   = BSP_IRQ_H264,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device rts_h264_encoder = {
#ifdef CONFIG_SOC_FPGA_CODE
	.name		= "rts3901-fpga-h264",
#else
	.name		= "rts3901-h264",
#endif
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bsp_h264_resource),
	.resource	= bsp_h264_resource,
	.dev		= {
		.init_name = "rts_h264_hx280enc",
	}
};

static struct resource bsp_jpg_resource[] = {
	[0] = {
		.start = BSP_ISP_JPEG_PADDR,
		.end   = BSP_ISP_JPEG_PADDR + BSP_ISP_JPEG_PSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BSP_IRQ_MCU8051,
		.end   = BSP_IRQ_MCU8051,
		.flags = IORESOURCE_IRQ,
	}
};

static struct platform_device rts_jpg_encoder = {
#ifdef CONFIG_SOC_FPGA_CODE
	.name		= "rts3901-fpga-mjpeg",
#else
	.name		= "rts3901-mjpeg",
#endif
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bsp_jpg_resource),
	.resource	= bsp_jpg_resource,
	.dev		= {
		.init_name = "rts_jpgenc",
	}
};

static int __init rts_soc_init_camera(void)
{
	int ret;
#ifdef CONFIG_SOC_RESV_CONTIG_MEM
	extern phys_addr_t resvd_contig_mem_base;
	phys_addr_t isp_resvd_mem_base;
	size_t isp_resvd_mem_size;
#endif

	/* core init */
	printk("INFO: initializing ISP device ...\n");

#ifdef CONFIG_SOC_RESV_CONTIG_MEM
	isp_resvd_mem_base = resvd_contig_mem_base + ISP_RESVD_MEM_OFFSET;
	isp_resvd_mem_size = (CONFIG_SOC_RESVD_MEM_SIZE * SZ_1M) - ISP_RESVD_MEM_OFFSET;
	printk(KERN_INFO"isp resvd mem addr : 0x%08x, size : 0x%x\n", isp_resvd_mem_base, isp_resvd_mem_size);
	rts_soc_pdata.resvd_mem_base = isp_resvd_mem_base;
	rts_soc_pdata.resvd_mem_size = isp_resvd_mem_size;
#endif

	ret = platform_device_register(&rts_soc_camera);
	if (ret < 0) {
		printk("ERROR: unable to add ISP device\n");
		return ret;
	}

	return 0;
}

static int __init rts_camera_init(void)
{
	rts_soc_init_camera();
	printk(KERN_INFO "ISP camera platform devices added\n");
	platform_device_register(&rts_h264_encoder);
	platform_device_register(&rts_jpg_encoder);
	return 0;
}

arch_initcall(rts_camera_init);
