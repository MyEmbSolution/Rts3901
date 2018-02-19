#ifndef MS_RXI310_DEFS_H
#define MS_RXI310_DEFS_H

#define _DDR_64MB_
#ifdef _DDR_64MB_
#define MEM_SIZE	0x4000000
#endif

#ifdef _DDR_256MB_
#define MEM_SIZE	0x10000000
#endif

#define _BIST_MEM_SIZE_           0x400

#define MS_PCTL_CCR_OFFSET        0x000
#define MS_PCTL_DCR_OFFSET        0x004
#define MS_PCTL_IOCR_OFFSET       0x008
#define MS_PCTL_CSR_OFFSET        0x00c
#define MS_PCTL_DRR_OFFSET        0x010
#define MS_PCTL_TPR0_OFFSET       0x014
#define MS_PCTL_TPR1_OFFSET       0x018
#define MS_PCTL_TPR2_OFFSET       0x01c
#define MS_PCTL_MR_OFFSET         0x020
#define MS_PCTL_EMR1_OFFSET       0x024
#define MS_PCTL_EMR2_OFFSET       0x028
#define MS_PCTL_EMR3_OFFSET       0x02c
#define MS_PCTL_CSR2_OFFSET       0x030
#define MS_PCTL_SRST_OFFSET       0x034
#define MS_PCTL_DTR2_OFFSET       0x038
#define MS_PCTL_DTR3_OFFSET       0x03c
#define MS_PCTL_GDLLCR_OFFSET     0x040
#define MS_PCTL_DLLCR0_OFFSET     0x044
#define MS_PCTL_DLLCR1_OFFSET     0x048
#define MS_PCTL_DLLCR2_OFFSET     0x04c
#define MS_PCTL_DLLCR3_OFFSET     0x050
#define MS_PCTL_DLLCR4_OFFSET     0x054
#define MS_PCTL_DLLCR5_OFFSET     0x058
#define MS_PCTL_DLLCR6_OFFSET     0x05c
#define MS_PCTL_DLLCR7_OFFSET     0x060
#define MS_PCTL_DLLCR8_OFFSET     0x064
#define MS_PCTL_DQTR0_OFFSET      0x068
#define MS_PCTL_DQTR1_OFFSET      0x06c
#define MS_PCTL_DQTR2_OFFSET      0x070
#define MS_PCTL_DQTR3_OFFSET      0x074
#define MS_PCTL_DQTR4_OFFSET      0x078
#define MS_PCTL_DQTR5_OFFSET      0x07c
#define MS_PCTL_DQTR6_OFFSET      0x080
#define MS_PCTL_DQTR7_OFFSET      0x084
#define MS_PCTL_DQSTR_OFFSET      0x088
#define MS_PCTL_DQSBTR_OFFSET     0x08c
#define MS_PCTL_ODTCR_OFFSET      0x090
#define MS_PCTL_DTR0_OFFSET       0x094
#define MS_PCTL_DTR1_OFFSET       0x098
#define MS_PCTL_DTAR_OFFSET       0x09c
#define MS_PCTL_ZQCR0_OFFSET      0x0a0
#define MS_PCTL_ZQCR1_OFFSET      0x0a4
#define MS_PCTL_ZQSR_OFFSET       0x0a8
#define MS_PCTL_RSLR0_OFFSET      0x0ac
#define MS_PCTL_RSLR1_OFFSET      0x0b0
#define MS_PCTL_RSLR2_OFFSET      0x0b4
#define MS_PCTL_RSLR3_OFFSET      0x0b8
#define MS_PCTL_RDGR0_OFFSET      0x0bc
#define MS_PCTL_RDGR1_OFFSET      0x0c0
#define MS_PCTL_RDGR2_OFFSET      0x0c4
#define MS_PCTL_RDGR3_OFFSET      0x0c8
#define MS_PCTL_MXSL_OFFSET       0x0cc

#define MS_PCTL_BCR_OFFSET        0x0d0
#define MS_PCTL_BALR0_OFFSET      0x0d4
#define MS_PCTL_BALR1_OFFSET      0x0d8
#define MS_PCTL_BDR0_OFFSET       0x0dc
#define MS_PCTL_BDR1_OFFSET       0x0e0
#define MS_PCTL_BBR_OFFSET        0x0e4
#define MS_PCTL_BSR_OFFSET        0x0e8
#define MS_PCTL_BYR_OFFSET        0x0ec
#define MS_PCTL_BFA_OFFSET        0x0f0
#define MS_PCTL_IDR_OFFSET        0x0f8
#define MS_PCTL_ERR_OFFSET        0x0fc

#define MS_WRAP_SCR_OFFSET        0x224
#define MS_WRAP_QCR_OFFSET        0x230
#define MS_WRAP_PCR_OFFSET        0x234
#define MS_WRAP_QTR0_OFFSET       0x240
#define MS_WRAP_QTR1_OFFSET       0x244
#define MS_WRAP_QTR2_OFFSET       0x248
#define MS_WRAP_QTR3_OFFSET       0x24c
#define MS_WRAP_QTR4_OFFSET       0x250
#define MS_WRAP_QTR5_OFFSET       0x254
#define MS_WRAP_QTR6_OFFSET       0x258
#define MS_WRAP_QTR7_OFFSET       0x25c
#define MS_WRAP_QTR8_OFFSET       0x260
#define MS_WRAP_QTR9_OFFSET       0x264
#define MS_WRAP_QTR10_OFFSET      0x268
#define MS_WRAP_QTR11_OFFSET      0x26c
#define MS_WRAP_QTR12_OFFSET      0x270
#define MS_WRAP_QTR13_OFFSET      0x274
#define MS_WRAP_QTR14_OFFSET      0x278
#define MS_WRAP_QTR15_OFFSET      0x27c

#define MS_PHY_DLY0               0x100
#define MS_PHY_DLY1_RST           0x104
#define MS_PHY_DLY_CLK            0x108
#define MS_PHY_DLY_ST             0x10c
#define MS_PHY_DLY_NUM            0x100


#endif /* MS_RXI310_DEFS_H*/
