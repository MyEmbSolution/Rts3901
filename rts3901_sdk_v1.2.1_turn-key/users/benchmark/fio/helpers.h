#ifndef FIO_HELPERS_H
#define FIO_HELPERS_H

#include "compiler/compiler.h"

#include <sys/types.h>
#include <time.h>

extern int fallocate(int fd, int mode, off_t offset, off_t len);

#endif /* FIO_HELPERS_H_ */
