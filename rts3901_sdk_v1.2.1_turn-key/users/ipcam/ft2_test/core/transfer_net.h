/*
 * Realtek Semiconductor Corp.
 *
 * inc/ft2tr.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _CORE_TRANSFER_H
#define _CORE_TRANSFER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include "list.h"
#include "ft2errno.h"
#include "ft2log.h"
#include "tr_buffer.h"


#ifndef FT2_SOCKET_PORT
#define FT2_SOCKET_PORT		6789
#endif

#ifdef FT2_CLIENT_NUM
#undef FT2_CLIENT_NUM
#endif
#define FT2_CLIENT_NUM		1

typedef int (*RECV_FUNC)(void *receiver, struct tr_data *data);

struct ft2_transfer {
	int srvfd;
	int connfd;
	pthread_t th_listren;
	pthread_t th_recv;
	pthread_t th_send;

	unsigned char srv_exit;
	volatile unsigned char conn_status;

	pthread_mutex_t fast_mutex;
	pthread_mutex_t slow_mutex;
	struct list_head fast_list;
	struct list_head slow_list;

	RECV_FUNC recv_func;
	void *receiver;

	unsigned long heartbeat;
};
typedef struct ft2_transfer *FT2TrInst;

struct tr_send_slow {
	struct list_head list;
	struct tr_data *data;
};

struct tr_send_fast {
	struct list_head list;
	char resp;
};

int send_tr_data(FT2TrInst inst, struct tr_data *buf);
int send_ack(FT2TrInst inst);
int send_nak(FT2TrInst inst);
int ft2_new_transfer_inst(FT2TrInst *inst, void *receiver, RECV_FUNC cb);
int ft2_delete_transfer_inst(FT2TrInst inst);

#endif
