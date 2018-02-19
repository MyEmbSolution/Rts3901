/*
 * Realtek Semiconductor Corp.
 *
 * bsp/bspinit.h:
 *
 * Copyright (C) 2006-2012  Tony Wu (tonywu@realtek.com)
 */
#ifndef __BSPINIT_H_
#define __BSPINIT_H_

#include <asm/gcmpregs.h>
#include <asm/smp-boot.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <config.h>
.text
	.globl smp_setup
	.set noreorder
	.ent smp_setup
smp_setup:

	/*
	 * Change the CCA from non-coherent to coherent
	 * Assumption: GCMP => coherent CCA is usable
	 */
	and     t2,~M_ConfigK0
	or      t2,K_CacheAttrCCS<<S_ConfigK0
	mtc0    t2, C0_Config

	/* Check the CPU number */
	mfc0	v1, CP0_PRID, 1
	andi	v1, 0x3ff
#1:

	/* Initialise CM */
#define	GCMP	t0

	/* Rewrite the GCMPB register to initialise CM_DEFAULT_TARGET */
	li	GCMP, GCMP_BASE
	bnez    v1, 1f
	nop

	lw	t1, GCMPGCBOFS(GCMPB)(GCMP)
	or	t1, GCMP_GCB_GCMPB_CMDEFTGT_MEM<<GCMP_GCB_GCMPB_CMDEFTGT_SHF
	sw	t1, GCMPGCBOFS(GCMPB)(GCMP)

	/* Disable the CM regions */
	lui	t1, 0xffff
	sw	t1, GCMP_GCB_CMxBASE_OFS(0)(GCMP)
	sw	t1, GCMP_GCB_CMxMASK_OFS(0)(GCMP)
	sw	t1, GCMP_GCB_CMxBASE_OFS(1)(GCMP)
	sw	t1, GCMP_GCB_CMxMASK_OFS(1)(GCMP)
	sw	t1, GCMP_GCB_CMxBASE_OFS(2)(GCMP)
	sw	t1, GCMP_GCB_CMxMASK_OFS(2)(GCMP)
	sw	t1, GCMP_GCB_CMxBASE_OFS(3)(GCMP)
	sw	t1, GCMP_GCB_CMxMASK_OFS(3)(GCMP)
1:
	/* Allow interventions from all other cores (including self) */
	li	t1, 0xff
	sw	t1, GCMPCLCBOFS(COHCTL)(GCMP)
	sll zero,zero,3	/* FIXME: Other hazards? */

	beqz	v1, core0
	nop
	li	t0, CORENLOOP
	jr	t0
	nop
	.end smp_setup

	.globl coren
	.globl coren_end
	.section ".text.smploop", "ax"
	.set noreorder
	.ent coren

coren:
	la	t0, SMPBOOT
initloop:
	lw	t1, SMP_LAUNCH_FLAGS(t0)
	andi	t1, SMP_LAUNCH_FGONE
	beqz	t1, initloop
	nop

	/* each SMPBOOT struct is 32 byte wide */
	sll	t1, v1, 5
	addu	t0, t1

	/* set CPU online flag */
	lw	t1, SMP_LAUNCH_FLAGS(t0)
	ori	t1, SMP_LAUNCH_FREADY
	sw	t1, SMP_LAUNCH_FLAGS(t0)

	/*
	 * enable count interrupt in mask
	 * but don't enable interrupts yet
	 */
	mfc0	t2, CP0_STATUS
	li	t1, M_StatusIM2 /* FIXME should calculate dynamically from Config3.ippti */
	or	t1, t2
	mtc0	t1, CP0_STATUS

waitcode_start:
	/*
	 * Poll CPU go flag
	 */
1:      mfc0    t1,C0_Count
	addu    t1,SMP_LAUNCH_PERIOD
	mtc0    t1,C0_Compare

swwait: /* Software wait */
	mfc0    t4,C0_Count
	subu    t4,t1
	bltz    t4,swwait
	nop
	b       checklaunch
	nop

checklaunch:
	lw	t1, SMP_LAUNCH_FLAGS(t0)
	andi	t1, SMP_LAUNCH_FGO
	beqz	t1, 1b
	nop

	/* Reset the counter and interrupts to give naive clients a chance */
	mtc0	t2, CP0_STATUS
	mfc0	t2, CP0_COUNT
	subu	t2, 1
	mtc0	t2, CP0_COMPARE

	/* we're off */
	lw	t2, SMP_LAUNCH_PC(t0)
	lw	gp, SMP_LAUNCH_GP(t0)
	lw	sp, SMP_LAUNCH_SP(t0)
	lw	a0, SMP_LAUNCH_A0(t0)
	move	a1, zero
	move	a2, zero
	move	a3, zero
	ori	t1, SMP_LAUNCH_FGONE
	jr	t2
	sw	t1, SMP_LAUNCH_FLAGS(t0)
coren_end:
	.end coren

.text
	.set noreorder
	.ent core0
core0:
	la	t0, SMPBOOT
	addiu	t1, t0, (32*NR_CPUS - 4)
1:
	sw	zero, 0(t0)
	bne	t0, t1, 1b
	add     t0, 4

	/* set CPU online flag */
	la	t0, SMPBOOT
	li	t1, SMP_LAUNCH_FREADY|SMP_LAUNCH_FGO|SMP_LAUNCH_FGONE
	sw	t1, SMP_LAUNCH_FLAGS(t0)
	jr	ra
	nop
	.end core0

#endif
