/*
 * Realtek Semiconductor Corp.
 *
 * bsp/irq-gic.c
 *   TarokoMP GIC interrupt initialization and handlers
 *
 * This file is part of BSP irq handlers, and will be
 * included by bsp/irq.c if CONFIG_IRQ_GIC is set.
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */

/*
 * Defined in <asm/gic.h>
 *
 * #define GIC_UNUSED             0xdead
 * #define GIC_FLAG_IPI           0x0001
 * #define GIC_FLAG_TRANSPARENT   0x0002
 * #define GIC_FLAG_SW0           0x0100
 * #define GIC_FLAG_SW1           0x0200
 * #define GIC_FLAG_TIMER         0x0400
 *
 * struct gic_intr_map {
 *       unsigned int cpunum;
 *       unsigned int pin;
 *       unsigned int polarity; not used
 *       unsigned int trigtype; not used
 *       unsigned int flags;
 * };
 *
 * GIC functions are moved to arch/rlx/kernel/irq-gic.c
 */
#define X GIC_UNUSED
static struct gic_intr_map __initdata gic_intr_map[GIC_NUM_INTRS] = {
	/* IRQ */
	{ /*00*/ X,          X,            X, X, 0 },
	{ /*01*/ X,          X,            X, X, 0 },
	{ /*02*/ X,          X,            X, X, 0 },
	{ /*03*/ X,          X,            X, X, 0 },
	{ /*04*/ X,          X,            X, X, 0 },
	{ /*05*/ GIC_CPU0,   GIC_CPU_INT5, X, X, GIC_FLAG_TRANSPARENT }, /*GMAC*/
	{ /*06*/ X,          X,            X, X, 0 },
	{ /*07*/ X,          X,            X, X, 0 },
	{ /*08*/ X,          X,            X, X, 0 },
	{ /*09*/ GIC_CPU0,   GIC_CPU_VEC1, X, X, GIC_FLAG_TRANSPARENT }, /*PCIe*/
	{ /*10*/ X,          X,            X, X, 0 },
	{ /*11*/ GIC_CPU0,   GIC_CPU_VEC3, X, X, GIC_FLAG_TRANSPARENT }, /*USB3*/
	{ /*12*/ X,          X,            X, X, 0 },
	{ /*13*/ X,          X,            X, X, 0 },
	{ /*14*/ X,          X,            X, X, 0 },
	{ /*15*/ GIC_CPU0,   GIC_CPU_VEC7, X, X, GIC_FLAG_TIMER }, /*Local timer*/
	{ /*16*/ X,          X,            X, X, 0 },
	{ /*17*/ X,          X,            X, X, 0 },
	{ /*18*/ X,          X,            X, X, 0 },
	{ /*19*/ X,          X,            X, X, 0 },
	{ /*20*/ X,          X,            X, X, 0 },
	{ /*21*/ X,          X,            X, X, 0 },
	{ /*22*/ GIC_CPU0,   GIC_CPU_INT6, X, X, GIC_FLAG_TRANSPARENT }, /*UART0*/
	/* IPI */
};
#undef X
