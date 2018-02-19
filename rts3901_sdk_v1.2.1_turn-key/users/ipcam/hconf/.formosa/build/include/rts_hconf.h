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
#include <linux/types.h>
#include <sys/types.h>
#include <uapi/rts_hconf.h>

struct rts_hconf_header {
	__u32 magic;
	__u32 num;
	__u32 len;			/* data length */
} __attribute__((packed));

struct rts_hconf_entry {
	__u32 id;
	__u16 type;
	__u16 len;			/* total length */
	__u8 data[0];
} __attribute__((packed));

#define print_dbg(fmt, arg...)			\
	printf("%s: " fmt, __func__, ##arg)
#define print_err(fmt, arg...)			\
	printf("%s: ERROR: " fmt, __func__, ##arg)
#define print_raw(fmt, arg...)			\
	printf(fmt, ##arg)

#define cond_err(cond, cmd, fmt, arg...)	\
	if (cond) {				\
		print_err(fmt, ##arg);		\
		cmd;				\
	}					\

void rts_hconf_header_dump(struct rts_hconf_header *header);
void rts_hconf_entry_dump(struct rts_hconf_entry *entry);

struct rts_hconf_header *rts_hconf_header(const void *buf);
struct rts_hconf_entry *rts_hconf_entry(const void *buf);

int rts_hconf_read_header(struct rts_hconf_header *header);
int rts_hconf_write_header(const struct rts_hconf_header *header);

int rts_hconf_read_entry(struct rts_hconf_entry *entry);
int rts_hconf_write_entry(const struct rts_hconf_entry *entry);
int rts_hconf_del_entry(const struct rts_hconf_entry *entry);

int rts_hconf_save(const char *fname);
int rts_hconf_update(const char *fname);

int rts_hconf_sync(void);
int rts_hconf_clear(void);
#endif /* RTS_HCONF_H */
