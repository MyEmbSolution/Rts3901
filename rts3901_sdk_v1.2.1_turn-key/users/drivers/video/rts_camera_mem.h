/*
 * Realtek Semiconductor Corp.
 *
 * rts_camera_mem.h
 *
 * Copyright (C) 2016      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _RTS_CAMERA_MEM_H
#define _RTS_CAMERA_MEM_H

struct rtscam_mem_info {
	struct device	*dev;
	void            *virt_base;
	dma_addr_t      device_base;
	phys_addr_t     pfn_base;
	int             size;
	unsigned long   *bitmap;
	int             bitmap_size;
	int		initialized;
};

int rtscam_mem_init(struct rtscam_mem_info *rtsmem, struct device *dev,
		dma_addr_t bus_addr, dma_addr_t device_addr, size_t size);
int rtscam_mem_release(struct rtscam_mem_info *rtsmem);
void *rtscam_mem_alloc(struct rtscam_mem_info *rtsmem,
		size_t size, dma_addr_t *phy_addr);
void rtscam_mem_free(struct rtscam_mem_info *rtsmem, size_t size,
		void *vaddr, dma_addr_t phy_addr);
long rtscam_mem_get_left_size(struct rtscam_mem_info *rtsmem);
long rtscam_mem_get_used_size(struct rtscam_mem_info *rtsmem);
long rtscam_mem_get_total_size(struct rtscam_mem_info *rtsmem);

#endif

