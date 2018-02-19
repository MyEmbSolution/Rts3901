#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#define XB2_RTC_EN 0
#define XB2_RTC_TIME_LOAD 0x4
#define XB2_RTC_DATE_LOAD 0x8
#define XB2_RTC_LOAD 0xc
#define XB2_RTC_ALARM_EN 0x10
#define XB2_RTC_ALARM0_TIME 0x0014
#define XB2_RTC_ALARM0_DATE 0x0018
#define XB2_RTC_ALARM1_TIME 0x001C
#define XB2_RTC_ALARM1_DATE 0x0020
#define XB2_RTC_ALARM2_TIME 0x0024
#define XB2_RTC_ALARM2_DATE 0x0028
#define XB2_RTC_ALARM3_TIME 0x002C
#define XB2_RTC_ALARM3_DATE 0x0030
#define XB2_RTC_TIME_NOW 0x0034
#define XB2_RTC_DATE_NOW 0x0038
#define XB2_RTC_LEAP_YEAR 0x003C

#define ALARM0_ENABLE 0
#define ALARM1_ENABLE 1
#define ALARM2_ENABLE 2
#define ALARM3_ENABLE 3

#define SEC_VALUE(x) (x & 0x3f)
#define MIN_VALUE(x) ((x & 0x3f00) >> 8)
#define HOUR_VALUE(x) ((x & 0x1f0000) >> 16)
#define WEEK_VALUE(x) ((x & 0x7000000) >> 24)

#define DAY_VALUE(x) (x & 0x1f)
#define MON_VALUE(x) ((x & 0xf00) >> 8)
#define YEAR_VALUE(x) ((x & 0x7f0000) >> 16)
#define CEN_VALUE(x) ((x & 0x7f000000) >> 24)

#define TIME_REG(weekday, hour, min, sec) (((weekday & 0x7f)<<24) | \
	((hour&0x1f) << 16) | ((min & 0x3f) << 8) | (sec & 0x3f))

#define DATE_REG(cen, year, mon, day) (((cen & 0x7f)<<24) | \
	((year & 0x7f) << 16) | ((mon & 0xf) << 8) | (day & 0x1f))

struct rts_rtc {
	struct resource *mem;
	void __iomem *base;
	struct rtc_device *rtc;
	int irq;
	spinlock_t lock;
};

static inline uint32_t rts_rtc_reg_read(struct rts_rtc *rtc, size_t reg)
{
	return readl(rtc->base + reg);
}

static inline void rts_rtc_reg_write(struct rts_rtc *rtc, size_t reg,
	uint32_t val)
{
	writel(val, rtc->base + reg);
}

static inline void rts_rtc_reg_setbit(struct rts_rtc *rtc, size_t reg,
	uint32_t bit)
{
	set_bit(bit, rtc->base + reg);
}

static inline void rts_rtc_reg_clearbit(struct rts_rtc *rtc, size_t reg,
	uint32_t bit)
{
	clear_bit(bit, rtc->base + reg);
}

static int rts_rtc_read_time(struct device *dev, struct rtc_time *rtc_tm)
{
	uint32_t time, date;
	ulong flags;

	struct platform_device *pdev = to_platform_device(dev);
	struct rts_rtc *pdata = platform_get_drvdata(pdev);

	spin_lock_irqsave(&pdata->lock, flags);

	time =	readl(pdata->base + XB2_RTC_TIME_NOW);
	date =  readl(pdata->base + XB2_RTC_DATE_NOW);

	spin_unlock_irqrestore(&pdata->lock, flags);

	rtc_tm->tm_min  = MIN_VALUE(time);
	rtc_tm->tm_hour = HOUR_VALUE(time);
	rtc_tm->tm_sec  = SEC_VALUE(time);
	rtc_tm->tm_wday = WEEK_VALUE(time);
	rtc_tm->tm_mday = DAY_VALUE(date);
	rtc_tm->tm_mon  = MON_VALUE(date) - 1;
	rtc_tm->tm_year = YEAR_VALUE(date) + CEN_VALUE(date) * 100 - 1900;

	dev_dbg(dev, "read time %04d.%02d.%02d %02d:%02d:%02d\n",
		 1900 + rtc_tm->tm_year, rtc_tm->tm_mon, rtc_tm->tm_mday,
		 rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);

	return rtc_valid_tm(rtc_tm);
}

static int rts_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rts_rtc *pdata = platform_get_drvdata(pdev);
	uint32_t century, year, time, date;
	ulong flags;

	century = (tm->tm_year + 1900) / 100;
	year = tm->tm_year % 100;

	time = TIME_REG(tm->tm_wday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	date = DATE_REG(century, year, (tm->tm_mon + 1), tm->tm_mday);

	dev_dbg(dev, "set time %04d.%02d.%02d %02d:%02d:%02d 0x%x 0x%x\n",
		 1900 + tm->tm_year, tm->tm_mon, tm->tm_mday,
		 tm->tm_hour, tm->tm_min, tm->tm_sec, time, date);

	spin_lock_irqsave(&pdata->lock, flags);

	rts_rtc_reg_write(pdata, XB2_RTC_TIME_LOAD, time);
	rts_rtc_reg_write(pdata, XB2_RTC_DATE_LOAD, date);
	rts_rtc_reg_write(pdata, XB2_RTC_LOAD, 0xff);

	spin_unlock_irqrestore(&pdata->lock, flags);

	return 0;
}

static int rts_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rts_rtc *pdata = platform_get_drvdata(pdev);
	uint32_t cen, year, time, date;
	struct rtc_time *rtc_tm = &(alrm->time);
	ulong flags;

	cen = (rtc_tm->tm_year + 1900) / 100;
	year = rtc_tm->tm_year % 100;

	rtc_tm->tm_wday = 1 << (rtc_tm->tm_wday - 1);

	time = TIME_REG(rtc_tm->tm_wday, rtc_tm->tm_hour,
		rtc_tm->tm_min, rtc_tm->tm_sec);
	date = DATE_REG(cen, year, (rtc_tm->tm_mon + 1), rtc_tm->tm_hour);

	dev_dbg(dev, "set alarm %04d.%02d.%02d %02d:%02d:%02d %x %x\n",
		 1900 + rtc_tm->tm_year, rtc_tm->tm_mon, rtc_tm->tm_mday,
		 rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec, time, date);

	spin_lock_irqsave(&pdata->lock, flags);

	rts_rtc_reg_write(pdata, XB2_RTC_ALARM3_TIME, time);
	rts_rtc_reg_write(pdata, XB2_RTC_ALARM3_DATE, date);

	spin_unlock_irqrestore(&pdata->lock, flags);

	return 0;
}

static int rts_rtc_get_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	ulong flags;
	struct platform_device *pdev = to_platform_device(dev);
	struct rts_rtc *pdata = platform_get_drvdata(pdev);
	uint32_t time, date;
	struct rtc_time *rtc_tm = &(alrm->time);

	spin_lock_irqsave(&pdata->lock, flags);
	time =	readl(pdata->base + XB2_RTC_ALARM3_TIME);
	date =  readl(pdata->base + XB2_RTC_ALARM3_DATE);

	alrm->enabled = rts_rtc_reg_read(pdata, XB2_RTC_ALARM_EN)
		& ALARM3_ENABLE;
	spin_unlock_irqrestore(&pdata->lock, flags);

	rtc_tm->tm_min  = MIN_VALUE(time);
	rtc_tm->tm_hour = HOUR_VALUE(time);
	rtc_tm->tm_sec  = SEC_VALUE(time);
	rtc_tm->tm_wday = WEEK_VALUE(time);
	rtc_tm->tm_mday = DAY_VALUE(date);
	rtc_tm->tm_mon  = MON_VALUE(date) - 1;
	rtc_tm->tm_year = YEAR_VALUE(date) + CEN_VALUE(date) * 100 - 1900;


	dev_dbg(dev, "get alarm %04d.%02d.%02d %02d:%02d:%02d\n",
		 1900 + rtc_tm->tm_year, rtc_tm->tm_mon, rtc_tm->tm_mday,
		 rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);

	alrm->pending = 0;

	return 0;
}

static irqreturn_t rts_rtc_interrupt(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct rts_rtc *pdata = platform_get_drvdata(pdev);
	unsigned long events = 0;

	events = RTC_IRQF | RTC_AF;
	if (likely(pdata->rtc))
		rtc_update_irq(pdata->rtc, 1, events);

	return events ? IRQ_HANDLED : IRQ_NONE;
}

static int rts_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rts_rtc *pdata = platform_get_drvdata(pdev);
	ulong flags;

	if (pdata->irq <= 0)
		return -EINVAL;

	spin_lock_irqsave(&pdata->lock, flags);
	if (enabled)
		rts_rtc_reg_setbit(pdata, XB2_RTC_ALARM_EN, ALARM3_ENABLE);
	else
		rts_rtc_reg_clearbit(pdata, XB2_RTC_ALARM_EN, ALARM3_ENABLE);
	spin_unlock_irqrestore(&pdata->lock, flags);

	return 0;
}

static struct rtc_class_ops rts_rtc_ops = {
	.read_time	= rts_rtc_read_time,
	.set_time	= rts_rtc_set_time,
	.read_alarm	= rts_rtc_get_alarm,
	.set_alarm	= rts_rtc_set_alarm,
	.alarm_irq_enable = rts_rtc_alarm_irq_enable,
};

static int rts_rtc_probe(struct platform_device *pdev)
{
	int ret;
	struct rts_rtc *rtc;

	rtc = kzalloc(sizeof(*rtc), GFP_KERNEL);
	if (!rtc)
		return -ENOMEM;
	rtc->irq = platform_get_irq(pdev, 0);
	if (rtc->irq < 0) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "Failed to get platform irq\n");
		goto err_free;
	}
	rtc->mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!rtc->mem) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "Failed to get platform mmio memory\n");
		goto err_free;
	}
	rtc->mem = request_mem_region(rtc->mem->start, resource_size(rtc->mem),
					pdev->name);
	if (!rtc->mem) {
		ret = -EBUSY;
		dev_err(&pdev->dev, "Failed to request mmio memory region\n");
		goto err_free;
	}
	rtc->base = ioremap_nocache(rtc->mem->start, resource_size(rtc->mem));
	if (!rtc->base) {
		ret = -EBUSY;
		dev_err(&pdev->dev, "Failed to ioremap mmio memory\n");
		goto err_release_mem_region;
	}
	spin_lock_init(&rtc->lock);

	platform_set_drvdata(pdev, rtc);

	device_init_wakeup(&pdev->dev, 1);
	rtc->rtc = rtc_device_register(pdev->name, &pdev->dev, &rts_rtc_ops,
					THIS_MODULE);
	if (IS_ERR(rtc->rtc)) {
		ret = PTR_ERR(rtc->rtc);
		dev_err(&pdev->dev, "Failed to register rtc device: %d\n", ret);
		goto err_iounmap;
	}
	ret = request_irq(rtc->irq, rts_rtc_interrupt, 0, pdev->name, pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request rtc irq: %d\n", ret);
		goto err_unregister_rtc;
	}
	rts_rtc_reg_write(rtc, XB2_RTC_EN, 1);

	return 0;

err_unregister_rtc:
	rtc_device_unregister(rtc->rtc);
err_iounmap:
	platform_set_drvdata(pdev, NULL);
	iounmap(rtc->base);
err_release_mem_region:
	release_mem_region(rtc->mem->start, resource_size(rtc->mem));
err_free:
	kfree(rtc);

	return ret;
}

static int rts_rtc_remove(struct platform_device *pdev)
{
	struct rts_rtc *rtc = platform_get_drvdata(pdev);

	free_irq(rtc->irq, rtc);

	rtc_device_unregister(rtc->rtc);

	iounmap(rtc->base);
	release_mem_region(rtc->mem->start, resource_size(rtc->mem));

	kfree(rtc);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver rts_rtc_driver = {
	.probe	 = rts_rtc_probe,
	.remove	 = rts_rtc_remove,
	.driver	 = {
		.name  = "rts-rtc",
		.owner = THIS_MODULE,
	},
};

module_platform_driver(rts_rtc_driver);
