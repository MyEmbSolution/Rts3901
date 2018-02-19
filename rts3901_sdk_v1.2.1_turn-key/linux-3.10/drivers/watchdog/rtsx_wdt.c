/* Driver for Realtek ipcam card reader
 *
 * Copyright(c) 2014 Realtek Semiconductor Corp. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author:
 *   Peter Sun <peter_sun@realsil.com.cn>
 *   No. 128, West Shenhu Road, Suzhou Industry Park, Suzhou, China
 */

#include <linux/module.h>	/* For module specific items */
#include <linux/moduleparam.h>	/* For new moduleparam's */
#include <linux/types.h>	/* For standard types (like size_t) */
#include <linux/errno.h>	/* For the -ENODEV/... values */
#include <linux/kernel.h>	/* For printk/panic/... */
#include <linux/fs.h>		/* For file operations */
#include <linux/miscdevice.h>	/* For MODULE_ALIAS_MISCDEV
				   (WATCHDOG_MINOR) */
#include <linux/watchdog.h>	/* For the watchdog specific items */
#include <linux/init.h>		/* For __init/__exit/... */
#include <linux/platform_device.h>	/* For platform_driver framework */
#include <linux/spinlock.h>	/* For spin_lock/spin_unlock/... */
#include <linux/uaccess.h>	/* For copy_to_user/put_user/... */

#define DRIVER_NAME "rtsx-wdt"

#define WATCHDOG_CFG_REG 0
#define WATCHDOG_CTL 0x4
#define WATCHDOG_INT_EN 0x4
#define WATCHDOG_INT_FLAG 0x4

#define WDOG_RST_PAD_PUE 9
#define WDOG_RST_PAD_PDE 8
#define WDOG_RST_PAD_SR_SLOW 7
#define WDOG_RST_PAD_DRV_8MA 6
#define WDOG_RST_PMU_VOLTAGE_3V3 5
#define WDOG_RST_PMU_ENABLE 4
#define WDOG_TIME	2
#define WDOG_RST_EN	1
#define WDOG_EN		0

#define WATCHDOG_TIMEOUT 8

#define RTS_GETFIELD(val, width, offset)	\
			((val >> offset) & ((1 << width) - 1))
#define RTS_SETFIELD(reg, field, width, offset)	((reg & \
			(~(((1 << width) - 1) <<	offset))) \
			| ((field & ((1 << width) - 1)) << offset))

static struct {
	unsigned long inuse;
	spinlock_t io_lock;
} rtsx_wdt_device;

static void __iomem *wdt_reg;
static int expect_close;
static int timeout = WATCHDOG_TIMEOUT;
static bool nowayout = WATCHDOG_NOWAYOUT;

static void rts_set_field(void __iomem *reg,
			  unsigned int field, unsigned int width,
			  unsigned int offset)
{
	unsigned int val = readl(reg);
	val = RTS_SETFIELD(val, field, width, offset);
	writel(val, reg);
	return;
}

static int rtsx_wdt_set(int new_timeout)
{
	u8 time;

	spin_lock(&rtsx_wdt_device.io_lock);

	if (new_timeout < 1)
		timeout = 1;
	else if (new_timeout < 2)
		timeout = 2;
	else if (new_timeout < 4)
		timeout = 4;
	else
		timeout = 8;

	time = (time = timeout / 2) < 3 ? time : 3;

	rts_set_field(wdt_reg + WATCHDOG_CFG_REG, time, 2, WDOG_TIME);

	spin_unlock(&rtsx_wdt_device.io_lock);

	return 0;
}

static void rtsx_wdt_start(void)
{
	spin_lock(&rtsx_wdt_device.io_lock);

	rtsx_wdt_set(timeout);

#ifdef CONFIG_EXTERNAL_RESET
	rts_set_field(wdt_reg + WATCHDOG_CFG_REG, 1, 1, WDOG_RST_PMU_ENABLE);
#else
	rts_set_field(wdt_reg + WATCHDOG_CFG_REG, 1, 1, WDOG_RST_EN);
#endif
	rts_set_field(wdt_reg + WATCHDOG_CFG_REG, 1, 1, WDOG_EN);

	spin_unlock(&rtsx_wdt_device.io_lock);
	pr_info("Started watchdog timer\n");
}

static void rtsx_wdt_stop(void)
{
	spin_lock(&rtsx_wdt_device.io_lock);

	rts_set_field(wdt_reg + WATCHDOG_CFG_REG, 0, 1, WDOG_EN);

	spin_unlock(&rtsx_wdt_device.io_lock);
	pr_info("Stopped watchdog timer\n");
}

static void rtsx_wdt_ping(void)
{
	spin_lock(&rtsx_wdt_device.io_lock);
	rts_set_field(wdt_reg + WATCHDOG_CTL, 1, 1, 0);
	spin_unlock(&rtsx_wdt_device.io_lock);
}

static int rtsx_wdt_open(struct inode *inode, struct file *file)
{
	if (test_and_set_bit(0, &rtsx_wdt_device.inuse))
		return -EBUSY;

	if (nowayout)
		__module_get(THIS_MODULE);

	rtsx_wdt_start();
	rtsx_wdt_ping();

	return nonseekable_open(inode, file);
}

static int rtsx_wdt_release(struct inode *inode, struct file *file)
{
	if (expect_close == 42) {
		rtsx_wdt_stop();
		module_put(THIS_MODULE);
	} else {
		pr_crit("device closed\n");
		rtsx_wdt_ping();
	}
	clear_bit(0, &rtsx_wdt_device.inuse);
	return 0;
}

static ssize_t rtsx_wdt_write(struct file *file, const char *data,
			      size_t len, loff_t *ppos)
{
	if (len) {
		if (!nowayout) {
			size_t i;

			expect_close = 0;

			for (i = 0; i != len; i++) {
				char c;
				if (get_user(c, data + i))
					return -EFAULT;
				if (c == 'V')
					expect_close = 42;
			}
		}
		rtsx_wdt_ping();
		return len;
	}
	return 0;
}

void reboot_by_wdt(int new_timeout)
{
	rtsx_wdt_set(new_timeout);
	rtsx_wdt_start();
}
EXPORT_SYMBOL_GPL(reboot_by_wdt);

static long rtsx_wdt_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int new_timeout;
	unsigned int value;
	static const struct watchdog_info ident = {
		.options = WDIOF_SETTIMEOUT |
		    WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
		.identity = "RTSX_WDT Watchdog",
	};
	switch (cmd) {
	case WDIOC_GETSUPPORT:
		if (copy_to_user(argp, &ident, sizeof(ident)))
			return -EFAULT;
		break;
	case WDIOC_GETSTATUS:
	case WDIOC_GETBOOTSTATUS:
		value = 0;
		if (copy_to_user(argp, &value, sizeof(int)))
			return -EFAULT;
		break;
	case WDIOC_SETOPTIONS:
		if (copy_from_user(&value, argp, sizeof(int)))
			return -EFAULT;
		switch (value) {
		case WDIOS_ENABLECARD:
			rtsx_wdt_start();
			break;
		case WDIOS_DISABLECARD:
			rtsx_wdt_stop();
			break;
		default:
			return -EINVAL;
		}
		break;
	case WDIOC_KEEPALIVE:
		rtsx_wdt_ping();
		break;
	case WDIOC_SETTIMEOUT:
		if (copy_from_user(&new_timeout, argp, sizeof(int)))
			return -EFAULT;
		if (rtsx_wdt_set(new_timeout))
			return -EINVAL;
	case WDIOC_GETTIMEOUT:
		return copy_to_user(argp, &timeout, sizeof(int));
	default:
		return -ENOTTY;
	}

	return 0;
}

static const struct file_operations rtsx_wdt_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.write = rtsx_wdt_write,
	.unlocked_ioctl = rtsx_wdt_ioctl,
	.open = rtsx_wdt_open,
	.release = rtsx_wdt_release,
};

static struct miscdevice rtsx_wdt_miscdev = {
	.minor = WATCHDOG_MINOR,
	.name = "watchdog",
	.fops = &rtsx_wdt_fops,
};

static int rtsx_wdt_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *r;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r) {
		pr_err("failed to retrieve resources\n");
		return -ENODEV;
	}

	wdt_reg = ioremap_nocache(r->start, resource_size(r));
	if (!wdt_reg) {
		pr_err("failed to remap I/O resources\n");
		return -ENXIO;
	}

	spin_lock_init(&rtsx_wdt_device.io_lock);

	rtsx_wdt_stop();

	if (rtsx_wdt_set(timeout))
		rtsx_wdt_set(WATCHDOG_TIMEOUT);

	ret = misc_register(&rtsx_wdt_miscdev);
	if (ret < 0) {
		pr_err("failed to register watchdog device\n");
		goto unmap;
	}

	pr_info("timer margin: %d sec\n", timeout);

	return 0;

unmap:
	iounmap(wdt_reg);
	return ret;
}

static int rtsx_wdt_remove(struct platform_device *pdev)
{
	misc_deregister(&rtsx_wdt_miscdev);
	iounmap(wdt_reg);
	return 0;
}

static void rtsx_wdt_shutdown(struct platform_device *pdev)
{
	rtsx_wdt_stop();
}

static struct platform_driver rtsx_wdt_driver = {
	.probe = rtsx_wdt_probe,
	.remove = rtsx_wdt_remove,
	.shutdown = rtsx_wdt_shutdown,
	.driver = {
		   .name = "watchdog-platform",
		   }
};

module_platform_driver(rtsx_wdt_driver);
