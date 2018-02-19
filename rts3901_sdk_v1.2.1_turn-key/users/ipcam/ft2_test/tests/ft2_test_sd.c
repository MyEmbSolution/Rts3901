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
#define card_exist		"/sys/kernel/debug/mmc0/sd-platform/card_exist"
#define support_uhs		"/sys/kernel/debug/mmc0/sd-platform/support_uhs"
#define sd20_input		"/mnt/sdc/sd20.input"
#define sd30_input		"/mnt/sdc/sd30.input"
#define sd20_output		"/media/sdcard/sd20.output"
#define sd30_output		"/media/sdcard/sd30.output"

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

static int write_sysfs(const char *sys, const char *buf, int len)
{
	int fd, n;
	int err = 0;

	fd = open(sys, O_WRONLY);
	if (fd < 0) {
		FT2_LOG_ERR("open %s failed\n", sys);
		return fd;
	}

	n = write(fd, buf, len);
	if (n < len) {
		FT2_LOG_ERR("write %s failed\n", sys);
		err = -EIO;
		goto finish;
	}

finish:
	close(fd);
	return err;
}

static int switch_normal_mode(void)
{
	int err = 0;

	err |= write_sysfs(card_exist, "0", 1);
	sleep(1);
	err |= write_sysfs(support_uhs, "0", 1);
	err |= write_sysfs(card_exist, "1", 1);
	sleep(10);

	if (err)
		FT2_LOG_ERR("switch to normal mode failed\n");
	return err;
}

static int switch_uhs_mode(void)
{
	int err = 0;

	err |= write_sysfs(card_exist, "0", 1);
	sleep(1);
	err |= write_sysfs(support_uhs, "1", 1);
	err |= write_sysfs(card_exist, "1", 1);
	sleep(10);

	if (err)
		FT2_LOG_ERR("switch to UHS mode failed\n");
	return err;
}

static int test_switch_mode(int uhs, const char *src, const char *dst)
{
	int err;

	FT2_LOG_INFO("switch to %s mode\n", uhs ? "SD30" : "SD20");

	if (uhs)
		switch_uhs_mode();
	else
		switch_normal_mode();

	err = file_copy(src, dst);
	if (err) {
		FT2_LOG_ERR("copy failed: %d\n", err);
		goto finish;
	}

	err = file_compare(src, dst);
	if (err) {
		FT2_LOG_ERR("compare failed: %d\n", err);
		goto finish;
	}

finish:
	FT2_LOG_INFO("Test mode %d, status %d\n", uhs, err);
	return err;
}


static int ft2_sd_test(void)
{
	int err;

	err = test_switch_mode(0, sd20_input, sd20_output);
	if (err) {
		FT2_LOG_ERR("normal mode failed: %d\n", err);
		goto finish;
	}

	err = test_switch_mode(1, sd30_input, sd30_output);
	if (err) {
		FT2_LOG_ERR("uhs mode failed: %d\n", err);
		goto finish;
	}

finish:
	if (err)
		FT2_LOG_ERR("ERROR occured, please check\n");
	else
		FT2_LOG_INFO("SD Test Pass\n");
	return err;
}

int ft2_sd_check(struct protocol_command *pcmd)
{
	if (!pcmd)
		return -FT2_NULL_POINT;

	return FT2_OK;
}

int ft2_sd_runonce(void *priv, struct protocol_command *pcmd, char *path)
{
	int err;

	err = ft2_sd_test();
	if (err) {
		append_pt_command_result(pcmd,
			FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
		return -FT2_ERROR;
	}

	append_pt_command_result(pcmd,
		FT2_TEST_PASS, strlen(FT2_TEST_PASS));
	return FT2_OK;
}
