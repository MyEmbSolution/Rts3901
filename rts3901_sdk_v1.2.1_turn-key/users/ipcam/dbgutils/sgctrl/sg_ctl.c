/* Realtek Card Reader Library
 * Search card reader through SCSI Generic driver
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>
#include <errno.h>
#include <glob.h>
#include <assert.h>

#include "sg_ctl.h"

int handle_scsi_cmnd(int fd, int direction,
			int cmd_len, unsigned char *cmd,
			int sb_len, unsigned char *sense,
			int buf_size, unsigned char *buf,
			int *resid, int timeout)
{
	sg_io_hdr_t io_hdr;
	int err;

	assert(fd > 0);
	assert(cmd && (cmd_len > 0));

	memset(&io_hdr, 0 ,sizeof(sg_io_hdr_t));
	io_hdr.interface_id = 'S';
	io_hdr.dxfer_direction = direction;
	io_hdr.cmd_len = cmd_len;
	io_hdr.cmdp = cmd;
	io_hdr.mx_sb_len = sb_len;
	io_hdr.sbp = sense;
	io_hdr.dxfer_len = buf_size;
	io_hdr.dxferp = buf;
//	io_hdr.flags = SG_FLAG_LUN_INHIBIT;
	io_hdr.timeout = timeout;

	err = ioctl(fd, SG_IO, &io_hdr);
	if (err < 0) {
		return err;
	}

	if (io_hdr.masked_status != 0) {
		return -EIO;
	}

	if (resid) {
		*resid = io_hdr.resid;
	}

	return 0;
}


static SG_DEVICE *new_sg_device()
{
	SG_DEVICE *sg_dev;

	sg_dev = (SG_DEVICE *)malloc(sizeof(SG_DEVICE));
	if (sg_dev == NULL) {
		return NULL;
	}

	memset(sg_dev, 0, sizeof(SG_DEVICE));

	return sg_dev;
}


static SG_DEVICE *append_sg_device(SG_DEVICE *head, SG_DEVICE *new_dev)
{
	SG_DEVICE *ptr;
	
	assert(head != NULL);

	ptr = head;
	if (ptr->next != NULL) {
		ptr = ptr->next;
	}

	ptr->next = new_dev;
	new_dev->next = NULL;

	return head;
}


static SG_DEVICE *search_sg_device(SG_DEVICE *head, SCSI_ID id)
{
	SG_DEVICE *ptr;
	
	ptr = head;
	while (ptr != NULL) {
		if (ptr->scsi_id == id) {
			return ptr;
		}
		ptr = ptr->next;
	}

	return NULL;
}


static char *match_error(int err)
{
	switch (err) {
		case GLOB_ABORTED:
			return "filesystem problem";

		case GLOB_NOMATCH:
			return "no match of pattern";

		case GLOB_NOSPACE:
			return "no dynamic memory";
	};

	return "unknown problem";
}

/* Parameters:
 * 	dev_num: how many devices been connected to the host
 * 	sg_dev: SG_DEVICE list
 * Returns:
 *	0: Success
 *	<0: No device connected, or error
 */ 
int sg_scan_device(int *dev_num, SG_DEVICE **sg_dev)
{
	SG_DEVICE *head, *ptr;
	int result, i, fd, lun, count;
	glob_t match_results;
	struct scsi_idlun idlun;
	char host_info[64];
	
	assert(sg_dev != NULL);

	result = glob(SG_DEV_NAME, 0, NULL, &match_results);
	if (result != 0) {
		fprintf(stderr, "glob match fail (%s)\n",
				match_error(result));
		globfree(&match_results);
		return -ENXIO;
	}
	
	head = NULL;
	count = 0;
	for (i = 0; i < match_results.gl_pathc; i++) {
		fd = open(match_results.gl_pathv[i], O_RDWR);
		if (fd < 0) {
			continue;
		}
		
		// Get host information
		host_info[0] = 64;  // Array length
		if (ioctl(fd, SCSI_IOCTL_PROBE_HOST, host_info) < 0) {
			close(fd);
			continue;
		}
		host_info[63] = '\0';
		if (!strcasestr(host_info, "usb") && !strcasestr(host_info, "pci")) {
			close(fd);
			continue;
		}

		if (ioctl(fd, SCSI_IOCTL_GET_IDLUN, &idlun) < 0) {
			close(fd);
			continue;
		}
		
		lun = (int)((idlun.dev_id >> 8) & 0x000000ff);

		ptr = search_sg_device(head, idlun.dev_id & 0xffff00ff);
		if (ptr != NULL) {
			if (ptr->name == NULL) {
				ptr->name_count = (lun/16 + 1) * 16;
				ptr->name = (sg_name *)malloc(ptr->name_count * sizeof(sg_name));
				memset(ptr->name, 0, ptr->name_count * sizeof(sg_name));
			}
			
			if (ptr->max_lun < lun) {
				ptr->max_lun = lun;
			}
			
			if (lun > ptr->name_count - 1) {
				// name buffer is too small, 
				// we should realloc more memory
				sg_name *temp;

				temp = realloc(ptr->name, ptr->name_count + 16);
				if (temp == NULL) {
					close(fd);
					sg_release_device(head);
					fprintf(stderr, "no dynamic memory\n");
					return -ENOMEM;
				}

				ptr->name = temp;
				ptr->name_count += 16;
			}
			
			strcpy(ptr->name[lun], match_results.gl_pathv[i]);				
		} else {
			ptr = new_sg_device();
			if (ptr == NULL) {
				close(fd);
				sg_release_device(head);
				fprintf(stderr, "no dynamic memory\n");
				return -ENOMEM;
			}

			++ count;
			
			// multiple of 16
			ptr->name_count = (lun/16 + 1) * 16;
			ptr->name = (sg_name *)malloc(ptr->name_count * sizeof(sg_name));
			memset(ptr->name, 0, ptr->name_count * sizeof(sg_name));
			ptr->max_lun = lun;
			ptr->scsi_id = idlun.dev_id & 0xffff00ff;
			strcpy(ptr->name[lun], match_results.gl_pathv[i]);				

			if (head == NULL) {
				head = ptr;
			} else {
				append_sg_device(head, ptr);
			}
		}

		close(fd);
	}

	globfree(&match_results);
	
	if (head == NULL) {
		return -ENXIO;
	}	
	
	*sg_dev = head;
	if (dev_num) {
		*dev_num = count;
	}

	return 0;
}

void sg_release_device(SG_DEVICE *head)
{
	SG_DEVICE *ptr1, *ptr2;
	
	assert(head != NULL);
	
	ptr1 = head;

	while (ptr1 != NULL) {
		ptr2 = ptr1->next;
		
		if (ptr1->name) {
			free(ptr1->name);
		}
		free(ptr1);
		
		ptr1 = ptr2;
	}

	return;
}


int sg_open_devfs(SG_DEVICE *sg_dev, int lun, int oflag)
{
	int fd;

	assert(sg_dev != NULL);

	if ((sg_dev->name == NULL) || (sg_dev->name_count <= lun)) {
		return -1;
	}
	
	fd = open(sg_dev->name[lun], oflag);

	return fd;
}

void sg_close_devfs(int fd)
{
	close(fd);
}


