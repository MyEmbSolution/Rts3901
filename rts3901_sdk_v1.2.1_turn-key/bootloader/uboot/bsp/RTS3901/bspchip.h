/*
 * Realtek Semiconductor Corp.
 *
 * bsp.h
 * Board Support Package header file
 *
 * Tony Wu (tonywu@realtek.com.tw)
 * Dec. 07, 2007
 */

#ifndef _BSPCHIP_H_
#define _BSPCHIP_H_

/*
 *****************************************************************************************
 * DEFINITIONS AND TYPES
 *****************************************************************************************
 */

/* Register Macro */
#ifndef REG64
#define REG64(reg)      (*(volatile u64 *)(reg))
#endif
#ifndef REG32
#define REG32(reg)      (*(volatile u32 *)(reg))
#endif
#ifndef REG16
#define REG16(reg)      (*(volatile u16 *)(reg))
#endif
#ifndef REG8
#define REG8(reg)       (*(volatile u8  *)(reg))
#endif

/*Memory Macro*/
#ifndef MEM32
#define MEM32(addr)	(*(volatile u32 *)(addr))
#endif

/*
 *****************************************************************************************
 * Timer
 *****************************************************************************************
 */

#define TIMER0_IOBASE		0xB8810000	/* Timer0 register base address mapping */
#define TIMER0_FREQ		25000000	/* The frequency of timer module        */

/* Register offset from TIMERx_IOBASE */
#define TIMER_TLCR1		0x00
#define TIMER_TCV		0x04
#define TIMER_TCR		0x08
#define TIMER_EOIA		0x0c

/*
 *****************************************************************************************
 * Uart
 *****************************************************************************************
 */

#define UART1_IOBASE		0xB8810100	/* Uart1 register base address mapping  */
#define UART1_FREQ			24000000	/* Serial Clock                         */
#define XB2_UART1_PULL_UP_CTRL 0xb887003c
#define XB2_UART1_PULL_DOWN_CTRL 0xb8870030

/* Register offset from UARTx_IOBASE */
#define UART_RBR		0x00
#define UART_THR		0x00
#define UART_IER		0x04
#define UART_IIR		0x08
#define UART_LCR		0x0C
#define UART_LSR		0x14
#define UART_DLL		0x00
#define UART_DLH		0x04
#define UART_CTR 		0xFC

/* uart LCR register bits */
#define UART_LCR_PEN		0x00000008	/* parity enable bit */
#define UART_LCR_EPS		0X00000010
#define UART_LCR_PARITY		(UART_LCR_PEN | UART_LCR_EPS)
#define UART_LCR_STOP		0x00000004	/* stop bit          */
#define UART_LCR_DLS		0x00000003	/* 8 bits mode       */
#define UART_LCR_DLAB		0x00000080	/* DLAB bit          */

#define UART_DL_BAUD_9600		(UART_BAUD_BASE / 9600)
#define UART_DL_BAUD_19200		(UART_BAUD_BASE / 19200)
#define UART_DL_BAUD_38400		(UART_BAUD_BASE / 38400)
#define UART_DL_BAUD_57600		(UART_BAUD_BASE / 57600)
#define UART_DL_BAUD_115200		(UART_BAUD_BASE / 115200)

/* baudrate platform specific setting */
#define UART_BAUD_BASE			(UART1_FREQ >> 4)
#define UART_BAUD_DEFAULT		UART_DL_BAUD_57600

#define SYSBASE_VA 0xb8860000
#define SYSBASE_REG(x) ((x) + SYSBASE_VA)

#define CLK_CHANGE		SYSBASE_REG(0x1000)
#define BUS_CLK_CHANGE			0x80
#define XB2_CLK_CHANGE			0x40
#define CPU_CLK_CHANGE			0x20
#define DRAM_CLK_CHANGE		0x10

#define UART_CLK_CFG_REG		SYSBASE_REG(0x1028)
#define CLK_ENABLE 0x100
#define CLOCK_SELECT_USB_PLL_2    0x0
#define CLOCK_SELECT_USB_PLL_3    0x1
#define CLOCK_SELECT_USB_PLL_5    0x2
#define CLOCK_SELECT_USB_PLL_7    0x3
#define CLOCK_SELECT_PLL0_2       0x4
#define CLOCK_SELECT_PLL0_3       0x5
#define CLOCK_SELECT_PLL0_5       0x6
#define CLOCK_SELECT_PLL0_7       0x7
#define CLOCK_SELECT_PLL1_2       0x8
#define CLOCK_SELECT_PLL1_3       0x9
#define CLOCK_SELECT_PLL1_5       0xA
#define CLOCK_SELECT_PLL1_7       0xB
#define CLOCK_SELECT_PLL2_2       0xC
#define CLOCK_SELECT_PLL2_3       0xD
#define CLOCK_SELECT_PLL2_5       0xE
#define CLOCK_SELECT_PLL2_7       0xF
#define CLOCK_SELECT_PLL3_2       0x10
#define CLOCK_SELECT_PLL3_3       0x11
#define CLOCK_SELECT_PLL3_5       0x12
#define CLOCK_SELECT_PLL3_7       0x13
#define CLOCK_SELECT_PLL4_2       0x14
#define CLOCK_SELECT_PLL4_3       0x15
#define CLOCK_SELECT_PLL4_5       0x16
#define CLOCK_SELECT_PLL4_7       0x17
#define CLOCK_SELECT_PLL5_2       0x18
#define CLOCK_SELECT_PLL5_3       0x19
#define CLOCK_SELECT_PLL5_5       0x1A
#define CLOCK_SELECT_PLL5_7       0x1B
#define CLOCK_SELECT_DIV2    	(0x1<<5)
#define CLOCK_SELECT_DIV4    	(0x2<<5)
#define CLOCK_SELECT_DIV8    	(0x3<<5)
#define CLOCK_SELECT_DIV16    	(0x4<<5)
#define CLOCK_SELECT_DIV32   	(0x5<<5)
#define CLOCK_SELECT_DIV64   	(0x6<<5)
#define CLOCK_SELECT_DIV128   	(0x7<<5)

#define DRAM_OCP_BUS_CLK_CFG_REG		SYSBASE_REG(0x100C)
#define CPU_CLK_CFG_REG		SYSBASE_REG(0x1010)
#define XB2_CLK_CFG_REG		SYSBASE_REG(0x1014)
#define BUS_CLK_CFG_REG		SYSBASE_REG(0x1018)
#define ETHERNET_CLK_CFG_REG		SYSBASE_REG(0x1024)
#define UART_CLK_CFG_REG		SYSBASE_REG(0x1028)

#define SYS_PLL0_CFG0		SYSBASE_REG(0x4100)
#define SYS_PLL0_CFG1		SYSBASE_REG(0x4104)
#define SYS_PLL0_CTRL		SYSBASE_REG(0x4108)
#define SYS_PLL0_STAT		SYSBASE_REG(0x410c)

#define SYS_PLL1_CFG0		SYSBASE_REG(0x4200)
#define SYS_PLL1_CFG1		SYSBASE_REG(0x4204)
#define SYS_PLL1_CTRL		SYSBASE_REG(0x4208)
#define SYS_PLL1_STAT		SYSBASE_REG(0x420c)

#define SYS_PLL2_CFG0		SYSBASE_REG(0x4300)
#define SYS_PLL2_CFG1		SYSBASE_REG(0x4304)
#define SYS_PLL2_CTRL		SYSBASE_REG(0x4308)
#define SYS_PLL2_STAT		SYSBASE_REG(0x430c)

#define PLL_EN_WD_PLL		0x01
#define CMU_EN_CKOOBS_PLL 0x02
#define LDO_EN_PLL			0x04
#define CMU_EN_PLL			0x08
#define PLL_SETP1			LDO_EN_PLL
#define PLL_SETP2			(LDO_EN_PLL|PLL_EN_WD_PLL)
#define PLL_SETP3			(CMU_EN_PLL|LDO_EN_PLL|CMU_EN_CKOOBS_PLL|PLL_EN_WD_PLL)

#define 	XB2_VA		0xB8870000
#define 	XB2BASE_REG(x) ((x) + XB2_VA)
#define	UART_TX_EN_REG		XB2BASE_REG(0x54)
#define   UART0_TX_EN			0x01
#define   UART1_TX_EN			(0x01<<8)
#define   UART2_TX_EN			(0x01<<16)

#define 	SPIC_VA		0xB8030000
#define 	SPICBASE_REG(x) ((x) + SPIC_VA)
#define   SPIC_BAUD_REG	SPICBASE_REG(0x14)

#define   SPIC_BAUDR		0x14
#define   SPIC_READ_DUAL_DATA	0xE4
#define   SPIC_AUTO_LENGTH	0x11C
#define   SPIC_VALID_CMD	0x120


/* some register about PWM and GPIO */
#define GPIO_BASE_ADDR		0xb8800000
#define XB2_SPI_PULLCTRL 	(GPIO_BASE_ADDR + 0x28)
#define ALL_SPI_NOPULL		0
#define GPIO_ETN_LED_SEL_REG	(GPIO_BASE_ADDR + 0x006c)
#define GPIO_ETN_LED_SEL_PWM	0

#define PWM_BASE_ADDR		0xb8820040
#define PWM_EN_REG		(PWM_BASE_ADDR + 0)
#define PWM_KEEP_REG		(PWM_BASE_ADDR +	0x04)
#define PWM_HIGH_REG		(PWM_BASE_ADDR + 0x08)
#define PWM_LOW_REG		(PWM_BASE_ADDR + 0x0c)
#define PWM_NUM_REG		(PWM_BASE_ADDR + 0x14)
#define PWM_INV_REG		(PWM_BASE_ADDR + 0x18)

/* some register about watchdog */
#define WATCH_DOG_BASEADDR		0xb8862000
#define WATCH_DOG_CFG_REG		(WATCH_DOG_BASEADDR + 0x0000)
#define WATCH_DOG_CTL_REG		(WATCH_DOG_BASEADDR + 0x0004)
#define WATCH_DOG_INT_ENABLE_REG	(WATCH_DOG_BASEADDR + 0x0008)
#define WATCH_DOG_INT_FLAG_REG		(WATCH_DOG_BASEADDR + 0x000c)

#define WATCH_DOG_PAD_PULLDOWN		0x1
#define WATCH_DOG_TIMEOUT_1S		0x0
#define WATCH_DOG_RESET_SYSTEM		0x1
#define WATCH_DOG_ENABLE		0x1
#define WATCH_DOG_CFG_DATA		((WATCH_DOG_PAD_PULLDOWN << 9) \
					| (WATCH_DOG_TIMEOUT_1S << 2) \
					| (WATCH_DOG_RESET_SYSTEM << 1) \
					| (WATCH_DOG_ENABLE << 0))

/* some register about GPIO for led */
#define XB2_SHARE_GPIO_SELECT	0xB880004C
#define XB2_SHARE_GPIO_OE	0xB8800050
#define XB2_SHARE_GPIO_O	0xB8800058

#define SYS_EFUSE_CFG		0xb8865000
#define SYS_EFUSE_CTL		0xb8865004
#define SYS_EFUSE_CNT		0xb8865008
#define SYS_EFUSE_READ_DAT	0xb886500c

#define EFUSE_PWR_BFW		15
#define EFUSE_ENB_BFW		14
#define EFUSE_POR_W_10_BFW	13
#define EFUSE_POR_10_BFW	12
#define EFUSE_PROGRAM_BFW	11
#define EFUSE_READ_BFW		10
#define EFUSE_TMRF_BFW		8
#define EFUSE_TMRF_BITNUM	2
#define EFUSE_B_BFW		5
#define EFUSE_B_BITNUM		3
#define EFUSE_A_BFW		0
#define EFUSE_A_BITNUM		5

/* Values for SPIC_CTRL before relocate */
#define SPICCTRL_DDR_ADDR_BASE	0x82000000
#define SPICCTRL_DDR_ADDR_END	0x820000a0

#define SCKDV			0x1
#define SPIC_BAUD_VALUE		(SCKDV << 0)

#define RD_DUAL_O_CMD		0x3B

#define CS_H_WR_DUM_LEN		0x5
#define CS_H_RD_DUM_LEN		0x0
#define AUCK_DUM_LEN		0x0
#define AUTO_ADDR_LENGTH	0x3
#define RD_DUMMY_LENGTH		0x11
#define SPIC_AUTO_LENGTH_VALUE	((CS_H_WR_DUM_LEN << 28) \
		| (CS_H_RD_DUM_LEN << 26) | (AUCK_DUM_LEN << 18) \
		| (AUTO_ADDR_LENGTH << 16) | RD_DUMMY_LENGTH)

#define WR_BLOCKING		(1 << 9)
#define RD_DUAL_I		(1 << 1)
#define SPIC_VALID_CMD_VALUE	(WR_BLOCKING | RD_DUAL_I)

#endif

