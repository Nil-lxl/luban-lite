/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#include <stdio.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <aic_core.h>
#include <aic_drv.h>
#include <string.h>
#include <aic_osal.h>
#include <hal_rtc.h>

#define STRESS_WAKEUP_FLAG   (1 << 3)
#define STRESS_STOP_FLAG     (1 << 2)
volatile uint32_t stress_test_running = 0;
static volatile uint32_t wakeup_count = 0;
static struct rt_event stress_event;
struct stress_params {
    uint32_t loop_count;
    uint32_t sec;
};
static struct stress_params pm_stress;

#if defined(AIC_RTC_DRV_V121) && defined(AIC_PM_DRV_V15)
static rt_alarm_t stress_alarm = RT_NULL;

static void stress_alarm_callback(rt_alarm_t alarm, time_t timestamp)
{
    rt_event_send(&stress_event, STRESS_WAKEUP_FLAG);
}

static void stress_test_thread(void *parameter)
{
    struct stress_params *params = (struct stress_params *)parameter;
    struct rt_alarm_setup setup = {0};
    uint32_t loop_count = 0;
    uint32_t sec = 3;
    struct tm p_tm;
    time_t now;

    if (params != RT_NULL) {
        loop_count = params->loop_count;
        sec = params->sec;
    }

    rt_kprintf("loop_count:%d, alarm sec:%d\n", loop_count, sec);

    while (stress_test_running) {
        //1.keep active
        rt_thread_delay(sec * RT_TICK_PER_SECOND);

        //2.set the alarm sec
        now = time(NULL) + sec;
        gmtime_r(&now, &p_tm);
        setup.wktime = p_tm;

        if (stress_alarm) {
            rt_alarm_stop(stress_alarm);
            rt_alarm_delete(stress_alarm);
        }
        stress_alarm = rt_alarm_create(stress_alarm_callback, &setup);
        if (!stress_alarm) {
            rt_kprintf("Failed to create alarm\n");
            break;
        }
        stress_alarm->flag = RT_ALARM_ONESHOT;
        rt_alarm_start(stress_alarm);

        //3.request to deep sleep
        rt_kprintf("Stress test: entering deep sleep (wakeup count = %d)\n", wakeup_count);
        rt_pm_default_set(PM_SLEEP_MODE_DEEP);
        rt_pm_module_release(PM_POWER_ID, PM_SLEEP_MODE_NONE);

        //4.wake up event
        rt_uint32_t e;
        rt_event_recv(&stress_event, STRESS_WAKEUP_FLAG | STRESS_STOP_FLAG,
                      RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                      RT_WAITING_FOREVER, &e);
        if (e & STRESS_STOP_FLAG) {
            rt_kprintf("Stress test stopped by user.\n");
            break;
        }
        wakeup_count++;

        //5.exit
        if (loop_count && wakeup_count >= loop_count) {
            rt_kprintf("Stress test finished after %d cycles.\n", wakeup_count);
            break;
        }
    }

    //delete the resource for next time
    if (stress_alarm) {
        rt_alarm_stop(stress_alarm);
        rt_alarm_delete(stress_alarm);
        rt_event_detach(&stress_event);
        stress_alarm = RT_NULL;
    }
    stress_test_running = 0;
    rt_kprintf("Stress test thread exit.\n");
}
#else
static struct rt_lptimer stress_lptimer;

static void stress_lptimer_callback(void *parameter)
{
    rt_event_send(&stress_event, STRESS_WAKEUP_FLAG);
}

static void stress_test_thread(void *parameter)
{
    struct stress_params *params = (struct stress_params *)parameter;
    uint32_t loop_count = 0;
    uint32_t sec = 3;

    if (params != RT_NULL) {
        loop_count = params->loop_count;
        sec = params->sec;
        if (sec < 1)
            sec = 1;
    }

    rt_kprintf("loop_count:%d, lptimer sec:%d\n", loop_count, sec);

    rt_lptimer_init(&stress_lptimer, "stress_lpt", stress_lptimer_callback, RT_NULL,
                    sec * RT_TICK_PER_SECOND, RT_TIMER_FLAG_ONE_SHOT);

    while (stress_test_running) {
        //1.keep active: pm lock held, no unintended sleep
        rt_thread_delay((sec - 1) * RT_TICK_PER_SECOND);

        //2.start lptimer as wakeup source
        rt_lptimer_start(&stress_lptimer);

        //3.request to sleep
        rt_kprintf("Stress test: entering sleep (wakeup count = %d)\n", wakeup_count);
        rt_pm_module_release(PM_POWER_ID, PM_SLEEP_MODE_NONE);

        //4.wake up event
        rt_uint32_t e;
        rt_event_recv(&stress_event, STRESS_WAKEUP_FLAG | STRESS_STOP_FLAG,
                      RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                      RT_WAITING_FOREVER, &e);
        if (e & STRESS_STOP_FLAG) {
            rt_kprintf("Stress test stopped by user.\n");
            break;
        }
        wakeup_count++;
        rt_lptimer_stop(&stress_lptimer);

        //5.hold lock for next keep-active phase
        rt_pm_module_request(PM_POWER_ID, PM_SLEEP_MODE_NONE);

        if (loop_count && wakeup_count >= loop_count) {
            rt_kprintf("Stress test finished after %d cycles.\n", wakeup_count);
            break;
        }
    }

    rt_lptimer_detach(&stress_lptimer);
    rt_pm_module_release(PM_POWER_ID, PM_SLEEP_MODE_NONE);
    rt_event_detach(&stress_event);
    stress_test_running = 0;
    rt_kprintf("Stress test thread exit.\n");
}
#endif

int pm_stress_test(int argc, char **argv)
{
    struct stress_params *params = &pm_stress;
    uint32_t loop, sec;
    rt_thread_t tid;
    rt_err_t ret;

    if (argc != 3) {
        rt_kprintf("Usage: pm_stress <loop> <sec>\n");
        rt_kprintf("  loop: 0 for infinite, positive for finite cycles\n");
        rt_kprintf("  sec:  alarm interval in seconds\n");
        return -1;
    }

    loop = atoi(argv[1]);
    sec = atoi(argv[2]);

    if (stress_test_running) {
        rt_kprintf("Stress test already running.\n");
        return -1;
    }

    params->loop_count = loop;
    params->sec = sec;

#if defined(AIC_RTC_DRV_V121) && defined(AIC_PM_DRV_V15)
    rt_pm_default_set(PM_SLEEP_MODE_DEEP);
#endif

    ret = rt_event_init(&stress_event, "stress_ev", RT_IPC_FLAG_PRIO);
    if (ret != RT_EOK)
    {
        rt_kprintf("init stress_ev failed\n");
        return -1;
    }

    wakeup_count = 0;
    stress_test_running = 1;

    tid = rt_thread_create("stress", stress_test_thread,
                           (void *)params, 2048, 25, 10);
    if (tid) {
        rt_thread_startup(tid);
        rt_kprintf("Stress test started, loop = %s.\n", loop ? "finite" : "infinite");
        return 0;
    } else {
        stress_test_running = 0;
        rt_kprintf("Failed to create stress test thread.\n");
        return -1;
    }
}
MSH_CMD_EXPORT_ALIAS(pm_stress_test, pm_stress, start stress test with alarm);

int pm_stress_stop(void)
{
    if (!stress_test_running) {
        rt_kprintf("Stress test is not running.\n");
        return -1;
    }
    stress_test_running = 0;
    rt_event_send(&stress_event, STRESS_STOP_FLAG);
    rt_kprintf("Stopping stress test...\n");
    return 0;
}
MSH_CMD_EXPORT(pm_stress_stop, stop stress test);
