/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <drivers/pin.h>
#include <aic_core.h>
#include <aic_drv.h>
#include "rtconfig.h"
#include <realtek_bt_drv.h>

unsigned int bt_power_pin = 0;
unsigned int bt_reset_pin = 0;
unsigned int bt_wake_pin = 0;
unsigned int bt_rts_pin = 0;
unsigned int bt_cts_pin = 0;

#ifndef AIC_WIRELESS_PWR_GPIO
int realtek_bt_power_on(void)
{
    unsigned int g;
    unsigned int p;

    g = GPIO_GROUP(bt_power_pin);
    p = GPIO_GROUP_PIN(bt_power_pin);

    /* power on */
    hal_gpio_set_value(g, p, 1);
    aicos_msleep(10);

    return 0;
}

int realtek_bt_power_off(void)
{
    unsigned int g;
    unsigned int p;

    g = GPIO_GROUP(bt_power_pin);
    p = GPIO_GROUP_PIN(bt_power_pin);

    /* power off */
    hal_gpio_set_value(g, p, 0);
    aicos_msleep(200);

    return 0;
}
#endif

int realtek_bt_reset(void)
{
    unsigned int g;
    unsigned int p;

    if (bt_reset_pin < 0)
        return -1;

    g = GPIO_GROUP(bt_reset_pin);
    p = GPIO_GROUP_PIN(bt_reset_pin);

    /* reset at least 200ms low */
    hal_gpio_set_value(g, p, 0);
    aicos_msleep(300);
    hal_gpio_set_value(g, p, 1);
    aicos_msleep(10);

    return 0;
}

int realtek_bt_rts_on(void)
{
    unsigned int g;
    unsigned int p;

    g = GPIO_GROUP(bt_rts_pin);
    p = GPIO_GROUP_PIN(bt_rts_pin);

    /* rts on */
    hal_gpio_set_value(g, p, 1);

    return 0;
}

int realtek_bt_rts_off(void)
{
    unsigned int g;
    unsigned int p;

    g = GPIO_GROUP(bt_rts_pin);
    p = GPIO_GROUP_PIN(bt_rts_pin);

    /* rts off */
    hal_gpio_set_value(g, p, 0);

    return 0;
}

int realtek_bt_cts_on(void)
{
    unsigned int g;
    unsigned int p;

    g = GPIO_GROUP(bt_cts_pin);
    p = GPIO_GROUP_PIN(bt_cts_pin);

    /* power on */
    hal_gpio_set_value(g, p, 1);
    aicos_msleep(10);

    return 0;
}

int realtek_bt_wake_on(void)
{
    unsigned int g;
    unsigned int p;

    g = GPIO_GROUP(bt_wake_pin);
    p = GPIO_GROUP_PIN(bt_wake_pin);

    /* power on */
    hal_gpio_set_value(g, p, 1);
    aicos_msleep(10);

    return 0;
}

int realtek_bt_pin_init(void)
{
#ifndef AIC_WIRELESS_PWR_GPIO
    bt_power_pin = hal_gpio_name2pin(AIC_DEV_REALTEK_BT_PWR_GPIO);
    if (bt_power_pin < 0) {
        pr_err("Not found bt power(%s) pin.\n", AIC_DEV_REALTEK_BT_PWR_GPIO);
        return -1;
    }
    hal_gpio_direction_output(GPIO_GROUP(bt_power_pin), GPIO_GROUP_PIN(bt_power_pin));
    realtek_bt_power_on();
#endif

    bt_reset_pin = hal_gpio_name2pin(AIC_DEV_REALTEK_BT_RST_GPIO);
    if (bt_reset_pin < 0) {
        pr_err("Not found bt reset(%s) pin.\n", AIC_DEV_REALTEK_BT_RST_GPIO);
        return -1;
    }
    hal_gpio_direction_output(GPIO_GROUP(bt_reset_pin), GPIO_GROUP_PIN(bt_reset_pin));
    realtek_bt_reset();

    bt_cts_pin = hal_gpio_name2pin(AIC_DEV_REALTEK_BT_CTS_GPIO);
    if (bt_cts_pin < 0) {
        pr_err("Not found bt cts(%s) pin.\n", AIC_DEV_REALTEK_BT_CTS_GPIO);
        return -1;
    }
    hal_gpio_direction_output(GPIO_GROUP(bt_cts_pin), GPIO_GROUP_PIN(bt_cts_pin));
    realtek_bt_cts_on();

    bt_rts_pin = hal_gpio_name2pin(AIC_DEV_REALTEK_BT_RTS_GPIO);
    if (bt_rts_pin < 0) {
        pr_err("Not found bt rts(%s) pin.\n", AIC_DEV_REALTEK_BT_RTS_GPIO);
        return -1;
    }
    hal_gpio_direction_output(GPIO_GROUP(bt_rts_pin), GPIO_GROUP_PIN(bt_rts_pin));

    bt_wake_pin = hal_gpio_name2pin(AIC_DEV_REALTEK_BT_WAKE_GPIO);
    if (bt_wake_pin < 0) {
        pr_err("Not found bt wake(%s) pin.\n", AIC_DEV_REALTEK_BT_WAKE_GPIO);
        return -1;
    }
    hal_gpio_direction_output(GPIO_GROUP(bt_wake_pin), GPIO_GROUP_PIN(bt_wake_pin));
    realtek_bt_wake_on();

    return 0;
}
