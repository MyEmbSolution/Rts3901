/*
 * RTP network protocol
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

#ifndef AVFORMAT_RTPTCPPROTO_H
#define AVFORMAT_RTPTCPPROTO_H

typedef struct RTPTcpFixHeader {
    /* byte 0 */
    unsigned char magic;

    /* byte 1 */
    unsigned char channel;

    /* bytes 2, 3 */
    unsigned short len;
} RTPTcpFixHeader ;

#define RTP_MAX_FU_SIZE  200

enum {
    RTP_BUF_IDLE = 0,
    RTP_BUF_READY,
    RTP_BUF_READY_SECOND,
    RTP_BUF_READING,
    RTP_BUF_WRITING
};

typedef struct RTPBufInfo {
    int  buf_num;
    uint8_t **buf;
    int  buf_size;
    int  data_num;
    int  *data_len;
    int  status;
} RTPBufInfo;

typedef struct RTPRtspBufContext {
    RTPBufInfo rtp_buf1_info;
    RTPBufInfo rtp_buf2_info;
    RTPBufInfo rtp_buf3_info;

    RTPBufInfo rtcp_buf1_info;
    RTPBufInfo rtcp_buf2_info;
    pthread_mutex_t mutex;
    int init_ok;
} RTPRtspBufContext;

typedef struct RTPTcpContext {
    int rtp_channel;
    int rtcp_channel;
    RTPRtspBufContext rtp_rtsp_buf_st;
} RTPTcpContext;

void *ff_rtp_get_send_buf(void *s1);
void ff_rtp_put_send_buf(void *s1, void *info);
void *ff_rtcp_get_send_buf(void *s1);

#endif /* AVFORMAT_RTPPROTO_H */
