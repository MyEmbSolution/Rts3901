/*
 * videobuf2-dma-contig.h - DMA contig memory allocator for videobuf2
 *
 * Copyright (C) 2010 Samsung Electronics
 *
 * Author: Pawel Osciak <pawel@osciak.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#ifndef _RTS_DMA_CONTIG_H
#define _RTS_DMA_CONTIG_H

#include <media/videobuf2-core.h>
#include <linux/dma-mapping.h>
#include "rts_camera_mem.h"

static inline dma_addr_t
rts_dma_contig_plane_dma_addr(struct vb2_buffer *vb, unsigned int plane_no)
{
	dma_addr_t *addr = vb2_plane_cookie(vb, plane_no);

	return *addr;
}

void *rts_dma_contig_init_ctx(struct device *dev,
		struct rtscam_mem_info *rtsmem);
void rts_dma_contig_cleanup_ctx(void *alloc_ctx);

extern const struct vb2_mem_ops rts_dma_contig_memops;

#endif
