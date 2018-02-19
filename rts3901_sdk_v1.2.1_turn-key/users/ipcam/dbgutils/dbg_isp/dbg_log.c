#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "dbg_log.h"

uint32_t g_show_mask;

FILE *realsil_log_file = NULL;

int realsil_log_flag = 1;

int realsil_log_name(const char *logname)
{
	if (logname)
		setenv("RTS_DBG_LOG_FILENAME", logname, 1);
}

int realsil_log_open()
{
	char *lfname;

	if (!realsil_log_file) {
		lfname = getenv("RTS_DBG_LOG_FILENAME");
		if (lfname)
			realsil_log_file = fopen(lfname, "w");
	}
	return 0;
}

int realsil_log_close()
{
	if (realsil_log_file)
		fclose(realsil_log_file);
	return 0;
}

