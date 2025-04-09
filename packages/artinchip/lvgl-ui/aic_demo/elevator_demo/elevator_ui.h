/*
 * Copyright (C) 2023-2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Keliang Liu <keliang.liu@artinchip.com>
 *          Huahui Mai <huahui.mai@artinchip.com>
 */

#ifndef ELEVATOR_UI_H
#define ELEVATOR_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#define ELEVATOR_MAX_LEVEL 36

void elevator_ui_init();

#ifdef LV_ELEVATOR_UART_COMMAND
int inside_up_down_ioctl(void *data, unsigned int data_len);
int inside_level_ioctl(void *data, unsigned int data_len);
int replayer_ioctl(void *data, unsigned int data_len);
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* ELEVATOR_UI_H */

