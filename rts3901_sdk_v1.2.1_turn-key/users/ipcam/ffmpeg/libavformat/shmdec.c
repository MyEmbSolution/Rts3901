/*
 * SHM (ffserver live feed) demuxer
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
#include <sys/shm.h>
#include <unistd.h>
#include "libavutil/intreadwrite.h"
#include "libavutil/intfloat.h"
#include "avformat.h"
#include "internal.h"
#include "shm.h"
#include "avio_internal.h"

#define SHM_RING_SAFE_DISTANCE (16 * 1024)

static void shm_set_index_and_filesize(AVFormatContext *s, int *shm_update, int *need_close)
{
	SHMContext *shm = s->priv_data;
	uint8_t *pos = shm->shm_buf;
	uint32_t tag = 0;

	pos += shm_r32(pos, &tag);
	pos += shm_r64(pos, &shm->shm_size);
	pos += shm_r64(pos, &shm->write_index);
	pos += shm_r32(pos, shm_update);
	pos += shm_r32(pos, need_close);
}

static int shm_is_avail_data(AVFormatContext *s, int size)
{
	SHMContext *shm = s->priv_data;
	int64_t avail_size;

	if (shm->read_index == shm->write_index)
		return 0;
	else if (shm->read_index < shm->write_index)
		avail_size = shm->write_index - shm->read_index;
	else
		avail_size = (shm->shm_size - SHM_HEADER_SIZE)
			- (shm->read_index - shm->write_index);

	if (size <= avail_size)
		return 1;
	else
		return 0;
}

static int shm_close(AVFormatContext *s)
{
    SHMContext *shm = s->priv_data;
    int i;

    for (i = 0; i < s->nb_streams; i++)
        av_freep(&s->streams[i]->codec->rc_eq);

    if (shm->shm_buf) {
	    shmdt(shm->shm_buf);
	    shm->shm_buf = NULL;
    }

    return 0;
}

static int shm_update_header(AVFormatContext *s)
{
	SHMContext *shm = s->priv_data;
	AVStream *st;
	AVIOContext *pb = s->pb;
	AVCodecContext *codec;
	uint8_t *pos = NULL;
	int i, nb_streams;
	uint32_t tag = 0;
	uint32_t bit_rate = 0;
	int ret = AVERROR(EAGAIN);;
	AVDictionaryEntry *t;
	int need_close = 0;

	pos = shm->shm_buf;

	/* header */
	pos += shm_r32(pos, &tag);
	if (tag != MKTAG('S', 'H', 'M', '1'))
		goto fail;
	pos += shm_r64(pos, &shm->shm_size);
	pos += shm_r64(pos, &shm->write_index);
	pos += shm_r32(pos, &shm->shm_update);
	pos += shm_r32(pos, &need_close);
	pos += shm_r32(pos, &nb_streams);
	pos += shm_r32(pos, &bit_rate);

	/* read each stream */
	for(i=0;i<nb_streams;i++) {
		char rc_eq_buf[128];
		int old_w, old_h, old_n, old_d;

		st = s->streams[i];

		avpriv_set_pts_info(st, 64, 1, 1000000);

		codec = st->codec;
		/* generic info */
		pos += shm_r32(pos, &codec->codec_id);
		{
			uint8_t tmp = 0;
			pos += shm_r8(pos, &tmp);
			codec->codec_type = tmp;
		}
		pos += shm_r32(pos, &codec->bit_rate);
		pos += shm_r32(pos, &codec->flags);
		pos += shm_r32(pos, &codec->flags2);
		pos += shm_r32(pos, &codec->debug);

		if (codec->flags & CODEC_FLAG_GLOBAL_HEADER) {
		    char *old_extradata = NULL;
		    int old_extradata_size = codec->extradata_size;
		    if (codec->extradata != NULL) {
			old_extradata = av_malloc(codec->extradata_size);
			if (old_extradata == NULL)
			    return AVERROR(ENOMEM);
			memcpy(old_extradata, codec->extradata, codec->extradata_size);
			av_free(codec->extradata);
		    }
		    pos += shm_r32(pos, &codec->extradata_size);
		    if (ff_alloc_extradata(codec, codec->extradata_size))
			    return AVERROR(ENOMEM);
		    pos += shm_rbuf(pos, codec->extradata, codec->extradata_size);

		    if (old_extradata_size != 0
			&& ((old_extradata_size != codec->extradata_size)
			|| memcmp(old_extradata, codec->extradata, old_extradata_size)))
			ret = AVERROR(EPROTO);

		    if (old_extradata_size)
			av_free(old_extradata);
		}

		/* specific info */
		switch(codec->codec_type) {
		case AVMEDIA_TYPE_VIDEO:
			old_w = codec->width;
			old_h = codec->height;
			old_n = codec->time_base.num;
			old_d = codec->time_base.den;

			pos += shm_r32(pos, &codec->time_base.num);
			pos += shm_r32(pos, &codec->time_base.den);
			{
				uint16_t tmp;

				pos += shm_r16(pos, &tmp);
				codec->width = tmp;

				pos += shm_r16(pos, &tmp);
				codec->height = tmp;

				pos += shm_r16(pos, &tmp);
				codec->gop_size = tmp;
			}

			if (old_w != codec->width
			    || old_h != codec->height
			    || old_n != codec->time_base.num
			    || old_d != codec->time_base.den)
			    ret = AVERROR(EPROTO);

			pos += shm_r32(pos, (uint32_t*) &codec->pix_fmt);
			{
				uint8_t tmp;
				pos += shm_r8(pos, &tmp);
				codec->qmin = tmp;

				pos += shm_r8(pos, &tmp);
				codec->qmax = tmp;

				pos += shm_r8(pos, &tmp);
				codec->max_qdiff = tmp;
			}

			{
				uint16_t tmp;
				pos += shm_r16(pos, &tmp);
				codec->qcompress = tmp / 10000.0;

				pos += shm_r16(pos, &tmp);
				codec->qblur = tmp / 10000.0;
			}

			pos += shm_r32(pos, &codec->bit_rate_tolerance);
			pos += snprintf(rc_eq_buf, sizeof(rc_eq_buf), "%s",
					pos);
			pos++;
			codec->rc_eq = av_strdup(rc_eq_buf);

			pos += shm_r32(pos, &codec->rc_max_rate);
			pos += shm_r32(pos, &codec->rc_min_rate);
			pos += shm_r32(pos, &codec->rc_buffer_size);
			pos += shm_r64(pos,
					(uint64_t *) &codec->i_quant_factor);
			codec->i_quant_factor =
				av_int2double(codec->i_quant_factor);
			pos += shm_r64(pos,
					(uint64_t *) &codec->b_quant_factor);
			codec->b_quant_factor =
				av_int2double(codec->b_quant_factor);
			pos += shm_r64(pos,
					(uint64_t *) &codec->i_quant_offset);
			codec->i_quant_offset =
				av_int2double(codec->i_quant_offset);
			pos += shm_r64(pos,
					(uint64_t *) &codec->b_quant_offset);
			codec->b_quant_offset =
				av_int2double(codec->b_quant_offset);
			pos += shm_r32(pos, &codec->dct_algo);
			pos += shm_r32(pos, &codec->strict_std_compliance);
			pos += shm_r32(pos, &codec->max_b_frames);
			pos += shm_r32(pos, &codec->mpeg_quant);
			pos += shm_r32(pos, &codec->intra_dc_precision);
			pos += shm_r32(pos, &codec->me_method);
			pos += shm_r32(pos, &codec->mb_decision);
			pos += shm_r32(pos, &codec->nsse_weight);
			pos += shm_r32(pos, &codec->frame_skip_cmp);
			pos += shm_r64(pos,
					(uint64_t *) &codec->rc_buffer_aggressivity);
			codec->rc_buffer_aggressivity =
				av_int2double(codec->rc_buffer_aggressivity);
			pos += shm_r32(pos, &codec->codec_tag);

			{
				uint8_t tmp;
				pos += shm_r8(pos, &tmp);
				codec->thread_count = tmp;
			}

			pos += shm_r32(pos, &codec->coder_type);
			pos += shm_r32(pos, &codec->me_cmp);
			pos += shm_r32(pos, &codec->me_subpel_quality);
			pos += shm_r32(pos, &codec->me_range);
			pos += shm_r32(pos, &codec->keyint_min);
			pos += shm_r32(pos, &codec->scenechange_threshold);
			pos += shm_r32(pos, &codec->b_frame_strategy);
			pos += shm_r64(pos, (uint64_t *) &codec->qcompress);
			codec->qcompress = av_int2double(codec->qcompress);
			pos += shm_r64(pos, (uint64_t *) &codec->qblur);
			codec->qblur = av_int2double(codec->qblur);
			pos += shm_r32(pos, &codec->max_qdiff);
			pos += shm_r32(pos, &codec->refs);
			break;
		case AVMEDIA_TYPE_AUDIO:
			pos += shm_r32(pos, &codec->sample_rate);
			{
				uint16_t tmp;
				pos += shm_r16(pos, &tmp);
				codec->channels = tmp;

				pos += shm_r16(pos, &tmp);
				codec->frame_size = tmp;
			}
			pos += shm_r32(pos, &codec->bits_per_coded_sample);
			pos += shm_r32(pos, &codec->sample_fmt);

			break;
		default:
			goto fail;
		}
	}

	/* init packet demux */
	shm->ring_begin = shm->shm_buf + SHM_HEADER_SIZE;
	shm->ring_pos = shm->ring_begin;
	shm->ring_end = shm->shm_buf + shm->shm_size;

	shm->read_index = shm->write_index;

	return ret;
fail:
	return -1;
}

static int shm_read_header(AVFormatContext *s)
{
	SHMContext *shm = s->priv_data;
	AVStream *st;
	AVIOContext *pb = s->pb;
	AVCodecContext *codec;
	uint8_t *pos = NULL;
	int i, nb_streams;
	uint32_t tag = 0;
	uint32_t bit_rate = 0;
	int ret = 0;
	AVDictionaryEntry *t;
	int need_close = 0;

	shm->shm_size = 0;
	ret = shm_create_shm(s);
	if (ret < 0)
		return ret;

	pos = shm->shm_buf;

	/* header */
	pos += shm_r32(pos, &tag);
	if (tag != MKTAG('S', 'H', 'M', '1'))
		goto fail;

	pos += shm_r64(pos, &shm->shm_size);
	pos += shm_r64(pos, &shm->write_index);
	pos += shm_r32(pos, &shm->shm_update);
	pos += shm_r32(pos, &need_close);
	pos += shm_r32(pos, &nb_streams);
	pos += shm_r32(pos, &bit_rate);

	/* read each stream */
	for(i=0;i<nb_streams;i++) {
		char rc_eq_buf[128];

		st = avformat_new_stream(s, NULL);
		if (!st)
			goto fail;

		avpriv_set_pts_info(st, 64, 1, 1000000);

		codec = st->codec;
		/* generic info */
		pos += shm_r32(pos, &codec->codec_id);
		{
			uint8_t tmp = 0;
			pos += shm_r8(pos, &tmp);
			codec->codec_type = tmp;
		}
		pos += shm_r32(pos, &codec->bit_rate);
		pos += shm_r32(pos, &codec->flags);
		pos += shm_r32(pos, &codec->flags2);
		pos += shm_r32(pos, &codec->debug);

		if (codec->flags & CODEC_FLAG_GLOBAL_HEADER) {
			pos += shm_r32(pos, &codec->extradata_size);
			if (ff_alloc_extradata(codec, codec->extradata_size))
				return AVERROR(ENOMEM);
			pos += shm_rbuf(pos, codec->extradata, codec->extradata_size);
		}

		/* specific info */
		switch(codec->codec_type) {
		case AVMEDIA_TYPE_VIDEO:
			pos += shm_r32(pos, &codec->time_base.num);
			pos += shm_r32(pos, &codec->time_base.den);
			{
				uint16_t tmp;

				pos += shm_r16(pos, &tmp);
				codec->width = tmp;

				pos += shm_r16(pos, &tmp);
				codec->height = tmp;

				pos += shm_r16(pos, &tmp);
				codec->gop_size = tmp;
			}

			pos += shm_r32(pos, (uint32_t*) &codec->pix_fmt);
			{
				uint8_t tmp;
				pos += shm_r8(pos, &tmp);
				codec->qmin = tmp;

				pos += shm_r8(pos, &tmp);
				codec->qmax = tmp;

				pos += shm_r8(pos, &tmp);
				codec->max_qdiff = tmp;
			}

			{
				uint16_t tmp;
				pos += shm_r16(pos, &tmp);
				codec->qcompress = tmp / 10000.0;

				pos += shm_r16(pos, &tmp);
				codec->qblur = tmp / 10000.0;
			}

			pos += shm_r32(pos, &codec->bit_rate_tolerance);
			pos += snprintf(rc_eq_buf, sizeof(rc_eq_buf), "%s",
					pos);
			pos++;
			codec->rc_eq = av_strdup(rc_eq_buf);

			pos += shm_r32(pos, &codec->rc_max_rate);
			pos += shm_r32(pos, &codec->rc_min_rate);
			pos += shm_r32(pos, &codec->rc_buffer_size);
			pos += shm_r64(pos,
					(uint64_t *) &codec->i_quant_factor);
			codec->i_quant_factor =
				av_int2double(codec->i_quant_factor);
			pos += shm_r64(pos,
					(uint64_t *) &codec->b_quant_factor);
			codec->b_quant_factor =
				av_int2double(codec->b_quant_factor);
			pos += shm_r64(pos,
					(uint64_t *) &codec->i_quant_offset);
			codec->i_quant_offset =
				av_int2double(codec->i_quant_offset);
			pos += shm_r64(pos,
					(uint64_t *) &codec->b_quant_offset);
			codec->b_quant_offset =
				av_int2double(codec->b_quant_offset);
			pos += shm_r32(pos, &codec->dct_algo);
			pos += shm_r32(pos, &codec->strict_std_compliance);
			pos += shm_r32(pos, &codec->max_b_frames);
			pos += shm_r32(pos, &codec->mpeg_quant);
			pos += shm_r32(pos, &codec->intra_dc_precision);
			pos += shm_r32(pos, &codec->me_method);
			pos += shm_r32(pos, &codec->mb_decision);
			pos += shm_r32(pos, &codec->nsse_weight);
			pos += shm_r32(pos, &codec->frame_skip_cmp);
			pos += shm_r64(pos,
					(uint64_t *) &codec->rc_buffer_aggressivity);
			codec->rc_buffer_aggressivity =
				av_int2double(codec->rc_buffer_aggressivity);
			pos += shm_r32(pos, &codec->codec_tag);

			{
				uint8_t tmp;
				pos += shm_r8(pos, &tmp);
				codec->thread_count = tmp;
			}

			pos += shm_r32(pos, &codec->coder_type);
			pos += shm_r32(pos, &codec->me_cmp);
			pos += shm_r32(pos, &codec->me_subpel_quality);
			pos += shm_r32(pos, &codec->me_range);
			pos += shm_r32(pos, &codec->keyint_min);
			pos += shm_r32(pos, &codec->scenechange_threshold);
			pos += shm_r32(pos, &codec->b_frame_strategy);
			pos += shm_r64(pos, (uint64_t *) &codec->qcompress);
			codec->qcompress = av_int2double(codec->qcompress);
			pos += shm_r64(pos, (uint64_t *) &codec->qblur);
			codec->qblur = av_int2double(codec->qblur);
			pos += shm_r32(pos, &codec->max_qdiff);
			pos += shm_r32(pos, &codec->refs);
			break;
		case AVMEDIA_TYPE_AUDIO:
			pos += shm_r32(pos, &codec->sample_rate);
			{
				uint16_t tmp;
				pos += shm_r16(pos, &tmp);
				codec->channels = tmp;

				pos += shm_r16(pos, &tmp);
				codec->frame_size = tmp;
			}
			pos += shm_r32(pos, &codec->bits_per_coded_sample);
			pos += shm_r32(pos, &codec->sample_fmt);

			break;
		default:
			goto fail;
		}
	}

	/* init packet demux */
	shm->ring_begin = shm->shm_buf + SHM_HEADER_SIZE;
	shm->ring_pos = shm->ring_begin;
	shm->ring_end = shm->shm_buf + shm->shm_size;

	if (t = av_dict_get(s->metadata, "shm_read_position", NULL, 0))
		shm->read_index = strtoll(t->value, NULL, 0);
	else
		shm->read_index = shm->write_index;

	return 0;
fail:
	shm_close(s);
	return -1;
}

/* return < 0 if eof */
static int shm_read_packet(AVFormatContext *s, AVPacket *pkt)
{
	SHMContext *shm = s->priv_data;
	SHMPacketHeader *h = NULL;
	int size = 0;
	int ret = 0;
	uint8_t *pos = NULL;
	int shm_update = 0;
	SHMPacketHeader shm_packet;
	int need_close = 0;
	int err;

	if (shm->shm_buf == NULL) {
		err = shm_read_header(s);
		if (err)
			return AVERROR(EAGAIN);
		else
			return AVERROR(EPROTO);
	}

	shm_set_index_and_filesize(s, &shm_update, &need_close);
	if (need_close) {
		shm_close(s);
		return AVERROR(EAGAIN);
	} else {
		if (shm_update != shm->shm_update)
			return shm_update_header(s);
	}
	if (shm->read_index >= shm->ring_end - shm->ring_begin
		|| shm->read_index < 0) {
		shm->read_index = shm->write_index;
		return AVERROR(EAGAIN);
	}

	h = (SHMPacketHeader *) (shm->ring_begin + shm->read_index);

	shm_packet = *h;
	size = shm_packet.p.size;
	if (size == 0)
		return AVERROR(EAGAIN);

	ret = shm_is_avail_data(s, sizeof(*h));
	if (!ret)
		return AVERROR(EAGAIN);

	if (PACKET_ID != shm_packet.p.id) {
		av_log(s, AV_LOG_ERROR,
			"%s: wrong position of ring buffer \n", __func__);
		shm->read_index = shm->write_index;
		return AVERROR(EAGAIN);
	}

	if (size > (shm->shm_size - sizeof(SHMPacketHeader))) {
		shm->read_index = shm->write_index;
		return AVERROR(EAGAIN);
	}
	ret = av_new_packet(pkt, size);
	if (ret < 0)
		return AVERROR(ENOMEM);

	if (shm_packet.p.flags & FLAG_KEY_FRAME)
		pkt->flags |= AV_PKT_FLAG_KEY;

	pos = shm->ring_begin + shm->read_index;
	pos += sizeof(*h);

	if ((pos + size) <= shm->ring_end) {
		memcpy(pkt->data, pos, size);
		pos += size + shm_packet.p.fill_size;
	} else {
		int sub_len = shm->ring_end - pos;
		memcpy(pkt->data, pos, sub_len);
		pos = shm->ring_begin;
		memcpy(((uint8_t *) pkt->data) + sub_len, pos, size - sub_len);
		pos += (size - sub_len) + shm_packet.p.fill_size;
	}

	if (pos >= shm->ring_end)
		pos = shm->ring_begin;

	pkt->pts = shm_packet.p.pts;
	pkt->dts = shm_packet.p.dts;
	pkt->stream_index = shm_packet.p.stream_index;
	shm->read_index = pos - shm->ring_begin;

	return 0;
}

/* seek to a given time in the file. The file read pointer is
   positioned at or before pts. XXX: the following code is quite
   approximative */
static int shm_seek(AVFormatContext *s, int stream_index,
		int64_t wanted_pts, int flags)
{
	SHMContext *shm = s->priv_data;
	SHMPacketHeader *h = NULL;
	int size = 0;
	int ret = 0;
	uint8_t *pos = NULL;
	int64_t read_index;
	uint64_t prev_pts = 0;
	int shm_update = 0;
	int need_close = 0;

	shm_set_index_and_filesize(s, &shm_update, &need_close);
	if (need_close)
		return -1;

	if (shm->shm_update != shm_update)
		return -1;

	read_index = shm->read_index;
	h = (SHMPacketHeader *) (shm->ring_begin + read_index);

	if (read_index == shm->write_index)
		h = (SHMPacketHeader *) (shm->ring_begin + h->p.prev_index);

	if (wanted_pts > h->p.pts) {
		while (wanted_pts > h->p.pts) {
			if (h->p.next_index != -1
				&& h->p.next_index != shm->write_index) {
				read_index = h->p.next_index;
				h = (SHMPacketHeader *) (shm->ring_begin + read_index);
			} else {
				ret = -1;
				break;
			}
		}
		if (wanted_pts <= h->p.pts) {
			shm->read_index = read_index;
			ret = 0;
		}
	} else if (wanted_pts < h->p.pts) {
		while (wanted_pts < h->p.pts) {
			if (h->p.prev_index != -1) {
				int interval;

				if (h->p.prev_index >= shm->write_index)
					interval = h->p.prev_index - shm->write_index;
				else
					interval = shm->shm_size - SHM_HEADER_SIZE
						- shm->write_index + h->p.prev_index;
				if (interval < SHM_RING_SAFE_DISTANCE) {
					ret = -1;
					break;
				}
				read_index = h->p.prev_index;
				h = (SHMPacketHeader *) (shm->ring_begin + read_index);
			} else {
				ret = -1;
				break;
			}
		}
		if (wanted_pts >= h->p.pts) {
			shm->read_index = read_index;
			ret = 0;
		}
	}

	return ret;
}

static int64_t shm_read_timestamp(struct AVFormatContext *s, int stream_index,
                              int64_t *pos, int64_t pos_limit)
{
	SHMContext *shm = s->priv_data;
	SHMPacketHeader *h = NULL;
	int shm_update = 0;
	int need_close = 0;
	SHMPacketHeader *curr_h = NULL;

	shm_set_index_and_filesize(s, &shm_update, &need_close);
	if (need_close)
		return -1;

	if (shm->shm_update != shm_update)
		return 0;
	if (shm->read_index == shm->write_index) {
		curr_h = (SHMPacketHeader *) (shm->ring_begin + shm->read_index);
		if (PACKET_ID != curr_h->p.id)
			return 0;
		h = (SHMPacketHeader *) (shm->ring_begin + curr_h->p.prev_index);
	} else
		h = (SHMPacketHeader *) (shm->ring_begin + shm->read_index);

	if (PACKET_ID != h->p.id)
		return 0;

	return h->p.pts;
}

static int shm_probe(AVProbeData *p)
{
	return 0;
}

AVInputFormat ff_shm_demuxer = {
	.name           = "shm",
	.long_name      = NULL_IF_CONFIG_SMALL("SHM (FFserver live feed)"),
	.priv_data_size = sizeof(SHMContext),
	.read_probe     = shm_probe,
	.read_header    = shm_read_header,
	.read_packet    = shm_read_packet,
	.read_close     = shm_close,
	.read_seek      = shm_seek,
	.read_timestamp = shm_read_timestamp,
	.flags          = AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH,
};
