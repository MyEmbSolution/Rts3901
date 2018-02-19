#ifndef _RTS_DBG_CTRL_H_
#define _RTS_DBG_CTRL_H_
#include <stdint.h>

int list_video_ctrls(int fd);
int query_video_ctrl(int fd, uint32_t id);
int get_video_ctrl(int fd, uint32_t id, int32_t *value);
int set_video_ctrl(int fd, uint32_t id, int32_t value);

#endif
