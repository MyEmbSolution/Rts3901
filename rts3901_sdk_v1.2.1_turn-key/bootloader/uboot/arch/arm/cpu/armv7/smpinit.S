/*
 * Realtek Semiconductor Corp.
 *
 * smpinit.S: smp specific synchronization code.
 *
 * Viller Hsiao (villerhsiao@realtek.com)
 * Dec. 26, 2012
 */

#include <asm-offsets.h>
#include <config.h>
#include <version.h>
#include <asm/system.h>
#include <linux/linkage.h>

#ifdef CONFIG_SMP
#include <asm/smp-boot.h>

#if !defined(CONFIG_SYS_GIC_OFFSET)
#if defined(CONFIG_CPU_ARMA7) || defined(CONFIG_CPU_ARMA15)
#define CONFIG_SYS_GIC_OFFSET	(0x2000)
#else
#define CONFIG_SYS_GIC_OFFSET	(0x100)
#endif
#endif

/*
 * smp_setup
 *
 * It initialize SCU and jump to core0() to update its boot
 * status.
 */
ENTRY(smp_setup)
	/* SCU initialization */
	ldr	r2, =CONFIG_SYS_PERIPHERAL_PADDR + 0xc
	ldr	r1, =0xffff
	str	r1, [r2]		@ Invalidate SCU tag RAMS
	ldr	r2, =CONFIG_SYS_PERIPHERAL_PADDR
	ldr	r1, [r2]
	orr	r1, r1, #1
	str	r1, [r2]		@ Enable SCU

core0:  /* Reset SMPBOOT area */
	ldr	r0, =SMPBOOT
	add	r1, r0, #(32*CONFIG_NR_CPUS)
	mov	r2, #0
1:	str	r2, [r0]
	add	r0, #4
	cmp	r0, r1
	bne	1b

	/* Set CPU online flag for core0 */
	ldr	r0, =SMPBOOT
	ldr	r1, =(SMP_LAUNCH_FREADY|SMP_LAUNCH_FGO|SMP_LAUNCH_FGONE)
	str	r1, [r0, #SMP_LAUNCH_FLAGS]

	/* Return */
	mov	pc, lr
ENDPROC(smp_setup)

/*
 * coren
 *
 * Update boot status for other cpus and do synchronization
 * with core0. It will be relocated from ROM to RAM.
 */
ENTRY(coren)
	/* Wait for core 0 ready */
	ldr	r0, =SMPBOOT
1:	ldr	r1, [r0, #SMP_LAUNCH_FLAGS]
	and	r1, #SMP_LAUNCH_FGONE
	cmp	r1, #0
	beq	1b

	/* Each SMPBOOT struct is 32 byte wide */
	mrc	p15, 0, r2, c0, c0, 5
	and	r2, r2, #0xf
	lsl	r1, r2, #LOG2CPULAUNCH
	add	r0, r1

	/* Set CPU online flag */
	ldr	r1, [r0, #SMP_LAUNCH_FLAGS]
	orr	r1, #SMP_LAUNCH_FREADY
	str	r1, [r0, #SMP_LAUNCH_FLAGS]

	/* Enable GIC for waking up from sleep */
	movt	r3, #(CONFIG_SYS_PERIPHERAL_PADDR >> 16)
	orr	r2, r3, #CONFIG_SYS_GIC_OFFSET
	mov	r1, #1
	str	r1, [r2]
	dsb
	wfe					@ Wait for signal from primary CPU

	/* Poll CPU go flag */
2:	ldr	r1, [r0, #SMP_LAUNCH_FLAGS]	@ r1: SMP_LAUNCH_FLAGS
	and	r2, r1, #SMP_LAUNCH_FGO
	cmp	r2, #0
	beq	2b

	/* We're off. Disable gic */
	movt	r3, #(CONFIG_SYS_PERIPHERAL_PADDR >> 16)
	orr	r2, r3, #CONFIG_SYS_GIC_OFFSET
	mov	r3, #0
	str	r3, [r2]
	dsb

	/* Change CPU status */
	ldr	r2, [r0, #SMP_LAUNCH_PC]
	orr	r1, #SMP_LAUNCH_FGONE
	str	r1, [r0, #SMP_LAUNCH_FLAGS]
	mov	r0, r2				@ r0: startup routine in kernel

	/* Invalidate dcache */
	mov	r3, #0
	mcr	15, 2, r3, c0, c0, 0		@ CSSELR: data cache, level 0
	mrc	15, 1, r2, c0, c0, 0		@ r2: CCSIDR
	mvn	r3, #0				@ r3: 0xffffffff
	and	r4, r2, #7			@ r4: CCSIDR[Line size]
	ubfx	ip, r2, #13, #15		@ ip: CCSIDR[NumSets]
	add	r4, r4, #4
	ubfx	r2, r2, #3, #10			@ r2: CCSIDR[way]
	add	r6, r2, #1			@ r6: CCSIDR[way] + 1
	mov	r1, r6				@ r1: CCSIDR[way] + 1
1:	add	r3, r3, #1			@ Calculate log_2_n_round_up(num_ways)
	lsrs	r1, r1, #1
	bne	1b
	tst	r6, r2
	addne	r3, r3, #1
	rsb	r3, r3, #32
2:	mov	r1, ip				@ Loop by each set/way
	lsl	r6, r2, r3
3:	orr	r5, r6, r1, lsl r4
	mcr	15, 0, r5, c7, c6, 2		@ Invalidate D-cache by set/way
	subs	r1, r1, #1
	bcs	3b
	subs	r2, r2, #1
	bcs	2b
	dsb

	/* Go to secondary startup in kernel */
	mov	pc, r0

	.globl coren_end
coren_end:
ENDPROC(coren)

/*
 * text_copy
 *
 * Copy coren loop function from ROM to RAM
 */
ENTRY(text_copy)
	ldr	r0, =CORENLOOP
	ldr	r1, =coren
	ldr	r2, =coren_end
1:	ldr	r3, [r1]
	str	r3, [r0]
	add	r0, #4
	add	r1, #4
	cmp	r1, r2
	ble	1b
	mov	pc, lr
ENDPROC(text_copy)

#endif /* CONFIG_SMP */
