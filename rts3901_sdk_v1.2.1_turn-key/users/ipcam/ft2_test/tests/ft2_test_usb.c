/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_usb.c
 *
 * Copyright (C) 2015      Lei Wang<lei_wang@realsil.com.cn>
 */

#include <stdio.h>
#include <string.h>
#include <ftw.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <linux/usb/ch9.h>

#include "protocol.h"
#include "ft2errno.h"
#include "ft2log.h"


#define	TEST_CASES	7	/* 0..(TEST_CASES-1) */
struct usbtest_param {
	/* inputs */
	unsigned		test_num;
	unsigned		iterations;
	unsigned		length;
	unsigned		vary;	/* temp not use */
	unsigned		sglen;	/* temp not use */
	/* outputs */
	struct timeval		duration;
};
#define USBTEST_REQUEST	_IOWR('U', 100, struct usbtest_param)

struct testdev {
	char			*name;
	unsigned		ifnum : 8;
	int			test;
	struct usbtest_param	param;
};
static struct testdev		*testdevs;

static int testdev_ifnum(FILE *fd)
{
	struct usb_device_descriptor dev;

	if (fread(&dev, sizeof dev, 1, fd) != 1)
		return -1;

	if (dev.bLength != sizeof dev || dev.bDescriptorType != USB_DT_DEVICE)
		return -1;

	/* realsil FT2 testusb */
	if (dev.idVendor == 0x0bda && dev.idProduct == 0x3901)
		return 0;

	return -1;
}

static const char *usb_dir_find(void)
{
	static char udev_usb_path[] = "/dev/bus/usb";

	if (access(udev_usb_path, F_OK) == 0)
		return udev_usb_path;

	return NULL;
}

static int find_testdev(const char *name, const struct stat *sb, int flag)
{
	FILE				*fd;
	int				ifnum;
	struct testdev	*entry;

	(void)sb; /* unused */

	if (flag != FTW_F)
		return 0;

	fd = fopen(name, "rb");
	if (!fd) {
		FT2_LOG_ERR("%s\n", name);
		return 0;
	}

	ifnum = testdev_ifnum(fd);
	fclose(fd);
	if (ifnum < 0)
		return 0;

	entry = calloc(1, sizeof *entry);
	if (!entry)
		goto nomem;

	entry->name = strdup(name);
	if (!entry->name) {
		free(entry);
nomem:
		FT2_LOG_ERR("malloc failed\n");
		return 0;
	}

	entry->ifnum = ifnum;
	testdevs = entry;

	FT2_LOG_INFO("%s: %s  %u\n", __func__, entry->name, entry->ifnum);
	return 0;
}

static int usbdev_ioctl(int fd, int ifno, unsigned request, void *param)
{
	struct usbdevfs_ioctl	wrapper;

	wrapper.ifno = ifno;
	wrapper.ioctl_code = request;
	wrapper.data = param;

	return ioctl (fd, USBDEVFS_IOCTL, &wrapper);
}

static int handle_testdev(struct testdev *arg)
{
	struct testdev		*dev = arg;
	int			fd, i;
	int			status;

	if ((fd = open (dev->name, O_RDWR)) < 0) {
		FT2_LOG_ERR("can't open dev file r/w");
		return -ENODEV;
	}

	for (i = 0; i < TEST_CASES; i++) {
		if (dev->test != -1 && dev->test != i)
			continue;
		dev->param.test_num = i;

		status = usbdev_ioctl (fd, dev->ifnum,
				USBTEST_REQUEST, &dev->param);
		if (status < 0 && errno == EOPNOTSUPP)
			continue;

		if (status < 0) {
			char	buf [80];
			int	err = errno;

			if (strerror_r (errno, buf, sizeof buf)) {
				snprintf (buf, sizeof buf, "error %d", err);
				errno = err;
			}
			FT2_LOG_ERR ("%s test %d --> %d (%s)\n",
				dev->name, i, errno, buf);
		} else
			FT2_LOG_INFO("%s test %d, %4d.%.06d secs\n",
				dev->name, i,
				(int) dev->param.duration.tv_sec,
				(int) dev->param.duration.tv_usec);

		fflush (stdout);
	}

	close(fd);
	return status;
}

static int usb_test_func()
{
	const char *usb_dir = NULL;
	struct usbtest_param	param;
	int status = 0;
	int test_count = 10;

	usb_dir = usb_dir_find();
	if (!usb_dir) {
		FT2_LOG_ERR("usb device files are missing!\n");
		return -FT2_ERROR;
	}

	if (ftw (usb_dir, find_testdev, 3) != 0) {
		FT2_LOG_ERR("ftw failed; are usb device files missing?\n");
		return -FT2_ERROR;
	}

	if (!testdevs) {
		FT2_LOG_ERR("no test devices recognized!\n");
		return -FT2_ERROR;
	}

	param.iterations = 1;
	while (test_count-- > 0) {
		param.length = 512;
		testdevs->param = param;

		testdevs->test = 1;
		status = handle_testdev(testdevs);
		if (status < 0) {
			FT2_LOG_ERR("Test 1 failed, status=%d\n", status);
			return -FT2_ERROR;
		}

		testdevs->test = 2;
		status = handle_testdev(testdevs);
		if (status < 0) {
			FT2_LOG_ERR("Test 2 failed, status=%d\n", status);
			return -FT2_ERROR;
		}

		testdevs->test = 3;
		status = handle_testdev(testdevs);
		if (status < 0) {
			FT2_LOG_ERR("Test 3 failed, status=%d\n", status);
			return -FT2_ERROR;
		}

		testdevs->test = 4;
		status = handle_testdev(testdevs);
		if (status < 0) {
			FT2_LOG_ERR("Test 4 failed, status=%d\n", status);
			return -FT2_ERROR;
		}

		testdevs->test = 5;
		param.length = 16;
		testdevs->param = param;
		status = handle_testdev(testdevs);
		if (status < 0) {
			FT2_LOG_ERR("Test 5 failed, status=%d\n", status);
			return -FT2_ERROR;
		}

		testdevs->test = 6;
		param.length = 16;
		testdevs->param = param;
		status = handle_testdev(testdevs);
		if (status < 0) {
			FT2_LOG_ERR("Test 6 failed, status=%d\n", status);
			return -FT2_ERROR;
		}
	}

	return FT2_OK;
}

int ft2_usb_check(struct protocol_command *pcmd)
{
	if (!pcmd)
		return -FT2_NULL_POINT;

	return FT2_OK;
}

int ft2_usb_runonce(void *priv, struct protocol_command *pcmd, char *path)
{
	int ret = FT2_OK;

	if (!pcmd)
		return -FT2_NULL_POINT;

	ret = usb_test_func();
	if (ret) {
		append_pt_command_result(pcmd,
				FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
		return ret;
	}

	append_pt_command_result(pcmd, FT2_TEST_PASS, strlen(FT2_TEST_PASS));
	return FT2_OK;
}
