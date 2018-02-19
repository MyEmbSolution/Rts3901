/*
 * libmtdops.h
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
#ifndef LIBMTDOPS_H
#define LIBMTDOPS_H /* LIBMTDOPS_H */

#include <linux/types.h>
#include <inttypes.h>
#include <libmtd.h>
#include <stdio.h>

#define MTD_DEV_NAME_PATTERN			"/dev/mtd%d"
#define MTD_DEV_NAME_LEN_MAX			32
#define MTD_DEV_NUM_INLINE			10
#define MTD_DEV_OPS_UNIT			0x10000

#define print_dbg(fmt, arg...)				\
	printf("%s: " fmt, __func__, ##arg)
#define print_err(fmt, arg...)				\
	printf("%s: ERROR: " fmt, __func__, ##arg)
#define print_warn(fmt, arg...)				\
	printf("%s: WARNING: " fmt, __func__, ##arg)

struct mtd_node {
	char name[MTD_DEV_NAME_LEN_MAX];
};
static inline struct mtd_node mtd_node_at(int i)
{
	struct mtd_node node;
	sprintf(node.name, MTD_DEV_NAME_PATTERN, i);
	return node;
}

struct mtd_map {
	libmtd_t		mtd;
	__u64			total_size;
	struct mtd_info		info;
	struct mtd_dev_info	*map;
	struct mtd_dev_info	_map[MTD_DEV_NUM_INLINE];
};

struct mtd_map *mtd_map_alloc(void);
void mtd_map_free(struct mtd_map *map);
void mtd_map_dump(const struct mtd_map *map);

int mtd_erase_node(const struct mtd_map *map, const char *name);
int mtd_write_node(const struct mtd_map *map, const char *name,
	const void *data, int len);

/* cross partition ops */
int mtds_erase(const struct mtd_map *map, __u64 offset, __u64 len);
int mtds_read(const struct mtd_map *map, int offset, void *data, int len);
int __mtds_write(const struct mtd_map *map, int offset, void *data, int len);
int mtds_write(const struct mtd_map *map, int offset, void *data, int len);
#endif /* LIBMTDOPS_H */

