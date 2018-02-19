/* Realtek Card Reader Library
 * Main API entry point
 *
 * Copyright(c) 2009 Realtek Semiconductor Corp. All rights reserved.  
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published 
 * by the Free Software Foundation; either version 2.1 of the License, 
 * or (at your option) any later version.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <stdint.h>

#include "sg_ctl.h"
#include "rtcr.h"

/**
 * rts_open - Open a LUN of a card reader.
 * @dev: device number, counted from zero
 * @lun: logical unit number, counted from zero
 *
 * Given a device number and logical unit number, 
 * rts_open() returns a file descriptor, a small, non-negative integer 
 * for use in subsequent function calls (rts_test_unit_ready, etc.).   
 * The file descriptor returned by a successful call will be the 
 * lowest-numbered file descriptor not currently open for the process.
 */
int rts_open(int dev, int lun)
{
	return rts_open_get_name(dev, lun, NULL, 0);
}

/**
 * rts_open_get_name - Open a LUN of a card reader, and get the device name.
 * @dev: device number, counted from zero
 * @lun: logical unit number, counted from zero
 * @buf: buffer to store device node name
 * @buf_len: buffer length
 *
 * Given a device number and logical unit number, 
 * rts_open_get_name() returns a file descriptor, a small, non-negative integer 
 * for use in subsequent function calls (rts_test_unit_ready, etc.).   
 * The file descriptor returned by a successful call will be the 
 * lowest-numbered file descriptor not currently open for the process.
 */
int rts_open_get_name(int dev, int lun, char *buf, int buf_len)
{
	int dev_num, i, fd;
	SG_DEVICE *sg_dev, *ptr;

	if (sg_scan_device(&dev_num, &sg_dev) < 0) {
		return -1;
	}

	if (dev > dev_num - 1) {
		sg_release_device(sg_dev);
		return -1;
	}

	ptr = sg_dev;
	for (i = 0; i < dev; i ++) {
		if (ptr->next == NULL) {
			sg_release_device(sg_dev);
			return -1;
		}
		ptr = ptr->next;
	}
	
	if (buf && buf_len) {
		strncpy(buf, ptr->name[lun], buf_len - 1);
	}
	
	fd = sg_open_devfs(ptr, lun, O_RDWR);
	sg_release_device(sg_dev);

	return fd;
}

/**
 * rts_open_name - Open the device name of a SCSI device.
 * @dev_name: device name
 *
 * Given the device name of a SCSI device,
 * rts_open_name() returns a file descriptor, a small, non-negative integer 
 * for use in subsequent function calls (rts_test_unit_ready, etc.).   
 * The file descriptor returned by a successful call will be the 
 * lowest-numbered file descriptor not currently open for the process.
 */
int rts_open_name(const char *dev_name) 
{
	return open(dev_name, O_RDWR);
}

/**
 * rts_close - Close a LUN of a card reader.
 * @fd: file descriptor
 *
 * rts_close() closes a file descriptor, so that it no longer 
 * refers to any file and may be reused.
 */
void rts_close(int fd)
{
	sg_close_devfs(fd);
}

/**
 * rts_handle_scsi_cmnd - Handle SCSI command.
 * @fd: file descriptor
 * @direction: Data direction
 * @cmd_len: length of SCSI command
 * @cmd: SCSI command buffer
 * @sb_len: length of sense buffer
 * @sense: sense buffer
 * @buf_size: length of data buffer
 * @buf: data buffer
 * @resid: residue
 * @timeout: time out in millisecond
 *
 * rts_handle_scsi_cmnd() is used to handle any SCSI command.
 */
int rts_handle_scsi_cmnd(int fd, int direction,
			int cmd_len, unsigned char *cmd,
			int sb_len, unsigned char *sense,
			int buf_size, unsigned char *buf,
			int *resid, int timeout)
{
	return handle_scsi_cmnd(fd, direction, cmd_len, cmd, 
			sb_len, sense, buf_size, buf, resid, timeout);
}

/**
 * rts_test_unit_ready - Send TEST_UNIT_READY command.
 * @fd: file descriptor
 *
 * The TEST_UNIT_READY command provides a means to check if the device
 * is ready.
 */
int rts_test_unit_ready(int fd)
{
	/* request READY status */
	unsigned char cmdblk[6] = {
        TEST_UNIT_READY, /* command */
                      0, /* lun/reserved */
                      0, /* reserved */
                      0, /* reserved */
                      0, /* reserved */
                      0  /* control */
	};

	return handle_scsi_cmnd(fd, SG_DXFER_NONE, sizeof(cmdblk), cmdblk, 
			0, NULL, 0, NULL, NULL, 10000);	
}

/**
 * rts_inquiry - Send INQUIRY command.
 * @fd: file descriptor
 * @buf: data buffer
 * @buf_len: buffer length
 *
 * The INQUIRY command requests that information regarding parameters of 
 * the device itself be sent to the host.
 */
int rts_inquiry(int fd, char *buf, int buf_len)
{
	unsigned char cmdblk[10] = {
     		      INQUIRY,  /* command */
                	    0,  /* lun/reserved */
                  	    0,  /* page code */
                  	    0,  /* reserved */
	    		    96,  /* allocation length */
                  	    0   /* reserved/flag/link */
	};

	assert(buf != NULL);

	return handle_scsi_cmnd(fd, SG_DXFER_FROM_DEV, sizeof(cmdblk), cmdblk, 
			0, NULL, buf_len, (unsigned char *)buf, NULL, 10000);
}

/**
 * rts_read_capacity - Send READ_CAPACITY command.
 * @fd: file descriptor
 * @lba: last block address
 * @blk_len: block length
 *
 * The READ_CAPACITY command allows the host to request capacities 
 * of the currently installed medium.
 */
int rts_read_capacity(int fd, unsigned int *lba, unsigned int *blk_len)
{
	int result;
	unsigned char rcvBuffer[8];
	unsigned char cmdblk[10] = {
            READ_CAPACITY, /* command */
                      	0, /* lun/reserved */
		      	0, /* logical block address */
		      	0, /* logical block address */
		        0, /* logical block address */
		        0, /* logical block address */
                      	0, /* reserved */
                      	0, /* reserved */
                      	0, /* reserved */
                      	0  /* control */
	};

	assert(lba && blk_len);	
	
	result = handle_scsi_cmnd(fd, SG_DXFER_FROM_DEV, sizeof(cmdblk), cmdblk, 
			0, NULL, 8, rcvBuffer, NULL, 10000);
	if (result < 0) {
		return result;
	}

	*lba = ((rcvBuffer[0] << 24) & 0xFF000000 |
		(rcvBuffer[1] << 16) & 0xFF0000 | (rcvBuffer[2] << 8)
		& 0xFF00 | rcvBuffer[3] & 0xFF);
	*blk_len =  ((rcvBuffer[4] << 24) & 0xFF000000 |
		(rcvBuffer[5] << 16) & 0xFF0000 | (rcvBuffer[6] << 8)
		& 0xFF00 | rcvBuffer[7] & 0xFF);

	return 0;	
}

/**
 * rts_read10 - Send READ_10 command.
 * @fd: file descriptor
 * @lba: logical block address
 * @buf: data buffer
 * @buf_len: buffer length
 *
 * The READ_10 command requests that the device transfer data to the host.
 */
int rts_read10(int fd, unsigned int lba, int sec_num, unsigned char *buf, int buf_len)
{
	unsigned char cmdblk[10] = {
      		READ_10, /* command */
                      0, /* lun/reserved */
		      0, /* logical block address */
		      0, /* logical block address */
		      0, /* logical block address */
		      0, /* logical block address */
                      0, /* reserved */
                      0, /* transfer length */
                      0, /* transfer length */
                      0  /* control */
	};

	assert(buf && (buf_len > 0) && (sec_num > 0));	

	cmdblk[2] = (unsigned char)(lba >> 24);
	cmdblk[3] = (unsigned char)(lba >> 16);
	cmdblk[4] = (unsigned char)(lba >> 8);
	cmdblk[5] = (unsigned char)lba;
	cmdblk[7] = (unsigned char)(sec_num >> 8);
	cmdblk[8] = (unsigned char)sec_num;

	return handle_scsi_cmnd(fd, SG_DXFER_FROM_DEV, sizeof(cmdblk), cmdblk, 
			0, NULL, buf_len, (unsigned char *)buf, NULL, 10000);
}

/**
 * rts_write10 - Send WRITE_10 command.
 * @fd: file descriptor
 * @lba: logical block address
 * @buf: data buffer
 * @buf_len: buffer length
 *
 * The WRITE_10 command requests that the device write the data 
 * transferred by the host to the medium.
 */
int rts_write10(int fd, unsigned int lba, int sec_num, unsigned char *buf, int buf_len)
{
	unsigned char cmdblk[10] = {
      		WRITE_10, /* command */
                      0, /* lun/reserved */
		      0, /* logical block address */
		      0, /* logical block address */
		      0, /* logical block address */
		      0, /* logical block address */
                      0, /* reserved */
                      0, /* transfer length */
                      0, /* transfer length */
                      0  /* control */
	};

	assert(buf && (buf_len > 0) && (sec_num > 0));	

	cmdblk[2] = (unsigned char)(lba >> 24);
	cmdblk[3] = (unsigned char)(lba >> 16);
	cmdblk[4] = (unsigned char)(lba >> 8);
	cmdblk[5] = (unsigned char)lba;
	cmdblk[7] = (unsigned char)(sec_num >> 8);
	cmdblk[8] = (unsigned char)sec_num;

	return handle_scsi_cmnd(fd, SG_DXFER_TO_DEV, sizeof(cmdblk), cmdblk, 
			0, NULL, buf_len, (unsigned char *)buf, NULL, 10000);
}

/**
 * rts_read_status - Send READ_STATUS command.
 * @fd: file descriptor
 * @status: status buffer
 * @status_len: status length
 * @act_len: actual status length
 *
 * The READ_STATUS command is Realtek's vendor command.
 * It requests the status information from the device.
 * The parameter status_len saves the real status length 
 * returned by the device.
 */
int rts_read_status(int fd, unsigned char *status, int status_len, int *act_len)
{
	int result, resid;
	unsigned char cmdblk [10] = {
      		      	0xF0, /* command */
                      	0x09, /* subcommand */
		      	0, 
			0, 
		        0, 
	};

	assert(status && (status_len > 0));	
	
	result = handle_scsi_cmnd(fd, SG_DXFER_FROM_DEV, sizeof(cmdblk), cmdblk, 
			0, NULL, status_len, status, &resid, 10000);
	if (result < 0) {
		return result;
	}

	if (act_len) {
		*act_len = status_len - resid;
	}

	return 0;
}

/**
 * rts_read_ppstatus - Send READ_PPSTATUS command.
 * @fd: file descriptor
 * @status: status buffer
 * @status_len: status length
 * @act_len: actual status length
 *
 * The READ_PPSTATUS command is Realtek's vendor command.
 * It requests the detailed status information from the device.
 * The parameter status_len saves the real status length 
 * returned by the device.
 */
int rts_read_ppstatus(int fd, unsigned char *status, int status_len, int *act_len)
{
	int result, resid;
	unsigned char cmdblk [10] = {
      		      	0xF0, /* command */
                      	0x10, /* subcommand */
		      	0x10, 
			0, 
		        0, 
	};

	assert(status && (status_len > 0));	
	
	result = handle_scsi_cmnd(fd, SG_DXFER_FROM_DEV, sizeof(cmdblk), cmdblk, 
			0, NULL, status_len, status, &resid, 10000);
	if (result < 0) {
		return result;
	}

	if (act_len) {
		*act_len = status_len - resid;
	}

	return 0;
}


/**
 * rts_read_mem - Send READ_MEM command.
 * @fd: file descriptor
 * @addr: memory address
 * @len: data length
 * @data: data buffer
 *
 * The READ_MEM command is Realtek's vendor command.
 * It requests the internal memory value from the device.
 */
int rts_read_mem(int fd, unsigned short addr, unsigned short len, unsigned char *data)
{
	unsigned char cmdblk [10] = {
      		      	0xF0, /* command */
                      	0x0D, /* subcommand */
		      	0, 
			0, 
		        0, 
			0,
	};

	assert(data && len);

	cmdblk[2] = (unsigned char)(addr >> 8);
	cmdblk[3] = (unsigned char)addr;
	cmdblk[4] = (unsigned char)(len >> 8);
	cmdblk[5] = (unsigned char)len;	
	
	return handle_scsi_cmnd(fd, SG_DXFER_FROM_DEV, sizeof(cmdblk), cmdblk, 
			0, NULL, len, data, NULL, 10000);
}

/**
 * rts_write_mem - Send WRITE_MEM command.
 * @fd: file descriptor
 * @addr: memory address
 * @len: data length
 * @data: data buffer
 *
 * The WRITE_MEM command is Realtek's vendor command.
 * It sends data to the internal memory of the device.
 */
int rts_write_mem(int fd, unsigned short addr, unsigned short len, unsigned char *data)
{
	unsigned char cmdblk [10] = {
      		      	0xF0, /* command */
                      	0x0E, /* subcommand */
		      	0, 
			0, 
		        0, 
			0,
	};

	assert(data && len);

	cmdblk[2] = (unsigned char)(addr >> 8);
	cmdblk[3] = (unsigned char)addr;
	cmdblk[4] = (unsigned char)(len >> 8);
	cmdblk[5] = (unsigned char)len;	
	
	return handle_scsi_cmnd(fd, SG_DXFER_TO_DEV, sizeof(cmdblk), cmdblk, 
			0, NULL, len, data, NULL, 10000);
}

/**
 * rts_read_page - Send vendor command READ_PAGE.
 * @fd: file descriptor
 * @addr: memory address
 * @check_ecc: check ecc or not
 * @data: data buffer
 * @data_len: buffer length
 *
 * The READ_PAGE command is Realtek's vendor command.
 * It reads from xD card using physical address.
 */
int rts_read_page(int fd, unsigned long addr, int check_ecc, unsigned char *data, int data_len)
{
	unsigned char cmdblk [10] = {
      		      	0xF0, /* command */
                      	0x11, /* subcommand */
		      	0, 
			0, 
		        0, 
			0,
			0,
			1,
	};

	assert(data && data_len);

	cmdblk[2] = (unsigned char)(addr >> 24);
	cmdblk[3] = (unsigned char)(addr >> 16);
	cmdblk[4] = (unsigned char)(addr >> 8);
	cmdblk[5] = (unsigned char)addr;
	if (check_ecc) {
		cmdblk[6] = 1;
	} else {
		cmdblk[6] = 0;
	}
	
	return handle_scsi_cmnd(fd, SG_DXFER_FROM_DEV, sizeof(cmdblk), cmdblk, 
			0, NULL, data_len, data, NULL, 10000);
}

/**
 * rts_write_page - Send vendor command WRITE_PAGE.
 * @fd: file descriptor
 * @addr: memory address
 * @calc_ecc: calculate ecc or not
 * @data: data buffer
 * @data_len: buffer length
 *
 * The WRITE_PAGE command is Realtek's vendor command.
 * It writes to xD card using physical address.
 */
int rts_write_page(int fd, unsigned long addr, int calc_ecc, unsigned char *data, int data_len)
{
	unsigned char cmdblk [10] = {
      		      	0xF0, /* command */
                      	0x11, /* subcommand */
		      	0, 
			0, 
		        0, 
			0,
			0,
			2,
	};

	assert(data && data_len);

	cmdblk[2] = (unsigned char)(addr >> 24);
	cmdblk[3] = (unsigned char)(addr >> 16);
	cmdblk[4] = (unsigned char)(addr >> 8);
	cmdblk[5] = (unsigned char)addr;
	if (calc_ecc) {
		cmdblk[6] = 1;
	} else {
		cmdblk[6] = 0;
	}
	
	return handle_scsi_cmnd(fd, SG_DXFER_TO_DEV, sizeof(cmdblk), cmdblk, 
			0, NULL, data_len, data, NULL, 10000);
}

/**
 * rts_ppread10 - Send vendor command PP_READ10.
 * @fd: file descriptor
 * @lba: logical block address
 * @buf: data buffer
 * @buf_len: buffer length
 *
 * The PP_READ10 command requests that the device transfer data to the host.
 */
int rts_ppread10(int fd, unsigned int lba, int sec_num, unsigned char *buf, int buf_len)
{
	unsigned char cmdblk[12] = {
      		   0xF0, /* command */
                   0x10, /* lun/reserved */
		   0x1A, /* logical block address */
		      0, /* logical block address */
		      0, /* logical block address */
		      0, /* logical block address */
                      0, /* reserved */
                      0, /* transfer length */
                      0, /* transfer length */
                      0  /* control */
	};

	assert(buf && (buf_len > 0) && (sec_num > 0));	

	cmdblk[4] = (unsigned char)(lba >> 24);
	cmdblk[5] = (unsigned char)(lba >> 16);
	cmdblk[6] = (unsigned char)(lba >> 8);
	cmdblk[7] = (unsigned char)lba;
	cmdblk[9] = (unsigned char)(sec_num >> 8);
	cmdblk[10] = (unsigned char)sec_num;

	return handle_scsi_cmnd(fd, SG_DXFER_FROM_DEV, sizeof(cmdblk), cmdblk, 
			0, NULL, buf_len, (unsigned char *)buf, NULL, 10000);
}

/**
 * rts_ppwrite10 - Send vendor command PP_WRITE10.
 * @fd: file descriptor
 * @lba: logical block address
 * @buf: data buffer
 * @buf_len: buffer length
 *
 * The PP_WRITE10 command requests that the device write the data 
 * transferred by the host to the medium.
 */
int rts_ppwrite10(int fd, unsigned int lba, int sec_num, unsigned char *buf, int buf_len)
{
	unsigned char cmdblk[12] = {
      		   0xF0, /* command */
                   0x10, /* lun/reserved */
		   0x0A, /* logical block address */
		      0, /* logical block address */
		      0, /* logical block address */
		      0, /* logical block address */
                      0, /* reserved */
                      0, /* transfer length */
                      0, /* transfer length */
                      0  /* control */
	};

	assert(buf && (buf_len > 0) && (sec_num > 0));	

	cmdblk[4] = (unsigned char)(lba >> 24);
	cmdblk[5] = (unsigned char)(lba >> 16);
	cmdblk[6] = (unsigned char)(lba >> 8);
	cmdblk[7] = (unsigned char)lba;
	cmdblk[9] = (unsigned char)(sec_num >> 8);
	cmdblk[10] = (unsigned char)sec_num;

	return handle_scsi_cmnd(fd, SG_DXFER_TO_DEV, sizeof(cmdblk), cmdblk, 
			0, NULL, buf_len, (unsigned char *)buf, NULL, 10000);
}

int rts_request_sense(int fd, unsigned char *sense)
{
	int resid = 0;
	unsigned char cmdblk[10] = {
		REQUEST_SENSE, 
		0, 
		0, 
		0, 
		18
	};

	assert(sense != NULL);

	return  rts_handle_scsi_cmnd(fd, SG_DXFER_FROM_DEV, sizeof(cmdblk), cmdblk, 
			0, NULL, 18, sense, &resid, 10000);
}

int rts_read_eeprom(int fd, unsigned char *bin, int *len, int timeout)
{
	int ret = 0;
	int result, resid;
	unsigned char sense[18];
	unsigned char cmdblk [10] = {
      		      	0xf0, /* command */
                      	0x04, /* subcommand */
		      	0, 
			0, 
		        0, 
			0,
	};

	assert(bin != NULL);

	cmdblk[4] = (unsigned char)(*len >> 8);
	cmdblk[5] = (unsigned char)*len;	
	
	ret = rts_handle_scsi_cmnd(fd, SG_DXFER_FROM_DEV, sizeof(cmdblk), cmdblk,
			18, sense, *len, bin, &resid, timeout);

	*len -= resid;

	return ret;
}

int rts_write_eeprom(int fd, unsigned char *bin, int len, int timeout)
{

	int resid;
	unsigned char sense[18];
	unsigned char cmdblk [10] = {
      		      	0xf0, /* command */
                      	0x05, /* subcommand */
		      	0, 
			0, 
		        0, 
			0,
	};

	assert(bin != NULL);

	cmdblk[4] = (unsigned char)((len) >> 8);
	cmdblk[5] = (unsigned char)len;	
	
	return rts_handle_scsi_cmnd(fd, SG_DXFER_TO_DEV, sizeof(cmdblk), cmdblk, 
			18, sense, len, bin, &resid, timeout);
}

int rts_read_host_reg(int fd, unsigned char addr, unsigned long *data, int timeout)
{
	int result, resid;
	unsigned char sense[18], buf[4];
	unsigned char cmdblk [10] = {
      		      	0xf0, /* command */
                      	0x10, /* subcommand */
		      	0x1d, 
			0, 
		        0, 
			0,
	};

	cmdblk[4] = addr;
	
	result = rts_handle_scsi_cmnd(fd, SG_DXFER_FROM_DEV, sizeof(cmdblk), cmdblk, 
			18, sense, 4, buf, &resid, timeout);

	*data = ((unsigned long)buf[0] << 24) | ((unsigned long)buf[1] << 16) |
		((unsigned long)buf[2] << 8) | ((unsigned long)buf[3]);

	return result;
}


int rts_write_host_reg(int fd, unsigned char addr, unsigned long data, int timeout)
{
	int result, resid;
	unsigned char sense[18], buf[4];
	unsigned char cmdblk [10] = {
      		      	0xf0, /* command */
                      	0x10, /* subcommand */
		      	0x0d, 
			0, 
		        0, 
			0,
	};

	cmdblk[4] = addr;

	buf[0] = (unsigned char)(data >> 24);
	buf[1] = (unsigned char)(data >> 16);
	buf[2] = (unsigned char)(data >> 8);
	buf[3] = (unsigned char)(data);
	
	return rts_handle_scsi_cmnd(fd, SG_DXFER_TO_DEV, sizeof(cmdblk), cmdblk, 
			18, sense, 4, buf, &resid, timeout);

}

int rts_trace_msg(int fd, char *buf, int buf_len, int timeout)
{
	int result, resid;
	unsigned char sense[18];
	unsigned char cmdblk [10] = {
      		      	0xf0, /* command */
                      	0x18, /* subcommand */
		      	0, 
			0, 
		        0, 
			0,
	};

	assert(buf );

	result = rts_handle_scsi_cmnd(fd, SG_DXFER_FROM_DEV, sizeof(cmdblk), cmdblk, 
			18, sense, buf_len, (unsigned char *)buf, &resid, timeout);
}

int rts_set_clock(int fd, unsigned char card, unsigned char clk, int timeout)
{
	int result, resid;
	unsigned char sense[18];
	unsigned char cmdblk [10] = {
      		      	0xf0, /* command */
                      	0x10, /* subcommand */
		      	0x05, 
			0x01, 
		        0, 
			0,
	};

	cmdblk[4] = card;
	cmdblk[5] = clk;
	
	return rts_handle_scsi_cmnd(fd, SG_DXFER_NONE, sizeof(cmdblk), cmdblk, 
			18, sense, 0, NULL, &resid, timeout);
}

int rts_get_clock(int fd, unsigned char card, unsigned char *clk, int timeout)
{
	int result, resid;
	unsigned char sense[18], buf[1];
	unsigned char cmdblk [10] = {
      		      	0xf0, /* command */
                      	0x10, /* subcommand */
		      	0x15, 
			0x01, 
		        0, 
			0,
	};

	cmdblk[4] = card;
	
	result = rts_handle_scsi_cmnd(fd, SG_DXFER_FROM_DEV, sizeof(cmdblk), cmdblk, 
			18, sense, 1, buf, &resid, timeout);

	*clk = buf[0];

	return result;
}

int rts_read_cfg_byte(int fd, unsigned char func, unsigned short addr, unsigned char *buf, 
		unsigned short len, int timeout)
{
	int result, resid;
	unsigned char sense[18];
	unsigned char cmdblk [10] = {
      		      	0xf0, /* command */
                      	0x10, /* subcommand */
		      	0x1E, 
			0, 
		        0, 
			0,
	};

	cmdblk[3] = func;
	cmdblk[4] = (unsigned char)(addr >> 8);
	cmdblk[5] = (unsigned char)addr;
	cmdblk[6] = (unsigned char)(len >> 8);
	cmdblk[7] = (unsigned char)len;
	
	return  rts_handle_scsi_cmnd(fd, SG_DXFER_FROM_DEV, sizeof(cmdblk), cmdblk, 
			18, sense, len, buf, &resid, timeout);
}

int rts_write_cfg_byte(int fd, unsigned char func, unsigned short addr, unsigned char *buf,
		unsigned short len, int timeout)
{
	int result, resid;
	unsigned char sense[18];
	unsigned char cmdblk [10] = {
      		      	0xf0, /* command */
                      	0x10, /* subcommand */
		      	0x0E, 
			0, 
		        0, 
			0,
	};

	cmdblk[3] = func;
	cmdblk[4] = (unsigned char)(addr >> 8);
	cmdblk[5] = (unsigned char)addr;
	cmdblk[6] = (unsigned char)(len >> 8);
	cmdblk[7] = (unsigned char)len;
	
	return rts_handle_scsi_cmnd(fd, SG_DXFER_TO_DEV, sizeof(cmdblk), cmdblk, 
			18, sense, len, buf, &resid, timeout);
}
