/*
 * Realtek Semiconductor Corp.
 *
 * tests/ft2_test_rtc.c
 *
 * Copyright (C) 2014      Peter Sun<peter_sun@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "protocol.h"
#include "ft2errno.h"
#include "ft2log.h"

static const char default_rtc[] = "/dev/rtc0";

static int readrtctime(struct rtc_time *rtc_tm)
{
	int fd = 0, retval;
	const char *rtc = default_rtc;

	fd = open(rtc, O_RDONLY);
	if (fd == -1) {
		retval = -1;
		goto failed;
	}

	/* Read the RTC time/date */
	retval = ioctl(fd, RTC_RD_TIME, rtc_tm);
	if (retval == -1) {
		FT2_LOG_ERR("RTC_RD_TIME ioctl");
		goto failed;
	}

	FT2_LOG_INFO("Current RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",
		     rtc_tm->tm_mday, rtc_tm->tm_mon + 1,
		     rtc_tm->tm_year + 1900, rtc_tm->tm_hour, rtc_tm->tm_min,
		     rtc_tm->tm_sec);
failed:
	if (fd > 0)
		close(fd);

	return retval;
}

unsigned long
rtc2time(const unsigned int year0, const unsigned int mon0,
       const unsigned int day, const unsigned int hour,
       const unsigned int min, const unsigned int sec)
{
	unsigned int mon = mon0, year = year0;

	/* 1..12 -> 11,12,1..10 */
	if (0 >= (int) (mon -= 2)) {
		mon += 12;	/* Puts Feb last since it has leap day */
		year -= 1;
	}

	return ((((unsigned long)
		  (year/4 - year/100 + year/400 + 367*mon/12 + day) +
		  year*365 - 719499
	    )*24 + hour /* now have hours */
	  )*60 + min /* now have minutes */
	)*60 + sec; /* finally seconds */
}

static int rtc_tm_to_time(struct rtc_time *tm, unsigned long *time)
{
	*time = rtc2time(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	return 0;
}

int32_t ft2_rtc_check(struct protocol_command *pcmd)
{
	if (!pcmd)
		return -FT2_NULL_POINT;

	return FT2_OK;
}

int32_t ft2_rtc_runonce(void *priv, struct protocol_command *pcmd,
			int8_t *path)
{
	struct rtc_time rtc_tm;
	int32_t ret;
	unsigned long secs;
	uint8_t *timebuf;

	if (!pcmd)
		return -FT2_NULL_POINT;

	ret = readrtctime(&rtc_tm);
	if (ret < 0)
		goto failed;

	rtc_tm_to_time(&rtc_tm, &secs);

	timebuf = malloc(16);
	if (timebuf == 0)
		goto failed;

	memset(timebuf, 0, 16);
	snprintf(timebuf, 16, "%d", secs);
	append_pt_command_result(pcmd, timebuf, strlen(timebuf));
	free(timebuf);
	return FT2_OK;

failed:
	append_pt_command_result(pcmd, FT2_TEST_FAIL, strlen(FT2_TEST_FAIL));
	return -FT2_ERROR;
}
