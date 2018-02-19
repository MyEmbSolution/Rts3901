#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

int rts_calc_timeval(struct timeval begin, struct timeval end)
{

	/*if tv_usec is unsigned int, is it right*/
	int ms = (end.tv_sec - begin.tv_sec) * 1000 + (end.tv_usec - begin.tv_usec) / 1000;

	return ms;

}

int save_data_to_file(void *data, int length, char *filename)
{
	FILE * pFile = NULL;

	pFile = fopen (filename, "wb");
	if (NULL == pFile) {
		fprintf(stderr, "open file %s failed\n", filename);
		return -1;
	}
	fwrite(data, 1, length, pFile);
	fclose(pFile);
	return 0;
}


int check_path(const char *path_name)
{
	struct stat stat_buff;

	if ( 0 > stat(path_name, &stat_buff))
		return -1;

	if (S_ISDIR(stat_buff.st_mode))
		return 0;
	return -1;
}

uint32_t get_file_size(const char *name)
{
	struct stat stat_buff;

	stat(name, &stat_buff);
	//fprintf(stdout, "%s : %d\n", name, stat_buff.st_size);
	return (uint32_t)stat_buff.st_size;
}
