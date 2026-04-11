/*
 * Copyright (C) 2023-2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui Mai <huahui.mai@artinchip.com>
 *
 */

#ifndef ELEVATOR_UART_H
#define ELEVATOR_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_common.h>
#include <aic_list.h>

#define UART_IOCTL_NR(n)    (((n) >> 8) & 0xFF)

#define UART_IOCTL_UP_DOWN _IOR(0x0, 'F', unsigned int)
#define UART_IOCTL_LEVEL   _IOR(0x1, 'F', unsigned int)
#define UART_IOCTL_REPLAY  _IOR(0x2, 'F', unsigned int)

typedef int uart_ui_ioctl_t(void *data, unsigned int data_len);

int evevator_uart_main(void);

struct uart_ioctl_desc {
	unsigned int cmd;
	uart_ui_ioctl_t *ui_func;
	const char *name;
};

struct disp_task {
    const struct uart_ioctl_desc *ioctl;
    unsigned int data_len;
    char data[12];
    struct list_head next;
};

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* ELEVATOR_UART_H */

