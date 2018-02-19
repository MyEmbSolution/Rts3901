/* Realtek Card Reader Library
 * Header file
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

#ifndef _SG_CTL_H_
#define _SG_CTL_H_

#define SG_NAME_MAX_LEN		16
#define SG_DEV_NAME		"/dev/sg*"

struct scsi_idlun {
	unsigned int	dev_id;
	unsigned int	host_unique_id;
};

// (scsi_device_id | (channel << 16) | (host_no << 24))
// we ignore LUN here because we only care about device
typedef unsigned int SCSI_ID;
typedef char sg_name[SG_NAME_MAX_LEN];

typedef struct sg_device {
	SCSI_ID scsi_id;
	sg_name *name;
	int name_count;
	int max_lun;
	struct sg_device *next;
} SG_DEVICE;


int handle_scsi_cmnd(int fd, int direction,
			int cmd_len, unsigned char *cmd,
			int sb_len, unsigned char *sense,
			int buf_size, unsigned char *buf,
			int *resid, int timeout);
int sg_scan_device(int *dev_num, SG_DEVICE **sg_dev);
void sg_release_device(SG_DEVICE *head);
int sg_open_devfs(SG_DEVICE *sg_dev, int lun, int oflag);
void sg_close_devfs(int fd);

#endif  // _SG_CTL_H_

