#include <common.h>
#include <malloc.h>
#include <asm/io.h>

#define TEST_MEM32(addr)	(*(volatile u32 *)(addr))

static void fephy_write(u32 page, u32 reg, u32 data)
{
	u32 temp, rdata;

	/*cfg page*/
	rdata = 0xffffffff;
	temp = 0x80000000 + (0x1F<<16) + page; /*addr:31  data:page*/
	TEST_MEM32(0xb8400060) = temp;
	while ((rdata & 0x80000000) != 0x0) {
		rdata = TEST_MEM32(0xb8400060);
	}
	/*write reg*/
	rdata = 0xffffffff;
	temp = 0x80000000+(reg<<16)+data;
	TEST_MEM32(0xb8400060) = temp;
	while ((rdata & 0x80000000) != 0x0) {
		rdata = TEST_MEM32(0xb8400060);
	}
}

static int do_fephy_write(int argc, char * const argv[])
{
	unsigned long page;
	unsigned long addr;
	unsigned long data;
	char *endp;

	page = simple_strtoul(argv[1], &endp, 16);
	if (*argv[1] == 0 || *endp != 0)
		return -1;
	addr = simple_strtoul(argv[2], &endp, 16);
	if (*argv[2] == 0 || *endp != 0)
		return -1;
	data = simple_strtoul(argv[3], &endp, 16);
	if (*argv[3] == 0 || *endp != 0)
		return -1;
	fephy_write(page, addr, data);

	return 0;
}

static u32 fephy_read(u32 page, u32 reg)
{
	u32 temp, wdata, rdata;

	/*cfg page*/
	rdata = 0xffffffff;
	temp = 0x80000000 + (0x1F<<16) + page; /*addr:31  data:page*/
	TEST_MEM32(0xb8400060) = temp;
	while ((rdata & 0x80000000) != 0x0) {
		rdata = TEST_MEM32(0xb8400060);
	}
	/*read reg*/
	wdata = (reg<<16) & 0x1f0000;
	TEST_MEM32(0xb8400060) = wdata;
	rdata = TEST_MEM32(0xb8400060);
	while ((rdata & 0x80000000) == 0x0) {
		rdata = TEST_MEM32(0xb8400060);
	}
	return (rdata & 0xffff);
}

static u32 do_fephy_read(int argc, char * const argv[])
{
	unsigned long page;
	unsigned long addr;
	unsigned long data;
	char *endp;

	page = simple_strtoul(argv[1], &endp, 16);
	if (*argv[1] == 0 || *endp != 0)
		return -1;
	addr = simple_strtoul(argv[2], &endp, 16);
	if (*argv[2] == 0 || *endp != 0)
		return -1;

	data = fephy_read(page, addr);

	printf("0x%x\n", (unsigned int)data);

	return 0;
}

static u32 do_fephy_dump(int argc, char * const argv[])
{
	unsigned long page;
	unsigned long addr;
	unsigned long cycle;
	unsigned long data;
	int i;
	char *endp;

	page = simple_strtoul(argv[1], &endp, 16);
	if (*argv[1] == 0 || *endp != 0)
		return -1;
	addr = simple_strtoul(argv[2], &endp, 16);
	if (*argv[2] == 0 || *endp != 0)
		return -1;
	cycle = simple_strtoul(argv[3], &endp, 16);
	if (*argv[1] == 0 || *endp != 0)
		return -1;

	for (i = 0; i < cycle; i++) {
		data = fephy_read(page, addr);
		printf("%d: 0x%x\n", i, (unsigned int)data);
	}
	return 0;
}

static u32 do_fephy_check(int argc, char * const argv[])
{
	unsigned long page;
	unsigned long addr_start;
	unsigned long addr_end;
	unsigned long data;
	int i;
	char *endp;

	page = simple_strtoul(argv[1], &endp, 16);
	if (*argv[1] == 0 || *endp != 0)
		return -1;
	addr_start = simple_strtoul(argv[2], &endp, 16);
	if (*argv[2] == 0 || *endp != 0)
		return -1;
	addr_end = simple_strtoul(argv[3], &endp, 16);
	if (*argv[1] == 0 || *endp != 0)
		return -1;

	for (i = addr_start; i <= addr_end; i++) {
		data = fephy_read(page, i);
		printf("%d: 0x%x\n", i, (unsigned int)data);
	}
	return 0;
}

static int do_fephy(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{
	int ret = 0;
	const char *cmd;

	/* need at least two arguments */
	if (argc < 2) {
		printf("cmd wong\n");
		goto done;
	}

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

	if (strcmp(cmd, "check") == 0) {
		ret = do_fephy_check(argc, argv);
		goto done;
	}

done:
	 return ret;
}

#define FEPHY_TEST_HELP
U_BOOT_CMD(
	fephy,	5,	1,	do_fephy,
	"fephy read/write",
	"fephy rd page_index register_index -- all setting should be hex number\n"
	"fephy rd page_index register_index value -- all setting should be hex number\n"
	"fephy dump page_index register_index cycle_num -- all setting should be hex number\n"
	"fephy dump page_index register_start register_end-- all setting should be hex number\n"
	FEPHY_TEST_HELP
);


