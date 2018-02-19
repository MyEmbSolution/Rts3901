/*
 * Realtek Semiconductor Corp.
 *
 * inc/ft2buffer.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <string.h>
#include "list.h"
#include "ft2errno.h"
#include "ft2log.h"


#ifndef _CORE_TR_BUFFER_H
#define _CORE_TR_BUFFER_H

#ifndef TR_MAX_SIZE
#define TR_MAX_SIZE		256
#endif

#ifndef TR_BUF_NUM
#define TR_BUF_NUM		32
#endif

struct tr_data {
	uint8_t data[TR_MAX_SIZE + 1];
	unsigned int length;
};

struct tr_buffer {
	struct tr_data data;
	unsigned int user_count;
	struct list_head list;
};

struct tr_buffer_manager {
	struct tr_buffer buffers[TR_BUF_NUM];
	pthread_mutex_t buf_mutex;
	struct list_head available_list;
};

#define ARRAY_SIZE(array)	(sizeof(array)/sizeof(array[0]))

struct tr_data *alloc_tr_buffer();
struct tr_data *get_tr_buffer(struct tr_data *buf);
void put_tr_buffer(struct tr_data *buf);
int clean_up_tr_buffers();

#endif
