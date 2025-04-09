/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao.Li <siyao.li@artinchip.com>
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <getopt.h>
#include "aic_drv_irq.h"
#include "aic_hal_gpio.h"
#include <aic_core.h>
#include "aic_common.h"

#define TEST_GPIO_INPUT_PIN         ("PD.15")
#define GPIO_PINS_NUM_PER_GROUP     32
#define GPIO_NO_SET_MODE_FLAG       1
#define GPIO_SET_MODE_FLAG          0

u32 rising_time_point = 0;
u32 falling_time_point = 0;
volatile u32 rising_flg = 0;
volatile u32 falling_flg = 0;

static void cmd_gpio_usage(void)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: test_gpio [options]\n");
    printf("test_gpio input <PIN>      : Configure PIN as input-pin, and print pressed count. Default as PD.15\n");
    printf("test_gpio output <PIN>     : Configure PIN as output-pin\n");
    printf("test_gpio check <PIN>      : Check PIN configuration status\n");
    printf("test_gpio help             : Get this help\n");
    printf("\n");
    printf("Example: test_gpio input PD.4\n");
}

static void cmd_gpio_irq_callback(void *args)
{
    u32 value;
    u32 pin = (u32)args;
    u32 g, p;

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_get_value(g, p, &value);
    if (value) {
        /* obtain the rising level time*/
        rising_time_point = aic_get_time_us();
        rising_flg = 1;
    } else {
        /* obtain the falling level time*/
        falling_time_point = aic_get_time_us();
        falling_flg = 1;
    }
    if (rising_flg && falling_flg) {
        rising_flg = 0;
        falling_flg = 0;
        printf("time interval: %dus\n", abs(falling_time_point - rising_time_point));
    }
}

static u32 test_gpio_pin_check(char *arg_pin, int set_mode_flag)
{
    u32 pin;
    if (arg_pin == NULL || strlen(arg_pin) == 0) {
        printf("pin set default PD.15\n");
        pin = hal_gpio_name2pin(TEST_GPIO_INPUT_PIN);
    } else {
        if (set_mode_flag)
            printf("pin set      : [%s]\n", arg_pin);
        pin = hal_gpio_name2pin(arg_pin);
    }

    return pin;
}

static int cmd_gpio_get_pin_cfg_status(int argc, char **argv)
{
    int ret;
    u32 pin, g, p;
    pin = test_gpio_pin_check(argv[1], GPIO_NO_SET_MODE_FLAG);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_FUN);
    printf("pin funtion  : [%d]\n", ret);

    ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_GEN_IRQ_MODE);
    switch(ret) {
    case PIN_IRQ_MODE_EDGE_FALLING:
        printf("irq mode     : edge falling\n");
        break;
    case PIN_IRQ_MODE_EDGE_RISING:
        printf("irq mode     : edge rising\n");
        break;
    case PIN_IRQ_MODE_LEVEL_LOW:
        printf("irq mode     : level low\n");
        break;
    case PIN_IRQ_MODE_LEVEL_HIGH:
        printf("irq mode     : level high\n");
        break;
    case PIN_IRQ_MODE_EDGE_BOTH:
        printf("irq mode     : edge both\n");
        break;
    default:
        printf("irq mode     : Unknown(%d)\n", ret);
        break;
    }

    ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_PIN_DRV);
    switch(ret) {
    case PIN_PULL_DIS:
        printf("pull mode    : disabled\n");
        break;
    case PIN_PULL_DOWN:
        printf("pull mode    : down\n");
        break;
    case PIN_PULL_UP:
        printf("pull mode    : up\n");
        break;
    default:
        printf("pull mode    : Unknown(%d)\n", ret);
        break;
    }

    ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_IE);
    if (ret < 0) {
        printf("input enable : disabled\n");
    } else {
        printf("input enable : enabled\n");
    }

    ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_OE);
    if (ret < 0) {
        printf("output enable: disabled\n");
    } else {
        printf("output enable: enabled\n");
    }

    return 0;
}


static int cmd_gpio_input_pin_cfg(int argc, char **argv)
{
    int ret;
    u32 pin, g, p;
    unsigned int gpio_irq;
    pin = test_gpio_pin_check(argv[1], GPIO_SET_MODE_FLAG);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_func(g, p, 1);
    hal_gpio_direction_input(g, p);
    hal_gpio_set_irq_mode(g, p, PIN_IRQ_MODE_EDGE_BOTH);

    gpio_irq = AIC_GPIO_TO_IRQ(g * GPIO_GROUP_SIZE + p);
    drv_irq_register(gpio_irq, cmd_gpio_irq_callback, (void *)pin);
    hal_gpio_enable_irq(g, p);

    cmd_gpio_get_pin_cfg_status(argc, argv);
    ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_IE);
    if (ret < 0) {
        printf("Set the input pin failed\n");
    } else {
        printf("Set the input pin successfully\n");
    }

    return 0;
}

static int cmd_gpio_output_pin_cfg(int argc, char **argv)
{
    int ret;
    u32 pin, g, p;
    pin = test_gpio_pin_check(argv[1], GPIO_SET_MODE_FLAG);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_func(g, p, 1);
    hal_gpio_direction_output(g, p);

    for(int i = 0; i < 5; i++)
    {
        hal_gpio_clr_output(g, p);
        aicos_mdelay(50);
        hal_gpio_set_output(g, p);
        cmd_gpio_get_pin_cfg_status(argc, argv);
        ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_OE);
        if (ret < 0) {
            printf("Set the output pin failed\n");
        } else {
            printf("Set the output pin successfully\n");
        }
        aicos_mdelay(50);
    }
    return 0;
}

static int cmd_test_gpio(int argc, char *argv[])
{
    if (argc < 3) {
        cmd_gpio_usage();
        return 0;
    }

    if (!strcmp(argv[1], "input")) {
        cmd_gpio_input_pin_cfg(argc - 1, &argv[1]);
        return 0;
    }

    if (!strcmp(argv[1], "output")) {
        cmd_gpio_output_pin_cfg(argc - 1, &argv[1]);
        return 0;
    }

    if (!strcmp(argv[1], "check")) {
        cmd_gpio_get_pin_cfg_status(argc - 1, &argv[1]);
        return 0;
    }

    cmd_gpio_usage();

    return 0;
}
CONSOLE_CMD(test_gpio, cmd_test_gpio, "GPIO test example");
