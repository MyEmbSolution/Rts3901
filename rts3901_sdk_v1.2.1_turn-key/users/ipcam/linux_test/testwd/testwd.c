#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/watchdog.h>

int fd;

/*
 * This function simply sends an IOCTL to the driver, which in turn ticks
 * the PC Watchdog card to reset its internal timer so it doesn't trigger
 * a computer reset.
 */
static void keep_alive(void)
{
	int dummy;

	ioctl(fd, WDIOC_KEEPALIVE, &dummy);
}

/*
 * The main program.  Run the program with "-d" to disable the card,
 * or "-e" to enable the card.
 */

static void term(int sig)
{
	close(fd);
	printf("Stopping watchdog ticks...\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	int flags;

	fd = open("/dev/watchdog", O_WRONLY);

	if (fd == -1) {
		printf("Watchdog device not enabled.\n");
		fflush(stdout);
		exit(-1);
	}

	if (argc > 1) {
		if (!strncasecmp(argv[1], "-d", 2)) {
			flags = WDIOS_DISABLECARD;
			ioctl(fd, WDIOC_SETOPTIONS, &flags);
			printf("Watchdog card disabled.\n");
			fflush(stdout);
			goto end;
		} else if (!strncasecmp(argv[1], "-e", 2)) {
			flags = WDIOS_ENABLECARD;
			ioctl(fd, WDIOC_SETOPTIONS, &flags);
			printf("Watchdog card enabled.\n");
			fflush(stdout);
			goto end;
		} else {
			printf("-d to disable, -e to enable.\n");
			printf("run by itself to tick the card.\n");
			fflush(stdout);
			goto end;
		}
	} else {
		printf("Watchdog Ticking Away!\n");
		fflush(stdout);
	}

	signal(SIGINT, term);

	while (1) {
		printf("kick dog!\n");
		keep_alive();
		sleep(1);
	}
end:
	close(fd);
	return 0;
}
