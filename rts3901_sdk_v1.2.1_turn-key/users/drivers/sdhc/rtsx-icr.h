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

#ifndef _RTSX_ICR_H_
#define _RTSX_ICR_H_

#include <linux/version.h>
#include <linux/completion.h>

static inline void reinit_completion(struct completion *x)
{
	INIT_COMPLETION(*x);
}

#define RTSX_ICR_DRV_NAME		"sd-platform"

#define RTSX_ICR_CMD_TIMEOUT_MS		200
#define RTSX_ICR_DAT_TIMEOUT_MS		10000

#ifdef CONFIG_MMC_REALTEK_IPCAM_DEBUG_FS
struct rtsx_icr_debugfs;
struct rtsx_icr;
void rtsx_icr_add_debugfs(struct rtsx_icr *icr);
static void rtsx_icr_remove_debugfs(struct rtsx_icr *icr);
#else  /* CONFIG_MMC_REALTEK_IPCAM_DEBUG_FS */
#define rtsx_icr_add_debugfs(icr)
#define rtsx_icr_remove_debugfs(icr)
#endif

struct rtsx_icr {
	struct device			*dev;

	int				irq;

	unsigned long			addr;
	unsigned long			addr_len;
	void __iomem			*remap_addr;

	dma_addr_t			resv_addr;
	void				*resv_buf;

	u32				cmd_idx;
	dma_addr_t			cmd_addr;
	void				*cmd_ptr;

	u32				sg_tbl_idx;
	dma_addr_t			sg_tbl_addr;
	void				*sg_tbl_ptr;

	u32				bier;
	u32				trans_status;
	u32				trans_wait;
	u32				card_status;
	bool				asic_mode;

	struct delayed_work		detect_work;
	struct delayed_work		idle_work;
	int				data_errors;

	spinlock_t			__lock;
	unsigned long			__lock_flags;
	struct mutex			mutex;

	struct completion		done_cmd;
	struct completion		*done;

	bool				removed;
	bool				short_dma_mode;

	const u32			*sd_pull_ctl_enable_tbl;
	const u32			*sd_pull_ctl_disable_tbl;

	u8				state;
#define RTSX_ICR_STATE_IDLE		0
#define RTSX_ICR_STATE_RUN		1

	struct mmc_host			*mmc;
	struct mmc_request		*mrq;

	u8				power_state;
#define RTSX_SD_POWER_OFF		0
#define RTSX_SD_POWER_ON		1
	bool				initial_mode;
	unsigned int			clock;
	unsigned int			max_clock;
	unsigned int			min_clock;
	u8				sd_mode;
#define RTSX_SD_DEFAULT_MODE		0
#define RTSX_SD_HS_MODE			1
#define RTSX_SD_SDR_MODE		2
#define RTSX_SD_DDR_MODE		3

	int				cmd_timeout_ms;
	int				data_timeout_ms;

#ifdef CONFIG_MMC_REALTEK_IPCAM_DEBUG_FS
	struct rtsx_icr_debugfs		*debugfs;
#endif	/* CONFIG_MMC_REALTEK_IPCAM_DEBUG_FS */
	u8				trigger_gpio;
	u8				trigger_cmd;
	u8				trigger_count;
};

#define icr_dev(icr) ((icr)->dev)
#ifdef DEBUG
#define icr_dbg(icr, fmt, arg...)				\
	dev_dbg(icr_dev(icr), "%s: " fmt, __func__, ##arg)
#define icr_err(icr, fmt, arg...)				\
	dev_err(icr_dev(icr), "%s error: " fmt, __func__, ##arg)
#else  /* DEBUG */
#define icr_dbg(icr, fmt, arg...)
#define icr_err(icr, fmt, arg...)				\
	dev_dbg(icr_dev(icr), "%s error: " fmt, __func__, ##arg)
#endif /* DEBUG */

#define rtsx_icr_control_table(addr, val) (((u32)(addr) << 16) | (u8)(val))

#define rtsx_icr_readb(icr, reg) \
	ioread8((icr)->remap_addr + reg)
#define rtsx_icr_readw(icr, reg) \
	ioread16((icr)->remap_addr + reg)
#define rtsx_icr_readl(icr, reg) \
	ioread32((icr)->remap_addr + reg)

#define rtsx_icr_writeb(icr, reg, value) \
	iowrite8(value, (icr)->remap_addr + reg)
#define rtsx_icr_writew(icr, reg, value) \
	iowrite16(value, (icr)->remap_addr + reg)
#define rtsx_icr_writel(icr, reg, value) \
	iowrite32(value, (icr)->remap_addr + reg)

static inline void rtsx_icr_lock(struct rtsx_icr *icr)
{
	spin_lock(&icr->__lock);
}
static inline void rtsx_icr_unlock(struct rtsx_icr *icr)
{
	spin_unlock(&icr->__lock);
}
static inline void rtsx_icr_lock_irqsave(struct rtsx_icr *icr)
{
	spin_lock_irqsave(&icr->__lock, icr->__lock_flags);
}
static inline void rtsx_icr_unlock_irqrestore(struct rtsx_icr *icr)
{
	spin_unlock_irqrestore(&icr->__lock, icr->__lock_flags);
}

static inline void rtsx_icr_init_cmd(struct rtsx_icr *icr)
	__acquires(&icr->__lock)
{
	rtsx_icr_lock_irqsave(icr);
	icr->cmd_idx = 0;
}
void __rtsx_icr_add_cmd(struct rtsx_icr *icr, u8 cmd_type, u16 reg_addr,
		u8 mask, u8 data);
static inline void rtsx_icr_read(struct rtsx_icr *icr, u16 reg_addr)
{
	__rtsx_icr_add_cmd(icr, 0, reg_addr, 0, 0);
}
static inline void rtsx_icr_write(struct rtsx_icr *icr, u16 reg_addr,
		u8 mask, u8 data)
{
	__rtsx_icr_add_cmd(icr, 1, reg_addr, mask, data);
}
static inline void rtsx_icr_check(struct rtsx_icr *icr, u16 reg_addr,
		u8 mask, u8 data)
{
	__rtsx_icr_add_cmd(icr, 2, reg_addr, mask, data);
}
static inline void rtsx_icr_write_be32(struct rtsx_icr *icr, u16 reg_addr,
	u32 data)
{
	rtsx_icr_write(icr, reg_addr,     0xFF, data >> 24);
	rtsx_icr_write(icr, reg_addr + 1, 0xFF, data >> 16);
	rtsx_icr_write(icr, reg_addr + 2, 0xFF, data >> 8);
	rtsx_icr_write(icr, reg_addr + 3, 0xFF, data);
}

void rtsx_icr_send_cmd(struct rtsx_icr *icr)
	__releases(&icr->__lock);
int rtsx_icr_wait_cmd(struct rtsx_icr *icr, int timeout);
static inline u8 *rtsx_icr_get_data(struct rtsx_icr *icr)
{
	return icr->cmd_ptr;
}
static inline void rtsx_icr_stop_cmd(struct rtsx_icr *icr)
{
	rtsx_icr_writel(icr, HCBCTLR, CMD_STOP);
	rtsx_icr_writel(icr, HDBCTLR, DATA_STOP);
}

int rtsx_icr_reg_read(struct rtsx_icr *icr, u16 addr, u8 *data);
int rtsx_icr_reg_write(struct rtsx_icr *icr, u16 addr, u8 mask, u8 data);
int rtsx_icr_pp_read(struct rtsx_icr *icr, u8 *buf, int len);
int rtsx_icr_pp_write(struct rtsx_icr *icr, u8 *buf, int len);
static inline int rtsx_icr_transfer_cmd_timeout(struct rtsx_icr *icr,
	int timeout)
{
	rtsx_icr_send_cmd(icr);
	return rtsx_icr_wait_cmd(icr, timeout);
}
static inline int rtsx_icr_transfer_cmd(struct rtsx_icr *icr)
{
	return rtsx_icr_transfer_cmd_timeout(icr, icr->cmd_timeout_ms);
}
int rtsx_icr_transfer_data(struct rtsx_icr *icr, struct mmc_data *data);
int rtsx_icr_transfer_stop(struct rtsx_icr *icr);

int rtsx_icr_set_power_mode(struct rtsx_icr *icr, unsigned char power_mode);
int rtsx_icr_set_bus_width(struct rtsx_icr *icr, unsigned char bus_width);
int rtsx_icr_set_timing(struct rtsx_icr *icr, unsigned char timing);
int rtsx_icr_set_clock(struct rtsx_icr *icr, unsigned int clock);

#endif /* _RTSX_ICR_H_ */
