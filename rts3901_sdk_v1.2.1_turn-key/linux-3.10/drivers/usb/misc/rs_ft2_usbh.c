/*
 * Realsil FT2 USB Host Testing driver
 *
 * Copyright (C) 2015 lei_wang (lei_wang@realsil.com.cn)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/scatterlist.h>
#include <linux/mutex.h>
#include <linux/usb.h>

/*
 * set up all urbs so they can be used with either bulk or interrupt
 */
#define INTERRUPT_RATE	1	/* msec/transfer */
#define GUARD_BYTE		0xA5

/*
 * sync with device, default pattern=1
 */
static unsigned pattern = 1;
static unsigned mod_pattern;
module_param_named(pattern, mod_pattern, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mod_pattern, "i/o pattern (0 == zeroes)");

static bool check_indata = 0;
module_param(check_indata, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(check_indata, "0 = not check in data, 1 = check in data");


/*
 * keep "struct usbtest_param" the same with application testusb.c
 * and rs_ft2_usbh.c share the same application with usbtest.c driver
 * FIXME make these public somewhere; usbdevfs.h?
 */
struct usbtest_param {
	/* inputs */
	unsigned		test_num;	/* 0..(TEST_CASES-1) */
	unsigned		iterations;
	unsigned		length;
	unsigned		vary;
	unsigned		sglen;

	/* outputs */
	struct timeval		duration;
};
#define USBTEST_REQUEST	_IOWR('U', 100, struct usbtest_param)

/*
 * this is accessed only through usbfs ioctl calls.
 * one ioctl to issue a test ... one lock per device.
 * tests create other threads if they need them.
 * urbs and buffers are allocated dynamically,
 * and data generated deterministically.
 */
struct usbtest_dev {
	struct usb_interface *intf;
	int alt;
	int in_pipe1;
	int in_pipe2;
	int out_pipe;
	int in_int_pipe1;
	int in_int_pipe2;
	struct mutex lock;
};

static struct usb_device *testdev_to_usbdev(struct usbtest_dev *test)
{
	return interface_to_usbdev(test->intf);
}

static int get_endpoints(struct usbtest_dev *dev, struct usb_interface *intf)
{
	struct usb_host_interface *alt;
	struct usb_host_endpoint *in1, *in2, *out;
	struct usb_host_endpoint *int_in1, *int_in2;
	struct usb_device *udev;
	int tmp;

	for (tmp = 0; tmp < intf->num_altsetting; tmp++) {
		unsigned	ep;

		in1 = in2 = out = NULL;
		int_in1 = int_in2 = NULL;
		alt = intf->altsetting + tmp;

		/*
		 * take the first altsetting with in-bulk + out-bulk;
		 * ignore other endpoints and altsettings.
		 */
		for (ep = 0; ep < alt->desc.bNumEndpoints; ep++) {
			struct usb_host_endpoint	*e;

			e = alt->endpoint + ep;
			switch (e->desc.bmAttributes) {
			case USB_ENDPOINT_XFER_BULK:
				break;
			case USB_ENDPOINT_XFER_INT:
				goto try_int;
			default:
				continue;
			}

			if (usb_endpoint_dir_in(&e->desc)) {
				if (!in1)
					in1 = e;
				else if (!in2)
					in2 = e;
			} else {
				if (!out)
					out = e;
			}
			continue;
try_int:
			if (usb_endpoint_dir_in(&e->desc)) {
				if (!int_in1)
					int_in1 = e;
				else if (!int_in2)
					int_in2 = e;
			}
		}
		if (in1 && in2 && out && int_in1 && int_in2)
			goto found;
	}
	return -EINVAL;

found:
	dev_dbg(&intf->dev, "found all eps\n");

	udev = testdev_to_usbdev(dev);
	dev->alt = alt->desc.bAlternateSetting;
	if (alt->desc.bAlternateSetting != 0) {
		tmp = usb_set_interface(udev,
				alt->desc.bInterfaceNumber,
				alt->desc.bAlternateSetting);
		if (tmp < 0)
			return tmp;
	}

	if (in1)
		dev->in_pipe1 = usb_rcvbulkpipe(udev,
			in1->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
	if (in2)
		dev->in_pipe2 = usb_rcvbulkpipe(udev,
			in2->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
	if (out)
		dev->out_pipe = usb_sndbulkpipe(udev,
			out->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
	if (int_in1) {
		dev->in_int_pipe1 =
			usb_rcvintpipe(udev,
						int_in1->desc.bEndpointAddress
						& USB_ENDPOINT_NUMBER_MASK);
	}
	if (int_in2) {
		dev->in_int_pipe2 =
			usb_rcvintpipe(udev,
						int_in2->desc.bEndpointAddress
						& USB_ENDPOINT_NUMBER_MASK);
	}

	dev_dbg(&intf->dev,
			"ep1=%x, ep2=%x, ep3=%x, ep4=%x, ep5=%x\n",
			dev->in_pipe1, dev->in_pipe2,
			dev->out_pipe,
			dev->in_int_pipe1, dev->in_int_pipe2);

	return 0;
}

static void simple_callback(struct urb *urb)
{
	complete(urb->context);
}

static struct urb *usbtest_alloc_urb(
	struct usb_device	*udev,
	int			pipe,
	unsigned long	bytes,
	unsigned		transfer_flags,
	unsigned		offset)
{
	struct urb		*urb;

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb)
		return urb;
	usb_fill_bulk_urb(urb, udev, pipe, NULL, bytes, simple_callback, NULL);
	urb->interval = (udev->speed == USB_SPEED_HIGH)
			? (INTERRUPT_RATE << 3)
			: INTERRUPT_RATE;
	urb->transfer_flags = transfer_flags;
	if (usb_pipein(pipe))
		urb->transfer_flags |= URB_SHORT_NOT_OK;

	if (urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP)
		urb->transfer_buffer = usb_alloc_coherent(udev, bytes + offset,
			GFP_KERNEL, &urb->transfer_dma);
	else
		urb->transfer_buffer = kmalloc(bytes + offset, GFP_KERNEL);

	if (!urb->transfer_buffer) {
		usb_free_urb(urb);
		return NULL;
	}

	/*
	 * To test unaligned transfers add an offset and fill the
	 * unused memory with a guard value
	 */
	if (offset) {
		memset(urb->transfer_buffer, GUARD_BYTE, offset);
		urb->transfer_buffer += offset;
		if (urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP)
			urb->transfer_dma += offset;
	}

	/*
	 * For inbound transfers use guard byte so that test fails if
	 * data not correctly copied
	 */
	memset(urb->transfer_buffer,
			usb_pipein(urb->pipe) ? GUARD_BYTE : 0,
			bytes);

	return urb;
}

static struct urb *
simple_alloc_urb(struct usb_device *udev, int pipe, unsigned long bytes)
{
	return usbtest_alloc_urb(udev, pipe, bytes, URB_NO_TRANSFER_DMA_MAP, 0);
}

static inline unsigned long buffer_offset(void *buf)
{
	return (unsigned long)buf & (ARCH_KMALLOC_MINALIGN - 1);
}

static int check_guard_bytes(struct urb *urb)
{
	u8 *buf = urb->transfer_buffer;
	u8 *guard = buf - buffer_offset(buf);
	unsigned i;

	for (i = 0; guard < buf; i++, guard++) {
		if (*guard != GUARD_BYTE) {
			dev_err(&urb->dev->dev,
				"guard byte[%d] %d (not %d)\n",
				i, *guard, GUARD_BYTE);
			return -EINVAL;
		}
	}
	return 0;
}

static inline void simple_fill_buf(struct urb *urb)
{
	unsigned	i;
	u8		*buf = urb->transfer_buffer;
	unsigned	len = urb->transfer_buffer_length;

	switch (pattern) {
	default:
		/* FALLTHROUGH */
	case 0:
		memset(buf, 0, len);
		break;
	case 1:			/* mod63 */
		for (i = 0; i < len; i++)
			*buf++ = (u8) (i % 63);
		break;
	case 2:
		for (i = 0; i < len; i++)
			*buf++ = (u8) (62 - i % 63);
		break;
	}
}

static int simple_check_buf(struct urb *urb)
{
	unsigned	i;
	u8		expected;
	u8		*buf = urb->transfer_buffer;
	unsigned	len = urb->actual_length;
	int ret;

	if (!check_indata)
		return 0;

	ret = check_guard_bytes(urb);
	if (ret)
		return ret;

	dev_dbg(&urb->dev->dev, "%s: show data\n", __func__);

	/* step1: print in ep data */
	for (i = 0; i < len; i++) {
		printk("%d", buf[i]);
		if (i % 63 == 62)
			printk("\n");
	}
	printk("\n");

	/* step2: check in ep data length */

	/* step3: check in ep data pattern */
	for (i = 0; i < len; i++, buf++) {
		switch (pattern) {
		/* all-zeroes has no synchronization issues */
		case 0:
			expected = 0;
			break;
		/*
		 * mod63 stays in sync with short-terminated transfers,
		 * or otherwise when host and gadget agree on how large
		 * each usb transfer request should be.  resync is done
		 * with set_interface or set_config.
		 */
		case 1:			/* mod63 */
			expected = i % 63;
			break;
		case 2:
			expected = 62 - i % 63;
			break;
		/* always fail unsupported patterns */
		default:
			expected = !*buf;
			break;
		}
		if (*buf == expected)
			continue;

		dev_err(&urb->dev->dev,
			"buf[%d] = %d (not %d)\n", i, *buf, expected);
		return -EINVAL;
	}
	return 0;
}

static int simple_io(
	struct usbtest_dev	*tdev,
	struct urb		*urb,
	int			iterations,
	int			vary,
	int			expected,
	const char		*label
)
{
	struct usb_device	*udev = urb->dev;
	int			max = urb->transfer_buffer_length;
	struct completion	completion;
	int			retval = 0;

	urb->context = &completion;
	while (retval == 0 && iterations-- > 0) {
		init_completion(&completion);
		if (usb_pipeout(urb->pipe)) {
			simple_fill_buf(urb);
		}
		retval = usb_submit_urb(urb, GFP_KERNEL);
		if (retval != 0)
			break;

		/* NOTE:  no timeouts; can't be broken out of by interrupt */
		wait_for_completion(&completion);
		retval = urb->status;
		urb->dev = udev;
		if (retval == 0 && usb_pipein(urb->pipe))
			retval = simple_check_buf(urb);

		if (vary) {
			int	len = urb->transfer_buffer_length;

			len += vary;
			len %= max;
			if (len == 0)
				len = (vary < max) ? vary : max;
			urb->transfer_buffer_length = len;
		}

		/* FIXME if endpoint halted, clear halt (and log) */
	}
	urb->transfer_buffer_length = max;

	if (expected != retval)
		dev_err(&udev->dev,
			"%s failed, iterations left %d, status %d (not %d)\n",
				label, iterations, retval, expected);
	return retval;
}

static void simple_free_urb(struct urb *urb)
{
	unsigned long offset = buffer_offset(urb->transfer_buffer);

	if (urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP)
		usb_free_coherent(
			urb->dev,
			urb->transfer_buffer_length + offset,
			urb->transfer_buffer - offset,
			urb->transfer_dma - offset);
	else
		kfree(urb->transfer_buffer - offset);
	usb_free_urb(urb);
}

struct complete_ctx {
	spinlock_t	lock;
	u32			complete_count;
	struct completion intr_ep_complete;
};

static void usb_intr_ep_irq(struct urb *urb)
{
	int status;
	struct complete_ctx *ctx = urb->context;

	spin_lock(&ctx->lock);

	dev_dbg(&urb->dev->dev,
			"%s: %d\n", __func__, ctx->complete_count);

	switch (urb->status) {
	case 0:				/* success */
		break;
	case -ECONNRESET:	/* unlink */
	case -ENOENT:
	case -ESHUTDOWN:
		spin_unlock(&ctx->lock);
		return;
	/* -EPIPE:  should clear the halt */
	default:		/* error */
		goto resubmit;
	}

	status = simple_check_buf(urb);
	if (status < 0) {
		dev_err(&urb->dev->dev, "receive data error\n");
		complete(&ctx->intr_ep_complete);
		spin_unlock(&ctx->lock);
		return;
	}

	ctx->complete_count++;
	if (ctx->complete_count > 4) {
		complete(&ctx->intr_ep_complete);
		spin_unlock(&ctx->lock);
		return;
	}

resubmit:
	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status)
		dev_err(&urb->dev->dev, "intr urb resubmit failed\n");

	spin_unlock(&ctx->lock);
}

static int test_int_pipe(struct usb_device *udev, int pipe, unsigned long bytes)
{
	struct urb *urb;
	int maxp = usb_maxpacket(udev, pipe, usb_pipeout(pipe));
	struct usb_host_endpoint *ep;
	int status;
	struct complete_ctx ctx;

	/* bytes should be maxpacketsize 16 bytes */
	if (bytes < maxp)
		return -EINVAL;

	ep = usb_pipe_endpoint(udev, pipe);
	if (!ep)
		return -ENOMEM;

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb)
		return -ENOMEM;

	/* transfer with DMA */
	urb->transfer_flags = URB_NO_TRANSFER_DMA_MAP;
	urb->transfer_buffer =
		usb_alloc_coherent(udev, maxp, GFP_KERNEL, &urb->transfer_dma);
	if (!urb->transfer_buffer) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	/*
	 * if read packet size from IN ep is short than wMaxPacketsize
	 * then consider this transfer fail
	 */
	if (usb_pipein(pipe))
		urb->transfer_flags |= URB_SHORT_NOT_OK;

	spin_lock_init(&ctx.lock);
	ctx.complete_count = 0;
	init_completion(&ctx.intr_ep_complete);

	usb_fill_int_urb(urb, udev, pipe, urb->transfer_buffer, maxp,
			 usb_intr_ep_irq, &ctx, ep->desc.bInterval);

	/*
	 * For inbound transfers use guard byte so that test fails if
	 * data not correctly copied
	 */
	memset(urb->transfer_buffer,
			usb_pipein(urb->pipe) ? GUARD_BYTE : 0,
			maxp);

	status = usb_submit_urb(urb, GFP_KERNEL);
	if (status) {
		if (urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP)
			usb_free_coherent(urb->dev, urb->transfer_buffer_length,
				urb->transfer_buffer, urb->transfer_dma);
		usb_free_urb(urb);

		dev_err(&udev->dev, "submit intr urb failed\n");
		return status;
	}

	wait_for_completion_timeout(&ctx.intr_ep_complete, HZ * 5);

	if (ctx.complete_count < 5) {
		status = -EINVAL;
		dev_dbg(&udev->dev, "intr ep transfer failed!\n");
	} else {
		status = 0;
		dev_dbg(&udev->dev, "intr ep transfer ok!\n");
	}

	usb_kill_urb(urb);

	usb_free_coherent(urb->dev, urb->transfer_buffer_length,
				urb->transfer_buffer, urb->transfer_dma);
	usb_free_urb(urb);

	ctx.complete_count = 0;

	return status;

}

static int
usbtest_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usbtest_dev	*dev;
	int status;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	mutex_init(&dev->lock);
	dev->intf = intf;

	status = get_endpoints(dev, intf);
	if (status < 0) {
		dev_warn(&dev->intf->dev ,
				"couldn't get endpoints, %d\n", status);
		kfree(dev);
		return status;
	}

	usb_set_intfdata(intf, dev);

	dev_info(&dev->intf->dev, "%s ok!\n", __func__);

	return 0;
}

static int
usbtest_ioctl(struct usb_interface *intf, unsigned int code, void *buf)
{
	struct usbtest_dev		*dev = usb_get_intfdata(intf);
	struct usb_device		*udev = testdev_to_usbdev(dev);
	struct usbtest_param	*param = buf;
	int					retval = -EOPNOTSUPP;
	struct urb			*urb;
	struct timeval		start;

	if (code != USBTEST_REQUEST)
		return -EOPNOTSUPP;

	if (param->iterations <= 0)
		return -EINVAL;

	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

	/*
	 * when do ioctl test, don't need to set alternate setting
	 * probe has set alternate setting
	 */

	do_gettimeofday(&start);
	switch (param->test_num) {

	case 0:
		dev_info(&intf->dev, "TEST 0:  NOP\n");
		retval = 0;
		break;

	case 1:
		pattern = 1;
		if (dev->out_pipe == 0)
			break;
		dev_info(&intf->dev,
				"TEST 3:  write %d bytes %u times\n",
				param->length, param->iterations);
		urb = simple_alloc_urb(udev, dev->out_pipe, param->length);
		if (!urb) {
			retval = -ENOMEM;
			break;
		}
		/* FIRMWARE:  bulk sink (maybe accepts short writes) */
		retval = simple_io(dev, urb, param->iterations, 0, 0, "test1");
		simple_free_urb(urb);
		break;

	case 2:
		if (dev->in_pipe1 == 0)
			break;
		urb = simple_alloc_urb(udev, dev->in_pipe1, param->length);
		if (!urb) {
			retval = -ENOMEM;
			break;
		}
		/* FIRMWARE:  bulk source (maybe generates short writes) */
		retval = simple_io(dev, urb, param->iterations, 0, 0, "test2");
		simple_free_urb(urb);
		break;

	case 3:
		pattern = 2;
		if (dev->out_pipe == 0)
			break;
		dev_info(&intf->dev,
				"TEST 3:  write %d bytes %u times\n",
				param->length, param->iterations);
		urb = simple_alloc_urb(udev, dev->out_pipe, param->length);
		if (!urb) {
			retval = -ENOMEM;
			break;
		}
		/* FIRMWARE:  bulk sink (maybe accepts short writes) */
		retval = simple_io(dev, urb, param->iterations, 0, 0, "test3");
		simple_free_urb(urb);
		break;

	case 4:
		if (dev->in_pipe2 == 0)
			break;
		urb = simple_alloc_urb(udev, dev->in_pipe2, param->length);
		if (!urb) {
			retval = -ENOMEM;
			break;
		}
		/* FIRMWARE:  bulk source (maybe generates short writes) */
		retval = simple_io(dev, urb, param->iterations, 0, 0, "test4");
		simple_free_urb(urb);
		break;

	case 5:
		pattern = 1;
		if (dev->in_int_pipe1 == 0)
			break;
		dev_info(&intf->dev,
				"TEST 4:  read %d bytes %u times\n",
				param->length, param->iterations);

		retval = test_int_pipe(udev, dev->in_int_pipe1, param->length);
		break;

	case 6:
		pattern = 1;
		if (dev->in_int_pipe2 == 0)
			break;
		dev_info(&intf->dev,
				"TEST 5:  read %d bytes %u times\n",
				param->length, param->iterations);

		retval = test_int_pipe(udev, dev->in_int_pipe2, param->length);
		break;

	default:
		break;
	}
	do_gettimeofday(&param->duration);
	param->duration.tv_sec -= start.tv_sec;
	param->duration.tv_usec -= start.tv_usec;
	if (param->duration.tv_usec < 0) {
		param->duration.tv_usec += 1000 * 1000;
		param->duration.tv_sec -= 1;
	}

	mutex_unlock(&dev->lock);
	return retval;
}

static void usbtest_disconnect(struct usb_interface *intf)
{
	struct usbtest_dev	*dev = usb_get_intfdata(intf);

	usb_set_intfdata(intf, NULL);
	dev_dbg(&intf->dev, "disconnect\n");
	kfree(dev);
}

#ifdef CONFIG_PM
static int usbtest_suspend(struct usb_interface *intf, pm_message_t message)
{
	return 0;
}

static int usbtest_resume(struct usb_interface *intf)
{
	return 0;
}
#else
#define usbtest_suspend NULL
#define usbtest_resume NULL
#endif

/*
 * distinguish usb device by idVendor, idProduct,
 * not by usb class, subclass, protocol
 */
static const struct usb_device_id id_table[] = {
	{ USB_DEVICE(0x0bda, 0x3901),
	},
	{ }
};
MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver usbtest_driver = {
	.name		= "usbtest",
	.id_table		= id_table,
	.probe		= usbtest_probe,
	.unlocked_ioctl = usbtest_ioctl,
	.disconnect	= usbtest_disconnect,
	.suspend		= usbtest_suspend,
	.resume		= usbtest_resume,
};

static int __init usbtest_init(void)
{
	return usb_register(&usbtest_driver);
}
module_init(usbtest_init);

static void __exit usbtest_exit(void)
{
	usb_deregister(&usbtest_driver);
}
module_exit(usbtest_exit);

MODULE_DESCRIPTION("Realsil FT2 Host Test Driver");
MODULE_AUTHOR("lei_wang@realsil.com.cn");
MODULE_LICENSE("GPL");
