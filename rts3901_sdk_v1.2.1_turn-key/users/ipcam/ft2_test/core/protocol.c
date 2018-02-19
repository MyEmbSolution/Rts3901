/*
 * Realtek Semiconductor Corp.
 *
 * core/protocol.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include "protocol.h"


struct protocol_command *alloc_pt_command(char *name, unsigned int id)
{
	struct protocol_command *pcmd = NULL;

	pcmd = (struct protocol_command*)calloc(1, sizeof(struct pt_command));
	if (!pcmd)
		return NULL;

	if (name)
		snprintf(pcmd->name, sizeof(pcmd->name), "%s", name);
	if (id)
		pcmd->id = id;

	return (struct protocol_command*)pcmd;
}

int delete_pt_command(struct protocol_command *pcmd)
{
	struct protocol_param *param= NULL, *next = NULL;

	if (!pcmd)
		return FT2_OK;

	param = pcmd->parameter;
	while (param) {
		next = param->next;
		free(param);
		param = next;
	}

	param = pcmd->result;
	while (param) {
		next = param->next;
		free(param);
		param = next;
	}

	free(pcmd);

	return FT2_OK;
}

int delete_pt_command_result(struct protocol_command *pcmd)
{
	struct protocol_param *result= NULL, *next = NULL;

	if (!pcmd)
		return FT2_OK;

	result = pcmd->result;
	while (result) {
		next = result->next;
		free(result);
		result = next;
	}

	return FT2_OK;
}

int append_pt_command_param(struct protocol_command* pcmd, char *param, int len)
{
	struct protocol_param *parameter = NULL, *next;

	if (!pcmd || !param)
		return -FT2_NULL_POINT;

	parameter = (struct protocol_param *)calloc(1, sizeof(*parameter));
	if (!parameter)
		return -FT2_NO_MEMORY;

	snprintf(parameter->content, sizeof(parameter->content),
			"%.*s", len, param);

	if (!pcmd->parameter)
		pcmd->parameter = parameter;
	else {
		next = pcmd->parameter;
		while (next->next)
			next = next->next;
		next->next = parameter;
	}

	return FT2_OK;
}

int append_pt_command_result(struct protocol_command* pcmd, char *param, int len)
{
	struct protocol_param *parameter = NULL, *next;

	if (!pcmd || !param)
		return -FT2_NULL_POINT;

	parameter = (struct protocol_param *)calloc(1, sizeof(*parameter));
	if (!parameter)
		return -FT2_NO_MEMORY;

	snprintf(parameter->content, sizeof(parameter->content),
			"%.*s", len, param);

	if (!pcmd->result)
		pcmd->result = parameter;
	else {
		next = pcmd->result;
		while (next->next)
			next = next->next;
		next->next = parameter;
	}

	return FT2_OK;
}

static int send_data(struct ft2_protocol *protocol, char *data, int length,
		struct tr_data **buffer, int finish)
{
	struct tr_data *resp = NULL;
	int left = length;
	char *ptr = data;

	if (buffer && *buffer)
		resp = *buffer;
	else
		resp = alloc_tr_buffer();

	if (!resp)
		return -FT2_BUFFER_EMPTY;

	while (left > 0) {
		int len = left;
		if (len > TR_MAX_SIZE - resp->length)
			len = TR_MAX_SIZE - resp->length;

		left -= len;
		snprintf(resp->data + resp->length,
				sizeof(resp->data) - resp->length,
				"%.*s", len, ptr);
		resp->length += len;
		ptr += len;
		if (left > 0) {
			send_tr_data(protocol->transfer, resp);
			put_tr_buffer(resp);
			resp = alloc_tr_buffer();
			if (!resp)
				return -FT2_BUFFER_EMPTY;
		}
	}

	if (!buffer) {
		send_tr_data(protocol->transfer, resp);
		put_tr_buffer(resp);
		return FT2_OK;
	}

	if (finish) {
		send_tr_data(protocol->transfer, resp);
		put_tr_buffer(resp);
		*buffer = NULL;
	} else
		*buffer = resp;

	return FT2_OK;
}

static unsigned int calc_sum(struct protocol_command *pcmd)
{
	unsigned int sum = 0;
	int i = 0;
	struct protocol_param *prst = NULL;

	if (!pcmd)
		return 0;

	if (pcmd->type != FT2_COMMAND_CODE_REQ &&
			pcmd->type != FT2_COMMAND_CODE_RSP)
		return 0;

	sum += pcmd->type;
	for (i = 0; i < strlen(pcmd->name); i++) {
		sum += pcmd->name[i];
	}

	if (pcmd->type == FT2_COMMAND_CODE_REQ)
		prst = pcmd->parameter;
	else
		prst = pcmd->result;

	while (prst) {
		sum += FT2_COMMAND_CODE_PARAM;
		for (i = 0; i < strlen(prst->content); i++)
			sum += prst->content[i];
		prst = prst->next;
	}

	sum %= 0x100;
	return sum;
}

int send_commond_result(FT2ProtInst inst, struct protocol_command *pcmd)
{
	struct ft2_protocol *protocol = inst;
	struct tr_data *resp = NULL;
	char *ptr = NULL;
	char ch;
	int ret = FT2_OK;
	struct protocol_param *prst = NULL;
	char chsum[3];
	unsigned int sum = 0;

	if (!inst || !pcmd)
		return -FT2_NULL_POINT;

	pcmd->type = FT2_COMMAND_CODE_RSP;

	pthread_mutex_lock(&protocol->mutex_resp);
	/**/
	ch = FT2_COMMAND_CODE_START;
	ret = send_data(protocol, &ch, 1, &resp, 0);
	if (ret) {
		FT2_LOG_ERR("send response fail at %d, ret = %d\n",
				__LINE__, ret);
		goto exit;
	}

	ch = pcmd->type;
	ret = send_data(protocol, &ch, 1, &resp, 0);
	if (ret) {
		FT2_LOG_ERR("send response fail at %d, ret = %d\n",
				__LINE__, ret);
		goto exit;
	}

	ret = send_data(protocol, pcmd->name, strlen(pcmd->name), &resp, 0);
	if (ret) {
		FT2_LOG_ERR("send response fail at %d, ret = %d\n",
				__LINE__, ret);
		goto exit;
	}

	prst = pcmd->result;

	while (prst) {
		ch = FT2_COMMAND_CODE_PARAM;
		ret = send_data(protocol, &ch, 1, &resp, 0);
		if (ret) {
			FT2_LOG_ERR("send response fail at %d, ret = %d\n",
				__LINE__, ret);
			goto exit;
		}

		ret = send_data(protocol, prst->content,
				strlen(prst->content), &resp, 0);
		if (ret) {
			FT2_LOG_ERR("send response fail at %d, ret = %d\n",
				__LINE__, ret);
			goto exit;
		}
		prst = prst->next;
	}

	sum = calc_sum(pcmd);
	chsum[0] = FT2_COMMAND_CODE_END;
	snprintf(chsum + 1, 3, "%x%x", sum/16, sum%16);
	ret = send_data(protocol, chsum, 3, &resp, 1);
	if (ret) {
		FT2_LOG_ERR("send response fail at %d, ret = %d\n",
				__LINE__, ret);
		goto exit;
	}

	ret = FT2_OK;
exit:
	pthread_mutex_unlock(&protocol->mutex_resp);

	return ret;
}

struct protocol_command *pop_commond(FT2ProtInst inst)
{
	struct pt_command *pcmd = NULL;
	struct ft2_protocol *protocol = inst;

	if (!inst)
		return NULL;

	pthread_mutex_lock(&protocol->mutex_parse);
	if (!list_empty(&protocol->parse_list)) {
		pcmd = list_first_entry(&protocol->parse_list,
				struct pt_command, list);
		list_del(&pcmd->list);
	}
	pthread_mutex_unlock(&protocol->mutex_parse);

	return (struct protocol_command *)pcmd;
}

int process_ack(struct ft2_protocol *protocol)
{
	FT2_LOG_OPT("+\n");
	return FT2_OK;
}

int process_nak(struct ft2_protocol *protocol)
{
	FT2_LOG_OPT("-\n");
	return FT2_OK;
}

static int checksum(char *cmdstr, int length)
{
	unsigned int sum = 0;
	unsigned int sum1 = 0;
	int i = 0;

	if (length < 5)
		return -FT2_PROTOCOL_ERROR;

	for (i = 1; i < length - 3; i++) {
		sum += (unsigned int) cmdstr[i];
	}
	sum %= 0x100;
	for ( i = length -2; i < length; i++) {
		char ch = cmdstr[i];
		if (ch >= '0' && ch <= '9')
			sum1 = sum1 * 16 + (ch - '0');
		else if (ch >= 'a' && ch <= 'f')
			sum1 = sum1 * 16 + (ch - 'a' + 10);
		else if (ch >= 'A' && ch <= 'F')
			sum1 = sum1 * 16 + (ch - 'A' + 10);
		else
			return -FT2_PROTOCOL_ERROR;
	}
	sum1 %= 0x100;

	//FT2_LOG_INFO("%x %x\n", sum, sum1);

	if (sum != sum1) {
		FT2_LOG_ERR("checksum fail, want %02x but get %02x\n",
				sum, sum1);
		return -FT2_PROTOCOL_ERROR;
	}

	return FT2_OK;
}

int process_cmd(struct ft2_protocol *protocol, char *cmdstr, int length)
{
	char *ptr = cmdstr;
	char *rst;
	int i = 0;
	int len;
	int ret = FT2_OK;
	struct protocol_command *pcmd = NULL;

	if (!cmdstr)
		return -FT2_NULL_POINT;
	FT2_LOG_OPT("%.*s\n", length, cmdstr);

	if (*(ptr + 1) != FT2_COMMAND_CODE_REQ) {
		FT2_LOG_ERR("wrong cmd code [%c], wanted [%c]\n",
				*(ptr+1),
				FT2_COMMAND_CODE_REQ);
		send_nak(protocol->transfer);
		return -FT2_PROTOCOL_ERROR;
	}

	ret = checksum(cmdstr, length);
	if (ret) {
		send_nak(protocol->transfer);
		return ret;
	}

	rst = strchr(cmdstr, FT2_COMMAND_CODE_PARAM);
	if (rst)
		len = rst - cmdstr - 2;
	else
		len = length - 5;

	pcmd = alloc_pt_command(NULL, 0);
	if (!pcmd) {
		FT2_LOG_ERR("alloc memory for test command fail\n");
		send_nak(protocol->transfer);
		return -FT2_NO_MEMORY;
	}

	snprintf(pcmd->name, sizeof(pcmd->name), "%.*s", len, cmdstr + 2);
	pcmd->type = FT2_COMMAND_CODE_REQ;

	ptr = rst;
	while (ptr) {
		rst = strchr(ptr + 1, FT2_COMMAND_CODE_PARAM);
		if (rst)
			len = rst - ptr - 1;
		else
			len = (cmdstr + length - 4) - ptr;
		append_pt_command_param(pcmd, ptr + 1, len);

		ptr = rst;
	}

	ret = FT2_OK;
	if (protocol->check_command)
		ret = protocol->check_command(protocol->checker, pcmd);
	if (ret) {
		send_nak(protocol->transfer);
		delete_pt_command(pcmd);
		return ret;
	}

	pthread_mutex_lock(&protocol->mutex_parse);
	list_add_tail(&((struct pt_command*)pcmd)->list, &protocol->parse_list);
	pthread_mutex_unlock(&protocol->mutex_parse);

	send_ack(protocol->transfer);
	return FT2_OK;
}

static int store_tmp_content(struct ft2_protocol *protocol, char *data, int length)
{
	unsigned char flag = 0;
	if (protocol->tmp_length == 0) {
		protocol->tmp_length = 128;
		flag = 1;
	}
	while (protocol->tmp_length < protocol->tmp_used + length) {
		protocol->tmp_length *= 2;
		flag = 1;
	}

	if (flag)
		protocol->tmp_content = realloc(protocol->tmp_content,
				protocol->tmp_length);
	if (!protocol->tmp_content) {
		protocol->tmp_length = 0;
		return -FT2_NO_MEMORY;
	}

	memcpy(protocol->tmp_content + protocol->tmp_used, data, length);
	protocol->tmp_used += length;

	return FT2_OK;
}

int parse_command(struct ft2_protocol *protocol, struct tr_data *data)
{
	char *ptr;
	char *start;
	int length;
	int ret = FT2_OK;

	if (!protocol || !data)
		return -FT2_NULL_POINT;

	//FT2_LOG_INFO("%s\n", data->data);
	start = (char *)data->data;
	length = data->length;
	ptr = start;
	while (ptr - start < length) {
		switch (protocol->parse_stage) {
		case 0:
			if (*ptr == FT2_COMMAND_CODE_ACK) {
				process_ack(protocol);
				ptr++;
			} else if (*ptr == FT2_COMMAND_CODE_NAK) {
				process_nak(protocol);
				ptr++;
			} else if (*ptr == FT2_COMMAND_CODE_START) {
				char *rst = strchr(ptr, FT2_COMMAND_CODE_END);
				if (rst && rst - start + 3 <= length) {
					ret = process_cmd(protocol, ptr,
							rst - ptr + 3);
					ptr = rst + 3;
				} else {
					protocol->parse_stage = 1;
				}
			} else {
				ptr++;
			}
			break;
		case 1:
		{
			char *rst = strchr(ptr, FT2_COMMAND_CODE_END);
			if (rst) {
				if (rst - start <= length -3) {
					store_tmp_content(protocol,
							ptr,
							rst - ptr + 3);
					ret = process_cmd(protocol, protocol->tmp_content,
							protocol->tmp_used);
					protocol->tmp_used = 0;
					protocol->tmp_left = 0;
					ptr = rst + 3;
					protocol->parse_stage = 0;
				} else {
					store_tmp_content(protocol,
						ptr,
						length - (ptr - start));
					ptr = start + length;
					protocol->tmp_left = rst - start + 3 -
						length;
					protocol->parse_stage = 2;
				}
			} else {
				store_tmp_content(protocol,
						ptr,
						length - (ptr - start));
				ptr = start + length;
			}
		}
			break;
		case 2:
		{
			if (length - (ptr - start) >= protocol->tmp_left) {
				store_tmp_content(protocol,
						ptr, protocol->tmp_left);
				ret = process_cmd(protocol, protocol->tmp_content,
						protocol->tmp_used);
				ptr += protocol->tmp_left;
				protocol->tmp_used = 0;
				protocol->tmp_left = 0;
				protocol->parse_stage = 0;
			} else {
				unsigned len = length - (ptr - start);
				store_tmp_content(protocol,
						ptr, len);
				protocol->tmp_left -= len;
				ptr += len;
			}
		}
			break;
		default:
			protocol->parse_stage = 0;
			protocol->tmp_used = 0;
			protocol->tmp_left = 0;
			break;
		}
	}

	return FT2_OK;
}

void* ft2_protocol_parse_func(void *data)
{
	struct ft2_protocol *protocol = (struct ft2_protocol*)data;
	struct protocol_data *incoming = NULL;

	if (!protocol)
		return NULL;

	while (1) {
		if (protocol->status == srv_ready) {
			usleep(10000);
			continue;
		} else if (protocol->status == srv_stop) {
			break;
		}
		//FT2_LOG_INFO("parse\n");
		incoming = NULL;
		pthread_mutex_lock(&protocol->mutex_recv);
		if (!list_empty(&protocol->recv_list)) {
			incoming = list_first_entry(&protocol->recv_list,
					struct protocol_data,
					list);
			list_del(&incoming->list);
		}
		pthread_mutex_unlock(&protocol->mutex_recv);
		if (incoming) {
			parse_command(protocol, incoming->transfer);
			put_tr_buffer(incoming->transfer);
			free(incoming);
			incoming = NULL;
		} else {
			usleep(10000);
		}
	}

	FT2_LOG_INFO("parse thread terminated.\n");
	return NULL;
}

int receive_func(void *receiver, struct tr_data *data)
{
	struct ft2_protocol *protocol = (struct ft2_protocol*)receiver;
	struct protocol_data *recv_data = NULL;
	if (!protocol || !data)
		return -FT2_NULL_POINT;

	recv_data = (struct protocol_data *)calloc(1, sizeof(*recv_data));
	if (!recv_data)
		return -FT2_NO_MEMORY;

	recv_data->transfer = get_tr_buffer(data);

	pthread_mutex_lock(&protocol->mutex_recv);
	list_add_tail(&recv_data->list, &protocol->recv_list);
	pthread_mutex_unlock(&protocol->mutex_recv);

	return FT2_OK;
}

int ft2_new_protocol_inst(FT2ProtInst *inst, void *checker, CHECK_COMMAND cb)
{
	int ret = 0;
	struct ft2_protocol *prot = NULL;
	if (!inst) {
		FT2_LOG_ERR("NULL Point, invalid argument\n");
		return -FT2_NULL_POINT;
	}
	prot = (struct ft2_protocol*)calloc(1, sizeof( struct ft2_protocol));
	if (!prot) {
		FT2_LOG_ERR("malloc protocol fail\n");
		return -FT2_NO_MEMORY;
	}

	pthread_mutex_init(&prot->mutex_recv, NULL);
	pthread_mutex_init(&prot->mutex_parse, NULL);
	pthread_mutex_init(&prot->mutex_resp, NULL);

	INIT_LIST_HEAD(&prot->recv_list);
	INIT_LIST_HEAD(&prot->parse_list);

	prot->status = srv_ready;
	ret = pthread_create(&prot->th_parse, NULL,
			ft2_protocol_parse_func, (void*)prot);
	if (ret) {
		FT2_LOG_ERR("create parse thread fail\n");
		ret =  -FT2_THREAD_FAIL;
		goto error;
	}

	ret = ft2_new_transfer_inst(&prot->transfer, (void*)prot, receive_func);
	if (ret)
		goto error1;

	prot->checker = checker;
	prot->check_command = cb;

	prot->status = srv_start;

	*inst = prot;
	return FT2_OK;

error1:
	prot->status = srv_stop;
	pthread_join(prot->th_parse, NULL);
error:

	pthread_mutex_destroy(&prot->mutex_recv);
	pthread_mutex_destroy(&prot->mutex_parse);
	pthread_mutex_destroy(&prot->mutex_resp);
	free(prot);

	return ret;
}

int ft2_delete_protocol_inst(FT2ProtInst inst)
{
	if (!inst)
		return FT2_OK;

	ft2_delete_transfer_inst(inst->transfer);

	inst->status = srv_stop;
	if (inst->th_parse > 0)
		pthread_join(inst->th_parse, NULL);

	pthread_mutex_destroy(&inst->mutex_recv);
	pthread_mutex_destroy(&inst->mutex_parse);
	pthread_mutex_destroy(&inst->mutex_resp);

	if (inst->tmp_content)
		free(inst->tmp_content);
	inst->tmp_content = NULL;
	inst->tmp_length = 0;
	inst->tmp_used = 0;
	inst->tmp_left = 0;
	free(inst);
	return FT2_OK;
}

