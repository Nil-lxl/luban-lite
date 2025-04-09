/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao.Li <siyao.li@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <rtthread.h>
#include "rtdevice.h"
#include "aic_core.h"
#include "aic_hal_gpio.h"

#define TWINKLE_DEFAULT_PIN       "PC.6"
#define TWINKLE_DEFAULT_NUM       15

static const char sopts[] = "p:n:h";
static const struct option lopts[] = {
    {"pin",          optional_argument,  NULL, 'p'},
    {"number",       optional_argument,  NULL, 'n'},
    {"help",         no_argument,        NULL, 'h'},
    {0, 0, 0, 0}
};

static void test_gpio_usage(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -p, --pin\t\tSet the PIN as light-pin\n");
    printf("\t -n, --number\t\tSet the number of twinkle\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example:%s -p PC.6 -n 15\n", program);
    printf("\n");
    printf("The twinkle pin corresponding to board type:\n");
    printf("[PC.6]      d12x_demo68-nand/nor\n");
    printf("[PC.6]      d13x_demo88-nand/nor\n");
    printf("[PC.6]      d21x_demo128-nand\n");
    printf("[PA.5]      d13x_kunlunpi88-nor\n");
    printf("[PD.2]      d12x_hmi-nor\n");
}

static void test_twinkle_output_pin(char *arg_pin, int twinkle_num)
{
    unsigned int ret;
    // 1.get pin number
    u32 pin = rt_pin_get(arg_pin);

    // 2.set pin mode to Output
    rt_pin_mode(pin, PIN_MODE_OUTPUT);

    // 3.set pin High and Low
    for(int i = 0; i < twinkle_num; i++)
    {
        rt_pin_write(pin, PIN_LOW);
        rt_thread_mdelay(1000);
        hal_gpio_get_outcfg(GPIO_GROUP(pin), GPIO_GROUP_PIN(pin), &ret);
        printf("GPIO group_%d_pin_%d read = %d\n", GPIO_GROUP(pin),
                GPIO_GROUP_PIN(pin), ret);

        rt_pin_write(pin, PIN_HIGH);
        rt_thread_mdelay(1000);
        hal_gpio_get_outcfg(GPIO_GROUP(pin), GPIO_GROUP_PIN(pin), &ret);
        printf("GPIO group_%d_pin_%d read = %d\n", GPIO_GROUP(pin),
                GPIO_GROUP_PIN(pin), ret);
    }

}

int test_twinkle(int argc, char **argv)
{
    int c = 0;
    int twinkle_num = TWINKLE_DEFAULT_NUM;
    char *pin_name = TWINKLE_DEFAULT_PIN;

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'p':
            pin_name = optarg;
            break;
        case 'n':
            twinkle_num = atoi(optarg);
            break;
        case 'h':
        default:
            test_gpio_usage(argv[0]);
            return 0;
        }
    }

    test_twinkle_output_pin(pin_name, twinkle_num);

    return 0;
}

MSH_CMD_EXPORT(test_twinkle, twinkle sample);

