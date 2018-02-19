#ifndef __HW_CONTROL_H__
#define __HW_CONTROL_H__
#include <stdint.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include "list.h"
#include "octopus.h"

#ifndef MSG_EXCEPT
#define MSG_EXCEPT      020000
#endif

#define OCTOPUS_MSG_MASTER_FILE	"octopus.msg"
#define MSG_FILE_PREFIX "/var/tmp"

#define MSG_BUF_SIZE			(1024 * 4)
#define MAX_MSG_PATH_LEN		128
#define MAX_

struct msgbuf {
	long mtype;
	char mtext[1];
};

#ifndef MSG_TYPE_REQUEST
#define MSG_TYPE_REQUEST	(1)
#endif

enum {
	DIR_IN			= 0,
	DIR_OUT			= 1,

	NO_DATA			= 0,
	HAS_DATA		= 1,

	OPT_STATUS_OK		= 0,
	OPT_STATUS_FAIL		= 0x80000001,
	OPT_STATUS_UNKNOWN_CMD	= 0x80000002,
	OPT_STATUS_BUSY		= 0x80000003,
	OPT_STATUS_NO_PRIVILEGE	= 0x80000004,

	CMD_DOMAIN_CTL		= 1,
	CMD_DOMAIN_V4L2		= 2,
	CMD_DOMAIN_ISP		= 3,
	CMD_DOMAIN_USB		= 4,
	CMD_DOMAIN_NM		= 5,
	CMD_DOMAIN_IO		= 6,


	/* define CMD code for CTL domain */
	CC_CTL_NEW_MQ		= 1,
	CC_CTL_DEL_MQ		= 2,
	CC_CTL_LOCK_MQ		= 3,
	CC_CTL_UNLOCK_MQ	= 4,

	/* define CMD code for UVC domain */

	/* define CMD code for ISP domain */

	/* define CMD code for USB domain */

};

#define DATA_DIR_RCV(cmd_code) (((cmd_code) & (1 << 31)) == 0)
#define DATA_DIR_SND(cmd_code) (((cmd_code) & (1 << 31)) == 1)

#define MAKE_CTL_CMD(has_data, dir, code) \
	((has_data << 31) | (dir << 30) | (code & 0XFFFF))

#define CTL_CMD_HAS_DATA_OUT(cmd) \
	((cmd & (HAS_DATA << 31)) && (cmd & (DIR_OUT << 30)))

/* Control Command. begin */
#define CMD_CTL_NEW_MQ		MAKE_CTL_CMD(NO_DATA, 0, CC_CTL_NEW_MQ)
#define CMD_CTL_DEL_MQ		MAKE_CTL_CMD(NO_DATA, 0, CC_CTL_DEL_MQ)
#define CMD_CTL_LOCK_MQ		MAKE_CTL_CMD(NO_DATA, 0, CC_CTL_LOCK_MQ)
#define CMD_CTL_UNLOCK_MQ	MAKE_CTL_CMD(NO_DATA, 0, CC_CTL_UNLOCK_MQ)
/* Control Command. end */

struct ctl_msg_p {
	/* bit31: 0: no data, 1: has data
	 * bit30: 0: in, 1:out
	 * bit16~29: reserved
	 * bit0~15: cmd_code
	 */
	uint32_t  cmd;
	char msg_file[MAX_MSG_PATH_LEN];
	char dev_name[MAX_DEV_PATH_LEN];
};

struct isp_msg_p {
};

/*  */
struct cmd_msg {
	long mtype;

	uint32_t domain;
	int pid;
	int status;
	int seq;
	int length;
	uint8_t priv[0];
};

/* struct rsp_msg {
	long mtype;
	uint32_t domain;
	int status;
	int length;
	uint8_t data[0];
}; */

struct msg_queue {
	struct list_head next;
	int msg;
	int locked;
	int pid;
	int ref;
	int master;
	int (*init)(struct msg_queue *mq, const char *dev);
	int (*handler)(struct msg_queue *mq, struct cmd_msg *cmd);
	void (*release)(struct msg_queue *mq);
	char msg_file[MAX_MSG_PATH_LEN];
	void *private_data;
};


/* log utils */
enum {
	OPT_LOG_FINE		= (1 << 0),
	OPT_LOG_INFO		= (1 << 1),
	OPT_LOG_WARNING		= (1 << 2),
	OPT_LOG_ERROR		= (1 << 3),
	OPT_LOG_FATAL		= (1 << 4),
};

extern uint32_t opt_log_level;

#define opt_log(level, fmt, args...)		\
	do { \
		if (level & opt_log_level) \
			syslog(LOG_ERR, "%-20s--> "fmt, __func__, ##args); \
	} while (0)

#define opt_fine(fmt, args...) opt_log(OPT_LOG_FINE, fmt, ##args)
#define opt_info(fmt, args...) opt_log(OPT_LOG_INFO, fmt, ##args)
#define opt_warning(fmt, args...) opt_log(OPT_LOG_WARNING, fmt, ##args)
#define opt_error(fmt, args...) opt_log(OPT_LOG_ERROR, fmt, ##args)
#define opt_fatal(fmt, args...) opt_log(OPT_LOG_FINE, fmt, ##args)

static inline void dump_ctl_msg(void *p)
{
	struct ctl_msg_p *ctl = p;
	opt_log(OPT_LOG_FINE, "CTL cmd:%d msg_file:%s dev_name:%s\n",
			ctl->cmd, ctl->msg_file, ctl->dev_name);
}

static inline void dump_v4l2_msg(void *p)
{
	uint32_t request = *(uint32_t *) p;

	opt_log(OPT_LOG_FINE, "V4L2 request: '%c' %d\n",
			(request >> _IOC_TYPEBITS) & _IOC_TYPEMASK,
			(request >> _IOC_NRSHIFT) & _IOC_NRMASK);
}

static inline void dump_nm_msg(void *p)
{
	int request = *(int *)p;

	opt_log(OPT_LOG_FINE, "NM request: %x\n", request);
}

static inline void dump_io_msg(void *p)
{
	uint8_t *request = (uint8_t *) p;
	opt_log(OPT_LOG_FINE, "IO PTZ request: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, \n",
		request[0], request[1], request[2],
		request[3], request[4], request[5], request[6]);
}

#define dump_cmd_msg(cmd)	\
	do {\
		opt_log(OPT_LOG_FINE, "msg type:%d pid:%d\n",\
				(int) cmd->mtype, cmd->pid); \
		if (cmd->domain == CMD_DOMAIN_CTL) \
			dump_ctl_msg(cmd->priv); \
		else if (cmd->domain == CMD_DOMAIN_V4L2) \
			dump_v4l2_msg(cmd->priv); \
		else if (cmd->domain == CMD_DOMAIN_NM) \
			dump_nm_msg(cmd->priv); \
		else if (cmd->domain == CMD_DOMAIN_IO) \
			dump_io_msg(cmd->priv); \
		opt_log(OPT_LOG_FINE, "\n"); \
	} while (0)


#endif
