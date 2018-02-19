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

#define ENABLE_QPI_MODE			0x35
#define EXIT_QPI_MODE				0xf5

#define ENABLE_4BYTE_MODE			0xb7
#define EXIT_4BYTE_MODE			0xe9

/***** spi_flash.api/portmap
 * DESCRIPTION
 *  This is the structure used for accessing the spi_flash register
 *  portmap.
 * EXAMPLE
 *  struct spi_flash_portmap *portmap;
 *  portmap = (struct spi_flash_portmap *) spi_flash_BASE;
 *  foo = INP(portmap->ctrlr0 );
 * SOURCE
 */
struct spi_flash_portmap {
/*typedef uint32_t unsigned int;*/

/* Channel registers                                    */
/* The offset address for each of the channel registers */
/*  is shown for channel 0. For other channel numbers   */
/*  use the following equation.                         */
/*                                                      */
/*    offset = (channel_num * 0x058) + channel_0 offset */
/*                                                      */
	struct {
		volatile uint32_t ctrlr0;    /* Control Reg 0           (0x000) */
		volatile uint32_t ctrlr1;
		volatile uint32_t ssienr;    /* SPIC enable Reg1        (0x008) */
		volatile uint32_t mwcr;
		volatile uint32_t ser;       /* Slave enable Reg        (0x010) */
		volatile uint32_t baudr;
		volatile uint32_t txftlr;    /* TX_FIFO threshold level (0x018) */
		volatile uint32_t rxftlr;
		volatile uint32_t txflr;     /* TX_FIFO threshold level (0x020) */
		volatile uint32_t rxflr;
		volatile uint32_t sr;        /* Destination Status Reg  (0x028) */
		volatile uint32_t imr;
		volatile uint32_t isr;       /* Interrupt Stauts Reg    (0x030) */
		volatile uint32_t risr;
		volatile uint32_t txoicr;    /* TX_FIFO overflow_INT clear (0x038) */
		volatile uint32_t rxoicr;
		volatile uint32_t rxuicr;    /* RX_FIFO underflow_INT clear (0x040) */
		volatile uint32_t msticr;
		volatile uint32_t icr;       /* Interrupt clear Reg     (0x048) */
		volatile uint32_t dmacr;
		volatile uint32_t dmatdlr;   /* DMA TX_data level       (0x050) */
		volatile uint32_t dmardlr;
		volatile uint32_t idr;       /* Identiation Scatter Reg (0x058) */
		volatile uint32_t spi_flash_version;
		/*volatile uint32_t dr[32]; */   /* Data Reg          (0x060~0xdc)*/
		union{
			volatile uint8_t  byte;
			volatile uint16_t half;
			volatile uint32_t word;
		} dr[32];
		volatile uint32_t rd_fast_single;
		volatile uint32_t rd_dual_o; /* Read dual data cmd Reg  (0x0e4) */
		volatile uint32_t rd_dual_io;
		volatile uint32_t rd_quad_o; /* Read quad data cnd Reg  (0x0ec) */
		volatile uint32_t rd_quad_io;
		volatile uint32_t wr_single; /* write single cmd Reg    (0x0f4) */
		volatile uint32_t wr_dual_i;
		volatile uint32_t wr_dual_ii;/* write dual addr/data cmd(0x0fc) */
		volatile uint32_t wr_quad_i;
		volatile uint32_t wr_quad_ii;/* write quad addr/data cnd(0x104) */
		volatile uint32_t wr_enable;
		volatile uint32_t rd_status; /* read status cmd Reg     (0x10c) */
		volatile uint32_t ctrlr2;
		volatile uint32_t fbaudr;    /* fast baud rate Reg      (0x114) */
		volatile uint32_t addr_length;
		volatile uint32_t auto_length; /* Auto addr length Reg  (0x11c) */
		volatile uint32_t valid_cmd;
		volatile uint32_t flash_size; /* Flash size Reg         (0x124) */
		volatile uint32_t flush_fifo;
	};
};




/* rlx spi slave */
struct rlx_spi_slave {
	struct spi_slave slave;
	struct spi_flash_portmap *base_address;
	u8 mode;
	u8 fifo_depth;
	u32 speed_hz;
	u32 input_hz;
	u32 req_hz;
};



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

#define RLX_SPI_BASEADDR 0xB8030000
#define RLX_SPI_FIFO_DEPTH	128

#define bfoSPI_FLASH_CTRLR0_FAST_RD         ((uint32_t)   20)
#define bfwSPI_FLASH_CTRLR0_FAST_RD         ((uint32_t)    1)
#define bfoSPI_FLASH_CTRLR0_DATA_CH         ((uint32_t)   18)
#define bfwSPI_FLASH_CTRLR0_DATA_CH         ((uint32_t)    2)
#define bfoSPI_FLASH_CTRLR0_ADDR_CH         ((uint32_t)   16)
#define bfwSPI_FLASH_CTRLR0_ADDR_CH         ((uint32_t)    2)
#define bfoSPI_FLASH_CTRLR0_CFS             ((uint32_t)   12)
#define bfwSPI_FLASH_CTRLR0_CFS             ((uint32_t)    3)
#define bfoSPI_FLASH_CTRLR0_SRL             ((uint32_t)   11)
#define bfwSPI_FLASH_CTRLR0_SRL             ((uint32_t)    1)
#define bfoSPI_FLASH_CTRLR0_SLV_OE          ((uint32_t)   10)
#define bfwSPI_FLASH_CTRLR0_SLV_OE          ((uint32_t)    1)
#define bfoSPI_FLASH_CTRLR0_TMOD            ((uint32_t)    8)
#define bfwSPI_FLASH_CTRLR0_TMOD            ((uint32_t)    2)
#define bfoSPI_FLASH_CTRLR0_SCPOL           ((uint32_t)    7)
#define bfwSPI_FLASH_CTRLR0_SCPOL           ((uint32_t)    1)
#define bfoSPI_FLASH_CTRLR0_FRF             ((uint32_t)    4)
#define bfwSPI_FLASH_CTRLR0_FRF             ((uint32_t)    2)
#define bfoSPI_FLASH_CTRLR0_DFS             ((uint32_t)    0)
#define bfwSPI_FLASH_CTRLR0_DFS             ((uint32_t)    4)

#define bfoSPI_FLASH_CTRLR1_NDF             ((uint32_t)    0)
#define bfwSPI_FLASH_CTRLR1_NDF             ((uint32_t)   16)

#define bfoSPI_FLASH_SSIENR_SSI_EN          ((uint32_t)    0)
#define bfwSPI_FLASH_SSIENR_SSI_EN          ((uint32_t)    1)

#define bfoSPI_FLASH_MWCR_MHS               ((uint32_t)    2)
#define bfwSPI_FLASH_MWCR_MHS               ((uint32_t)    1)
#define bfoSPI_FLASH_MWCR_MDD               ((uint32_t)    1)
#define bfwSPI_FLASH_MWCR_MDD               ((uint32_t)    1)
#define bfoSPI_FLASH_MWCR_MWMOD             ((uint32_t)    0)
#define bfwSPI_FLASH_MWCR_MWMOD             ((uint32_t)    1)

#define bfoSPI_FLASH_SER                    ((uint32_t)    0)
#define bfwSPI_FLASH_SER                    ((uint32_t)    4)

#define bfoSPI_FLASH_BAUDR_SCKDV            ((uint32_t)    0)
#define bfwSPI_FLASH_BAUDR_SCKDV            ((uint32_t)   16)

#define bfoSPI_FLASH_TXFTLR_TFT             ((uint32_t)    0)
#define bfwSPI_FLASH_TXFTLR_TFT             ((uint32_t)    3)

#define bfoSPI_FLASH_RXFTLR_RFT             ((uint32_t)    0)
#define bfwSPI_FLASH_RXFTLR_RFT             ((uint32_t)    3)

#define bfoSPI_FLASH_TXFLR_TXTFL            ((uint32_t)    0)
#define bfwSPI_FLASH_TXFLR_TXTFL            ((uint32_t)    3)

#define bfoSPI_FLASH_RXFLR_RXTFL            ((uint32_t)    0)
#define bfwSPI_FLASH_RXFLR_RXTFL            ((uint32_t)    3)

#define bfoSPI_FLASH_SR_BUSY                ((uint32_t)    0)
#define bfwSPI_FLASH_SR_BUSY                ((uint32_t)    1)
#define bfoSPI_FLASH_SR_TFNF                ((uint32_t)    1)
#define bfwSPI_FLASH_SR_TFNF                ((uint32_t)    1)
#define bfoSPI_FLASH_SR_TFE                 ((uint32_t)    2)
#define bfwSPI_FLASH_SR_TFE                 ((uint32_t)    1)
#define bfoSPI_FLASH_SR_RFNE                ((uint32_t)    3)
#define bfwSPI_FLASH_SR_RFNE                ((uint32_t)    1)
#define bfoSPI_FLASH_SR_RFF                 ((uint32_t)    4)
#define bfwSPI_FLASH_SR_RFF                 ((uint32_t)    1)
#define bfoSPI_FLASH_SR_TXE                 ((uint32_t)    5)
#define bfwSPI_FLASH_SR_TXE                 ((uint32_t)    1)
#define bfoSPI_FLASH_SR_DCOL                ((uint32_t)    6)
#define bfwSPI_FLASH_SR_DCOL                ((uint32_t)    1)

#define bfoSPI_FLASH_IMR_TXEIM              ((uint32_t)     0)
#define bfwSPI_FLASH_IMR_TXEIM              ((uint32_t)     1)
#define bfoSPI_FLASH_IMR_TXOIM              ((uint32_t)     1)
#define bfwSPI_FLASH_IMR_TXOIM              ((uint32_t)     1)
#define bfoSPI_FLASH_IMR_RXUIM              ((uint32_t)     2)
#define bfwSPI_FLASH_IMR_RXUIM              ((uint32_t)     1)
#define bfoSPI_FLASH_IMR_RXOIM              ((uint32_t)     3)
#define bfwSPI_FLASH_IMR_RXOIM              ((uint32_t)     1)
#define bfoSPI_FLASH_IMR_RXFIM              ((uint32_t)     4)
#define bfwSPI_FLASH_IMR_RXFIM              ((uint32_t)     1)
#define bfoSPI_FLASH_IMR_FSEIM              ((uint32_t)     5)
#define bfwSPI_FLASH_IMR_FSEIM              ((uint32_t)     1)
#define bfoSPI_FLASH_IMR_WBEIM              ((uint32_t)     6)
#define bfwSPI_FLASH_IMR_WBEIM              ((uint32_t)     1)
#define bfoSPI_FLASH_IMR_BYEIM              ((uint32_t)     7)
#define bfwSPI_FLASH_IMR_BYEIM              ((uint32_t)     1)
#define bfoSPI_FLASH_IMR_ACTIM              ((uint32_t)     8)
#define bfwSPI_FLASH_IMR_ACTIM              ((uint32_t)     1)
#define bfoSPI_FLASH_IMR_TXEIM_PEND         ((uint32_t)     9)
#define bfwSPI_FLASH_IMR_TXEIM_PEND         ((uint32_t)     1)

#define bfoSPI_FLASH_ISR_TXEIS              ((uint32_t)     0)
#define bfwSPI_FLASH_ISR_TXEIS              ((uint32_t)     1)
#define bfoSPI_FLASH_ISR_TXOIS              ((uint32_t)     1)
#define bfwSPI_FLASH_ISR_TXOIS              ((uint32_t)     1)
#define bfoSPI_FLASH_ISR_RXUIS              ((uint32_t)     2)
#define bfwSPI_FLASH_ISR_RXUIS              ((uint32_t)     1)
#define bfoSPI_FLASH_ISR_RXOIS              ((uint32_t)     3)
#define bfwSPI_FLASH_ISR_RXOIS              ((uint32_t)     1)
#define bfoSPI_FLASH_ISR_RXFIS              ((uint32_t)     4)
#define bfwSPI_FLASH_ISR_RXFIS              ((uint32_t)     1)
#define bfoSPI_FLASH_ISR_FSEIS              ((uint32_t)     5)
#define bfwSPI_FLASH_ISR_FSEIS              ((uint32_t)     1)
#define bfoSPI_FLASH_ISR_WBEIS              ((uint32_t)     6)
#define bfwSPI_FLASH_ISR_WBEIS              ((uint32_t)     1)
#define bfoSPI_FLASH_ISR_BYEIS              ((uint32_t)     7)
#define bfwSPI_FLASH_ISR_BYEIS              ((uint32_t)     1)
#define bfoSPI_FLASH_ISR_ACTIS              ((uint32_t)     8)
#define bfwSPI_FLASH_ISR_ACTIS              ((uint32_t)     1)
#define bfoSPI_FLASH_ISR_TXEIS_PEND         ((uint32_t)     9)
#define bfwSPI_FLASH_ISR_TXEIS_PEND         ((uint32_t)     1)

#define bfoSPI_FLASH_RISR_TXEIR             ((uint32_t)     0)
#define bfwSPI_FLASH_RISR_TXEIR             ((uint32_t)     1)
#define bfoSPI_FLASH_RISR_TXOIR             ((uint32_t)     1)
#define bfwSPI_FLASH_RISR_TXOIR             ((uint32_t)     1)
#define bfoSPI_FLASH_RISR_RXUIR             ((uint32_t)     2)
#define bfwSPI_FLASH_RISR_RXUIR             ((uint32_t)     1)
#define bfoSPI_FLASH_RISR_RXOIR             ((uint32_t)     3)
#define bfwSPI_FLASH_RISR_RXOIR             ((uint32_t)     1)
#define bfoSPI_FLASH_RISR_RXFIR             ((uint32_t)     4)
#define bfwSPI_FLASH_RISR_RXFIR             ((uint32_t)     1)
#define bfoSPI_FLASH_RISR_FSEIR             ((uint32_t)     5)
#define bfwSPI_FLASH_RISR_FSEIR             ((uint32_t)     1)
#define bfoSPI_FLASH_RISR_WBEIR             ((uint32_t)     6)
#define bfwSPI_FLASH_RISR_WBEIR             ((uint32_t)     1)
#define bfoSPI_FLASH_RISR_BYEIR             ((uint32_t)     7)
#define bfwSPI_FLASH_RISR_BYEIR             ((uint32_t)     1)
#define bfoSPI_FLASH_RISR_ACTIR             ((uint32_t)     8)
#define bfwSPI_FLASH_RISR_ACTIR             ((uint32_t)     1)

#define bfoSPI_FLASH_TXOICR_TXOICR          ((uint32_t)     0)
#define bfwSPI_FLASH_TXOICR_TXOICR          ((uint32_t)     1)

#define bfoSPI_FLASH_RXOICR_RXOICR          ((uint32_t)     0)
#define bfwSPI_FLASH_RXOICR_RXOICR          ((uint32_t)     1)

#define bfoSPI_FLASH_RXUICR_RXUICR          ((uint32_t)     0)
#define bfwSPI_FLASH_RXUICR_RXUICR          ((uint32_t)     1)

#define bfoSPI_FLASH_MSTICR_MSTICR          ((uint32_t)     0)
#define bfwSPI_FLASH_MSTICR_MSTICR          ((uint32_t)     1)

#define bfoSPI_FLASH_ICR_ICR                ((uint32_t)     0)
#define bfwSPI_FLASH_ICR_ICR                ((uint32_t)     1)

#define bfoSPI_FLASH_DMACR_RDMAE            ((uint32_t)     0)
#define bfwSPI_FLASH_DMACR_RDMAE            ((uint32_t)     1)
#define bfoSPI_FLASH_DMACR_TDMAE            ((uint32_t)     1)
#define bfwSPI_FLASH_DMACR_TDMAE            ((uint32_t)     1)

#define bfoSPI_FLASH_DMATDLR_DMATDL         ((uint32_t)     0)
#define bfwSPI_FLASH_DMATDLR_DMATDL         ((uint32_t)     3)

#define bfoSPI_FLASH_DMARDLR_DMARDL         ((uint32_t)     0)
#define bfwSPI_FLASH_DMARDLR_DMARDL         ((uint32_t)     3)

#define bfoSPI_FLASH_DR0_dr0                ((uint32_t)     0)
#define bfwSPI_FLASH_DR0_dr0                ((uint32_t)    16)
#define bfoSPI_FLASH_DR1_dr1                ((uint32_t)     0)
#define bfwSPI_FLASH_DR1_dr1                ((uint32_t)    16)
#define bfoSPI_FLASH_DR2_dr2                ((uint32_t)     0)
#define bfwSPI_FLASH_DR2_dr2                ((uint32_t)    16)
#define bfoSPI_FLASH_DR3_dr3                ((uint32_t)     0)
#define bfwSPI_FLASH_DR3_dr3                ((uint32_t)    16)
#define bfoSPI_FLASH_DR4_dr4                ((uint32_t)     0)
#define bfwSPI_FLASH_DR4_dr4                ((uint32_t)    16)
#define bfoSPI_FLASH_DR5_dr5                ((uint32_t)     0)
#define bfwSPI_FLASH_DR5_dr5                ((uint32_t)    16)
#define bfoSPI_FLASH_DR6_dr6                ((uint32_t)     0)
#define bfwSPI_FLASH_DR6_dr6                ((uint32_t)    16)
#define bfoSPI_FLASH_DR7_dr7                ((uint32_t)     0)
#define bfwSPI_FLASH_DR7_dr7                ((uint32_t)    16)
#define bfoSPI_FLASH_DR8_dr8                ((uint32_t)     0)
#define bfwSPI_FLASH_DR8_dr8                ((uint32_t)    16)
#define bfoSPI_FLASH_DR9_dr9                ((uint32_t)     0)
#define bfwSPI_FLASH_DR9_dr9                ((uint32_t)    16)
#define bfoSPI_FLASH_DR10_dr10              ((uint32_t)     0)
#define bfwSPI_FLASH_DR10_dr10              ((uint32_t)    16)
#define bfoSPI_FLASH_DR11_dr11              ((uint32_t)     0)
#define bfwSPI_FLASH_DR11_dr11              ((uint32_t)    16)
#define bfoSPI_FLASH_DR12_dr12              ((uint32_t)     0)
#define bfwSPI_FLASH_DR12_dr12              ((uint32_t)    16)
#define bfoSPI_FLASH_DR13_dr13              ((uint32_t)     0)
#define bfwSPI_FLASH_DR13_dr13              ((uint32_t)    16)
#define bfoSPI_FLASH_DR14_dr14              ((uint32_t)     0)
#define bfwSPI_FLASH_DR14_dr14              ((uint32_t)    16)
#define bfoSPI_FLASH_DR15_dr15              ((uint32_t)     0)
#define bfwSPI_FLASH_DR15_dr15              ((uint32_t)    16)

#define bfoSPI_FLASH_AUTO_LEN_ADDR          ((uint32_t)    16)
#define bfwSPI_FLASH_AUTO_LEN_ADDR          ((uint32_t)     2)
#define bfoSPI_FLASH_AUTO_LEN_DUM           ((uint32_t)     0)
#define bfwSPI_FLASH_AUTO_LEN_DUM           ((uint32_t)    16)


static inline struct spi_flash_portmap *get_rlx_spi_base(void)
{
	return (struct spi_flash_portmap *)RLX_SPI_BASEADDR;
}

static inline struct rlx_spi_slave *to_rlx_spi(struct spi_slave *slave)
{
	return container_of(slave, struct rlx_spi_slave, slave);
}

#endif /* __ANDES_SPI_H */

