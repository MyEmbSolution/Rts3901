#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if_alg.h>
#include <errno.h>
#include <unistd.h>

#ifndef AF_ALG
#define AF_ALG 38
#endif
#define SOL_ALG 279

#define BUFFER_MAX_BYTES	(1024 * 1024)

#define __CMSG_NXTHDR_T(ctl, len, cmsg) __cmsg_nxthdr_t((ctl), (len), (cmsg))
#define CMSG_NXTHDR_T(mhdr, cmsg) cmsg_nxthdr_t((mhdr), (cmsg))

#define CMSG_ALIGN_T(len) (((len)+sizeof(long)-1) & ~(sizeof(long)-1))

#define CMSG_DATA_T(cmsg) ((void *)((char *)(cmsg) + \
				    CMSG_ALIGN_T(sizeof(struct cmsghdr))))
#define CMSG_SPACE_T(len) (CMSG_ALIGN_T(sizeof(struct cmsghdr)) + \
			   CMSG_ALIGN_T(len))
#define CMSG_LEN_T(len) (CMSG_ALIGN_T(sizeof(struct cmsghdr)) + (len))

#define __CMSG_FIRSTHDR_T(ctl, len)	((len) >= sizeof(struct cmsghdr) ? \
					(struct cmsghdr *)(ctl) : \
					(struct cmsghdr *)NULL)
#define CMSG_FIRSTHDR_T(msg)	__CMSG_FIRSTHDR_T((msg)->msg_control, \
				(msg)->msg_controllen)
#define CMSG_OK_T(mhdr, cmsg)	((cmsg)->cmsg_len >= \
				sizeof(struct cmsghdr) && \
				(cmsg)->cmsg_len <= (unsigned long) \
				((mhdr)->msg_controllen - ((char *)(cmsg) - \
				(char *)(mhdr)->msg_control)))

struct cmsghdr *__cmsg_nxthdr_t(void *__ctl, __kernel_size_t __size,
		struct cmsghdr *__cmsg)
{
	struct cmsghdr *__ptr;

	__ptr = (struct cmsghdr *)(((unsigned char *) __cmsg) +
				  CMSG_ALIGN(__cmsg->cmsg_len));
	if ((unsigned long)((char *)(__ptr+1) - (char *) __ctl) > __size)
		return (struct cmsghdr *)0;

	return __ptr;
}

struct cmsghdr *cmsg_nxthdr_t (struct msghdr *__msg, struct cmsghdr *__cmsg)
{
	return __cmsg_nxthdr_t(__msg->msg_control,
			       __msg->msg_controllen, __cmsg);
}

int main(int argc, char *argv[])
{
	int opfd;
	int tfmfd;
	struct sockaddr_alg sa = {
		.salg_family = AF_ALG,
		.salg_type = "skcipher",
		.salg_name = "cbc(aes)",
	};
	char algname[128];
	char key[32] = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00,
			0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
			0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
			0x07, 0x00, 0x00, 0x00, 0x08};
	char ivec[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	int keylen = 16;
	int iveclen = 16;
	char input_text[BUFFER_MAX_BYTES];
	char output_text[BUFFER_MAX_BYTES];
	char deoutput_text[BUFFER_MAX_BYTES];
	struct msghdr in_msg;
	struct msghdr out_msg;
	struct cmsghdr *cmsg;
	struct iovec *in_iov;
	struct iovec *out_iov;
	struct af_alg_iv *af_iv;
	int in_len, out_len, iv_op, ret, i;
	FILE *in_fd;

	memset(&in_msg, 0, sizeof(in_msg));
	memset(&out_msg, 0, sizeof(out_msg));

	if (argc == 2) {
		if (strcmp(argv[1], "-h") == 0) {
			printf("help:\n");
			printf("testcrypto <input file> [ <mode name> <crypto name> <more> ]\n");
			return 0;
		}
	} else if (argc == 4 || argc == 5) {
		sprintf(algname, "%s(%s)", argv[2], argv[3]);
		memcpy(sa.salg_name, algname, strlen(algname));
		sa.salg_name[strlen(algname)] = '\0';
		if (strcmp(argv[3], "des") == 0) {
			keylen = 8;
			iveclen = 8;
		} else if (strcmp(argv[3], "aes") == 0) {
			iveclen = 16;
			if (argc == 5) {
				switch (atoi(argv[4])) {
				case 192:
					keylen = 24;
					break;
				case 256:
					keylen = 32;
					break;
				case 128:
				default:
					keylen = 16;
					break;
				}
			}
		} else if (strcmp(argv[3], "des3") == 0 ||
			   strcmp(argv[3], "des3_ede") == 0) {
			keylen = 24;
			iveclen = 8;
		} else {
			printf("param err\n");
			return -1;
		}
	} else {
		printf("param err, use 'testcrypto -h'\n");
		return -1;
	}

	printf("alg name is %s\n", sa.salg_name);

	in_fd = fopen(argv[1], "r");
	if (in_fd < 0) {
		printf("Open %s Error\n", argv[1]);
		return -1;
	}

	in_len = fread(input_text, 1, BUFFER_MAX_BYTES - 1, in_fd);

	fclose(in_fd);

	if (strstr((char *)sa.salg_name, "ecb") != NULL)
		iv_op = 0;
	else
		iv_op = 1;

	/* socket */
	tfmfd = socket(AF_ALG, SOCK_SEQPACKET, 0);
	if (tfmfd < 0) {
		printf("socket err\n");
		return tfmfd;
	}

	/* bind */
	ret = bind(tfmfd, (struct sockaddr *)&sa, sizeof(sa));
	if (ret < 0) {
		printf("bind err\n");
		return ret;
	}

	/* set socket opt (set key) */
	ret = setsockopt(tfmfd, SOL_ALG, ALG_SET_KEY, key, keylen);
	if (ret < 0) {
		printf("setsockopt err\n");
		return ret;
	}

	/* accept */
	opfd = accept(tfmfd, NULL, 0);
	if (opfd < 0) {
		printf("accept err\n");
		return opfd;
	}

	/*
	 * encrypto
	 */

	if (strstr((char *)sa.salg_name, "ecb") != NULL ||
			strstr((char *)sa.salg_name, "cbc") != NULL)
		out_len = (in_len / 16 + 1) * 16;
	else
		out_len = in_len;

	/* init in_msg & sendmsg */
	/* iovec (store encrypto input text) */
	in_iov = (struct iovec *)malloc(sizeof(struct iovec));
	in_iov->iov_base = input_text;
	in_iov->iov_len = out_len;
	in_msg.msg_iov = in_iov;
	in_msg.msg_iovlen = 1;

	/* msg_control (store iv & op) */
	if (iv_op == 0) {
		in_msg.msg_controllen = CMSG_SPACE_T(sizeof(int));
		in_msg.msg_control = (void *)malloc(in_msg.msg_controllen);
	} else {
		in_msg.msg_controllen = CMSG_SPACE_T(sizeof(int)) +
			CMSG_SPACE_T(sizeof(struct af_alg_iv) + iveclen);
		in_msg.msg_control = (void *)malloc(in_msg.msg_controllen);
	}

	cmsg = CMSG_FIRSTHDR_T(&in_msg);
	cmsg->cmsg_len = CMSG_LEN_T(sizeof(int));
	cmsg->cmsg_level = SOL_ALG;
	cmsg->cmsg_type = ALG_SET_OP;
	*(int *)CMSG_DATA_T(cmsg) = ALG_OP_ENCRYPT;

	if (iv_op == 1) {
		cmsg = CMSG_NXTHDR_T(&in_msg, cmsg);
		cmsg->cmsg_len =
			CMSG_LEN_T(sizeof(struct af_alg_iv) + iveclen);
		cmsg->cmsg_level = SOL_ALG;
		cmsg->cmsg_type = ALG_SET_IV;
		af_iv = (struct af_alg_iv *)CMSG_DATA_T(cmsg);
		af_iv->ivlen = iveclen;
		memcpy(af_iv->iv, ivec, iveclen);
	}

	ret = sendmsg(opfd, &in_msg, 0);
	if (ret < 0) {
		printf("sendmsg err, errno = %d\n", errno);
		return ret;
	}

	/* init out_msg & recvmsg */
	out_iov = (struct iovec *)malloc(sizeof(struct iovec));
	out_iov->iov_base = output_text;
	out_iov->iov_len = out_len;
	out_msg.msg_iov = out_iov;
	out_msg.msg_iovlen = 1;

	ret = recvmsg(opfd, &out_msg, 0);
	if (ret < 0) {
		printf("read err\n");
		return ret;
	}

	free(in_iov);
	free(in_msg.msg_control);
	free(out_iov);

	/*
	 * decrypto
	 */

	memset(&in_msg, 0, sizeof(in_msg));
	memset(&out_msg, 0, sizeof(out_msg));
	out_len = in_len;

	/* init in_msg & sendmsg */
	/* iovec (store encrypto input text) */
	in_iov = (struct iovec *)malloc(sizeof(struct iovec));
	in_iov->iov_base = output_text;
	in_iov->iov_len = out_len;
	in_msg.msg_iov = in_iov;
	in_msg.msg_iovlen = 1;

	/* msg_control (store iv & op) */
	if (iv_op == 0) {
		in_msg.msg_controllen = CMSG_SPACE_T(sizeof(int));
		in_msg.msg_control = (void *)malloc(in_msg.msg_controllen);
	} else {
		in_msg.msg_controllen = CMSG_SPACE_T(sizeof(int)) +
			CMSG_SPACE_T(sizeof(struct af_alg_iv) + iveclen);
		in_msg.msg_control = (void *)malloc(in_msg.msg_controllen);
	}

	cmsg = CMSG_FIRSTHDR_T(&in_msg);
	cmsg->cmsg_len = CMSG_LEN_T(sizeof(int));
	cmsg->cmsg_level = SOL_ALG;
	cmsg->cmsg_type = ALG_SET_OP;
	*(int *)CMSG_DATA_T(cmsg) = ALG_OP_DECRYPT;

	if (iv_op == 1) {
		cmsg = CMSG_NXTHDR_T(&in_msg, cmsg);
		cmsg->cmsg_len =
			CMSG_LEN_T(sizeof(struct af_alg_iv) + iveclen);
		cmsg->cmsg_level = SOL_ALG;
		cmsg->cmsg_type = ALG_SET_IV;
		af_iv = (struct af_alg_iv *)CMSG_DATA_T(cmsg);
		af_iv->ivlen = iveclen;
		memcpy(af_iv->iv, ivec, iveclen);
	}

	ret = sendmsg(opfd, &in_msg, 0);
	if (ret < 0) {
		printf("sendmsg err, errno = %d\n", errno);
		return ret;
	}

	/* init out_msg & recvmsg */
	out_iov = (struct iovec *)malloc(sizeof(struct iovec));
	out_iov->iov_base = deoutput_text;
	out_iov->iov_len = out_len;
	out_msg.msg_iov = out_iov;
	out_msg.msg_iovlen = 1;

	ret = recvmsg(opfd, &out_msg, 0);
	if (ret < 0) {
		printf("read err\n");
		return ret;
	}

	free(in_iov);
	free(in_msg.msg_control);
	free(out_iov);

	close(opfd);
	close(tfmfd);

	for (i = 0; i < in_len; i++) {
		if (deoutput_text[i] != input_text[i]) {
			printf("compare error\n");
			return -1;
		}
	}

	printf("crypto test OK\n");

	return 0;
}
