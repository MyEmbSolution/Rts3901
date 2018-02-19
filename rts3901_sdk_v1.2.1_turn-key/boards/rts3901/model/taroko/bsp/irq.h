/*
 * Realtek Semiconductor Corp.
 *
 * bsp/irq.h:
 *
 * Copyright (C) 2014 Wei WANG (wei_wang@realsil.com.cn)
 */
#ifndef _BSP_IRQ_H
#define _BSP_IRQ_H

#ifdef NR_IRQS
#undef NR_IRQS
#endif

/* 8 (cpu) + 8 (vec) + 23 (gpio) + 10 (xb2) */
#define NR_IRQS 49

#endif /* _BSP_IRQ_H */
