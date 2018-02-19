/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_fake.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include "protocol.h"
#include "ft2errno.h"
#include "ft2log.h"

#define DEFAULT_I2C_BUS		"/dev/i2c-0"
#define DEFAULT_EEPROM_ADDR	0x50
#define BYTES_PER_PAGE		32
#define MAX_BYTES		BYTES_PER_PAGE
#define PAGES_NUM		(TOTAL_BYTES / BYTES_PER_PAGE)
#define TOTAL_BYTES		8192	/* GT24C64 has 64k bits */

struct i2c_msg {
	__u16 addr;
	__u16 flags;
#define I2C_M_TEN	0x10
#define I2C_M_RD	0x01
#define I2C_M_NOSTART	0x4000
#define I2C_M_REV_DIR_ADDR	0x2000
#define I2C_M_IGNORE_NAK	0x1000
#define I2C_M_NO_RD_ACK		0x0800
	__s16 len;
	int8_t *buf;
};

static int32_t eeprom_write(int32_t fd,
			    uint32_t addr,
			    uint32_t offset, uint8_t *buf, uint8_t len)
{
	struct i2c_rdwr_ioctl_data msg_rdwr;
	struct i2c_msg i2cmsg;
	int32_t ret, i;
	char _buf[MAX_BYTES + 2];

	if (len > MAX_BYTES) {
		FT2_LOG_ERR("I can only write MAX_BYTES bytes at a time!\n");
		return -1;
	}

	if (len + offset > TOTAL_BYTES) {
		FT2_LOG_ERR("Sorry, len(%d)+offset(%d) > (page boundary)\n",
			    len, offset);
		return -1;
	}

	_buf[0] = (offset & 0xff00) >> 8;
	_buf[1] = offset & 0xff;

	for (i = 0; i < len; i++)
		_buf[2 + i] = buf[i];

	msg_rdwr.msgs = &i2cmsg;
	msg_rdwr.nmsgs = 1;

	i2cmsg.addr = addr;
	i2cmsg.flags = 0;
	i2cmsg.len = 2 + len;
	i2cmsg.buf = _buf;

	ret = ioctl(fd, I2C_RDWR, &msg_rdwr);
	if (ret < 0) {
		FT2_LOG_INFO("ioctl returned %d\n", ret);
		return -1;
	}

	if (len > 0)
		FT2_LOG_INFO
		    ("Wrote %d bytes to eeprom at 0x%02x, offset %08x\n", len,
		     addr, offset);
	return 0;
}

static int32_t eeprom_read(int32_t fd,
			   uint32_t addr,
			   uint32_t offset, uint8_t *buf, uint8_t len)
{
	struct i2c_rdwr_ioctl_data msg_rdwr;
	struct i2c_msg i2cmsg;
	int32_t ret;

	if (eeprom_write(fd, addr, offset, NULL, 0) < 0)
		return -1;

	msg_rdwr.msgs = &i2cmsg;
	msg_rdwr.nmsgs = 1;

	i2cmsg.addr = addr;
	i2cmsg.flags = I2C_M_RD;
	i2cmsg.len = len;
	i2cmsg.buf = buf;

	ret = ioctl(fd, I2C_RDWR, &msg_rdwr);
	if (ret < 0)
		return -1;

	FT2_LOG_INFO("Read %d bytes from eeprom at 0x%02x, offset %08x\n",
		     len, addr, offset);

	return 0;
}

int32_t ft2_i2c_check(struct protocol_command *pcmd)
{
	if (!pcmd)
		return -FT2_NULL_POINT;

	return FT2_OK;
}

int32_t ft2_i2c_runonce(void *priv, struct protocol_command *pcmd,
			int8_t *path)
{
	int32_t i;
	uint8_t buf[TOTAL_BYTES];
	int32_t fd;

	if (!pcmd)
		return -FT2_NULL_POINT;

	fd = open(DEFAULT_I2C_BUS, O_RDWR);
	if (fd < 0) {
		FT2_LOG_ERR("Could not open i2c at %s\n", DEFAULT_I2C_BUS);
		goto failed;
	}

	memset(buf, 0, TOTAL_BYTES);
	for (i = 0; i < PAGES_NUM; i++)
		if (eeprom_read
			(fd, DEFAULT_EEPROM_ADDR,
			i * BYTES_PER_PAGE, buf + (i * BYTES_PER_PAGE),
			BYTES_PER_PAGE) < 0) {
			FT2_LOG_ERR("read addr:0x%x failed\n",
				    i * BYTES_PER_PAGE);
			goto failed;
		}

	for (i = 0; i < TOTAL_BYTES; i += 2) {
		if ((buf[i] != 0x55) && buf[i + 1] != 0xaa) {
			FT2_LOG_ERR("data that returned is error\n");
			goto failed;
		}
	}

	close(fd);
	FT2_LOG_INFO("i2c test ok!\n");
	append_pt_command_result(pcmd, FT2_TEST_PASS, strlen(FT2_TEST_PASS));
	return FT2_OK;

failed:
	append_pt_command_result(pcmd, FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
	return -FT2_ERROR;
}
