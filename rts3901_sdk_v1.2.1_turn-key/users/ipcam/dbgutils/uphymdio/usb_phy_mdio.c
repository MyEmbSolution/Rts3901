#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "define.h"

#define USB_PHY_MDIO_IOC_MAGIC		0x74

#define USB_PHY_MDIO_IOC_READ		_IOWR(USB_PHY_MDIO_IOC_MAGIC, 0xA2, int)
#define USB_PHY_MDIO_IOC_WRITE		_IOWR(USB_PHY_MDIO_IOC_MAGIC, 0xA3, int)

int mdio_read(int fd, u16 addr, u16 *val)
{
	int err = 0;
	u8 buf[2];

	buf[0] = (u8)(addr >> 8);
	buf[1] = (u8)addr;

	err = ioctl(fd, USB_PHY_MDIO_IOC_READ, buf);
	if (err < 0)
		return err;

	if (val)
		*val = ((u16)buf[0] << 8) | buf[1];

	return 0;
}

int mdio_write(int fd, u16 addr, u16 val)
{
	int err = 0;
	u8 buf[4];

	buf[0] = (u8)(addr >> 8);
	buf[1] = (u8)addr;
	buf[2] = (u8)(val >> 8);
	buf[3] = (u8)val;

	err = ioctl(fd, USB_PHY_MDIO_IOC_WRITE, buf);
	if (err < 0)
		return err;

	return 0;
}

extern int optind;

int main(int argc, char *argv[])
{
	int fd;
	int retval;
	int c, read = 0, write = 0;
	u16 addr, val;

	optind = 0;
	while ((c = getopt(argc, argv, ":rw")) != -1) {
		switch (c) {
		case 'r':
			read = 1;
			break;

		case 'w':
			write = 1;
			break;

		case ':':
			fprintf(stderr, "-%c requires argument\n", optopt);
			exit(1);

		case'?':
			fprintf(stderr, "-%c is invalid\n", optopt);
			exit(1);
		}
	}

	if ((!read && !write) || (read && write)) {
		fprintf(stderr, "Usage: %s [-r|w] addr [val]\n", argv[0]);
		exit(1);
	} else {
		if (read && (optind != (argc - 1))) {
			fprintf(stderr, "Usage: %s [-r|w] addr [val]\n", argv[0]);
			exit(1);
		}
		if (write && (optind != (argc - 2))) {
			fprintf(stderr, "Usage: %s [-r|w] addr [val]\n", argv[0]);
			exit(1);
		}
	}

	addr = (u16)strtoul(argv[optind], NULL, 0);

	fd = open("/dev/usb_phy_mdio", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Open device fail!\n");
		exit(1);
	}

	if (read) {
		retval = mdio_read(fd, addr, &val);
		if (retval < 0) {
			fprintf(stderr, "Read register fail\n");
			close(fd);
			exit(1);
		}

		printf("0x%04x\n", val);
	} else {
		val = (u16)strtoul(argv[optind + 1], NULL, 0);

		retval = mdio_write(fd, addr, val);
		if (retval < 0) {
			fprintf(stderr, "Write register fail\n");
			close(fd);
			exit(1);
		}
	}

	close(fd);

	return 0;
}
