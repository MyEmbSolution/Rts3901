#include <common.h>
#include <asm/ddr_def.h>
#include "ms_rxi310_defs.h"

#define _DEBUG_DDR_BIST_
#define ALIGN32_FLOOR(addr)      ((addr) & 0xFFFFFFFC)
#define ALIGN16_FLOOR(addr)      ((addr) & 0xFFFFFFFe)

#define _MEM32_(addr)	(*(volatile u32 *)(addr))
#define WRITE_MEM32(addr, val)   (*(volatile unsigned int *) (addr)) = (val)
#define READ_MEM32(addr)         (*(volatile unsigned int *) (addr))
#define WRITE_MEM16(addr, val)   (*(volatile unsigned short *) (addr)) = (val)
#define READ_MEM16(addr)         (*(volatile unsigned short *) (addr))
#define WRITE_MEM8(addr, val)    (*(volatile unsigned char *) (addr)) = (val)
#define READ_MEM8(addr)          (*(volatile unsigned char *) (addr))

#define MS_DRAMC_BASE            0xB8010000
#define BIST_BASE_ADDR           0xB80A0000


#define CMD_SRAM_SIZE            0x1000          /* 4k*/
#define MSK_SRAM_SIZE            0x2800          /* 10k*/
#define WD_SRAM_SIZE             0x8800          /* 34k*/
#define RD_SRAM_SIZE             0x8800          /* 34k*/

#define CMD_SRAM_BASE            BIST_BASE_ADDR  /*4KB*/
#define CMD_SRAM_TOP             CMD_SRAM_BASE + (1 * CMD_SRAM_SIZE) /* 4KB: */
#define MSK_SRAM_BASE            CMD_SRAM_TOP                        /*      0x0_1000*/
#define MSK_SRAM_TOP             MSK_SRAM_BASE + (1 * MSK_SRAM_SIZE) /* 10KB: */
#define WD_SRAM_BASE             MSK_SRAM_TOP                        /*      0x0_3800 */
#define WD_SRAM_TOP              WD_SRAM_BASE  + (1 * WD_SRAM_SIZE)  /* 34KB:*/
#define RG_SRAM_BASE             WD_SRAM_TOP                         /*      0x0_C000*/
#define RG_SRAM_TOP              RG_SRAM_BASE  + (1 * RD_SRAM_SIZE)  /* 34KB:       */
#define RD_SRAM_BASE             RG_SRAM_TOP                         /*      0x1_4800 */
#define RD_SRAM_TOP              RD_SRAM_BASE  + (1 * RD_SRAM_SIZE)  /* 34KB: */

#define CMD_SRAM_MAX_ADDR        CMD_SRAM_BASE + (272 *  4 * 2 - 4)
#define MSK_SRAM_MAX_ADDR        MSK_SRAM_BASE + (2112 * 4 * 1 - 4)
#define WD_SRAM_MAX_ADDR         WD_SRAM_BASE  + (2112 * 4 * 4 - 4)
#define RG_SRAM_MAX_ADDR         RG_SRAM_BASE  + (2112 * 4 * 4 - 4)
#define RD_SRAM_MAX_ADDR         RD_SRAM_BASE  + (2112 * 4 * 4 - 4)

#define CR_DIS_REF               0x0

#define BIST_LOOP_EN             0x0
#define BIST_CMP_EN              0x0
#define BIST_AT_STOP_EN          0x0
#define BIST_DIS_MASK_EN         0x0


unsigned int bist_pattern[] = {
0x8d1ce4fe, 	0x2e495904,	0xe6b5d418,	0x55bdfa51,	0x55bdfa51,	0xfc8d1ce4,	0x82e4959,	0x30e6b5d4,
0xa355bdfa,	0xc8fc8d1c,	0xb3082e49,	0xa930e6b5,	0xf4a355bd,	0x38c8fc8d,	0x93b3082e,	0x6ba930e6,
0x7bf4a355,	0x1a38c8fc,	0x5d93b308,	0xcc6ba930,	0xab7bf4a3,	0xf81a38c8,	0x105d93b3,	0x61cc6ba9,
0x47ab7bf4, 	0x91f81a38,	0x67105d93, 	0x5361cc6b,	0xe847ab7b,	0x7191f81a,	0x2667105d,	0xd65361cc,
0xf6e847ab,	0x347191f8,	0xbb266710,	0x99d65361,	0x57f6e847,	0xf0347191,	0x20bb2667,	0xc299d653,
0x8f57f6e8,	0x22f03471,	0xce20bb26,	0xa7c299d6,	0xd08f57f6,	0xe222f034,	0x4dce20bb,	0xada7c299,
0xecd08f57,	0x69e222f0,	0x774dce20,	0x32ada7c2,	0xafecd08f,	0xe069e222,	0x41774dce,	0x8532ada7,
0x1eafecd0,	0x45e069e2,	0x9d41774d,	0x4f8532ad,	0xa11eafec,	0xc445e069,	0x9b9d4177,	0x5b4f8532,
0xd8a11eaf,	0xd2c445e0,	0xee9b9d41,	0x655b4f85,	0x5fd8a11e,	0xc0d2c445,	0x83ee9b9d,	0xa655b4f,
0x3c5fd8a1,	0x8bc0d2c4,	0x3a83ee9b,	0x9f0a655b,	0x433c5fd8,	0x898bc0d2,	0x363a83ee,	0xb79f0a65,
0xb1433c5f,	0xa5898bc0,	0xdc363a83,	0xcab79f0a,	0xbfb1433c,	0x81a5898b,	0x6dc363a,	0x14cab79f,
0x79bfb143,	0x1681a589,	0x7506dc36,	0x3e14cab7,	0x8779bfb1,	0x121681a5,	0x6d7506dc,	0x6f3e14ca,
0x638779bf,	0x4b121681,	0xb96d7506,	0x956f3e14,	0x7f638779,	0x24b1216,	0xcb96d75,	0x28956f3e,
0xf27f6387,	0x2c024b12,	0xea0cb96d,	0x7d28956f,	0xef27f63,	0x242c024b,	0xdaea0cb9,	0xde7d2895,
0xc60ef27f,	0x97242c02,	0x73daea0c,	0x2ade7d28,	0xfec60ef2,	0x497242c,	0x1873daea,	0x512ade7d,
0xe4fec60e,	0x59049724,	0xd41873da,	0xfa512ade,	0x1ce4fec6,	0x49590497,	0xb5d41873,	0xbdfa512a,
0x8d1ce4fe,	0x2e495904,	0xe6b5d418,	0x55bdfa51,	0xfc8d1ce4,	0x82e4959,	0x30e6b5d4,	0xa355bdfa,
0xc8fc8d1c,	0xb3082e49,	0xa930e6b5,	0xf4a355bd,	0x38c8fc8d,	0x93b3082e,	0x6ba930e6,	0x7bf4a355,
0x1a38c8fc,	0x5d93b308,	0xcc6ba930,	0xab7bf4a3,	0xf81a38c8,	0x105d93b3,	0x61cc6ba9,	0x47ab7bf4,
0x91f81a38,	0x67105d93,	0x5361cc6b,	0xe847ab7b,	0x7191f81a,	0x2667105d,	0xd65361cc,	0xf6e847ab,
0x347191f8,	0xbb266710,	0x99d65361,	0x57f6e847,	0xf0347191,	0x20bb2667,	0xc299d653,	0x8f57f6e8,
0x22f03471,	0xce20bb26,	0xa7c299d6,	0xd08f57f6,	0xe222f034,	0x4dce20bb,	0xada7c299,	0xecd08f57,
0x69e222f0,	0x774dce20,	0x32ada7c2,	0xafecd08f,	0xe069e222,	0x41774dce,	0x8532ada7,	0x1eafecd0,
0x45e069e2,	0x9d41774d,	0x4f8532ad,	0xa11eafec,	0xc445e069,	0x9b9d4177,	0x5b4f8532,	0xd8a11eaf,
0xd2c445e0,	0xee9b9d41,	0x655b4f85,	0x5fd8a11e,	0xc0d2c445,	0x83ee9b9d,	0xa655b4f,	0x3c5fd8a1,
0x8bc0d2c4,	0x3a83ee9b,	0x9f0a655b,	0x433c5fd8,	0x898bc0d2,	0x363a83ee,	0xb79f0a65,	0xb1433c5f,
0xa5898bc0,	0xdc363a83,	0xcab79f0a,	0xbfb1433c,	0x81a5898b,	0x6dc363a,	0x14cab79f,	0x79bfb143,
0x1681a589,	0x7506dc36,	0x3e14cab7,	0x8779bfb1,	0x121681a5,	0x6d7506dc,	0x6f3e14ca,	0x638779bf,
0x4b121681,	0xb96d7506,	0x956f3e14, 	0x7f638779, 	0x24b1216,	0xcb96d75,	0x28956f3e,	0xf27f6387,
0x2c024b12,	0xea0cb96d,	0x7d28956f,	0xef27f63,	0x242c024b,	0xdaea0cb9,	0xde7d2895,	0xc60ef27f,
0x97242c02,	0x73daea0c,	0x2ade7d28,	0xfec60ef2,	0x497242c,	0x1873daea,	0x512ade7d,	0xe4fec60e,
0x59049724,	0xd41873da,	0xfa512ade,	0x1ce4fec6,	0x49590497,	0xb5d41873,	0xbdfa512a,	0x8d1ce4fe,
};

void bist_cmd_wr (struct bist_cmd_data_b37 *,  uint32_t addr);
void bist_wr_128 (struct bist_data_b128 *,     uint32_t addr);
void bist_rd_128 (struct bist_data_b128 *,     uint32_t addr);

void set_bist_data(void)
{
	unsigned int data_ram_addr0,  data_ram_addr1;
	unsigned int i;
	struct ms_rxi310_portmap *dram_ctrl = (struct ms_rxi310_portmap *)MS_DRAMC_BASE;
	dram_ctrl->csr = 0x300;
	dram_ctrl->bcr = 0xf00;
	dram_ctrl->bcr = 0;

	data_ram_addr0 = 0xb80a3800;
	data_ram_addr1 = 0xb80ac000;
	for (i = 0; i < 0x100; i = i+1) {
		/*WRITE_MEM32((data_ram_addr + (i * 4)), bist_pattern[i]);*/
		_MEM32_(data_ram_addr0 + (i * 4)) = bist_pattern[i];
		/*WRITE_MEM32((data_ram_addr + (i * 4)), bist_pattern[i]);*/
		_MEM32_(data_ram_addr1 + (i * 4)) = _MEM32_(data_ram_addr0 + (i * 4));
	}

/*
	while (((dram_ctrl->bsram1) & 0x7fff) < 0x100) {
		printf("bsram1 is %x\n ", dram_ctrl->bsram1);
	}
*/
}

void set_bist_data_reverse(void)
{
	unsigned int data_ram_addr0, data_ram_addr1 ;
	unsigned int i;
	struct ms_rxi310_portmap *dram_ctrl = (struct ms_rxi310_portmap *)MS_DRAMC_BASE;
	dram_ctrl->csr = 0x300;
	dram_ctrl->bcr = 0xf00;
	dram_ctrl->bcr = 0;

	data_ram_addr0 = 0xb80a3800;
	data_ram_addr1 = 0xb80ac000;
	for (i = 0; i < 0x100; i = i+1) {
		/*WRITE_MEM32((data_ram_addr + (i * 4)), bist_pattern[i]);*/
		_MEM32_(data_ram_addr0 + (i * 4)) = (~bist_pattern[i]);
		/*WRITE_MEM32((data_ram_addr + (i * 4)), bist_pattern[i]);*/
		_MEM32_(data_ram_addr1 + (i * 4)) = _MEM32_(data_ram_addr0 + (i * 4));
	}
/*
	while (((dram_ctrl->bsram1) & 0x7fff) < 0x100) {
		printf("bsram1 is %x\n ", dram_ctrl->bsram1);
	}
*/
}

int bist_test_once(unsigned int  ddr_offset)
{
	struct ms_rxi310_portmap *dram_ctrl = (struct ms_rxi310_portmap *) MS_DRAMC_BASE;
	unsigned int tmp;
	unsigned char i;

	dram_ctrl->bcr = 0x0100000a;
#if 0
#ifdef _DDR_64MB_
	bist_cmd.CMD_BANK = DW_BITS_GET(ddr_offset, 11, 13);     /*cmd_bank_num_r*/
	bist_cmd.CMD_PAGE = DW_BITS_GET(ddr_offset, 14, 14);    /*cmd_page_num_r*/
	bist_cmd.CMD_COLU = DW_BITS_GET(ddr_offset, 1, 10); 	/*cmd_colu_num_r*/
#endif

#ifdef _DDR_256MB_
	bist_cmd.CMD_BANK = DW_BITS_GET(ddr_offset, 11, 3);       /*cmd_bank_num_r*/
	bist_cmd.CMD_PAGE = DW_BITS_GET(ddr_offset, 14, 14);       /*cmd_page_num_r*/
	bist_cmd.CMD_COLU = DW_BITS_GET(ddr_offset, 1, 10); 	/*cmd_colu_num_r*/
#endif
	bist_cmd.BST_LEN  = 0x10;             /*isu_bst_len_r*/

	for (i = 0; i < 8 ; i++) {
		bist_cmd.CMD_COLU = DW_BITS_GET((ddr_offset + i * 128), 1, 10);
		bist_cmd.WRRD     = WR_CMD;
		bist_cmd_wr(&bist_cmd, (CMD_SRAM_BASE +  i * 16));


		bist_cmd.WRRD     = RD_CMD;
		bist_cmd_wr(&bist_cmd, (CMD_SRAM_BASE + i * 16 + 8));
	}
#else
	for (i = 0; i < 8 ; i++) {
		tmp = ddr_offset + i * 128;
		_MEM32_(CMD_SRAM_BASE + i * 16) = 0x01|0x20|((tmp & 0x7fe) << 5) | ((tmp & 0x7ffc000) << 5);
		_MEM32_(CMD_SRAM_BASE + i * 16 + 4) = (tmp & 0x3800) >> 9;
		_MEM32_(CMD_SRAM_BASE + i * 16 + 8) = 0x20|((tmp & 0x7fe) << 5) | ((tmp & 0x7ffc000) << 5);			    _MEM32_(CMD_SRAM_BASE + i * 16 + 12) = (tmp & 0x3800) >> 9;
	}
#endif
	if (ddr_offset != 0)
		dram_ctrl->bcr |= 0x6000;


	/********************************************************
	*******          enter BIST_MODE               *********
	********************************************************/

	/* Wait BIST_IDLE, ~DT_IDLE, ~MEM_IDLE*/
/*	while (DW_BITS_GET(dram_ctrl->csr,  PCTL_CSR_MEM_IDLE_BFO, 3) != 0x3) {
		printf("[INFO] Wait BIST_MODE\n");
	}
*/
/*	printf("check count , bsram0 %x, bsram1 %x\n", dram_ctrl->bsram0, dram_ctrl->bsram1);*/
	/********************************************************
	*******          BIST_MODE start               *********
	********************************************************/
	/*DW_BIT_SET(dram_ctrl-> ccr, PCTL_CCR_BTT_BFO); // it can't use DW_BIT_SET, because CCR Read_data mean status,*/
	/*write_data mean opeartions*/
	dram_ctrl->ccr = 1 << (PCTL_CCR_BTT_BFO) ;
	/********************************************************
	*******      check rd_data is correct?         *********
	********************************************************/

	/********************************************************
	*******        Issue BIST_MODE done            *********
	********************************************************/
	/* Wait BIST_DONE */
	while (!DW_BIT_GET(dram_ctrl->ccr, PCTL_CCR_BTT_BFO)) {
/*		printf("wait bist action %x\n",  dram_ctrl->ccr);*/
	}

	/* check if error*/
	if (DW_BITS_GET(dram_ctrl->bst, PCTL_BST_ERR_CNT_BFO, 14) != 0) {
		printf("[ERR] BIST error, error count is %x, error bit is %x\n", DW_BITS_GET(dram_ctrl->bst, PCTL_BST_ERR_CNT_BFO, 14), dram_ctrl->ber);
		dram_ctrl->bcr = 0;
		dram_ctrl->ccr = 0x100;
		return -1;
	}

	dram_ctrl->ccr = 0x100;
	return 0;

}

void bist_test_done(void)
{
	struct ms_rxi310_portmap *dram_ctrl = (struct ms_rxi310_portmap *) MS_DRAMC_BASE;

	/********************************************************
	*******        Exit BIST_MODE                  *********
	********************************************************/
	/* exit bist access*/
	dram_ctrl->csr = 0x700;
	dram_ctrl->csr = 0x600;
}

void DDR_Bist(void)
{
	u32 mem_offset;

	init_bf_print();
	set_bist_data();
	for (mem_offset = 0; mem_offset < 0x100000; mem_offset += _BIST_MEM_SIZE_) {
		if ((bist_test_once(mem_offset)) == -1) {
			printf("test fail\n");
			break;
		}
	}

	set_bist_data_reverse();
	for (mem_offset = 0; mem_offset < 0x100000; mem_offset += _BIST_MEM_SIZE_) {
		if ((bist_test_once(mem_offset)) == -1) {
			printf("test fail\n");
			break;
		}
	}

	bist_test_done();
	printf("DDR bist test finish\n");
}

