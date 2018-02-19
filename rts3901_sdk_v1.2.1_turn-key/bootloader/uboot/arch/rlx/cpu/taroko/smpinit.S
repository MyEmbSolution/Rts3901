/*
 * Realtek Semiconductor Corp.
 *
 * bsp/bspinit.h:
 *
 * Copyright (C) 2006-2012  Tony Wu (tonywu@realtek.com)
 */
#ifndef __BSPINIT_H_
#define __BSPINIT_H_

#include <asm/smp-boot.h>
#include <asm/rlxregs.h>
#include <asm/regdef.h>
#include <config.h>
#include <asm/mmcr.h>

.text
	.globl smp_setup
	.set noreorder
	.ent smp_setup
smp_setup:
	/* Check the CPU number */
	mfc0	v1, CP0_PRID
	li	v0, 0x30000
	and	v1, v0
	srl	v1, 16
	bnez	v1, 1f
	 nop
	/*
	 * Change the SMMU from non-coherent to coherent
	 */
	li	t2, MMCR_BASE

	li	t1, 0x5 << MMCR_SHF(SMMU_CCA, KSEG0)
	lw	t3, MMCR_SMMU_CCA_OFS(t2)
	or	t3, t1
	sw	t3, MMCR_SMMU_CCA_OFS(t2)

	/* Set all cores to coherent state */
	lw	t0, MMCR_CORE_OFS(t2)
	andi	t0, 0xf << MMCR_SHF(CORE, NUMCORES)
	srl	t0, MMCR_SHF(CORE, NUMCORES)

	li	t1, 0x1 << MMCR_SHF(CORESTATE, COHERENT)

	lw	t3, MMCR_CORESTATE0_OFS(t2)
	or	t3, t1
	sw	t3, MMCR_CORESTATE0_OFS(t2)
	addi	t0, -1
	beqz	t0, 1f
	 nop

	lw	t3, MMCR_CORESTATE1_OFS(t2)
	or	t3, t1
	sw	t3, MMCR_CORESTATE1_OFS(t2)
	addi	t0, -1
	beqz	t0, 1f
	 nop

	lw	t3, MMCR_CORESTATE2_OFS(t2)
	or	t3, t1
	sw	t3, MMCR_CORESTATE2_OFS(t2)
	addi	t0, -1
	beqz	t0, 1f
	 nop

	lw	t3, MMCR_CORESTATE3_OFS(t2)
	or	t3, t1
	sw	t3, MMCR_CORESTATE3_OFS(t2)
	addi	t0, -1
	beqz	t0, 1f
	 nop

1:
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

waitcode_start:
	/*
	 * Poll CPU go flag
	 */
1:	mfc0    t1,C0_Count
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
	add	t0, 4

	/* set CPU online flag */
	la	t0, SMPBOOT
	li	t1, SMP_LAUNCH_FREADY|SMP_LAUNCH_FGO|SMP_LAUNCH_FGONE
	sw	t1, SMP_LAUNCH_FLAGS(t0)

	/* Set all cores to online state */
	li	t2, MMCR_BASE

	lw	t0, MMCR_CORE_OFS(t2)
	andi	t0, 0xf << MMCR_SHF(CORE, NUMCORES)
	srl	t0, MMCR_SHF(CORE, NUMCORES)

	li	t1, 0x1 << MMCR_SHF(CORESTATE, ONLINE)

	lw	t3, MMCR_CORESTATE1_OFS(t2)
	or	t3, t1
	sw	t3, MMCR_CORESTATE1_OFS(t2)
	addi	t0, -1
	beqz	t0, 1f
	 nop

	lw	t3, MMCR_CORESTATE2_OFS(t2)
	or	t3, t1
	sw	t3, MMCR_CORESTATE2_OFS(t2)
	addi	t0, -1
	beqz	t0, 1f
	 nop

	lw	t3, MMCR_CORESTATE3_OFS(t2)
	or	t3, t1
	sw	t3, MMCR_CORESTATE3_OFS(t2)
	addi	t0, -1
	beqz	t0, 1f
	 nop
1:
	jr	ra
	 nop
	.end core0

#endif
