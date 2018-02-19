 /*
 * Copyright 2014  Darcy Lu (darcy_lu@realsil.com.cn)
 */

#include <config.h>
#include <common.h>
#include <dma.h>

#include <linux/compiler.h>

#include "bspchip.h"

/* ----------------------------------------
 DMA COPY start
 ----------------------------------------*/
void dma_copy(u32 src_addr, u32 dst_addr, u32 trans_length)
{
	u32 rd_data;

	u32 src_addr_cur;
	u32 dst_addr_cur;
	u32 cur_transfer_size;
	u64 ltmp;

	src_addr_cur = src_addr;
	dst_addr_cur = dst_addr;

	/*select SRC_TR_WIDTH->32 bits
	select BLOCK_TS->2048
	so Block Transfer Size = 8K Bytes*/
	while (trans_length) {
		if (trans_length > 8192)
			cur_transfer_size = 8192;
		else
			cur_transfer_size = trans_length;

		/*1. check Channel Enable register*/
		rd_data = REG64(CHENREG);
		if (rd_data != 0)
			printf("*E: Channel is occupied");

		/*2. Clear any pending interrupts*/
		REG64(CLEARTFR) = CH0_INT;
		REG64(CLEARBLOCK) = CH0_INT;
		REG64(CLEARSRCTRAN) = CH0_INT;
		REG64(CLEARDSTTRAN) = CH0_INT;
		REG64(CLEARERR) = CH0_INT;

		/*3. Program Channel registers*/
		/*3.a SAR*/
		REG64(SAR0) = src_addr_cur;
		rd_data = REG64(SAR0);

		if (rd_data != src_addr_cur)
			printf("*E: set SRC_ADDR Failed!%x %x\n", src_addr_cur, rd_data);

		/*3.b DAR*/
		REG64(DAR0) = dst_addr_cur;
		rd_data = REG64(DAR0);

		if (rd_data != dst_addr_cur)
			printf("*E: set DST_ADDR Failed! %x", rd_data);

		/*3.c Program CTL & CFG according to Row1, in Table 7-1*/
		REG64(CTL0) = 0x01; /*enable interrupt*/
		REG64(CFG0) = 0x00;

		/*3.d Program CTL for length...*/
		ltmp = cur_transfer_size/4;
		/*
			bit 3:1 	DST_TR_WIDTH		010		32bit
			bit 6:4 	SRC_TR_WIDTH		010		32bit
			bit 8:7 	DINC			00		increment
			bit 10:9 	SINC			00		increment
			bit 13:11 	DEST_MSIZE		100		32
			bit 16:14 	SRC_MSIZE		100		32
			bit 43:32	BLOCK_TS
		*/
		REG64(CTL0) = (ltmp<<32)|0x12025;

		/*3.e Program CFG*/
		REG64(CFG0) = 0x00;

		/*4 Enable Channel Enable*/
		rd_data = REG64(DMACFGREG);
		if ((rd_data&0x00000001) == 0)
			REG64(DMACFGREG) = 0x01;

		/*DMA channel0 enable*/
		REG64(CHENREG) = 0x101;

		/*5. Transfer Start*/

		/*6. Transfer complete, interrupt assert*/
		while (1) {
			rd_data = REG64(RAWTFR);
			if ((rd_data&0x00000001) == 1)
				break;
		}

		src_addr_cur += cur_transfer_size;
		dst_addr_cur += cur_transfer_size;
		trans_length -= cur_transfer_size;
	}

	/*Clear any pending interrupts*/
	REG64(CLEARTFR) = CH0_INT;
	REG64(CLEARBLOCK) = CH0_INT;
	REG64(CLEARSRCTRAN) = CH0_INT;
	REG64(CLEARDSTTRAN) = CH0_INT;
	REG64(CLEARERR) = CH0_INT;
}

