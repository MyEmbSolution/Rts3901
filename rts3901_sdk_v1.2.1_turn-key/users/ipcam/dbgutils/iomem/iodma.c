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

#define DBG_IOMEM_IOC_MAGIC 		0x73

#define DBG_IOMEM_IOC_ALLOC_DMABUF	_IOWR(DBG_IOMEM_IOC_MAGIC, 0xAA, int)
#define DBG_IOMEM_IOC_FREE_DMABUF	_IOWR(DBG_IOMEM_IOC_MAGIC, 0xAB, int)
#define DBG_IOMEM_IOC_COPY_DMABUF	_IOWR(DBG_IOMEM_IOC_MAGIC, 0xAC, int)

static void print_usage()
{
	fprintf(stdout, "Usage: iodma [-a|f|c] ...\n"
			"\t -a len\n"
			"\t -f\n"
			"\t -c src dst\n");
}

int alloc_mem(int fd, int argc, char *argv[])
{
	int err = 0;
	u32 val;
	u8 buf[4];

	if (argc != 1) {
		print_usage();
		return -EIO;
	}

	val = (u32)strtoul(argv[0], NULL, 0);
	buf[0] = (u8)(val >> 24);
	buf[1] = (u8)(val >> 16);
	buf[2] = (u8)(val >> 8);
	buf[3] = (u8)val;

	err = ioctl(fd, DBG_IOMEM_IOC_ALLOC_DMABUF, buf);
	if (err < 0)
		return err;

	val = ((u32)buf[0] << 24) | ((u32)buf[1] << 16) |
		((u32)buf[2] << 8) | buf[3];
	printf("Physical address of allocated memory is 0x%08x\n", val);

	return 0;
}

int free_mem(int fd, int argc, char *argv[])
{
	int err = 0;

	if (argc != 0) {
		print_usage();
		return -EIO;
	}

	err = ioctl(fd, DBG_IOMEM_IOC_FREE_DMABUF, NULL);
	if (err < 0)
		return err;

	return 0;
}

int copy_dma(int fd, int argc, char *argv[])
{
	int err = 0;
	u8 buf[12];
	u32 src, dst, len, result;

	if (argc != 3) {
		print_usage();
		return -EIO;
	}

	src = (u32)strtoul(argv[0], NULL, 0);
	dst = (u32)strtoul(argv[1], NULL, 0);
	len = (u32)strtoul(argv[2], NULL, 0);

	buf[0] = (u8)(src >> 24);
	buf[1] = (u8)(src >> 16);
	buf[2] = (u8)(src >> 8);
	buf[3] = (u8)src;
	buf[4] = (u8)(dst >> 24);
	buf[5] = (u8)(dst >> 16);
	buf[6] = (u8)(dst >> 8);
	buf[7] = (u8)dst;
	buf[8] = (u8)(len >> 24);
	buf[9] = (u8)(len >> 16);
	buf[10] = (u8)(len >> 8);
	buf[11] = (u8)len;

	err = ioctl(fd, DBG_IOMEM_IOC_COPY_DMABUF, buf);
	if (err < 0)
		return err;

	result = ((u32)buf[1] << 24) | ((u32)buf[2] << 16) |
		((u32)buf[3] << 8) | buf[4];
	if (buf[0] == 1) {
		/* error code */
		printf("Error code: -%d\n", result);
	} else {
		/* finished length */
		printf("%d bytes copied.\n", result);
	}

	return 0;
}

extern int optind;

int main(int argc, char *argv[])
{
	int fd;
	int retval;
	int c, allocmem = 0, freemem = 0, copydma = 0;
	int (*funcptr)(int, int, char **) = NULL;

	optind = 0;
	while ((c = getopt(argc, argv, ":afc")) != -1) {
		switch (c) {
		case 'a':
			allocmem = 1;
			funcptr = alloc_mem;
			break;

		case 'f':
			freemem = 1;
			funcptr = free_mem;
			break;

		case 'c':
			copydma = 1;
			funcptr = copy_dma;
			break;

		case ':':
			fprintf(stderr, "-%c requires argument\n", optopt);
			exit(EXIT_FAILURE);

		case'?':
			fprintf(stderr, "-%c is invalid\n", optopt);
			exit(EXIT_FAILURE);
		}
	}

	if (((allocmem + freemem + copydma) != 1) || !funcptr) {
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
		fprintf(stderr, "iodma fail\n");
		(void)close(fd);
		exit(EXIT_FAILURE);
	}

	(void)close(fd);

	return 0;
}
