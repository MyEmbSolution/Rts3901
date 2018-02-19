#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "define.h"

#define DBG_IOMEM_IOC_MAGIC 	0x73

#define DBG_IOMEM_IOC_IOREAD32	_IOWR(DBG_IOMEM_IOC_MAGIC, 0xA2, int)
#define DBG_IOMEM_IOC_IOWRITE32	_IOWR(DBG_IOMEM_IOC_MAGIC, 0xA3, int)

static void print_usage()
{
	fprintf(stdout, "Usage: iomem [-r|w|d] ...\n"
			"\t -r addr\n"
			"\t -w addr val\n"
			"\t -d addr len file\n");
}

static int _ioread32(int fd, u32 addr, u32 *val)
{
	int err = 0;
	u8 buf[4];

	buf[0] = (u8)(addr >> 24);
	buf[1] = (u8)(addr >> 16);
	buf[2] = (u8)(addr >> 8);
	buf[3] = (u8)addr;

	err = ioctl(fd, DBG_IOMEM_IOC_IOREAD32, buf);
	if (err < 0)
		return err;

	if (val)
		*val = ((u32)buf[0] << 24) | ((u32)buf[1] << 16) |
				((u32)buf[2] << 8) | buf[3];

	return 0;
}

static int _iowrite32(int fd, u32 addr, u32 val)
{
	int err = 0;
	u8 buf[8];

	buf[0] = (u8)(addr >> 24);
	buf[1] = (u8)(addr >> 16);
	buf[2] = (u8)(addr >> 8);
	buf[3] = (u8)addr;
	buf[4] = (u8)(val >> 24);
	buf[5] = (u8)(val >> 16);
	buf[6] = (u8)(val >> 8);
	buf[7] = (u8)val;

	err = ioctl(fd, DBG_IOMEM_IOC_IOWRITE32, buf);
	if (err < 0)
		return err;

	return 0;
}

static int _iodump(int fd, u32 addr, u8 *buf, int len)
{
	u32 val;
	int cnt = len >> 2;
	int i, err = 0;

	if (!buf || ((len & 0x03) != 0))
		return -EIO;

	for (i = 0; i < cnt; i++) {
		err = _ioread32(fd, addr, &val);
		if (err < 0)
			return err;
		buf[i*4] = (u8)val;
		buf[i*4+1] = (u8)(val >> 8);
		buf[i*4+2] = (u8)(val >> 16);
		buf[i*4+3] = (u8)(val >> 24);
		addr += 4;
	}

	return 0;
}

int dbg_ioread32(int fd, int argc, char *argv[])
{
	u32 addr, val;
	int err = 0;

	if (argc != 1) {
		print_usage();
		return -EIO;
	}

	addr = (u32)strtoul(argv[0], NULL, 0);
	err = _ioread32(fd, addr, &val);
	if (err < 0)
		return err;

	printf("0x%08x\n", val);

	return 0;
}

int dbg_iowrite32(int fd, int argc, char *argv[])
{
	u32 addr, val;
	int err = 0;

	if (argc != 2) {
		print_usage();
		return -EIO;
	}

	addr = (u32)strtoul(argv[0], NULL, 0);
	val = (u32)strtoul(argv[1], NULL, 0);
	err = _iowrite32(fd, addr, val);
	if (err < 0)
		return err;

	return 0;
}

int dbg_iodump(int fd, int argc, char *argv[])
{
	u32 addr, len;
	u8 *buf;
	char *filename;
	int err = 0;
	FILE *fp;

	if (argc != 3) {
		print_usage();
		return -EIO;
	}

	addr = (u32)strtoul(argv[0], NULL, 0);
	len = (u32)strtoul(argv[1], NULL, 0);
	filename = argv[2];

	if (len & 0x03) {
		fprintf(stderr, "buf must be aligned to 4 bytes\n");
		return -EIO;
	}

	buf = malloc(len);
	if (!buf)
		return -ENOMEM;

	err = _iodump(fd, addr, buf, len);
	if (err < 0) {
		free(buf);
		return err;
	}

	fp = fopen(filename, "w+b");
	if (!fp) {
		fprintf(stderr, "Open %s fail\n", filename);
		free(buf);
		return -EIO;
	}

	if (fwrite(buf, len, 1, fp) != 1) {
		fprintf(stderr, "write fail\n");
		free(buf);
		fclose(fp);
		return -EIO;
	}

	fclose(fp);
	free(buf);

	return 0;
}

extern int optind;

int main(int argc, char *argv[])
{
	int fd;
	int retval;
	int c, read = 0, write = 0, dump = 0;
	int (*funcptr)(int, int, char **) = NULL;

	optind = 0;
	while ((c = getopt(argc, argv, ":rwd")) != -1) {
		switch (c) {
		case 'r':
			read = 1;
			funcptr = dbg_ioread32;
			break;

		case 'w':
			write = 1;
			funcptr = dbg_iowrite32;
			break;

		case 'd':
			dump = 1;
			funcptr = dbg_iodump;
			break;
		case ':':
			fprintf(stderr, "-%c requires argument\n", optopt);
			exit(EXIT_FAILURE);

		case'?':
			fprintf(stderr, "-%c is invalid\n", optopt);
			exit(EXIT_FAILURE);
		}
	}

	if (((read + write + dump) != 1) || !funcptr) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	fd = open("/dev/dbg_iomem", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Open device fail!\n");
		exit(EXIT_FAILURE);
	}

	retval = funcptr(fd, argc - optind, argv + optind);
	if (retval < 0) {
		fprintf(stderr, "iomem fail\n");
		(void)close(fd);
		exit(EXIT_FAILURE);
	}

	(void)close(fd);

	return 0;
}
