/* Driver for Realtek thermal
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

#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/delay.h>


#define SYS_TM_CTRL 0x0
#define SYS_TM_CFG0 0x4
#define SYS_TM_CFG1 0x8
#define SYS_TM_CFG2 0xc
#define SYS_TM_CFG3 0x10
#define SYS_TM_CFG4 0x14
#define SYS_TM_CFG5 0x18
#define SYS_TM_CFG6 0x1c
#define SYS_TM_OUT 0x20
#define SYS_TM_DEBUG 0x24
#define SYS_TM_OUT_D 0x28
#define SYS_TM_CT_HIGH 0x2c
#define SYS_TM_CT_LOW 0x30
#define SYS_TM_CT_INT_EN 0x34
#define SYS_TM_CT_INT_FLAG 0x38

#define CENTIGRADE_TO_K(temp) (temp + 27315)
#define K_TO_CENTIGRADE(temp) (temp - 27315)

struct rts_trip_point {
	unsigned long temp;
	enum thermal_trip_type type;
};

struct rts_thsens_platform_data {
	struct rts_trip_point trip_points[THERMAL_MAX_TRIPS];
	int num_trips;
};


struct rts_thermal_zone {
	struct thermal_zone_device *therm_dev;
	struct delayed_work therm_work;
	struct work_struct init_work;
	struct rts_thsens_platform_data *trip_tab;
	unsigned int cur_index;
	void __iomem *base;
	u32 thermal_k[18];
};

static u32 actual_k[] = {
	26815, 27315, 27815, 28315, 28815, 29315, 29815, 30315,
	30815, 31315, 31815, 32315, 32815, 33315, 33815, 34315,
	34815, 35315
};

static inline uint32_t rts_thermal_reg_read(struct rts_thermal_zone *zone,
	uint32_t reg)
{
	return readl(zone->base + reg);
}


static inline void rts_thermal_reg_write(struct rts_thermal_zone *zone,
	uint32_t reg, uint32_t val)
{
	writel(val, zone->base + reg);
}

u32 temp2code(struct rts_thermal_zone *zone, u32 actk)
{
	int i;
	int x, x1, y1, x2, y2;
	u32 thk;

	for (i = 0; i < ARRAY_SIZE(actual_k)-1; i++)
		if (actk <= actual_k[i])
			break;

	if (i > 0)
		i--;

	x = actk;
	x1 = actual_k[i];
	y1 = zone->thermal_k[i];
	x2 = actual_k[i+1];
	y2 = zone->thermal_k[i+1];

	thk = (x-x1)*(y2-y1)/(x2-x1) + y1;

	return	thk * 256 / 25;
}

u32 code2temp(struct rts_thermal_zone *zone, u32 code)
{
	int i;
	int x, x1, y1, x2, y2;
	u32 thk;

	thk = code * 25 / 256;

	for (i = 0; i < ARRAY_SIZE(zone->thermal_k)-1; i++)
		if (thk <= zone->thermal_k[i])
			break;

	if (i > 0)
		i--;

	x = thk;
	x1 = zone->thermal_k[i];
	y1 = actual_k[i];
	x2 = zone->thermal_k[i+1];
	y2 = actual_k[i+1];

	thk = (x-x1)*(y2-y1)/(x2-x1) + y1;

	return	thk;
}


/* Callback to get current temperature */
static int rts_sys_get_temp(struct thermal_zone_device *thermal,
		unsigned long *temp)
{
	struct rts_thermal_zone *pzone = thermal->devdata;
	unsigned long code = rts_thermal_reg_read(pzone, SYS_TM_OUT);

	*temp = code2temp(pzone, code);

	return 0;
}

/* Callback to get trip point type */
static int rts_sys_get_trip_type(struct thermal_zone_device *thermal,
		int trip, enum thermal_trip_type *type)
{
	struct rts_thermal_zone *pzone = thermal->devdata;
	struct rts_thsens_platform_data *ptrips = pzone->trip_tab;
	if (trip >= ptrips->num_trips)
		return -EINVAL;

	*type = ptrips->trip_points[trip].type;

	return 0;
}

/* Callback to get trip point temperature */
static int rts_sys_get_trip_temp(struct thermal_zone_device *thermal,
		int trip, unsigned long *temp)
{
	struct rts_thermal_zone *pzone = thermal->devdata;
	struct rts_thsens_platform_data *ptrips = pzone->trip_tab;

	if (trip >= ptrips->num_trips)
		return -EINVAL;

	*temp = ptrips->trip_points[trip].temp;

	return 0;
}

/* Callback to get critical trip point temperature */
static int rts_sys_get_crit_temp(struct thermal_zone_device *thermal,
		unsigned long *temp)
{

	struct rts_thermal_zone *pzone = thermal->devdata;
	struct rts_thsens_platform_data *ptrips = pzone->trip_tab;
	int i;

	for (i = ptrips->num_trips - 1; i >= 0; i--) {
		if (ptrips->trip_points[i].type == THERMAL_TRIP_CRITICAL) {
			*temp = ptrips->trip_points[i].temp;
			return 0;
		}
	}

	return -EINVAL;
}

static struct thermal_zone_device_ops thdev_ops = {
	.get_temp = rts_sys_get_temp,
	.get_trip_type = rts_sys_get_trip_type,
	.get_trip_temp = rts_sys_get_trip_temp,
	.get_crit_temp = rts_sys_get_crit_temp,
};

static void rts_thermal_update_config(struct rts_thermal_zone *pzone,
		unsigned int idx)
{
	unsigned int dft_low, dft_high;

	pzone->cur_index = idx;

	dft_low = temp2code(pzone, pzone->trip_tab->trip_points[idx].temp);
	dft_high = temp2code(pzone, pzone->trip_tab->trip_points[idx+1].temp);

	rts_thermal_reg_write(pzone, SYS_TM_CT_LOW, dft_low);
	rts_thermal_reg_write(pzone, SYS_TM_CT_HIGH, dft_high);
	rts_thermal_reg_write(pzone, SYS_TM_CT_INT_FLAG, 1);

}

static irqreturn_t rts_thermal_irq_handler(int irq, void *irq_data)
{
	struct rts_thermal_zone *pzone = irq_data;
	struct rts_thsens_platform_data *ptrips = pzone->trip_tab;
	unsigned int idx = pzone->cur_index;
	unsigned int code;
	int flags;
	u32 highc, lowc;

	flags = rts_thermal_reg_read(pzone, SYS_TM_CT_INT_FLAG);
	if (flags == 0)
		return IRQ_NONE;

	flags = rts_thermal_reg_read(pzone, SYS_TM_CT_INT_EN);
	if (flags == 0)
		return IRQ_NONE;

	rts_thermal_reg_write(pzone, SYS_TM_CT_INT_EN, 0);
	rts_thermal_reg_write(pzone, SYS_TM_CT_INT_FLAG, 1);

	if (idx < ptrips->num_trips - 1) {

		code = rts_thermal_reg_read(pzone, SYS_TM_OUT);

		dev_dbg(&pzone->therm_dev->device,
			"current code is 0x%x\n", code);

		lowc = pzone->trip_tab->trip_points[idx].temp;
		highc = pzone->trip_tab->trip_points[idx+1].temp;

		if (code < lowc) {
			if (idx > 0)
				idx -= 1;
		} else if (code > highc) {
			if (idx < ptrips->num_trips - 2)
				idx++;
		}

		rts_thermal_update_config(pzone, idx);

		dev_dbg(&pzone->therm_dev->device,
			"thermal set to idx %d\n", idx);

	}

	schedule_delayed_work(&pzone->therm_work,
	      msecs_to_jiffies(300));

	return IRQ_HANDLED;
}

static void rts_thermal_work(struct work_struct *work)
{
	struct rts_thermal_zone *pzone;
	pzone = container_of((struct delayed_work *)work,
		struct rts_thermal_zone, therm_work);

	thermal_zone_device_update(pzone->therm_dev);
	dev_dbg(&pzone->therm_dev->device, "thermal work finished\n");

	kobject_uevent(&pzone->therm_dev->device.kobj, KOBJ_CHANGE);

	rts_thermal_reg_write(pzone, SYS_TM_CT_INT_EN, 1);
}

static void rts_init_thermal(struct work_struct *work)
{
	struct rts_thermal_zone *pzone;
	pzone = container_of(work, struct rts_thermal_zone, init_work);

	rts_thermal_reg_write(pzone, SYS_TM_CFG5, 0x20);
	rts_thermal_reg_write(pzone, SYS_TM_CFG6, 0x5e39ac);
	rts_thermal_reg_write(pzone, SYS_TM_CTRL, 0x36);
	rts_thermal_reg_write(pzone, SYS_TM_CTRL, 0x37);
	mdelay(200);
	rts_thermal_update_config(pzone, 0);
	rts_thermal_reg_write(pzone, SYS_TM_CT_INT_EN, 1);
}


static int rts_thermal_probe(struct platform_device *pdev)
{
	struct rts_thermal_zone *pzone = NULL;
	struct resource *mem;
	struct rts_thsens_platform_data *ptrips;
	int irq;
	int ret;

	if (pdev->dev.platform_data == NULL)
		return -EINVAL;

	pzone = devm_kzalloc(&pdev->dev, sizeof(*pzone), GFP_KERNEL);
	if (!pzone)
		return -ENOMEM;

	memcpy(pzone->thermal_k, pdev->dev.platform_data,
		ARRAY_SIZE(pzone->thermal_k) * sizeof(u32));

	ptrips =
		devm_kzalloc(&pdev->dev, sizeof(*ptrips), GFP_KERNEL);
	if (!ptrips)
		return -ENOMEM;

	ptrips->num_trips = 2;
	ptrips->trip_points[0].temp = CENTIGRADE_TO_K(-500);
	ptrips->trip_points[0].type = THERMAL_TRIP_HOT;
	ptrips->trip_points[1].temp = CENTIGRADE_TO_K(8000);
	ptrips->trip_points[1].type = THERMAL_TRIP_HOT;

	pzone->trip_tab = ptrips;
	INIT_DELAYED_WORK(&pzone->therm_work, rts_thermal_work);
	INIT_WORK(&pzone->init_work, rts_init_thermal);
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -EINVAL;
	}

	pzone->base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(pzone->base))
		return PTR_ERR(pzone->base);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "Get IRQ failed.\n");
		ret = irq;
		goto failed;
	}
	ret = devm_request_irq(&pdev->dev, irq, rts_thermal_irq_handler,
		IRQF_SHARED, "rts_thermal", pzone);

	pzone->therm_dev = thermal_zone_device_register("rts_thermal_zone",
		ptrips->num_trips, 0, pzone, &thdev_ops, NULL, 0, 0);

	if (IS_ERR(pzone->therm_dev)) {
		dev_err(&pdev->dev, "Register thermal zone device failed.\n");
		ret = PTR_ERR(pzone->therm_dev);
		goto failed;
	}
	dev_info(&pzone->therm_dev->device, "Thermal zone device registered.\n");

	platform_set_drvdata(pdev, pzone);

	schedule_work(&pzone->init_work);
failed:

	return ret;
}

static int rts_thermal_remove(struct platform_device *pdev)
{
	struct rts_thermal_zone *pzone = platform_get_drvdata(pdev);

	thermal_zone_device_unregister(pzone->therm_dev);
	cancel_delayed_work_sync(&pzone->therm_work);

	return 0;
}

static struct platform_driver rts_thermal_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "rts-thermal",
	},
	.probe = rts_thermal_probe,
	.remove = rts_thermal_remove,
};

module_platform_driver(rts_thermal_driver);
