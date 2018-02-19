#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>

#define RTS_EFUSE_IOC_MAGIC 0x75

#define RTS_EFUSE_IOC_IOREAD_EFUSE _IOWR(RTS_EFUSE_IOC_MAGIC, 0xA2, int)
#define RTS_EFUSE_IOC_IOWRITE_EFUSE _IOWR(RTS_EFUSE_IOC_MAGIC, 0xA3, int)

extern int optind;
static void print_help()
{
	fprintf(stdout, "Usage: efuse_mfg [-r|w|d|p] ...\n"
			"\t -r [addr]       read single data, addr range[0-23]\n"
			"\t -w [addr] [0xdeadbfXX] XX:data to be written, addr range[0-23]\n"
			"\t -d              dump all efuse data\n");
}

static int _ioread32(int fd, uint32_t addr, uint32_t *val)
{
	int err = 0;
	uint8_t buf[4];

	*(uint32_t *)buf = htobe32(addr);

	err = ioctl(fd, RTS_EFUSE_IOC_IOREAD_EFUSE, buf);
	if (err < 0)
		return err;

	if (val)
		*val = be32toh(*(uint32_t *)buf);

	return 0;
}

static int _iowrite32(int fd, uint32_t addr, uint32_t val)
{
	int err = 0;
	uint8_t buf[8];

	*(uint32_t *)buf = htobe32(addr);
	*(uint32_t *)(buf + 4) = htobe32(val);

	err = ioctl(fd, RTS_EFUSE_IOC_IOWRITE_EFUSE, buf);
	if (err < 0)
		return err;

	return 0;
}

int efuse_read(int fd, int argc, char *argv[])
{
	uint32_t addr, val;
	int err = 0;

	if (argc != 1) {
		print_help();
		return -EIO;
	}

	addr = (uint32_t)strtoul(argv[0], NULL, 0);
	err = _ioread32(fd, addr, &val);
	if (err < 0)
		return err;

	fprintf(stdout, "0x%02X\n", val);

	return 0;
}

int efuse_dump(int fd, int argc, char *argv[])
{
	uint32_t val;
	int err = 0;
	int i;

	if (argc != 0) {
		print_help();
		return -EIO;
	}

	for (i = 0; i < 24; i++)
	{
		if (_ioread32(fd, i, &val) < 0)
			return err;
		if (i % 8 == 0)
			fprintf(stdout, "\n%02d-%02d: ",i ,i + 7);
		fprintf(stdout, "0x%02X ", val);
	}
	putchar('\n');

	return 0;
}


int efuse_write(int fd, int argc, char *argv[])
{
	uint32_t addr, val;
	int err = 0;

	if (argc != 2) {
		print_help();
		return -EIO;
	}

	addr = (uint32_t)strtoul(argv[0], NULL, 0);
	val = (uint32_t)strtoul(argv[1], NULL, 0);
	err = _iowrite32(fd, addr, val);
	if (err < 0)
		return err;

	return 0;
}

int main(int argc, char *argv[])
{
	int fd;
	int c, retval;
	int (*funcptr)(int, int, char **) = NULL;
	int cmd_cnt = 0;

	optind = 0;
	while ((c = getopt(argc, argv, ":rwd")) != -1) {
		switch (c) {
			case 'r':
				cmd_cnt++;
				funcptr = efuse_read;
				break;
			case 'w':
				cmd_cnt++;
				funcptr = efuse_write;
				break;
			case 'd':
				cmd_cnt++;
				funcptr = efuse_dump;
				break;
				break;
			case ':':
				if (optopt == 'r' || optopt == 'w' || optopt == 'p')
					fprintf(stderr, "Option -%c requres an argument.\n", optopt);
				else
					fprintf(stderr, "Invalid argument.\n");

				break;
			case '?':
				print_help();
				exit(EXIT_FAILURE);
				break;
		}
	}
	if ((cmd_cnt != 1 || !funcptr)) {
		print_help();
		exit(EXIT_FAILURE);
	}

	fd = open("/dev/efuse", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Open device fail!\n");
		exit(EXIT_FAILURE);
	}

	retval = funcptr(fd, argc - optind, argv + optind);
	if (retval < 0) {
		fprintf(stderr, "efuse_mfg fail\n");
		(void)close(fd);
		exit(EXIT_FAILURE);
	}

	(void)close(fd);

	return 0;


}
