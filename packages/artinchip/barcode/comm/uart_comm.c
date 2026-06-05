/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */

#include <string.h>
#include <rtthread.h>
#include <aic_core.h>
#include "uart_comm.h"
#include "barcode_config.h"


#ifdef BARCODE_ENABLE_UART

static int uart_config_device(rt_device_t uart)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    if (rt_device_control(uart, RT_SERIAL_GET_CONFIG, &config) != RT_EOK) {
        rt_kprintf("UART: get configure parameter failed\n");
        return -1;
    }

    config.baud_rate = BARCODE_UART_BAUDRATE;
    config.data_bits = 8;
    config.stop_bits = 1;
    config.parity = PARITY_NONE;

    if (rt_device_control(uart, RT_DEVICE_CTRL_CONFIG, &config) != RT_EOK) {
        rt_kprintf("UART: set baudrate failed\n");
        return -1;
    }

    return 0;
}

int uart_comm_init(uart_comm_handle_t *handle, const char *port)
{
    int ret;

    if (!handle || !port) {
        rt_kprintf("UART: invalid parameters\n");
        return -1;
    }

    memset(handle, 0, sizeof(uart_comm_handle_t));
    strncpy(handle->port, port, sizeof(handle->port) - 1);

    handle->uart_dev = rt_device_find(port);
    if (!handle->uart_dev) {
        rt_kprintf("UART: find %s failed\n", port);
        return -RT_ERROR;
    }

    if (uart_config_device(handle->uart_dev) != 0) {
        rt_kprintf("UART: config %s failed\n", port);
        return -RT_ERROR;
    }

    ret = rt_device_open(handle->uart_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    if (ret != RT_EOK) {
        rt_kprintf("UART: open %s failed\n", port);
        return -RT_ERROR;
    }

    handle->initialized = true;
    return RT_EOK;
}

void uart_comm_deinit(uart_comm_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return;
    }

    if (handle->uart_dev) {
        rt_device_close(handle->uart_dev);
        handle->uart_dev = RT_NULL;
    }

    handle->initialized = false;
}

int uart_comm_send(uart_comm_handle_t *handle, const unsigned char *msg, int len)
{
    if (!handle || !handle->initialized || !msg || len <= 0) {
        rt_kprintf("UART: send parameter error, handle:[%p], msg:[%p], len:[%d]\n",
                   (void*)handle, (void*)msg, len);
        return -1;
    }

    return rt_device_write(handle->uart_dev, 0, msg, len);
}

#endif /* BARCODE_ENABLE_UART */
