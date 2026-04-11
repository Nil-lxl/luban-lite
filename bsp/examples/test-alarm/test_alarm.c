/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <finsh.h>
#include <drivers/rtc.h>
#include <drivers/alarm.h>

#include "aic_core.h"

static struct rt_alarm *g_alarm = RT_NULL;
static aicos_sem_t g_alarm_sem = NULL;

static void test_alarm_callback(rt_alarm_t alarm, time_t timestamp)
{
    if (g_alarm_sem)
        aicos_sem_give(g_alarm_sem);
}

static int cmd_test_alarm(int argc, char **argv)
{
    struct rt_alarm_setup setup = {0};
    u64 start_ms = 0;
    u32 timeout = 0, actual = 0;
    time_t now;
    struct tm p_tm;

    if (argc != 2) {
        pr_err("Invalid parameter\n");
        return -1;
    }
    timeout = atoi(argv[1]);

    if (!g_alarm_sem) {
        g_alarm_sem = aicos_sem_create(0);
        if (!g_alarm_sem) {
            pr_err("Failed to create alarm sem\n");
            return -1;
        }
    }

    if (g_alarm)
        rt_alarm_delete(g_alarm);

    now = time(NULL) + timeout;
    gmtime_r(&now, &p_tm);

    start_ms = aic_get_time_ms();
    setup.wktime = p_tm;
    g_alarm = rt_alarm_create(test_alarm_callback, &setup);
    if (!g_alarm) {
        pr_err("Failed to create alarm [%d sec]\n", timeout);
        return -1;
    }

    g_alarm->flag = RT_ALARM_ONESHOT;
    rt_alarm_start(g_alarm);

    printf("Wait for the alarm timeout ...\n");
    if (aicos_sem_take(g_alarm_sem, (timeout + 2) * 1000) < 0) {
        pr_err("Timeout\n");
        return -1;
    }

    actual = aic_get_time_ms() - start_ms;
    printf("The actual alarm timeout: %d ms\n", actual);
    if (actual <= (timeout - 1) * 1000 || actual >= (timeout + 1) * 1000) {
        pr_err("Alarm timeout is actually %d ms, expect %d s\n", actual, timeout);
        return -1;
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_test_alarm, test_alarm, test RTC alarm);
