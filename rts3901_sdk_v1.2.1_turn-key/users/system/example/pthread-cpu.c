/*
 * Realtek Semiconductor Corp.
 *
 * Tony Wu (tonywu@realtek.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <syscall.h>

int my_getcpu(void)
{
	int c, s, cpu;
	s = syscall(__NR_getcpu, &c, NULL, NULL);
	cpu = (s == -1) ? s : c;

	return cpu;
}

static void wait_thread(void)
{
	time_t start_time = time(NULL);

	while (time(NULL) == start_time) {
		// do nothing except chew CPU slices for up to one second.
	}
}

static void *thread_func(void *vptr_args)
{
	int i;
	int cpu;

	cpu = my_getcpu();
	for (i = 0; i < 20; i++) {
		printf("cpu%d: b\n", cpu);
		wait_thread();
	}

	return NULL;
}

int main(void)
{
	int i;
	int cpu;
	pthread_t thread;

	cpu = my_getcpu();
	if (pthread_create(&thread, NULL, thread_func, NULL) != 0) {
		return EXIT_FAILURE;
	}

	for (i = 0; i < 20; i++) {
		printf("cpu%d: a\n", cpu);
		wait_thread();
	}

	if (pthread_join(thread, NULL) != 0) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
