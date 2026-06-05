/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */

#ifndef UART_COMM_H
#define UART_COMM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>
#include <stdbool.h>
#include "barcode_config.h"

typedef struct {
    rt_device_t uart_dev;
    char port[16];
    bool initialized;
} uart_comm_handle_t;

#ifdef BARCODE_ENABLE_UART

/**
 * Initialize UART communication
 * @param handle UART handle
 * @param port UART port name
 * @return 0 on success, negative value on failure
 */
int uart_comm_init(uart_comm_handle_t *handle, const char *port);

/**
 * Deinitialize UART communication
 * @param handle UART handle
 */
void uart_comm_deinit(uart_comm_handle_t *handle);

/**
 * Send data via UART
 * @param handle UART handle
 * @param msg Data pointer
 * @param len Data length
 * @return Actual bytes sent, negative value on failure
 */
int uart_comm_send(uart_comm_handle_t *handle, const unsigned char *msg, int len);

#endif /* BARCODE_ENABLE_UART */

#ifdef __cplusplus
}
#endif

#endif /* UART_COMM_H */
