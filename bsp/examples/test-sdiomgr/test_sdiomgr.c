/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#include <rtthread.h>
#include <rtconfig.h>
#include <rtdevice.h>

static rt_err_t wait_for_sdio_ready(rt_int32_t timeout_ms)
{
    rt_device_t sdio_mgr;

    sdio_mgr = rt_device_find("sdio_mgr");
    if (sdio_mgr == RT_NULL) {
        rt_kprintf("SDIO manager device not found\n");
        return -RT_ERROR;
    }

    if (rt_device_open(sdio_mgr, RT_DEVICE_OFLAG_RDWR) != RT_EOK) {
        return -RT_ERROR;
    }

    struct rt_sdio_mgr_wait_params w_params;
    rt_uint32_t recv_events = 0;

    w_params.wait_events = SDIO_EVENT_SDIO_DEVICE_READY;
    w_params.wait_mode = WAIT_MODE_ALL;
    w_params.timeout_ticks = rt_tick_from_millisecond(timeout_ms);
    w_params.recv_events = &recv_events;

    rt_kprintf("[APP] Waiting for events:0x%08x initialization...\n", w_params.wait_events);

    rt_err_t result = rt_device_control(sdio_mgr, SDIO_MGR_CMD_WAIT_READY, &w_params);
    if (result == RT_EOK) {
        rt_kprintf("[APP] SDIO ready!\n");
    } else if (result == -RT_ETIMEOUT) {
        rt_kprintf("[APP] Timeout waiting for SDIO\n");
    } else {
        rt_kprintf("[APP] Error waiting for SDIO: %d\n", result);
    }

    rt_device_close(sdio_mgr);
    return result;
}

static void test_thread_entry(void * parameter)
{
    if (wait_for_sdio_ready(1000) == RT_EOK) {
        rt_wlan_set_mode("wlan0", RT_WLAN_STATION);
    } else {
        rt_kprintf("[APP] Cannot start SDIO operations\n");
    }
}
static int create_test_thread(void)
{
    rt_thread_t test_thread = rt_thread_create("test_thread", test_thread_entry, RT_NULL, 4096, 20, 10);

    if (test_thread != RT_NULL) {
        rt_thread_startup(test_thread);
        return 0;
    }
    return -1;
}
INIT_LATE_APP_EXPORT(create_test_thread);

