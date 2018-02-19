/*
 * SPI flash internal definitions
 *
 * Copyright (C) 2008 Atmel Corporation
 * Copyright (C) 2013 Jagannadha Sutradharudu Teki, Xilinx Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _SF_INTERNAL_H_
#define _SF_INTERNAL_H_

#define SPI_FLASH_16MB_BOUN		0x1000000

/* SECT flags */
#define SECT_4K				(1 << 1)
#define SECT_32K			(1 << 2)
#define E_FSR				(1 << 3)
#define QPI_I				(1 << 4)
#define QPI_II				(1 << 5)
#define SR_CFG				(1 << 6)
#define SR_3REG				(1 << 7)
#define SR_3REG1			(1 << 8)

/* Erase commands */
#define CMD_ERASE_4K			0x20
#define CMD_ERASE_32K			0x52
#define CMD_ERASE_CHIP			0xc7
#define CMD_ERASE_64K			0xd8

/* Write commands */
#define CMD_WRITE_STATUS_1	0x01
#define CMD_WRITE_STATUS_2	0x31
#define CMD_WRITE_STATUS_3	0x11
#define CMD_PAGE_PROGRAM		0x02
#define CMD_WRITE_DISABLE		0x04
#define CMD_READ_STATUS_1		0x05
#define CMD_READ_STATUS_2		0x35
#define CMD_READ_STATUS_3		0x15
#define CMD_READ_CONFIG		0x15
#define CMD_ENTER_QPI_I		0x35
#define CMD_EXIT_QPI_I			0xF5
#define CMD_ENTER_QPI_II		0x38
#define CMD_EXIT_QPI_II			0xFF
#define CMD_WRITE_ENABLE		0x06
#define CMD_FLAG_STATUS			0x70
#define CMD_EN4B				0xb7	/*enable 4byte mode*/
#define CMD_EX4B				0xe9	/*exit 4byte mode*/

/* Read commands */
#define CMD_READ_ARRAY_SLOW		0x03
#define CMD_READ_ARRAY_FAST		0x0b
#define CMD_READ_ID			0x9f

#define WR_QUAD_II				0x04
#define WR_QUAD_I				0x03
#define WR_DUAL_II				0x02
#define WR_DUAL_I				0x01
#define WR_MULTI_NONE			0x00


#define RD_QUAD_IO				0x10
#define RD_QUAD_O				0x08
#define RD_DUAL_IO				0x04
#define RD_DUAL_O				0x02
#define FRD_SINGLE				0x01
#define FRD_NONE				0x0

#define SPI_AUTO_MODE			0x00
#define SPI_USER_MODE			0x01

/* Bank addr access commands */
#ifdef CONFIG_SPI_FLASH_BAR
# define CMD_BANKADDR_BRWR		0x17
# define CMD_BANKADDR_BRRD		0x16
# define CMD_EXTNADDR_WREAR		0xC5
# define CMD_EXTNADDR_RDEAR		0xC8
#endif

/* Common status */
#define STATUS_WIP			0x01
#define STATUS_PEC			0x80

/* Flash timeout values */
#define SPI_FLASH_PROG_TIMEOUT		(2 * CONFIG_SYS_HZ)
#define SPI_FLASH_PAGE_ERASE_TIMEOUT	(5 * CONFIG_SYS_HZ)
#define SPI_FLASH_SECTOR_ERASE_TIMEOUT	(10 * CONFIG_SYS_HZ)

/* SST specific */
#ifdef CONFIG_SPI_FLASH_SST
# define SST_WP			0x01	/* Supports AAI word program */
# define CMD_SST_BP		0x02    /* Byte Program */
# define CMD_SST_AAI_WP		0xAD	/* Auto Address Incr Word Program */

int sst_write_wp(struct spi_flash *flash, u32 offset, size_t len,
		const void *buf);
#endif

/* Send a single-byte command to the device and read the response */
int spi_flash_cmd(struct spi_slave *spi, u8 cmd, void *response, size_t len);

int spi_flash_write_enable(struct spi_slave *spi);

int spi_flash_write_disable(struct spi_slave *spi);

int spi_flash_set_status(struct spi_slave *spi, u16 data, u8 cmd, u8 data_len);

/*
 * Send a multi-byte command to the device and read the response. Used
 * for flash array reads, etc.
 */
int spi_flash_cmd_read(struct spi_slave *spi, const u8 *cmd,
		size_t cmd_len, void *data, size_t data_len);

/*
 * Send a multi-byte command to the device followed by (optional)
 * data. Used for programming the flash array, etc.
 */
int spi_flash_cmd_write(struct spi_slave *spi, const u8 *cmd, size_t cmd_len,
		const void *data, size_t data_len);


/* Flash erase(sectors) operation, support all possible erase commands */
int spi_flash_cmd_erase_ops(struct spi_flash *flash, u32 offset, size_t len);

/* Program the status register */
static inline int spi_flash_cmd_write_status(struct spi_flash *flash, u16 sr, u8 cmd, u8 data_len)
{
	return spi_flash_set_status(flash->spi, sr, cmd, data_len);
}

int spi_flash_cmd_read_status(struct spi_flash *flash, u8 *sr, u8 cmd);

/* Set quad enbale bit */
int spi_flash_set_qeb(struct spi_flash *flash);

/* Enable writing on the SPI flash */
static inline int spi_flash_cmd_write_enable(struct spi_flash *flash)
{
/*	return spi_flash_cmd(flash->spi, CMD_WRITE_ENABLE, NULL, 0);*/
	return spi_flash_write_enable(flash->spi);
}

/* Disable writing on the SPI flash */
static inline int spi_flash_cmd_write_disable(struct spi_flash *flash)
{
/*	return spi_flash_cmd(flash->spi, CMD_WRITE_DISABLE, NULL, 0);*/
	return spi_flash_write_disable(flash->spi);
}

/*
 * Send the read status command to the device and wait for the wip
 * (write-in-progress) bit to clear itself.
 */
int spi_flash_cmd_wait_ready(struct spi_flash *flash, unsigned long timeout);

/*
 * Used for spi_flash write operation
 * - SPI claim
 * - spi_flash_cmd_write_enable
 * - spi_flash_cmd_write
 * - spi_flash_cmd_wait_ready
 * - SPI release
 */
int spi_flash_write_common(struct spi_flash *flash, const u8 *cmd,
		size_t cmd_len, const void *buf, size_t buf_len);

/*
 * Flash write operation, support all possible write commands.
 * Write the requested data out breaking it up into multiple write
 * commands as needed per the write size.
 */
int spi_flash_cmd_write_ops(struct spi_flash *flash, u32 offset,
		size_t len, const void *buf);

/*
 * Same as spi_flash_cmd_read() except it also claims/releases the SPI
 * bus. Used as common part of the ->read() operation.
 */
int spi_flash_read_common(struct spi_flash *flash, const u8 *cmd,
		size_t cmd_len, void *data, size_t data_len);

/* Flash read operation, support all possible read commands */
int spi_flash_cmd_read_ops(struct spi_flash *flash, u32 offset,
		size_t len, void *data);

int spi_flash_enable_QPI_ops(struct spi_flash *flash);

int spi_flash_exit_QPI_ops(struct spi_flash *flash);
unsigned int  spi_flash_read_word_ops(struct spi_flash *flash, unsigned int offset);


#endif /* _SF_INTERNAL_H_ */