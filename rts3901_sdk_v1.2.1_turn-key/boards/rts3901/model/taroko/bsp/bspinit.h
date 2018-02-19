/*
 * Realtek Semiconductor Corp.
 *
 * bsp/bspinit.h:
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#ifndef __BSPINIT_H_
#define __BSPINIT_H_

#include <asm/asm.h>
#include <bspcpu.h>

.macro  kernel_entry_setup
/* Enable CP3 */
mfc0	t0, CP0_STATUS
or	t0, ST0_CU3
mtc0	t0, CP0_STATUS
/* Configure IMEM & DMEM address */
li	t0, cpu_imem0_base
mtc3	t0, LXCP3_IWBASE0
li	t0, cpu_imem0_top
mtc3	t0, LXCP3_IWTOP0
li	t0, cpu_imem1_base
mtc3	t0, LXCP3_IWBASE1
li	t0, cpu_imem1_top
mtc3	t0, LXCP3_IWTOP1
li	t0, cpu_dmem0_base
mtc3	t0, LXCP3_DWBASE0
li	t0, cpu_dmem0_top
mtc3	t0, LXCP3_DWTOP0
li	t0, cpu_dmem1_base
mtc3	t0, LXCP3_DWBASE1
li	t0, cpu_dmem1_top
mtc3	t0, LXCP3_DWTOP1
/* Enable IMEM0 & DMEM0 */
mfc0	t0, LXCP0_CCTL, 0
and	t0, ~(CCTL_IMEM0ON | CCTL_DMEM0ON)
mtc0	t0, LXCP0_CCTL
or	t0, CCTL_IMEM0ON | CCTL_DMEM0ON
mtc0	t0, LXCP0_CCTL, 0
/* Enable IMEM1 & DMEM1 */
mfc0	t0, LXCP0_CCTL, 1
and	t0, ~(CCTL_IMEM1ON | CCTL_DMEM1ON)
mtc0	t0, LXCP0_CCTL
or	t0, CCTL_IMEM1ON | CCTL_DMEM1ON
mtc0	t0, LXCP0_CCTL, 1
.endm

.macro  smp_slave_setup
.endm

#endif
