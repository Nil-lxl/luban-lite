/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <string.h>
#include <rtthread.h>
#include <aic_core.h>

#define BC_UART_TX_MSG_POOL_SIZE (4*1024)
static char tx_msg_pool[BC_UART_TX_MSG_POOL_SIZE] = {0};

static int serial_config_uart(rt_device_t uart)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    if (rt_device_control(uart, RT_SERIAL_GET_CONFIG, &config) != RT_EOK) {
        rt_kprintf("uart get configure parameter fail!\n");
        return -1;
    }

    config.baud_rate = BAUD_RATE_115200;
    config.data_bits = 8;
    config.stop_bits = 1;
    config.parity = PARITY_NONE;

    if (rt_device_control(uart, RT_DEVICE_CTRL_CONFIG, &config) != RT_EOK) {
        rt_kprintf("uart set baudrate fail!\n");
        return -1;
    }

    return 0;
}

static int serial_send_msg(rt_device_t serial, unsigned char *msg, int len)
{
    memset(tx_msg_pool, 0, sizeof(tx_msg_pool));
    memcpy(tx_msg_pool, msg, len > BC_UART_TX_MSG_POOL_SIZE ? BC_UART_TX_MSG_POOL_SIZE : len);
    return rt_device_write(serial, 0, tx_msg_pool, sizeof(tx_msg_pool));
}

int uart_send_msg(rt_device_t uart, unsigned char *msg, int len)
{
    if (uart == NULL || msg == NULL || len < 0) {
        rt_kprintf("uart_send_msg parameter error! uart:[%d], msg:[%s], len:[%d]\n",
            uart, msg, len);
        return -1;
    }

    return serial_send_msg(uart, msg, len);
}

int uart_init(const char * port, rt_device_t * p_uart)
{
    int ret = -1;

    *p_uart = rt_device_find(port);
    if (!*p_uart)
    {
        printf("find %s failed!\n", port);
        return -RT_ERROR;
    }

    if (serial_config_uart(*p_uart) != 0) {
        printf("config %s failed!\n", port);
        return -RT_ERROR;
    }

    ret = rt_device_open(*p_uart, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    if (ret != RT_EOK) {
        printf("open %s failed !\n", port);
        return -RT_ERROR;
    }
    return ret;
}
