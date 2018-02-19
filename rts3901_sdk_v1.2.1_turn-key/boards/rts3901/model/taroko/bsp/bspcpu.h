/*
 * Realtek Semiconductor Corp.
 *
 * bsp/bspcpu.h:
 *     bsp CPU and memory header file
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#ifndef __BSPCPU_H_
#define __BSPCPU_H_

#define cpu_scache_size     0
#define cpu_dcache_size     (16 << 10)
#define cpu_icache_size     (32 << 10)
#define cpu_scache_line     0
#define cpu_dcache_line     32
#define cpu_icache_line     32
#define cpu_tlb_entry       64
#define cpu_mem_size        (256 << 20)

#define cpu_imem_size       0
#define cpu_dmem_size       0
#define cpu_smem_size       0

#define cpu_imem0_base		0x18900000
#define cpu_imem0_top		0x18901fff
#define cpu_imem1_base		0x18902000
#define cpu_imem1_top		0x18903fff
#define cpu_dmem0_base		0x18930000
#define cpu_dmem0_top		0x18933fff
#define cpu_dmem1_base		0x18934000
#define cpu_dmem1_top		0x18937fff

#endif
