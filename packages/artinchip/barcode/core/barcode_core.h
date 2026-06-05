/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */

#ifndef BARCODE_CORE_H
#define BARCODE_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "camera/camera_capture.h"
#include "decoder/barcode_decoder.h"
#include "display/barcode_display.h"
#include "comm/uart_comm.h"
#include "led/barcode_led.h"
#include "config/barcode_config.h"

#include <rtthread.h>
#include <rtdevice.h>
#include "aic_osal.h"

typedef struct {
    camera_capture_t camera;
    barcode_decoder_t decoder;
    barcode_display_t display;
    uart_comm_handle_t uart;
    barcode_led_t led;

    struct rt_device_pwm *pwm_dev;

    /* Note: running flag is accessed by multiple threads without lock.
     * This is intentional for performance. The flag is only written when
     * a fatal error occurs or during shutdown. */
    volatile bool running;
    unsigned char *frame_buffer;
    aicos_thread_t shot_thread;
    aicos_thread_t decode_thread;
} barcode_system_t;

/**
 * @brief Initialize barcode system
 * @param sys System handle
 * @return 0 on success, negative value on failure
 */
int barcode_system_init(barcode_system_t *sys);

/**
 * @brief Start barcode system
 * @param sys System handle
 */
void barcode_system_start(barcode_system_t *sys);

/**
 * @brief Stop barcode system
 * @param sys System handle
 */
void barcode_system_stop(barcode_system_t *sys);

/**
 * @brief Deinitialize barcode system
 * @param sys System handle
 */
void barcode_system_deinit(barcode_system_t *sys);

#ifdef __cplusplus
}
#endif

#endif /* BARCODE_CORE_H */
