/*
 * Realtek Semiconductor Corp.
 *
 * rts_camera_soc.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _UAPI_RTS_CAMERA_SOC_H
#define _UAPI_RTS_CAMERA_SOC_H

#include <linux/types.h>

enum rtscam_soc_cmd_error_type {
	RTSCAM_CMD_STS_CMD_ERR = 0x01,
	RTSCAM_CMD_STS_LEN_ERR = 0x02,
	RTSCAM_CMD_STS_ADDR_ERR = 0x03
};

struct rtscam_soc_cmd_stru {
	__u16 cmdcode;
	__u8 index;
	__u8 length;
	__u16 param;
	__u16 addr;
	__u8 *buf;
	__u32 error_type;
};

struct rtscam_soc_mtd_state {
	int index;
	int state;
};

struct rtscam_soc_dma_buf {
	unsigned long size;
	off_t offset;
	unsigned long dma_addr;
	unsigned long vaddr;
};

struct rtscam_soc_ldc_stru {
	unsigned int length;
	__u8 *ptable;
};

struct rtscam_soc_fw {
	unsigned int length;
	__u8 *fw;
};

#define RTSOCIOC_ISPCMD		_IOWR('s', 0x31, struct rtscam_soc_cmd_stru)
#define RTSOCIOC_G_HWOFFSET	_IOR('s', 0x34, unsigned long)
#define RTSOCIOC_G_HWIOSIZE	_IOR('s', 0x35, unsigned int)
#define RTSOCIOC_G_MTDSTATE	_IOWR('s', 0x36, struct rtscam_soc_mtd_state)
#define RTSOCIOC_G_STREAMID	_IOR('s', 0x37, int)

#define RTSOCIOC_ALLOC_DMA	_IOWR('s', 0x38, struct rtscam_soc_dma_buf)
#define RTSOCIOC_FREE_DMA	_IOW('s', 0x39, unsigned long)

#define RTSOCIOC_S_TD		_IOW('s', 0x3a, int)
#define RTSOCIOC_G_TD		_IOR('s', 0x3b, int)
#define RTSOCIOC_ENABLE_LDC	_IOW('s', 0x3c, struct rtscam_soc_ldc_stru)
#define RTSOCIOC_DISABLE_LDC	_IO('s', 0x3d)
#define RTSOCIOC_G_LDC		_IOR('s', 0x3e, int)

#define RTSOCIOC_CAMERA_DETACH	_IO('s', 0x40)
#define RTSOCIOC_CAMERA_ATTACH	_IO('s', 0x41)
#define RTSOCIOC_CAMERA_LOAD_FW _IOW('s', 0x42, struct rtscam_soc_fw)
#define RTSOCIOC_CAMERA_DUMP_FW _IOWR('s', 0x43, struct rtscam_soc_fw)
#define RTSOCIOC_CAMERA_G_FW_SIZE	_IOR('s', 0x44, unsigned int)

#endif
