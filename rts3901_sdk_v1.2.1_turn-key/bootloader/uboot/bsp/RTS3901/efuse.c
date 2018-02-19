/*
 * Command for accessing Ethernet efuse.
 *
 * Copyright (C) Realsil Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include "bspchip.h"

#define EFUSE_MAX_ADDR		24

#define EFUSE_TMRF_1200OHM	0x1
#define EFUSE_READ_DONE		0x1

#define DW_BIT_SET0(__datum, __bfws)                          \
	((__datum) = ((uint32_t) (__datum) & ~DW_BIT_MASK(__bfws)) &    \
	~(DW_BIT_MASK(__bfws)))

unsigned char efuse_read_byte(unsigned int addr)
{
	unsigned int reg;
	unsigned char data;

	if (addr >= EFUSE_MAX_ADDR)
		return -1;

	/*power on*/
	reg = readl(SYS_EFUSE_CFG);
	DW_BIT_SET(reg, EFUSE_PWR_BFW);
	writel(reg, SYS_EFUSE_CFG);

	/*por and porgram*/
	reg = readl(SYS_EFUSE_CFG);
	DW_BIT_SET(reg, EFUSE_POR_10_BFW);
	DW_BIT_SET0(reg, EFUSE_PROGRAM_BFW);
	writel(reg, SYS_EFUSE_CFG);

	/*delay 490+ns*/
	udelay(1);

	/*ENB, TMRF and A*/
	reg = readl(SYS_EFUSE_CFG);
	DW_BIT_SET0(reg, EFUSE_ENB_BFW);
	DW_BITS_SET_VAL(reg, EFUSE_TMRF_BFW, EFUSE_TMRF_1200OHM,
			EFUSE_TMRF_BITNUM);
	DW_BITS_SET_VAL(reg, EFUSE_A_BFW, addr, EFUSE_A_BITNUM);
	writel(reg, SYS_EFUSE_CFG);

	/*XXX 9.5+ns delay?*/
	/*READ enable*/
	reg = readl(SYS_EFUSE_CFG);
	DW_BIT_SET(reg, EFUSE_READ_BFW);
	writel(reg, SYS_EFUSE_CFG);

	/*XXX 34+ns delay?*/
	/*read data*/
	data = (unsigned char)readl(SYS_EFUSE_READ_DAT);

	/*disalbe*/
	reg = readl(SYS_EFUSE_CFG);
	DW_BIT_SET0(reg, EFUSE_READ_BFW);
	DW_BIT_SET(reg, EFUSE_ENB_BFW);
	DW_BIT_SET(reg, EFUSE_PROGRAM_BFW);
	DW_BIT_SET0(reg, EFUSE_POR_10_BFW);
	writel(reg, SYS_EFUSE_CFG);

	/*Read done*/
	writel(EFUSE_READ_DONE, SYS_EFUSE_CTL);

	/*power off*/
	reg = readl(SYS_EFUSE_CFG);
	DW_BIT_SET0(reg, EFUSE_PWR_BFW);
	writel(reg, SYS_EFUSE_CFG);

	return data;
}

