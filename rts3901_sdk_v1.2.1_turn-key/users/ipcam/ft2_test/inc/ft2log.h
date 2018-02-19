/*
 * Realtek Semiconductor Corp.
 *
 * inc/ft2log.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _INC_FT2LOG_H
#define _INC_FT2LOG_H

#define DEBUG_FT2

#ifdef DEBUG_FT2
#define FT2_LOG_INFO(...)			\
	do {\
		fprintf(stdout, "info [%s:%d]: ", __FUNCTION__, __LINE__);\
		fprintf(stdout, __VA_ARGS__);\
	} while(0)

#define FT2_LOG_WARNING(...)			\
	do {\
		fprintf(stdout, "warn [%s:%d]: ", __FUNCTION__, __LINE__);\
		fprintf(stdout, __VA_ARGS__);\
	} while(0)

#define FT2_LOG_ERR(...)			\
	do {\
		fprintf(stdout, "err [%s:%d]: ", __FUNCTION__, __LINE__);\
		fprintf(stdout, __VA_ARGS__);\
	} while(0)

#define FT2_LOG_OPT(...)			\
	do {\
		fprintf(stdout, "[%s:%d]: ", __FUNCTION__, __LINE__);\
		fprintf(stdout, __VA_ARGS__);\
	} while(0)

#else
#define FT2_LOG_INFO(...)
#define FT2_LOG_WARNING(...)
#define FT2_LOG_ERR(...)
#define FT2_LOG_OPT(...)
#endif /* #ifdef DEBUG */

#endif
