/*
 * Realtek Semiconductor Corp.
 *
 * rtstream.c
 *
 * Copyright (C) 2016      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include "linux/rts_rtstream.h"
#include "rts_camera.h"

#define RTS_RTSTREAM_DEV_NAME		"rtstream"

struct ctrl_item {
	struct list_head list;

	struct rtstream_ctrl ctrl;
};

struct unit_item {
	struct list_head list;

	struct rtstream_unit_info info;
};

struct stream_item {
	struct list_head list;

	int stream_no;
	int unit_count;
	struct list_head units;
	struct list_head ctrls;
};

struct rtstream_manager {
	struct list_head streams;

	struct rtscam_ge_device *rdev;
};

static struct rtstream_manager manager;

static struct stream_item *find_stream(struct rtstream_manager *rtstreamer,
		int stream_no)
{
	struct stream_item *stream;

	if (!stream_no)
		return NULL;

	list_for_each_entry(stream, &rtstreamer->streams, list) {
		if (stream->stream_no == stream_no) {
			return stream;
		}
	}

	return NULL;
}

static int generate_stream_no(struct rtstream_manager *rtstreamer)
{
	int stream_no = 0;

	while (find_stream(rtstreamer, ++stream_no)) {
		udelay(1);
	}

	return stream_no;
}

static void clear_stream(struct stream_item *stream)
{
	struct unit_item *unit, *tmp;

	list_for_each_entry_safe(unit, tmp, &stream->units, list) {
		list_del_init(&unit->list);
		vfree(unit);
	}
}

static void clear_ctrls(struct stream_item *stream)
{
	struct ctrl_item *ctrl, *tmp;

	list_for_each_entry_safe(ctrl, tmp, &stream->ctrls, list) {
		list_del_init(&ctrl->list);
		vfree(ctrl);
	}
}

static int set_stream_info(struct rtstream_manager *rtstreamer,
		struct rtstream_info *info)
{

	struct stream_item *stream = NULL;
	int i;
	int ret = 0;

	if (!info)
		return -EINVAL;

	if (info->no) {
		stream = find_stream(rtstreamer, info->no);
		if (stream)
			clear_stream(stream);
		else
			return -EINVAL;
	} else {
		stream = vzalloc(sizeof(*stream));
		if (!stream)
			return -EINVAL;

		INIT_LIST_HEAD(&stream->units);
		INIT_LIST_HEAD(&stream->ctrls);
		stream->stream_no = generate_stream_no(rtstreamer);
	}

	for (i = 0; i < info->unit_count; i++) {
		struct rtstream_unit_info *uinfo = info->units + i;
		struct unit_item *item = vzalloc(sizeof(*item));

		if (!item) {
			rtsprintk(RTS_TRACE_ERROR,
					"malloc unit item[%d] fail\n",
					uinfo->id);
			ret = -EINVAL;
			break;
		}

		copy_from_user(&item->info, (void __user *)uinfo, sizeof(*uinfo));
		list_add_tail(&item->list, &stream->units);
	}

	if (ret) {
		clear_stream(stream);
		if (!info->no)
			vfree(stream);
		return ret;
	}

	stream->unit_count = info->unit_count;
	if (!find_stream(rtstreamer, stream->stream_no))
		list_add_tail(&stream->list, &rtstreamer->streams);
	info->no = stream->stream_no;

	return 0;
}

static int delete_stream(struct rtstream_manager *rtstreamer, int stream_no)
{
	struct stream_item *stream;

	stream = find_stream(rtstreamer, stream_no);
	if (!stream)
		return 0;

	list_del_init(&stream->list);
	clear_stream(stream);
	clear_ctrls(stream);

	vfree(stream);

	return 0;
}

static int enum_stream(struct rtstream_manager *rtstreamer,
		struct rtstream_stream_desc *desc)
{
	struct stream_item *stream;
	int i = 0;

	if (!desc)
		return -EINVAL;
	if (desc->index < 0)
		desc->index = 0;

	list_for_each_entry(stream, &rtstreamer->streams, list) {
		if (desc->index == i++) {
			desc->stream_no = stream->stream_no;
			return 0;
		}
	}

	return -EINVAL;
}

static int enum_unit(struct rtstream_manager *rtstreamer,
		struct rtstream_unit_desc *desc)
{
	struct stream_item *stream;
	struct unit_item *unit;
	int i = 0;

	if (!desc)
		return -EINVAL;
	if (desc->index < 0)
		desc->index = 0;

	stream = find_stream(rtstreamer, desc->stream_no);
	if (!stream)
		return -EINVAL;

	list_for_each_entry(unit, &stream->units, list) {
		if (desc->index == i++) {
			memcpy(&desc->unit, &unit->info,
					sizeof(struct rtstream_unit_info));
			return 0;
		}
	}

	return -EINVAL;
}

static struct ctrl_item *find_ctrl(struct stream_item *stream, char *name)
{
	struct ctrl_item *ctrl;

	list_for_each_entry(ctrl, &stream->ctrls, list) {
		if (0 ==strcmp(ctrl->ctrl.name, name))
			return ctrl;
	}

	return NULL;
}

static int get_ctrls(struct rtstream_manager *rtstreamer,
		struct rtstream_ctrls *ctrls)
{
	struct stream_item *stream;
	int i;

	if (!ctrls)
		return -EINVAL;

	stream = find_stream(rtstreamer, ctrls->stream_no);
	if (!stream)
		return -EINVAL;

	for (i = 0; i < ctrls->count; i++) {
		struct rtstream_ctrl *ctrl = ctrls->ctrls + i;
		struct ctrl_item *item = find_ctrl(stream, ctrl->name);
		if (!item)
			return -EINVAL;
		ctrl->value = item->ctrl.value;
	}

	return 0;
}

static int set_ctrls(struct rtstream_manager *rtstreamer,
		struct rtstream_ctrls *ctrls)
{
	struct stream_item *stream;
	int i;

	if (!ctrls)
		return -EINVAL;

	stream = find_stream(rtstreamer, ctrls->stream_no);
	if (!stream)
		return -EINVAL;

	for (i = 0; i < ctrls->count; i++) {
		struct rtstream_ctrl *ctrl = ctrls->ctrls + i;
		struct ctrl_item *item = NULL;

		if (strlen(ctrl->name) == 0)
			continue;

		item = find_ctrl(stream, ctrl->name);
		if (item) {
			item->ctrl.value = ctrl->value;
		} else {
			item = vzalloc(sizeof(*item));
			strcpy(item->ctrl.name, ctrl->name);
			item->ctrl.value = ctrl->value;
			list_add_tail(&item->list, &stream->ctrls);
		}
	}

	return 0;
}

static int rtstream_open(struct file *filp)
{
	struct rtscam_ge_device *gdev = rtscam_devdata(filp);
	struct rtstream_manager *rtstreamer = rtscam_ge_get_drvdata(gdev);

	filp->private_data = rtstreamer;

	return 0;
}

static int rtstream_close(struct file *filp)
{
	struct rtstream_manager *rtstreamer = filp->private_data;

	filp->private_data = NULL;

	if (!rtstreamer)
		return -EINVAL;

	return 0;
}

static long rtstream_do_ioctl(struct file *filp, unsigned int cmd,
		void *arg)
{
	struct rtstream_manager *rtstreamer = filp->private_data;
	int ret = 0;

	if (!rtstreamer)
		return -EINVAL;

	if (_IOC_TYPE(cmd) != RTS_RTSTREAM_IOC_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case RTSTREAM_IOC_S_STREAMINFO:
		ret = set_stream_info(rtstreamer, arg);
		break;
	case RTSTREAM_IOC_DELETE_STREAM:
		ret = delete_stream(rtstreamer, *(int *)arg);
		break;
	case RTSTREAM_IOC_ENUM_STREAM:
		ret = enum_stream(rtstreamer, arg);
		break;
	case RTSTREAM_IOC_ENUM_UNIT:
		ret = enum_unit(rtstreamer, arg);
		break;
	case RTSTREAM_IOC_S_CTRLS:
		ret = set_ctrls(rtstreamer, arg);
		break;
	case RTSTREAM_IOC_G_CTRLS:
		ret = get_ctrls(rtstreamer, arg);
		break;
	default:
		rtsprintk(RTS_TRACE_ERROR,
				"unknown[rtstream] ioctl 0x%08x, '%c' 0x%x\n",
				cmd, _IOC_TYPE(cmd), _IOC_NR(cmd));
		ret = -ENOTTY;
		break;
	}

	return ret;
}

static long rtstream_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	return rtscam_usercopy(filp, cmd, arg, rtstream_do_ioctl);
}

static struct rtscam_ge_file_operations rtstream_fops = {
	.owner		= THIS_MODULE,
	.open		= rtstream_open,
	.release	= rtstream_close,
	.unlocked_ioctl	= rtstream_ioctl,
};

static ssize_t show_streaminfo(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct rtscam_ge_device *gdev = container_of(dev,
			struct rtscam_ge_device, dev);
	struct rtstream_manager *rtstreamer = rtscam_ge_get_drvdata(gdev);
	struct stream_item *stream;
	struct unit_item *unit;
	struct ctrl_item *ctrl;
	int num = 0;

	list_for_each_entry(stream, &rtstreamer->streams, list) {
		num += scnprintf(buf + num, PAGE_SIZE,
				"[%d] stream contain %d units\n",
				stream->stream_no,
				stream->unit_count);
		list_for_each_entry(unit, &stream->units, list) {
			num += scnprintf(buf + num, PAGE_SIZE,
					"\t[unit] %c%c%c%c format = %d %d %d %d %d %d %d %d\n",
					unit->info.id & 0xff,
					(unit->info.id >> 8) & 0xff,
					(unit->info.id >> 16) & 0xff,
					(unit->info.id >> 24) & 0xff,
					unit->info.format,
					unit->info.video.width,
					unit->info.video.height,
					unit->info.video.numerator,
					unit->info.video.denominator,
					unit->info.isp_id,
					unit->info.rotation,
					unit->info.videostab);
		}
		list_for_each_entry(ctrl, &stream->ctrls, list) {
			num += scnprintf(buf + num, PAGE_SIZE,
					"\t\t%s\t = %d\n", ctrl->ctrl.name,
					ctrl->ctrl.value);
		}
	}

	return num;
}
DEVICE_ATTR(streaminfo, S_IRUGO, show_streaminfo, NULL);

static int __create_device(struct rtstream_manager *rtstreamer)
{
	struct rtscam_ge_device *gdev;
	int ret;

	if (rtstreamer->rdev)
		return 0;

	gdev = rtscam_ge_device_alloc();
	if (!gdev)
		return -ENOMEM;

	strlcpy(gdev->name, RTS_RTSTREAM_DEV_NAME, sizeof(gdev->name));
	gdev->release = rtscam_ge_device_release;
	gdev->fops = &rtstream_fops;

	rtscam_ge_set_drvdata(gdev, rtstreamer);
	ret = rtscam_ge_register_device(gdev);
	if (ret) {
		rtscam_ge_device_release(gdev);
		return ret;
	}

	rtstreamer->rdev = gdev;
	device_create_file(&gdev->dev, &dev_attr_streaminfo);

	return 0;
}

static void __remove_device(struct rtstream_manager *rtstreamer)
{
	struct rtscam_ge_device *gdev;

	if (!rtstreamer->rdev)
		return;

	gdev = rtstreamer->rdev;

	device_remove_file(&gdev->dev, &dev_attr_streaminfo);

	rtscam_ge_unregister_device(gdev);
}

static int __init rtstream_init(void)
{
	rtsprintk(RTS_TRACE_INFO, "%s\n", __func__);

	INIT_LIST_HEAD(&manager.streams);

	__create_device(&manager);

	return 0;
}

static void __exit rtstream_exit(void)
{
	struct stream_item *stream, *tmp;

	rtsprintk(RTS_TRACE_INFO, "%s\n", __func__);

	__remove_device(&manager);

	list_for_each_entry_safe(stream, tmp, &manager.streams, list) {
		list_del_init(&stream->list);
		clear_stream(stream);
		clear_ctrls(stream);
		vfree(stream);
	}
}

module_init(rtstream_init);
module_exit(rtstream_exit);

MODULE_DESCRIPTION("Realsil rtstream driver");
MODULE_AUTHOR("Ming Qian <ming_qian@realsil.com.cn>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.0.1");

