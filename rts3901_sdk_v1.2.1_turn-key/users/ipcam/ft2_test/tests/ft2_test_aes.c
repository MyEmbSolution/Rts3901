/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_aes.c
 *
 * Copyright (C) 2014      Wind Han<wind_han@realsil.com.cn>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/socket.h>
#include <linux/if_alg.h>
#include "protocol.h"
#include "ft2errno.h"
#include "ft2log.h"
#include <errno.h>

#include <sys/select.h>
#include <sys/time.h>

#define FT2_AES_MODE_NUM	6
#define FT2_AES_ALGS_NUM	3
#define FT2_AES_BUF_MAX_BYTES	(1024 * 1024)
#ifndef SOL_ALG
#define SOL_ALG			279
#endif

static char ft2_aes_mode[FT2_AES_MODE_NUM][10] = {
				"ecb", "cbc", "cbc-cs1",
				"cbc-cs2", "cbc-cs3", "ctr"};
static char ft2_aes_algs[FT2_AES_ALGS_NUM][10] = {
				"aes", "des", "des3_ede"};

int ft2_aes_check(struct protocol_command *pcmd)
{
	return FT2_OK;
}

static int aes_check_path(char *path)
{
	char dirpath[128];
	char filename[128];
	int i, j;

	if (!path || strlen(path) == 0)
		return -FT2_ERROR;

	if (0 > access(path, R_OK | W_OK)) {
		FT2_LOG_ERR("%s is not exist or no right to rw\n", path);
		return -FT2_ERROR;
	}

	snprintf(dirpath, sizeof(dirpath), "%s/aes", path);
	if (0 > access(dirpath, F_OK)) {
		if (0 > mkdir(dirpath, 0777)) {
			FT2_LOG_ERR("fail to mkdir %s\n", dirpath);
			return -FT2_ERROR;
		}
	}

	snprintf(dirpath, sizeof(dirpath), "%s/aes/src", path);
	if (0 > access(dirpath, F_OK)) {
		if (0 > mkdir(dirpath, 0777)) {
			FT2_LOG_ERR("fail to mkdir %s\n", dirpath);
			return -FT2_ERROR;
		}
	}

	snprintf(filename, sizeof(filename), "%s/en.input", dirpath);
	if (0 > access(filename, R_OK)) {
		FT2_LOG_ERR("input %s is not exist\n", filename);
		return -FT2_ERROR;
	}

	for (i = 0; i < FT2_AES_MODE_NUM; i++) {
		for (j = 0; j < FT2_AES_ALGS_NUM; j++) {
			snprintf(filename, sizeof(filename),
					"%s/%s_%s_de.input", dirpath,
					ft2_aes_mode[i], ft2_aes_algs[j]);
			if (0 > access(filename, R_OK)) {
				FT2_LOG_ERR("input %s is not exist\n",
						filename);
				return -FT2_ERROR;
			}
		}
	}

	snprintf(dirpath, sizeof(dirpath), "%s/aes/dst", path);
	if (0 > access(dirpath, F_OK)) {
		if (0 > mkdir(dirpath, 0777)) {
			FT2_LOG_ERR("fail to mkdir %s\n", dirpath);
			return -FT2_ERROR;
		}
	}

	return FT2_OK;
}

static int ft2_aes_crypto(char *path, char *mode, char *alg, int type)
{
	int opfd = -1;
	int tfmfd = -1;
	struct sockaddr_alg sa = {
		.salg_family = AF_ALG,
		.salg_type = "skcipher",
		.salg_name = "cbc(aes)",
	};
	char srcpath[128];
	char dstpath[128];
	char key[32] = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00,
			0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
			0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
			0x07, 0x00, 0x00, 0x00, 0x08};
	char ivec[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	int keylen = 16;
	int iveclen = 16;
	int iv_op, in_len, out_len, ret, i;
	FILE *in_fd, *out_fd;
	char *input_text = NULL;
	char *output_text = NULL;
	struct msghdr in_msg;
	struct msghdr out_msg;
	struct cmsghdr *cmsg;
	struct iovec *in_iov = NULL;
	struct iovec *out_iov = NULL;
	struct af_alg_iv *af_iv;
	char charval;

	memset(&in_msg, 0, sizeof(in_msg));
	memset(&out_msg, 0, sizeof(out_msg));
	input_text = (char *)calloc(1, sizeof(char) * FT2_AES_BUF_MAX_BYTES);
	output_text = (char *)calloc(1, sizeof(char) * FT2_AES_BUF_MAX_BYTES);

	if (!path || !mode || !alg) {
		FT2_LOG_ERR("paramter path | mode | alg is null\n");
		ret = -FT2_NULL_POINT;
		goto out;
	}

	/* alg name */
	snprintf(sa.salg_name, sizeof(sa.salg_name), "%s(%s)", mode, alg);

	/* key & iv len */
	if (0 == strcasecmp(alg, "des")) {
		keylen = 8;
		iveclen = 8;
	} else if (0 == strcasecmp(alg, "aes")) {
		keylen = 16;
		iveclen = 16;
	} else if (0 == strcasecmp(alg, "des3_ede")) {
		keylen = 24;
		iveclen = 8;
	} else {
		FT2_LOG_ERR("paramter alg is error\n");
		ret = -FT2_INVALID_ARGUMENT;
		goto out;
	}

	if (strstr(sa.salg_name, "ecb"))
		iv_op = 0;
	else
		iv_op = 1;

	if (type == ALG_OP_ENCRYPT)
		snprintf(srcpath, sizeof(srcpath), "%s/aes/src/en.input", path);
	else
		snprintf(srcpath, sizeof(srcpath), "%s/aes/src/%s_%s_de.input",
				path, mode, alg);

	in_fd = fopen(srcpath, "r");
	if (0 > in_fd) {
		FT2_LOG_ERR("fail to open %s\n", srcpath);
		ret = -FT2_INVALID_ARGUMENT;
		goto out;
	}

	in_len = fread(input_text, 1, FT2_AES_BUF_MAX_BYTES, in_fd);
	fclose(in_fd);

	if (type == ALG_OP_ENCRYPT) {
		if (0 == strcasecmp(mode, "ecb") ||
				0 == strcasecmp(mode, "cbc"))
			out_len = (in_len / 16 + 1) * 16;
		else
			out_len = in_len;
	} else {
		out_len = in_len;
	}

	/* socket */
	tfmfd = socket(AF_ALG, SOCK_SEQPACKET, 0);
	if (tfmfd < 0) {
		FT2_LOG_ERR("socket error\n");
		ret = -FT2_ERROR;
		goto out;
	}

	/* bind */
	ret = bind(tfmfd, (struct sockaddr *)&sa, sizeof(sa));
	if (ret < 0) {
		FT2_LOG_ERR("bind error\n");
		ret = -FT2_ERROR;
		goto out;
	}

	/* set socket opt (key) */
	ret = setsockopt(tfmfd, SOL_ALG, ALG_SET_KEY, key, keylen);
	if (ret < 0) {
		FT2_LOG_ERR("setsockopt error\n");
		ret = -FT2_ERROR;
		goto out;
	}

	/* accept */
	opfd = accept(tfmfd, NULL, 0);
	if (opfd < 0) {
		FT2_LOG_ERR("accept error\n");
		ret = -FT2_ERROR;
		goto out;
	}

	/* init in_msg & sendmsg
	 * iovec (store crypto input text)
	 */
	in_iov = (struct iovec *)calloc(1, sizeof(struct iovec));
	in_iov->iov_base = (void *)input_text;
	in_iov->iov_len = out_len;
	in_msg.msg_iov = in_iov;
	in_msg.msg_iovlen = 1;

	/* msg_control (store iv & op) */
	if (iv_op == 0)
		in_msg.msg_controllen = CMSG_SPACE(sizeof(int));
	else
		in_msg.msg_controllen = CMSG_SPACE(sizeof(int)) +
			CMSG_SPACE(sizeof(struct af_alg_iv) + iveclen);
	in_msg.msg_control = (void *)calloc(1, in_msg.msg_controllen);

	cmsg = CMSG_FIRSTHDR(&in_msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_ALG;
	cmsg->cmsg_type = ALG_SET_OP;
	*(int *)CMSG_DATA(cmsg) = type;

	if (iv_op == 1) {
		cmsg = CMSG_NXTHDR(&in_msg, cmsg);
		cmsg->cmsg_len = CMSG_LEN(sizeof(struct af_alg_iv) + iveclen);
		cmsg->cmsg_level = SOL_ALG;
		cmsg->cmsg_type = ALG_SET_IV;
		af_iv = (struct af_alg_iv *)CMSG_DATA(cmsg);
		af_iv->ivlen = iveclen;
		memcpy(af_iv->iv, ivec, iveclen);
	}

	ret = sendmsg(opfd, &in_msg, 0);
	if (ret < 0) {
		FT2_LOG_ERR("sendmsg error, errno = %d\n", errno);
		ret = -FT2_ERROR;
		goto out;
	}

	/* init out_msg & recvmsg */
	out_iov = (struct iovec *)calloc(1, sizeof(struct iovec));
	out_iov->iov_base = (void *)output_text;
	out_iov->iov_len = out_len;
	out_msg.msg_iov = out_iov;
	out_msg.msg_iovlen = 1;

	ret = recvmsg(opfd, &out_msg, MSG_WAITALL);
	if (ret < 0) {
		FT2_LOG_ERR("recvmsg error, errno = %d\n", errno);
		ret = -FT2_ERROR;
		goto out;
	}

	if (type == ALG_OP_ENCRYPT)
		snprintf(dstpath, sizeof(dstpath),
				"%s/aes/dst/%s_%s_en.output",
				path, mode, alg);
	else
		snprintf(dstpath, sizeof(dstpath),
				"%s/aes/dst/%s_%s_de.output",
				path, mode, alg);

	out_fd = fopen(dstpath, "rw");
	if (!out_fd) {
		FT2_LOG_ERR("fail to open %s\n", dstpath);
		ret = -FT2_INVALID_ARGUMENT;
		goto out;
	}

	for (i = 0; i < in_len; i++) {
		charval = output_text[i];
		fputc(charval, out_fd);
	}

	fclose(out_fd);

	ret = FT2_OK;
out:
	if (in_iov)
		free(in_iov);
	if (in_msg.msg_control)
		free(in_msg.msg_control);
	if (out_iov)
		free(out_iov);
	if (opfd > 0)
		close(opfd);
	if (tfmfd > 0)
		close(tfmfd);
	if (input_text)
		free(input_text);
	if (output_text)
		free(output_text);
	return ret;
}

int ft2_aes_runonce(void *priv, struct protocol_command *pcmd, char *path)
{
	int i, j, ret;

	if (!pcmd)
		return -FT2_NULL_POINT;

	if (aes_check_path(path)) {
		append_pt_command_result(pcmd,
				FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
		return -FT2_ERROR;
	}

	/* encrypt */
	for (i = 0; i < FT2_AES_MODE_NUM; i++) {
		for (j = 0; j < FT2_AES_ALGS_NUM; j++) {
			ret = ft2_aes_crypto(path, ft2_aes_mode[i],
					ft2_aes_algs[j], ALG_OP_ENCRYPT);
			if (ret) {
				FT2_LOG_ERR("encrypt %s(%s) is failed\n",
						ft2_aes_mode[i],
						ft2_aes_algs[j]);
				goto error;
			}
		}
	}

	/* decrypt */
	for (i = 0; i < FT2_AES_MODE_NUM; i++) {
		for (j = 0; j < FT2_AES_ALGS_NUM; j++) {
			ret = ft2_aes_crypto(path, ft2_aes_mode[i],
					ft2_aes_algs[j], ALG_OP_DECRYPT);
			if (ret) {
				FT2_LOG_ERR("decrypt %s(%s) is failed\n",
						ft2_aes_mode[i],
						ft2_aes_algs[j]);
				goto error;
			}
		}
	}

	append_pt_command_result(pcmd,
			FT2_TEST_PASS, strlen(FT2_TEST_PASS));
	return FT2_OK;
error:
	append_pt_command_result(pcmd,
			FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
	return -FT2_ERROR;
}
