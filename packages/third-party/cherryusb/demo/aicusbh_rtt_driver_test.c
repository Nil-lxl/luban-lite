/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: artinchip
 */

#include <rtconfig.h>
#include "aic_osal.h"

void aicusbh_hid_hotplug_irq(char *name, int32_t hotplug)
{
    /* warnning currunt code is running in irq context */
    rt_kprintf("%s device %s is detected\n", name, hotplug ? "pluging" : "unpluging");
}

static void hid_thread(void *p)
{
    // uint8_t intverl = 0;
    uint16_t size = 0;
    char *buff = NULL;
    rt_device_t dev;
    char *name = p;
    int ret = 0;

    dev = rt_device_find(name);
    if (dev == RT_NULL)
    {
        free(name);
        rt_kprintf("can't find device:%s\n", name);
        return;
    } else {
        rt_kprintf("find HID device named: %s\n", name);
    }

    if (rt_device_open(dev, 0) != RT_EOK)
    {
        free(name);
        rt_kprintf("Failed to open dev %s\n", name);
        return;
    }

    if (memcmp(name, "keyboard", strlen("keyboard")) == 0)
        size = 8;
    else if (memcmp(name, "mouse", strlen("mouse")) == 0)
        size = 4;
    else
        size = 0;

    // rt_device_control(dev, 0, &size);
    // rt_device_control(dev, 1, &intverl);

    buff = malloc(size);
    if (!buff) {
        free(name);
        rt_kprintf("malloc error\n");
        return;
    }

    while (1) {
        ret = rt_device_read(dev, 0, buff, size);
        if (ret <= 0) {
            printf("rt_device_read error, error code = %d\n", ret);
            break;;
        }
        if (size == 4)
            printf("HID receve: %d, %d, %d, %d\n", buff[0], buff[1], buff[2], buff[3]);
        else if (size == 8)
            printf("HID receve: %d, %d, %d, %d, %d, %d, %d, %d\n", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5], buff[6], buff[7]);
        // aicos_msleep(intverl);
    }

    free(buff);
    free(name);
    rt_device_close(dev);
}

int test_hid(int argc, char *argv[])
{
    char *name;
    int len;

    if (argc != 2) {
        printf("command error: test_hid [device name]\n");
        return -1;
    }

    len = strlen(argv[1]);
    name = malloc(len);
    if (!name) {
        printf("Can't malloc test_hid name buffer\n");
        return -1;
    }

    strcpy(name, argv[1]);
    aicos_thread_create("test_hid", 4096, 20, hid_thread, name);
    return 0;
}
MSH_CMD_EXPORT(test_hid, "test_hid [hid device]");

