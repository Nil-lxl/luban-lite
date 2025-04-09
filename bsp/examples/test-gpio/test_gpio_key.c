/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
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

#define TEST_GPIO_KEY_PIN               "PA.2"

static int g_end_time = 0;
static int g_start_time = 0;
static int g_long_press_time = 3000;
static unsigned int g_init_pin_status;

static const char sopts[] = "i:t:h";
static const struct option lopts[] = {
    {"input",       optional_argument,  NULL, 'i'},
    {"time",        optional_argument,  NULL, 't'},
    {"help",        no_argument,        NULL, 'h'},
    {0, 0, 0, 0}
};

static void test_gpio_key_usage(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -i, --input\t\tConfigure PIN as key-input-pin.Default as PA.2\n");
    printf("\t -t  --time\t\tSet long press time, unit ms.\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example:\n");
    printf("\t%s -t 200 -i PA.2\n", program);
}

static void test_gpio_key_input_irq_handler(void *args)
{
    unsigned int val;
    u32 pin = *((u32 *)(args));

    // Obtain the pin level status when the interrupt is triggered.
    hal_gpio_get_value(GPIO_GROUP(pin), GPIO_GROUP_PIN(pin), &val);

    if (val == g_init_pin_status) {
        g_end_time = aic_get_time_ms();
        if ((g_end_time - g_start_time) > g_long_press_time) {
            rt_kprintf("Long press\n");
        } else {
            rt_kprintf("Short press\n");
        }
    } else {
        g_start_time = aic_get_time_ms();
    }
}

/**
 * @brief This function will check if the pins are set correctly, if set incorrectly, use the default pin settings.
 */

static u32 test_gpio_key_pin_check(char *arg_pin)
{
    u32 pin;

    if (arg_pin == RT_NULL || strlen(arg_pin) == 0) {
        rt_kprintf("pin set default PA.2\n");
        pin = rt_pin_get(TEST_GPIO_KEY_PIN);
    } else {
        rt_kprintf("pin set          : [%s]\n", arg_pin);
        pin = rt_pin_get(arg_pin);
    }

    return pin;
}

static u32 test_gpio_key_input_pin_cfg(char *arg_pin)
{
    // 1.get pin number
    static u32 pin = 0;
    pin = test_gpio_key_pin_check(arg_pin);

    // 2.set pin mode to Input-PullUp
    rt_pin_mode(pin, PIN_MODE_INPUT_PULLUP);

    // 3. Obtain the initial state of the pin level when button is not pressed.
    hal_gpio_get_value(GPIO_GROUP(pin), GPIO_GROUP_PIN(pin),
                       &g_init_pin_status);
    rt_kprintf("Current pin status: %d\n", g_init_pin_status);

    // 4.attach irq handler
    rt_pin_attach_irq(pin, PIN_IRQ_MODE_RISING_FALLING,
                      test_gpio_key_input_irq_handler, &pin);

    // 5.enable pin irq
    rt_pin_irq_enable(pin, PIN_IRQ_ENABLE);

    return pin;
}


int test_gpio_key(int argc, char **argv)
{
    int c = 0;

    if (argc < 2) {
        test_gpio_key_usage(argv[0]);
        return -1;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'i':
            test_gpio_key_input_pin_cfg(optarg);
            break;
        case 't':
            g_long_press_time = atoi(optarg);
            if (g_long_press_time <= 0) {
                test_gpio_key_usage(argv[0]);
                return 0;
            }
            break;
        case 'h':
        default:
            test_gpio_key_usage(argv[0]);
            return 0;
        }
    }

    return 0;
}

MSH_CMD_EXPORT(test_gpio_key, gpio key device sample);
