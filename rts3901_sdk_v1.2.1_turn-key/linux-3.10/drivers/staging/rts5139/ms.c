/* Driver for Realtek RTS51xx USB card reader
 *
 * Copyright(c) 2009 Realtek Semiconductor Corp. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author:
 *   wwang (wei_wang@realsil.com.cn)
 *   No. 450, Shenhu Road, Suzhou Industry Park, Suzhou, China
 */

#include <linux/blkdev.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "debug.h"
#include "trace.h"
#include "rts51x.h"
#include "rts51x_transport.h"
#include "rts51x_scsi.h"
#include "rts51x_card.h"
#include "ms.h"

static inline void ms_set_err_code(struct rts51x_chip *chip, u8 err_code)
{
	struct ms_info *ms_card = &(chip->ms_card);

	ms_card->err_code = err_code;
}

static inline int ms_check_err_code(struct rts51x_chip *chip, u8 err_code)
{
	struct ms_info *ms_card = &(chip->ms_card);

	return (ms_card->err_code == err_code);
}

static int ms_parse_err_code(struct rts51x_chip *chip)
{
	TRACE_RET(chip, STATUS_FAIL);
}

static int ms_transfer_tpc(struct rts51x_chip *chip, u8 trans_mode, u8 tpc, u8 cnt, u8 cfg)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;

	RTS51X_DEBUGP(("ms_transfer_tpc: tpc = 0x%x\n", tpc));

	rts51x_init_cmd(chip);

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TPC, 0xFF, tpc);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_BYTE_CNT, 0xFF, cnt);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANS_CFG, 0xFF, cfg);
	rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_DATA_SOURCE, 0x01, PINGPONG_BUFFER);

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANSFER, 0xFF, MS_TRANSFER_START | trans_mode);
	rts51x_add_cmd(chip, CHECK_REG_CMD, MS_TRANSFER, MS_TRANSFER_END, MS_TRANSFER_END);

	rts51x_add_cmd(chip, READ_REG_CMD, MS_TRANS_CFG, 0, 0);

	retval = rts51x_send_cmd(chip, MODE_CR, 100);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	// For MS-PRO Certification, Timeout at least 3 seconds
	retval = rts51x_get_rsp(chip, 2, 5000);

#ifdef RCC_BUG_FIX_SP
	if((chip->option.rcc_fail_flag == 2)&&(chip->option.rcc_bug_fix_en==1))
	{//if rcc bug fix support and happen,then fix the bug
		u8 tmpvalue;
		int num = 0;

		chip->rcc_read_response = 0;
		rts51x_ep0_read_register(chip, SFSM_ED, &tmpvalue);
		if(!(tmpvalue&CARD_ERR))//if card is not error,then it is stalled by rcc bug
		{
			for(num=0;num < 5000;num++)
			{
				rts51x_ep0_read_register(chip, MS_TRANSFER, &tmpvalue);
				if(tmpvalue&MS_TRANSFER_END)
				{
					break;
				}
				wait_timeout(1);
			}
		}
		if((tmpvalue&MS_TRANSFER_ERR)||(num==5000))
		{
			retval = STATUS_FAIL;
		}
		else
		{
			*chip->rsp_buf = tmpvalue;
			rts51x_ep0_read_register(chip, MS_TRANS_CFG, chip->rsp_buf+1);
			retval = STATUS_SUCCESS;
		}
		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, MC_FIFO_CTL, FIFO_FLUSH, FIFO_FLUSH);
//		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, SFSM_ED, 0xf8, 0xf8);
	}
#endif

	if (CHECK_MS_TRANS_FAIL(chip, retval)) {
		rts51x_clear_ms_error(chip);
		ms_set_err_code(chip, MS_TO_ERROR);
		TRACE_RET(chip, ms_parse_err_code(chip));
	}

	if (!(tpc & 0x08)) {		// Read Packet
		// Check CRC16 & Ready Timeout
		if (chip->rsp_buf[1] & MS_CRC16_ERR) {
			ms_set_err_code(chip, MS_CRC16_ERROR);
			TRACE_RET(chip, ms_parse_err_code(chip));
		}
	} else {			// Write Packet
		// For MS-Pro Certification 2-3-19 & 2-9-3
		// We shall retry when write packet error occurred.
        	// When Read packet error(BREQ&ERR), CED=0,
		// thus MS_TRANS_FINISHED_INTERRUPT detection will timeout
        	// So, we will retry out of this function.
		if (CHK_MSPRO(ms_card) && !(chip->rsp_buf[1] & 0x80)) {
			// DAT[3..1] have meaning when MS-Pro(4-bit) and MS_INT_EXPECTED is set
			// We check Write packet error only after reseting MS-Pro(4-bit INT) card
			if (chip->rsp_buf[1] & (MS_INT_ERR | MS_INT_CMDNK)) {
				ms_set_err_code(chip, MS_CMD_NK);
				TRACE_RET(chip, ms_parse_err_code(chip));
			}
		}
	}

	// Check Timeout of Ready Signal
	if (chip->rsp_buf[1] & MS_RDY_TIMEOUT) {
		rts51x_clear_ms_error(chip);
		ms_set_err_code(chip, MS_TO_ERROR);
		TRACE_RET(chip, ms_parse_err_code(chip));
	}

	return STATUS_SUCCESS;
}

int ms_transfer_data(struct rts51x_chip *chip, u8 trans_mode, u8 tpc, u16 sec_cnt,
		u8 cfg, int mode_2k, int use_sg, void *buf, int buf_len)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;
	u8 val, err_code = 0, flag = 0;
	enum dma_data_direction dir;
	unsigned int pipe;

	if (!buf || !buf_len) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	if (trans_mode == MS_TM_AUTO_READ) {
		pipe = RCV_BULK_PIPE(chip);
		dir = DMA_FROM_DEVICE;
		flag = MODE_CDIR;
		err_code = MS_FLASH_READ_ERROR;
	} else if (trans_mode == MS_TM_AUTO_WRITE) {
		pipe = SND_BULK_PIPE(chip);
		dir = DMA_TO_DEVICE;
		flag = MODE_CDOR;
		err_code = MS_FLASH_WRITE_ERROR;
	} else {
		TRACE_RET(chip, STATUS_FAIL);
	}

	rts51x_init_cmd(chip);

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TPC, 0xFF, tpc);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_SECTOR_CNT_H, 0xFF, (u8)(sec_cnt >> 8));
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_SECTOR_CNT_L, 0xFF, (u8)sec_cnt);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANS_CFG, 0xFF, cfg);
	rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_DATA_SOURCE, 0x01, RING_BUFFER);

	if (mode_2k) {
		rts51x_add_cmd(chip, WRITE_REG_CMD, MS_CFG, MS_2K_SECTOR_MODE, MS_2K_SECTOR_MODE);
	} else {
		rts51x_add_cmd(chip, WRITE_REG_CMD, MS_CFG, MS_2K_SECTOR_MODE, 0);
	}

	trans_dma_enable(dir, chip, sec_cnt * 512, DMA_512);

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANSFER, 0xFF, MS_TRANSFER_START | trans_mode);
	rts51x_add_cmd(chip, CHECK_REG_CMD, MS_TRANSFER, MS_TRANSFER_END, MS_TRANSFER_END);

	retval = rts51x_send_cmd(chip, flag | STAGE_MS_STATUS, 100);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	retval = rts51x_transfer_data_rcc(chip, pipe, buf, buf_len, use_sg, NULL, 15000,flag);
	if (retval != STATUS_SUCCESS) {
		ms_set_err_code(chip, err_code);
		rts51x_clear_ms_error(chip);
		TRACE_RET(chip, retval);
	}

#ifdef RCC_BUG_FIX_SP
	if(chip->rcc_read_response)//for read data stall
	{
		u8 tmpvalue;
		u16 num;

		chip->rcc_read_response = 0;
//		rts51x_ep0_write_register(chip, SFSM_ED, 0xf8, 0xf8);
		for(num=0;num < 10000;num++)
		{
			rts51x_ep0_read_register(chip, MS_TRANSFER, &tmpvalue);
			if(tmpvalue&MS_TRANSFER_END)
			{
				break;
			}
			wait_timeout(1);
		}

		if((tmpvalue&MS_TRANSFER_ERR)||(num==10000))
		{
			ms_set_err_code(chip, err_code);
			rts51x_clear_ms_error(chip);
			TRACE_RET(chip, STATUS_FAIL);
		}
		*chip->rsp_buf = tmpvalue;
		rts51x_ep0_read_register(chip, MS_TRANS_CFG, chip->rsp_buf+1);
		rts51x_ep0_read_register(chip, MS_INT_REG, chip->rsp_buf+2);
	}
	else
	{
		retval = rts51x_get_rsp(chip, 3, 15000);

		if((chip->option.rcc_fail_flag == 2)&&(chip->option.rcc_bug_fix_en==1))
		{//if rcc bug fix support and happen,then fix the bug
			u8 tmpvalue;
			u16 num = 0;
			rts51x_reset_pipe(chip,0);//send clear feature to clear stall
			rts51x_ep0_write_register(chip, MC_FIFO_CTL, FIFO_FLUSH, FIFO_FLUSH);
//			rts51x_reset_pipe(chip,0);//send clear feature to clear stall
			rts51x_ep0_write_register(chip, SFSM_ED, 0xf8, 0xf8);
			for(num=0;num < 10000;num++)
			{
				rts51x_ep0_read_register(chip, MS_TRANSFER, &tmpvalue);
				if(tmpvalue&MS_TRANSFER_END)
				{
					break;
				}
				wait_timeout(1);
			}

			if((tmpvalue&MS_TRANSFER_ERR)||(num==10000))
			{
				ms_set_err_code(chip, err_code);
				rts51x_clear_ms_error(chip);
				TRACE_RET(chip, STATUS_FAIL);
			}
			*chip->rsp_buf = tmpvalue;
			rts51x_ep0_read_register(chip, MS_TRANS_CFG, chip->rsp_buf+1);
			rts51x_ep0_read_register(chip, MS_INT_REG, chip->rsp_buf+2);
			retval = STATUS_SUCCESS;
		}

		if (CHECK_MS_TRANS_FAIL(chip, retval)) {
			ms_set_err_code(chip, err_code);
			rts51x_clear_ms_error(chip);
			TRACE_RET(chip, STATUS_FAIL);
		}
	}
#else
	retval = rts51x_get_rsp(chip, 3, 15000);
	if (CHECK_MS_TRANS_FAIL(chip, retval)) {
		ms_set_err_code(chip, err_code);
		rts51x_clear_ms_error(chip);
		TRACE_RET(chip, STATUS_FAIL);
	}
#endif

// 	RTS51X_READ_REG(chip, MS_TRANS_CFG, &val);
	ms_card->last_rw_int = val = chip->rsp_buf[1];
	if (val & (MS_INT_CMDNK | MS_INT_ERR | MS_CRC16_ERR | MS_RDY_TIMEOUT)) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	return STATUS_SUCCESS;
}

int ms_write_bytes(struct rts51x_chip *chip, u8 tpc, u8 cnt, u8 cfg, u8 *data, int data_len)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, i;

	if (!data || (data_len < cnt)) {
		TRACE_RET(chip, STATUS_ERROR);
	}

	rts51x_init_cmd(chip);

	for (i = 0; i < cnt; i++) {
		rts51x_add_cmd(chip, WRITE_REG_CMD, PPBUF_BASE2 + i, 0xFF, data[i]);
	}
	// SRAM2 16-bit issue
	if (cnt % 2) {
		rts51x_add_cmd(chip, WRITE_REG_CMD, PPBUF_BASE2 + i, 0xFF, 0xFF);
	}

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TPC, 0xFF, tpc);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_BYTE_CNT, 0xFF, cnt);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANS_CFG, 0xFF, cfg);
	rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_DATA_SOURCE, 0x01, PINGPONG_BUFFER);

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANSFER, 0xFF, MS_TRANSFER_START | MS_TM_WRITE_BYTES);
	rts51x_add_cmd(chip, CHECK_REG_CMD, MS_TRANSFER, MS_TRANSFER_END, MS_TRANSFER_END);

	retval = rts51x_send_cmd(chip, MODE_CR, 100);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	// For MS-PRO Certification, Timeout at least 3 seconds
	retval = rts51x_get_rsp(chip, 1, 5000);

#ifdef RCC_BUG_FIX_SP
	if((chip->option.rcc_fail_flag == 2)&&(chip->option.rcc_bug_fix_en==1))
	{//if rcc bug fix support and happen,then fix the bug
		u8 tmpvalue;
		int num = 0;

		rts51x_ep0_read_register(chip, SFSM_ED, &tmpvalue);
		if(!(tmpvalue&CARD_ERR))//if card is not error,then it is stalled by rcc bug
		{
			for(num=0;num < 5000;num++)
			{
				rts51x_ep0_read_register(chip, MS_TRANSFER, &tmpvalue);
				if(tmpvalue&MS_TRANSFER_END)
				{
					break;
				}
				wait_timeout(1);
			}
		}
		if((tmpvalue&MS_TRANSFER_ERR)||(num==5000))
		{
			retval = STATUS_FAIL;
		}
		else
		{
			*chip->rsp_buf = tmpvalue;
			retval = STATUS_SUCCESS;
		}
		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, MC_FIFO_CTL, FIFO_FLUSH, FIFO_FLUSH);
//		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, SFSM_ED, 0xf8, 0xf8);
	}
#endif

	if (CHECK_MS_TRANS_FAIL(chip, retval)) {
		u8 val = 0;

		//sangdy2010-05-11:don't use macro to operate in error handle. because it will quit function if error occurs
		//RTS51X_READ_REG(chip, MS_TRANS_CFG, &val);
		rts51x_ep0_read_register(chip, MS_TRANS_CFG, &val);
		RTS51X_DEBUGP(("MS_TRANS_CFG: 0x%02x\n", val));

		rts51x_clear_ms_error(chip);

		if (!(tpc & 0x08)) {		// Read Packet
			// Check CRC16 & Ready Timeout
			if (val & MS_CRC16_ERR) {
				ms_set_err_code(chip, MS_CRC16_ERROR);
				TRACE_RET(chip, ms_parse_err_code(chip));
			}
		} else {			// Write Packet
			// For MS-Pro Certification 2-3-19 & 2-9-3
			// We shall retry when write packet error occurred.
			// When Read packet error(BREQ&ERR), CED=0,
			// thus MS_TRANS_FINISHED_INTERRUPT detection will timeout
			// So, we will retry out of this function.
			if (CHK_MSPRO(ms_card) && !(val & 0x80)) {
				// DAT[3..1] have meaning when MS-Pro(4-bit) and MS_INT_EXPECTED is set
				// We check Write packet error only after reseting MS-Pro(4-bit INT) card
				if (val & (MS_INT_ERR | MS_INT_CMDNK)) {
					ms_set_err_code(chip, MS_CMD_NK);
					TRACE_RET(chip, ms_parse_err_code(chip));
				}
			}
		}

		// Check Timeout of Ready Signal
		if (val & MS_RDY_TIMEOUT) {
			ms_set_err_code(chip, MS_TO_ERROR);
			TRACE_RET(chip, ms_parse_err_code(chip));
		}


		ms_set_err_code(chip, MS_TO_ERROR);
		TRACE_RET(chip, ms_parse_err_code(chip));
	}

	return STATUS_SUCCESS;
}

int ms_read_bytes(struct rts51x_chip *chip, u8 tpc, u8 cnt, u8 cfg, u8 *data, int data_len)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, i;

	if (!data) {
		TRACE_RET(chip, STATUS_ERROR);
	}

	rts51x_init_cmd(chip);

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TPC, 0xFF, tpc);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_BYTE_CNT, 0xFF, cnt);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANS_CFG, 0xFF, cfg);
	rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_DATA_SOURCE, 0x01, PINGPONG_BUFFER);

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANSFER, 0xFF, MS_TRANSFER_START | MS_TM_READ_BYTES);
	rts51x_add_cmd(chip, CHECK_REG_CMD, MS_TRANSFER, MS_TRANSFER_END, MS_TRANSFER_END);

	for (i = 0; i < data_len - 1; i++) {
	       rts51x_add_cmd(chip, READ_REG_CMD, PPBUF_BASE2 + i, 0, 0);
	}
	// SRAM2 16-bit issue
	if (data_len % 2) {
		rts51x_add_cmd(chip, READ_REG_CMD, PPBUF_BASE2 + data_len, 0, 0);
	} else {
		rts51x_add_cmd(chip, READ_REG_CMD, PPBUF_BASE2 + data_len - 1, 0, 0);
	}

	retval = rts51x_send_cmd(chip, MODE_CR, 100);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	// For MS-PRO Certification, Timeout at least 3 seconds
	retval = rts51x_get_rsp(chip, data_len + 1, 5000);

#ifdef RCC_BUG_FIX_SP
	if((chip->option.rcc_fail_flag == 2)&&(chip->option.rcc_bug_fix_en==1))
	{//if rcc bug fix support and happen,then fix the bug
		u8 tmpvalue;
		int num = 0;

		chip->rcc_read_response = 0;
		rts51x_ep0_read_register(chip, SFSM_ED, &tmpvalue);
		if(!(tmpvalue&CARD_ERR))//if card is not error,then it is stalled by rcc bug
		{
			for(num=0;num < 5000;num++)
			{
				rts51x_ep0_read_register(chip, MS_TRANSFER, &tmpvalue);
				if(tmpvalue&MS_TRANSFER_END)
				{
					break;
				}
				wait_timeout(1);
			}
		}
		if((tmpvalue&MS_TRANSFER_ERR)||(num==5000))
		{
			retval = STATUS_FAIL;
		}
		else
		{
			*chip->rsp_buf = tmpvalue;
			for(i=0; i<data_len-1;i++)
			{
				rts51x_ep0_read_register(chip, PPBUF_BASE2+i, chip->rsp_buf+i+1);
			}
			if(data_len%2)
			{
				rts51x_ep0_read_register(chip, PPBUF_BASE2+data_len, chip->rsp_buf+data_len);
			}
			else
			{
				rts51x_ep0_read_register(chip, PPBUF_BASE2+data_len-1, chip->rsp_buf+data_len);
			}
			retval = STATUS_SUCCESS;
		}
		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, MC_FIFO_CTL, FIFO_FLUSH, FIFO_FLUSH);
//		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, SFSM_ED, 0xf8, 0xf8);
	}
#endif

	if (CHECK_MS_TRANS_FAIL(chip, retval)) {
		u8 val = 0;

		//sangdy2010-05-11:don't use macro to operate in error handle. because it will quit function if error occurs
		//RTS51X_READ_REG(chip, MS_TRANS_CFG, &val);
		rts51x_ep0_read_register(chip, MS_TRANS_CFG, &val);
		RTS51X_DEBUGP(("MS_TRANS_CFG: 0x%02x\n", val));

		rts51x_clear_ms_error(chip);

		if (!(tpc & 0x08)) {		// Read Packet
			// Check CRC16 & Ready Timeout
			if (val & MS_CRC16_ERR) {
				ms_set_err_code(chip, MS_CRC16_ERROR);
				TRACE_RET(chip, ms_parse_err_code(chip));
			}
		} else {			// Write Packet
			// For MS-Pro Certification 2-3-19 & 2-9-3
			// We shall retry when write packet error occurred.
			// When Read packet error(BREQ&ERR), CED=0,
			// thus MS_TRANS_FINISHED_INTERRUPT detection will timeout
			// So, we will retry out of this function.
			if (CHK_MSPRO(ms_card) && !(val & 0x80)) {
				// DAT[3..1] have meaning when MS-Pro(4-bit) and MS_INT_EXPECTED is set
				// We check Write packet error only after reseting MS-Pro(4-bit INT) card
				if (val & (MS_INT_ERR | MS_INT_CMDNK)) {
					ms_set_err_code(chip, MS_CMD_NK);
					TRACE_RET(chip, ms_parse_err_code(chip));
				}
			}
		}

		// Check Timeout of Ready Signal
		if (val & MS_RDY_TIMEOUT) {
			ms_set_err_code(chip, MS_TO_ERROR);
			TRACE_RET(chip, ms_parse_err_code(chip));
		}

		ms_set_err_code(chip, MS_TO_ERROR);
		TRACE_RET(chip, ms_parse_err_code(chip));
	}

	rts51x_read_rsp_buf(chip, 1, data, data_len);

	return STATUS_SUCCESS;
}

int ms_set_rw_reg_addr(struct rts51x_chip *chip,
		u8 read_start, u8 read_cnt, u8 write_start, u8 write_cnt)
{
	int retval, i;
	u8 data[4];

	data[0] = read_start;
	data[1] = read_cnt;
	data[2] = write_start;
	data[3] = write_cnt;

	for (i = 0; i < MS_MAX_RETRY_COUNT; i ++) {
		retval = ms_write_bytes(chip, SET_RW_REG_ADRS, 4, NO_WAIT_INT, data, 4);
		if (retval == STATUS_SUCCESS) {
			return STATUS_SUCCESS;
		}
		rts51x_clear_ms_error(chip);
	}

	TRACE_RET(chip, STATUS_FAIL);
}

static int ms_send_cmd(struct rts51x_chip *chip, u8 cmd, u8 cfg)
{
	u8 data[2];

	data[0] = cmd;
	data[1] = 0;

	return ms_write_bytes(chip, PRO_SET_CMD, 1, cfg, data, 1);
}

static int ms_set_cmd(struct rts51x_chip *chip,
			   u8 read_start, u8 read_count,
			   u8 write_start, u8 write_count,
			   u8 cmd, u8 cfg, u8 *data, int data_len,
			   u8 *int_stat)
{
	int retval, i;
	u8 val;

	if (!data || (data_len <= 0) || (data_len > 128)) {
		RTS51X_DEBUGP(("ms_set_cmd (data_len = %d)\n", data_len));
		TRACE_RET(chip, STATUS_FAIL);
	}

	retval = ms_set_rw_reg_addr(chip, read_start, read_count, write_start, write_count);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
		retval = ms_write_bytes(chip, WRITE_REG, write_count, NO_WAIT_INT, data, data_len);
		if (retval == STATUS_SUCCESS) {
			break;
		}
	}
	if (i == MS_MAX_RETRY_COUNT) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	ms_set_err_code(chip, MS_NO_ERROR);

	for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
		retval = ms_send_cmd(chip, cmd, WAIT_INT);
		if (retval == STATUS_SUCCESS) {
			break;
		}
	}
	if (i == MS_MAX_RETRY_COUNT) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	// GET_INT Register
	ms_set_err_code(chip, MS_NO_ERROR);
	retval = ms_read_bytes(chip, GET_INT, 1, NO_WAIT_INT, &val, 1);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	if (int_stat) {
		*int_stat = val;
	}

	return STATUS_SUCCESS;
}

#ifdef MS_SPEEDUP
static int ms_auto_set_cmd(struct rts51x_chip *chip,
			   u8 read_start, u8 read_count,
			   u8 write_start, u8 write_count,
			   u8 cmd, u8 cfg, u8 *data, int data_len,
			   u8 *int_stat)
{
	int retval;
	int i;

	if (!data || (data_len <= 0) || (data_len > 128)) {
		RTS51X_DEBUGP(("ms_auto_set_cmd (data_len = %d)\n", data_len));
		TRACE_RET(chip, STATUS_FAIL);
	}

	rts51x_init_cmd(chip);

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_READ_START, 0xFF, read_start);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_READ_COUNT, 0xFF, read_count);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_WRITE_START, 0xFF, write_start);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_WRITE_COUNT, 0xFF, write_count);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_COMMAND, 0xFF, cmd);

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANS_CFG, 0xFF, cfg);
	rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_DATA_SOURCE, 0x01, PINGPONG_BUFFER);

	for (i = 0; i < data_len; i++) {
		rts51x_add_cmd(chip, WRITE_REG_CMD, PPBUF_BASE2 + i, 0xFF, data[i]);
	}

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANSFER, 0xFF, MS_TRANSFER_START | MS_TM_SET_CMD);
	rts51x_add_cmd(chip, CHECK_REG_CMD, MS_TRANSFER, MS_TRANSFER_END, MS_TRANSFER_END);

	retval = rts51x_send_cmd(chip, MODE_CR | STAGE_MS_STATUS, 100);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	retval = rts51x_get_rsp(chip, 3, 5000);

#ifdef RCC_BUG_FIX_SP
	if((chip->option.rcc_fail_flag == 2)&&(chip->option.rcc_bug_fix_en==1))
	{//if rcc bug fix support and happen,then fix the bug
		u8 tmpvalue;
		int num = 0;

		chip->rcc_read_response = 0;
		rts51x_ep0_read_register(chip, SFSM_ED, &tmpvalue);
		if(!(tmpvalue&CARD_ERR))//if card is not error,then it is stalled by rcc bug
		{
			for(num=0;num < 5000;num++)
			{
				rts51x_ep0_read_register(chip, MS_TRANSFER, &tmpvalue);
				if(tmpvalue&MS_TRANSFER_END)
				{
					break;
				}
				wait_timeout(1);
			}
		}
		if((tmpvalue&MS_TRANSFER_ERR)||(num==5000))
		{
			retval = STATUS_FAIL;
		}
		else
		{
			*chip->rsp_buf = tmpvalue;
			rts51x_ep0_read_register(chip, MS_TRANS_CFG, chip->rsp_buf+1);
			rts51x_ep0_read_register(chip, MS_INT_REG, chip->rsp_buf+2);
			retval = STATUS_SUCCESS;
		}
		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, MC_FIFO_CTL, FIFO_FLUSH, FIFO_FLUSH);
//		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, SFSM_ED, 0xf8, 0xf8);
	}
#endif

	if (CHECK_MS_TRANS_FAIL(chip, retval)) {
		rts51x_clear_ms_error(chip);
		TRACE_RET(chip, STATUS_FAIL);
	}

	if (int_stat) {
		*int_stat = chip->rsp_buf[2];
	}

	return STATUS_SUCCESS;
}
#endif

static int ms_set_init_para(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;

	if (CHK_HG8BIT(ms_card)) {
		if (chip->asic_code) {
			ms_card->ms_clock = chip->option.asic_ms_hg_clk;
		} else {
			ms_card->ms_clock = chip->option.fpga_ms_hg_clk;
		}
	} else if (CHK_MSPRO(ms_card) || CHK_MS4BIT(ms_card)) {
		if (chip->asic_code) {
			ms_card->ms_clock = chip->option.asic_ms_4bit_clk;
		} else {
			ms_card->ms_clock = chip->option.fpga_ms_4bit_clk;
		}
	} else {
		if (chip->asic_code) {
			ms_card->ms_clock = 38;//sangdy2011-03-11:for SSC may over spec,so decrease 1
		} else {
			ms_card->ms_clock = CLK_40;
		}
	}

	retval = switch_clock(chip, ms_card->ms_clock);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	retval = rts51x_select_card(chip, MS_CARD);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	return STATUS_SUCCESS;
}

int ms_switch_clock(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;

	retval = rts51x_select_card(chip, MS_CARD);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	retval = switch_clock(chip, ms_card->ms_clock);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	return STATUS_SUCCESS;
}

static void ms_pull_ctl_disable(struct rts51x_chip *chip)
{
	if (CHECK_PKG(chip, LQFP48)) {
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL1, 0xFF, 0x55);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL2, 0xFF, 0x55);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL3, 0xFF, 0x95);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL4, 0xFF, 0x55);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL5, 0xFF, 0x55);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL6, 0xFF, 0xA5);
	} else {
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL1, 0xFF, 0x65);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL2, 0xFF, 0x55);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL3, 0xFF, 0x95);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL4, 0xFF, 0x55);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL5, 0xFF, 0x56);//sangdy:modify QFN24 GPIO to pull up when card extracted
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL6, 0xFF, 0x59);
	}
}

static void ms_pull_ctl_enable(struct rts51x_chip *chip)
{
	if (CHECK_PKG(chip, LQFP48)) {
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL1, 0xFF, 0x55);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL2, 0xFF, 0x55);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL3, 0xFF, 0x95);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL4, 0xFF, 0x55);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL5, 0xFF, 0x55);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL6, 0xFF, 0xA5);//sangdy:MS_BS,MS_CLK pull down
	} else {
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL1, 0xFF, 0x65);//sangdy:MS_CLK pull down
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL2, 0xFF, 0x55);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL3, 0xFF, 0x95);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL4, 0xFF, 0x55);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL5, 0xFF, 0x55);
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL6, 0xFF, 0x59);//sangdy:MS_BS pull down
	}
}

static int ms_prepare_reset(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;

	ms_card->ms_type = 0;
	ms_card->check_ms_flow = 0;
	ms_card->switch_8bit_fail = 0;
	ms_card->delay_write.delay_write_flag = 0;

	ms_card->pro_under_formatting = 0;

	rts51x_init_cmd(chip);

	if (chip->asic_code) {
		ms_pull_ctl_enable(chip);
	} else {
		rts51x_add_cmd(chip, WRITE_REG_CMD, FPGA_PULL_CTL, FPGA_MS_PULL_CTL_BIT | 0x20, 0);
	}
	// Tri-state MS output
	rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_OE, MS_OUTPUT_EN, 0);

	if(!chip->option.FT2_fast_mode)
	{//sangdy2010-07-13:not FT2 fast mode,close card power
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PWR_CTL, POWER_MASK, POWER_OFF);
	}

	retval = rts51x_send_cmd(chip, MODE_C, 100);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	if(!chip->option.FT2_fast_mode)
	{//sangdy2010-07-13:not FT2 fast mode,delay and open power
		wait_timeout(250);

		card_power_on(chip, MS_CARD);
		wait_timeout(150);

#ifdef SUPPORT_OCP
		//sangdy2010-06-19:add for OCP support
		 rts51x_get_card_status(chip, &(chip->card_status));
		chip->ocp_stat = (chip->card_status>>4)&0x03;//get OCP status

		if (chip->ocp_stat & (MS_OCP_NOW|MS_OCP_EVER)) {
			RTS51X_DEBUGP(("Over current, OCPSTAT is 0x%x\n", chip->ocp_stat));
			TRACE_RET(chip, STATUS_FAIL);
		}
#endif
	}

	rts51x_init_cmd(chip);

	// Enable MS Output
	rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_OE, MS_OUTPUT_EN, MS_OUTPUT_EN);

	// Reset Registers
	if (chip->asic_code) {
		rts51x_add_cmd(chip, WRITE_REG_CMD, MS_CFG, 0xFF,
			SAMPLE_TIME_RISING | PUSH_TIME_DEFAULT | NO_EXTEND_TOGGLE | MS_BUS_WIDTH_1);
	} else {
		rts51x_add_cmd(chip, WRITE_REG_CMD, MS_CFG, 0xFF,
			SAMPLE_TIME_FALLING | PUSH_TIME_DEFAULT | NO_EXTEND_TOGGLE | MS_BUS_WIDTH_1);
	}
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANS_CFG, 0xFF, NO_WAIT_INT | NO_AUTO_READ_INT_REG);

	retval = rts51x_send_cmd(chip, MODE_C, 100);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}
//	rts51x_clear_ms_error(chip);

	return ms_set_init_para(chip);
}

static int ms_identify_media_type(struct rts51x_chip *chip, int switch_8bit_bus)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, i;
	u8 val;

	retval = ms_set_rw_reg_addr(chip, Pro_StatusReg, 6, SystemParm, 1);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	// Get Register form MS-PRO card
	for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
		retval = ms_transfer_tpc(chip, MS_TM_READ_BYTES, READ_REG, 6, NO_WAIT_INT);
		if (retval == STATUS_SUCCESS) {
			break;
		}
	}
	if (i == MS_MAX_RETRY_COUNT) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	// Procedure to identify Media Type
	// Read register: type, category, class
	//
	// Type Register
	RTS51X_READ_REG(chip, PPBUF_BASE2 + 2, &val);
	RTS51X_DEBUGP(("Type register: 0x%x\n", val));
	if (val != 0x01) {
		// For MS-PRO certification 2-7-5: VTE tester will set "Type Register = 0x02"
		// We shall Power-Off MS_3V3, and we can't do initializing MS too.
		// If not 2, we need check as if Memory Stick Card!
		if (val != 0x02) {
			ms_card->check_ms_flow = 1;
		}
		TRACE_RET(chip, STATUS_FAIL);
	}

	// Category Register
	RTS51X_READ_REG(chip, PPBUF_BASE2 + 4, &val);
	RTS51X_DEBUGP(("Category register: 0x%x\n", val));
	if (val != 0) {
		ms_card->check_ms_flow = 1;
		TRACE_RET(chip, STATUS_FAIL);
	}

	// Class Register
	RTS51X_READ_REG(chip, PPBUF_BASE2 + 5, &val);
	RTS51X_DEBUGP(("Class register: 0x%x\n", val));
	if (val == 0) {
		RTS51X_READ_REG(chip, PPBUF_BASE2, &val);
		if (val & WRT_PRTCT) {
			chip->card_wp |= MS_CARD;
		} else {
			chip->card_wp &= ~MS_CARD;
		}
	} else if ((val == 0x01) || (val == 0x02) ||(val == 0x03)) {
		chip->card_wp |= MS_CARD;
	} else {
		ms_card->check_ms_flow = 1;
		TRACE_RET(chip, STATUS_FAIL);
	}

	ms_card->ms_type |= TYPE_MSPRO;

	// Check MSPro-HG Card, use IF Mode Register to distinguish
	RTS51X_READ_REG(chip, PPBUF_BASE2 + 3, &val);
	RTS51X_DEBUGP(("IF Mode register: 0x%x\n", val));
	if (val == 0) {
		ms_card->ms_type &= 0x0F;
	} else if (val == 7) {
		if (switch_8bit_bus) {
			ms_card->ms_type |= MS_HG;
		} else {
			ms_card->ms_type &= 0x0F;
		}
	} else {
		TRACE_RET(chip, STATUS_FAIL);
	}

	// end Procedure to identify Media Type
	return STATUS_SUCCESS;
}

static int ms_confirm_cpu_startup(struct rts51x_chip *chip)
{
	int retval, i, k;
	u8 val;

	//Confirm CPU StartUp
	k = 0;
	do {
		if (monitor_card_cd(chip, MS_CARD) == CD_NOT_EXIST) {
			TRACE_RET(chip, STATUS_FAIL);
		}

		for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
			retval = ms_read_bytes(chip, GET_INT, 1, NO_WAIT_INT, &val, 1);
			if (retval == STATUS_SUCCESS) {
				break;
			}
		}
		if (i == MS_MAX_RETRY_COUNT) {
			TRACE_RET(chip, STATUS_FAIL);
		}

		if (k > 100) {
			TRACE_RET(chip, STATUS_FAIL);
		}
		k++;
		wait_timeout(100);
	} while (!(val & INT_REG_CED));

	// For MSPro Certification 2-7-9, MS-PRO Fatal error occured(CED=1 & ERR=1).
	// We shall Power-Off MS_3V3, and we can't do initializing MS too.
	for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
		retval = ms_read_bytes(chip, GET_INT, 1, NO_WAIT_INT, &val, 1);
		if (retval == STATUS_SUCCESS) {
			break;
		}
	}
	if (i == MS_MAX_RETRY_COUNT) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	if (val & INT_REG_ERR) {
		// For MSPro Certification 2-7-10, Write-Disabled Status
		if (val & INT_REG_CMDNK) {		// CMDNK = 1
			chip->card_wp |= (MS_CARD);
		} else {				// CMDNK = 0
			TRACE_RET(chip, STATUS_FAIL);
		}
	}
	//--  end confirm CPU startup

	return STATUS_SUCCESS;
}

static int ms_switch_parallel_bus(struct rts51x_chip *chip)
{
	int retval, i;
	u8 data[2];

	// For MS-Pro Certification 2-8-3, if switch 4-bit bus fail(after retry),
	// ++ set ms parallel 4-bit interface
	data[0] = PARALLEL_4BIT_IF;
	data[1] = 0;
	for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
		retval = ms_write_bytes(chip, WRITE_REG, 1, NO_WAIT_INT, data, 2);
		if (retval == STATUS_SUCCESS) {
			break;
		}
	}
	if (retval != STATUS_SUCCESS) {
		// For MS-Pro Certification 2-8-3:
		// We shall power reset MS-PRo Card if switched-bus fail.
		TRACE_RET(chip, retval);
	}

	return STATUS_SUCCESS;
}

static int ms_switch_8bit_bus(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, i;
	u8 data[2];

	data[0] = PARALLEL_8BIT_IF;
	data[1] = 0;
	for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
		retval = ms_write_bytes(chip, WRITE_REG, 1, NO_WAIT_INT, data, 2);
		if (retval == STATUS_SUCCESS) {
			break;
		}
	}
	if (retval != STATUS_SUCCESS) {
		// For MS-Pro Certification 2-8-3:
		// We shall power reset MS-PRo Card if switched-bus fail.
		TRACE_RET(chip, STATUS_FAIL);
	}

	// Switch MS-PRO into 8 bits Parallel mode
	// Because HG run as 60MHz, and we shall guarantee the tPDod 10ns,
	// So we shall set "SAMPLE_TIME_FALLING" to delay sample the output data from HG card.
	RTS51X_WRITE_REG(chip, MS_CFG, 0x98, MS_BUS_WIDTH_8 | SAMPLE_TIME_FALLING);
	ms_card->ms_type |= MS_8BIT;
	retval = ms_set_init_para(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	// To solve the issue that the socket is 4-bit only,
	// We shall try some command under 8-bit here, such as GET_INT TPC...
	for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
		// At least 1 GET_INT TPC Fail, bus not match, shall reset to run with 4-bit
		retval = ms_transfer_tpc(chip, MS_TM_READ_BYTES, GET_INT, 1, NO_WAIT_INT);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}
	}

	return STATUS_SUCCESS;
}

static int ms_pro_reset_flow(struct rts51x_chip *chip, int switch_8bit_bus)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, i;

	for (i = 0; i < 3; i++) {
		retval = ms_prepare_reset(chip);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		retval = ms_identify_media_type(chip, switch_8bit_bus);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		retval = ms_confirm_cpu_startup(chip);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		retval = ms_switch_parallel_bus(chip);
		if (retval != STATUS_SUCCESS) {
			if (monitor_card_cd(chip, MS_CARD) == CD_NOT_EXIST) {
				TRACE_RET(chip, STATUS_FAIL);
			}
			continue;
		} else {
			break;
		}
	}

	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	// Switch MS-PRO into Parallel mode
	RTS51X_WRITE_REG(chip, MS_CFG, 0x18, MS_BUS_WIDTH_4);

	// Set MS-PRO Card timing 40MHz, and set Push Timing as "ODD"
	RTS51X_WRITE_REG(chip, MS_CFG, PUSH_TIME_ODD, PUSH_TIME_ODD);

	retval = ms_set_init_para(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	// If MSPro HG Card, We shall try to switch to 8-bit bus
	if (CHK_MSHG(ms_card) && switch_8bit_bus) {
		retval = ms_switch_8bit_bus(chip);
		if (retval != STATUS_SUCCESS) {
			ms_card->switch_8bit_fail = 1;
			TRACE_RET(chip, retval);
		}
	}

	return STATUS_SUCCESS;
}

#ifdef XC_POWERCLASS
static int msxc_change_power(struct rts51x_chip *chip, u8 mode)
{
	int retval;
	u8 buf[6];

	ms_cleanup_work(chip);

	// Set Parameter Register
	retval = ms_set_rw_reg_addr(chip, 0, 0, Pro_DataCount1, 6);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	buf[0] = 0;
	buf[1] = mode;
	buf[2] = 0;
	buf[3] = 0;
	buf[4] = 0;
	buf[5] = 0;

	retval = ms_write_bytes(chip, PRO_WRITE_REG , 6, NO_WAIT_INT, buf, 6);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	// Set "CHG_POWER" Command by "SET_CMD" TPC, Maybe try with "EX_SET_CMD" TPC.
	retval = ms_send_cmd(chip, XC_CHG_POWER, WAIT_INT);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	RTS51X_READ_REG(chip, MS_TRANS_CFG, buf);
	if (buf[0] & (MS_INT_CMDNK | MS_INT_ERR)) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	return STATUS_SUCCESS;
}
#endif

static int ms_read_attribute_info(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, i;
	u8 val, *buf, class_code, device_type, sub_class, data[16];
	u16 total_blk = 0, blk_size = 0;
#ifdef SUPPORT_MSXC
	u32 xc_total_blk = 0, xc_blk_size = 0;
#endif
	u32 sys_info_addr = 0, sys_info_size;
#ifdef SUPPORT_PCGL_1P18
	u32 model_name_addr = 0, model_name_size;
	int found_sys_info = 0, found_model_name = 0;
#endif

	retval = ms_set_rw_reg_addr(chip, Pro_IntReg, 2, Pro_SystemParm, 7);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	if (CHK_MS8BIT(ms_card)) {
		data[0] = PARALLEL_8BIT_IF;
	} else {
		data[0] = PARALLEL_4BIT_IF;
	}
	data[1] = 0;

	// For MS-Pro Certification 2-7-15
	//
	// The Maximun sector count of Attribute Data is 0x40 sectors
	data[2] = 0x40;
	data[3] = 0;
	data[4] = 0;
	data[5] = 0;
	// Start address 0
	data[6] = 0;
	data[7] = 0;

	for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
		retval = ms_write_bytes(chip, PRO_WRITE_REG, 7, NO_WAIT_INT, data, 8);
		if (retval == STATUS_SUCCESS) {
			break;
		}
	}
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	buf = (u8 *)kmalloc(64 * 512, GFP_KERNEL);
	if (buf == NULL) {
		TRACE_RET(chip, STATUS_NOMEM);
	}

	for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
		retval = ms_send_cmd(chip, PRO_READ_ATRB, WAIT_INT);
		if (retval != STATUS_SUCCESS) {
			continue;
		}
		// For MS-Pro Certification 2-7-15,
		// We shall check BREQ after sending "READ_ATTRIBUTE" cmd
		retval = rts51x_read_register(chip, MS_TRANS_CFG, &val);
		if (retval != STATUS_SUCCESS) {
			kfree(buf);
			TRACE_RET(chip, STATUS_FAIL);
		}
		if (!(val & MS_INT_BREQ)) {
			kfree(buf);
			TRACE_RET(chip, STATUS_FAIL);
		}
		// For MS-PRO Certification 2-7-15, we wait INT signal before transfer completed
		retval = ms_transfer_data(chip, MS_TM_AUTO_READ, PRO_READ_LONG_DATA,
				0x40, WAIT_INT, 0, 0, buf, 64 * 512);
		if (retval == STATUS_SUCCESS) {
			break;
		} else {
			rts51x_clear_ms_error(chip);
		}
	}
	if (retval != STATUS_SUCCESS) {
		kfree(buf);
		TRACE_RET(chip, retval);
	}

	i = 0;
	// Due to many Sandisk's MS-PRO Card, they will still put BREQ to '1' after "STOP" command.
	do {
		retval = rts51x_read_register(chip, MS_TRANS_CFG, &val);
		if (retval != STATUS_SUCCESS) {
			kfree(buf);
			TRACE_RET(chip, retval);
		}

		if ((val & MS_INT_CED) || !(val & MS_INT_BREQ)) {
			break;
		}

		retval = ms_transfer_tpc(chip, MS_TM_NORMAL_READ, PRO_READ_LONG_DATA, 0, WAIT_INT);
		if (retval != STATUS_SUCCESS) {
			kfree(buf);
			TRACE_RET(chip, retval);
		}

		i++;
	} while (i < 1024);

	if (retval != STATUS_SUCCESS) {
		kfree(buf);
		TRACE_RET(chip, retval);
	}

	if ((buf[0] != 0xa5) && (buf[1] != 0xc3)) {
		//Signature code is wrong
		kfree(buf);
		TRACE_RET(chip, STATUS_FAIL);
	}

	// For MsPro Certification 2-5-2. We shall judge wheather
	// "Device Informantion Entry Count" <= 12 and >=1.
	if ((buf[4] < 1) || (buf[4] > 12)) {
		kfree(buf);
		TRACE_RET(chip, STATUS_FAIL);
	}

	for (i = 0; i < buf[4]; i ++) {
		int cur_addr_off = 16 + i * 12;

#ifdef SUPPORT_MSXC
		if ((buf[cur_addr_off + 8] == 0x10) || (buf[cur_addr_off + 8] == 0x13))
#else
		if (buf[cur_addr_off + 8] == 0x10)
#endif
		{
			// Device Information ID = 0x10 means
			// this entry is "System Information" Entry
			sys_info_addr = ((u32)buf[cur_addr_off + 0] << 24) |
				((u32)buf[cur_addr_off + 1] << 16) |
				((u32)buf[cur_addr_off + 2] << 8) | buf[cur_addr_off + 3];
			sys_info_size = ((u32)buf[cur_addr_off + 4] << 24) |
				((u32)buf[cur_addr_off + 5] << 16) |
				((u32)buf[cur_addr_off + 6] << 8) | buf[cur_addr_off + 7];
			RTS51X_DEBUGP(("sys_info_addr = 0x%x, sys_info_size = 0x%x\n",
					sys_info_addr, sys_info_size));
			if (sys_info_size != 96)  {  // Size shall equal to 96
				kfree(buf);
				TRACE_RET(chip, STATUS_FAIL);
			}
			if (sys_info_addr < 0x1A0) {
				// Address shall be equal to
				// or greater than 0x1A0
				kfree(buf);
				TRACE_RET(chip, STATUS_FAIL);
			}
			if ((sys_info_size + sys_info_addr) > 0x8000) {
				// Address + Size can't be over than 0x8000
				kfree(buf);
				TRACE_RET(chip, STATUS_FAIL);
			}

#ifdef SUPPORT_MSXC
			if (buf[cur_addr_off + 8] == 0x13) {
				ms_card->ms_type |= MS_XC;
			}
#endif
#ifdef SUPPORT_PCGL_1P18
			found_sys_info = 1;
#else
			break;
#endif
		}
#ifdef SUPPORT_PCGL_1P18
		if (buf[cur_addr_off + 8] == 0x15) {
			// Device Information ID = 0x15 means
			// this entry is "Model Name" Entry
			model_name_addr = ((u32)buf[cur_addr_off + 0] << 24) |
				((u32)buf[cur_addr_off + 1] << 16) |
				((u32)buf[cur_addr_off + 2] << 8) | buf[cur_addr_off + 3];
			model_name_size = ((u32)buf[cur_addr_off + 4] << 24) |
				((u32)buf[cur_addr_off + 5] << 16) |
				((u32)buf[cur_addr_off + 6] << 8) | buf[cur_addr_off + 7];
			RTS51X_DEBUGP(("model_name_addr = 0x%x, model_name_size = 0x%x\n",
					model_name_addr, model_name_size));
			if (model_name_size != 48)  {  // Size shall equal to 48
				kfree(buf);
				TRACE_RET(chip, STATUS_FAIL);
			}
			if (model_name_addr < 0x1A0) {
				// Address shall be equal to
				// or greater than 0x1A0
				kfree(buf);
				TRACE_RET(chip, STATUS_FAIL);
			}
			if ((model_name_size + model_name_addr) > 0x8000) {
				// Address + Size can't be over than 0x8000
				kfree(buf);
				TRACE_RET(chip, STATUS_FAIL);
			}

			found_model_name = 1;
		}

		if (found_sys_info && found_model_name) {
			break;
		}
#endif
	}

	if (i == buf[4]) {
		// Can't find System Information Entry and Model Name Entry
		kfree(buf);
		TRACE_RET(chip, STATUS_FAIL);
	}

	class_code =  buf[sys_info_addr + 0];
	device_type = buf[sys_info_addr + 56];
	sub_class = buf[sys_info_addr + 46];
#ifdef SUPPORT_MSXC
	if (CHK_MSXC(ms_card)) {
		xc_total_blk = ((u32)buf[sys_info_addr + 6] << 24) |
				((u32)buf[sys_info_addr + 7] << 16) |
				((u32)buf[sys_info_addr + 8] << 8) |
				buf[sys_info_addr + 9];
		xc_blk_size = ((u32)buf[sys_info_addr + 32] << 24) |
				((u32)buf[sys_info_addr + 33] << 16) |
				((u32)buf[sys_info_addr + 34] << 8) |
				buf[sys_info_addr + 35];
		RTS51X_DEBUGP(("xc_total_blk = 0x%x, xc_blk_size = 0x%x\n", xc_total_blk, xc_blk_size));
	} else {
		total_blk = ((u16)buf[sys_info_addr + 6] << 8) | buf[sys_info_addr + 7];
		blk_size = ((u16)buf[sys_info_addr + 2] << 8) | buf[sys_info_addr + 3];
		RTS51X_DEBUGP(("total_blk = 0x%x, blk_size = 0x%x\n", total_blk, blk_size));
	}
#else
	total_blk = ((u16)buf[sys_info_addr + 6] << 8) | buf[sys_info_addr + 7];
	blk_size = ((u16)buf[sys_info_addr + 2] << 8) | buf[sys_info_addr + 3];
	RTS51X_DEBUGP(("total_blk = 0x%x, blk_size = 0x%x\n", total_blk, blk_size));
#endif

	RTS51X_DEBUGP(("class_code = 0x%x, device_type = 0x%x, sub_class = 0x%x\n",
			class_code, device_type, sub_class));

	memcpy(ms_card->raw_sys_info, buf + sys_info_addr, 96);
	memcpy(ms_card->raw_ms_id, ms_card->raw_sys_info + 64, 16);
	RTS51X_DEBUGP(("MSPro ID:\n"));
	RTS51X_DUMP(ms_card->raw_ms_id,16);
#ifdef SUPPORT_PCGL_1P18
	memcpy(ms_card->raw_model_name, buf + model_name_addr, 48);
#endif

	kfree(buf);

	// Confirm System Information
#ifdef SUPPORT_MSXC
	if (CHK_MSXC(ms_card)) {
		if (class_code != 0x03) {
			TRACE_RET(chip, STATUS_FAIL);
		}
	} else {
		if (class_code != 0x02) {
			TRACE_RET(chip, STATUS_FAIL);
		}
	}
#else
	if (class_code != 0x02) {
		TRACE_RET(chip, STATUS_FAIL);
	}
#endif

	if (device_type != 0x00) {
		if ((device_type == 0x01) || (device_type == 0x02)
			|| (device_type == 0x03)) {
			chip->card_wp |= MS_CARD;
		} else {
			TRACE_RET(chip, STATUS_FAIL);
		}
	}

	// For MsPro Certification 2-5-3
	if (sub_class & 0xC0) {
		// MsPro enter the "Read/Write-Protect Status",
		// we return "card not exist"
		TRACE_RET(chip, STATUS_FAIL);
	}

	RTS51X_DEBUGP(("class_code: 0x%x, device_type: 0x%x, sub_class: 0x%x\n",
		class_code, device_type, sub_class));

#ifdef SUPPORT_MSXC
	if (CHK_MSXC(ms_card)) {
		chip->capacity[chip->card2lun[MS_CARD]] = ms_card->capacity = xc_total_blk * xc_blk_size;
	} else {
		chip->capacity[chip->card2lun[MS_CARD]] = ms_card->capacity = total_blk * blk_size;
	}
#else
	chip->capacity[chip->card2lun[MS_CARD]] = ms_card->capacity = total_blk * blk_size;
#endif

	return STATUS_SUCCESS;
}

#ifdef SUPPORT_MAGIC_GATE
int mg_set_tpc_para_sub(struct rts51x_chip *chip, int type, u8 mg_entry_num);
#endif

static int reset_ms_pro(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;
#ifdef XC_POWERCLASS
	u8 change_power_class = 2;  // We support power class 2 at most
#endif

#ifdef XC_POWERCLASS
Retry:
#endif
	retval = ms_pro_reset_flow(chip, 1);
	if (retval != STATUS_SUCCESS) {
		if (ms_card->switch_8bit_fail) {
			retval = ms_pro_reset_flow(chip, 0);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, retval);
			}
		} else {
			TRACE_RET(chip, retval);
		}
	}

	retval = ms_read_attribute_info(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

#ifdef XC_POWERCLASS
	// If this card is HG8Bit card, we can't change to power class 1
	if (CHK_HG8BIT(ms_card)) {
		change_power_class = 0;
	}

	if (change_power_class && CHK_MSXC(ms_card)) {
		// power_class_mode = (sub_class & 0x18) >> 3
		u8 power_class_mode = (ms_card->raw_sys_info[46] & 0x18) >> 3;
		RTS51X_DEBUGP(("power_class_mode = 0x%x", power_class_mode));
		if (change_power_class > power_class_mode) {
			change_power_class = power_class_mode;
		}
		if (change_power_class) {
			retval = msxc_change_power(chip, change_power_class);
			if (retval != STATUS_SUCCESS) {
				change_power_class --;
				goto Retry;
			}
		}
	}
#endif

#ifdef SUPPORT_MAGIC_GATE
	// Set READ/WRITE_SHORT_DATA length as 32 Bytes
	retval = mg_set_tpc_para_sub(chip, 0, 0);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}
#endif

	if (CHK_HG8BIT(ms_card)) {
		chip->card_bus_width[chip->card2lun[MS_CARD]] = 8;
	} else {
		chip->card_bus_width[chip->card2lun[MS_CARD]] = 4;
	}

	return STATUS_SUCCESS;
}


static int ms_read_status_reg(struct rts51x_chip *chip)
{
	int retval;
	u8 val[2];

	retval = ms_set_rw_reg_addr(chip, StatusReg0, 2, 0, 0);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	retval = ms_read_bytes(chip, READ_REG, 2, NO_WAIT_INT, val, 2);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	// Unable to correct error
	if (val[1] & (STS_UCDT | STS_UCEX | STS_UCFG)) {
		ms_set_err_code(chip, MS_FLASH_READ_ERROR);
		TRACE_RET(chip, STATUS_FAIL);
	}

	return STATUS_SUCCESS;
}


static int ms_check_boot_block(struct rts51x_chip *chip, u16 block_addr)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;
	u8 extra[MS_EXTRA_SIZE], data[10], val = 0;

	if (CHK_MS4BIT(ms_card)) {
		// Parallel interface
		data[0] = 0x88;
	} else {
		// Serial interface
		data[0] = 0x80;
	}
	// Block Address
	data[1] = 0;
	data[2] = (u8)(block_addr >> 8);
	data[3] = (u8)block_addr;
	// Page Number
	// Extra data access mode
	data[4] = 0x40;
	data[5] = 0;

	retval = ms_set_cmd(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6,
			    BLOCK_READ, WAIT_INT, data, 6, &val);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	if (val & INT_REG_CMDNK) {
		// Command can not be executed
		ms_set_err_code(chip, MS_CMD_NK);
		TRACE_RET(chip, STATUS_FAIL);
	}
	// Command error termination
	if (val & INT_REG_CED) {
		if (val & INT_REG_ERR) {
			retval = ms_read_status_reg(chip);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, retval);
			}
			retval = ms_set_rw_reg_addr(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, retval);
			}
		}
	}

	retval = ms_read_bytes(chip, READ_REG, MS_EXTRA_SIZE, NO_WAIT_INT, extra, MS_EXTRA_SIZE);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	if (!(extra[0] & BLOCK_OK) || (extra[1] & NOT_BOOT_BLOCK)) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	return STATUS_SUCCESS;
}

static int ms_read_extra_data(struct rts51x_chip *chip,
		u16 block_addr, u8 page_num, u8 *buf, int buf_len)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;
	u8 val = 0, data[10];

	if (CHK_MS4BIT(ms_card)) {
		// Parallel interface
		data[0] = 0x88;
	} else {
		// Serial interface
		data[0] = 0x80;
	}
	// Block Address
	data[1] = 0;
	data[2] = (u8)(block_addr >> 8);
	data[3] = (u8)block_addr;
	// Page Number
	// Extra data access mode
	data[4] = 0x40;
	data[5] = page_num;

#ifdef MS_SPEEDUP
	retval = ms_auto_set_cmd(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6,
			    BLOCK_READ, WAIT_INT, data, 6, &val);
#else
	retval = ms_set_cmd(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6,
			    BLOCK_READ, WAIT_INT, data, 6, &val);
#endif
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	if (val & INT_REG_CMDNK) {
		// Command can not be executed
		ms_set_err_code(chip, MS_CMD_NK);
		TRACE_RET(chip, STATUS_FAIL);
	}
	// Command error termination
	if (val & INT_REG_CED) {
		if (val & INT_REG_ERR) {
			retval = ms_read_status_reg(chip);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, retval);
			}
			retval = ms_set_rw_reg_addr(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, retval);
			}
		}
	}

	retval = ms_read_bytes(chip, READ_REG, MS_EXTRA_SIZE, NO_WAIT_INT, data, MS_EXTRA_SIZE);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	if (buf && buf_len) {
		if (buf_len > MS_EXTRA_SIZE) {
			buf_len = MS_EXTRA_SIZE;
		}
		memcpy(buf, data, buf_len);
	}

	return STATUS_SUCCESS;
}


static int ms_write_extra_data(struct rts51x_chip *chip,
		u16 block_addr, u8 page_num, u8 *buf, int buf_len)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, i;
	u8 val = 0, data[16];

	if (!buf || (buf_len < MS_EXTRA_SIZE)) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	// Write REG
	if (CHK_MS4BIT(ms_card)) {
		// Parallel interface
		data[0] = 0x88;
	} else {
		// Serial interface
		data[0] = 0x80;
	}
	// Block Address
	data[1] = 0;
	data[2] = (u8)(block_addr >> 8);
	data[3] = (u8)block_addr;
	// Page Number
	// Extra data access mode
	data[4] = 0x40;
	data[5] = page_num;

	for (i = 6; i < MS_EXTRA_SIZE + 6; i ++) {
		data[i] = buf[i - 6];
	}

#ifdef MS_SPEEDUP
	retval = ms_auto_set_cmd(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6 + MS_EXTRA_SIZE,
			    BLOCK_WRITE, WAIT_INT, data, 16, &val);
#else
	retval = ms_set_cmd(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6 + MS_EXTRA_SIZE,
			    BLOCK_WRITE, WAIT_INT, data, 16, &val);
#endif
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	if (val & INT_REG_CMDNK) {
		// Command can not be executed
		ms_set_err_code(chip, MS_CMD_NK);
		TRACE_RET(chip, STATUS_FAIL);
	}
	// Command error termination
	if (val & INT_REG_CED) {
		if (val & INT_REG_ERR) {
			ms_set_err_code(chip, MS_FLASH_WRITE_ERROR);
			TRACE_RET(chip, STATUS_FAIL);
		}
	}

	return STATUS_SUCCESS;
}


static int ms_read_page(struct rts51x_chip *chip, u16 block_addr, u8 page_num)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;
	u8 val = 0, data[6];

	if (CHK_MS4BIT(ms_card)) {
		// Parallel interface
		data[0] = 0x88;
	} else {
		// Serial interface
		data[0] = 0x80;
	}
	// Block Address
	data[1] = 0;
	data[2] = (u8)(block_addr >> 8);
	data[3] = (u8)block_addr;
	// Page Number
	// Single page access mode
	data[4] = 0x20;
	data[5] = page_num;

	// ms_read_page is used in reset flow,
	// so it is irrelevant with performance
// #ifdef MS_SPEEDUP
// 	retval = ms_auto_set_cmd(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6,
// 			    BLOCK_READ, WAIT_INT, data, 6, &val);
// #else
	retval = ms_set_cmd(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6,
			    BLOCK_READ, WAIT_INT, data, 6, &val);
// #endif
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	if (val & INT_REG_CMDNK) {
		// Command can not be executed
		ms_set_err_code(chip, MS_CMD_NK);
		TRACE_RET(chip, STATUS_FAIL);
	}

	if (val & INT_REG_CED) {
		if (val & INT_REG_ERR) {
			if (!(val & INT_REG_BREQ)) {
				ms_set_err_code(chip,  MS_FLASH_READ_ERROR);
				TRACE_RET(chip, STATUS_FAIL);
			}
			retval = ms_read_status_reg(chip);
			if (retval != STATUS_SUCCESS) {
				ms_set_err_code(chip,  MS_FLASH_WRITE_ERROR);
			}
		} else {
			if (!(val & INT_REG_BREQ)) {
				ms_set_err_code(chip, MS_BREQ_ERROR);
				TRACE_RET(chip, STATUS_FAIL);
			}
		}
	}

	retval = ms_transfer_tpc(chip, MS_TM_NORMAL_READ, READ_PAGE_DATA, 0, NO_WAIT_INT);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	// Uncorrectable error
	// Notice that the data of the page which has uncorrectable error is read out also,
	// just return failed
	if (ms_check_err_code(chip, MS_FLASH_WRITE_ERROR)) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	return STATUS_SUCCESS;
}


static int ms_set_bad_block(struct rts51x_chip *chip, u16 phy_blk)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;
	u8 val = 0, data[8], extra[MS_EXTRA_SIZE];

	retval = ms_read_extra_data(chip, phy_blk, 0, extra, MS_EXTRA_SIZE);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	ms_set_err_code(chip, MS_NO_ERROR);

	if (CHK_MS4BIT(ms_card)) {
		// Parallel interface
		data[0] = 0x88;
	} else {
		// Serial interface
		data[0] = 0x80;
	}
	// Block Address
	data[1] = 0;
	data[2] = (u8)(phy_blk >> 8);
	data[3] = (u8)phy_blk;
	data[4] = 0x80;
	data[5] = 0;
	data[6] = extra[0] & 0x7F;
	data[7] = 0xFF;

#ifdef MS_SPEEDUP
	retval = ms_set_cmd(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 7,
			    BLOCK_WRITE, WAIT_INT, data, 7, &val);
#else
	retval = ms_set_cmd(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 7,
			    BLOCK_WRITE, WAIT_INT, data, 7, &val);
#endif
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	if (val & INT_REG_CMDNK) {
		// Command can not be executed
		ms_set_err_code(chip, MS_CMD_NK);
		TRACE_RET(chip, STATUS_FAIL);
	}

	if (val & INT_REG_CED) {
		if (val & INT_REG_ERR) {
			ms_set_err_code(chip, MS_FLASH_WRITE_ERROR);
			TRACE_RET(chip, STATUS_FAIL);
		}
	}

	return STATUS_SUCCESS;
}


static int ms_erase_block(struct rts51x_chip *chip, u16 phy_blk)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, i = 0;
	u8 val = 0, data[6];

	retval = ms_set_rw_reg_addr(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	ms_set_err_code(chip, MS_NO_ERROR);

	if (CHK_MS4BIT(ms_card)) {
		// Parallel interface
		data[0] = 0x88;
	} else {
		// Serial interface
		data[0] = 0x80;
	}
	// Block Address
	data[1] = 0;
	data[2] = (u8)(phy_blk >> 8);
	data[3] = (u8)phy_blk;
	data[4] = 0;
	data[5] = 0;

ERASE_RTY:
#ifdef MS_SPEEDUP
	retval = ms_auto_set_cmd(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6,
			    BLOCK_ERASE, WAIT_INT, data, 6, &val);
#else
	retval = ms_set_cmd(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6,
			    BLOCK_ERASE, WAIT_INT, data, 6, &val);
#endif

	if (val & INT_REG_CMDNK) {
		if (i < 3) {
			i ++;
			goto ERASE_RTY;
		}

		// Command can not be executed
		ms_set_err_code(chip, MS_CMD_NK);
		ms_set_bad_block(chip, phy_blk);
		TRACE_RET(chip, STATUS_FAIL);
	}

	if (val & INT_REG_CED) {
		if (val & INT_REG_ERR) {
			ms_set_err_code(chip, MS_FLASH_WRITE_ERROR);
			TRACE_RET(chip, STATUS_FAIL);
		}
	}

	return STATUS_SUCCESS;
}


static void ms_set_page_status(u16 log_blk, u8 type, u8 *extra, int extra_len)
{
	if (!extra || (extra_len < MS_EXTRA_SIZE)) {
		return;
	}

	memset(extra, 0xFF, MS_EXTRA_SIZE);

	if (type == setPS_NG) {
		//set page status as 1:NG,and block status keep 1:OK
		extra[0] = 0xB8;
	} else {
		//set page status as 0:Data Error,and block status keep 1:OK
		extra[0] = 0x98;
	}

	extra[2] = (u8)(log_blk >> 8);
	extra[3] = (u8)log_blk;
}

static int ms_init_page(struct rts51x_chip *chip, u16 phy_blk, u16 log_blk, u8 start_page, u8 end_page)
{
	int retval;
	u8 extra[MS_EXTRA_SIZE], i;

	memset(extra, 0xff, MS_EXTRA_SIZE);

	extra[0] = 0xf8;	// Block, page OK, data erased
	extra[1] = 0xff;
	extra[2] = (u8)(log_blk >> 8);
	extra[3] = (u8)log_blk;

	for (i = start_page; i < end_page; i++) {
		if (monitor_card_cd(chip, MS_CARD) == CD_NOT_EXIST) {
			TRACE_RET(chip, STATUS_FAIL);
		}

		retval = ms_write_extra_data(chip, phy_blk, i, extra, MS_EXTRA_SIZE);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}
	}

	return STATUS_SUCCESS;
}

static int ms_copy_page(struct rts51x_chip *chip, u16 old_blk, u16 new_blk,
		u16 log_blk, u8 start_page, u8 end_page)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, rty_cnt, uncorrect_flag = 0;
	u8 extra[MS_EXTRA_SIZE], val, i, j, data[16];

	RTS51X_DEBUGP(("Copy page from 0x%x to 0x%x, logical block is 0x%x\n",
		old_blk, new_blk, log_blk));
	RTS51X_DEBUGP(("start_page = %d, end_page = %d\n", start_page, end_page));

	// Extra data of the last block of one segment may be failed read,
	// and it result in status register 1 keeping failure informations.
	// So when the following check status register 1 f/w will return fail.
	// To avoid this add reading extra data of new block before function
	// ms_read_status_reg which is for checking new block extra data status.
	retval = ms_read_extra_data(chip, new_blk, 0, extra, MS_EXTRA_SIZE);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	retval = ms_read_status_reg(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	RTS51X_READ_REG(chip, PPBUF_BASE2, &val);

	if (val & BUF_FULL) {
		// Clear Buffer
		retval = ms_send_cmd(chip, CLEAR_BUF, WAIT_INT);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		// GET_INT Register
		retval = ms_read_bytes(chip, GET_INT, 1, NO_WAIT_INT, &val, 1);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		if (!(val & INT_REG_CED)) {
			ms_set_err_code(chip, MS_FLASH_WRITE_ERROR);
			TRACE_RET(chip, STATUS_FAIL);
		}
	}

	for (i = start_page; i < end_page; i++) {
		if (monitor_card_cd(chip, MS_CARD) == CD_NOT_EXIST) {
			TRACE_RET(chip, STATUS_FAIL);
		}

		// for MS check procedure
		// check the page status of the source page is [1,1]:OK,
		// if not OK,means that the page is uncorrectable error
		// and the page status og targe page shall not be set [1,1]:OK
		ms_read_extra_data(chip, old_blk, i, extra, MS_EXTRA_SIZE);

		retval = ms_set_rw_reg_addr(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		// Write REG
		ms_set_err_code(chip, MS_NO_ERROR);

		if (CHK_MS4BIT(ms_card)) {
			// Parallel interface
			data[0] = 0x88;
		} else {
			// Serial interface
			data[0] = 0x80;
		}
		// Block Address
		data[1] = 0;
		data[2] = (u8)(old_blk >> 8);
		data[3] = (u8)old_blk;
		data[4] = 0x20;
		data[5] = i;

		retval = ms_write_bytes(chip, WRITE_REG , 6, NO_WAIT_INT, data, 6);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		// read page data to internal buffer by command
		retval = ms_send_cmd(chip, BLOCK_READ, WAIT_INT);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		// GET_INT Register
		ms_set_err_code(chip, MS_NO_ERROR);
		retval = ms_read_bytes(chip, GET_INT, 1, NO_WAIT_INT, &val, 1);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		if (val & INT_REG_CMDNK) {
			// Command can not be executed
			ms_set_err_code(chip, MS_CMD_NK);
			TRACE_RET(chip, STATUS_FAIL);
		}

		if (val & INT_REG_CED) {
			if (val & INT_REG_ERR) {
				retval = ms_read_status_reg(chip);
				if (retval != STATUS_SUCCESS) {
					// for MS check procedure
					// if occurs uncorrectable error,
					// after read(dump) the buffer data,don't write back
					uncorrect_flag = 1;
					RTS51X_DEBUGP(("Uncorrectable error\n"));
				} else {
					uncorrect_flag = 0;
				}

				// Read Sector Data of Source Block Page to SRAM2
				retval = ms_transfer_tpc(chip, MS_TM_NORMAL_READ, READ_PAGE_DATA, 0, NO_WAIT_INT);
				if (retval != STATUS_SUCCESS) {
					TRACE_RET(chip, retval);
				}

				// Uncorrectable data in old physical block
				// means something wrong with read/write
				// In new physical block, the data is not guaranteed,
				// but no problem with read/write
				if (uncorrect_flag) {
					//set the page status of the source page as [0.1]:NG
					ms_set_page_status(log_blk, setPS_NG, extra, MS_EXTRA_SIZE);
					if (i == 0) {
						// Set Update Status to 0 when the bad page
						// is the first page of the block as it happens
						extra[0] &= 0xEF;
					}
					ms_write_extra_data(chip, old_blk, i, extra, MS_EXTRA_SIZE);
					RTS51X_DEBUGP(("page %d : extra[0] = 0x%x\n", i, extra[0]));
					MS_SET_BAD_BLOCK_FLG(ms_card);

					//set the page status of the targe page as [0.0]:ERROR
					ms_set_page_status(log_blk, setPS_Error, extra, MS_EXTRA_SIZE);
					ms_write_extra_data(chip, new_blk, i, extra, MS_EXTRA_SIZE);
					continue;
				}

				for (rty_cnt = 0; rty_cnt < MS_MAX_RETRY_COUNT; rty_cnt ++) {
					retval = ms_transfer_tpc(chip, MS_TM_NORMAL_WRITE,
							WRITE_PAGE_DATA, 0, NO_WAIT_INT);
					if (retval == STATUS_SUCCESS) {
						break;
					}
				}
				if (rty_cnt == MS_MAX_RETRY_COUNT) {
					TRACE_RET(chip, STATUS_FAIL);
				}
			}

			if (!(val & INT_REG_BREQ)) {
				ms_set_err_code(chip, MS_BREQ_ERROR);
				TRACE_RET(chip, STATUS_FAIL);
			}
		}

		retval = ms_set_rw_reg_addr(chip, OverwriteFlag,
				MS_EXTRA_SIZE, SystemParm, (6+MS_EXTRA_SIZE));

		// Write REG
		ms_set_err_code(chip, MS_NO_ERROR);

		if (CHK_MS4BIT(ms_card)) {
			// Parallel interface
			data[0] = 0x88;
		} else {
			// Serial interface
			data[0] = 0x80;
		}
		// Block Address
		data[1] = 0;
		data[2] = (u8)(new_blk >> 8);
		data[3] = (u8)new_blk;
		data[4] = 0x20;
		data[5] = i;

		// for MS check procedure
		if ((extra[0] & 0x60) != 0x60) {
			data[6] = extra[0];
		} else {
			data[6] = 0xF8;
		}
		data[6 + 1] = 0xFF;
		data[6 + 2] = (u8)(log_blk >> 8);
		data[6 + 3] = (u8)log_blk;

		for (j = 4; j <= MS_EXTRA_SIZE; j++) {
			data[6 + j] = 0xFF;
		}

		retval = ms_write_bytes(chip, WRITE_REG, (6 + MS_EXTRA_SIZE), NO_WAIT_INT, data, 16);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		retval = ms_send_cmd(chip, BLOCK_WRITE, WAIT_INT);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		// GET_INT Register
		ms_set_err_code(chip, MS_NO_ERROR);
		retval = ms_read_bytes(chip, GET_INT, 1, NO_WAIT_INT, &val, 1);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		if (val & INT_REG_CMDNK) {
			// Command can not be executed
			ms_set_err_code(chip, MS_CMD_NK);
			TRACE_RET(chip, STATUS_FAIL);
		}

		if (val & INT_REG_CED) {
			if (val & INT_REG_ERR) {
				ms_set_err_code(chip, MS_FLASH_WRITE_ERROR);
				TRACE_RET(chip, STATUS_FAIL);
			}
		}

		if (i == 0) {
			retval = ms_set_rw_reg_addr(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 7);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, retval);
			}

			ms_set_err_code(chip, MS_NO_ERROR);

			if (CHK_MS4BIT(ms_card)) {
				// Parallel interface
				data[0] = 0x88;
			} else {
				// Serial interface
				data[0] = 0x80;
			}
			// Block Address
			data[1] = 0;
			data[2] = (u8)(old_blk >> 8);
			data[3] = (u8)old_blk;
			data[4] = 0x80;
			data[5] = 0;
			data[6] = 0xEF;
			data[7] = 0xFF;

			retval = ms_write_bytes(chip, WRITE_REG, 7, NO_WAIT_INT, data, 8);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, retval);
			}

			retval = ms_send_cmd(chip, BLOCK_WRITE, WAIT_INT);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, retval);
			}

			// GET_INT Register
			ms_set_err_code(chip, MS_NO_ERROR);
			retval = ms_read_bytes(chip, GET_INT, 1, NO_WAIT_INT, &val, 1);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, retval);
			}

			if (val & INT_REG_CMDNK) {
				// Command can not be executed
				ms_set_err_code(chip, MS_CMD_NK);
				TRACE_RET(chip, STATUS_FAIL);
			}

			if (val & INT_REG_CED) {
				if (val & INT_REG_ERR) {
					ms_set_err_code(chip, MS_FLASH_WRITE_ERROR);
					TRACE_RET(chip, STATUS_FAIL);
				}
			}
		}
	}

	return STATUS_SUCCESS;
}

#ifdef MS_SPEEDUP
static int ms_auto_copy_page(struct rts51x_chip *chip, u16 old_blk, u16 new_blk,
		u16 log_blk, u8 start_page, u8 end_page)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;
	u8 page_len, bus_width, val = 0;
	u8 extra[MS_EXTRA_SIZE];

	RTS51X_DEBUGP(("Auto copy page from 0x%x to 0x%x, logical block is 0x%x\n",
		old_blk, new_blk, log_blk));
	RTS51X_DEBUGP(("start_page = %d, end_page = %d\n", start_page, end_page));

	page_len = end_page - start_page;

	// Extra data of the last block of one segment may be failed read,
	// and it result in status register 1 keeping failure informations.
	// So when the following check status register 1 f/w will return fail.
	// To avoid this add reading extra data of new block before function
	// ms_read_status_reg which is for checking new block extra data status.
	retval = ms_read_extra_data(chip, new_blk, 0, extra, MS_EXTRA_SIZE);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	retval = ms_read_status_reg(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	RTS51X_READ_REG(chip, PPBUF_BASE2, &val);

	if (val & BUF_FULL) {
		// Clear Buffer
		retval = ms_send_cmd(chip, CLEAR_BUF, WAIT_INT);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		// GET_INT Register
		retval = ms_read_bytes(chip, GET_INT, 1, NO_WAIT_INT, &val, 1);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		if (!(val & INT_REG_CED)) {
			ms_set_err_code(chip, MS_FLASH_WRITE_ERROR);
			TRACE_RET(chip, STATUS_FAIL);
		}
	}

	if (CHK_MS4BIT(ms_card)) {
		// Parallel interface
		bus_width = 0x88;
	} else {
		// Serial interface
		bus_width = 0x80;
	}

	rts51x_init_cmd(chip);

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_OLD_BLOCK_0, 0xFF, (u8)old_blk);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_OLD_BLOCK_1, 0xFF, (u8)(old_blk >> 8));
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_NEW_BLOCK_0, 0xFF, (u8)new_blk);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_NEW_BLOCK_1, 0xFF, (u8)(new_blk >> 8));
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_LOG_BLOCK_0, 0xFF, (u8)log_blk);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_LOG_BLOCK_1, 0xFF, (u8)(log_blk >> 8));
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_PAGE_START, 0xFF, start_page);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_PAGE_LENGTH, 0xFF, page_len);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_BUS_WIDTH, 0xFF, bus_width);

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANSFER, 0xFF, MS_TRANSFER_START | MS_TM_COPY_PAGE);
	rts51x_add_cmd(chip, CHECK_REG_CMD, MS_TRANSFER, MS_TRANSFER_END, MS_TRANSFER_END);

	retval = rts51x_send_cmd(chip, MODE_CR, 100);
	if (retval != STATUS_SUCCESS) {
		rts51x_clear_ms_error(chip);
		TRACE_RET(chip, retval);
	}

	retval = rts51x_get_rsp(chip, 1, 5000);

#ifdef RCC_BUG_FIX_SP
	if((chip->option.rcc_fail_flag == 2)&&(chip->option.rcc_bug_fix_en==1))
	{//if rcc bug fix support and happen,then fix the bug
		u8 tmpvalue;
		int num = 0;

		chip->rcc_read_response = 0;
		rts51x_ep0_read_register(chip, SFSM_ED, &tmpvalue);
		if(!(tmpvalue&CARD_ERR))//if card is not error,then it is stalled by rcc bug
		{
			for(num=0;num < 5000;num++)
			{
				rts51x_ep0_read_register(chip, MS_TRANSFER, &tmpvalue);
				if(tmpvalue&MS_TRANSFER_END)
				{
					break;
				}
				wait_timeout(1);
			}
		}
		if((tmpvalue&MS_TRANSFER_ERR)||(num==5000))
		{
			retval = STATUS_FAIL;
		}
		else
		{
			*chip->rsp_buf = tmpvalue;
			retval = STATUS_SUCCESS;
		}
		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, MC_FIFO_CTL, FIFO_FLUSH, FIFO_FLUSH);
//		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, SFSM_ED, 0xf8, 0xf8);
	}
#endif

	if (CHECK_MS_TRANS_FAIL(chip, retval)) {
		rts51x_clear_ms_error(chip);
		if (retval == STATUS_TIMEDOUT) {
			TRACE_RET(chip, retval);
		}
		TRACE_GOTO(chip, Fail);
	}

	return STATUS_SUCCESS;

Fail:
	retval = ms_erase_block(chip, new_blk);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	retval = ms_copy_page(chip, old_blk, new_blk, log_blk, start_page, end_page);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	return STATUS_SUCCESS;
}
#endif

static int reset_ms(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;
	u16 i, reg_addr, block_size;
	u8 val, j, *ptr;
#ifndef SUPPORT_MAGIC_GATE
	u16 eblock_cnt;
#endif

	retval = ms_prepare_reset(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	ms_card->ms_type |= TYPE_MS;

	retval = ms_send_cmd(chip, MS_RESET, NO_WAIT_INT);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	// detect write protect status
	retval = ms_read_status_reg(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	RTS51X_READ_REG(chip, PPBUF_BASE2, &val);
	if (val & WRT_PRTCT) {
		chip->card_wp |= MS_CARD;
	} else {
		chip->card_wp &= ~MS_CARD;
	}

	i = 0;

RE_SEARCH:
	// Search For Boot Block
	while (i < (MAX_DEFECTIVE_BLOCK + 2)) {
		if (monitor_card_cd(chip, MS_CARD) == CD_NOT_EXIST) {
			TRACE_RET(chip, STATUS_FAIL);
		}

		retval = ms_check_boot_block(chip, i);
		if (retval != STATUS_SUCCESS) {
			i++;
			continue;
		}

		ms_card->boot_block = i;
		break;
	}

	if (i == (MAX_DEFECTIVE_BLOCK + 2)) {
		RTS51X_DEBUGP(("No boot block found!"));
		TRACE_RET(chip, STATUS_FAIL);
	}

	// for MS chcek Procedure TEST7009 (7-9-5)
	// Each Boot Block has 3 meaningful pages(p0, p1 ans p2),check if they are uncorrectable
	for (j = 0; j < 3; j++) {
		retval = ms_read_page(chip, ms_card->boot_block, j);
		if (retval != STATUS_SUCCESS) {
			// use MS_FLASH_WRITE_ERROR to indicate uncorrectable ECC error
			if (ms_check_err_code(chip, MS_FLASH_WRITE_ERROR)) {
				// due to g_byBootBlockNum occur uncorrectable ecc error,
				// try to find the 2nd BootBlock
				// if the 2nd BootBlock is also occur uncorrectable ecc error, return 0 to PC
				// else,we should use the data of 2nd Bootblock
				i = ms_card->boot_block + 1;
				ms_set_err_code(chip, MS_NO_ERROR);
				goto RE_SEARCH;
			}
		}
	}

	// Read boot block contents
	retval = ms_read_page(chip, ms_card->boot_block, 0);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	// Read MS system information as sys_info
	retval = rts51x_seq_read_register(chip, PPBUF_BASE2 + 0x1A0, 96, ms_card->raw_sys_info);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	memcpy(ms_card->raw_ms_id, ms_card->raw_sys_info + 0x0C, 16);
	RTS51X_DEBUGP(("MS ID:\n"));
	RTS51X_DUMP(ms_card->raw_ms_id,16);

	// Read useful block contents
	rts51x_init_cmd(chip);

	rts51x_add_cmd(chip, READ_REG_CMD, HEADER_ID0, 0, 0);
	rts51x_add_cmd(chip, READ_REG_CMD, HEADER_ID1, 0, 0);

	for (reg_addr = DISABLED_BLOCK0; reg_addr <= DISABLED_BLOCK3; reg_addr ++) {
		rts51x_add_cmd(chip, READ_REG_CMD, reg_addr, 0, 0);
	}

	for (reg_addr = BLOCK_SIZE_0; reg_addr <= PAGE_SIZE_1; reg_addr ++) {
		rts51x_add_cmd(chip, READ_REG_CMD, reg_addr, 0, 0);
	}

	rts51x_add_cmd(chip, READ_REG_CMD, MS_Device_Type, 0, 0);
	rts51x_add_cmd(chip, READ_REG_CMD, MS_4bit_Support, 0, 0);

	retval = rts51x_send_cmd(chip, MODE_CR, 100);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	retval = rts51x_get_rsp(chip, 16, 100);

#ifdef RCC_BUG_FIX_SP
	if((chip->option.rcc_fail_flag == 2)&&(chip->option.rcc_bug_fix_en==1))
	{//if rcc bug fix support and happen,then fix the bug
		u8 tmpvalue;

		chip->rcc_read_response = 0;
		rts51x_ep0_read_register(chip, SFSM_ED, &tmpvalue);
		if(!(tmpvalue&CARD_ERR))//if card is not error,then it is stalled by rcc bug
		{
			rts51x_ep0_read_register(chip,HEADER_ID0,chip->rsp_buf);
			rts51x_ep0_read_register(chip,HEADER_ID0,chip->rsp_buf+1);
			for(reg_addr=DISABLED_BLOCK0;reg_addr<=DISABLED_BLOCK3;reg_addr++)
			{
				rts51x_ep0_read_register(chip,reg_addr,chip->rsp_buf+reg_addr-DISABLED_BLOCK0+2);
			}
			for(reg_addr=BLOCK_SIZE_0;reg_addr<=PAGE_SIZE_1;reg_addr++)
			{
				rts51x_ep0_read_register(chip, reg_addr, chip->rsp_buf+reg_addr-BLOCK_SIZE_0+6);
			}
			rts51x_ep0_read_register(chip, MS_Device_Type, chip->rsp_buf+14);
			rts51x_ep0_read_register(chip, MS_4bit_Support, chip->rsp_buf+15);
			retval = STATUS_SUCCESS;
		}
		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, MC_FIFO_CTL, FIFO_FLUSH, FIFO_FLUSH);
//		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, SFSM_ED, 0xf8, 0xf8);
	}
#endif

	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	ptr = rts51x_get_rsp_data(chip);

	RTS51X_DEBUGP(("Boot block data:\n"));
	RTS51X_DUMP(ptr, 16);

	// Block ID error
	// HEADER_ID0, HEADER_ID1
	if (ptr[0] != 0x00 || ptr[1] != 0x01) {
		i = ms_card->boot_block + 1;
		goto RE_SEARCH;
	}

	// Page size error
	// PAGE_SIZE_0, PAGE_SIZE_1
	if (ptr[12] != 0x02 || ptr[13] != 0x00) {
		i = ms_card->boot_block + 1;
		goto RE_SEARCH;
	}

	// for MS check procedure,if device type is ROM1 or ROM3,treat as Read only stick
	// MS_Device_Type
	if ((ptr[14] == 1) || (ptr[14] == 3)) {
		chip->card_wp |= MS_CARD;
	}

	// BLOCK_SIZE_0, BLOCK_SIZE_1
	block_size = ((u16)ptr[6] << 8) | ptr[7];
	if (block_size == 0x0010) {
		// Block size 16KB
		ms_card->block_shift = 5;
		ms_card->page_off = 0x1F;
	} else if (block_size == 0x0008) {
		// Block size 8KB
		ms_card->block_shift = 4;
		ms_card->page_off = 0x0F;
	}

	// BLOCK_COUNT_0, BLOCK_COUNT_1
	ms_card->total_block = ((u16)ptr[8] << 8) | ptr[9];

#ifdef SUPPORT_MAGIC_GATE
	// According to PCGL, the Number of blocks of Memory Stick classic
	// series media should be fixed as follows.
	// Shink certain block size in order to match the volume capacity
	// by calculating CHS addressing and that by calculating LBA addressing,
	// CHS capacity is smaller than LBA capacity in case of 64MB
	//(32KB == 2 Erase Blocks smaller) and 128MB(96KB == 6 Erase Blocks smaller).
	j = ptr[10];  // Higher byte of effective block

	if (ms_card->block_shift == 4)  { // 4MB or 8MB
		if (j < 2)  { // Effective block for 4MB: 0x1F0
			ms_card->capacity = 0x1EE0;
		} else { // Effective block for 8MB: 0x3E0
			ms_card->capacity = 0x3DE0;
		}
	} else  { // 16MB, 32MB, 64MB or 128MB
		if (j < 5)  { // Effective block for 16MB: 0x3E0
			ms_card->capacity = 0x7BC0;
		} else if (j < 0xA) { // Effective block for 32MB: 0x7C0
			ms_card->capacity = 0xF7C0;
		} else if (j < 0x11) { // Effective block for 64MB: 0xF80
			ms_card->capacity = 0x1EF80;
		} else { // Effective block for 128MB: 0x1F00
			ms_card->capacity = 0x3DF00;
		}
	}
#else
	// EBLOCK_COUNT_0, EBLOCK_COUNT_1
	eblock_cnt = ((u16)ptr[10] << 8) | ptr[11];

	ms_card->capacity = ((u32)eblock_cnt - 2) << ms_card->block_shift;
#endif

	chip->capacity[chip->card2lun[MS_CARD]] = ms_card->capacity;

	// Switch I/F Mode
	// MS_4bit_Support
	if (ptr[15]) {
		retval = ms_set_rw_reg_addr(chip, 0, 0, SystemParm, 1);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, STATUS_FAIL);
		}

		// set Parallel IF. see spec. V1.4 TABLE5.7.10
		RTS51X_WRITE_REG(chip, PPBUF_BASE2, 0xFF, 0x88);
		RTS51X_WRITE_REG(chip, PPBUF_BASE2 + 1, 0xFF, 0);

		retval = ms_transfer_tpc(chip, MS_TM_WRITE_BYTES, WRITE_REG , 1, NO_WAIT_INT);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, STATUS_FAIL);
		}

		// No check INT if the card is 4-bits
		RTS51X_WRITE_REG(chip, MS_CFG, 0x58 | MS_NO_CHECK_INT,
				MS_BUS_WIDTH_4 | PUSH_TIME_ODD | MS_NO_CHECK_INT);

		ms_card->ms_type |= MS_4BIT;
	}

	if (CHK_MS4BIT(ms_card)) {
		chip->card_bus_width[chip->card2lun[MS_CARD]] = 4;
	} else {
		chip->card_bus_width[chip->card2lun[MS_CARD]] = 1;
	}

	return STATUS_SUCCESS;
}

static int ms_init_l2p_tbl(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int size, i, seg_no, retval;
	u16 defect_block, reg_addr;
	u8 val1, val2;

	ms_card->segment_cnt = ms_card->total_block >> 9;
	RTS51X_DEBUGP(("ms_card->segment_cnt = %d\n", ms_card->segment_cnt));

	size = ms_card->segment_cnt * sizeof(struct zone_entry);
	ms_card->segment = (struct zone_entry *)vmalloc(size);
	if (ms_card->segment == NULL) {
		TRACE_RET(chip, STATUS_FAIL);
	}
	memset(ms_card->segment, 0, size);

	retval = ms_read_page(chip, ms_card->boot_block, 1);
	if (retval != STATUS_SUCCESS) {
		TRACE_GOTO(chip, INIT_FAIL);
	}

	reg_addr = PPBUF_BASE2;
	for (i = 0; i < (((ms_card->total_block >> 9) * 10) + 1); i ++) {
		// max defect count + information block
		//
		retval = rts51x_read_register(chip, reg_addr ++, &val1);
		if (retval != STATUS_SUCCESS) {
			TRACE_GOTO(chip, INIT_FAIL);
		}
		retval = rts51x_read_register(chip, reg_addr ++, &val2);
		if (retval != STATUS_SUCCESS) {
			TRACE_GOTO(chip, INIT_FAIL);
		}

		defect_block = ((u16)val1 << 8) | val2;
		if (defect_block == 0xFFFF) {
			break;
		}
		seg_no = defect_block / 512;
		ms_card->segment[seg_no].defect_list[ms_card->segment[seg_no].disable_count ++] = defect_block;
	}


	for (i = 0; i < ms_card->segment_cnt; i++) {
		ms_card->segment[i].build_flag = 0;
		ms_card->segment[i].l2p_table = NULL;
		ms_card->segment[i].free_table = NULL;
		ms_card->segment[i].get_index = 0;
		ms_card->segment[i].set_index = 0;
		ms_card->segment[i].unused_blk_cnt = 0;

		RTS51X_DEBUGP(("defective block count of segment %d is %d\n",
					i, ms_card->segment[i].disable_count));
	}

	return STATUS_SUCCESS;

INIT_FAIL:
	if (ms_card->segment) {
		vfree(ms_card->segment);
		ms_card->segment = NULL;
	}

	return STATUS_FAIL;
}

static u16 ms_get_l2p_tbl(struct rts51x_chip *chip, int seg_no, u16 log_off)
{
	struct ms_info *ms_card = &(chip->ms_card);
	struct zone_entry *segment;

	if (ms_card->segment == NULL) {
		return 0xFFFF;
	}

	segment = &(ms_card->segment[seg_no]);

	if (segment->l2p_table) {
		return segment->l2p_table[log_off];
	}

	return 0xFFFF;
}

static void ms_set_l2p_tbl(struct rts51x_chip *chip, int seg_no, u16 log_off, u16 phy_blk)
{
	struct ms_info *ms_card = &(chip->ms_card);
	struct zone_entry *segment;

	if (ms_card->segment == NULL) {
		return;
	}

	segment = &(ms_card->segment[seg_no]);
	if (segment->l2p_table) {
		segment->l2p_table[log_off] = phy_blk;
	}
}

static void ms_set_unused_block(struct rts51x_chip *chip, u16 phy_blk)
{
	struct ms_info *ms_card = &(chip->ms_card);
	struct zone_entry *segment;
	int seg_no;

	seg_no = (int)phy_blk >> 9;
	segment = &(ms_card->segment[seg_no]);

	segment->free_table[segment->set_index ++] = phy_blk;
	if (segment->set_index >= MS_FREE_TABLE_CNT) {
		segment->set_index = 0;
	}
	segment->unused_blk_cnt ++;
}

static u16 ms_get_unused_block(struct rts51x_chip *chip, int seg_no)
{
	struct ms_info *ms_card = &(chip->ms_card);
	struct zone_entry *segment;
	u16 phy_blk;

	segment = &(ms_card->segment[seg_no]);

	if (segment->unused_blk_cnt <= 0) {
		return 0xFFFF;
	}

	phy_blk = segment->free_table[segment->get_index];
	segment->free_table[segment->get_index++] = 0xFFFF;
	if (segment->get_index >= MS_FREE_TABLE_CNT) {
		segment->get_index = 0;
	}
	segment->unused_blk_cnt --;

	return phy_blk;
}

static const unsigned short ms_start_idx[] = {0, 494, 990, 1486, 1982, 2478, 2974, 3470,
	3966, 4462, 4958, 5454, 5950, 6446, 6942, 7438, 7934};

static int ms_arbitrate_l2p(struct rts51x_chip *chip, u16 phy_blk, u16 log_off, u8 us1, u8 us2)
{
	struct ms_info *ms_card = &(chip->ms_card);
	struct zone_entry *segment;
	int seg_no;
	u16 tmp_blk;

	seg_no = (int)phy_blk >> 9;
	segment = &(ms_card->segment[seg_no]);
	tmp_blk = segment->l2p_table[log_off];

	if (us1 != us2) {
		if (us1 == 0) {
			if (!(chip->card_wp & MS_CARD)) {
				ms_erase_block(chip, tmp_blk);
			}
			ms_set_unused_block(chip, tmp_blk);
			segment->l2p_table[log_off] = phy_blk;
		} else {
			if (!(chip->card_wp & MS_CARD)) {
				ms_erase_block(chip, phy_blk);
			}
			ms_set_unused_block(chip, phy_blk);
		}
	} else {
		if (phy_blk < tmp_blk) {
			if (!(chip->card_wp & MS_CARD)) {
				ms_erase_block(chip, phy_blk);
			}
			ms_set_unused_block(chip, phy_blk);
		} else {
			if (!(chip->card_wp & MS_CARD)) {
				ms_erase_block(chip, tmp_blk);
			}
			ms_set_unused_block(chip, tmp_blk);
			segment->l2p_table[log_off] = phy_blk;
		}
	}

	return STATUS_SUCCESS;
}

static int ms_build_l2p_tbl(struct rts51x_chip *chip, int seg_no)
{
	struct ms_info *ms_card = &(chip->ms_card);
	struct zone_entry *segment;
	int retval, table_size, disable_cnt, defect_flag, i;
	u16 start, end, phy_blk, log_blk, tmp_blk;
	u8 extra[MS_EXTRA_SIZE], us1, us2;

	RTS51X_DEBUGP(("ms_build_l2p_tbl: %d\n", seg_no));

	if (ms_card->segment == NULL) {
		retval = ms_init_l2p_tbl(chip);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}
	}

	if (ms_card->segment[seg_no].build_flag) {
		RTS51X_DEBUGP(("l2p table of segment %d has been built\n", seg_no));
		return STATUS_SUCCESS;
	}

	if (seg_no == 0) {
		table_size = 494;
	} else {
		table_size = 496;
	}

	segment = &(ms_card->segment[seg_no]);

	if (segment->l2p_table == NULL) {
		segment->l2p_table = (u16 *)vmalloc(table_size * 2);
		if (segment->l2p_table == NULL) {
			TRACE_GOTO(chip, BUILD_FAIL);
		}
	}
	memset((u8 *)(segment->l2p_table), 0xff, table_size * 2);

	if (segment->free_table == NULL) {
		segment->free_table = (u16 *)vmalloc(MS_FREE_TABLE_CNT * 2);
		if (segment->free_table == NULL) {
			TRACE_GOTO(chip, BUILD_FAIL);
		}
	}
	memset((u8 *)(segment->free_table), 0xff, MS_FREE_TABLE_CNT * 2);

	start = (u16)seg_no << 9;
	end = (u16)(seg_no + 1) << 9;

	disable_cnt = segment->disable_count;

	segment->get_index = segment->set_index = 0;
	segment->unused_blk_cnt = 0;

	for (phy_blk = start; phy_blk < end; phy_blk++) {
		if (disable_cnt) {
			defect_flag = 0;
			for (i = 0; i < segment->disable_count; i++) {
				if (phy_blk == segment->defect_list[i]) {
					defect_flag = 1;
					break;
				}
			}
			if (defect_flag) {
				disable_cnt --;
				continue;
			}
		}

// 		RTS51X_DEBUGP(("==> phy_blk = 0x%x\n", phy_blk));

		retval = ms_read_extra_data(chip, phy_blk, 0, extra, MS_EXTRA_SIZE);
		if (retval != STATUS_SUCCESS) {
			RTS51X_DEBUGP(("read extra data fail\n"));
			ms_set_bad_block(chip, phy_blk);
			continue;
		}

// 		RTS51X_DEBUGP(("extra[0] = 0x%x\n", extra[0]));
// 		RTS51X_DEBUGP(("extra[1] = 0x%x\n", extra[1]));

		if (seg_no == ms_card->segment_cnt - 1) {
			if (!(extra[1] & NOT_TRANSLATION_TABLE)) {
				if (!(chip->card_wp & MS_CARD)) {
					retval = ms_erase_block(chip, phy_blk);
					if (retval != STATUS_SUCCESS) {
						// This block is a bad block,
						// we continue to the next
						continue;
					}
					// make sure that this block
					// would be put to free table
					extra[2] = 0xff;
					extra[3] = 0xff;
				}
			}
		}

		if (!(extra[0] & BLOCK_OK)) {
			continue;
		}
		if (!(extra[1] & NOT_BOOT_BLOCK)) {
			continue;
		}
		if ((extra[0] & PAGE_OK) != PAGE_OK) {
			continue;
		}

		log_blk = ((u16)extra[2] << 8) | extra[3];

// 		RTS51X_DEBUGP(("log_blk = 0x%x\n", log_blk));

		if (log_blk == 0xFFFF) {
			if (!(chip->card_wp & MS_CARD)) {
				retval = ms_erase_block(chip, phy_blk);
				if (retval != STATUS_SUCCESS) {
					continue;
				}
			}
			ms_set_unused_block(chip, phy_blk);
			continue;
		}

		if ((log_blk < ms_start_idx[seg_no]) ||
				(log_blk >= ms_start_idx[seg_no+1])) {
			if (!(chip->card_wp & MS_CARD)) {
				retval = ms_erase_block(chip, phy_blk);
				if (retval != STATUS_SUCCESS) {
					continue;
				}
			}
			ms_set_unused_block(chip, phy_blk);
			continue;
		}

		if (segment->l2p_table[log_blk - ms_start_idx[seg_no]] == 0xFFFF) {
			segment->l2p_table[log_blk - ms_start_idx[seg_no]] = phy_blk;
			continue;
		}

		us1 = extra[0] & 0x10;
		tmp_blk = segment->l2p_table[log_blk - ms_start_idx[seg_no]];
		retval = ms_read_extra_data(chip, tmp_blk, 0, extra, MS_EXTRA_SIZE);
		if (retval != STATUS_SUCCESS) {
			continue;
		}
		us2 = extra[0] & 0x10;

		(void)ms_arbitrate_l2p(chip, phy_blk, log_blk-ms_start_idx[seg_no], us1, us2);
		continue;
	}

	segment->build_flag = 1;

	RTS51X_DEBUGP(("unused block count: %d\n", segment->unused_blk_cnt));

	// Logical Address Confirmation Process
	if (seg_no == ms_card->segment_cnt - 1) {
		if (segment->unused_blk_cnt < 2) {
			chip->card_wp |= MS_CARD;
		}
	} else {
		if (segment->unused_blk_cnt < 1) {
			chip->card_wp |= MS_CARD;
		}
	}

	if (chip->card_wp & MS_CARD) {
		return STATUS_SUCCESS;
	}

	for (log_blk = ms_start_idx[seg_no]; log_blk < ms_start_idx[seg_no + 1]; log_blk ++) {
		if (segment->l2p_table[log_blk-ms_start_idx[seg_no]] == 0xFFFF) {
			phy_blk = ms_get_unused_block(chip, seg_no);
			if (phy_blk == 0xFFFF) {
				chip->card_wp |= MS_CARD;
				return STATUS_SUCCESS;
			}
			retval = ms_init_page(chip, phy_blk, log_blk, 0, 1);
			if (retval != STATUS_SUCCESS) {
				TRACE_GOTO(chip, BUILD_FAIL);
			}
			segment->l2p_table[log_blk-ms_start_idx[seg_no]] = phy_blk;
			if (seg_no == ms_card->segment_cnt - 1) {
				if (segment->unused_blk_cnt < 2) {
					chip->card_wp |= MS_CARD;
					return STATUS_SUCCESS;
				}
			} else {
				if (segment->unused_blk_cnt < 1) {
					chip->card_wp |= MS_CARD;
					return STATUS_SUCCESS;
				}
			}
		}
	}

	// Make boot block be the first normal block
	if (seg_no == 0) {
		for (log_blk = 0; log_blk < 494; log_blk ++) {
			tmp_blk = segment->l2p_table[log_blk];
			if (tmp_blk < ms_card->boot_block) {
				RTS51X_DEBUGP(("Boot block is not the first normal block.\n"));

				if (chip->card_wp & MS_CARD) {
					break;
				}

				phy_blk = ms_get_unused_block(chip, 0);
#ifdef MS_SPEEDUP
				retval = ms_auto_copy_page(chip, tmp_blk, phy_blk,
						log_blk, 0, ms_card->page_off + 1);
#else
				retval = ms_copy_page(chip, tmp_blk, phy_blk,
						log_blk, 0, ms_card->page_off + 1);
#endif
				if (retval != STATUS_SUCCESS) {
					TRACE_RET(chip, retval);
				}

				segment->l2p_table[log_blk] = phy_blk;

				retval = ms_set_bad_block(chip, tmp_blk);
				if (retval != STATUS_SUCCESS) {
					TRACE_RET(chip, retval);
				}
			}
		}
	}

// 	RTS51X_DEBUGP(("L2P table:\n"));
// 	for (i = 0; i < table_size; i++) {
// 		RTS51X_DEBUGP(("%03x : %03x\n", i, segment->l2p_table[i]));
// 	}

	return STATUS_SUCCESS;

BUILD_FAIL:
	segment->build_flag = 0;
	if (segment->l2p_table) {
		vfree(segment->l2p_table);
		segment->l2p_table = NULL;
	}
	if (segment->free_table) {
		vfree(segment->free_table);
		segment->free_table = NULL;
	}

	return STATUS_FAIL;
}


int reset_ms_card(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;

	memset(ms_card, 0, sizeof(struct ms_info));

	enable_card_clock(chip, MS_CARD);

	retval = rts51x_select_card(chip, MS_CARD);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	ms_card->ms_type = 0;
	ms_card->last_rw_int = 0;

	retval = reset_ms_pro(chip);
	if (retval != STATUS_SUCCESS) {
		if (ms_card->check_ms_flow) {
			retval = reset_ms(chip);
			if (retval != STATUS_SUCCESS) {
				if(chip->option.reset_or_rw_fail_set_pad_drive)
				{//sangdy2010-07-26:if reset or rw fail,then set SD20 PAD drive to 8mA.
					rts51x_write_register(chip, CARD_DRIVE_SEL, SD20_DRIVE_MASK, DRIVE_8mA);
				}
				TRACE_RET(chip, retval);
			}
		} else {
			if(chip->option.reset_or_rw_fail_set_pad_drive)
			{//sangdy2010-07-26:if reset or rw fail,then set SD20 PAD drive to 8mA.
				rts51x_write_register(chip, CARD_DRIVE_SEL, SD20_DRIVE_MASK, DRIVE_8mA);
			}
			TRACE_RET(chip, retval);
		}
	}

	retval = ms_set_init_para(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	if (!CHK_MSPRO(ms_card)) {
		// Build table for the last segment,
		// to check if L2P talbe block exist,erasing it
		retval = ms_build_l2p_tbl(chip, ms_card->total_block / 512 - 1);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}
	}

	RTS51X_DEBUGP(("ms_card->ms_type = 0x%x\n", ms_card->ms_type));

	return STATUS_SUCCESS;
}

static int mspro_set_rw_cmd(struct rts51x_chip *chip, u32 start_sec, u16 sec_cnt, u8 cmd)
{
	int retval, i;
	u8 data[8];

	data[0] = cmd;
	data[1] = (u8)(sec_cnt >> 8);
	data[2] = (u8)sec_cnt;
	data[3] = (u8)(start_sec >> 24);
	data[4] = (u8)(start_sec >> 16);
	data[5] = (u8)(start_sec >> 8);
	data[6] = (u8)start_sec;
	data[7] = 0;

	for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
		retval = ms_write_bytes(chip, PRO_EX_SET_CMD, 7, WAIT_INT, data, 8);
		if (retval == STATUS_SUCCESS) {
			break;
		}
	}
	if (i == MS_MAX_RETRY_COUNT) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	return STATUS_SUCCESS;
}


void mspro_stop_seq_mode(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;

	if (ms_card->seq_mode) {
		retval = ms_switch_clock(chip);
		if (retval != STATUS_SUCCESS) {
			return;
		}

		ms_card->seq_mode = 0;
		ms_card->total_sec_cnt = 0;
		ms_card->last_rw_int = 0;
		ms_send_cmd(chip, PRO_STOP, WAIT_INT);

		rts51x_ep0_write_register(chip, MC_FIFO_CTL, FIFO_FLUSH, FIFO_FLUSH);
	}
}

static inline int ms_auto_tune_clock(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;

	if (chip->asic_code) {
		if (ms_card->ms_clock > 30) {
			ms_card->ms_clock -= 20;
		}
	} else {
		if (ms_card->ms_clock == CLK_80) {
			ms_card->ms_clock = CLK_60;
		} else if (ms_card->ms_clock == CLK_60) {
			ms_card->ms_clock = CLK_40;
		}
	}

	retval = ms_switch_clock(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	return STATUS_SUCCESS;
}

static int mspro_rw_multi_sector(struct scsi_cmnd *srb, struct rts51x_chip *chip, u32 start_sector, u16 sector_cnt)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, mode_2k = 0;
	u16 count;
	u8 val, trans_mode, rw_tpc, rw_cmd;

	ms_set_err_code(chip, MS_NO_ERROR);

	ms_card->counter = 0;

	if (CHK_MSHG(ms_card)) {
		if ((start_sector % 4) || (sector_cnt % 4)) {
			if (srb->sc_data_direction == DMA_FROM_DEVICE) {
				rw_tpc = PRO_READ_LONG_DATA;
				rw_cmd = PRO_READ_DATA;
			} else {
				rw_tpc = PRO_WRITE_LONG_DATA;
				rw_cmd = PRO_WRITE_DATA;
			}
		} else {
			if (srb->sc_data_direction == DMA_FROM_DEVICE) {
				rw_tpc = PRO_READ_QUAD_DATA;
				rw_cmd = PRO_READ_2K_DATA;
			} else {
				rw_tpc = PRO_WRITE_QUAD_DATA;
				rw_cmd = PRO_WRITE_2K_DATA;
			}
			mode_2k = 1;
		}
	} else {
		if (srb->sc_data_direction == DMA_FROM_DEVICE) {
			rw_tpc = PRO_READ_LONG_DATA;
			rw_cmd = PRO_READ_DATA;
		} else {
			rw_tpc = PRO_WRITE_LONG_DATA;
			rw_cmd = PRO_WRITE_DATA;
		}
	}

	retval = ms_switch_clock(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	if (srb->sc_data_direction == DMA_FROM_DEVICE) {
		trans_mode = MS_TM_AUTO_READ;
	} else {
		trans_mode = MS_TM_AUTO_WRITE;
	}

// 	RTS51X_READ_REG(chip, MS_TRANS_CFG, &val);
	val = ms_card->last_rw_int;

	if (ms_card->seq_mode) {
		if ((ms_card->pre_dir != srb->sc_data_direction)
				|| ((ms_card->pre_sec_addr + ms_card->pre_sec_cnt) != start_sector)
				|| (mode_2k && (ms_card->seq_mode & MODE_512_SEQ))
				|| (!mode_2k && (ms_card->seq_mode & MODE_2K_SEQ))
				|| !(val & MS_INT_BREQ)
				|| ((ms_card->total_sec_cnt + sector_cnt) > 0xFE00)) {
			ms_card->seq_mode = 0;
			ms_card->total_sec_cnt = 0;
			ms_card->last_rw_int = 0;
			if (val & MS_INT_BREQ) {
				retval = ms_send_cmd(chip, PRO_STOP, WAIT_INT);
				if (retval != STATUS_SUCCESS) {
					TRACE_RET(chip, retval);
				}

				rts51x_ep0_write_register(chip, MC_FIFO_CTL, FIFO_FLUSH, FIFO_FLUSH);
			}
		}
	}

	if (!ms_card->seq_mode) {
		ms_card->total_sec_cnt = 0;
		if (sector_cnt >= 0x80) {
			if ((ms_card->capacity - start_sector) > 0xFE00) {
				count = 0xFE00;
			} else {
				count = (u16)(ms_card->capacity - start_sector);
			}
			if (count > sector_cnt) {
				if (mode_2k) {
					ms_card->seq_mode |= MODE_2K_SEQ;
				} else {
					ms_card->seq_mode |= MODE_512_SEQ;
				}
			}
		} else {
			count = sector_cnt;
		}
		retval = mspro_set_rw_cmd(chip, start_sector, count, rw_cmd);
		if (retval != STATUS_SUCCESS) {
			ms_card->seq_mode = 0;
			TRACE_RET(chip, retval);
		}
	}

	retval = ms_transfer_data(chip, trans_mode, rw_tpc, sector_cnt, WAIT_INT, mode_2k,
			scsi_sg_count(srb), scsi_sglist(srb), scsi_bufflen(srb));
	if (retval != STATUS_SUCCESS) {
		ms_card->seq_mode = 0;
		rts51x_ep0_read_register(chip, MS_TRANS_CFG, &val);
		rts51x_clear_ms_error(chip);
		if (val & MS_INT_BREQ) {
			ms_send_cmd(chip, PRO_STOP, WAIT_INT);
		}
		if (val & (MS_CRC16_ERR | MS_RDY_TIMEOUT)) {
			RTS51X_DEBUGP(("MSPro CRC error, tune clock!\n"));
			ms_auto_tune_clock(chip);
		}

		TRACE_RET(chip, retval);
	}

	ms_card->pre_sec_addr = start_sector;
	ms_card->pre_sec_cnt = sector_cnt;
	ms_card->pre_dir = srb->sc_data_direction;
	ms_card->total_sec_cnt += sector_cnt;

	return STATUS_SUCCESS;
}

static int mspro_read_format_progress(struct rts51x_chip *chip, const int short_data_len)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, i;
	u32 total_progress, cur_progress;
	u8 cnt, tmp;
	u8 data[8];

	ms_card->format_status = FORMAT_FAIL;

	retval = ms_switch_clock(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	RTS51X_READ_REG(chip, MS_TRANS_CFG, &tmp);

	//sangdy2010-07-07:for sandisk ProHG extremeIII 8GB card,it may drive INT_CED high while don't
	//drive INT_BREQ low.Then if check INT_BREQ high and read progress status,the reading will be fail.
	//So check INT_CED first.And if INT_CED is set, clear pro_under_formatting.
	if ((tmp &  (MS_INT_CED | MS_INT_CMDNK | MS_INT_ERR)) == MS_INT_CED) {
		ms_card->format_status = FORMAT_SUCCESS;
		ms_card->pro_under_formatting = 0;
		return STATUS_SUCCESS;
	}
	if (!((tmp & (MS_INT_BREQ|MS_INT_CED|MS_INT_CMDNK|MS_INT_ERR))==MS_INT_BREQ)) {
		ms_card->pro_under_formatting =0;
		TRACE_RET(chip, STATUS_FAIL);
	}

	if (short_data_len >= 256) {
		cnt = 0;
	} else {
		cnt = (u8)short_data_len;
	}

	retval = ms_read_bytes(chip, PRO_READ_SHORT_DATA, cnt, WAIT_INT, data, 8);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	total_progress = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
	cur_progress = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];

	RTS51X_DEBUGP(("total_progress = %d, cur_progress = %d\n",
				total_progress, cur_progress));

	if (total_progress == 0) {
		ms_card->progress = 0;
	} else {
		u64 ulltmp = (u64)cur_progress * (u64)65535;
		do_div(ulltmp, total_progress);
		ms_card->progress = (u16)ulltmp;
	}
	RTS51X_DEBUGP(("progress = %d\n", ms_card->progress));

	// H/W READ_BYTES mode can't support "WAIT_INT" function, so we should polling to wait INT here
	for (i = 0; i < 2500; i++) {
		RTS51X_READ_REG(chip, MS_TRANS_CFG, &tmp);
		if (tmp & (MS_INT_CED | MS_INT_CMDNK | MS_INT_BREQ |MS_INT_ERR)) {
			break;
		}

		wait_timeout(1);
	}

	if (i == 2500) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	RTS51X_DEBUGP(("MSPro format tmp:%d\n",tmp));

	if (tmp & (MS_INT_CMDNK | MS_INT_ERR)) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	//sangdy2010-07-07:for sandisk ProHG extremeIII 8GB card,it may drive INT_CED high while don't
	//drive INT_BREQ low.Then if check INT_BREQ high and read progress status,the reading will be fail.
	//So check INT_CED first.And if INT_CED is set, clear pro_under_formatting.
	if (tmp & MS_INT_CED) {
		ms_card->format_status = FORMAT_SUCCESS;
		ms_card->pro_under_formatting = 0;
	} else if (tmp & MS_INT_BREQ) {
		ms_card->format_status = FORMAT_IN_PROGRESS;
	} else {
		ms_card->format_status = FORMAT_FAIL;
		ms_card->pro_under_formatting = 0;
		TRACE_RET(chip, STATUS_FAIL);
	}

	RTS51X_DEBUGP(("MSPro format format_status:%d\n",ms_card->format_status));

	return STATUS_SUCCESS;
}

void mspro_polling_format_status(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int i;

//	if (ms_card->pro_under_formatting  && (rts51x_get_stat(chip) != RTSX_STAT_SS)) {
	if (ms_card->pro_under_formatting) {
// 		rts51x_set_stat(chip, RTSX_STAT_RUN);

		// We found that the format progress is determined by polling rate,
		for (i = 0; i < 65535; i++) {
			mspro_read_format_progress(chip, MS_SHORT_DATA_LEN);
			if (ms_card->format_status != FORMAT_IN_PROGRESS) {
				break;
			}
		}
	}

	return;
}

void mspro_format_sense(struct rts51x_chip *chip, unsigned int lun)
{
	struct ms_info *ms_card = &(chip->ms_card);

	if (CHK_FORMAT_STATUS(ms_card, FORMAT_SUCCESS)) {
		set_sense_type(chip, lun, SENSE_TYPE_NO_SENSE);
		ms_card->pro_under_formatting = 0;
		ms_card->progress = 0;
	} else if (CHK_FORMAT_STATUS(ms_card, FORMAT_IN_PROGRESS)) {
		// Logical Unit Not Ready Format in Progress
		set_sense_data(chip, lun, CUR_ERR, 0x02, 0, 0x04, 0x04,
				0, (u16)(ms_card->progress));
	} else {
		// Format Command Failed
		set_sense_type(chip, lun, SENSE_TYPE_FORMAT_CMD_FAILED);
		ms_card->pro_under_formatting = 0;
		ms_card->progress = 0;
	}
}

int mspro_format(struct scsi_cmnd *srb, struct rts51x_chip *chip, int short_data_len, int quick_format)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, i;
	u8 buf[8], tmp;
	u16 para;

	retval = ms_switch_clock(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	retval = ms_set_rw_reg_addr(chip, 0x00, 0x00, Pro_TPCParm, 0x01);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	memset(buf, 0, 2);
	switch (short_data_len) {
		case 32:
			buf[0] = 0;
			break;
		case 64:
			buf[0] = 1;
			break;
		case 128:
			buf[0] = 2;
			break;
		case 256:
		default:
			buf[0] = 3;
			break;
	}

	for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
		retval = ms_write_bytes(chip, PRO_WRITE_REG, 1, NO_WAIT_INT, buf, 2);
		if (retval == STATUS_SUCCESS) {
			break;
		}
	}
	if (i == MS_MAX_RETRY_COUNT) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	// Format command
	if (quick_format) {
		para = 0x0000;
	} else {
		para = 0x0001;
	}
	retval = mspro_set_rw_cmd(chip, 0, para, PRO_FORMAT);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	// Check INT
	RTS51X_READ_REG(chip, MS_TRANS_CFG, &tmp);
	if (tmp & (MS_INT_CMDNK | MS_INT_ERR)) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	if ((tmp & (MS_INT_BREQ | MS_INT_CED)) == MS_INT_BREQ) {
		ms_card->pro_under_formatting = 1;
		ms_card->progress = 0;
		ms_card->format_status = FORMAT_IN_PROGRESS;
		return STATUS_SUCCESS;
	}

	if (tmp & MS_INT_CED) {
		ms_card->pro_under_formatting = 0;
		ms_card->progress = 0;
		ms_card->format_status = FORMAT_SUCCESS;
		set_sense_type(chip, SCSI_LUN(srb), SENSE_TYPE_NO_SENSE);
		return STATUS_SUCCESS;
	}

	TRACE_RET(chip, STATUS_FAIL);
}


#ifdef MS_SPEEDUP
static int ms_read_multiple_pages(struct rts51x_chip *chip, u16 phy_blk, u16 log_blk,
		u8 start_page, u8 end_page, u8 *buf, void **ptr, unsigned int *offset)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;
	int send_blkend;
	u8 extra[MS_EXTRA_SIZE], val1, val2, data[6];
	u8 page_cnt = end_page - start_page, page_addr, sec_cnt;

	if (end_page != (ms_card->page_off + 1)) {
		send_blkend = 1;
	} else {
		send_blkend = 0;
	}

	// for Digital Read Protect bit issue
	retval = ms_read_extra_data(chip, phy_blk, start_page, extra, MS_EXTRA_SIZE);
	if (retval == STATUS_SUCCESS) {
		// if COPY bit & Access bit not set [1,1],then read protect
		if ((extra[1] & 0x30) != 0x30) {
			ms_set_err_code(chip, MS_FLASH_READ_ERROR);
			TRACE_RET(chip, STATUS_FAIL);
		}
	}

	if (CHK_MS4BIT(ms_card)) {
		// Parallel interface
		data[0] = 0x88;
	} else {
		// Serial interface
		data[0] = 0x80;
	}
	// Block Address
	data[1] = 0;
	data[2] = (u8)(phy_blk >> 8);
	data[3] = (u8)phy_blk;
	// Page Number
	// Extra data access mode
	data[4] = 0;
	data[5] = start_page;

	retval = ms_auto_set_cmd(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6,
			    BLOCK_READ, WAIT_INT, data, 6, &val1);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	rts51x_init_cmd(chip);

	if (send_blkend) {
		rts51x_add_cmd(chip, WRITE_REG_CMD, MS_BLKEND, SET_BLKEND, SET_BLKEND);
	} else {
		rts51x_add_cmd(chip, WRITE_REG_CMD, MS_BLKEND, SET_BLKEND, 0);
	}
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANS_CFG, WAIT_INT, NO_WAIT_INT);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_SECTOR_CNT_L, 0xFF, (u8)page_cnt);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_SECTOR_CNT_H, 0xFF, 0);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TPC, 0xFF, READ_PAGE_DATA);

	trans_dma_enable(DMA_FROM_DEVICE, chip, 512 * page_cnt, DMA_512);

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANSFER, 0xFF, MS_TRANSFER_START |  MS_TM_MULTI_READ);
	rts51x_add_cmd(chip, CHECK_REG_CMD, MS_TRANSFER, MS_TRANSFER_END, MS_TRANSFER_END);

	retval = rts51x_send_cmd(chip, MODE_CDIR | STAGE_MS_STATUS, 100);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	retval = rts51x_transfer_data_partial(chip, RCV_BULK_PIPE(chip), (void *)buf,
		ptr, offset, 512 * page_cnt, scsi_sg_count(chip->srb), NULL, 2000);
	if (retval != STATUS_SUCCESS) {
		rts51x_clear_ms_error(chip);
		if (retval == STATUS_TIMEDOUT) {
			TRACE_RET(chip, retval);
		}
		TRACE_GOTO(chip, Fail);
	}

#ifdef RCC_BUG_FIX_SP
	if(chip->rcc_read_response)//for read data stall
	{
		u8 tmpvalue;
		u16 num;

		chip->rcc_read_response = 0;
//		rts51x_ep0_write_register(chip, SFSM_ED, 0xf8, 0xf8);
		for(num=0;num < 600;num++)
		{
			rts51x_ep0_read_register(chip, MS_TRANSFER, &tmpvalue);
			if(tmpvalue&MS_TRANSFER_END)
			{
				break;
			}
			wait_timeout(1);
		}

		if((tmpvalue&MS_TRANSFER_ERR)||(num==600))
		{
			rts51x_clear_ms_error(chip);
			if(num==600)
			{
				TRACE_RET(chip,STATUS_TIMEDOUT);
			}
			TRACE_GOTO(chip, Fail);
		}
	}
	else
	{
		retval = rts51x_get_rsp(chip, 3,600);

		if((chip->option.rcc_fail_flag == 2)&&(chip->option.rcc_bug_fix_en==1))
		{//if rcc bug fix support and happen,then fix the bug
			u8 tmpvalue;
			u16 num = 0;

			rts51x_reset_pipe(chip,0);//send clear feature to clear stall
			rts51x_ep0_write_register(chip, MC_FIFO_CTL, FIFO_FLUSH, FIFO_FLUSH);
//			rts51x_reset_pipe(chip,0);//send clear feature to clear stall
			rts51x_ep0_write_register(chip, SFSM_ED, 0xf8, 0xf8);
			for(num=0;num < 600;num++)
			{
				rts51x_ep0_read_register(chip, MS_TRANSFER, &tmpvalue);
				if(tmpvalue&MS_TRANSFER_END)
				{
					break;
				}
				wait_timeout(1);
			}

			if((tmpvalue&MS_TRANSFER_ERR)||(num==600))
			{
				rts51x_clear_ms_error(chip);
				if(num==600)
				{
					TRACE_RET(chip,STATUS_TIMEDOUT);
				}
				TRACE_GOTO(chip, Fail);
			}
			*chip->rsp_buf = tmpvalue;
			rts51x_ep0_read_register(chip, MS_TRANS_CFG, chip->rsp_buf+1);
			rts51x_ep0_read_register(chip, MS_INT_REG, chip->rsp_buf+2);
			retval = STATUS_SUCCESS;
		}

		if (CHECK_MS_TRANS_FAIL(chip, retval)) {
			rts51x_clear_ms_error(chip);
			if(retval==STATUS_TIMEDOUT)
			{
				TRACE_RET(chip,STATUS_TIMEDOUT);
			}
			TRACE_GOTO(chip, Fail);
		}
	}
#else
	retval = rts51x_get_rsp(chip, 3, 200);
	if (CHECK_MS_TRANS_FAIL(chip, retval)) {
		rts51x_clear_ms_error(chip);
		if (retval == STATUS_TIMEDOUT) {
			TRACE_RET(chip, retval);
		}
		TRACE_GOTO(chip, Fail);
	}
#endif

	return STATUS_SUCCESS;

Fail:
	rts51x_init_cmd(chip);

	rts51x_add_cmd(chip, READ_REG_CMD, MS_SECTOR_CNT_L, 0, 0);

	retval = rts51x_send_cmd(chip, MODE_CR | STAGE_MS_STATUS, 100);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	retval = rts51x_get_rsp(chip, 3, 200);

#ifdef RCC_BUG_FIX_SP
	if((chip->option.rcc_fail_flag == 2)&&(chip->option.rcc_bug_fix_en==1))
	{//if rcc bug fix support and happen,then fix the bug
		u8 tmpvalue;

		chip->rcc_read_response = 0;
		rts51x_ep0_read_register(chip, SFSM_ED, &tmpvalue);
		if(!(tmpvalue&CARD_ERR))//if card is not error,then it is stalled by rcc bug
		{
			rts51x_ep0_read_register(chip,MS_SECTOR_CNT_L,chip->rsp_buf);
			rts51x_ep0_read_register(chip, MS_TRANS_CFG, chip->rsp_buf+1);
			rts51x_ep0_read_register(chip, MS_INT_REG, chip->rsp_buf+2);
			retval = STATUS_SUCCESS;
		}
		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, MC_FIFO_CTL, FIFO_FLUSH, FIFO_FLUSH);
//		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, SFSM_ED, 0xf8, 0xf8);
	}
#endif

	if (CHECK_MS_TRANS_FAIL(chip, retval)) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	sec_cnt = chip->rsp_buf[0];
	RTS51X_DEBUGP(("%d pages need be trasferred, %d pages remained\n",
		       (int)page_cnt, (int)sec_cnt));
	page_addr = start_page + (page_cnt - sec_cnt);

	if (CHK_MS4BIT(ms_card)) {
		val1 = chip->rsp_buf[1];
		RTS51X_DEBUGP(("MS_TRANS_CFG: 0x%x\n", val1));
	} else {
		// we don't need to care MS_TRANS_CFG if 1-bit card
		val1 = 0;
	}

	val2 = chip->rsp_buf[2];
	RTS51X_DEBUGP(("GET_INT: 0x%x\n", val2));

	if ((val1 & INT_CMDNK) || (val2 & INT_REG_CMDNK)) {
		// Command can not be executed
		ms_set_err_code(chip, MS_CMD_NK);
		TRACE_RET(chip, STATUS_FAIL);
	}

	if ((val1 & INT_ERR) || (val2 & INT_REG_ERR)) {
		if ((val1 & INT_BREQ) || (val2 & INT_REG_BREQ)) {
			retval = ms_read_status_reg(chip);
			if (retval != STATUS_SUCCESS) {
				// for MS check procedure
				// when reading data,if uncorrectable error occurs,
				// such a page status is set to 1:NG,
				// and block status in not set to 0:NG,when W/P OFF
				// besides,page status and block status are not changed when W/P ON
				if (!(chip->card_wp & MS_CARD)) {
					//because BREQ may still set,we can't do as below directly.
					//After reset MS ,we can do as below.
					reset_ms(chip);
					ms_set_page_status(log_blk, setPS_NG, extra, MS_EXTRA_SIZE);
					ms_write_extra_data(chip, phy_blk,
							page_addr, extra, MS_EXTRA_SIZE);
				}
				ms_set_err_code(chip, MS_FLASH_READ_ERROR);
				TRACE_RET(chip, STATUS_FAIL);
			}
		} else {
			ms_set_err_code(chip, MS_FLASH_READ_ERROR);
			TRACE_RET(chip, STATUS_FAIL);
		}
	} else {
		if (CHK_MS4BIT(ms_card)) {
			if (!(val1 & INT_BREQ) && !(val2 & INT_REG_BREQ)) {
				ms_set_err_code(chip, MS_BREQ_ERROR);
				TRACE_RET(chip, STATUS_FAIL);
			}
		} else {
			if (!(val2 & INT_REG_BREQ)) {
				ms_set_err_code(chip, MS_BREQ_ERROR);
				TRACE_RET(chip, STATUS_FAIL);
			}
		}
	}

	TRACE_RET(chip, STATUS_FAIL);
}

static int ms_write_multiple_pages(struct rts51x_chip *chip, u16 old_blk, u16 new_blk,
		u16 log_blk, u8 start_page, u8 end_page, u8 *buf, void **ptr, unsigned int *offset)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, i;
	int send_blkend;
	u8 val, data[16];
	u8 page_cnt = end_page - start_page;

	if ((end_page == (ms_card->page_off + 1)) || (page_cnt == 1)) {
		send_blkend = 0;
	} else {
		send_blkend = 1;
	}

	if (!start_page) {
		if (CHK_MS4BIT(ms_card)) {
			// Parallel interface
			data[0] = 0x88;
		} else {
			// Serial interface
			data[0] = 0x80;
		}
		// Block Address
		data[1] = 0;
		data[2] = (u8)(old_blk >> 8);
		data[3] = (u8)old_blk;
		data[4] = 0x80;
		data[5] = 0;
		data[6] = 0xEF;
		data[7] = 0xFF;

		retval = ms_auto_set_cmd(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 7,
				    BLOCK_WRITE, WAIT_INT, data, 7, &val);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}
	}

	retval = ms_set_rw_reg_addr(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, (6 + MS_EXTRA_SIZE));
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	ms_set_err_code(chip, MS_NO_ERROR);

	if (CHK_MS4BIT(ms_card)) {
		// Parallel interface
		data[0] = 0x88;
	} else {
		// Serial interface
		data[0] = 0x80;
	}
	// Block Address
	data[1] = 0;
	data[2] = (u8)(new_blk >> 8);
	data[3] = (u8)new_blk;
	// Page Number
	// Extra data access mode
	if (page_cnt == 1) {
		// Single page access mode
		data[4] = 0x20;
	} else {
		// Block access mode
		data[4] = 0;
	}
	data[5] = start_page;
	data[6] = 0xF8;
	data[7] = 0xFF;
	data[8] = (u8)(log_blk >> 8);
	data[9] = (u8)log_blk;

	for (i = 0x0A; i < 0x10; i++) {
		// ECC
		data[i] = 0xFF;
	}

	retval = ms_auto_set_cmd(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, (6 + MS_EXTRA_SIZE),
			    BLOCK_WRITE, WAIT_INT, data, 16, &val);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	rts51x_init_cmd(chip);

	if (send_blkend) {
		rts51x_add_cmd(chip, WRITE_REG_CMD, MS_BLKEND, SET_BLKEND, SET_BLKEND);
	} else {
		rts51x_add_cmd(chip, WRITE_REG_CMD, MS_BLKEND, SET_BLKEND, 0);
	}
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANS_CFG, WAIT_INT, NO_WAIT_INT);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_SECTOR_CNT_L, 0xFF, (u8)page_cnt);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_SECTOR_CNT_H, 0xFF, 0);
	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TPC, 0xFF, WRITE_PAGE_DATA);
	rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_DATA_SOURCE, 0x01, RING_BUFFER);

	trans_dma_enable(DMA_TO_DEVICE, chip, 512 * page_cnt, DMA_512);

	rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANSFER, 0xFF, MS_TRANSFER_START |  MS_TM_MULTI_WRITE);
	rts51x_add_cmd(chip, CHECK_REG_CMD, MS_TRANSFER, MS_TRANSFER_END, MS_TRANSFER_END);

	retval = rts51x_send_cmd(chip, MODE_CDOR | STAGE_MS_STATUS, 100);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	retval = rts51x_transfer_data_partial(chip, SND_BULK_PIPE(chip), (void *)buf,
		ptr, offset, 512 * page_cnt, scsi_sg_count(chip->srb), NULL, 2000);
	if (retval != STATUS_SUCCESS) {
		rts51x_clear_ms_error(chip);
		TRACE_RET(chip, retval);
	}

	retval = rts51x_get_rsp(chip, 3, 2000);

#ifdef RCC_BUG_FIX_SP
	if((chip->option.rcc_fail_flag == 2)&&(chip->option.rcc_bug_fix_en==1))
	{//if rcc bug fix support and happen,then fix the bug
		u8 tmpvalue;
		int num = 0;

		rts51x_ep0_read_register(chip, SFSM_ED, &tmpvalue);
		if(!(tmpvalue&CARD_ERR))//if card is not error,then it is stalled by rcc bug
		{
			for(num=0;num < 2000;num++)
			{
				rts51x_ep0_read_register(chip, MS_TRANSFER, &tmpvalue);
				if(tmpvalue&MS_TRANSFER_END)
				{
					break;
				}
				wait_timeout(1);
			}
		}
		if((tmpvalue&MS_TRANSFER_ERR)||(num==2000))
		{
			retval = STATUS_FAIL;
		}
		else
		{
			*chip->rsp_buf = tmpvalue;
			rts51x_ep0_read_register(chip, MS_TRANS_CFG, chip->rsp_buf+1);
			rts51x_ep0_read_register(chip, MS_INT_REG, chip->rsp_buf+2);
			retval = STATUS_SUCCESS;
		}
		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, MC_FIFO_CTL, FIFO_FLUSH, FIFO_FLUSH);
//		rts51x_reset_pipe(chip,0);//send clear feature to clear stall
		rts51x_ep0_write_register(chip, SFSM_ED, 0xf8, 0xf8);
	}
#endif

	if (CHECK_MS_TRANS_FAIL(chip, retval)) {
		rts51x_clear_ms_error(chip);
		TRACE_RET(chip, STATUS_FAIL);
	}

	return STATUS_SUCCESS;
}

#else

static int ms_read_multiple_pages(struct rts51x_chip *chip, u16 phy_blk, u16 log_blk,
		u8 start_page, u8 end_page, u8 *buf, void **ptr, unsigned int *offset)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, i;
	u8 extra[MS_EXTRA_SIZE], page_addr, val, trans_cfg, data[6];

	// for Digital Read Protect bit issue
	retval = ms_read_extra_data(chip, phy_blk, start_page, extra, MS_EXTRA_SIZE);
	if (retval == STATUS_SUCCESS) {
		// if COPY bit & Access bit not set [1,1],then read protect
		if ((extra[1] & 0x30) != 0x30) {
			ms_set_err_code(chip, MS_FLASH_READ_ERROR);
			TRACE_RET(chip, STATUS_FAIL);
		}
	}

	retval = ms_set_rw_reg_addr(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 6);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	// Write REG
	if (CHK_MS4BIT(ms_card)) {
		// Parallel interface
		data[0] = 0x88;
	} else {
		// Serial interface
		data[0] = 0x80;
	}
	// Block Address
	data[1] = 0;
	data[2] = (u8)(phy_blk >> 8);
	data[3] = (u8)phy_blk;
	// Page Number
	// Extra data access mode
	data[4] = 0;
	data[5] = start_page;

	for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
		retval = ms_write_bytes(chip, WRITE_REG, 6, NO_WAIT_INT, data, 6);
		if (retval == STATUS_SUCCESS) {
			break;
		}
	}
	if (i == MS_MAX_RETRY_COUNT) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	ms_set_err_code(chip, MS_NO_ERROR);

	retval = ms_send_cmd(chip, BLOCK_READ, WAIT_INT);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	for (page_addr = start_page; page_addr < end_page; page_addr ++) {
		ms_set_err_code(chip, MS_NO_ERROR);

		if (monitor_card_cd(chip, MS_CARD) == CD_NOT_EXIST) {
			ms_set_err_code(chip, MS_NO_CARD);
			chip->card_exist &= ~MS_CARD;
			chip->card_ready &= ~MS_CARD;
			TRACE_RET(chip, STATUS_FAIL);
		}

		// GET_INT Register
		retval = ms_read_bytes(chip, GET_INT, 1, NO_WAIT_INT, &val, 1);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}
		if (val & INT_REG_CMDNK) {
			// Command can not be executed
			ms_set_err_code(chip, MS_CMD_NK);
			TRACE_RET(chip, STATUS_FAIL);
		}
		if (val & INT_REG_ERR) {
			if (val & INT_REG_BREQ) {
				retval = ms_read_status_reg(chip);
				if (retval != STATUS_SUCCESS) {
					// for MS check procedure
					// when reading data,if uncorrectable error occurs,
					// such a page status is set to 1:NG,
					// and block status in not set to 0:NG,when W/P OFF
					// besides,page status and block status are not changed when W/P ON
					if (!(chip->card_wp & MS_CARD)) {
						//because BREQ may still set,we can't do as below directly.
						//After reset MS ,we can do as below.
						reset_ms(chip);
						ms_set_page_status(log_blk, setPS_NG, extra, MS_EXTRA_SIZE);
						ms_write_extra_data(chip, phy_blk,
								page_addr, extra, MS_EXTRA_SIZE);
					}
					ms_set_err_code(chip, MS_FLASH_READ_ERROR);
					TRACE_RET(chip, STATUS_FAIL);
				}
			} else {
				ms_set_err_code(chip, MS_FLASH_READ_ERROR);
				TRACE_RET(chip, STATUS_FAIL);
			}
		} else {
			if (!(val & INT_REG_BREQ)) {
				ms_set_err_code(chip, MS_BREQ_ERROR);
				TRACE_RET(chip, STATUS_FAIL);
			}
		}

		if (page_addr == (end_page - 1)) {
			if (!(val & INT_REG_CED)) {
				retval = ms_send_cmd(chip, BLOCK_END, WAIT_INT);
				if (retval != STATUS_SUCCESS) {
					TRACE_RET(chip, retval);
				}
			}

			// GET_INT Register
			// Here We can't check CMDNK here,
			// Because MS Card sometimes will return CMDNK=1 and CED=1 while last page
			retval = ms_read_bytes(chip, GET_INT, 1, NO_WAIT_INT, &val, 1);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, retval);
			}
			if (!(val & INT_REG_CED)) {
				ms_set_err_code(chip, MS_FLASH_READ_ERROR);
				TRACE_RET(chip, STATUS_FAIL);
			}

			trans_cfg = NO_WAIT_INT;
		} else {
			trans_cfg = WAIT_INT;
		}

		rts51x_init_cmd(chip);

		rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TPC, 0xFF, READ_PAGE_DATA);
		rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANS_CFG, 0xFF, trans_cfg);

		trans_dma_enable(DMA_FROM_DEVICE, chip, 512, DMA_512);

		rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANSFER, 0xFF,
				MS_TRANSFER_START |  MS_TM_NORMAL_READ);
		rts51x_add_cmd(chip, CHECK_REG_CMD, MS_TRANSFER, MS_TRANSFER_END, MS_TRANSFER_END);

		retval = rts51x_send_cmd(chip, MODE_CDIR, 100);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		retval = rts51x_transfer_data_partial(chip, RCV_BULK_PIPE(chip), (void *)buf,
			ptr, offset, 512, scsi_sg_count(chip->srb), NULL, 2000);
		if (retval != STATUS_SUCCESS) {
			if (retval == STATUS_TIMEDOUT) {
				ms_set_err_code(chip, MS_TO_ERROR);
				rts51x_clear_ms_error(chip);
				TRACE_RET(chip, retval);
			}

			retval = rts51x_ep0_read_register(chip, MS_TRANS_CFG, &val);
			if (retval != STATUS_SUCCESS) {
				ms_set_err_code(chip, MS_TO_ERROR);
				rts51x_clear_ms_error(chip);
				TRACE_RET(chip, STATUS_FAIL);
			}
			if (val & (MS_CRC16_ERR | MS_RDY_TIMEOUT)) {
				ms_set_err_code(chip, MS_CRC16_ERROR);
				rts51x_clear_ms_error(chip);
				TRACE_RET(chip, STATUS_FAIL);
			}
		}

		retval = rts51x_get_rsp(chip, 1, 2000);
		if (CHECK_MS_TRANS_FAIL(chip, retval)) {
			if (retval == STATUS_TIMEDOUT) {
				ms_set_err_code(chip, MS_TO_ERROR);
				rts51x_clear_ms_error(chip);
				TRACE_RET(chip, retval);
			}

			retval = rts51x_ep0_read_register(chip, MS_TRANS_CFG, &val);
			if (retval != STATUS_SUCCESS) {
				ms_set_err_code(chip, MS_TO_ERROR);
				rts51x_clear_ms_error(chip);
				TRACE_RET(chip, retval);
			}
			if (val & (MS_CRC16_ERR | MS_RDY_TIMEOUT)) {
				ms_set_err_code(chip, MS_CRC16_ERROR);
				rts51x_clear_ms_error(chip);
				TRACE_RET(chip, STATUS_FAIL);
			}
		}
	}

	return STATUS_SUCCESS;
}

static int ms_write_multiple_pages(struct rts51x_chip *chip, u16 old_blk, u16 new_blk,
		u16 log_blk, u8 start_page, u8 end_page, u8 *buf, void **ptr, unsigned int *offset)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, i;
	u8 page_addr, val, data[16];

	if (!start_page) {
		retval = ms_set_rw_reg_addr(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, 7);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		if (CHK_MS4BIT(ms_card)) {
			// Parallel interface
			data[0] = 0x88;
		} else {
			// Serial interface
			data[0] = 0x80;
		}
		// Block Address
		data[1] = 0;
		data[2] = (u8)(old_blk >> 8);
		data[3] = (u8)old_blk;
		data[4] = 0x80;
		data[5] = 0;
		data[6] = 0xEF;
		data[7] = 0xFF;

		retval = ms_write_bytes(chip, WRITE_REG, 7, NO_WAIT_INT, data, 8);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		retval = ms_send_cmd(chip, BLOCK_WRITE, WAIT_INT);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		// GET_INT Register
		ms_set_err_code(chip, MS_NO_ERROR);
		retval = ms_transfer_tpc(chip, MS_TM_READ_BYTES, GET_INT, 1, NO_WAIT_INT);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}
	}

	retval = ms_set_rw_reg_addr(chip, OverwriteFlag, MS_EXTRA_SIZE, SystemParm, (6 + MS_EXTRA_SIZE));
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	ms_set_err_code(chip, MS_NO_ERROR);

	if (CHK_MS4BIT(ms_card)) {
		// Parallel interface
		data[0] = 0x88;
	} else {
		// Serial interface
		data[0] = 0x80;
	}
	// Block Address
	data[1] = 0;
	data[2] = (u8)(new_blk >> 8);
	data[3] = (u8)new_blk;
	// Page Number
	// Extra data access mode
	if ((end_page - start_page) == 1) {
		// Single page access mode
		data[4] = 0x20;
	} else {
		// Block access mode
		data[4] = 0;
	}
	data[5] = start_page;
	data[6] = 0xF8;
	data[7] = 0xFF;
	data[8] = (u8)(log_blk >> 8);
	data[9] = (u8)log_blk;

	for (i = 0x0A; i < 0x10; i++) {
		// ECC
		data[i] = 0xFF;
	}

	for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
		retval = ms_write_bytes(chip, WRITE_REG, 6 + MS_EXTRA_SIZE, NO_WAIT_INT, data, 16);
		if (retval == STATUS_SUCCESS) {
			break;
		}
	}
	if (i == MS_MAX_RETRY_COUNT) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	for (i = 0; i < MS_MAX_RETRY_COUNT; i++) {
		retval = ms_send_cmd(chip, BLOCK_WRITE, WAIT_INT);
		if (retval == STATUS_SUCCESS) {
			break;
		}
	}
	if (i == MS_MAX_RETRY_COUNT) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	// GET_INT Register
	retval = ms_read_bytes(chip, GET_INT, 1, NO_WAIT_INT, &val, 1);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	for (page_addr = start_page; page_addr < end_page; page_addr ++) {
		ms_set_err_code(chip, MS_NO_ERROR);

		if (monitor_card_cd(chip, MS_CARD) == CD_NOT_EXIST) {
			ms_set_err_code(chip, MS_NO_CARD);
			TRACE_RET(chip, STATUS_FAIL);
		}

		if (val & INT_REG_CMDNK) {
			// Command can not be executed
			ms_set_err_code(chip, MS_CMD_NK);
			TRACE_RET(chip, STATUS_FAIL);
		}
		if (val & INT_REG_ERR) {
			ms_set_err_code(chip, MS_FLASH_WRITE_ERROR);
			TRACE_RET(chip, STATUS_FAIL);
		}
		if (!(val & INT_REG_BREQ)) {
			ms_set_err_code(chip, MS_BREQ_ERROR);
			TRACE_RET(chip, STATUS_FAIL);
		}

		udelay(30);

		rts51x_init_cmd(chip);

		rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TPC, 0xFF, WRITE_PAGE_DATA);
		rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANS_CFG, 0xFF, WAIT_INT);

		trans_dma_enable(DMA_TO_DEVICE, chip, 512, DMA_512);

		rts51x_add_cmd(chip, WRITE_REG_CMD, MS_TRANSFER, 0xFF,
				MS_TRANSFER_START |  MS_TM_NORMAL_WRITE);
		rts51x_add_cmd(chip, CHECK_REG_CMD, MS_TRANSFER, MS_TRANSFER_END, MS_TRANSFER_END);

		retval = rts51x_send_cmd(chip, MODE_CDOR, 100);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		retval = rts51x_transfer_data_partial(chip, SND_BULK_PIPE(chip), (void *)buf,
			ptr, offset, 512, scsi_sg_count(chip->srb), NULL, 2000);
		if (retval != STATUS_SUCCESS) {
			ms_set_err_code(chip, MS_TO_ERROR);
			rts51x_clear_ms_error(chip);

			if (retval == STATUS_TIMEDOUT) {
				TRACE_RET(chip, STATUS_TIMEDOUT);
			} else {
				TRACE_RET(chip, STATUS_FAIL);
			}
		}

		retval = rts51x_get_rsp(chip, 1, 2000);
		if (CHECK_MS_TRANS_FAIL(chip, retval)) {
			ms_set_err_code(chip, MS_TO_ERROR);
			rts51x_clear_ms_error(chip);

			if (retval == STATUS_TIMEDOUT) {
				TRACE_RET(chip, STATUS_TIMEDOUT);
			} else {
				TRACE_RET(chip, STATUS_FAIL);
			}
		}

		// GET_INT Register
		retval = ms_read_bytes(chip, GET_INT, 1, NO_WAIT_INT, &val, 1);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		if ((end_page - start_page) == 1) {
			if (!(val & INT_REG_CED)) {
				// Command can not be executed
				ms_set_err_code(chip, MS_FLASH_WRITE_ERROR);
				TRACE_RET(chip, STATUS_FAIL);
			}
		} else {
			if (page_addr == (end_page - 1)) {
				if (!(val & INT_REG_CED)) {
					retval = ms_send_cmd(chip, BLOCK_END, WAIT_INT);
					if (retval != STATUS_SUCCESS) {
						TRACE_RET(chip, retval);
					}
				}

				// GET_INT Register
				retval = ms_read_bytes(chip, GET_INT, 1, NO_WAIT_INT, &val, 1);
				if (retval != STATUS_SUCCESS) {
					TRACE_RET(chip, retval);
				}
			}

			if ((page_addr == (end_page - 1)) || (page_addr == ms_card->page_off)) {
				if (!(val & INT_REG_CED)) {
					// Command can not be executed
					ms_set_err_code(chip, MS_FLASH_WRITE_ERROR);
					TRACE_RET(chip, STATUS_FAIL);
				}
			}
		}
	}

	return STATUS_SUCCESS;
}
#endif

static int ms_finish_write(struct rts51x_chip *chip, u16 old_blk, u16 new_blk,
		u16 log_blk, u8 page_off)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval, seg_no;

#ifdef MS_SPEEDUP
	retval = ms_auto_copy_page(chip, old_blk, new_blk, log_blk,
			page_off, ms_card->page_off + 1);
#else
	retval = ms_copy_page(chip, old_blk, new_blk, log_blk,
			page_off, ms_card->page_off + 1);
#endif
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	seg_no = old_blk >> 9;

	if (MS_TST_BAD_BLOCK_FLG(ms_card)) {
		MS_CLR_BAD_BLOCK_FLG(ms_card);
		ms_set_bad_block(chip, old_blk);
	} else {
		retval = ms_erase_block(chip, old_blk);
		if (retval == STATUS_SUCCESS) {
			ms_set_unused_block(chip, old_blk);
		}
	}

	ms_set_l2p_tbl(chip, seg_no, log_blk - ms_start_idx[seg_no], new_blk);

	return STATUS_SUCCESS;
}

static int ms_prepare_write(struct rts51x_chip *chip, u16 old_blk, u16 new_blk,
		u16 log_blk, u8 start_page)
{
	int retval;

	if (start_page) {
#ifdef MS_SPEEDUP
		retval = ms_auto_copy_page(chip, old_blk, new_blk, log_blk, 0, start_page);
#else
		retval = ms_copy_page(chip, old_blk, new_blk, log_blk, 0, start_page);
#endif
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}
	}

	return STATUS_SUCCESS;
}

int ms_delay_write(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	struct ms_delay_write_tag *delay_write = &(ms_card->delay_write);
	int retval;

	if (delay_write->delay_write_flag) {
		retval = ms_set_init_para(chip);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}

		delay_write->delay_write_flag = 0;
		retval = ms_finish_write(chip,
				delay_write->old_phyblock, delay_write->new_phyblock,
				delay_write->logblock, delay_write->pageoff);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, retval);
		}
	}

	return STATUS_SUCCESS;
}

static inline void ms_rw_fail(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	if (srb->sc_data_direction == DMA_FROM_DEVICE) {
		set_sense_type(chip, SCSI_LUN(srb), SENSE_TYPE_MEDIA_UNRECOVER_READ_ERR);
	} else {
		set_sense_type(chip, SCSI_LUN(srb), SENSE_TYPE_MEDIA_WRITE_ERR);
	}
}

static int ms_rw_multi_sector(struct scsi_cmnd *srb, struct rts51x_chip *chip, u32 start_sector, u16 sector_cnt)
{
	struct ms_info *ms_card = &(chip->ms_card);
	unsigned int lun = SCSI_LUN(srb);
	int retval, seg_no;
	unsigned int offset = 0;
	u16 old_blk = 0, new_blk = 0, log_blk, total_sec_cnt = sector_cnt;
	u8 start_page, end_page = 0, page_cnt;
	u8 *buf;
	void *ptr = NULL;
	struct ms_delay_write_tag *delay_write = &(ms_card->delay_write);

	ms_set_err_code(chip, MS_NO_ERROR);

	ms_card->counter = 0;

	buf = (u8 *)scsi_sglist(srb);

	retval = ms_switch_clock(chip);
	if (retval != STATUS_SUCCESS) {
		ms_rw_fail(srb, chip);
		TRACE_RET(chip, retval);
	}

	log_blk = (u16)(start_sector >> ms_card->block_shift);
	start_page = (u8)(start_sector & ms_card->page_off);

	for (seg_no = 0; seg_no < sizeof(ms_start_idx)/2; seg_no++) {
		if (log_blk < ms_start_idx[seg_no+1]) {
			break;
		}
	}

	if (ms_card->segment[seg_no].build_flag == 0) {
		retval = ms_build_l2p_tbl(chip, seg_no);
		if (retval != STATUS_SUCCESS) {
			chip->card_fail |= MS_CARD;
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
			TRACE_RET(chip, retval);
		}
	}

	if (srb->sc_data_direction == DMA_TO_DEVICE) {
		if (delay_write->delay_write_flag &&
				(delay_write->logblock == log_blk) &&
				(start_page > delay_write->pageoff)) {
			delay_write->delay_write_flag = 0;
#ifdef MS_SPEEDUP
			retval = ms_auto_copy_page(chip,
				delay_write->old_phyblock,
				delay_write->new_phyblock, log_blk,
				delay_write->pageoff, start_page);
#else
			retval = ms_copy_page(chip,
				delay_write->old_phyblock,
				delay_write->new_phyblock, log_blk,
				delay_write->pageoff, start_page);
#endif
			if (retval != STATUS_SUCCESS) {
				set_sense_type(chip, lun, SENSE_TYPE_MEDIA_WRITE_ERR);
				TRACE_RET(chip, retval);
			}
			old_blk = delay_write->old_phyblock;
			new_blk = delay_write->new_phyblock;
		} else if (delay_write->delay_write_flag &&
				(delay_write->logblock == log_blk) &&
				(start_page == delay_write->pageoff)) {
			delay_write->delay_write_flag = 0;
			old_blk = delay_write->old_phyblock;
			new_blk = delay_write->new_phyblock;
		} else {
			retval = ms_delay_write(chip);
			if (retval != STATUS_SUCCESS) {
				set_sense_type(chip, lun, SENSE_TYPE_MEDIA_WRITE_ERR);
				TRACE_RET(chip, retval);
			}
			old_blk = ms_get_l2p_tbl(chip, seg_no, log_blk - ms_start_idx[seg_no]);
			new_blk  = ms_get_unused_block(chip, seg_no);
			if ((old_blk == 0xFFFF) || (new_blk == 0xFFFF)) {
				set_sense_type(chip, lun, SENSE_TYPE_MEDIA_WRITE_ERR);
				TRACE_RET(chip, STATUS_FAIL);
			}

			retval = ms_prepare_write(chip, old_blk, new_blk, log_blk, start_page);
			if (retval != STATUS_SUCCESS) {
				if (monitor_card_cd(chip, MS_CARD) == CD_NOT_EXIST) {
					set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
					TRACE_RET(chip, STATUS_FAIL);
				}

				set_sense_type(chip, lun, SENSE_TYPE_MEDIA_WRITE_ERR);
				TRACE_RET(chip, retval);
			}
		}
	} else {
		retval = ms_delay_write(chip);
		if (retval != STATUS_SUCCESS) {
			if (monitor_card_cd(chip, MS_CARD) == CD_NOT_EXIST) {
				set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
				TRACE_RET(chip, STATUS_FAIL);
			}

			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_UNRECOVER_READ_ERR);
			TRACE_RET(chip, retval);
		}
		old_blk = ms_get_l2p_tbl(chip, seg_no, log_blk - ms_start_idx[seg_no]);
		if (old_blk == 0xFFFF) {
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_UNRECOVER_READ_ERR);
			TRACE_RET(chip, STATUS_FAIL);
		}
	}

	RTS51X_DEBUGP(("seg_no = %d, old_blk = 0x%x, new_blk = 0x%x\n", seg_no, old_blk, new_blk));

	while (total_sec_cnt) {
		if ((start_page + total_sec_cnt) > (ms_card->page_off + 1)) {
			end_page = ms_card->page_off + 1;
		} else {
			end_page = start_page + (u8)total_sec_cnt;
		}
		page_cnt = end_page - start_page;

		RTS51X_DEBUGP(("start_page = %d, end_page = %d, page_cnt = %d\n",
				start_page, end_page, page_cnt));

		if (srb->sc_data_direction == DMA_FROM_DEVICE) {
			retval = ms_read_multiple_pages(chip,
				old_blk, log_blk, start_page, end_page,
				buf, &ptr, &offset);
		} else {
			retval = ms_write_multiple_pages(chip, old_blk,
				new_blk, log_blk, start_page, end_page,
				buf, &ptr, &offset);
		}

		if (retval != STATUS_SUCCESS) {
			if (monitor_card_cd(chip, MS_CARD) == CD_NOT_EXIST) {
				set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
				TRACE_RET(chip, STATUS_FAIL);
			}

			ms_rw_fail(srb, chip);
			TRACE_RET(chip, retval);
		}

		// Update L2P table if need
		if (srb->sc_data_direction == DMA_TO_DEVICE) {
			if (end_page == (ms_card->page_off + 1)) {
				retval = ms_erase_block(chip, old_blk);
				if (retval == STATUS_SUCCESS) {
					ms_set_unused_block(chip, old_blk);
				}
				ms_set_l2p_tbl(chip, seg_no, log_blk - ms_start_idx[seg_no], new_blk);
			}
		}

		total_sec_cnt -= page_cnt;

		if (total_sec_cnt == 0) {
			// Data transfer finished
			break;
		}

		log_blk ++;

		for (seg_no = 0; seg_no < sizeof(ms_start_idx)/2; seg_no++) {
			if (log_blk < ms_start_idx[seg_no+1]) {
				break;
			}
		}

		if (ms_card->segment[seg_no].build_flag == 0) {
			retval = ms_build_l2p_tbl(chip, seg_no);
			if (retval != STATUS_SUCCESS) {
				chip->card_fail |= MS_CARD;
				set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
				TRACE_RET(chip, retval);
			}
		}

		old_blk = ms_get_l2p_tbl(chip, seg_no, log_blk - ms_start_idx[seg_no]);
		if (old_blk == 0xFFFF) {
			ms_rw_fail(srb, chip);
			TRACE_RET(chip, STATUS_FAIL);
		}

		if (srb->sc_data_direction == DMA_TO_DEVICE) {
			new_blk = ms_get_unused_block(chip, seg_no);
			if (new_blk == 0xFFFF) {
				ms_rw_fail(srb, chip);
				TRACE_RET(chip, STATUS_FAIL);
			}
		}

		RTS51X_DEBUGP(("seg_no = %d, old_blk = 0x%x, new_blk = 0x%x\n", seg_no, old_blk, new_blk));

		start_page = 0;
	}

	if (srb->sc_data_direction == DMA_TO_DEVICE) {
		if (end_page < (ms_card->page_off + 1)) {
			delay_write->delay_write_flag = 1;
			delay_write->old_phyblock = old_blk;
			delay_write->new_phyblock = new_blk;
			delay_write->logblock = log_blk;
			delay_write->pageoff = end_page;
		}
	}

	scsi_set_resid(srb, 0);

	return STATUS_SUCCESS;
}

int ms_rw(struct scsi_cmnd *srb, struct rts51x_chip *chip, u32 start_sector, u16 sector_cnt)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;

	if (CHK_MSPRO(ms_card)) {
		retval = mspro_rw_multi_sector(srb, chip, start_sector, sector_cnt);
	} else {
		retval = ms_rw_multi_sector(srb, chip, start_sector, sector_cnt);
	}

	return retval;
}


void ms_free_l2p_tbl(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int i = 0;

	if (ms_card->segment != NULL) {
		for (i = 0; i < ms_card->segment_cnt; i++) {
			if (ms_card->segment[i].l2p_table != NULL) {
				vfree(ms_card->segment[i].l2p_table);
				ms_card->segment[i].l2p_table = NULL;
			}
			if (ms_card->segment[i].free_table != NULL) {
				vfree(ms_card->segment[i].free_table);
				ms_card->segment[i].free_table = NULL;
			}
		}
		vfree(ms_card->segment);
		ms_card->segment = NULL;
	}
}

void ms_cleanup_work(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);

	//sangdy2010-06-29:modify for magic gate.it may be set 2K mode
	//but magic gate don't use 2K mode.So clear it to reset state.
	if (CHK_MSPRO(ms_card)) {
		if(ms_card->seq_mode)
		{
			RTS51X_DEBUGP(("MS Pro: stop transmission\n"));
			mspro_stop_seq_mode(chip);
			ms_card->counter = 0;
		}
		if(CHK_MSHG(ms_card))
		{
			u8 value;
			rts51x_read_register(chip, MS_CFG, &value);
			if(value&MS_2K_SECTOR_MODE)
			{
				//clear 2K mode
				rts51x_write_register(chip,MS_CFG,MS_2K_SECTOR_MODE,0x00);
			}
		}
	} else if ((!CHK_MSPRO(ms_card)) && ms_card->delay_write.delay_write_flag) {
		RTS51X_DEBUGP(("MS: delay write\n"));
		ms_delay_write(chip);
		ms_card->counter = 0;
	}
}

int ms_power_off_card3v3(struct rts51x_chip *chip)
{
	int retval;

	rts51x_init_cmd(chip);

	//sangdy2010-07-13:modify closing clock sequence to avoid glitch
	rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_CLK_EN, MS_CLK_EN, 0);
	if (chip->asic_code) {
		ms_pull_ctl_disable(chip);
	} else {
		rts51x_add_cmd(chip, WRITE_REG_CMD, FPGA_PULL_CTL,
			       FPGA_MS_PULL_CTL_BIT | 0x20, FPGA_MS_PULL_CTL_BIT);
	}
	rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_OE, MS_OUTPUT_EN, 0);
	if(!chip->option.FT2_fast_mode)
	{//sangdy2010-07-13:not FT2 fast mode ,close card power
		rts51x_add_cmd(chip, WRITE_REG_CMD, CARD_PWR_CTL, POWER_MASK, POWER_OFF);
	}

	retval = rts51x_send_cmd(chip, MODE_C, 100);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	return STATUS_SUCCESS;
}

int release_ms_card(struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;

	RTS51X_DEBUGP(("release_ms_card\n"));

	ms_card->delay_write.delay_write_flag = 0;
	ms_card->pro_under_formatting = 0;

	chip->card_ready &= ~MS_CARD;
	chip->card_fail &= ~MS_CARD;
	chip->card_wp &= ~MS_CARD;

	ms_free_l2p_tbl(chip);

	//sangdy:if release card, then force hardware stage to idle state,for no card must be idle state
	//rts51x_ep0_write_register(chip, SFSM_ED, HW_CMD_STOP, HW_CMD_STOP);
	//Sangdy2010-10-08:for AMD platform(EHCI VID:1002,PID:4396),using ctrol pipe may cause host blue screen
	//so use bulk pipe
	rts51x_write_register(chip, SFSM_ED, HW_CMD_STOP, HW_CMD_STOP);


	memset(ms_card->raw_sys_info, 0, 96);
	memset(ms_card->raw_ms_id, 0, 16);
#ifdef SUPPORT_PCGL_1P18
	memset(ms_card->raw_model_name, 0, 48);
#endif

	retval = ms_power_off_card3v3(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, retval);
	}

	return STATUS_SUCCESS;
}

