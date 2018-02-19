/* Definitions for data structures and routines for the Realtek 
 * card reader library
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

#ifndef _RTCR_H
#define _RTCR_H 1

#include <sys/types.h>

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

int rts_open(int dev, int lun);
int rts_open_get_name(int dev, int lun, char *buf, int buf_len);
int rts_open_name(const char *dev_name);
void rts_close(int fd);
int rts_handle_scsi_cmnd(int fd, int direction,
			int cmd_len, unsigned char *cmd,
			int sb_len, unsigned char *sense,
			int buf_size, unsigned char *buf,
			int *resid, int timeout);

int rts_test_unit_ready(int fd);
int rts_inquiry(int fd, char *buf, int buf_len);
int rts_read_capacity(int fd, unsigned int *lba, unsigned int *blk_len);
int rts_read10(int fd, unsigned int lba, int sec_num, unsigned char *buf, int buf_len);
int rts_write10(int fd, unsigned int lba, int sec_num, unsigned char *buf, int buf_len);
int rts_read_status(int fd, unsigned char *status, int status_len, int *act_len);
int rts_read_ppstatus(int fd, unsigned char *status, int status_len, int *act_len);
int rts_read_mem(int fd, unsigned short addr, unsigned short len, unsigned char *data);
int rts_write_mem(int fd, unsigned short addr, unsigned short len, unsigned char *data);
int rts_read_page(int fd, unsigned long addr, int check_ecc, unsigned char *data, int data_len);
int rts_write_page(int fd, unsigned long addr, int calc_ecc, unsigned char *data, int data_len);
int rts_ppread10(int fd, unsigned int lba, int sec_num, unsigned char *buf, int buf_len);
int rts_ppwrite10(int fd, unsigned int lba, int sec_num, unsigned char *buf, int buf_len);

int rts_request_sense(int fd, unsigned char *sense);
int rts_read_eeprom(int fd, unsigned char *bin, int *len, int timeout);
int rts_write_eeprom(int fd, unsigned char *bin, int len, int timeout);
int rts_read_host_reg(int fd, unsigned char addr, unsigned long *data, int timeout);
int rts_write_host_reg(int fd, unsigned char addr, unsigned long data, int timeout);
int rts_trace_msg(int fd, char *buf, int buf_len, int timeout);
int rts_set_clock(int fd, unsigned char card, unsigned char clk, int timeout);
int rts_get_clock(int fd, unsigned char card, unsigned char *clk, int timeout);
int rts_read_cfg_byte(int fd, unsigned char func, unsigned short addr, unsigned char *buf, 
		unsigned short len, int timeout);
int rts_write_cfg_byte(int fd, unsigned char func, unsigned short addr, unsigned char *buf,
		unsigned short len, int timeout);

#ifdef __cplusplus
}
#endif	/* C++ */

#endif  //  _RTCR_H

