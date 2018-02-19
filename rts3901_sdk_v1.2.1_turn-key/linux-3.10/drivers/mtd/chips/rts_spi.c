/*
 * Driver for rlx SPI Controllers
 *
 * Copyright (C) 2006 Realtek Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <asm/rts_spi.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/gpio.h>

#define DRV_AUTHOR	"Darcy Lu"
#define DRV_DESC	"rlx on-chip SPI Controller Driver"
#define DRV_VERSION	"1.0"

MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);
MODULE_LICENSE("GPL");

/***** spi_flash.api/portmap
 * DESCRIPTION
 *  This is the structure used for accessing the spi_flash register
 *  portmap.
 * EXAMPLE
 *  struct spi_flash_portmap *portmap;
 *  portmap = (struct spi_flash_portmap *) spi_flash_BASE;
 *  foo = INP(portmap->ctrlr0 );
 * SOURCE
 */
 #define DRV_NAME "rts-spic"
/*#define _DEBUG_SPI_CONTROLLER_*/
static u8 QPIMode;
static u8 internal_dummy = 1;
struct spi_flash_portmap {
/*typedef u32 unsigned int;*/

/* Channel registers                                    */
/* The offset address for each of the channel registers */
/*  is shown for channel 0. For other channel numbers   */
/*  use the following equation.                         */
/*                                                      */
/*    offset = (channel_num * 0x058) + channel_0 offset */
/*                                                      */
	struct {
		volatile u32 ctrlr0;    /* Control Reg 0           (0x000) */
		volatile u32 ctrlr1;
		volatile u32 ssienr;    /* SPIC enable Reg1        (0x008) */
		volatile u32 mwcr;
		volatile u32 ser;       /* Slave enable Reg        (0x010) */
		volatile u32 baudr;
		volatile u32 txftlr;    /* TX_FIFO threshold level (0x018) */
		volatile u32 rxftlr;
		volatile u32 txflr;     /* TX_FIFO threshold level (0x020) */
		volatile u32 rxflr;
		volatile u32 sr;        /* Destination Status Reg  (0x028) */
		volatile u32 imr;
		volatile u32 isr;       /* Interrupt Stauts Reg    (0x030) */
		volatile u32 risr;
		volatile u32 txoicr;    /* TX_FIFO overflow_INT clear (0x038) */
		volatile u32 rxoicr;
		volatile u32 rxuicr;    /* RX_FIFO underflow_INT clear (0x040) */
		volatile u32 msticr;
		volatile u32 icr;       /* Interrupt clear Reg     (0x048) */
		volatile u32 dmacr;
		volatile u32 dmatdlr;   /* DMA TX_data level       (0x050) */
		volatile u32 dmardlr;
		volatile u32 idr;       /* Identiation Scatter Reg (0x058) */
		volatile u32 spi_flash_version;
		/*volatile u32 dr[32]; */   /* Data Reg          (0x060~0xdc)*/
		union{
			volatile u8  byte;
			volatile u16 half;
			volatile u32 word;
		} dr[32];
		volatile u32 rd_fast_single;
		volatile u32 rd_dual_o; /* Read dual data cmd Reg  (0x0e4) */
		volatile u32 rd_dual_io;
		volatile u32 rd_quad_o; /* Read quad data cnd Reg  (0x0ec) */
		volatile u32 rd_quad_io;
		volatile u32 wr_single; /* write single cmd Reg    (0x0f4) */
		volatile u32 wr_dual_i;
		volatile u32 wr_dual_ii;/* write dual addr/data cmd(0x0fc) */
		volatile u32 wr_quad_i;
		volatile u32 wr_quad_ii;/* write quad addr/data cnd(0x104) */
		volatile u32 wr_enable;
		volatile u32 rd_status; /* read status cmd Reg     (0x10c) */
		volatile u32 ctrlr2;
		volatile u32 fbaudr;    /* fast baud rate Reg      (0x114) */
		volatile u32 addr_length;
		volatile u32 auto_length; /* Auto addr length Reg  (0x11c) */
		volatile u32 valid_cmd;
		volatile u32 flash_size; /* Flash size Reg         (0x124) */
		volatile u32 flush_fifo;
	};
};


static  struct spi_flash_portmap *get_rlx_spi_base(void)
{
	return (struct spi_flash_portmap *)RLX_SPI_BASEADDR;
}




/*****/

/***** spi_flash.functions/dw_set_dummy_cycle()
 * DESCRIPTION
 *  This function is used to set the Baudr controller.
 *  ARGUMENTS
 *  dev         -- DMA controller device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EINVAL  -- if dum_cycle is out the range
 * SEE ALSO
 *  dw_device
 * SOURCE
 */
uint32_t spi_flash_set_dummy_cycle(struct spi_flash_portmap *spi_flash_map , uint32_t dum_cycle)
{
	uint32_t cycle;

	/* Disable SPI_FLASH*/
	spi_flash_map->ssienr = 0;

	/* if using fast_read baud_rate*/
	if (((spi_flash_map->ctrlr0) & 0x00400000))
		cycle = (spi_flash_map->fbaudr);
	else
		cycle = (spi_flash_map->baudr);

	if (dum_cycle != 0)
		cycle = (cycle * dum_cycle * 2) + internal_dummy;
	else
		cycle = 0;

	if (cycle > 0x10000)
		return DW_ECHRNG;

	DW_BITS_SET_VAL(spi_flash_map->auto_length, bfoSPI_FLASH_AUTO_LEN_DUM,
			cycle, bfwSPI_FLASH_AUTO_LEN_DUM);

	return 0;
}


static int spi_flash_setdr(struct spi_flash_portmap *spi_flash_map, enum spi_flash_dr_number dr_num,
			uint32_t data, enum spi_flash_byte_num byte_num)
{
	uint32_t wr_data;
	wr_data = data;

	if (dr_num > DR31)
		return DW_EINVAL;
	else {
		if (byte_num == DATA_BYTE) {
			spi_flash_map->dr[dr_num].byte = wr_data;
		} else if (byte_num == DATA_HALF) {
			spi_flash_map->dr[dr_num].half = wr_data;
		} else if (byte_num == DATA_WORD) {
			spi_flash_map->dr[dr_num].word = wr_data;
		} else {
			return DW_EINVAL;
		}
	}
	return 0;
}

static int spi_flash_getdr(struct spi_flash_portmap *spi_flash_map,
			enum spi_flash_dr_number dr_num,
			enum spi_flash_byte_num byte_num)
{
	u32 data;

	if (dr_num > DR31)
		return DW_ECHRNG;
	else {
		if (byte_num == DATA_BYTE) {
			data = spi_flash_map->dr[dr_num].byte & 0x000000ff;
		} else if (byte_num == DATA_HALF) {
			data = spi_flash_map->dr[dr_num].half & 0x0000ffff;
		} else if (byte_num == DATA_WORD) {
			data =  spi_flash_map->dr[dr_num].word;
		} else
			return DW_EIO;
	}

	return data;

}



static int spi_flash_wait_busy(struct spi_flash_portmap *spi_flash_map)
{
	int res = 0;
	u32 count = 0;

	for (count = 0; count < 1000; count++) {
		if (DW_BIT_GET_UNSHIFTED(spi_flash_map->sr, bfoSPI_FLASH_SR_TXE)) {
			printk("spi_flash_wait_busy: transfer error. \n");
			res = -1;
			break;
		} else {
			if ((!DW_BIT_GET_UNSHIFTED(spi_flash_map->sr, bfoSPI_FLASH_SR_BUSY))) {
				/* not busy*/
				break;
			}
		}
		udelay(50);
	}
	return res;
}

static u8 flash_rx_cmd(struct spi_flash_portmap *spi_flash_map, u8 cmd)
{
	uint32_t rd_data;
	int res = 0;

	/*printf("set rx_cmd address:%x \n", (int)spi_flash_map);*/
	/* Disble SPI_FLASH*/
	spi_flash_map->ssienr = 0;

	/* set ctrlr0: RX_mode*/
	rd_data = spi_flash_map->ctrlr0;
	if (QPIMode == 1)
		spi_flash_map->ctrlr0 = (rd_data & 0xffc0fcff) | 0x00000300|QPI_CHN_SETTING;
	else
		spi_flash_map->ctrlr0 = (rd_data & 0xffc0fcff) | 0x00000300;

	/* set flash_cmd: write cmd to fifo*/
	spi_flash_setdr(spi_flash_map, DR0, cmd, DATA_BYTE);

	/*printf("bef enable spi_flash, cmd is %x, ser is %x, \n", cmd, spi_flash_map->ser);*/
	/* Enable SPI_FLASH*/
	spi_flash_map->ssienr = 1;
	/*printf("rx cmd wait\n");*/
	res = spi_flash_wait_busy(spi_flash_map);

	if (res != 0) {
		printk("flash_rx_cmd:error when wait busy\n");
		return res;
	}

	/*printf("rx_cmd end\n");*/

	return res;
}



int flash_tx_cmd(struct spi_flash_portmap *spi_flash_map, uint8_t cmd)
{
	/* struct device_info inst_info;*/
	uint32_t rd_data;
	int tmp, res = 0;

	/* Disble SPI_FLASH*/
	spi_flash_map->ssienr = 0;
	tmp = spi_flash_map->auto_length;

	/* set ctrlr0: TX mode*/
	rd_data = spi_flash_map->ctrlr0;
	if (QPIMode == 1)
		spi_flash_map->ctrlr0 = (rd_data & 0xffc0fcff) | QPI_CHN_SETTING;
	else
		spi_flash_map->ctrlr0 = (rd_data & 0xffc0fcff);

	/* set flash_cmd: wren to fifo*/
	spi_flash_setdr(spi_flash_map, DR0, cmd, DATA_BYTE);

	/* Enable SPI_FLASH*/
	spi_flash_map->ssienr = 1;

	res = spi_flash_wait_busy(spi_flash_map);

	spi_flash_map->ssienr = 0;
	spi_flash_map->auto_length = tmp;

	if (res != 0) {
		printk("flash_tx_cmd:error when wait busy\n");
		return res;
	}

	return res;
}


static u8 flash_get_status(struct spi_flash_portmap *spi_flash_map)
{

	uint8_t cmd_rdsr = 0x05;

	/* Disable SPI_FLASH*/
	spi_flash_map->ssienr = 0;

	/* Set Ctrlr1; 1 byte data frames*/
	spi_flash_map->ctrlr1 = 1;

	/* Set tuning dummy cycles*/
	DW_BITS_SET_VAL(spi_flash_map->auto_length, bfoSPI_FLASH_AUTO_LEN_DUM,
	0, bfwSPI_FLASH_AUTO_LEN_DUM);

	flash_rx_cmd(spi_flash_map, cmd_rdsr);

	return spi_flash_getdr(spi_flash_map, DR0, DATA_BYTE);
}

static void flash_wait_busy_delay100us(struct spi_flash_portmap *spi_flash_map, u32 timeout_count)
{
	/*Check flash is in write progress or not*/
	u32 count = 0;

	for (count = 0; count < timeout_count; count++) {
		if (!(flash_get_status(spi_flash_map) & 0x1))
			break;
		udelay(100);
	}
}

static void flash_wait_busy_delay10ms(struct spi_flash_portmap *spi_flash_map, u32 timeout_count)
{
	/*Check flash is in write progress or not*/
	u32 count = 0;

	for (count = 0; count < timeout_count; count++) {
		if (!(flash_get_status(spi_flash_map) & 0x1))
			break;
		msleep(10);
	}
}

static void readspi_fn(struct work_struct *wait_work)
{
	int i, j, res = 0;
	u32 rd_data, tmp, addr_len, ctrlr0, ctrlr1;
	u8 data_len, cmd_len;
	u8 *data_in, *cmd;

	struct spi_flash_portmap *spi_flash_map;
	struct rlx_spi	*spic = container_of(wait_work, struct rlx_spi, readspi);
	struct spi_message *msg = spic->msg;

	spi_flash_map = spic->base_address;
	cmd_len = spic->para.cmd_len;
	data_len = spic->para.data_len;
	cmd = spic->para.cmd;
	data_in = spic->para.data_in;

	/*printk("enter rlx_flash_read , address is %x\n", spi_flash_map);*/
	spi_flash_map->ssienr = 0;
	tmp = spi_flash_map->valid_cmd;
	addr_len = spi_flash_map->addr_length;
	ctrlr1 = spi_flash_map->ctrlr1;
	spi_flash_map->valid_cmd = 0x200;

	spi_flash_map->ctrlr1 = data_len;

	/* set ctrlr0: RX_mode*/
	ctrlr0 = spi_flash_map->ctrlr0;

	if (cmd[0] != spic->fast_read_cmd) {
		spi_flash_set_dummy_cycle(spi_flash_map, 0);
		spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300;
	} else {
		spi_flash_set_dummy_cycle(spi_flash_map, spic->dummy_cycle);

		switch (spic->autoread_type) {
		case RD_QUAD_IO:
			spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300 | (0x000a0000);
			break;

		case RD_QUAD_O:
			spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300 | (0x00080000);
			break;

		case RD_DUAL_IO:
			spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300 | (0x00050000);
			break;

		case RD_DUAL_O:
			spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300 | (0x00040000);
			break;

		default:
			spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300;
			break;
		}
	}

	if (QPIMode == 1)
		spi_flash_map->ctrlr0 |= QPI_CHN_SETTING;


	/*printk("qpi mod is %x, ctrlr0 is%x\n", QPIMode, spi_flash_map->ctrlr0);*/
	/* Writecmd, addr, data into FIFO*/
	spi_flash_setdr(spi_flash_map, DR0, cmd[0], DATA_BYTE);

	spi_flash_map->addr_length = (cmd_len - 1) % 4;

	if (cmd_len > 1) {
		for (i = 1; i <= (cmd_len - 1); i++)
			spi_flash_setdr(spi_flash_map, DR0, cmd[i], DATA_BYTE);
	} else {
		if (cmd[0] == 0x9f) {
			spi_flash_map->ctrlr1 = 3;
			data_len = 3;
		}
	}

	if (data_len == 0) {
		/*printk("uread cmd is %1x, dlen is %x, ctrl0 is %x, auto_len is %x, cmd_len is %x\n", cmd[0], data_len, spi_flash_map->ctrlr0, spi_flash_map->auto_length, cmd_len);*/
		goto END;
	}
#ifdef _DEBUG_SPI_CONTROLLER_
	printk("uread cmd is %1x, dlen is %x, ctrl0 is %x, auto_len is %x, cmd_len is %x\n", cmd[0], data_len, spi_flash_map->ctrlr0, spi_flash_map->auto_length, cmd_len);
	printk("\n");
#endif
	/* Enable SPI_FLASH*/
	spi_flash_map->ssienr = 1;

	res = spi_flash_wait_busy(spi_flash_map);
	if (res) {
		printk("flash_read: spi_flash_wait_busy failed\n");
		goto END;
	}

	/* Disble SPI_FLASH*/
	spi_flash_map->ssienr = 0;

	/*get data from FIFO, only for little endian CPU*/
	for (i = 0; i < (data_len/4); i++) {
		rd_data = spi_flash_getdr(spi_flash_map, i, DATA_WORD);
		for (j = 0; j < 4; j++) {
			*data_in = ((rd_data >> (j*8))&0xff);
			data_in++;
		}
	}

	for (i = 0; i < (data_len%4); i++) {
		/*not 4-byte alignment*/
		rd_data = spi_flash_getdr(spi_flash_map, i, DATA_BYTE);
		*data_in = rd_data&0xff;
		data_in++;
	}
	spi_flash_map->valid_cmd  = tmp;
	spi_flash_map->ctrlr0 = ctrlr0;
	spi_flash_map->addr_length = addr_len;
	spi_flash_map->ctrlr1 = ctrlr1;
	spi_flash_set_dummy_cycle(spi_flash_map, spic->dummy_cycle);

END:
	msg->complete(msg->context);
	spic->para.res = res;

	return;
}

static int legacy_flash_wait_busy(struct spi_flash_portmap *spi_flash_map)
{
	int res = 0;
	while (1) {
		if (DW_BIT_GET_UNSHIFTED(spi_flash_map->sr, bfoSPI_FLASH_SR_TXE)) {
			printk("spi_flash_wait_busy: transfer error. \n");
			res = -1;
			break;
		} else {
		if ((!DW_BIT_GET_UNSHIFTED(spi_flash_map->sr, bfoSPI_FLASH_SR_BUSY))) {
			break;
			}
		}
	}
	return res;
}

static int  legacy_rlx_flash_read(struct rlx_spi *spic, u8 *cmd, size_t cmd_len, u8 *data_in, u8 data_len, struct spi_message *msg)
{
		u32 ctrlr0, ctrlr1, rd_data, addr_len = 0;
	int i, j, tmp, res = 0;

	struct spi_flash_portmap *spi_flash_map;

	spi_flash_map = spic->base_address;

	spi_flash_map->ssienr = 0;
	tmp = spi_flash_map->valid_cmd;
	addr_len = spi_flash_map->addr_length;
	ctrlr1 = spi_flash_map->ctrlr1;
	spi_flash_map->valid_cmd = 0x200;

	spi_flash_map->ctrlr1 = data_len;

	/* set ctrlr0: RX_mode*/
	ctrlr0 = spi_flash_map->ctrlr0;

	if (cmd[0] != spic->fast_read_cmd) {
		spi_flash_set_dummy_cycle(spi_flash_map, 0);
		spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300;
	} else {
		spi_flash_set_dummy_cycle(spi_flash_map, spic->dummy_cycle);

		switch (spic->autoread_type) {
		case RD_QUAD_IO:
			spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300 | (0x000a0000);
			break;

		case RD_QUAD_O:
			spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300 | (0x00080000);
			break;

		case RD_DUAL_IO:
			spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300 | (0x00050000);
			break;

		case RD_DUAL_O:
			spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300 | (0x00040000);
			break;

		default:
			spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300;
			break;
		}
	}

	if (QPIMode == 1)
		spi_flash_map->ctrlr0 |= QPI_CHN_SETTING;


	/* Writecmd, addr, data into FIFO*/
	spi_flash_setdr(spi_flash_map, DR0, cmd[0], DATA_BYTE);

	spi_flash_map->addr_length = (cmd_len - 1) % 4;

	if (cmd_len > 1) {
		for (i = 1; i <= (cmd_len - 1); i++)
			spi_flash_setdr(spi_flash_map, DR0, cmd[i], DATA_BYTE);
	} else {
		if (cmd[0] == 0x9f) {
			spi_flash_map->ctrlr1 = 3;
			data_len = 3;
		}
	}

	if (data_len == 0) {
		return res;
	}
#ifdef _DEBUG_SPI_CONTROLLER_
	printk("uread cmd is %1x, dlen is %x, ctrl0 is %x, auto_len is %x, cmd_len is %x\n", cmd[0], data_len, spi_flash_map->ctrlr0, spi_flash_map->auto_length, cmd_len);
	printk("\n");
#endif
	/* Enable SPI_FLASH*/
	spi_flash_map->ssienr = 1;

	res = legacy_flash_wait_busy(spi_flash_map);
	if (res) {
		printk("flash_read: spi_flash_wait_busy failed\n");
		return res;
	}

	/* Disble SPI_FLASH*/
	spi_flash_map->ssienr = 0;

	/*get data from FIFO, only for little endian CPU*/
	for (i = 0; i < (data_len/4); i++) {
		rd_data = spi_flash_getdr(spi_flash_map, i, DATA_WORD);
		for (j = 0; j < 4; j++) {
			*data_in = ((rd_data >> (j*8))&0xff);
			data_in++;
		}
	}

	for (i = 0; i < (data_len%4); i++) {
		/*not 4-byte alignment*/
		rd_data = spi_flash_getdr(spi_flash_map, i, DATA_BYTE);
		*data_in = rd_data&0xff;
		data_in++;
	}
	spi_flash_map->valid_cmd  = tmp;
	spi_flash_map->ctrlr0 = ctrlr0;
	spi_flash_map->addr_length = addr_len;
	spi_flash_map->ctrlr1 = ctrlr1;
	spi_flash_set_dummy_cycle(spi_flash_map, spic->dummy_cycle);
	return res;
}
static int  rlx_flash_read(struct rlx_spi	*spic, u8 *cmd, size_t cmd_len, u8 *data_in, u8 data_len, struct spi_message *msg)
{
	if (spic->polling) {
		return legacy_rlx_flash_read(spic, cmd, cmd_len, data_in, data_len, msg);
	}

	spic->para.cmd = cmd;
	spic->para.cmd_len = cmd_len;
	spic->para.data_len = data_len;
	spic->para.data_in = data_in;
	spic->msg = msg;

	schedule_work(&spic->readspi);
	return 0;
}

static void writespi_fn(struct work_struct *wait_work)
{
	int i, j;
	u32 tmp;
	u32 init_data;
	u32 addr_len;
	u8 *data_out, *cmd, cmd_len, data_len, res = 0;
	struct spi_flash_portmap *spi_flash_map;
	struct rlx_spi	*spic = container_of(wait_work, struct rlx_spi, writespi);
	struct spi_message *msg = spic->msg;

	cmd_len = spic->para.cmd_len;
	data_len = spic->para.data_len;
	cmd = spic->para.cmd;
	data_out = spic->para.data_out;
	spi_flash_map = spic->base_address;

	spi_flash_map->ssienr = 0;
	/*set ctrlr0: TX mode*/
	init_data = spi_flash_map->ctrlr0;
	addr_len = spi_flash_map->addr_length;
	spi_flash_set_dummy_cycle(spi_flash_map, 0);

	if (cmd[0] == spic->fast_write_cmd)	 {
		switch (spic->userwrite_type) {
		case WR_QUAD_II_T:
			spi_flash_map->ctrlr0 = (init_data & 0xffc0fcff) | (0x000a0000);
			break;

		case WR_QUAD_I_T:
			spi_flash_map->ctrlr0 = (init_data & 0xffc0fcff) | (0x00080000);
			break;

		case WR_DUAL_II_T:
			spi_flash_map->ctrlr0 = (init_data & 0xffc0fcff) | (0x00050000);
			break;

		case WR_DUAL_I_T:
			spi_flash_map->ctrlr0 = (init_data & 0xffc0fcff) | (0x00040000);
			break;

		default:
			spi_flash_map->ctrlr0 = init_data & 0xffc0fcff;
			break;
		}
	} else
		spi_flash_map->ctrlr0 = init_data & 0xffc0fcff;

	if (QPIMode == 1)
		spi_flash_map->ctrlr0 |= QPI_CHN_SETTING;

	spi_flash_map->addr_length = (cmd_len - 1) % 4;

	/* Write cmd, addr, data into FIFO*/
	spi_flash_setdr(spi_flash_map, DR0, cmd[0], DATA_BYTE);
	for (i = 1; i < cmd_len; i++) {
		spi_flash_setdr(spi_flash_map, DR0, cmd[i], DATA_BYTE);
	}

	/*write data, only for little endian CPU*/
	if (data_len != 0) {
		for (i = 0; i < (data_len/4); i++) {
			tmp = 0;
			for (j = 0; j < 4; j++) {
				tmp = (*data_out << (j*8)) | tmp;
				data_out++;
			}
			spi_flash_setdr(spi_flash_map, i, tmp, DATA_WORD);
		}

		for (i = 0; i < (data_len%4); i++) {
			tmp = *data_out;
			spi_flash_setdr(spi_flash_map, i, tmp, DATA_BYTE);
			data_out++;
		}
	}
#ifdef _DEBUG_SPI_CONTROLLER_
	printk("uwrite cmd is %1x, dlen is %x, ctrl0 is %x, auto_len is %x, cmd_len is %x\n", cmd[0], data_len, spi_flash_map->ctrlr0, spi_flash_map->auto_length, cmd_len);
	printk("\n");
#endif
	/* Enable SPI_FLASH*/
	spi_flash_map->ssienr = 1;

	res = spi_flash_wait_busy(spi_flash_map);
	if (res) {
		printk("flash_write: spi_flash_wait_busy failed\n");
		goto END;
	}

	if (((cmd[0] == 0x35) && (spic->flags & QPI_I)) || ((cmd[0] == 0x38) && (spic->flags & QPI_II))) {
		printk("enter QPI mode\n");
		QPIMode = 1;
	}

	/*darcy_lu 2015-07-23, for block erase command,
	typical time is 280~2000ms for different spi flash,
	but for other command,  typical time is no more than 15ms*/
	if (cmd[0] == BLOCK_ERASE_CMD)
		flash_wait_busy_delay10ms(spi_flash_map, 300);
	else
		flash_wait_busy_delay100us(spi_flash_map, 300);

	spi_flash_map->ctrlr0 = init_data;
	spi_flash_map->addr_length = addr_len;
	spi_flash_set_dummy_cycle(spi_flash_map, spic->dummy_cycle);

END:
	msg->complete(msg->context);
	spic->para.res = res;

	return;
}

static int legacy_rlx_flash_write(struct rlx_spi *spic, u8 *cmd, size_t cmd_len, u8 *data_out, u8 data_len, struct spi_message *msg)
{
	int res = 0;
	int i, j;
	u32 tmp;
	u32 init_data;
	u32 addr_len;
	struct spi_flash_portmap *spi_flash_map;

	spi_flash_map = spic->base_address;

	spi_flash_map->ssienr = 0;
	/*set ctrlr0: TX mode*/
	init_data = spi_flash_map->ctrlr0;
	addr_len = spi_flash_map->addr_length;
	spi_flash_set_dummy_cycle(spi_flash_map, 0);

	if (cmd[0] == spic->fast_write_cmd)	 {
		switch (spic->userwrite_type) {
		case WR_QUAD_II_T:
			spi_flash_map->ctrlr0 = (init_data & 0xffc0fcff) | (0x000a0000);
			break;

		case WR_QUAD_I_T:
			spi_flash_map->ctrlr0 = (init_data & 0xffc0fcff) | (0x00080000);
			break;

		case WR_DUAL_II_T:
			spi_flash_map->ctrlr0 = (init_data & 0xffc0fcff) | (0x00050000);
			break;

		case WR_DUAL_I_T:
			spi_flash_map->ctrlr0 = (init_data & 0xffc0fcff) | (0x00040000);
			break;

		default:
			spi_flash_map->ctrlr0 = init_data & 0xffc0fcff;
			break;
		}
	} else
		spi_flash_map->ctrlr0 = init_data & 0xffc0fcff;

	if (QPIMode == 1)
		spi_flash_map->ctrlr0 |= QPI_CHN_SETTING;

	spi_flash_map->addr_length = (cmd_len - 1) % 4;

	/* Write cmd, addr, data into FIFO*/
	spi_flash_setdr(spi_flash_map, DR0, cmd[0], DATA_BYTE);
	for (i = 1; i < cmd_len; i++) {
		spi_flash_setdr(spi_flash_map, DR0, cmd[i], DATA_BYTE);
	}

	/*write data, only for little endian CPU*/
	if (data_len != 0) {
		for (i = 0; i < (data_len/4); i++) {
			tmp = 0;
			for (j = 0; j < 4; j++) {
				tmp = (*data_out << (j*8)) | tmp;
				data_out++;
			}
			spi_flash_setdr(spi_flash_map, i, tmp, DATA_WORD);
		}

		for (i = 0; i < (data_len%4); i++) {
			tmp = *data_out;
			spi_flash_setdr(spi_flash_map, i, tmp, DATA_BYTE);
			data_out++;
		}
	}
#ifdef _DEBUG_SPI_CONTROLLER_
	printk("uwrite cmd is %1x, dlen is %x, ctrl0 is %x, auto_len is %x, cmd_len is %x\n", cmd[0], data_len, spi_flash_map->ctrlr0, spi_flash_map->auto_length, cmd_len);
	printk("\n");
#endif
	/* Enable SPI_FLASH*/
	spi_flash_map->ssienr = 1;

	res = legacy_flash_wait_busy(spi_flash_map);
	if (res) {
		printk("flash_write: spi_flash_wait_busy failed\n");
		return res;
	}

	if (((cmd[0] == 0x35) && (spic->flags & QPI_I)) || ((cmd[0] == 0x38) && (spic->flags & QPI_II))) {
		printk("enter QPI mode\n");
		QPIMode = 1;
	}
	legacy_flash_wait_busy(spi_flash_map);

	spi_flash_map->ctrlr0 = init_data;
	spi_flash_map->addr_length = addr_len;
	spi_flash_set_dummy_cycle(spi_flash_map, spic->dummy_cycle);
	return res;
}

static int rlx_flash_write(struct rlx_spi	*spic, u8 *cmd, size_t cmd_len, u8 *data_out, u8 data_len, struct spi_message *msg)
{
	if (spic->polling) {
		return legacy_rlx_flash_write(spic, cmd, cmd_len, data_out, data_len, msg);
	}

	spic->para.cmd = cmd;
	spic->para.cmd_len = cmd_len;
	spic->para.data_out = data_out;
	spic->para.data_len = data_len;
	spic->msg = msg;

	schedule_work(&spic->writespi);

	return 0;
}

int spi_flash_set_auto(struct rlx_spi	*spic)
{
	struct spi_flash_portmap *spi_flash_map;
	spi_flash_map = spic->base_address;

	/* Disble SPI_FLASH*/
	spi_flash_map->ssienr = 0;



	/*Set valid_cmd_reg: auto_cmd*/
	/*
	if (spi->autorw_type & WR_QUAD_II_T) {
		spi_flash_map->wr_quad_ii = spi->write_cmd;
	} else if(spi->autorw_type & WR_QUAD_I_T) {
		spi_flash_map->wr_quad_i = spi->write_cmd;
	} else if (spi->autorw_type == WR_DUAL_II_T) {
		spi_flash_map->wr_dual_ii = spi->write_cmd;
	} else if (spi->autorw_type == WR_DUAL_I_T) {
		spi_flash_map->wr_dual_i = spi->write_cmd;
	}
	*/

	if (spic->autoread_type & RD_QUAD_IO_T) {
		spi_flash_map->rd_quad_io = spic->fast_read_cmd;
	} else if (spic->autoread_type & RD_QUAD_O_T) {
		spi_flash_map->rd_quad_o = spic->fast_read_cmd;
	} else if (spic->autoread_type & RD_DUAL_IO_T) {
		spi_flash_map->rd_dual_io = spic->fast_read_cmd;
	} else if (spic->autoread_type & RD_DUAL_O_T) {
		spi_flash_map->rd_dual_o = spic->fast_read_cmd;
	} else if (spic->autoread_type & FRD_SINGLE_T) {
		spi_flash_map->rd_fast_single  = spic->fast_read_cmd;
	}

	spi_flash_set_dummy_cycle(spi_flash_map, spic->dummy_cycle);

	spi_flash_map->valid_cmd = (spic->autoread_type | 0x200);
#define CONFIG_USE_WRONG_WRCMD
#ifdef CONFIG_USE_WRONG_WRCMD
	spi_flash_map->wr_single = 0xBB;
#endif

#undef CONFIG_OPEN_SPIFLASH_WMPU
#ifdef CONFIG_OPEN_SPIFLASH_WMPU
#define WMPU_ENTRY_ENABLE	(0xF << 16)
#define SPI_VADDR0		0xBFC00000
#define SPI_VADDR0_MASK		(0x1FF << 3)
#define SPI_VADDR0_EXTRA_MASK	(0x3FF << 12)
#define SPI_VADDR1		0xBC000000
#define SPI_VADDR1_MASK		(0x1FF << 3)
#define SPI_VADDR1_EXTRA_MASK	(0x1FFF << 12)
#define SPI_VADDR2		0x9FC00000
#define SPI_VADDR2_MASK		(0x1FF << 3)
#define SPI_VADDR2_EXTRA_MASK	(0x3FF << 12)
#define SPI_VADDR3		0x9C000000
#define SPI_VADDR3_MASK		(0x1FF << 3)
#define SPI_VADDR3_EXTRA_MASK	(0x1FFF << 12)
	/* Entry 0: 0xBFC0_0000 ~ 0xBFFF_FFFF */
	write_c0_watchlo0(get_watch_write_value(SPI_VADDR0));
	write_c0_watchhi0(SPI_VADDR0_MASK | WMPU_GLOBAL_MATCH
			| WMPU_MORE_ENTRY);
	write_lxc0_wmpxmask0(SPI_VADDR0_EXTRA_MASK);
	/* Entry 1: 0xBC00_0000 ~ 0xBDFF_FFFF */
	write_c0_watchlo1(get_watch_write_value(SPI_VADDR1));
	write_c0_watchhi1(SPI_VADDR1_MASK | WMPU_GLOBAL_MATCH
			| WMPU_MORE_ENTRY);
	write_lxc0_wmpxmask1(SPI_VADDR1_EXTRA_MASK);
	/* Entry 2: 0x9FC0_0000 ~ 0x9FFF_FFFF */
	write_c0_watchlo2(get_watch_write_value(SPI_VADDR2));
	write_c0_watchhi2(SPI_VADDR2_MASK | WMPU_GLOBAL_MATCH
			| WMPU_MORE_ENTRY);
	write_lxc0_wmpxmask2(SPI_VADDR2_EXTRA_MASK);
	/* Entry 3: 0x9C00_0000 ~ 0x9DFF_FFFF */
	write_c0_watchlo3(get_watch_write_value(SPI_VADDR3));
	write_c0_watchhi3(SPI_VADDR3_MASK | WMPU_GLOBAL_MATCH);
	write_lxc0_wmpxmask3(SPI_VADDR3_EXTRA_MASK);
	/* Enable entry0 and entry 1, watchpoint mode */
	write_lxc0_wmpctl(WMPU_ENTRY_ENABLE | WMPU_KERNELWATCH_ENABLE
			| WMPU_WATCHPOINT_MODE);
#endif

/*	printk("set auto command, %x,  %x, %x\n", spi_flash_map->auto_length,  spic->autoread_type, spi_flash_map->valid_cmd);*/
	return 0;
}

int spi_flash_set_addr_length(struct rlx_spi	*spic, u8 length)
{
	struct spi_flash_portmap *spi_flash_map;
	spi_flash_map = spic->base_address;

	/* Disble SPI_FLASH*/
	spi_flash_map->ssienr = 0;
	if (length == 4)
		DW_BITS_SET_VAL(spi_flash_map->auto_length, bfoSPI_FLASH_AUTO_LEN_ADDR,
		0, bfwSPI_FLASH_AUTO_LEN_ADDR);
	else
		DW_BITS_SET_VAL(spi_flash_map->auto_length, bfoSPI_FLASH_AUTO_LEN_ADDR,
		3, bfwSPI_FLASH_AUTO_LEN_ADDR);

	return 0;
}

static int rlx_spi_setup(struct spi_device *spi)
{
	int	ret = 0;
	struct spi_flash_portmap *spi_flash_map;
	struct rlx_spi_master *spicm = spi_master_get_devdata(spi->master);
	struct rlx_spi			*spic = kmalloc(sizeof(struct rlx_spi), GFP_KERNEL);

	spicm->spic = spic;
	/*spic = spi_master_get_devdata(spi->master);*/
	spic->base_address = get_rlx_spi_base();

	spi_flash_map = spic->base_address;

	spi_flash_map->ser = 1;

	/*disable all interrupt*/
	spi_flash_map->imr = 0;

	INIT_WORK(&spic->readspi, readspi_fn);
	INIT_WORK(&spic->writespi, writespi_fn);
	spic->polling = 0;

	spicm->setautomode = spi_flash_set_auto;
	spicm->setaddrlen = spi_flash_set_addr_length;
	/*printk("spic set up, addr=%x, %x, %x, %x\n", spi, spic, spi_flash_map, spicm->setautomode);*/
	QPIMode = 0;

	return ret;
}

static void rlx_spi_cleanup(struct spi_device *spi)
{
	struct rlx_spi_master *spicm = spi_master_get_devdata(spi->master);
	struct rlx_spi	*spic = spicm->spic;

	if (spic->polling == 0) {
		cancel_work_sync(&spic->readspi);
		cancel_work_sync(&spic->writespi);
	}
	kfree(spicm->spic);
	return;
}

static int rlx_spi_transfer(struct spi_device *spi, struct spi_message *msg)
{
	struct spi_transfer	*xfer1, *xfer2;
	struct spi_flash_portmap *spi_flash_map;
	struct rlx_spi_master *spicm = spi_master_get_devdata(spi->master);
	struct rlx_spi			*spic = spicm->spic;

	spi_flash_map = spic->base_address;

	/*tx transfer*/
	xfer1 = list_first_entry(&msg->transfers, struct spi_transfer, transfer_list);
	msg->actual_length = xfer1->len;
#ifdef _DEBUG_SPI_CONTROLLER_
	printk("begin transfer\n");
	printk("%x, %x, %x, %x, %x, %x\n", spi_flash_map->valid_cmd, spi_flash_map->auto_length, spi_flash_map->ctrlr0, spi_flash_map->baudr, spi_flash_map->addr_length, xfer1->len)	;
#endif

	if (list_is_last(&xfer1->transfer_list, &msg->transfers)) {
		/*writesr, erase_sector, only one transfer*/
		rlx_flash_write(spic, (u8 *)xfer1->tx_buf, xfer1->len, NULL, 0, msg);
	} else {
		xfer2 = list_entry(xfer1->transfer_list.next, struct spi_transfer, transfer_list);
		if (xfer2->tx_buf) {
			/*write data , two transfers*/
			rlx_flash_write(spic, (u8 *)xfer1->tx_buf, xfer1->len, (u8 *)xfer2->tx_buf, xfer2->len, msg);
			msg->actual_length += xfer2->len;
		} else {
			/*read data, read sr, two transfers */
			rlx_flash_read(spic, (u8 *)xfer1->tx_buf, xfer1->len, (u8 *)xfer2->rx_buf, xfer2->len, msg);
			msg->actual_length += xfer2->len;
		}
	}
	msg->status = spic->para.res;
	if (spic->polling) {
		msg->complete(msg->context);
	}
	return 0;
}
/*
static irqreturn_t rts_spi_isr(int irq, void *dev_id)
{
	struct spi_flash_portmap *spi_flash_map;
	struct rlx_spi_master *spicm = dev_id;
	spi_flash_map = (struct spi_flash_portmap *)spicm->IO_BaseAddr;

	printk("spi isr %x, %x\n", spi_flash_map->isr, spi_flash_map->imr);
	return IRQ_HANDLED;
}

static int rts_spi_acquire_irq(struct rlx_spi_master *spicm)
{
	int err = 0;

	err = request_irq(spicm->irq, rts_spi_isr, IRQF_SHARED,
			RTS_SPI_DRV_NAME,  spicm);
	if (err)
		printk("request IRQ %d failed\n", spicm->irq);
	else
		printk("request IRQ %d success\n", spicm->irq);
	return err;
}
*/

static ssize_t show_polling(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct spi_master *master = platform_get_drvdata(pdev);
	struct rlx_spi_master *spicm = spi_master_get_devdata(master);
	struct rlx_spi *spic = spicm->spic;

	return sprintf(buf, "%d\n", spic->polling);
}

static ssize_t store_polling(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	long status = -EIO;
	long value;
	struct platform_device *pdev = to_platform_device(dev);
	struct spi_master *master = platform_get_drvdata(pdev);
	struct rlx_spi_master *spicm = spi_master_get_devdata(master);
	struct rlx_spi *spic = spicm->spic;

	status = strict_strtoul(buf, 0, &value);
	if ((status == 0) && (value == 1) && (spic->polling == 0)) {
		cancel_work_sync(&spic->readspi);
		cancel_work_sync(&spic->writespi);
		spic->polling = 1;
	}

	return size;
}

static DEVICE_ATTR(polling, S_IRUGO | S_IWUSR, show_polling, store_polling);

static int rlx_spi_probe(struct platform_device *pdev)
{
	struct resource		*regs;
	int	irq;
	int	ret = 0;
	int	status = 0;
	struct spi_master	*master;
	struct rlx_spi_master	*spicm;

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs)
		return -ENXIO;

/*
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;
*/
	/* setup spi core then atmel-specific driver state */
	master = spi_alloc_master(&pdev->dev, sizeof *spicm);

	if (!master)
		return ENOMEM;

	/* the spi->mode bits understood by this driver: */
	spicm = spi_master_get_devdata(master);
	spicm->master = master;
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH;
	master->dev.of_node = pdev->dev.of_node;
	master->bus_num = pdev->id;
	master->num_chipselect = master->dev.of_node ? 0 : 4;
	master->setup = rlx_spi_setup;
	master->transfer = rlx_spi_transfer;
	master->cleanup = rlx_spi_cleanup;
	platform_set_drvdata(pdev, master);
	spicm->irq = irq;
	status = spi_register_master(master);

	printk("INFO:allocate spi master %x, %x\n", pdev->id, master->bus_num);

	if (status != 0) {
		printk("problem registering spi master\n");
		return ENOMEM;
	}

	spin_lock_init(&spicm->lock);
	INIT_LIST_HEAD(&spicm->queue);

	spicm->pdev = pdev;
	spicm->IO_BaseAddr = (u32)ioremap(regs->start, resource_size(regs));
/*	rts_spi_acquire_irq(spicm);*/
	ret = sysfs_create_file(&pdev->dev.kobj, &dev_attr_polling.attr);
	if (ret)
		dev_err(&pdev->dev, "%s:sysfs_create_file failed\n", __func__);

	return ret;
}

/* stop hardware and remove the driver */
static int spic_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);

	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_polling.attr);
	if (!master)
		return 0;

	/* Disconnect from the SPI framework */
	spi_unregister_master(master);

	/* Prevent double remove */
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#define spic_spi_suspend NULL
#define spic_spi_resume NULL

MODULE_ALIAS("platform:spic-spi");
static struct platform_driver spic_spi_driver = {
	.driver	= {
		.name	= "spic-platform",
		.owner	= THIS_MODULE,
	},
	.suspend	= spic_spi_suspend,
	.resume		= spic_spi_resume,
	.remove		= spic_spi_remove,
};

static int __init spic_spi_init(void)
{
	return platform_driver_probe(&spic_spi_driver, rlx_spi_probe);
}
subsys_initcall(spic_spi_init);

static void __exit spic_spi_exit(void)
{
	platform_driver_unregister(&spic_spi_driver);
}
module_exit(spic_spi_exit);

