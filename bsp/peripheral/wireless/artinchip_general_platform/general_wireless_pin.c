/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: lv.wu <lv.wu@artinchip.com>
 */

#include "rtconfig.h"
#include "rtthread.h"
#include <aic_core.h>
#include <aic_drv.h>

int aic_platform_wlan_hw_reset(void)
{
    unsigned int g;
    unsigned int p;
    int pin = 0;

    pin = hal_gpio_name2pin(AIC_WIRELESS_PWR_GPIO);
    if (pin < 0)
        return -1;

    /* power on pin */
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    /* reset */
    hal_gpio_set_value(g, p, 0);
    aicos_msleep(10);
    hal_gpio_set_value(g, p, 1);
    aicos_msleep(10);

    return 0;
}

int aic_platform_wlan_power_on(void)
{
    unsigned int g;
    unsigned int p;
    int pin = 0;

    pin = hal_gpio_name2pin(AIC_WIRELESS_PWR_GPIO);
    if (pin < 0)
        return -1;

    /* power on pin */
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    /* power on */
    hal_gpio_set_value(g, p, 1);
    aicos_msleep(10);

    return 0;
}

int aic_platform_wlan_power_off(void)
{
    unsigned int g;
    unsigned int p;
    int pin = 0;

    pin = hal_gpio_name2pin(AIC_WIRELESS_PWR_GPIO);
    if (pin < 0)
        return -1;

    /* power on pin */
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    /* power off */
    hal_gpio_set_value(g, p, 0);
    aicos_msleep(10);

    return 0;
}

int aic_platform_bt_hw_reset(void)
{
    unsigned int g;
    unsigned int p;
    int pin = 0;

    pin = hal_gpio_name2pin(AIC_WIRELESS_BT_PWR_GPIO);
    if (pin < 0)
        return -1;

    /* power on pin */
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    /* reset */
    hal_gpio_set_value(g, p, 0);
    aicos_msleep(10);
    hal_gpio_set_value(g, p, 1);
    aicos_msleep(10);

    return 0;
}

int aic_platform_bt_power_on(void)
{
    unsigned int g;
    unsigned int p;
    int pin = 0;

    pin = hal_gpio_name2pin(AIC_WIRELESS_BT_PWR_GPIO);
    if (pin < 0)
        return -1;

    /* power on pin */
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    /* power on */
    hal_gpio_set_value(g, p, 1);
    aicos_msleep(10);

    return 0;
}

int aic_platform_bt_power_off(void)
{
    unsigned int g;
    unsigned int p;
    int pin = 0;

    pin = hal_gpio_name2pin(AIC_WIRELESS_BT_PWR_GPIO);
    if (pin < 0)
        return -1;

    /* power on pin */
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    /* power off */
    hal_gpio_set_value(g, p, 0);
    aicos_msleep(10);

    return 0;
}

static int aic_platform_wireless_pin_init(void)
{
    unsigned int g;
    unsigned int p;
    int pin = 0;

    /* init wifi wk host pin */
    pin = hal_gpio_name2pin(AIC_WIRELESS_WIFI_WK_HOST_GPIO);
    if (pin > 0) {
        g = GPIO_GROUP(pin);
        p = GPIO_GROUP_PIN(pin);
        hal_gpio_direction_output(g, p);
        hal_gpio_set_value(g, p, 0);
        aicos_msleep(10);
        hal_gpio_set_value(g, p, 1);
        aicos_msleep(10);
    } else {
        LOG_E("Failed to get wifi wk host pin, invalid GPIO: %s, pin[%d]",
                  AIC_WIRELESS_WIFI_WK_HOST_GPIO, pin);
    }

    /* init wifi pwr pin */
    pin = hal_gpio_name2pin(AIC_WIRELESS_PWR_GPIO);
    if (pin > 0) {
        g = GPIO_GROUP(pin);
        p = GPIO_GROUP_PIN(pin);
        hal_gpio_direction_output(g, p);
        hal_gpio_set_value(g, p, 0);
        aicos_msleep(10);
        hal_gpio_set_value(g, p, 1);
        aicos_msleep(10);
    } else {
        LOG_E("Failed to get wifi pwr pin, invalid GPIO: %s, pin[%d]",
                  AIC_WIRELESS_PWR_GPIO, pin);
    }

    /* init BT pwr pin */
    pin = hal_gpio_name2pin(AIC_WIRELESS_BT_PWR_GPIO);
    if (pin > 0) {
        g = GPIO_GROUP(pin);
        p = GPIO_GROUP_PIN(pin);
        hal_gpio_direction_output(g, p);
        hal_gpio_set_value(g, p, 0);
        aicos_msleep(10);
        hal_gpio_set_value(g, p, 1);
        aicos_msleep(10);
    } else {
        LOG_E("Failed to get BT pwr pin, invalid GPIO: %s, pin[%d]",
                  AIC_WIRELESS_BT_PWR_GPIO, pin);
    }

    return 0;
}

INIT_PREV_EXPORT(aic_platform_wireless_pin_init);
