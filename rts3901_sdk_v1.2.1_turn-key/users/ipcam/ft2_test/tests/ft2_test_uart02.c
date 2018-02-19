/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_uart02.c
 *
 * Copyright (C) 2014      Peter Sun<peter_sun@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "protocol.h"
#include "ft2errno.h"
#include "ft2log.h"
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#define SERIAL_DATA_LEN 16

#define DEVICE0 "/dev/ttyS0"
#define DEVICE2 "/dev/ttyS2"

int32_t serial_fd0;
int32_t serial_fd2;

static int32_t init_serial(void)
{
	struct termios options;

	serial_fd0 = open(DEVICE0, O_RDWR | O_NOCTTY | O_NDELAY);
	if (serial_fd0 < 0)
		return -1;

	tcgetattr(serial_fd0, &options);

	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~CSIZE;
	options.c_cflag &= ~CRTSCTS;
	options.c_cflag |= CS8;
	options.c_cflag &= ~CSTOPB;
	options.c_iflag |= IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;

	cfsetispeed(&options, B57600);
	cfsetospeed(&options, B57600);
	tcflush(serial_fd0, TCIFLUSH);
	tcsetattr(serial_fd0, TCSANOW, &options);

	serial_fd2 = open(DEVICE2, O_RDWR | O_NOCTTY | O_NDELAY);
	if (serial_fd2 < 0)
		return -1;

	tcgetattr(serial_fd2, &options);

	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~CSIZE;
	options.c_cflag &= ~CRTSCTS;
	options.c_cflag |= CS8;
	options.c_cflag &= ~CSTOPB;
	options.c_iflag |= IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;

	cfsetispeed(&options, B57600);
	cfsetospeed(&options, B57600);
	tcflush(serial_fd2, TCIFLUSH);
	tcsetattr(serial_fd2, TCSANOW, &options);

	return 0;
}

static int32_t deinit_serial(void)
{
	if (serial_fd0)
		close(serial_fd0);
	if (serial_fd2)
		close(serial_fd2);
}

static int32_t uart_send(int32_t fd, uint8_t *data, int32_t datalen)
{
	int32_t len = 0;
	len = write(fd, data, datalen);
	if (len == datalen) {
		return len;
	} else {
		tcflush(fd, TCOFLUSH);
		return -1;
	}

	return 0;
}

static int32_t uart_recv(int32_t fd, uint8_t *data, int32_t datalen)
{
	int32_t len = 0, ret = 0, i;
	fd_set fs_read;
	struct timeval tv_timeout;

	FD_ZERO(&fs_read);
	FD_SET(fd, &fs_read);
	tv_timeout.tv_sec = 1;
	tv_timeout.tv_usec = 0;

	ret = select(fd + 1, &fs_read, NULL, NULL, &tv_timeout);
	if (ret <= 0)
		goto failed;

	if (FD_ISSET(fd, &fs_read))
		ret = read(fd, data, datalen);
	else {
		ret = -1;
		goto failed;
	}

	if (ret != datalen) {
		ret = -1;
		goto failed;
	}

	for (i = 0; i < datalen; i++)
		if (data[i] != 0x55) {
			ret = -1;
			break;
		}

failed:
	return ret;
}

static int32_t testproc()
{
	int32_t ret;
	int32_t i;
	uint8_t buf[SERIAL_DATA_LEN];

	memset(buf, 0x55, SERIAL_DATA_LEN);

	ret = init_serial();
	if (ret < 0) {
		FT2_LOG_ERR("uart0 2 init error\n");
		goto failed;
	}

	ret = uart_send(serial_fd0, buf, SERIAL_DATA_LEN);
	if (ret < 0) {
		FT2_LOG_ERR("uart0 send data error\n");
		goto failed;
	}

	memset(buf, 0, sizeof(buf));
	ret = uart_recv(serial_fd0, buf, SERIAL_DATA_LEN);
	if (ret < 0) {
		FT2_LOG_ERR("uart0 recv data error\n");
		goto failed;
	}

	ret = uart_send(serial_fd2, buf, SERIAL_DATA_LEN);
	if (ret < 0) {
		FT2_LOG_ERR("uart2 send data error\n");
		goto failed;
	}

	memset(buf, 0, sizeof(buf));
	ret = uart_recv(serial_fd2, buf, SERIAL_DATA_LEN);
	if (ret < 0) {
		FT2_LOG_ERR("uart2 recv data error\n");
		goto failed;
	}

failed:
	deinit_serial();
	return ret;
}

int32_t ft2_uart02_check(struct protocol_command *pcmd)
{
	if (!pcmd)
		return -FT2_NULL_POINT;

	return FT2_OK;
}

int32_t ft2_uart02_runonce(void *priv, struct protocol_command *pcmd,
			   uint8_t *path)
{
	struct protocol_param *parameter = NULL;
	int32_t ret;

	if (!pcmd)
		return -FT2_NULL_POINT;

	ret = testproc();
	if (ret < 0) {
		append_pt_command_result(pcmd,
					 FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
		return -FT2_ERROR;
	}

	FT2_LOG_INFO("uart test ok!\n");
	append_pt_command_result(pcmd, FT2_TEST_PASS, strlen(FT2_TEST_PASS));

	return FT2_OK;
}
