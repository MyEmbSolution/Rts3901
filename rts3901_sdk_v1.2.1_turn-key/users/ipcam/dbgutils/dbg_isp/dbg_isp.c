#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <rtsisp.h>
#include <rtsv4l2.h>
#include <rtscamkit.h>
#include "dbg_log.h"
#include "dbg_ctrl.h"
#include "dbg_priv.h"
#include "dbg_strm.h"
#include "dbg_h264.h"

#define VERSION_CMD 			"1.1.4"

enum rts_dbg_cmd_level {
	ctrl_level = 0,
	cmnd_level,
	strm_level,
	prvw_level,
	drv_level,
	reversed,
};

struct rts_dbg_cmd {
	struct list_head list;
	int cmd_id;

	uint32_t arg1;
	uint32_t arg2;
	uint32_t arg3;
	uint32_t arg4;
	uint8_t *buf;
	struct rts_dbg_option *option;

	enum rts_dbg_cmd_level level;
};

#define VAL_OPT_LIST_CTRL			0x01
#define VAL_OPT_QUERY_CTRL			0x02
#define VAL_OPT_GET_CTRL			0x03
#define VAL_OPT_SET_CTRL			0x04
#define VAL_OPT_LIST_STREAM			0x05
#define VAL_OPT_SET_FMT				0x06
#define VAL_OPT_GET_FMT				0x07
#define VAL_OPT_SET_FPS				0x08
#define VAL_OPT_GET_FPS				0x09
#define VAL_OPT_R_MEM				0x0a
#define VAL_OPT_W_MEM				0x0b
#define VAL_OPT_R_I2C				0x0c
#define VAL_OPT_W_I2C				0x0d
#define VAL_OPT_ENCODE_H264			0x0e
#define VAL_OPT_PREVIEW_NUM			0x0f
#define VAL_OPT_ENCODE_H264_STAB		0x10
#define VAL_OPT_LOG_SHOW			0x11
#define VAL_OPT_LOG_NAME			0x12
#define VAL_OPT_TD_ON				0x14
#define VAL_OPT_TD_OFF				0x15
#define VAL_OPT_CAMERA_DETACH			0x16
#define VAL_OPT_CAMERA_ATTACH			0x17
#define VAL_OPT_FW_VERSION			0x18
#define VAL_OPT_H264_GDR			0x19
#define VAL_OPT_H264_ROTATION			0x21
#define VAL_OPT_DEHAZE				0x22
#define VAL_OPT_ENCODE_MJPG			0x23
#define VAL_OPT_LOG_SHOW_H264			0x24
#define VAL_OPT_LOG_SHOW_BPS			0x25
#define VAL_OPT_LOG_SHOW_MJPG			0x26
#define VAL_OPT_LOG_SHOW_BMP			0x27
#define VAL_OPT_LDC_ON				0x28
#define VAL_OPT_LDC_OFF				0x29
#define VAL_OPT_G_LDC				0x2a
#define VAL_OPT_H264_ROI			0x2b

#define VAL_OPT_PREVIEW				'p'
#define VAL_OPT_FRAME_SAVE			's'
#define VAL_OPT_BMP_SAVE			'b'
#define VAL_OPT_FRAME_NUMBER			'n'
#define VAL_OPT_H264_BPS			'r'
#define VAL_OPT_H264_INTRAQPDELTA		'A'

struct option longopts[] = {
	{"help", no_argument, NULL, 'h'},
	{"device", required_argument, NULL, 'd'},
	{"list-ctrl", no_argument, NULL, VAL_OPT_LIST_CTRL},
	{"query-ctrl", required_argument, NULL, VAL_OPT_QUERY_CTRL},
	{"get-ctrl", required_argument, NULL, VAL_OPT_GET_CTRL},
	{"set-ctrl", required_argument, NULL, VAL_OPT_SET_CTRL},
	{"list-stream", no_argument, NULL, VAL_OPT_LIST_STREAM},
	{"get-fmt", no_argument, NULL, VAL_OPT_GET_FMT},
	{"set-fmt", required_argument, NULL, VAL_OPT_SET_FMT},
	{"get-fps", no_argument, NULL, VAL_OPT_GET_FPS},
	{"set-fps", required_argument, NULL, VAL_OPT_SET_FPS},
	{"preview", no_argument, NULL, VAL_OPT_PREVIEW},
	{"read-mem", required_argument, NULL, VAL_OPT_R_MEM},
	{"write-mem", required_argument, NULL, VAL_OPT_W_MEM},
	{"read-i2c", required_argument, NULL, VAL_OPT_R_I2C},
	{"write-i2c", required_argument, NULL, VAL_OPT_W_I2C},
	{"save", required_argument, NULL, VAL_OPT_FRAME_SAVE},
	{"bmp", no_argument, NULL, VAL_OPT_BMP_SAVE},
	{"number", required_argument, NULL, VAL_OPT_FRAME_NUMBER},
	{"h264", no_argument, NULL, VAL_OPT_ENCODE_H264},
	{"max", required_argument, NULL, VAL_OPT_PREVIEW_NUM},
	{"stab", no_argument, NULL, VAL_OPT_ENCODE_H264_STAB},
	{"bps", required_argument, NULL, VAL_OPT_H264_BPS},
	{"show", no_argument, NULL, VAL_OPT_LOG_SHOW},
	{"show-h264", no_argument, NULL, VAL_OPT_LOG_SHOW_H264},
	{"show-bps", no_argument, NULL, VAL_OPT_LOG_SHOW_BPS},
	{"show-mjpeg", no_argument, NULL, VAL_OPT_LOG_SHOW_MJPG},
	{"show-bmp", no_argument, NULL, VAL_OPT_LOG_SHOW_BMP},
	{"tdon", no_argument, NULL, VAL_OPT_TD_ON},
	{"tdoff", no_argument, NULL, VAL_OPT_TD_OFF},
	{"detach", no_argument, NULL, VAL_OPT_CAMERA_DETACH},
	{"attach", no_argument, NULL, VAL_OPT_CAMERA_ATTACH},
	{"fw-version", no_argument, NULL, VAL_OPT_FW_VERSION},
	{"gdr", required_argument, NULL, VAL_OPT_H264_GDR},
	{"intraQpDelta", required_argument, NULL, VAL_OPT_H264_INTRAQPDELTA},
	{"rotation", required_argument, NULL, VAL_OPT_H264_ROTATION},
	{"dehaze", required_argument, NULL, VAL_OPT_DEHAZE},
	{"mjpeg", no_argument, NULL, VAL_OPT_ENCODE_MJPG},
	{"ldcon", required_argument, NULL, VAL_OPT_LDC_ON},
	{"ldcoff", no_argument, NULL, VAL_OPT_LDC_OFF},
	{"get-ldc", no_argument, NULL, VAL_OPT_G_LDC},
	{"roi", required_argument, NULL, VAL_OPT_H264_ROI},
	{0,0,0,0}
};

int g_iMainexit = 0;
int g_help = 0;

static void Termination(int sign)
{
	DBG_LOG_PRINT("dbg_isp terminated\n");
	g_iMainexit = 1;
}

void print_usage()
{
	if (g_help)
		return;
	DBG_LOG_PRINT("----Welcome to Realtek ISP dbg tool. Version v" VERSION_CMD "\n");
	DBG_LOG_PRINT("\nUsage for this program:\n");
	DBG_LOG_PRINT("--help       | -h  --                 print help message\n");
	DBG_LOG_PRINT("--device     | -d  --                 select camera device\n");
	DBG_LOG_PRINT("--list-ctrl                           list all ctrls\n");
	DBG_LOG_PRINT("--query-ctrl <id>                     query certain ctrl with id\n");
	DBG_LOG_PRINT("--get-ctrl <id>                       get ctrl cur value\n");
	DBG_LOG_PRINT("--set-ctrl <id> <val>                 set ctrl value\n"
	"                                      (For negative values: -s <id> -- -42)\n");
	DBG_LOG_PRINT("--list-stream                         list all stream info\n");
	DBG_LOG_PRINT("--get-fmt                             get current fmt info\n");
	DBG_LOG_PRINT("--set-fmt <index> <width> <height>    set fmt, index set list-stream\n");
	DBG_LOG_PRINT("--get-fps                             get current fps info\n");
	DBG_LOG_PRINT("--set-fps <numerator> <denominator>   set fps\n");
	DBG_LOG_PRINT("--preview     | -p                    start preview\n");
	DBG_LOG_PRINT("--save <path> | -s                    save frame\n");
	DBG_LOG_PRINT("--read-mem <addr> <len>               read xmem\n");
	DBG_LOG_PRINT("--write-mem <addr> <len>  <...data>   write xmem\n");
	DBG_LOG_PRINT("--read-i2c <slaveid> <len>            read i2c(general mode)\n");
	DBG_LOG_PRINT("--write-i2c <slaveid> <len>           write i2c(general mode)\n");
	DBG_LOG_PRINT("--detach                              disable mcu\n");
	DBG_LOG_PRINT("--attach                              enable mcu\n");
	DBG_LOG_PRINT("--tdon                                enable td\n");
	DBG_LOG_PRINT("--tdoff                               disable td\n");
	DBG_LOG_PRINT("--fw-version                          get fireware version\n");
	DBG_LOG_PRINT("--bmp        | -b                     convert yuv to bmp\n");
	DBG_LOG_PRINT("--number     | -n                     the number of frames that will be saved\n");
	DBG_LOG_PRINT("--h264                                enable h264\n");
	DBG_LOG_PRINT("--mjpeg                               enable mjpeg\n");
	DBG_LOG_PRINT("--max                                 the max number of frames that will be previewed\n");
	DBG_LOG_PRINT("--gdr                                 set gdr\n");
	DBG_LOG_PRINT("--intraQpDelta                        set intraQpDelta\n");
	DBG_LOG_PRINT("--rotation                            set image rotation\n");
	DBG_LOG_PRINT("--dehaze                              enable/disable dehaze\n");
	DBG_LOG_PRINT("--ldcon <ldctable>                    enable ldc\n");
	DBG_LOG_PRINT("--ldcoff                              disable ldc\n");
	DBG_LOG_PRINT("--get-ldc                             get ldc state\n");
	DBG_LOG_PRINT("--roi <left><top><right><bottom>      set h264 roi\n");
	DBG_LOG_PRINT("--show                                show frame time\n");
	DBG_LOG_PRINT("--show-h264                           show h264 frame info\n");
	DBG_LOG_PRINT("--show-bps                            show h264 bps\n");
	DBG_LOG_PRINT("--show-mjpeg                          show mjpeg info\n");
	DBG_LOG_PRINT("--show-bmp                            show bps info\n");
	g_help++;
}

static const char *get_cmd_name(int val)
{
	int size = sizeof(longopts)/sizeof(longopts[0]);
	int i;
	for (i = 0; i < size; i++) {
		if (val == longopts[i].val)
			return longopts[i].name;
	}
	return NULL;
}

static struct rts_dbg_cmd *alloc_cmd(int id, int size, enum rts_dbg_cmd_level level)
{
	struct rts_dbg_cmd *cmd;

	cmd = malloc(size);
	if (!cmd) {
		DBG_LOG_ERR("alloc cmd buf failed, cmd id = %d\n", id);
		return NULL;
	}
	memset(cmd, 0, size);
	cmd->cmd_id = id;
	cmd->level = level;
	//DBG_LOG_LOG("id = %d, cmd_id = %d\n", id, cmd->cmd_id);

	return cmd;
}

int parse_option(int argc, char *argv[], struct rts_dbg_option *option)
{
	int c = 0;
	struct rts_dbg_cmd *cmd = NULL;
	int ret = 0;
	static int CMDSIZE = sizeof(struct rts_dbg_cmd);

	if (!option)
		return -1;

	snprintf(option->devname, sizeof(option->devname), "/dev/video51");
	option->saveflag = 0;
	option->bmpflag = 0;
	option->mjpgflag = 0;
	option->framenum = 1;
	option->h264flag = 0;
	option->previewnum = 0;
	option->h264stab = 0;
	option->h264bps = 0;
	option->gdr = 0;
	option->rotation = 0;
	option->roi = 0;
	g_show_mask = 0;
	while((c = getopt_long(argc, argv, ":hd:ps:b:n:r:A:", longopts, NULL)) != -1) {
		cmd = NULL;
		switch (c) {
		case 'h':
			print_usage();
			break;
		case 'd':
			snprintf(option->devname, sizeof(option->devname),
				"%s", optarg);
			break;
		case VAL_OPT_LIST_CTRL:
			cmd = alloc_cmd(c, CMDSIZE, ctrl_level);
			break;
		case VAL_OPT_QUERY_CTRL:
		case VAL_OPT_GET_CTRL:
			cmd = alloc_cmd(c, CMDSIZE, ctrl_level);
			if (cmd) {
				cmd->arg1 = (uint32_t)strtol(optarg, NULL, 0);
			}
			break;
		case VAL_OPT_SET_CTRL:
			if (optind + 1 > argc) {
				DBG_LOG_ERR("--set-ctrl need more argument.\n");
				return -1;
			}
			cmd = alloc_cmd(c, CMDSIZE, ctrl_level);
			if (cmd) {
				cmd->arg1 = (uint32_t)strtol(optarg, NULL, 0);
				cmd->arg2 = (uint32_t)strtol(argv[optind], NULL, 0);
				if (strncmp(argv[optind], "--", 2) == 0 && optind + 2 <= argc)
					cmd->arg2 = (uint32_t)strtol(argv[optind + 1], NULL, 0);
				else
					cmd->arg2 = (uint32_t)strtol(argv[optind], NULL, 0);
			}
			break;
		case VAL_OPT_LIST_STREAM:
		case VAL_OPT_GET_FMT:
		case VAL_OPT_GET_FPS:
			cmd = alloc_cmd(c, CMDSIZE, strm_level);
			break;

		case VAL_OPT_SET_FMT:
			if (optind + 2 > argc) {
				DBG_LOG_ERR("--set-fmt need more argument.\n");
				return -1;
			}
			cmd = alloc_cmd(c, CMDSIZE, strm_level);
			if (cmd) {
				cmd->arg1 = (uint32_t)strtol(optarg, NULL, 0);
				cmd->arg2 = (uint32_t)strtol(argv[optind], NULL, 0);
				cmd->arg3 = (uint32_t)strtol(argv[optind + 1], NULL, 0);
			}
			break;
		case VAL_OPT_SET_FPS:
			if (optind + 1 > argc) {
				DBG_LOG_ERR("--set-fmt need more argument.\n");
				return -1;
			}
			cmd = alloc_cmd(c, CMDSIZE, strm_level);
			if (cmd) {
				cmd->arg1 = (uint32_t)strtol(optarg, NULL, 0);
				cmd->arg2 = (uint32_t)strtol(argv[optind], NULL, 0);
			}
			break;
		case VAL_OPT_PREVIEW:
			cmd = alloc_cmd(c, CMDSIZE, prvw_level);
			break;
		case VAL_OPT_R_MEM:
		case VAL_OPT_R_I2C:
			if (optind + 1 > argc) {
				DBG_LOG_ERR("--read-mem/i2c need more argument.\n");
				return -1;
			}
			cmd = alloc_cmd(c, CMDSIZE, cmnd_level);
			if (cmd) {
				cmd->arg1 = (uint32_t)strtol(optarg, NULL, 0);
				cmd->arg2 = (uint32_t)strtol(argv[optind], NULL, 0);
				if (cmd->arg2 > 0)
					cmd->buf = (uint8_t*) malloc(cmd->arg2);
			}
			break;
		case VAL_OPT_W_MEM:
		case VAL_OPT_W_I2C:
			if (optind + 1 > argc) {
				DBG_LOG_ERR("--write-mem/i2c need more argument.\n");
				return -1;
			}
			cmd = alloc_cmd(c, CMDSIZE, cmnd_level);
			if (cmd) {
				cmd->arg1 = (uint32_t)strtol(optarg, NULL, 0);
				cmd->arg2 = (uint32_t)strtol(argv[optind], NULL, 0);
				if (cmd->arg2 > 0) {
					if (optind + 1 + cmd->arg2  > argc) {
						DBG_LOG_ERR("--write-mem/i2c need more argument.\n");
						return -1;
					}
					cmd->buf = (uint8_t*) malloc(cmd->arg2);
					if (cmd->buf) {
						int loop;
						for (loop = 0; loop < cmd->arg2; loop++)
							cmd->buf[loop] = (uint8_t)strtol(argv[optind + 1 + loop], NULL, 0);
					}
				}
			}
			break;
		case 's':
			if (check_path(optarg)) {
				DBG_LOG_ERR("%s is not valid path to save frame\n", optarg);
				option->saveflag = 0;
			} else {
				int len = 0;
				snprintf(option->savepath, sizeof(option->savepath),
					"%s", optarg);
				option->saveflag = 1;
				len = strlen(option->savepath);
				if (len > 0) {
					if ('/' != option->savepath[len - 1]) {
						option->savepath[len] = '/';
						option->savepath[len+1] = '\0';
					}
				}
			}
			break;
		case VAL_OPT_BMP_SAVE:
			option->bmpflag = 1;
			break;
		case VAL_OPT_FRAME_NUMBER:
			option->framenum = (int)strtol(optarg, NULL, 0);
			//printf("%s %d\n", optarg, option->framenum);
			break;
		case VAL_OPT_ENCODE_H264:
			option->h264flag = 1;
			break;
		case VAL_OPT_PREVIEW_NUM:
			option->previewnum = (int)strtol(optarg, NULL, 0);
			break;
		case VAL_OPT_ENCODE_H264_STAB:
			option->h264stab = 1;
			break;
		case VAL_OPT_H264_BPS:
			option->h264bps = (unsigned int)strtol(optarg, NULL, 0);
			break;
		case VAL_OPT_TD_ON:
		case VAL_OPT_TD_OFF:
			cmd = alloc_cmd(c, CMDSIZE, strm_level);
			break;
		case VAL_OPT_DEHAZE:
			cmd = alloc_cmd(c, CMDSIZE, strm_level);
			if (cmd)
				cmd->arg1 = (uint32_t)strtol(optarg, NULL, 0);
			break;
		case VAL_OPT_LDC_ON:
		{
			unsigned int length;
			FILE *pfile = fopen(optarg, "r");
			if (!pfile)
				break;
			fseek(pfile, 0l,SEEK_END);
			length = ftell(pfile);
			rewind(pfile);
			cmd = alloc_cmd(c, CMDSIZE, strm_level);
			if (!cmd) {
				fclose(pfile);
				pfile = NULL;
				break;
			}
			cmd->buf = (uint8_t *)calloc(1, length);
			if (!cmd->buf) {
				free(cmd);
				cmd = NULL;
				fclose(pfile);
				pfile = NULL;
			}
			fread(cmd->buf, 1, length, pfile);
			fclose(pfile);
			pfile = NULL;
			cmd->arg1 = length;
		}
			break;
		case VAL_OPT_LDC_OFF:
		case VAL_OPT_G_LDC:
			cmd = alloc_cmd(c, CMDSIZE, strm_level);
			break;
		case VAL_OPT_CAMERA_DETACH:
		case VAL_OPT_CAMERA_ATTACH:
			cmd = alloc_cmd(c, CMDSIZE, drv_level);
			break;
		case VAL_OPT_FW_VERSION:
			cmd = alloc_cmd(c, CMDSIZE, cmnd_level);
			break;
		case VAL_OPT_H264_GDR:
			option->gdr = (unsigned int)strtol(optarg, NULL, 0);
			break;
		case VAL_OPT_H264_INTRAQPDELTA:
			option->intraQpDelta = (int)strtol(optarg, NULL, 0);
			break;
		case VAL_OPT_H264_ROTATION:
			option->rotation = (unsigned int)strtol(optarg, NULL, 0);
			break;
		case VAL_OPT_LOG_SHOW:
			g_show_mask |= DBG_SHOW_FRAME_TIME;
			break;
		case VAL_OPT_LOG_SHOW_H264:
			g_show_mask |= DBG_SHOW_H264_FRAME;
			break;
		case VAL_OPT_LOG_SHOW_BPS:
			g_show_mask |= DBG_SHOW_H264_BPS;
			break;
		case VAL_OPT_LOG_SHOW_MJPG:
			g_show_mask |= DBG_SHOW_MJPEG;
			break;
		case VAL_OPT_LOG_SHOW_BMP:
			g_show_mask |= DBG_SHOW_BMP;
			break;
		case VAL_OPT_ENCODE_MJPG:
			option->mjpgflag = 1;
			break;
		case VAL_OPT_H264_ROI:
		{
			uint32_t top;
			uint32_t left;
			uint32_t bottom;
			uint32_t right;

			if (optind + 3 > argc) {
				DBG_LOG_ERR("--roi need more argument.\n");
				return -1;
			}
			left = (uint32_t)strtol(optarg, NULL, 0);
			top = (uint32_t)strtol(argv[optind], NULL, 0);
			right = (uint32_t)strtol(argv[optind + 1], NULL, 0);
			bottom = (uint32_t)strtol(argv[optind + 2], NULL, 0);
			if (left < right && top < bottom) {
				option->roi = 1;
				option->left = left;
				option->top = top;
				option->right = right;
				option->bottom = bottom;
			}
		}
			break;
		case ':':
			DBG_LOG_PRINT("required argument : -%c\n", optopt);
			return -1;
		case '?':
			DBG_LOG_PRINT("invalid param: -%c\n", optopt);
			return -1;
		default:
			print_usage();
			break;
		}

		if (cmd != NULL) {
			//DBG_LOG_LOG("Cmd (%d) is added\n", cmd->cmd_id);
			cmd->option = option;
			list_add_tail(&cmd->list, &option->cmd_list);
		}
	}

	if (option->previewnum != 0 && option->previewnum < option->framenum)
		option->previewnum = option->framenum;
	return ret;
}

static void dump_data(uint8_t *data, int len)
{
	int i;
	if (!data)
		return;
	for (i =0; i < len; i++) {
		DBG_LOG_OPT("0x%02x", data[i]);
		if (i % 16 == 15 || i == len - 1)
			DBG_LOG_OPT("\n");
		else
			DBG_LOG_OPT(" ");
	}
}

static int process_cmd(int fd, struct rts_dbg_cmd *cmd)
{
	int ret = 0;
	struct timeval tv_begin;
	struct timeval tv_end;
	struct rts_isp_mem_rw mem_rw;
	struct rts_isp_i2c_rw i2c_rw;

	if (!cmd) {
		DBG_LOG_ERR("cmd is NULL, can not process\n");
		return -1;
	}

	gettimeofday(&tv_begin, F_OK);

	switch (cmd->cmd_id) {
	case VAL_OPT_LIST_CTRL:
		ret = list_video_ctrls(fd);
		break;
	case VAL_OPT_QUERY_CTRL:
		ret = query_video_ctrl(fd, cmd->arg1);
		break;
	case VAL_OPT_GET_CTRL:
		ret = get_video_ctrl(fd, cmd->arg1, NULL);
		break;
	case VAL_OPT_SET_CTRL:
		ret = set_video_ctrl(fd, cmd->arg1, (int32_t)cmd->arg2);
		break;
	case VAL_OPT_LIST_STREAM:
		ret = list_stream_info(fd);
		break;
	case VAL_OPT_SET_FMT:
	{
		uint32_t pixelformat;
		ret = get_pixelformat(fd, cmd->arg1, &pixelformat);
		if (ret)
			break;
		ret = set_fmt(fd, pixelformat, cmd->arg2, cmd->arg3);
		break;
	}
	case VAL_OPT_GET_FMT:
		ret = get_fmt(fd, NULL, NULL, NULL);
		break;
	case VAL_OPT_SET_FPS:
		ret = set_fps(fd, cmd->arg1, cmd->arg2);
		break;
	case VAL_OPT_GET_FPS:
		ret = get_fps(fd, NULL, NULL);
		break;
	case VAL_OPT_PREVIEW:
		ret = preview_func(fd, cmd->option);
		break;
	case VAL_OPT_R_MEM:
		if (cmd->buf) {
			mem_rw.addr = cmd->arg1;
			mem_rw.length = cmd->arg2;
			mem_rw.pdata = cmd->buf;
			ret = rts_isp_read_xmem(fd, &mem_rw);
			if (!ret)
				dump_data(cmd->buf, cmd->arg2);
		} else {
			DBG_LOG_ERR("buf alloc failed or len =0\n");
			ret = -1;
		}
		break;
	case VAL_OPT_W_MEM:
		if (cmd->buf) {
			dump_data(cmd->buf, cmd->arg2);
			mem_rw.addr = cmd->arg1;
			mem_rw.length = cmd->arg2;
			mem_rw.pdata = cmd->buf;
			ret = rts_isp_write_xmem(fd, &mem_rw);
		} else {
			DBG_LOG_ERR("buf alloc failed or len =0\n");
			ret = -1;
		}
		break;
	case VAL_OPT_R_I2C:
		if (cmd->buf) {
			i2c_rw.mode = rts_isp_i2c_mode_general;
			i2c_rw.slave_id = cmd->arg1;
			i2c_rw.len = cmd->arg2;
			ret = rts_isp_read_i2c(fd, &i2c_rw);
			if (!ret)
				dump_data(i2c_rw.data, i2c_rw.len);
		} else {
			DBG_LOG_ERR("buf alloc failed or len =0\n");
			ret = -1;
		}
		break;
	case VAL_OPT_W_I2C:
		if (cmd->buf && cmd->arg2 <= 8) {
			dump_data(cmd->buf, cmd->arg2);
			i2c_rw.mode = rts_isp_i2c_mode_general;
			i2c_rw.slave_id = cmd->arg1;
			i2c_rw.len = cmd->arg2;
			memcpy(i2c_rw.data, cmd->buf, cmd->arg2);
			ret = rts_isp_write_i2c(fd, &i2c_rw);
		} else {
			DBG_LOG_ERR("buf alloc failed or len<%d> invalid\n",
					cmd->arg2);
			ret = -1;
		}
		break;
	case VAL_OPT_TD_ON:
		ret = switch_td(fd, 1);
		break;
	case VAL_OPT_TD_OFF:
		ret = switch_td(fd, 0);
		break;
	case VAL_OPT_DEHAZE:
		if (cmd->arg1)
			ret = rts_isp_set_dehaze(fd, RTS_TRUE);
		else
			ret = rts_isp_set_dehaze(fd, RTS_FALSE);
		break;
	case VAL_OPT_LDC_ON:
		ret = rts_isp_enable_ldc(fd, cmd->buf, cmd->arg1);
		break;
	case VAL_OPT_LDC_OFF:
		ret = rts_isp_disable_ldc(fd);
		break;
	case VAL_OPT_G_LDC:
		DBG_LOG_LOG("LDC %s\n", rts_isp_get_ldc_state(fd)?"on":"off");
		break;
	case VAL_OPT_FW_VERSION:
	{
		struct rts_isp_fw_version_t fw_version;
		ret = rts_isp_get_fw_version(fd, &fw_version);
		if (!ret) {
			DBG_LOG_OPT("Fireware Version:\n");
			DBG_LOG_OPT("header = %x\n", fw_version.header);
			DBG_LOG_OPT("len = %x\n", fw_version.len);
			DBG_LOG_OPT("magictag = %08x\n", fw_version.magictag);
			DBG_LOG_OPT("ic_name = %04x\n", fw_version.ic_name);
			DBG_LOG_OPT("vid = %04x\n", fw_version.vid);
			DBG_LOG_OPT("pid = %04x\n", fw_version.pid);
			DBG_LOG_OPT("fw_ver = %08x\n", fw_version.fw_ver);
			DBG_LOG_OPT("cus_ver = %08x\n", fw_version.cus_ver);
		}
	}
		break;
	default:
		DBG_LOG_ERR("Unknown cmd (%d)\n", cmd->cmd_id);
		ret = -1;
	}

	gettimeofday(&tv_end, F_OK);

	if (ret)
		DBG_LOG_ERR("Cmd (%s) fail. Use time:%d\n",
			get_cmd_name(cmd->cmd_id),
			rts_calc_timeval(tv_begin, tv_end));
	else
		DBG_LOG_LOG("Cmd (%s) success. Use time:%d\n",
			get_cmd_name(cmd->cmd_id),
			rts_calc_timeval(tv_begin, tv_end));

	return ret;
}

int main(int argc, char *argv[])
{
	struct rts_dbg_option option;
	struct dbg_h264_parm h264_parm;
	struct rts_dbg_cmd *cmd, *tmp;
	int fd = -1;
	int ret = 0;
	int cmd_num = 0;
	int drv_cmd = 0;

	init_dft_h264enc_parm(&h264_parm);

	realsil_log_open();
	memset(&option, 0 , sizeof(option));

	signal(SIGINT, Termination);
	signal(SIGTERM, Termination);
	signal(SIGQUIT, Termination);
	signal(SIGTSTP, Termination);
	signal(SIGPIPE, SIG_IGN);

	INIT_LIST_HEAD(&option.cmd_list);
	option.priv = &h264_parm;
	ret = parse_option(argc, argv, &option);
	if (ret < 0) {
		DBG_LOG_ERR("parse option failed\n");
		goto exit;
	}

	list_for_each_entry(cmd, &option.cmd_list, list) {
		cmd_num++;
		if (drv_level == cmd->level) {
			drv_cmd = cmd->cmd_id;
			break;
		}
	}
	if (drv_cmd > 0) {
		if (drv_cmd == VAL_OPT_CAMERA_DETACH)
			ret = rts_isp_soc_detach();
		else if (drv_cmd == VAL_OPT_CAMERA_ATTACH)
			ret = rts_isp_soc_attach();
		else
				ret = -1;
		DBG_LOG_LOG("execute drv cmd result = %d\n", ret);
		goto exit;
	}
	if (cmd_num == 0) {
		print_usage();
		return ret;
	}

	if (strstr(option.devname, "video"))
		fd = rts_v4l2_open(option.devname, O_RDWR | O_NONBLOCK);
	else
		fd = open(option.devname, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		DBG_LOG_ERR("open %s failed\n", option.devname);
		goto exit;
	}

	/*deal with ctrl*/
	list_for_each_entry(cmd, &option.cmd_list, list) {
		if (ctrl_level == cmd->level)
			process_cmd(fd, cmd);
	}

	/*deal with debug cmd*/
	list_for_each_entry(cmd, &option.cmd_list, list) {
		if (cmnd_level == cmd->level)
			process_cmd(fd, cmd);
	}

	/*deal with stream setting*/
	list_for_each_entry(cmd, &option.cmd_list, list) {
		if (strm_level == cmd->level)
			process_cmd(fd, cmd);
	}

	/*final, deal with preview*/
	list_for_each_entry(cmd, &option.cmd_list, list) {
		if (prvw_level == cmd->level) {
			process_cmd(fd, cmd);
		}
	}

exit:
	if (fd > 0){
		if (strstr(option.devname, "video"))
			rts_v4l2_close(fd);
		else
			close(fd);
	}
	fd = -1;
	list_for_each_entry_safe(cmd, tmp, &option.cmd_list, list) {
		list_del(&cmd->list);
		if(cmd->buf != NULL) {
			free(cmd->buf);
			cmd->buf = NULL;
		}
		free(cmd);
	}
	realsil_log_close();
	return 0;
}
