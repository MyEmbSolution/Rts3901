/*
 */

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>

void handleClient(void);
void handleThread(void);

int main(int argc, char **argv)
{
	pid_t pid;
	int status;

	pid = fork();
	if (pid == 0)
		handleClient();
	else
		waitpid(-1, &status, WNOHANG);
}

void handleClient(void)
{
	pid_t pid;
	int status;

	pid = fork();
	if (pid == 0)
		handleThread();
	else
		waitpid(-1, &status, WNOHANG);
}

static void *thread1_func(void *arg)
{
	int i;

	for (i = 0; i < 10; i++) {
		printf("Thread 1\n");
		sleep(1);
	}
	return 0;
}

static void *thread2_func(void *arg)
{
	int i;

	for (i = 0; i < 10; i++) {
		printf("Thread 2\n");
		sleep(1);
	}
	return 0;
}

void  handleThread(void)
{
	pthread_t thread1;
	pthread_t thread2;

	printf("Creating Thread 1\n");
	if (pthread_create(&thread1, NULL, thread1_func, NULL)) {
		abort();
	}

	printf("Creating Thread 2\n");
	if (pthread_create(&thread2, NULL, thread2_func, NULL)) {
		abort();
	}

	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
}
