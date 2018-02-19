#include <stdio.h>
#include <stdlib.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(void)
{
	int fd, retval;
	unsigned long data;
	int times = 10;

	/* 2011.1.1 12:00:00 Saturday */
	struct rtc_time rtc_tm = {
		.tm_wday = 6,
		.tm_year = 113,
		.tm_mon = 0,
		.tm_mday = 1,
		.tm_hour = 12,
		.tm_min = 0,
		.tm_sec = 0,
	};

	fd = open("/dev/rtc0", O_RDONLY);
	if (fd == -1) {
		printf("/dev/rtc0 open error");
		exit(1);
	}

	fprintf(stderr, "set RTC date/time is %d-%d-%d,%02d:%02d:%02d.\n",
		rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
		rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

	retval = ioctl(fd, RTC_SET_TIME, &rtc_tm);
	if (retval == -1) {
		printf("RTC_SET_TIME error");
		exit(1);
	}

	while (times--) {
		sleep(5);

		/* Read the RTC time/date */
		retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
		if (retval == -1) {
			printf("RTC_RD_TIME error");
			exit(1);
		}

		fprintf(stderr, "Current RTC date/time is %d-%d-%d,%02d:%02d:%02d.\n",
			rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
			rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
	}
	/* Setting alarm time */
	rtc_tm.tm_sec += 5;

	/* setting */
	retval = ioctl(fd, RTC_ALM_SET, &rtc_tm);
	if (retval == -1) {
		printf("RTC_ALM_SET error");
		exit(1);
	}

	/* Read the current alarm settings */
	retval = ioctl(fd, RTC_ALM_READ, &rtc_tm);
	if (retval == -1) {
		printf("RTC_ALM_READ error");
		exit(1);
	}
	fprintf(stderr, "Alarm time now set to %02d:%02d:%02d.\n",
		rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

	/* Enable alarm interrupts after setting */
	retval = ioctl(fd, RTC_AIE_ON, 0);
	if (retval == -1) {
		printf("RTC_AIE_ON error");
		exit(1);
	}

	/* This blocks until the alarm ring causes an interrupt */
	retval = read(fd, &data, sizeof(unsigned long));
	if (retval == -1) {
		printf("read");
		exit(1);
	}

	fprintf(stderr, " okay. Alarm rang.\n");
}
