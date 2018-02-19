/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <serial.h>
#include <stdio_dev.h>
#include <version.h>
#include <net.h>
#include <environment.h>
#include <nand.h>
#include <onenand_uboot.h>
#include <spi.h>
#include <dma.h>

#ifdef CONFIG_BITBANGMII
#include <miiphy.h>
#endif

#ifdef TIMER_TEST
#include <asm/rlxregs.h>
#endif

DECLARE_GLOBAL_DATA_PTR;
#define DATA_MASK_PATTERN	0x01100000
ulong monitor_flash_len;

#define TEST32(addr)	(*(volatile u32 *)(addr))

const unsigned int POLY32 = 175; /*POLY= 2^32 + 2^7 + 2^5 + 2^3 + 2^2 + 2^1 + 1*/



static char *failed = "*** failed ***\n";

/*
 * mips_io_port_base is the begin of the address space to which x86 style
 * I/O ports are mapped.
 */
const unsigned long mips_io_port_base = -1;

int __board_early_init_f(void)
{
	/*
	 * Nothing to do in this dummy implementation
	 */
	return 0;
}
int board_early_init_f(void)
	__attribute__((weak, alias("__board_early_init_f")));

#ifdef _MEM_TEST1_
static int mem_test1(void)
{
	u32 base_addr, i, j, count = 0;
	u32 repet = 1, pass_num = 0;
	u32 step = 0x800000;
	base_addr = 0xa1c00000;
	u32 tmp_spi, tmp_ddr, tmp2_spi, tmp2_ddr;

	printf("sample edge : %x\n", TEST32(0xb8860070));
	TEST32(0xb8800000) = 0xff;
	TEST32(0xb8800008) = 0;


	while (1) {
		for (base_addr = 0xa1c00000; base_addr < 0xaf000000; base_addr = base_addr+step) {

			printf("=====memtest, base_addr:%x=====\n", base_addr);
			printf("1. memtest, single wirte\n");
			for (i = 0; i < step; i = i+4) {
				TEST32(base_addr + i) = TEST32(0xBC400000 + i);
			}

			printf("2. memtest, single read\n");
			for (i = 0; i < step; i = i+4) {
				tmp_ddr = TEST32(base_addr + i);
				tmp_spi = TEST32(0xBC400000 + i);
				if (tmp_ddr != tmp_spi) {
					TEST32(0xb8800008) = 0xff;
					printf("before: ddr address %x, ddr %x, flash addr %x, flash %x\n", (base_addr + i - 4), tmp2_ddr, 0xBC400000 + i - 4, tmp2_spi);
					printf("error: ddr address %x, ddr %x, flash addr %x, flash %x\n", (base_addr + i), tmp_ddr, 0xBC400000 + i , tmp_spi);
					count += 1;

					if (count > 3)
						while (1);
				}
				tmp2_ddr = tmp_ddr;
				tmp2_spi = tmp_spi;
			}

			printf("3. memtest, burst read/write\n");
			for (i = 0; i < step; i = i+0x4000) {
				dma_copy(0x1C400000 + i, 0x18910000, 0x4000);
				dma_copy(0x18910000, (base_addr&0x0FFFFFFF) + i + step, 0x4000);
				for (j = 0; j < 0x4000; j = j+4) {
					tmp_ddr = TEST32(base_addr + step + i + j);
					tmp_spi = TEST32(0xa0f00000 + j);
					if (tmp_ddr != tmp_spi)	{
						TEST32(0xb8800008) = 0xff;
						printf("burst write before: address %x, dmem %x, ram %x\n", (base_addr + i + j + step - 4), tmp2_spi, tmp2_ddr);
						printf("burst write error: address %x, dmem %x, ram %x\n", (base_addr + i + j + step), tmp_spi, tmp_ddr);
						count += 1;
					}
					tmp2_ddr = tmp_ddr;
					tmp2_spi = tmp_spi;
				}
			}
		}

		if (count == 0) {
			pass_num += 1;
			printf("!!!!!mem test --%x OK!!!!!\n", repet);
		} else
			printf("!!!!!mem test --%x FAIL!!!!!\n", repet);

		printf("!!!!!%x of %xPASS !!!!!\n", pass_num, repet);
		repet++;
		count = 0;
       }
	return 0;
}
#endif

#if ((defined _MEM_TEST2_) || (defined _MEM_TEST3_))
static u32 random(void)
{
	unsigned long long rand ;
	rand = get_counter() * 100000;
	rand = rand * 1664525L + 1013904223L;
	return rand;
}

static void gen_random_data(void)
{
	u32 i;
	for (i = 0; i < 0x1000; i = i+4) {
		TEST32(0xa0f00000 + i) = (random() | DATA_MASK_PATTERN);
	}
}

static void gen_fixpattern_data(void)
{
	u32 i;
	u32 data[10] = {0, 0xFFFFFFFF, 0xAAAAAAAA, 0x55555555, 0xA5A5A5A5, 0x5A5A5A5A, 0xFF0000FF, 0xFFFF00, 0xAA5555AA, 0x55AAAA55};
	u32 fixdata[] = {
		0x00000000,
		0x00000000,
		0xFFFFFFFF,
		0xFFFFFFFF,

		0x00000000,
		0x00000000,
		0xFFFFFFFF,
		0xFFFFFFFF,

		0x55555555,
		0x55555555,
		0xAAAAAAAA,
		0xAAAAAAAA,

		0x55555555,
		0x55555555,
		0xAAAAAAAA,
		0xAAAAAAAA,

		0xA5A5A5A5,
		0xA5A5A5A5,
		0x5A5A5A5A,
		0x5A5A5A5A,

		0xA5A5A5A5,
		0xA5A5A5A5,
		0x5A5A5A5A,
		0x5A5A5A5A,

		0xFF0000FF,
		0xFF0000FF,
		0x00FFFF00,
		0x00FFFF00,

		0xFF0000FF,
		0xFF0000FF,
		0x00FFFF00,
		0x00FFFF00,

		0xAA5555AA,
		0xAA5555AA,
		0x55AAAA55,
		0x55AAAA55,

		0xAA5555AA,
		0xAA5555AA,
		0x55AAAA55,
		0x55AAAA55};

	for (i = 0; i < sizeof(fixdata); i = i+4) {
		TEST32(0xa0f00000 + i) = (fixdata[i/4] | DATA_MASK_PATTERN);
	}

	for (; i < 0x1000; i = i+4) {
		TEST32(0xa0f00000 + i) = (data[(random())%10] | DATA_MASK_PATTERN);
	}
}

static int data_compare(void)
{
	u32 i, base_addr;
	u32 tmp, tmp1, count = 0;
	u32 len;

	/*single write/read test*/
	base_addr = min(((random())&0xFFFFFFC)|0xa0000000, 0xaffc0000);
	base_addr = max(0xa1008000, base_addr);
	len = ((random())&0xFFC);

	printf("single write, addr=%x, len=%x\n", base_addr, len);
	for (i = 0; i < len; i = i+4) {
		TEST32(base_addr  + i) = TEST32(0xa0f00000 + i);
	}

	for (i = 0; i < len; i = i+4) {
		tmp = TEST32(base_addr  + i);
		if (tmp != TEST32(0xa0f00000 + i)) {
			TEST32(0xb8800008) = 0xff;
			count++;
			printf("single write before: addr %x, ddr %x, dmem_offset %x, dmem %x \n", base_addr+i-4, tmp1, i-4, TEST32(0xa0f00000 + i - 4));
			printf("single write error: addr %x, ddr %x, dmem_offset %x, dmem %x \n", base_addr+i, tmp, i, TEST32(0xa0f00000 + i));
			while (1);
		}
		tmp1 = tmp;
	}

	printf("single read\n");
	for (i = 0; i < len; i = i+4) {
		TEST32(0xa0f01000 + i) = TEST32(base_addr  + i);
	}

	for (i = 0; i < len; i = i+4) {
		if (TEST32(0xa0f00000 + i) != TEST32(0xa0f01000 + i)) {
			TEST32(0xb8800008) = 0xff;
			count++;
			printf("single read before: addr %x, ddr %x, dmem_offset %x, dmem %x \n", base_addr+i-4, TEST32(base_addr  + i - 4), i-4, TEST32(0xa0f01000 + i-4));
			printf("single read error: addr %x, ddr %x, dmem_offset %x, dmem %x \n", base_addr+i, TEST32(base_addr  + i), i, TEST32(0xa0f01000 + i));
			while (1);
		}
	}

	/*burst write/read test*/
	base_addr = min(((random())&0xFFFFFFC)|0xa0000000, 0xaffc0000);
	base_addr = max(0xa1008000, base_addr);
	len = ((random())&0xFFC);
	printf("burst write, addr=%x len=%x\n", base_addr, len);
	dma_copy(0x18910000, (base_addr&0x0FFFFFFF), len);
	for (i = 0; i < len; i = i+4) {
		tmp = TEST32(base_addr  + i);
		if (tmp != TEST32(0xa0f00000 + i)) {
			TEST32(0xb8800008) = 0xff;
			count++;
			printf("burst write before: addr %x, ddr %x, dmem_offset %x, dmem %x \n", base_addr+i-4, tmp1, i-4, TEST32(0xa0f00000 + i-4));
			printf("burst write error: addr %x, ddr %x, dmem_offset %x, dmem %x \n", base_addr+i, tmp, i, TEST32(0xa0f00000 + i));
			while (1);
		}
		tmp1 = tmp;
	}

	printf("burst read\n");
	dma_copy((base_addr&0x0FFFFFFF), 0x18911000, 0x1000);
	for (i = 0; i < len; i = i+4) {
		tmp = TEST32(base_addr  + i);
		if (tmp != TEST32(0xa0f01000 + i)) {
			TEST32(0xb8800008) = 0xff;
			count++;
			printf("burst read before: addr %x, ddr %x, dmem_offset %x, dmem %x \n", base_addr+i-4, tmp1, i-4, TEST32(0xa0f01000 + i-4));
			printf("burst read error: addr %x, ddr %x, dmem_offset %x, dmem %x \n", base_addr+i, tmp, i, TEST32(0xa0f01000 + i));
			while (1);
		}
	}
	return count;
}

static int random_test(void)
{
	u32 count = 0;

	gen_random_data();
	count = data_compare();

	return count;
}



static int fixpattern_test(void)
{
	u32 count = 0;

	gen_fixpattern_data();
	count = data_compare();

	return count;
}


static int mem_test2(void)
{
	u32 count = 0;
	u32 repet = 1, pass_num = 0;

	printf("begin memory test\n");

	/*set GPIO 0~7 output enable , and set it to 0*/
	TEST32(0xb8800000) = 0xff;
	TEST32(0xb8800008) = 0;
#if 0
		printf("write test\n");
		i = 0xa2000000;


		TEST32(i) = 0x5a5a5a5a;
		TEST32(i + 4) = 0x5a5a5a5a;
		TEST32(i + 8) = 0x5a5a5a5a;
		TEST32(i + 12) = 0x5a5a5a5a;

		TEST32(i + 16) = 0xa5a5a5a5;
		TEST32(i + 20) = 0xa5a5a5a5;
		TEST32(i + 24) = 0xa5a5a5a5;
		TEST32(i + 28) = 0xa5a5a5a5;

		TEST32(i + 32) = 0x5a5a5a5a;
		TEST32(i + 36) = 0x5a5a5a5a;
		TEST32(i + 40) = 0x5a5a5a5a;
		TEST32(i + 44) = 0x5a5a5a5a;

		while (1) {

			tmp = TEST32(i);
			tmp = TEST32(i + 4);
			tmp = TEST32(i + 8);
			tmp = TEST32(i + 12);

			tmp = TEST32(i + 16);
			tmp = TEST32(i + 20);
			tmp = TEST32(i + 24);
			tmp = TEST32(i + 28);
		}
#else
	while (1) {
		count += fixpattern_test();
		count += random_test();

		if (count == 0) {
			pass_num += 1;
			printf("!!!!!mem test --%x OK!!!!!\n", repet);
		} else
			printf("!!!!!mem test --%x FAIL!!!!!\n", repet);

		printf("!!!!!%x of %x PASS !!!!!\n", pass_num, repet);
		printf("\n");
		repet++;
		count = 0;
	}
	return 0;
#endif
}
#endif

#ifdef _MEM_TEST3_
static u32 rand32(u32 seed_tmp)
{
	u32 seed_value;
	seed_value = (seed_tmp << 1) ^ ((seed_tmp & 0x80000000) ? POLY32 : 0);
	return seed_value;
}

static int mem_test3(void)
{
	u32 i, base_addr;
	u32 tmp, count = 0;
	u32 seed, burn_in_len = 0x10000;

	printf("new random test\n");

	/*set GPIO 0~7 output enable , and set it to 0*/
	TEST32(0xb8800000) = 0xff;
	TEST32(0xb8800008) = 0;
	while (1) {
		fixpattern_test();
		random_test();

		/*write uncacheable DDR, and read uncacheable DDR*/
		base_addr = min(((random())&0xFFFFFFC)|0xa0000000, (0xb0000000 - burn_in_len));
		base_addr = max(0xa1008000, base_addr);
		printf("uncacheable, 0x%x\n", base_addr);
		seed = 0x12345678;
		for (i = 0; i < burn_in_len; i = i+4) {
			seed = rand32(seed) | DATA_MASK_PATTERN;
			TEST32(base_addr + i) = seed;
		}

		seed = 0x12345678;
		for (i = 0; i < burn_in_len; i = i+4) {
			seed = rand32(seed) | DATA_MASK_PATTERN;
			tmp = TEST32(base_addr + i);
			if (tmp != seed) {
				TEST32(0xb8800008) = 0x40;
				printf("ddr test fail, addr=%x, ddr=%x, golden=%x\n", base_addr + i, tmp, seed);
				while (1);
			}
		}

		/*write uncacheable DDR, and read cacheable DDR*/
		base_addr = min(((random())&0xFFFFFFC)|0xa0000000, (0xb0000000 - burn_in_len));
		base_addr = max(0xa1008000, base_addr);
		printf("cacheable, 0x%x\n", base_addr);
		seed = 0x5a5a5a5a;
		for (i = 0; i < burn_in_len; i = i+4) {
			seed = rand32(seed) | DATA_MASK_PATTERN;
			TEST32(base_addr + i) = seed;
		}

		seed = 0x5a5a5a5a;
		for (i = 0; i < burn_in_len; i = i+4) {
			seed = rand32(seed) | DATA_MASK_PATTERN;
			tmp = TEST32(base_addr + i - 0x20000000);
			if (tmp != seed) {
				printf("ddr test fail, addr=%x, ddr=%x, golden=%x\n", base_addr + i, tmp, seed);
				while (1);
			}
		}
		/*after read from cacheable DDR, do flush cache*/
		flush_cache(base_addr - 0x20000000, burn_in_len);

		if (!(count % 10))
			TEST32(0xb8800008) ^= 0x10;
		count++;
		printf("test number %x OK\n", count);
		printf("\n");
	}
}
#endif

static int init_func_ram(void)
{
#ifdef	CONFIG_BOARD_TYPES
	int board_type = gd->board_type;
#else
	int board_type = 0;	/* use dummy arg */
#endif
	puts("DRAM:  ");

	gd->ram_size = initdram(board_type);
	if (gd->ram_size > 0) {
		char dram_clk[32];
		snprintf(dram_clk, sizeof(dram_clk)/sizeof(dram_clk[0]),
				" @ %d MHz\n", get_dram_clock());
		print_size(gd->ram_size, dram_clk);
#ifdef _MEM_TEST1_
		mem_test1();
#endif
#ifdef _MEM_TEST2_
		mem_test2();
#endif
		return 0;
	}
	puts(failed);
	return 1;
}

static int display_banner(void)
{

	printf("\n\n%s\n\n", version_string);
	return 0;
}

static int display_ddr_test(void)
{

	printf("\n\n%s\n\n", ddrtest_string);
	return 0;
}

#ifndef CONFIG_SYS_NO_FLASH
static void display_flash_config(ulong size)
{
	puts("Flash: ");
	print_size(size, "\n");
}
#endif

static int init_baudrate(void)
{
	gd->baudrate = getenv_ulong("baudrate", 10, CONFIG_BAUDRATE);
	return 0;
}


/*
 * Breath some life into the board...
 *
 * The first part of initialization is running from Flash memory;
 * its main purpose is to initialize the RAM so that we
 * can relocate the monitor code to RAM.
 */

/*
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependend #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */
typedef int (init_fnc_t)(void);

init_fnc_t *init_sequence[] = {
	board_early_init_f,
	timer_init,
	env_init,		/* initialize environment */
#ifdef CONFIG_INCA_IP
	incaip_set_cpuclk,	/* set cpu clock according to env. variable */
#endif
	init_baudrate,		/* initialize baudrate settings */
	serial_init,		/* serial communications setup */
	console_init_f,
#ifndef _MEM_TEST3_
	display_banner,		/* say that we are here */
	checkboard,
	init_func_ram,
#endif
	NULL,
};


init_fnc_t *init_sequence_ddrtest[] = {
	env_init,		/* initialize environment */
	init_baudrate,		/* initialize baudrate settings */
	serial_init,		/* serial communications setup */
	console_init_f,
	display_ddr_test,		/* say that we are here */
	NULL,
};

void init_bf_print(void)
{
	gd_t gd_data;
	init_fnc_t **init_fnc_ptr;
	gd = &gd_data;
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("" : : : "memory");

	memset((void *)gd, 0, sizeof(gd_t));

	for (init_fnc_ptr = init_sequence_ddrtest; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0)
			hang();
	}
}

void board_init_f(ulong bootflag)
{
	gd_t gd_data, *id;
	bd_t *bd;
	init_fnc_t **init_fnc_ptr;
	ulong addr, addr_sp, len;
	ulong *s;

	/* Pointer is writable since we allocated a register for it.
	 */
	gd = &gd_data;
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("" : : : "memory");

	memset((void *)gd, 0, sizeof(gd_t));

	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0)
			hang();
	}

#ifdef TIMER_TEST
	/* some register about GPIO for led */
#define XB2_SHARE_GPIO_SELECT	0xB880004C
#define XB2_SHARE_GPIO_OE	0xB8800050
#define XB2_SHARE_GPIO_O	0xB8800058
	/*********************timer test*****************/
	printf("timer test begin!\n");
	timer_init();
	/* init GPIO */
	TEST32(XB2_SHARE_GPIO_SELECT) = 1 << 1;
	TEST32(XB2_SHARE_GPIO_OE) = 1 << 1;
	unsigned int cause;
	unsigned int count = 0;
	while(1) {
		cause = read_c0_cause();
		if (cause & (1 << 30)) {
			write_c0_compare(read_c0_compare() + CONFIG_TIMER_2);
			printf("%d s passed\n", count);
			TEST32(XB2_SHARE_GPIO_O) ^= 1 << 1;
			count += 1;
		}
	}
	/*********************timer test end*************/
#endif

	/*
	 * Now that we have DRAM mapped and working, we can
	 * relocate the code and continue running from DRAM.
	 */
	addr = CONFIG_SYS_SDRAM_BASE + gd->ram_size;

	/* We can reserve some RAM "on top" here.
	 */

	/* round down to next 4 kB limit.
	 */
	addr &= ~(4096 - 1);
	debug("Top of RAM usable for U-Boot at: %08lx\n", addr);

	/* Reserve memory for U-Boot code, data & bss
	 * round down to next 16 kB limit
	 */
	len = bss_end() - CONFIG_SYS_MONITOR_BASE;
	addr -= len;
	addr &= ~(16 * 1024 - 1);

	debug("Reserving %ldk for U-Boot at: %08lx\n", len >> 10, addr);

	 /* Reserve memory for malloc() arena.
	 */
	addr_sp = addr - TOTAL_MALLOC_LEN;
	debug("Reserving %dk for malloc() at: %08lx\n",
			TOTAL_MALLOC_LEN >> 10, addr_sp);

	/*
	 * (permanently) allocate a Board Info struct
	 * and a permanent copy of the "global" data
	 */
	addr_sp -= sizeof(bd_t);
	bd = (bd_t *)addr_sp;
	gd->bd = bd;
	debug("Reserving %zu Bytes for Board Info at: %08lx\n",
			sizeof(bd_t), addr_sp);

	addr_sp -= sizeof(gd_t);
	id = (gd_t *)addr_sp;
	debug("Reserving %zu Bytes for Global Data at: %08lx\n",
			sizeof(gd_t), addr_sp);

	/* Reserve memory for boot params.
	 */
	addr_sp -= CONFIG_SYS_BOOTPARAMS_LEN;
	bd->bi_boot_params = addr_sp;
	debug("Reserving %dk for boot params() at: %08lx\n",
			CONFIG_SYS_BOOTPARAMS_LEN >> 10, addr_sp);

	/*
	 * Finally, we set up a new (bigger) stack.
	 *
	 * Leave some safety gap for SP, force alignment on 16 byte boundary
	 * Clear initial stack frame
	 */
	addr_sp -= 16;
	addr_sp &= ~0xF;
	s = (ulong *)addr_sp;
	*s-- = 0;
	*s-- = 0;
	addr_sp = (ulong)s;
	debug("Stack Pointer at: %08lx\n", addr_sp);

	/*
	 * Save local variables to board info struct
	 */
	bd->bi_memstart	= CONFIG_SYS_SDRAM_BASE;	/* start of DRAM */
	bd->bi_memsize	= gd->ram_size;		/* size of DRAM in bytes */
	bd->bi_baudrate	= gd->baudrate;		/* Console Baudrate */

	memcpy(id, (void *)gd, sizeof(gd_t));

#ifdef _IMEM_DMA_TEST_
	printf("IMEM fill, DMA test\n");
	dma_copy(0, 0x00010000, 0x2000);
	printf("dma copy end\n");
	while(1);
#endif
#ifdef _MEM_TEST3_
	dma_copy(0x1fc00000, 0x18900000, 0x8000);
	dma_copy(0x18900000, 0x0, 0x8000);
	printf("spi, %x ,ddr %x\n", TEST32(0xbfc00000), TEST32(0x80000000));
	relocate_code(addr_sp, id, 0x81000000);
#else
#ifdef CONFIG_RAM_VERSION
	set_sp(addr_sp, id, addr);
#else
	relocate_code(addr_sp, id, addr);
#endif
#endif

	/* NOTREACHED - relocate_code() does not return */
}

#ifdef INTERRUPT_TEST_SPIC
void printf_spic_status(void)
{
	printf("IMR: 0x%lx, ISR: 0x%lx, TXFTLR: 0x%lx, RXFTLR: 0x%lx, RISR: 0x%lx.\n",
			(unsigned long)TEST32(0xb803002c),
			(unsigned long)TEST32(0xb8030030),
			(unsigned long)TEST32(0xb8030018),
			(unsigned long)TEST32(0xb803001c),
			(unsigned long)TEST32(0xb8030034));
	printf("ICR: 0x%lx\n", (unsigned long)TEST32(0xb8030048));
}
#endif
/*
 * This is the next part if the initialization sequence: we are now
 * running from RAM and have a "normal" C environment, i. e. global
 * data can be written, BSS has been cleared, the stack size in not
 * that critical any more, etc.
 */

void board_init_r(gd_t *id, ulong dest_addr)
{
#ifndef CONFIG_SYS_NO_FLASH
	ulong size;
#endif
	bd_t *bd;

#ifdef _MEM_TEST3_
		printf("Now Run DDR burn in Test in IMEM\n");
		mem_test3();
#else

	/* init timer */
	timer_init();

	gd = id;
	gd->flags |= GD_FLG_RELOC;	/* tell others: relocation done */

	debug("Now running in RAM - U-Boot at: %08lx\n", dest_addr);


	gd->reloc_off = dest_addr - CONFIG_SYS_MONITOR_BASE;

	monitor_flash_len = image_copy_end() - dest_addr;

	serial_initialize();

	bd = gd->bd;

	/* The Malloc area is immediately below the monitor copy in DRAM */
	mem_malloc_init(CONFIG_SYS_MONITOR_BASE + gd->reloc_off -
			TOTAL_MALLOC_LEN, TOTAL_MALLOC_LEN);

#ifndef CONFIG_SYS_NO_FLASH
	/* configure available FLASH banks */
	size = flash_init();
	display_flash_config(size);
	bd->bi_flashstart = CONFIG_SYS_FLASH_BASE;
	bd->bi_flashsize = size;

#if CONFIG_SYS_MONITOR_BASE == CONFIG_SYS_FLASH_BASE
	bd->bi_flashoffset = monitor_flash_len;	/* reserved area for U-Boot */
#else
	bd->bi_flashoffset = 0;
#endif
#else
	bd->bi_flashstart = 0;
	bd->bi_flashsize = 0;
	bd->bi_flashoffset = 0;
#endif

#ifdef CONFIG_CMD_NAND
	puts("NAND:  ");
	nand_init();		/* go init the NAND */
#endif

#if defined(CONFIG_CMD_ONENAND)
	onenand_init();
#endif

	/* relocate environment function pointers etc. */
	env_relocate();

#ifdef INTERRUPT_TEST_TIMER
	/*******************timer interrupt test********************/
	__asm__ __volatile__ (
	"	.set mips3		\n"
	/* push stack */
	"	subu	$29, 8		\n"
	"	sw	$8, 4($29)	\n"
	"	sw	$9, 0($29)	\n"
	/* set interrupt vector */
	"	li	$8, 0x80000400	\n"
	"	mtlxc0	$8, $2		\n"
	/* init GPIO */
	"	li	$8, 0xb880004c	\n"
	"	li	$9, 0x2		\n"
	"	sw	$9, 0($8)	\n"
	"	li	$8, 0xb8800050	\n"
	"	li	$9, 0x2		\n"
	"	sw	$9, 0($8)	\n"
	/* enable timer interrupt */
	"	li	$8, 1 << 23	\n"
	"	mtlxc0	$8, $0		\n"
	"	nop			\n"
	"	mfc0	$8, $12		\n"
	"	and	$8, ~(0xff << 8)\n"
	"	or	$8, 0x1		\n"
	"	mtc0	$8, $12		\n"
	/* pop stack */
	"	lw	$8, 4($29)	\n"
	"	lw	$9, 0($29)	\n"
	"	addu	$29, 8		\n"
	);
	printf("\ntimer interrupt enabled\n\n");
#endif
#ifdef INTERRUPT_TEST_SPIC
	/*******************spic interrupt test********************/
	__asm__ __volatile__ (
	"	.set mips3		\n"
	/* push stack */
	"	subu	$29, 8		\n"
	"	sw	$8, 4($29)	\n"
	"	sw	$9, 0($29)	\n"
	/* enable SPIC interrupt */
	"	li	$8, 0xb803002c	\n"
	"	li	$9, 0x7f	\n"
	"	sw	$9, 0($8)	\n"
	"	mfc0	$8, $12		\n"
	"	and	$8, ~(0xff << 8)\n"
	"	or	$8, 0x2001	\n"
	"	mtc0	$8, $12		\n"
	"	nop			\n"
	/* pop stack */
	"	lw	$8, 4($29)	\n"
	"	lw	$9, 0($29)	\n"
	"	addu	$29, 8		\n"
	);
	printf_spic_status();
	printf("spic interrupt enabled\n");
	/*************************************************/
#endif

#if defined(CONFIG_PCI)
	/*
	 * Do pci configuration
	 */
	pci_init();
#endif

/** leave this here (after malloc(), environment and PCI are working) **/
	/* Initialize stdio devices */
	stdio_init();

	jumptable_init();

	/* Initialize the console (after the relocation and devices init) */
	console_init_r();
/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

	/* Initialize from environment */
	load_addr = getenv_ulong("loadaddr", 16, load_addr);

#ifdef CONFIG_CMD_SPI
	puts("SPI:   ");
	spi_init();		/* go init the SPI */
	puts("ready\n");
#endif

#if defined(CONFIG_MISC_INIT_R)
	/* miscellaneous platform dependent initialisations */
	misc_init_r();
#endif

#ifdef CONFIG_BITBANGMII
	bb_miiphy_init();
#endif
#if defined(CONFIG_CMD_NET)
	puts("Net:   ");
	eth_initialize(gd->bd);
#endif

	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;)
		main_loop();
#endif
	/* NOTREACHED - no way out of command loop except booting */
}
