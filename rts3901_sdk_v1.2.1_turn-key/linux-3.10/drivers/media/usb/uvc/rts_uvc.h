/*
 * Realtek Semiconductor Corp.
 *
 * rts_uvc.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _RTS_UVC_H
#define _RTS_UVC_H

struct rts_usb_device_info {
	__u8 port;
	__u8 speed;
	__u16 vendor_id;
	__u16 product_id;
	__u16 bcd_device;
	__u8 reserved[32];
};

struct rts_usb_desc_buf {
	__u32 length;
	__u8 *data;
};

#define RTS_UVC_REQUEST_LEN		(1024 * 4)

struct rts_usb_dev_request {
	struct {
		__u8  bRequestType;
		__u8  bRequest;
		__u16 wValue;
		__u16 wIndex;
		__u16 wLength;
	} request;

	__u8 *data;
};

extern int uvc_xu_get_id(struct uvc_video_chain *chain, int *id);
extern int uvc_get_usb_dev_info(struct uvc_video_chain *chain, struct rts_usb_device_info *info);
extern int uvc_get_usb_descriptor(struct uvc_video_chain *chain, struct rts_usb_desc_buf *udesc);
extern int uvc_query_request(struct uvc_device *dev, struct rts_usb_dev_request *request);

#define RTS_IOCTL_GET_DEV_INFO        _IOR('V', 1, struct rts_usb_device_info)
#define RTS_IOCTL_GET_USB_DESCRIPTOR  _IOWR('V', 2, struct rts_usb_desc_buf)
#define RTS_IOCTL_GET_XU_ID           _IOR('V', 3, int)
#define RTS_IOCTL_DEV_REQUEST         _IOWR('V', 4, struct rts_usb_dev_request)

#endif

