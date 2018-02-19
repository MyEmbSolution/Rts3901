/*
 * SHM (ffserver live feed) common header
 * Copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVFORMAT_SHM_H
#define AVFORMAT_SHM_H

#include <stdint.h>
#include "avformat.h"
#include "avio.h"

/* The SHM file is made of blocks of fixed size */
#define SHM_PACKET_SIZE 4096
#define PACKET_ID       0x666d666d

/* each packet contains frames (which can span several packets */
#define FRAME_HEADER_SIZE    16
#define FLAG_KEY_FRAME       0x01
#define FLAG_DTS             0x02

enum {
	READ_HEADER,
	READ_DATA,
	SHM_HEADER_SIZE		= 4096,
};


typedef struct SHMContext {
    int64_t shm_size;
    int64_t prev_index;
    int64_t write_index;
    int64_t read_index;

    /* read and write */
    int64_t start_time;
    uint8_t *shm_buf;
    int shm_id;
    int shm_delete;
    int shm_update;

    uint8_t *ring_begin;
    uint8_t *ring_end;
    uint8_t *ring_pos;
} SHMContext;

typedef union SHMPacketHeader {
	struct {
		uint32_t id;
		uint32_t flags;
		int32_t duration;
		int32_t size;
		int32_t stream_index;
		int32_t fill_size;
		uint64_t pts;
		uint64_t dts;
		int64_t prev_index;
		int64_t next_index;
	} p;

	uint8_t aligned[64];
} SHMPacketHeader;

inline static int shm_r8(void *p, uint8_t *v)
{
	*v = *((uint8_t *) p);
	return sizeof(*v);
}

inline static int shm_w8(void *p, uint8_t v)
{
	*((uint8_t *) p) = v;
	return sizeof(v);
}

inline static int shm_r16(void *p, uint16_t *v)
{
	*v = *((uint16_t *) p);
	return sizeof(*v);
}

inline static int shm_w16(void *p, uint16_t v)
{
	*((uint16_t *) p) = v;
	return sizeof(v);
}

inline static int shm_r32(void *p, uint32_t *v)
{
	*v = *((uint32_t *) p);
	return sizeof(*v);
}

inline static int shm_w32(void *p, uint32_t v)
{
	*((uint32_t *) p) = v;
	return sizeof(v);
}

inline static int shm_r64(void *p, uint64_t *v)
{
	*v = *((uint64_t *) p);
	return sizeof(*v);
}

inline static int shm_w64(void *p, uint64_t v)
{
	*((uint64_t *) p) = v;
	return sizeof(v);
}

inline static int shm_rbuf(void *pos, void *data, int size)
{
	memcpy(data, pos, size);
	return size;
}

inline static int shm_wbuf(void *pos, const void *data, int size)
{
	memcpy(pos, data, size);
	return size;
}

int shm_create_shm(AVFormatContext *s);

#endif /* AVFORMAT_SHM_H */
