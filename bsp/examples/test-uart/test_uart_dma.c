/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <rtthread.h>
#include <aic_core.h>
#include <stdio.h>
#include "string.h"
#define SAMPLE_UART_NAME       "uart2"

struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
};

static rt_device_t serial;
static struct rt_messagequeue rx_mq;
static char str[] = "1234567890abcdefghijklmnopqrstuvwxyz!@#$&*";
static int run_times = 1;

static void cmd_uart_test_help(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("\ttest_uart_dma <serial_name> <run_times>\n");
    rt_kprintf("\tdefault use uart2&execution once\n\n");
    rt_kprintf("\tFor example:\n");
    rt_kprintf("\t\ttest_uart_dma\n");
    rt_kprintf("\t\ttest_uart_dma uart1\n");
    rt_kprintf("\t\ttest_uart_dma uart1 10\n");
}

static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    struct rx_msg msg;
    rt_err_t result;
    msg.dev = dev;
    msg.size = size;
    result = rt_mq_send(&rx_mq, &msg, sizeof(msg));
    if (result == -RT_EFULL) {
        rt_kprintf("message queue full!\n");
    }
    return result;
}

static void serial_rx_thread_entry(void *parameter)
{
    struct rx_msg msg;
    rt_err_t result;
    rt_uint32_t rx_length;
    static char rx_buffer[256];
    rt_uint32_t current_times = 0;

    while (current_times < run_times) {
        rt_memset(&msg, 0, sizeof(msg));
        result = rt_mq_recv(&rx_mq, &msg, sizeof(msg), RT_WAITING_FOREVER);
        if (result == 0) {
            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);
            rx_buffer[rx_length] = '\0';
            rt_kprintf("send: %s\n", str);
            rt_kprintf("recv: %s\n",rx_buffer);
            /* If write data more than 12 must use printf */
            // printf("recv: %s\n",rx_buffer);
            current_times++;
        }
    }
    /* delete thread */
    rt_thread_delete(rt_thread_self());
}

static void serial_tx_thread_entry(void *parameter)
{
    rt_uint32_t current_times = 0;
    while(current_times < run_times) {
        rt_device_write(serial, 0, str, (sizeof(str)-1));
        rt_thread_mdelay(5);
        current_times++;
    }
    /* delete thread */
    rt_thread_delete(rt_thread_self());
}

static int test_uart_dma(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    char uart_name[RT_NAME_MAX];
    static char msg_pool[256];
    static rt_uint8_t count = 0;

    if (argc == 3) {
        rt_strncpy(uart_name, argv[1], RT_NAME_MAX);
        run_times = atoi(argv[2]);
    } else if (argc > 3) {
        cmd_uart_test_help();
        return 0;
    } else {
        rt_strncpy(uart_name, SAMPLE_UART_NAME, RT_NAME_MAX);
        run_times = 1;
    }

    serial = rt_device_find(uart_name);
    if (!serial) {
        rt_kprintf("find %s failed!\n", uart_name);
        return RT_ERROR;
    }

    rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX);
    if (count == 0) {
        rt_device_set_rx_indicate(serial, uart_input);
        rt_mq_init(&rx_mq, "rx_mq",
                   msg_pool,
                   sizeof(struct rx_msg),
                   sizeof(msg_pool),
                   RT_IPC_FLAG_FIFO);
    }

    rt_thread_t rx_thread = rt_thread_create("rx_serial", serial_rx_thread_entry,
                                                        RT_NULL, 1024 * 2, 25, 10);
    rt_thread_t tx_thread = rt_thread_create("tx_serial", serial_tx_thread_entry,
                                                        RT_NULL, 1024 * 2, 25, 10);

    if (rx_thread != RT_NULL && tx_thread != RT_NULL) {
        rt_thread_startup(rx_thread);
        rt_thread_startup(tx_thread);
    } else {
        rt_kprintf("thread create error\n");
        ret = RT_ERROR;
    }
    count++;

    return ret;
}
MSH_CMD_EXPORT(test_uart_dma, uart device dma sample);

