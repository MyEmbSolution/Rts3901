/*
 * Realtek Semiconductor Corp.
 *
 * bsp/irq.c
 *   bsp interrupt initialization and handler code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#ifdef CONFIG_USE_UAPI
#include <generated/uapi/linux/version.h>
#else
#include <linux/version.h>
#endif
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>

#include <asm/irq.h>
#include <asm/irq_cpu.h>
#include <asm/irq_vec.h>
#include <asm/rlxregs.h>

#include <asm/mmcr.h>
#include <asm/gic.h>

#include "bspchip.h"

/*
 * FFS
 *
 * Given pending, use ffs to find first leading non-zero. Then,
 * Use offset to shift bit range. For example, use CAUSEB_IP as offset
 * to look for bit starting at 12 in status register, so that ffs is
 * rounded between 0~7
 */
#ifdef CONFIG_CPU_HAS_CLS
static inline int clz(unsigned int x)
{
	int r;

	__asm__ __volatile__(
	".set push                                      \n"
	".set noreorder                                 \n"
	"cls    %0, %2                                  \n"
	"srl    %1, %2, 31                              \n"
	"addiu  %0, 1                                   \n"
	"movn   %0, $0, %1                              \n"
	".set pop                                       \n"
	: "=&r" (x), "=r" (r)
	: "r" (x));

	return x;
}

static inline int irq_ffs(unsigned int pending, unsigned int offset)
{
	return -clz(pending) + 31 - offset;
}
#endif

#ifdef CONFIG_SMP
#include "irq-ipi.c"
#endif

#ifdef CONFIG_IRQ_ICTL
#include "irq-ictl.c"
#endif

#ifdef CONFIG_IRQ_GIC
#include "irq-gic.c"
#define IRQ_GIC_CASCADE		6
#endif

/*
 * bsp_irq_dispatch
 */
asmlinkage void bsp_irq_dispatch(void)
{
	unsigned int pending = read_c0_cause() & read_c0_status() & ST0_IM;
	int irq;

#ifdef CONFIG_CPU_HAS_CLS
	irq = irq_ffs(pending, CAUSEB_IP);
	if (irq < 0)
		spurious_interrupt();
#else
	irq = 0;
	if (pending & CAUSEF_IP3)
		irq = 3;
	else if (pending & CAUSEF_IP4)
		irq = 4;
#ifdef CONFIG_IRQ_GIC
	else if (pending & CAUSEF_IP6)
		irq = IRQ_GIC_CASCADE;
#endif
	else
		spurious_interrupt();
#endif /* !CONFIG_CPU_HAS_CLS */

#ifdef CONFIG_SMP
	if (((1 << irq) & bsp_ipi_map[smp_processor_id()]))
		bsp_ipi_dispatch();
	else
#endif
#ifdef CONFIG_IRQ_GIC
	{
		if (irq == IRQ_GIC_CASCADE)
			irq = gic_get_int();

		do_IRQ(BSP_IRQ_GIC_BASE + irq);
	}
#else
		do_IRQ(BSP_IRQ_CPU_BASE + irq);
#endif
}

void __init bsp_irq_init(void)
{
	/* initialize IRQ action handlers */
	rlx_cpu_irq_init(BSP_IRQ_CPU_BASE);
	rlx_vec_irq_init(BSP_IRQ_LOPI_BASE);

#ifdef CONFIG_IRQ_GIC
	/* install gic_intr_map to GIC registers */
	gic_init(gic_intr_map, BSP_IRQ_GIC_BASE);
#endif
#ifdef CONFIG_IRQ_ICTL
	bsp_ictl_irq_init(BSP_IRQ_ICTL_BASE);
#endif
#ifdef CONFIG_SMP
	bsp_ipi_init(gic_intr_map);
#endif
}
