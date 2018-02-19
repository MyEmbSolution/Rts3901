/*
 * Realtek Semiconductor Corp.
 *
 * bsp/timer.c:
 *     bsp timer initialization
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#ifdef CONFIG_USE_UAPI
#include <generated/uapi/linux/version.h>
#else
#include <linux/version.h>
#endif
#include <linux/init.h>
#include <linux/timex.h>

#include <asm/time.h>

#include "bspchip.h"
#include "clock.h"

#ifdef CONFIG_CEVT_EXT
inline void bsp_timer_ack(void)
{
	unsigned volatile int eoi;
	eoi = REG32(BSP_TIMER0_EOI);
}

void __init bsp_timer_init(void)
{
	/* disable timer */
	REG32(BSP_TIMER0_TCR) = 0x00000000;

	/* initialize timer registers */
	REG32(BSP_TIMER0_TLCR) = BSP_TIMER0_FREQ / HZ;

	/* hook up timer interrupt handler */
	ext_clockevent_init(BSP_IRQ_TIMER0);

	/* enable timer */
	REG32(BSP_TIMER0_TCR) = 0x00000003;       /* 0000-0000-0000-0011 */
}
#endif

#ifdef CONFIG_CEVT_RLX
unsigned int __cpuinit get_c0_compare_int(void)
{
	return BSP_IRQ_COPMARE;
}

void __init bsp_timer_init(void)
{
//	rlx_hpt_frequency = BSP_CPU_FREQ;

	write_c0_count(0);
	clear_c0_cause(CAUSEF_DC);
	rlx_clockevent_init(BSP_IRQ_COPMARE);
	rlx_clocksource_init();
#ifdef CONFIG_HAVE_CLK
	rlx_clock_init();
#endif
}
#endif
