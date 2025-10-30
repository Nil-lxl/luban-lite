/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>

#define DBG_TAG  "tsen.cmd"

#include "aic_core.h"
#include "aic_log.h"
#include "sensor.h"

/* Global macro and variables */
#define AIC_TSEN_NAME                   "temp_tsen_cpu"
#define THREAD_PRIORITY                 25
#define THREAD_STACK_SIZE               2048
#define THREAD_TIMESLICE                50

static rt_device_t g_tsen_dev = RT_NULL;
static rt_sensor_t g_sensor;
static int g_samples_num = 10;
static rt_thread_t  g_tsen_thread = RT_NULL;

static void tsen_polling_mode(void *parameter)
{
    int num = 0;
    struct rt_sensor_data data;

    while (1) {
        for (num = 0; num < g_samples_num; num++) {
            if (rt_device_read(g_tsen_dev, 0, &data,1) == 1) {
                if (data.data.temp / 10 >= AIC_TSEN_HIGH_TEMP_ALARM_THD)
                    rt_kprintf("High temp alarm! temp:%3d.%d C\n",
                               data.data.temp / 10,
                               (rt_uint32_t)data.data.temp % 10);
            } else {
                rt_kprintf("read data failed!");
            }
        }
        rt_thread_mdelay(10000);
    }

}

int test_tsen_htp(void)
{
    g_sensor = (rt_sensor_t)g_tsen_dev;

    g_tsen_dev = rt_device_find(AIC_TSEN_NAME);
    if (g_tsen_dev == RT_NULL) {
        rt_kprintf("Failed to find %s device\n", AIC_TSEN_NAME);
        return -RT_ERROR;
    }

    rt_device_open(g_tsen_dev, RT_DEVICE_FLAG_RDONLY);

    g_tsen_thread = rt_thread_create("tsen_htp_polling", tsen_polling_mode,
                                     RT_NULL, THREAD_STACK_SIZE,
                                     THREAD_PRIORITY, THREAD_TIMESLICE);
    if (g_tsen_thread != RT_NULL)
        rt_thread_startup(g_tsen_thread);

    return RT_EOK;

}

INIT_APP_EXPORT(test_tsen_htp);
