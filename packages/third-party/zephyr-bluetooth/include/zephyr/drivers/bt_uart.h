/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-16     Lenoyan      the first version
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BT_UART_H_
#define ZEPHYR_INCLUDE_DRIVERS_BT_UART_H_

#include <stddef.h>

#include <aic_common.h>
#include <zephyr/kernel.h>
#include <rtthread.h>
#include <rtdevice.h>

#include <zephyr/drivers/bluetooth.h>

typedef void (*uart_irq_callback_user_data_t)(const struct rt_device *uart, void *user_data);

int bt_uart_init(void);
int bt_uart_baudrate_update(uint32_t baudrate);
int bt_uart_flow_control_update(uint32_t flow_control);
int uart_fifo_read(const struct rt_device *dev, u8 *data, const int size);
void bt_uart_drain(const struct rt_device *uart);
int uart_fifo_fill(const struct rt_device *dev, const u8 *data, int size);
int uart_poll_out(const struct rt_device *dev, u8 c);
void uart_irq_rx_enable(struct rt_device *uart);
void uart_irq_rx_disable(struct rt_device *uart);
int uart_irq_rx_status_get(struct rt_device *uart);
void uart_irq_tx_enable(struct rt_device *uart);
void uart_irq_tx_disable(struct rt_device *uart);
int uart_irq_rx_ready(struct rt_device *uart);
int uart_irq_tx_ready(struct rt_device *uart);
int uart_irq_callback_user_data_set(const struct rt_device *uart, uart_irq_callback_user_data_t cb,
        void *user_data);

#endif /* ZEPHYR_INCLUDE_DRIVERS_BT_UART_H_ */
