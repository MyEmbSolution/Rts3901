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
 *   Micky Ching <micky_ching@realsil.com.cn>
 *   No. 128, West Shenhu Road, Suzhou Industry Park, Suzhou, China
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/idr.h>
#include <linux/platform_device.h>
#include <linux/completion.h>
#include <linux/mfd/core.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <asm/unaligned.h>

#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/card.h>

#include "rtsx-icr-regs.h"
#include "rtsx-icr.h"

enum rtsx_sdhc_devtype {
	TYPE_FPGA = (1 << 0),
	TYPE_RLE0745 = (1 << 1),
	TYPE_RTS3901 = (1 << 2),
};

static struct platform_device_id rtsx_sdhc_devtypes[] = {
	{
		.name = "rle0745-fpga-sdhc",
		.driver_data = TYPE_RLE0745 | TYPE_FPGA,
	}, {
		.name = "rle0745-sdhc",
		.driver_data = TYPE_RLE0745,
	}, {
		.name = "rts3901-fpga-sdhc",
		.driver_data = TYPE_RTS3901 | TYPE_FPGA,
	}, {
		.name = "rts3901-sdhc",
		.driver_data = TYPE_RTS3901,
	},
	{ }
};

static void dump_reg_range(struct rtsx_icr *icr, u16 start, u16 end)
{
	u16 len = end - start + 1;
	int i;
	u8 *data = kzalloc(8, GFP_KERNEL);

	if (!data)
		return;

	for (i = 0; i < len; i += 8, start += 8) {
		int j, n = min(8, len - i);

		for (j = 0; j < n; j++)
			rtsx_icr_reg_read(icr, start + j, data + j);
		icr_err(icr, "0x%04X(%d): %8ph\n", start, n, data);
	}

	kfree(data);
}

static void rtsx_icr_dump_regs(struct rtsx_icr *icr)
{
	dump_reg_range(icr, 0xFF00, 0xFF1F);
}

static void rtsx_icr_dump_all_regs(struct rtsx_icr *icr)
{
	dump_reg_range(icr, 0xFD50, 0xFD6F);
	dump_reg_range(icr, 0xFF00, 0xFF5F);
}


void rtsx_icr_remove_card(struct rtsx_icr *icr)
{
	if (icr->trigger_gpio)
		gpio_free(icr->trigger_gpio);

	icr->card_status &= ~SD_EXIST;
	mmc_detect_change(icr->mmc, 0);
}

void rtsx_icr_insert_card(struct rtsx_icr *icr)
{
	if (icr->trigger_gpio) {
		gpio_request(icr->trigger_gpio, "TRIGGER_GPIO");
		gpio_direction_output(icr->trigger_gpio, 0);
		gpio_set_value(icr->trigger_gpio, 1);
	}

	icr->card_status |= SD_EXIST;
	icr->data_errors = 0;
	icr->short_dma_mode = 0;
	mmc_detect_change(icr->mmc, 0);
}

/*
 * pull control rule.
 * | SD pull control | enable | disable |
 * +-----------------+--------+---------+
 * | SD_DAT[3:0]     | up     | down    |
 * | SD_CD           | up     | up      |
 * | SD_WP           | up     | down    |
 * | SD_CMD          | up     | down    |
 * | SD_CLK          | down   | down    |
 */
static const u32 rtsx_icr_pull_ctl_enable_tbl[] = {
	rtsx_icr_control_table(SDDAT_L_PULL_CTL, 0xAA),
	rtsx_icr_control_table(SDCMD_PULL_CTL, 0xA9),
	0,
};

static const u32 rtsx_icr_pull_ctl_disable_tbl[] = {
	rtsx_icr_control_table(SDDAT_L_PULL_CTL, 0x55),
	rtsx_icr_control_table(SDCMD_PULL_CTL, 0x95),
	0,
};

static void rtsx_icr_enable_bus_int(struct rtsx_icr *icr)
{
	icr->bier = DATA_DONE_INT | TRANS_OK | TRANS_FAIL |
		SD_INT | SD_EXIST | CARD_ERR_INT;
	rtsx_icr_writel(icr, BIER, icr->bier);
	icr_dbg(icr, "BIER: 0x%08x\n", icr->bier);
}

static void rtsx_icr_enable_end_int(struct rtsx_icr *icr)
{
	icr->bier |= SDHOST_END_INT;
	rtsx_icr_writel(icr, BIER, icr->bier);
}

static void rtsx_icr_disable_end_int(struct rtsx_icr *icr)
{
	icr->bier &= ~SDHOST_END_INT;
	rtsx_icr_writel(icr, BIER, icr->bier);
}

void __rtsx_icr_add_cmd(struct rtsx_icr *icr, u8 cmd_type, u16 reg_addr,
		u8 mask, u8 data)
{
	u32 val = 0;
	u32 *ptr = (u32 *)(icr->cmd_ptr) + icr->cmd_idx;

	BUG_ON(icr->cmd_idx >= CMD_NUM_MAX);

	val |= (u32)(cmd_type & 0x03) << 30;
	val |= (u32)(reg_addr & 0x3FFF) << 16 | (u32)mask << 8 | (u32)data;

	put_unaligned_le32(val, ptr);
	ptr++;
	icr->cmd_idx++;
}

void rtsx_icr_send_cmd(struct rtsx_icr *icr)
	__releases(&icr->__lock)
{
	u32 val = CMD_START | (icr->cmd_idx * 4 & CMD_LEN_MASK);

	/* To guarantee the data in command buffer
	 * be flushed into DDR before CMD_START
	 */
	iob();

	reinit_completion(&icr->done_cmd);
	icr->done = &icr->done_cmd;
	icr->trans_status = TRANS_READY;
	rtsx_icr_writel(icr, HCBAR, icr->cmd_addr);
	rtsx_icr_writel(icr, HCBCTLR, val);
	rtsx_icr_unlock_irqrestore(icr);
}

int rtsx_icr_wait_cmd(struct rtsx_icr *icr, int timeout)
{
	long timeleft;
	int err = 0;

	timeleft = wait_for_completion_interruptible_timeout(icr->done,
			msecs_to_jiffies(timeout));
	if (timeleft <= 0) {
		err = -ETIMEDOUT;
		goto out;
	}

	rtsx_icr_lock_irqsave(icr);
	if (icr->trans_status == TRANS_FAIL)
		err = -EIO;
	else if (icr->trans_status != TRANS_OK)
		err = -EINVAL;
	rtsx_icr_unlock_irqrestore(icr);

out:
	rtsx_icr_lock_irqsave(icr);
	icr->done = NULL;
	rtsx_icr_unlock_irqrestore(icr);

	if (err < 0) {
		icr_err(icr, "%d: HCBAR = %08X, HCBCTLR = %08X\n",
			err, rtsx_icr_readl(icr, HCBAR),
			rtsx_icr_readl(icr, HCBCTLR));
		if (!icr->removed)
			rtsx_icr_stop_cmd(icr);
		rtsx_icr_dump_regs(icr); /* should place after stop */
	}

	return err;
}

int rtsx_icr_reg_read(struct rtsx_icr *icr, u16 addr, u8 *data)
{
	u32 val = HAIMR_START_READ | ((u32)(addr & 0x3FFF) << 16);
	int i;

	rtsx_icr_writel(icr, HAIMR, val);

	for (i = 0; i < REG_RW_RETRY_CNT; i++) {
		val = rtsx_icr_readl(icr, HAIMR);
		if (!(val & HAIMR_START)) {
			if (data)
				*data = val & 0xFF;
			return 0;
		}
	}

	icr_err(icr, "read register 0x%02X failed\n", addr);
	return -ETIMEDOUT;
}

int rtsx_icr_reg_write(struct rtsx_icr *icr, u16 addr, u8 mask, u8 data)
{
	int i;
	u32 val = HAIMR_START_WRITE |
		((u32)(addr & 0x3FFF) << 16 | (u32)mask << 8 | (u32)data);

	rtsx_icr_writel(icr, HAIMR, val);

	for (i = 0; i < REG_RW_RETRY_CNT; i++) {
		val = rtsx_icr_readl(icr, HAIMR);
		if (!(val & HAIMR_START)) {
			if (data != (u8)val)
				return -EIO;
			return 0;
		}
	}

	icr_err(icr, "write register 0x%02X failed\n", addr);
	return -ETIMEDOUT;
}

static int __rtsx_icr_do_pp_rw(struct rtsx_icr *icr, u16 offset,
	u8 *buf, int len, int read)
{
	int i, err = 0;
	u8 *ptr = buf;
	u16 reg = PINGPONG_BUF + offset;

	BUG_ON(len > CMD_NUM_MAX);

	rtsx_icr_init_cmd(icr);
	if (read)
		for (i = 0; i < len; ++i)
			rtsx_icr_read(icr, reg++);
	else
		for (i = 0; i < len; ++i)
			rtsx_icr_write(icr, reg++, 0xFF, *ptr++);
	err = rtsx_icr_transfer_cmd(icr);
	if (err < 0)
		return err;

	if (read)
		memcpy(ptr, rtsx_icr_get_data(icr), len);

	return err;
}

static int __rtsx_icr_pp_rw(struct rtsx_icr *icr, u8 *buf, int len,
		int read)
{
	int err = 0, i;
	int step = CMD_NUM_MAX;
	int q_len = len / step;
	int r_len = len % step;
	u8 *ptr = buf;

	if (!buf || len <= 0)
		return 0;

	BUG_ON(len > 512);

	for (i = 0; i < q_len; i++, ptr += step) {
		err = __rtsx_icr_do_pp_rw(icr, ptr - buf, ptr, step, read);
		if (err)
			return err;
	}

	if (r_len) {
		err = __rtsx_icr_do_pp_rw(icr, ptr - buf, ptr, r_len, read);
		if (err)
			return err;
	}

	return err;
}

int rtsx_icr_pp_read(struct rtsx_icr *icr, u8 *buf, int len)
{
	return __rtsx_icr_pp_rw(icr, buf, len, true);
}

int rtsx_icr_pp_write(struct rtsx_icr *icr, u8 *buf, int len)
{
	return __rtsx_icr_pp_rw(icr, buf, len, false);
}

static void rtsx_icr_dump_dma_table(struct rtsx_icr *icr)
{
	u64 *ptr = (u64 *)(icr->sg_tbl_ptr);
	int i;

	for (i = 0; i < icr->sg_tbl_idx; i++, ptr++) {
		u64 val = get_unaligned_le64(ptr);

		icr_err(icr, "ADMA table at %08X: %08x, %04x, %04x\n",
			icr->sg_tbl_addr + 8 * i,
			(u32)(val >> 32), (u16)(val >> 16), (u16)val);
	}
}

static void rtsx_icr_add_sg_tbl(struct rtsx_icr *icr, dma_addr_t addr,
		unsigned int len, int end)
{
	u64 *ptr = (u64 *)(icr->sg_tbl_ptr) + icr->sg_tbl_idx;
	u64 val;
	u8 option = SG_VALID | SG_TRANS_DATA | (end ? SG_END : 0);

	BUG_ON(len > SG_LEN_MAX);

	if (len == SG_LEN_MAX)
		len = 0;		/* len = 0 means SG_LEN_MAX */

	val = ((u64)addr << 32) | ((u64)len << 16) | option;
	icr_dbg(icr, "ADMA table at %08X: %08x, %04x, %04x\n",
		(unsigned)ptr,
		(u32)(val >> 32), (u16)(val >> 16), (u16)val);

	put_unaligned_le64(val, ptr);
	icr->sg_tbl_idx++;
}

int rtsx_icr_transfer_data(struct rtsx_icr *icr, struct mmc_data *data)
{
	struct completion done_data;
	struct scatterlist *sglist = data->sg;
	int num_sg = data->sg_len;
	bool write = data->flags & MMC_DATA_WRITE;
	int timeout = data->blocks > 1 ? icr->data_timeout_ms :
		icr->cmd_timeout_ms;
	struct scatterlist *sg;
	dma_addr_t addr;
	long timeleft;
	unsigned int len;
	int err = 0, i, count;
	int dma_dir = write ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
	u32 dir = write ? DATA_WRITE : DATA_READ;
	u32 val = DATA_START | dir;

	icr_dbg(icr, "num_sg = %d\n", num_sg);

	if (icr->removed)
		return -EINVAL;

	if ((sglist == NULL) || (num_sg <= 0))
		return -EINVAL;

	count = dma_map_sg(icr_dev(icr), sglist, num_sg, dma_dir);
	icr_dbg(icr, "DMA mapping count: %d\n", count);
	if (count < 1)
		return -EINVAL;

	icr->sg_tbl_idx = 0;
	for_each_sg(sglist, sg, count, i) {
		addr = sg_dma_address(sg);
		len = sg_dma_len(sg);
		rtsx_icr_add_sg_tbl(icr, addr, len, i == count - 1);
	}

	if (write)
		rtsx_icr_enable_end_int(icr);

	rtsx_icr_lock_irqsave(icr);

	icr->done = &done_data;
	icr->trans_status = TRANS_READY;
	if (write)
		icr->trans_wait = DATA_DONE_INT | SDHOST_END_INT;

	init_completion(&done_data);
	if (write)
		rtsx_icr_reg_write(icr, SD_TRANSFER,
			0xFF, SD_TRANSFER_START | SD_TM_AUTO_WRITE_3);
	else
		rtsx_icr_reg_write(icr, SD_TRANSFER,
			0xFF, SD_TRANSFER_START | SD_TM_AUTO_READ_2);
	rtsx_icr_writel(icr, HDBAR, icr->sg_tbl_addr);
	rtsx_icr_writel(icr, HDBCTLR, val);

	rtsx_icr_unlock_irqrestore(icr);

	timeleft = wait_for_completion_interruptible_timeout(icr->done,
			msecs_to_jiffies(timeout));
	if (timeleft <= 0) {
		err = -ETIMEDOUT;
		goto out;
	}

	rtsx_icr_lock_irqsave(icr);
	if (icr->trans_status == TRANS_FAIL)
		err = -EIO;
	else if (icr->trans_status != TRANS_OK)
		err = -EINVAL;
	rtsx_icr_unlock_irqrestore(icr);

out:
	if (write)
		rtsx_icr_disable_end_int(icr);

	rtsx_icr_lock_irqsave(icr);
	icr->done = NULL;
	if (write)
		icr->trans_wait = 0;
	rtsx_icr_unlock_irqrestore(icr);

	dma_unmap_sg(icr_dev(icr), sglist, num_sg, dma_dir);

	if (err < 0) {
		icr_err(icr, "%d: HDBAR = %08X -> %08X, HDBCTLR = %08X\n",
			err, icr->sg_tbl_addr, rtsx_icr_readl(icr, HDBAR),
			rtsx_icr_readl(icr, HDBCTLR));
		icr_err(icr, "SDMAR = %08x, SDMACTLR = %08x\n",
			rtsx_icr_readl(icr, SDMAR),
			rtsx_icr_readl(icr, SDMACTLR));
		if (!icr->removed)
			rtsx_icr_stop_cmd(icr);
		rtsx_icr_dump_all_regs(icr);
		rtsx_icr_dump_dma_table(icr);
		if (icr->data_errors++)
			rtsx_icr_remove_card(icr);
	} else if (icr->data_errors) {
		icr->data_errors = 0;
	}

	return err;
}

int rtsx_icr_transfer_stop(struct rtsx_icr *icr)
{
	if (icr->done)
		complete(icr->done);

	if (!icr->removed)
		rtsx_icr_stop_cmd(icr);

	return 0;
}

static void rtsx_icr_start_run(struct rtsx_icr *icr)
{
	if (icr->removed)
		return;

	if (icr->state != RTSX_ICR_STATE_RUN) {
		icr->state = RTSX_ICR_STATE_RUN;
		icr_dbg(icr, "enter state: runing.\n");
	}

	mod_delayed_work(system_wq, &icr->idle_work, msecs_to_jiffies(200));
}

static void rtsx_icr_idle(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct rtsx_icr *icr = container_of(dwork, struct rtsx_icr,
			idle_work);

	mutex_lock(&icr->mutex);
	icr_dbg(icr, "enter state: idle\n");
	icr->state = RTSX_ICR_STATE_IDLE;
	mutex_unlock(&icr->mutex);
}

static void rtsx_icr_card_detect(struct work_struct *work)
{
	struct delayed_work *dwork;
	struct rtsx_icr *icr;
	u32 irq_status;

	dwork = to_delayed_work(work);
	icr = container_of(dwork, struct rtsx_icr, detect_work);

	mutex_lock(&icr->mutex);
	icr_dbg(icr, "card_status: 0x%08x\n", icr->card_status);

	rtsx_icr_lock_irqsave(icr);
	irq_status = rtsx_icr_readl(icr, BIPR);
	rtsx_icr_unlock_irqrestore(icr);

	icr_dbg(icr, "irq_status: 0x%08x\n", irq_status);

	icr->card_status &= ~SD_INT;

	if (!(icr->card_status & SD_EXIST)) /* always detect unplug */
		rtsx_icr_remove_card(icr);
	else if (icr->card_status & irq_status & SD_EXIST)
		rtsx_icr_insert_card(icr);

	mutex_unlock(&icr->mutex);
}

static irqreturn_t rtsx_icr_isr(int irq, void *dev_id)
{
	struct rtsx_icr *icr = dev_id;
	u32 int_reg;

	if (!icr)
		return IRQ_NONE;

	rtsx_icr_lock(icr);

	int_reg = rtsx_icr_readl(icr, BIPR);
	rtsx_icr_writel(icr, BIPR, int_reg); /* clear BIPR */

	if (int_reg == 0xFFFFFFFF) {
		rtsx_icr_unlock(icr);
		return IRQ_HANDLED;
	}

	int_reg &= icr->bier;
	if (!int_reg) {
		rtsx_icr_unlock(icr);
		return IRQ_NONE;
	}

	icr_dbg(icr, "---------- IRQ: 0x%08x ----------\n", int_reg);
	if (int_reg & CARD_ERR_INT) {
		icr->trans_wait = 0;
		icr->trans_status = TRANS_FAIL;
		if (icr->done)
			complete(icr->done);
	}

	if (icr->trans_wait) {
		icr->trans_wait &=
			~(int_reg & (DATA_DONE_INT | SDHOST_END_INT));
		if (!icr->trans_wait) {
			icr->trans_status = TRANS_OK;
			if (icr->done)
				complete(icr->done);
		}
	} else if (icr->trans_status == TRANS_READY) {
		icr->trans_status = int_reg & TRANS_STATUS_MASK;
		if ((icr->trans_status != TRANS_READY) && icr->done)
			complete(icr->done);
	}

	if (int_reg & SD_INT) {
		icr->card_status |= SD_INT;
		if (int_reg & SD_EXIST)
			icr->card_status |= SD_EXIST;
		else
			icr->card_status &= ~SD_EXIST;
		schedule_delayed_work(&icr->detect_work,
				msecs_to_jiffies(200));
	}

	rtsx_icr_unlock(icr);
	return IRQ_HANDLED;
}

static int rtsx_icr_acquire_irq(struct rtsx_icr *icr)
{
	int err = 0;

	err = request_irq(icr->irq, rtsx_icr_isr, IRQF_SHARED,
			RTSX_ICR_DRV_NAME, icr);
	if (err)
		icr_err(icr, "request IRQ %d failed\n", icr->irq);

	return err;
}

static int rtsx_icr_init_hw(struct rtsx_icr *icr)
{
	int err = 0;

	rtsx_icr_writel(icr, OCP_WRAPPER_EN, OCP_WRAPPER_ENABLE);
	rtsx_icr_writel(icr, HCBAR, icr->cmd_addr);
	rtsx_icr_enable_bus_int(icr);

	icr->card_status = rtsx_icr_readl(icr, BIPR) & SD_EXIST;

	return err;
}

static int rtsx_icr_init(struct rtsx_icr *icr)
{
	int err;

	spin_lock_init(&icr->__lock);
	mutex_init(&icr->mutex);
	init_completion(&icr->done_cmd);

	icr->sd_pull_ctl_enable_tbl = rtsx_icr_pull_ctl_enable_tbl;
	icr->sd_pull_ctl_disable_tbl = rtsx_icr_pull_ctl_disable_tbl;

	err = rtsx_icr_init_hw(icr);

	return err;
}

static inline void rtsx_icr_clear_error(struct rtsx_icr *icr)
{
	rtsx_icr_reg_write(icr, CARD_STOP, SD_STOP | SD_CLEAR_ERROR,
			SD_STOP | SD_CLEAR_ERROR);
	rtsx_icr_reg_write(icr, SD_CFG1, SD_ASYNC_FIFO_CTL, SD_ASYNC_FIFO_RST);
}

static inline void rtsx_icr_reg_set_sd_cmd(struct rtsx_icr *icr,
	struct mmc_command *cmd)
{
	if (!cmd)
		return;

	rtsx_icr_write(icr, SD_CMD0, 0xFF, 0x40 | cmd->opcode);
	rtsx_icr_write_be32(icr, SD_CMD1, cmd->arg);
}

static inline void rtsx_icr_reg_get_sd_resp(struct rtsx_icr *icr)
{
	int i;
	for (i = SD_CMD0; i < SD_CMD0 + 5; i++)
		rtsx_icr_read(icr, i);
}

static inline void rtsx_icr_reg_get_pp_resp(struct rtsx_icr *icr)
{
	int i;
	for (i = PINGPONG_BUF; i < PINGPONG_BUF + 16; i++)
		rtsx_icr_read(icr, i);
}

static inline void rtsx_icr_reg_get_resp(struct rtsx_icr *icr, int resp_type)
{
	if (resp_type == SD_RSP_TYPE_R2)
		rtsx_icr_reg_get_pp_resp(icr);
	else if (resp_type != SD_RSP_TYPE_R0)
		rtsx_icr_reg_get_sd_resp(icr);
}

static void rtsx_icr_get_resp(struct rtsx_icr *icr, struct mmc_command *cmd,
	u8 *buf, int resp_type)
{
	int i;

	if (resp_type == SD_RSP_TYPE_R2) {
		for (i = 0; i < 4; i++)
			cmd->resp[i] = get_unaligned_be32(buf + 4 * i);
	} else {
		cmd->resp[0] = get_unaligned_be32(buf);
	}

	icr_dbg(icr, "response: 0x%08x 0x%08x 0x%08x 0x%08x\n",
		cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);
}
static inline void rtsx_icr_reg_set_data_len(struct rtsx_icr *icr,
	u16 blocks, u16 blksz)
{
	if (blocks == 0 || blksz == 0)
		return;

	rtsx_icr_write(icr, SD_BLOCK_CNT_L, 0xFF, blocks);
	rtsx_icr_write(icr, SD_BLOCK_CNT_H, 0xFF, blocks >> 8);
	rtsx_icr_write(icr, SD_BYTE_CNT_L, 0xFF, blksz);
	rtsx_icr_write(icr, SD_BYTE_CNT_H, 0xFF, blksz >> 8);
}

static int rtsx_icr_resp_type(struct rtsx_icr *icr, struct mmc_command *cmd)
{
	switch (mmc_resp_type(cmd)) {
	case MMC_RSP_NONE:
		return SD_RSP_TYPE_R0;
	case MMC_RSP_R1:
		return SD_RSP_TYPE_R1;
	case MMC_RSP_R1B:
		return SD_RSP_TYPE_R1b;
	case MMC_RSP_R2:
		return SD_RSP_TYPE_R2;
	case MMC_RSP_R3:
		return SD_RSP_TYPE_R3;
	default:
		icr_err(icr, "unknown cmd->flag\n");
		return -EINVAL;
	}
}

static int rtsx_icr_resp_status_index(int resp_type)
{
	if ((resp_type & SD_RSP_LEN_MASK) == SD_RSP_LEN_6)
		return 5;
	else if ((resp_type & SD_RSP_LEN_MASK) == SD_RSP_LEN_17)
		return 16;
	else
		return 0;
}

static int rtsx_icr_resp_timeout(struct rtsx_icr *icr, int resp_type)
{
	if (resp_type == SD_RSP_TYPE_R1b)
		return 10 * icr->cmd_timeout_ms;

	return icr->cmd_timeout_ms;
}

static int rtsx_icr_check_resp(struct rtsx_icr *icr, u8 *buf, int resp_type)
{
	if ((buf[0] & 0xC0) != 0) {
		icr_err(icr, "invalid response bit: 0x%2x\n", buf[0]);
		return -EILSEQ;
	}

	if (!(resp_type & SD_NO_CHECK_CRC7)) {
		int stat_idx = rtsx_icr_resp_status_index(resp_type);

		if (buf[stat_idx] & SD_ERR_CRC7) {
			icr_err(icr, "CRC7 error: 0x%02x\n", buf[stat_idx]);
			return -EILSEQ;
		}
	}
	return 0;
}

static int rtsx_icr_send_cmd_get_resp(struct rtsx_icr *icr,
		struct mmc_command *cmd)
{
	int err = 0;
	u8 *ptr;
	int resp_type = rtsx_icr_resp_type(icr, cmd);
	int timeout = rtsx_icr_resp_timeout(icr, resp_type);

	if (resp_type < 0)
		return -EINVAL;

	icr_dbg(icr, "SD/MMC CMD %d, arg = 0x%08x\n", cmd->opcode, cmd->arg);

	if (cmd->opcode == SD_SWITCH_VOLTAGE) {
		err = rtsx_icr_reg_write(icr, SD_BUS_STAT, 0xFF,
				SD_CLK_TOGGLE_EN);
		if (err < 0)
			return err;
	}

	rtsx_icr_init_cmd(icr);
	rtsx_icr_reg_set_sd_cmd(icr, cmd);
	rtsx_icr_write(icr, SD_CFG2, 0xFF, resp_type);
	rtsx_icr_write(icr, CARD_DATA_SOURCE, 0x01, SRC_PINGPONG_BUF);
	rtsx_icr_write(icr, SD_TRANSFER, 0xFF,
		SD_TRANSFER_START | SD_TM_CMD_RSP);
	rtsx_icr_check(icr, SD_TRANSFER, SD_TRANSFER_END | SD_STAT_IDLE,
		SD_TRANSFER_END | SD_STAT_IDLE);
	rtsx_icr_reg_get_resp(icr, resp_type);
	rtsx_icr_read(icr, SD_STAT1);
	err = rtsx_icr_transfer_cmd_timeout(icr, timeout);
	if (err)
		return err;

	if (resp_type == SD_RSP_TYPE_R0)
		return err;

	ptr = rtsx_icr_get_data(icr) + 1; /* skip for check command */
	rtsx_icr_get_resp(icr, cmd, ptr + 1, resp_type); /* skip opcode */
	return rtsx_icr_check_resp(icr, ptr, resp_type);
}

static int rtsx_icr_read_short_data(struct rtsx_icr *icr,
	struct mmc_command *cmd, u8 *buf, int len)
{
	int err = 0;
	u8 trans_mode = cmd->opcode == MMC_SEND_TUNING_BLOCK ?
		SD_TM_AUTO_TUNING : SD_TM_NORMAL_READ;

	rtsx_icr_init_cmd(icr);
	rtsx_icr_reg_set_sd_cmd(icr, cmd);
	rtsx_icr_reg_set_data_len(icr, 1, len);
	rtsx_icr_write(icr, SD_CFG2, 0xFF, SD_RSP_TYPE_R1);
	if (trans_mode != SD_TM_AUTO_TUNING)
		rtsx_icr_write(icr, CARD_DATA_SOURCE, 0x01, SRC_PINGPONG_BUF);
	rtsx_icr_write(icr, SD_TRANSFER, 0xFF, SD_TRANSFER_START | trans_mode);
	rtsx_icr_check(icr, SD_TRANSFER, SD_TRANSFER_END, SD_TRANSFER_END);
	err = rtsx_icr_transfer_cmd(icr);
	if (err)
		return err;

	return rtsx_icr_pp_read(icr, buf, len);
}

static int rtsx_icr_write_short_data(struct rtsx_icr *icr,
	struct mmc_command *cmd, u8 *buf, int len)
{
	int err = 0;

	err = rtsx_icr_send_cmd_get_resp(icr, cmd);
	if (err)
		return err;

	err = rtsx_icr_pp_write(icr, buf, len);
	if (err)
		return err;

	rtsx_icr_init_cmd(icr);
	rtsx_icr_reg_set_sd_cmd(icr, cmd);
	rtsx_icr_reg_set_data_len(icr, 1, len);
	rtsx_icr_write(icr, SD_CFG2, 0xFF, SD_RSP_TYPE_R1_DATA);
	rtsx_icr_write(icr, SD_TRANSFER, 0xFF,
		SD_TRANSFER_START | SD_TM_AUTO_WRITE_3);
	rtsx_icr_check(icr, SD_TRANSFER, SD_TRANSFER_END, SD_TRANSFER_END);
	err = rtsx_icr_transfer_cmd(icr);
	if (err)
		return err;

	return err;
}

static int rtsx_icr_short_data_xfer(struct rtsx_icr *icr,
		struct mmc_request *mrq)
{
	struct mmc_command *cmd = mrq->cmd;
	struct mmc_data *data = mrq->data;
	u8 *buf;
	int initial_mode = icr->initial_mode;
	int clock = icr->clock;
	int err = 0;

	icr_dbg(icr, "SD/MMC CMD %d, arg = 0x%08x blocks %d blocksize %d\n",
		cmd->opcode, cmd->arg, data->blocks, data->blksz);

	buf = kzalloc(data->blksz, GFP_NOIO);
	if (!buf)
		return -ENOMEM;

	if (initial_mode)
		rtsx_icr_set_clock(icr, DATA_MIN_CLK);

	if (data->flags & MMC_DATA_READ) {
		err = rtsx_icr_read_short_data(icr, cmd, buf, data->blksz);
		sg_copy_from_buffer(data->sg, data->sg_len, buf, data->blksz);
	} else {
		sg_copy_to_buffer(data->sg, data->sg_len, buf, data->blksz);
		err = rtsx_icr_write_short_data(icr, cmd, buf, data->blksz);
	}

	if (initial_mode)
		rtsx_icr_set_clock(icr, clock);

	kfree(buf);
	return err;
}

static int rtsx_icr_read_long_data(struct rtsx_icr *icr,
	struct mmc_request *mrq)
{
	struct mmc_host *mmc = icr->mmc;
	struct mmc_card *card = mmc->card;
	struct mmc_command *cmd = mrq->cmd;
	struct mmc_data *data = mrq->data;
	int resp_type = rtsx_icr_resp_type(icr, cmd);
	int err = 0;
	u8 cfg2 = 0;

	if (resp_type < 0)
		return -EINVAL;

	if (!card)
		cfg2 |= SD_NO_CHECK_CRC_TIMEOUT;

	rtsx_icr_init_cmd(icr);
	rtsx_icr_reg_set_sd_cmd(icr, cmd);
	rtsx_icr_reg_set_data_len(icr, data->blocks, data->blksz);
	rtsx_icr_write(icr, SD_CFG2, 0xFF, cfg2 | resp_type);
	rtsx_icr_write(icr, CARD_DATA_SOURCE, 0x01, SRC_RING_BUF);
	err = rtsx_icr_transfer_cmd(icr);
	if (err) {
		rtsx_icr_clear_error(icr);
		return err;
	}

	err = rtsx_icr_transfer_data(icr, data);
	if (err) {
		rtsx_icr_clear_error(icr);
		return err;
	}

	return 0;
}

static int rtsx_icr_write_long_data(struct rtsx_icr *icr,
	struct mmc_request *mrq)
{
	struct mmc_host *mmc = icr->mmc;
	struct mmc_card *card = mmc->card;
	struct mmc_command *cmd = mrq->cmd;
	struct mmc_data *data = mrq->data;
	int err = 0;
	u8 cfg2 = 0;

	err = rtsx_icr_send_cmd_get_resp(icr, cmd);
	if (err < 0)
		return err;

	if (!card)
		cfg2 |= SD_NO_CHECK_CRC_TIMEOUT;

	rtsx_icr_init_cmd(icr);
	rtsx_icr_reg_set_data_len(icr, data->blocks, data->blksz);
	rtsx_icr_write(icr, SD_CFG2, 0xFF, cfg2 | SD_RSP_TYPE_R1_DATA);
	rtsx_icr_write(icr, CARD_DATA_SOURCE, 0x01, SRC_RING_BUF);
	err = rtsx_icr_transfer_cmd(icr);
	if (err) {
		rtsx_icr_clear_error(icr);
		return err;
	}

	err = rtsx_icr_transfer_data(icr, data);
	if (err) {
		rtsx_icr_clear_error(icr);
		return err;
	}

	return err;
}

static int rtsx_icr_long_data_xfer(struct rtsx_icr *icr,
		struct mmc_request *mrq)
{
	struct mmc_data *data = mrq->data;
	int initial_mode = icr->initial_mode;
	int clock = icr->clock;
	int err = 0;

	icr_dbg(icr, "SD/MMC CMD %d, arg = 0x%08x blocks %d blocksize %d\n",
		mrq->cmd->opcode, mrq->cmd->arg, data->blocks, data->blksz);

	if (initial_mode)
		rtsx_icr_set_clock(icr, DATA_MIN_CLK);

	if (data->flags & MMC_DATA_WRITE)
		err = rtsx_icr_write_long_data(icr, mrq);
	else
		err = rtsx_icr_read_long_data(icr, mrq);

	if (initial_mode)
		rtsx_icr_set_clock(icr, clock);

	return err;
}

static int sdmmc_get_cd(struct mmc_host *mmc)
{
	struct rtsx_icr *icr = mmc_priv(mmc);

	return !icr->removed && (icr->card_status & SD_EXIST);
}

static void sdmmc_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct rtsx_icr *icr = mmc_priv(mmc);
	struct mmc_command *cmd = mrq->cmd;
	struct mmc_data *data = mrq->data;
	int data_size = 0;

	if (!sdmmc_get_cd(mmc)) {
		cmd->error = -ENOMEDIUM;
		goto finish;
	}

	if (data)
		data_size = data->blocks * data->blksz;

	mutex_lock(&icr->mutex);

	rtsx_icr_start_run(icr);
	icr->mrq = mrq;

	if (!data)
		cmd->error = rtsx_icr_send_cmd_get_resp(icr, cmd);
	else if (data_size >= 512)
		data->error = rtsx_icr_long_data_xfer(icr, mrq);
	else {
		if (icr->short_dma_mode)
			data->error = rtsx_icr_long_data_xfer(icr, mrq);
		if (!icr->short_dma_mode || data->error) {
			data->error = rtsx_icr_short_data_xfer(icr, mrq);
			if (icr->short_dma_mode && !data->error) {
				icr->short_dma_mode = 0;
				icr_err(icr, "disable short dma mode\n");
			}
		}
	}

	if (mmc_op_multi(cmd->opcode) && mrq->stop)
		cmd->error = rtsx_icr_send_cmd_get_resp(icr, mrq->stop);

	if (data && !cmd->error && !data->error)
		data->bytes_xfered = data_size;

	icr->mrq = NULL;
	mutex_unlock(&icr->mutex);

finish:
	if (icr->trigger_gpio && (cmd->opcode == icr->trigger_cmd)) {
		if (!icr->trigger_count) {
			gpio_set_value(icr->trigger_gpio, 0);
			gpio_set_value(icr->trigger_gpio, 1);
		} else {
			icr->trigger_count--;
		}
	}
	if (cmd->error || (data && data->error))
		icr_err(icr, "CMD %d 0x%08x (%d, %d)\n",
			cmd->opcode, cmd->arg,
			cmd->error, data ? data->error : 0);

	mmc_request_done(mmc, mrq);
}

static void rtsx_icr_pull_ctl_enable(struct rtsx_icr *icr)
{
	const u32 *tbl = icr->sd_pull_ctl_enable_tbl;

	while (*tbl & 0xFFFF0000) {
		rtsx_icr_write(icr, *tbl >> 16, 0xFF, *tbl);
		tbl++;
	}
}

static void rtsx_icr_pull_ctl_disable(struct rtsx_icr *icr)
{
	const u32 *tbl = icr->sd_pull_ctl_disable_tbl;

	while (*tbl & 0xFFFF0000) {
		rtsx_icr_write(icr, *tbl >> 16, 0xFF, *tbl);
		tbl++;
	}
}

static int rtsx_icr_power_on_asic(struct rtsx_icr *icr)
{
	int err;

	gpio_set_value(SD_PWR_CTRL_C2, 1);
	rtsx_icr_reg_write(icr, H_SD_LDO_CFG0, 0xFF, SD_LDO_TUNE33);
	rtsx_icr_reg_write(icr, H_SD_LDO_CTRL, 0xFF,
		SD_LDO_SSC_LDO_EN | SD_LDO_SSC_BIAS_EN | SD_LDO_LDO3318_POW);
	rtsx_icr_reg_write(icr, H_SSC_PLL_POW, 0xFF, U_SSC_PLL_POW);

	rtsx_icr_init_cmd(icr);

	rtsx_icr_write(icr, SD_AUTO_RESET_FIFO, AUTO_RESET_FIFO_EN,
			AUTO_RESET_FIFO_EN);
	rtsx_icr_write(icr, CARD_CLK_EN, SD_CLK_EN, SD_CLK_EN);
	rtsx_icr_pull_ctl_enable(icr);

	err = rtsx_icr_transfer_cmd(icr);
	if (err < 0)
		return err;

	udelay(150);

	err = rtsx_icr_reg_write(icr, CARD_OE, SD_OUTPUT_EN, SD_OUTPUT_EN);
	if (err < 0)
		return err;

	err = rtsx_icr_reg_write(icr, SDCMD_DRV_SEL, 0xFF,
		SD_CMD_DRV_SEL | SD_CLK_DRV_SEL);
	if (err < 0)
                return err;

	err = rtsx_icr_reg_write(icr, SDDAT_L_DRV_SEL, 0xFF,
			SD_DAT0_DRV_SEL | SD_DAT1_DRV_SEL |
			SD_DAT2_DRV_SEL | SD_DAT3_DRV_SEL);
	if (err < 0)
		return err;

	return err;
}

static int rtsx_icr_power_on_fpga(struct rtsx_icr *icr)
{
	int err;

	rtsx_icr_init_cmd(icr);

	rtsx_icr_write(icr, SD_AUTO_RESET_FIFO, AUTO_RESET_FIFO_EN,
			AUTO_RESET_FIFO_EN);
	rtsx_icr_pull_ctl_enable(icr);

	err = rtsx_icr_transfer_cmd(icr);
	if (err < 0)
		return err;

	gpio_set_value(SD_PULL_CTRL_C2, 1);
	gpio_set_value(SD_PWR_CTRL_C2, 1);
	msleep(20);

	err = rtsx_icr_reg_write(icr, CARD_OE, SD_OUTPUT_EN, SD_OUTPUT_EN);
	if (err < 0)
		return err;

	return 0;
}

static int rtsx_icr_power_on(struct rtsx_icr *icr)
{
	int err;

	if (icr->power_state == RTSX_SD_POWER_ON)
		return 0;

	if (icr->asic_mode)
		err = rtsx_icr_power_on_asic(icr);
	else
		err = rtsx_icr_power_on_fpga(icr);

	if (err)
		return err;

	icr->power_state = RTSX_SD_POWER_ON;
	return 0;
}

static int rtsx_icr_power_off_asic(struct rtsx_icr *icr)
{
	int err;

	rtsx_icr_init_cmd(icr);

	rtsx_icr_write(icr, CARD_CLK_EN, SD_CLK_EN, 0);
	rtsx_icr_write(icr, CARD_OE, SD_OUTPUT_EN, 0);
	rtsx_icr_pull_ctl_disable(icr);

	err = rtsx_icr_transfer_cmd(icr);
	if (err < 0)
		return err;

	rtsx_icr_reg_write(icr, H_SD_LDO_CFG0, 0xFF, 0);
	rtsx_icr_reg_write(icr, H_SD_LDO_CTRL, 0xFF, 0);
	rtsx_icr_reg_write(icr, H_SSC_PLL_POW, 0xFF, 0);
	gpio_set_value(SD_PWR_CTRL_C2, 0);

	return 0;
}

static int rtsx_icr_power_off_fpga(struct rtsx_icr *icr)
{
	int err;

	rtsx_icr_init_cmd(icr);

	rtsx_icr_write(icr, CARD_CLK_EN, SD_CLK_EN, 0);
	rtsx_icr_write(icr, CARD_OE, SD_OUTPUT_EN, 0);

	err = rtsx_icr_transfer_cmd(icr);
	if (err < 0)
		return err;

	gpio_set_value(SD_PWR_CTRL_C2, 0);
	gpio_set_value(SD_PULL_CTRL_C2, 0);
	msleep(20);

	return 0;
}

static int rtsx_icr_power_off(struct rtsx_icr *icr)
{
	int err = 0;

	if (icr->power_state == RTSX_SD_POWER_OFF)
		return 0;

	if (icr->asic_mode)
		err = rtsx_icr_power_off_asic(icr);
	else
		err = rtsx_icr_power_off_fpga(icr);

	icr->power_state = RTSX_SD_POWER_OFF;

	return err;
}

int rtsx_icr_set_bus_width(struct rtsx_icr *icr, unsigned char bus_width)
{
	int err = 0;
	u8 width[] = {
		[MMC_BUS_WIDTH_1] = SD_BUS_WIDTH_1BIT,
		[MMC_BUS_WIDTH_4] = SD_BUS_WIDTH_4BIT,
		[MMC_BUS_WIDTH_8] = SD_BUS_WIDTH_8BIT,
	};

	if (bus_width <= MMC_BUS_WIDTH_8)
		err = rtsx_icr_reg_write(icr, SD_CFG1, 0x03,
				width[bus_width]);
	else
		err = -EINVAL;

	return err;
}

static int rtsx_icr_set_timing_asic(struct rtsx_icr *icr, unsigned char timing)
{
	int err = 0;

	rtsx_icr_init_cmd(icr);

	switch (timing) {
	case MMC_TIMING_UHS_SDR104:
	case MMC_TIMING_UHS_SDR50:
		icr->sd_mode = RTSX_SD_SDR_MODE;
		rtsx_icr_write(icr, SD_CFG1, SD_MODE_SEL_MASK, SD_30_MODE);
		break;

	case MMC_TIMING_UHS_DDR50:
		icr->sd_mode = RTSX_SD_DDR_MODE;
		rtsx_icr_write(icr, SD_CFG1, SD_MODE_SEL_MASK, SD_DDR_MODE);
		rtsx_icr_write(icr, SD_PUSH_POINT_CTL,
				TX_DDR_CMD_DAT_TYPE_MASK,
				TX_DDR_CMD_DAT_TYPE_FIX);
		rtsx_icr_write(icr, SD_SAMPLE_POINT_CTL,
			RX_DDR_DAT_TYPE_MASK | RX_DDR_CMD_TYPE_MASK,
			RX_DDR_DAT_TYPE_VAR | RX_DDR_CMD_TYPE_VAR);
		break;

	case MMC_TIMING_MMC_HS:
	case MMC_TIMING_SD_HS:
		icr->sd_mode = RTSX_SD_HS_MODE;
		rtsx_icr_write(icr, SD_CFG1, SD_MODE_SEL_MASK, SD_20_MODE);
		rtsx_icr_write(icr, SD_PUSH_POINT_CTL, TX_SD20_SEL_MASK,
			TX_SD20_SEL_AHEAD_1_4);
		rtsx_icr_write(icr, SD_SAMPLE_POINT_CTL, RX_SD20_SEL_MASK,
			RX_SD20_SEL_DELAY_1_4);
		break;

	default:
		icr->sd_mode = RTSX_SD_DEFAULT_MODE;
		rtsx_icr_write(icr, SD_CFG1, SD_MODE_SEL_MASK, SD_20_MODE);
		rtsx_icr_write(icr, SD_PUSH_POINT_CTL, 0xFF, 0);
		rtsx_icr_write(icr, SD_SAMPLE_POINT_CTL, RX_SD20_SEL_MASK,
			RX_SD20_SEL_RISING);
		break;
	}

	err = rtsx_icr_transfer_cmd(icr);

	return err;
}

static int rtsx_icr_set_timing_fpga(struct rtsx_icr *icr, unsigned char timing)
{
	int err = 0;

	rtsx_icr_init_cmd(icr);

	switch (timing) {
	case MMC_TIMING_UHS_SDR104:
	case MMC_TIMING_UHS_SDR50:
		icr->sd_mode = RTSX_SD_SDR_MODE;
		rtsx_icr_write(icr, SD_CFG1, SD_MODE_SEL_MASK, SD_30_MODE);
		break;

	case MMC_TIMING_UHS_DDR50:
		icr->sd_mode = RTSX_SD_DDR_MODE;
		rtsx_icr_write(icr, SD_CFG1, SD_MODE_SEL_MASK, SD_DDR_MODE);
		rtsx_icr_write(icr, SD_PUSH_POINT_CTL,
				TX_DDR_CMD_DAT_TYPE_MASK,
				TX_DDR_CMD_DAT_TYPE_FIX);
		rtsx_icr_write(icr, SD_SAMPLE_POINT_CTL,
			RX_DDR_DAT_TYPE_MASK | RX_DDR_CMD_TYPE_MASK,
			RX_DDR_DAT_TYPE_VAR | RX_DDR_CMD_TYPE_VAR);
		break;

	case MMC_TIMING_MMC_HS:
	case MMC_TIMING_SD_HS:
		icr->sd_mode = RTSX_SD_HS_MODE;
		rtsx_icr_write(icr, SD_CFG1, SD_MODE_SEL_MASK, SD_20_MODE);
		rtsx_icr_write(icr, SD_PUSH_POINT_CTL, TX_SD20_SEL_MASK,
			TX_SD20_SEL_AHEAD_1_4);
		rtsx_icr_write(icr, SD_SAMPLE_POINT_CTL, RX_SD20_SEL_MASK,
			RX_SD20_SEL_DELAY_1_4);
		break;

	default:
		icr->sd_mode = RTSX_SD_DEFAULT_MODE;
		rtsx_icr_write(icr, SD_CFG1, SD_MODE_SEL_MASK, SD_20_MODE);
		rtsx_icr_write(icr, SD_PUSH_POINT_CTL, 0xFF, 0);
		rtsx_icr_write(icr, SD_SAMPLE_POINT_CTL, RX_SD20_SEL_MASK,
			RX_SD20_SEL_RISING);
		break;
	}

	err = rtsx_icr_transfer_cmd(icr);

	return err;
}

int rtsx_icr_set_timing(struct rtsx_icr *icr, unsigned char timing)
{
	int err = 0;

	if (icr->asic_mode)
		err = rtsx_icr_set_timing_asic(icr, timing);
	else
		err = rtsx_icr_set_timing_fpga(icr, timing);

	return err;
}

static int rtsx_icr_uhs_mode(struct rtsx_icr *icr)
{
	return icr->sd_mode == RTSX_SD_SDR_MODE ||
		icr->sd_mode == RTSX_SD_DDR_MODE;
}

static unsigned int rtsx_icr_src_clock(struct rtsx_icr *icr, unsigned int clock)
{
	if (icr->initial_mode)
		return 512 * clock;

	clock <<= H_SSC_DIV_MAX_SHIFT - fls(clock / DATA_MIN_CLK);
	while (clock > DATA_MAX_CLK)
		clock >>= 1;

	return clock;
}

static int rtsx_icr_set_clock_asic(struct rtsx_icr *icr, unsigned int clock)
{
	int err = 0;
	unsigned src_clock, final_clock;
	u8 clk_div;
	u8 ssc_div, cfg_div = SD_CLK_DIVIDE_0;

	if (clock == 0) {
		err = rtsx_icr_reg_write(icr, SD_BUS_STAT, 0xFF,
			SD_CLK_TOGGLE_STOP);
		return err;
	}

	if (clock > icr->max_clock)
		clock = icr->max_clock;

	src_clock = rtsx_icr_src_clock(icr, clock);
	ssc_div = (src_clock / 1000000) * 4 / 5 - 3;

	final_clock = src_clock;
	if (icr->initial_mode) {
		final_clock /= 128;
		cfg_div = SD_CLK_DIVIDE_128;
	} else if (!rtsx_icr_uhs_mode(icr)) {
		final_clock /= 2;
	}

	clk_div = fls(final_clock / clock);
	clk_div -= (bool) clk_div;
	final_clock /= 1 << clk_div;

	rtsx_icr_init_cmd(icr);
	rtsx_icr_write(icr, SD_BUS_STAT, SD_CLK_TOGGLE_STOP, 0);
	rtsx_icr_write(icr, CARD_CLK_EN, SD_CLK_EN, SD_CLK_EN);
	rtsx_icr_write(icr, H_CLK_CHANGE, CLK_CHANGE_MASK,
			CLK_CHANGE_START);
	rtsx_icr_write(icr, H_SSC_DIV_N1, 0xFF, ssc_div);
	rtsx_icr_write(icr, SD_CFG1, SD_CLK_DIVIDE_MASK, cfg_div);
	rtsx_icr_write(icr, H_CRC_CLK_DIV, U_CRC_CLK_DIV_MASK, clk_div);
	rtsx_icr_write(icr, H_CLK_CHANGE, CLK_CHANGE_MASK,
			CLK_CHANGE_END);
	err = rtsx_icr_transfer_cmd(icr);

	udelay(10);			/* wait clock stable */

	icr_dbg(icr, "clock %d: source %d, ssc_div %d, clk_div %d\n",
		final_clock, src_clock, ssc_div, clk_div);
	return err;
}

static int rtsx_icr_set_clock_fpga(struct rtsx_icr *icr, unsigned int clock)
{
	int err = 0;
	unsigned final_clock = FPGA_SRC_CLK;
	u8 clk_div;
	u8 cfg_div = SD_CLK_DIVIDE_0;

	if (clock == 0) {
		err = rtsx_icr_reg_write(icr, SD_BUS_STAT, 0xFF,
			SD_CLK_TOGGLE_STOP);
		return err;
	}

	if (clock > icr->max_clock)
		clock = icr->max_clock;

	if (icr->initial_mode) {
		final_clock /= 128;
		cfg_div = SD_CLK_DIVIDE_128;
	} else if (!rtsx_icr_uhs_mode(icr)) {
		final_clock /= 2;
	}

	clk_div = fls(final_clock / clock);
	clk_div -= (bool) clk_div;
	final_clock /= 1 << clk_div;

	rtsx_icr_init_cmd(icr);
	rtsx_icr_write(icr, SD_BUS_STAT, SD_CLK_TOGGLE_STOP, 0);
	rtsx_icr_write(icr, CARD_CLK_EN, SD_CLK_EN, SD_CLK_EN);
	rtsx_icr_write(icr, H_CLK_CHANGE, CLK_CHANGE_MASK,
			CLK_CHANGE_START);
	rtsx_icr_write(icr, SD_CFG1, SD_CLK_DIVIDE_MASK, cfg_div);
	rtsx_icr_write(icr, H_CRC_CLK_DIV, U_CRC_CLK_DIV_MASK, clk_div);
	rtsx_icr_write(icr, H_CLK_CHANGE, CLK_CHANGE_MASK,
			CLK_CHANGE_END);
	err = rtsx_icr_transfer_cmd(icr);

	udelay(10);			/* wait clock stable */

	icr_dbg(icr, "final clock %d, div %d\n", final_clock, clk_div);
	return err;
}

int rtsx_icr_set_clock(struct rtsx_icr *icr, unsigned int clock)
{
	int err = 0;

	if (clock && (clock < icr->min_clock))
		rtsx_icr_remove_card(icr);

	icr->initial_mode = (bool) (clock < CLK_1MHz);

	icr_dbg(icr, "set clock to %u\n", clock);

	if (icr->asic_mode)
		err |= rtsx_icr_set_clock_asic(icr, clock);
	else
		err |= rtsx_icr_set_clock_fpga(icr, clock);

	if (!err)
		icr->clock = clock;

	return err;
}

static void sdmmc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct rtsx_icr *icr = mmc_priv(mmc);

	if (icr->removed)
		return;

	mutex_lock(&icr->mutex);

	rtsx_icr_start_run(icr);

	if ((ios->power_mode != MMC_POWER_OFF) && rtsx_icr_power_on(icr))
		icr_err(icr, "set power mode to %u\n", ios->power_mode);

	if (rtsx_icr_set_bus_width(icr, ios->bus_width))
		icr_err(icr, "set bus width to %u\n", ios->bus_width);

	if (rtsx_icr_set_timing(icr, ios->timing))
		icr_err(icr, "set timing to %u\n", ios->timing);

	if (rtsx_icr_set_clock(icr, ios->clock))
		icr_err(icr, "set clock to %u\n", ios->clock);

	if ((ios->power_mode == MMC_POWER_OFF) && rtsx_icr_power_off(icr))
		icr_err(icr, "set power mode to %u\n", ios->power_mode);

	mutex_unlock(&icr->mutex);
}

static int sdmmc_get_ro(struct mmc_host *mmc)
{
	struct rtsx_icr *icr = mmc_priv(mmc);
	int ro = 0;
	u32 val;

	if (icr->removed)
		return -ENOMEDIUM;

	mutex_lock(&icr->mutex);

	rtsx_icr_start_run(icr);

	val = rtsx_icr_readl(icr, BIPR);
	icr_dbg(icr, "BIPR = 0x%08x\n", val);
	if (val & SD_WRITE_PROTECT)
		ro = 1;

	mutex_unlock(&icr->mutex);

	return ro;
}

static int rtsx_icr_wait_voltage_stable_1(struct rtsx_icr *icr)
{
	int err = 0;
	u8 stat = 0;

	mdelay(1);

	err = rtsx_icr_reg_read(icr, SD_BUS_STAT, &stat);
	if (err < 0)
		return err;

	if (stat & SD_CMD_DATA_STATUS_MASK)
		return -EINVAL;

	err = rtsx_icr_reg_write(icr, SD_BUS_STAT, 0xFF,
			SD_CLK_TOGGLE_STOP);

	return err;
}

static int rtsx_icr_wait_voltage_stable_2(struct rtsx_icr *icr)
{
	int err = 0;
	u8 stat = 0;
	u8 mask = SD_CMD_DATA_STATUS_MASK;

	msleep(50);			/* wait 1.8V output stable */

	err = rtsx_icr_reg_write(icr, SD_BUS_STAT, 0xFF, SD_CLK_TOGGLE_EN);
	if (err < 0)
		return err;

	msleep(20);			/* wait card drive SD_DAT[3:0] high */

	err = rtsx_icr_reg_read(icr, SD_BUS_STAT, &stat);
	if (err < 0)
		return err;

	if ((stat & mask) != mask) {
		icr_err(icr, "SD_BUS_STAT = 0x%x\n", stat);
		rtsx_icr_reg_write(icr, SD_BUS_STAT,
				SD_CLK_TOGGLE_EN | SD_CLK_TOGGLE_STOP, 0);
		rtsx_icr_reg_write(icr, CARD_CLK_EN, 0xFF, 0);
		return -EINVAL;
	}

	return err;
}

static int sdmmc_card_busy(struct mmc_host *mmc)
{
	struct rtsx_icr *icr = mmc_priv(mmc);
	u8 stat = 0;
	u8 mask = SD_CMD_DATA_STATUS_MASK;
	int busy;

	rtsx_icr_reg_read(icr, SD_BUS_STAT, &stat);
	busy = !(stat & mask);
	icr_dbg(icr, "card is %s\n", busy ? "busy" : "not busy");
	if (!busy)
		rtsx_icr_reg_write(icr, SD_BUS_STAT, SD_CLK_TOGGLE_EN, 0);

	return busy;
}

static int rtsx_icr_switch_voltage_asic(struct rtsx_icr *icr, u8 voltage)
{
	int err = 0;

	if (voltage == VOLTAGE_OUTPUT_1V8) {
		err = rtsx_icr_wait_voltage_stable_1(icr);
		if (err < 0)
			goto out;
	}

	if (voltage == VOLTAGE_OUTPUT_1V8)
		rtsx_icr_reg_write(icr, H_SD_LDO_CFG0, 0xFF, SD_LDO_TUNE18);
	else
		rtsx_icr_reg_write(icr, H_SD_LDO_CFG0, 0xFF, SD_LDO_TUNE33);

	if (voltage == VOLTAGE_OUTPUT_1V8) {
		err = rtsx_icr_wait_voltage_stable_2(icr);
		if (err < 0)
			goto out;
	}

	err = rtsx_icr_reg_write(icr, SD_BUS_STAT,
			SD_CLK_TOGGLE_EN | SD_CLK_TOGGLE_STOP, 0);
out:
	return err;
}

static inline int rtsx_icr_switch_voltage_fpga(struct rtsx_icr *icr,
		u8 voltage)
{
	gpio_set_value(SD_VOTAGE_SWITCH_CTRL_C2, voltage);
	msleep(50);			/* wait votage stable */
	return 0;
}

static int sdmmc_switch_voltage(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct rtsx_icr *icr = mmc_priv(mmc);
	u8 voltage;
	int err;

	if (icr->removed)
		return -ENOMEDIUM;

	if (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_330)
		voltage = VOLTAGE_OUTPUT_3V3;
	else
		voltage = VOLTAGE_OUTPUT_1V8;

	icr_dbg(icr, "switch signal_voltage to %s\n",
		voltage == VOLTAGE_OUTPUT_1V8 ? "1.8V" : "3.3V");

	mutex_lock(&icr->mutex);

	rtsx_icr_start_run(icr);

	if (icr->asic_mode)
		err = rtsx_icr_switch_voltage_asic(icr, voltage);
	else
		err = rtsx_icr_switch_voltage_fpga(icr, voltage);

	mutex_unlock(&icr->mutex);

	if (err)
		icr_err(icr, "%d: switch voltage failed\n", err);
	return err;
}

static int rtsx_icr_change_phase(struct rtsx_icr *icr, u8 sample_point)
{
	int err;

	icr_dbg(icr, "sample_point = %d\n", sample_point);

	rtsx_icr_init_cmd(icr);
	rtsx_icr_write(icr, SD_CFG1, SD_ASYNC_FIFO_NOT_RST, 0);
	rtsx_icr_write(icr, H_PHSEL1, U_PHSEL1_MASK, sample_point);
	err = rtsx_icr_transfer_cmd(icr);
	if (err < 0)
		return err;

	return 0;
}

static inline u32 rtsx_icr_test_phase_bit(u32 phase_map, unsigned int bit)
{
	bit %= TUNING_PHASE_MAX;
	return phase_map & (1 << bit);
}

static int rtsx_icr_get_phase_len(u32 phase_map, unsigned int start_bit)
{
	int i;

	for (i = 0; i < TUNING_PHASE_MAX; i++)
		if (rtsx_icr_test_phase_bit(phase_map, start_bit + i) == 0)
			return i;

	return TUNING_PHASE_MAX;
}

static int rtsx_icr_search_final_phase(struct rtsx_icr *icr, u32 phase_map)
{
	int start = 0, len = 0;
	int start_final = 0, len_final = 0;
	int final_phase = -EINVAL;

	if (phase_map == 0) {
		icr_err(icr, "phase map: %x\n", phase_map);
		return final_phase;
	}

	while (start < TUNING_PHASE_MAX) {
		len = rtsx_icr_get_phase_len(phase_map, start);
		if (len_final < len) {
			start_final = start;
			len_final = len;
		}
		start += len ? len : 1;
	}

	final_phase = (start_final + len_final / 2) % TUNING_PHASE_MAX;
	icr_dbg(icr, "phase info: [map:%x] [maxlen:%d] [final:%d]\n",
		phase_map, len_final, final_phase);

	return final_phase;
}

static void rtsx_icr_wait_data_idle(struct rtsx_icr *icr)
{
	int err = 0;
	int i;
	u8 val = 0;

	for (i = 0; i < 100; i++) {
		err = rtsx_icr_reg_read(icr, SD_DATA_STATE, &val);
		if (val & SD_DATA_IDLE)
			return;

		udelay(100);
	}
	icr_dbg(icr, "wait data idle timeout\n");
}

static int rtsx_icr_tuning_rx_cmd(struct rtsx_icr *icr, u8 opcode,
		u8 sample_point)
{
	int err = 0;
	struct mmc_command cmd = {0};

	err = rtsx_icr_change_phase(icr, sample_point);
	if (err)
		return err;

	cmd.opcode = opcode;
	err = rtsx_icr_read_short_data(icr, &cmd, NULL, 0x40);
	if (err) {
		rtsx_icr_wait_data_idle(icr);
		rtsx_icr_clear_error(icr);
		return err;
	}

	return err;
}

static void rtsx_icr_tuning_rx_phase(struct rtsx_icr *icr, u8 opcode,
		u32 *phase_map)
{
	int i;
	u32 raw_phase_map = 0;

	for (i = 0; i < TUNING_PHASE_MAX; i++)
		if (!rtsx_icr_tuning_rx_cmd(icr, opcode, i))
			raw_phase_map |= 1 << i;

	if (phase_map)
		*phase_map = raw_phase_map;
}

static int rtsx_icr_tuning_rx(struct rtsx_icr *icr, u8 opcode)
{
	int err = 0;
	int i;
	u32 raw_phase_map[TUNING_RETRY_CNT] = {0}, phase_map;
	int final_phase;

	for (i = 0; i < TUNING_RETRY_CNT; i++) {
		rtsx_icr_tuning_rx_phase(icr, opcode, &(raw_phase_map[i]));
		if (!raw_phase_map[i])
			break;
	}

	phase_map = 0xFFFFFFFF;
	for (i = 0; i < TUNING_RETRY_CNT; i++) {
		icr_dbg(icr, "RX raw_phase_map[%d] = 0x%08x\n", i,
			raw_phase_map[i]);
		phase_map &= raw_phase_map[i];
	}
	icr_dbg(icr, "RX phase_map = 0x%08x\n", phase_map);

	if (!phase_map)
		return -EINVAL;

	final_phase = rtsx_icr_search_final_phase(icr, phase_map);
	if (final_phase < 0)
		return final_phase;

	err = rtsx_icr_change_phase(icr, final_phase);
	if (err)
		return err;

	return err;
}

static int sdmmc_execute_tuning(struct mmc_host *mmc, u32 opcode)
{
	struct rtsx_icr *icr = mmc_priv(mmc);
	int err = 0;

	if (icr->removed)
		return -ENOMEDIUM;

	mutex_lock(&icr->mutex);
	rtsx_icr_start_run(icr);
	if (icr->sd_mode != RTSX_SD_DDR_MODE)
		err = rtsx_icr_tuning_rx(icr, MMC_SEND_TUNING_BLOCK);
	mutex_unlock(&icr->mutex);

	return err;
}

static const struct mmc_host_ops sdmmc_ops = {
	.request			= sdmmc_request,
	.set_ios			= sdmmc_set_ios,
	.get_ro				= sdmmc_get_ro,
	.get_cd				= sdmmc_get_cd,
	.start_signal_voltage_switch	= sdmmc_switch_voltage,
	.card_busy			= sdmmc_card_busy,
	.execute_tuning			= sdmmc_execute_tuning,
};

static void rtsx_icr_init_mmc_host(struct rtsx_icr *icr)
{
	struct mmc_host *mmc = icr->mmc;

	mmc->f_min = 250000;
	mmc->f_max = 208000000;
	mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;
	mmc->caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA |
		MMC_CAP_BUS_WIDTH_TEST | MMC_CAP_SD_HIGHSPEED |
		MMC_CAP_MMC_HIGHSPEED;
	if (icr->asic_mode)
		mmc->caps |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
			MMC_CAP_UHS_SDR50;
	mmc->max_current_330 = 400;
	mmc->max_current_180 = 800;
	mmc->ops = &sdmmc_ops;

	mmc->max_segs = 256;
	mmc->max_seg_size = 65536;
	mmc->max_blk_size = 512;
	mmc->max_blk_count = 65535;
	mmc->max_req_size = 524288;
}

static int rtsx_icr_gpio_request_fpga(void)
{
	int err;

	err = gpio_request(SD_PWR_CTRL_C2, "SD_PWR_CTRL_C2");
	if (err)
		goto release_1;

	err = gpio_request(SD_VOTAGE_SWITCH_CTRL_C2,
			"SD_VOTAGE_SWITCH_CTRL_C2");
	if (err)
		goto release_2;

	err = gpio_request(SD_PULL_CTRL_C2, "SD_PULL_CTRL_C2");
	if (err)
		goto release_3;

	gpio_direction_output(SD_PWR_CTRL_C2, 0);
	gpio_direction_output(SD_VOTAGE_SWITCH_CTRL_C2, 0);
	gpio_direction_output(SD_PULL_CTRL_C2, 0);

	gpio_set_value(SD_PWR_CTRL_C2, 0);
	gpio_set_value(SD_PULL_CTRL_C2, 0);

	return 0;

release_3:
	gpio_free(SD_VOTAGE_SWITCH_CTRL_C2);
release_2:
	gpio_free(SD_PWR_CTRL_C2);
release_1:
	return err;
}

static void rtsx_icr_gpio_free_fpga(void)
{
	gpio_free(SD_PWR_CTRL_C2);
	gpio_free(SD_VOTAGE_SWITCH_CTRL_C2);
	gpio_free(SD_PULL_CTRL_C2);
}

static int rtsx_icr_gpio_request_asic(void)
{
	int err;

	err = gpio_request(SD_PWR_CTRL_C2, "SD_PWR_CTRL_C2");
	if (err)
		return err;

	gpio_direction_output(SD_PWR_CTRL_C2, 0);
	gpio_set_value(SD_PWR_CTRL_C2, 0);

	return 0;
}

static void rtsx_icr_gpio_free_asic(void)
{
	gpio_free(SD_PWR_CTRL_C2);
}

static int rtsx_icr_probe(struct platform_device *pdev)
{
	int err = 0;
	struct resource *res;
	struct rtsx_icr *icr;
	struct mmc_host *mmc;
	const struct platform_device_id *id_entry;
	enum rtsx_sdhc_devtype sdhc_devtype;

	id_entry = platform_get_device_id(pdev);
	sdhc_devtype = id_entry->driver_data;

	mmc = mmc_alloc_host(sizeof(*icr), &pdev->dev);
	if (!mmc)
		return -ENOMEM;
	icr = mmc_priv(mmc);

	if ((sdhc_devtype & TYPE_FPGA) == 0)
		icr->asic_mode = 1;
	icr->max_clock = icr->asic_mode ? DATA_MAX_CLK : FPGA_MAX_CLK;
	icr->dev = &pdev->dev;
	icr->mmc = mmc;
	icr->power_state = RTSX_SD_POWER_OFF;
	icr->cmd_timeout_ms = RTSX_ICR_CMD_TIMEOUT_MS;
	icr->data_timeout_ms = RTSX_ICR_DAT_TIMEOUT_MS;
	rtsx_icr_init_mmc_host(icr);

	platform_set_drvdata(pdev, icr);

	err = platform_get_irq(pdev, 0);
	if (err < 0) {
		pr_err("device has no IRQ, check setup\n");
		goto free_mmc;
	}
	icr->irq = err;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("device has no MEM, check setup\n");
		err = -ENOMEM;
		goto free_mmc;
	}
	icr->addr = res->start;
	icr->addr_len = res->end - res->start + 1;
	if (!request_mem_region(icr->addr, icr->addr_len, RTSX_ICR_DRV_NAME)) {
		pr_err("request memory failed\n");
		err = -ENOMEM;
		goto free_mmc;
	}
	icr->remap_addr = ioremap(icr->addr, icr->addr_len);
	if (!icr->remap_addr) {
		pr_err("mapping memory failed\n");
		err = -ENOMEM;
		goto free_mem;
	}

	icr->resv_buf = dma_alloc_coherent(icr->dev, RESV_BUF_LEN,
			&icr->resv_addr, GFP_KERNEL);
	if (!icr->resv_buf) {
		err = -ENXIO;
		goto unmap_resource;
	}
	icr->cmd_ptr = icr->resv_buf;
	icr->cmd_addr = icr->resv_addr;
	icr->sg_tbl_ptr = icr->resv_buf + CMD_BUF_LEN;
	icr->sg_tbl_addr = icr->resv_addr + CMD_BUF_LEN;

	INIT_DELAYED_WORK(&icr->detect_work, rtsx_icr_card_detect);
	INIT_DELAYED_WORK(&icr->idle_work, rtsx_icr_idle);

	err = rtsx_icr_acquire_irq(icr);
	if (err < 0)
		goto free_dma;
	synchronize_irq(icr->irq);

	if (icr->asic_mode) {
		err = rtsx_icr_gpio_request_asic();
		if (err) {
			pr_err("request GPIO failed\n");
			goto unmap_resource;
		}
	} else {
		err = rtsx_icr_gpio_request_fpga();
		if (err) {
			pr_err("request GPIO failed\n");
			goto unmap_resource;
		}
	}

	err = rtsx_icr_init(icr);
	if (err < 0)
		goto free_irq;

	schedule_delayed_work(&icr->idle_work, msecs_to_jiffies(200));

	mmc_add_host(mmc);

	rtsx_icr_add_debugfs(icr);
	return 0;

free_irq:
	free_irq(icr->irq, (void *)icr);
free_dma:
	dma_free_coherent(icr_dev(icr), RESV_BUF_LEN, icr->resv_buf,
			icr->resv_addr);
unmap_resource:
	iounmap(icr->remap_addr);
free_mem:
	release_mem_region(icr->addr, icr->addr_len);
free_mmc:
	platform_set_drvdata(pdev, NULL);
	mmc_free_host(mmc);

	return err;
}

static int rtsx_icr_remove(struct platform_device *pdev)
{
	struct rtsx_icr *icr = platform_get_drvdata(pdev);
	struct mmc_host *mmc = icr->mmc;

	if (icr->mrq) {
		icr_dbg(icr, "RTSX_ICR driver removed during transfer\n");
		rtsx_icr_transfer_stop(icr);
		icr->mrq->cmd->error = -ENOMEDIUM;
		if (icr->mrq->stop)
			icr->mrq->stop->error = -ENOMEDIUM;
		mmc_request_done(mmc, icr->mrq);
	}

	rtsx_icr_remove_debugfs(icr);
	mmc_remove_host(mmc);
	icr->removed = true;

	cancel_delayed_work_sync(&icr->detect_work);
	cancel_delayed_work_sync(&icr->idle_work);

	free_irq(icr->irq, icr);
	dma_free_coherent(icr_dev(icr), RESV_BUF_LEN, icr->resv_buf,
			icr->resv_addr);
	iounmap(icr->remap_addr);
	mmc_free_host(mmc);
	platform_set_drvdata(pdev, NULL);
	release_mem_region(icr->addr, icr->addr_len);

	if (icr->asic_mode)
		rtsx_icr_gpio_free_asic();
	else
		rtsx_icr_gpio_free_fpga();

	pr_info("RTSX_ICR driver removed\n");

	return 0;
}

#ifdef CONFIG_PM

static int rtsx_icr_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct rtsx_icr *icr = platform_get_drvdata(pdev);
	struct mmc_host *mmc = icr->mmc;

	int err = 0;

	icr_dbg(icr, "\n");
	err = mmc_suspend_host(mmc);
	if (err)
		return err;

	cancel_delayed_work(&icr->detect_work);
	cancel_delayed_work(&icr->idle_work);

	mutex_lock(&icr->mutex);

	rtsx_icr_writel(icr, BIER, 0);
	icr->bier = 0;

	mutex_unlock(&icr->mutex);
	return err;
}

static int rtsx_icr_resume(struct platform_device *pdev)
{
	struct rtsx_icr *icr = platform_get_drvdata(pdev);
	struct mmc_host *mmc = icr->mmc;
	int err = 0;

	icr_dbg(icr, "\n");

	mutex_lock(&icr->mutex);

	err = rtsx_icr_init_hw(icr);
	if (err)
		goto out;

	schedule_delayed_work(&icr->idle_work, msecs_to_jiffies(200));

out:
	mutex_unlock(&icr->mutex);
	err = mmc_resume_host(mmc);
	if (err)
		return err;

	return err;
}

#else /* CONFIG_PM */

#define rtsx_icr_suspend NULL
#define rtsx_icr_resume NULL

#endif /* CONFIG_PM */

static struct platform_driver rtsx_icr_driver = {
	.probe		= rtsx_icr_probe,
	.remove		= rtsx_icr_remove,
	.suspend	= rtsx_icr_suspend,
	.resume		= rtsx_icr_resume,
	.driver		= {
		.name = RTSX_ICR_DRV_NAME,
	},
	.id_table	= rtsx_sdhc_devtypes,
};

#ifdef CONFIG_MMC_REALTEK_IPCAM_DEBUG_FS
struct rtsx_icr_debugfs {
	struct dentry *root;
	u64 ocp_register;
	u32 ip_register;
};

#define simple_param_template(func, param)		\
static int func##_set(void *data, u64 value)		\
{							\
	((struct rtsx_icr *) data)->param = value;	\
	return 0;					\
}							\
static int func##_get(void *data, u64 *value)		\
{							\
	*value = ((struct rtsx_icr *) data)->param;	\
	return 0;					\
}							\
DEFINE_SIMPLE_ATTRIBUTE(func##_fops, func##_get, func##_set, "%lld\n")

static int rtsx_icr_ocp_register_set(void *data, u64 value)
{
	struct rtsx_icr *icr = data;
	struct rtsx_icr_debugfs *dfs = icr->debugfs;
	u32 addr;
	u32 val;

	if (value < 0xFFFFFFFF)
		addr = value;
	else
		addr = value >> 32;

	if (addr > 0x40) {
		icr_err(icr, "register 0x%4x out of range\n", addr);
		return -EINVAL;
	}

	mutex_lock(&icr->mutex);
	rtsx_icr_lock_irqsave(icr); /* for BIPR */
	if (value > 0xFFFFFFFF)
		rtsx_icr_writel(icr, addr, value);
	val = rtsx_icr_readl(icr, addr);
	rtsx_icr_unlock_irqrestore(icr);
	mutex_unlock(&icr->mutex);

	dfs->ocp_register = ((u64) addr << 32) | val;
	icr_dbg(icr, "OCP register: 0x%08x: 0x%08x\n", addr, val);

	return 0;
}

static int rtsx_icr_ocp_register_get(void *data, u64 *value)
{
	struct rtsx_icr *icr = data;
	struct rtsx_icr_debugfs *dfs = icr->debugfs;
	u32 addr = dfs->ocp_register >> 32;

	if (addr > 0x40) {
		icr_err(icr, "register 0x%4x out of range\n", addr);
		return -EINVAL;
	}

	dfs->ocp_register &= ~0xFFFFFFFF;

	mutex_lock(&icr->mutex);
	rtsx_icr_lock_irqsave(icr); /* for BIPR */
	dfs->ocp_register |= rtsx_icr_readl(icr, addr);
	rtsx_icr_unlock_irqrestore(icr);
	mutex_unlock(&icr->mutex);

	*value = dfs->ocp_register;

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(rtsx_icr_dbg_ocp_register_fops,
	rtsx_icr_ocp_register_get, rtsx_icr_ocp_register_set, "0x%016llx\n");

static int rtsx_icr_ip_register_set(void *data, u64 value)
{
	struct rtsx_icr *icr = data;
	struct rtsx_icr_debugfs *dfs = icr->debugfs;
	u16 addr;
	u8 val;

	if (value <= 0xFFFF)
		addr = value;
	else
		addr = value >> 16;

	if (addr < REG_MIN_ADDR || addr > REG_MAX_ADDR) {
		icr_err(icr, "register 0x%04x out of range\n", addr);
		return -EINVAL;
	}

	mutex_lock(&icr->mutex);
	if (value > 0xFFFF)
		rtsx_icr_reg_write(icr, addr, value >> 8, value);

	rtsx_icr_reg_read(icr, addr, &val);
	mutex_unlock(&icr->mutex);

	dfs->ip_register = (addr << 16) | val;

	icr_dbg(icr, "IP register: 0x%04x, 0x%02x\n", addr, val);

	return 0;
}

static int rtsx_icr_ip_register_get(void *data, u64 *value)
{
	struct rtsx_icr *icr = data;
	struct rtsx_icr_debugfs *dfs = icr->debugfs;
	u16 addr = dfs->ip_register >> 16;
	u8 val;

	if (addr < REG_MIN_ADDR || addr > REG_MAX_ADDR) {
		icr_err(icr, "register 0x%04x out of range\n", addr);
		return -EINVAL;
	}

	mutex_lock(&icr->mutex);
	rtsx_icr_reg_read(icr, addr, &val);
	mutex_unlock(&icr->mutex);

	dfs->ip_register &= ~0xFFFF;
	dfs->ip_register |= val;
	*value = dfs->ip_register;

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(rtsx_icr_dbg_ip_register_fops,
	rtsx_icr_ip_register_get, rtsx_icr_ip_register_set, "0x%08llx\n");

static int rtsx_icr_card_exist_set(void *data, u64 value)
{
	struct rtsx_icr *icr = data;

	if ((bool)(icr->card_status & SD_EXIST) == (bool)value)
		return 0;

	if (value)
		rtsx_icr_insert_card(icr);
	else
		rtsx_icr_remove_card(icr);

	return 0;
}

static int rtsx_icr_card_exist_get(void *data, u64 *value)
{
	struct rtsx_icr *icr = data;

	*value = icr->card_status & SD_EXIST;

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(rtsx_icr_dbg_card_exist_fops,
	rtsx_icr_card_exist_get, rtsx_icr_card_exist_set, "0x%llx\n");

static int rtsx_icr_support_uhs_set(void *data, u64 value)
{
	struct rtsx_icr *icr = data;
	struct mmc_host *mmc = icr->mmc;
	u32 uhs_caps = MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
		MMC_CAP_UHS_SDR50;

	if (value == 1)
		mmc->caps |= uhs_caps;
	else if (value)
		mmc->caps = value;
	else
		mmc->caps &= ~uhs_caps;

	return 0;
}
static int rtsx_icr_support_uhs_get(void *data, u64 *value)
{
	struct rtsx_icr *icr = data;
	struct mmc_host *mmc = icr->mmc;

	*value = mmc->caps;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(rtsx_icr_dbg_support_uhs_fops,
	rtsx_icr_support_uhs_get, rtsx_icr_support_uhs_set, "0x%llx\n");

static int rtsx_icr_trigger_set(void *data, u64 value)
{
	struct rtsx_icr *icr = data;

	icr->trigger_gpio = value & 0xFF;
	icr->trigger_cmd = (value & 0xFF00) >> 8;
	icr->trigger_count = (value & 0xFF0000) >> 16;
	return 0;
}
static int rtsx_icr_trigger_get(void *data, u64 *value)
{
	struct rtsx_icr *icr = data;

	*value = (icr->trigger_count << 16) | (icr->trigger_cmd << 8) |
		(icr->trigger_gpio);

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(rtsx_icr_dbg_trigger_fops,
	rtsx_icr_trigger_get, rtsx_icr_trigger_set, "0x%llx\n");

simple_param_template(rtsx_icr_max_clock, max_clock);
simple_param_template(rtsx_icr_min_clock, min_clock);
simple_param_template(rtsx_icr_cmd_timeout, cmd_timeout_ms);
simple_param_template(rtsx_icr_data_timeout, data_timeout_ms);

void rtsx_icr_add_debugfs(struct rtsx_icr *icr)
{
	struct rtsx_icr_debugfs *dfs;
	struct mmc_host *host = icr->mmc;

	if (!host->debugfs_root)
		return;

	icr->debugfs = kmalloc(sizeof(*dfs), GFP_KERNEL);
	if (!icr->debugfs)
		return;

	dfs = icr->debugfs;
	dfs->ip_register = REG_MIN_ADDR << 16;

	dfs->root = debugfs_create_dir(dev_name(icr->dev), host->debugfs_root);
	if (IS_ERR(dfs->root))
		return;

	if (!dfs->root)
		goto err_exit;

#define add_file(fname, fops)					\
	if (!debugfs_create_file(fname, S_IRUSR | S_IWUSR,	\
			dfs->root, icr, &fops))			\
		goto err_exit

	add_file("ip_register", rtsx_icr_dbg_ip_register_fops);
	add_file("ocp_register", rtsx_icr_dbg_ocp_register_fops);
	add_file("card_exist", rtsx_icr_dbg_card_exist_fops);
	add_file("support_uhs", rtsx_icr_dbg_support_uhs_fops);
	add_file("trigger", rtsx_icr_dbg_trigger_fops);
	add_file("max_clock", rtsx_icr_max_clock_fops);
	add_file("min_clock", rtsx_icr_min_clock_fops);
	add_file("cmd_timeout", rtsx_icr_cmd_timeout_fops);
	add_file("data_timeout", rtsx_icr_data_timeout_fops);

#undef add_file

	return;
err_exit:
	debugfs_remove_recursive(dfs->root);
	dfs->root = NULL;
	icr_err(icr, "failed init debugfs\n");
}

static void rtsx_icr_remove_debugfs(struct rtsx_icr *icr)
{
	debugfs_remove_recursive(icr->debugfs->root);
	kfree(icr->debugfs);
}
#endif	/* CONFIG_MMC_REALTEK_IPCAM_DEBUG_FS */

module_platform_driver(rtsx_icr_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Micky Ching <micky_ching@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek ipcam sd/mmc host controller driver");
