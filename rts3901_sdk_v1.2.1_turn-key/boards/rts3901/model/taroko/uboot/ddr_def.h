#ifndef _DDR_DEF_H_
#define _DDR_DEF_H_

/*************************************
*		DDR controller register		    	 *
*************************************/
#define DDRC_BASE	0xB8010000
#ifdef DDR_CTL_V1
#define PCTL_CCR				0x0
#define PCTL_DCR				0x4
#define PCTL_IOCR				0x8
#define PCTL_CSR				0xc
#define PCTL_DRR				0x10
#define PCTL_TPR0				0x14
#define PCTL_TPR1				0x18
#define PCTL_TPR2				0x1c
#define PCTL_MR0				0x20
#define PCTL_MR1				0x24
#define PCTL_MR2				0x28
#define PCTL_SVN_ID				0xF4
#define PCTL_IDR				0xF8
#define PCTL_MISC				0x224
#define PCTL_WRAP_IDR				0x2A0
#define PCTL_WRAP_VERSION			0x2A4
#endif

#ifdef DDR_CTL_V2
#define PCTL_CCR				0x0
#define PCTL_DCR				0x4
#define PCTL_IOCR				0x8
#define PCTL_CSR				0xc
#define PCTL_DRR				0x10
#define PCTL_TPR0				0x14
#define PCTL_TPR1				0x18
#define PCTL_TPR2				0x1c
#define PCTL_TPR3				0x20
#define PCTL_CMD_DPIN				0x28
#define PCTL_TIE_DPIN				0x2c
#define PCTL_MR_INFO				0x30
#define PCTL_MR0				0x34
#define PCTL_MR1				0x38
#define PCTL_MR2				0x3c
#define PCTL_MR3				0x40
#define PCTL_MR4				0x44
#define PCTL_MR5				0x48
#define PCTL_MR6				0x4c
#define PCTL_SVN_ID				0xF4
#define PCTL_IDR				0xF8
#define PCTL_MISC				0x224
#define PCTL_WRAP_IDR				0x2A0
#define PCTL_WRAP_VERSION			0x2A4
#endif


#define PCTL_CCR_INIT_BFO		0
#define PCTL_CCR_INIT_BFW		1
#define PCTL_CCR_DTT_BFO		1
#define PCTL_CCR_DTT_BFW		1
#define PCTL_CCR_BTT_BFO		2
#define PCTL_CCR_BTT_BFW		1
#define PCTL_CCR_DPIT_BFO		3
#define PCTL_CCR_DPIT_BFW		1
#define PCTL_CCR_FLUSH_FIFO_BFO		8
#define PCTL_CCR_FLUSH_FIFO_BFW		1

#define PCTL_DCR_DDR3_BFO		0
#define PCTL_DCR_DDR3_BFW		3
#define PCTL_DCR_SDR_BFO		3
#define PCTL_DCR_SDR_BFW		1
#define PCTL_DCR_DQ32_BFO		4
#define PCTL_DCR_DQ32_BFW		1
#define PCTL_DCR_HALF_DQ_BFO		5
#define PCTL_DCR_HALF_DQ_BFW       	1
#define PCTL_DCR_DFI_RATE_BFO      	8
#define PCTL_DCR_DFI_RATE_BFW      	3
#define PCTL_DCR_DBI_BFO      	  	17
#define PCTL_DCR_DBI_BFW      		1
#define PCTL_DCR_PAR_BFO      		18
#define PCTL_DCR_PAR_BFW      		1
#define PCTL_DCR_GEAR_DOWN_BFO  	19
#define PCTL_DCR_GEAR_DOWN_BFW		1

#define PCTL_IOCR_RD_PIPE_BFO      	8
#define PCTL_IOCR_RD_PIPE_BFW      	4
#define PCTL_IOCR_TPHY_WD_BFO      	12
#define PCTL_IOCR_TPHY_WD_BFW      	5
#define PCTL_IOCR_TPHY_WL_BFO      	17
#define PCTL_IOCR_TPHY_WL_BFW      	3
#define PCTL_IOCR_TPHY_RD_EN_BFO   	20
#define PCTL_IOCR_TPHY_RD_EN_BFW   	5

#define PCTL_CSR_MEM_IDLE_BFO      	8
#define PCTL_CSR_MEM_IDLE_BFW      	1
#define PCTL_CSR_DT_IDLE_BFO       	9
#define PCTL_CSR_DT_IDLE_BFW       	1
#define PCTL_CSR_BIST_IDLE_BFO     	10
#define PCTL_CSR_BIST_IDLE_BFW     	1
#define PCTL_CSR_DT_FAIL_BFO       	11
#define PCTL_CSR_DT_FAIL_BFW       	1
#define PCTL_CSR_BT_FAIL_BFO       	12
#define PCTL_CSR_BT_FAIL_BFW       	1

#define PCTL_DRR_TRFC_BFO          	0
#define PCTL_DRR_TRFC_BFW          	7
#define PCTL_DRR_TREF_BFO          	8
#define PCTL_DRR_TREF_BFW          	24
#define PCTL_DRR_REF_NUM_BFO       	24
#define PCTL_DRR_REF_NUM_BFW       	4
#define PCTL_DRR_REF_DIS_BFO       	28
#define PCTL_DRR_REF_DIS_BFW       	1

#define PCTL_TPR0_TRP_BFO          	0
#define PCTL_TPR0_TRP_BFW          	4
#define PCTL_TPR0_TRAS_BFO         	4
#define PCTL_TPR0_TRAS_BFW         	5
#define PCTL_TPR0_TWR_BFO         	9
#define PCTL_TPR0_TWR_BFW          	4
#define PCTL_TPR0_TRTP_BFO         	13
#define PCTL_TPR0_TRTP_BFW         	3

#define PCTL_TPR1_TRRD_BFO         	0
#define PCTL_TPR1_TRRD_BFW         	4
#define PCTL_TPR1_TRC_BFO          	4
#define PCTL_TPR1_TRC_BFW          	6
#define PCTL_TPR1_TRCD_BFO         	10
#define PCTL_TPR1_TRCD_BFW         	4
#define PCTL_TPR1_TCCD_BFO         	14
#define PCTL_TPR1_TCCD_BFW         	3
#define PCTL_TPR1_TWTR_BFO         	17
#define PCTL_TPR1_TWTR_BFW         	3
#define PCTL_TPR1_TRTW_BFO         	20
#define PCTL_TPR1_TRTW_BFW         	4
#define PCTL_TPR1_TFAW_BFO         	24
#define PCTL_TPR1_TFAW_BFW         	5

#define PCTL_TPR2_INIT_REF_NUM_BFO 	0
#define PCTL_TPR2_INIT_REF_NUM_BFW 	4
#define PCTL_TPR2_INIT_NS_EN_BFO   	4
#define PCTL_TPR2_INIT_NS_EN_BFW   	1
#define PCTL_TPR2_TMRD_BFO         	5
#define PCTL_TPR2_TMRD_BFW         	3

#define PCTL_MR_CWL_BFO            	0
#define PCTL_MR_CWL_BFW            	5
#define PCTL_MR_CL_BFO            	5
#define PCTL_MR_CL_BFW            	5
#define PCTL_MR_ADDLAT_BFO             	10
#define PCTL_MR_ADDLAT_BFW             	5

#define PCTL_EMR0_CASLAT_BFO 		3
#define PCTL_EMR0_CASLAT_MASK 		0x1D
#define PCTL_EMR0_TWRAUTO_BFO		9
#define PCTL_EMR0_TWRAUTO_BFW		3

#define PCTL_EMR1_ADDLAT_BFO       	3
#define PCTL_EMR1_ADDLAT_BFW       	3

#define PCTL_EMR2_CWL_BFO       	3
#define PCTL_EMR2_CWL_BFW       	3

#define PCTL_CMD_DPIN_RSTN_BFO     	0
#define PCTL_CMD_DPIN_RSTN_BFW     	1
#define PCTL_CMD_DPIN_CKE_BFO      	1
#define PCTL_CMD_DPIN_CKE_BFW      	1
#define PCTL_CMD_DPIN_ODT_BFO      	2
#define PCTL_CMD_DPIN_ODT_BFW      	1

#define PCTL_BCR_STOP_BFO          	0
#define PCTL_BCR_STOP_BFW          	1
#define PCTL_BCR_CMP_BFO           	1
#define PCTL_BCR_CMP_BFW           	1
#define PCTL_BCR_LOOP_BFO          	2
#define PCTL_BCR_LOOP_BFW          	1
#define PCTL_BCR_DIS_MASK_BFO      	3
#define PCTL_BCR_DIS_MASK_BFW      	1
#define PCTL_BCR_AT_STOP_BFO       	4
#define PCTL_BCR_AT_STOP_BFW       	1
#define PCTL_BCR_FLUSH_CMD_BFO     	8
#define PCTL_BCR_FLUSH_CMD_BFW     	1
#define PCTL_BCR_FLUSH_WD_BFO      	9
#define PCTL_BCR_FLUSH_WD_BFW      	1
#define PCTL_BCR_FLUSH_RGD_BFO     	10
#define PCTL_BCR_FLUSH_RGD_BFW     	1
#define PCTL_BCR_FLUSH_RD_BFO      	11
#define PCTL_BCR_FLUSH_RD_BFW      	1
#define PCTL_BCR_FLUSH_RD_EXPC_BFO 	16
#define PCTL_BCR_FLUSH_RD_EXPC_BFW 	14

#define PCTL_BST_ERR_FST_TH_BFO    	0
#define PCTL_BST_ERR_FST_TH_BFW    	12
#define PCTL_BST_ERR_CNT_BFO       	16
#define PCTL_BST_ERR_CNT_BFW       	14

#define PCTL_BSRAM0_CMD_LEVEL_BFO  	0
#define PCTL_BSRAM0_CMD_LEVEL_BFW  	12
#define PCTL_BSRAM0_WD_LEVEL_BFO   	16
#define PCTL_BSRAM0_WD_LEVEL_BFW   	14

#define PCTL_BSRAM1_RG_LEVEL_BFO   	0
#define PCTL_BSRAM1_RG_LEVEL_BFW   	14
#define PCTL_BSRAM1_RD_LEVEL_BFO   	16
#define PCTL_BSRAM1_RD_LEVEL_BFW   	14

#define WRAP_MISC_PAGE_SIZE_BFO    	0
#define WRAP_MISC_PAGE_SIZE_BFW    	4
#define WRAP_MISC_BANK_SIZE_BFO    	4
#define WRAP_MISC_BANK_SIZE_BFW    	2
#define WRAP_MISC_BST_SIZE_BFO     	6
#define WRAP_MISC_BST_SIZE_BFW     	2
#define WRAP_MISC_DDR_PARAL_BFO    	8
#define WRAP_MISC_DDR_PARAL_BFW    	1

#define PCTL_DPIN_START			0x08
#ifndef LANGUAGE_ASSEMBLY
struct ms_rxi310_portmap {
	volatile unsigned int ccr;           /* 0x000 */
	volatile unsigned int dcr;           /* 0x004 */
	volatile unsigned int iocr;          /* 0x008 */
	volatile unsigned int csr;           /* 0x00c */
	volatile unsigned int drr;           /* 0x010 */
	volatile unsigned int tpr0;          /* 0x014 */
	volatile unsigned int tpr1;          /* 0x018 */
	volatile unsigned int tpr2;          /* 0x01c */
	volatile unsigned int tpr3;          /* 0x020 */
	volatile unsigned int cdpindiff;   /* 0x024*/
	volatile unsigned int cdpin;         /* 0x028*/
	volatile unsigned int tdpin;         /* 0x02c */
	volatile unsigned int mr;            /* 0x030 */
	volatile unsigned int emr0;          /* 0x034 */
	volatile unsigned int emr1;          /* 0x038 */
	volatile unsigned int emr2;          /* 0x03c */
	volatile unsigned int emr3;          /* 0x040 */
	volatile unsigned int emr4;          /* 0x044 */
	volatile unsigned int emr5;          /* 0x048 */
	volatile unsigned int emr6;          /* 0x04c */
	volatile unsigned int dllcr3;        /* 0x050 */
	volatile unsigned int dllcr4;        /* 0x054 */
	volatile unsigned int dllcr5;        /* 0x058 */
	volatile unsigned int dllcr6;        /* 0x05c */
	volatile unsigned int dllcr7;        /* 0x060 */
	volatile unsigned int dllcr8;        /* 0x064 */
	volatile unsigned int dqtr0;         /* 0x068 */
	volatile unsigned int dqtr1;         /* 0x06c */
	volatile unsigned int dqtr2;         /* 0x070 */
	volatile unsigned int dqtr3;         /* 0x074 */
	volatile unsigned int dqtr4;         /* 0x078 */
	volatile unsigned int dqtr5;         /* 0x07c */
	volatile unsigned int dqtr6;         /* 0x080 */
	volatile unsigned int dqtr7;         /* 0x084 */
	volatile unsigned int dqstr;         /* 0x088 */
	volatile unsigned int dqsbtr;        /* 0x08c */
	volatile unsigned int odtcr;         /* 0x090 */
	volatile unsigned int dtr0;          /* 0x094 */
	volatile unsigned int dtr1;          /* 0x098 */
	volatile unsigned int dtar;          /* 0x09c */
	volatile unsigned int zqcr0;         /* 0x0a0 */
	volatile unsigned int zqcr1;         /* 0x0a4 */
	volatile unsigned int zqsr;          /* 0x0a8 */
	volatile unsigned int rslr0;         /* 0x0ac */
	volatile unsigned int rslr1;         /* 0x0b0 */
	volatile unsigned int rslr2;         /* 0x0b4 */
	volatile unsigned int rslr3;         /* 0x0b8 */
	volatile unsigned int rdgr0;         /* 0x0bc */
	volatile unsigned int rdgr1;         /* 0x0c0 */
	volatile unsigned int rdgr2;         /* 0x0c4 */
	volatile unsigned int rdgr3;         /* 0x0c8 */
	volatile unsigned int mxsl;          /* 0x0cc */
	volatile unsigned int bcr;           /* 0x0d0 */
	volatile unsigned int bct;           /* 0x0d4 */
	volatile unsigned int bcm;           /* 0x0d8 */
	volatile unsigned int bst;           /* 0x0dc */
	volatile unsigned int bsram0;        /* 0x0e0 */
	volatile unsigned int bsram1;        /* 0x0e4 */
	volatile unsigned int ber;           /* 0x0e8 */
	volatile unsigned int byr;           /* 0x0ec */
	volatile unsigned int err;           /* 0x0f0*/
	volatile unsigned int pctl_svn;      /* 0x0f4 */
	volatile unsigned int pctl_idr;      /* 0x0f8 */

	/* SDR_PHY CONTROL REGISTER*/
	volatile unsigned int phy_dly0;      /* 0x100 */
	volatile unsigned int phy_dly1_rst;  /* 0x104 */
	volatile unsigned int phy_dly_clk;   /* 0x108 */
	volatile unsigned int phy_dly_st;    /* 0x10c */
	volatile unsigned int phy_dly_num;   /* 0x110 */
	volatile unsigned int reserved0[69];

	/* WRAP CONTROL REGISTER*/
	volatile unsigned int misc;          /* 0x224 */
	volatile unsigned int cq_ver;        /* 0x228 */
	volatile unsigned int cq_mon;        /* 0x22c */
	volatile unsigned int wq_ver;        /* 0x230 */
	volatile unsigned int wq_mon;        /* 0x234 */
	volatile unsigned int rq_ver;        /* 0x240 */
	volatile unsigned int rq_mon;        /* 0x244 */
	volatile unsigned int reserved1[22];
	volatile unsigned int wwrap_idr;     /* 0x2a0 */
	volatile unsigned int wrap_svn;      /* 0x2a4 */

}; /*ms_rxi310_portmap*/

#define QFIFO_CMD_BANK_BFO		34 /* [36:34]*/
#define QFIFO_CMD_BANK_BFW		3
#define QFIFO_CMD_PAGE_BFO		19 /* [33:19]*/
#define QFIFO_CMD_PAGE_BFW		15
#define QFIFO_CMD_COLU_BFO		6 /*[15: 6]*/
#define QFIFO_CMD_COLU_BFW		10
#define QFIFO_BST_LEN_BFO		1 /* [5:1]*/
#define QFIFO_BST_LEN_BFW		4
#define QFIFO_CMD_WRRD_BFO		0 /*[0]*/
#define QFIFO_CMD_WRRD_BFW		1

enum qfifo_wrrd {
	RD_CMD      = 0,
	WR_CMD      = 1
};

struct bist_cmd_data_b37 {
	uint32_t        CMD_BANK;
	uint32_t        CMD_PAGE;
	uint32_t        CMD_COLU;
	uint32_t        BST_LEN;
	enum qfifo_wrrd WRRD;
};

struct bist_data_b128 {
	uint32_t        data_3;
	uint32_t        data_2;
	uint32_t        data_1;
	uint32_t        data_0;
};

#endif /* not assembly*/

#define MR3_VALUE 0
#define DLL_RESET (1<<8)
#ifdef CONFIG_DDR_H5TQ2G_112M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			3
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			3
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			8928
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			160000

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			4

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			35000

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			4

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			14000

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			4

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			14000

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			50000

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			40000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			4

/*write lantency: 5*/
#define CWL				5

/*read lantency:6*/
#define CL				6

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			1

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			1

#define CAL_SETTING			0x04


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1

/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO))

#define MR0_VALUE			(CAL_SETTING  << PCTL_EMR0_CASLAT_BFO) | \
					(TWR_AUTO_PRECHARGE_25 << PCTL_EMR0_TWRAUTO_BFO)|DLL_RESET
#define CLW5_SETTING			0
#define MR2_VALUE			(CLW5_SETTING << PCTL_EMR2_CWL_BFO)
#define MR1_VALUE			0x40
#define MR_INFO_VALUE		0x62
#define DDRC_SET_OFFSET			1
#endif

#ifdef CONFIG_DDR_H5TQ2G_400M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			3
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			3
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			2500
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			250000

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			4

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			35000

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			4

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			14000

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			4

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			14000

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			50000

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			40000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			4

/*write lantency: 5*/
#define CWL				5

/*read lantency:6*/
#define CL				6

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			1

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			1

#define CAL_SETTING			0x04


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			(CAL_SETTING  << PCTL_EMR0_CASLAT_BFO) | \
					(TWR_AUTO_PRECHARGE_25 << PCTL_EMR0_TWRAUTO_BFO)|DLL_RESET

/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO)|0x10)
#define CLW5_SETTING			0
#define MR2_VALUE			(CLW5_SETTING << PCTL_EMR2_CWL_BFO)

#define MR1_VALUE			0x40

#define MR_INFO_VALUE		0x62

#define DDRC_SET_OFFSET			1
#endif

#ifdef CONFIG_DDR_H5TQ2G_667M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			3
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			3
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			1500
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			160500

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			4

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			36000

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			4

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			13500

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			4

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			13500

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			49500

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			45000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			4

/*write lantency: 5*/
#define CWL				5

/*read lantency:6*/
#define CL				6

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			1

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			1

#define CAL_SETTING			0x04


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			(CAL_SETTING  << PCTL_EMR0_CASLAT_BFO) | \
					(TWR_AUTO_PRECHARGE_25 << PCTL_EMR0_TWRAUTO_BFO)|DLL_RESET

/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO))

#define MR2_VALUE			0x200
#define MR1_VALUE			0x06
#define MR_INFO_VALUE		0x62
#define DDRC_SET_OFFSET	1
#endif

#ifdef CONFIG_DDR_H5TQ2G_800M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			3
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			3
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			1250
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			160500

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			4

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			37500

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			6

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			13750

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			4

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			13750

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			48750

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			40000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			4

/*write lantency: 5*/
#define CWL				8

/*read lantency:6*/
#define CL				11

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			3

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			2

#define CAL_SETTING			0x0e


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			(CAL_SETTING  << PCTL_EMR0_CASLAT_BFO) | \
					(TWR_AUTO_PRECHARGE_25 << PCTL_EMR0_TWRAUTO_BFO)|DLL_RESET

/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO)|0x10)

#define MR2_VALUE			0x218
#define MR1_VALUE			0x6
#define MR_INFO_VALUE		0xc4
#define DDRC_SET_OFFSET	1
#endif

#ifdef CONFIG_DDR_NT5CC128M16IP_800M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			3
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			3
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			1250
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			160500

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			4

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			37500

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			6

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			13750

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			4

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			13750

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			48750

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			40000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			4

/*write lantency: 5*/
#define CWL				8

/*read lantency:6*/
#define CL				11

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			3

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			2

#define CAL_SETTING			0x0e


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			(CAL_SETTING  << PCTL_EMR0_CASLAT_BFO) | \
					(TWR_AUTO_PRECHARGE_25 << PCTL_EMR0_TWRAUTO_BFO)|DLL_RESET

/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO)|0x10)

#define MR2_VALUE			0x218
#define MR1_VALUE			0x6
#define MR_INFO_VALUE		0xc4
#define DDRC_SET_OFFSET	1
#endif


#ifdef CONFIG_DDR_M15F2G16128A_800M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			3
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			3
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			1250
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			160500

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			4

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			37500

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			6

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			13750

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			4

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			13750

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			48750

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			40000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			4

/*write lantency: 5*/
#define CWL				8

/*read lantency:6*/
#define CL				11

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			3

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			2

#define CAL_SETTING			0x0e


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			(CAL_SETTING  << PCTL_EMR0_CASLAT_BFO) | \
					(TWR_AUTO_PRECHARGE_25 << PCTL_EMR0_TWRAUTO_BFO)|DLL_RESET

/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO)|0x10)

#define MR2_VALUE			0x218
#define MR1_VALUE			0x6
#define MR_INFO_VALUE		0xc4
#define DDRC_SET_OFFSET	1
#endif

#ifdef CONFIG_DDR_M15F1G1664A_800M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			3
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			3
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			1250
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			160500

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			4

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			37500

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			6

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			13750

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			4

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			13750

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			48750

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			40000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			4

/*write lantency: 5*/
#define CWL				8

/*read lantency:6*/
#define CL				11

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			3

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			2

#define CAL_SETTING			0x0e


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			(CAL_SETTING  << PCTL_EMR0_CASLAT_BFO) | \
					(TWR_AUTO_PRECHARGE_25 << PCTL_EMR0_TWRAUTO_BFO)|DLL_RESET

/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO)|0x10)

#define MR2_VALUE			0x218
#define MR1_VALUE			0x6
#define MR_INFO_VALUE		0xc4
#define DDRC_SET_OFFSET	1
#endif

#ifdef CONFIG_DDR_W632GG6KB_667M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			3
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			3
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			1500
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			160500

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			4

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			36000

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			4

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			13500

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			4

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			13500

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			49500

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			45000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			4

/*write lantency: 5*/
#define CWL				7

/*read lantency:6*/
#define CL				9

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			2

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			2

#define CAL_SETTING			0x0a


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			(CAL_SETTING  << PCTL_EMR0_CASLAT_BFO) | \
					(TWR_AUTO_PRECHARGE_25 << PCTL_EMR0_TWRAUTO_BFO)|DLL_RESET

/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO)|0x10)

#define MR2_VALUE			0x210
#define MR1_VALUE			0x06
#define MR_INFO_VALUE		0xa4
#define DDRC_SET_OFFSET	1
#endif


#ifdef CONFIG_DDR_W632GG6KB_800M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			3
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			3
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			1250
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			160500

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			4

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			37500

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			6

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			13750

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			4

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			13750

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			48750

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			40000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			4

/*write lantency: 5*/
#define CWL				8

/*read lantency:6*/
#define CL				11

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			3

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			2

#define CAL_SETTING			0x0e


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			(CAL_SETTING  << PCTL_EMR0_CASLAT_BFO) | \
					(TWR_AUTO_PRECHARGE_25 << PCTL_EMR0_TWRAUTO_BFO)|DLL_RESET


/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO)|0x10)

#define MR2_VALUE			0x218
#define MR1_VALUE			0x6
#define MR_INFO_VALUE		0xc4
#define DDRC_SET_OFFSET	1
#endif

#ifdef CONFIG_DDR_W631GG6KB_800M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			3
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			3
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			1250
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			160500

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			4

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			37500

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			6

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			13750

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			4

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			13750

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			48750

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			40000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			4

/*write lantency: 5*/
#define CWL				8

/*read lantency:6*/
#define CL				11

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			3

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			2

#define CAL_SETTING			0x0e


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			(CAL_SETTING  << PCTL_EMR0_CASLAT_BFO) | \
					(TWR_AUTO_PRECHARGE_25 << PCTL_EMR0_TWRAUTO_BFO)|DLL_RESET


/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO)|0x10)

#define MR2_VALUE			0x218
#define MR1_VALUE			0x6
#define MR_INFO_VALUE		0xc4
#define DDRC_SET_OFFSET	1
#endif

#ifdef CONFIG_DDR_NT5CC64M16IP_800M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			3
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			3
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			1250
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			160500

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			4

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			37500

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			6

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			13750

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			4

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			13750

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			48750

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			40000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			4

/*write lantency: 5*/
#define CWL				8

/*read lantency:6*/
#define CL				11

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			3

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			2

#define CAL_SETTING			0x0e


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			(CAL_SETTING  << PCTL_EMR0_CASLAT_BFO) | \
					(TWR_AUTO_PRECHARGE_25 << PCTL_EMR0_TWRAUTO_BFO)|DLL_RESET

/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO)|0x10)

#define MR2_VALUE			0x218
#define MR1_VALUE			0x6
#define MR_INFO_VALUE		0xc4
#define DDRC_SET_OFFSET	1
#endif


#ifdef CONFIG_DDR3_2GBIT_GENERAL_800M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			3
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			3
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			1250
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			160500

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			4

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			37500

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			6

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			13750

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			4

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			13750

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			48750

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			40000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			4

/*write lantency: 5*/
#define CWL				8

/*read lantency:6*/
#define CL				11

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			3

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			2

#define CAL_SETTING			0x0e


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			(CAL_SETTING  << PCTL_EMR0_CASLAT_BFO) | \
					(TWR_AUTO_PRECHARGE_25 << PCTL_EMR0_TWRAUTO_BFO)|DLL_RESET

/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO)|0x10)

#define MR2_VALUE			0x218
#define MR1_VALUE			0x6
#define MR_INFO_VALUE		0xc4
#define DDRC_SET_OFFSET	1
#endif

#ifdef CONFIG_DDR3_1GBIT_GENERAL_800M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			3
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			3
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			1250
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			160500

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			4

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			37500

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			6

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			13750

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			4

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			13750

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			48750

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			40000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			4

/*write lantency: 5*/
#define CWL				8

/*read lantency:6*/
#define CL				11

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			3

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			2

#define CAL_SETTING			0x0e


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			(CAL_SETTING  << PCTL_EMR0_CASLAT_BFO) | \
					(TWR_AUTO_PRECHARGE_25 << PCTL_EMR0_TWRAUTO_BFO)|DLL_RESET

/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO)|0x10)

#define MR2_VALUE			0x218
#define MR1_VALUE			0x6
#define MR_INFO_VALUE		0xc4
#define DDRC_SET_OFFSET	1
#endif

#ifdef CONFIG_DDR_W9751V6KG_400M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			2
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			2
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			2500
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			105000

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			3

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			45000

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			4

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			15000

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			2

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			15000

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			60000

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			45000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			2

/*write lantency: 5*/
#define CWL				5

/*read lantency:6*/
#define CL				6

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			1

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			1

#define CAL_SETTING			0x04


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			0x1263

/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO)|0x10)

/*MR2*/
#define CLW5_SETTING			0
#define MR2_VALUE			(CLW5_SETTING << PCTL_EMR2_CWL_BFO)
#define MR1_VALUE			0x40
#define MR_INFO_VALUE		0x62
#define DDRC_SET_OFFSET		1
#endif

#ifdef CONFIG_DDR_W9751V6KG_533M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			2
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			2
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			1875
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			105000

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			3

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			45000

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			4

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			13125

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			2

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			13125

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			58125

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			45000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			2

/*write lantency: 5*/
#define CWL				6

/*read lantency:6*/
#define CL				7

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			1

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			1

#define CAL_SETTING			0x04


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			0x1273

/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO)|0x10)

/*MR2*/
#define CLW5_SETTING			0
#define MR2_VALUE			(CLW5_SETTING << PCTL_EMR2_CWL_BFO)
#define MR1_VALUE			0x40
#define MR_INFO_VALUE		0x63
#define DDRC_SET_OFFSET		1
#endif


#ifdef CONFIG_DDR_H5TQ2G_200M
#define DRAM_CHIP_NUM			1
#define DRAM_TYPE			3
#define DRAM_SDR			0
#define DRAM_HALF_DQ			1
#define DRAM_COL_BIT			10
#define DRAM_BANK_BIT			3
#define DRAM_BST_LEN			8
#define DRAM_PERIOD_PS			2500
#define DFI_RATIO			0x02
#define PCTL_PERIOD_PS			(DRAM_PERIOD_PS * DFI_RATIO)

/*Refresh Command to Refresh command:160ns,
JESD79-3F-DDR3, page 170, table 61*/
#define DRAM_TRFC_PS			500000

/*Serial Refresh Command Periodic Interval:60us,
DRAM_TREF_PS < TREF_NUM*(7.8us)*1000000  */
#define TREF_NUM			9
#define DRAM_TREF_PS			60000000

/*Read command to precharge delay cycles, max(4nCK, 7.5ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRTP_TCK			4

/*Write Command to Percharge Command:15ns,
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWR_PS			15000

/*Active Command to Percharge Command:35ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRAS_PS			35000

/*Active Command to Active Command at Different Bank:max(4nCK,6ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TRRD_TCK			4

/*Percharge Command to Active Command:13.75ns,
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRP_PS			14000

/*Read Command to Write Command:8,
large than(DRAM read latency + 6 - DRAM write latency)*/
#define DRAM_TRTW_TCK			8

/*Write Command to Read Command:max(4nCK,7.5ns),
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TWTR_TCK			4

/*Read/Write Command to Read/Write Command:4ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TCCD_TCK			4

/*Active Command to Read/Write Command:13.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRCD_PS			14000

/*Active Command to Active Command at Same Bank:48.75ns
JESD79-3F-DDR3, page 174, table 65*/
#define DRAM_TRC_PS			50000

/* Active window:37.5ns
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TFAW_PS			40000

/*Mode Register Command to The Next Valid Command:4
JESD79-3F-DDR3, page 183, table 68*/
#define DRAM_TMRD_TCK			4

/*write lantency: 5*/
#define CWL				5

/*read lantency:6*/
#define CL				6

/*additional lantency*/
#define AL				0

/*delay latency from DFI read command to
dfi_rddata_en signal. If it works on DDR,
the smallest value is 1*/
#define TPHY_RDATA_EN			1

/*delay latency from dfi_wrdata_en signal
to dfi_wddata. If it works on DDR,
the smallest value is 1*/
#define TPHY_WDATA			1

#define CAL_SETTING			0x04


/*MR0*/

#define TWR_AUTO_PRECHARGE_25		1
#define MR0_VALUE			(CAL_SETTING  << PCTL_EMR0_CASLAT_BFO) | \
					(TWR_AUTO_PRECHARGE_25 << PCTL_EMR0_TWRAUTO_BFO)

/*IOCR*/
#define IOCR_VALUE			((TPHY_RDATA_EN<<PCTL_IOCR_TPHY_RD_EN_BFO)|\
					(TPHY_WDATA<<PCTL_IOCR_TPHY_WD_BFO)|0x10)
#define CLW5_SETTING			0
#define MR2_VALUE			(CLW5_SETTING << PCTL_EMR2_CWL_BFO)

#define MR1_VALUE			0x40
#define MR_INFO_VALUE		0x62
#define DDRC_SET_OFFSET			1
#endif


/*CCR*/
#define DRAM_INIT_EN			0x01

/*DCR*/
#define DCR_VALUE			((DFI_RATIO << PCTL_DCR_DFI_RATE_BFO)| \
					(DRAM_HALF_DQ << PCTL_DCR_HALF_DQ_BFO)|\
					(DRAM_SDR<<PCTL_DCR_SDR_BFO)|(DRAM_TYPE<<PCTL_DCR_DDR3_BFO))



/*CSR*/
#define DISABLE_BIST_FUNC		1
#define DISABLE_MEMORY_ACCESS		1
#define MEM_MEM_ACCESS_STATE		0
#define DISABLE_DT_MODE			1
#define CSR_DIS_BIST_MEM		((DISABLE_BIST_FUNC<<PCTL_CSR_BIST_IDLE_BFO)|\
					(DISABLE_DT_MODE<<PCTL_CSR_DT_IDLE_BFO)|\
					(DISABLE_MEMORY_ACCESS<<PCTL_CSR_MEM_IDLE_BFO))
#define CSR_MEM_ACCESS_STATE		((DISABLE_BIST_FUNC<<PCTL_CSR_BIST_IDLE_BFO)|\
					(DISABLE_DT_MODE<<PCTL_CSR_DT_IDLE_BFO)|\
					(MEM_MEM_ACCESS_STATE<<PCTL_CSR_MEM_IDLE_BFO))

/*DRR*/
#define REF_DIS				1
#define REF_EN				0
#define DRR_VALUE			((REF_EN<<PCTL_DRR_REF_DIS_BFO)|(TREF_NUM << PCTL_DRR_REF_NUM_BFO)|\
					((DRAM_TREF_PS/PCTL_PERIOD_PS - 0x100) << PCTL_DRR_TREF_BFO)|\
					((DRAM_TRFC_PS/PCTL_PERIOD_PS + 1) << PCTL_DRR_TRFC_BFO))

/*TPR0*/
#define TPR0_VALUE			(((DRAM_TRTP_TCK/DFI_RATIO + DDRC_SET_OFFSET) << PCTL_TPR0_TRTP_BFO) | \
					((DRAM_TWR_PS / PCTL_PERIOD_PS + DDRC_SET_OFFSET) << PCTL_TPR0_TWR_BFO) | \
					((DRAM_TRAS_PS / PCTL_PERIOD_PS + DDRC_SET_OFFSET) << PCTL_TPR0_TRAS_BFO) | \
					((DRAM_TRP_PS / PCTL_PERIOD_PS + DDRC_SET_OFFSET) << PCTL_TPR0_TRP_BFO))

/*TPR1*/
#define TPR1_VALUE			(((DRAM_TFAW_PS / PCTL_PERIOD_PS + DDRC_SET_OFFSET) << PCTL_TPR1_TFAW_BFO) | \
					((DRAM_TRTW_TCK / DFI_RATIO + DDRC_SET_OFFSET) << PCTL_TPR1_TRTW_BFO) | \
					((DRAM_TWTR_TCK / DFI_RATIO + DDRC_SET_OFFSET) << PCTL_TPR1_TWTR_BFO) | \
					((DRAM_TCCD_TCK / DFI_RATIO) << PCTL_TPR1_TCCD_BFO) | \
					((DRAM_TRCD_PS / PCTL_PERIOD_PS + DDRC_SET_OFFSET) << PCTL_TPR1_TRCD_BFO) | \
					((DRAM_TRC_PS / PCTL_PERIOD_PS + DDRC_SET_OFFSET) << PCTL_TPR1_TRC_BFO) | \
					((DRAM_TRRD_TCK / DFI_RATIO + DDRC_SET_OFFSET) << PCTL_TPR1_TRRD_BFO))

/*TPR2*/
#define INIT_REF_NUM			4
#define INIT_NS_EN			1
#define TPR2_VALUE			(((DRAM_TMRD_TCK / DFI_RATIO + 1) << PCTL_TPR2_TMRD_BFO) | \
					(INIT_NS_EN << PCTL_TPR2_INIT_NS_EN_BFO) | \
					(INIT_REF_NUM << PCTL_TPR2_INIT_REF_NUM_BFO))

/*MR_INFO*/
/*#define MR_INFO_VALUE			(((CWL/DFI_RATIO) << PCTL_MR_CWL_BFO) | \
					((CL/DFI_RATIO) << PCTL_MR_CL_BFO) |\
					((AL/DFI_RATIO) << PCTL_MR_ADDLAT_BFO))
*/

/*MR1*/
#define AL0_SETTING			0
#define RZQ_DIV4			0x04
#define RZQ_DIV7			0x02




/*MISC*/
#define MISC_VLAUE			((((DRAM_BST_LEN - 4) >> 2) << WRAP_MISC_BST_SIZE_BFO) | \
					((DRAM_BANK_BIT - 1) << WRAP_MISC_BANK_SIZE_BFO) | \
					(((DRAM_COL_BIT - 8) + (DRAM_CHIP_NUM - 1) + 1) << WRAP_MISC_PAGE_SIZE_BFO))

/********************************
*	system ddr config register		*
********************************/
#define SYS_DDR_CONFIG_ADDR			0xB886000C
#define ASSERT_ALL_RST				0
#define DEASSERT_RST_N				0x08
#define DEASSERT_PTR_RST_N			0x04
#define DEASSERT_CRT_RST_N			0x02
#define DEASSERT_PLL_LDO_RST_N			0x01
#define SYS_DDR_CFG_STEP1			ASSERT_ALL_RST
#define SYS_DDR_CFG_STEP2			DEASSERT_CRT_RST_N
#define SYS_DDR_CFG_STEP3			(DEASSERT_CRT_RST_N|DEASSERT_PLL_LDO_RST_N)
#define SYS_DDR_CFG_STEP4_1			(DEASSERT_CRT_RST_N|DEASSERT_PLL_LDO_RST_N|DEASSERT_RST_N)
#define SYS_DDR_CFG_STEP4_2			(DEASSERT_CRT_RST_N|DEASSERT_PLL_LDO_RST_N|\
						DEASSERT_PTR_RST_N|DEASSERT_RST_N)

#define SYS_DDR_STATUS_ADDR			0xB8860010
#define DPI_PLL_STABLE				0x01

#ifdef CONFIG_TARGET_ASIC
/********************************
*		ddr phy register*
********************************/
#define DDR_PHY_PLL_CTL		0xB8080000
#define DDR_PHY_PLL_CTL_MASK	0xffffffdf


#define DDR_PHY_PLL_CTL0			0xB8080004
#define MCK_CLK0_EN				0x10000
#define MCK_CMD_EN				0x20000
#define MCK_DQS0_EN				0x40000
#define MCK_DQS1_EN				0x80000
#define MCK_DQS2_EN				0x100000
#define MCK_DQS3_EN				0x200000
#define MCK_DQ0_EN				0x400000
#define MCK_DQ1_EN				0x800000
#define MCK_DQ2_EN				0x1000000
#define MCK_DQ3_EN				0x2000000
#define MCK_CLK1_EN				0x4000000
#define DPI_POST_PI_CL				0x1000
#define DPI_POST_PI_EN				0xFFF
#define DDR_PHY_PLL_CTL0_MASK			0xffffefff
#define DDR_PHY_PLL_CLKEN_MASK			0xf800ffff
#define DDR_PHY_PLL_CLKEN_VALUE	0x000f0000


#define DDR_PHY_PLL_CTL1			0xB8080008
#define MCK_CLK0_OUTPUT_EN			0x10000
#define MCK_CMD_OUTPUT_EN			0x20000
#define MCK_DQS0_OUTPUT_EN			0x40000
#define MCK_DQS1_OUTPUT_EN			0x80000
#define MCK_DQS2_OUTPUT_EN			0x100000
#define MCK_DQS3_OUTPUT_EN			0x200000
#define MCK_DQ0_OUTPUT_EN			0x400000
#define MCK_DQ1_OUTPUT_EN			0x800000
#define MCK_DQ2_OUTPUT_EN			0x1000000
#define MCK_DQ3_OUTPUT_EN			0x2000000
#define MCK_CLK1_OUTPUT_EN			0x4000000
#define MCK_CLK0_SYNC_EN		0x01
#define MCK_CMD_SYNC_EN			0x02
#define MCK_DQS0_SYNC_EN			0x4
#define MCK_DQS1_SYNC_EN			0x8
#define MCK_DQS2_SYNC_EN			0x10
#define MCK_DQS3_SYNC_EN			0x20
#define MCK_DQ0_SYNC_EN			0x40
#define MCK_DQ1_SYNC_EN			0x80
#define MCK_DQ2_SYNC_EN			0x100
#define MCK_DQ3_SYNC_EN			0x200
#define MCK_CLK1_SYNC_EN			0x400
#define MCK_OUTPUT_SETTING			(MCK_DQ1_OUTPUT_EN|MCK_DQ0_OUTPUT_EN|MCK_DQS1_OUTPUT_EN|\
						MCK_DQS0_OUTPUT_EN|MCK_CMD_OUTPUT_EN|MCK_CLK0_OUTPUT_EN)
#define DDR_PHY_PLL_CLKOE_MASK		0xf800ffff
#define DDR_PHY_PLL_CLKOE_VALUE		0x000f0000

#define DDR_PHY_PLL_PI0					0xB8080010
#define DDR_PHY_PLL_PI0_MASK			0xffffffc0

#define DDR_PHY_PLL_PI1					0xB8080014

#define DDR_PHY_PLL_PI2					0xB8080018

#define DDR_PHY_PLL_CTL3				0xB808001C
#define DDR_PHY_PLL_CTL3_MASK			0xCC24080B

#define DDR_PHY_SSC1				0xB8080024
/*SSC profile step number:[27:16]*/
#define DPI_TBASE				0

#define DDR_PHY_F_MASK				0xffffe000

#define DDR_PHY_SSC2				0xB8080028
#define DDR_PHY_N_MASK				0xffffff00

#define REF_PLL					25000000
#define DDR_PLL					REF_PLL * (DPI_N_CODE + 2 + DPI_F_CODE/8192)

#define DDR_PHY_PAD_CTRL_PROG		0xb808021c
#define ZCLK_SEL_MASK				0xf9ffffff
#define ZCLK_DIV_32					0x4000000
#define DZQ_UP_SEL_MASK			0x8fffffff
#define DZQ_UP_SEL0_VALUE			0
#define DZQ_UP_SEL1_VALUE			0x10000000
#define DZQ_AUTO_UP_MASK			0xf7ffffff
#define DZQ_AUTO_UP_VALUE			0x08000000
#define ZPROG_MASK					0xffffc000
#define ZCTL_START_MASK			0xfeffffff
#define ZCTL_START1_VALUE			0x01000000
#define ZCTL_START0_VALUE			0x00000000

#define DDR_PHY_PAD_ZCTRL_STATUS		0xb8080220
#define ZCTRL_STATUS					0x40000000

#define DDR_PHY_WRLVL_CTRL			0xB8080228
/*cs output delay enable:[29]*/
#define CS_PH_EN				0x20000000
/*cs output delay:[23:21]*/
#define CS_PH_SEL_1_CYCLE			0x00
#define CS_PH_SEL_2_CYCLE			0x200000
#define CS_PH_SEL_3_CYCLE			0x400000
#define CS_PH_SEL_4_CYCLE			0x600000
#define CS_PH_SEL_5_CYCLE			0x800000
#define CS_PH_SEL_6_CYCLE			0xa00000
#define CS_PH_SEL_7_CYCLE			0xc00000
#define CS_PH_SEL_8_CYCLE			0xe00000
/*Write level feedback MUX selection:[11:8], [7:4]*/
#define WRLVL_FB_DQ0				0x100
#define WRLVL_FB_DQ1				0x200
#define WRLVL_FB_DQ2				0x400
#define WRLVL_FB_DQ3				0x800
#define WRLVL_FB_DQS0				0x10
#define WRLVL_FB_DQS1				0x20
#define WRLVL_FB_DQS2				0x40
#define WRLVL_FB_DQS3				0x80
#define WRLVL_CTRL_SETTING			(CS_PH_EN|CS_PH_SEL_4_CYCLE|WRLVL_FB_DQ0|\
						WRLVL_FB_DQ1|WRLVL_FB_DQ2|WRLVL_FB_DQ3| \
						WRLVL_FB_DQS0|WRLVL_FB_DQS1|WRLVL_FB_DQS2|WRLVL_FB_DQS3)
#define WRLVL_CTRL_MASK			0xdf1fffff


#define DDR_PHY_DQ_DLY_0_0			0xB8080238
#define DDR_PHY_DQ_DLY_0_1			0xB808023C
#define DDR_PHY_DQ_DLY_1				0xB8080248
#define DDR_PHY_DQ_DLY_2				0xB808024C
#define DDR_PHY_READ_CTRL_00			0xB8080254
#define DDR_PHY_READ_CTRL_01			0xB8080258
/*dqs_en latency from read command: [4:0]*/
#define TM_DQS_EN_2_CYCLE			0x04
#define TM_DQS_EN_2_CYCLE_5			0x05
#define TM_DQS_EN_3_CYCLE			0x06
#define TM_DQS_EN_3_CYCLE_5			0x07
#define TM_DQS_EN_4_CYCLE			0x08
#define TM_DQS_EN_4_CYCLE_5			0x09
#define TM_DQS_EN_5_CYCLE			0x0a
#define TM_DQS_EN_5_CYCLE_5			0x0b
#define TM_DQS_EN_6_CYCLE			0x0c
#define TM_DQS_EN_6_CYCLE_5			0x0d
#define TM_DQS_EN_7_CYCLE			0x0e
#define TM_DQS_EN_7_CYCLE_5			0x0f
#define TM_DQS_EN_8_CYCLE			0x10
#define TM_DQS_EN_8_CYCLE_5			0x11
#define TM_DQS_EN_9_CYCLE			0x12
#define TM_DQS_EN_9_CYCLE_5			0x13
#define TM_DQS_EN_10_CYCLE			0x14
#define TM_DQS_EN_10_CYCLE_5			0x15
#define TM_DQS_EN_11_CYCLE			0x16
#define TM_DQS_EN_11_CYCLE_5			0x17
#define TM_DQS_EN_12_CYCLE			0x18
#define TM_DQS_EN_12_CYCLE_5			0x19
#define TM_DQS_EN_13_CYCLE			0x1a
#define TM_DQS_EN_13_CYCLE_5			0x1b
#define TM_DQS_EN_14_CYCLE			0x1c
#define TM_DQS_EN_14_CYCLE_5			0x1d
#define TM_DQS_EN_15_CYCLE			0x1e
#define TM_DQS_EN_15_CYCLE_5			0x1f
#define READ_CTRL_00_SETTING			TM_DQS_EN_15_CYCLE_5
#define READ_CTRL_01_SETTING			TM_DQS_EN_15_CYCLE_5
#define READ_CTRL_0_MASK				0xffffffe0

#define DDR_PHY_READ_CTRL_1			0xB8080264
/*Central point of DQS enable calibration (FW change value during dqsen_cal_en == 0):[17:12]*/
#define DQSEN_CAL_EN				0x10000
/*read FIFO latency from read command:[4:0]*/
#define TM_RD_FIFO_2_CYCLE			0x04
#define TM_RD_FIFO_2_CYCLE_5			0x05
#define TM_RD_FIFO_3_CYCLE			0x06
#define TM_RD_FIFO_3_CYCLE_5			0x07
#define TM_RD_FIFO_4_CYCLE			0x08
#define TM_RD_FIFO_4_CYCLE_5			0x09
#define TM_RD_FIFO_5_CYCLE			0x0a
#define TM_RD_FIFO_5_CYCLE_5			0x0b
#define TM_RD_FIFO_6_CYCLE			0x0c
#define TM_RD_FIFO_6_CYCLE_5			0x0d
#define TM_RD_FIFO_7_CYCLE			0x0e
#define TM_RD_FIFO_7_CYCLE_5			0x0f
#define TM_RD_FIFO_8_CYCLE			0x10
#define TM_RD_FIFO_8_CYCLE_5			0x11
#define TM_RD_FIFO_9_CYCLE			0x12
#define TM_RD_FIFO_9_CYCLE_5			0x13
#define TM_RD_FIFO_10_CYCLE			0x14
#define TM_RD_FIFO_10_CYCLE_5			0x15
#define TM_RD_FIFO_11_CYCLE			0x16
#define TM_RD_FIFO_11_CYCLE_5			0x17
#define TM_RD_FIFO_12_CYCLE			0x18
#define TM_RD_FIFO_12_CYCLE_5			0x19
#define TM_RD_FIFO_13_CYCLE			0x1a
#define TM_RD_FIFO_13_CYCLE_5			0x1b
#define TM_RD_FIFO_14_CYCLE			0x1c
#define TM_RD_FIFO_14_CYCLE_5			0x1d
#define TM_RD_FIFO_15_CYCLE			0x1e
#define TM_RD_FIFO_15_CYCLE_5			0x1f
#define READ_CTRL_1_SETTING			(DQSEN_CAL_EN|TM_RD_FIFO_6_CYCLE)
#define READ_CTRL_1_MASK				0xffffffe0


#define DDR_PHY_READ_CTRL_2_0			0xB8080268
#define DDR_PHY_READ_CTRL_2_1			0xB808026c
#define DDR_PHY_READ_CTRL_2_VALUE	0x00C03fff
#define DDR_PHY_READ_CTRL_5			0xB808028C
#define DDR_PHY_READ_CTRL_5_MASK		0xff3fffff


#define DDR_PHY_DQS_IN_DLY_0_0		0xB80802B8
#define DDR_PHY_DQS_IN_DLY_0_1		0xB80802BC
#define DDR_PHY_DQS_IN_DLY_0_2		0xB80802C0
#define DDR_PHY_DQS_IN_DLY_0_3		0xB80802C4

#define DDR_PHY_DQS_IN_DLY_1_0		0xB80802C8
#define DDR_PHY_DQS_IN_DLY_1_1		0xB80802CC
#define DDR_PHY_DQS_IN_DLY_1_2		0xB80802D0
#define DDR_PHY_DQS_IN_DLY_1_3		0xB80802D4

#define DDR_PHY_DQS_IN_DLY_2_0		0xB80802D8
#define DDR_PHY_DQS_IN_DLY_2_1		0xB80802DC
#define DDR_PHY_DQS_IN_DLY_2_2		0xB80802E0
#define DDR_PHY_DQS_IN_DLY_2_3		0xB80802E4

#define DDR_PHY_DQS_IN_DLY_3_0		0xB80802E8
#define DDR_PHY_DQS_IN_DLY_3_1		0xB80802EC
#define DDR_PHY_DQS_IN_DLY_3_2		0xB80802F0
#define DDR_PHY_DQS_IN_DLY_3_3		0xB80802F4

#define DDR_PHY_DQS_P_ODT_SEL			0xB808035C
#define DQS_P_P_ODT_MASK				0xfff0fff0
#define DQS_P_P_ODT_SET0				0
#define DQS_P_P_ODT_SET1				0x00010001
#define DQS_P_N_ODT_MASK				0xf0fff0ff
#define DQS_P_N_ODT_SET0				0
#define DQS_P_N_ODT_SET1				0x01000100

#define DDR_PHY_DQS_N_ODT_SEL			0xB8080360
#define DQS_N_P_ODT_MASK				0xfff0fff0
#define DQS_N_P_ODT_SET0				0
#define DQS_N_P_ODT_SET1				0x00010001
#define DQS_N_N_ODT_MASK				0xf0fff0ff
#define DQS_N_N_ODT_SET0				0
#define DQS_N_N_ODT_SET1				0x01000100


#define DDR_PHY_ADR_OCD_SEL			0xb8080368
#define DDR_PHY_ADR_OCD_SEL_MASK		0
#define DDR_PHY_ADR_OCD_SEL_VALUE	0

#define DDR_PHY_CK_OCD_SEL			0xb808036c
#define DDR_PHY_CK_OCD_SEL_MASK		0
#define DDR_PHY_CK_OCD_SEL_VALUE	0

#define DDR_PHY_PAD_BUS_1			0xb8080374
#define DDR_PHY_PAD_BUS_1_MASK	0xffffffef
#define DDR_PHY_PAD_BUS_1_VALUE	0x0f

#define DDR_PHY_DPI_CTRL_0			0xB808037C
/*Command output delay:[27:25]*/
#define CMD_PH_SEL_1_CYCLE			0x00
#define CMD_PH_SEL_2_CYCLE			0x2000000
#define CMD_PH_SEL_3_CYCLE			0x4000000
#define CMD_PH_SEL_4_CYCLE			0x6000000
#define CMD_PH_SEL_5_CYCLE			0x8000000
#define CMD_PH_SEL_6_CYCLE			0xa000000
#define CMD_PH_SEL_7_CYCLE			0xc000000
#define CMD_PH_SEL_8_CYCLE			0xe000000
/*Command output delay enable:[24]*/
#define CMD_PH_EN				0x1000000
/*Update delay chain setting after write data enable:[23:21]*/
#define WR_UPDATA_DLY				0xE00000
/*Update delay chain setting after read data enable:[20:16]*/
#define RD_UPDATA_DLY				0xF0000
/*Update delay chain setting after receiving refresh command:[13:8]*/
#define REF_UPDATA_DLY				0x2000
/*3-point calibration set mode[5:4]
[00] during no read operation
[01] during refresh
[10] immediately
[11] disable auto calibration
*/
#define CAL_SET_MODE				0x10
/*Reset read FIFO pointer mode[3:2]
[00] during no read operation
[01] during refresh
[10] disable reset FIFO pointer
[11] force reset FIFO pointer
*/
#define RST_FIFO_MODE				0x00
/*FW set delay chain of data slice mode[1:0]
[00] Read : during no read
	Write: during refresh
[01] during refresh
[10] immediately
[11] reserved
*/
#define DQS_EN_PUPD3				0x40
#define FW_SET_MODE_IMME			0x02
#define FW_SET_MODE_HW			0
#define DPI_CTRL_0_SETTING			(CMD_PH_SEL_4_CYCLE|CMD_PH_EN|WR_UPDATA_DLY|\
						RD_UPDATA_DLY|REF_UPDATA_DLY|CAL_SET_MODE)
#define CMD_OUTPUT_DLY_2T			0x3ef2000

#define DPI_CTRL_0_CMD_DLY_MASK	0xf0ffffff
#define DPI_CTRL_SET_MODE_MASK	0xfffffffc
#define DPI_CTRL_DQS_EN_MASK		0xffffff3f


#define DDR_PHY_DPI_CTRL_1			0xB8080380
/*Enable of the following row:[3]*/
#define WRITE_EN_1				0x08
#define WRITE_EN_0				0x02
/*FW set read delay chain of data slice (one cycle pulse, HW auto clear) : [2] */
#define FW_SET_RD_DLY				0x04
#define FW_SET_WR_DLY				0x01
#define DPI_CTRL_1_SET_RD_DLY			(WRITE_EN_1|FW_SET_RD_DLY)
#define DPI_CTRL_1_SET_WR_DLY			(WRITE_EN_0|FW_SET_WR_DLY)
#define FW_SET_RD_DLY_MASK		0xfffffff3

#define DDR_PHY_BIST_2TO1_0			0xB80803D8
/*For address and command slice:[31]
1 '1:2 frequency ratio between PHY & MC
0 '1:1 frequency ratio between PHY & MC
*/
#define ADDR_CMD_RATIO_2to1			0x80000000
/*For data slice:[28]
1 '1:2 frequency ratio between PHY & MC
0 '1:1 frequency ratio between PHY & MC
*/
#define DATA_TATIO_2to1				0x10000000
#define DFI_RATIO_SETTING			(ADDR_CMD_RATIO_2to1|DATA_TATIO_2to1)
#define ADDR_CMD_RATIO_2to1_MASK		0x6fffffff

#define RD_CMD_DLY	0x40000000
#define WR_CMD_DLY 0x20000000


#ifdef CONFIG_DDR_H5TQ2G_200M
/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x0E

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0

#define DDR_PHY_PLL_CTL0_VALUE		0x1000

#define DDR_PHY_PLL_CTL3_VALUE	0x25105134

#define DDR_PHY_DQS_IN_DLY_0_0_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_0_1_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_0_2_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_0_3_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_1_0_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_1_1_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_1_2_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_1_3_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_2_0_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_2_1_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_2_2_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_2_3_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_3_0_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_3_1_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_3_2_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_3_3_VALUE		0x14141414

#define DDR_PHY_PLL_CTL_VALUE	0x40

#define DDR_PHY_PLL_PI0_VALUE			0x0000000a

#define DDR_PHY_READ_CTRL_5_VALUE	0x00c03fff

#define ZPROG_VALUE0				0x243a
#define ZPROG_VALUE1				0x43a

#define READ_CTRL_1_VALUE				0x1000a

#define TM_RD_FIFO					TM_DQS_EN_6_CYCLE

#define CS_OUTPUT_DLY_2T			0x20200ff0

#define MCK_EN_SETTING				(DPI_POST_PI_CL|MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN)
#endif


#ifdef CONFIG_DDR_H5TQ2G_400M
/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x1E

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0

#define DDR_PHY_PLL_CTL0_VALUE		0x1000

#define DDR_PHY_PLL_CTL3_VALUE	0x25105134

#define DDR_PHY_DQS_IN_DLY_0_0_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_0_1_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_0_2_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_0_3_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_1_0_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_1_1_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_1_2_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_1_3_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_2_0_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_2_1_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_2_2_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_2_3_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_3_0_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_3_1_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_3_2_VALUE		0x14141414
#define DDR_PHY_DQS_IN_DLY_3_3_VALUE		0x14141414

#define DDR_PHY_PLL_CTL_VALUE	0x40

#define DDR_PHY_PLL_PI0_VALUE			0x0000000a

#define DDR_PHY_READ_CTRL_5_VALUE	0x00c03fff

#define ZPROG_VALUE0				0x243a
#define ZPROG_VALUE1				0x43a

#define READ_CTRL_1_VALUE				0x1000a

#define TM_RD_FIFO					TM_DQS_EN_6_CYCLE

#define CS_OUTPUT_DLY_2T			0x20200ff0

#define MCK_EN_SETTING				(DPI_POST_PI_CL|MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN)
#endif

#ifdef CONFIG_DDR_H5TQ2G_667M
/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x33

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0xa3e

#define DDR_PHY_PLL_CTL0_VALUE		0x1000

#define DDR_PHY_PLL_CTL3_VALUE	0x23905354

#define DDR_PHY_DQS_IN_DLY_0_0_VALUE		0x0a0a0a0b
#define DDR_PHY_DQS_IN_DLY_0_1_VALUE		0x09090a0b
#define DDR_PHY_DQS_IN_DLY_0_2_VALUE		0x0d0d0d0d
#define DDR_PHY_DQS_IN_DLY_0_3_VALUE		0x0d0d0d0d
#define DDR_PHY_DQS_IN_DLY_1_0_VALUE		0x0a0a0a0a
#define DDR_PHY_DQS_IN_DLY_1_1_VALUE		0x0b0c0c0c
#define DDR_PHY_DQS_IN_DLY_1_2_VALUE		0x0d0d0d0d
#define DDR_PHY_DQS_IN_DLY_1_3_VALUE		0x0d0d0d0d
#define DDR_PHY_DQS_IN_DLY_2_0_VALUE		0x0a0a0a0b
#define DDR_PHY_DQS_IN_DLY_2_1_VALUE		0x09090a0b
#define DDR_PHY_DQS_IN_DLY_2_2_VALUE		0x0d0d0d0d
#define DDR_PHY_DQS_IN_DLY_2_3_VALUE		0x0d0d0d0d
#define DDR_PHY_DQS_IN_DLY_3_0_VALUE		0x0a0a0a0a
#define DDR_PHY_DQS_IN_DLY_3_1_VALUE		0x0b0c0c0c
#define DDR_PHY_DQS_IN_DLY_3_2_VALUE		0x0d0d0d0d
#define DDR_PHY_DQS_IN_DLY_3_3_VALUE		0x0d0d0d0d

#define DDR_PHY_PLL_CTL_VALUE	0x40

#define DDR_PHY_PLL_PI0_VALUE			0x1010001D

#define DDR_PHY_READ_CTRL_5_VALUE	0x00c03fff

#define ZPROG_VALUE0				0x10fe
#define ZPROG_VALUE1				0x017e

#define READ_CTRL_1_VALUE				0x1000c
#define TM_RD_FIFO					TM_DQS_EN_7_CYCLE

#define DQ_DLY_0_VALUE_0				0x66666665

#define DQ_DLY_0_VALUE_1				0x56566656

#define DQ_DLY_1_VALUE_0				0x33333838

#define DQ_DLY_1_VALUE_1				0x33333838

#define DQ_DLY_2_VALUE				0x3388

#define CS_OUTPUT_DLY_2T			0x20230030

#define TX_DLY_CHAIN

#define READ_CTRL_2_SET

#define MCK_CLK_SYNC
#define MCK_EN_SETTING				(DPI_POST_PI_CL|MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN)
#define TX_DLY_CHAIN_1333
#endif

#ifdef CONFIG_DDR_H5TQ2G_800M
/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x3e

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0

#define DDR_PHY_PLL_CTL3_VALUE	0x27905354

#define CLK_PI					0x8
#define DQS_PI					0x5
#define DQ_PI					0x13

#define DDR_PHY_PLL_CTL0_VALUE_0 (DPI_POST_PI_CL|DPI_POST_PI_EN)

#define DDR_PHY_PLL_CTL0_VALUE_1	(DPI_POST_PI_CL|MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN|\
						MCK_DQ0_EN|MCK_DQ1_EN)

#define DDR_PHY_PLL_CTL1_VALUE	(MCK_CLK0_OUTPUT_EN|MCK_CMD_OUTPUT_EN|MCK_DQS0_OUTPUT_EN|\
						MCK_DQS1_OUTPUT_EN|MCK_DQ0_OUTPUT_EN|MCK_DQ1_OUTPUT_EN|\
						MCK_DQ0_SYNC_EN|MCK_DQ1_SYNC_EN)

#define DDR_PHY_BIST_2TO1_0_VALUE		(DFI_RATIO_SETTING|RD_CMD_DLY|WR_CMD_DLY)

#define ZPROG_VALUE0				0x10fe
#define ZPROG_VALUE1				0x017e

#define DDR_PHY_DPI_CTRL_0_VALUE		0x3ef2010
#define DDR_PHY_WRLVL_CTRL_VALUE		0x20200ff0
#define DQS_EN_DLY					TM_DQS_EN_6_CYCLE_5

#define RX_FIFO_DLY					0x0c

#ifdef CONFIG_BGA234_DEMO_BOARD
#define RX_DQ0_P_DLY				0x08
#define RX_DQ0_N_DLY				0x09
#define RX_DQ8_P_DLY				0x08
#define RX_DQ8_N_DLY				0x09
#else
#define RX_DQ0_P_DLY				0x09
#define RX_DQ1_P_DLY				0x09
#define RX_DQ2_P_DLY				0x0a
#define RX_DQ3_P_DLY				0x09
#define RX_DQ4_P_DLY				0x09
#define RX_DQ5_P_DLY				0x09
#define RX_DQ6_P_DLY				0x09
#define RX_DQ7_P_DLY				0x09

#define RX_DQ8_P_DLY				0x0b
#define RX_DQ9_P_DLY				0x09
#define RX_DQ10_P_DLY				0x09
#define RX_DQ11_P_DLY				0x09
#define RX_DQ12_P_DLY				0x0c
#define RX_DQ13_P_DLY				0x0b
#define RX_DQ14_P_DLY				0x0b
#define RX_DQ15_P_DLY				0x0a

#define RX_DQ0_N_DLY				0x0a
#define RX_DQ1_N_DLY				0x0b
#define RX_DQ2_N_DLY				0x0b
#define RX_DQ3_N_DLY				0x0b
#define RX_DQ4_N_DLY				0x0c
#define RX_DQ5_N_DLY				0x0a
#define RX_DQ6_N_DLY				0x09
#define RX_DQ7_N_DLY				0x0b

#define RX_DQ8_N_DLY				0x0c
#define RX_DQ9_N_DLY				0x0a
#define RX_DQ10_N_DLY				0x0a
#define RX_DQ11_N_DLY				0x0a
#define RX_DQ12_N_DLY				0x0d
#define RX_DQ13_N_DLY				0x0c
#define RX_DQ14_N_DLY				0x0d
#define RX_DQ15_N_DLY				0x0c
#endif

#define DQ_DQS_MUX_TABLE_SEL		0x20200330

#define DQS0_READ_ODT_RANGE		0x1fc
#define DQS1_READ_ODT_RANGE		0x1fe

#endif


#ifdef CONFIG_DDR_NT5CC128M16IP_800M
/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x3e

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0

#define DDR_PHY_PLL_CTL3_VALUE	0x27905354

#define CLK_PI					0x8
#define DQS_PI					0x5
#define DQ_PI					0x13

#define DDR_PHY_PLL_CTL0_VALUE_0 (DPI_POST_PI_CL|DPI_POST_PI_EN)

#define DDR_PHY_PLL_CTL0_VALUE_1	(DPI_POST_PI_CL|MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN|\
						MCK_DQ0_EN|MCK_DQ1_EN)

#define DDR_PHY_PLL_CTL1_VALUE	(MCK_CLK0_OUTPUT_EN|MCK_CMD_OUTPUT_EN|MCK_DQS0_OUTPUT_EN|\
						MCK_DQS1_OUTPUT_EN|MCK_DQ0_OUTPUT_EN|MCK_DQ1_OUTPUT_EN|\
						MCK_DQ0_SYNC_EN|MCK_DQ1_SYNC_EN)

#define DDR_PHY_BIST_2TO1_0_VALUE		(DFI_RATIO_SETTING|RD_CMD_DLY|WR_CMD_DLY)

#define ZPROG_VALUE0				0x10fe
#define ZPROG_VALUE1				0x017e

#define DDR_PHY_DPI_CTRL_0_VALUE		0x3ef2010
#define DDR_PHY_WRLVL_CTRL_VALUE		0x20200ff0
#define DQS_EN_DLY					TM_DQS_EN_6_CYCLE_5

#define RX_FIFO_DLY					0x0c

#ifdef CONFIG_BGA234_DEMO_BOARD
#define RX_DQ0_P_DLY				0x08
#define RX_DQ0_N_DLY				0x09
#define RX_DQ8_P_DLY				0x08
#define RX_DQ8_N_DLY				0x09
#else
#define RX_DQ0_P_DLY				0x09
#define RX_DQ1_P_DLY				0x09
#define RX_DQ2_P_DLY				0x0a
#define RX_DQ3_P_DLY				0x09
#define RX_DQ4_P_DLY				0x09
#define RX_DQ5_P_DLY				0x09
#define RX_DQ6_P_DLY				0x09
#define RX_DQ7_P_DLY				0x09

#define RX_DQ8_P_DLY				0x0b
#define RX_DQ9_P_DLY				0x09
#define RX_DQ10_P_DLY				0x09
#define RX_DQ11_P_DLY				0x09
#define RX_DQ12_P_DLY				0x0c
#define RX_DQ13_P_DLY				0x0b
#define RX_DQ14_P_DLY				0x0b
#define RX_DQ15_P_DLY				0x0a

#define RX_DQ0_N_DLY				0x0a
#define RX_DQ1_N_DLY				0x0b
#define RX_DQ2_N_DLY				0x0b
#define RX_DQ3_N_DLY				0x0b
#define RX_DQ4_N_DLY				0x0c
#define RX_DQ5_N_DLY				0x0a
#define RX_DQ6_N_DLY				0x09
#define RX_DQ7_N_DLY				0x0b

#define RX_DQ8_N_DLY				0x0c
#define RX_DQ9_N_DLY				0x0a
#define RX_DQ10_N_DLY				0x0a
#define RX_DQ11_N_DLY				0x0a
#define RX_DQ12_N_DLY				0x0d
#define RX_DQ13_N_DLY				0x0c
#define RX_DQ14_N_DLY				0x0d
#define RX_DQ15_N_DLY				0x0c
#endif

#define DQ_DQS_MUX_TABLE_SEL		0x20200330

#define DQS0_READ_ODT_RANGE		0x1fc
#define DQS1_READ_ODT_RANGE		0x1fe


#endif

#ifdef CONFIG_DDR_NT5CC64M16IP_800M
/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x3e

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0

#define DDR_PHY_PLL_CTL3_VALUE	0x27905354

#define CLK_PI					0xa
#define DQS_PI					0x5
#define DQ_PI					0x13

#define DDR_PHY_PLL_CTL0_VALUE_0 (DPI_POST_PI_CL|DPI_POST_PI_EN)

#define DDR_PHY_PLL_CTL0_VALUE_1	(DPI_POST_PI_CL|MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN|\
						MCK_DQ0_EN|MCK_DQ1_EN)

#define DDR_PHY_PLL_CTL1_VALUE	(MCK_CLK0_OUTPUT_EN|MCK_CMD_OUTPUT_EN|MCK_DQS0_OUTPUT_EN|\
						MCK_DQS1_OUTPUT_EN|MCK_DQ0_OUTPUT_EN|MCK_DQ1_OUTPUT_EN|\
						MCK_DQ0_SYNC_EN|MCK_DQ1_SYNC_EN)

#define DDR_PHY_BIST_2TO1_0_VALUE		(DFI_RATIO_SETTING|RD_CMD_DLY|WR_CMD_DLY)

#define ZPROG_VALUE0				0x10fe
#define ZPROG_VALUE1				0x017e

#define DDR_PHY_DPI_CTRL_0_VALUE		0x3ef2010
#define DDR_PHY_WRLVL_CTRL_VALUE		0x20200ff0
#define DQS_EN_DLY					TM_DQS_EN_6_CYCLE_5

#define RX_FIFO_DLY					0x0c

#ifdef CONFIG_BGA234_DEMO_BOARD
#define RX_DQ0_P_DLY				0x08
#define RX_DQ0_N_DLY				0x09
#define RX_DQ8_P_DLY				0x08
#define RX_DQ8_N_DLY				0x0a
#else
#define RX_DQ0_P_DLY				0x09
#define RX_DQ1_P_DLY				0x09
#define RX_DQ2_P_DLY				0x0a
#define RX_DQ3_P_DLY				0x09
#define RX_DQ4_P_DLY				0x09
#define RX_DQ5_P_DLY				0x09
#define RX_DQ6_P_DLY				0x09
#define RX_DQ7_P_DLY				0x09

#define RX_DQ8_P_DLY				0x0b
#define RX_DQ9_P_DLY				0x09
#define RX_DQ10_P_DLY				0x09
#define RX_DQ11_P_DLY				0x09
#define RX_DQ12_P_DLY				0x0c
#define RX_DQ13_P_DLY				0x0b
#define RX_DQ14_P_DLY				0x0b
#define RX_DQ15_P_DLY				0x0a

#define RX_DQ0_N_DLY				0x0a
#define RX_DQ1_N_DLY				0x0b
#define RX_DQ2_N_DLY				0x0b
#define RX_DQ3_N_DLY				0x0b
#define RX_DQ4_N_DLY				0x0c
#define RX_DQ5_N_DLY				0x0a
#define RX_DQ6_N_DLY				0x09
#define RX_DQ7_N_DLY				0x0b

#define RX_DQ8_N_DLY				0x0c
#define RX_DQ9_N_DLY				0x0a
#define RX_DQ10_N_DLY				0x0a
#define RX_DQ11_N_DLY				0x0a
#define RX_DQ12_N_DLY				0x0d
#define RX_DQ13_N_DLY				0x0c
#define RX_DQ14_N_DLY				0x0d
#define RX_DQ15_N_DLY				0x0c
#endif

#define DQ_DQS_MUX_TABLE_SEL		0x20200330

#define DQS0_READ_ODT_RANGE		0x1fc
#define DQS1_READ_ODT_RANGE		0x1fe


#endif

#ifdef CONFIG_DDR_W632GG6KB_667M
/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x33

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0xa3e

#define DDR_PHY_PLL_CTL3_VALUE	0x27905354

#define CLK_PI					0x12
#define DQS_PI					0x12
#define DQ_PI					0x1f

#define DDR_PHY_PLL_CTL0_VALUE_0 (DPI_POST_PI_CL|DPI_POST_PI_EN)

#define DDR_PHY_PLL_CTL0_VALUE_1	(DPI_POST_PI_CL|MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN|\
						MCK_DQ0_EN|MCK_DQ1_EN)

#define DDR_PHY_PLL_CTL1_VALUE	(MCK_CLK0_OUTPUT_EN|MCK_CMD_OUTPUT_EN|MCK_DQS0_OUTPUT_EN|\
						MCK_DQS1_OUTPUT_EN|MCK_DQ0_OUTPUT_EN|MCK_DQ1_OUTPUT_EN|\
						MCK_DQ0_SYNC_EN|MCK_DQ1_SYNC_EN|MCK_CLK0_SYNC_EN|MCK_DQS0_SYNC_EN|MCK_DQS1_SYNC_EN)

#define DDR_PHY_BIST_2TO1_0_VALUE		DFI_RATIO_SETTING

#define ZPROG_VALUE0				0x10fe
#define ZPROG_VALUE1				0x017e

#define DDR_PHY_DPI_CTRL_0_VALUE		0x3ef2010
#define DDR_PHY_WRLVL_CTRL_VALUE		0x20200ff0
#define DQS_EN_DLY				0x10

#define RX_FIFO_DLY					0x0d

#ifdef CONFIG_BGA234_DEMO_BOARD
#define RX_DQ0_P_DLY				0x09
#define RX_DQ8_P_DLY				0x0a
#define RX_DQ0_N_DLY				0x07
#define RX_DQ8_N_DLY				0x09
#else
#define RX_DQ0_P_DLY				0x09
#define RX_DQ1_P_DLY				0x09
#define RX_DQ2_P_DLY				0x0a
#define RX_DQ3_P_DLY				0x09
#define RX_DQ4_P_DLY				0x09
#define RX_DQ5_P_DLY				0x09
#define RX_DQ6_P_DLY				0x09
#define RX_DQ7_P_DLY				0x09

#define RX_DQ8_P_DLY				0x0b
#define RX_DQ9_P_DLY				0x09
#define RX_DQ10_P_DLY				0x09
#define RX_DQ11_P_DLY				0x09
#define RX_DQ12_P_DLY				0x0c
#define RX_DQ13_P_DLY				0x0b
#define RX_DQ14_P_DLY				0x0b
#define RX_DQ15_P_DLY				0x0a

#define RX_DQ0_N_DLY				0x0a
#define RX_DQ1_N_DLY				0x0b
#define RX_DQ2_N_DLY				0x0b
#define RX_DQ3_N_DLY				0x0b
#define RX_DQ4_N_DLY				0x0c
#define RX_DQ5_N_DLY				0x0a
#define RX_DQ6_N_DLY				0x09
#define RX_DQ7_N_DLY				0x0b

#define RX_DQ8_N_DLY				0x0c
#define RX_DQ9_N_DLY				0x0a
#define RX_DQ10_N_DLY				0x0a
#define RX_DQ11_N_DLY				0x0a
#define RX_DQ12_N_DLY				0x0d
#define RX_DQ13_N_DLY				0x0c
#define RX_DQ14_N_DLY				0x0d
#define RX_DQ15_N_DLY				0x0c
#endif
#define DQ_DQS_MUX_TABLE_SEL		0x20230030

#define DQS0_READ_ODT_RANGE		0x1ff
#define DQS1_READ_ODT_RANGE		0x1ff

#endif

#ifdef CONFIG_DDR_W632GG6KB_800M
/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x3e

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0

#define DDR_PHY_PLL_CTL3_VALUE	0x27905354

#define CLK_PI					0x8
#define DQS_PI					0x5
#define DQ_PI					0x13

#define DDR_PHY_PLL_CTL0_VALUE_0 (DPI_POST_PI_CL|DPI_POST_PI_EN)

#define DDR_PHY_PLL_CTL0_VALUE_1	(DPI_POST_PI_CL|MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN|\
						MCK_DQ0_EN|MCK_DQ1_EN)

#define DDR_PHY_PLL_CTL1_VALUE	(MCK_CLK0_OUTPUT_EN|MCK_CMD_OUTPUT_EN|MCK_DQS0_OUTPUT_EN|\
						MCK_DQS1_OUTPUT_EN|MCK_DQ0_OUTPUT_EN|MCK_DQ1_OUTPUT_EN|\
						MCK_DQ0_SYNC_EN|MCK_DQ1_SYNC_EN)

#define DDR_PHY_BIST_2TO1_0_VALUE		(DFI_RATIO_SETTING|RD_CMD_DLY|WR_CMD_DLY)

#define ZPROG_VALUE0				0x10fe
#define ZPROG_VALUE1				0x017e

#define DDR_PHY_DPI_CTRL_0_VALUE		0x3ef2010
#define DDR_PHY_WRLVL_CTRL_VALUE		0x20200ff0
#define DQS_EN_DLY					TM_DQS_EN_6_CYCLE_5

#define RX_FIFO_DLY					0x0c

#ifdef CONFIG_BGA234_DEMO_BOARD
#define RX_DQ0_P_DLY				0x08
#define RX_DQ0_N_DLY				0x09
#define RX_DQ8_P_DLY				0x08
#define RX_DQ8_N_DLY				0x09
#else
#define RX_DQ0_P_DLY				0x09
#define RX_DQ1_P_DLY				0x09
#define RX_DQ2_P_DLY				0x0a
#define RX_DQ3_P_DLY				0x09
#define RX_DQ4_P_DLY				0x09
#define RX_DQ5_P_DLY				0x09
#define RX_DQ6_P_DLY				0x09
#define RX_DQ7_P_DLY				0x09

#define RX_DQ8_P_DLY				0x0b
#define RX_DQ9_P_DLY				0x09
#define RX_DQ10_P_DLY				0x09
#define RX_DQ11_P_DLY				0x09
#define RX_DQ12_P_DLY				0x0c
#define RX_DQ13_P_DLY				0x0b
#define RX_DQ14_P_DLY				0x0b
#define RX_DQ15_P_DLY				0x0a

#define RX_DQ0_N_DLY				0x0a
#define RX_DQ1_N_DLY				0x0b
#define RX_DQ2_N_DLY				0x0b
#define RX_DQ3_N_DLY				0x0b
#define RX_DQ4_N_DLY				0x0c
#define RX_DQ5_N_DLY				0x0a
#define RX_DQ6_N_DLY				0x09
#define RX_DQ7_N_DLY				0x0b

#define RX_DQ8_N_DLY				0x0c
#define RX_DQ9_N_DLY				0x0a
#define RX_DQ10_N_DLY				0x0a
#define RX_DQ11_N_DLY				0x0a
#define RX_DQ12_N_DLY				0x0d
#define RX_DQ13_N_DLY				0x0c
#define RX_DQ14_N_DLY				0x0d
#define RX_DQ15_N_DLY				0x0c
#endif

#define DQ_DQS_MUX_TABLE_SEL		0x20200330

#define DQS0_READ_ODT_RANGE		0x1fc
#define DQS1_READ_ODT_RANGE		0x1fe

#endif

#ifdef CONFIG_DDR_W631GG6KB_800M
/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x3e

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0

#define DDR_PHY_PLL_CTL3_VALUE	0x27905354

#define CLK_PI					0xe
#define DQS_PI					0x11
#define DQ_PI					0x1d

#define DDR_PHY_PLL_CTL0_VALUE_0 (DPI_POST_PI_CL|DPI_POST_PI_EN)

#define DDR_PHY_PLL_CTL0_VALUE_1	(DPI_POST_PI_CL|MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN|\
						MCK_DQ0_EN|MCK_DQ1_EN)

#define DDR_PHY_PLL_CTL1_VALUE	(MCK_CLK0_OUTPUT_EN|MCK_CMD_OUTPUT_EN|MCK_DQS0_OUTPUT_EN|\
						MCK_DQS1_OUTPUT_EN|MCK_DQ0_OUTPUT_EN|MCK_DQ1_OUTPUT_EN|\
						MCK_DQ0_SYNC_EN|MCK_DQ1_SYNC_EN|MCK_DQS0_SYNC_EN|MCK_DQS1_SYNC_EN)

#define DDR_PHY_BIST_2TO1_0_VALUE		(DFI_RATIO_SETTING|RD_CMD_DLY|WR_CMD_DLY)

#define ZPROG_VALUE0				0x10fe
#define ZPROG_VALUE1				0x017e

#define DDR_PHY_DPI_CTRL_0_VALUE		0x3ef2010
#define DDR_PHY_WRLVL_CTRL_VALUE		0x20200ff0
#define DQS_EN_DLY					TM_DQS_EN_6_CYCLE_5

#define RX_FIFO_DLY					0x0c

#ifdef CONFIG_BGA234_DEMO_BOARD
#define RX_DQ0_P_DLY				0x07
#define RX_DQ0_N_DLY				0x07
#define RX_DQ8_P_DLY				0x08
#define RX_DQ8_N_DLY				0x08
#else
#define RX_DQ0_P_DLY				0x09
#define RX_DQ1_P_DLY				0x09
#define RX_DQ2_P_DLY				0x0a
#define RX_DQ3_P_DLY				0x09
#define RX_DQ4_P_DLY				0x09
#define RX_DQ5_P_DLY				0x09
#define RX_DQ6_P_DLY				0x09
#define RX_DQ7_P_DLY				0x09

#define RX_DQ8_P_DLY				0x0b
#define RX_DQ9_P_DLY				0x09
#define RX_DQ10_P_DLY				0x09
#define RX_DQ11_P_DLY				0x09
#define RX_DQ12_P_DLY				0x0c
#define RX_DQ13_P_DLY				0x0b
#define RX_DQ14_P_DLY				0x0b
#define RX_DQ15_P_DLY				0x0a

#define RX_DQ0_N_DLY				0x0a
#define RX_DQ1_N_DLY				0x0b
#define RX_DQ2_N_DLY				0x0b
#define RX_DQ3_N_DLY				0x0b
#define RX_DQ4_N_DLY				0x0c
#define RX_DQ5_N_DLY				0x0a
#define RX_DQ6_N_DLY				0x09
#define RX_DQ7_N_DLY				0x0b

#define RX_DQ8_N_DLY				0x0c
#define RX_DQ9_N_DLY				0x0a
#define RX_DQ10_N_DLY				0x0a
#define RX_DQ11_N_DLY				0x0a
#define RX_DQ12_N_DLY				0x0d
#define RX_DQ13_N_DLY				0x0c
#define RX_DQ14_N_DLY				0x0d
#define RX_DQ15_N_DLY				0x0c
#endif

#define DQ_DQS_MUX_TABLE_SEL		0x20230030

#define DQS0_READ_ODT_RANGE		0x1fc
#define DQS1_READ_ODT_RANGE		0x1fe

#endif

#ifdef CONFIG_DDR_M15F2G16128A_800M
/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x3e

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0

#define DDR_PHY_PLL_CTL3_VALUE	0x27905354

#define CLK_PI					0x8
#define DQS_PI					0x5
#define DQ_PI					0x13

#define DDR_PHY_PLL_CTL0_VALUE_0 (DPI_POST_PI_CL|DPI_POST_PI_EN)

#define DDR_PHY_PLL_CTL0_VALUE_1	(DPI_POST_PI_CL|MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN|\
						MCK_DQ0_EN|MCK_DQ1_EN)

#define DDR_PHY_PLL_CTL1_VALUE	(MCK_CLK0_OUTPUT_EN|MCK_CMD_OUTPUT_EN|MCK_DQS0_OUTPUT_EN|\
						MCK_DQS1_OUTPUT_EN|MCK_DQ0_OUTPUT_EN|MCK_DQ1_OUTPUT_EN|\
						MCK_DQ0_SYNC_EN|MCK_DQ1_SYNC_EN)

#define DDR_PHY_BIST_2TO1_0_VALUE		(DFI_RATIO_SETTING|RD_CMD_DLY|WR_CMD_DLY)

#define ZPROG_VALUE0				0x10fe
#define ZPROG_VALUE1				0x017e

#define DDR_PHY_DPI_CTRL_0_VALUE		0x3ef2010
#define DDR_PHY_WRLVL_CTRL_VALUE		0x20200ff0
#define DQS_EN_DLY					TM_DQS_EN_6_CYCLE_5

#define RX_FIFO_DLY					0x0c

#ifdef CONFIG_BGA234_DEMO_BOARD
#define RX_DQ0_P_DLY				0x08
#define RX_DQ0_N_DLY				0x09
#define RX_DQ8_P_DLY				0x08
#define RX_DQ8_N_DLY				0x09
#else
#define RX_DQ0_P_DLY				0x09
#define RX_DQ1_P_DLY				0x09
#define RX_DQ2_P_DLY				0x0a
#define RX_DQ3_P_DLY				0x09
#define RX_DQ4_P_DLY				0x09
#define RX_DQ5_P_DLY				0x09
#define RX_DQ6_P_DLY				0x09
#define RX_DQ7_P_DLY				0x09

#define RX_DQ8_P_DLY				0x0b
#define RX_DQ9_P_DLY				0x09
#define RX_DQ10_P_DLY				0x09
#define RX_DQ11_P_DLY				0x09
#define RX_DQ12_P_DLY				0x0c
#define RX_DQ13_P_DLY				0x0b
#define RX_DQ14_P_DLY				0x0b
#define RX_DQ15_P_DLY				0x0a

#define RX_DQ0_N_DLY				0x0a
#define RX_DQ1_N_DLY				0x0b
#define RX_DQ2_N_DLY				0x0b
#define RX_DQ3_N_DLY				0x0b
#define RX_DQ4_N_DLY				0x0c
#define RX_DQ5_N_DLY				0x0a
#define RX_DQ6_N_DLY				0x09
#define RX_DQ7_N_DLY				0x0b

#define RX_DQ8_N_DLY				0x0c
#define RX_DQ9_N_DLY				0x0a
#define RX_DQ10_N_DLY				0x0a
#define RX_DQ11_N_DLY				0x0a
#define RX_DQ12_N_DLY				0x0d
#define RX_DQ13_N_DLY				0x0c
#define RX_DQ14_N_DLY				0x0d
#define RX_DQ15_N_DLY				0x0c
#endif

#define DQ_DQS_MUX_TABLE_SEL		0x20200330

#define DQS0_READ_ODT_RANGE		0x1fc
#define DQS1_READ_ODT_RANGE		0x1fe

#endif

#ifdef CONFIG_DDR_M15F1G1664A_800M
/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x3e

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0

#define DDR_PHY_PLL_CTL3_VALUE	0x27905354

#define CLK_PI					0xa
#define DQS_PI					0x7
#define DQ_PI					0x14

#define DDR_PHY_PLL_CTL0_VALUE_0 (DPI_POST_PI_CL|DPI_POST_PI_EN)

#define DDR_PHY_PLL_CTL0_VALUE_1	(DPI_POST_PI_CL|MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN|\
						MCK_DQ0_EN|MCK_DQ1_EN)

#define DDR_PHY_PLL_CTL1_VALUE	(MCK_CLK0_OUTPUT_EN|MCK_CMD_OUTPUT_EN|MCK_DQS0_OUTPUT_EN|\
						MCK_DQS1_OUTPUT_EN|MCK_DQ0_OUTPUT_EN|MCK_DQ1_OUTPUT_EN|\
						MCK_DQ0_SYNC_EN|MCK_DQ1_SYNC_EN)

#define DDR_PHY_BIST_2TO1_0_VALUE		(DFI_RATIO_SETTING|RD_CMD_DLY|WR_CMD_DLY)

#define ZPROG_VALUE0				0x10fe
#define ZPROG_VALUE1				0x017e

#define DDR_PHY_DPI_CTRL_0_VALUE		0x3ef2010
#define DDR_PHY_WRLVL_CTRL_VALUE		0x20200ff0
#define DQS_EN_DLY					TM_DQS_EN_7_CYCLE

#define RX_FIFO_DLY					0x0c

#ifdef CONFIG_BGA234_DEMO_BOARD
#define RX_DQ0_P_DLY				0x08
#define RX_DQ0_N_DLY				0x08
#define RX_DQ8_P_DLY				0x08
#define RX_DQ8_N_DLY				0x09
#else
#define RX_DQ0_P_DLY				0x09
#define RX_DQ1_P_DLY				0x09
#define RX_DQ2_P_DLY				0x0a
#define RX_DQ3_P_DLY				0x09
#define RX_DQ4_P_DLY				0x09
#define RX_DQ5_P_DLY				0x09
#define RX_DQ6_P_DLY				0x09
#define RX_DQ7_P_DLY				0x09

#define RX_DQ8_P_DLY				0x0b
#define RX_DQ9_P_DLY				0x09
#define RX_DQ10_P_DLY				0x09
#define RX_DQ11_P_DLY				0x09
#define RX_DQ12_P_DLY				0x0c
#define RX_DQ13_P_DLY				0x0b
#define RX_DQ14_P_DLY				0x0b
#define RX_DQ15_P_DLY				0x0a

#define RX_DQ0_N_DLY				0x0a
#define RX_DQ1_N_DLY				0x0b
#define RX_DQ2_N_DLY				0x0b
#define RX_DQ3_N_DLY				0x0b
#define RX_DQ4_N_DLY				0x0c
#define RX_DQ5_N_DLY				0x0a
#define RX_DQ6_N_DLY				0x09
#define RX_DQ7_N_DLY				0x0b

#define RX_DQ8_N_DLY				0x0c
#define RX_DQ9_N_DLY				0x0a
#define RX_DQ10_N_DLY				0x0a
#define RX_DQ11_N_DLY				0x0a
#define RX_DQ12_N_DLY				0x0d
#define RX_DQ13_N_DLY				0x0c
#define RX_DQ14_N_DLY				0x0d
#define RX_DQ15_N_DLY				0x0c
#endif

#define DQ_DQS_MUX_TABLE_SEL		0x20200330

#define DQS0_READ_ODT_RANGE		0x1fc
#define DQS1_READ_ODT_RANGE		0x1fe

#endif

#ifdef CONFIG_DDR3_2GBIT_GENERAL_800M
/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x3e

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0

#define DDR_PHY_PLL_CTL3_VALUE	0x27905354

#define CLK_PI					0x9
#define DQS_PI					0x9
#define DQ_PI					0x17

#define DDR_PHY_PLL_CTL0_VALUE_0 (DPI_POST_PI_CL|DPI_POST_PI_EN)

#define DDR_PHY_PLL_CTL0_VALUE_1	(DPI_POST_PI_CL|MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN|\
						MCK_DQ0_EN|MCK_DQ1_EN)

#define DDR_PHY_PLL_CTL1_VALUE	(MCK_CLK0_OUTPUT_EN|MCK_CMD_OUTPUT_EN|MCK_DQS0_OUTPUT_EN|\
						MCK_DQS1_OUTPUT_EN|MCK_DQ0_OUTPUT_EN|MCK_DQ1_OUTPUT_EN|\
						MCK_DQ0_SYNC_EN|MCK_DQ1_SYNC_EN)

#define DDR_PHY_BIST_2TO1_0_VALUE		(DFI_RATIO_SETTING|RD_CMD_DLY|WR_CMD_DLY)

#define ZPROG_VALUE0				0x10fe
#define ZPROG_VALUE1				0x017e

#define DDR_PHY_DPI_CTRL_0_VALUE		0x3ef2010
#define DDR_PHY_WRLVL_CTRL_VALUE		0x20200ff0
#define DQS_EN_DLY					TM_DQS_EN_6_CYCLE_5

#define RX_FIFO_DLY					0x0c

#ifdef CONFIG_BGA234_DEMO_BOARD
#define RX_DQ0_P_DLY				0x09
#define RX_DQ0_N_DLY				0x09
#define RX_DQ8_P_DLY				0x08
#define RX_DQ8_N_DLY				0x09
#else
#define RX_DQ0_P_DLY				0x09
#define RX_DQ1_P_DLY				0x09
#define RX_DQ2_P_DLY				0x0a
#define RX_DQ3_P_DLY				0x09
#define RX_DQ4_P_DLY				0x09
#define RX_DQ5_P_DLY				0x09
#define RX_DQ6_P_DLY				0x09
#define RX_DQ7_P_DLY				0x09

#define RX_DQ8_P_DLY				0x0b
#define RX_DQ9_P_DLY				0x09
#define RX_DQ10_P_DLY				0x09
#define RX_DQ11_P_DLY				0x09
#define RX_DQ12_P_DLY				0x0c
#define RX_DQ13_P_DLY				0x0b
#define RX_DQ14_P_DLY				0x0b
#define RX_DQ15_P_DLY				0x0a

#define RX_DQ0_N_DLY				0x0a
#define RX_DQ1_N_DLY				0x0b
#define RX_DQ2_N_DLY				0x0b
#define RX_DQ3_N_DLY				0x0b
#define RX_DQ4_N_DLY				0x0c
#define RX_DQ5_N_DLY				0x0a
#define RX_DQ6_N_DLY				0x09
#define RX_DQ7_N_DLY				0x0b

#define RX_DQ8_N_DLY				0x0c
#define RX_DQ9_N_DLY				0x0a
#define RX_DQ10_N_DLY				0x0a
#define RX_DQ11_N_DLY				0x0a
#define RX_DQ12_N_DLY				0x0d
#define RX_DQ13_N_DLY				0x0c
#define RX_DQ14_N_DLY				0x0d
#define RX_DQ15_N_DLY				0x0c
#endif

#define DQ_DQS_MUX_TABLE_SEL		0x20230030

#define DQS0_READ_ODT_RANGE		0x1fc
#define DQS1_READ_ODT_RANGE		0x1fe

#endif

#ifdef CONFIG_DDR3_1GBIT_GENERAL_800M
/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x3e

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0

#define DDR_PHY_PLL_CTL3_VALUE	0x27905354

#define CLK_PI					0xc
#define DQS_PI					0xc
#define DQ_PI					0x19

#define DDR_PHY_PLL_CTL0_VALUE_0 (DPI_POST_PI_CL|DPI_POST_PI_EN)

#define DDR_PHY_PLL_CTL0_VALUE_1	(DPI_POST_PI_CL|MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN|\
						MCK_DQ0_EN|MCK_DQ1_EN)

#define DDR_PHY_PLL_CTL1_VALUE	(MCK_CLK0_OUTPUT_EN|MCK_CMD_OUTPUT_EN|MCK_DQS0_OUTPUT_EN|\
						MCK_DQS1_OUTPUT_EN|MCK_DQ0_OUTPUT_EN|MCK_DQ1_OUTPUT_EN|\
						MCK_DQ0_SYNC_EN|MCK_DQ1_SYNC_EN)

#define DDR_PHY_BIST_2TO1_0_VALUE		(DFI_RATIO_SETTING|RD_CMD_DLY|WR_CMD_DLY)

#define ZPROG_VALUE0				0x10fe
#define ZPROG_VALUE1				0x017e

#define DDR_PHY_DPI_CTRL_0_VALUE		0x3ef2010
#define DDR_PHY_WRLVL_CTRL_VALUE		0x20200ff0
#define DQS_EN_DLY					TM_DQS_EN_6_CYCLE_5

#define RX_FIFO_DLY					0x0c

#ifdef CONFIG_BGA234_DEMO_BOARD
#define RX_DQ0_P_DLY				0x08
#define RX_DQ0_N_DLY				0x08
#define RX_DQ8_P_DLY				0x08
#define RX_DQ8_N_DLY				0x09
#else
#define RX_DQ0_P_DLY				0x09
#define RX_DQ1_P_DLY				0x09
#define RX_DQ2_P_DLY				0x0a
#define RX_DQ3_P_DLY				0x09
#define RX_DQ4_P_DLY				0x09
#define RX_DQ5_P_DLY				0x09
#define RX_DQ6_P_DLY				0x09
#define RX_DQ7_P_DLY				0x09

#define RX_DQ8_P_DLY				0x0b
#define RX_DQ9_P_DLY				0x09
#define RX_DQ10_P_DLY				0x09
#define RX_DQ11_P_DLY				0x09
#define RX_DQ12_P_DLY				0x0c
#define RX_DQ13_P_DLY				0x0b
#define RX_DQ14_P_DLY				0x0b
#define RX_DQ15_P_DLY				0x0a

#define RX_DQ0_N_DLY				0x0a
#define RX_DQ1_N_DLY				0x0b
#define RX_DQ2_N_DLY				0x0b
#define RX_DQ3_N_DLY				0x0b
#define RX_DQ4_N_DLY				0x0c
#define RX_DQ5_N_DLY				0x0a
#define RX_DQ6_N_DLY				0x09
#define RX_DQ7_N_DLY				0x0b

#define RX_DQ8_N_DLY				0x0c
#define RX_DQ9_N_DLY				0x0a
#define RX_DQ10_N_DLY				0x0a
#define RX_DQ11_N_DLY				0x0a
#define RX_DQ12_N_DLY				0x0d
#define RX_DQ13_N_DLY				0x0c
#define RX_DQ14_N_DLY				0x0d
#define RX_DQ15_N_DLY				0x0c
#endif

#define DQ_DQS_MUX_TABLE_SEL		0x20230030

#define DQS0_READ_ODT_RANGE		0x1fc
#define DQS1_READ_ODT_RANGE		0x1fe

#endif

#ifdef CONFIG_DDR_W9751V6KG_400M
#define NO_ZQ_CAL

/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x1e

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0


#define DDR_PHY_PLL_CTL3_VALUE	0x25105134


#define CLK_PI					0x0
#define DQS_PI					0x0

#define DQ_PI					0x10

#define DDR_PHY_PLL_CTL0_VALUE_0 DPI_POST_PI_EN

#define DDR_PHY_PLL_CTL0_VALUE_1	(MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN|\
						MCK_DQ0_EN|MCK_DQ1_EN)
#define DDR_PHY_PLL_CTL1_VALUE	(MCK_CLK0_OUTPUT_EN|MCK_CMD_OUTPUT_EN|MCK_DQS0_OUTPUT_EN|\
						MCK_DQS1_OUTPUT_EN|MCK_DQ0_OUTPUT_EN|MCK_DQ1_OUTPUT_EN|\
						MCK_DQ0_SYNC_EN|MCK_DQ1_SYNC_EN)

#define DDR_PHY_BIST_2TO1_0_VALUE		DFI_RATIO_SETTING

#define ZPROG_VALUE0				0x10fe
#define ZPROG_VALUE1				0x017e

#define DDR_PHY_DPI_CTRL_0_VALUE		0x3ef2010
#define DDR_PHY_WRLVL_CTRL_VALUE		0x20200ff0
#define DQS_EN_DLY					TM_DQS_EN_6_CYCLE

#define RX_FIFO_DLY					0x0a

#define RX_DQ0_P_DLY				0x10
#define RX_DQ0_N_DLY				0x10
#define RX_DQ8_P_DLY				0x10
#define RX_DQ8_N_DLY				0x10

#define DQ_DQS_MUX_TABLE_SEL		0x20200330

#define DQS0_READ_ODT_RANGE		0x0000fe
#define DQS1_READ_ODT_RANGE		0x0000fe
#endif


#ifdef CONFIG_DDR_W9751V6KG_533M
#define NO_ZQ_CAL

/*Integer code of loop divider:[7:0]*/
#define DPI_N_CODE					0x28

/*Fractional code of loop divider:[12:0]*/
#define DPI_F_CODE				0x147a


#define DDR_PHY_PLL_CTL3_VALUE	0x26505254


#define CLK_PI					0x0
#define DQS_PI					0x0

#define DQ_PI					0x10

#define DDR_PHY_PLL_CTL0_VALUE_0 DPI_POST_PI_EN

#define DDR_PHY_PLL_CTL0_VALUE_1	(MCK_DQS1_EN|MCK_DQS0_EN|\
						MCK_CMD_EN|MCK_CLK0_EN|DPI_POST_PI_EN|\
						MCK_DQ0_EN|MCK_DQ1_EN)
#define DDR_PHY_PLL_CTL1_VALUE	(MCK_CLK0_OUTPUT_EN|MCK_CMD_OUTPUT_EN|MCK_DQS0_OUTPUT_EN|\
						MCK_DQS1_OUTPUT_EN|MCK_DQ0_OUTPUT_EN|MCK_DQ1_OUTPUT_EN|\
						MCK_DQ0_SYNC_EN|MCK_DQ1_SYNC_EN)

#define DDR_PHY_BIST_2TO1_0_VALUE		(DFI_RATIO_SETTING|RD_CMD_DLY|WR_CMD_DLY)

#define ZPROG_VALUE0				0x10fe
#define ZPROG_VALUE1				0x017e

#define DDR_PHY_DPI_CTRL_0_VALUE		0x3ef2010
#define DDR_PHY_WRLVL_CTRL_VALUE		0x20200ff0
#define DQS_EN_DLY					TM_DQS_EN_6_CYCLE

#define RX_FIFO_DLY					0x0a

#define RX_DQ0_P_DLY				0x0f
#define RX_DQ0_N_DLY				0x0d
#define RX_DQ8_P_DLY				0x0f
#define RX_DQ8_N_DLY				0x0c

#define DQ_DQS_MUX_TABLE_SEL		0x20200330

#define DQS0_READ_ODT_RANGE		0x0000fe
#define DQS1_READ_ODT_RANGE		0x0000fe
#endif


#define DDR_PHY_PLL_PI0_VALUE	(CLK_PI|(DQS_PI<<16)|(DQS_PI<<24))
#define DDR_PHY_PLL_PI1_VALUE	(DQ_PI<<16)
#define DDR_PHY_PLL_PI2_VALUE	DQ_PI

#define DDR_PHY_POST_PI_EN		(DPI_POST_PI_CL|DPI_POST_PI_EN)


#define DDR_PHY_READ_CTRL_5_VALUE	0x00c03fff




#define READ_CTRL_1_VALUE		(0x10000|RX_FIFO_DLY)

#ifdef CONFIG_BGA234_DEMO_BOARD
#define RX_DQ1_P_DLY				RX_DQ0_P_DLY
#define RX_DQ2_P_DLY				RX_DQ0_P_DLY
#define RX_DQ3_P_DLY				RX_DQ0_P_DLY
#define RX_DQ4_P_DLY				RX_DQ0_P_DLY
#define RX_DQ5_P_DLY				RX_DQ0_P_DLY
#define RX_DQ6_P_DLY				RX_DQ0_P_DLY
#define RX_DQ7_P_DLY				RX_DQ0_P_DLY

#define RX_DQ9_P_DLY				RX_DQ8_P_DLY
#define RX_DQ10_P_DLY				RX_DQ8_P_DLY
#define RX_DQ11_P_DLY				RX_DQ8_P_DLY
#define RX_DQ12_P_DLY				RX_DQ8_P_DLY
#define RX_DQ13_P_DLY				RX_DQ8_P_DLY
#define RX_DQ14_P_DLY				RX_DQ8_P_DLY
#define RX_DQ15_P_DLY				RX_DQ8_P_DLY

#define RX_DQ1_N_DLY				RX_DQ0_N_DLY
#define RX_DQ2_N_DLY				RX_DQ0_N_DLY
#define RX_DQ3_N_DLY				RX_DQ0_N_DLY
#define RX_DQ4_N_DLY				RX_DQ0_N_DLY
#define RX_DQ5_N_DLY				RX_DQ0_N_DLY
#define RX_DQ6_N_DLY				RX_DQ0_N_DLY
#define RX_DQ7_N_DLY				RX_DQ0_N_DLY

#define RX_DQ9_N_DLY				RX_DQ8_N_DLY
#define RX_DQ10_N_DLY				RX_DQ8_N_DLY
#define RX_DQ11_N_DLY				RX_DQ8_N_DLY
#define RX_DQ12_N_DLY				RX_DQ8_N_DLY
#define RX_DQ13_N_DLY				RX_DQ8_N_DLY
#define RX_DQ14_N_DLY				RX_DQ8_N_DLY
#define RX_DQ15_N_DLY				RX_DQ8_N_DLY
#endif

#define DDR_PHY_DQS_IN_DLY_0_0_VALUE	(RX_DQ0_P_DLY|(RX_DQ1_P_DLY<<8)|(RX_DQ2_P_DLY<<16)|(RX_DQ3_P_DLY<<24))
#define DDR_PHY_DQS_IN_DLY_1_0_VALUE (RX_DQ4_P_DLY|(RX_DQ5_P_DLY<<8)|(RX_DQ6_P_DLY<<16)|(RX_DQ7_P_DLY<<24))


#define DDR_PHY_DQS_IN_DLY_2_0_VALUE	(RX_DQ0_N_DLY|(RX_DQ1_N_DLY<<8)|(RX_DQ2_N_DLY<<16)|(RX_DQ3_N_DLY<<24))
#define DDR_PHY_DQS_IN_DLY_3_0_VALUE (RX_DQ7_N_DLY|(RX_DQ6_N_DLY<<8)|(RX_DQ5_N_DLY<<16)|(RX_DQ4_N_DLY<<24))


#define DDR_PHY_DQS_IN_DLY_0_1_VALUE	(RX_DQ8_P_DLY|(RX_DQ9_P_DLY<<8)|(RX_DQ10_P_DLY<<16)|(RX_DQ11_P_DLY<<24))
#define DDR_PHY_DQS_IN_DLY_1_1_VALUE (RX_DQ12_P_DLY|(RX_DQ13_P_DLY<<8)|(RX_DQ14_P_DLY<<16)|(RX_DQ15_P_DLY<<24))


#define DDR_PHY_DQS_IN_DLY_2_1_VALUE	(RX_DQ8_N_DLY|(RX_DQ9_N_DLY<<8)|(RX_DQ10_N_DLY<<16)|(RX_DQ11_N_DLY<<24))
#define DDR_PHY_DQS_IN_DLY_3_1_VALUE (RX_DQ12_N_DLY|(RX_DQ13_N_DLY<<8)|(RX_DQ14_N_DLY<<16)|(RX_DQ15_N_DLY<<24))

#endif
#endif
