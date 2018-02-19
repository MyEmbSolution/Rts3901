/*
 * Realtek Semiconductor Corp.
 *
 * rlxboard.c:
 *	uboot bsp file
 *
 * Jethro Hsu (jethro@realtek.com)
 */

#include <common.h>
#include <command.h>
#include <asm/rlxregs.h>
#include <asm/io.h>
#include <netdev.h>
#include "bspchip.h"
#include "ddr_def.h"

/*#define _DDR_SETTING_DEBUG_*/
phys_size_t initdram(int board_type)
{
#ifdef _DDR_SETTING_DEBUG_
	printf("DDR controller setting\n");
	printf("2a0 wrap_idr: %x\n", MEM32(DDRC_BASE + PCTL_WRAP_IDR));
	printf("f4 svn_id: %x\n", MEM32(DDRC_BASE + PCTL_SVN_ID));
	printf("f8 idr: %x\n", MEM32(DDRC_BASE + PCTL_IDR));
	printf("224 misc: %x\n", MEM32(DDRC_BASE + PCTL_MISC));
	printf("4 dcr: %x\n", MEM32(DDRC_BASE + PCTL_DCR));
	printf("08 iocr: %x\n", MEM32(DDRC_BASE + PCTL_IOCR));
	printf("3c mr2: %x\n", MEM32(DDRC_BASE + PCTL_MR2));
	printf("38 mr1: %x\n", MEM32(DDRC_BASE + PCTL_MR1));
	printf("34 mr0: %x\n", MEM32(DDRC_BASE + PCTL_MR0));
	printf("10 drr: %x\n", MEM32(DDRC_BASE + PCTL_DRR));
	printf("14 tpr0: %x\n", MEM32(DDRC_BASE + PCTL_TPR0));
	printf("18 tpr1: %x\n", MEM32(DDRC_BASE + PCTL_TPR1));
	printf("1c tpr2: %x\n", MEM32(DDRC_BASE + PCTL_TPR2));
	printf("0c csr: %x\n", MEM32(DDRC_BASE + PCTL_CSR));
	printf("30 mrinfo: %x\n", MEM32(DDRC_BASE + PCTL_MR_INFO));
#endif
	/* Sdram is setup by assembler code */
	/* If memory could be changed, we should return the true value here */
	/*
	probe DDR size ,
	*1. use uncachable address to probe size , if use cachable address , the value
	*will save in cache , the addess seemed not wrapped,
	*2. for 128MB flash, the address 0xAFFFFFFC will wrap to 0x0xA7FFFFFC,
	*for 64MB flash , the address 0xA7FFFFFC will wrap to 0xA3FFFFFC
	*/

	MEM32(0xAFFFFFFC) = 0x10000000;
	MEM32(0xA7FFFFFC) = 0x08000000;
	MEM32(0xA3FFFFFC) = 0x04000000;
	return MEM32(0xAFFFFFFC);

}

int checkboard(void)
{
	u32 prid;
	u32 cpufreq;
	prid = read_c0_prid();
	prid &= 0xffff;
	cpufreq = CPU_FREQUCNCY;
#ifdef CONFIG_BOARD_rle0745
	printf("Board: IPCAM RLE0745 CPU: ");
#endif

#ifdef CONFIG_BOARD_rts3901
	printf("Board: IPCAM RTS3901 CPU: %dM :", cpufreq/1000000);
#endif
	switch (prid) {
	case 0x0000dc06:
		printf("rx5381");
		break;
	case 0x0000dc05:
		printf("rx4381");
		break;
	case 0x0000dc04:
		printf("rx5271");
		break;
	case 0x0000dc03:
		printf("rx4271");
		break;
	case 0x0000dc02:
		printf("rx5281");
		break;
	case 0x0000dc01:
		printf("rx4281");
		break;
	default:
		printf("unknown");
	}
	printf(" prid=0x%x\n", prid);

	return 0;
}

void _machine_restart(void)
{
	/* set watchdog */
	/* watchdog time: 1s, reset system when timeout, enable watchdog */
	writel(WATCH_DOG_CFG_DATA, WATCH_DOG_CFG_REG);
	printf("set watchdog, resetting...");
	while (1)
		;
}

int get_dram_clock(void)
{
#ifdef CONFIG_TARGET_ASIC
	u64 dpi_n_code, dpi_f_code;
	dpi_f_code = MEM32(DDR_PHY_SSC1);
	dpi_n_code = MEM32(DDR_PHY_SSC2);
	u64 clock = REF_PLL * dpi_f_code / 8192;
	clock = (clock + REF_PLL * (dpi_n_code + 2) + 500000) / 1000000;
	return (int)clock;
#endif
#ifdef CONFIG_TARGET_FPGA
	return 224;
#endif
}
