#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define READERS 100
#define WRITERS 100

static void *read_test(void *data)
{
	char buf[100];
	int fd, good = 0, bad = 0;

	fd = open("/dev/null", O_WRONLY);

	while (1) {
		if ((read(fd, buf, sizeof(buf)) != -1) || (errno != EBADF)) {
			int err = errno;
			fprintf(stderr, "%d: (%d/%d) got %d instead of %d\n",
				pthread_self(), bad, bad+good, err, EBADF);
			bad++;
		} else {
			good++;
		}

	}

	return 0;
}

static void *write_test(void *data)
{
	char buf[100];
	int fd, good = 0, bad = 0;

	fd = open("/dev/full", O_WRONLY);

	while (1) {
		if ((write(fd, buf, sizeof(buf)) != -1) || (errno != ENOSPC)) {
			int err = errno;
			fprintf(stderr, "%d: (%d/%d) got %d instead of %d\n",
				pthread_self(), bad, bad+good, err, ENOSPC);
			bad++;
		} else {
			good++;

		}
	}

	return 0;
}

int main(void)
{
	pthread_t id;
	int i;

	for (i=0; i<READERS; i++)
		pthread_create(&id, 0, read_test, 0);

	for (i=0; i<(WRITERS-1); i++)
		pthread_create(&id, 0, write_test, 0);

	write_test(0);

	return 0;
}
