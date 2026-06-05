/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */

#include <string.h>
#include "aic_core.h"
#include "aic_log.h"
#include "led/barcode_led.h"
#include "config/barcode_config.h"

#ifdef AIC_BARCODE_DEMO_LED

/**
 * @brief Initialize LED control
 */
int barcode_led_init(barcode_led_t *led)
{
    if (!led) {
        return -1;
    }

    memset(led, 0, sizeof(barcode_led_t));

    led->led_pin = rt_pin_get(BARCODE_DEMO_LED_GPIO);
    if (led->led_pin <= 0) {
        pr_err("LED: pin get failed!\n");
        return -1;
    }

    rt_pin_mode(led->led_pin, PIN_MODE_OUTPUT);
    led->initialized = true;

    pr_info("LED: initialized successfully on pin %s\n", BARCODE_DEMO_LED_GPIO);
    return 0;
}

/**
 * @brief Deinitialize LED control
 */
void barcode_led_deinit(barcode_led_t *led)
{
    if (!led || !led->initialized) {
        return;
    }

    // Turn off LED before deinit
    barcode_led_off(led);

    led->initialized = false;
    led->led_pin = 0;

    pr_info("LED: deinitialized\n");
}

/**
 * @brief Turn on LED
 */
void barcode_led_on(barcode_led_t *led)
{
    if (!led || !led->initialized) {
        return;
    }

    rt_pin_write(led->led_pin, PIN_HIGH);
}

/**
 * @brief Turn off LED
 */
void barcode_led_off(barcode_led_t *led)
{
    if (!led || !led->initialized) {
        return;
    }

    rt_pin_write(led->led_pin, PIN_LOW);
}

/**
 * @brief Set LED state
 */
void barcode_led_set(barcode_led_t *led, bool on)
{
    if (!led || !led->initialized) {
        return;
    }

    if (on) {
        barcode_led_on(led);
    } else {
        barcode_led_off(led);
    }
}

#endif /* BARCODE_LED_ENABLE */
