/*
 * Realtek Semiconductor Corp.
 *
 * core/rtsftest.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <list.h>
#include <rtscamkit.h>
#include "rtsftest.h"

#ifndef RTS_FTEST_LEVEL_DEFAULT
#define RTS_FTEST_LEVEL_DEFAULT		RTS_FTEST_LEVEL_5
#endif

#ifndef RTS_FTEST_BURNTIME_DEFAULT
#define RTS_FTEST_BURNTIME_DEFAULT	10000
#endif

#ifndef RTS_FTEST_SEQ_DEFAULT
#define RTS_FTEST_SEQ_DEFAULT		1024
#endif

#ifndef RTS_FTEST_FREQ_DEFAULT
#define RTS_FTEST_FREQ_DEFAULT		1
#endif

#ifndef RTS_FTEST_VERSION_DFT
#define RTS_FTEST_VERSION_DFT		"1.0.1"
#endif

struct rts_ftest_case_ex {
	struct rts_ftest_case testcase;
	void *data;

	unsigned int left_times;
	unsigned int pass_times;
	unsigned int fail_times;
	unsigned int burning_time;

	struct list_head list;
};

struct rts_ftest_env {
	struct list_head testcases;
	struct rts_ftest_env *priv;
	unsigned int left_times;
};

static int check_env(struct rts_ftest_env *ftenv)
{
	if (!ftenv || ftenv->priv != ftenv)
		return -RTS_FTEST_ERROR;
	return RTS_FTEST_OK;
}

int rts_ftest_initialize_env(RtsFtestEnv *env)
{
	struct rts_ftest_env *ftenv;

	if (!env)
		return -RTS_FTEST_E_NULL_POINT;

	ftenv = (struct rts_ftest_env*)calloc(1, sizeof(*ftenv));
	if (!ftenv)
		return -RTS_FTEST_E_NO_MEMORY;

	INIT_LIST_HEAD(&ftenv->testcases);
	ftenv->priv = ftenv;

	*env = ftenv;

	return RTS_FTEST_OK;
}

void rts_ftest_cleanup_env(RtsFtestEnv env)
{
	struct rts_ftest_env *ftenv = env;
	struct rts_ftest_case_ex *ex, *tmp;

	if (check_env(ftenv))
		return;

	list_for_each_entry_safe(ex, tmp, &ftenv->testcases, list) {
		list_del_init(&ex->list);
		free(ex);
	}

	free(ftenv);
	ftenv = NULL;
}

struct rts_ftest_case_ex *find_testcase(struct rts_ftest_env *ftenv,
		const char *name)
{
	struct rts_ftest_case_ex *ex;

	if (!ftenv || !name || 0 == strlen(name))
		return NULL;

	list_for_each_entry(ex, &ftenv->testcases, list) {
		if (0 == strcmp(ex->testcase.name, name))
			return ex;
	}

	return NULL;
}

int rts_ftest_add_test(RtsFtestEnv env, struct rts_ftest_case *test)
{
	struct rts_ftest_env *ftenv = env;
	struct rts_ftest_case_ex *ex;

	if (check_env(ftenv))
		return -RTS_FTEST_E_NOT_INITIALIZED; 

	if (!test)
		return -RTS_FTEST_E_NULL_POINT;

	if (!test->name || 0 == strlen(test->name))
		return -RTS_FTEST_E_INVALID_ARG;

	if (!test->ops || !test->ops->runonce || !test->ops->initialize ||
			!test->ops->cleanup)
		return -RTS_FTEST_E_NOT_REALIZED;

	ex = find_testcase(ftenv, test->name);
	if (ex)
		return -RTS_FTEST_E_EXIST;

	ex = (struct rts_ftest_case_ex *)calloc(1, sizeof(*ex));
	if (!ex)
		return -RTS_FTEST_E_NO_MEMORY;

	memcpy(&ex->testcase, test, sizeof(*test));
	ex->burning_time = RTS_FTEST_BURNTIME_DEFAULT;
	if (!ex->testcase.version)
		ex->testcase.version = RTS_FTEST_VERSION_DFT;
	if (ex->testcase.priority <= RTS_FTEST_LEVEL_UNKOWN ||
			ex->testcase.priority >= RTS_FTEST_LEVEL_RESERVED)
		ex->testcase.priority = RTS_FTEST_LEVEL_DEFAULT;
	if (0 == ex->testcase.sequence)
		ex->testcase.sequence = RTS_FTEST_SEQ_DEFAULT;
	if (0 == ex->testcase.frequency)
		ex->testcase.frequency = RTS_FTEST_FREQ_DEFAULT;

	list_add_tail(&ex->list, &ftenv->testcases);

	return RTS_FTEST_OK;
}

int rts_ftest_del_test(RtsFtestEnv env, const char *name)
{
	struct rts_ftest_env *ftenv = env;
	struct rts_ftest_case_ex *ex = find_testcase(ftenv, name);

	if (ex) {
		list_del_init(&ex->list);
		free(ex);
	}

	return RTS_FTEST_OK;
}

static void rts_ftest_sort(struct rts_ftest_env *ftenv)
{
	struct rts_ftest_case_ex *ex;
	struct rts_ftest_case_ex *threshold = NULL;
	struct rts_ftest_case_ex *min = NULL;
	int move = 0;

	do {
		move = 0;
		min = NULL;
		list_for_each_entry(ex, &ftenv->testcases, list) {
			if (ex == threshold)
				break;
			if (!min) {
				min = ex;
				continue;
			}
			if (min->testcase.sequence > ex->testcase.sequence)
				min = ex;
		}
		if (min) {
			move = 1;
			list_del_init(&min->list);
			list_add_tail(&min->list, &ftenv->testcases);
			if (!threshold)
				threshold = min;
		}
	} while (move);
}

static void rts_ftest_update_times(struct rts_ftest_env *ftenv, int reset)
{
	struct rts_ftest_case_ex *ex;

	ftenv->left_times = 0;

	list_for_each_entry(ex, &ftenv->testcases, list) {
		if (reset) {
			ex->left_times = ex->testcase.frequency;
			ex->pass_times = 0;
			ex->fail_times = 0;
		}
		ftenv->left_times += ex->left_times;
	}
}

static int rts_ftest_run_testcase(struct rts_ftest_env *ftenv,
		struct rts_ftest_case_ex *ex, unsigned int continuous)
{
	int ret;
	struct rts_ftest_ops *tops = ex->testcase.ops;

	if (ex->left_times == 0)
		return RTS_FTEST_OK;

	RTS_INFO("run testcase : %s\n", ex->testcase.name);
	RTS_INFO("++++++++%s Start++++++++\n", ex->testcase.name);
	RTS_INFO("%s Version is V%s\n",
			ex->testcase.name, ex->testcase.version);

	//RTS_INFO("initialize\n")
	ret = tops->initialize(ftenv, &ex->data);
	if (ret) {
		RTS_ERR("%s initialize fail\n", ex->testcase.name);
		if (continuous) {
			ex->fail_times += ex->left_times;
			ex->left_times = 0;
		} else {
			ex->fail_times++;
			ex->left_times--;
		}
		return ret;
	}

	if (tops->setup) {
		//RTS_INFO("setup\n")
		ret = tops->setup(ex->data);
		if (ret) {
			RTS_ERR("%s setup fail\n", ex->testcase.name);
			tops->cleanup(ex->data);
			if (continuous) {
				ex->fail_times += ex->left_times;
				ex->left_times = 0;
			} else {
				ex->fail_times++;
				ex->left_times--;
			}
			return ret;
		}
	}

	while (ex->left_times > 0) {
		RTS_INFO("%s <%d>\n", ex->testcase.name,
				ex->testcase.frequency - ex->left_times + 1);
		if (tops->warmup) {
			//RTS_INFO("warmup\n")
			ret = tops->warmup(ex->data);
			if (ret) {
				RTS_ERR("%s warmup fail\n", ex->testcase.name);
				goto next;
			}
		}
		if (tops->preprocess) {
			//RTS_INFO( "preprocess\n")
			ret = tops->preprocess(ex->data);
			if (ret) {
				RTS_ERR("%s preprocess fail\n",
						ex->testcase.name);
				goto next;
			}
		}

		//RTS_INFO( "runonce\n")
		ret = tops->runonce(ex->data, ex->burning_time);
		if (ret) {
			RTS_ERR("%s runonce fail\n", ex->testcase.name);
			goto next;
		}

		if (tops->postprocess) {
			//RTS_INFO( "postprocess\n")
			ret = tops->postprocess(ex->data);
			if (ret) {
				RTS_ERR("%s postprocess fail\n",
						ex->testcase.name);
				goto next;
			}
		}
next:
		if (ret) {
			RTS_ERR("------------------------\n");
			RTS_ERR("--------  FAIL  --------\n");
			RTS_ERR("------------------------\n");
			ex->fail_times++;
		} else {
			RTS_INFO("++++++++++++++++++++++++\n");
			RTS_INFO("++++++++  PASS  ++++++++\n");
			RTS_INFO("++++++++++++++++++++++++\n");
			ex->pass_times++;
		}
		ex->left_times--;
		if (!continuous)
			break;
	}

	//RTS_INFO( "cleanup\n")
	tops->cleanup(ex->data);

	RTS_INFO("++++++++%s End++++++++\n", ex->testcase.name);
	return ret;
}

static int rts_ftest_run_tests_one_loop(struct rts_ftest_env *ftenv)
{
	unsigned int priority = RTS_FTEST_LEVEL_1;
	struct rts_ftest_case_ex *ex;

	for (;priority < RTS_FTEST_LEVEL_RESERVED; priority++) {
		list_for_each_entry(ex, &ftenv->testcases, list) {
			if (priority != ex->testcase.priority)
				continue;
			rts_ftest_run_testcase(ftenv,
					ex, ex->testcase.continuous);
		}
	}

	return RTS_FTEST_OK;
}

int rts_ftest_run_tests(RtsFtestEnv env)
{
	struct rts_ftest_env *ftenv = env;
	struct rts_ftest_case_ex *ex;
	int result = RTS_FTEST_OK;

	if (check_env(ftenv))
		return -RTS_FTEST_E_NOT_INITIALIZED; 

	rts_ftest_sort(ftenv);
	rts_ftest_update_times(ftenv, 1);

	while (ftenv->left_times > 0) {
		rts_ftest_run_tests_one_loop(ftenv);
		rts_ftest_update_times(ftenv, 0);
	}

	list_for_each_entry(ex, &ftenv->testcases, list) {
		RTS_NOTICE("%s pass : %d ; fail : %d.\n", ex->testcase.name,
			ex->pass_times, ex->fail_times);
		if (ex->fail_times)
			result = - RTS_FTEST_ERROR;
	}

	return result;
}

int rts_ftest_run_test(RtsFtestEnv env, const char *name)
{
	struct rts_ftest_env *ftenv = env;
	struct rts_ftest_case_ex *ex;

	if (check_env(ftenv))
		return -RTS_FTEST_E_NOT_INITIALIZED; 

	ex = find_testcase(ftenv, name);
	if (!ex)
		return -RTS_FTEST_E_NOT_FOUND;

	ex->left_times = ex->testcase.frequency;
	ex->pass_times = 0;
	ex->fail_times = 0;

	rts_ftest_run_testcase(ftenv, ex, ex->testcase.continuous);

	if (ex->fail_times)
		return -RTS_FTEST_ERROR;
	return RTS_FTEST_OK;
}

int rts_ftest_set_burning_time(RtsFtestEnv env, const char *name,
		unsigned int burning_time)
{
	struct rts_ftest_env *ftenv = env;
	struct rts_ftest_case_ex *ex;

	if (check_env(ftenv))
		return -RTS_FTEST_E_NOT_INITIALIZED; 

	ex = find_testcase(ftenv, name);
	if (!ex)
		return -RTS_FTEST_E_NOT_FOUND;

	if (burning_time > 0)
		ex->burning_time = burning_time;

	return RTS_FTEST_OK;
}

int rts_ftest_set_frequence(RtsFtestEnv env, const char *name,
	       unsigned int frequency)
{
	struct rts_ftest_env *ftenv = env;
	struct rts_ftest_case_ex *ex;

	if (check_env(ftenv))
		return -RTS_FTEST_E_NOT_INITIALIZED; 

	ex = find_testcase(ftenv, name);
	if (!ex)
		return -RTS_FTEST_E_NOT_FOUND;

	if (frequency > 0)
		ex->testcase.frequency = frequency;

	return RTS_FTEST_OK;
}

long rts_calc_timeval(struct timeval begin, struct timeval end)
{
	long ms;

	ms = (end.tv_sec * 1000 + end.tv_usec / 1000) -
		(begin.tv_sec * 1000 + begin.tv_usec / 1000);

	return ms;
}

