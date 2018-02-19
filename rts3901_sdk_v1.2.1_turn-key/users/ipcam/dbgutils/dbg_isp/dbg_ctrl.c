#include <stdlib.h>
#include <stdio.h>
#include <rtsv4l2.h>
#include "dbg_priv.h"
#include "dbg_log.h"
#include "dbg_ctrl.h"

static void __query_ctrl_info(struct v4l2_queryctrl *queryctrl)
{
	DBG_LOG_PRINT("Ctrl id:0x%08x\n                  "
		"type:0x%08x\n                  "
		"name:%s\n                  min:%d\n                  "
		"max:%d\n                  step:%d\n                  "
		"default:%d\n                  flags:0x%08x\n",
                queryctrl->id,
                queryctrl->type,
                (char*)queryctrl->name,
                queryctrl->minimum,
                queryctrl->maximum,
                queryctrl->step,
                queryctrl->default_value,
                queryctrl->flags);
}

int list_video_ctrls(int fd)
{
	struct v4l2_queryctrl queryctrl;
	queryctrl.id = 0;
	while (0 == rts_v4l2_query_next_ctrl(fd, &queryctrl))
		__query_ctrl_info(&queryctrl);
	return 0;
}

int query_video_ctrl(int fd, uint32_t id)
{
	struct v4l2_queryctrl queryctrl;
	int ret;

	queryctrl.id = id;
	ret = rts_v4l2_queryctrl(fd, &queryctrl);
	if (ret)
		DBG_LOG_ERR("Query ctrl(0x%x) failed\n", id);
	else
		__query_ctrl_info(&queryctrl);
	return ret;
}

int check_video_ctrl(int fd, uint32_t id)
{
	struct v4l2_queryctrl queryctrl;
	queryctrl.id = id;
	return rts_v4l2_queryctrl(fd, &queryctrl);
}

int get_video_ctrl(int fd, uint32_t id, int32_t *value)
{
	struct v4l2_control ctrl;
	int ret;

	ret = check_video_ctrl(fd, id);
	if (ret) {
		DBG_LOG_ERR("ctrl (0x%x) is invalid, get failed\n", id);
		return ret;
	}
	ctrl.id = id;
	ret = rts_v4l2_get_ctrl(fd, &ctrl);
	if (ret) {
		DBG_LOG_ERR("get ctrl (0x%x) failed : %d\n", id, ret);
		return ret;
	}
	DBG_LOG_PRINT("Ctrl id = 0x%x, value = %d\n", id, ctrl.value);
	if (value)
		*value = ctrl.value;
	return 0;
}

int set_video_ctrl(int fd, uint32_t id, int32_t value)
{
	struct v4l2_control ctrl;
	int ret;

	ret = check_video_ctrl(fd, id);
	if (ret) {
		DBG_LOG_ERR("ctrl (0x%x) is invalid, set failed\n", id);
		return ret;
	}
	ctrl.id = id;
	ctrl.value = value;
	ret = rts_v4l2_set_ctrl(fd, &ctrl);
	if (ret)
		DBG_LOG_ERR("set ctrl(id = 0x%x, value = %d) failed : %d\n",
		id, value, ret);
	else
		DBG_LOG_LOG("set ctrl(id = 0x%x, value = %d) succeed\n",
		id, value);
	return ret;
}

