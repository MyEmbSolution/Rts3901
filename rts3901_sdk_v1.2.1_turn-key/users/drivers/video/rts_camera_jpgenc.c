/*
 * Mjpeg encoder device driver (kernel module)
 *
 * Realtek Semiconductor Corp.
 *
 * rts_camera_jpgenc.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/rts_sysctl.h>
#include <linux/delay.h>
#include "linux/rts_camera_jpgenc.h"
#include "rts_camera.h"
#include "rts_hw_id.h"

#define RTS_JPG_ENC_DRV_NAME		"rts_jpgenc"
#define RTS_JPG_ENC_DEV_NAME		"rtsjpgenc"

#define RTS_REG_MJPEG_ISP_BUF_CONFIG		0x0000004c
#define RTS_REG_INT_EN_JPEG_TO_HOST		0x00000060
#define RTS_REG_INT_FLAG_JPEG_TO_HOST		0x00000064
#define RTS_REG_MJPEG_ENC_VERSION		0x00000100
#define RTS_REG_MJPEG_READ_INTERVAL		0x00000050

#define RTS_JPG_CLOCK_ON_NEED			1

struct rtscam_jpg_enc {
	struct device *dev;
	struct clk *clk;
	void __iomem *hwregs;
	unsigned long iobaseaddr;
	unsigned int iosize;

	int irq_enable;

	struct rtscam_ge_device *jdev;
	atomic_t use_count;

	unsigned long clk_rate;

	struct mutex lock;

	char name[PLATFORM_NAME_SIZE];
	kernel_ulong_t devtype;
};

static int rtscam_jpgenc_enable_clk(struct rtscam_jpg_enc *rjpgenc, int enable)
{
	struct clk *clk = rjpgenc->clk;

	if (IS_ERR(clk)) {
		rtsprintk(RTS_TRACE_ERROR, "jpgenc:get clk fail\n");
		return -EINVAL;
	}

	if (enable) {
		rts_sys_force_reset(FORCE_RESET_JPG);
		udelay(1);
		clk_set_rate(clk, rjpgenc->clk_rate);
		clk_prepare_enable(clk);
		rjpgenc->clk_rate = clk_get_rate(clk);
	} else {
		clk_disable_unprepare(clk);
	}

	return 0;
}

static int rtscam_jpgenc_read_reg(struct rtscam_jpg_enc *rjpgenc, off_t reg)
{
	return le32_to_cpu(ioread32(rjpgenc->hwregs + reg));
}

static void rtscam_jpgenc_write_reg(struct rtscam_jpg_enc *rjpgenc,
		u32 value, off_t reg)
{
	iowrite32(cpu_to_le32(value), rjpgenc->hwregs + reg);
}

static int rtscam_jpgenc_reserve_io(struct rtscam_jpg_enc *rjpgenc)
{
	u32 ver_r = rtscam_jpgenc_read_reg(rjpgenc, RTS_REG_MJPEG_ENC_VERSION);
	u32 ver_t = 0x01;

	if ((ver_r & 0xf) != ver_t) {
		rtsprintk(RTS_TRACE_ERROR, "jpgenc:HW not found\n");
		return -EINVAL;
	}

	return 0;
}

static int rtscam_jpgenc_enable_interrupt(struct rtscam_jpg_enc *rjpgenc,
		int enable)
{
	u32 int_en;
	u32 int_f = 0xffffffff;

	if (enable)
		int_en = 0xffffffff;
	else
		int_en = 0;

	rtscam_jpgenc_write_reg(rjpgenc, int_en, RTS_REG_INT_EN_JPEG_TO_HOST);
	rtscam_jpgenc_write_reg(rjpgenc, int_f, RTS_REG_INT_FLAG_JPEG_TO_HOST);

	return 0;
}

static int rtscam_jpgenc_config_isp_buffer(struct rtscam_jpg_enc *rjpgenc)
{
	u32 start = 0;
	u32 size = 0x100;

	rtscam_jpgenc_write_reg(rjpgenc, ((start<<3) + (size<<19)),
			RTS_REG_MJPEG_ISP_BUF_CONFIG);

	return 0;
}

static irqreturn_t rtscam_jpgenc_irq(int irq, void *data)
{
	struct rtscam_jpg_enc *rjpgenc = data;
	u32 status;
	u32 mask;
	const off_t reg = RTS_REG_INT_FLAG_JPEG_TO_HOST;

	status = rtscam_jpgenc_read_reg(rjpgenc, reg);

	/*isp buffer overflow*/
	mask = 0x1;
	if (status & mask) {
		rtsprintk(RTS_TRACE_ERROR, "jpgenc:isp buffer overflow\n");
		rtscam_jpgenc_write_reg(rjpgenc, mask, reg);
		return IRQ_HANDLED;
	}

	/*mjpeg frame buffer overflow*/
	mask = 0x2;
	if (status & mask) {
		rtsprintk(RTS_TRACE_ERROR,
				"jpgenc:mjpeg frame buffer overflow\n");
		rtscam_jpgenc_write_reg(rjpgenc, mask, reg);
		return IRQ_HANDLED;
	}

	/*encode finish*/
	mask = 0x4;
	if(status & mask) {
		rtsprintk(RTS_TRACE_DEBUG,
				"jpgenc:mjpeg encode finish\n");
		rtscam_jpgenc_write_reg(rjpgenc, mask, reg);
		rtscam_ge_kill_fasync(rjpgenc->jdev, SIGIO, POLL_IN);
		return IRQ_HANDLED;
	}

	/*Reserved*/
	mask = 0xffffffff<<3;
	if(status & mask)
		rtscam_jpgenc_write_reg(rjpgenc, mask, reg);

	return IRQ_HANDLED;
}

static int rtscam_jpgenc_open(struct file *filp)
{
	struct rtscam_ge_device *gdev = rtscam_devdata(filp);
	struct rtscam_jpg_enc *rjpgenc = rtscam_ge_get_drvdata(gdev);

	filp->private_data = rjpgenc;

#if RTS_JPG_CLOCK_ON_NEED
	mutex_lock(&rjpgenc->lock);
	if (atomic_inc_return(&rjpgenc->use_count) == 1) {
		rtscam_jpgenc_enable_clk(rjpgenc, 1);
		rtscam_jpgenc_enable_interrupt(rjpgenc, rjpgenc->irq_enable);
		rtscam_jpgenc_config_isp_buffer(rjpgenc);
	}
	mutex_unlock(&rjpgenc->lock);
#endif

	return 0;
}

static int rtscam_jpgenc_close(struct file *filp)
{
	struct rtscam_jpg_enc *rjpgenc = filp->private_data;

	filp->private_data = NULL;

	if (!rjpgenc)
		return -EINVAL;

#if RTS_JPG_CLOCK_ON_NEED
	mutex_lock(&rjpgenc->lock);
	if (atomic_dec_return(&rjpgenc->use_count) == 0)
		rtscam_jpgenc_enable_clk(rjpgenc, 0);
	mutex_unlock(&rjpgenc->lock);
#endif

	return 0;
}

static long rtscam_jpgenc_do_ioctl(struct file *filp, unsigned int cmd,
		void *arg)
{
	struct rtscam_jpg_enc *rjpgenc = filp->private_data;
	int err = 0;

	if(_IOC_TYPE(cmd) != RTSJPGENC_IOC_MAGIC)
		return -ENOTTY;
	if(_IOC_NR(cmd) > RTSJPGENC_IOC_MAXNR)
		return -ENOTTY;

	switch (cmd) {
	case RTSJPGENC_IOCGHWOFFSET:
		*(unsigned long *)arg = rjpgenc->iobaseaddr;
		break;
	case RTSJPGENC_IOCGHWIOSIZE:
		*(unsigned int *)arg = rjpgenc->iosize;
		break;
	case RTSJPGENC_IOCHWRESET:
		rts_sys_force_reset(FORCE_RESET_JPG);
		break;
	default:
		rtsprintk(RTS_TRACE_ERROR,
				"unknown[rtsjpgenc] ioctl 0x%08x, '%c' 0x%x\n",
				cmd, _IOC_TYPE(cmd), _IOC_NR(cmd));
		err = -ENOTTY;
		break;
	}

	return err;
}

static long rtscam_jpgenc_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	return rtscam_usercopy(filp, cmd, arg, rtscam_jpgenc_do_ioctl);
}

static struct rtscam_ge_file_operations rtscam_jpgenc_fops = {
	.owner		= THIS_MODULE,
	.open		= rtscam_jpgenc_open,
	.release	= rtscam_jpgenc_close,
	.unlocked_ioctl	= rtscam_jpgenc_ioctl,
};

static int __create_device(struct rtscam_jpg_enc *rjpgenc)
{
	struct rtscam_ge_device *gdev;
	int ret;

	if (rjpgenc->jdev)
		return 0;

	gdev = rtscam_ge_device_alloc();
	if (!gdev)
		return -ENOMEM;

	strlcpy(gdev->name, RTS_JPG_ENC_DEV_NAME, sizeof(gdev->name));
	gdev->parent = get_device(rjpgenc->dev);
	gdev->release = rtscam_ge_device_release;
	gdev->fops = &rtscam_jpgenc_fops;

	rtscam_ge_set_drvdata(gdev, rjpgenc);
	ret = rtscam_ge_register_device(gdev);
	if (ret) {
		rtscam_ge_device_release(gdev);
		return ret;
	}

	rjpgenc->jdev = gdev;

	return 0;
}

static void __remove_device(struct rtscam_jpg_enc *rjpgenc)
{
	struct rtscam_ge_device *gdev;

	if (!rjpgenc->jdev)
		return;

	gdev = rjpgenc->jdev;
	put_device(gdev->parent);
	rtscam_ge_unregister_device(gdev);
}

static ssize_t rtscam_jpgenc_get_clk_rate(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct rtscam_jpg_enc *rjpgenc = dev_get_drvdata(dev);
	int num = 0;

	num += scnprintf(buf, PAGE_SIZE, "%lu\n", rjpgenc->clk_rate);

	return num;
}

static ssize_t rtscam_jpgenc_set_clk_rate(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long rate;
	struct rtscam_jpg_enc *rjpgenc = dev_get_drvdata(dev);

	sscanf(buf, "%lu", &rate);

	if (rate)
		rjpgenc->clk_rate = rate;

	return count;
}

static DEVICE_ATTR(clock, S_IRUGO | S_IWUGO,
		rtscam_jpgenc_get_clk_rate, rtscam_jpgenc_set_clk_rate);

int rtscam_jpgenc_init_clk(struct rtscam_jpg_enc *rjpgenc)
{
	rjpgenc->clk = devm_clk_get(rjpgenc->dev, "jpeg_ck");
	if (IS_ERR(rjpgenc->clk)) {
		rtsprintk(RTS_TRACE_ERROR, "Get jpgenc clk failed\n");
		return PTR_ERR(rjpgenc->clk);
	}

	if (TYPE_FPGA & rjpgenc->devtype) {
		clk_set_parent(rjpgenc->clk, clk_get(NULL, "usb_pll_2"));
		rjpgenc->clk_rate = 60000000;
	} else {
		clk_set_parent(rjpgenc->clk, clk_get(NULL, "sys_pll0"));
		rjpgenc->clk_rate = 240000000;
	}

	return 0;
}

int rtscam_jpgenc_probe(struct platform_device *pdev)
{
	struct rtscam_jpg_enc *rjpgenc;
	struct resource *res;
	void __iomem *base;
	int irq;
	const struct platform_device_id *id_entry;
	int err = 0;

	rtsprintk(RTS_TRACE_INFO, "%s\n", __func__);

	id_entry = platform_get_device_id(pdev);
	if (!id_entry) {
		rtsprintk(RTS_TRACE_ERROR, "not support soc mjpeg\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(pdev, 0);

	if (NULL == res) {
		rtsprintk(RTS_TRACE_ERROR, "Missing platform resource data\n");
		return -ENODEV;
	}

	rjpgenc = devm_kzalloc(&pdev->dev, sizeof(*rjpgenc), GFP_KERNEL);
	if (NULL == rjpgenc) {
		rtsprintk(RTS_TRACE_ERROR,
			"Couldn't allocate rts camera jpgenc object\n");
		return -ENOMEM;
	}
	rjpgenc->dev = get_device(&pdev->dev);
	atomic_set(&rjpgenc->use_count, 0);
	mutex_init(&rjpgenc->lock);
	strncpy(rjpgenc->name, id_entry->name, PLATFORM_NAME_SIZE);
	rjpgenc->devtype = id_entry->driver_data;

	err = rtscam_jpgenc_init_clk(rjpgenc);
	if (err)
		return err;

	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base)) {
		rtsprintk(RTS_TRACE_ERROR, "Couldn't ioremap resource\n");
		return PTR_ERR(base);
	}
	rjpgenc->hwregs = base;
	rjpgenc->iobaseaddr = res->start;
	rjpgenc->iosize = resource_size(res);

	if (irq > 0) {
		err = devm_request_irq(rjpgenc->dev, irq, rtscam_jpgenc_irq,
				IRQF_SHARED, RTS_JPG_ENC_DRV_NAME, rjpgenc);
		if (err) {
			rtsprintk(RTS_TRACE_ERROR,
					"jpgenc:request irq fail\n");
			return err;
		}
		rjpgenc->irq_enable = 1;
	}

	rtscam_jpgenc_enable_clk(rjpgenc, 1);
	err = rtscam_jpgenc_reserve_io(rjpgenc);
#if RTS_JPG_CLOCK_ON_NEED
	rtscam_jpgenc_enable_clk(rjpgenc, 0);
#endif
	if (err) {
		rtsprintk(RTS_TRACE_ERROR, "jpgenc: Reserve IO fail\n");
		return err;
	}
	__create_device(rjpgenc);

	platform_set_drvdata(pdev, rjpgenc);
	device_create_file(rjpgenc->dev, &dev_attr_clock);

	return 0;
}

int rtscam_jpgenc_remove(struct platform_device *pdev)
{
	struct rtscam_jpg_enc *rjpgenc = platform_get_drvdata(pdev);

	device_remove_file(rjpgenc->dev, &dev_attr_clock);
	__remove_device(rjpgenc);
#if !RTS_JPG_CLOCK_ON_NEED
	rtscam_jpgenc_enable_clk(rjpgenc, 0);
#endif
	put_device(rjpgenc->dev);
	rjpgenc->dev = NULL;

	return 0;
}

static struct platform_device_id rtsx_soc_mjpeg_devtypes[] = {
	{
		.name = "rle0745-fpga-mjpeg",
		.driver_data = TYPE_RLE0745 | TYPE_FPGA
	},
	{
		.name = "rle0745-mjpeg",
		.driver_data = TYPE_RLE0745
	},
	{
		.name = "rts3901-fpga-mjpeg",
		.driver_data = TYPE_RTS3901 | TYPE_FPGA
	},
	{
		.name = "rts3901-mjpeg",
		.driver_data = TYPE_RTS3901
	},
	{
		.name = "rts3903-fpga-mjpeg",
		.driver_data = TYPE_RTS3903 | TYPE_FPGA
	},
	{
		.name = "rts3903-mjpeg",
		.driver_data = TYPE_RTS3903
	},
	{}
};

static struct platform_driver rtscam_jpgenc_driver = {
	.driver		= {
		.name	= RTS_JPG_ENC_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= rtscam_jpgenc_probe,
	.remove		= rtscam_jpgenc_remove,
	.id_table	= rtsx_soc_mjpeg_devtypes,
};

module_platform_driver(rtscam_jpgenc_driver);

MODULE_DESCRIPTION("Realsil Mjpeg encoder device driver");
MODULE_AUTHOR("Ming Qian <ming_qian@realsil.com.cn>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1.1");
MODULE_ALIAS("platform:" RTS_JPG_ENC_DRV_NAME);

