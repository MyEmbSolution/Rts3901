/*
 * Realtek Semiconductor Corp.
 *
 * bsp/setup.c
 *     bsp setup code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#include <linux/console.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

#include <asm/addrspace.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/time.h>
#include <asm/reboot.h>
#include <asm/mmcr.h>

#include "bspchip.h"

#ifdef CONFIG_RTSX_WATCHDOG
extern void reboot_by_wdt(int new_timeout);
#else
#define reboot_by_wdt(new_timeout)
#endif

static void bsp_machine_restart(char *command)
{
	local_irq_disable();
	reboot_by_wdt(1);
	while (1) ;
}

static void bsp_machine_halt(void)
{
	printk("System halted.\n");
	while(1);
}

static void bsp_machine_power_off(void)
{
	printk("System halted. Please power off.\n");
	while(1);
}

/*
 * callback function
 */
void __init bsp_setup(void)
{
	extern void bsp_smp_init(void);

	/* define io/mem region */
	iomem_resource.start = 0x18000000;
	iomem_resource.end = 0x1fffffff;

	/* set reset vectors */
	_machine_restart = bsp_machine_restart;
	_machine_halt = bsp_machine_halt;
	pm_power_off = bsp_machine_power_off;

#ifdef CONFIG_SMP_CMP
	mmcr_init_core(0);
#endif
#ifdef CONFIG_SMP
	bsp_smp_init();
#endif
}
