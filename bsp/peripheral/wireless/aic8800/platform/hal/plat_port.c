/*
 * Copyright (C) 2018-2024 AICSemi Ltd.
 *
 * All Rights Reserved
 */
#include <drivers/pin.h>
#include <aic_log.h>
#include <aic_hal_gpio.h>

unsigned int wifi_pwrkey_pin = 0;

static void platform_pwr_wifi_pin_set(int on)
{
    if (wifi_pwrkey_pin > 0) {
        unsigned int g;
        unsigned int p;

        /* power on pin */
        g = GPIO_GROUP(wifi_pwrkey_pin);
        p = GPIO_GROUP_PIN(wifi_pwrkey_pin);

        /* reset */
        hal_gpio_set_value(g, p, on);
    }
}

void platform_pwr_wifi_pin_init(void)
{
    wifi_pwrkey_pin = hal_gpio_name2pin(AIC_DEV_AIC8800_WLAN0_PWR_GPIO);
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

