/*
 * rts_hconf.h
 *
 * Copyright(C) 2015 Micky Ching, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef RTS_HCONF_H
#define RTS_HCONF_H /* RTS_HCONF_H */

#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/mtd/mtd.h>
#include <uapi/linux/rts_hconf.h>

struct rts_hconf_entry {
	u32 id;
	u16 type;
	u16 len;			/* data length */
	u8 *data;
} __attribute__((packed));
#define hconf_entry_hlen(e)		8
#define hconf_entry_len(e)		(hconf_entry_hlen(e) + (e)->len)
#define hconf_entry_data(e)		((u8 *)(e) + hconf_entry_hlen(e))

struct rts_hconf_header {
	u32 magic;
	u32 num;
	u32 len;			/* total length */

	/* -------------------- private section -------------------- */
	u32 cap_num;
	struct rts_hconf_entry *entry;
} __attribute__((packed));
#define hconf_header_hlen(h)		12

struct rts_hconf {
	struct cdev cdev;
	struct mtd_info *mtd;

	struct rts_hconf_header header;
};
#define hconf_from_inode(inode)			\
	container_of((inode)->i_cdev, struct rts_hconf, cdev)

#define hconf_dbg(fmt, arg...)			\
	pr_err("%s " fmt, __func__, ##arg)
#define hconf_err(fmt, arg...)			\
	pr_err("%s " fmt, __func__, ##arg)

struct rts_hconf *rts_hconf_pointer(void);
int rts_hconf_mtd_block_erase(struct rts_hconf *hconf, int index);
int rts_hconf_mtd_block_read(struct rts_hconf *hconf, void *buf, int index);
int rts_hconf_mtd_block_write(struct rts_hconf *hconf, void *buf, int index);

int rts_hconf_cache_clear(struct rts_hconf *hconf);
int rts_hconf_cache_init(struct rts_hconf *hconf);
int rts_hconf_cache_sync(struct rts_hconf *hconf);
int rts_hconf_cache_from_buf(struct rts_hconf *hconf, const void *buf);
int rts_hconf_cache_to_buf(struct rts_hconf *hconf, void *buf);
int rts_hconf_header_from_buf(struct rts_hconf *hconf, const void *buf);
int rts_hconf_header_to_buf(struct rts_hconf *hconf, void *buf);
int rts_hconf_entry_from_buf(struct rts_hconf *hconf, const void *buf);
int rts_hconf_entry_to_buf(struct rts_hconf *hconf, void *buf);
int rts_hconf_del_entry(struct rts_hconf *hconf, void *buf);

#endif /* RTS_HCONF_H */
