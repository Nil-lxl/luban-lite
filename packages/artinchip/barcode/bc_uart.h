/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _BC_UART_H_
#define _BC_UART_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int uart_send_msg(rt_device_t uart, unsigned char *msg, int len);
int uart_init(const char * port, rt_device_t * p_uart);

#ifdef __cplusplus
}
#endif

#endif  // _BC_UART_H_
