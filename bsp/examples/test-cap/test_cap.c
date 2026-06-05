/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>
#include "boot_param.h"
#include "aic_core.h"

#define WATER_MARK      50

struct aic_cap_usr {
    rt_uint8_t id;
    rt_uint32_t freq;
    float duty;
};

static unsigned long g_start_us[AIC_CAPS_CH_NUM] = {0};
static unsigned long g_test_time_us[AIC_CAPS_CH_NUM] = {0};

/* callback function */
static rt_err_t cap_cb(rt_device_t dev, rt_size_t size)
{
    struct rt_inputcapture_data inputcap_data[WATER_MARK];
    rt_device_read(dev, 0, (void *)inputcap_data, size);
#ifdef ULOG_USING_ISR_LOG
    struct aic_cap_usr *data = (struct aic_cap_usr *)dev->user_data;

    rt_kprintf("cap%d: freq:%dHz, duty:%d.%02d%%\n",
        data->id, data->freq, (rt_uint32_t)data->duty, (rt_uint32_t)(data->duty * 100) % 100);

    for (int i = 0; i < size; i++)
        rt_kprintf("%s: pulsewidth:%d us\n", &dev->parent.name, inputcap_data[i].pulsewidth_us);
#endif

    if ((aic_timer_get_us() - g_start_us[data->id]) > g_test_time_us[data->id]) {
        rt_device_close(dev);
#ifdef ULOG_USING_ISR_LOG
        rt_kprintf("the timeout period has expired, cap%d close\n", data->id);
#endif
#ifdef RT_USING_PM
        rt_pm_release(PM_SLEEP_MODE_NONE);
#endif
    }

    return RT_EOK;
}

int test_cap(int argc, char **argv)
{
    rt_uint32_t watermark = WATER_MARK;
    rt_device_t cap_dev = RT_NULL;
    char device_name[8] = {"cap"};
    int ret;
    int ch;

    if (argc < 2) {
        rt_kprintf("Usage: test_cap channel [second]\n");
        return -RT_ERROR;
    }

    ch = atoi(argv[1]);

    if (argc == 3)
        g_test_time_us[ch] = atoi(argv[2]) * US_PER_SEC;
    else
        g_test_time_us[ch] = 10 * US_PER_SEC;//default 10s

    strcat(device_name, argv[1]);

    cap_dev =  rt_device_find(device_name);
    if (cap_dev == RT_NULL) {
        rt_kprintf("Can't find %s device!\n", device_name);
        return -RT_ERROR;
    }

    /* set callback function */
    rt_device_set_rx_indicate(cap_dev, cap_cb);

#ifdef RT_USING_PM
    rt_pm_request(PM_SLEEP_MODE_NONE);
#endif
    ret = rt_device_open(cap_dev, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK) {
        rt_kprintf("Failed to open %s device!\n", device_name);
        return ret;
    }

    ret = rt_device_control(cap_dev, INPUTCAPTURE_CMD_SET_WATERMARK, &watermark);
    if (ret != RT_EOK) {
        rt_kprintf("Failed to set %s device watermark!\n", device_name);
        return ret;
    }

    rt_kprintf("cap%d open.\n", ch);

    g_start_us[ch] = aic_timer_get_us();

    return RT_EOK;
}
MSH_CMD_EXPORT_ALIAS(test_cap, test_cap, Test the cap);
