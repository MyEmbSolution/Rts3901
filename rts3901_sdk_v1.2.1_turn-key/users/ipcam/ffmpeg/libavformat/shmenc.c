/*
 * SHM (ffserver live feed) muxer
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

#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include "libavutil/intreadwrite.h"
#include "libavutil/intfloat.h"
#include "libavutil/avassert.h"
#include "libavutil/parseutils.h"
#include "url.h"
#include "avformat.h"
#include "internal.h"
#include "shm.h"

int shm_create_shm(AVFormatContext *s)
{
	SHMContext *shm = s->priv_data;
	key_t  key = ftok(s->filename, 0);

	if (key  == -1)
		return -errno;

	shm->shm_id = shmget(key, shm->shm_size, IPC_CREAT | 0666);
	if (shm->shm_id < 0)
		return -errno;

	shm->shm_buf = shmat(shm->shm_id, NULL, 0);
	if (((void *) -1) ==  shm->shm_buf)
		return -errno;

	return 0;
}

static int shm_update_write_index(AVFormatContext *s)
{
	SHMContext *shm = s->priv_data;
	uint8_t *pos = shm->shm_buf;
	uint64_t index =
		((uint8_t *) shm->ring_pos - (uint8_t *) shm->ring_begin);

	pos += 12;
	shm_w64(pos, index);

	return 0;
}

static int shm_write_header(AVFormatContext *s)
{
	SHMContext *shm = s->priv_data;
	AVDictionaryEntry *t;
	AVStream *st;
	AVCodecContext *codec;
	uint8_t *pos = NULL;
	int bit_rate, i;
	int ret = 0;

	shm->shm_delete = 1;

	if (t = av_dict_get(s->metadata, "creation_time", NULL, 0)) {
		ret = av_parse_time(&shm->start_time, t->value, 0);
		if (ret < 0)
			return ret;
	}

	if (t = av_dict_get(s->metadata, "shm_delete", NULL, 0))
		shm->shm_delete = atoi(t->value);

	if (t = av_dict_get(s->metadata, "shm_size", NULL, 0)) {
		shm->shm_size = strtoll(t->value, NULL, 0);
		if (shm->shm_size > (1024 * 1024 * 1024)
				|| shm->shm_size < SHM_PACKET_SIZE)
			shm->shm_size = (1024 * 256);
	} else {
		shm->shm_size = (1024 * 256);
	}


	if (shm->shm_size % (sizeof(SHMPacketHeader))) {
		shm->shm_size = (shm->shm_size / sizeof(SHMPacketHeader))
			* sizeof(SHMPacketHeader);
		shm->shm_size += sizeof(SHMPacketHeader) + SHM_HEADER_SIZE;
	}

	ret = shm_create_shm(s);
	if (ret < 0)
		return ret;

	pos = shm->shm_buf;
	/* header */
	pos += shm_w32(pos, 0); /* reserve tag*/
	pos += shm_w64(pos, shm->shm_size);
	pos += shm_w64(pos, 0); /* current write position */
	pos += shm_r32(pos, &shm->shm_update); /* read prev shm update */
	pos += shm_w32(pos, 0); /* write need close*/

	pos += shm_w32(pos, s->nb_streams);
	bit_rate = 0;
	for(i=0;i<s->nb_streams;i++) {
		st = s->streams[i];
		bit_rate += st->codec->bit_rate;
	}
	pos += shm_w32(pos, bit_rate);

	/* pos += shm_w32(pos, MKBETAG('M', 'A', 'I', 'N')); */
	/* list of streams */
	for(i=0;i<s->nb_streams;i++) {
		st = s->streams[i];
		avpriv_set_pts_info(st, 64, 1, 1000000);
		codec = st->codec;
		/* generic info */
		pos += shm_w32(pos, codec->codec_id);
		pos += shm_w8(pos, codec->codec_type);
		pos += shm_w32(pos, codec->bit_rate);
		pos += shm_w32(pos, codec->flags);
		pos += shm_w32(pos, codec->flags2);
		pos += shm_w32(pos, codec->debug);

		if (codec->flags & CODEC_FLAG_GLOBAL_HEADER) {
			pos += shm_w32(pos, codec->extradata_size);
			pos += shm_wbuf(pos, codec->extradata, codec->extradata_size);
		}

		/* pos += shm_w32(pos, MKBETAG('C', 'O', 'M', 'M')); */

		/* specific info */
		switch(codec->codec_type) {
		case AVMEDIA_TYPE_VIDEO:
			pos += shm_w32(pos, codec->time_base.num);
			pos += shm_w32(pos, codec->time_base.den);
			pos += shm_w16(pos, codec->width);
			pos += shm_w16(pos, codec->height);
			pos += shm_w16(pos, codec->gop_size);
			pos += shm_w32(pos, codec->pix_fmt);
			pos += shm_w8(pos, codec->qmin);
			pos += shm_w8(pos, codec->qmax);
			pos += shm_w8(pos, codec->max_qdiff);
			pos += shm_w16(pos, (int) (codec->qcompress * 10000.0));
			pos += shm_w16(pos, (int) (codec->qblur * 10000.0));
			pos += shm_w32(pos, codec->bit_rate_tolerance);
			pos += snprintf(pos, 64, "%s",
				codec->rc_eq ? codec->rc_eq : "tex^qComp");
			pos += shm_w8(pos, 0);
			pos += shm_w32(pos, codec->rc_max_rate);
			pos += shm_w32(pos, codec->rc_min_rate);
			pos += shm_w32(pos, codec->rc_buffer_size);
			pos += shm_w64(pos, av_double2int(codec->i_quant_factor));
			pos += shm_w64(pos, av_double2int(codec->b_quant_factor));
			pos += shm_w64(pos, av_double2int(codec->i_quant_offset));
			pos += shm_w64(pos, av_double2int(codec->b_quant_offset));
			pos += shm_w32(pos, codec->dct_algo);
			pos += shm_w32(pos, codec->strict_std_compliance);
			pos += shm_w32(pos, codec->max_b_frames);
			pos += shm_w32(pos, codec->mpeg_quant);
			pos += shm_w32(pos, codec->intra_dc_precision);
			pos += shm_w32(pos, codec->me_method);
			pos += shm_w32(pos, codec->mb_decision);
			pos += shm_w32(pos, codec->nsse_weight);
			pos += shm_w32(pos, codec->frame_skip_cmp);
			pos += shm_w64(pos, av_double2int(codec->rc_buffer_aggressivity));
			pos += shm_w32(pos, codec->codec_tag);
			pos += shm_w8(pos, codec->thread_count);
			pos += shm_w32(pos, codec->coder_type);
			pos += shm_w32(pos, codec->me_cmp);
			pos += shm_w32(pos, codec->me_subpel_quality);
			pos += shm_w32(pos, codec->me_range);
			pos += shm_w32(pos, codec->keyint_min);
			pos += shm_w32(pos, codec->scenechange_threshold);
			pos += shm_w32(pos, codec->b_frame_strategy);
			pos += shm_w64(pos, av_double2int(codec->qcompress));
			pos += shm_w64(pos, av_double2int(codec->qblur));
			pos += shm_w32(pos, codec->max_qdiff);
			pos += shm_w32(pos, codec->refs);
			/* pos += shm_w32(pos, MKBETAG('S', 'T', 'V', 'I')); */
			break;
		case AVMEDIA_TYPE_AUDIO:
			pos += shm_w32(pos, codec->sample_rate);
			pos += shm_w16(pos, codec->channels);
			pos += shm_w16(pos, codec->frame_size);
			pos += shm_w32(pos, codec->bits_per_coded_sample);
			pos += shm_w32(pos, codec->sample_fmt);
			/* pos += shm_w32(pos, MKBETAG('S', 'T', 'A', 'U')); */
			break;
		default:
			return -1;
		}
	}

	pos += shm_w64(pos, 0); // end of header

	/* flush until end of block reached */
	while(pos < (shm->shm_buf + SHM_HEADER_SIZE))
		pos += shm_w8(pos, 0);

	/* init packet mux */
	shm->ring_begin = shm->shm_buf + SHM_HEADER_SIZE;
	shm->ring_pos = shm->ring_begin;
	shm->ring_end = shm->shm_buf + shm->shm_size;
	shm->write_index = 0;
	shm->prev_index = -1;

	pos = shm->shm_buf + 20;
	shm->shm_update++;
	pos += shm_w32(pos, shm->shm_update);/* write current shm update */
	pos += shm_w32(pos, 0);/* clear need close */

	pos = shm->shm_buf;
	pos += shm_w32(pos, MKTAG('S', 'H', 'M', '1'));

	return 0;
}

static int shm_write_packet(AVFormatContext *s, AVPacket *pkt)
{
	SHMContext *shm = s->priv_data;
	SHMPacketHeader *h = NULL;
	SHMPacketHeader *next_h = NULL;
	int len = ((char *) shm->ring_end - (char *) shm->ring_pos);

	if (len < sizeof(*h) || (len % sizeof(*h))) {
		/* TODO: how to avoid this ? */
		av_log(s, AV_LOG_FATAL, "%s: wrong position of ring buffer \n",
				__func__);
		shm->ring_pos = shm->ring_begin;
		return AVERROR(EAGAIN);
	}

	if (pkt->size > (shm->shm_size - 2 * sizeof(*h))) {
		av_log(s, AV_LOG_ERROR, "%s Packt size(%d) is too big\n",
				__func__, pkt->size);
		return AVERROR(ENOMEM);
	}
	if (shm->prev_index != -1) {
		h = (SHMPacketHeader *) (shm->ring_begin + shm->prev_index);
		h->p.next_index = shm->ring_pos;
	}

	h = (SHMPacketHeader *) shm->ring_pos;
	h->p.id = PACKET_ID;
	h->p.flags = 0;
	h->p.duration = pkt->duration;
	h->p.size = pkt->size;
	h->p.stream_index = pkt->stream_index;
	h->p.pts = shm->start_time + pkt->pts;
	h->p.dts = shm->start_time + pkt->dts;
	h->p.fill_size = sizeof(*h) - pkt->size % (sizeof(*h));
	h->p.fill_size %= sizeof(*h);
	h->p.prev_index = shm->prev_index;
	h->p.next_index = -1;

	if (h->p.pts != h->p.dts)
		h->p.flags |= FLAG_DTS;
	if (pkt->flags & AV_PKT_FLAG_KEY)
		h->p.flags |= FLAG_KEY_FRAME;

	shm->ring_pos += sizeof(*h);
	len = ((char *) shm->ring_end - (char *) shm->ring_pos);
	if (len > (pkt->size + h->p.fill_size)) {
		memcpy(shm->ring_pos, pkt->data, pkt->size);
		shm->ring_pos += pkt->size + h->p.fill_size;
	} else { /* len <= (pkt->size + fill_size) */
		memcpy(shm->ring_pos, pkt->data, len);
		shm->ring_pos = shm->ring_begin;
		if (pkt->size - len > 0)
			memcpy(shm->ring_pos, pkt->data + len, pkt->size - len);
		shm->ring_pos += (pkt->size - len) + h->p.fill_size;
	}

	len = ((char *) shm->ring_end - (char *) shm->ring_pos);
	if (len < sizeof(*h) || (len % sizeof(*h))) {
		shm->ring_pos = shm->ring_begin;
	}

	next_h = (SHMPacketHeader *) shm->ring_pos;
	next_h->p.id = PACKET_ID;
	next_h->p.prev_index = (uint8_t *)h - shm->ring_begin;
	next_h->p.size = 0;

	shm_update_write_index(s);

	shm->prev_index = (uint8_t *)h - shm->ring_begin;

	return 0;
}

static int shm_write_trailer(AVFormatContext *s)
{
	SHMContext *shm = s->priv_data;
	struct shmid_ds buf;
	uint8_t *pos = NULL;

	pos = shm->shm_buf + 24;
	shm_w32(pos, 1);/* write need close */

	if (shm->shm_delete)
		shmctl(shm->shm_id, IPC_RMID, NULL);

	if (shm->shm_buf) {
		shmdt(shm->shm_buf);
		shm->shm_buf = NULL;
	}

	return 0;
}

AVOutputFormat ff_shm_muxer = {
    .name              = "shm",
    .long_name         = NULL_IF_CONFIG_SMALL("SHM (FFserver live feed)"),
    .extensions        = "shm",
    .priv_data_size    = sizeof(SHMContext),
    .audio_codec       = AV_CODEC_ID_MP2,
    .video_codec       = AV_CODEC_ID_MPEG1VIDEO,
    .write_header      = shm_write_header,
    .write_packet      = shm_write_packet,
    .write_trailer     = shm_write_trailer,
    .flags             = AVFMT_TS_NEGATIVE,
};
