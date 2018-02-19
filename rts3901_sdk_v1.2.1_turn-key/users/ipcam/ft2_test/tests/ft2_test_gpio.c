/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_uart1.c
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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FT2_GPIO_NUM 5

static int gpioexport(uint32_t iostart, uint32_t ioend)
{
	int32_t fd = -1;
	int32_t i, ret;
	uint8_t sgi[] = "1";

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (fd < 0) {
		ret = -1;
		FT2_LOG_ERR("open fail!\n");
		goto failed;
	}

	for (i = iostart; i <= ioend; i++) {
		ret = write(fd, sgi, 2);
		if (ret <= 0) {
			FT2_LOG_ERR("write %d fail!\n", i);
			goto failed;
		}
		sgi[0]++;
	}

	close(fd);
	return 0;

failed:
	if (fd >= 0)
		close(fd);
	return -1;
}

static int gpiodeexport(uint32_t iostart, uint32_t ioend)
{
	int32_t fd = -1;
	int32_t i, ret;
	uint8_t sgi[] = "1";

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (fd < 0) {
		FT2_LOG_ERR("open fail!\n");
		goto failed;
	}

	for (i = iostart; i <= ioend; i++) {
		ret = write(fd, sgi, 1);
		if (ret <= 0) {
			FT2_LOG_ERR("write %d fail!\n", i);
			goto failed;
		}
		sgi[0]++;
	}

	close(fd);
	return 0;

failed:
	if (fd >= 0)
		close(fd);
	return -1;
}

static int gpiooutandvalue(uint8_t gostart, uint8_t gostop, uint8_t value)
{
	uint8_t sgd[] = "/sys/class/gpio/gpio1/direction";
	uint8_t sgv[] = "/sys/class/gpio/gpio1/value";
	int32_t fd = -1;
	int32_t i, len, ret;

	len = strlen("/sys/class/gpio/gpio1");

	sgd[len - 1] = gostart + 48;
	for (i = gostart; i <= gostop; i++) {
		fd = open(sgd, O_WRONLY);
		if (fd < 0) {
			FT2_LOG_ERR("open %s fail!\n", sgd);
			goto failed;
		}

		ret = write(fd, "out", 4);
		if (ret <= 0) {
			FT2_LOG_ERR("write %s fail!\n", sgd);
			goto failed;
		}

		close(fd);
		fd = -1;
		sgd[len - 1]++;
	}

	sgv[len - 1] = gostart + 48;
	for (i = gostart; i <= gostop; i++) {
		fd = open(sgv, O_WRONLY);
		if (fd < 0) {
			FT2_LOG_ERR("open %s fail!\n", sgv);
			goto failed;
		}
		if (value)
			ret = write(fd, "1", 2);
		else
			ret = write(fd, "0", 2);

		if (ret <= 0) {
			FT2_LOG_ERR("write %s fail!\n", sgv);
			goto failed;
		}

		close(fd);
		fd = 0;
		sgv[len - 1]++;
	}

	return 0;

failed:
	if (fd >= 0)
		close(fd);
	return -1;

}

static int gpioinput(uint8_t gistart, uint8_t gistop)
{
	uint8_t sgd[] = "/sys/class/gpio/gpio1/direction";
	int32_t fd = -1;
	int32_t i, len, ret;

	len = strlen("/sys/class/gpio/gpio1");

	sgd[len - 1] = gistart  + '0';
	for (i = gistart; i <= gistop; i++) {
		fd = open(sgd, O_WRONLY);
		if (fd < 0) {
			FT2_LOG_ERR("open %s fail!\n", sgd);
			goto failed;
		}

		ret = write(fd, "in", 3);
		if (ret <= 0) {
			FT2_LOG_ERR("write %s fail!\n", sgd);
			goto failed;
		}

		close(fd);
		fd = -1;
		sgd[len - 1]++;
	}

	return 0;

failed:
	if (fd >= 0)
		close(fd);
	return -1;
}

static int gpioinputvalue(uint8_t gistart, uint8_t gistop, uint8_t *value)
{
	uint8_t sgv[] = "/sys/class/gpio/gpio1/value";
	int32_t fd = -1;
	int32_t i, len, ret;
	uint8_t val;

	len = strlen("/sys/class/gpio/gpio1");

	sgv[len - 1] = gistart + '0';
	for (i = gistart; i <= gistop; i++) {
		fd = open(sgv, O_RDONLY);
		if (fd < 0) {
			FT2_LOG_ERR("open %s fail!\n", sgv);
			goto failed;
		}

		ret = read(fd, &val, 1);
		if (ret <= 0) {
			FT2_LOG_ERR("read %s fail!\n", sgv);
			goto failed;
		}

		ret = strncmp(&val, value, 1);
		if (ret) {
			FT2_LOG_ERR("compare %s fail!\n", sgv);
			goto failed;
		}

		close(fd);
		fd = -1;
		sgv[len - 1]++;
	}

	return 0;

failed:
	if (fd >= 0)
		close(fd);
	return -1;
}

int ft2_gpio_check(struct protocol_command *pcmd)
{
	if (!pcmd)
		return -FT2_NULL_POINT;

	return FT2_OK;
}

int ft2_gpio_runonce(void *priv, struct protocol_command *pcmd, char *path)
{
	int ret;
	int i, j;

	if (!pcmd)
		return -FT2_NULL_POINT;

	ret = gpioexport(1, FT2_GPIO_NUM);
	if (ret < 0) {
		FT2_LOG_ERR("gpio export fail!\n");
		goto failed;
	}

	for (i = 1;i < FT2_GPIO_NUM;i++) {
		/* set all gpio in */
		ret = gpioinput(1, FT2_GPIO_NUM);
		if (ret < 0) {
			FT2_LOG_ERR("gpioinput(1, %d) fail!\n", FT2_GPIO_NUM);
			goto failed;
		}

		/* gpio i out '1', then check other gpio in value */
		ret = gpiooutandvalue(i, i, 1);
		if (ret < 0) {
			FT2_LOG_ERR("gpiooutandvalue %d fail\n", i);
			return -1;
		}

		for (j = 1;j < FT2_GPIO_NUM;j++) {
			if (j == i)
				continue;

			ret = gpioinputvalue(j, j, "1");
			if (ret < 0) {
				FT2_LOG_ERR("gpioinputvalue %d fail\n", j);
				return -1;
			}
		}

		/* gpio i out '0', then check other gpio in value */
		ret = gpiooutandvalue(i, i, 0);
                if (ret < 0) {
			FT2_LOG_ERR("gpiooutandvalue %d fail\n", i);
			return -1;
		}

                for (j = 1;j < FT2_GPIO_NUM;j++) {
			if (j == i)
				continue;

			ret = gpioinputvalue(j, j, "0");
			if (ret < 0) {
				FT2_LOG_ERR("gpioinputvalue %d fail\n", j);
				return -1;
			}
                }
        }

	ret = gpiodeexport(1, FT2_GPIO_NUM);
	if (ret < 0) {
		FT2_LOG_ERR("gpio deexport fail!\n");
		goto failed;
	}

	FT2_LOG_INFO("gpio test ok!\n");
	append_pt_command_result(pcmd, FT2_TEST_PASS, strlen(FT2_TEST_PASS));
	return FT2_OK;

failed:
	append_pt_command_result(pcmd, FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
	return -FT2_ERROR;
}
