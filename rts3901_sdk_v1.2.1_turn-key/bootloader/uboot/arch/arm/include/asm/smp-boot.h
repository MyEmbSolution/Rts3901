/*
 * Realtek Semiconductor Corp.
 *
 * smpboot.h: smp specific synchronization status definition.
 *
 * Viller Hsiao (villerhsiao@realtek.com)
 * Dec. 26, 2012
 */

#ifndef _SMP_BOOT_H_
#define _SMP_BOOT_H_

#define	LOG2CPULAUNCH		5
#define	SMP_LAUNCH_PC		0
#define	SMP_LAUNCH_GP		4
#define	SMP_LAUNCH_SP		8
#define	SMP_LAUNCH_A0		12
#define	SMP_LAUNCH_FLAGS	28

#define	SMP_LAUNCH_FREADY	1
#define	SMP_LAUNCH_FGO		2
#define	SMP_LAUNCH_FGONE	4

#endif
