/* Driver for Realtek RTS51xx USB card reader
 *
 * Copyright(c) 2009 Realtek Semiconductor Corp. All rights reserved.
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
 *   wwang (wei_wang@realsil.com.cn)
 *   No. 450, Shenhu Road, Suzhou Industry Park, Suzhou, China
 */

#include <linux/blkdev.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/errno.h>
#include <linux/freezer.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/utsname.h>
#include <linux/jiffies.h>
#include <linux/usb.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_devinfo.h>
#include <scsi/scsi_eh.h>
#include <scsi/scsi_host.h>

#include "debug.h"
#include "rts51x.h"
#include "rts51x_chip.h"
#include "rts51x_card.h"
#include "rts51x_scsi.h"
#include "rts51x_transport.h"
#include "rts51x_common.h"
#include "rts51x_fop.h"

MODULE_DESCRIPTION(RTS51X_DESC);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

#ifdef SCSI_SCAN_DELAY
static unsigned int delay_use = 5;
module_param(delay_use, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(delay_use, "seconds to delay before using a new device");
#endif

static int auto_delink_en = 0;
module_param(auto_delink_en, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(auto_delink_en, "enable auto delink");

static int ss_en = 0;
module_param(ss_en, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ss_en, "enable selective suspend");

static unsigned int ss_delay = 50;
module_param(ss_delay, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ss_delay, "seconds to delay before entering selective suspend");

static int needs_remote_wakeup = 0;
module_param(needs_remote_wakeup, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(needs_remote_wakeup, "ss state needs remote wakeup supported");

static int polling_pipe = 1;
module_param(polling_pipe, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(polling_pipe, "polling pipe (0: ctl, 1: bulk, 2: intr)");

static const char* rts5139_info(struct Scsi_Host *host)
{
	return "SCSI emulation for RTS5139 USB card reader";
}

struct scsi_host_template rts51x_host_template = {
	/* basic userland interface stuff */
	.name =				RTS51X_NAME,
	.proc_name =			RTS51X_NAME,
	.info =				rts5139_info,

	/* command interface -- queued only */
	.queuecommand =			queuecommand,

	/* error and abort handlers */
	.eh_abort_handler =		command_abort,
	.eh_device_reset_handler =	device_reset,
	.eh_bus_reset_handler =		bus_reset,

	/* queue commands only, only one command per LUN */
	.can_queue =			1,
	.cmd_per_lun =			1,

	/* unknown initiator id */
	.this_id =			-1,

	.slave_alloc =			slave_alloc,
	.slave_configure =		slave_configure,

	/* lots of sg segments can be handled */
	.sg_tablesize =			SG_ALL,

	/* limit the total size of a transfer to 120 KB */
	.max_sectors =                  240,

	/* merge commands... this seems to help performance, but
	 * periodically someone should test to see which setting is more
	 * optimal.
	 */
	.use_clustering =		1,

	/* emulated HBA */
	.emulated =			1,

	/* we do our own delay after a device or bus reset */
	.skip_settle_delay =		1,

	/* sysfs device attributes */
// 	.sdev_attrs =			sysfs_device_attr_list,

	/* module management */
	.module =			THIS_MODULE
};

#ifdef SUPPORT_FILE_OP
static const struct file_operations rts51x_fops = {
	.owner =		THIS_MODULE,
	.read =			rts51x_read,
	.write =		rts51x_write,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
	.ioctl =		rts51x_ioctl,
#else
	.unlocked_ioctl =	rts51x_ioctl,
#endif
	.open =			rts51x_open,
	.release =		rts51x_release,
};

#if  LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
static char *rts51x_devnode(struct device *dev, umode_t *mode)
{
	if (mode)
		*mode = 0666;

	return kasprintf(GFP_KERNEL, "%s", dev_name(dev));
}
#elif  LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 32)
static char *rts51x_devnode(struct device *dev, mode_t *mode)
{
	if (mode)
		*mode = 0666;

	return kasprintf(GFP_KERNEL, "%s", dev_name(dev));
}
#elif  LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 31)
static char *rts51x_nodename(struct device *dev)
{
	return kasprintf(GFP_KERNEL, "%s", dev_name(dev));
}
#endif

/*
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with the driver core
 */
static struct usb_class_driver rts51x_class = {
	.name =		"rts51x%d",
#if  LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 32)
	.devnode =	rts51x_devnode,
#else
#if  LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 31)
	.nodename = 	rts51x_nodename,
#endif
#endif
	.fops =		&rts51x_fops,
	.minor_base =	192,
};
#endif

#ifdef CONFIG_PM	/* Minimal support for suspend and resume */
void rts51x_try_to_enter_ss(struct rts51x_chip *chip)
{
	RTS51X_DEBUGP(("Ready to enter SS state\n"));
#if  LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
	usb_autopm_put_interface(chip->usb->pusb_intf);
#else
	usb_autopm_enable(chip->usb->pusb_intf);
#endif
}

void rts51x_try_to_exit_ss(struct rts51x_chip *chip)
{
	RTS51X_DEBUGP(("Exit from SS state\n"));
#if  LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
	usb_autopm_get_interface(chip->usb->pusb_intf);
#else
	usb_autopm_disable(chip->usb->pusb_intf);
#endif
}

int rts51x_suspend(struct usb_interface *iface, pm_message_t message)
{
	struct rts51x_chip *chip = usb_get_intfdata(iface);

	RTS51X_DEBUGP(("%s, message.event = 0x%x\n", __func__, message.event));

	/* Wait until no command is running */
	mutex_lock(&chip->usb->dev_mutex);

	chip->fake_card_ready = chip->card_ready;
	rts51x_do_before_power_down(chip);

	if (message.event == PM_EVENT_AUTO_SUSPEND) {
		RTS51X_DEBUGP(("Enter SS state"));
		chip->resume_from_scsi = 0;
		RTS51X_SET_STAT(chip, STAT_SS);
	} else {
		RTS51X_DEBUGP(("Enter SUSPEND state"));
		RTS51X_SET_STAT(chip, STAT_SUSPEND);
	}

	/* When runtime PM is working, we'll set a flag to indicate
	 * whether we should autoresume when a SCSI request arrives. */

	mutex_unlock(&chip->usb->dev_mutex);
	return 0;
}

int rts51x_resume(struct usb_interface *iface)
{
	struct rts51x_chip *chip = usb_get_intfdata(iface);

	RTS51X_DEBUGP(("%s\n", __func__));

	/* For remote wakeup: card inseration */
	if (RTS51X_CHK_STAT(chip, STAT_SS)) {
		if (!chip->resume_from_scsi) {
#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
			if (chip->option.ss_en) {
				if (GET_PM_USAGE_CNT(chip) <= 0) {
					RTS51X_DEBUGP(("Incr pm_usage_cnt\n"));
					SET_PM_USAGE_CNT(chip, 1);
				}
			}
#else
			usb_autopm_get_interface_async(iface);
#endif
			mutex_lock(&chip->usb->dev_mutex);
		}

		rts51x_init_cards(chip);

		if (!chip->resume_from_scsi)
			mutex_unlock(&chip->usb->dev_mutex);

	} else {
		mutex_lock(&chip->usb->dev_mutex);

		rts51x_init_chip(chip);
		rts51x_init_cards(chip);

		mutex_unlock(&chip->usb->dev_mutex);
	}

	RTS51X_SET_STAT(chip, STAT_RUN);

	return 0;
}

int rts51x_reset_resume(struct usb_interface *iface)
{
	struct rts51x_chip *chip = usb_get_intfdata(iface);

	RTS51X_DEBUGP(("%s\n", __func__));

	mutex_lock(&chip->usb->dev_mutex);

	RTS51X_SET_STAT(chip, STAT_RUN);

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
	if (chip->option.ss_en) {
		SET_PM_USAGE_CNT(chip, 1);
	}
#else
		usb_autopm_get_interface_async(iface);
#endif

	rts51x_init_chip(chip);
	rts51x_init_cards(chip);

	mutex_unlock(&chip->usb->dev_mutex);

	/* FIXME: Notify the subdrivers that they need to reinitialize
	 * the device */
	return 0;
}

#else /* CONFIG_PM */

void rts51x_try_to_enter_ss(struct rts51x_chip *chip)
{
}

void rts51x_try_to_exit_ss(struct rts51x_chip *chip)
{
}

#endif /* CONFIG_PM */

/*
 * The next two routines get called just before and just after
 * a USB port reset, whether from this driver or a different one.
 */

int rts51x_pre_reset(struct usb_interface *iface)
{
	struct rts51x_chip *chip = usb_get_intfdata(iface);

	RTS51X_DEBUGP(("%s\n", __func__));

	/* Make sure no command runs during the reset */
	mutex_lock(&chip->usb->dev_mutex);
	return 0;
}

int rts51x_post_reset(struct usb_interface *iface)
{
	struct rts51x_chip *chip = usb_get_intfdata(iface);

	RTS51X_DEBUGP(("%s\n", __func__));

	/* Report the reset to the SCSI core */
// 	usb_stor_report_bus_reset(us);

	/* FIXME: Notify the subdrivers that they need to reinitialize
	 * the device */

	mutex_unlock(&chip->usb->dev_mutex);
	return 0;
}

static int rts51x_control_thread(void * __chip)
{
	struct rts51x_chip *chip = (struct rts51x_chip *)__chip;
	struct Scsi_Host *host = rts51x_to_host(chip);

	for(;;) {
		if (wait_for_completion_interruptible(&chip->usb->cmnd_ready))
			break;

		if (test_bit(FLIDX_DISCONNECTING, &chip->usb->dflags)) {
			RTS51X_DEBUGP(("-- exiting from rts51x-control\n"));
			break;
		}

		/* lock the device pointers */
		mutex_lock(&(chip->usb->dev_mutex));

		/* lock access to the state */
		scsi_lock(host);

		/* When we are called with no command pending, we're done */
		if (chip->srb == NULL) {
			scsi_unlock(host);
			mutex_unlock(&chip->usb->dev_mutex);
			RTS51X_DEBUGP(("-- exiting from control thread\n"));
			break;
		}

		/* has the command timed out *already* ? */
		if (test_bit(FLIDX_TIMED_OUT, &chip->usb->dflags)) {
			chip->srb->result = DID_ABORT << 16;
			goto SkipForAbort;
		}

		scsi_unlock(host);

		/* reject the command if the direction indicator
		 * is UNKNOWN
		 */
		if (chip->srb->sc_data_direction == DMA_BIDIRECTIONAL) {
			RTS51X_DEBUGP(("UNKNOWN data direction\n"));
			chip->srb->result = DID_ERROR << 16;
		}

		/* reject if target != 0 or if LUN is higher than
		 * the maximum known LUN
		 */
		else if (chip->srb->device->id) {
			RTS51X_DEBUGP(("Bad target number (%d:%d)\n",
				  chip->srb->device->id, chip->srb->device->lun));
			chip->srb->result = DID_BAD_TARGET << 16;
		}

		else if (chip->srb->device->lun > chip->max_lun) {
			RTS51X_DEBUGP(("Bad LUN (%d:%d)\n",
				  chip->srb->device->id, chip->srb->device->lun));
			chip->srb->result = DID_BAD_TARGET << 16;
		}

		/* we've got a command, let's do it! */
		else {
			RTS51X_DEBUGDO(scsi_show_command(chip->srb));
			rts51x_invoke_transport(chip->srb, chip);
		}

		/* lock access to the state */
		scsi_lock(host);

		/* indicate that the command is done */
		if (chip->srb->result != DID_ABORT << 16) {
// 			RTS51X_DEBUGP(("scsi cmd done, result=0x%x\n",
// 				   chip->srb->result));
			chip->srb->scsi_done(chip->srb);
		} else {
SkipForAbort:
			RTS51X_DEBUGP(("scsi command aborted\n"));
		}

		/* If an abort request was received we need to signal that
		 * the abort has finished.  The proper test for this is
		 * the TIMED_OUT flag, not srb->result == DID_ABORT, because
		 * the timeout might have occurred after the command had
		 * already completed with a different result code. */
		if (test_bit(FLIDX_TIMED_OUT, &chip->usb->dflags)) {
			complete(&(chip->usb->notify));

			/* Allow USB transfers to resume */
			clear_bit(FLIDX_ABORTING, &chip->usb->dflags);
			clear_bit(FLIDX_TIMED_OUT, &chip->usb->dflags);
		}

		/* finished working on this command */
		chip->srb = NULL;
		scsi_unlock(host);

		/* unlock the device pointers */
		mutex_unlock(&chip->usb->dev_mutex);
	} /* for (;;) */

	complete(&chip->usb->control_exit);

	/* Wait until we are told to stop */
/*	for (;;) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			break;
		schedule();
	}
	__set_current_state(TASK_RUNNING);*/
	return 0;
}

void mspro_polling_format_status(struct rts51x_chip *chip);

static int rts51x_polling_thread(void * __chip)
{
	struct rts51x_chip *chip = (struct rts51x_chip *)__chip;

#ifdef SCSI_SCAN_DELAY
	// Wait until SCSI scan finished
	wait_timeout((delay_use + 5) * HZ);
#endif

	for(;;) {
		wait_timeout(POLLING_INTERVAL);

		/* if the device has disconnected, we are free to exit */
		if (test_bit(FLIDX_DISCONNECTING, &chip->usb->dflags)) {
			RTS51X_DEBUGP(("-- exiting from rts51x-polling\n"));
			break;
		}

		/* if the device has disconnected, we are free to exit */
// 		if (kthread_should_stop()) {
// 			printk(KERN_INFO "Stop polling thread!\n");
// 			break;
// 		}

#ifdef CONFIG_PM
		if (RTS51X_CHK_STAT(chip, STAT_SS) ||
				RTS51X_CHK_STAT(chip, STAT_SS_PRE) ||
				RTS51X_CHK_STAT(chip, STAT_SUSPEND)) {
			continue;
		}

		if (ss_en) {
			if (RTS51X_CHK_STAT(chip, STAT_IDLE)) {
				if (time_after(jiffies, chip->ss_enter_time))
				{
					rts51x_try_to_enter_ss(chip);
					RTS51X_SET_STAT(chip, STAT_SS_PRE);
					continue;
				}
			}
			else
				chip->ss_enter_time = jiffies + msecs_to_jiffies(ss_delay * 1000);
		}
#endif

		mspro_polling_format_status(chip);

		/* lock the device pointers */
		mutex_lock(&(chip->usb->dev_mutex));

		rts51x_polling_func(chip);

		/* unlock the device pointers */
		mutex_unlock(&chip->usb->dev_mutex);
	} /* for (;;) */

	complete(&chip->usb->polling_exit);

	/* Wait until we are told to stop */
// 	for (;;) {
// 		set_current_state(TASK_INTERRUPTIBLE);
// 		if (kthread_should_stop())
// 			break;
// 		schedule();
// 	}
// 	__set_current_state(TASK_RUNNING);
	return 0;
}

#ifdef SCSI_SCAN_DELAY
/* Thread to carry out delayed SCSI-device scanning */
static int rts51x_scan_thread(void * __chip)
{
	struct rts51x_chip *chip = (struct rts51x_chip *)__chip;

	printk(KERN_DEBUG
		"rts51x: device found at %d\n", chip->usb->pusb_dev->devnum);

	set_freezable();
	/* Wait for the timeout to expire or for a disconnect */
	if (delay_use > 0) {
		printk(KERN_DEBUG "rts51x: waiting for device "
				"to settle before scanning\n");
		wait_event_freezable_timeout(chip->usb->delay_wait,
				test_bit(FLIDX_DONT_SCAN, &chip->usb->dflags),
				delay_use * HZ);
	}

	/* If the device is still connected, perform the scanning */
	if (!test_bit(FLIDX_DONT_SCAN, &chip->usb->dflags)) {
		scsi_scan_host(rts51x_to_host(chip));
		printk(KERN_DEBUG "rts51x: device scan complete\n");

		/* Should we unbind if no devices were detected? */
	}

	complete_and_exit(&chip->usb->scanning_done, 0);
}
#endif

/* Associate our private data with the USB device */
static int associate_dev(struct rts51x_chip *chip, struct usb_interface *intf)
{
	struct rts51x_usb *rts51x = chip->usb;
#ifdef SUPPORT_FILE_OP
	int retval;
#endif

	/* Fill in the device-related fields */
	rts51x->pusb_dev = interface_to_usbdev(intf);
	rts51x->pusb_intf = intf;
	rts51x->ifnum = intf->cur_altsetting->desc.bInterfaceNumber;
	RTS51X_DEBUGP(("Vendor: 0x%04x, Product: 0x%04x, Revision: 0x%04x\n",
			le16_to_cpu(rts51x->pusb_dev->descriptor.idVendor),
			le16_to_cpu(rts51x->pusb_dev->descriptor.idProduct),
			le16_to_cpu(rts51x->pusb_dev->descriptor.bcdDevice)));
	RTS51X_DEBUGP(("Interface Subclass: 0x%02x, Protocol: 0x%02x\n",
			intf->cur_altsetting->desc.bInterfaceSubClass,
			intf->cur_altsetting->desc.bInterfaceProtocol));

	/* Store our private data in the interface */
	usb_set_intfdata(intf, chip);

#ifdef SUPPORT_FILE_OP
	/* we can register the device now, as it is ready */
	retval = usb_register_dev(intf, &rts51x_class);
	if (retval) {
		/* something prevented us from registering this driver */
		RTS51X_DEBUGP(("Not able to get a minor for this device."));
		usb_set_intfdata(intf, NULL);
		return -ENOMEM;
	}
#endif
	/* Always allocate control request buffer by kmalloc
	 * and leave all DMA things to kernel */
	rts51x->cr = kmalloc(sizeof(*rts51x->cr),GFP_KERNEL);
	if (!rts51x->cr) {
		RTS51X_DEBUGP(("usb_ctrlrequest allocation failed\n"));
		usb_set_intfdata(intf, NULL);
		return -ENOMEM;
	}

	/* Allocate the device-related DMA-mapped buffers */
	rts51x->iobuf = usb_buffer_alloc(rts51x->pusb_dev, RTS51X_IOBUF_SIZE,
			GFP_KERNEL, &rts51x->iobuf_dma);
	if (!rts51x->iobuf) {
		RTS51X_DEBUGP(("I/O buffer allocation failed\n"));
		usb_set_intfdata(intf, NULL);
		return -ENOMEM;
	}
	return 0;
}

static void rts51x_init_options(struct rts51x_chip *chip)
{
	struct rts51x_option *option = &(chip->option);

	option->led_blink_speed = 7;
	option->mspro_formatter_enable = 1;

	option->fpga_sd_sdr104_clk = CLK_100;
	option->fpga_sd_sdr50_clk = CLK_100;
	option->fpga_sd_ddr50_clk = CLK_100;
	option->fpga_sd_hs_clk = CLK_100;
	option->fpga_mmc_52m_clk = CLK_80;
	option->fpga_ms_hg_clk = CLK_80;
	option->fpga_ms_4bit_clk = CLK_80;

	option->asic_sd_sdr104_clk = 98;
	option->asic_sd_sdr50_clk = 98;
	option->asic_sd_ddr50_clk = 98;
	option->asic_sd_hs_clk = 97;
	option->asic_mmc_52m_clk = 95;
	option->asic_ms_hg_clk = 116;
	option->asic_ms_4bit_clk = 77;

	option->sd_ddr_tx_phase = 0;
	option->mmc_ddr_tx_phase = 1;

	option->sd_speed_prior = 0;
	option->sd_ctl = SD_PUSH_POINT_AUTO | SD_SAMPLE_POINT_AUTO | SUPPORT_UHS50_MMC44;

	option->ss_en = ss_en;
	option->ss_delay = ss_delay;
	option->needs_remote_wakeup = needs_remote_wakeup;

	option->auto_delink_en = auto_delink_en;

	option->FT2_fast_mode = 0;
	option->pwr_delay = 800;
	option->xd_rw_step = 0;
	option->D3318_off_delay = 50;
	option->delink_delay = 100;
	option->rts5129_D3318_off_enable = 0;
	option->sd20_pad_drive = 0;
	option->reset_or_rw_fail_set_pad_drive = 1;
	option->rcc_fail_flag = 0;
	option->rcc_bug_fix_en = 1;
	option->debounce_num = 2;
	option->polling_time = 100;
	option->led_toggle_interval = 6;
	option->xd_rwn_step = 0;
	option->sd_send_status_en = 0;
	option->sdr50_tx_phase = 0x01;
	option->sdr50_rx_phase = 0x05;
	option->ddr50_tx_phase = 0x09;
	option->ddr50_rx_phase = 0x06;
	option->sdr50_phase_sel = 0;
	option->sd30_pad_drive = 1;
	option->ms_errreg_fix = 0;
	option->reset_mmc_first = 0;
	option->speed_mmc = 1;
	option->led_always_on = 0;

	option->polling_pipe = polling_pipe;
	option->support_mmc = 1;
	option->sof_wd_en = 0;
	option->ocp_en = 1;
	option->ocp_deglitch_time = 0x5;
	option->ocp_para = 0x3;

	option->dev_flags = TURN_ON_LED_AT_START | CHECK_MEDIA_CHANGE;

	option->cprm_cmd_timeout = 20000;
}

/* Get the pipe settings */
static int get_pipes(struct rts51x_chip *chip)
{
	struct rts51x_usb *rts51x = chip->usb;
	struct usb_host_interface *altsetting =
		rts51x->pusb_intf->cur_altsetting;
	int i;
	struct usb_endpoint_descriptor *ep;
	struct usb_endpoint_descriptor *ep_in = NULL;
	struct usb_endpoint_descriptor *ep_out = NULL;
	struct usb_endpoint_descriptor *ep_int = NULL;

	/*
	 * Find the first endpoint of each type we need.
	 * We are expecting a minimum of 2 endpoints - in and out (bulk).
	 * An optional interrupt-in is OK (necessary for CBI protocol).
	 * We will ignore any others.
	 */
	for (i = 0; i < altsetting->desc.bNumEndpoints; i++) {
		ep = &altsetting->endpoint[i].desc;

		if (usb_endpoint_xfer_bulk(ep)) {
			if (usb_endpoint_dir_in(ep)) {
				if (!ep_in)
					ep_in = ep;
			} else {
				if (!ep_out)
					ep_out = ep;
			}
		}

		else if (usb_endpoint_is_int_in(ep)) {
			if (!ep_int)
				ep_int = ep;
		}
	}

	if (!ep_in || !ep_out) {
		RTS51X_DEBUGP(("Endpoint sanity check failed! Rejecting dev.\n"));
		return -EIO;
	}

	/* Calculate and store the pipe values */
	rts51x->send_ctrl_pipe = usb_sndctrlpipe(rts51x->pusb_dev, 0);
	rts51x->recv_ctrl_pipe = usb_rcvctrlpipe(rts51x->pusb_dev, 0);
	rts51x->send_bulk_pipe = usb_sndbulkpipe(rts51x->pusb_dev,
		usb_endpoint_num(ep_out));
	rts51x->recv_bulk_pipe = usb_rcvbulkpipe(rts51x->pusb_dev,
		usb_endpoint_num(ep_in));
	if (ep_int) {
		rts51x->recv_intr_pipe = usb_rcvintpipe(rts51x->pusb_dev,
			usb_endpoint_num(ep_int));
		rts51x->ep_bInterval = ep_int->bInterval;
	}
	return 0;
}

/* Initialize all the dynamic resources we need */
static int rts51x_acquire_resources(struct rts51x_chip *chip)
{
	struct rts51x_usb *rts51x = chip->usb;
	int retval;

	rts51x->current_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!rts51x->current_urb) {
		RTS51X_DEBUGP(("URB allocation failed\n"));
		return -ENOMEM;
	}

	rts51x->intr_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!rts51x->intr_urb) {
		RTS51X_DEBUGP(("URB allocation failed\n"));
		return -ENOMEM;
	}

	chip->cmd_buf = chip->rsp_buf = rts51x->iobuf;

	rts51x_init_options(chip);

	// Init rts51xx device
	// ...
	retval = rts51x_init_chip(chip);
	if (retval != STATUS_SUCCESS) {
		return -EIO;
	}

	return 0;
}

/* Release all our dynamic resources */
static void rts51x_release_resources(struct rts51x_chip *chip)
{
	RTS51X_DEBUGP(("-- %s\n", __func__));

	/* Tell the control thread to exit.  The SCSI host must
	 * already have been removed and the DISCONNECTING flag set
	 * so that we won't accept any more commands.
	 */
	RTS51X_DEBUGP(("-- sending exit command to thread\n"));
	complete(&chip->usb->cmnd_ready);
	if (chip->usb->ctl_thread)
		wait_for_completion(&chip->usb->control_exit);
//		kthread_stop(chip->usb->ctl_thread);
	if (chip->usb->polling_thread)
		wait_for_completion(&chip->usb->polling_exit);

// 	if (chip->usb->polling_thread)
// 		kthread_stop(chip->usb->polling_thread);

	wait_timeout(200);

	// Release rts51xx device here
	// ...
	rts51x_release_chip(chip);

	usb_free_urb(chip->usb->current_urb);
	usb_free_urb(chip->usb->intr_urb);
}

/* Dissociate from the USB device */
static void dissociate_dev(struct rts51x_chip *chip)
{
	struct rts51x_usb *rts51x = chip->usb;

	RTS51X_DEBUGP(("-- %s\n", __func__));

	/* Free buffers */
	kfree(rts51x->cr);
	usb_buffer_free(rts51x->pusb_dev, RTS51X_IOBUF_SIZE, rts51x->iobuf,
			rts51x->iobuf_dma);

	/* Remove our private data from the interface */
	usb_set_intfdata(rts51x->pusb_intf, NULL);

#ifdef SUPPORT_FILE_OP
	/* give back our minor */
	usb_deregister_dev(rts51x->pusb_intf, &rts51x_class);
#endif

	kfree(rts51x);
	chip->usb = NULL;
}

/* First stage of disconnect processing: stop SCSI scanning,
 * remove the host, and stop accepting new commands
 */
static void quiesce_and_remove_host(struct rts51x_chip *chip)
{
	struct rts51x_usb *rts51x = chip->usb;
	struct Scsi_Host *host = rts51x_to_host(chip);

	/* If the device is really gone, cut short reset delays */
	if (rts51x->pusb_dev->state == USB_STATE_NOTATTACHED)
		set_bit(FLIDX_DISCONNECTING, &rts51x->dflags);

#ifdef SCSI_SCAN_DELAY
	/* Prevent SCSI-scanning (if it hasn't started yet)
	 * and wait for the SCSI-scanning thread to stop.
	 */
	set_bit(FLIDX_DONT_SCAN, &rts51x->dflags);
	wake_up(&rts51x->delay_wait);
	wait_for_completion(&rts51x->scanning_done);
#endif

	/* Removing the host will perform an orderly shutdown: caches
	 * synchronized, disks spun down, etc.
	 */
	scsi_remove_host(host);

	/* Prevent any new commands from being accepted and cut short
	 * reset delays.
	 */
	scsi_lock(host);
	set_bit(FLIDX_DISCONNECTING, &rts51x->dflags);
	scsi_unlock(host);
#ifdef SCSI_SCAN_DELAY
	wake_up(&rts51x->delay_wait);
#endif
	/* Balance runtime_usage */
	if(!(RTS51X_CHK_STAT(chip, STAT_SS) || RTS51X_CHK_STAT(chip, STAT_SS_PRE)))
		usb_autopm_put_interface(chip->usb->pusb_intf);
}

/* Second stage of disconnect processing: deallocate all resources */
static void release_everything(struct rts51x_chip *chip)
{
	rts51x_release_resources(chip);
	dissociate_dev(chip);

	kfree(chip->polling_card_status_buf);
	/* Drop our reference to the host; the SCSI core will free it
	 * (and "chip" along with it) when the refcount becomes 0. */
	scsi_host_put(rts51x_to_host(chip));
}

static int rts51x_probe(struct usb_interface *intf,
			 const struct usb_device_id *id)
{
	struct Scsi_Host *host;
	struct rts51x_chip *chip;
	struct rts51x_usb *rts51x;
	int result;
	struct task_struct *th;

	RTS51X_DEBUGP(("%s detected\n", RTS51X_NAME));

	rts51x = kzalloc(sizeof(struct rts51x_usb), GFP_KERNEL);
	if (!rts51x) {
		printk(KERN_WARNING RTS51X_TIP
			"Unable to allocate rts51x_usb\n");
		return -ENOMEM;
	}

	/*
	 * Ask the SCSI layer to allocate a host structure, with extra
	 * space at the end for our private us_data structure.
	 */
	host = scsi_host_alloc(&rts51x_host_template, sizeof(*chip));
	if (!host) {
		printk(KERN_WARNING RTS51X_TIP
			"Unable to allocate the scsi host\n");
		kfree(rts51x);
		return -ENOMEM;
	}

	/*
	 * Allow 16-byte CDBs and thus > 2TB
	 */
	host->max_cmd_len = 16;
	chip = host_to_rts51x(host);
	memset(chip, 0, sizeof(struct rts51x_chip));

	chip->polling_card_status_buf = kmalloc(sizeof(u16), GFP_KERNEL);
	if (!chip->polling_card_status_buf) {
		scsi_host_put(host);
		kfree(rts51x);
		return -ENOMEM;
	}

	chip->vendor_id = id->idVendor;
	chip->product_id = id->idProduct;

	mutex_init(&(rts51x->dev_mutex));
	init_completion(&rts51x->cmnd_ready);
	init_completion(&rts51x->control_exit);
	init_completion(&rts51x->polling_exit);
	init_completion(&(rts51x->notify));
#ifdef SCSI_SCAN_DELAY
	init_waitqueue_head(&rts51x->delay_wait);
	init_completion(&rts51x->scanning_done);
#endif

	chip->usb = rts51x;

	/* Associate the us_data structure with the USB device */
	result = associate_dev(chip, intf);
	if (result)
		goto BadDevice;

	/* Find the endpoints and calculate pipe values */
	result = get_pipes(chip);
	if (result)
		goto BadDevice;

	/* Acquire all the other resources and add the host */
	result = rts51x_acquire_resources(chip);
	if (result)
		goto BadDevice;

	/* Start up our control thread */
	th = kthread_run(rts51x_control_thread, chip, RTS51X_CTL_THREAD);
	if (IS_ERR(th)) {
		printk(KERN_WARNING RTS51X_TIP
		       "Unable to start control thread\n");
		result = PTR_ERR(th);
		goto BadDevice;
	}
	rts51x->ctl_thread = th;

	result = scsi_add_host(rts51x_to_host(chip), &rts51x->pusb_intf->dev);
	if (result) {
		printk(KERN_WARNING RTS51X_TIP
			"Unable to add the scsi host\n");
		goto BadDevice;
	}

#ifdef SCSI_SCAN_DELAY
	/* Start up the thread for delayed SCSI-device scanning */
	th = kthread_create(rts51x_scan_thread, chip, RTS51X_SCAN_THREAD);
	if (IS_ERR(th)) {
		printk(KERN_WARNING RTS51X_TIP
		       "Unable to start the device-scanning thread\n");
		complete(&rts51x->scanning_done);
		quiesce_and_remove_host(chip);
		result = PTR_ERR(th);
		goto BadDevice;
	}

	wake_up_process(th);
#else
	scsi_scan_host(rts51x_to_host(chip));
#endif

	/* Start up our polling thread */
	th = kthread_run(rts51x_polling_thread, chip, RTS51X_POLLING_THREAD);
	if (IS_ERR(th)) {
		printk(KERN_WARNING RTS51X_TIP
		       "Unable to start polling thread\n");
		result = PTR_ERR(th);
		goto BadDevice;
	}
	rts51x->polling_thread = th;

#ifdef CONFIG_PM
	if (ss_en) {
		rts51x->pusb_intf->needs_remote_wakeup = needs_remote_wakeup;
		if (needs_remote_wakeup)
			chip->option.ww_enable = 1;
#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
		SET_PM_USAGE_CNT(chip, 1);
		RTS51X_DEBUGP(("pm_usage_cnt = %d\n", GET_PM_USAGE_CNT(chip)));
#else
		usb_autopm_get_interface(rts51x->pusb_intf);
#endif

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
		rts51x->pusb_dev->autosuspend_disabled = 0;
#else
		usb_enable_autosuspend(rts51x->pusb_dev);
		pm_suspend_ignore_children(&rts51x->pusb_intf->dev, true);
#endif
	}
#endif

	return 0;

	/* We come here if there are any problems */
BadDevice:
	RTS51X_DEBUGP(("rts51x_probe() failed\n"));
	release_everything(chip);
	return result;
}

static void rts51x_disconnect(struct usb_interface *intf)
{
	struct rts51x_chip *chip = (struct rts51x_chip *)usb_get_intfdata(intf);

	RTS51X_DEBUGP(("rts51x_disconnect() called\n"));
	quiesce_and_remove_host(chip);
	release_everything(chip);
}

/***********************************************************************
 * Initialization and registration
 ***********************************************************************/

struct usb_device_id rts5139_usb_ids[] = {
	{ USB_DEVICE(0x0BDA, 0x0139) },
	{ USB_DEVICE(0x0BDA, 0x0129) },
	{ USB_DEVICE(0x0BDA, 0x0140) },
	{ }		/* Terminating entry */
};
EXPORT_SYMBOL_GPL(rts5139_usb_ids);

MODULE_DEVICE_TABLE(usb, rts5139_usb_ids);

struct usb_driver rts51x_driver = {
	.name =		RTS51X_NAME,
	.probe =	rts51x_probe,
	.disconnect =	rts51x_disconnect,
	.suspend =	rts51x_suspend,
	.resume =	rts51x_resume,
	.reset_resume =	rts51x_reset_resume,
	.pre_reset =	rts51x_pre_reset,
	.post_reset =	rts51x_post_reset,
	.id_table =	rts5139_usb_ids,
	.soft_unbind =	1,
};

static int __init rts51x_init(void)
{
	int retval;

	printk(KERN_INFO "Initializing %s USB card reader driver...\n", RTS51X_NAME);

	if (ss_en)
		rts51x_driver.supports_autosuspend = 1;

	/* register the driver, return usb_register return code if error */
	retval = usb_register(&rts51x_driver);
	if (retval == 0) {
		printk(KERN_INFO "Realtek %s USB card reader support registered.\n", RTS51X_NAME);
	}
	return retval;
}

static void __exit rts51x_exit(void)
{
	RTS51X_DEBUGP(("rts51x_exit() called\n"));

	/* Deregister the driver
	 * This will cause disconnect() to be called for each
	 * attached unit
	 */
	RTS51X_DEBUGP(("-- calling usb_deregister()\n"));
	usb_deregister(&rts51x_driver) ;
}

module_init(rts51x_init);
module_exit(rts51x_exit);
