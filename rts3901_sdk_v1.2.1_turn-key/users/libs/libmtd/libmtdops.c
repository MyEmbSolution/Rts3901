/*
 * libmtdops.c
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

#include <inttypes.h>
#include <endian.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <linux/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <mtd_swab.h>
#include <mtd/mtd-user.h>
#include <libmtdops.h>

static int mtd_map_init_info(struct mtd_map *map)
{
	map->mtd = libmtd_open();
	if (!map->mtd)
		return -1;

	if (mtd_get_info(map->mtd, &map->info) < 0)
		return -1;

	if (map->info.highest_mtd_num > MTD_DEV_NUM_INLINE)
		return -1;

	return 0;
}

static int mtd_map_init_map(struct mtd_map *map)
{
	struct mtd_info *info = &map->info;
	int i;

	for (i = info->lowest_mtd_num; i <= info->highest_mtd_num; i++) {
		if (mtd_get_dev_info1(map->mtd, i, &map->map[i]) < 0)
			return -1;
		map->total_size += map->map[i].size;
	}

	return 0;
}

struct mtd_map *mtd_map_alloc(void)
{
	struct mtd_map *map;

	map = calloc(1, sizeof(*map));
	if (!map)
		return NULL;
	map->map = map->_map;

	if (mtd_map_init_info(map) < 0)
		goto failed;

	if (mtd_map_init_map(map) < 0)
		goto failed;
	return map;

failed:
	mtd_map_free(map);
	return NULL;
}


void mtd_map_free(struct mtd_map *map)
{
	if (map->map != map->_map) {
		free(map->map);
	}
	libmtd_close(map->mtd);
	free(map);
}


void mtd_map_dump(const struct mtd_map *map)
{
	const struct mtd_info *info = &map->info;
	int i;

	print_dbg("MTD flash size: 0x%llx\n", map->total_size);
	for (i = info->lowest_mtd_num; i <= info->highest_mtd_num; i++) {
		const struct mtd_dev_info *m = &map->map[i];

		print_dbg("%s(%d): %s, size: 0x%08llx, eb_size: 0x%08x, eb_cnt: 0x%08x\n",
			m->name, m->mtd_num, m->type_str,
			m->size, m->eb_size, m->eb_cnt);
	}
}


static int __mtd_erase_node(const char *name, __u64 offset, __u64 len)
{
	struct erase_info_user erase = {
		.start = offset,
		.length = len,
	};
	int fd;

	/* print_dbg("%s: offset 0x%llx, len 0x%llx\n", name, offset, len); */
	if (offset % MTD_DEV_OPS_UNIT || len % MTD_DEV_OPS_UNIT)
		print_err("erase(0x%llx, 0x%llx) not aligned\n", offset, len);

	fd = open(name, O_RDWR);
	if (fd < 0)
		return -1;

	if (ioctl(fd, MEMERASE, &erase) < 0)
		goto close_dev;

	close(fd);
	return 0;
close_dev:
	close(fd);
	return -1;
}

int mtd_erase_node(const struct mtd_map *map, const char *name)
{
	const struct mtd_info *info = &map->info;
	int i;

	for (i = info->lowest_mtd_num; i <= info->highest_mtd_num; i++) {
		struct mtd_node node = mtd_node_at(i);
		const struct mtd_dev_info *m = &map->map[i];

		sprintf(node.name, MTD_DEV_NAME_PATTERN, i);
		if (!strcmp(node.name, name))
			return __mtd_erase_node(node.name, 0, m->size);
	}
	return 0;
}

int mtd_write_node(const struct mtd_map *map, const char *name,
	const void *data, int len)
{
	int err, fd;

	err = mtd_erase_node(map, name);
	if (err)
		return err;

	fd = open(name, O_RDWR);
	if (fd < 0) {
		print_err("open %s failed\n", name);
		return fd;
	}

	if (write(fd, data, len) != len) {
		print_err("write %s failed\n", name);
		return -1;
	}

	return 0;
}

int mtds_erase(const struct mtd_map *map, __u64 offset, __u64 len)
{
	const struct mtd_info *info = &map->info;
	__u64 pos = 0, unit = 0;
	int i;

	for (i = info->lowest_mtd_num; i <= info->highest_mtd_num; i++) {
		struct mtd_node node = mtd_node_at(i);
		const struct mtd_dev_info *m = &map->map[i];

		if (offset < pos + m->size) {
			unit = len < m->size ? len : m->size;
			if (__mtd_erase_node(node.name, offset - pos, unit) < 0)
				return -1;
			offset += unit;
			len -= unit;
			if (!len)
				break;
		}
		pos += m->size;
	}

	if (len)
		print_err("erase left: %lld bytes\n", len);
	return 0;
}

static int __mtd_read_node(const char *name, void *data,
	__u64 offset, __u64 len)
{
	int fd;

	/* print_dbg("%s: offset 0x%llx, len 0x%llx\n", name, offset, len); */
	fd = open(name, O_RDONLY);
	if (fd < 0) {
		print_err("open %s failed\n", name);
		return fd;
	}

	if (lseek(fd, offset, SEEK_SET) < 0) {
		print_err("seek %s to %lld failed\n", name, offset);
		close(fd);
		return -1;
	}

	if (read(fd, data, len) != len) {
		print_err("write %s %lld bytes failed\n", name, len);
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int mtds_read(const struct mtd_map *map, int offset, void *data, int len)
{
	const struct mtd_info *info = &map->info;
	__u64 pos = 0, unit = 0;
	int i;

	for (i = info->lowest_mtd_num; i <= info->highest_mtd_num; i++) {
		struct mtd_node node = mtd_node_at(i);
		const struct mtd_dev_info *m = &map->map[i];

		if (offset < pos + m->size) {
			unit = len < m->size ? len : m->size;
			if (__mtd_read_node(node.name, data,
					offset - pos, unit) < 0)
				return -1;
			offset += unit;
			len -= unit;
			if (!len)
				break;
		}
		pos += m->size;
	}

	if (len)
		print_err("read left: %d bytes\n", len);
	return 0;
}

int __mtd_write_node(const char *name, void *data, __u64 offset, __u64 len)
{
	int fd;

	/* print_dbg("%s: offset 0x%llx, len 0x%llx\n", name, offset, len); */
	fd = open(name, O_WRONLY);
	if (fd < 0) {
		print_err("open %s failed\n", name);
		return fd;
	}

	if (lseek(fd, offset, SEEK_SET) < 0) {
		print_err("seek %s to %lld failed\n", name, offset);
		close(fd);
		return -1;
	}

	if (write(fd, data, len) != len) {
		print_err("write %s %lld bytes failed\n", name, len);
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int __mtds_write(const struct mtd_map *map, int offset, void *data, int len)
{
	const struct mtd_info *info = &map->info;
	__u64 pos = 0, unit = 0;
	int i;

	for (i = info->lowest_mtd_num; i <= info->highest_mtd_num; i++) {
		struct mtd_node node = mtd_node_at(i);
		const struct mtd_dev_info *m = &map->map[i];

		if (offset < pos + m->size) {
			unit = len < m->size ? len : m->size;
			if (__mtd_write_node(node.name, data,
					offset - pos, unit) < 0)
				return -1;
			offset += unit;
			len -= unit;
			if (!len)
				break;
		}
		pos += m->size;
	}

	if (len)
		print_err("write left: %d bytes\n", len);
	return 0;
}

int mtds_write(const struct mtd_map *map, int offset, void *data, int len)
{
	if (mtds_erase(map, offset, len) < 0)
		return -1;
	return __mtds_write(map, offset, data, len);
}
