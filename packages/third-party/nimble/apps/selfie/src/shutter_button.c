/*
 * Copyright (c) 2025-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <rtthread.h>
#include "rtdevice.h"
#include "aic_core.h"
#include "aic_hal_gpio.h"
#include "aic_time.h"
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "selfie.h"

#define SHUTTER_BUTTON_GPIO_PIN               "PE.5"

static int g_end_time = 0;
static int g_start_time = 0;
static int g_long_press_time = 3000;
static unsigned int g_pin_status;

static void shutter_button_gpio_input_irq_handler(void *args)
{
    unsigned int val;
    unsigned int shutter_button = 0;
    int result = 0;
    u32 pin = *((u32 *)(args));

    // Obtain the pin level status when the interrupt is triggered.
    hal_gpio_get_value(GPIO_GROUP(pin), GPIO_GROUP_PIN(pin), &val);

    if (val == g_pin_status) {
        g_end_time = aic_get_time_ms();
        if ((g_end_time - g_start_time) > g_long_press_time) {
            rt_kprintf("Long press\n");
        } else {
            rt_kprintf("Short press\n");
        }
    } else {
        g_start_time = aic_get_time_ms();
        ble_npl_eventq_put(ble_hs_evq_get(), &ble_hs_ev_shutter);
    }
}

static u32 shutter_button_gpio_pin_check(char *arg_pin)
{
    u32 pin;

    if (arg_pin == RT_NULL || strlen(arg_pin) == 0) {
        rt_kprintf("pin set default %s\n", SHUTTER_BUTTON_GPIO_PIN);
        pin = rt_pin_get(SHUTTER_BUTTON_GPIO_PIN);
    } else {
        rt_kprintf("pin set          : [%s]\n", arg_pin);
        pin = rt_pin_get(arg_pin);
    }

    return pin;
}

static u32 shutter_button_gpio_input_pin_cfg(char *arg_pin)
{
    static u32 pin = 0;

    pin = shutter_button_gpio_pin_check(arg_pin);
    rt_pin_mode(pin, PIN_MODE_INPUT_PULLUP);

    hal_gpio_get_value(GPIO_GROUP(pin), GPIO_GROUP_PIN(pin), &g_pin_status);
    rt_kprintf("Current pin status: %d\n", g_pin_status);

    rt_pin_attach_irq(pin, PIN_IRQ_MODE_RISING_FALLING,
                      shutter_button_gpio_input_irq_handler, &pin);

    rt_pin_irq_enable(pin, PIN_IRQ_ENABLE);

    return pin;
}

static s32 shutter_button_gpio_init(void)
{
    shutter_button_gpio_input_pin_cfg(NULL);

    return 0;
}

INIT_APP_EXPORT(shutter_button_gpio_init);
