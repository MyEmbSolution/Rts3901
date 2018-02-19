/*
20150128
lei_wang (lei_wang@realsil.com.cn)

adb driver
*/
#include <linux/kernel.h>
#include <linux/usb/ch9.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>

#include "f_adb.c"

#define ADB_VENDOR_ID		0x18d1
#define ADB_PRODUCT_ID		0x0001
#define DRIVER_DESC		"android adb"
#define DRIVER_VERSION	"2015/01/28"

static struct usb_device_descriptor adb_device_desc = {
	.bLength			=	sizeof adb_device_desc,
	.bDescriptorType	=	USB_DT_DEVICE,
	.bcdUSB			=	cpu_to_le16(0x0200),
	.bDeviceClass		=	USB_CLASS_PER_INTERFACE,

	/* Vendor and product id can be overridden by module parameters.  */
	.idVendor			=	cpu_to_le16(ADB_VENDOR_ID),
	.idProduct		=	cpu_to_le16(ADB_PRODUCT_ID),
	.bNumConfigurations =	1,
};

static struct usb_string strings_dev[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = "realsil",
	[USB_GADGET_PRODUCT_IDX].s = DRIVER_DESC,
	[USB_GADGET_SERIAL_IDX].s = "01234567890123456789",
	{  } /* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language       = 0x0409,       /* en-us */
	.strings        = strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_configuration adb_config_driver = {
	.label			= "Android adb device",
	.bConfigurationValue	= 1,
	.bmAttributes		= USB_CONFIG_ATT_SELFPOWER,
};

static int __init adb_bind(struct usb_composite_dev *cdev)
{
	int status;

	status = adb_setup();
	if (status != 0)
		return status;

	status = usb_string_ids_tab(cdev, strings_dev);
	if (status < 0)
		return status;

	adb_device_desc.iManufacturer = strings_dev[USB_GADGET_MANUFACTURER_IDX].id;
	adb_device_desc.iProduct = strings_dev[USB_GADGET_PRODUCT_IDX].id;
	adb_device_desc.iSerialNumber = strings_dev[USB_GADGET_SERIAL_IDX].id;

	status = usb_add_config(cdev, &adb_config_driver, adb_bind_config);
	if (status < 0)
		return status;

	dev_info(&cdev->gadget->dev, DRIVER_DESC ", version: " DRIVER_VERSION "\n");

	return 0;
}

static int __exit adb_unbind(struct usb_composite_dev *cdev)
{
	adb_cleanup();
	return 0;
}

static __refdata struct usb_composite_driver adb_driver = {
	.name		= "g_adb",
	.dev			= &adb_device_desc,
	.max_speed	= USB_SPEED_HIGH,
	.needs_serial	= 1,
	.strings		= dev_strings,
	.bind		= adb_bind,
	.unbind		= adb_unbind,
};

static int __init adb_module_init(void)
{
	return usb_composite_probe(&adb_driver);
}

static void __exit adb_module_exit(void)
{
	usb_composite_unregister(&adb_driver);
}

module_init(adb_module_init);
module_exit(adb_module_exit);


MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Realsil");
MODULE_LICENSE("GPL");
