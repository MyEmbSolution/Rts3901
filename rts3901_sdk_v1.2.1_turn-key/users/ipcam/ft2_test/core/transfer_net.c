/*
 * Realtek Semiconductor Corp.
 *
 * core/transfer.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */

#include "transfer_net.h"


int send_tr_data(FT2TrInst inst, struct tr_data *buf)
{
	struct tr_send_slow *slow = NULL;
	if (!inst || !buf)
		return -FT2_NULL_POINT;

	slow = (struct tr_send_slow *)calloc(1, sizeof(*slow));
	if (!slow)
		return -FT2_NO_MEMORY;

	slow->data = get_tr_buffer(buf);

	pthread_mutex_lock(&inst->slow_mutex);
	list_add_tail(&slow->list, &inst->slow_list);
	pthread_mutex_unlock(&inst->slow_mutex);

	return FT2_OK;
}

static int send_char(FT2TrInst inst, char ch)
{
	struct tr_send_fast *fast = NULL;

	if (!inst)
		return FT2_NULL_POINT;

	fast = (struct tr_send_fast*)calloc(1, sizeof(*fast));
	if (!fast)
		return FT2_NO_MEMORY;

	fast->resp = ch;

	pthread_mutex_lock(&inst->fast_mutex);
	list_add_tail(&fast->list, &inst->fast_list);
	pthread_mutex_unlock(&inst->fast_mutex);

	return FT2_OK;
}

int send_ack(FT2TrInst inst)
{
	return send_char(inst, '+');
}

int send_nak(FT2TrInst inst)
{
	return send_char(inst, '-');
}

void clear_send_buf(struct ft2_transfer *transfer)
{
	struct tr_send_fast *fast, *tf;
	struct tr_send_slow *slow, *ts;

	if (!transfer)
		return;

	pthread_mutex_lock(&transfer->fast_mutex);
	list_for_each_entry_safe(fast, tf, &transfer->fast_list, list) {
		list_del(&fast->list);
		free(fast);
	}
	INIT_LIST_HEAD(&transfer->fast_list);
	pthread_mutex_unlock(&transfer->fast_mutex);

	pthread_mutex_lock(&transfer->slow_mutex);
	list_for_each_entry_safe(slow, ts, &transfer->slow_list, list) {
		list_del(&slow->list);
		if (slow->data)
			put_tr_buffer(slow->data);
		free(slow);
	}
	INIT_LIST_HEAD(&transfer->slow_list);
	pthread_mutex_unlock(&transfer->slow_mutex);
}

static void close_connection(struct ft2_transfer *transfer)
{
	int fd;

	if (!transfer)
		return;

	fd = transfer->connfd;
	transfer->connfd = -1;
	if (fd > 0) {
		FT2_LOG_WARNING("close connection %d\n", fd);
		close(fd);
	}
}

static int check_readable(int fd)
{
	struct timeval tv;
	fd_set myset;
	FD_ZERO(&myset);
	FD_SET(fd, &myset);
	tv.tv_sec = 0;
	tv.tv_usec = 10000;

	if (select(fd + 1, &myset, NULL, NULL, &tv) > 0)
		return FT2_OK;
	return -FT2_ERROR;
}

static int check_writeable(int fd)
{
	struct timeval tv;
	fd_set myset;
	FD_ZERO(&myset);
	FD_SET(fd, &myset);
	tv.tv_sec = 0;
	tv.tv_usec = 10000;

	if (select(fd + 1, NULL, &myset, NULL, &tv) > 0)
		return FT2_OK;
	return -FT2_ERROR;
}

static int do_write_fast(struct ft2_transfer *transfer)
{
	struct tr_send_fast *fast = NULL;
	int ret = FT2_OK;

	if (!transfer)
		return -FT2_NULL_POINT;

	pthread_mutex_lock(&transfer->fast_mutex);
	while (!list_empty(&transfer->fast_list)) {
		fast = list_first_entry(&transfer->fast_list,
				struct tr_send_fast, list);
		list_del(&fast->list);
		ret = write(transfer->connfd, &fast->resp, 1);
		free(fast);
		fast = NULL;
		if (ret < 0) {
			FT2_LOG_ERR("send fast data fail, %d\n", errno);
			ret = -FT2_WRITE_FAIL;
			break;
		}
		ret = FT2_OK;
	}
	pthread_mutex_unlock(&transfer->fast_mutex);

	return ret;
}

static int do_write_slow(struct ft2_transfer *transfer)
{
	struct tr_send_slow *slow = NULL;

	if (!transfer)
		return -FT2_NULL_POINT;

	pthread_mutex_lock(&transfer->slow_mutex);
	if (!list_empty(&transfer->slow_list)) {
		slow = list_first_entry(&transfer->slow_list,
				struct tr_send_slow, list);
		list_del(&slow->list);
	}
	pthread_mutex_unlock(&transfer->slow_mutex);

	if (slow) {
		int ret = write(transfer->connfd,
				slow->data->data, slow->data->length);
		//FT2_LOG_INFO("write %d, %s\n", slow->data->length, slow->data->data);
		put_tr_buffer(slow->data);
		free(slow);
		slow = NULL;
		if (ret < 0) {
			FT2_LOG_ERR("send slow data fail, %d\n", errno);
			return -FT2_WRITE_FAIL;
		}
	}

	return FT2_OK;
}

static int do_write_heartbeat(struct ft2_transfer *transfer)
{
	unsigned long occur = time(NULL);
	if (!transfer)
		return -FT2_NULL_POINT;

	if (occur - transfer->heartbeat > 0) {
		char hb = 0xff;
		int ret = write(transfer->connfd, &hb, 1);
		if (ret < 0) {
			FT2_LOG_ERR("send heartbeat fail, %d\n", errno);
			return -FT2_WRITE_FAIL;
		}
		transfer->heartbeat = occur;
	}
	return FT2_OK;
}

static void *send_func(void *data)
{
	struct ft2_transfer *transfer = (struct ft2_transfer *)data;
	int ret = FT2_OK;

	if (!transfer)
		return NULL;

	clear_send_buf(transfer);
	transfer->heartbeat = time(NULL);

	while(transfer->connfd > 0) {
		if (FT2_OK != check_writeable(transfer->connfd))
			continue;

		usleep(10000);
		ret = do_write_heartbeat(transfer);
		if (ret + FT2_WRITE_FAIL == 0) {
			close_connection(transfer);
			continue;
		}

		ret = do_write_fast(transfer);
		if (ret + FT2_WRITE_FAIL == 0) {
			close_connection(transfer);
			continue;
		}

		ret = do_write_slow(transfer);
		if (ret + FT2_WRITE_FAIL == 0) {
			close_connection(transfer);
			continue;
		}
	}

	clear_send_buf(transfer);

	transfer->conn_status--;
	return NULL;
}

static void *receive_func(void *data)
{
	struct ft2_transfer *transfer = (struct ft2_transfer *)data;
	struct tr_data *buf = NULL;
	int ret = 0;

	if (!transfer)
		return NULL;

	while(transfer->connfd > 0) {
		buf = NULL;
		if (FT2_OK != check_readable(transfer->connfd))
			continue;

		buf = alloc_tr_buffer();
		if (!buf) {
			FT2_LOG_ERR("buffer overflow, receive fail\n");
			continue;
		}
		ret = read(transfer->connfd, buf->data, sizeof(buf->data));
		if (ret > 0) {
			buf->length = ret;
			if (transfer->recv_func)
				transfer->recv_func(transfer->receiver, buf);
		} else {
			close_connection(transfer);
		}
		put_tr_buffer(buf);
	}

	transfer->conn_status--;
	return NULL;
}

static void *listen_func(void *data)
{
	struct ft2_transfer *transfer = (struct ft2_transfer *)data;
	int ret = 0;

	if (!transfer)
		return NULL;

	while(!transfer->srv_exit) {
		usleep(1000);
		if (0 == transfer->conn_status) {
			struct sockaddr_in client_addr;
			socklen_t length = sizeof(client_addr);
			char value[] = {0xff};
			int ret = 0;
			transfer->connfd = accept(transfer->srvfd,
					(struct sockaddr*)&client_addr,
					&length);
			if (transfer->connfd <= 0)
				continue;
			FT2_LOG_INFO("connection[%d] : %s:%d\n",
					transfer->connfd,
					inet_ntoa(client_addr.sin_addr),
					client_addr.sin_port);
			ret = write(transfer->connfd, value, sizeof(value));
			ret = write(transfer->connfd, value, sizeof(value));
			if (ret > 0) {
				ret = pthread_create(&transfer->th_recv, NULL,
						receive_func, (void*)transfer);
				if (ret) {
					FT2_LOG_ERR("create receive thread fail\n");
					close_connection(transfer);
					continue;
				}
				transfer->conn_status++;
				ret = pthread_create(&transfer->th_send, NULL,
						send_func, (void*)transfer);
				if (ret) {
					FT2_LOG_ERR("create send thread fail\n");
					close_connection(transfer);
					pthread_join(transfer->th_recv, NULL);
					continue;
				}
				transfer->conn_status++;
			} else {
				close_connection(transfer);
			}
		}
	}
}

static int create_socket_srv(struct ft2_transfer *transfer)
{
	struct sockaddr_in serv_addr;
	long attr_arg;
	int ret = 0;

	if (!transfer)
		return FT2_NULL_POINT;

	transfer->srvfd = socket(AF_INET, SOCK_STREAM, 0);
	attr_arg = fcntl(transfer->srvfd, F_GETFL, NULL);
	attr_arg |= O_NONBLOCK;
	fcntl(transfer->srvfd, F_SETFL, attr_arg);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(FT2_SOCKET_PORT);

	bind(transfer->srvfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	listen(transfer->srvfd, FT2_CLIENT_NUM);
	FT2_LOG_INFO("listening on port : %d\n", FT2_SOCKET_PORT);

	transfer->srv_exit = 0;
	transfer->connfd = -1;
	ret = pthread_create(&transfer->th_listren, NULL,
			listen_func, (void*)transfer);
	if (ret) {
		FT2_LOG_ERR("create socket server thread fail\n");
		transfer->srv_exit = 1;
		return -FT2_THREAD_FAIL;
	}

	return FT2_OK;
}

int ft2_new_transfer_inst(FT2TrInst *inst, void *receiver, RECV_FUNC cb)
{
	int ret = 0;
	struct ft2_transfer *transfer = NULL;
	if (!inst) {
		FT2_LOG_ERR("NULL Point, invalid argument\n");
		return -FT2_NULL_POINT;
	}

	transfer = (struct ft2_transfer*)calloc(1, sizeof(*transfer));
	if (!transfer) {
		FT2_LOG_ERR("malloc transfer fail\n");
		return -FT2_NO_MEMORY;
	}

	pthread_mutex_init(&transfer->fast_mutex, NULL);
	pthread_mutex_init(&transfer->slow_mutex, NULL);

	INIT_LIST_HEAD(&transfer->fast_list);
	INIT_LIST_HEAD(&transfer->slow_list);

	transfer->receiver = receiver;
	transfer->recv_func = cb;

	ret = create_socket_srv(transfer);
	if (ret) {
		pthread_mutex_destroy(&transfer->fast_mutex);
		pthread_mutex_destroy(&transfer->slow_mutex);
		free(transfer);
		FT2_LOG_ERR("create socket server fail, ret = %d\n", ret);
		return ret;
	}

	*inst = transfer;
	return FT2_OK;
}

int ft2_delete_transfer_inst(FT2TrInst inst)
{
	if (!inst)
		return FT2_OK;

	if (inst->connfd > 0) {
		close_connection(inst);
		pthread_join(inst->th_recv, NULL);
		pthread_join(inst->th_send, NULL);
	}
	if (!inst->srv_exit) {
		inst->srv_exit = 1;
		pthread_join(inst->th_listren, NULL);
	}
	clear_send_buf(inst);
	pthread_mutex_destroy(&inst->fast_mutex);
	pthread_mutex_destroy(&inst->slow_mutex);
	free(inst);
	return FT2_OK;
}
