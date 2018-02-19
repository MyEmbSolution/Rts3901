/*
 * Realtek Semiconductor Corp.
 *
 * bsp/prom.c
 *     bsp early initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <asm/bootinfo.h>
#include <asm/addrspace.h>
#include <linux/console.h>
#include <linux/serial_reg.h>
#include <linux/rts_sysctl.h>
#include <asm/io.h>
#include <asm/time.h>

#include "bspcpu.h"
#include "bspchip.h"

extern char arcs_cmdline[];
unsigned long _prom_memsize;
int _prom_argc;
char **_prom_argv, **_prom_envp;
struct rts_soc_hw_id RTS_SOC_HW_ID;

EXPORT_SYMBOL_GPL(RTS_SOC_HW_ID);

#ifdef CONFIG_EARLY_PRINTK
static int promcons_output = 0;
static unsigned int uart_initialized = 0;

static inline unsigned int serial_in(int offset)
{
	return REG32(BSP_UART1_VADDR + (offset << 2)) & 0xff;
}

static inline void serial_out(int offset, int value)
{
	REG32(BSP_UART1_VADDR + (offset << 2)) = value;
}

void unregister_prom_console(void)
{
	if (promcons_output) {
		promcons_output = 0;
	}
}

void disable_early_printk(void)
		__attribute__ ((alias("unregister_prom_console")));

static void early_uart_init(void)
{
	serial_out(UART_LCR, 0x80);
	serial_out(UART_IER, 0x0);
	serial_out(UART_TX, 0x1a);
	serial_out(UART_LCR, 0x3);
	if (REG32(BSP_UART1_VADDR + (UART_CTR << 2)) == 0x44570110) {
		REG32(UART_CLK_CFG_REG) = CLK_ENABLE|CLOCK_SELECT_USB_PLL_5|CLOCK_SELECT_DIV4;
		REG32(UART_TX_EN_REG) = UART1_TX_EN;
	}
	uart_initialized = 1;
}

int prom_putchar(char c)
{
	if (!uart_initialized)
		early_uart_init();

	while ((serial_in(UART_LSR) & UART_LSR_THRE) == 0)
	       ;

	serial_out(UART_TX, c);

	return 1;
}
#endif

const char *get_system_type(void)
{
	return "RLX Linux for IPCam Platform";
}

static char *prom_getenv(char *envname)
{
	char **env = _prom_envp;
	int i;

	i = strlen(envname);

	while (*env) {
		if (strncmp(envname, *env, i) == 0 && *(*env+i) == '=')
			return *env + i + 1;
		env++;
	}

	return 0;
}

/**
 * get_memsize - get the size of memory as a single bank
 */
static unsigned long get_memsize(void)
{
	phys_addr_t memsize = 0;
	char *memsize_str;

	/* otherwise look in the environment */
	memsize_str = prom_getenv("memsize");

	if (memsize_str != NULL) {
		pr_info("prom memsize = %s\n", memsize_str);
		memsize = simple_strtol(memsize_str, NULL, 0);
	}

	if (memsize == 0) {
		if (_prom_memsize != 0) {
			memsize = _prom_memsize;
			pr_info("_prom_memsize = 0x%x\n", memsize);
			/* add in memory that the bootloader doesn't
			 * report */
		} else {
			memsize = cpu_mem_size;
			pr_info("Memsize not passed by bootloader, "
				"defaulting to 0x%x\n", memsize);
		}
	}

	return memsize;
}

#ifdef CONFIG_CEVT_RLX
static unsigned long get_cpufreq(void)
{
	unsigned long cpufreq = 0;
	char *cpufreq_str;

	cpufreq_str = prom_getenv("cpufreq");
	if (cpufreq_str != NULL) {
		pr_info("prom cpufreq = %s\n", cpufreq_str);
		cpufreq = simple_strtoul(cpufreq_str, NULL, 0);
	}

	if (cpufreq == 0)
		cpufreq = BSP_CPU_FREQ;

	return cpufreq;
}
#endif

static void __init prom_init(void)
{
	_prom_argc = (int)fw_arg0;
	_prom_argv = (char **)fw_arg1;
	_prom_envp = (char **)fw_arg2;
	_prom_memsize = (unsigned long)fw_arg3;
}

static void __init set_arbiter(void)
{
	REG32(BSP_ARBITER_XB0_CTRL0) = 0x01231062;
	REG32(BSP_ARBITER_XB0_CTRL1) = 0x15101271;
	REG32(BSP_ARBITER_XB0_CTRL2) = 0x08214012;
	REG32(BSP_ARBITER_XB0_CTRL3) = 0xa1012b19;
	REG32(BSP_ARBITER_XB1_CTRL0) = 0x25302135;
	REG32(BSP_ARBITER_XB1_CTRL1) = 0x24360000;
}

void __init prom_init_cmdline(void)
{
	int i;

	/* Always ignore argv[0] */
	for (i = 1; i < _prom_argc; i++) {
		strlcat(arcs_cmdline, _prom_argv[i], COMMAND_LINE_SIZE);
		if (i < (_prom_argc - 1))
			strlcat(arcs_cmdline, " ", COMMAND_LINE_SIZE);
	}
}

/* Do basic initialization */
void __init bsp_init(void)
{
	u_long mem_size;
	u32 hw_id;

	prom_init();
	prom_init_cmdline();

#ifdef CONFIG_CEVT_RLX
	rlx_hpt_frequency = (unsigned int)get_cpufreq();
#endif

	mem_size = get_memsize();
	add_memory_region(0, mem_size, BOOT_MEM_RAM);

	set_arbiter();

	/* Init HW ID */
	hw_id = REG32(BSP_HW_ID_REG);
	RTS_SOC_HW_ID.hw_ver = (u16)hw_id;
	RTS_SOC_HW_ID.hw_rev = (u8)(hw_id >> 16);
	RTS_SOC_HW_ID.isp_ver = (u8)(hw_id >> 24);
	if (RTS_SOC_HW_ID.hw_rev == 1) {
		if (REG32(BSP_TS_DBG_REG) & (1 << 24))
			RTS_SOC_HW_ID.hw_rev = HW_ID_REV_C;
	}
	pr_info("hw_ver: 0x%x, hw_rev: 0x%x, isp_ver: 0x%x\n",
			RTS_SOC_HW_ID.hw_ver,
			RTS_SOC_HW_ID.hw_rev,
			RTS_SOC_HW_ID.isp_ver);
}

void __init bsp_free_prom_memory(void)
{
	return;
}
