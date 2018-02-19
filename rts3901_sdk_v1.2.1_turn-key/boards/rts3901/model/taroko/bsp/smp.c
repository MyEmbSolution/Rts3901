/*
 * Realtek Semiconductor Corp.
 *
 * bsp/smp.c
 *     bsp SMP initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#ifdef CONFIG_USE_UAPI
#include <generated/uapi/linux/version.h>
#else
#include <linux/version.h>
#endif
#include <linux/smp.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <asm/gcmpregs.h>
#include <asm/smp-ops.h>
#include <asm/mmcr.h>

#include "bspchip.h"

/*
 * Called in kernel/smp-*.c to do secondary CPU initialization.
 *
 * All platform-specific initialization should be implemented in
 * this function.
 *
 * Known SMP callers are:
 *     kernel/smp-cmp.c
 *
 * This function is called by the secondary CPU.
 */
void __cpuinit bsp_smp_init_secondary(void)
{
	change_c0_status(ST0_IM, 0xff00);
	change_lxc0_estatus(EST0_IM, 0xff0000);
}

/*
 * Called in bsp/setup.c to initialize SMP operations
 *
 * Depends on SMP type, bsp_smp_init calls corresponding
 * SMP operation initializer in arch/mips/kernel
 *
 * Known SMP types are:
 *     CONFIG_SMP_CMP
 */
void __init bsp_smp_init(void)
{
	register_cmp_smp_ops();
}
