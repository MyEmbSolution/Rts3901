#include <common.h>
#include <malloc.h>
#include <asm/io.h>
#include <netdev.h>

int do_set_ethaddr(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{
	char *eth_addr = "00:10:20:30:40:50";
	if (argc < 2)
		goto usage;

	eth_addr = argv[1];
	--argc;
	++argv;

	setenv("ethaddr", eth_addr);

	if (set_ethaddr() != 0)
		printf("set ethaddr fail\n");
	return 0;

usage:
	return -1;
}

#define SET_ETHADDR_HELP

U_BOOT_CMD(
	setethaddr, 3,	1, do_set_ethaddr,
	"set eth address",
	"setethaddr x0:x1:x2:x3:x4:x5 --x0~x5 can not be all zero or ff, [x0]&[0x01] can not be 1"
	SET_ETHADDR_HELP
);
