#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/stat.h>
#include <linux/types.h>
#include <linux/ioctl.h>

#include "define.h"

#define DBG_IOMEM_IOC_MAGIC 	0x73

#define DBG_IOMEM_IOC_IOREAD32	_IOWR(DBG_IOMEM_IOC_MAGIC, 0xA2, int)
#define DBG_IOMEM_IOC_IOWRITE32	_IOWR(DBG_IOMEM_IOC_MAGIC, 0xA3, int)

static int dbg_ioread32(int fd, u32 addr, u32 *val)
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

static int dbg_iowrite32(int fd, u32 addr, u32 val)
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

static int fephy_write(u32 page, u32 reg, u32 data)
{
	u32 temp, rdata;
	int fd, ret = 0;

	fd = open("/dev/dbg_iomem", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Open device fail!\n");
		exit(EXIT_FAILURE);
	}

	/*cfg page*/
	rdata = 0xffffffff;
	temp = 0x80000000 + (0x1F<<16) + page; /*addr:31  data:page*/
	ret = dbg_iowrite32(fd, 0xb8400060, temp);
	if (ret < 0)
		goto out;
	while ((rdata & 0x80000000) != 0x0) {
		ret = dbg_ioread32(fd, 0xb8400060, &rdata);
		if (ret < 0)
			goto out;
	}

	/*write reg*/
	rdata = 0xffffffff;
	temp = 0x80000000+(reg<<16)+data;
	ret = dbg_iowrite32(fd, 0xb8400060, temp);
	if (ret < 0)
		goto out;
	while ((rdata & 0x80000000) != 0x0) {
		ret = dbg_ioread32(fd, 0xb8400060, &rdata);
		if (ret < 0)
			goto out;
	}
out:
	(void)close(fd);
	return ret;
}

static int do_fephy_write(int argc, char * const argv[])
{
	unsigned long page;
	unsigned long addr;
	unsigned long data;
	char *endp;

	page = strtoul(argv[1], &endp, 16);
	if (*argv[1] == 0 || *endp != 0)
		return -1;
	addr = strtoul(argv[2], &endp, 16);
	if (*argv[2] == 0 || *endp != 0)
		return -1;
	data = strtoul(argv[3], &endp, 16);
	if (*argv[3] == 0 || *endp != 0)
		return -1;
	fephy_write(page, addr, data);

	return 0;
}

static u32 fephy_read(u32 page, u32 reg)
{
	u32 temp, wdata, rdata;
	int fd, ret = 0;

	fd = open("/dev/dbg_iomem", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Open device fail!\n");
		exit(EXIT_FAILURE);
	}

	/*cfg page*/
	rdata = 0xffffffff;
	temp = 0x80000000 + (0x1F<<16) + page; /*addr:31  data:page*/
	ret = dbg_iowrite32(fd, 0xb8400060, temp);
	if (ret < 0)
		goto out;
	while ((rdata & 0x80000000) != 0x0) {
		ret = dbg_ioread32(fd, 0xb8400060, &rdata);
		if (ret < 0)
			goto out;
	}

	/*read reg*/
	wdata = (reg<<16) & 0x1f0000;
	ret = dbg_iowrite32(fd, 0xb8400060, wdata);
	if (ret < 0)
		goto out;
	do {
		ret = dbg_ioread32(fd, 0xb8400060, &rdata);
		if (ret < 0)
			goto out;
	} while ((rdata & 0x80000000) == 0x0);

	(void)close(fd);
	return (rdata & 0xffff);
out:
	(void)close(fd);
	return (u32)ret;

}

static u32 do_fephy_read(int argc, char * const argv[])
{
	unsigned long page;
	unsigned long addr;
	unsigned long data;
	char *endp;

	page = strtoul(argv[1], &endp, 16);
	if (*argv[1] == 0 || *endp != 0)
		return -1;
	addr = strtoul(argv[2], &endp, 16);
	if (*argv[2] == 0 || *endp != 0)
		return -1;

	data = fephy_read(page, addr);

	printf("0x%x\n", data);

	return 0;
}

static u32 do_fephy_dump(int argc, char * const argv[])
{
	unsigned long page;
	unsigned long addr;
	unsigned long cycle;
	unsigned long data, i;
	char *endp;

	page = strtoul(argv[1], &endp, 16);
	if (*argv[1] == 0 || *endp != 0)
		return -1;
	addr = strtoul(argv[2], &endp, 16);
	if (*argv[2] == 0 || *endp != 0)
		return -1;
	cycle = strtoul(argv[3], &endp, 16);
	if (*argv[1] == 0 || *endp != 0)
		return -1;

	for (i = 0; i < cycle; i++) {
		data = fephy_read(page, addr);
		printf("%d: %x\n", i, data);
	}
	return 0;
}

int main(int argc, char * const argv[])
{
	const char *cmd;
	int ret;

	/* need at least two arguments */
	if (argc < 2)
		printf("cmd wrong\n");

	cmd = argv[1];
	--argc;
	++argv;

	if (strcmp(cmd, "wr") == 0) {
		ret = do_fephy_write(argc, argv);
		goto done;
	}

	if (strcmp(cmd, "rd") == 0) {
		ret = do_fephy_read(argc, argv);
		goto done;
	}

	if (strcmp(cmd, "dump") == 0) {
		ret = do_fephy_dump(argc, argv);
		goto done;
	}

done:
	 return 0;
}


