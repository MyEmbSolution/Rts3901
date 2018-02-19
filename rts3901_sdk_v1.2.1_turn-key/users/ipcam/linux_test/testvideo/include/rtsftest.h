/*
 * Realtek Semiconductor Corp.
 *
 * include/rtsftest.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _INCLUDE_RTSFTEST_H
#define _INCLUDE_RTSFTEST_H

enum rts_ftest_result {
	RTS_FTEST_OK		= 0,
	RTS_FTEST_ERROR		= 1,
	RTS_FTEST_E_NO_MEMORY,
	RTS_FTEST_E_NULL_POINT,
	RTS_FTEST_E_NOT_INITIALIZED,
	RTS_FTEST_E_NOT_REALIZED,
	RTS_FTEST_E_INVALID_ARG,
	RTS_FTEST_E_EXIST,
	RTS_FTEST_E_OPEN_FAIL,
	RTS_FTEST_E_NOT_FOUND,
	RTS_FTEST_E_NOT_SUPPORT,
	RTS_FTEST_RESERVED,
};

enum rts_ftest_priority {
	RTS_FTEST_LEVEL_UNKOWN = 0,
	RTS_FTEST_LEVEL_1 = 1,
	RTS_FTEST_LEVEL_2,
	RTS_FTEST_LEVEL_3,
	RTS_FTEST_LEVEL_4,
	RTS_FTEST_LEVEL_5, /*default priority*/
	RTS_FTEST_LEVEL_6,
	RTS_FTEST_LEVEL_7,
	RTS_FTEST_LEVEL_8,
	RTS_FTEST_LEVEL_9,
	RTS_FTEST_LEVEL_RESERVED
};

typedef void* RtsFtestEnv;

struct rts_ftest_ops {
	int (*initialize)(RtsFtestEnv env, void **arg);
	int (*setup)(void *arg);
	int (*warmup)(void *arg);
	int (*preprocess)(void *arg);
	int (*runonce)(void *arg, unsigned int time);
	int (*postprocess)(void *arg);
	void (*cleanup)(void *arg);
};

struct rts_ftest_case {
	const char *name;
	const char *version;
	unsigned int priority;
	unsigned int frequency;
	unsigned int continuous;
	unsigned int sequence;
	struct rts_ftest_ops *ops;
};

int rts_ftest_initialize_env(RtsFtestEnv *env);
void rts_ftest_cleanup_env(RtsFtestEnv env);
int rts_ftest_add_test(RtsFtestEnv env, struct rts_ftest_case *test);
int rts_ftest_del_test(RtsFtestEnv env, const char *name);
int rts_ftest_run_tests(RtsFtestEnv env);
int rts_ftest_run_test(RtsFtestEnv env, const char *name);

int rts_ftest_set_burning_time(RtsFtestEnv env, const char *name,
		unsigned int burning_time);
int rts_ftest_set_frequence(RtsFtestEnv env, const char *name,
	       unsigned int frequency);
long rts_calc_timeval(struct timeval begin, struct timeval end);

#endif
