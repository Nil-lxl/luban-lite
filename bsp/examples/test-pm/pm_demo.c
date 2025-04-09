/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
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

#define BUTTON_FLAG         (1 << 0)
#define TOUCH_FLAG          (1 << 1)
#define TOUCH_TIMEOUT       (1 << 2)
struct rt_event pm_event;
rt_timer_t touch_timer;

void pm_key_irq_callback(void *args)
{
    rt_event_send(&pm_event, BUTTON_FLAG);
}

static void pm_thread(void *parameter)
{
    rt_uint8_t current_mode, touch_int_occurred = 0;
    rt_uint32_t e;

    while (1)
    {
        if (rt_event_recv(&pm_event, (BUTTON_FLAG | TOUCH_FLAG | TOUCH_TIMEOUT),
                    RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                    RT_WAITING_FOREVER, &e) == RT_EOK)
        {
            current_mode = rt_pm_get_sleep_mode();

            if (current_mode != PM_SLEEP_MODE_NONE)
            {
                /* current mode is sleep mode, so execute exit sleep flow */
                if ((e & BUTTON_FLAG) ||
                    ((e & TOUCH_FLAG) && !touch_int_occurred))
                {
                    rt_pm_module_request(PM_POWER_ID, PM_SLEEP_MODE_NONE);
                    rt_timer_start(touch_timer);
                    if (e & TOUCH_FLAG)
                        touch_int_occurred = 1;
                }

            }
            else
            {
                /* current mode is NONE mode */
                if ((e & BUTTON_FLAG) || (e & TOUCH_TIMEOUT))
                {
                    /* request sleep */
                    rt_pm_module_release(PM_POWER_ID, PM_SLEEP_MODE_NONE);
                    rt_timer_stop(touch_timer);
                }
                else if (e & TOUCH_FLAG)
                {
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
}

int pm_demo(void)
{
    rt_err_t ret;
    rt_thread_t thread;

    pm_key_init();
    touch_timer_init();

    ret = rt_event_init(&pm_event, "pm_event", RT_IPC_FLAG_PRIO);
    if (ret != RT_EOK)
    {
        rt_kprintf("init pm_event failed\n");
        return -1;
    }

    thread = rt_thread_create("pm_thread", pm_thread, RT_NULL,
                                  2048, 30, 10);
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        rt_kprintf("create pm thread failed!\n");
        ret = -RT_ERROR;
    }

    return 0;
}

INIT_ENV_EXPORT(pm_demo);
