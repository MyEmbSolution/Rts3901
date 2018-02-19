#include <common.h>
#include <malloc.h>
#include <asm/io.h>
#include <netdev.h>

int do_set_ipaddr(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{
	char *eth_addr = "192.168.1.1";
	if (argc < 2)
		goto usage;

	eth_addr = argv[1];
	--argc;
	++argv;

	if (setenv("ipaddr", eth_addr) != 0)
		printf("set ip address fail\n");
	return 0;

usage:
	return -1;
}

#define SET_ETHADDR_HELP

U_BOOT_CMD(
	setipaddr, 3,	1, do_set_ipaddr,
	"set ip address",
	"setipaddr xxx.xxx.xxx.xxx"
	SET_ETHADDR_HELP
);

