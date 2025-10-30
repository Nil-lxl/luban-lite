/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Weihui.Xu <weihui.xu@artinchip.com>
 */
#include <stdio.h>
#include <console.h>
#include <getopt.h>

#include "aic_common.h"
#include "aic_log.h"
#include "aic_core.h"
#include "drv_rtc.h"

#define RTC_FAIL    (-1)
#define RTC_SUCCESS 0

static int get_time()
{
    printf("local_time: %s", local_time());
    printf("timezone: UTC%c%d\n", '+', tz_get());

    return RTC_SUCCESS;
}

static time_t set_time(char **argv)
{
    /* set time and date */
    struct tm tm_new = {0};
    time_t time_sec;

    tm_new.tm_year = strtoul(argv[1], NULL , 10) - 1900;
    tm_new.tm_mon = strtoul(argv[2], NULL , 10) - 1; /* .tm_mon's range is [0-11] */
    tm_new.tm_mday = strtoul(argv[3], NULL , 10);
    tm_new.tm_hour = strtoul(argv[4], NULL , 10);
    tm_new.tm_min = strtoul(argv[5], NULL , 10);
    tm_new.tm_sec = strtoul(argv[6], NULL , 10);

    if (tm_new.tm_year <= 0) {
        printf("year is out of range [1900-]\n");
        return RTC_FAIL;
    }
    if (tm_new.tm_mon > 11) { /* .tm_mon's range is [0-11] */
        printf("month is out of range [1-12]\n");
        return RTC_FAIL;
    }
    if (tm_new.tm_mday == 0 || tm_new.tm_mday > 31) {
        printf("day is out of range [1-31]\n");
        return RTC_FAIL;
    }
    if (tm_new.tm_hour > 23) {
        printf("hour is out of range [0-23]\n");
        return RTC_FAIL;
    }
    if (tm_new.tm_min > 59) {
        printf("minute is out of range [0-59]\n");
        return RTC_FAIL;
    }
    if (tm_new.tm_sec > 60) {
        printf("second is out of range [0-60]\n");
        return RTC_FAIL;
    }

    printf("old: %s", local_time());
    time_sec = timegm(&tm_new);

    return time_sec;
}

static int date(int argc, char **argv)
{
    time_t time_sec;

    if (argc == 1) {
        get_time();
    } else if (argc >= 7) {
        time_sec = set_time(argv);
        if(time_sec < 0) {
            printf("set time fail.\n");
            return RTC_FAIL;
        }
        rtc_set_secs(&time_sec);
    } else {
        printf("please input: date [year month day hour min sec] or date\n");
        printf("e.g: date 2018 01 01 23 59 59 or date\n");
    }

    return 0;
}

CONSOLE_CMD(date, date, "RTC test");
