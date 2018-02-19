/*
 * rtp-rtc.c  --  Realsil RTP
 *
 * Copyright 2013 Realsil Semiconductor Corp.
 *
 * Author: Wind Han <wind_han@realsil.com.cn>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/slab.h>
#include <linux/bcd.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/completion.h>
#include <linux/mfd/rtp-mfd.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#define ALL_TIME_REGS				8
#define ALL_ALM_REGS				6

#define RTC_SET_TIME_RETRIES			5
#define RTC_GET_TIME_RETRIES			5

struct rtp_rtc {
	struct rtp_mfd_chip *chip;
	struct rtc_device *rtc;
	struct notifier_block nb;
	unsigned int alarm_enabled:1;
};

/*
 * Read current time and date in RTC
 */
static int rtp_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct rtp_rtc *rtp_rtc = dev_get_drvdata(dev);
	struct rtp_mfd_chip *chip = rtp_rtc->chip;
	int ret;
	int count = 0;
	uint8_t rtc_data[ALL_TIME_REGS + 1];

	printk(KERN_DEBUG "Read rtp rtc time\n");
	/* Read twice to make sure we don't read a corrupt, partially
	 * incremented, value.
	 */
	do {
		ret = rtp_bulk_read(chip, RTP_REG_CAL_SEC,
					ALL_TIME_REGS, rtc_data);
		if (ret != 0)
			continue;

		tm->tm_sec = rtc_data[0] & 0x3F;
		tm->tm_min = rtc_data[1] & 0x3F;
		tm->tm_hour = rtc_data[2] & 0x1F;
		tm->tm_wday = (rtc_data[3] & 0x07) == 0x07 ?
				0 : (rtc_data[3] & 0x07);
		tm->tm_mday = rtc_data[4] & 0x1F;
		tm->tm_mon = (rtc_data[5] & 0x0F) - 1;
		tm->tm_year = (rtc_data[6] & 0x7F) * 100
				+ (rtc_data[7] & 0x7F) - 1900;

		dev_dbg(dev, "RTC date/time %4d-%02d-%02d(%d) %02d:%02d:%02d\n",
			1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday, tm->tm_wday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);

		return ret;

	} while (++count < RTC_GET_TIME_RETRIES);
	dev_err(dev, "Timed out reading current time\n");

	return -EIO;

}

/*
 * Set current time and date in RTC
 */
static int rtp_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct rtp_rtc *rtp_rtc = dev_get_drvdata(dev);
	struct rtp_mfd_chip *chip = rtp_rtc->chip;
	int ret;
	uint8_t rtc_data[ALL_TIME_REGS + 1];

	printk(KERN_DEBUG "Set rtp rtc time\n");
	rtc_data[0] = tm->tm_sec;
	rtc_data[1] = tm->tm_min;
	rtc_data[2] = tm->tm_hour;
	rtc_data[3] = tm->tm_wday == 0 ? 7 : tm->tm_wday;
	rtc_data[4] = tm->tm_mday;
	rtc_data[5] = tm->tm_mon + 1;
	rtc_data[6] = (1900 + tm->tm_year) / 100;
	rtc_data[7] = (1900 + tm->tm_year) % 100;

	/* Stop calendar while updating the TC registers */
	ret = rtp_clr_bits(chip, RTP_REG_CAL_ALARM, 0x01);
	if (ret < 0) {
		dev_err(dev, "Failed to disable rtc: %d\n", ret);
		return ret;
	}

	/* update all the time registers in one shot */
	ret = rtp_bulk_write(chip, RTP_REG_CAL_SEC, ALL_TIME_REGS,
				 rtc_data);
	if (ret < 0) {
		dev_err(dev, "Failed to write RTC times: %d\n", ret);
		return ret;
	}

	/* Start calendar again */
	ret = rtp_set_bits(chip, RTP_REG_CAL_ALARM, 0x01);
	if (ret < 0) {
		dev_err(dev, "Failed to enable rtc: %d\n", ret);
		return ret;
	}

	return 0;
}

/*
 * Read alarm time and date in RTC
 */
static int rtp_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtp_rtc *rtp_rtc = dev_get_drvdata(dev);
	struct rtp_mfd_chip *chip = rtp_rtc->chip;
	int ret;
	uint8_t val;
	uint8_t alrm_data[ALL_ALM_REGS + 1];

	printk(KERN_DEBUG "Read rtp rtc alarm\n");
	ret = rtp_bulk_read(chip, RTP_REG_ALARM_SEC, ALL_ALM_REGS,
				alrm_data);
	if (ret != 0) {
		dev_err(dev, "Failed to read alarm time: %d\n", ret);
		return ret;
	}

	/* some of these fields may be wildcard/"match all" */
	alrm->time.tm_sec = alrm_data[0] & 0x3F;
	alrm->time.tm_min = alrm_data[1] & 0x3F;
	alrm->time.tm_hour = alrm_data[2] & 0x1F;
	alrm->time.tm_mday = alrm_data[3] & 0x1F;
	alrm->time.tm_mon = (alrm_data[4] & 0x0F) - 1;
	alrm->time.tm_year = 2000 + (alrm_data[5] & 0x7F) - 1900;

	ret = rtp_reg_read(chip, RTP_REG_CAL_ALARM, &val);
	if (ret < 0) {
		dev_err(dev, "Failed to read alarm enable: %d\n", ret);
		return ret;
	}

	if (val & 0x02)
		alrm->enabled = 1;
	else
		alrm->enabled = 0;

	return 0;
}

static int rtp_rtc_stop_alarm(struct rtp_rtc *rtp_rtc)
{
	struct rtp_mfd_chip *chip = rtp_rtc->chip;

	printk(KERN_DEBUG "Stop rtp rtc alarm\n");
	rtp_rtc->alarm_enabled = 0;

	return rtp_clr_bits(chip, RTP_REG_CAL_ALARM, 0x02);
}

static int rtp_rtc_start_alarm(struct rtp_rtc *rtp_rtc)
{
	struct rtp_mfd_chip *chip = rtp_rtc->chip;

	printk(KERN_DEBUG "Start rtp rtc alarm\n");
	rtp_rtc->alarm_enabled = 1;

	return rtp_set_bits(chip, RTP_REG_CAL_ALARM, 0x02);
}

static int rtp_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtp_rtc *rtp_rtc = dev_get_drvdata(dev);
	struct rtp_mfd_chip *chip = rtp_rtc->chip;
	int ret;
	uint8_t alrm_data[ALL_TIME_REGS + 1];

	printk(KERN_DEBUG "Set rtp rtc alarm\n");
	ret = rtp_rtc_stop_alarm(rtp_rtc);
	if (ret < 0) {
		dev_err(dev, "Failed to stop alarm: %d\n", ret);
		return ret;
	}

	alrm_data[0] = alrm->time.tm_sec;
	alrm_data[1] = alrm->time.tm_min;
	alrm_data[2] = alrm->time.tm_hour;
	alrm_data[3] = alrm->time.tm_mday;
	alrm_data[4] = alrm->time.tm_mon + 1;
	alrm_data[5] = alrm->time.tm_year + 1900 - 2000;

	ret = rtp_bulk_write(chip, RTP_REG_ALARM_SEC, ALL_ALM_REGS,
				 alrm_data);
	if (ret != 0) {
		dev_err(dev, "Failed to read alarm time: %d\n", ret);
		return ret;
	}

	if (alrm->enabled) {
		ret = rtp_rtc_start_alarm(rtp_rtc);
		if (ret < 0) {
			dev_err(dev, "Failed to start alarm: %d\n", ret);
			return ret;
		}
	}

	return 0;
}

static int rtp_rtc_alarm_irq_enable(struct device *dev,
					unsigned int enabled)
{
	struct rtp_rtc *rtp_rtc = dev_get_drvdata(dev);

	printk(KERN_DEBUG "enable/disable rtp rtc alarm\n");
	if (enabled)
		return rtp_rtc_start_alarm(rtp_rtc);
	else
		return rtp_rtc_stop_alarm(rtp_rtc);
}

static int rtp_alarm_event(struct notifier_block *nb, unsigned long event,
			       void *data)
{
	struct rtp_rtc *rtp_rtc = container_of(nb, struct rtp_rtc,
						       nb);
	struct rtp_mfd_chip *chip = rtp_rtc->chip;
	uint64_t irqs;

	chip->read_irqs(chip, &irqs);
	irqs &= chip->irq_enable;
	irqs &= RTP_RTC_NOTIFIER_ON;

	if (irqs & RTP_IRQ_ALARM)
		rtc_update_irq(rtp_rtc->rtc, 1, RTC_IRQF | RTC_AF);

	chip->write_irqs(chip, irqs);

	return 0;
}

static const struct rtc_class_ops rtp_rtc_ops = {
	.read_time = rtp_rtc_read_time,
	.set_time = rtp_rtc_set_time,
	.read_alarm = rtp_rtc_read_alarm,
	.set_alarm = rtp_rtc_set_alarm,
	.alarm_irq_enable = rtp_rtc_alarm_irq_enable,
};

#ifdef CONFIG_PM
/* Turn off the alarm if it should not be a wake source. */
static int rtp_rtc_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtp_rtc *rtp_rtc = dev_get_drvdata(&pdev->dev);
	int ret;

	printk(KERN_DEBUG "rtp rtc suspend\n");
	if (rtp_rtc->alarm_enabled)
		ret = rtp_rtc_start_alarm(rtp_rtc);
	else
		ret = rtp_rtc_stop_alarm(rtp_rtc);

	if (ret != 0)
		dev_err(&pdev->dev, "Failed to update RTC alarm: %d\n", ret);

	return 0;
}

/* Enable the alarm if it should be enabled (in case it was disabled to
 * prevent use as a wake source).
 */
static int rtp_rtc_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtp_rtc *rtp_rtc = dev_get_drvdata(&pdev->dev);
	int ret;

	printk(KERN_DEBUG "rtp rtc resume\n");
	if (rtp_rtc->alarm_enabled) {
		ret = rtp_rtc_start_alarm(rtp_rtc);
		if (ret != 0)
			dev_err(&pdev->dev, "Failed to restart RTC alarm: %d\n", ret);
	}

	return 0;
}

/* Unconditionally disable the alarm */
static int rtp_rtc_freeze(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtp_rtc *rtp_rtc = dev_get_drvdata(&pdev->dev);
	int ret;

	printk(KERN_DEBUG "rtp rtc freeze\n");
	ret = rtp_rtc_stop_alarm(rtp_rtc);
	if (ret != 0)
		dev_err(&pdev->dev, "Failed to stop RTC alarm: %d\n", ret);

	return 0;
}
#else
#define rtp_rtc_suspend NULL
#define rtp_rtc_resume NULL
#define rtp_rtc_freeze NULL
#endif

static ssize_t rtp_rtc_time_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtp_rtc *rtp_rtc = platform_get_drvdata(pdev);
	struct rtc_time tm;

	rtc_read_time(rtp_rtc->rtc, &tm);
	return sprintf(buf, "RTC date/time %4d-%02d-%02d(%d) %02d:%02d:%02d\n",
		       1900 + tm.tm_year, tm.tm_mon + 1, tm.tm_mday,
		       tm.tm_wday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static ssize_t rtp_rtc_time_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	char buftemp[5];
	struct platform_device *pdev = to_platform_device(dev);
	struct rtp_rtc *rtp_rtc = platform_get_drvdata(pdev);
	struct rtc_time tm;

	if (count != 16)
		return count;

	/* year */
	buftemp[0] = buf[0];
	buftemp[1] = buf[1];
	buftemp[2] = buf[2];
	buftemp[3] = buf[3];
	buftemp[4] = '\0';
	tm.tm_year = simple_strtoul(buftemp, NULL, 10) - 1900;
	/* mon */
	buftemp[0] = buf[4];
	buftemp[1] = buf[5];
	buftemp[2] = '\0';
	tm.tm_mon = simple_strtoul(buftemp, NULL, 10) - 1;
	/* day */
	buftemp[0] = buf[6];
	buftemp[1] = buf[7];
	buftemp[2] = '\0';
	tm.tm_mday = simple_strtoul(buftemp, NULL, 10) - 1;
	/* week */
	buftemp[0] = buf[8];
	buftemp[1] = '\0';
	tm.tm_wday = simple_strtoul(buftemp, NULL, 10);
	/* hour */
	buftemp[0] = buf[9];
	buftemp[1] = buf[10];
	buftemp[2] = '\0';
	tm.tm_hour = simple_strtoul(buftemp, NULL, 10);
	/* min */
	buftemp[0] = buf[11];
	buftemp[1] = buf[12];
	buftemp[2] = '\0';
	tm.tm_min = simple_strtoul(buftemp, NULL, 10);
	/* sec */
	buftemp[0] = buf[13];
	buftemp[1] = buf[14];
	buftemp[2] = '\0';
	tm.tm_sec = simple_strtoul(buftemp, NULL, 10);

	rtc_set_time(rtp_rtc->rtc, &tm);

	return count;
}

static ssize_t rtp_rtc_alarm_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtp_rtc *rtp_rtc = platform_get_drvdata(pdev);
	struct rtc_wkalrm alrm;

	rtc_read_alarm(rtp_rtc->rtc, &alrm);
	return sprintf(buf, "RTC alarm date/time %4d-%02d-%02d(%d) %02d:%02d:%02d\n",
		       1900 + alrm.time.tm_year, alrm.time.tm_mon + 1,
		       alrm.time.tm_mday, alrm.time.tm_wday, alrm.time.tm_hour,
		       alrm.time.tm_min, alrm.time.tm_sec);
}

static ssize_t rtp_rtc_alarm_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtp_rtc *rtp_rtc = platform_get_drvdata(pdev);
	struct rtc_wkalrm alrm;
	char buftemp[5];

	if (count != 15)
		return count;

	/* year */
	buftemp[0] = buf[0];
	buftemp[1] = buf[1];
	buftemp[2] = buf[2];
	buftemp[3] = buf[3];
	buftemp[4] = '\0';
	alrm.time.tm_year = simple_strtoul(buftemp, NULL, 10) - 1900;
	/* mon */
	buftemp[0] = buf[4];
	buftemp[1] = buf[5];
	buftemp[2] = '\0';
	alrm.time.tm_mon = simple_strtoul(buftemp, NULL, 10) - 1;
	/* day */
	buftemp[0] = buf[6];
	buftemp[1] = buf[7];
	buftemp[2] = '\0';
	alrm.time.tm_mday = simple_strtoul(buftemp, NULL, 10) - 1;
	/* hour */
	buftemp[0] = buf[8];
	buftemp[1] = buf[9];
	buftemp[2] = '\0';
	alrm.time.tm_hour = simple_strtoul(buftemp, NULL, 10);
	/* min */
	buftemp[0] = buf[10];
	buftemp[1] = buf[11];
	buftemp[2] = '\0';
	alrm.time.tm_min = simple_strtoul(buftemp, NULL, 10);
	/* sec */
	buftemp[0] = buf[12];
	buftemp[1] = buf[13];
	buftemp[2] = '\0';
	alrm.time.tm_sec = simple_strtoul(buftemp, NULL, 10);

	rtc_set_alarm(rtp_rtc->rtc, &alrm);

	return count;
}

static ssize_t rtp_rtc_irq_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "rtp rtc alarm");
}

static ssize_t rtp_rtc_irq_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	int retval;
	struct platform_device *pdev = to_platform_device(dev);
	struct rtp_rtc *rtp_rtc = platform_get_drvdata(pdev);

	retval = count;
	if (strncmp(buf, "alarm", 5) == 0) {
		struct rtc_wkalrm alrm;
		int err = rtc_read_alarm(rtp_rtc->rtc, &alrm);

		if (!err && alrm.enabled)
			rtc_update_irq(rtp_rtc->rtc, 1, RTC_AF | RTC_IRQF);

	} else if (strncmp(buf, "trigger", 7) == 0)
		rtc_update_irq(rtp_rtc->rtc, 1, RTC_UF | RTC_IRQF);
	else
		retval = -EINVAL;

	return retval;
}

static struct device_attribute rtp_rtc_attrs[] = {
	RTP_MFD_ATTR(rtp_rtc_time),
	RTP_MFD_ATTR(rtp_rtc_alarm),
	RTP_MFD_ATTR(rtp_rtc_irq),
};

static int rtp_rtc_probe(struct platform_device *pdev)
{
	struct rtp_mfd_chip *chip = dev_get_drvdata(pdev->dev.parent);
	struct rtp_rtc *rtp_rtc;
	int j;
	int ret = 0;
	uint8_t val;

	struct rtc_time tm;
	struct rtc_time tm_def = {	//	2011.1.1 12:00:00 Saturday
			.tm_wday = 6,
			.tm_year = 111,
			.tm_mon = 0,
			.tm_mday = 1,
			.tm_hour = 12,
			.tm_min = 0,
			.tm_sec = 0,
		};

	rtp_rtc = kzalloc(sizeof(*rtp_rtc), GFP_KERNEL);
	if (rtp_rtc == NULL)
		return -ENOMEM;

	platform_set_drvdata(pdev, rtp_rtc);
	rtp_rtc->chip = chip;

	/* enable calendar */
	ret = rtp_set_bits(chip, RTP_REG_CAL_ALARM, 0x01);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to enable calendar: %d\n", ret);
		goto err;
	}

	/* set osc clk */
	val = 0x85;
	ret = rtp_reg_write(chip, RTP_REG_OSC_CLK_SET, &val);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to set osc clk: %d\n", ret);
		goto err;
	}

	/* set init time */
	ret = rtp_rtc_read_time(&pdev->dev, &tm);
	if (ret)
	{
		dev_err(&pdev->dev, "Failed to read RTC time\n");
		goto err;
	}

	ret = rtc_valid_tm(&tm);
	if (ret) {
		dev_err(&pdev->dev,"invalid date/time and init time\n");
		rtp_rtc_set_time(&pdev->dev, &tm_def); // 2011-01-01 12:00:00
		dev_info(&pdev->dev, "set RTC date/time %4d-%02d-%02d(%d) %02d:%02d:%02d\n",
			 1900 + tm_def.tm_year, tm_def.tm_mon + 1,
			 tm_def.tm_mday, tm_def.tm_wday, tm_def.tm_hour,
			 tm_def.tm_min, tm_def.tm_sec);
	}

	rtp_rtc->rtc = rtc_device_register("rtp_rtc", &pdev->dev,
					       &rtp_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtp_rtc->rtc)) {
		ret = PTR_ERR(rtp_rtc->rtc);
		goto err;
	}

	rtp_rtc->nb.notifier_call = rtp_alarm_event;
	ret = rtp_register_notifier(rtp_rtc->chip, &rtp_rtc->nb,
					RTP_RTC_NOTIFIER_ON);
	if (ret)
		goto notifier_err;

	for (j = 0; j < ARRAY_SIZE(rtp_rtc_attrs); j++) {
		ret = device_create_file(&pdev->dev, &rtp_rtc_attrs[j]);
		if (ret)
			goto sysfs_failed;
	}

	printk("%s:ok\n",__func__);

	return 0;

sysfs_failed:
	while (j--)
		device_remove_file(&pdev->dev, &rtp_rtc_attrs[j]);
	rtp_unregister_notifier(rtp_rtc->chip, &rtp_rtc->nb,
				    RTP_RTC_NOTIFIER_ON);
notifier_err:
	rtc_device_unregister(rtp_rtc->rtc);
err:
	kfree(rtp_rtc);
	return ret;
}

static int rtp_rtc_remove(struct platform_device *pdev)
{
	struct rtp_rtc *rtp_rtc = platform_get_drvdata(pdev);
	int j;

	rtc_device_unregister(rtp_rtc->rtc);
	rtp_unregister_notifier(rtp_rtc->chip, &rtp_rtc->nb,
				    RTP_RTC_NOTIFIER_ON);
	for (j = 0; j < ARRAY_SIZE(rtp_rtc_attrs); j++)
		device_remove_file(&pdev->dev, &rtp_rtc_attrs[j]);
	kfree(rtp_rtc);

	return 0;
}

static const struct dev_pm_ops rtp_rtc_pm_ops = {
	.suspend = rtp_rtc_suspend,
	.resume = rtp_rtc_resume,

	.freeze = rtp_rtc_freeze,
	.thaw = rtp_rtc_resume,
	.restore = rtp_rtc_resume,

	.poweroff = rtp_rtc_suspend,
};

static struct platform_driver rtp_rtc_driver = {
	.probe = rtp_rtc_probe,
	.remove = rtp_rtc_remove,
	.driver = {
		.name = "rtp-rtc",
		.pm = &rtp_rtc_pm_ops,
	},
};

static int __init rtp_rtc_init(void)
{
	return platform_driver_register(&rtp_rtc_driver);
}
subsys_initcall(rtp_rtc_init);

static void __exit rtp_rtc_exit(void)
{
	platform_driver_unregister(&rtp_rtc_driver);
}
module_exit(rtp_rtc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek Power Manager Driver");
