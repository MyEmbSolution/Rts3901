/*
 * Register definitions for the Andes SPI Controller
 *
 * (C) Copyright 2011 Andes Technology
 * Macpaul Lin <macpaul@andestech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __RLX_SPI_H
#define __RLX_SPI_H

#define RTS_SPI_DRV_NAME		"spic-platform"


#define WR_QUAD_II_T				0x04
#define WR_QUAD_I_T				0x03
#define WR_DUAL_II_T				0x02
#define WR_DUAL_I_T				0x01
#define WR_MULTI_NONE_T			0x00

#define RD_QUAD_IO_T				0x10
#define RD_QUAD_O_T				0x08
#define RD_DUAL_IO_T				0x04
#define RD_DUAL_O_T				0x02
#define FRD_SINGLE_T				0x01




/* --------------------------------------------------------------------
**
** File     : SPI_public.h
** Created  : $Date: 2008/10/30 $
** Abstract : This is the public header file for the SPI
**            low-level-driver
**
** --------------------------------------------------------------------
*/

/****** drivers.SPI_FLASH/SPI_FLASH.api
 * NAME
 *  SPI API overview
 * DESCRIPTION
 *  This section gives an overview of the SPI software driver
 *  Application Programming Interface (API).
 * SEE ALSO
 *  SPI_FLASH.data, SPI_FLASH.functions
 ***/

/****** SPI_FLASH.api/SPI_FLASH.data_types
 * NAME
 *  DW_ahb_SPI data types and definitions
 * DESCRIPTION
 *  The data types below are used as function arguments for the DMA
 *  Controller API. Users of this driver must pass the relevant data
 *  types below to the API function that is being used.
 *
 *    - enum spi_flash_dr_number
 *
 * SEE ALSO
 *  SPI_FLASH.api, SPI_FLASH.configuration, SPI_FLASH.command, SPI_FLASH.status,
 ***/

/****** SPI_FLASH.api/SPI_FLASH.function
 * NAME
 *  SPI configuration functions
 * DESCRIPTION
 *  SPI_FLASH driver are listed
 *  below:
 *    - spi_flash_enable()
 *    - spi_flash_disable()
 *    - spi_flash_set_eeprom_mode()/ spi_flash_set_rx_mode/
 *    - spi_flash_set_tx_mode()
 *    - spi_flash_setser()
 *    - spi_flash_setctrlr1()
 *    - spi_flash_setdr()
 *    - spi_flash_setbaudr()
 *    - spi_flash_getbaudr()
 *    - spi_flash_getdr()
 *    - spi_flash_getidr()
 *    - spi_flash_set_multi_ch()
 *    - spi_flash_set_dummy_cycle()
 *    - spi_flash_isStatus_busy()
 *    - spi_flash_wait_busy()
 *
 *
 * SEE ALSO
 *  SPI_FLASH.api, SPI_FLASH.data_types, SPI_FLASH.command, SPI_FLASH.status,
 ***/

#define CMD_1CHN	0
#define CMD_4CHN	0x00200000
#define DATA_1CHN	0
#define DATA_2CHN	0x00040000
#define DATA_4CHN	0x00080000
#define ADDR_1CHN	0
#define ADDR_2CHN	0x00010000
#define ADDR_4CHN	0x00020000
#define QPI_CHN_SETTING	(CMD_4CHN|DATA_4CHN|ADDR_4CHN)
#define SPI_CHN_SETTING	(CMD_1CHN|DATA_1CHN|ADDR_1CHN)

enum spi_flash_byte_num {
    DATA_BYTE         = 0,
    DATA_HALF         = 1,
    DATA_WORD         = 2
};

enum spi_flash_dr_number {
    DR0               = 0 ,
    DR1               = 1 ,
    DR2               = 2 ,
    DR3               = 3 ,
    DR4               = 4 ,
    DR5               = 5 ,
    DR6               = 6 ,
    DR7               = 7 ,
    DR8               = 8 ,
    DR9               = 9 ,
    DR10              = 10,
    DR11              = 11,
    DR12              = 12,
    DR13              = 13,
    DR14              = 14,
    DR15              = 15,
    DR16              = 16,
    DR17              = 17,
    DR18              = 18,
    DR19              = 19,
    DR20              = 20,
    DR21              = 21,
    DR22              = 22,
    DR23              = 23,
    DR24              = 24,
    DR25              = 25,
    DR26              = 26,
    DR27              = 27,
    DR28              = 28,
    DR29              = 29,
    DR30              = 30,
    DR31              = 31
};


#define DW_EPERM            1   /* operation not permitted*/
#define DW_EIO              5   /* I/O error*/
#define DW_ENXIO            6   /* no such device or address*/
#define DW_ENOMEM           12  /* out of memory*/
#define DW_EACCES           13  /* permission denied*/
#define DW_EBUSY            16  /* device or resource busy*/
#define DW_ENODEV           19  /* no such device*/
#define DW_EINVAL           22  /* invalid argument*/
#define DW_ENOSPC           28  /*no space left on device*/
#define DW_ENOSYS           38  /* function not implemented/supported*/
#define DW_ECHRNG           44  /* channel number out of range*/
#define DW_ENODATA          61  /* no data available*/
#define DW_ETIME            62  /* timer expired*/
#define DW_EPROTO           71  /* protocol error*/

#define RLX_SPI_BASEADDR 0xB8030000
#define RLX_SPI_FIFO_DEPTH	128

#define bfoSPI_FLASH_CTRLR0_FAST_RD         ((u32)   20)
#define bfwSPI_FLASH_CTRLR0_FAST_RD         ((u32)    1)
#define bfoSPI_FLASH_CTRLR0_DATA_CH         ((u32)   18)
#define bfwSPI_FLASH_CTRLR0_DATA_CH         ((u32)    2)
#define bfoSPI_FLASH_CTRLR0_ADDR_CH         ((u32)   16)
#define bfwSPI_FLASH_CTRLR0_ADDR_CH         ((u32)    2)
#define bfoSPI_FLASH_CTRLR0_CFS             ((u32)   12)
#define bfwSPI_FLASH_CTRLR0_CFS             ((u32)    3)
#define bfoSPI_FLASH_CTRLR0_SRL             ((u32)   11)
#define bfwSPI_FLASH_CTRLR0_SRL             ((u32)    1)
#define bfoSPI_FLASH_CTRLR0_SLV_OE          ((u32)   10)
#define bfwSPI_FLASH_CTRLR0_SLV_OE          ((u32)    1)
#define bfoSPI_FLASH_CTRLR0_TMOD            ((u32)    8)
#define bfwSPI_FLASH_CTRLR0_TMOD            ((u32)    2)
#define bfoSPI_FLASH_CTRLR0_SCPOL           ((u32)    7)
#define bfwSPI_FLASH_CTRLR0_SCPOL           ((u32)    1)
#define bfoSPI_FLASH_CTRLR0_FRF             ((u32)    4)
#define bfwSPI_FLASH_CTRLR0_FRF             ((u32)    2)
#define bfoSPI_FLASH_CTRLR0_DFS             ((u32)    0)
#define bfwSPI_FLASH_CTRLR0_DFS             ((u32)    4)

#define bfoSPI_FLASH_CTRLR1_NDF             ((u32)    0)
#define bfwSPI_FLASH_CTRLR1_NDF             ((u32)   16)

#define bfoSPI_FLASH_SSIENR_SSI_EN          ((u32)    0)
#define bfwSPI_FLASH_SSIENR_SSI_EN          ((u32)    1)

#define bfoSPI_FLASH_MWCR_MHS               ((u32)    2)
#define bfwSPI_FLASH_MWCR_MHS               ((u32)    1)
#define bfoSPI_FLASH_MWCR_MDD               ((u32)    1)
#define bfwSPI_FLASH_MWCR_MDD               ((u32)    1)
#define bfoSPI_FLASH_MWCR_MWMOD             ((u32)    0)
#define bfwSPI_FLASH_MWCR_MWMOD             ((u32)    1)

#define bfoSPI_FLASH_SER                    ((u32)    0)
#define bfwSPI_FLASH_SER                    ((u32)    4)

#define bfoSPI_FLASH_BAUDR_SCKDV            ((u32)    0)
#define bfwSPI_FLASH_BAUDR_SCKDV            ((u32)   16)

#define bfoSPI_FLASH_TXFTLR_TFT             ((u32)    0)
#define bfwSPI_FLASH_TXFTLR_TFT             ((u32)    3)

#define bfoSPI_FLASH_RXFTLR_RFT             ((u32)    0)
#define bfwSPI_FLASH_RXFTLR_RFT             ((u32)    3)

#define bfoSPI_FLASH_TXFLR_TXTFL            ((u32)    0)
#define bfwSPI_FLASH_TXFLR_TXTFL            ((u32)    3)

#define bfoSPI_FLASH_RXFLR_RXTFL            ((u32)    0)
#define bfwSPI_FLASH_RXFLR_RXTFL            ((u32)    3)

#define bfoSPI_FLASH_SR_BUSY                ((u32)    0)
#define bfwSPI_FLASH_SR_BUSY                ((u32)    1)
#define bfoSPI_FLASH_SR_TFNF                ((u32)    1)
#define bfwSPI_FLASH_SR_TFNF                ((u32)    1)
#define bfoSPI_FLASH_SR_TFE                 ((u32)    2)
#define bfwSPI_FLASH_SR_TFE                 ((u32)    1)
#define bfoSPI_FLASH_SR_RFNE                ((u32)    3)
#define bfwSPI_FLASH_SR_RFNE                ((u32)    1)
#define bfoSPI_FLASH_SR_RFF                 ((u32)    4)
#define bfwSPI_FLASH_SR_RFF                 ((u32)    1)
#define bfoSPI_FLASH_SR_TXE                 ((u32)    5)
#define bfwSPI_FLASH_SR_TXE                 ((u32)    1)
#define bfoSPI_FLASH_SR_DCOL                ((u32)    6)
#define bfwSPI_FLASH_SR_DCOL                ((u32)    1)

#define bfoSPI_FLASH_IMR_TXEIM              ((u32)     0)
#define bfwSPI_FLASH_IMR_TXEIM              ((u32)     1)
#define bfoSPI_FLASH_IMR_TXOIM              ((u32)     1)
#define bfwSPI_FLASH_IMR_TXOIM              ((u32)     1)
#define bfoSPI_FLASH_IMR_RXUIM              ((u32)     2)
#define bfwSPI_FLASH_IMR_RXUIM              ((u32)     1)
#define bfoSPI_FLASH_IMR_RXOIM              ((u32)     3)
#define bfwSPI_FLASH_IMR_RXOIM              ((u32)     1)
#define bfoSPI_FLASH_IMR_RXFIM              ((u32)     4)
#define bfwSPI_FLASH_IMR_RXFIM              ((u32)     1)
#define bfoSPI_FLASH_IMR_FSEIM              ((u32)     5)
#define bfwSPI_FLASH_IMR_FSEIM              ((u32)     1)
#define bfoSPI_FLASH_IMR_WBEIM              ((u32)     6)
#define bfwSPI_FLASH_IMR_WBEIM              ((u32)     1)
#define bfoSPI_FLASH_IMR_BYEIM              ((u32)     7)
#define bfwSPI_FLASH_IMR_BYEIM              ((u32)     1)
#define bfoSPI_FLASH_IMR_ACTIM              ((u32)     8)
#define bfwSPI_FLASH_IMR_ACTIM              ((u32)     1)
#define bfoSPI_FLASH_IMR_TXEIM_PEND         ((u32)     9)
#define bfwSPI_FLASH_IMR_TXEIM_PEND         ((u32)     1)

#define bfoSPI_FLASH_ISR_TXEIS              ((u32)     0)
#define bfwSPI_FLASH_ISR_TXEIS              ((u32)     1)
#define bfoSPI_FLASH_ISR_TXOIS              ((u32)     1)
#define bfwSPI_FLASH_ISR_TXOIS              ((u32)     1)
#define bfoSPI_FLASH_ISR_RXUIS              ((u32)     2)
#define bfwSPI_FLASH_ISR_RXUIS              ((u32)     1)
#define bfoSPI_FLASH_ISR_RXOIS              ((u32)     3)
#define bfwSPI_FLASH_ISR_RXOIS              ((u32)     1)
#define bfoSPI_FLASH_ISR_RXFIS              ((u32)     4)
#define bfwSPI_FLASH_ISR_RXFIS              ((u32)     1)
#define bfoSPI_FLASH_ISR_FSEIS              ((u32)     5)
#define bfwSPI_FLASH_ISR_FSEIS              ((u32)     1)
#define bfoSPI_FLASH_ISR_WBEIS              ((u32)     6)
#define bfwSPI_FLASH_ISR_WBEIS              ((u32)     1)
#define bfoSPI_FLASH_ISR_BYEIS              ((u32)     7)
#define bfwSPI_FLASH_ISR_BYEIS              ((u32)     1)
#define bfoSPI_FLASH_ISR_ACTIS              ((u32)     8)
#define bfwSPI_FLASH_ISR_ACTIS              ((u32)     1)
#define bfoSPI_FLASH_ISR_TXEIS_PEND         ((u32)     9)
#define bfwSPI_FLASH_ISR_TXEIS_PEND         ((u32)     1)

#define bfoSPI_FLASH_RISR_TXEIR             ((u32)     0)
#define bfwSPI_FLASH_RISR_TXEIR             ((u32)     1)
#define bfoSPI_FLASH_RISR_TXOIR             ((u32)     1)
#define bfwSPI_FLASH_RISR_TXOIR             ((u32)     1)
#define bfoSPI_FLASH_RISR_RXUIR             ((u32)     2)
#define bfwSPI_FLASH_RISR_RXUIR             ((u32)     1)
#define bfoSPI_FLASH_RISR_RXOIR             ((u32)     3)
#define bfwSPI_FLASH_RISR_RXOIR             ((u32)     1)
#define bfoSPI_FLASH_RISR_RXFIR             ((u32)     4)
#define bfwSPI_FLASH_RISR_RXFIR             ((u32)     1)
#define bfoSPI_FLASH_RISR_FSEIR             ((u32)     5)
#define bfwSPI_FLASH_RISR_FSEIR             ((u32)     1)
#define bfoSPI_FLASH_RISR_WBEIR             ((u32)     6)
#define bfwSPI_FLASH_RISR_WBEIR             ((u32)     1)
#define bfoSPI_FLASH_RISR_BYEIR             ((u32)     7)
#define bfwSPI_FLASH_RISR_BYEIR             ((u32)     1)
#define bfoSPI_FLASH_RISR_ACTIR             ((u32)     8)
#define bfwSPI_FLASH_RISR_ACTIR             ((u32)     1)

#define bfoSPI_FLASH_TXOICR_TXOICR          ((u32)     0)
#define bfwSPI_FLASH_TXOICR_TXOICR          ((u32)     1)

#define bfoSPI_FLASH_RXOICR_RXOICR          ((u32)     0)
#define bfwSPI_FLASH_RXOICR_RXOICR          ((u32)     1)

#define bfoSPI_FLASH_RXUICR_RXUICR          ((u32)     0)
#define bfwSPI_FLASH_RXUICR_RXUICR          ((u32)     1)

#define bfoSPI_FLASH_MSTICR_MSTICR          ((u32)     0)
#define bfwSPI_FLASH_MSTICR_MSTICR          ((u32)     1)

#define bfoSPI_FLASH_ICR_ICR                ((u32)     0)
#define bfwSPI_FLASH_ICR_ICR                ((u32)     1)

#define bfoSPI_FLASH_DMACR_RDMAE            ((u32)     0)
#define bfwSPI_FLASH_DMACR_RDMAE            ((u32)     1)
#define bfoSPI_FLASH_DMACR_TDMAE            ((u32)     1)
#define bfwSPI_FLASH_DMACR_TDMAE            ((u32)     1)

#define bfoSPI_FLASH_DMATDLR_DMATDL         ((u32)     0)
#define bfwSPI_FLASH_DMATDLR_DMATDL         ((u32)     3)

#define bfoSPI_FLASH_DMARDLR_DMARDL         ((u32)     0)
#define bfwSPI_FLASH_DMARDLR_DMARDL         ((u32)     3)

#define bfoSPI_FLASH_DR0_dr0                ((u32)     0)
#define bfwSPI_FLASH_DR0_dr0                ((u32)    16)
#define bfoSPI_FLASH_DR1_dr1                ((u32)     0)
#define bfwSPI_FLASH_DR1_dr1                ((u32)    16)
#define bfoSPI_FLASH_DR2_dr2                ((u32)     0)
#define bfwSPI_FLASH_DR2_dr2                ((u32)    16)
#define bfoSPI_FLASH_DR3_dr3                ((u32)     0)
#define bfwSPI_FLASH_DR3_dr3                ((u32)    16)
#define bfoSPI_FLASH_DR4_dr4                ((u32)     0)
#define bfwSPI_FLASH_DR4_dr4                ((u32)    16)
#define bfoSPI_FLASH_DR5_dr5                ((u32)     0)
#define bfwSPI_FLASH_DR5_dr5                ((u32)    16)
#define bfoSPI_FLASH_DR6_dr6                ((u32)     0)
#define bfwSPI_FLASH_DR6_dr6                ((u32)    16)
#define bfoSPI_FLASH_DR7_dr7                ((u32)     0)
#define bfwSPI_FLASH_DR7_dr7                ((u32)    16)
#define bfoSPI_FLASH_DR8_dr8                ((u32)     0)
#define bfwSPI_FLASH_DR8_dr8                ((u32)    16)
#define bfoSPI_FLASH_DR9_dr9                ((u32)     0)
#define bfwSPI_FLASH_DR9_dr9                ((u32)    16)
#define bfoSPI_FLASH_DR10_dr10              ((u32)     0)
#define bfwSPI_FLASH_DR10_dr10              ((u32)    16)
#define bfoSPI_FLASH_DR11_dr11              ((u32)     0)
#define bfwSPI_FLASH_DR11_dr11              ((u32)    16)
#define bfoSPI_FLASH_DR12_dr12              ((u32)     0)
#define bfwSPI_FLASH_DR12_dr12              ((u32)    16)
#define bfoSPI_FLASH_DR13_dr13              ((u32)     0)
#define bfwSPI_FLASH_DR13_dr13              ((u32)    16)
#define bfoSPI_FLASH_DR14_dr14              ((u32)     0)
#define bfwSPI_FLASH_DR14_dr14              ((u32)    16)
#define bfoSPI_FLASH_DR15_dr15              ((u32)     0)
#define bfwSPI_FLASH_DR15_dr15              ((u32)    16)

#define bfoSPI_FLASH_AUTO_LEN_ADDR          ((u32)    16)
#define bfwSPI_FLASH_AUTO_LEN_ADDR          ((u32)     2)
#define bfoSPI_FLASH_AUTO_LEN_DUM           ((u32)     0)
#define bfwSPI_FLASH_AUTO_LEN_DUM           ((u32)    16)

#define DW_BITS_MASK(__bfws, __bits) ((u32) ((__bfws) == 32) ?  \
	0x0 : (((0xffffffff)>>(32 - __bits)) << (__bfws)))

#define DW_BIT_MASK(__bfws) ((u32) ((__bfws) == 32) ?  \
	0x0 : (0x1 << (__bfws)))

#define DW_BIT_MASK_WIDTH(__bfws, __bits) ((u32) ((__bfws) == 32) ?  \
	0xFFFFFFFF : (((1 << (__bits)) - 1) << (__bfws)))

#define DW_BIT_GET_UNSHIFTED(__datum, __bfws)                       \
	((u32) ((__datum) & DW_BIT_MASK(__bfws)))

#define DW_BITS_SET_VAL(__datum, __bfws, __val, bit_num)                          \
	((__datum) = ((u32) (__datum) & ~DW_BIT_MASK_WIDTH(__bfws, bit_num)) |    \
	((__val << (__bfws)) & DW_BIT_MASK_WIDTH(__bfws, bit_num)))


#define WR_QUAD_II				0x04
#define WR_QUAD_I				0x03
#define WR_DUAL_II				0x02
#define WR_DUAL_I				0x01
#define WR_MULTI_NONE			0x00


#define RD_QUAD_IO				0x10
#define RD_QUAD_O				0x08
#define RD_DUAL_IO				0x04
#define RD_DUAL_O				0x02
#define FRD_SINGLE				0x01

#define QPI_I				(1 << 0)
#define QPI_II				(1 << 1)
#define SR_CFG				(1 << 2)
#define SR_3REG				(1 << 3)
#define SR_3REG1			(1 << 4)

#define BLOCK_ERASE_CMD	0xD8	/*usually 64KB*/

struct restore_para {
	u8 cmd_len;
	u8 data_len;
	u8 *cmd;
	u8 *data_in;
	u8 *data_out;
	u8 res;
};

/* rlx spi slave */
struct rlx_spi {
	struct spi_flash_portmap *base_address;
	u8 mode;
	u8 fifo_depth;
	u32 speed_hz;
	u32 input_hz;
	u32 req_hz;
	u16 flags;
	u8 fast_read_cmd;
	u8 autoread_type;
	u8 dummy_cycle;
	u8 fast_write_cmd;
	u8 userwrite_type;
	struct spi_message *msg;
	struct restore_para para;
	struct work_struct readspi;
	struct work_struct writespi;
	int polling;
};

/*rlx spi master */
struct rlx_spi_master {
	struct spi_master *master;
	struct platform_device          *pdev;
	int	irq;
	u32 IO_BaseAddr;
	struct rlx_spi *spic;
	int (*setautomode) (struct rlx_spi *rlx_ctrl);
	int (*setaddrlen) (struct rlx_spi *rlx_ctrl, u8 len);
	struct list_head	queue;
	struct tasklet_struct	tasklet;
	spinlock_t lock;
};

#endif /* __ANDES_SPI_H */

