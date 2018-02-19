/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_disk.c
 *
 * Copyright (C) 2015      Lei Wang<lei_wang@realsil.com.cn>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "protocol.h"
#include "ft2errno.h"
#include "ft2log.h"


#define BUF_SIZE		1024
#define disk_input		"/mnt/disk/disk.input"
#define disk_output		"/media/sdcard/disk.output"

static int file_copy(const char *src, const char *dst)
{
	char buf[BUF_SIZE];
	int s, d, n;
	int err = 0;

	s = open(src, O_RDONLY);
	if (s < 0) {
		FT2_LOG_ERR("open %s failed\n", src);
		return s;
	}

	d = open(dst, O_WRONLY | O_CREAT);
	if (d < 0) {
		FT2_LOG_ERR("open %s failed\n", dst);

		/* retry open */
		sleep(10);
		d = open(dst, O_WRONLY | O_CREAT);
		if (d < 0) {
			FT2_LOG_ERR("retry open %s failed\n", dst);
			close(s);
			return d;
		}
	}

	while ((n = read(s, buf, BUF_SIZE)) > 0) {
		int x;

		x = write(d, buf, n);
		if (x < n) {
			FT2_LOG_ERR("no space to write\n");
			err = -ENOMEM;
			goto finish;
		}
	}

finish:
	close(d);
	close(s);
	return err;
}

static int file_compare(const char *src, const char *dst)
{
	char buf1[BUF_SIZE];
	char buf2[BUF_SIZE];
	int s, d, n;
	int err = 0;

	s = open(src, O_RDONLY);
	if (s < 0) {
		FT2_LOG_ERR("open %s failed\n", src);
		return s;
	}

	d = open(dst, O_RDONLY);
	if (d < 0) {
		FT2_LOG_ERR("open %s failed\n", dst);
		close(s);
		return d;
	}

	while ((n = read(s, buf1, BUF_SIZE)) > 0) {
		int x;

		x = read(d, buf2, BUF_SIZE);
		if (x != n) {
			FT2_LOG_ERR("file length not match %d -> %d\n", n, x);
			err = -EIO;
			goto finish;
		}

		err = memcmp(buf1, buf2, n);
		if (err) {
			FT2_LOG_ERR("memcmp failed\n");
			err = -EIO;
			goto finish;
		}
	}

finish:
	close(d);
	close(s);
	return err;
}

static int ft2_disk_test()
{
	int ret;

	ret = file_copy(disk_input, disk_output);
	if (ret) {
		FT2_LOG_ERR("copy failed: %d\n");
		return -FT2_ERROR;
	}

	ret = file_compare(disk_input, disk_output);
	if (ret) {
		FT2_LOG_ERR("compare failed\n");
		return -FT2_ERROR;
	}

	return FT2_OK;
}

int ft2_disk_check(struct protocol_command *pcmd)
{
	if (!pcmd)
		return -FT2_NULL_POINT;

	return FT2_OK;
}

int ft2_disk_runonce(void *priv, struct protocol_command *pcmd, char *path)
{
	int err;

	err = ft2_disk_test();
	if (err) {
		append_pt_command_result(pcmd,
			FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
		return -FT2_ERROR;
	}

	FT2_LOG_INFO("disk test ok\n");
	append_pt_command_result(pcmd,
		FT2_TEST_PASS, strlen(FT2_TEST_PASS));
	return FT2_OK;
}

