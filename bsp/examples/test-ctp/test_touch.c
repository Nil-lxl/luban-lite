/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date                Notes
 * 2024-08-14          the first version
 */

#include <rtthread.h>
#include <rtdevice.h>

#define THREAD_PRIORITY         25
#define THREAD_STACK_SIZE       4096
#define THREAD_TIMESLICE        5
#define TOUCH_DEFAULT_NAME      "gt911"

static rt_sem_t     g_touch_sem = RT_NULL;
static rt_device_t  g_dev = RT_NULL;
static struct rt_touch_data *g_read_data = RT_NULL;
static struct rt_touch_info g_info = {0};

static void cmd_touch_test_help(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("\ttest_touch <sensor_name>\n");
    rt_kprintf("\tFor example:\n");
    rt_kprintf("\t\ttest_touch\n");
    rt_kprintf("\t\ttest_touch gt911\n");
}

static void touch_entry(void *parameter)
{
    rt_device_control(g_dev, RT_TOUCH_CTRL_GET_INFO, &g_info);

    g_read_data = (struct rt_touch_data *)rt_malloc(sizeof(struct rt_touch_data) * g_info.point_num);

    while (1) {
        rt_sem_take(g_touch_sem, RT_WAITING_FOREVER);

        if (rt_device_read(g_dev, 0, g_read_data, g_info.point_num) > 0) {
            for (rt_uint8_t i = 0; i < g_info.point_num; i++) {
                if (g_read_data[i].event == RT_TOUCH_EVENT_DOWN ||
                    g_read_data[i].event == RT_TOUCH_EVENT_MOVE ||
                    g_read_data[i].event == RT_TOUCH_EVENT_UP) {
                    rt_kprintf("%d %d %d %d\n", g_read_data[i].track_id,
                               g_read_data[i].x_coordinate,
                               g_read_data[i].y_coordinate,
                               g_read_data[i].event);
                }
            }
        }
        rt_device_control(g_dev, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);

        /*
         * If using polling mode,
         * the following conmments must be opened,
         * and the masking related to semaphores must be removed.
         */
        //rt_thread_mdelay(10);
    }
}

static rt_err_t rx_callback(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(g_touch_sem);
    rt_device_control(dev, RT_TOUCH_CTRL_DISABLE_INT, RT_NULL);
    return 0;
}

static void test_touch(int argc, char *argv[])
{
    rt_thread_t  touch_thread = RT_NULL;
    char touch_name[RT_NAME_MAX];

    if (argc == 2) {
        rt_strncpy(touch_name, argv[1], RT_NAME_MAX);
    } else if (argc > 2) {
        cmd_touch_test_help();
        return;
    } else {
        rt_strncpy(touch_name, TOUCH_DEFAULT_NAME, RT_NAME_MAX);
    }

    g_dev = rt_device_find(touch_name);
    if (g_dev == RT_NULL) {
        rt_kprintf("can't find device: touch\n");
        return;
    }

    if (rt_device_open(g_dev, RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        rt_kprintf("open device failed!");
        return;
    } else {
        rt_device_control(g_dev, RT_TOUCH_CTRL_GET_INFO, &g_info);
        rt_kprintf("type       :%d\n", g_info.type);
        rt_kprintf("point_num  :%d\n", g_info.point_num);
        rt_kprintf("range_x    :%d\n", g_info.range_x);
        rt_kprintf("range_y    :%d\n", g_info.range_y);
    }

    rt_device_set_rx_indicate(g_dev, rx_callback);

    g_touch_sem = rt_sem_create("dsem", 0, RT_IPC_FLAG_FIFO);
    if (g_touch_sem == RT_NULL) {
        rt_kprintf("create dynamic semaphore failed.\n");
        return;
    }

    touch_thread = rt_thread_create("touch",
                                     touch_entry,
                                     RT_NULL,
                                     THREAD_STACK_SIZE,
                                     THREAD_PRIORITY,
                                     THREAD_TIMESLICE);

    if (touch_thread != RT_NULL)
        rt_thread_startup(touch_thread);

    return;
}
MSH_CMD_EXPORT(test_touch, test touch sample);
