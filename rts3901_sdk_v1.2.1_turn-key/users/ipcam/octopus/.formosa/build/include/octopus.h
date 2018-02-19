#ifndef __OCTOPUS_H__
#define __OCTOPUS_H__
#include <stdint.h>
#include "ipcam_errorbase.h"

#define MAX_DEV_PATH_LEN		128

int opt_open_dev(const char *name);
void opt_close_dev(int msg);

int opt_lock_dev(int msg);
int opt_trylock_dev(int msg, int timeout);
int opt_unlock_dev(int msg);

#endif
