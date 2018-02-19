/*
 * testusb_device.c
 * aim to test eps(control, bulk in/out, intr in) of usb device
 * lei_wang (lei_wang@realsil.com.cn)
 * 20150706
 *
 * layers:
 * HOST ------> testusb_device
 * HOST ------> rs_ft2_usbh driver
 * |
 * |
 * |
 * DEVICE -----> rs_ft2_usbg driver
 *
 * notice: HOST can be PC or IPcam
	   DEVICE is usb device of IPcam
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

#define	TEST_CASES	7
struct usbtest_param {
	/* inputs */
	unsigned		test_num;	/* 0..(TEST_CASES-1) */
	unsigned		iterations;
	unsigned		length;
	unsigned		vary;	/* temp not use */
	unsigned		sglen;	/* temp not use */

	/* outputs */
	struct timeval		duration;
};
#define USBTEST_REQUEST	_IOWR('U', 100, struct usbtest_param)

struct testdev {
/*	struct testdev	*next; */
	char			*name;
	unsigned		ifnum : 8;
/*	unsigned		forever : 1; */
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
		perror(name);
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
		perror("malloc");
		return 0;
	}

	entry->ifnum = ifnum;

	fprintf(stderr, "%s: %s  %u\n", __func__, entry->name, entry->ifnum);

/*	entry->next = testdevs; */
	testdevs = entry;
	return 0;
}

static int usbdev_ioctl (int fd, int ifno, unsigned request, void *param)
{
	struct usbdevfs_ioctl	wrapper;

	wrapper.ifno = ifno;
	wrapper.ioctl_code = request;
	wrapper.data = param;

	return ioctl (fd, USBDEVFS_IOCTL, &wrapper);
}

static int handle_testdev (void *arg)
{
	struct testdev		*dev = arg;
	int			fd, i;
	int			status;

	if ((fd = open (dev->name, O_RDWR)) < 0) {
		perror ("can't open dev file r/w");
		return 0;
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
			printf ("%s test %d --> %d (%s)\n",
				dev->name, i, errno, buf);
		} else
			printf ("%s test %d, %4d.%.06d secs\n", dev->name, i,
				(int) dev->param.duration.tv_sec,
				(int) dev->param.duration.tv_usec);

		fflush (stdout);
	}

	close (fd);

	return status;
}

static int parse_num(unsigned *num, const char *str)
{
	unsigned long val;
	char *end;

	errno = 0;
	val = strtoul(str, &end, 0);
	if (errno || *end || val > UINT_MAX)
		return -1;
	*num = val;
	return 0;
}

int main (int argc, char **argv)
{
	const char			*usb_dir = NULL;
	int					test = -1 /* all */;
	struct usbtest_param	param;
	int status = 0;
	int log_fd;

	int test_count;
	int test_index = 0;
	char c;

	if (argc < 2)
		goto usage;

	while ((c = getopt (argc, argv, "t:c:")) != -1)
	switch (c) {
	case 'c':	/* count iterations */
		if (parse_num(&test_count, optarg))
			goto usage;
		continue;
	case 't':	/* run just one test */
		test_index = atoi (optarg);
		if (test_index < 1 || test_index > 6)
			goto usage;
		continue;
	case '?':
	case 'h':
	default:
usage:
		fprintf (stderr,
			"usage: %s [options]\n"
			"Options:\n"
			"\t-t testnum(1 2 5 6) only run specified case\n"
			"\t-c iterations\n",
			argv[0]);
		return 1;
	}
	if (optind != argc)
		goto usage;

#if 0
	log_fd = open("/var/log/ft2", O_WRONLY | O_CREAT | O_TRUNC, 0640);
	if (log_fd < 0)
		return -1;
	// redirect stdout and stderr to the log file
	dup2(log_fd, 1);
	dup2(log_fd, 2);
	close(log_fd);
#endif

	usb_dir = usb_dir_find();
	if (!usb_dir) {
		fputs ("USB device files are missing!\n", stderr);
		return -1;
	}

	if (ftw (usb_dir, find_testdev, 3) != 0) {
		fputs ("ftw failed; are USB device files missing?\n", stderr);
		return -1;
	}

	if (!testdevs) {
		fputs ("no test devices recognized!\n", stderr);
		return -1;
	}

	while (test_count-- > 0) {
		param.iterations = 1;
		param.length = 512;
		testdevs->param = param;
		switch (test_index) {
		case 1:
			testdevs->test = 1;
			status = handle_testdev(testdevs);
			if (status < 0) {
				fprintf(stderr, "Test 1 failed, status=%d\n", status);
				exit(1);
			}

			testdevs->test = 2;
			status = handle_testdev(testdevs);
			if (status < 0) {
				fprintf(stderr, "Test 2 failed, status=%d\n", status);
				exit(1);
			}
			break;
		case 2:
			testdevs->test = 3;
			status = handle_testdev(testdevs);
			if (status < 0) {
				fprintf(stderr, "Test 3 failed, status=%d\n", status);
				exit(1);
			}

			testdevs->test = 4;
			status = handle_testdev(testdevs);
			if (status < 0) {
				fprintf(stderr, "Test 4 failed, status=%d\n", status);
				exit(1);
			}
			break;
		case 5:
			testdevs->test = 5;
			param.length = 16;
			testdevs->param = param;
			status = handle_testdev(testdevs);
			if (status < 0) {
				fprintf(stderr, "Test 5 failed, status=%d\n", status);
				exit(1);
			}
			break;

		case 6:
			testdevs->test = 6;
			param.length = 16;
			testdevs->param = param;
			status = handle_testdev(testdevs);
			if (status < 0) {
				fprintf(stderr, "Test 6 failed, status=%d\n", status);
				exit(1);
			}
			break;
		}
	}

	return 0;
}
