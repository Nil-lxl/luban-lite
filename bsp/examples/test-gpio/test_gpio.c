/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo.Dong <guojun.dong@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <rtthread.h>
#include "rtdevice.h"
#include "aic_core.h"
#include "aic_hal_gpio.h"

#define INPUT_KEY_PIN               "PD.15"
#define GPIO_NO_SET_MODE_FLAG       0
#define GPIO_SET_MODE_FLAG          1

static int time_turnaround = 10;
static int times_enter_irq = 0;
static const char sopts[] = "i:o:c:t:h";
static const struct option lopts[] = {
    {"input",       optional_argument,  NULL, 'i'},
    {"output",      optional_argument,  NULL, 'o'},
    {"check",       optional_argument,  NULL, 'c'},
    {"time",        optional_argument,  NULL, 't'},
    {"help",        no_argument,        NULL, 'h'},
    {0, 0, 0, 0}
};

static void test_gpio_usage(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -i, --input\t\tConfigure PIN as input-pin, and print pressed count. Default as PD.15\n");
    printf("\t -o, --output\t\tConfigure PIN as output-pin .\n");
    printf("\t -c, --check\t\tCheck PIN configuration status\n");
    printf("\t -t  --time\t\tOutput turnaround time, uint ms.\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example: configuration for either input-only io or output-only io\n");
    printf("         %s -i \n", program);
    printf("    or\n");
    printf("         %s -i PD.4 \n", program);
    printf("Example: output io directly interfaces with input io. Out io ---> Input io\n");
    printf("         %s -t 200 -i PD.4 -o PD.5\n", program);
}

static void test_gpio_input_irq_handler(void *args)
{
    unsigned int ret;
    u32 pin = *((u32 *)(args));

    times_enter_irq++;
    hal_gpio_get_value(GPIO_GROUP(pin), GPIO_GROUP_PIN(pin), &ret);
    printf("gpio group_%d_pin_%d read = %d\n", GPIO_GROUP(pin), GPIO_GROUP_PIN(pin), ret);
}

/*
 * parameter:
 *          arg_pin: input para of pin-name
 * return :
 *          0:      pin check failed !!!!
 *          other:  pin number
 *
 */
static u32 test_gpio_pin_check(char *arg_pin, int set_mode_flag)
{
    u32 pin;

    if (arg_pin == RT_NULL || strlen(arg_pin) == 0) {
        printf("pin set default PD.15\n");
        pin = rt_pin_get(INPUT_KEY_PIN);
    } else {
        if (set_mode_flag)
            printf("pin set          : [%s]\n", arg_pin);
        pin = rt_pin_get(arg_pin);
    }

    return pin;
}

static void test_gpio_check_pin_status(char *arg_pin)
{
    int ret, g , p;
    u32 pin;

    pin = test_gpio_pin_check(arg_pin, GPIO_NO_SET_MODE_FLAG);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_FUN);
    printf("pin funtion      : [%d]\n", ret);

    ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_GEN_IRQ_MODE);
    switch(ret) {
    case PIN_IRQ_MODE_EDGE_FALLING:
        printf("irq mode         : edge falling\n");
        break;
    case PIN_IRQ_MODE_EDGE_RISING:
        printf("irq mode         : edge rising\n");
        break;
    case PIN_IRQ_MODE_LEVEL_LOW:
        printf("irq mode         : level low\n");
        break;
    case PIN_IRQ_MODE_LEVEL_HIGH:
        printf("irq mode         : level high\n");
        break;
    case PIN_IRQ_MODE_EDGE_BOTH:
        printf("irq mode         : edge both\n");
        break;
    default:
        printf("irq mode         : Unknown(%d)\n", ret);
        break;
    }

    ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_PULL);
    switch(ret) {
    case PIN_PULL_DIS:
        printf("pull mode        : disabled\n");
        break;
    case PIN_PULL_DOWN:
        printf("pull mode        : down\n");
        break;
    case PIN_PULL_UP:
        printf("pull mode        : up\n");
        break;
    default:
        printf("pull mode        : Unknown(%d)\n", ret);
        break;
    }

    ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_PIN_DRV);
    printf("pin output drive : %d\n", ret);

    ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_IE);
    if (ret < 0) {
        printf("input enable     : disabled\n");
    } else {
        printf("input enable     : enabled\n");
    }

    ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_OE);
    if (ret < 0) {
        printf("output enable    : disabled\n");
    } else {
        printf("output enable    : enabled\n");
    }
}

static u32 test_gpio_input_pin_cfg(char *arg_pin)
{
    // 1.get pin number
    static u32 pin = 0;
    pin = test_gpio_pin_check(arg_pin, GPIO_SET_MODE_FLAG);

    // 2.set pin mode to Input-PullUp
    rt_pin_mode(pin, PIN_MODE_INPUT_PULLUP);

    // 3.attach irq handler
    rt_pin_attach_irq(pin, PIN_IRQ_MODE_RISING_FALLING,
                      test_gpio_input_irq_handler, &pin);

    // 4.enable pin irq
    rt_pin_irq_enable(pin, PIN_IRQ_ENABLE);

    return pin;
}

static u32 test_gpio_output_pin(char *arg_pin)
{
    // 1.get pin number
    u32 pin = test_gpio_pin_check(arg_pin, GPIO_SET_MODE_FLAG);

    // 2.set pin mode to Output
    rt_pin_mode(pin, PIN_MODE_OUTPUT);

    // 3.set pin High and Low
    for(int i = 0; i < 5; i++)
    {
        rt_pin_write(pin, PIN_LOW);
        rt_thread_mdelay(time_turnaround);
        rt_pin_write(pin, PIN_HIGH);
        rt_thread_mdelay(time_turnaround);
    }

    return pin;
}

int test_gpio(int argc, char **argv)
{
    int c = 0;
    int get_irq_result = -1;
    u32 input_pin = -1;
    if (argc < 2) {
        test_gpio_usage(argv[0]);
        return -1;
    }

    times_enter_irq = 0;
    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'i':
            input_pin = test_gpio_input_pin_cfg(optarg);
            get_irq_result++;
            break;
        case 'o':
            test_gpio_output_pin(optarg);
            get_irq_result++;
            break;
        case 'c':
            test_gpio_check_pin_status(optarg);
            break;
        case 't':
            time_turnaround = atoi(optarg);
            if (time_turnaround <= 0) {
                test_gpio_usage(argv[0]);
                return 0;
            }
            break;
        case 'h':
        default:
            test_gpio_usage(argv[0]);
            return 0;
        }
    }

    if ((get_irq_result == 1) && (times_enter_irq >= (5 * 2))) {
        if (input_pin >= 0) {
            rt_pin_detach_irq(input_pin);
            rt_pin_irq_enable(input_pin, PIN_IRQ_DISABLE);
        }
        return 0;
    } else {
        return -1;
    }

    return 0;
}

MSH_CMD_EXPORT(test_gpio, gpio device sample);
