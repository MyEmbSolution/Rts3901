/*
 * Realtek Semiconductor Corp.
 *
 * bsp/force-reset.c:
 *     force reset registers initialization code
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

#include <linux/rts_sysctl.h>

#include "bspchip.h"

struct rts_force_reset_regs {
	u32 force_reg_reset;
#define FORCE_BUS_VIDEO_RESET		0x100
#define FORCE_BUS_SD_RESET		0x80
#define FORCE_BUS_I2S_RESET		0x40
#define FORCE_BUS_U2DEV_RESET		0x20
#define FORCE_BUS_U2HOST_RESET		0x10

	u32 force_reg_reset_fwc;
#define FORCE_RTC32K_RESET		0x08
#define FORCE_U2DEV_UTMI_RESET		0x04
#define FORCE_U2HOST_UTMI_RESET		0x02
#define FORCE_MCU_CLK_RESET		0x01

	u32 force_reg_async_reset;
#define FORCE_H264_ASYNC_RESET		0x200
#define FORCE_I2C_CLK_ASYNC_RESET	0x100
#define FORCE_UART_CLK_ASYNC_RESET	0x080
#define FORCE_ETHERNET_CLK_ASYNC_RESET	0x040
#define FORCE_SD_CLK_ASYNC_RESET	0x020
#define FORCE_CIPHER_CLK_ASYNC_RESET	0x010
#define FORCE_I2S_CLK_ASYNC_RESET	0x008
#define FORCE_ISP_CLK_ASYNC_RESET	0x004
#define FORCE_JPG_CLK_ASYNC_RESET	0x002
#define FORCE_MIPI_CLK_ASYNC_RESET	0x001
};

struct rts_force_reset {
	struct rts_force_reset_regs __iomem	*regs;
	struct mutex				mutex;
};

static struct rts_force_reset force_reset;

#define RTS_FRR_SET(addr, mask)				\
do {							\
	u32 val;					\
	val = readl((addr));				\
	val |= (mask);					\
	writel(val, (addr));				\
} while (0)

#define RTS_FRR_CLR(addr, mask)				\
do {							\
	u32 val;					\
	val = readl((addr));				\
	val &= ~(mask);					\
	writel(val, (addr));				\
} while (0)

#define RTS_FORCE_RESET_AUTO(addr, mask)	RTS_FRR_SET(addr, mask)

#define RTS_FORCE_RESET(addr, mask)			\
do {							\
	RTS_FRR_SET(addr, mask);			\
	RTS_FRR_CLR(addr, mask);			\
} while (0)

void rts_sys_force_reset(int model)
{
	struct rts_force_reset_regs *regs = force_reset.regs;

	mutex_lock(&force_reset.mutex);

	switch (model) {
	case FORCE_RESET_VIDEO:
		RTS_FORCE_RESET_AUTO(&regs->force_reg_reset, FORCE_BUS_VIDEO_RESET);
		break;

	case FORCE_RESET_H264:
		RTS_FORCE_RESET(&regs->force_reg_async_reset, FORCE_H264_ASYNC_RESET);
		break;

	case FORCE_RESET_JPG:
		RTS_FORCE_RESET(&regs->force_reg_async_reset, FORCE_JPG_CLK_ASYNC_RESET);
		break;

	case FORCE_RESET_MIPI:
		RTS_FORCE_RESET(&regs->force_reg_async_reset, FORCE_MIPI_CLK_ASYNC_RESET);
		break;

	case FORCE_RESET_SD:
		RTS_FORCE_RESET_AUTO(&regs->force_reg_reset, FORCE_BUS_SD_RESET);
		RTS_FORCE_RESET(&regs->force_reg_async_reset, FORCE_SD_CLK_ASYNC_RESET);
		break;

	case FORCE_RESET_CIPHER:
		RTS_FORCE_RESET(&regs->force_reg_async_reset, FORCE_CIPHER_CLK_ASYNC_RESET);
		break;

	case FORCE_RESET_I2S:
		RTS_FORCE_RESET_AUTO(&regs->force_reg_reset, FORCE_BUS_I2S_RESET);
		RTS_FORCE_RESET(&regs->force_reg_async_reset, FORCE_I2S_CLK_ASYNC_RESET);
		break;

	case FORCE_RESET_I2C:
		RTS_FORCE_RESET(&regs->force_reg_async_reset, FORCE_I2C_CLK_ASYNC_RESET);
		break;

	case FORCE_RESET_U2DEV:
		RTS_FORCE_RESET_AUTO(&regs->force_reg_reset, FORCE_BUS_U2DEV_RESET);
		RTS_FORCE_RESET(&regs->force_reg_reset_fwc, FORCE_U2DEV_UTMI_RESET);
		break;

	case FORCE_RESET_U2HOST:
		RTS_FORCE_RESET_AUTO(&regs->force_reg_reset, FORCE_BUS_U2HOST_RESET);
		RTS_FORCE_RESET(&regs->force_reg_reset_fwc, FORCE_U2HOST_UTMI_RESET);
		break;

	case FORCE_RESET_MCU:
		RTS_FORCE_RESET(&regs->force_reg_reset_fwc, FORCE_MCU_CLK_RESET);
		break;

	case FORCE_RESET_ISP:
		RTS_FORCE_RESET(&regs->force_reg_async_reset, FORCE_ISP_CLK_ASYNC_RESET);
		break;

	case FORCE_RESET_MCU_PREPARE:
		RTS_FRR_SET(&regs->force_reg_reset_fwc, FORCE_MCU_CLK_RESET);
		break;

	case FORCE_RESET_MCU_DONE:
		RTS_FRR_CLR(&regs->force_reg_reset_fwc, FORCE_MCU_CLK_RESET);
		break;

	case FORCE_RESET_UART:
		RTS_FORCE_RESET(&regs->force_reg_async_reset, FORCE_UART_CLK_ASYNC_RESET);
		break;

	case FORCE_RESET_ETHERNET:
		RTS_FRR_SET(&regs->force_reg_async_reset, FORCE_ETHERNET_CLK_ASYNC_RESET);
		break;

	case FORCE_RESET_ETHERNET_CLR:
		RTS_FRR_CLR(&regs->force_reg_async_reset, FORCE_ETHERNET_CLK_ASYNC_RESET);
		break;

	default:
		printk("ERROR: invalid reset model %d\n", model);
		break;
	}

	xb2flush();

	mutex_unlock(&force_reset.mutex);
}
EXPORT_SYMBOL_GPL(rts_sys_force_reset);

static int __init bsp_force_reset_init(void)
{
	mutex_init(&force_reset.mutex);

	force_reset.regs = ioremap(BSP_FORCE_RESET_PADDR, BSP_FORCE_RESET_PSIZE);
	if (!force_reset.regs) {
		printk("ERROR: unable to ioremap 0x%lx\n", BSP_FORCE_RESET_PADDR);
		return -ENOMEM;
	}
	
	return 0;
}
arch_initcall(bsp_force_reset_init);
