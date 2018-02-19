/*
 * RTP network protocol
 * Copyright (c) 2002 Fabrice Bellard
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
 * RTP protocol
 */

#include "libavutil/parseutils.h"
#include "libavutil/avstring.h"
#include "avformat.h"
#include "avio_internal.h"
#include "rtp.h"
#include "rtptcpproto.h"
#include "url.h"

#include <stdarg.h>
#include "internal.h"
#include "network.h"
#include "os_support.h"
#include <fcntl.h>
#include <pthread.h>

#if HAVE_POLL_H
#include <sys/poll.h>
#endif

static int ff_rtp_sub_buf_init(RTPBufInfo *buf_info, int buf_num, int buf_size);
static int ff_rtp_sub_buf_uninit(RTPBufInfo *buf_info);
static int ff_rtp_buf_context_init(RTPTcpContext *s, int buf_size);
static int ff_rtp_buf_context_uninit(RTPTcpContext *s);
static RTPBufInfo *ff_rtp_swap_write_buf(URLContext *h);
static void ff_rtp_update_write_buf(URLContext *h,
	uint8_t *buf, int len);
static void ff_rtcp_update_write_buf(URLContext *h,
	uint8_t *buf, int len);

static int ff_rtp_sub_buf_init(RTPBufInfo *buf_info, int buf_num, int buf_size)
{
    int i, j;

    buf_info->buf_num = buf_num;

    buf_info->buf = av_malloc(sizeof(uint8_t *) * buf_num);
    if (buf_info->buf == NULL)
        return AVERROR(ENOMEM);


    buf_info->data_len = av_malloc(sizeof(int) * buf_num);
    if (buf_info->data_len == NULL) {
        av_free(buf_info->buf);
        buf_info->buf = NULL;
        return AVERROR(ENOMEM);
    }

    for (i=0; i<buf_num; i++) {
        buf_info->buf[i] = av_malloc(buf_size);
        if (buf_info->buf[i] == NULL) {
            av_free(buf_info->data_len);
            buf_info->data_len = NULL;

            for (j=0; j<i; j++) {
                av_free(buf_info->buf[j]);
                buf_info->buf[j] = NULL;
            }

            av_free(buf_info->buf);
            buf_info->buf = NULL;

            return AVERROR(ENOMEM);
        }

        buf_info->data_len[i] = 0;
    }

    buf_info->buf_size = buf_size;
    buf_info->data_num = 0;

    buf_info->status = RTP_BUF_IDLE;

    return 0;
}

static int ff_rtp_sub_buf_uninit(RTPBufInfo *buf_info)
{
    int i;

    for (i=0; i<buf_info->buf_num; i++) {
        if (buf_info->buf[i])
            av_free(buf_info->buf[i]);
    }

    buf_info->buf_num = 0;
    buf_info->status = RTP_BUF_IDLE;

    if (buf_info->buf)
        av_free(buf_info->buf);
    if (buf_info->data_len)
        av_free(buf_info->data_len);

    return 0;
}

static int ff_rtp_buf_context_init(RTPTcpContext *s, int buf_size)
{
    RTPRtspBufContext *bufctx = &(s->rtp_rtsp_buf_st);
    int ret;

    bufctx->init_ok = 0;

    ret = ff_rtp_sub_buf_init(&bufctx->rtp_buf1_info, RTP_MAX_FU_SIZE, buf_size);
    if (ret != 0)
        return AVERROR(ENOMEM);

    ret = ff_rtp_sub_buf_init(&bufctx->rtp_buf2_info, RTP_MAX_FU_SIZE, buf_size);
    if (ret != 0) {
        ff_rtp_sub_buf_uninit(&bufctx->rtp_buf1_info);
        return AVERROR(ENOMEM);
    }

    ret = ff_rtp_sub_buf_init(&bufctx->rtp_buf3_info, RTP_MAX_FU_SIZE, buf_size);
    if (ret != 0) {
        ff_rtp_sub_buf_uninit(&bufctx->rtp_buf1_info);
        ff_rtp_sub_buf_uninit(&bufctx->rtp_buf2_info);
        return AVERROR(ENOMEM);
    }

    ret = ff_rtp_sub_buf_init(&bufctx->rtcp_buf1_info, 1, buf_size);
    if (ret != 0) {
        ff_rtp_sub_buf_uninit(&bufctx->rtp_buf1_info);
        ff_rtp_sub_buf_uninit(&bufctx->rtp_buf2_info);
        ff_rtp_sub_buf_uninit(&bufctx->rtp_buf3_info);
        return AVERROR(ENOMEM);
    }

    ret = ff_rtp_sub_buf_init(&bufctx->rtcp_buf2_info, 1, buf_size);
    if (ret != 0) {
        ff_rtp_sub_buf_uninit(&bufctx->rtp_buf1_info);
        ff_rtp_sub_buf_uninit(&bufctx->rtp_buf2_info);
        ff_rtp_sub_buf_uninit(&bufctx->rtp_buf3_info);
        ff_rtp_sub_buf_uninit(&bufctx->rtcp_buf1_info);
        return AVERROR(ENOMEM);
    }

    ret = pthread_mutex_init(&bufctx->mutex, NULL);
    if (ret != 0) {
        ff_rtp_sub_buf_uninit(&bufctx->rtp_buf1_info);
        ff_rtp_sub_buf_uninit(&bufctx->rtp_buf2_info);
        ff_rtp_sub_buf_uninit(&bufctx->rtp_buf3_info);
        ff_rtp_sub_buf_uninit(&bufctx->rtcp_buf1_info);
        ff_rtp_sub_buf_uninit(&bufctx->rtcp_buf2_info);
        return AVERROR(ENOMEM);
    }

    bufctx->init_ok = 1;

    bufctx->rtp_buf1_info.status = RTP_BUF_WRITING;
    bufctx->rtp_buf2_info.status = RTP_BUF_IDLE;
    bufctx->rtp_buf3_info.status = RTP_BUF_IDLE;

    return 0;
}

static int ff_rtp_buf_context_uninit(RTPTcpContext *s)
{
    RTPRtspBufContext *bufctx = &(s->rtp_rtsp_buf_st);

    if(bufctx->init_ok == 0)
        return 0;

    pthread_mutex_lock(&bufctx->mutex);

    ff_rtp_sub_buf_uninit(&bufctx->rtp_buf1_info);
    ff_rtp_sub_buf_uninit(&bufctx->rtp_buf2_info);
    ff_rtp_sub_buf_uninit(&bufctx->rtp_buf3_info);
    ff_rtp_sub_buf_uninit(&bufctx->rtcp_buf1_info);
    ff_rtp_sub_buf_uninit(&bufctx->rtcp_buf2_info);

    bufctx->init_ok = 0;

    pthread_mutex_unlock(&bufctx->mutex);

    pthread_mutex_destroy(&bufctx->mutex);

    return 0;
}

static void ff_rtp_update_write_buf(URLContext *h,
	uint8_t *buf, int len)
{
    RTPTcpContext *s = h->priv_data;
    RTPRtspBufContext *bufctx = &(s->rtp_rtsp_buf_st);
    RTPBufInfo *buf_info = NULL;

    if(bufctx->init_ok == 0)
        return;

    pthread_mutex_lock(&bufctx->mutex);

    if (bufctx->rtp_buf1_info.status == RTP_BUF_WRITING)
        buf_info = &bufctx->rtp_buf1_info;
    else if (bufctx->rtp_buf2_info.status == RTP_BUF_WRITING)
        buf_info = &bufctx->rtp_buf2_info;
    else if (bufctx->rtp_buf3_info.status == RTP_BUF_WRITING)
        buf_info = &bufctx->rtp_buf3_info;

    if (buf_info) {
        uint8_t *write_ptr = buf_info->buf[buf_info->data_num];
        memcpy(write_ptr, buf, len);

        buf_info->data_len[buf_info->data_num] = len;

        buf_info->data_num++;

        if (buf_info->data_num == buf_info->buf_num)
	    ff_rtp_swap_write_buf(h);
    } else
	av_log(h, AV_LOG_ERROR, "rtptcp drop: no writable buf\n");


    pthread_mutex_unlock(&bufctx->mutex);

    return;
}

static RTPBufInfo *ff_rtp_swap_write_buf(URLContext *h)
{
    RTPTcpContext *s = h->priv_data;
    RTPRtspBufContext *bufctx = &(s->rtp_rtsp_buf_st);
    RTPBufInfo *write_buf_info = NULL;
    RTPBufInfo *idle_buf_info = NULL;
    RTPBufInfo *ready_buf_info = NULL;
    RTPBufInfo *ready_second_buf_info = NULL;
    RTPBufInfo *read_buf_info = NULL;

    if (bufctx->init_ok == 0)
        return NULL;

    if (bufctx->rtp_buf1_info.status == RTP_BUF_WRITING) {
	write_buf_info = &bufctx->rtp_buf1_info;

        if (bufctx->rtp_buf2_info.status == RTP_BUF_IDLE)
            idle_buf_info = &bufctx->rtp_buf2_info;
        else if (bufctx->rtp_buf3_info.status == RTP_BUF_IDLE)
            idle_buf_info = &bufctx->rtp_buf3_info;

        if (bufctx->rtp_buf2_info.status == RTP_BUF_READY)
            ready_buf_info = &bufctx->rtp_buf2_info;
        else if (bufctx->rtp_buf3_info.status == RTP_BUF_READY)
            ready_buf_info = &bufctx->rtp_buf3_info;

        if (bufctx->rtp_buf2_info.status == RTP_BUF_READY_SECOND)
            ready_second_buf_info = &bufctx->rtp_buf2_info;
        else if (bufctx->rtp_buf3_info.status == RTP_BUF_READY_SECOND)
            ready_second_buf_info = &bufctx->rtp_buf3_info;

        if (bufctx->rtp_buf2_info.status == RTP_BUF_READING)
            read_buf_info = &bufctx->rtp_buf2_info;
        else if (bufctx->rtp_buf3_info.status == RTP_BUF_READING)
            read_buf_info = &bufctx->rtp_buf3_info;
    } else if (bufctx->rtp_buf2_info.status == RTP_BUF_WRITING) {
	write_buf_info = &bufctx->rtp_buf2_info;

        if (bufctx->rtp_buf1_info.status == RTP_BUF_IDLE)
            idle_buf_info = &bufctx->rtp_buf1_info;
        else if (bufctx->rtp_buf3_info.status == RTP_BUF_IDLE)
            idle_buf_info = &bufctx->rtp_buf3_info;

        if (bufctx->rtp_buf1_info.status == RTP_BUF_READY)
            ready_buf_info = &bufctx->rtp_buf1_info;
        else if (bufctx->rtp_buf3_info.status == RTP_BUF_READY)
            ready_buf_info = &bufctx->rtp_buf3_info;

        if (bufctx->rtp_buf1_info.status == RTP_BUF_READY_SECOND)
            ready_second_buf_info = &bufctx->rtp_buf1_info;
        else if (bufctx->rtp_buf3_info.status == RTP_BUF_READY_SECOND)
            ready_second_buf_info = &bufctx->rtp_buf3_info;

        if (bufctx->rtp_buf1_info.status == RTP_BUF_READING)
            read_buf_info = &bufctx->rtp_buf1_info;
        else if (bufctx->rtp_buf3_info.status == RTP_BUF_READING)
            read_buf_info = &bufctx->rtp_buf3_info;
    } else if (bufctx->rtp_buf3_info.status == RTP_BUF_WRITING) {
        write_buf_info = &bufctx->rtp_buf3_info;

        if (bufctx->rtp_buf1_info.status == RTP_BUF_IDLE)
            idle_buf_info = &bufctx->rtp_buf1_info;
        else if (bufctx->rtp_buf2_info.status == RTP_BUF_IDLE)
            idle_buf_info = &bufctx->rtp_buf2_info;

        if (bufctx->rtp_buf1_info.status == RTP_BUF_READY)
            ready_buf_info = &bufctx->rtp_buf1_info;
        else if (bufctx->rtp_buf2_info.status == RTP_BUF_READY)
            ready_buf_info = &bufctx->rtp_buf2_info;

        if (bufctx->rtp_buf1_info.status == RTP_BUF_READY_SECOND)
            ready_second_buf_info = &bufctx->rtp_buf1_info;
        else if (bufctx->rtp_buf2_info.status == RTP_BUF_READY_SECOND)
            ready_second_buf_info = &bufctx->rtp_buf2_info;

        if (bufctx->rtp_buf1_info.status == RTP_BUF_READING)
            read_buf_info = &bufctx->rtp_buf1_info;
        else if (bufctx->rtp_buf2_info.status == RTP_BUF_READING)
            read_buf_info = &bufctx->rtp_buf2_info;
    }

    if (write_buf_info->data_num == 0)
        return NULL;

    if (ready_buf_info && ready_second_buf_info) {
        ready_second_buf_info->status = RTP_BUF_READY;
        write_buf_info->status = RTP_BUF_READY_SECOND;
        ready_buf_info->status = RTP_BUF_WRITING;
        ready_buf_info->data_num = 0;

	av_log(h, AV_LOG_ERROR, "rtptcp drop: ready and ready second\n");

        return ready_second_buf_info;
    } else if (ready_buf_info && idle_buf_info) {
        write_buf_info->status = RTP_BUF_READY_SECOND;
        idle_buf_info->status = RTP_BUF_WRITING;
        idle_buf_info->data_num = 0;
        return ready_buf_info;
    } else if (ready_second_buf_info && idle_buf_info) {
        ready_second_buf_info->status = RTP_BUF_READY;
        write_buf_info->status = RTP_BUF_READY_SECOND;
        idle_buf_info->status = RTP_BUF_WRITING;
        idle_buf_info->data_num = 0;
        return ready_second_buf_info;
    }  else if (ready_buf_info && read_buf_info) {
        ready_buf_info->status = RTP_BUF_WRITING;
        ready_buf_info->data_num = 0;

	av_log(h, AV_LOG_ERROR, "rtptcp drop: ready and read\n");

        write_buf_info->status = RTP_BUF_READY;
        return write_buf_info;
    } else if (ready_second_buf_info && read_buf_info) {
        ready_second_buf_info->status = RTP_BUF_WRITING;
        ready_second_buf_info->data_num = 0;
        write_buf_info->status = RTP_BUF_READY;

	av_log(h, AV_LOG_ERROR, "rtptcp drop: ready second and read\n");

        return write_buf_info;
    } else if (idle_buf_info && read_buf_info) {
        write_buf_info->status = RTP_BUF_READY;
        idle_buf_info->status = RTP_BUF_WRITING;
        idle_buf_info->data_num = 0;
        return write_buf_info;
    } else if (idle_buf_info){
        write_buf_info->status = RTP_BUF_READY;
        idle_buf_info->status = RTP_BUF_WRITING;
        idle_buf_info->data_num = 0;
        return write_buf_info;
    }

    return NULL;
}

static void ff_rtcp_update_write_buf(URLContext *h,
	uint8_t *buf, int len)
{
    RTPTcpContext *s = h->priv_data;
    RTPRtspBufContext *bufctx = &(s->rtp_rtsp_buf_st);
    RTPBufInfo *buf_info = NULL;
    RTPBufInfo *other_buf_info = NULL;

    if (bufctx->init_ok == 0)
        return;

    pthread_mutex_lock(&bufctx->mutex);

    if (bufctx->rtcp_buf1_info.status == RTP_BUF_IDLE) {
        buf_info = &bufctx->rtcp_buf1_info;
        other_buf_info = &bufctx->rtcp_buf2_info;
    } else if(bufctx->rtcp_buf2_info.status == RTP_BUF_IDLE) {
        buf_info = &bufctx->rtcp_buf2_info;
        other_buf_info = &bufctx->rtcp_buf1_info;
    } else if(bufctx->rtcp_buf1_info.status == RTP_BUF_READY) {
        buf_info = &bufctx->rtcp_buf1_info;
        other_buf_info = &bufctx->rtcp_buf2_info;
    } else if(bufctx->rtcp_buf2_info.status == RTP_BUF_READY) {
        buf_info = &bufctx->rtcp_buf2_info;
        other_buf_info = &bufctx->rtcp_buf1_info;
    }

    if (buf_info) {
        buf_info->data_num = 1;
        buf_info->data_len[0] = len;
        memcpy(buf_info->buf[0], buf, len);

        buf_info->status = RTP_BUF_READY;

        if(other_buf_info->status == RTP_BUF_READY)
            other_buf_info->status = RTP_BUF_IDLE;
    }

    pthread_mutex_unlock(&bufctx->mutex);

    return;
}

void *ff_rtp_get_send_buf(void *s1)
{
    URLContext *s2 = s1;

    RTPTcpContext *s = NULL;
    RTPRtspBufContext *bufctx = NULL;
    RTPBufInfo *buf_info = NULL;

    if (NULL == s1)
        return NULL;

    s = s2->priv_data;
    bufctx = &(s->rtp_rtsp_buf_st);

    if (bufctx->init_ok == 0)
        return NULL;

    pthread_mutex_lock(&bufctx->mutex);

    if (bufctx->rtp_buf1_info.status == RTP_BUF_READY)
        buf_info = &bufctx->rtp_buf1_info;
    else if (bufctx->rtp_buf2_info.status == RTP_BUF_READY)
        buf_info = &bufctx->rtp_buf2_info;
    else if (bufctx->rtp_buf3_info.status == RTP_BUF_READY)
        buf_info = &bufctx->rtp_buf3_info;
    else if (bufctx->rtp_buf1_info.status == RTP_BUF_READY_SECOND)
        buf_info = &bufctx->rtp_buf1_info;
    else if (bufctx->rtp_buf2_info.status == RTP_BUF_READY_SECOND)
        buf_info = &bufctx->rtp_buf2_info;
    else if (bufctx->rtp_buf3_info.status == RTP_BUF_READY_SECOND)
        buf_info = &bufctx->rtp_buf3_info;

    if (buf_info)
        buf_info->status = RTP_BUF_READING;
    else {
	buf_info = ff_rtp_swap_write_buf(s2);
	if (buf_info)
            buf_info->status = RTP_BUF_READING;
    }

    pthread_mutex_unlock(&bufctx->mutex);

    return buf_info;
}

void ff_rtp_put_send_buf(void *s1, void *info)
{
    RTPTcpContext *s = NULL;
    RTPRtspBufContext *bufctx = NULL;
    URLContext *s2 = s1;
    RTPBufInfo *buf_info = info;

    if (NULL == s1 || NULL == buf_info)
        return;

    s = s2->priv_data;
    bufctx = &(s->rtp_rtsp_buf_st);

    if (bufctx->init_ok == 0)
        return;

    pthread_mutex_lock(&bufctx->mutex);

    buf_info->status = RTP_BUF_IDLE;

    pthread_mutex_unlock(&bufctx->mutex);

    return;
}

void *ff_rtcp_get_send_buf(void *s1)
{
    RTPTcpContext *s = NULL;
    RTPRtspBufContext *bufctx = NULL;
    RTPBufInfo *buf_info = NULL;
    URLContext *s2 = s1;

    if (NULL == s1)
        return NULL;

    s = s2->priv_data;
    bufctx = &(s->rtp_rtsp_buf_st);

    if (0 == bufctx->init_ok)
        return NULL;

    pthread_mutex_lock(&bufctx->mutex);

    if (bufctx->rtcp_buf1_info.status == RTP_BUF_READY)
        buf_info = &bufctx->rtcp_buf1_info;
    else if (bufctx->rtcp_buf2_info.status == RTP_BUF_READY)
        buf_info = &bufctx->rtcp_buf2_info;

    if (buf_info)
        buf_info->status = RTP_BUF_READING;
    pthread_mutex_unlock(&bufctx->mutex);

    return buf_info;
}


/**
 * url syntax: rtp://host:port[?option=val...]
 * option: 'ttl=n'            : set the ttl value (for multicast only)
 *         'rtcpport=n'       : set the remote rtcp port to n
 *         'localrtpport=n'   : set the local rtp port to n
 *         'localrtcpport=n'  : set the local rtcp port to n
 *         'pkt_size=n'       : set max packet size
 *         'connect=0/1'      : do a connect() on the UDP socket
 *         'sources=ip[,ip]'  : list allowed source IP addresses
 *         'block=ip[,ip]'    : list disallowed source IP addresses
 *         'write_to_source=0/1' : send packets to the source address of the latest received packet
 * deprecated option:
 *         'localport=n'      : set the local port to n
 *
 * if rtcpport isn't set the rtcp port will be the rtp port + 1
 * if local rtp port isn't set any available port will be used for the local
 * rtp and rtcp ports
 * if the local rtcp port is not set it will be the local rtp port + 1
 */
#define RTP_TCP_HEADER_SIZE 4

#define RTP_MAX_PACKET_SIZE (1514-14-20-20)
#define RTP_MAX_RTPENC_SIZE (RTP_MAX_PACKET_SIZE-RTP_TCP_HEADER_SIZE)

static int rtp_tcp_open(URLContext *h, const char *uri, int flags)
{
    RTPTcpContext *s = h->priv_data;
    int rtp_channel, rtcp_channel;
    char buf[1024];
    char path[1024];
    const char *p;

    /* extract parameters */
    rtp_channel = -1;
    rtcp_channel = -1;

    p = strchr(uri, '?');
    if (p) {
        if (av_find_info_tag(buf, sizeof(buf), "rtpchannel", p))
            rtp_channel = strtol(buf, NULL, 10);

        if (av_find_info_tag(buf, sizeof(buf), "rtcpchannel", p))
            rtcp_channel = strtol(buf, NULL, 10);
    }

    s->rtp_channel = rtp_channel;
    s->rtcp_channel = rtcp_channel;

    if (ff_rtp_buf_context_init(s, RTP_MAX_PACKET_SIZE) != 0)
        return AVERROR(ENOMEM);

    h->max_packet_size = RTP_MAX_PACKET_SIZE;/*1514-14(LINK)-20(IP)-20(TCP)*/
    h->is_streamed = 1;
    return 0;
}

static int rtp_tcp_write(URLContext *h, const uint8_t *buf, int size)
{
    RTPTcpContext *s = h->priv_data;
    URLContext *hd;
    RTPTcpFixHeader *rtp_tcp_header;

    if (size < 2)
        return AVERROR(EINVAL);

    if (RTP_PT_IS_RTCP(buf[5])) {
        /* RTCP payload type */
        rtp_tcp_header = buf;

        rtp_tcp_header->magic = '\$';
        rtp_tcp_header->channel = s->rtcp_channel;
        rtp_tcp_header->len = htons(size - 4);

        ff_rtcp_update_write_buf(h,
                buf,
                size);
    } else {
         /* RTP payload type */
        rtp_tcp_header = buf;
        rtp_tcp_header->magic = '\$';
        rtp_tcp_header->channel = s->rtp_channel;
        rtp_tcp_header->len = htons(size - 4);

        ff_rtp_update_write_buf(h,
                buf,
                size);
    }

    return 0;
}

static int rtp_tcp_close(URLContext *h)
{
    RTPTcpContext *s = h->priv_data;

    ff_rtp_buf_context_uninit(s);

    return 0;
}

URLProtocol ff_rtptcp_protocol = {
    .name                      = "rtptcp",
    .url_open                  = rtp_tcp_open,
    .url_read                  = NULL,
    .url_write                 = rtp_tcp_write,
    .url_close                 = rtp_tcp_close,
    .url_get_file_handle       = NULL,
    .url_get_multi_file_handle = NULL,
    .priv_data_size            = sizeof(RTPTcpContext),
    .flags                     = URL_PROTOCOL_FLAG_NETWORK,
};

