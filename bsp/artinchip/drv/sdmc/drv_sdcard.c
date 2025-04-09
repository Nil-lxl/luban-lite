/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#include "aic_core.h"

#ifdef AIC_SDMC1_USING_HOTPLUG

#include <drivers/mmcsd_core.h>
#include <dfs_fs.h>

#define HOTPLUG_SDMC 1
#define SD_CHECK_PIN (rt_pin_get(AIC_SDMC1_HOTPLUG_PIN))
#define SD_DEVICE    "sd0"

extern void aic_mmcsd_change(u8 id);

static void sd_hotplug_detection_thread(void *parameter)
{
    rt_uint8_t re_sd_check_pin = 1;
    rt_device_t device;

    rt_thread_mdelay(500);

    while (1)
    {
        rt_thread_mdelay(200);
        if (re_sd_check_pin && (re_sd_check_pin = rt_pin_read(SD_CHECK_PIN)) == 0) {
            printf("card insertion detected!\n");
            device = rt_device_find(SD_DEVICE);
            if (device == NULL)
                aic_mmcsd_change(HOTPLUG_SDMC);
        }
        if (!re_sd_check_pin && (re_sd_check_pin = rt_pin_read(SD_CHECK_PIN)) != 0) {
            printf("card removal detected!\n");
            aic_mmcsd_change(HOTPLUG_SDMC);
        }
    }
}

int aic_sd_hotplug_detection(void)
{
    rt_thread_t tid;

    rt_pin_mode(SD_CHECK_PIN, PIN_MODE_INPUT_PULLUP);

    tid = rt_thread_create("sd_hotplug_detection", sd_hotplug_detection_thread, RT_NULL,
                           2048, RT_THREAD_PRIORITY_MAX - 2, 20);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    else
        printf("create sd_hotplug_detection thread err!\n");

    return RT_EOK;
}
INIT_LATE_APP_EXPORT(aic_sd_hotplug_detection);
#endif

