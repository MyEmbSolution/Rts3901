/*
 * Command for accessing Ethernet efuse.
 *
 * Copyright (C) Realsil Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>

#define EFUSE_MAX_ADDR		24

extern unsigned char efuse_read_byte(unsigned int addr);

static int do_efuse_read(int argc, char * const argv[])
{
	unsigned int addr;
	unsigned char data;
	char *endp;

	if (argc != 2) {
		printf("command error\n");
		return -1;
	}

	addr = simple_strtoul(argv[1], &endp, 0);

	if (*argv[1] == 0 || *endp != 0 || addr >= EFUSE_MAX_ADDR)
		return -1;

	data = efuse_read_byte(addr);

	printf("0x%02x\n", data);

	return 0;
}

static int do_efuse_read_all(int argc, char * const argv[])
{
	unsigned int addr;
	unsigned char data;

	if (argc != 1) {
		printf("command error\n");
		return -1;
	}

	for (addr = 0; addr < EFUSE_MAX_ADDR; addr++) {
		data = efuse_read_byte(addr);
		printf("0x%02x, ", data);
		if (!((addr + 1) % 8))
			printf("\n");
	}

	return 0;
}

static int do_efuse(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{
	const char *cmd;
	int ret = -1;

	/* need at least two arguments */
	if (argc < 2)
		goto usage;

	cmd = argv[1];
	--argc;
	++argv;

	if (strcmp(cmd, "read") == 0) {
		ret = do_efuse_read(argc, argv);
		goto done;
	}
	if (strcmp(cmd, "readall") == 0) {
		ret = do_efuse_read_all(argc, argv);
		goto done;
	}
done:
	if (ret != -1)
		return ret;
usage:
	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	efuse,	3,	1,	do_efuse,
	"efuse readall | read addr",
	"read addr	- (from 0-23): read 1 byte data from addr in efuse.\n"
	"efuse readall	- read all data in efuse.\n"
);
