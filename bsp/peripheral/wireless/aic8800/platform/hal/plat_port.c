/*
 * Copyright (C) 2018-2024 AICSemi Ltd.
 *
 * All Rights Reserved
 */
#include <drivers/pin.h>
#include <aic_log.h>
#include <aic_hal_gpio.h>

unsigned int wifi_pwrkey_pin = 0;
unsigned int wifi_rstkey_pin = 0;
unsigned int bt_rstkey_pin = 0;

static void platform_pwr_wifi_pin_set(int on)
{
    if (wifi_pwrkey_pin > 0) {
        hal_gpio_set_pin_value(wifi_pwrkey_pin, on);
    }
}

void platform_pwr_wifi_pin_init(void)
{
    wifi_pwrkey_pin = hal_gpio_name2pin(AIC_WIRELESS_PWR_GPIO);
    if (wifi_pwrkey_pin > 0) {
        hal_gpio_direction_output(GPIO_GROUP(wifi_pwrkey_pin),
                                  GPIO_GROUP_PIN(wifi_pwrkey_pin));
    }
}

void platform_pwr_wifi_pin_enable(void)
{
    platform_pwr_wifi_pin_set(1);
}

void platform_pwr_wifi_pin_disable(void)
{
    platform_pwr_wifi_pin_set(0);
}

static void platform_rst_wifi_pin_set(int on)
{
    if (wifi_rstkey_pin > 0) {
        hal_gpio_set_pin_value(wifi_rstkey_pin, on);
    }
}

void platform_rst_wifi_pin_init(void)
{
    wifi_rstkey_pin = hal_gpio_name2pin(AIC_DEV_AIC8800_WLAN0_RST_GPIO);
    if (wifi_rstkey_pin > 0) {
        hal_gpio_direction_output(GPIO_GROUP(wifi_rstkey_pin),
                                  GPIO_GROUP_PIN(wifi_rstkey_pin));
    }
}

void platform_rst_wifi_pin_enable(void)
{
    platform_rst_wifi_pin_set(0);
}

void platform_rst_wifi_pin_disable(void)
{
    platform_rst_wifi_pin_set(1);
}

static void platform_rst_bt_pin_set(int on)
{
    if (bt_rstkey_pin > 0) {
        hal_gpio_set_pin_value(bt_rstkey_pin, on);
    }
}

void platform_rst_bt_pin_init(void)
{
    bt_rstkey_pin = hal_gpio_name2pin(AIC_DEV_AIC8800_BT_RST_GPIO);
    if (bt_rstkey_pin > 0) {
        hal_gpio_direction_output(GPIO_GROUP(bt_rstkey_pin),
                                  GPIO_GROUP_PIN(bt_rstkey_pin));
    }
}

void platform_rst_bt_pin_enable(void)
{
    platform_rst_bt_pin_set(1);
}

void platform_rst_bt_pin_disable(void)
{
    platform_rst_bt_pin_set(0);
}

