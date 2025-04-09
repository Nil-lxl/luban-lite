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

extern void Default_Handler(void);
extern void * g_irqvector[MAX_IRQ_ENTRY];
/* A bitmap indicating which pin is the wakeup source */
static uint32_t rt_pm_pin_wakeup_source[GPIO_GROUP_MAX] = {0};

void rt_pm_set_pin_wakeup_source(rt_base_t pin)
{
    rt_pm_pin_wakeup_source[pin >> 5] |= (1 << (pin % GPIO_GROUP_SIZE));
}

void rt_pm_clear_pin_wakeup_source(rt_base_t pin)
{
    rt_pm_pin_wakeup_source[pin >> 5] &= ~(1 << (pin % GPIO_GROUP_SIZE));
}

void rt_pm_disable_pin_irq_nonwakeup(void)
{
    int index, offset;
    uint32_t tmp;

    /* disable pin irq */
    for (index = 0; index < ARRAY_SIZE(rt_pm_pin_wakeup_source); index++)
    {
        if (rt_pm_pin_wakeup_source[index] == 0)
        {
            hal_gpio_group_set_irq_en(index, 0);
        }
        else
        {
            tmp = rt_pm_pin_wakeup_source[index];

            for (offset = 0; offset < GPIO_GROUP_SIZE; offset++)
            {
                if (!(tmp & 1))
                    hal_gpio_disable_irq(index, offset);

                tmp >>= 1;
            }
        }
    }
}

void rt_pm_resume_pin_irq(void)
{
    int index, pin_name, group, pin;

    /* Enable pin irq */
    for (index = MAX_IRQn; index < MAX_IRQ_ENTRY; index++)
    {
        if (g_irqvector[index] && g_irqvector[index] != Default_Handler)
        {
            pin_name = AIC_IRQ_TO_GPIO(index);
            group = GPIO_GROUP(pin_name);
            pin = GPIO_GROUP_PIN(pin_name);
            hal_gpio_enable_irq(group, pin);
        }
    }
}
