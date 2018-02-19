#ifndef _RTS_DBG_OPT_H_
#define _RTS_DBG_OPT_H_

#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>

#define FMT_FOURCC(f) (f & 0xff) , ((f & 0xff00)>>8) , ((f & 0xff0000)>>16) , ((f & 0xff000000)>>24)

int rts_calc_timeval(struct timeval begin, struct timeval end);
int save_data_to_file(void *data, int length, char *filename);
int check_path(const char *path_name);
uint32_t get_file_size(const char *name);

#endif

