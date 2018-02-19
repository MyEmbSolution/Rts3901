/*
 * RTP packetization for H.264 (RFC3984)
 * Copyright (c) 2008 Luca Abeni
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

/**
 * @file
 * @brief H.264 packetization
 * @author Luca Abeni <lucabe72@email.it>
 */

#include "avformat.h"
#include "avc.h"
#include "rtpenc.h"

static const uint8_t *avc_mp4_find_startcode(const uint8_t *start, const uint8_t *end, int nal_length_size)
{
    unsigned int res = 0;

    if (end - start < nal_length_size)
        return NULL;
    while (nal_length_size--)
        res = (res << 8) | *start++;

    if (res > end - start)
        return NULL;

    return start + res;
}

static void nal_send(AVFormatContext *s1, const uint8_t *buf, int size, int last)
{
    RTPMuxContext *s = s1->priv_data;

    av_log(s1, AV_LOG_DEBUG, "Sending NAL %x of len %d M=%d\n", buf[0] & 0x1F, size, last);
    if (size <= s->max_payload_size) {
        memcpy(s->buf, buf, size);
	ff_rtp_send_data_h264(s1, s->buf, size, last);
    } else {
        uint8_t type = buf[0] & 0x1F;
        uint8_t nri = buf[0] & 0x60;

        if (s->flags & FF_RTP_FLAG_H264_MODE0) {
            av_log(s1, AV_LOG_ERROR,
                   "NAL size %d > %d, try -slice-max-size %d\n", size,
                   s->max_payload_size, s->max_payload_size);
            return;
        }
        av_log(s1, AV_LOG_DEBUG, "NAL size %d > %d\n", size, s->max_payload_size);
        s->buf[0] = 28 | nri;        /* FU Indicator; Type = 28 ---> FU-A */
        s->buf[1] = type | (1 << 7);

        buf += 1;
        size -= 1;
        while (size + 2 > s->max_payload_size) {
            memcpy(&s->buf[2], buf, s->max_payload_size - 2);
            ff_rtp_send_data_h264(s1, s->buf, s->max_payload_size, 0);

            buf += s->max_payload_size - 2;
            size -= s->max_payload_size - 2;
            s->buf[1] &= ~(1 << 7);
        }
        s->buf[1] |= 1 << 6;

        memcpy(&s->buf[2], buf, size);
        ff_rtp_send_data_h264(s1, s->buf, size + 2, last);
    }
}

void ff_rtp_send_h264(AVFormatContext *s1, const uint8_t *buf1, int size, int flags)
{
    const uint8_t *r, *end = buf1 + size;
    RTPMuxContext *s = s1->priv_data;
    int max_seg = 0;
    uint8_t *sps, *pps, *iframe;

    s->timestamp = s->cur_timestamp;

    if (flags & AV_PKT_FLAG_KEY) {
        r = buf1;
        sps = ff_avc_find_startcode(buf1, end);
        sps += 4;
        pps = ff_avc_find_startcode(sps + 1, end);
        pps += 4;
        iframe = ff_avc_find_startcode(pps + 1, end);
        iframe += 4;

        nal_send(s1, sps, pps - sps - 4, 0);
        nal_send(s1, pps, iframe - pps - 4, 0);
        nal_send(s1, iframe, end - iframe,  1);
    } else {
        nal_send(s1, buf1 + 4, end - buf1 - 4, 1);
    }
}

