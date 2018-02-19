/*
 * Realtek Semiconductor Corp.
 *
 * rts_uvc.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/usb.h>
#include <asm/unaligned.h>
#include "uvcvideo.h"
#include "rts_uvc.h"

int uvc_xu_get_id(struct uvc_video_chain *chain, int *id)
{
	struct uvc_entity *entity;

	if (!id)
		return -EINVAL;

	/* Find the extension unit. */
	list_for_each_entry(entity, &chain->entities, chain) {
		if (UVC_ENTITY_TYPE(entity) == UVC_VC_EXTENSION_UNIT) {
			*id = entity->id;
			return 0;
		}
	}

	return -EINVAL;
}

int uvc_get_usb_dev_info(struct uvc_video_chain *chain, struct rts_usb_device_info *info)
{
	if (!info)
		return -EINVAL;

	info->port = chain->dev->udev->portnum;
	info->port = chain->dev->udev->speed;
	info->vendor_id = chain->dev->udev->descriptor.idVendor;
	info->product_id = chain->dev->udev->descriptor.idProduct;
	info->bcd_device = chain->dev->udev->descriptor.bcdDevice;

	return 0;
}

int uvc_get_usb_descriptor(struct uvc_video_chain *chain, struct rts_usb_desc_buf *udesc)
{
	struct usb_device *udev = chain->dev->udev;
	int length;
	int i;
	struct usb_config_descriptor *desc = NULL;
	u8 *pos;

	if (!udesc || !udesc->data)
		return -EINVAL;

	length = sizeof(udev->descriptor);

	for (i = 0; i < udev->descriptor.bNumConfigurations; i++) {
		desc = (struct usb_config_descriptor *)(udev->rawdescriptors[i]);
		length += le16_to_cpu(desc->wTotalLength);
	}

	if (udesc->length < length) {
		uvc_printk(KERN_ERR,
				"descriptor length is %d, buffer size is %d\n",
				length, udesc->length);
		return -EINVAL;
	}

	udesc->length = length;

	pos = udesc->data;
	length = sizeof(udev->descriptor);
	copy_to_user(pos, &(udev->descriptor), length);
	pos += length;

	for (i = 0; i < udev->descriptor.bNumConfigurations; i++) {
		desc = (struct usb_config_descriptor *)(udev->rawdescriptors[i]);
		length = le16_to_cpu(desc->wTotalLength);
		copy_to_user(pos, desc, length);
		pos += length;
	}

	return 0;
}

int uvc_query_request(struct uvc_device *dev, struct rts_usb_dev_request *request)
{
	struct usb_device *udev = dev->udev;
	u8 *buf;
	unsigned int pipe;
	int ret = 0;
	u8 type;
	u16 length;

	if (!request)
		return -EINVAL;
	length = request->request.wLength;
	if (length && !request->data)
		return -EINVAL;
	if (length > RTS_UVC_REQUEST_LEN) {
		uvc_printk(KERN_ERR,
			"USB control request data len (%d) must lower %d",
			request->request.wLength, RTS_UVC_REQUEST_LEN);
		return -ENOMEM;
	}

	if (!dev->rts_req_buf) {
		dev->rts_req_buf = kzalloc(RTS_UVC_REQUEST_LEN, GFP_KERNEL);
		if (!dev->rts_req_buf)
			return -ENOMEM;
	}

	buf = dev->rts_req_buf;
	type = request->request.bRequestType;

	if (!(type & 0x80) && length > 0)
		copy_from_user(buf, request->data, length);

	pipe = (type & 0x80) ? usb_rcvctrlpipe(udev, 0)
		: usb_sndctrlpipe(udev, 0);

	ret = usb_control_msg(udev, pipe, request->request.bRequest,
			request->request.bRequestType,
			request->request.wValue,
			request->request.wIndex,
			buf,
			request->request.wLength,
			UVC_CTRL_CONTROL_TIMEOUT);
	/*
	uvc_printk(KERN_INFO, "requesttype = %x\n\
			request = %x\n\
			value = %x\n\
			index = %x\n\
			length = %x\n\
			ret = %d\n",
			request->request.bRequestType,
			request->request.bRequest,
			request->request.wValue,
			request->request.wIndex,
			request->request.wLength,
			ret);
	*/

	if (ret == length) {
		if ((type & 0x80) && (length > 0))
			copy_to_user(request->data, buf, length);
		ret = 0;
	} else {
		ret = -EIO;
	}

	return ret;
}

