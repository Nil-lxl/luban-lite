/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */

#ifndef BARCODE_LED_H
#define BARCODE_LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>
#include <rtdevice.h>
#include <stdbool.h>
#include "config/barcode_config.h"

typedef struct {
    rt_uint32_t led_pin;
    bool initialized;
} barcode_led_t;

#ifdef AIC_BARCODE_DEMO_LED
/**
 * @brief Initialize LED control
 * @param led LED handle
 * @return 0 on success, negative value on failure
 */
int barcode_led_init(barcode_led_t *led);

/**
 * @brief Deinitialize LED control
 * @param led LED handle
 */
void barcode_led_deinit(barcode_led_t *led);

/**
 * @brief Turn on LED
 * @param led LED handle
 */
void barcode_led_on(barcode_led_t *led);

/**
 * @brief Turn off LED
 * @param led LED handle
 */
void barcode_led_off(barcode_led_t *led);

/**
 * @brief Set LED state
 * @param led LED handle
 * @param on true to turn on, false to turn off
 */
void barcode_led_set(barcode_led_t *led, bool on);

#endif /* AIC_BARCODE_DEMO_LED */

#ifdef __cplusplus
}
#endif

#endif /* BARCODE_LED_H */
