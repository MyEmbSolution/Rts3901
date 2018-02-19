/*
 * Realtek Semiconductor Corp.
 *
 * core/tr_buffer.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include "tr_buffer.h"


static struct tr_buffer_manager *manager = NULL;

int clean_up_tr_buffers()
{
	if (!manager)
		return FT2_OK;
	pthread_mutex_destroy(&manager->buf_mutex);
	free(manager);
	manager = NULL;
	return FT2_OK;
}

int init_tr_buffers()
{
	int i = 0;

	if (manager)
		return FT2_OK;

	manager = (struct tr_buffer_manager*)calloc(1, sizeof(*manager));
	if (!manager) {
		FT2_LOG_ERR("init tr buffers fail, no memory\n");
		return -FT2_NO_MEMORY;
	}
	pthread_mutex_init(&manager->buf_mutex, NULL);

	pthread_mutex_lock(&manager->buf_mutex);
	INIT_LIST_HEAD(&manager->available_list);
	for (i = 0; i < ARRAY_SIZE(manager->buffers); i++) {
		struct tr_buffer *buffer = &manager->buffers[i];
		list_add_tail(&buffer->list, &manager->available_list);
	}
	pthread_mutex_unlock(&manager->buf_mutex);

	return FT2_OK;
}

static struct tr_buffer *get_buffer(struct tr_buffer *buf)
{
	if (!manager || !buf)
		return buf;

	pthread_mutex_lock(&manager->buf_mutex);
	buf->user_count++;
	pthread_mutex_unlock(&manager->buf_mutex);
	return buf;
}

struct tr_data *alloc_tr_buffer()
{
	struct tr_buffer *buffer= NULL;
	if (init_tr_buffers() < FT2_OK)
		return (struct tr_data*)buffer;
	pthread_mutex_lock(&manager->buf_mutex);
	if (!list_empty(&manager->available_list)) {
		buffer = list_first_entry(&manager->available_list, struct tr_buffer, list);
		list_del(&buffer->list);
	}
	pthread_mutex_unlock(&manager->buf_mutex);

	return (struct tr_data*)get_buffer(buffer);
}

struct tr_data *get_tr_buffer(struct tr_data *buf)
{
	return (struct tr_data *)get_buffer((struct tr_buffer*)buf);
}

void put_tr_buffer(struct tr_data *buf)
{
	struct tr_buffer *buffer = (struct tr_buffer *)buf;
	if (!manager || !buf)
		return;
	pthread_mutex_lock(&manager->buf_mutex);
	if (buffer->user_count > 0) {
		buffer->user_count--;
		if (buffer->user_count == 0) {
			memset(&buffer->data, 0, sizeof(struct tr_data));
			list_add_tail(&buffer->list, &manager->available_list);
		}
	}
	pthread_mutex_unlock(&manager->buf_mutex);
}
