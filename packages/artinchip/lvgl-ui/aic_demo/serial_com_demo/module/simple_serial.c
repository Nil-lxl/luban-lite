/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  zequan liang <zequan liang@artinchip.com>
 */
#include "stdlib.h"
#include "string.h"
#include <rtthread.h>
#include <aic_core.h>
#include "simple_serial.h"

#define UART_MQ_SIZE 128

static rt_mq_t g_rx_mq;
static rt_device_t g_serial;

struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
};

static rt_err_t serial_input_mq(rt_device_t dev, rt_size_t size);

int simple_serial_init(char *uart, int boaud, int data_bits, int stop_bits, int parity)
{
    g_rx_mq = rt_mq_create("uart_rx_mq", sizeof(struct rx_msg), UART_MQ_SIZE, RT_IPC_FLAG_FIFO);
    if (!g_rx_mq) {
        printf("rt mq create failed, msq size = %d\n", sizeof(struct rx_msg)  * UART_MQ_SIZE);
        return -1;
    }

    g_serial = rt_device_find(uart);
    if (!g_serial)
    {
        printf("find %s failed!\n", uart);
        return -RT_ERROR;
    }

    /* config */
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    config.baud_rate = boaud;
    config.data_bits = data_bits;
    config.stop_bits = stop_bits;
    config.parity = parity;

    if (rt_device_control(g_serial, RT_DEVICE_CTRL_CONFIG, &config) != RT_EOK) {
        printf("uart set baudrate fail!\n");
        return -1;
    }

    rt_err_t ret = rt_device_open(g_serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    if (ret != RT_EOK) {
        printf("open %s failed !\n", uart);
        return -RT_ERROR;
    }

    /* recv config */
    rt_device_set_rx_indicate(g_serial, serial_input_mq);

    return 0;
}

int simple_serial_send(unsigned char *data, int len)
{
    if (rt_device_write(g_serial, 0, data, len) != RT_EOK) {
        rt_kprintf("rt write serial failed, name = %d", g_serial->parent.name);
        return -1;
    }
    return 0;
}

int simple_serial_recv(unsigned char *data, int max_len)
{
    struct rx_msg msg = {0};
    int len = -1;

    rt_memset(&msg, 0, sizeof(msg));
    if (rt_mq_recv(g_rx_mq, &msg, sizeof(msg), RT_WAITING_FOREVER) == 0) {
        if (msg.size >= max_len - 1) {
            len = rt_device_read(msg.dev, 0, data, max_len - 1);
            printf("The %s receive buffer is configured too small. config size = %d, recv size = %d", g_serial->parent.name, max_len, msg.size);
        } else {
            len = rt_device_read(msg.dev, 0, data, msg.size);
        }
    }

    return len;
}

static rt_err_t serial_input_mq(rt_device_t dev, rt_size_t size)
{
    struct rx_msg msg = {0};
    rt_err_t result = 0;
    msg.dev = dev;
    msg.size = size;

    result = rt_mq_send(g_rx_mq, &msg, sizeof(msg));
    if (result == -RT_EFULL) {
        printf("message queue full!\n");
    }

    return result;
}
