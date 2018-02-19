/*
 * Realtek Semiconductor Corp.
 *
 * bsp/smp.c
 *     bsp SMP initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */

#include <asm/gcmpregs.h>
#include <asm/smp-boot.h>

static void smp_gcmp_request(int core)
{
	unsigned int type;
	int i;

	GCMPCLCB(OTHER) = core << GCMP_CCB_OTHER_CORENUM_SHF;
	type = (GCMPCOCB(CFG) & GCMP_CCB_CFG_IOCUTYPE_MSK) >> GCMP_CCB_CFG_IOCUTYPE_SHF;

	switch (type) {
	case GCMP_CCB_CFG_IOCUTYPE_CPU:
		if (GCMPCOCB(COHCTL) != 0) /* Should not happen */
			GCMPCOCB(COHCTL) = 0;

		/* allow CPUi access to the GCR registers */
		GCMPGCB(GCSRAP) |= 1 << (core + GCMP_GCB_GCSRAP_CMACCESS_SHF);

		/* start CPUi */
		if (GCMPGCB(CPCSR) & GCMP_GCB_CPCSR_EX_MSK) {
			CPCCLCB(OTHER) = core << 16;
			CPCCOCB(CMD) = 3;	/* Power up */
		} else
			GCMPCOCB(RESETR) = 0;

		/* Poll the Coherence Control register waiting for it to join the domain */
		for (i = SMP_LAUNCH_PERIOD; (i >= 0) && (GCMPCOCB(COHCTL) == 0); i--)
			;
		break;

	case GCMP_CCB_CFG_IOCUTYPE_NCIOCU:
		break;
	case GCMP_CCB_CFG_IOCUTYPE_CIOCU:
		GCMPCOCB(COHCTL) = 0xff;
		break;
	default:
		break;
	}
}

static void smp_gcmp_start(void)
{
	unsigned int ncores, niocus;
	unsigned int gcr = GCMPGCB(GC);
	int i;

	niocus = ((gcr & GCMP_GCB_GC_NUMIOCU_MSK) >> GCMP_GCB_GC_NUMIOCU_SHF);
	ncores = ((gcr & GCMP_GCB_GC_NUMCORES_MSK) >> GCMP_GCB_GC_NUMCORES_SHF) + 1;

	/* requester 0 is already in the coherency domain */
	for (i = 1; i < ncores; i++)
		smp_gcmp_request(i);

	for (i = 0; i < niocus; i++)
		smp_gcmp_request(4+i);
}

void smp_power_on(void)
{
	/* If the CPC is present, enable it */
	if (GCMPGCB(CPCSR) & GCMP_GCB_CPCSR_EX_MSK) {
		/* printk("GCB present\n"); */
		GCMPGCB(CPCBA) = CPC_BASE_ADDR | GCMP_GCB_CPCBA_EN_MSK;
	}
	else
		return;

	smp_gcmp_start();
}
