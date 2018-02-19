#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/usb/video.h>

#include "f_uvc.h"
#include <linux/usb/composite.h>

#include "android_uvc.c"

#define RTSX_UVC_VENDOR_ID		0x0bda	/* Linux Foundation */
#define RTSX_UVC_PRODUCT_ID		0x3901	/* Webcam A/V gadget */
#define RTSX_UVC_DEVICE_BCD		0x3901	/* 0.10 */

extern void bypass_usb_pullup(void);
static const char rtsx_uvc_vendor_label[] = "Generic";
static const char rtsx_uvc_product_label[] = "USB Camera";
static const char rtsx_uvc_serial_label[] = "200901010001";
static const char rtsx_uvc_config_label[] = "USB Camera";

/* string IDs are assigned dynamically */

#define CONFIG_DESCRIPTION_IDX		USB_GADGET_FIRST_AVAIL_IDX

static struct usb_string rtsx_uvc_strings[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = rtsx_uvc_vendor_label,
	[USB_GADGET_PRODUCT_IDX].s = rtsx_uvc_product_label,
	[USB_GADGET_SERIAL_IDX].s = rtsx_uvc_serial_label,
	[CONFIG_DESCRIPTION_IDX].s = rtsx_uvc_config_label,
	{}
};

static struct usb_gadget_strings rtsx_uvc_stringtab = {
	.language = 0x0409,	/* en-us */
	.strings = rtsx_uvc_strings,
};

static struct usb_gadget_strings *rtsx_uvc_device_strings[] = {
	&rtsx_uvc_stringtab,
	NULL,
};

static struct usb_device_descriptor rtsx_uvc_device_descriptor = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = cpu_to_le16(0x0200),
	.bDeviceClass = USB_CLASS_MISC,
	.bDeviceSubClass = 0x02,
	.bDeviceProtocol = 0x01,
	.bMaxPacketSize0 = 64,	/* dynamic */
	.idVendor = cpu_to_le16(RTSX_UVC_VENDOR_ID),
	.idProduct = cpu_to_le16(RTSX_UVC_PRODUCT_ID),
	.bcdDevice = cpu_to_le16(RTSX_UVC_DEVICE_BCD),
	.iManufacturer = 0,	/* dynamic */
	.iProduct = 0,		/* dynamic */
	.iSerialNumber = 0,	/* dynamic */
	.bNumConfigurations = 0,	/* dynamic */
};

static struct usb_configuration rtsx_uvc_config_driver = {
	.label = rtsx_uvc_config_label,
	.bConfigurationValue = 1,
	.iConfiguration = 0,	/* dynamic */
	.bmAttributes = USB_CONFIG_ATT_SELFPOWER,
	.MaxPower = CONFIG_USB_GADGET_VBUS_DRAW,
};

static int __init rtsx_uvc_config_bind(struct usb_configuration *c)
{
	return uvc_bind_config(c, (const struct uvc_descriptor_header * const *)
			       uvc_fs_control_cls, NULL, NULL, NULL, NULL);

}

static int /* __init_or_exit */ rtsx_uvc_unbind(struct usb_composite_dev *cdev)
{
	return 0;
}

static int __init rtsx_uvc_bind(struct usb_composite_dev *cdev)
{
	int ret;
	/* Register our configuration. */

	ret = usb_string_ids_tab(cdev, rtsx_uvc_strings);
	if (ret < 0)
		goto error;

	rtsx_uvc_device_descriptor.iManufacturer =
	    rtsx_uvc_strings[USB_GADGET_MANUFACTURER_IDX].id;
	rtsx_uvc_device_descriptor.iProduct =
	    rtsx_uvc_strings[USB_GADGET_PRODUCT_IDX].id;
	rtsx_uvc_device_descriptor.iSerialNumber =
	    rtsx_uvc_strings[USB_GADGET_SERIAL_IDX].id;
	rtsx_uvc_config_driver.iConfiguration =
	    rtsx_uvc_strings[CONFIG_DESCRIPTION_IDX].id;

	ret = rtsx_uvc_add_config(cdev, &rtsx_uvc_config_driver,
				  rtsx_uvc_config_bind);
	if (ret < 0)
		goto error;

	INFO(cdev, "rtsx_uvc Video Gadget\n");
	return 0;

error:
	rtsx_uvc_unbind(cdev);
	return ret;
}

/* --------------------------------------------------------------------------
 * Driver
 */

static __refdata struct usb_composite_driver rtsx_uvc_driver = {
	.name = "rtsx_uvc",
	.dev = &rtsx_uvc_device_descriptor,
	.strings = rtsx_uvc_device_strings,
	.max_speed = USB_SPEED_HIGH,
	.bind = rtsx_uvc_bind,
	.unbind = rtsx_uvc_bind,
};

static int __init rtsx_uvc_init(void)
{
	int ret;
	bypass_usb_pullup();
	rtsx_uvc_function_init();
	rtsx_uvc_function_enable();

	ret = usb_composite_probe(&rtsx_uvc_driver);

	return ret;
}

static void __exit rtsx_uvc_cleanup(void)
{
	rtsx_uvc_function_cleanup();
	usb_composite_unregister(&rtsx_uvc_driver);
}

module_init(rtsx_uvc_init);
module_exit(rtsx_uvc_cleanup);
