#include <common.h>
#include <malloc.h>
#include <asm/io.h>
#include <net.h>
#include <spi_flash.h>
#include <configs/rlxboard.h>

#ifndef _MEM_TEST3_
static char *tmpfile;
#define GETMEM(addr)	(*(volatile u32 *)(addr))

static u32 _do_get_file_(char *filename)
{
	u32 len;
	tmpfile = filename;

	setenv("bootfile", tmpfile);
	NetLoop(TFTPSRV);

	len = getenv_hex("filesize", 0);

	return len;
}

static int _do_write_file_(u32 offset, u32 len, u32 loadaddr)
{
	int ret = 0;
	char *buf;

	buf = (char *)loadaddr;
#ifdef CONFIG_RLX_SPI
	ret = spi_flash_update_external(offset, len, buf);
#endif
	return ret;
}

static int do_update_uboot(void)
{
	int ret = 0;
	u32 len = 0;
	u32 offset = 0;
	u32 loadaddr = CONFIG_LOADADDR;

	len = _do_get_file_("/u-boot.bin");
	ret = _do_write_file_(offset, len, loadaddr);

	debug("get %s from %x , write %x byte to offset %x\n", tmpfile, loadaddr, len, offset);
	return ret;
}
/*
static int do_update_uboot(int argc, char * const argv[])
{
	int ret;
	char *buf;

	u32 offset = 0;
	u32 len = SPI_SECTOR_SIZE * SPI_UBOOT_BLK_CNT;
	buf = CONFIG_LOADADDR;
	tmp_bootfile = "/u-boot.bin";
	setenv("bootfile", tmp_bootfile);
	NetLoop(TFTPGET);

	ret = spi_flash_update_external(offset, len, buf);

	printf("update uboot from ethernet and write to flash %s\n", getenv("bootfile"));
	return ret;
}
*/
static int do_update_fw(void)
{
	int ret = 0;
	u32 offset = SPI_FW_OFFSET;
	u32 len = 0;
	u32 loadaddr = CONFIG_LOADADDR;

	if (offset == NO_FW_PARTITION) {
		printf("no mcu fw partition\n");
		return ret;
	}

	len = _do_get_file_("/fw.bin");
	ret = _do_write_file_(offset, len, loadaddr);

	debug("get %s from %x , write %x byte to offset %x\n", tmpfile, loadaddr, len, offset);
	return ret;
}

static int do_update_hw(void)
{
	int ret = 0;
	u32 offset = SPI_HW_OFFSET;
	u32 len = 0;
	u32 loadaddr = CONFIG_LOADADDR;

	len = _do_get_file_("/hw.bin");
	ret = _do_write_file_(offset, len, loadaddr);

	debug("get %s from %x , write %x byte to offset %x\n", tmpfile, loadaddr, len, offset);
	return ret;
}

static int do_update_sw(void)
{
	int ret = 0;
	u32 offset = SPI_SW_OFFSET;
	u32 len = 0;
	u32 loadaddr = CONFIG_LOADADDR;

	len = _do_get_file_("/sw.bin");
	ret = _do_write_file_(offset, len, loadaddr);

	debug("get %s from %x , write %x byte to offset %x\n", tmpfile, loadaddr, len, offset);
	return ret;
}

static int do_update_kernel(void)
{
	int ret = 0;
	u32 offset = SPI_KERNEL_OFFSET;
	u32 len = 0;
	u32 loadaddr = CONFIG_LOADADDR;

	len = _do_get_file_("/vmlinux.img");
	ret = _do_write_file_(offset, len, loadaddr);

	debug("get %s from %x , write %x byte to offset %x\n", tmpfile, loadaddr, len, offset);
	return ret;
}

#ifdef SPI_ROOTFS_OFFSET
static int do_update_rootfs(void)
{
	int ret = 0;
	u32 offset = SPI_ROOTFS_OFFSET;
	u32 len = 0;
	u32 loadaddr = CONFIG_LOADADDR;

	len = _do_get_file_("/rootfs.bin");
	ret = _do_write_file_(offset, len, loadaddr);

	debug("get %s from %x , write %x byte to offset %x\n", tmpfile, loadaddr, len, offset);
	return ret;
}
#endif

static int do_update_ldctable(void)
{
	int ret = 0;
	u32 offset = SPI_LDC_OFFSET ;
	u32 len = 0;
	u32 loadaddr = CONFIG_LOADADDR;

	len = _do_get_file_("/ldc.bin");
	ret = _do_write_file_(offset, len, loadaddr);

	debug("get %s from %x , write %x byte to offset %x\n", tmpfile, loadaddr, len, offset);
	return ret;
}

static int _do_write_all(u32 all_len)
{
	int ret = 0;

	u32 magic_num;
	u32 ram_offset = 0x100;
	u32 flash_offset = 0;
	u32 filelen;

	all_len -= 0x100;
	while (all_len != 0) {

		magic_num = htonl(GETMEM(CONFIG_LOADADDR + ram_offset));
		debug("%x:%x\n", (CONFIG_LOADADDR + ram_offset), magic_num);
		flash_offset = htonl(GETMEM(CONFIG_LOADADDR + ram_offset + 0x14));
		filelen = htonl(GETMEM(CONFIG_LOADADDR + ram_offset + 0x1C));

		switch (magic_num) {
		/*uboot*/
		case 0x626f6f74:

		/*MCU fw*/
		case 0x6669726d:

		/*hw config*/
		case 0x68617264:

		/*sw config*/
		case 0x6a667332:

		/*kernel*/
		case 0x6c696e78:

		/*rootfs*/
		case 0x726f6f74:

		/*ldc table*/
		case 0x6c646300:
			printf("offset is %x\n", flash_offset);
			break;

		/*file end*/
		case 0x46454f46:
			printf("file end\n");
			return 0;


		default:
			printf("user defined magic number %x at %x\n", magic_num, (CONFIG_LOADADDR + ram_offset));	
			break;
		}
		ret = _do_write_file_(flash_offset, filelen, (CONFIG_LOADADDR + ram_offset + 0x20));
		if (ret != 0)
			return ret;
		debug("get %x file from %x, wtite %x bytes to flash offset %x\n", magic_num, ram_offset, filelen, flash_offset);
		all_len -= (filelen + 0x24);
		ram_offset += (filelen + 0x24);
	}
	debug("update all image from ethernet and write to flash\n");
	return ret;
}

static int do_update_flash(void)
{
	int ret = 0;
	u32 all_len = 0;

	all_len = getenv_hex("filesize", 0);

	ret = _do_write_all(all_len);

	return ret;
}

static int do_update_all(void)
{
	int ret = 0;
	u32 all_len = 0;

	all_len = _do_get_file_("/linux.bin");

	ret = _do_write_all(all_len);

	return ret;
}


static int do_update(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{

	const char *cmd;
	int ret = 0;

	/* need at least two arguments */
	if (argc < 2)
		goto usage;

	cmd = argv[1];
	--argc;
	++argv;

	if (strcmp(cmd, "uboot") == 0) {
		ret = do_update_uboot();
		goto done;
	}

	if (strcmp(cmd, "fw") == 0) {
		ret = do_update_fw();
		goto done;
	}

	if (strcmp(cmd, "sw") == 0) {
		ret = do_update_sw();
		goto done;
	}

	if (strcmp(cmd, "hw") == 0) {
		ret = do_update_hw();
		goto done;
	}

	if (strcmp(cmd, "kernel") == 0) {
		ret = do_update_kernel();
		goto done;
	}

#ifdef SPI_ROOTFS_OFFSET
	if (strcmp(cmd, "rootfs") == 0) {
		ret = do_update_rootfs();
		goto done;
	}
#endif

	if (strcmp(cmd, "ldc") == 0) {
		ret = do_update_ldctable();
		goto done;
	}

	if (strcmp(cmd, "flash") == 0) {
		ret = do_update_flash();
		goto done;
	}

	if (strcmp(cmd, "all") == 0) {
		ret = do_update_all();
		goto done;
	}

done:
	printf("update done\n");
	if (ret != -1)
		return ret;

usage:
	return CMD_RET_USAGE;

}
#ifdef CONFIG_CMD_SF_TEST
#define UPDATE_TEST_HELP "\nsf test offset len		" \
		"- run a very basic destructive test"
#else
#define UPDATE_TEST_HELP
#endif

U_BOOT_CMD(
	update,	5,	1,	do_update,
	"update image",
	"uboot	- get uboot.bin by tftp and write to flash\n"
	"fw		- get mcu fw by tftp and write to flash\n"
	"sw		- get software config by tftp and write to flash\n"
	"hw		- get hardware config by tftp and write to flash\n"
	"kernel		- get kernel image by tftp and write to flash\n"
	"rootfs		- get rootfs by tftp and write to flash\n"
	"ldc		- get ldc table by tftp and write to flash\n"
	"flash		- parse image in ram, and write it to flash\n"
	"all		- get all image by tftp , parse it and  write to flash\n"
	UPDATE_TEST_HELP
);
#endif
