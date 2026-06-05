/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */

#ifndef BARCODE_CONFIG_H
#define BARCODE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// barcode buffer
#define BARCODE_MAX_BUF_LEN         256
#define BARCODE_VIDEO_BUF_NUM       3
#define BARCODE_VIDEO_BUF_PLANE_NUM 2
#define BARCODE_BUFFER_SIZE         (180 * 1024)

// UART config
#define BARCODE_ENABLE_UART
#define BARCODE_UART_PORT           "uart2"
#define BARCODE_UART_BAUDRATE       BAUD_RATE_115200
#define BARCODE_UART_TX_POOL_SIZE   (4 * 1024)

// display config
// #define BARCODE_ENABLE_DISPLAY
#define BARCODE_DISPLAY_ROTATION
#define BARCODE_ROTATION_ANGLE      MPP_ROTATION_90
#define BARCODE_UI_LAYER_ALPHA      64
#define BARCODE_FONT_WIDTH          32
#define BARCODE_TEXT_START_X        10
#define BARCODE_TEXT_START_Y        60
#define BARCODE_VID_SCALE_OFFSET    0
#ifdef BARCODE_ENABLE_DISPLAY
// #define BARCODE_DISPLAY_RESULT_TEXT
#endif

// PWM config
#define BARCODE_PWM_CHANNEL         1
#define BARCODE_PWM_PERIOD          1000000
#define BARCODE_PWM_DUTY_CYCLE      500000

// decoder config
// BARCODE_ENABLE_DECODER should be defined in Sconscript
#define BARCODE_DECODER_TYPE_COUNT  14

// thread config
#define BARCODE_SHOT_THREAD_STACK   (1024 * 8)
#define BARCODE_DECODE_THREAD_STACK (1024 * 32)
#define BARCODE_SHOT_THREAD_PRIO    0
#define BARCODE_DECODE_THREAD_PRIO  0

// event flag
#define BARCODE_DATA_READY_EVENT    1

#ifdef __cplusplus
}
#endif

#endif /* BARCODE_CONFIG_H */
