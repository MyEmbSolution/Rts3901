/*
 * rtp-irq.c  --  Realsil RTP
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/bug.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/mfd/rtp-mfd.h>
#include <linux/kthread.h>

static void rtp_irq_work(struct work_struct *work)
{
	struct rtp_mfd_chip *chip = container_of(work,
						     struct rtp_mfd_chip,
						     irq_work);
	uint64_t irqs = 0;

	pm_stay_awake(chip->dev);
	while (1) {
		if (chip->read_irqs(chip, &irqs))
			break;

		irqs &= chip->irq_enable;
		if (irqs == 0)
			break;

		blocking_notifier_call_chain(&chip->notifier_list, 0, chip);
	}
	enable_irq(chip->chip_irq);
	pm_relax(chip->dev);
}

static irqreturn_t rtp_irq(int irq, void *data)
{
	struct rtp_mfd_chip *chip = data;

	disable_irq_nosync(irq);
	(void) schedule_work(&chip->irq_work);

	return IRQ_HANDLED;
}

int rtp_irq_init(struct rtp_mfd_chip *chip, int irq)
{
	int ret;

	printk(KERN_DEBUG "Init rtp irq, irq = %d\n", irq);

	if (!irq) {
		dev_warn(chip->dev, "No interrupt support, no core IRQ\n");
		return 0;
	}

	/* Clear unattended interrupts */
	chip->write_irqs(chip, 0xffffffff | (uint64_t) 0xffffffff << 32);

	mutex_init(&chip->irq_lock);
	chip->chip_irq = irq;
	INIT_WORK(&chip->irq_work, rtp_irq_work);
	BLOCKING_INIT_NOTIFIER_HEAD(&chip->notifier_list);

	ret = request_irq(irq, rtp_irq, IRQF_DISABLED, "rtp_mfd",
			  chip);
	if (ret != 0) {
		dev_err(chip->dev, "Failed to request IRQ: %d\n", ret);
		return ret;
	}

	irq_set_irq_type(irq, IRQ_TYPE_LEVEL_LOW);
	device_init_wakeup(chip->dev, 1);

	return ret;
}

int rtp_irq_exit(struct rtp_mfd_chip *chip)
{
	printk(KERN_DEBUG "Exit rtp irq\n");

	if (chip->chip_irq)
		free_irq(chip->chip_irq, chip);
	return 0;
}
