/*
 * Realtek Semiconductor Corp.
 *
 * rts_camera_mem.c
 *
 * Copyright (C) 2016      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/sizes.h>
#include <linux/delay.h>
#include <asm/unaligned.h>
#include "rts_camera.h"
#include "rts_camera_mem.h"

#define RTS_DC_MAX_SIZE			(1024*1024*16)
#define RTS_MEM_SIZE_SMALL		4
#define RTS_MEM_SMALL_REGION		256

struct dma_coherent_mem {
	void            *virt_base;
	dma_addr_t      device_base;
	phys_addr_t     pfn_base;
	int             size;
	int             flags;
	unsigned long   *bitmap;
};

int rtscam_mem_init(struct rtscam_mem_info *rtsmem, struct device *dev,
                    dma_addr_t bus_addr, dma_addr_t device_addr, size_t size)
{
	int dma;

	if (!rtsmem)
		return -EINVAL;

	if (rtsmem->initialized)
		rtscam_mem_release(rtsmem);

	dma = dma_declare_coherent_memory(dev, bus_addr, device_addr, size,
	                                  DMA_MEMORY_MAP | DMA_MEMORY_EXCLUSIVE);
	if (!(dma & DMA_MEMORY_MAP)) {
		if (!(dma & DMA_MEMORY_IO))
			dma_release_declared_memory(dev);
		return -ENOMEM;
	}

	rtsmem->virt_base = dev->dma_mem->virt_base;
	rtsmem->device_base = dev->dma_mem->device_base;
	rtsmem->pfn_base = dev->dma_mem->pfn_base;
	rtsmem->size = dev->dma_mem->size;
	rtsmem->bitmap = dev->dma_mem->bitmap;
	rtsmem->bitmap_size = BITS_TO_LONGS(rtsmem->size) * sizeof(long);

	rtsmem->dev = get_device(dev);
	rtsmem->initialized = 1;

	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_mem_init);

int rtscam_mem_release(struct rtscam_mem_info *rtsmem)
{
	if (!rtsmem || !rtsmem->initialized)
		return 0;

	dma_release_declared_memory(rtsmem->dev);

	put_device(rtsmem->dev);
	rtsmem->dev = NULL;
	rtsmem->virt_base = NULL;
	rtsmem->device_base = 0;
	rtsmem->pfn_base = 0;
	rtsmem->size = 0;
	rtsmem->bitmap = NULL;
	rtsmem->bitmap_size = 0;
	rtsmem->initialized = 0;

	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_mem_release);

static void __set_region(void *bitmap, long start, long end)
{
	long nr;

	for (nr = start; nr < end; nr++) {
		set_bit(nr, bitmap);
	}
}

static void __clear_region(void *bitmap, long start, long end)
{
	long nr;

	for (nr = start; nr < end; nr++) {
		clear_bit(nr, bitmap);
	}
}

static long __find_region_p(void *bitmap, long start, long end, long size)
{
	long nr;
	long number = 0;

	if (!bitmap)
		return -EINVAL;
	if (end <= start)
		return -EINVAL;
	if (size > end - start)
		return -EINVAL;

	for (nr = start; nr < end; nr++) {
		if (test_bit(nr, bitmap)) {
			number = 0;
			continue;
		}
		number++;
		if (number == size)
			break;
	}

	if (number < size)
		return -ENOMEM;

	__set_region(bitmap, nr + 1 - size, nr + 1);

	return nr + 1 - size;
}

static long __find_region_n(void *bitmap, long start, long end, long size)
{
	long nr;
	long number = 0;

	if (!bitmap)
		return -EINVAL;
	if (end <= start)
		return -EINVAL;
	if (size > end - start)
		return -EINVAL;

	for (nr = end - 1; nr >= start; nr--) {
		if (test_bit(nr, bitmap)) {
			number = 0;
			continue;
		}
		number++;
		if (number == size)
			break;
	}

	if (number < size)
		return -ENOMEM;

	__set_region(bitmap, nr, nr + size);

	return nr;
}

void *rtscam_mem_alloc(struct rtscam_mem_info *rtsmem,
                       size_t size, dma_addr_t *phy_addr)
{
	long page_cnt;
	long start;
	long end;
	long nr;
	long offset;

	if (!rtsmem || !rtsmem->initialized)
		return NULL;

	if (size == 0)
		return NULL;

	size = PAGE_ALIGN(size);
	page_cnt = size >> PAGE_SHIFT;

	start = 0;
	end = rtsmem->size;

	if (page_cnt <= RTS_MEM_SIZE_SMALL)
		nr = __find_region_n(rtsmem->bitmap,
		                     start, end, page_cnt);
	else
		nr = __find_region_p(rtsmem->bitmap,
		                     start, end, page_cnt);

	if (nr < 0)
		return NULL;

	offset = nr << PAGE_SHIFT;

	if (phy_addr)
		*phy_addr = rtsmem->device_base + offset;

	return rtsmem->virt_base + offset;
}
EXPORT_SYMBOL_GPL(rtscam_mem_alloc);

void rtscam_mem_free(struct rtscam_mem_info *rtsmem, size_t size,
                     void *vaddr, dma_addr_t phy_addr)
{
	long page_cnt;
	long start;
	long end;

	if (!rtsmem || !rtsmem->initialized)
		return;

	if (!vaddr || !phy_addr)
		return;

	if (size == 0)
		return;

	if (vaddr - rtsmem->virt_base != phy_addr - rtsmem->device_base) {
		rtsprintk(RTS_TRACE_ERROR, "<%s, %d>invalid memory for free\n",
		          __func__, __LINE__);
		return;
	}

	size = PAGE_ALIGN(size);
	page_cnt = size >> PAGE_SHIFT;
	start = (vaddr - rtsmem->virt_base) >> PAGE_SHIFT;
	end = start + page_cnt;

	__clear_region(rtsmem->bitmap, start, end);
}
EXPORT_SYMBOL_GPL(rtscam_mem_free);

long rtscam_mem_get_left_size(struct rtscam_mem_info *rtsmem)
{
	long used_cnt;

	if (!rtsmem || !rtsmem->initialized)
		return 0;

	used_cnt = memweight(rtsmem->bitmap,
	                     rtsmem->bitmap_size);

	return (rtsmem->size - used_cnt) << PAGE_SHIFT;
}
EXPORT_SYMBOL_GPL(rtscam_mem_get_left_size);

long rtscam_mem_get_used_size(struct rtscam_mem_info *rtsmem)
{
	long used_cnt;

	if (!rtsmem || !rtsmem->initialized)
		return 0;

	used_cnt = memweight(rtsmem->bitmap,
	                     rtsmem->bitmap_size);

	return used_cnt << PAGE_SHIFT;
}
EXPORT_SYMBOL_GPL(rtscam_mem_get_used_size);

long rtscam_mem_get_total_size(struct rtscam_mem_info *rtsmem)
{
	if (!rtsmem || !rtsmem->initialized)
		return 0;

	return rtsmem->size << PAGE_SHIFT;
}
EXPORT_SYMBOL_GPL(rtscam_mem_get_total_size);
