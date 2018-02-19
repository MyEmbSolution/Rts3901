/*
 * Realtek Semiconductor Corp.
 *
 * core/tester.c
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#include "tester.h"

/* Fix Me:
 * to join these parameters into a structure
 */
unsigned long pic_count = 0;
unsigned long h264_count = 0;
unsigned long jpeg_count = 0;
unsigned long aud_amic_44k_count=0;
unsigned long aud_i2s_44k_count=0;
unsigned long aud_amic_48k_count=0;
unsigned long aud_i2s_48k_count=0;

#ifdef FT2_TEST_LOOP
unsigned char test_forever=1;
#else
unsigned char test_forever=0;
#endif

static int find_test_item(struct ft2_tester *tester, int id)
{
	struct test_entry *item;

	if (!tester)
		return 0;

	list_for_each_entry(item, &tester->test_items, list) {
		if (id == item->test.test_id)
			return 1;
	}
	return 0;
}

static int add_test_item(struct ft2_tester *tester, struct ft2_test_item *item)
{
	struct test_entry *entry = NULL;

	if (!tester || !item)
		return -FT2_NULL_POINT;

	entry = (struct test_entry*)calloc(1, sizeof(*entry));
	if (!entry)
		return -FT2_NO_MEMORY;

	memcpy(&entry->test, item, sizeof(struct ft2_test_item));
	entry->owner = tester;

	list_add_tail(&entry->list, &tester->test_items);

	return FT2_OK;
}

int ft2_register_test_item(FT2TesterInst inst,
		struct ft2_test_item *test_items, int count)
{
	int i = 0;
	int ret = FT2_OK;

	if (!inst || !test_items || !count)
		return FT2_NULL_POINT;

	for (i = 0; i < count; i++) {
		struct ft2_test_item *item = test_items + i;
		if (find_test_item(inst, item->test_id))
				continue;
		if (!item->runonce) {
			FT2_LOG_WARNING("test [%s] is not realized\n",
					item->test_name);
			continue;
		}

		ret = add_test_item(inst, item);
		if (ret)
			FT2_LOG_ERR("add test [%s] fail, ret = %d\n",
					item->test_name, ret);
	}

	return FT2_OK;
}


int ft2_set_root_path(FT2TesterInst inst, char *path)
{
	if (!inst || !path)
		return -FT2_NULL_POINT;

	snprintf(inst->root_path, sizeof(inst->root_path), "%s", path);

	return FT2_OK;
}

int ft2_get_root_path(FT2TesterInst inst, char *path, int len)
{
	if (!inst || !path)
		return -FT2_NULL_POINT;

	snprintf(path, len, "%s", inst->root_path);
	return FT2_OK;
}

static void increase_running_test(struct ft2_tester *tester) {
	if (!tester)
		return;
	pthread_mutex_lock(&tester->mutex_test);
	tester->running_test++;
	pthread_mutex_unlock(&tester->mutex_test);
}

static void reduce_running_test(struct ft2_tester *tester) {
	if (!tester)
		return;
	pthread_mutex_lock(&tester->mutex_test);
	tester->running_test--;
	pthread_mutex_unlock(&tester->mutex_test);
}

void* test_func(void *data)
{
	struct test_entry *item = (struct test_entry *)data;
	struct ft2_tester *tester = NULL;
	int ret = FT2_OK;

	if (!data) {
		FT2_LOG_ERR("data is NULL, test func return\n");
		return NULL;
	}

	tester = item->owner;

	if (item->test.initialize)
			item->priv = item->test.initialize();

	if (item->test.preprocess) {
		ret = item->test.preprocess(item->priv);
		if (ret) {
			FT2_LOG_ERR("test [%s] preprocess fail\n", item->test.test_name);
			append_pt_command_result(item->pcmd,
					FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
			goto exit;
		}
	}

	pthread_mutex_lock(&tester->mutex_test);
	tester->stop_all_item = 0;
	pthread_mutex_unlock(&tester->mutex_test);
	if (item->test.test_id == 0x00)
		goto exit;

	do {
		if (item->test.runonce) {
			ret = item->test.runonce(item->priv, item->pcmd, tester->root_path);
			if (ret) {
				FT2_LOG_INFO("test [%s] run fail\n", item->test.test_name);
				goto exit;
			}
		}

		send_commond_result(tester->protocol, item->pcmd);
		delete_pt_command_result(item->pcmd);
		item->pcmd->result = NULL;

		if (tester->stop_all_item)
			goto exit;

		if (test_forever && item->test.test_id > 4)
			 usleep(50000);
	} while (test_forever);

exit:
	pthread_mutex_lock(&tester->mutex_test);
	tester->stop_all_item = 1;
	pthread_mutex_unlock(&tester->mutex_test);

	if (item->test.postprocess) {
		ret = item->test.postprocess(item->priv);
		if (ret) {
			FT2_LOG_ERR("test [%s] postprocess fail\n", item->test.test_name);
			goto exit;
		}
	}

	if (item->test.cleanup)
		item->test.cleanup(item->priv);
	item->priv = NULL;
	delete_pt_command(item->pcmd);
	item->pcmd = NULL;
	free(data);
	pthread_detach(pthread_self());
	reduce_running_test(tester);
	return NULL;
}

static struct test_entry *find_test_entry(struct ft2_tester *tester,char *name)
{
	struct test_entry *item;

	if (!tester)
		return NULL;

	list_for_each_entry(item, &tester->test_items, list) {
		if (0 == strcmp(item->test.test_name, name)) {
			struct test_entry *entry = (struct test_entry *)calloc(1, sizeof(struct test_entry));
			if (entry)
				memcpy(entry, item, sizeof(struct test_entry));
			return entry;
		}
	}

	return NULL;
}
void *run_tests(void *data)
{
	struct ft2_tester *tester = (struct ft2_tester*)data;
	struct test_entry *entry;
	int ret = 0;
	int i = 0;

	if (!data)
		return NULL;

	while (!tester->exit_flag) {
		struct protocol_command *pcmd = pop_commond(tester->protocol);
		if (!pcmd) {
			usleep(1000);
			continue;
		}
		entry = find_test_entry(tester, pcmd->name);
		if (entry) {
			entry->owner = tester;
			entry->pcmd = pcmd;
			ret = pthread_create(&entry->th, NULL,
					test_func, (void*)entry);
			if (ret) {
				FT2_LOG_ERR("start test func for [%s] fail\n",
						pcmd->name);
				free(entry);
			} else
				increase_running_test(tester);
			entry = NULL;
		} else {
			FT2_LOG_WARNING("there is no test func for pcmd [%s]\n",
					pcmd->name);
		}
	}

	/*
	 * main test thread waiting for all test items to finish
	 */
	while (tester->running_test && i < 1000) {
		i++;
		if (i % 10 == 0)
			FT2_LOG_INFO("waiting test finish...\n");
		usleep(200000);
	}

	return NULL;
}

int ft2_start_run_tests(FT2TesterInst inst)
{
	struct ft2_tester *tester = inst;
	int ret = 0;

	if (!inst)
		return -FT2_NULL_POINT;

	tester->exit_flag = 0;
	tester->running_test = 0;
	tester->th_run = 0;
	ret = pthread_create(&tester->th_run, NULL, run_tests, (void*)tester);
	if (ret) {
		FT2_LOG_ERR("create run tests thread fail, ret = %d\n", ret);
		return -FT2_THREAD_FAIL;
	}
	return FT2_OK;
}

int ft2_stop_run_tests(FT2TesterInst inst)
{
	struct ft2_tester *tester = inst;

	if (!inst)
		return -FT2_NULL_POINT;

	tester->exit_flag = 1;
	if (tester->th_run)
		pthread_join(tester->th_run, NULL);
	tester->th_run = 0;
	return FT2_OK;
}

int ft2_check_command(void *checker, struct protocol_command *pcmd)
{
	struct ft2_tester *tester = checker;
	struct test_entry *item;

	if (!checker || !pcmd)
		return -FT2_NULL_POINT;

	list_for_each_entry(item, &tester->test_items, list) {
		if (0 == strncmp(pcmd->name, item->test.test_name,
					FT2_COMMAND_NAME_LENGTH)) {
			if (!item->test.check)
				return -FT2_NOT_REALIZED;
			else
				return item->test.check(pcmd);
		}
	}

	return -FT2_NOT_FOUND;
}

int clear_test_items(struct ft2_tester *tester)
{
	struct test_entry *item, *tmp;

	if (!tester)
		return FT2_OK;

	list_for_each_entry_safe(item, tmp, &tester->test_items, list) {
		list_del(&item->list);
		free(item);
	}
}

int ft2_new_tester(FT2TesterInst *inst)
{
	struct ft2_tester *tester = NULL;
	int ret = FT2_OK;

	if (!inst) {
		FT2_LOG_ERR("NULL Point, invalid argument\n");
		return -FT2_NULL_POINT;
	}

	tester = (struct ft2_tester*)calloc(1, sizeof(*tester));
	if (!tester) {
		FT2_LOG_ERR("malloc protocol fail\n");
		return -FT2_NO_MEMORY;
	}

	INIT_LIST_HEAD(&tester->test_items);

	ret = ft2_new_protocol_inst(&tester->protocol,
			(void*)tester, ft2_check_command);
	if (ret) {
		FT2_LOG_ERR("create protocol instance fail\n");
		goto error;
	}
	tester->exit_flag = 0;
	tester->th_run = 0;
	pthread_mutex_init(&tester->mutex_test, NULL);

	*inst = tester;
	return FT2_OK;

error:
	if (tester->protocol)
		ft2_delete_protocol_inst(tester->protocol);
	tester->protocol = NULL;
	if (tester)
		free(tester);
	tester = NULL;
	return ret;
}

int ft2_delete_tester(FT2TesterInst inst)
{
	struct ft2_tester *tester = inst;

	if (!inst)
		return FT2_OK;

	clear_test_items(tester);

	if (tester->protocol)
		ft2_delete_protocol_inst(tester->protocol);
	tester->protocol = NULL;

	pthread_mutex_destroy(&tester->mutex_test);
	free(tester);
	return FT2_OK;
}

