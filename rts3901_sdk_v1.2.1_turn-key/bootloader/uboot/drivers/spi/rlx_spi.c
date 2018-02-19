/*
 * (C) Copyright 2009
 * Frank Bodammer <frank.bodammer@gcd-solutions.de>
 * (C) Copyright 2009 Semihalf, Grzegorz Bernacki
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <spi.h>
#include "rlx_spi.h"

static uint8_t QPIMode;
static uint8_t internal_dummy = 1;
static uint8_t flash_rx_cmd(struct rlx_spi_slave *rlxslave, uint8_t cmd);

void spi_init(void)
{}


int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	return bus < 2 && cs < 3;
}




struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
				  unsigned int max_hz, unsigned int mode)
{
	struct rlx_spi_slave *rlxslave;

	if (!spi_cs_is_valid(bus, cs))
		return NULL;

	rlxslave = spi_alloc_slave(struct rlx_spi_slave, bus, cs);
	if (!rlxslave) {
		printf("SPI_error: Fail to allocate rlx_spi_slave\n");
		return NULL;
	}

	rlxslave->base_address = get_rlx_spi_base();
	rlxslave->mode = mode;
	rlxslave->fifo_depth = RLX_SPI_FIFO_DEPTH;
	rlxslave->input_hz = 166666700;
	rlxslave->speed_hz = rlxslave->input_hz / 2;
	rlxslave->req_hz = max_hz;
	rlxslave->base_address->ser = 1;
#ifdef CONFIG_TARGET_ASIC
	rlxslave->base_address->baudr = 1;
#endif
#ifdef CONFIG_TARGET_FPGA
	rlxslave->base_address->baudr = 8;
#endif
	rlxslave->base_address->fbaudr = rlxslave->base_address->baudr;

	/*default SPI mode*/
	QPIMode = 0;
	rlxslave->base_address->ssienr = 0;
	rlxslave->base_address->ctrlr0 &= 0xffc0ffff;
	/*printf("base_addr:%lx, 0x28:%lx,  0x2c:%x,  0xE0:%x, 0xE4:%x,
	0xE8:%x, 0xEC:%x\n",
	rlxslave->base_address, rlxslave->base_address->sr,
	rlxslave->base_address->imr, rlxslave->base_address->rd_fast_single,
	rlxslave->base_address->rd_dual_o, rlxslave->base_address->rd_dual_io,
	rlxslave->base_address->rd_quad_o);*/
	return &rlxslave->slave;
}



/***** spi_flash.functions/spi_flash_setctrlr1()
 * DESCRIPTION
 *  This function is used to set the ctrlr1 controller.
 *  ARGUMENTS
 *  dev         -- DMA controller device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EINVAL  -- if num_frame is out of range.
 * SEE ALSO
 *  dw_device
 * SOURCE
 */

uint32_t spi_flash_setctrlr1(struct rlx_spi_slave *rlxslave, uint32_t num_frame)
{
	struct spi_flash_portmap *spi_flash_map;

	spi_flash_map = rlxslave->base_address;
	if (num_frame > 0x00010000) {
		return DW_ENODATA;
	} else {
		spi_flash_map->ctrlr1 = num_frame;

/*	printf("spi_flash_setctrlr1 = 0x%08x\n", spi_flash_map->ctrlr1);*/

		return 0;
	}
}


/*****/

/***** spi_flash.functions/spi_flash_getdr()
 * DESCRIPTION
 *  This function is used to read the ctrlr1 controller.
 *  ARGUMENTS
 *  dev         -- DMA controller device handle
 * RETURN VALUE
 *  uint32_t    -- data popping from FIFO
 * SEE ALSO
 *  dw_device
 * SOURCE
 */

uint32_t spi_flash_getdr(struct rlx_spi_slave *rlxslave,
			enum spi_flash_dr_number dr_num,
			enum spi_flash_byte_num byte_num)
{
	uint32_t data;
	struct spi_flash_portmap *spi_flash_map;

	spi_flash_map = rlxslave->base_address;

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


void spi_free_slave(struct spi_slave *slave)
{
	free(slave);
}

int spi_claim_bus(struct spi_slave *slave)
{
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
	return;
}



/*****/
/***** spi_flash.functions/spi_flash_wait_nobusy()
 * DESCRIPTION
 *  This function is used to wait the spi_flash is not at busy state.
 *  ARGUMENTS
 *  dev         -- DMA controller device handle
 * SEE ALSO
 *  dw_device
 * SOURCE
 */

int spi_flash_wait_busy(struct rlx_spi_slave *rlxslave)
{
	struct spi_flash_portmap *spi_flash_map;

	spi_flash_map = rlxslave->base_address;
	int res = 0;

	while (1) {
		if (DW_BIT_GET_UNSHIFTED(spi_flash_map->sr, bfoSPI_FLASH_SR_TXE)) {
			printf("spi_flash_wait_busy: transfer error. \n");
			res = -1;
			break;
		} else {
			if ((!DW_BIT_GET_UNSHIFTED(spi_flash_map->sr, bfoSPI_FLASH_SR_BUSY)))   /* not busy*/{
				break;
			}
		}
	}
	return res;
}


/*****/

/***** spi_flash.functions/spi_flash_setdr()
 * DESCRIPTION
 *  This function is used to set the ctrlr1 controller.
 *  ARGUMENTS
 *  dev         -- DMA controller device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if dr_number is out the range
 *  -DW_EINVAL  -- if byte_num argument isn't available
 * SEE ALSO
 *  dw_device
 * SOURCE
 */

uint32_t spi_flash_setdr(struct spi_flash_portmap *spi_flash_map, enum spi_flash_dr_number dr_num,
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




uint8_t flash_get_status(struct rlx_spi_slave *rlxslave)
{
	struct spi_flash_portmap *spi_flash_map = rlxslave->base_address;

	uint8_t cmd_rdsr = 0x05;

	/* Disable SPI_FLASH*/
	spi_flash_map->ssienr = 0;

	/* Set Ctrlr1; 1 byte data frames*/
	spi_flash_map->ctrlr1 = 1;

	spi_flash_map->addr_length = 0;
	/* Set tuning dummy cycles*/
	DW_BITS_SET_VAL(spi_flash_map->auto_length, bfoSPI_FLASH_AUTO_LEN_DUM,
	0, bfwSPI_FLASH_AUTO_LEN_DUM);

	flash_rx_cmd(rlxslave, cmd_rdsr);

	return spi_flash_getdr(rlxslave, DR0, DATA_BYTE);
}



void flash_wait_busy(struct rlx_spi_slave *rlxslave)
{
	/*Check flash is in write progress or not*/
	while (1) {
		if (!(flash_get_status(rlxslave) & 0x1))
			break;
	}
}

static uint8_t flash_rx_cmd(struct rlx_spi_slave *rlxslave, uint8_t cmd)
{
	struct spi_flash_portmap *spi_flash_map;
	uint32_t rd_data;
	int res = 0;


	spi_flash_map = rlxslave->base_address;
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

	/*printf("bef enable spi_flash, cmd is %x, ser is %x, ctrl0 is %x\n", cmd, spi_flash_map->ser, spi_flash_map->ctrlr0);*/
	/* Enable SPI_FLASH*/
	spi_flash_map->ssienr = 1;
	/*printf("rx cmd wait\n");*/
	res = spi_flash_wait_busy(rlxslave);

	if (res != 0) {
		printf("flash_rx_cmd:error when wait busy\n");
		return res;
	}

	/*printf("rx_cmd end\n");*/

	return res;
}


int flash_tx_cmd(struct rlx_spi_slave *rlxslave, uint8_t cmd)
{
	struct spi_flash_portmap *spi_flash_map;
	/* struct device_info inst_info;*/
	uint32_t rd_data;
	int tmp, res = 0;

	spi_flash_map = rlxslave->base_address;

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

	res = spi_flash_wait_busy(rlxslave);

	spi_flash_map->ssienr = 0;
	spi_flash_map->auto_length = tmp;

	if (res != 0) {
		printf("flash_tx_cmd:error when wait busy\n");
		return res;
	}

	return res;
}



int flash_set_status(struct spi_slave *spi, u16 data, u8 cmd, u8 data_len)
{
	struct rlx_spi_slave *rlxslave = to_rlx_spi(spi);

	struct spi_flash_portmap *spi_flash_map = rlxslave->base_address;
	uint8_t cmd_wrsr, rd_data;
	uint32_t info;
	int res = 0;
	info = spi_flash_map->addr_length;


	/* Set flash_cmd: WREN to FIFO*/
	res = flash_tx_cmd(rlxslave, 0x06);
	if (res) {
		printf("flash_set_status:flash_tx_cmd failed\n");
		return res;
	}

	/* Disable SPI_FLASH*/
	spi_flash_map->ssienr = 0;

	/* set ctrlr0: TX mode*/
	rd_data = spi_flash_map->ctrlr0;
	if (QPIMode == 1)
		spi_flash_map->ctrlr0 = (rd_data & 0xffc0fcff) | QPI_CHN_SETTING;
	else
		spi_flash_map->ctrlr0 = (rd_data & 0xffc0fcff);

	spi_flash_map->addr_length = data_len;

	/* Set flash_cmd: WRSR to FIFO*/
	cmd_wrsr = cmd;

	spi_flash_map->dr[DR0].byte = cmd_wrsr;


	spi_flash_map->dr[DR0].byte = data&0xff;
	if (data_len == 2)
		spi_flash_map->dr[DR0].byte = data>>8;

	spi_flash_map->ssienr = 1;
	spi_flash_wait_busy(rlxslave);

	spi_flash_map->ssienr = 0;
	spi_flash_map->addr_length = info;

	flash_wait_busy(rlxslave);
	return res;
}



/*only in QPI mode*/
int flash_set_read_para(struct spi_slave *spi, u16 data, u8 cmd, u8 data_len)
{
	struct rlx_spi_slave *rlxslave = to_rlx_spi(spi);

	struct spi_flash_portmap *spi_flash_map = rlxslave->base_address;
	uint8_t cmd_wrsr, rd_data;
	uint32_t info;
	int res = 0;
	info = spi_flash_map->addr_length;


	/* Set flash_cmd: WREN to FIFO*/
	res = flash_tx_cmd(rlxslave, 0x06);
	if (res) {
		printf("flash_set_status:flash_tx_cmd failed\n");
		return res;
	}

	/* Disable SPI_FLASH*/
	spi_flash_map->ssienr = 0;

	/* set ctrlr0: TX mode*/
	rd_data = spi_flash_map->ctrlr0;

	spi_flash_map->ctrlr0 = (rd_data & 0xffc0fcff) | QPI_CHN_SETTING;

	spi_flash_map->addr_length = data_len;

	/* Set flash_cmd: WRSR to FIFO*/
	cmd_wrsr = cmd;

	spi_flash_map->dr[DR0].byte = cmd_wrsr;

	spi_flash_map->dr[DR0].byte = data&0xff;

	spi_flash_map->ssienr = 1;
	spi_flash_wait_busy(rlxslave);

	spi_flash_map->ssienr = 0;
	spi_flash_map->addr_length = info;

	flash_wait_busy(rlxslave);
	return res;
}


int flash_enable_write(struct spi_slave *spi)
{
	struct rlx_spi_slave *rlxslave = to_rlx_spi(spi);
	int res = 0;
	/*   spi_flash_map = dev->base_address;*/

	res = flash_tx_cmd(rlxslave, 0x06);
	if (res) {
		printf("flash_disable_write: flash_tx_cmd wrdi failed\n");
		return res;
	}

/*	printf("wren end\n");*/
	return res;
}


int flash_disable_write(struct spi_slave *spi)
{
	struct rlx_spi_slave *rlxslave = to_rlx_spi(spi);
	int res = 0;

	res = flash_tx_cmd(rlxslave, 0x04);
	if (res) {
		printf("flash_disable_write: flash_tx_cmd wrdi failed\n");
		return res;
	}

	return res;
}

int flash_enable_qpi(struct spi_slave *spi, u8 cmd)
{
	struct rlx_spi_slave *rlxslave = to_rlx_spi(spi);
	int res = 0;

	/* Set flash_cmd: WREN to FIFO*/
	res = flash_tx_cmd(rlxslave, 0x06);
	if (res) {
		printf("flash_set_status:flash_tx_cmd failed\n");
		return res;
	}

	res = flash_tx_cmd(rlxslave, cmd);

	QPIMode = 1;
	flash_wait_busy(rlxslave);
	return res;
}

int flash_exit_qpi(struct spi_slave *spi, u8 cmd)
{
	struct rlx_spi_slave *rlxslave = to_rlx_spi(spi);
	int res = 0;

	res = flash_tx_cmd(rlxslave, cmd);
	if (res) {
		printf("flash_disable_write: flash_tx_cmd wrdi failed\n");
		return res;
	}

	QPIMode = 0;
	flash_wait_busy(rlxslave);

	return res;
}

int flash_enable_4B(struct spi_slave *spi)
{
	struct rlx_spi_slave *rlxslave = to_rlx_spi(spi);
	int res = 0;
	/*   spi_flash_map = dev->base_address;*/

	res = flash_tx_cmd(rlxslave, ENABLE_4BYTE_MODE);
	if (res) {
		printf("flash_disable_write: flash_tx_cmd wrdi failed\n");
		return res;
	}

/*	printf("wren end\n");*/
	return res;
}


int flash_exit_4B(struct spi_slave *spi)
{
	struct rlx_spi_slave *rlxslave = to_rlx_spi(spi);
	int res = 0;

	res = flash_tx_cmd(rlxslave, EXIT_4BYTE_MODE);
	if (res) {
		printf("flash_disable_write: flash_tx_cmd wrdi failed\n");
		return res;
	}

	return res;
}

int spi_flash_set_AutoRead_AddrLen(struct spi_slave *spi, u8 addr_len)
{
	struct spi_flash_portmap *spi_flash_map;
	struct rlx_spi_slave *rlxslave = to_rlx_spi(spi);
	spi_flash_map = rlxslave->base_address;

	spi_flash_map->ssienr = 0;
	spi_flash_map = rlxslave->base_address;
	if (addr_len == 4)
		DW_BITS_SET_VAL(spi_flash_map->auto_length, bfoSPI_FLASH_AUTO_LEN_ADDR,
		0, bfwSPI_FLASH_AUTO_LEN_ADDR);
	else
		DW_BITS_SET_VAL(spi_flash_map->auto_length, bfoSPI_FLASH_AUTO_LEN_ADDR,
		3, bfwSPI_FLASH_AUTO_LEN_ADDR);

	return 0;
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


uint32_t rlx_flash_read(struct rlx_spi_slave *rlxslave, const u8 *cmd, size_t cmd_len, u8 *data_in, u8 data_len)
{
	struct spi_flash_portmap *spi_flash_map = rlxslave->base_address;

	uint32_t ctrlr0, rd_data = 0;
	int i, j, res = 0;

	/* set ctrlr0: RX_mode*/
	ctrlr0 = spi_flash_map->ctrlr0;
	if (QPIMode == 1)
		spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300
			| QPI_CHN_SETTING;
	/* SPI mode */
	else if (rlxslave->slave.autoread_type & RD_QUAD_IO_T)
		spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300
			| CMD_1CHN | ADDR_4CHN | DATA_4CHN;
	else if (rlxslave->slave.autoread_type & RD_QUAD_O_T)
		spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300
			| CMD_1CHN | ADDR_1CHN | DATA_4CHN;
	else if (rlxslave->slave.autoread_type & RD_DUAL_IO_T)
		spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300
			| CMD_1CHN | ADDR_2CHN | DATA_2CHN;
	else if (rlxslave->slave.autoread_type & RD_DUAL_O_T)
		spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300
			| CMD_1CHN | ADDR_1CHN | DATA_2CHN;
	else
		spi_flash_map->ctrlr0 = (ctrlr0 & 0xffc0fcff) | 0x00000300;

	spi_flash_map->addr_length = (cmd_len - 1)%4;

	if (cmd[0] == 0x9f)
		spi_flash_set_dummy_cycle(spi_flash_map, 0);
	else
		spi_flash_set_dummy_cycle(spi_flash_map, rlxslave->slave.dummy_cycle);
	spi_flash_setctrlr1(rlxslave, data_len);


	/* Write cmd, addr, data into FIFO*/
	spi_flash_setdr(spi_flash_map, DR0, cmd[0], DATA_BYTE);
	for (i = 1; i <= (cmd_len - 1); i++) {
		spi_flash_setdr(spi_flash_map, DR0, cmd[i], DATA_BYTE);
	}

	/* Enable SPI_FLASH*/
	spi_flash_map->ssienr = 1;

	res = spi_flash_wait_busy(rlxslave);
	if (res) {
		printf("flash_read: spi_flash_wait_busy failed\n");
		return res;
	}
	/* Disble SPI_FLASH*/
	spi_flash_map->ssienr = 0;

	for (i = 0; i < (data_len/4); i++) {
		rd_data = spi_flash_getdr(rlxslave, i, DATA_WORD);
		for (j = 0; j < 4; j++) {
			*data_in = ((rd_data >> (j*8))&0xff);
			data_in++;
		}
	}

	for (i = 0; i < (data_len%4); i++) {
		rd_data = spi_flash_getdr(rlxslave, i, DATA_BYTE);
		*data_in = rd_data&0xff;
		data_in++;
	}

	return res;
}



int rlx_flash_write(struct rlx_spi_slave *rlxslave, const u8 *cmd, size_t cmd_len, const u8 *data_out, u8 data_len)
{
	struct spi_flash_portmap *spi_flash_map = rlxslave->base_address;

	int res = 0;
	int i, j;
	u32 tmp, tmp_len;
	u32 init_data;

	/*set ctrlr0: TX mode*/
	init_data = spi_flash_map->ctrlr0;
	tmp_len = spi_flash_map->auto_length;

	if (cmd[0] == 0x01)
		spi_flash_map->addr_length = 1;
	else
		spi_flash_map->addr_length = (cmd_len - 1)%4;

	/* Write cmd, addr, data into FIFO*/
	spi_flash_setdr(spi_flash_map, DR0, cmd[0], DATA_BYTE);
/*	printf("write %x ", cmd[0]);*/
	for (i = 1; i < cmd_len; i++) {
/*		printf("%x ", cmd[i]);*/
		spi_flash_setdr(spi_flash_map, DR0, cmd[i], DATA_BYTE);
	}
/*	printf("data len is %x, addr len is %x\n", data_len, spi_flash_map->addr_length);*/

	if (data_out != NULL) {
		switch (rlxslave->slave.userwrite_type) {
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
	} else {
			spi_flash_map->ctrlr0 = (init_data & 0xffc0fcff);
	}

	if (QPIMode == 1)
		spi_flash_map->ctrlr0 |= QPI_CHN_SETTING;

	/*printf("write type %x, %x, %x\n", rlxslave->slave.userwrite_type, cmd[0], spi_flash_map->ctrlr0);*/

	/* Enable SPI_FLASH*/
	spi_flash_map->ssienr = 1;

/*	printf("after enable flash\n");*/

#ifdef INTERRUPT_TEST_SPIC
	/* if write data to flash, send much more to fifo overflow */
	if (data_len > 0x7f) {
		int i = 100;
		*((volatile unsigned int *)0xb803002c) = 0x3f;
		while (i--)
			spi_flash_setdr(spi_flash_map, DR0, 1, DATA_WORD);
		/* Open txfir interrupt */
		while (1)
			;
	}
#endif
	res = spi_flash_wait_busy(rlxslave);

	if (res) {
		printf("flash_write: spi_flash_wait_busy failed\n");
		return res;
	}

	flash_wait_busy(rlxslave);
	spi_flash_map->ssienr = 0;
	spi_flash_map->auto_length = tmp_len;
	return res;
}


int spi_flash_read_write(struct spi_slave *spi,
				const u8 *cmd, size_t cmd_len,
				const u8 *data_out, u8 *data_in,
				size_t data_len)
{
	struct spi_flash_portmap *spi_flash_map;
	struct rlx_spi_slave *rlxslave = to_rlx_spi(spi);
	int i, len_tmp, tmp, ret = 0;

	spi_flash_map = rlxslave->base_address;
	/* Disable SPI_FLASH*/
	spi_flash_map->ssienr = 0;


	if (data_in == NULL) {
		/*write command*/
		/* Disble SPI_FLASH*/
		spi_flash_map->ssienr = 0;

		ret = rlx_flash_write(rlxslave, cmd, cmd_len, data_out, data_len);
	} else if (data_out == NULL) {
		/*read command*/
		/*disable SPI_FLASH*/
		spi_flash_map->ssienr = 0;

		tmp = spi_flash_map->auto_length;
		DW_BITS_SET_VAL(spi_flash_map->auto_length, bfoSPI_FLASH_AUTO_LEN_DUM,
				0, bfwSPI_FLASH_AUTO_LEN_DUM);

		if (cmd_len == 1) {
			/*no address read command*/
			if (*cmd == 0x9f)	/*read  id command*/ {
				len_tmp = 3;
			} else {
				len_tmp = data_len;
			}
			spi_flash_setctrlr1(rlxslave, len_tmp);
			flash_rx_cmd(rlxslave, *cmd);
			for (i = 0; i < len_tmp; i++) {
				*data_in = spi_flash_getdr(rlxslave, DR0, DATA_BYTE);
				data_in++;
			}
		} else {
			ret = rlx_flash_read(rlxslave, cmd, cmd_len, data_in, data_len);
		}
		spi_flash_map->ssienr = 0;
		spi_flash_map->auto_length = tmp;
	}

	return ret;
}

int spi_flash_set_auto(struct spi_slave *spi)
{
	struct spi_flash_portmap *spi_flash_map;
	struct rlx_spi_slave *rlxslave = to_rlx_spi(spi);

	spi_flash_map = rlxslave->base_address;

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

	if (spi->autoread_type & RD_QUAD_IO_T) {
		spi_flash_map->rd_quad_io = spi->read_cmd;
	} else if (spi->autoread_type & RD_QUAD_O_T) {
		spi_flash_map->rd_quad_o = spi->read_cmd;
	} else if (spi->autoread_type & RD_DUAL_IO_T) {
		spi_flash_map->rd_dual_io = spi->read_cmd;
	} else if (spi->autoread_type & RD_DUAL_O_T) {
		spi_flash_map->rd_dual_o = spi->read_cmd;
	} else if (spi->autoread_type & FRD_SINGLE_T) {
		spi_flash_map->rd_fast_single  = spi->read_cmd;
	}

	spi_flash_set_dummy_cycle(spi_flash_map, spi->dummy_cycle);

	spi_flash_map->valid_cmd = (spi->autoread_type | 0x200);

/*	printf("set auto command, %x,  %x, %x\n", spi_flash_map->auto_length,  spi->autoread_type, spi_flash_map->valid_cmd);*/
	return 0;
}



int spi_flash_set_user(struct spi_slave *spi)
{
	struct spi_flash_portmap *spi_flash_map;
	struct rlx_spi_slave *rlxslave = to_rlx_spi(spi);
	spi_flash_map = rlxslave->base_address;

	spi_flash_map->ssienr = 0;
	spi_flash_map = rlxslave->base_address;
	spi_flash_map->valid_cmd = 0x200;

	return 0;
}
