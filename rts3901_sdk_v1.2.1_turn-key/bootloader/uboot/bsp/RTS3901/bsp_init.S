/* Memory sub-system initialization code */

#include <config.h>
#include <asm/regdef.h>
#include <asm/rlxregs.h>
#include "ddr_def.h"
#include "bspchip.h"

	.text
	.set noreorder
	.globl bsp_boot_init_plat
	.ent bsp_boot_init_plat
/************************************************************************
 *  bsp_boot_init_plat
 ************************************************************************/

bsp_boot_init_plat:
dram_init:
#ifdef CONFIG_TARGET_ASIC
#ifdef _SYS_PLL_INIT_
	/*pll0 1.0G
	ew 0xb8864100 0x240000
	ew 0xb8864108 0x04
	ew 0xb8864108 0x05
	delay 1
	ew 0xb8864108 0x0f */

	li      v0, SYS_PLL0_CFG0
	li      v1, (PLL0_N_SETTING<<16)
	sw      v1, 0(v0)

	li      v0, SYS_PLL0_CTRL
	li      v1, PLL_SETP1
	sw      v1, 0(v0)

	/*delay 200us*/
	li	t0, 0
	li	t1, 0x10
1:
	addu t0, 4
	blt  t0, t1, 1b
	nop

	li      v0, SYS_PLL0_CTRL
	li      v1, PLL_SETP2
	sw      v1, 0(v0)

	/*delay 200us*/
	li	t0, 0
	li	t1, 0x10
1:
	addu t0, 4
	blt  t0, t1, 1b
	nop

	li      v0, SYS_PLL0_CTRL
	li      v1, PLL_SETP3
	sw      v1, 0(v0)

	/*delay 200us*/
	li	t0, 0
	li	t1, 0x10
1:
	addu t0, 4
	blt  t0, t1, 1b
	nop

	/*pll0 1.0G
	ew 0xb8864100 0x240000
	ew 0xb8864108 0x04
	ew 0xb8864108 0x05
	delay 1
	ew 0xb8864108 0x0f */

	li      v0, SYS_PLL2_CFG0
	li      v1, (PLL2_N_SETTING<<16)
	sw      v1, 0(v0)

	li      v0, SYS_PLL2_CTRL
	li      v1, PLL_SETP1
	sw      v1, 0(v0)

	/*delay 200us*/
	li	t0, 0
	li	t1, 0x10
1:
	addu t0, 4
	blt  t0, t1, 1b
	nop

	li      v0, SYS_PLL2_CTRL
	li      v1, PLL_SETP2
	sw      v1, 0(v0)

	/*delay 200us*/
	li	t0, 0
	li	t1, 0x10
1:
	addu t0, 4
	blt  t0, t1, 1b
	nop

	li      v0, SYS_PLL2_CTRL
	li      v1, PLL_SETP3
	sw      v1, 0(v0)

	/*delay 200us*/
	li	t0, 0
	li	t1, 0x10
1:
	addu t0, 4
	blt  t0, t1, 1b
	nop

#if(_CHIP_ID_ &_RTS3901A_)
	/*CPU 240M  usb_pll/2 div1
	ew 0xb8861000 0x20
	ew 0xb8861010 0x100
	ew 0xb8861000 0x00*/
	li      v0, CLK_CHANGE
	li      v1, CPU_CLK_CHANGE
	sw      v1, 0(v0)

	li      v0, CPU_CLK_CFG_REG
	li      v1, (CLK_ENABLE|CLOCK_SELECT_USB_PLL_2)
	sw      v1, 0(v0)

	li      v0, CLK_CHANGE
	li      v1, 0x0
	sw      v1, 0(v0)

	/*XB1 120M usb_pll/2 div2
	ew 0xb8861000 0x80
	ew 0xb8861018 0x120
	ew 0xb8861000 0x00*/
	li      v0, CLK_CHANGE
	li      v1, BUS_CLK_CHANGE
	sw      v1, 0(v0)

	li      v0, BUS_CLK_CFG_REG
	li      v1, (CLK_ENABLE|CLOCK_SELECT_USB_PLL_2|CLOCK_SELECT_DIV2)
	sw      v1, 0(v0)

	li      v0, CLK_CHANGE
	li      v1, 0x0
	sw      v1, 0(v0)

	/*XB2 60M usb_pll/2 div4
	ew 0xb8861000 0x40
	ew 0xb8861014 0x140
	ew 0xb8861000 0x00*/
	li      v0, CLK_CHANGE
	li      v1, XB2_CLK_CHANGE
	sw      v1, 0(v0)

	li      v0, XB2_CLK_CFG_REG
	li      v1, (CLK_ENABLE|CLOCK_SELECT_USB_PLL_2|CLOCK_SELECT_DIV4)
	sw      v1, 0(v0)

	li      v0, CLK_CHANGE
	li      v1, 0x0
	sw      v1, 0(v0)


	/*DRAMC 240M usb_pll/2 div1
	ew 0xb8861000 0x10
	ew 0xb886100c 0x104
	ew 0xb8861000 0x00*/
	li      v0, CLK_CHANGE
	li      v1, DRAM_CLK_CHANGE
	sw      v1, 0(v0)

	li      v0, DRAM_OCP_BUS_CLK_CFG_REG
	li      v1, (CLK_ENABLE|CLOCK_SELECT_USB_PLL_2)
	sw      v1, 0(v0)

	li      v0, CLK_CHANGE
	li      v1, 0x0
	sw      v1, 0(v0)
#endif

#if(_CHIP_ID_ &_RTS3901B_)
/*CPU 500M  PLL0/2 div1
	ew 0xb8861000 0x20
	ew 0xb8861010 0x104
	ew 0xb8861000 0x00*/
	li      v0, CLK_CHANGE
	li      v1, CPU_CLK_CHANGE
	sw      v1, 0(v0)

	li      v0, CPU_CLK_CFG_REG
#if (defined CONFIG_CPU_500M)
	li      v1, (CLK_ENABLE|CLOCK_SELECT_PLL0_2)
#elif (defined CONFIG_CPU_333M)
	li      v1, (CLK_ENABLE|CLOCK_SELECT_PLL0_3)
#elif (defined CONFIG_CPU_250M)
	li      v1, (CLK_ENABLE|CLOCK_SELECT_PLL0_2|CLOCK_SELECT_DIV2)
#elif (defined CONFIG_CPU_200M)
	li      v1, (CLK_ENABLE|CLOCK_SELECT_PLL0_5)
#elif (defined CONFIG_CPU_600M)
	li      v1, (CLK_ENABLE|CLOCK_SELECT_PLL2_2)
#elif (defined CONFIG_CPU_400M)
	li      v1, (CLK_ENABLE|CLOCK_SELECT_PLL2_3)
#elif (defined CONFIG_CPU_300M)
	li      v1, (CLK_ENABLE|CLOCK_SELECT_PLL2_2|CLOCK_SELECT_DIV2)
#elif (defined CONFIG_CPU_240M)
	li      v1, (CLK_ENABLE|CLOCK_SELECT_PLL2_5)
#endif
	sw      v1, 0(v0)

	li      v0, CLK_CHANGE
	li      v1, 0x0
	sw      v1, 0(v0)

	/*XB2 60M USBPLL/2 div4
	ew 0xb8861000 0x40
	ew 0xb8861014 0x140
	ew 0xb8861000 0x00*/
	li      v0, CLK_CHANGE
	li      v1, XB2_CLK_CHANGE
	sw      v1, 0(v0)

	li      v0, XB2_CLK_CFG_REG
	li      v1, (CLK_ENABLE|CLOCK_SELECT_DIV4|CLOCK_SELECT_USB_PLL_2)
	sw      v1, 0(v0)

	li      v0, CLK_CHANGE
	li      v1, 0x0
	sw      v1, 0(v0)


	/*DRAMC 333M PLL0/3 div1
	ew 0xb8861000 0x10
	ew 0xb886100c 0x105
	ew 0xb8861000 0x00*/
	li      v0, CLK_CHANGE
	li      v1, DRAM_CLK_CHANGE
	sw      v1, 0(v0)
#ifdef CONFIG_DDR_MCM
	li      v0, DRAM_OCP_BUS_CLK_CFG_REG
	li      v1, (CLK_ENABLE|CLOCK_SELECT_DIV2|CLOCK_SELECT_PLL0_2)
	sw      v1, 0(v0)
#else
	li      v0, DRAM_OCP_BUS_CLK_CFG_REG
	li      v1, (CLK_ENABLE|CLOCK_SELECT_PLL0_3)
	sw      v1, 0(v0)
#endif
	li      v0, CLK_CHANGE
	li      v1, 0x0
	sw      v1, 0(v0)
#endif

	/*set spi pad no pull, for spi pad is pull up on board*/
	li      v0, XB2_SPI_PULLCTRL
	li      v1, ALL_SPI_NOPULL
	sw      v1, 0(v0)
#endif

	/*==========
	DDR PHY init start
	==========*/
	/*step1 , assert all reset signals*/
	li      v0, SYS_DDR_CONFIG_ADDR
	li      v1, SYS_DDR_CFG_STEP1
	sw      v1, 0(v0)

	/*step2, de-assert crt_rst_n*/
	li      v0, SYS_DDR_CONFIG_ADDR
	li      v1, SYS_DDR_CFG_STEP2
	sw      v1, 0(v0)

	/*step3, de-assert pll_ldo_rst_n*/
	li      v0, SYS_DDR_CONFIG_ADDR
	li      v1, SYS_DDR_CFG_STEP3
	sw      v1, 0(v0)

	/*set N value*/
	li      v0, DDR_PHY_SSC2
	li      v1, DPI_N_CODE
	sw      v1, 0(v0)

	/*set F Value*/
	li      v0, DDR_PHY_SSC1
	li      v1, DPI_F_CODE
	sw      v1, 0(v0)

	/*set PLL_CTL3, for cco, ldo, vco*/
	li      v0, DDR_PHY_PLL_CTL3
	li      v1, DDR_PHY_PLL_CTL3_VALUE
	sw      v1, 0(v0)

	/*set PLL_CTL0*/
	li      v0, DDR_PHY_PLL_CTL0
	li      v1, DDR_PHY_PLL_CTL0_VALUE_0
	sw      v1, 0(v0)

	/*disable  phase adj clk/2 bit[5]=0, set dq clock souce to dq bit[6]=0, */
#ifdef NO_ZQ_CAL
	li      v0, DDR_PHY_PLL_CTL
	li      v1, 2
	sw      v1, 0(v0)
#else
	li      v0, DDR_PHY_PLL_CTL
	li      v1, 0
	sw      v1, 0(v0)
#endif

	/*de-assert PLL reset*/
	li      v0, SYS_DDR_CONFIG_ADDR
	li      v1, SYS_DDR_CFG_STEP4_1
	sw      v1, 0(v0)

	/*de-assert PLL power on reset*/
	li      v0, SYS_DDR_CONFIG_ADDR
	li      v1, SYS_DDR_CFG_STEP4_2
	sw      v1, 0(v0)

	/*turning CK POST PI*/
	li      v0, DDR_PHY_PLL_PI0
	li      v1, DDR_PHY_PLL_PI0_VALUE
	sw      v1, 0(v0)

	li      v0, DDR_PHY_PLL_PI1
	li      v1, DDR_PHY_PLL_PI1_VALUE
	sw      v1, 0(v0)

	li      v0, DDR_PHY_PLL_PI2
	li      v1, DDR_PHY_PLL_PI2_VALUE
	sw      v1, 0(v0)


	/*turn on clock flow*/
	li      v0, DDR_PHY_PLL_CTL0
	li      v1, DDR_PHY_PLL_CTL0_VALUE_1
	sw      v1, 0(v0)

	/*clock oe & oe sync*/
	li      v0, DDR_PHY_PLL_CTL1
	li      v1, DDR_PHY_PLL_CTL1_VALUE
	sw      v1, 0(v0)

	check_pll:
	li      t0, DPI_PLL_STABLE
	li      t1, SYS_DDR_STATUS_ADDR
	lw      t1, 0(t1)
	and     t1, t0
	bne     t1, t0, check_pll
	nop

	/*PD always on*/
	li      v0, DDR_PHY_READ_CTRL_5
	li      v1, DDR_PHY_READ_CTRL_5_VALUE
	sw      v1, 0(v0)


	li      v0, DDR_PHY_BIST_2TO1_0
	li      v1, DDR_PHY_BIST_2TO1_0_VALUE
	sw      v1, 0(v0)

	/*cmd output delay 2T*/
	li      v0, DDR_PHY_DPI_CTRL_0
	li      v1, CMD_OUTPUT_DLY_2T
	sw      v1, 0(v0)


	/*cs output delay 2T, select dq/dqs mux table*/
	li      v0, DDR_PHY_WRLVL_CTRL
	li      v1, DQ_DQS_MUX_TABLE_SEL
	sw      v1, 0(v0)

	/*fw set mode : immediately*/
	li      v0, DDR_PHY_DPI_CTRL_0
	li      v1, (CMD_OUTPUT_DLY_2T|FW_SET_MODE_IMME)
	sw      v1, 0(v0)

	/*fw_set_wr_dly*/
	li      v0, DDR_PHY_DPI_CTRL_1
	li      v1, DPI_CTRL_1_SET_WR_DLY
	sw      v1, 0(v0)

#ifndef NO_ZQ_CAL
	/*zclk/32, dzq_up_sel*/
	li      v0, DDR_PHY_PAD_CTRL_PROG
	li      v1, ZCLK_DIV_32
	sw      v1, 0(v0)

	/*zq pad PD=0*/
	li      v0, DDR_PHY_PAD_BUS_1
	li      v1, DDR_PHY_PAD_BUS_1_VALUE
	sw      v1, 0(v0)

	/*dzq auto up*/
	li      v0, DDR_PHY_PAD_CTRL_PROG
	li      v1, (ZCLK_DIV_32|DZQ_AUTO_UP_VALUE)
	sw      v1, 0(v0)


	/*
	DDR3 PUPD III setting:
	OCD/ODT = 34/180ohm
	zprog = 0x43A
	*/
	li      v0, DDR_PHY_PAD_CTRL_PROG
	li      v1, (ZPROG_VALUE0|ZCLK_DIV_32|DZQ_AUTO_UP_VALUE)
	sw      v1, 0(v0)

	/*zctrl_start = 1*/
	li      v0, DDR_PHY_PAD_CTRL_PROG
	li      v1, (DZQ_UP_SEL0_VALUE|ZCTL_START1_VALUE|ZPROG_VALUE0|ZCLK_DIV_32|DZQ_AUTO_UP_VALUE)
	sw      v1, 0(v0)


	wait_zq_set0:
	li      t0, ZCTRL_STATUS
	li      t1, DDR_PHY_PAD_ZCTRL_STATUS
	lw      t1, 0(t1)
	and     t1, t0
	bne     t1, t0, wait_zq_set0
	nop

	/*zctrl_start = 0*/
	li      v0, DDR_PHY_PAD_CTRL_PROG
	li      v1, (DZQ_UP_SEL0_VALUE|ZCTL_START0_VALUE|ZPROG_VALUE0|ZCLK_DIV_32|DZQ_AUTO_UP_VALUE)
	sw      v1, 0(v0)

	/*OCD of address is pointed to set 0*/
	li      v0, DDR_PHY_ADR_OCD_SEL
	li      v1, DDR_PHY_ADR_OCD_SEL_VALUE
	sw      v1, 0(v0)

	/*OCD of CK is pointed to set 0*/
	li      v0, DDR_PHY_CK_OCD_SEL
	li      v1, DDR_PHY_CK_OCD_SEL_VALUE
	sw      v1, 0(v0)

	/*dzq_up_sel*/
	li      v0, DDR_PHY_PAD_CTRL_PROG
	li      v1, (DZQ_UP_SEL1_VALUE|ZCTL_START0_VALUE|ZPROG_VALUE0|ZCLK_DIV_32|DZQ_AUTO_UP_VALUE)
	sw      v1, 0(v0)

	/*dzq auto up*/
	li      v0, DDR_PHY_PAD_CTRL_PROG
	li      v1, (DZQ_UP_SEL1_VALUE|ZCTL_START0_VALUE|ZPROG_VALUE1|ZCLK_DIV_32|DZQ_AUTO_UP_VALUE)
	sw      v1, 0(v0)

	/*
	DDR3 PUPD III setting:
	OCD/ODT = 34/180ohm
	zprog = 0x43A
	*/
	li      v0, DDR_PHY_PAD_CTRL_PROG
	li      v1, (DZQ_UP_SEL1_VALUE|ZCTL_START1_VALUE|ZPROG_VALUE1|ZCLK_DIV_32|DZQ_AUTO_UP_VALUE)
	sw      v1, 0(v0)

	wait_zq_set1:
	li      t0, ZCTRL_STATUS
	li      t1, DDR_PHY_PAD_ZCTRL_STATUS
	lw      t1, 0(t1)
	and     t1, t0
	bne     t1, t0, wait_zq_set1
	nop

	/*zctrl_start = 0*/
	li      v0, DDR_PHY_PAD_CTRL_PROG
	li      v1, (DZQ_UP_SEL1_VALUE|ZCTL_START0_VALUE|ZPROG_VALUE1|ZCLK_DIV_32|DZQ_AUTO_UP_VALUE)
	sw      v1, 0(v0)

	/*p-odt of dqs_p is pointed to se1(180ohm)*/
	/*n-odt of dqs_p is pointed to se0(120ohm)*/
	li      v0, DDR_PHY_DQS_P_ODT_SEL
	li      v1, 0x00110011
	sw      v1, 0(v0)

	/*p-odt of dqs_n is pointed to se0(120ohm)*/
	/*n-odt of dqs_n is pointed to se1(180ohm)*/
	li      v0, DDR_PHY_DQS_N_ODT_SEL
	li      v1, 0x11001100
	sw      v1, 0(v0)
#endif




	/*TM_DQS_en_slice0*/
	li      v0, DDR_PHY_READ_CTRL_00
	li      v1, DQS_EN_DLY
	sw      v1, 0(v0)

	/*TM_DQS_en_slice1*/
	li      v0, DDR_PHY_READ_CTRL_01
	li      v1, DQS_EN_DLY
	sw      v1, 0(v0)

	li      v0, DDR_PHY_READ_CTRL_2_0
	li      v1, DQS0_READ_ODT_RANGE
	sw      v1, 0(v0)

	li      v0, DDR_PHY_READ_CTRL_2_1
	li      v1, DQS1_READ_ODT_RANGE
	sw      v1, 0(v0)

	/*TM_RD_fifo*/
	li      v0, DDR_PHY_READ_CTRL_1
	li      v1, READ_CTRL_1_VALUE
	sw      v1, 0(v0)


	/*RX delay tap*/
	li      v0, DDR_PHY_DQS_IN_DLY_0_0
	li      v1, DDR_PHY_DQS_IN_DLY_0_0_VALUE
	sw      v1, 0(v0)

	li      v0, DDR_PHY_DQS_IN_DLY_0_1
	li      v1, DDR_PHY_DQS_IN_DLY_0_1_VALUE
	sw      v1, 0(v0)


	li      v0, DDR_PHY_DQS_IN_DLY_1_0
	li      v1, DDR_PHY_DQS_IN_DLY_1_0_VALUE
	sw      v1, 0(v0)

	li      v0, DDR_PHY_DQS_IN_DLY_1_1
	li      v1, DDR_PHY_DQS_IN_DLY_1_1_VALUE
	sw      v1, 0(v0)


	li      v0, DDR_PHY_DQS_IN_DLY_2_0
	li      v1, DDR_PHY_DQS_IN_DLY_2_0_VALUE
	sw      v1, 0(v0)

	li      v0, DDR_PHY_DQS_IN_DLY_2_1
	li      v1, DDR_PHY_DQS_IN_DLY_2_1_VALUE
	sw      v1, 0(v0)

	li      v0, DDR_PHY_DQS_IN_DLY_3_0
	li      v1, DDR_PHY_DQS_IN_DLY_3_0_VALUE
	sw      v1, 0(v0)

	li      v0, DDR_PHY_DQS_IN_DLY_3_1
	li      v1, DDR_PHY_DQS_IN_DLY_3_1_VALUE
	sw      v1, 0(v0)



	/*fw_set_rd_dly*/
	li      v0, DDR_PHY_DPI_CTRL_1
	li      v1, DPI_CTRL_1_SET_RD_DLY
	sw      v1, 0(v0)


	/*fw set mode : HW default*/
	li      v0, DDR_PHY_DPI_CTRL_0
	li      v1, (CMD_OUTPUT_DLY_2T|FW_SET_MODE_HW)
	sw      v1, 0(v0)

	/*change dqsen mode*/
	li      v0, DDR_PHY_DPI_CTRL_0
	li      v1, (CMD_OUTPUT_DLY_2T|FW_SET_MODE_HW|DQS_EN_PUPD3)
	sw      v1, 0(v0)


	li      v0, DDR_PHY_PAD_BUS_1
	li      v1, 0x1f
	sw      v1, 0(v0)


#else
	/*sample edge : both negative*/
	li      v0, 0xb886f00c
        li      v1, 0x6
        sw      v1, 0(v0)
#endif
        /* setup ocp_wrapper */
        li      v0, DDRC_BASE + PCTL_MISC
        li      v1, MISC_VLAUE		/*BST_size, 8; bank_size 8; page_size 2KB*/
        sw      v1, 0(v0)

        /* setup pctl */
        li      v0, DDRC_BASE + PCTL_DCR /* CR_DCR :  DFI rate =2:1 , half DQ, DDR3,*/
        li      v1, DCR_VALUE
        sw      v1, 0(v0)

        li      v0, DDRC_BASE + PCTL_IOCR  /* tphy_wdata =0x101000 , RTK phy*/
        li      v1, IOCR_VALUE
        sw      v1, 0(v0)

        li      v0, DDRC_BASE + PCTL_MR2  /* CR_EMR2: CWL_Lentency=0 */
        li      v1, MR2_VALUE
        sw      v1, 0(v0)

        li      v0, DDRC_BASE + PCTL_MR1  /* CR_MR1 : */
        li      v1, MR1_VALUE
        sw      v1, 0(v0)

        li      v0, DDRC_BASE + PCTL_MR0  /* CR_MR0 */
        li      v1, MR0_VALUE
        sw      v1, 0(v0)
#ifdef DDR_CTL_V2
        li      v0, DDRC_BASE + PCTL_MR_INFO  /* CR_MR_INFO*/
        li      v1, MR_INFO_VALUE
        sw      v1, 0(v0)
#endif
        li      v0, DDRC_BASE + PCTL_DRR  /* CR_DRR : tREF[23:8]=0xdbc,  tRFC[7:0]=0X6 */
/*        li      v1, 0x090e4209        */
        li      v1, DRR_VALUE
        sw      v1, 0(v0)
        li      v0, DDRC_BASE + PCTL_TPR0  /* CR_TPR0: tRTP[15:13]=0x3, tWR[12:9]=3, tRAS[8:4]=2, tRP[3:0]=1 */
        li      v1, TPR0_VALUE
        sw      v1, 0(v0)
        li      v0, DDRC_BASE + PCTL_TPR1  /* CR_TPR1: tRTW[23:20]= 0x05, tWTR[19:17]=0x5, tCCD[16:14]=0x3, tRCD[13:10]=0x1, tRC[9:4]=0x3, tRRD[3:0]=0x3 */
        li      v1, TPR1_VALUE
        sw      v1, 0(v0)
        li      v0, DDRC_BASE + PCTL_TPR2  /* CR_TPR2: tMRD[7:5]=0x4, init_ref_num[3:0]=0x2 */
        li      v1, TPR2_VALUE
        sw      v1, 0(v0)
        li      v0, DDRC_BASE + PCTL_CSR  /* CR_CSR : {bist_idle, dt_idle, mem_idle} [10:8]=7 */
        li      v1, CSR_DIS_BIST_MEM
        sw      v1, 0(v0)
        li      v0, DDRC_BASE + PCTL_CCR  /* CR_CCR : {bist_idle, dt_idle, mem_idle} [10:8]=7 */
        li      v1, DRAM_INIT_EN
        sw      v1, 0(v0)
        /* check DT_mode */
check_DT:
        li      t0, 0x1
        li      t1, DDRC_BASE + PCTL_CCR
        lw      t1, 0(t1)
        and     t1, t0
        bne     t1, t0, check_DT
        nop

	/*wait 200us*/
/*
	li	t0, 0
	li	t1, 0x20
1:
	addu t0, 4
	blt  t0, t1, 1b
	nop
*/
	/*set DDR mode register by DPIN
	DPIN[0]:RST_N, DPIN[1]:CKE*/
	/*set RST_N =0, CKE=0 at least 200us*/
	li 	v0, DDRC_BASE + PCTL_TIE_DPIN
	li 	v1, 0x00
	sw	v1,0(v0)

	li 	v0, DDRC_BASE + PCTL_CCR
	li 	v1, PCTL_DPIN_START
	sw	v1,0(v0)

	/*wait 200us*/
	li	t0, 0
	li	t1, 0x20
1:
	addu t0, 4
	blt  t0, t1, 1b
	nop

	/*set RST_N =1, CKE=0 at least 500us*/
	li 	v0, DDRC_BASE +PCTL_TIE_DPIN
	li 	v1, 0x01
	sw	v1,0(v0)

	li 	v0, DDRC_BASE + PCTL_CCR
	li 	v1, PCTL_DPIN_START
	sw	v1,0(v0)

	/*wait 500us*/
	li	t0, 0
	li	t1, 0x40
1:
	addu t0, 4
	blt  t0, t1, 1b
	nop

	/*set RST_N =1, CKE=1*/
	li 	v0, DDRC_BASE +PCTL_TIE_DPIN
	li 	v1, 0x03
	sw	v1,0(v0)

	li 	v0, DDRC_BASE + PCTL_CCR
	li 	v1, PCTL_DPIN_START
	sw	v1,0(v0)

	/*set mode resgiter 2*/
	li 	v0, DDRC_BASE +PCTL_CMD_DPIN
	li 	v1, (0x40000|MR2_VALUE)
	sw	v1,0(v0)

	li 	v0, DDRC_BASE + PCTL_CCR
	li 	v1, PCTL_DPIN_START
	sw	v1,0(v0)

	/*set mode resgiter 3*/
	li 	v0, DDRC_BASE +PCTL_CMD_DPIN
	li 	v1, (0x60000|MR3_VALUE)
	sw	v1,0(v0)

	li 	v0, DDRC_BASE + PCTL_CCR
	li 	v1, PCTL_DPIN_START
	sw	v1,0(v0)

	/*set mode resgiter 1*/
	li 	v0, DDRC_BASE +PCTL_CMD_DPIN
	li 	v1, (0x20000|MR1_VALUE)
	sw	v1,0(v0)

	li 	v0, DDRC_BASE + PCTL_CCR
	li 	v1, PCTL_DPIN_START
	sw	v1,0(v0)

	/*set mode resgiter 0*/
	li 	v0, DDRC_BASE +PCTL_CMD_DPIN
	li 	v1, (0x00000|MR0_VALUE)
	sw	v1,0(v0)

	li 	v0, DDRC_BASE + PCTL_CCR
	li 	v1, PCTL_DPIN_START
	sw	v1,0(v0)

	li 	v0, DDRC_BASE +PCTL_CMD_DPIN
	li 	v1, 0xc00400
	sw	v1,0(v0)

	li 	v0, DDRC_BASE + PCTL_CCR
	li 	v1, PCTL_DPIN_START
	sw	v1,0(v0)

	li 	v0, DDRC_BASE +PCTL_TIE_DPIN
#ifdef CONFIG_ODT_OFF
	li 	v1, 0x03
#else
	li 	v1, 0x07
#endif
	sw	v1,0(v0)

        /* check Mem_mode */
check_MEM:
	li      v0, DDRC_BASE + PCTL_CSR  /* CR_CSR : MEM_MODE_n[8] */
        li      v1, CSR_MEM_ACCESS_STATE
        sw      v1, 0(v0)
        nop
        nop
        nop
        lw      t1, 0(v0)
        and     t1, v1
        bne     t1, v1, check_MEM
        nop
        jr      ra
        nop
        .end bsp_boot_init_plat


#ifdef CONFIG_SMP
	.globl bsp_boot_secondary
	.ent bsp_boot_secondary

bsp_boot_secondary:
	li	t0, CONFIG_BOOT_ADDR
	la	t1, _start
	sw	t1, 0(t0)
	jr	ra
	nop
        .end bsp_boot_secondary
#endif

