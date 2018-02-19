/*
 * Realtek Semiconductor Corp.
 *
 * bsp/irq-ipi.c
 *     SMP IPI interrupt initialization and handlers.
 *
 * This file is part of BSP irq handlers, and will be
 * included if CONFIG_SMP is set.
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */

unsigned int bsp_ipi_map[NR_CPUS];

void bsp_ipi_dispatch(void)
{
	int intr;

	intr = gic_get_int();
	if (intr < 0)
		return;  /* interrupt has already been cleared */

	do_IRQ(BSP_IRQ_GIC_BASE + intr);
}

/*
 * Handle SMP IPI interrupts
 *
 * Two IPI interrupts, resched and call, are handled here.
 */
static irqreturn_t bsp_ipi_resched(int irq, void *devid)
{
	scheduler_ipi();
	return IRQ_HANDLED;
}

static irqreturn_t bsp_ipi_call(int irq, void *devid)
{
	smp_call_function_interrupt();
	return IRQ_HANDLED;
}

static struct irqaction irq_resched = {
	.handler    = bsp_ipi_resched,
	.flags      = IRQF_PERCPU,
	.name       = "IPI resched"
};

static struct irqaction irq_call = {
	.handler    = bsp_ipi_call,
	.flags      = IRQF_PERCPU,
	.name       = "IPI call"
};

/*
 * Initialize IPI interrupts.
 *
 * IPI interrupts are routed via GIC.
 */
void __init bsp_ipi_init(struct gic_intr_map *intrmap)
{
	int irq;
	int cpu;

	/* build ipi_map for fast interrupt handling */
	gic_setup_ipi(bsp_ipi_map, GIC_CPU_INT3, GIC_CPU_INT4);

	/* setup ipi interrupt */
	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		irq = BSP_IRQ_GIC_BASE + GIC_IPI_RESCHED(cpu);
		setup_irq(irq, &irq_resched);
		irq_set_handler(irq, handle_percpu_irq);

		irq = BSP_IRQ_GIC_BASE + GIC_IPI_CALL(cpu);
		setup_irq(irq, &irq_call);
		irq_set_handler(irq, handle_percpu_irq);
	}
}
