/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtthread.h>
#include "rtdevice.h"
#include <aic_core.h>
#include "aic_hal_can.h"

/*
 * This program is used to test CAN selftest mode.
 */
static struct rt_semaphore g_rx_sem;
static rt_device_t g_can_dev;
static char g_can_dev_name[5];

static rt_err_t can_rx_call(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&g_rx_sem);

    return RT_EOK;
}

static void can_rx_thread(void *parameter)
{
    int i;
    rt_size_t size;
    struct rt_can_msg rxmsg = {0};

    rt_device_open(g_can_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);

    rt_sem_take(&g_rx_sem, RT_WAITING_FOREVER);

    rxmsg.hdr = -1;
    size = rt_device_read(g_can_dev, 0, &rxmsg, sizeof(rxmsg));
    if (!size)
    {
        rt_kprintf("CAN read error\n");
        goto __exit_can_rx;
    }

    rt_kprintf("%s received msg:\nID: 0x%x ", g_can_dev_name, rxmsg.id);

    if (rxmsg.len)
        rt_kprintf("DATA: ");
    for (i = 0; i < rxmsg.len; i++)
    {
        rt_kprintf("%02x ", rxmsg.data[i]);
    }

    rt_kprintf("\n");

__exit_can_rx:
    rt_sem_detach(&g_rx_sem);
    rt_device_close(g_can_dev);
}

static void parse_msg_data(rt_can_msg_t msg, char * optarg)
{
    char *token;
    uint8_t i = 0, id_received = 0;

    token = strtok(optarg, "#.");

    while (token)
    {
        if (!id_received)
        {
            msg->id = strtoul(token, NULL, 16);
            if (msg->id > 0x7FF)
                msg->ide = 1;
            else
                msg->ide = 0;

            id_received = 1;
        }
        else
        {
            /* frame data */
            msg->rtr = CAN_FRAME_TYPE_DATA;
            msg->data[i++] = strtoul(token, NULL, 16);
            if (i >= 8)
                break;
        }

        token = strtok(NULL, "#.");
    }

    msg->len = i;
}

static void usage(char * program)
{
    printf("\n");
    printf("%s - test CAN loopback.\n\n", program);
    printf("Usage: %s CAN_DEV CAN_FRAME\n", program);
    printf("\tCAN_FRAME format: frame_id#frame_data\n");
    printf("For example:\n");
    printf("\t%s can0 1a3#11.22.9a.88.ef.00\n", program);
    printf("\t%s can1 1a3#11.22.33.44.55.66.77.88\n", program);
    printf("\n");
}

int test_can_loopback(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    struct rt_can_msg msg = {0};
    rt_thread_t thread;

    if (argc != 3)
    {
        usage(argv[0]);
        return -RT_EINVAL;
    }

    snprintf(g_can_dev_name, 5, "%s", argv[1]);
    g_can_dev = rt_device_find(g_can_dev_name);
    if (!g_can_dev)
    {
        rt_kprintf("find %s failed!\n", g_can_dev_name);
        return -RT_EINVAL;
    }

    ret = rt_device_open(g_can_dev,
                         RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    if (ret)
    {
        rt_kprintf("%s open failed!\n", g_can_dev_name);
        return -RT_ERROR;
    }

    ret = rt_device_control(g_can_dev, RT_CAN_CMD_SET_BAUD, (void *)CAN1MBaud);
    if (ret)
    {
        rt_kprintf("%s set baudrate failed!\n", g_can_dev_name);
        ret = -RT_ERROR;
        goto __exit;
    }

    //enable CAN TX interrupt
    rt_device_control(g_can_dev, RT_DEVICE_CTRL_SET_INT, NULL);
    rt_device_control(g_can_dev, RT_CAN_CMD_SET_MODE,
                      (void *)CAN_SELFTEST_MODE);

    rt_device_set_rx_indicate(g_can_dev, can_rx_call);

    rt_sem_init(&g_rx_sem, "rx_sem", 0, RT_IPC_FLAG_PRIO);

    thread = rt_thread_create("can_rx", can_rx_thread, NULL,
                                  2048, 15, 10);
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        rt_kprintf("create can_rx thread failed!\n");
        ret = -RT_ERROR;
        goto __exit;
    }

    parse_msg_data(&msg, argv[2]);
    rt_device_write(g_can_dev, 0, &msg, sizeof(msg));

__exit:
    rt_device_close(g_can_dev);
    return ret;
}

MSH_CMD_EXPORT_ALIAS(test_can_loopback, test_can,
                     can device loopback sample);
