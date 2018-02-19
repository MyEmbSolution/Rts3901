#include <libmtdops.h>

int main(int argc, char *argv[])
{
	struct mtd_map *map;
	const struct mtd_info *info;
	int i;

	map = mtd_map_alloc();
	if (!map)
		return -1;

//	mtd_map_dump(map);

	info = &map->info;
	for (i = info->lowest_mtd_num; i <= info->highest_mtd_num; i++) {
		const struct mtd_dev_info *m = &map->map[i];

		if (!strncmp(m->name, "ldc", 3))
			printf("Found ldc table partition (/dev/mtd%d)!\n", m->mtd_num);
	}

	mtd_map_free(map);

	return 0;
}
