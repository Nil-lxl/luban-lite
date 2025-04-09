/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo.Dong <guojun.dong@artinchip.com>
 */
#include <finsh.h>
#include <drivers/rtc.h>
#include "aic_core.h"

/*
 * RTC set time test, if want to get time use "date" common
 * The specific code is in kernel/rt-thread/components/drivers/rtc/rtc.c
 */
static rt_err_t cmd_test_rtc(int argc, char **argv)
{
    rt_err_t ret = RT_EOK;
    time_t now;
    struct tm *local_time;
    int year, month, day, hour, min, sec;

    if (argc != 7) {
        rt_kprintf("Usage: test_rtc YYYY MM DD HH MM SS]\n");
        rt_kprintf("sample:\n");
        rt_kprintf("       test_rtc 2024 12 25 20 30 00\n");
        return -RT_ERROR;
    } else {
        year = atoi(argv[1]);
        month = atoi(argv[2]);
        day = atoi(argv[3]);
        hour = atoi(argv[4]);
        min = atoi(argv[5]);
        sec = atoi(argv[6]);
    }

    // set date with local timezone
    ret = set_date(year, month, day);
    if (ret != RT_EOK)
    {
        rt_kprintf("set RTC date failed");
        return ret;
    }
    // delay 1 ms for rtc sync
    rt_thread_mdelay(1);

    // set time with local timezone
    ret = set_time(hour, min, sec);
    if (ret != RT_EOK)
    {
        rt_kprintf("set RTC time failed");
        return ret;
    }

    // delay 1 ms for rtc sync
    rt_thread_mdelay(1);

    now = time(RT_NULL);
    local_time = localtime(&now);
    rt_kprintf("date: %04d-%02d-%02d\n", local_time->tm_year+1900, local_time->tm_mon+1, local_time->tm_mday);
    rt_kprintf("time: %02d:%02d:%02d\n", local_time->tm_hour, local_time->tm_min, local_time->tm_sec);

    return ret;
}
MSH_CMD_EXPORT_ALIAS(cmd_test_rtc, test_rtc, test RTC);
