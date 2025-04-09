/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <getopt.h>
#include <hal_cap.h>
#include "aic_hal_gpio.h"
#include <aic_core.h>
#include "aic_common.h"

static u32 g_gpio_group, g_gpio_pin;

static void cmd_hrtimer_usage(void)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: test_hrtimer [options]\n");
    printf("the hrtimer will timed toggle the %s as a demonstration\n", GPIO_TEST_PIN);
    printf("test_hrtimer <channel> <time> : Channel range [0, %d], the time unit is 1us\n", AIC_HRTIMER_CH_NUM - 1);
    printf("test_hrtimer help             : Get this help\n");
    printf("\n");
    printf("Example: test_hrtimer 0 20\n");
}

irqreturn_t hrtimer_irq_handler(int irq, void *args)
{
    u32 i;

    for (i = 0; i < AIC_HRTIMER_CH_NUM; i++) {
        if (hal_cap_is_pending(i))
            hal_gpio_toggle_output(g_gpio_group, g_gpio_pin);
    }

    return IRQ_HANDLED;
}

static void test_hrtimer_ch_init()
{
#ifdef AIC_USING_HRTIMER0
    hal_cap_ch_init(0);
#endif
#ifdef AIC_USING_HRTIMER1
    hal_cap_ch_init(1);
#endif
#ifdef AIC_USING_HRTIMER2
    hal_cap_ch_init(2);
#endif
#ifdef AIC_USING_HRTIMER3
    hal_cap_ch_init(3);
#endif
#ifdef AIC_USING_HRTIMER4
    hal_cap_ch_init(4);
#endif
#ifdef AIC_USING_HRTIMER5
    hal_cap_ch_init(5);
#endif
}

static void test_hrtimer_init(u32 ch)
{
    hal_cap_init();
    hal_cap_set_freq(ch, CAP_MAX_FREQ);
    aicos_request_irq(PWMCS_CAP_IRQn, hrtimer_irq_handler, 0, NULL, NULL);
}

static void test_hrtimer_start(u32 ch, u32 cnt)
{
    hal_cap_enable(ch);
    hal_cap_set_cnt(ch, cnt);
    hal_cap_int_enable(ch, 1);
    hal_cap_cnt_start(ch);
}

static void test_hrtimer_stop(u32 ch)
{
    hal_cap_cnt_stop(ch);
    hal_cap_int_enable(ch, 0);
    hal_cap_disable(ch);
}

static int cmd_test_hrtimer(int argc, char *argv[])
{
    u32 ch, time_us;
    u8 pin;

    if (argc != 3)
        goto cmd_usage;

    ch = atoi(argv[1]);

    if ((ch < 0) || (ch >= AIC_HRTIMER_CH_NUM))
        goto cmd_usage;

    time_us = atoi(argv[2]);

    if (time_us < 0)
        goto cmd_usage;

    /* gpio configuration*/
    pin = hal_gpio_name2pin(GPIO_TEST_PIN);
    if (pin < 0) {
        printf("please check the test output pin\n");
        return -1;
    }

    g_gpio_group = GPIO_GROUP(pin);
    g_gpio_pin = GPIO_GROUP_PIN(pin);
    hal_gpio_set_func(g_gpio_group, g_gpio_pin, 1);
    hal_gpio_direction_output(g_gpio_group, g_gpio_pin);

    /* hrtimer configuration */
    test_hrtimer_ch_init();
    test_hrtimer_init(ch);
    /* stop the hrtimer first */
    test_hrtimer_stop(ch);
    /* start the hrtimer */
    test_hrtimer_start(ch, time_us);

    return 0;

cmd_usage:
    cmd_hrtimer_usage();

    return -1;
}
CONSOLE_CMD(test_hrtimer, cmd_test_hrtimer, "Hrtimer test example");
