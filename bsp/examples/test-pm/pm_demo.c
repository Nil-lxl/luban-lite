/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */

#include <stdio.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <aic_core.h>
#include <aic_drv.h>
#include <string.h>
#include <aic_osal.h>
#include <hal_rtc.h>
#include <aic_drv_gpio.h>
#if defined(AIC_PM_INDEPENDENT_POWER_KEY) && defined(AIC_DISPLAY_DRV)
#include <drv_fb.h>
#endif

#define BUTTON_FLAG         (1 << 0)
#define TOUCH_FLAG          (1 << 1)
#define TOUCH_TIMEOUT       (1 << 2)
struct rt_event pm_event;
rt_timer_t touch_timer;
static rt_bool_t sleep_req = RT_FALSE;

#ifdef AIC_PM_DRV_V15
static volatile rt_uint8_t use_deep_sleep_mode = 0;
#endif

#if defined(AIC_RTC_DRV_V121) && defined(AIC_PM_DRV_V15)
int pm_rtc_io_irq_callback(void)
{
    rt_uint8_t current_mode = rt_pm_get_sleep_mode();

    if (current_mode == PM_SLEEP_MODE_NONE)
        use_deep_sleep_mode = 1;

    rt_event_send(&pm_event, BUTTON_FLAG);
    return 0;
}
#endif

void pm_key_irq_callback(void *args)
{
    rt_event_send(&pm_event, BUTTON_FLAG);
}

extern volatile uint32_t stress_test_running;

static void pm_thread(void *parameter)
{
    rt_uint8_t current_mode, touch_int_occurred = 0;
    rt_uint32_t e;

    while (1)
    {
        if (rt_event_recv(&pm_event, (BUTTON_FLAG | TOUCH_FLAG | TOUCH_TIMEOUT),
                    RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                    RT_WAITING_FOREVER, &e) == RT_EOK) {
            current_mode = rt_pm_get_sleep_mode();

            if (current_mode != PM_SLEEP_MODE_NONE) {
                /* current mode is sleep mode, so execute exit sleep flow */
                if ((e & BUTTON_FLAG) ||
                    ((e & TOUCH_FLAG) && !touch_int_occurred)) {
                    rt_pm_module_request(PM_POWER_ID, PM_SLEEP_MODE_NONE);
                    #if defined(AIC_PM_INDEPENDENT_POWER_KEY) && defined(AIC_DISPLAY_DRV)
                    panel_backlight_enable(0, 0);
                    #endif
                    rt_timer_start(touch_timer);
                    if (e & TOUCH_FLAG)
                        touch_int_occurred = 1;

                    sleep_req = RT_FALSE;
                }
            } else {
                /* current mode is NONE mode */
                if (((e & BUTTON_FLAG) || (e & TOUCH_TIMEOUT)) && !stress_test_running) {
                    if (!sleep_req) {
                        #if defined(AIC_PM_INDEPENDENT_POWER_KEY) && defined(AIC_DISPLAY_DRV)
                        panel_backlight_disable(0, 0);
                        #endif

                        #if defined(AIC_RTC_DRV_V121) && defined(AIC_PM_DRV_V15)
                        unsigned long level;
                        /* Set the default sleep mode based on the flag */
                        aicos_local_irq_save(&level);
                        if (use_deep_sleep_mode)
                            rt_pm_default_set(PM_SLEEP_MODE_DEEP);
                        else
                            rt_pm_default_set(PM_SLEEP_MODE_LIGHT);
                        aicos_local_irq_restore(level);
                        #endif

                        /* request sleep */
                        rt_pm_module_release(PM_POWER_ID, PM_SLEEP_MODE_NONE);
                        rt_timer_stop(touch_timer);

                        sleep_req = RT_TRUE;
                    }
                } else if (e & TOUCH_FLAG) {
                    /* There is a click on the screen to reset the timer */
                    rt_timer_start(touch_timer);
                }
                touch_int_occurred = 0;
            }
        }
    }
}

static void touch_timer_timeout(void *parameter)
{
    rt_event_send(&pm_event, TOUCH_TIMEOUT);
}

int touch_timer_init(void)
{
    rt_tick_t timeout;

    if (!AIC_PM_POWER_TOUCH_TIME_SLEEP)
        timeout = RT_TICK_MAX / 2 - 1;
    else
        timeout = AIC_PM_POWER_TOUCH_TIME_SLEEP * RT_TICK_PER_SECOND;

    touch_timer = rt_timer_create("tp_timer", touch_timer_timeout, RT_NULL,
                                  timeout, RT_TIMER_FLAG_PERIODIC);

    if (touch_timer)
        rt_timer_start(touch_timer);

    return 0;
}

void pm_key_init(void)
{
    rt_base_t pin;
    unsigned int g, p;

    pin = rt_pin_get(AIC_PM_POWER_KEY_GPIO);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_set_drive_strength(g, p, 3);
    hal_gpio_set_debounce(g, p, 0xFFF);

    rt_pin_mode(pin, PIN_MODE_INPUT_PULLUP);

    rt_pin_attach_irq(pin, PIN_IRQ_MODE_FALLING, pm_key_irq_callback, RT_NULL);
    rt_pin_irq_enable(pin, PIN_IRQ_ENABLE);
    /* Set AIC_PM_POWER_KEY_GPIO pin as wakeup source */
    rt_pm_set_pin_wakeup_source(pin);

    /* Register the pin to PM framework */
    gpio_pm_register(pin, RT_NULL, RT_NULL);
}

static void pm_demo_notify_callback(rt_uint8_t event, rt_uint8_t pm_mode, void *data)
{
    /* When waking up from sleep, reset the default mode to LIGHT */
    if (event == RT_PM_EXIT_SLEEP)
    {
        sleep_req = RT_FALSE;
        #if defined(AIC_RTC_DRV_V121) && defined(AIC_PM_DRV_V15)
        unsigned long level;
        aicos_local_irq_save(&level);
        use_deep_sleep_mode = 0;
        aicos_local_irq_restore(level);
        rt_pm_default_set(PM_SLEEP_MODE_LIGHT);
        #endif
    }
}
#if defined(AIC_RTC_DRV_V121) && defined(AIC_PM_DRV_V15)
void pm_rtc_io_init(void)
{
#define RTC_CTL_IO0_WAKE_HIZ_SLEEP_LOW  3
#define RTC_IO1_TRIG_RISING_EDGE        3
    rt_uint32_t en = 1;

    /* config the rtc io */
    hal_rtc_io_cfg(RTC_CTL_IO0_WAKE_HIZ_SLEEP_LOW);
    /* config the rtc io1 */
    hal_rtc_io1_cfg(en, en, RTC_IO1_TRIG_RISING_EDGE);
    /* regist the rtc io1 callback */
    hal_rtc_io1_register_callback(pm_rtc_io_irq_callback);
    /* enable the rtc io1 irq */
    hal_rtc_io1_irq_en(en);
}
#endif

int pm_demo(void)
{
    rt_err_t ret;
    rt_thread_t thread;

    pm_key_init();
    rt_pm_notify_set(pm_demo_notify_callback, RT_NULL);
#if defined(AIC_RTC_DRV_V121) && defined(AIC_PM_DRV_V15)
    rt_device_t rtc_dev = rt_device_find("rtc");
    if (rtc_dev == NULL) {
        rt_kprintf("can't find rtc device!\n");
        return -RT_ERROR;
    }
    ret = rt_device_init(rtc_dev);
    if (ret != RT_EOK) {
        rt_kprintf("Failed to open rtc device!\n");
        return ret;
    }
    pm_rtc_io_init();
#endif
    touch_timer_init();

    ret = rt_event_init(&pm_event, "pm_event", RT_IPC_FLAG_PRIO);
    if (ret != RT_EOK) {
        rt_kprintf("init pm_event failed\n");
        return -RT_ERROR;
    }

    thread = rt_thread_create("pm_thread", pm_thread, RT_NULL,
                                  2048, 30, 10);
    if (thread != RT_NULL) {
        rt_thread_startup(thread);
    } else {
        rt_kprintf("create pm thread failed!\n");
        ret = -RT_ERROR;
    }

    return 0;
}

INIT_ENV_EXPORT(pm_demo);
