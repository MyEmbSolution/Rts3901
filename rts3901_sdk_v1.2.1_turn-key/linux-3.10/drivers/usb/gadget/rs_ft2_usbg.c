/*
 * rs_ft2_usbg.c -- Realsil FT2 USB gadget test driver
 *
 * Copyright (C) 2003-2008 David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 * Copyright (C) 2015 by Realsil
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */

#define VERBOSE_DEBUG

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>


#define DRIVER_DESC		"FT2Gadget"
#define DRIVER_VERSION	"March 26, 2015"


/* module parameters: idVendor, idProduct... */
USB_GADGET_COMPOSITE_OPTIONS();

/*
 * when check_outdata=1, then need to sync pattern with host
 */
static bool check_outdata = 0;
module_param(check_outdata, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(check_outdata, "0 = not check out data, 1 = check out data");

static unsigned int pattern = 0;
module_param_named(pattern, pattern, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(pattern, "0 = all zeroes, 1 = mod63, 2 = none");

static unsigned int buflen = 512;
module_param(buflen, uint, S_IRUGO);
MODULE_PARM_DESC(buflen, "bulk out buffer length");


/*
FIX Me
1. function driver should be able to control pull up resistors
2. check more pointer in kernel to avoid kernel crush
3. manage debug level
*/

struct ft2_usbg_device {
	struct usb_function	function;
	struct usb_ep			*in_ep1;
	struct usb_ep			*in_ep2;
	struct usb_ep			*out_ep;
	struct usb_ep			*int_in_ep1;
	struct usb_ep			*int_in_ep2;

	int					cur_alt;
};

/*
change global value to ep->driver_data
*/
/* struct ft2_usbg_device *ft2_usbg_dev; */

static inline struct ft2_usbg_device *
		func_to_ft2_usbg_device(struct usb_function *f)
{
	return container_of(f, struct ft2_usbg_device, function);
}

static char const serial[] = "0123456789.0123456789.0123456789";
#define USB_GADGET_CONFIG_IDX		(USB_GADGET_FIRST_AVAIL_IDX + 0)
#define USB_GADGET_INTF_IDX		(USB_GADGET_FIRST_AVAIL_IDX + 1)

static struct usb_string strings_dev[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = "Realsil",		/*idx=1*/
	[USB_GADGET_PRODUCT_IDX].s = DRIVER_DESC,		/*idx=2*/
	[USB_GADGET_SERIAL_IDX].s = serial,		/*idx=3*/
	[USB_GADGET_CONFIG_IDX].s = "config loop in to out",	/*idx=4*/
	[USB_GADGET_INTF_IDX].s = "intf loop in to out",	/*idx=5*/
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings		= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

/*
usb_device_descriptor:
bDeviceClass=0xff, bDeviceSubClass=0, bDeviceProtocol=0

usb_interface_descriptor:
bInterfaceClass=0xff, bInterfaceSubClass=0, bInterfaceProtocol=0
*/
static struct usb_device_descriptor device_desc = {
	.bLength				=	sizeof(device_desc),
	.bDescriptorType		=	USB_DT_DEVICE,

	.bcdUSB				=	cpu_to_le16(0x0200),
	.bDeviceClass			=	USB_CLASS_VENDOR_SPEC,
/*	.bDeviceSubClass		=	0, */
/*	.bDeviceProtocol		=	0, */
	.idVendor				=	cpu_to_le16(0x0BDA),
	.idProduct			=	cpu_to_le16(0x3901),
	.bNumConfigurations	=	1,
};

static struct usb_configuration ft2_usbg_config_driver = {
	.label				= DRIVER_DESC,
	.bConfigurationValue	= 1,
	.bmAttributes			= USB_CONFIG_ATT_SELFPOWER,
};

static struct usb_interface_descriptor ft2_usbg_interface_desc = {
	.bLength		= sizeof(ft2_usbg_interface_desc),
	.bDescriptorType		= USB_DT_INTERFACE,

	.bNumEndpoints		= 5,
	.bInterfaceClass		= USB_CLASS_VENDOR_SPEC,
/*	.bInterfaceSubClass	= 0, */
/*	.bInterfaceProtocol		= 0, */
/*	.iInterface			= DYNAMIC */
};

/* ep1 bulk in */
static struct usb_endpoint_descriptor hs_ft2_usbg_ep1_desc = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize	= cpu_to_le16(512),
};

/* ep2 bulk in */
static struct usb_endpoint_descriptor hs_ft2_usbg_ep2_desc = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize	= cpu_to_le16(512),
};

/* ep3 bulk out */
static struct usb_endpoint_descriptor hs_ft2_usbg_ep3_desc = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize	= cpu_to_le16(512),
};

/* ep4 int in */
static struct usb_endpoint_descriptor hs_ft2_usbg_ep4_desc = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize	= cpu_to_le16(16),
	.bInterval		= 10,
};

/* ep5 int in */
static struct usb_endpoint_descriptor hs_ft2_usbg_ep5_desc = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize	= cpu_to_le16(16),
	.bInterval		= 10,
};

static struct usb_descriptor_header *hs_ft2_usbg_descs[] = {
	(struct usb_descriptor_header *) &ft2_usbg_interface_desc,
	(struct usb_descriptor_header *) &hs_ft2_usbg_ep1_desc,
	(struct usb_descriptor_header *) &hs_ft2_usbg_ep2_desc,
	(struct usb_descriptor_header *) &hs_ft2_usbg_ep3_desc,
	(struct usb_descriptor_header *) &hs_ft2_usbg_ep4_desc,
	(struct usb_descriptor_header *) &hs_ft2_usbg_ep5_desc,
	NULL,
};

static struct usb_request *alloc_ep_req(struct usb_ep *ep, int len)
{
	struct usb_request      *req;

	req = usb_ep_alloc_request(ep, GFP_ATOMIC);
	if (req) {
		if (len)
			req->length = len;
		else
			req->length = buflen;
		req->buf = kmalloc(req->length, GFP_ATOMIC);
		if (!req->buf) {
			usb_ep_free_request(ep, req);
			req = NULL;
		}
	}
	return req;
}

static void free_ep_req(struct usb_ep *ep, struct usb_request *req)
{
	kfree(req->buf);
	usb_ep_free_request(ep, req);

	req = NULL;
}

static void disable_ep(struct usb_composite_dev *cdev, struct usb_ep *ep)
{
	int			value;

	if (ep->driver_data) {
		value = usb_ep_disable(ep);
		if (value < 0)
			DBG(cdev, "disable %s --> %d\n", ep->name, value);
		ep->driver_data = NULL;
	}
}

/*
check_read_data: print out ep data, check out ep data len and pattern
check_read_data is called in source_sink_complete by out ep
option: if there is no need, let module parameter "check_outdata=0"
*/
static int check_read_data(struct ft2_usbg_device *lb, struct usb_request *req)
{
	unsigned		i;
	u8			*buf = req->buf;
	struct usb_composite_dev *cdev = lb->function.config->cdev;

	/*
	module parameter check_outdata, see top of this file
	check_outdata=0 not print out data, check out data ...
	check_outdata=1 print out data, check out data ...
	*/
	if (!check_outdata)
		return 0;

	DBG(cdev, "actual=%d, length=%d\n", req->actual, req->length);

	/* step 1: print out ep data */
	for (i = 0; i < req->actual; i++) {
		printk("%d", ((unsigned char *)req->buf)[i]);
		if (i % 63 == 62)
			printk("\n");
	}
	printk("\n");

	/* step2: check out data length
	judge req->actual, req->length to see whether out transfer failed or not
	*/

	/* step3: check out ep data pattern */
	for (i = 0; i < req->actual; i++, buf++) {
		switch (pattern) {

		/* all-zeroes has no synchronization issues */
		case 0:
			if (*buf == 0)
				continue;
			break;

		/* "mod63" stays in sync with short-terminated transfers,
		 * OR otherwise when host and gadget agree on how large
		 * each usb transfer request should be.  Resync is done
		 * with set_interface or set_config.  (We *WANT* it to
		 * get quickly out of sync if controllers or their drivers
		 * stutter for any reason, including buffer duplication...)
		 */
		case 1:
			if (*buf == (u8)(i % 63))
				continue;
			break;
		case 2:
			if (*buf == (u8)(62 - i % 63))
				continue;
			break;
		}
		ERROR(cdev, "bad OUT byte, buf[%d] = %d\n", i, *buf);
		usb_ep_set_halt(lb->out_ep);
		return -EINVAL;
	}
	return 0;
}

static void reinit_write_data(struct usb_ep *ep, struct usb_request *req)
{
	int i;

	switch (pattern) {
	case 0:
		memset(req->buf, 0, req->length);
		break;
	case 1:
		for (i = 0; i < req->length; i++)
			((unsigned char *)req->buf)[i] = i % 63;

		break;
	case 2:
		for (i = 0; i < req->length; i++)
			((unsigned char *)req->buf)[i] = 62 - i % 63;

		break;
	default:
		break;
	}
}

/*
 let "ep_flag" toggle to select in_ep1 or in_ep2
 ep_flag=0  -->  in_ep1
 ep_flag=1  -->  in_ep2
 */
static void source_sink_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct ft2_usbg_device		*lb = ep->driver_data;
	struct usb_composite_dev	*cdev = lb->function.config->cdev;
	int						status = req->status;

	DBG(cdev, "%s %s status=%d\n", __func__, ep->name, status);

	/* driver_data is null if ep has been disabled or not enabled */
	if (!lb)
		return;

	switch (status) {
	case 0:						/* normal completion? */
		if (ep == lb->out_ep)
			check_read_data(lb, req);

		break;

		/* "should never get here" */
		/* FALLTHROUGH */

	/* this endpoint is normally active while we're configured */
	case -ECONNABORTED:		/* hardware forced ep reset */
	case -ECONNRESET:			/* request dequeued */
	case -ESHUTDOWN:			/* disconnect from host */
		VDBG(cdev, "%s gone (%d), %d/%d\n", ep->name, status,
				req->actual, req->length);
		if (ep == lb->out_ep)
			check_read_data(lb, req);
		free_ep_req(ep, req);
		return;

	case -EOVERFLOW:/* buffer overrun on read means that
					* we didn't provide a big enough
					* buffer.
					*/
	default:
		DBG(cdev, "%s complete --> %d, %d/%d\n",
				ep->name,
				status,
				req->actual,
				req->length);

	case -EREMOTEIO:			/* short read */
		break;
	}


	/*
	 * ep1 and ep2 doesn't need to queue req again in it's complete function
	 * ep1 and ep2 need to queue just after out transfer has completed
	 */
	if (ep == lb->out_ep && req->actual > 0) {
		if (*((u8 *)req->buf) == 0) {
			status = usb_ep_queue(lb->in_ep1, req, GFP_ATOMIC);
			if (status) {
				ERROR(cdev,
					"kill %s:  resubmit %d bytes --> %d\n",
					lb->in_ep1->name,
					req->length,
					status);
				usb_ep_set_halt(lb->in_ep1);
				return;
			}
			return ;
		} else if (*((u8 *)req->buf) == 62) {
			status = usb_ep_queue(lb->in_ep2, req, GFP_ATOMIC);
			if (status) {
				ERROR(cdev,
					"kill %s:  resubmit %d bytes --> %d\n",
					lb->in_ep2->name,
					req->length,
					status);
				usb_ep_set_halt(lb->in_ep2);
				return;
			}
			return ;
		}
	}

	if (ep == lb->in_ep1 || ep == lb->in_ep2) {
		status = usb_ep_queue(lb->out_ep, req, GFP_ATOMIC);
		if (status) {
			ERROR(cdev, "kill %s:  resubmit %d bytes --> %d\n",
					lb->out_ep->name, req->length, status);
			usb_ep_set_halt(lb->out_ep);
			/* FIXME recover later ... somehow */
		}
		return ;
	}

	status = usb_ep_queue(ep, req, GFP_ATOMIC);
	if (status) {
		ERROR(cdev, "kill %s:  resubmit %d bytes --> %d\n",
				ep->name, req->length, status);
		usb_ep_set_halt(ep);
		/* FIXME recover later ... somehow */
	}

}

static int source_sink_start_ep(struct usb_ep *ep, bool is_int, bool is_in)
{
	struct ft2_usbg_device *ft2_usbg_dev = ep->driver_data;
	struct usb_composite_dev *cdev = ft2_usbg_dev->function.config->cdev;
	struct usb_request *req;
	int status;
	unsigned int tmp;

	if (is_int) {
		req = alloc_ep_req(ep, 16);
		if (!req) {
			ERROR(cdev, "alloc req for %s failed\n", ep->name);
			return -ENOMEM;
		}

		req->complete = source_sink_complete;

		tmp = pattern;
		pattern = 1;
		reinit_write_data(ep, req);
		pattern = tmp;

		status = usb_ep_queue(ep, req, GFP_ATOMIC);
		if (status) {
			ERROR(cdev, "start %s %s --> %d\n",
						is_int ? "INT-" : "",
						ep->name,
						status);
			free_ep_req(ep, req);
		}
	} else {
		req = alloc_ep_req(ep, 0);/*default 0, that is 512*/
		if (!req) {
			ERROR(cdev, "alloc req for %s failed\n", ep->name);
			return -ENOMEM;
		}

		req->complete = source_sink_complete;

		if (is_in)
			reinit_write_data(ep, req);
		else
			memset(req->buf, 0x55, req->length);

		status = usb_ep_queue(ep, req, GFP_ATOMIC);
		if (status) {
			ERROR(cdev, "start %s%s %s --> %d\n",
						is_int ? "INT-" : "",
						is_in ? "IN" : "OUT",
						ep->name,
						status);
			free_ep_req(ep, req);
		}
	}

	return status;
}

static void disable_source_sink(struct ft2_usbg_device *lb)
{
	struct usb_composite_dev *cdev = lb->function.config->cdev;

	disable_ep(cdev, lb->in_ep1);
	disable_ep(cdev, lb->in_ep2);
	disable_ep(cdev, lb->out_ep);
	disable_ep(cdev, lb->int_in_ep1);
	disable_ep(cdev, lb->int_in_ep2);

	/* free request is done in dwc_otg */
/*
	if (lb->in_ep1_req)
		free_ep_req(lb->in_ep1, lb->in_ep1_req);
	if (lb->in_ep2_req)
		free_ep_req(lb->in_ep2, lb->in_ep2_req);
	if (lb->out_ep_req)
		free_ep_req(lb->out_ep, lb->out_ep_req);
	if (lb->int_in_ep1_req)
		free_ep_req(lb->int_in_ep1, lb->int_in_ep1_req);
	if (lb->int_in_ep2_req)
		free_ep_req(lb->int_in_ep2, lb->int_in_ep2_req);
*/
	DBG(cdev, "%s disabled\n", lb->function.name);
}

static int enable_source_sink(struct usb_composite_dev *cdev,
					struct ft2_usbg_device *lb, int alt)
{
	int result = 0;
	struct usb_ep	 *ep;

	/*-------------------------- enable ep1 in ------------------------*/
	ep = lb->in_ep1;
	result = config_ep_by_speed(cdev->gadget, &(lb->function), ep);
	if (result)
		return result;

	result = usb_ep_enable(ep);
	if (result < 0)
		return result;
	ep->driver_data = lb;

	DBG(cdev, "%s  addr=0x%02x, size=%d, enabled!\n",
			ep->name,
			ep->desc->bEndpointAddress,
			ep->desc->wMaxPacketSize);

	/*-------------------------- enable ep2 in ------------------------*/
	ep = lb->in_ep2;
	result = config_ep_by_speed(cdev->gadget, &(lb->function), ep);
	if (result)
		return result;

	result = usb_ep_enable(ep);
	if (result < 0)
		return result;
	ep->driver_data = lb;

	DBG(cdev, "%s  addr=0x%02x, size=%d, enabled!\n",
			ep->name,
			ep->desc->bEndpointAddress,
			ep->desc->wMaxPacketSize);

	/*--------------------- enable ep3 out and queue data ----------------*/
	ep = lb->out_ep;
	result = config_ep_by_speed(cdev->gadget, &(lb->function), ep);
	if (result)
		return result;

	result = usb_ep_enable(ep);
	if (result < 0)
		return result;
	ep->driver_data = lb;

	result = source_sink_start_ep(ep, 0, 0);
	if (result < 0) {
fail3:
		ep = lb->in_ep1;
		usb_ep_disable(ep);
		ep->driver_data = NULL;

		ep = lb->in_ep2;
		usb_ep_disable(ep);
		ep->driver_data = NULL;

		ep = lb->out_ep;
		usb_ep_disable(ep);
		ep->driver_data = NULL;
		return result;
	}

	DBG(cdev, "%s addr=0x%02x, size=%d, enabled and start!\n",
			ep->name,
			ep->desc->bEndpointAddress,
			ep->desc->wMaxPacketSize);

	/*-------------------- enable ep4 int in and queue data --------------*/
	ep = lb->int_in_ep1;
	result = config_ep_by_speed(cdev->gadget, &(lb->function), ep);
	if (result)
		return result;

	result = usb_ep_enable(ep);
	if (result < 0)
		return result;
	ep->driver_data = lb;

	result = source_sink_start_ep(ep, 1, 1);
	if (result < 0) {
fail4:
		ep = lb->int_in_ep1;
		usb_ep_disable(ep);
		ep->driver_data = NULL;
		goto fail3;
	}

	DBG(cdev, "%s  addr=0x%02x, size=%d, enabled and start!\n",
			ep->name,
			ep->desc->bEndpointAddress,
			ep->desc->wMaxPacketSize);

	/*-------------------- enable ep5 int in and queue data --------------*/
	ep = lb->int_in_ep2;
	result = config_ep_by_speed(cdev->gadget, &(lb->function), ep);
	if (result)
		return result;

	result = usb_ep_enable(ep);
	if (result < 0)
		return result;
	ep->driver_data = lb;

	result = source_sink_start_ep(ep, 1, 1);
	if (result < 0) {
		ep = lb->int_in_ep2;
		usb_ep_disable(ep);
		ep->driver_data = NULL;
		goto fail4;
	}

	DBG(cdev, "%s  addr=0x%02x, size=%d, enabled and start!\n",
			ep->name,
			ep->desc->bEndpointAddress,
			ep->desc->wMaxPacketSize);

	lb->cur_alt = alt;
	return result;
}

static int ft2_usbg_function_bind(struct usb_configuration *c,
						struct usb_function *f)
{
	int id, ret;
	struct usb_composite_dev *cdev = c->cdev;
	struct ft2_usbg_device *ft2_usbg_dev = func_to_ft2_usbg_device(f);

	DBG(cdev, "%s\n", __func__);

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	ft2_usbg_interface_desc.bInterfaceNumber = id;

	/* judge to avoid NULL pointer */
	if (cdev == NULL || cdev->gadget == NULL)
		return -ENODEV;

	/* get ep1 in from cdev->gadget */
	ft2_usbg_dev->in_ep1 =
			usb_ep_autoconfig(cdev->gadget, &hs_ft2_usbg_ep1_desc);
	if (!ft2_usbg_dev->in_ep1) {
		ERROR(cdev, "%s: can't autoconfig ep1 on %s\n",
						f->name, cdev->gadget->name);
		return -ENODEV;
	}
	ft2_usbg_dev->in_ep1->driver_data = cdev;

	/* get ep2 in from cdev->gadget */
	ft2_usbg_dev->in_ep2 =
			usb_ep_autoconfig(cdev->gadget, &hs_ft2_usbg_ep2_desc);
	if (!ft2_usbg_dev->in_ep2) {
		ERROR(cdev, "%s: can't autoconfig ep2 on %s\n",
						f->name, cdev->gadget->name);
		return -ENODEV;
	}
	ft2_usbg_dev->in_ep2->driver_data = cdev;

	/* get ep3 out from cdev->gadget */
	ft2_usbg_dev->out_ep =
			usb_ep_autoconfig(cdev->gadget, &hs_ft2_usbg_ep3_desc);
	if (!ft2_usbg_dev->out_ep) {
		ERROR(cdev, "%s: can't autoconfig ep3 on %s\n",
						f->name, cdev->gadget->name);
		return -ENODEV;
	}
	ft2_usbg_dev->out_ep->driver_data = cdev;

	/* get ep4 int in in from cdev->gadget */
	ft2_usbg_dev->int_in_ep1 =
			usb_ep_autoconfig(cdev->gadget, &hs_ft2_usbg_ep4_desc);
	if (!ft2_usbg_dev->int_in_ep1) {
		ERROR(cdev, "%s: can't autoconfig ep4 on %s\n",
						f->name, cdev->gadget->name);
		return -ENODEV;
	}
	ft2_usbg_dev->int_in_ep1->driver_data = cdev;

	/* get ep5 int in in from cdev->gadget */
	ft2_usbg_dev->int_in_ep2 =
			usb_ep_autoconfig(cdev->gadget, &hs_ft2_usbg_ep5_desc);
	if (!ft2_usbg_dev->int_in_ep2) {
		ERROR(cdev, "%s: can't autoconfig ep5 on %s\n",
						f->name, cdev->gadget->name);
		return -ENODEV;
	}
	ft2_usbg_dev->int_in_ep2->driver_data = cdev;

	/*
	struct usb_endpoint_descriptor.bEndpointAddress and wMaxPacketSize
	are changed by usb_ep_autoconfig, so modify them.
	*/
	hs_ft2_usbg_ep1_desc.wMaxPacketSize	= cpu_to_le16(512);
	hs_ft2_usbg_ep2_desc.wMaxPacketSize	= cpu_to_le16(512);
	hs_ft2_usbg_ep3_desc.wMaxPacketSize	= cpu_to_le16(512);
	hs_ft2_usbg_ep4_desc.wMaxPacketSize = cpu_to_le16(16);
	hs_ft2_usbg_ep5_desc.wMaxPacketSize = cpu_to_le16(16);

	ret = usb_assign_descriptors(f, NULL, hs_ft2_usbg_descs, NULL);
	if (ret)
		return ret;

	DBG(cdev, "%s: %s, %s, %s, %s, %s\n",
						f->name,
						ft2_usbg_dev->in_ep1->name,
						ft2_usbg_dev->in_ep2->name,
						ft2_usbg_dev->out_ep->name,
						ft2_usbg_dev->int_in_ep1->name,
						ft2_usbg_dev->int_in_ep2->name);

	DBG(cdev, "%s ok!\n", __func__);

	return 0;
}

static void ft2_usbg_function_unbind(struct usb_configuration *c,
					struct usb_function *f)
{
	usb_free_all_descriptors(f);

	/*
	ft2_usbg_bind -> ft2_usbg_bind_config:
				alloc mem for struct ft2_usbg_device
	ft2_usbg_function_unbind			:
				free mem for struct ft2_usbg_device

	option:
	if declare global variable for struct ft2_usbg_device
	ft2_usbg_bind -> ft2_usbg_bind_config:
				alloc mem for struct ft2_usbg_device
	ft2_usbg_unbind					:
				free mem for struct ft2_usbg_device
	*/
	kfree(func_to_ft2_usbg_device(f));
}

static int ft2_usbg_function_set_alt(struct usb_function *f,
					unsigned intf, unsigned alt)
{
	struct ft2_usbg_device *ft2_usbg_dev = func_to_ft2_usbg_device(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	if (ft2_usbg_dev->in_ep1->driver_data || ft2_usbg_dev->in_ep2->driver_data)
		disable_source_sink(ft2_usbg_dev);

	return enable_source_sink(cdev, ft2_usbg_dev, alt);
}

static void ft2_usbg_function_disable(struct usb_function *f)
{
	struct ft2_usbg_device *ft2_usbg_dev = func_to_ft2_usbg_device(f);

	disable_source_sink(ft2_usbg_dev);
}

/*
static void ft2_usbg_function_free_func(struct usb_function *f)
{
	struct ft2_usbg_dev *dev =
				container_of(f, struct ft2_usbg_dev, function);

	usb_free_all_descriptors(f);
	kfree(dev);
}
*/

static int ft2_usbg_bind_config(struct usb_configuration *c)
{
	struct ft2_usbg_device *ft2_usbg_dev;

	DBG(c->cdev, "%s\n", __func__);

	ft2_usbg_dev = kzalloc(sizeof(struct ft2_usbg_device), GFP_KERNEL);
	if (!ft2_usbg_dev)
		return -ENOMEM;

	ft2_usbg_dev->function.name = "rs_ft2_usbg loopback";
	/* usb_assign_descriptors has do this f->hs_descriptors = xxx; */
/*	ft2_usbg_dev->function.hs_descriptors = hs_ft2_usbg_descs; */
	ft2_usbg_dev->function.strings = dev_strings;
	ft2_usbg_dev->function.bind = ft2_usbg_function_bind;
	ft2_usbg_dev->function.unbind = ft2_usbg_function_unbind;
	ft2_usbg_dev->function.set_alt = ft2_usbg_function_set_alt;
	ft2_usbg_dev->function.disable = ft2_usbg_function_disable;
	/* if has function.unbind, then doesn't need function.free_func */
/*	ft2_usbg_dev->function.free_func = ft2_usbg_function_free_func; */

	return usb_add_function(c, &ft2_usbg_dev->function);
}



static int __init ft2_usbg_bind(struct usb_composite_dev *cdev)
{
	int			status;

	DBG(cdev, "%s\n", __func__);

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */
	status = usb_string_ids_tab(cdev, strings_dev);
	if (status < 0)
		return status;

	device_desc.iManufacturer =
				strings_dev[USB_GADGET_MANUFACTURER_IDX].id;
	device_desc.iProduct =
				strings_dev[USB_GADGET_PRODUCT_IDX].id;
	device_desc.iSerialNumber =
				strings_dev[USB_GADGET_SERIAL_IDX].id;
	ft2_usbg_config_driver.iConfiguration =
				strings_dev[USB_GADGET_CONFIG_IDX].id;
	ft2_usbg_interface_desc.iInterface =
				strings_dev[USB_GADGET_INTF_IDX].id;

	status = usb_add_config(cdev,
				&ft2_usbg_config_driver, ft2_usbg_bind_config);
	if (status < 0)
		return status;

	DBG(cdev, "%s ok!\n", __func__);
	DBG(cdev, DRIVER_DESC ", version: " DRIVER_VERSION "\n");

	return 0;
}

static int ft2_usbg_unbind(struct usb_composite_dev *cdev)
{
	DBG(cdev, "%s\n", __func__);

	return 0;
}

#ifdef CONFIG_PM
static void ft2_usbg_suspend(struct usb_composite_dev *cdev)
{
	DBG(cdev, "%s\n", __func__);
}

static void ft2_usbg_resume(struct usb_composite_dev *cdev)
{
	DBG(cdev, "%s\n", __func__);
}

#else
#define ft2_usbg_suspend		NULL
#define ft2_usbg_resume		NULL
#endif

/*
file name					rs_ft2_usbg.c
module name				rs_ft2_usbg.ko
usb_composite_driver name	rs_ft2_usbg
DRIVER_DESC			"FT2Gadget"
usb_function name			"rs_ft2_usbg loopback";
*/
static __refdata struct usb_composite_driver ft2_usbg_driver = {
	.name		= "rs_ft2_usbg",
	.dev			= &device_desc,
	.strings		= dev_strings,
	.max_speed	= USB_SPEED_HIGH,
	.bind		= ft2_usbg_bind,
	.unbind		= ft2_usbg_unbind,
/*	.disconnect */

	.suspend		= ft2_usbg_suspend,
	.resume		= ft2_usbg_resume,
};

static int __init ft2_usbg_init(void)
{
	return usb_composite_probe(&ft2_usbg_driver);
}
module_init(ft2_usbg_init);

static void __exit ft2_usbg_exit(void)
{
	usb_composite_unregister(&ft2_usbg_driver);
}
module_exit(ft2_usbg_exit);


MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("lei_wang@realsil.com.cn");
MODULE_LICENSE("GPL");
