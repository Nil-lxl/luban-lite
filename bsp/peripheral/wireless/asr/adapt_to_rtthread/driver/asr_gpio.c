#include "asr_types.h"
#include "asr_sdio.h"
#include "asr_dbg.h"
#include "asr_rtos_api.h"
#include <drivers/pin.h>
#include <aic_log.h>
#include <aic_hal_gpio.h>

extern asr_sbus g_asr_sbus_drvdata;
unsigned int wifi_reset_pin = 0;

int asr_gpio_init(void)
{
    wifi_reset_pin = hal_gpio_name2pin(AIC_DEV_ASR_RST_GPIO);
    if (wifi_reset_pin > 0)
        hal_gpio_direction_output(GPIO_GROUP(wifi_reset_pin),
                                  GPIO_GROUP_PIN(wifi_reset_pin));

    return 0;
}

int asr_wlan_gpio_power(int on)
{
    pr_debug("%s: power-pins=%d,set %s\n",
        __func__, g_asr_sbus_drvdata.gpio_power, on?"on":"off");

    return kNoErr;
}

void asr_wlan_set_gpio_reset_pin(int on)
{
    pr_debug("%s: reset-pins=%d,set %s\n",
        __func__, g_asr_sbus_drvdata.gpio_power, on?"on":"off");

    unsigned int g;
    unsigned int p;

    /* power on pin */
    g = GPIO_GROUP(wifi_reset_pin);
    p = GPIO_GROUP_PIN(wifi_reset_pin);

    /* reset */
    hal_gpio_set_value(g, p, on);
}

int asr_wlan_gpio_reset(void)
{
    pr_debug("%s: reset-pins=%d,reset success.\n",
        __func__, g_asr_sbus_drvdata.gpio_reset);

    return kNoErr;
}
