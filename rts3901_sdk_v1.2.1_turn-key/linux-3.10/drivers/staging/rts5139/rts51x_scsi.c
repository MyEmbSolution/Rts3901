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

#include <scsi/scsi.h>
#include <scsi/scsi_eh.h>
#include <scsi/scsi_device.h>

#include "debug.h"
#include "rts51x.h"
#include "rts51x_chip.h"
#include "rts51x_scsi.h"
#include "rts51x_card.h"
#include "rts51x_transport.h"
#include "rts51x_sys.h"
#include "sd_cprm.h"
#include "ms_mg.h"
#include "trace.h"

void scsi_show_command(struct scsi_cmnd *srb)
{
#if DBG
	char *what = NULL;
	int i, unknown_cmd = 0;

	switch (srb->cmnd[0]) {
	case TEST_UNIT_READY: what = (char *)"TEST_UNIT_READY"; break;
	case REZERO_UNIT: what = (char *)"REZERO_UNIT"; break;
	case REQUEST_SENSE: what = (char *)"REQUEST_SENSE"; break;
	case FORMAT_UNIT: what = (char *)"FORMAT_UNIT"; break;
	case READ_BLOCK_LIMITS: what = (char *)"READ_BLOCK_LIMITS"; break;
	case 0x07: what = (char *)"REASSIGN_BLOCKS"; break;
	case READ_6: what = (char *)"READ_6"; break;
	case WRITE_6: what = (char *)"WRITE_6"; break;
	case SEEK_6: what = (char *)"SEEK_6"; break;
	case READ_REVERSE: what = (char *)"READ_REVERSE"; break;
	case WRITE_FILEMARKS: what = (char *)"WRITE_FILEMARKS"; break;
	case SPACE: what = (char *)"SPACE"; break;
	case INQUIRY: what = (char *)"INQUIRY"; break;
	case RECOVER_BUFFERED_DATA: what = (char *)"RECOVER_BUFFERED_DATA"; break;
	case MODE_SELECT: what = (char *)"MODE_SELECT"; break;
	case RESERVE: what = (char *)"RESERVE"; break;
	case RELEASE: what = (char *)"RELEASE"; break;
	case COPY: what = (char *)"COPY"; break;
	case ERASE: what = (char *)"ERASE"; break;
	case MODE_SENSE: what = (char *)"MODE_SENSE"; break;
	case START_STOP: what = (char *)"START_STOP"; break;
	case RECEIVE_DIAGNOSTIC: what = (char *)"RECEIVE_DIAGNOSTIC"; break;
	case SEND_DIAGNOSTIC: what = (char *)"SEND_DIAGNOSTIC"; break;
	case ALLOW_MEDIUM_REMOVAL: what = (char *)"ALLOW_MEDIUM_REMOVAL"; break;
	case SET_WINDOW: what = (char *)"SET_WINDOW"; break;
	case READ_CAPACITY: what = (char *)"READ_CAPACITY"; break;
	case READ_10: what = (char *)"READ_10"; break;
	case WRITE_10: what = (char *)"WRITE_10"; break;
	case SEEK_10: what = (char *)"SEEK_10"; break;
	case WRITE_VERIFY: what = (char *)"WRITE_VERIFY"; break;
	case VERIFY: what = (char *)"VERIFY"; break;
	case SEARCH_HIGH: what = (char *)"SEARCH_HIGH"; break;
	case SEARCH_EQUAL: what = (char *)"SEARCH_EQUAL"; break;
	case SEARCH_LOW: what = (char *)"SEARCH_LOW"; break;
	case SET_LIMITS: what = (char *)"SET_LIMITS"; break;
	case READ_POSITION: what = (char *)"READ_POSITION"; break;
	case SYNCHRONIZE_CACHE: what = (char *)"SYNCHRONIZE_CACHE"; break;
	case LOCK_UNLOCK_CACHE: what = (char *)"LOCK_UNLOCK_CACHE"; break;
	case READ_DEFECT_DATA: what = (char *)"READ_DEFECT_DATA"; break;
	case MEDIUM_SCAN: what = (char *)"MEDIUM_SCAN"; break;
	case COMPARE: what = (char *)"COMPARE"; break;
	case COPY_VERIFY: what = (char *)"COPY_VERIFY"; break;
	case WRITE_BUFFER: what = (char *)"WRITE_BUFFER"; break;
	case READ_BUFFER: what = (char *)"READ_BUFFER"; break;
	case UPDATE_BLOCK: what = (char *)"UPDATE_BLOCK"; break;
	case READ_LONG: what = (char *)"READ_LONG"; break;
	case WRITE_LONG: what = (char *)"WRITE_LONG"; break;
	case CHANGE_DEFINITION: what = (char *)"CHANGE_DEFINITION"; break;
	case WRITE_SAME: what = (char *)"WRITE_SAME"; break;
	case GPCMD_READ_SUBCHANNEL: what = (char *)"READ SUBCHANNEL"; break;
	case READ_TOC: what = (char *)"READ_TOC"; break;
	case GPCMD_READ_HEADER: what = (char *)"READ HEADER"; break;
	case GPCMD_PLAY_AUDIO_10: what = (char *)"PLAY AUDIO (10)"; break;
	case GPCMD_PLAY_AUDIO_MSF: what = (char *)"PLAY AUDIO MSF"; break;
	case GPCMD_GET_EVENT_STATUS_NOTIFICATION:
		what = (char *)"GET EVENT/STATUS NOTIFICATION"; break;
	case GPCMD_PAUSE_RESUME: what = (char *)"PAUSE/RESUME"; break;
	case LOG_SELECT: what = (char *)"LOG_SELECT"; break;
	case LOG_SENSE: what = (char *)"LOG_SENSE"; break;
	case GPCMD_STOP_PLAY_SCAN: what = (char *)"STOP PLAY/SCAN"; break;
	case GPCMD_READ_DISC_INFO: what = (char *)"READ DISC INFORMATION"; break;
	case GPCMD_READ_TRACK_RZONE_INFO:
		what = (char *)"READ TRACK INFORMATION"; break;
	case GPCMD_RESERVE_RZONE_TRACK: what = (char *)"RESERVE TRACK"; break;
	case GPCMD_SEND_OPC: what = (char *)"SEND OPC"; break;
	case MODE_SELECT_10: what = (char *)"MODE_SELECT_10"; break;
	case GPCMD_REPAIR_RZONE_TRACK: what = (char *)"REPAIR TRACK"; break;
	case 0x59: what = (char *)"READ MASTER CUE"; break;
	case MODE_SENSE_10: what = (char *)"MODE_SENSE_10"; break;
	case GPCMD_CLOSE_TRACK: what = (char *)"CLOSE TRACK/SESSION"; break;
	case 0x5C: what = (char *)"READ BUFFER CAPACITY"; break;
	case 0x5D: what = (char *)"SEND CUE SHEET"; break;
	case GPCMD_BLANK: what = (char *)"BLANK"; break;
	case REPORT_LUNS: what = (char *)"REPORT LUNS"; break;
	case MOVE_MEDIUM: what = (char *)"MOVE_MEDIUM or PLAY AUDIO (12)"; break;
	case READ_12: what = (char *)"READ_12"; break;
	case WRITE_12: what = (char *)"WRITE_12"; break;
	case WRITE_VERIFY_12: what = (char *)"WRITE_VERIFY_12"; break;
	case SEARCH_HIGH_12: what = (char *)"SEARCH_HIGH_12"; break;
	case SEARCH_EQUAL_12: what = (char *)"SEARCH_EQUAL_12"; break;
	case SEARCH_LOW_12: what = (char *)"SEARCH_LOW_12"; break;
	case SEND_VOLUME_TAG: what = (char *)"SEND_VOLUME_TAG"; break;
	case READ_ELEMENT_STATUS: what = (char *)"READ_ELEMENT_STATUS"; break;
	case GPCMD_READ_CD_MSF: what = (char *)"READ CD MSF"; break;
	case GPCMD_SCAN: what = (char *)"SCAN"; break;
	case GPCMD_SET_SPEED: what = (char *)"SET CD SPEED"; break;
	case GPCMD_MECHANISM_STATUS: what = (char *)"MECHANISM STATUS"; break;
	case GPCMD_READ_CD: what = (char *)"READ CD"; break;
	case 0xE1: what = (char *)"WRITE CONTINUE"; break;
	case WRITE_LONG_2: what = (char *)"WRITE_LONG_2"; break;
	case VENDOR_CMND: what = (char *)"Realtek's vendor command"; break;
	default: what = (char *)"(unknown command)"; unknown_cmd = 1; break;
	}

	if (srb->cmnd[0] != TEST_UNIT_READY) {
		RTS51X_DEBUGP(("Command %s (%d bytes)\n", what, srb->cmd_len));
	}
	if (unknown_cmd) {
		RTS51X_DEBUGP((""));
		for (i = 0; i < srb->cmd_len && i < 16; i++)
			RTS51X_DEBUGPN((" %02x", srb->cmnd[i]));
		RTS51X_DEBUGPN(("\n"));
	}
#endif
}

void set_sense_type(struct rts51x_chip *chip, unsigned int lun, int sense_type)
{
	switch (sense_type) {
	case SENSE_TYPE_MEDIA_CHANGE:
		set_sense_data(chip, lun, CUR_ERR, 0x06, 0, 0x28, 0, 0, 0);
     		break;

    	case SENSE_TYPE_MEDIA_NOT_PRESENT:
		set_sense_data(chip, lun, CUR_ERR, 0x02, 0, 0x3A, 0, 0, 0);
    		break;

	case SENSE_TYPE_MEDIA_LBA_OVER_RANGE:
		set_sense_data(chip, lun, CUR_ERR, 0x05, 0, 0x21, 0, 0, 0);
		break;

	case SENSE_TYPE_MEDIA_LUN_NOT_SUPPORT:
		set_sense_data(chip, lun, CUR_ERR, 0x05, 0, 0x25, 0, 0, 0);
		break;

	case SENSE_TYPE_MEDIA_WRITE_PROTECT:
		set_sense_data(chip, lun, CUR_ERR, 0x07, 0, 0x27, 0, 0, 0);
		break;

	case SENSE_TYPE_MEDIA_UNRECOVER_READ_ERR:
		set_sense_data(chip, lun, CUR_ERR, 0x03, 0, 0x11, 0, 0, 0);
		break;

	case SENSE_TYPE_MEDIA_WRITE_ERR:
		set_sense_data(chip, lun, CUR_ERR, 0x03, 0, 0x0C, 0x02, 0, 0);
		break;

	case SENSE_TYPE_MEDIA_INVALID_CMD_FIELD:
		set_sense_data(chip, lun, CUR_ERR, ILGAL_REQ, 0,
				ASC_INVLD_CDB, ASCQ_INVLD_CDB, CDB_ILLEGAL, 1);
		break;

	case SENSE_TYPE_MEDIA_INVALID_CMD_CODE:
		set_sense_data(chip, lun, CUR_ERR, ILGAL_REQ, 0, 0x20, 0x00, 0, 0);
		break;

	case SENSE_TYPE_FORMAT_IN_PROGRESS:
		set_sense_data(chip, lun, CUR_ERR, 0x02, 0, 0x04, 0x04, 0, 0);
		break;

	case SENSE_TYPE_FORMAT_CMD_FAILED:
		set_sense_data(chip, lun, CUR_ERR, 0x03, 0, 0x31, 0x01, 0, 0);
		break;

#ifdef SUPPORT_MAGIC_GATE
	case SENSE_TYPE_MG_KEY_FAIL_NOT_ESTAB:
		set_sense_data(chip, lun, CUR_ERR, 0x05, 0, 0x6F, 0x02, 0, 0);
		break;

	case SENSE_TYPE_MG_KEY_FAIL_NOT_AUTHEN:
		set_sense_data(chip, lun, CUR_ERR, 0x05, 0, 0x6F, 0x00, 0, 0);
		break;

	case SENSE_TYPE_MG_INCOMPATIBLE_MEDIUM:
		set_sense_data(chip, lun, CUR_ERR, 0x02, 0, 0x30, 0x00, 0, 0);
		break;

	case SENSE_TYPE_MG_WRITE_ERR:
		set_sense_data(chip, lun, CUR_ERR, 0x03, 0, 0x0C, 0x00, 0, 0);
		break;
#endif

#ifdef SUPPORT_SD_LOCK
	case SENSE_TYPE_MEDIA_READ_FORBIDDEN:
		set_sense_data(chip, lun, CUR_ERR, 0x07, 0, 0x11, 0x13, 0, 0);
		break;
#endif

    	case SENSE_TYPE_NO_SENSE:
	default:
		set_sense_data(chip, lun, CUR_ERR, 0, 0, 0, 0, 0, 0);
    		break;
	}
}

void set_sense_data(struct rts51x_chip *chip, unsigned int lun, u8 err_code, u8 sense_key,
		u32 info, u8 asc, u8 ascq, u8 sns_key_info0, u16 sns_key_info1)
{
	struct sense_data_t *sense = &(chip->sense_buffer[lun]);

	sense->err_code = err_code;
	sense->sense_key = sense_key;
	sense->info[0] = (u8)(info >> 24);
	sense->info[1] = (u8)(info >> 16);
	sense->info[2] = (u8)(info >> 8);
	sense->info[3] = (u8)info;

	sense->ad_sense_len = sizeof(struct sense_data_t) - 8;
	sense->asc = asc;
	sense->ascq = ascq;
	if ( sns_key_info0 != 0 ) {
	        sense->sns_key_info[0] = SKSV | sns_key_info0;
        	sense->sns_key_info[1] = (sns_key_info1 & 0xf0) >> 8;
	        sense->sns_key_info[2] = sns_key_info1 & 0x0f;
    	}
}

static int test_unit_ready(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	unsigned int lun = SCSI_LUN(srb);

	rts51x_init_cards(chip);

	if (!check_card_ready(chip, lun)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
		return TRANSPORT_FAILED;
	}

	if (!check_lun_mc(chip, lun)) {
		set_lun_mc(chip, lun);
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_CHANGE);
		return TRANSPORT_FAILED;
	}

#ifdef SUPPORT_SD_LOCK
	if (get_lun_card(chip, SCSI_LUN(srb)) == SD_CARD) {
		struct sd_info *sd_card = &(chip->sd_card);
		if (sd_card->sd_lock_notify) {
			sd_card->sd_lock_notify = 0;
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_CHANGE);
			return TRANSPORT_FAILED;
		} else if (sd_card->sd_lock_status & SD_LOCKED) {
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_READ_FORBIDDEN);
			return TRANSPORT_FAILED;
		}
	}
#endif

	return TRANSPORT_GOOD;
}

unsigned char formatter_inquiry_str[20] =
{
	'M','E','M','O','R','Y','S','T','I','C','K',
	'-', 'M', 'G', /* Byte[47:49] */
	0x0B,  /* Byte[50]: MG, MS, MSPro, MSXC */
	0x00,  /* Byte[51]: Category Specific Commands */
	0x00,  /* Byte[52]: Access Control and feature */
	0x20, 0x20, 0x20, /* Byte[53:55] */
};

// For WHQL1.6
// Caching mode page, page code 0x08
// Ref to spec sbc2r16.pdf, 6.3.3, page 126
static unsigned char inquiry_caching_mode[] =
{
	0x00|0x08,	// PS | Page Code
	0x12,		//Page Length
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
}; /* Caching Mode Page */

// For WHQL1.6
// Control mode page, Page Code 0x0A
// Ref to spec spc3r23.pdf, section 7.4.6, page 310
static unsigned char inquiry_control_mode[] =
{
	0x00 | 0x0A,		// PS | Page Code
	0x0A,		//Page Length
	0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00
}; /* Control Mode Page */


// For WHQL1.6
// Information Exceptions mode page, Page Code 0x1C
// Ref to spec spc3r23.pdf, section 7.4.11, page 319
static unsigned char inquiry_info_exception_mode[] =
{
	0x00|0x1C,	// PS | Page Code
	0x0A,		//Page Length
	0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00
}; /* Information Exceptions Mode Page */

static unsigned char inquiry_standard_header[] =
{
	QULIFIRE|DRCT_ACCESS_DEV,

	//RMB_DISC,
	RMB_DISC|0x0D,

	// For WHQL 1.6
	// The device must return a valid VERSION field of 0x4,0x5 or 0x6
	// Expectation: VERSION is 0x4(SPC-2) or later for non-SCSI bus type and
	// VERSION is 0x3(SPC) or later for SCSI bus type
	// Ref to SPC4, 6.4.2, page 310
	//0x04,
	0x04,	// VERSION for USB2.0

	// For WHQL 1.6
	// Checking that RESPONSE DATA FORMAT == 2
	// Ref to SPC4, 6.4.2, page 310
	0x02,

	0x5B,	// (n-4)

	0x02, 	//Byte[5]: This byte is set as 0x01 initially, but bit 0 has been defined as PROTECT in SPC-4.

	0,
	REL_ADR|WBUS_32|WBUS_16|SYNC|LINKED|CMD_QUE|SFT_RE
};

// For WHQL1.6
// Unit Serial Number VPD page, page code 0x80
// Ref to SPC4, 7.8.14, page 653
static unsigned char inquiry_unit_serial_num[] =
{
	QULIFIRE|DRCT_ACCESS_DEV,
	0x80,//PAGE CODE
	0x00,// Reserved
	0x10,// Page Length(n-3)
	'2', '0', '1', '2', '0', '6', '2', '9', '1', '4', '3', '4', '5', '3', '0', '0'
}; /* Unit Serial Number VPD page   */

static unsigned char inquiry_supported_vpd_pages[] =
{
	QULIFIRE|DRCT_ACCESS_DEV,
	0x00,
	0x00,	//(n-3), MSB
	0x03,	//(n-3), LSB
	0x00,
	0x80,
	0x83
};

// For WHQL1.6
// Ref to SPC3r23.pdf, 7.6.3, page 350
// Device Identification VPD page, page code 0x83
static unsigned char inquiry_device_identification[] =
{
	QULIFIRE|DRCT_ACCESS_DEV,
	0x83,//PAGE CODE
	0x00,// Page Length(n-3),MSB
	0x34,// Page Length(n-3), LSB
	// Identification Descriptor List

	/* Identification Descriptor Second(T10 Vendor ID based) */
	// Protocol Identifier | Code Set
	0x00 | 0x02,
	// PIV |Association | Identifier Type
	0x00 | 0x00 | 0x01,
	0x00,//Reserved
	0x18,//Identifier Length is set to 08h
	//Byte0~Byte7, T10 Vendor Identification
	0x72,0x65,0x61,0x6c,0x74,0x65,0x6b,0x20, //Realtek T10 ID: realtek(0x72,0x65,0x61,0x6c,0x74,0x65,0x6b)
	'2', '0', '1', '2', '0', '6', '2', '9', '1', '4', '3', '4', '5', '3', '0', '0',

	/* Identification Descriptor Second(NAA) */
	// Protocol Identifier | Code Set
	0x00 | 0x01,
	// PIV |Association | Identifier Type
	0x00 | 0x00 | 0x02,
	0x00,//Reserved
	0x08,//Identifier Length is set to 08h
	//Byte0~Byte7, T10 Vendor Identification
	0x00,0xE0,0x4C,0xFF,0xFF,0xFF,0xFF,0x00, // Realtek IEEE-ID: 0x00, 0xE0, 0x4c

	/* SCSI name string */
	// Protocol Identifier | Code Set
	0x00 | 0x03,
	// PIV |Association | Identifier Type
	0x00 | 0x00 | 0x08,
	0x00,//Reserved
	0x08,//Identifier Length is set to 08h
	//Byte0~Byte7, T10 Vendor Identification
	0x72,0x65,0x61,0x6c,0x74,0x65,0x6b,0x20, //Realtek T10 ID: realtek(0x72,0x65,0x61,0x6c,0x74,0x65,0x6b)

}; /* Device Identification VPD page   */


static unsigned char inquiry_extended[] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// Vendor Specific
	0, //  CLOCKING/QAS/IUS
	0, // RESERVED
	0x04, 0x60, 	// SPC-4
	0x04, 0xC0,	// SBC-3
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0	// Reserved
};

static int inquiry(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	unsigned int lun = SCSI_LUN(srb);
	char *inquiry_default = (char *)"Generic-xD/SD/M.S.      1.00 ";
	char *inquiry_sdms =   (char *)"Generic-SD/M.S.         1.00 ";
	char *inquiry_sd =      (char *)"Generic-SD              1.00 ";
	char *inquiry_string;
	unsigned char sendbytes;
	unsigned char *buf, *ptr, *vendor_str;
	u8 card = get_lun_card(chip, lun);
	u8 page_code, evpd;
	int pro_formatter_flag = 0;
	int finish_inquiry = 0;

	if((!chip->option.support_ms) && (!chip->option.support_xd)) {
		inquiry_string = inquiry_sd;
	} else if(!chip->option.support_xd) {
		inquiry_string = inquiry_sdms;
	} else {
		inquiry_string = inquiry_default;
	}

	evpd = srb->cmnd[1] & 0x01;
	page_code = srb->cmnd[2];
	RTS51X_DEBUGP(("EVPD: 0x%02x, PAGE CODE: 0x%02x\n", evpd, page_code));

	/* SPC4, 6.4.1, page 308
	 * If the page code field is not set to zero when the EVPD bit is set to zero,
	 * the command shall be terminated with CHECK CONDITION STATUS, with the
	 * sense key set to ILLEGAL REQUEST, and the addtional sense code set to
	 * INVALID FIELD IN CDB
	 */
	if ((0 == evpd) && (0 != page_code)) {
		set_sense_type(chip, SCSI_LUN(srb), SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if (1 == evpd) {
		if ((page_code != 0x80) && (page_code != 0x83) && (page_code != 0x00)) {
			set_sense_type(chip, SCSI_LUN(srb),
					SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
	}

	buf = vmalloc(scsi_bufflen(srb));
	if (buf == NULL) {
		TRACE_RET(chip, TRANSPORT_ERROR);
	}

	if (MS_FORMATTER_ENABLED(chip) &&
			(get_lun2card(chip, lun) & MS_CARD)) {
		if (!card || (card == MS_CARD)) {
			pro_formatter_flag = 1;
		}
	}

	switch (page_code) {
	case 0x83:
		if (scsi_bufflen(srb) < 56)
			sendbytes = (unsigned char)(scsi_bufflen(srb));
		else
			sendbytes = 56;
		memcpy(buf, inquiry_device_identification, sendbytes);
		finish_inquiry = 1;
		break;

	case 0x80:
		if (scsi_bufflen(srb) < 20)
			sendbytes = (unsigned char)(scsi_bufflen(srb));
		else
			sendbytes = 20;
		memcpy(buf, inquiry_unit_serial_num, sendbytes);
		finish_inquiry = 1;
		break;

	case 0x00:
		if (evpd) {
			if (scsi_bufflen(srb) < 7)
				sendbytes = (unsigned char)(scsi_bufflen(srb));
			else
				sendbytes = 7;
			memcpy(buf, inquiry_supported_vpd_pages, sendbytes);
			finish_inquiry = 1;
		} else {
			RTS51X_DEBUGP(("Standard inquiry flow\n"));
		}
		break;

	default:
		RTS51X_DEBUGP(("Standard inquiry flow\n"));
	}

	if (finish_inquiry)
		goto finish;

	if (scsi_bufflen(srb) < 96)
		sendbytes = (unsigned char)(scsi_bufflen(srb));
	else
		sendbytes = 96;
	ptr = buf;

	// Send byte 0~7
	if (sendbytes > 8) {
		memcpy(ptr, inquiry_standard_header, 8);
		ptr += 8;
	} else {
		memcpy(ptr, inquiry_standard_header, sendbytes);
		goto finish;
	}

	// Send byte 8~35
	if (sendbytes > 36) {
		memcpy(ptr, inquiry_string, 28);
		ptr += 28;
	} else {
		memcpy(ptr, inquiry_string, sendbytes - 8);
		goto finish;
	}

	// Send byte 36~55
	if (pro_formatter_flag)
		vendor_str = formatter_inquiry_str;
	else
		vendor_str = inquiry_extended;
	if (sendbytes > 56) {
		memcpy(ptr, vendor_str, 20);
		ptr += 20;
	} else {
		memcpy(ptr, vendor_str, sendbytes - 36);
		goto finish;
	}

	// Send byte 56~95
	memcpy(ptr, inquiry_extended + 20, sendbytes - 56);

finish:
	scsi_set_resid(srb, scsi_bufflen(srb) - sendbytes);

	rts51x_set_xfer_buf(buf, scsi_bufflen(srb), srb);
	vfree(buf);

	return TRANSPORT_GOOD;
}


static int start_stop_unit(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	unsigned int lun = SCSI_LUN(srb);

	scsi_set_resid(srb, scsi_bufflen(srb));

	if (srb->cmnd[1] == 1) {
		return TRANSPORT_GOOD;
	}

	switch (srb->cmnd[0x4]) {
	        case STOP_MEDIUM:
			// Media disabled
			return TRANSPORT_GOOD;

	        case UNLOAD_MEDIUM:
			// Media shall be unload
			if (check_card_ready(chip, lun)) {
				eject_card(chip, lun);
			}
            		return TRANSPORT_GOOD;

	        case MAKE_MEDIUM_READY:
        	case LOAD_MEDIUM:
			if (check_card_ready(chip, lun)) {
				return TRANSPORT_GOOD;
			} else {
				set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
				TRACE_RET(chip, TRANSPORT_FAILED);
			}

			break;

		default:
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_CODE);
			TRACE_RET(chip, TRANSPORT_FAILED);
	}

	TRACE_RET(chip, TRANSPORT_ERROR);
}

static int allow_medium_removal(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	int prevent;

	prevent = srb->cmnd[4] & 0x1;

	scsi_set_resid(srb, 0);

	if (prevent) {
		set_sense_type(chip, SCSI_LUN(srb), SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	return TRANSPORT_GOOD;
}

static void ms_mode_sense(struct rts51x_chip *chip, u8 cmd,
		int lun, u8 *buf, int buf_len)
{
	struct ms_info *ms_card = &(chip->ms_card);
	int sys_info_offset;
	int data_size = buf_len;
	int support_format = 0;
	int i = 0;

	if (cmd == MODE_SENSE) {
		sys_info_offset = 8;
		if (data_size > 0x68) {
			data_size = 0x68;
		}
		buf[i++] = 0x67;  // Mode Data Length
	} else {
		sys_info_offset = 12;
		if (data_size > 0x6C) {
			data_size = 0x6C;
		}
		buf[i++] = 0x00;  // Mode Data Length (MSB)
		buf[i++] = 0x6A;  // Mode Data Length (LSB)
	}

	// Medium Type Code
	if (check_card_ready(chip, lun)) {
		if (CHK_MSXC(ms_card)) {
			support_format = 1;
			buf[i++] = 0x40;
		} else if (CHK_MSPRO(ms_card)) {
			support_format = 1;
			buf[i++] = 0x20;
		} else {
			buf[i++] = 0x10;
		}

		// WP
		if (check_card_wp(chip, lun)) {
			buf[i++] = 0x80;
		} else {
			buf[i++] = 0x00;
		}
	} else {
		buf[i++] = 0x00;	// MediaType
		buf[i++] = 0x00;	// WP
	}

	buf[i++] = 0x00;		// Reserved

	if (cmd == MODE_SENSE_10) {
		buf[i++] = 0x00;  // Reserved
		buf[i++] = 0x00;  // Block descriptor length(MSB)
		buf[i++] = 0x00;  // Block descriptor length(LSB)

		// The Following Data is the content of "Page 0x20"
		if (data_size >= 9) {
			buf[i++] = 0x20;		// Page Code
		}
		if (data_size >= 10) {
			buf[i++] = 0x62;		// Page Length
		}
		if (data_size >= 11) {
			buf[i++] = 0x00;		// No Access Control
		}
		if (data_size >= 12) {
			if (support_format) {
				buf[i++] = 0xC0;	// SF, SGM
			} else {
				buf[i++] = 0x00;
			}
		}
	} else {
		// The Following Data is the content of "Page 0x20"
		if (data_size >= 5) {
			buf[i++] = 0x20;		// Page Code
		}
		if (data_size >= 6) {
			buf[i++] = 0x62;		// Page Length
		}
		if (data_size >= 7) {
			buf[i++] = 0x00;		// No Access Control
		}
		if (data_size >= 8) {
			if (support_format) {
				buf[i++] = 0xC0;	// SF, SGM
			} else {
				buf[i++] = 0x00;
			}
		}
	}

	if (data_size > sys_info_offset) {
		// 96 Bytes Attribute Data
		int len = data_size - sys_info_offset;
		len = (len < 96) ? len : 96;

		memcpy(buf + sys_info_offset, ms_card->raw_sys_info, len);
	}
}

static int fill_mode_sense(struct rts51x_chip *chip, unsigned int lun,
		unsigned char *buf, int buf_len, unsigned char page_code)
{
	int size = 0;
	unsigned char *ptr;

	switch (page_code) {
	case 0x3F:
	case 0x00:
		size = 48;
		ptr = buf;
		*ptr++ = 0x2F;
		*ptr++ = 0x00;
		if (check_card_wp(chip, lun))
			*ptr++ = 0x80;
		else
			*ptr++ = 0x00;
		*ptr++ = 0x00;

		if (buf_len >= size) {
			// Caching Mode Page, page code 0x08
			memcpy(ptr, inquiry_caching_mode, 20);
			ptr += 20;
			// Control Mode Page, page code 0x0A
			memcpy(ptr, inquiry_control_mode, 12);
			ptr += 12;
			// Information Exception Mode Page, page code 0x1C
			memcpy(ptr, inquiry_info_exception_mode, 12);
		} else {
			size = 4;
		}
		break;

	case 0x08:
		size = 24;
		ptr = buf;
		*ptr++ = 0x17;
		*ptr++ = 0x00;
		if (check_card_wp(chip, lun))
			*ptr++ = 0x80;
		else
			*ptr++ = 0x00;
		*ptr++ = 0x00;

		if (buf_len >= size)
			memcpy(ptr, inquiry_caching_mode, 20);
		else
			size = 4;
		break;

	case 0x0A:
		size = 16;
		ptr = buf;
		*ptr++ = 0x0F;
		*ptr++ = 0x00;
		if (check_card_wp(chip, lun))
			*ptr++ = 0x80;
		else
			*ptr++ = 0x00;
		*ptr++ = 0x00;

		if (buf_len >= size)
			memcpy(ptr, inquiry_control_mode, 12);
		else
			size = 4;
		break;

	case 0x1C:
		size = 16;
		ptr = buf;
		*ptr++ = 0x0F;
		*ptr++ = 0x00;
		if (check_card_wp(chip, lun))
			*ptr++ = 0x80;
		else
			*ptr++ = 0x00;
		*ptr++ = 0x00;

		if (buf_len >= size)
			memcpy(ptr, inquiry_info_exception_mode, 12);
		else
			size = 4;
		break;

	default:
		size = 4;
		buf[0] = 0x03;
		buf[1] = 0x00;
		if (check_card_wp(chip, lun))
			buf[2] = 0x80;
		else
			buf[2] = 0x00;
		buf[3] = 0x00;
	}

	return size;
}

static int fill_mode_sense_10(struct rts51x_chip *chip, unsigned int lun,
		unsigned char *buf, int buf_len, unsigned char page_code)
{
	int size = 0;
	unsigned char *ptr;

	switch (page_code) {
	case 0x3F:
	case 0x00:
		size = 52;
		ptr = buf;
		// mode data length
		*ptr++ = 0x00;
		*ptr++ = 0x32;
		// medium type
		*ptr++ = 0x00;
		// device-specific parameter
		if (check_card_wp(chip, lun))
			*ptr++ = 0x80;
		else
			*ptr++ = 0x00;
		// reverse
		*ptr++ = 0x00;
		*ptr++ = 0x00;
		// block descriptor length
		*ptr++ = 0x00;
		*ptr++ = 0x00;

		if (buf_len >= size) {
			// Caching Mode Page, page code 0x08
			memcpy(ptr, inquiry_caching_mode, 20);
			ptr += 20;
			// Control Mode Page, page code 0x0A
			memcpy(ptr, inquiry_control_mode, 12);
			ptr += 12;
			// Information Exception Mode Page, page code 0x1C
			memcpy(ptr, inquiry_info_exception_mode, 12);
		} else {
			size = 8;
		}
		break;

	case 0x08:
		size = 28;
		ptr = buf;
		// mode data length
		*ptr++ = 0x00;
		*ptr++ = 0x17;
		// medium type
		*ptr++ = 0x00;
		// device-specific parameter
		if (check_card_wp(chip, lun))
			*ptr++ = 0x80;
		else
			*ptr++ = 0x00;
		// reverse
		*ptr++ = 0x00;
		*ptr++ = 0x00;
		// block descriptor length
		*ptr++ = 0x00;
		*ptr++ = 0x00;

		if (buf_len >= size)
			memcpy(ptr, inquiry_caching_mode, 20);
		else
			size = 8;
		break;

	case 0x0A:
		size = 20;
		ptr = buf;
		// mode data length
		*ptr++ = 0x00;
		*ptr++ = 0x0F;
		// medium type
		*ptr++ = 0x00;
		// device-specific parameter
		if (check_card_wp(chip, lun))
			*ptr++ = 0x80;
		else
			*ptr++ = 0x00;
		// reverse
		*ptr++ = 0x00;
		*ptr++ = 0x00;
		// block descriptor length
		*ptr++ = 0x00;
		*ptr++ = 0x00;

		if (buf_len >= size)
			memcpy(ptr, inquiry_control_mode, 12);
		else
			size = 8;
		break;

	case 0x1C:
		size = 20;
		ptr = buf;
		// mode data length
		*ptr++ = 0x00;
		*ptr++ = 0x0F;
		// medium type
		*ptr++ = 0x00;
		// device-specific parameter
		if (check_card_wp(chip, lun))
			*ptr++ = 0x80;
		else
			*ptr++ = 0x00;
		// reverse
		*ptr++ = 0x00;
		*ptr++ = 0x00;
		// block descriptor length
		*ptr++ = 0x00;
		*ptr++ = 0x00;

		if (buf_len >= size)
			memcpy(ptr, inquiry_info_exception_mode, 12);
		else
			size = 8;
		break;

	default:
		size = 8;
		// mode data length
		buf[0] = 0x00;
		buf[1] = 0x03;
		// medium type
		buf[2] = 0x00;
		// device-specific parameter
		if (check_card_wp(chip, lun))
			buf[3] = 0x80;
		else
			buf[3] = 0x00;
		// reverse
		buf[4] = 0x00;
		buf[5] = 0x00;
		// block descriptor length
		buf[6] = 0x00;
		buf[7] = 0x00;
	}

	return size;
}

static int mode_sense(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	unsigned int lun = SCSI_LUN(srb);
	unsigned int dataSize;
	int status;
	int pro_formatter_flag;
	unsigned char pageCode, *buf;
	u8 card = get_lun_card(chip, lun);

	if (!check_card_ready(chip, lun)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
		scsi_set_resid(srb, scsi_bufflen(srb));
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	pro_formatter_flag = 0;
	dataSize = 108;
	// In Combo mode, device responses ModeSense command as a MS LUN
	// when no card is inserted
	if ((get_lun2card(chip, lun) & MS_CARD)) {
		if (!card || (card == MS_CARD)) {
			if (chip->option.mspro_formatter_enable) {
				pro_formatter_flag = 1;
			}
		}
	}

	buf = kmalloc(dataSize, GFP_KERNEL);
	if (buf == NULL) {
		TRACE_RET(chip, TRANSPORT_ERROR);
	}

	pageCode = srb->cmnd[2] & 0x3f;

	if ((pageCode == 0x3F) || (pageCode == 0x1C) ||
		(pageCode == 0x0A) || (pageCode == 0x08) ||
		(pageCode == 0x00) ||
		(pro_formatter_flag && (pageCode == 0x20))) {
		if (srb->cmnd[0] == MODE_SENSE) {
			if (pro_formatter_flag &&
					((pageCode == 0x3F) || (pageCode == 0x20))) {
				ms_mode_sense(chip, srb->cmnd[0], lun, buf, dataSize);
			} else {
				dataSize = fill_mode_sense(chip, lun, buf,
						dataSize, pageCode);
			}
		} else {
			if (pro_formatter_flag &&
					((pageCode == 0x3F) || (pageCode == 0x20))) {
				ms_mode_sense(chip, srb->cmnd[0], lun, buf, dataSize);
			} else {
				dataSize = fill_mode_sense_10(chip, lun, buf,
						dataSize, pageCode);
			}
		}
		status = TRANSPORT_GOOD;
	} else {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
		scsi_set_resid(srb, scsi_bufflen(srb));
		status = TRANSPORT_FAILED;
	}

	if (status == TRANSPORT_GOOD) {
		unsigned int len = min(scsi_bufflen(srb), dataSize);
		rts51x_set_xfer_buf(buf, len, srb);
		scsi_set_resid(srb, scsi_bufflen(srb) - len);
	}
	kfree(buf);

	return status;
}

static int request_sense(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	struct sense_data_t *sense;
	unsigned int lun = SCSI_LUN(srb);
	struct ms_info *ms_card = &(chip->ms_card);
	unsigned char *tmp, *buf;

	sense = &(chip->sense_buffer[lun]);

	if ((get_lun_card(chip, lun) == MS_CARD) && PRO_UNDER_FORMATTING(ms_card)) {
		mspro_format_sense(chip, lun);
	}

	buf = vmalloc(scsi_bufflen(srb));
	if (buf == NULL) {
		TRACE_RET(chip, TRANSPORT_ERROR);
	}

	tmp = (unsigned char *)sense;
	memcpy(buf, tmp, scsi_bufflen(srb));

	rts51x_set_xfer_buf(buf, scsi_bufflen(srb), srb);
	vfree(buf);

	scsi_set_resid(srb,0);
	// Reset Sense Data
	set_sense_type(chip, lun, SENSE_TYPE_NO_SENSE);
	return TRANSPORT_GOOD;
}

static int read_write(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
#ifdef SUPPORT_SD_LOCK
	struct sd_info *sd_card = &(chip->sd_card);
#endif
	unsigned int lun = SCSI_LUN(srb);
	int retval;
	u32 start_sec;
	u16 sec_cnt;

	if (!check_card_ready(chip, lun) || (chip->capacity[lun] == 0)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if (!check_lun_mc(chip, lun)) {
		set_lun_mc(chip, lun);
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_CHANGE);
		return TRANSPORT_FAILED;
	}

	rts51x_prepare_run(chip);
	RTS51X_SET_STAT(chip, STAT_RUN);

#ifdef SUPPORT_SD_LOCK
	if (sd_card->sd_erase_status) {
		// Accessing to any card is forbidden until the erase procedure of SD is completed
		RTS51X_DEBUGP(("SD card being erased!\n"));
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_READ_FORBIDDEN);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if (get_lun_card(chip, lun) == SD_CARD) {
		if (sd_card->sd_lock_status & SD_LOCKED) {
			RTS51X_DEBUGP(("SD card locked!\n"));
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_READ_FORBIDDEN);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
	}
#endif

	if ((srb->cmnd[0] == READ_10) || (srb->cmnd[0] == WRITE_10)) {
		start_sec = ((u32)srb->cmnd[2] << 24) | ((u32)srb->cmnd[3] << 16) |
			((u32)srb->cmnd[4] << 8) | ((u32)srb->cmnd[5]);
		sec_cnt = ((u16)(srb->cmnd[7]) << 8) | srb->cmnd[8];
	} else if ((srb->cmnd[0] == READ_6) || (srb->cmnd[0] == WRITE_6)) {
		start_sec = ((u32)(srb->cmnd[1] & 0x1F) << 16) |
			((u32)srb->cmnd[2] << 8) | ((u32)srb->cmnd[3]);
		sec_cnt = srb->cmnd[4];
	} else if ((srb->cmnd[0] == VENDOR_CMND) && (srb->cmnd[1] == SCSI_APP_CMD) &&
			((srb->cmnd[2] == PP_READ10) || (srb->cmnd[2] == PP_WRITE10))) {
		start_sec = ((u32)srb->cmnd[4] << 24) | ((u32)srb->cmnd[5] << 16) |
			((u32)srb->cmnd[6] << 8) | ((u32)srb->cmnd[7]);
		sec_cnt = ((u16)(srb->cmnd[9]) << 8) | srb->cmnd[10];
	} else {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	//sangdy2010-06-23:modify for device path exercise of DTM test will send start_sec 0xfffffffff and sec_cnt 1.
	if ((start_sec > chip->capacity[lun]) ||
			((start_sec + sec_cnt) > chip->capacity[lun])) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_LBA_OVER_RANGE);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if (sec_cnt == 0) {
		scsi_set_resid(srb, 0);
		return TRANSPORT_GOOD;
	}

	if ((srb->sc_data_direction == DMA_TO_DEVICE) && check_card_wp(chip, lun)) {
		RTS51X_DEBUGP(("Write protected card!\n"));
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_WRITE_PROTECT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	retval = card_rw(srb, chip, start_sec, sec_cnt);
	if (retval != STATUS_SUCCESS) {
// 		if (chip->need_release & chip->lun2card[lun]) {
// 			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
// 		} else {
			if (srb->sc_data_direction == DMA_FROM_DEVICE) {
				set_sense_type(chip, lun, SENSE_TYPE_MEDIA_UNRECOVER_READ_ERR);
			} else {
				set_sense_type(chip, lun, SENSE_TYPE_MEDIA_WRITE_ERR);
			}
// 		}
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	scsi_set_resid(srb, 0);

	return TRANSPORT_GOOD;
}

static int read_format_capacity(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	unsigned char *buf;
	unsigned int lun = SCSI_LUN(srb);
	unsigned int buf_len;
	u8 card = get_lun_card(chip, lun);
	int desc_cnt;
	int i = 0;

	if (!check_card_ready(chip, lun)) {
		if (!chip->option.mspro_formatter_enable) {
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
	}

// 	if (!check_lun_mc(chip, lun)) {
// 		set_lun_mc(chip, lun);
// 		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_CHANGE);
// 		return TRANSPORT_FAILED;
// 	}

	buf_len = (scsi_bufflen(srb) > 12) ? 0x14 : 12;

	buf = kmalloc(buf_len, GFP_KERNEL);
	if (buf == NULL) {
		TRACE_RET(chip, TRANSPORT_ERROR);
	}

	buf[i++] = 0;
	buf[i++] = 0;
	buf[i++] = 0;

	// Capacity List Length
	if ((buf_len > 12) && chip->option.mspro_formatter_enable &&
			(chip->lun2card[lun] & MS_CARD) &&
			(!card || (card == MS_CARD))) {
		buf[i++] = 0x10;
		desc_cnt = 2;
	} else {
		buf[i++] = 0x08;
		desc_cnt = 1;
	}

	while (desc_cnt) {
		if (check_card_ready(chip, lun)) {
			buf[i++] = (unsigned char)((chip->capacity[lun]) >> 24);
			buf[i++] = (unsigned char)((chip->capacity[lun]) >> 16);
			buf[i++] = (unsigned char)((chip->capacity[lun]) >> 8);
			buf[i++] = (unsigned char)(chip->capacity[lun]);

			if (desc_cnt == 2) {
				buf[i++] = 2;  //Byte[8]: Descriptor Type: Formatted medium
			} else {
				buf[i++] = 0;  //Byte[16]
			}
		} else {
			// If no media is present, then the Current/Maximum/Formattable
			// Capacity Descriptor parameters reports Number of Blocks set
			// to the maximum capacity of a media that the Drive is capable
			// of accessing  --- MMC-6
			buf[i++] = 0xFF;
			buf[i++] = 0xFF;
			buf[i++] = 0xFF;
			buf[i++] = 0xFF;

			if (desc_cnt == 2) {
				buf[i++] = 3;  //Byte[8]: Descriptor Type: No medium
			} else {
				buf[i++] = 0;  //Byte[16]
			}
		}

		buf[i++] = 0x00;
		buf[i++] = 0x02;
		buf[i++] = 0x00;

		desc_cnt --;
	}

	buf_len = min(scsi_bufflen(srb), buf_len);
	rts51x_set_xfer_buf(buf, buf_len, srb);
	kfree(buf);

	scsi_set_resid(srb, scsi_bufflen(srb) - buf_len);

	return TRANSPORT_GOOD;
}

static int read_capacity(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	unsigned char *buf;
	unsigned int lun = SCSI_LUN(srb);

	if (!check_card_ready(chip, lun)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if (chip->option.dev_flags & CHECK_MEDIA_CHANGE) {
		if (!check_lun_mc(chip, lun)) {
			set_lun_mc(chip, lun);
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_CHANGE);
			return TRANSPORT_FAILED;
		}
	}

	buf = kmalloc(8, GFP_KERNEL);
	if (buf == NULL) {
		TRACE_RET(chip, TRANSPORT_ERROR);
	}

	buf[0] = (unsigned char)((chip->capacity[lun] - 1) >> 24);
	buf[1] = (unsigned char)((chip->capacity[lun] - 1) >> 16);
	buf[2] = (unsigned char)((chip->capacity[lun] - 1) >> 8);
	buf[3] = (unsigned char)(chip->capacity[lun] - 1);

	buf[4] = 0x00;
	buf[5] = 0x00;
	buf[6] = 0x02;
	buf[7] = 0x00;

	rts51x_set_xfer_buf(buf, scsi_bufflen(srb), srb);
	kfree(buf);

	scsi_set_resid(srb, 0);

	return TRANSPORT_GOOD;
}

static int get_dev_status(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	unsigned int lun = SCSI_LUN(srb);
	unsigned int buf_len;
	u8 status[32] = {0};

	rts51x_pp_status(chip, lun, status, 32);

	buf_len = min(scsi_bufflen(srb), (unsigned int)sizeof(status));
	rts51x_set_xfer_buf(status, buf_len, srb);
	scsi_set_resid(srb, scsi_bufflen(srb) - buf_len);

	return TRANSPORT_GOOD;
}

static int read_status(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	u8 rts51x_status[16];
	unsigned int buf_len;
	unsigned int lun = SCSI_LUN(srb);

	rts51x_read_status(chip, lun, rts51x_status, 16);

	buf_len = min(scsi_bufflen(srb), (unsigned int)sizeof(rts51x_status));
	rts51x_set_xfer_buf(rts51x_status, buf_len, srb);
	scsi_set_resid(srb, scsi_bufflen(srb) - buf_len);

	return TRANSPORT_GOOD;
}

static int read_mem(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	unsigned int lun = SCSI_LUN(srb);
	unsigned short addr, len, i;
	int retval;
	u8 *buf;

	rts51x_prepare_run(chip);
	RTS51X_SET_STAT(chip, STAT_RUN);

	addr = ((u16)srb->cmnd[2] << 8) | srb->cmnd[3];
	len = ((u16)srb->cmnd[4] << 8) | srb->cmnd[5];

	//sangdy:Realtek burnin tool2.0.9.3 will read some memory which is not exist.It will cause MCU error
	//So filter these address.
	if (addr < 0xe000) {
		RTS51X_DEBUGP(("filter!addr=0x%x\n",addr));
		return TRANSPORT_GOOD;
	}

	buf = (u8 *)vmalloc(len);
	if (!buf) {
		TRACE_RET(chip, TRANSPORT_ERROR);
	}

	for (i = 0; i < len; i++) {
		retval = rts51x_ep0_read_register(chip, addr + i, buf + i);
		if (retval != STATUS_SUCCESS) {
			vfree(buf);
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_UNRECOVER_READ_ERR);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
	}

	len = (unsigned short)min(scsi_bufflen(srb), (unsigned int)len);
	rts51x_set_xfer_buf(buf, len, srb);
	scsi_set_resid(srb, scsi_bufflen(srb) - len);

	vfree(buf);

	return TRANSPORT_GOOD;
}

static int write_mem(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	unsigned int lun = SCSI_LUN(srb);
	unsigned short addr, len, i;
	int retval;
	u8 *buf;

	rts51x_prepare_run(chip);
	RTS51X_SET_STAT(chip, STAT_RUN);

	addr = ((u16)srb->cmnd[2] << 8) | srb->cmnd[3];
	len = ((u16)srb->cmnd[4] << 8) | srb->cmnd[5];

	//sangdy:Realtek burnin tool2.0.9.3 will read some memory which is not exist.It will cause MCU error
	//So filter these address.
	if (addr < 0xe000) {
		RTS51X_DEBUGP(("filter!addr=0x%x\n",addr));
		return TRANSPORT_GOOD;
	}

	len = (unsigned short)min(scsi_bufflen(srb), (unsigned int)len);
	buf = (u8 *)vmalloc(len);
	if (!buf) {
		TRACE_RET(chip, TRANSPORT_ERROR);
	}

	rts51x_get_xfer_buf(buf, len, srb);

	for (i = 0; i < len; i++) {
		retval = rts51x_ep0_write_register(chip, addr + i, 0xFF, buf[i]);
		if (retval != STATUS_SUCCESS) {
			vfree(buf);
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_WRITE_ERR);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
	}

	vfree(buf);
	scsi_set_resid(srb, scsi_bufflen(srb) - len);

	return TRANSPORT_GOOD;
}

static int get_sd_csd(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	struct sd_info *sd_card = &(chip->sd_card);
	unsigned int lun = SCSI_LUN(srb);

	if (!check_card_ready(chip, lun)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if (get_lun_card(chip, lun) != SD_CARD) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_UNRECOVER_READ_ERR);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	scsi_set_resid(srb, 0);
	rts51x_set_xfer_buf(sd_card->raw_csd, scsi_bufflen(srb), srb);

	return TRANSPORT_GOOD;
}

static int read_phy_register(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	int retval;
	u8 addr, len, i;
	u8 *buf;

	rts51x_prepare_run(chip);
	RTS51X_SET_STAT(chip, STAT_RUN);

	addr = srb->cmnd[5];
	len = srb->cmnd[7];

	if (len) {
		buf = (u8 *)vmalloc(len);
		if (!buf) {
			TRACE_RET(chip, TRANSPORT_ERROR);
		}

		for (i = 0; i < len; i++) {
			retval = rts51x_read_phy_register(chip, addr + i, buf + i);
			if (retval != STATUS_SUCCESS) {
				vfree(buf);
				set_sense_type(chip, SCSI_LUN(srb), SENSE_TYPE_MEDIA_UNRECOVER_READ_ERR);
				TRACE_RET(chip, TRANSPORT_FAILED);
			}
		}

		len = min(scsi_bufflen(srb), (unsigned int)len);
		rts51x_set_xfer_buf(buf, len, srb);
		scsi_set_resid(srb, scsi_bufflen(srb) - len);

		vfree(buf);
	}

	return TRANSPORT_GOOD;
}

static int write_phy_register(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	int retval;
	u8 addr, len, i;
	u8 *buf;

	rts51x_prepare_run(chip);
	RTS51X_SET_STAT(chip, STAT_RUN);

	addr = srb->cmnd[5];
	len = srb->cmnd[7];

	if (len) {
		len = min(scsi_bufflen(srb), (unsigned int)len);

		buf = (u8 *)vmalloc(len);
		if (buf == NULL) {
			TRACE_RET(chip, TRANSPORT_ERROR);
		}

		rts51x_get_xfer_buf(buf, len, srb);
		scsi_set_resid(srb, scsi_bufflen(srb) - len);

		for (i = 0; i < len; i++) {
			retval = rts51x_write_phy_register(chip, addr + i, buf[i]);
			if (retval != STATUS_SUCCESS) {
				vfree(buf);
				set_sense_type(chip, SCSI_LUN(srb), SENSE_TYPE_MEDIA_WRITE_ERR);
				TRACE_RET(chip, TRANSPORT_FAILED);
			}
		}

		vfree(buf);
	}

	return TRANSPORT_GOOD;
}

static int get_card_bus_width(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	unsigned int lun = SCSI_LUN(srb);
	u8 card, bus_width;

	if (!check_card_ready(chip, lun)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	card = get_lun_card(chip, lun);
	if ((card == SD_CARD) || (card == MS_CARD)) {
		bus_width = chip->card_bus_width[lun];
	} else {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_UNRECOVER_READ_ERR);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	scsi_set_resid(srb, 0);
	rts51x_set_xfer_buf(&bus_width, scsi_bufflen(srb), srb);

	return TRANSPORT_GOOD;
}

#ifdef _MSG_TRACE
static int trace_msg_cmd(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	unsigned char *buf = NULL;
	u8 clear;
       	unsigned int buf_len;

	buf_len = 4 + ((2 + MSG_FUNC_LEN + MSG_FILE_LEN + TIME_VAL_LEN) * TRACE_ITEM_CNT);

	if ((scsi_bufflen(srb) < buf_len) || (scsi_sglist(srb) == NULL)) {
		set_sense_type(chip, SCSI_LUN(srb), SENSE_TYPE_MEDIA_UNRECOVER_READ_ERR);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	clear = srb->cmnd[2];

	buf = (unsigned char *)vmalloc(scsi_bufflen(srb));
	if (buf == NULL) {
		TRACE_RET(chip, TRANSPORT_ERROR);
	}

	rts51x_trace_msg(chip, buf, clear);

	rts51x_set_xfer_buf(buf, scsi_bufflen(srb), srb);
	vfree(buf);

	scsi_set_resid(srb, 0);
	return TRANSPORT_GOOD;
}
#endif

// { wwang, 2010-04-27
// Add batch command
static int rw_mem_cmd_buf(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	int retval =  STATUS_SUCCESS;
	unsigned int lun = SCSI_LUN(srb);
	u8 cmd_type, mask, value, idx, mode, len;
	u16 addr;
	u32 timeout;

	rts51x_prepare_run(chip);
	RTS51X_SET_STAT(chip, STAT_RUN);

	switch (srb->cmnd[3]) {
	case INIT_BATCHCMD:
		rts51x_init_cmd(chip);
		break;

	case ADD_BATCHCMD:
		cmd_type = srb->cmnd[4];
		if (cmd_type > 2) {
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
		addr = (srb->cmnd[5] << 8) | srb->cmnd[6];
		mask = srb->cmnd[7];
		value = srb->cmnd[8];
		rts51x_add_cmd(chip, cmd_type, addr, mask, value);
		break;

	case SEND_BATCHCMD:
		mode = srb->cmnd[4];
		len = srb->cmnd[5];
		timeout = ((u32)srb->cmnd[6] << 24) | ((u32)srb->cmnd[7] << 16) |
			((u32)srb->cmnd[8] << 8) | ((u32)srb->cmnd[9]);
		retval = rts51x_send_cmd(chip, mode, 1000);
		if (retval != STATUS_SUCCESS) {
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_WRITE_ERR);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
		if (mode & STAGE_R) {
			retval = rts51x_get_rsp(chip, len, timeout);
			if (retval != STATUS_SUCCESS) {
				set_sense_type(chip, lun, SENSE_TYPE_MEDIA_UNRECOVER_READ_ERR);
				TRACE_RET(chip, TRANSPORT_FAILED);
			}
		}
		break;

	case GET_BATCHRSP:
		idx = srb->cmnd[4];
		value = chip->rsp_buf[idx];
		if (scsi_bufflen(srb) < 1) {
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
		rts51x_set_xfer_buf(&value, 1, srb);
		scsi_set_resid(srb, 0);
		break;

	default:
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if (retval != STATUS_SUCCESS) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_WRITE_ERR);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	return TRANSPORT_GOOD;
}

static int suit_cmd(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	int result;

	switch (srb->cmnd[3]) {
	case INIT_BATCHCMD:
	case ADD_BATCHCMD:
	case SEND_BATCHCMD:
	case GET_BATCHRSP:
		result = rw_mem_cmd_buf(srb, chip);
		break;
	default:
		result = TRANSPORT_ERROR;
	}

	return result;
}
// } wwang, 2010-04-27

static int app_cmd(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	int result;

	switch (srb->cmnd[2]) {
		case PP_READ10:
		case PP_WRITE10:
			result = read_write(srb, chip);
			break;

		// { wwang, 2010-04-27
		// Add batch command
		case SUIT_CMD:
			result = suit_cmd(srb, chip);
			break;
		// } wwang, 2010-04-27

		case READ_PHY:
			result = read_phy_register(srb, chip);
			break;

		case WRITE_PHY:
			result = write_phy_register(srb, chip);
			break;

		case GET_DEV_STATUS:
			result = get_dev_status(srb, chip);
			break;

		default:
			set_sense_type(chip, SCSI_LUN(srb), SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
			TRACE_RET(chip, TRANSPORT_FAILED);
	}

	return result;
}

static int vendor_cmnd(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	int result = TRANSPORT_GOOD;

	switch (srb->cmnd[1]) {
		case READ_STATUS:
			result = read_status(srb, chip);
			break;

		case READ_MEM:
			result = read_mem(srb, chip);
			break;

		case WRITE_MEM:
			result = write_mem(srb, chip);
			break;

		case GET_BUS_WIDTH:
			result = get_card_bus_width(srb, chip);
			break;

		case GET_SD_CSD:
			result = get_sd_csd(srb, chip);
			break;

#ifdef _MSG_TRACE
		case TRACE_MSG:
			result = trace_msg_cmd(srb, chip);
			break;
#endif

		case SCSI_APP_CMD:
			result = app_cmd(srb, chip);
			break;

		default:
			set_sense_type(chip, SCSI_LUN(srb), SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
			TRACE_RET(chip, TRANSPORT_FAILED);
	}

	return result;
}

static int ms_format_cmnd(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	unsigned int lun = SCSI_LUN(srb);
	int retval, quick_format;

	if (get_lun_card(chip, lun) != MS_CARD) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_LUN_NOT_SUPPORT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if ((srb->cmnd[3] != 0x4D) || (srb->cmnd[4] != 0x47) || (srb->cmnd[5] != 0x66) ||
			(srb->cmnd[6] != 0x6D) || (srb->cmnd[7] != 0x74)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if (srb->cmnd[8] & 0x01) {
		quick_format = 0;
	} else {
		quick_format = 1;
	}

	if (!(chip->card_ready & MS_CARD)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if (chip->card_wp & MS_CARD) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_WRITE_PROTECT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if (!CHK_MSPRO(ms_card)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_LUN_NOT_SUPPORT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	rts51x_prepare_run(chip);
	RTS51X_SET_STAT(chip, STAT_RUN);

	retval = mspro_format(srb, chip, MS_SHORT_DATA_LEN, quick_format);
	if (retval != STATUS_SUCCESS) {
		set_sense_type(chip, lun, SENSE_TYPE_FORMAT_CMD_FAILED);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	scsi_set_resid(srb, 0);
	return TRANSPORT_GOOD;
}

#ifdef SUPPORT_PCGL_1P18
int get_ms_information(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	unsigned int lun = SCSI_LUN(srb);
	u8 dev_info_id, data_len;
	u8 *buf;
	unsigned int buf_len;
	int i;

	if (!check_card_ready(chip, lun)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}
	if ((get_lun_card(chip, lun) != MS_CARD)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_LUN_NOT_SUPPORT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if ((srb->cmnd[2] != 0xB0) || (srb->cmnd[4] != 0x4D) ||
		(srb->cmnd[5] != 0x53) || (srb->cmnd[6] != 0x49) ||
		(srb->cmnd[7] != 0x44)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	dev_info_id = srb->cmnd[3];
	if ((CHK_MSXC(ms_card) && (dev_info_id == 0x10)) ||
			(!CHK_MSXC(ms_card) && (dev_info_id == 0x13)) ||
			!CHK_MSPRO(ms_card)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if (dev_info_id == 0x15) {
		buf_len = data_len = 0x3A;
	} else {
		buf_len = data_len = 0x6A;
	}

	buf = (u8 *)kmalloc(buf_len, GFP_KERNEL);
	if (!buf) {
		TRACE_RET(chip, TRANSPORT_ERROR);
	}

	i = 0;
	//  GET Memory Stick Media Information Response Header
	//
	buf[i++] = 0x00;		// Data length MSB
	buf[i++] = data_len; 		// Data length LSB
	// Device Information Type Code
	if (CHK_MSXC(ms_card)) {
		buf[i++] = 0x03;
	} else {
		buf[i++] = 0x02;
	}
	// SGM bit
	buf[i++] = 0x01;
	// Reserved
	buf[i++] = 0x00;
	buf[i++] = 0x00;
	buf[i++] = 0x00;
	// Number of Device Information
	buf[i++] = 0x01;

	//  Device Information Body
	//
	// Device Information ID Number
	buf[i++] = dev_info_id;
	// Device Information Length
	if (dev_info_id == 0x15) {
		data_len = 0x31;
	} else {
		data_len = 0x61;
	}
	buf[i++] = 0x00;		// Data length MSB
	buf[i++] = data_len; 		// Data length LSB
	// Valid Bit
	buf[i++] = 0x80;
	if ((dev_info_id == 0x10) || (dev_info_id == 0x13)) {
		// System Information
		memcpy(buf+i, ms_card->raw_sys_info, 96);
	} else {
		// Model Name
		memcpy(buf+i, ms_card->raw_model_name, 48);
	}

	rts51x_set_xfer_buf(buf, buf_len, srb);

	if (dev_info_id == 0x15) {
		scsi_set_resid(srb, scsi_bufflen(srb)-0x3C);
	} else {
		scsi_set_resid(srb, scsi_bufflen(srb)-0x6C);
	}

	kfree(buf);
	return STATUS_SUCCESS;
}
#endif

static int ms_sp_cmnd(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	int retval = TRANSPORT_ERROR;

	if (srb->cmnd[2] == MS_FORMAT) {
		retval = ms_format_cmnd(srb, chip);
	}
#ifdef SUPPORT_PCGL_1P18
	else if (srb->cmnd[2] == GET_MS_INFORMATION) {
		retval = get_ms_information(srb, chip);
	}
#endif

	return retval;
}

#ifdef SUPPORT_CPRM
static int sd_extention_cmnd(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	unsigned int lun = SCSI_LUN(srb);
	int result;

	rts51x_prepare_run(chip);
	RTS51X_SET_STAT(chip, STAT_RUN);

	sd_cleanup_work(chip);

	if (!check_card_ready(chip, lun)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}
	if ((get_lun_card(chip, lun) != SD_CARD)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_LUN_NOT_SUPPORT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	switch (srb->cmnd[0]) {
		case SD_PASS_THRU_MODE:
			result = sd_pass_thru_mode(srb, chip);
			break;

		case SD_EXECUTE_NO_DATA:
			result = sd_execute_no_data(srb, chip);
			break;

		case SD_EXECUTE_READ:
			result = sd_execute_read_data(srb, chip);
			break;

		case SD_EXECUTE_WRITE:
			result = sd_execute_write_data(srb, chip);
			break;

		case SD_GET_RSP:
			result = sd_get_cmd_rsp(srb, chip);
			break;

		case SD_HW_RST:
			result = sd_hw_rst(srb, chip);
			break;

		default:
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
			TRACE_RET(chip, TRANSPORT_FAILED);
	}

	return result;
}
#endif

#ifdef SUPPORT_MAGIC_GATE
int mg_report_key(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	unsigned int lun = SCSI_LUN(srb);
	int retval;
	u8 key_format;

	rts51x_prepare_run(chip);
	RTS51X_SET_STAT(chip, STAT_RUN);

	ms_cleanup_work(chip);

	if (!check_card_ready(chip, lun)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}
	if ((get_lun_card(chip, lun) != MS_CARD)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_LUN_NOT_SUPPORT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if (srb->cmnd[7] != KC_MG_R_PRO) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	// The Media is not MSPro, is MemoryStick, we don't support this type.
	if (!CHK_MSPRO(ms_card)) {
		set_sense_type(chip, lun, SENSE_TYPE_MG_INCOMPATIBLE_MEDIUM);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	key_format = srb->cmnd[10] & 0x3F;

	switch (key_format) {
	case KF_GET_LOC_EKB:
		if ((scsi_bufflen(srb) == 0x41C) &&
			(srb->cmnd[8] == 0x04) &&
			(srb->cmnd[9] == 0x1C))
		{
			retval = mg_get_local_EKB(srb, chip);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, TRANSPORT_FAILED);
			}
		} else {
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
		break;

	case KF_RSP_CHG:
		if ((scsi_bufflen(srb) == 0x24) &&
			(srb->cmnd[8] == 0x00) &&
			(srb->cmnd[9] == 0x24))
		{
			retval = mg_get_rsp_chg(srb, chip);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, TRANSPORT_FAILED);
			}
		} else {
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
		break;

	case KF_GET_ICV:
		ms_card->mg_entry_num = srb->cmnd[5];
		if ((scsi_bufflen(srb) == 0x404) &&
			(srb->cmnd[8] == 0x04) &&
			(srb->cmnd[9] == 0x04) &&
			(srb->cmnd[2] == 0x00) &&
			(srb->cmnd[3] == 0x00) &&
			(srb->cmnd[4] == 0x00) &&
			(srb->cmnd[5] < 32))
		{
			retval = mg_get_ICV(srb, chip);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, TRANSPORT_FAILED);
			}
		} else {
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
		break;

	default:
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	scsi_set_resid(srb, 0);
	return TRANSPORT_GOOD;
}

int mg_send_key(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	struct ms_info *ms_card = &(chip->ms_card);
	unsigned int lun = SCSI_LUN(srb);
	int retval;
	u8 key_format;

	rts51x_prepare_run(chip);
	RTS51X_SET_STAT(chip, STAT_RUN);

	ms_cleanup_work(chip);

	if (!check_card_ready(chip, lun)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_NOT_PRESENT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}
	if (check_card_wp(chip, lun)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_WRITE_PROTECT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}
	if ((get_lun_card(chip, lun) != MS_CARD)) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_LUN_NOT_SUPPORT);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	if (srb->cmnd[7] != KC_MG_R_PRO) {
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	// The Media is not MSPro, is MemoryStick, we don't support this type.
	if (!CHK_MSPRO(ms_card)) {
		set_sense_type(chip, lun, SENSE_TYPE_MG_INCOMPATIBLE_MEDIUM);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	key_format = srb->cmnd[10] & 0x3F;

	switch (key_format) {
	case KF_SET_LEAF_ID:
		if ((scsi_bufflen(srb) == 0x0C) &&
			(srb->cmnd[8] == 0x00) &&
			(srb->cmnd[9] == 0x0C))
		{
			retval = mg_set_leaf_id(srb, chip);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, TRANSPORT_FAILED);
			}
		} else {
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
		break;

	case KF_CHG_HOST:
		if ((scsi_bufflen(srb) == 0x0C) &&
			(srb->cmnd[8] == 0x00) &&
			(srb->cmnd[9] == 0x0C))
		{
			retval = mg_chg(srb, chip);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, TRANSPORT_FAILED);
			}
		} else {
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
		break;

	case KF_RSP_HOST:
		if ((scsi_bufflen(srb) == 0x0C) &&
			(srb->cmnd[8] == 0x00) &&
			(srb->cmnd[9] == 0x0C))
		{
			retval = mg_rsp(srb, chip);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, TRANSPORT_FAILED);
			}
		} else {
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
		break;

	case KF_SET_ICV:
		ms_card->mg_entry_num = srb->cmnd[5];
		if ((scsi_bufflen(srb) == 0x404) &&
			(srb->cmnd[8] == 0x04) &&
			(srb->cmnd[9] == 0x04) &&
			(srb->cmnd[2] == 0x00) &&
			(srb->cmnd[3] == 0x00) &&
			(srb->cmnd[4] == 0x00) &&
			(srb->cmnd[5] < 32))
		{
			retval = mg_set_ICV(srb, chip);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, TRANSPORT_FAILED);
			}
		} else {
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
		break;

	default:
		set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	scsi_set_resid(srb, 0);
	return TRANSPORT_GOOD;
}
#endif

static int verify(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
	unsigned int lun = SCSI_LUN(srb);
	u32 start_sec = 0;
	u32 sec_cnt = 0;

	start_sec = ((u32)srb->cmnd[2]<<24) | ((u32)srb->cmnd[3]<<16)
		| ((u32)srb->cmnd[4]<<8) | ((u32)srb->cmnd[5]);
	sec_cnt = ((u32)srb->cmnd[7]<<8) | ((u32)srb->cmnd[8]);

	if ((start_sec > chip->capacity[lun]) ||
			((start_sec + sec_cnt) > chip->capacity[lun])) {
		set_sense_type(chip, SCSI_LUN(srb),
				SENSE_TYPE_MEDIA_INVALID_CMD_FIELD);
		TRACE_RET(chip, TRANSPORT_FAILED);
	}

	return TRANSPORT_GOOD;
}

int rts51x_scsi_handler(struct scsi_cmnd *srb, struct rts51x_chip *chip)
{
#ifdef SUPPORT_SD_LOCK
	struct sd_info *sd_card = &(chip->sd_card);
#endif
	struct ms_info *ms_card = &(chip->ms_card);
	unsigned int lun = SCSI_LUN(srb);
	int result = TRANSPORT_GOOD;

#ifdef SUPPORT_SD_LOCK
	if (sd_card->sd_erase_status) {
		// Block all SCSI command except for REQUEST_SENSE and rs_ppstatus
		if (!((srb->cmnd[0] == VENDOR_CMND) && (srb->cmnd[1] == SCSI_APP_CMD) &&
				(srb->cmnd[2] == GET_DEV_STATUS)) &&
				(srb->cmnd[0] != REQUEST_SENSE)) {
			// Logical Unit Not Ready Format in Progress
			set_sense_data(chip, lun, CUR_ERR, 0x02, 0, 0x04, 0x04, 0, 0);
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
	}
#endif

	if ((get_lun_card(chip, lun) == MS_CARD) &&
			(ms_card->format_status == FORMAT_IN_PROGRESS)) {
		if ((srb->cmnd[0] != REQUEST_SENSE) && (srb->cmnd[0] != INQUIRY)) {
			// Logical Unit Not Ready Format in Progress
			set_sense_data(chip, lun, CUR_ERR, 0x02, 0, 0x04, 0x04,
					0, (u16)(ms_card->progress));
			TRACE_RET(chip, TRANSPORT_FAILED);
		}
	}

	switch (srb->cmnd[0]) {
		case READ_10:
		case WRITE_10:
		case READ_6:
		case WRITE_6:
			result = read_write(srb, chip);
			break;

		case TEST_UNIT_READY:
			result = test_unit_ready(srb, chip);
			break;

		case INQUIRY:
			result = inquiry(srb, chip);
			break;

		case READ_CAPACITY:
			result = read_capacity(srb, chip);
			break;

		case START_STOP:
			result = start_stop_unit(srb, chip);
			break;

		case ALLOW_MEDIUM_REMOVAL:
			result = allow_medium_removal(srb, chip);
			break;

		case REQUEST_SENSE:
			result = request_sense(srb, chip);
			break;

		case MODE_SENSE:
		case MODE_SENSE_10:
			result = mode_sense(srb, chip);
			break;

		case 0x23:
			result = read_format_capacity(srb, chip);
			break;

		case VENDOR_CMND:
			result = vendor_cmnd(srb, chip);
			break;

		case MS_SP_CMND:
			result = ms_sp_cmnd(srb, chip);
			break;

#ifdef SUPPORT_CPRM
		case SD_PASS_THRU_MODE:
		case SD_EXECUTE_NO_DATA:
		case SD_EXECUTE_READ:
		case SD_EXECUTE_WRITE:
		case SD_GET_RSP:
		case SD_HW_RST:
			result = sd_extention_cmnd(srb, chip);
			break;
#endif

#ifdef SUPPORT_MAGIC_GATE
		case CMD_MSPRO_MG_RKEY:
			result = mg_report_key(srb, chip);
			break;

		case CMD_MSPRO_MG_SKEY:
			result = mg_send_key(srb, chip);
			break;
#endif

		case VERIFY:
			result = verify(srb, chip);
			break;

		case FORMAT_UNIT:
		case MODE_SELECT:
			result = TRANSPORT_GOOD;
			break;

		default:
			set_sense_type(chip, lun, SENSE_TYPE_MEDIA_INVALID_CMD_CODE);
			result = TRANSPORT_FAILED;
	}

	return result;
}
