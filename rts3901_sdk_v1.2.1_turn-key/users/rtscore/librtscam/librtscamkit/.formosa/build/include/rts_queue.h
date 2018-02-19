/*
 * Realtek Semiconductor Corp.
 *
 * rts_queue.h
 *
 * Copyright (C) 2016      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _RTS_QUEUE_H
#define _RTS_QUEUE_H

typedef void *Item;

typedef struct rts_queue *Queue;

typedef void (*cleanup_item_func)(Item item);

Queue rts_queue_init();
void rts_queue_destroy(Queue q);
int rts_queue_empty(Queue q);

int rts_queue_push_back(Queue q, Item item);
Item rts_queue_pop(Queue q);
void rts_queue_clear(Queue q, cleanup_item_func cleanup);

void rts_queue_del(Queue q, Item item);
int rts_queue_insert(Queue q, Item newitem, Item item, int before);
int rts_queue_insert_at(Queue q, Item newitem, int index);
int rts_queue_swap(Queue q, Item a, Item b);
int rts_queue_check(Queue q, Item item);

Item rts_queue_enumerate(Queue q, int index);
int rts_queue_count(Queue q);


int rts_queue_lock(Queue q);
int rts_queue_unlock(Queue q);

#define list_queue(q, func, arg...) \
	do {\
		int i;\
		rts_queue_lock(q);\
		for (i = 0; i < rts_queue_count(q); i++) {\
			Item item = rts_queue_enumerate(q, i);\
			func(item, ##arg);\
		}\
		rts_queue_unlock(q);\
	} while (0)

#define list_queue_reverse(q, func, arg...) \
	do {\
		int i;\
		rts_queue_lock(q);\
		for (i = rts_queue_count(q) - 1; i >= 0; i--) {\
			Item item = rts_queue_enumerate(q, i);\
			func(item, ##arg);\
		}\
		rts_queue_unlock(q);\
	} while (0)

#define find_queue_item(q, anchor, cmpfunc, found) \
	do {\
		int i;\
		rts_queue_lock(q);\
		for (i = 0; i < rts_queue_count(q); i++) {\
			Item item = rts_queue_enumerate(q, i);\
			if (0 == cmpfunc(item, anchor)) {\
				found = item;\
				break;\
			}\
		}\
		rts_queue_unlock(q);\
	} while (0)

#endif

