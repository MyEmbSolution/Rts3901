/*
 * Realtek Semiconductor Corp.
 *
 * rts_camera_hx280enc.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 * Ming Qian, Realsil Software Engineering, <ming_qian@realsil.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * 
--------------------------------------------------------------------------------
--
--  Abstract : 6280/7280/8270/8290/H1 Encoder device driver (kernel module)
--
------------------------------------------------------------------------------
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/rts_sysctl.h>
#include <linux/delay.h>
#include "linux/rts_camera_hx280enc.h"
#include "rts_camera.h"
#include "rts_hw_id.h"

#define RTS_H264_HX280_DRV_NAME		"rts_h264_hx280enc"
#define RTS_H264_HX280_DEV_NAME		"rtshx280enc"

#define ENC_HW_ID1			0x62800000
#define ENC_HW_ID2			0x72800000
#define ENC_HW_ID3			0x82700000
#define ENC_HW_ID4			0x82900000
#define ENC_HW_ID5			0x48310000

#define RTS_H264_CLOCK_ON_NEED		1

struct rtscam_hx280_enc {
	struct device *dev;
	struct clk *clk;
	void __iomem *hwregs;
	unsigned long iobaseaddr;
	unsigned int iosize;

	struct rtscam_ge_device *hdev;
	atomic_t use_count;

	int clk_state;
	unsigned long clk_rate;

	struct mutex lock;

	char name[PLATFORM_NAME_SIZE];
	kernel_ulong_t devtype;
};

static int rtscam_hx280_enable_clk(struct rtscam_hx280_enc *rh264enc,
		int enable)
{
	struct clk *clk = rh264enc->clk;

	if (IS_ERR(clk)) {
		rtsprintk(RTS_TRACE_ERROR, "hx280enc:get h264 clk fail\n");
		return -EINVAL;
	}

	if (enable) {
		if (RTSCAM_STATE_ACTIVE == rh264enc->clk_state)
			return 0;

		rts_sys_force_reset(FORCE_RESET_H264);
		udelay(1);

		clk_set_rate(clk, rh264enc->clk_rate);
		clk_prepare_enable(clk);

		rh264enc->clk_state = RTSCAM_STATE_ACTIVE;
		rtsprintk(RTS_TRACE_DEBUG, "enable h264 clock\n");
		rh264enc->clk_rate = clk_get_rate(clk);
	} else {
		if (RTSCAM_STATE_PASSIVE == rh264enc->clk_state)
			return 0;

		rtsprintk(RTS_TRACE_DEBUG, "disable h264 clock\n");
		clk_disable_unprepare(clk);

		rh264enc->clk_state = RTSCAM_STATE_PASSIVE;
	}

	return 0;
}

static void rtscam_hx280_reset_asic(struct rtscam_hx280_enc *rh264enc)
{
	int i;

	iowrite32(cpu_to_le32(0), rh264enc->hwregs + 0x38);

	for (i = 4; i < rh264enc->iosize; i += 4)
		iowrite32(cpu_to_le32(0), rh264enc->hwregs + i);
}

static int rtscam_hx280_open(struct file *filp)
{
	struct rtscam_ge_device *gdev = rtscam_devdata(filp);
	struct rtscam_hx280_enc *rh264enc = rtscam_ge_get_drvdata(gdev);

#if RTS_H264_CLOCK_ON_NEED
	mutex_lock(&rh264enc->lock);
	if (atomic_inc_return(&rh264enc->use_count) == 1) {
		rtscam_hx280_enable_clk(rh264enc, 1);
		rtscam_hx280_reset_asic(rh264enc);
	}
	mutex_unlock(&rh264enc->lock);
#endif

	filp->private_data = rh264enc;
	return 0;
}

static int rtscam_hx280_close(struct file *filp)
{
	struct rtscam_hx280_enc *rh264enc = filp->private_data;

	filp->private_data = NULL;

	if (!rh264enc)
		return -EINVAL;

#if RTS_H264_CLOCK_ON_NEED
	mutex_lock(&rh264enc->lock);
	if (atomic_dec_return(&rh264enc->use_count) == 0) {
		rtscam_hx280_enable_clk(rh264enc, 0);
	}
	mutex_unlock(&rh264enc->lock);
#endif

	return 0;
}

static long rtscam_hx280_do_ioctl(struct file *filp, unsigned int cmd,
		void *arg)
{
	struct rtscam_hx280_enc *rh264enc = filp->private_data;
	int err = 0;

	if (!rh264enc)
		return -EINVAL;

	rtsprintk(RTS_TRACE_DEBUG, "ioctl cmd 0x%08x, '%c' %d\n",
			cmd, _IOC_TYPE(cmd), _IOC_NR(cmd));

	/*
	 * extract the type and number bitfields, and don't encode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if(_IOC_TYPE(cmd) != HX280ENC_IOC_MAGIC)
		return -ENOTTY;
	if(_IOC_NR(cmd) > HX280ENC_IOC_MAXNR)
		return -ENOTTY;

	switch (cmd) {
	case HX280ENC_IOCGHWOFFSET:
		*(unsigned long *)arg = rh264enc->iobaseaddr;
		break;
	case HX280ENC_IOCGHWIOSIZE:
		*(unsigned int *)arg = rh264enc->iosize;
		break;
	default:
		rtsprintk(RTS_TRACE_ERROR,
				"unknown[rtsmem] ioctl 0x%08x, '%c' 0x%x\n",
				cmd, _IOC_TYPE(cmd), _IOC_NR(cmd));
		err = -ENOTTY;
		break;
	}

	return err;
}

static long rtscam_hx280_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	return rtscam_usercopy(filp, cmd, arg, rtscam_hx280_do_ioctl);
}
static struct rtscam_ge_file_operations rtscam_hx280_fops = {
	.owner		= THIS_MODULE,
	.open		= rtscam_hx280_open,
	.release	= rtscam_hx280_close,
	.unlocked_ioctl	= rtscam_hx280_ioctl,
};

static int __create_device(struct rtscam_hx280_enc *rh264enc)
{
	struct rtscam_ge_device *gdev;
	int ret;

	if (rh264enc->hdev)
		return 0;

	gdev = rtscam_ge_device_alloc();
	if (!gdev)
		return -ENOMEM;

	strlcpy(gdev->name, RTS_H264_HX280_DEV_NAME, sizeof(gdev->name));
	gdev->parent = get_device(rh264enc->dev);
	gdev->release = rtscam_ge_device_release;
	gdev->fops = &rtscam_hx280_fops;

	rtscam_ge_set_drvdata(gdev, rh264enc);
	ret = rtscam_ge_register_device(gdev);
	if (ret) {
		rtscam_ge_device_release(gdev);
		return ret;
	}

	rh264enc->hdev = gdev;

	return 0;
}

static void __remove_device(struct rtscam_hx280_enc *rh264enc)
{
	struct rtscam_ge_device *gdev;

	if (!rh264enc->hdev)
		return;

	gdev = rh264enc->hdev;
	put_device(gdev->parent);
	rtscam_ge_unregister_device(gdev);
}

static irqreturn_t rtscam_hx280_irq(int irq, void *data)
{
	struct rtscam_hx280_enc *rh264enc = data;
	u32 irq_status;

	irq_status = le32_to_cpu(ioread32(rh264enc->hwregs + 0x04));

	if (irq_status & 0x01) {
		iowrite32(cpu_to_le32(irq_status & (~0x01)),
				rh264enc->hwregs + 0x04);

		/* Handle slice ready interrupts. The reference implementation
		 * doesn't signal slice ready interrupts to EWL.
		 * The EWL will poll the slices ready register value. */
		if ((irq_status & 0x1FE) == 0x100) {
			rtsprintk(RTS_TRACE_DEBUG,
					"Slice ready IRQ handled!\n");
			return IRQ_HANDLED;
		}

		/* All other interrupts will be signaled to EWL. */
		rtscam_ge_kill_fasync(rh264enc->hdev, SIGIO, POLL_IN);

		return IRQ_HANDLED;
	}

	rtsprintk(RTS_TRACE_DEBUG, "IRQ received, but NOT handled!\n");
	return IRQ_NONE;
}

static int rtscam_hx280_reserve_io(struct rtscam_hx280_enc *rh264enc)
{
	u32 hwid = le32_to_cpu(ioread32(rh264enc->hwregs));

	if((((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID1 >> 16) & 0xFFFF)) &&
		(((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID2 >> 16) & 0xFFFF)) &&
		(((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID3 >> 16) & 0xFFFF)) &&
		(((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID4 >> 16) & 0xFFFF)) &&
		(((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID5 >> 16) & 0xFFFF))) {
		rtsprintk(RTS_TRACE_ERROR,
			"hx280enc:HW not found at 0x%08lx, hwid = 0x%08x\n",
			rh264enc->iobaseaddr, hwid);
		return -EBUSY;
	}

	rtsprintk(RTS_TRACE_ERROR,
			"hx280enc:HW at base <0x%08lx> with ID <0x%08x>\n",
			rh264enc->iobaseaddr, hwid);

	return 0;
}

static ssize_t rtscam_hx280_get_clk_rate(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct rtscam_hx280_enc *rh264enc = dev_get_drvdata(dev);
	int num = 0;

	num += scnprintf(buf, PAGE_SIZE, "%lu\n", rh264enc->clk_rate);

	return num;
}

static ssize_t rtscam_hx280_set_clk_rate(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long rate;
	struct rtscam_hx280_enc *rh264enc = dev_get_drvdata(dev);

	sscanf(buf, "%lu", &rate);

	if (rate)
		rh264enc->clk_rate = rate;

	return count;
}

static DEVICE_ATTR(clock, S_IRUGO | S_IWUGO,
		rtscam_hx280_get_clk_rate, rtscam_hx280_set_clk_rate);

static int rtscam_hx280_init_clk(struct rtscam_hx280_enc *rh264enc)
{
	rh264enc->clk = devm_clk_get(rh264enc->dev, "h264_ck");
	if (IS_ERR(rh264enc->clk)) {
		rtsprintk(RTS_TRACE_ERROR, "Get h264 clk failed\n");
		return PTR_ERR(rh264enc->clk);
	}

	if (TYPE_FPGA & rh264enc->devtype) {
		clk_set_parent(rh264enc->clk, clk_get(NULL, "usb_pll_2"));
		rh264enc->clk_rate = 60000000;
	} else {
		if (RTS_SOC_HW_ID.hw_ver == HW_ID_VER_RTS3901 &&
				RTS_SOC_HW_ID.hw_rev == HW_ID_REV_A) {
			clk_set_parent(rh264enc->clk, clk_get(NULL, "usb_pll"));
			rh264enc->clk_rate = 240000000;
		} else {
			clk_set_parent(rh264enc->clk, clk_get(NULL, "sys_pll0"));
			rh264enc->clk_rate = 400000000;
		}
	}

	return 0;
}

int rtscam_hx280_probe(struct platform_device *pdev)
{
	struct rtscam_hx280_enc *rh264enc;
	struct resource *res;
	void __iomem *base;
	int irq;
	const struct platform_device_id *id_entry;
	int err = 0;

	rtsprintk(RTS_TRACE_INFO, "%s\n", __func__);

	id_entry = platform_get_device_id(pdev);
	if (!id_entry) {
		rtsprintk(RTS_TRACE_ERROR, "not support soc h264\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(pdev, 0);

	if (NULL == res || irq < 0) {
		rtsprintk(RTS_TRACE_ERROR, "Missing platform resource data\n");
		return -ENODEV;
	}

	rh264enc = devm_kzalloc(&pdev->dev, sizeof(*rh264enc), GFP_KERNEL);
	if (NULL == rh264enc) {
		rtsprintk(RTS_TRACE_ERROR,
			"Couldn't allocate rts camera h264enc object\n");
		return -ENOMEM;
	}

	rh264enc->dev = get_device(&pdev->dev);
	atomic_set(&rh264enc->use_count, 0);
	rh264enc->clk_state = RTSCAM_STATE_PASSIVE;
	mutex_init(&rh264enc->lock);
	strncpy(rh264enc->name, id_entry->name, PLATFORM_NAME_SIZE);
	rh264enc->devtype = id_entry->driver_data;

	err = rtscam_hx280_init_clk(rh264enc);
	if (err)
		return err;

	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base)) {
		rtsprintk(RTS_TRACE_ERROR, "Couldn't ioremap resource\n");
		return PTR_ERR(base);
	}
	rh264enc->hwregs = base;
	rh264enc->iobaseaddr = res->start;
	rh264enc->iosize = resource_size(res);

	err = devm_request_irq(rh264enc->dev, irq, rtscam_hx280_irq,
			IRQF_SHARED, RTS_H264_HX280_DRV_NAME, rh264enc);
	if (err) {
		rtsprintk(RTS_TRACE_ERROR, "request irq failed\n");
		return err;
	}

	rtscam_hx280_enable_clk(rh264enc, 1);
	err = rtscam_hx280_reserve_io(rh264enc);
	if (!err)
		rtscam_hx280_reset_asic(rh264enc);
#if RTS_H264_CLOCK_ON_NEED
	rtscam_hx280_enable_clk(rh264enc, 0);
#endif

	if (err) {
		rtsprintk(RTS_TRACE_ERROR, "hx280enc: Reserve IO fail\n");
		return err;
	}
	__create_device(rh264enc);

	platform_set_drvdata(pdev, rh264enc);
	device_create_file(rh264enc->dev, &dev_attr_clock);

	return 0;
}

int rtscam_hx280_remove(struct platform_device *pdev)
{
	struct rtscam_hx280_enc *rh264enc = platform_get_drvdata(pdev);

	device_remove_file(rh264enc->dev, &dev_attr_clock);
	__remove_device(rh264enc);
#if !RTS_H264_CLOCK_ON_NEED
	rtscam_hx280_enable_clk(rh264enc, 0);
#endif
	put_device(rh264enc->dev);
	rh264enc->dev = NULL;

	return 0;
}

static struct platform_device_id rtsx_soc_h264_devtypes[] = {
	{
		.name = "rle0745-fpga-h264",
		.driver_data = TYPE_RLE0745 | TYPE_FPGA
	},
	{
		.name = "rle0745-h264",
		.driver_data = TYPE_RLE0745
	},
	{
		.name = "rts3901-fpga-h264",
		.driver_data = TYPE_RTS3901 | TYPE_FPGA
	},
	{
		.name = "rts3901-h264",
		.driver_data = TYPE_RTS3901
	},
	{
		.name = "rts3903-fpga-h264",
		.driver_data = TYPE_RTS3903 | TYPE_FPGA
	},
	{
		.name = "rts3903-h264",
		.driver_data = TYPE_RTS3903
	},
	{}
};

static struct platform_driver rtscam_hx280_driver = {
	.driver		= {
		.name	= RTS_H264_HX280_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= rtscam_hx280_probe,
	.remove		= rtscam_hx280_remove,
	.id_table	= rtsx_soc_h264_devtypes,
};

module_platform_driver(rtscam_hx280_driver);

MODULE_DESCRIPTION("Realsil H264 encoder device driver");
MODULE_AUTHOR("Ming Qian <ming_qian@realsil.com.cn>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1.1");
MODULE_ALIAS("platform:" RTS_H264_HX280_DRV_NAME);

