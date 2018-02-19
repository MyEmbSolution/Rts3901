/*
 * SPI flash interface
 *
 * Copyright (C) 2008 Atmel Corporation
 * Copyright (C) 2010 Reinhard Meyer, EMK Elektronik
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <spi.h>

/*
satic int spi_flash_read_write(struct spi_slave *spi,
				const u8 *cmd, size_t cmd_len,
				const u8 *data_out, u8 *data_in,
				size_t data_len)
{
	unsigned long flags = SPI_XFER_BEGIN;
	int ret;

	if (data_len == 0)
		flags |= SPI_XFER_END;

	ret = spi_xfer(spi, cmd_len * 8, cmd, NULL, flags);
	if (ret) {
		debug("SF: Failed to send command (%zu bytes): %d\n",
		      cmd_len, ret);
	} else if (data_len != 0) {
		ret = spi_xfer(spi, data_len * 8, data_out, data_in,
					SPI_XFER_END);
		if (ret)
			debug("SF: Failed to transfer %zu bytes of data: %d\n",
			      data_len, ret);
	}

	return ret;
}
*/
int spi_flash_cmd_read(struct spi_slave *spi, const u8 *cmd,
		size_t cmd_len, void *data, size_t data_len)
{
	return spi_flash_read_write(spi, cmd, cmd_len, NULL, data, data_len);
}

int spi_flash_cmd(struct spi_slave *spi, u8 cmd, void *response, size_t len)
{
	return spi_flash_cmd_read(spi, &cmd, 1, response, len);
}

int spi_flash_cmd_write(struct spi_slave *spi, const u8 *cmd, size_t cmd_len,
		const void *data, size_t data_len)
{
	return spi_flash_read_write(spi, cmd, cmd_len, data, NULL, data_len);
}

int spi_flash_write_enable(struct spi_slave *spi)
{
	return flash_enable_write(spi);
}

int spi_flash_write_disable(struct spi_slave *spi)
{
	return flash_disable_write(spi);
}

int spi_flash_enable_QPI_mode(struct spi_slave *spi, u8 cmd)
{
	return flash_enable_qpi(spi, cmd);
}

int spi_flash_exit_QPI_mode(struct spi_slave *spi, u8 cmd)
{
	return flash_exit_qpi(spi, cmd);
}

int spi_flash_enable_4B_mode(struct spi_slave *spi)
{
	return flash_enable_4B(spi);
}

int spi_flash_exit_4B(struct spi_slave *spi)
{
	return flash_exit_4B(spi);
}

int spi_flash_set_auto_mode(struct spi_slave *spi)
{
	return spi_flash_set_auto(spi);
}

int spi_flash_set_user_mode(struct spi_slave *spi)
{
	return spi_flash_set_user(spi);
}

int spi_flash_set_3B_AutoRead(struct spi_slave *spi)
{
	return spi_flash_set_AutoRead_AddrLen(spi, 3);
}

int spi_flash_set_4B_AutoRead(struct spi_slave *spi)
{
	return spi_flash_set_AutoRead_AddrLen(spi, 4);
}

int spi_flash_set_status(struct spi_slave *spi, u16 data, u8 cmd, u8 data_len)
{
	return flash_set_status(spi, data, cmd, data_len);
}


int spi_flash_set_read_para(struct spi_slave *spi, u16 data, u8 cmd, u8 data_len)
{
	return flash_set_read_para(spi, data, cmd, data_len);
}

