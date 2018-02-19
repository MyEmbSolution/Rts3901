/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, <wd@denx.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <netdev.h>
#include <asm/rlxregs.h>
#include <asm/cacheops.h>
#include <asm/reboot.h>

#define cache_op(op, addr)						\
	__asm__ __volatile__(						\
	"	.set	push					\n"	\
	"	.set	noreorder				\n"	\
	"	.set	mips3\n\t				\n"	\
	"	cache	%0, %1					\n"	\
	"	.set	pop					\n"	\
	:								\
	: "i" (op), "R" (*(unsigned char *)(addr)))

void __attribute__((weak)) _machine_restart(void)
{
}

int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	_machine_restart();

	fprintf(stderr, "*** reset failed ***\n");
	return 0;
}

static inline unsigned long icache_line_size(void)
{
	return CONFIG_SYS_CACHELINE_SIZE;
}

static inline unsigned long dcache_line_size(void)
{
	return CONFIG_SYS_CACHELINE_SIZE;
}


void flush_cache(ulong start_addr, ulong size)
{
	unsigned long lsize = CONFIG_SYS_CACHELINE_SIZE;
	unsigned long addr = start_addr & ~(lsize - 1);
	unsigned long aend = (start_addr + size - 1) & ~(lsize - 1);

	/* aend will be miscalculated when size is zero, so we return here */
	if (size == 0)
		return;

	while (1) {
		cache_op(HIT_WRITEBACK_INV_D, addr);
		cache_op(HIT_INVALIDATE_I, addr);
		if (addr == aend)
			break;
		addr += lsize;
	}
}

void flush_dcache_range(ulong start_addr, ulong stop)
{
	unsigned long lsize = CONFIG_SYS_CACHELINE_SIZE;
	unsigned long addr = start_addr & ~(lsize - 1);
	unsigned long aend = (stop - 1) & ~(lsize - 1);

	while (1) {
		cache_op(HIT_WRITEBACK_INV_D, addr);
		if (addr == aend)
			break;
		addr += lsize;
	}
}

void invalidate_dcache_range(ulong start_addr, ulong stop)
{
	unsigned long lsize = CONFIG_SYS_CACHELINE_SIZE;
	unsigned long addr = start_addr & ~(lsize - 1);
	unsigned long aend = (stop - 1) & ~(lsize - 1);

	while (1) {
		cache_op(HIT_INVALIDATE_D, addr);
		if (addr == aend)
			break;
		addr += lsize;
	}
}

int cpu_eth_init(bd_t *bis)
{
	return rtl8168_initialize(bis);
}
