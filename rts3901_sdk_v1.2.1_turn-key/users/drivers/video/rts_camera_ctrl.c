/*
 * Realtek Semiconductor Corp.
 *
 * rts_camera_ctrl.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>

#include "rts_camera_priv.h"

__s32 rtscam_get_le_value(struct rtscam_video_ctrl_mapping *mapping,
		__u8 query, const __u8 *data)
{
	int bits = mapping->size;
	int offset = mapping->offset;
	__s32 value = 0;
	__u8 mask;

	data += offset / 8;
	offset &= 7;
	mask = ((1LL << bits) - 1) << offset;

	for (; bits > 0; data++) {
		__u8 byte = *data & mask;
		value |= offset > 0 ? (byte >> offset) : (byte << (-offset));
		bits -= 8 - (offset > 0 ? offset : 0);
		offset -= 8;
		mask = (1 << bits) - 1;
	}

	if (mapping->data_type == RTS_CTRL_DATA_TYPE_SIGNED)
		value |= -(value & (1 << (mapping->size - 1)));

	return value;
}

void rtscam_set_le_value(struct rtscam_video_ctrl_mapping *mapping,
		__s32 value, __u8 *data)
{
	int bits = mapping->size;
	int offset = mapping->offset;
	__u8 mask;

	if (mapping->v4l2_type == V4L2_CTRL_TYPE_BUTTON)
		value = -1;

	data += offset / 8;
	offset &= 7;

	for (; bits > 0; data++) {
		mask = ((1LL << bits) - 1) << offset;
		*data = (*data & ~mask) | ((value << offset) & mask);
		value >>= offset ? offset : 8;
		bits -= 8 - offset;
		offset = 0;
	}
}

int rtscam_ctrl_add_info(struct rtscam_video_ctrl *ctrl,
		struct rtscam_video_ctrl_info *info)
{
	ctrl->info = *info;
	INIT_LIST_HEAD(&ctrl->info.mappings);

	/*Allocate an array to save control values (cur, def, max, etc.)*/
	ctrl->ctrl_data = kzalloc(ctrl->info.size * RTS_CTRL_DATA_LAST + 1,
			GFP_KERNEL);
	if (NULL == ctrl->ctrl_data) {
		rtsprintk(RTS_TRACE_ERROR, "%s:kzalloc fail\n", __func__);
		return -ENOMEM;
	}

	ctrl->initialized = 1;

	rtsprintk(RTS_TRACE_CTRL, "Addded control %d/%u\n",
			ctrl->info.unit, ctrl->info.selector);

	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_ctrl_add_info);

void rtscam_ctrl_clr_info(struct rtscam_video_ctrl *ctrl)
{
	kfree(ctrl->ctrl_data);
	ctrl->ctrl_data = NULL;
	ctrl->initialized = 0;
}
EXPORT_SYMBOL_GPL(rtscam_ctrl_clr_info);

int rtscam_ctrl_add_mapping(struct rtscam_video_ctrl *ctrl,
		struct rtscam_video_ctrl_mapping *mapping)
{
	struct rtscam_video_ctrl_mapping *map;
	unsigned int size;

	/* Most mappings come from static kernel data and need to be duplicated.
	 * Mappings that come from userspace will be unnecessarily duplicated,
	 * this could be optimized.
	 * */

	map = kmemdup(mapping, sizeof(*mapping), GFP_KERNEL);
	if (NULL == map) {
		rtsprintk(RTS_TRACE_ERROR, "%s kmemdup fail\n", __func__);
		return -ENOMEM;
	}

	if (mapping->menu_info && mapping->menu_count > 0) {
		size = sizeof(*mapping->menu_info) * mapping->menu_count;
		map->menu_info = kmemdup(mapping->menu_info, size, GFP_KERNEL);
		if (NULL == map->menu_info) {
			kfree(map);
			return -ENOMEM;
		}
	} else {
		map->menu_info = NULL;
		map->menu_count = 0;
	}

	if (NULL == map->get)
		map->get = rtscam_get_le_value;
	if (NULL == map->set)
		map->set = rtscam_set_le_value;
	list_add_tail(&map->list, &ctrl->info.mappings);

	rtsprintk(RTS_TRACE_CTRL, "Adding mapping '%s' to ctrl %d/%u.\n",
			map->name, ctrl->info.unit, ctrl->info.selector);

	return 0;
}
EXPORT_SYMBOL_GPL(rtscam_ctrl_add_mapping);

void rtscam_ctrl_clr_mapping(struct rtscam_video_ctrl *ctrl)
{
	struct rtscam_video_ctrl_mapping *mapping, *tmp;

	list_for_each_entry_safe(mapping, tmp, &ctrl->info.mappings, list) {
		list_del(&mapping->list);
		if (mapping->menu_info)
			kfree(mapping->menu_info);
		kfree(mapping);
	}
}
EXPORT_SYMBOL_GPL(rtscam_ctrl_clr_mapping);

static inline __u8 *rtscam_ctrl_data(struct rtscam_video_ctrl *ctrl, int id)
{
	return ctrl->ctrl_data + id * ctrl->info.size;
}

static int rtscam_query_ctrl(struct rtscam_video_device *icd,
		struct rtscam_video_ctrl_info *info, __u8 query, __u8 *data)
{
	return rtscam_call_video_op(icd, query_ctrl, icd, info->unit,
			query, info->selector, info->size, data);
}

static int rtscam_ctrl_populate_cache(struct rtscam_video_device *icd,
		struct rtscam_video_ctrl *ctrl)
{
	int ret;
	struct rtscam_video_ctrl_info *info = &ctrl->info;

	if (info->flags & RTS_CTRL_FLAG_GET_DEF) {
		ret = rtscam_query_ctrl(icd, info, RTS_CTRL_GET_DEF,
				rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_DEF));
		if (ret < 0)
			return ret;
	}
	if (info->flags & RTS_CTRL_FLAG_GET_MIN) {
		ret = rtscam_query_ctrl(icd, info, RTS_CTRL_GET_MIN,
				rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_MIN));
		if (ret < 0)
			return ret;
	}
	if (info->flags & RTS_CTRL_FLAG_GET_MAX) {
		ret = rtscam_query_ctrl(icd, info, RTS_CTRL_GET_MAX,
				rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_MAX));
		if (ret < 0)
			return ret;
	}
	if (info->flags & RTS_CTRL_FLAG_GET_RES) {
		ret = rtscam_query_ctrl(icd, info, RTS_CTRL_GET_RES,
				rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_RES));
		if (ret < 0)
			return ret;
	}

	ctrl->cached = 1;
	return 0;
}

struct rtscam_video_ctrl *rtscam_find_ctrl(struct rtscam_video_device *icd,
		__u32 v4l2_id, struct rtscam_video_ctrl_mapping **mapping)
{
	struct rtscam_video_ctrl *ctrl = NULL;
	struct rtscam_video_ctrl *control = NULL;
	struct rtscam_video_ctrl_mapping *map = NULL;
	int next = v4l2_id & V4L2_CTRL_FLAG_NEXT_CTRL;

	*mapping = NULL;

	v4l2_id &= V4L2_CTRL_ID_MASK;

	list_for_each_entry(ctrl, &icd->controls, list) {
		if (!ctrl->initialized)
			continue;

		list_for_each_entry(map, &ctrl->info.mappings, list) {
			if ((map->id == v4l2_id) && !next) {
				control = ctrl;
				*mapping = map;
				break;
			}
			if ((*mapping == NULL || (*mapping)->id > map->id) &&
					(map->id > v4l2_id) && next) {
				control = ctrl;
				*mapping = map;
			}
		}
		if (control && !next)
			break;
	}

	if (control == NULL && !next)
		rtsprintk(RTS_TRACE_ERROR, "Ctrl 0x%08x not found\n", v4l2_id);

	if (NULL != *mapping) {
		map = *mapping;
		if (NULL == map->get)
			map->get = rtscam_get_le_value;
		if (NULL == map->set)
			map->set = rtscam_set_le_value;
	}

	return control;
}

int __rtscam_query_v4l2_ctrl(struct rtscam_video_device *icd,
		struct rtscam_video_ctrl *ctrl,
		struct rtscam_video_ctrl_mapping *mapping,
		struct v4l2_queryctrl *v4l2_ctrl)
{
	struct rtscam_video_ctrl_menu *menu;
	unsigned int i;

	memset(v4l2_ctrl, 0, sizeof(*v4l2_ctrl));
	v4l2_ctrl->id = mapping->id;
	v4l2_ctrl->type = mapping->v4l2_type;
	strlcpy(v4l2_ctrl->name, mapping->name, sizeof(v4l2_ctrl->name));
	v4l2_ctrl->flags = 0;

	if (!(ctrl->info.flags & RTS_CTRL_FLAG_GET_CUR))
		v4l2_ctrl->flags |= V4L2_CTRL_FLAG_WRITE_ONLY;
	if (!(ctrl->info.flags & RTS_CTRL_FLAG_SET_CUR))
		v4l2_ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	if (!ctrl->cached) {
		int ret = rtscam_ctrl_populate_cache(icd, ctrl);
		if (ret < 0)
			return ret;
	}

	if (ctrl->info.flags & RTS_CTRL_FLAG_GET_DEF) {
		v4l2_ctrl->default_value = mapping->get(mapping,
				RTS_CTRL_GET_DEF,
				rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_DEF));
	}

	switch (mapping->v4l2_type) {
	case V4L2_CTRL_TYPE_MENU:
		v4l2_ctrl->minimum = 0;
		v4l2_ctrl->maximum = mapping->menu_count - 1;
		v4l2_ctrl->step = 1;

		menu = mapping->menu_info;
		for (i = 0; i < mapping->menu_count; ++i, ++menu) {
			if (menu->value == v4l2_ctrl->default_value) {
				v4l2_ctrl->default_value = i;
				break;
			}
		}
		return 0;
	case V4L2_CTRL_TYPE_BOOLEAN:
		v4l2_ctrl->minimum = 0;
		v4l2_ctrl->maximum = 1;
		v4l2_ctrl->step = 1;
		return 0;
	case V4L2_CTRL_TYPE_BUTTON:
		v4l2_ctrl->minimum = 0;
		v4l2_ctrl->maximum = 0;
		v4l2_ctrl->step = 0;
		return 0;
	default:
		break;
	}

	if (ctrl->info.flags & RTS_CTRL_FLAG_GET_MIN) {
		v4l2_ctrl->minimum = mapping->get(mapping, RTS_CTRL_GET_MIN,
				rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_MIN));
	}

	if (ctrl->info.flags & RTS_CTRL_FLAG_GET_MAX) {
		v4l2_ctrl->maximum = mapping->get(mapping, RTS_CTRL_GET_MAX,
				rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_MAX));
	}

	if (ctrl->info.flags & RTS_CTRL_FLAG_GET_RES) {
		v4l2_ctrl->step = mapping->get(mapping, RTS_CTRL_GET_RES,
				rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_RES));
	}

	return 0;
}

int rtscam_query_v4l2_ctrl(struct rtscam_video_device *icd,
		struct v4l2_queryctrl *v4l2_ctrl)
{
	struct rtscam_video_ctrl *ctrl = NULL;
	struct rtscam_video_ctrl_mapping *mapping = NULL;
	int ret = 0;

	ret = mutex_lock_interruptible(&icd->ctrl_lock);
	if (ret < 0) {
		rtsprintk(RTS_TRACE_ERROR, "acquire ctrl lock fail\n");
		return -ERESTARTSYS;
	}

	ctrl = rtscam_find_ctrl(icd, v4l2_ctrl->id, &mapping);
	if (NULL == ctrl) {
		ret = -EINVAL;
		goto done;
	}
	ret = __rtscam_query_v4l2_ctrl(icd, ctrl, mapping, v4l2_ctrl);

done:
	mutex_unlock(&icd->ctrl_lock);
	return ret;
}

static int rtscam_ctrl_begin(struct rtscam_video_device *icd)
{
	return mutex_lock_interruptible(&icd->ctrl_lock) ? -ERESTARTSYS : 0;
}

static void rtscam_ctrl_end(struct rtscam_video_device *icd)
{
	mutex_unlock(&icd->ctrl_lock);
}

static int __rtscam_ctrl_get(struct rtscam_video_device *icd,
		struct rtscam_video_ctrl *ctrl,
		struct rtscam_video_ctrl_mapping *mapping,
		s32 *value)
{
	struct rtscam_video_ctrl_menu *menu = NULL;
	unsigned int i;
	int ret;

	if ((ctrl->info.flags & RTS_CTRL_FLAG_GET_CUR) == 0)
		return -EACCES;

	ctrl->loaded = 0;
	if (!ctrl->loaded) {
		ret = rtscam_query_ctrl(icd, &ctrl->info, RTS_CTRL_GET_CUR,
				rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_CURRENT));
		if (ret < 0)
			return ret;

		ctrl->loaded = 1;
	}

	*value = mapping->get(mapping, RTS_CTRL_GET_CUR,
			rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_CURRENT));

	if (mapping->v4l2_type == V4L2_CTRL_TYPE_MENU) {
		menu = mapping->menu_info;
		for (i = 0; i < mapping->menu_count; ++i, ++menu) {
			if (menu->value == *value) {
				*value = i;
				break;
			}
		}
	}

	return 0;
}

int rtscam_ctrl_get(struct rtscam_video_device *icd,
		struct v4l2_ext_control *xctrl)
{
	struct rtscam_video_ctrl *ctrl = NULL;
	struct rtscam_video_ctrl_mapping *mapping = NULL;

	ctrl = rtscam_find_ctrl(icd, xctrl->id, &mapping);
	if (NULL == ctrl)
		return -EINVAL;

	return __rtscam_ctrl_get(icd, ctrl, mapping, &xctrl->value);
}

static int __rtscam_ctrl_commit(struct rtscam_video_device *icd, int rollback)
{
	struct rtscam_video_ctrl *ctrl;
	int ret = 0;

	list_for_each_entry(ctrl, &icd->controls, list) {
		if (!ctrl->initialized)
			continue;

		if (ctrl->info.flags & RTS_CTRL_FLAG_AUTO_UPDATE ||
				!(ctrl->info.flags & RTS_CTRL_FLAG_GET_CUR))
			ctrl->loaded = 0;

		if (!ctrl->dirty)
			continue;

		if (!rollback)
			ret = rtscam_query_ctrl(icd, &ctrl->info,
				RTS_CTRL_SET_CUR,
				rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_CURRENT));
		else
			ret = 0;

		if (rollback || ret < 0)
			memcpy(rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_CURRENT),
				rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_BACKUP),
				ctrl->info.size);

		ctrl->dirty = 0;

		if (ret < 0)
			goto done;
	}

done:
	rtscam_ctrl_end(icd);
	return ret;
}

static inline int rtscam_ctrl_commit(struct rtscam_video_device *icd)
{
	return __rtscam_ctrl_commit(icd, 0);
}

static inline int rtscam_ctrl_rollback(struct rtscam_video_device *icd)
{
	return __rtscam_ctrl_commit(icd, 1);
}

static int rtscam_ctrl_set(struct rtscam_video_device *icd,
		struct v4l2_ext_control *xctrl)
{
	struct rtscam_video_ctrl *ctrl = NULL;
	struct rtscam_video_ctrl_mapping *mapping = NULL;
	s32 value;
	u32 step;
	s32 min;
	s32 max;
	int ret;

	ctrl = rtscam_find_ctrl(icd, xctrl->id, &mapping);
	if (NULL == ctrl)
		return -EINVAL;

	if (!(ctrl->info.flags & RTS_CTRL_FLAG_SET_CUR))
		return -EACCES;

	/*Clamp out of range values.*/
	switch (mapping->v4l2_type) {
	case V4L2_CTRL_TYPE_INTEGER:
		if (!ctrl->cached) {
			ret = rtscam_ctrl_populate_cache(icd, ctrl);
			if (ret < 0)
				return ret;
		}

		min = mapping->get(mapping, RTS_CTRL_GET_MIN,
				   rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_MIN));
		max = mapping->get(mapping, RTS_CTRL_GET_MAX,
				   rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_MAX));
		step = mapping->get(mapping, RTS_CTRL_GET_RES,
				    rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_RES));
		if (step == 0)
			step = 1;

		xctrl->value = min + ((u32)(xctrl->value - min) + step / 2)
			     / step * step;
		if (mapping->data_type == RTS_CTRL_DATA_TYPE_SIGNED)
			xctrl->value = clamp(xctrl->value, min, max);
		else
			xctrl->value = clamp_t(u32, xctrl->value, min, max);
		value = xctrl->value;
		break;
	case V4L2_CTRL_TYPE_BOOLEAN:
		xctrl->value = clamp(xctrl->value, 0, 1);
		value = xctrl->value;
		break;
	case V4L2_CTRL_TYPE_MENU:
		if (xctrl->value < 0 || xctrl->value >= mapping->menu_count)
			return -ERANGE;
		value = mapping->menu_info[xctrl->value].value;

		/* Valid menu indices are reported by the GET_RES request for
		 * UVC controls that support it.
		 */
		if (mapping->data_type == RTS_CTRL_DATA_TYPE_BITMASK &&
		    (ctrl->info.flags & RTS_CTRL_FLAG_GET_RES)) {
			if (!ctrl->cached) {
				ret = rtscam_ctrl_populate_cache(icd, ctrl);
				if (ret < 0)
					return ret;
			}

			step = mapping->get(mapping, RTS_CTRL_GET_RES,
				rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_RES));
			if (!(step & value))
				return -EINVAL;
		}
		break;
	default:
		value = xctrl->value;
		break;
	}

	/* If the mapping doesn't span the whole UVC control, the current value
	 * needs to be loaded from the device to perform the read-modify-write
	 * operation.
	 */
	if (!ctrl->loaded && (ctrl->info.size * 8) != mapping->size) {
		if ((ctrl->info.flags & RTS_CTRL_FLAG_GET_CUR) == 0) {
			memset(rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_CURRENT),
				0, ctrl->info.size);
		} else {
			ret = rtscam_query_ctrl(icd, &ctrl->info,
				RTS_CTRL_GET_CUR,
				rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_CURRENT));
			if (ret < 0)
				return ret;
		}

		ctrl->loaded = 1;
	}

	/* Backup the current value in case we need to rollback later. */
	if (!ctrl->dirty) {
		memcpy(rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_BACKUP),
		       rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_CURRENT),
		       ctrl->info.size);
	}

	mapping->set(mapping, value,
		rtscam_ctrl_data(ctrl, RTS_CTRL_DATA_CURRENT));

	ctrl->dirty = 1;
	ctrl->modified = 1;

	return 0;
}

static int __rtscam_get_ctrl(struct rtscam_video_device *icd,
		struct v4l2_ext_control *ctrls, __u32 count, __u32 *error_idx)
{
	int ret;
	unsigned int i = count;
	struct v4l2_ext_control *ctrl = ctrls;

	ret = rtscam_ctrl_begin(icd);
	if (ret < 0)
		goto exit;

	for (i = 0; i < count; i++) {
		ret = rtscam_ctrl_get(icd, ctrl);
		if (ret) {
			rtscam_ctrl_rollback(icd);
			goto exit;
		}
		ctrl++;
	}
	ret = rtscam_ctrl_rollback(icd);

exit:
	if (ret && error_idx)
		*error_idx = i;
	return ret;
}

static int __rtscam_set_ctrl(struct rtscam_video_device *icd,
		struct v4l2_ext_control *ctrls, __u32 count, __u32 *error_idx,
		int try)
{
	int ret;
	unsigned int i = count;
	struct v4l2_ext_control *ctrl = ctrls;

	ret = rtscam_ctrl_begin(icd);
	if (ret < 0)
		goto exit;

	for (i = 0; i < count; i++) {
		ret = rtscam_ctrl_set(icd, ctrl);
		if (ret < 0) {
			rtscam_ctrl_rollback(icd);
			goto exit;
		}
		ctrl++;
	}

	if (try)
		ret = rtscam_ctrl_rollback(icd);
	else
		ret = rtscam_ctrl_commit(icd);
exit:
	if (ret && error_idx)
		*error_idx = i;
	return ret;
}

int rtscam_get_ctrl(struct rtscam_video_device *icd,
		struct v4l2_control *ctrl)
{
	struct v4l2_ext_control xctrl;
	int ret;

	memset(&xctrl, 0, sizeof(xctrl));
	xctrl.id = ctrl->id;

	ret = __rtscam_get_ctrl(icd, &xctrl, 1, NULL);
	if (ret >= 0)
		ctrl->value = xctrl.value;

	return ret;
}

int rtscam_set_ctrl(struct rtscam_video_device *icd,
		struct v4l2_control *ctrl)
{
	struct v4l2_ext_control xctrl;
	int ret;

	memset(&xctrl, 0, sizeof(xctrl));
	xctrl.id = ctrl->id;
	xctrl.value = ctrl->value;

	ret = __rtscam_set_ctrl(icd, &xctrl, 1, NULL, 0);
	if (ret >= 0)
		ctrl->value = xctrl.value;

	return ret;
}

int rtscam_get_ext_ctrls(struct rtscam_video_device *icd,
		struct v4l2_ext_controls *ctrls)
{
	return __rtscam_get_ctrl(icd,
			ctrls->controls, ctrls->count, &ctrls->error_idx);
}

int rtscam_set_ext_ctrls(struct rtscam_video_device *icd,
		struct v4l2_ext_controls *ctrls)
{
	return __rtscam_set_ctrl(icd,
			ctrls->controls, ctrls->count, &ctrls->error_idx, 0);
}

int rtscam_try_ext_ctrls(struct rtscam_video_device *icd,
		struct v4l2_ext_controls *ctrls)
{
	return __rtscam_set_ctrl(icd,
			ctrls->controls, ctrls->count, &ctrls->error_idx, 1);
}

int rtscam_video_init_ctrl(struct rtscam_video_device *icd)
{
	int index = 0;
	int ret = 0;

	INIT_LIST_HEAD(&icd->controls);

	while (0 == ret) {
		struct rtscam_video_ctrl *ctrl;
		ret = rtscam_call_video_op(icd, get_ctrl, icd, &ctrl, index);
		if (ret < 0)
			break;
		list_add_tail(&ctrl->list, &icd->controls);
		if (!ctrl->cached)
			rtscam_ctrl_populate_cache(icd, ctrl);
		index++;
	}

	if (0 == index)
		return ret;

	return 0;
}

int rtscam_video_release_ctrl(struct rtscam_video_device *icd)
{
	struct rtscam_video_ctrl *ctrl, *tmp;

	list_for_each_entry_safe(ctrl, tmp, &icd->controls, list) {
		list_del(&ctrl->list);
		rtscam_call_video_op(icd, put_ctrl, icd, ctrl);
	}

	return 0;
}

