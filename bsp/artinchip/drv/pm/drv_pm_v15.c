/*
 * Copyright (c) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#include <stdio.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <aic_core.h>
#include <aic_drv.h>
#include <string.h>
#include <aic_osal.h>
#include <aic_utils.h>
#include <hal_rtc.h>
#include <boot_param.h>

uint64_t sleep_counter;
uint64_t resume_counter;

uint32_t save_context[38] = {0};
extern size_t __sram_pm_start;
extern void light_suspend_resume(uint32_t *context);
extern void save_context_and_suspend(uint32_t *context);
extern void restore_context_and_resume(void);
extern uint32_t light_suspend_resume_size;
extern uint32_t save_context_and_suspend_size;
extern uint32_t restore_context_and_resume_size;
static void (*aic_suspend_fn)(uint32_t *);
extern void aic_board_sysclk_init(void);
uint8_t g_deep_wakeup = 0;

RT_WEAK void rt_pm_board_level_power_off(void)
{
    return;
}

RT_WEAK void rt_pm_board_level_power_on(void)
{
    return;
}

void aic_pm_enter_idle(void)
{
    __WFI();
}

void aic_pm_enter_light_sleep(void)
{
    uint32_t val;

    /* disable PLL_FRA2: display pll */
    hal_clk_disable(CLK_PLL_FRA2);
    /* disable PLL_FRA1: audio pll */
    hal_clk_disable(CLK_PLL_FRA1);
    /* disable PLL_INT1: bsp pll */
    hal_clk_disable(CLK_PLL_INT1);
    /* Turn off board level power that can be controlled via GPIO */
    rt_pm_board_level_power_off();
    /* deinit all non-wakup pinmux configuration */
    aic_board_pinmux_deinit();
    /* change cpu frequency to 24M */
    hal_clk_set_parent(CLK_CPU, CLK_OSC24M);
    /* disable PLL_INT0: cpu pll */
    hal_clk_disable(CLK_PLL_INT0);
    /* disable usb ldo */
    *(volatile uint32_t *)(SYSCFG_BASE + 0x408) &= ~(1U << 0);
    /* disable the XTAL */
    val = readl(CMU_BASE + PLL_IN_REG);
    val &= ~(1 << 29);
    writel((0xA1C << 20) | PLL_IN_REG, cmu_reg(0xFE8));
    aic_mdelay(1);
    writel(val, CMU_BASE + PLL_IN_REG);

    rt_memcpy((void *)&__sram_pm_start, light_suspend_resume, light_suspend_resume_size);
    aic_suspend_fn = (void *)&__sram_pm_start;
    aicos_icache_invalid();
    aicos_dcache_clean_invalid();
    aic_suspend_fn((void *)save_context);

    /* wakeup flow */
    /* enable the XTAL */
    val = readl(CMU_BASE + PLL_IN_REG);
    val |= (1 << 29);
    writel((0xA1C << 20) | PLL_IN_REG, cmu_reg(0xFE8));
    aic_mdelay(1);
    writel(val, CMU_BASE + PLL_IN_REG);
    /* enable usb ldo*/
    *(volatile uint32_t *)(SYSCFG_BASE + 0x408) |= (1U << 0);
    /* enable PLL_INT0: cpu pll */
    hal_clk_enable(CLK_PLL_INT0);
    /* change cpu frequency to pll */
    hal_clk_set_parent(CLK_CPU, CLK_PLL_INT0);
    /* restore all pinmux configuration */
    aic_board_pinmux_init();
    /* Turn on board level power that can be controlled via GPIO */
    rt_pm_board_level_power_on();
    /* disable PLL_INT1: bsp pll */
    hal_clk_enable(CLK_PLL_INT1);
    /* enable PLL_FRA1: audio pll */
    hal_clk_enable(CLK_PLL_FRA1);
    /* enable PLL_FRA2: display pll */
    hal_clk_enable(CLK_PLL_FRA2);
}

void aic_pm_enter_deep_sleep(void)
{
    uint8_t save_irq_en[MAX_IRQn] = {0};
    uint32_t func_addr = 0x0;
    uint32_t i;

    /* set the wakeup addr */
    func_addr = (uint32_t)&restore_context_and_resume;
    hal_rtc_set_wakeup_addr(func_addr);

    /* save the interrupt status of each peripheral  */
    for (i = 0; i < MAX_IRQn; i++) {
        save_irq_en[i] = (uint8_t)csi_vic_get_enabled_irq(i);
        if (save_irq_en[i])
            aicos_irq_disable(i);
    }

    /* save the context and enter suspend */
    rt_memcpy((void *)&__sram_pm_start, save_context_and_suspend, save_context_and_suspend_size);
    aic_suspend_fn = (void *)&__sram_pm_start;
    aicos_icache_invalid();
    aicos_dcache_clean_invalid();
    aic_suspend_fn((void *)save_context);

    /* sysclk reinitialization */
    aic_board_sysclk_init();

    /* restore all pinmux configuration */
    aic_board_pinmux_init();

    /* get interrupt level from info */
    CLIC->CLICCFG = (((CLIC->CLICINFO & CLIC_INFO_CLICINTCTLBITS_Msk)
                    >> CLIC_INFO_CLICINTCTLBITS_Pos) << CLIC_CLICCFG_NLBIT_Pos);

    for (i = 0; i < MAX_IRQn; i++)
        CLIC->CLICINT[i].ATTR = 1; /* use vector interrupt */

    CLIC->CLICINT[Machine_Software_IRQn].ATTR = 0x3;

    /* restore the interrupt status of each peripheral  */
    for (i = 0; i < MAX_IRQn; i++) {
        if (save_irq_en[i])
            aicos_irq_enable(i);
    }

    /* reset the tick */
    uint64_t tmp_counter = ((uint64_t)csi_coret_get_valueh() << 32) |
                           csi_coret_get_value();

    uint32_t tick_resolution = drv_get_sys_freq() / CONFIG_SYSTICK_HZ;

    csi_coret_set_load(tmp_counter + tick_resolution);

    rt_pm_request(PM_SLEEP_MODE_NONE);
}

static void aic_sleep(struct rt_pm *pm, uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_NONE:
        break;
    case PM_SLEEP_MODE_IDLE:
        aic_pm_enter_idle();
        break;
    case PM_SLEEP_MODE_LIGHT:
        aic_pm_enter_light_sleep();
        break;
    case PM_SLEEP_MODE_DEEP:
        aic_pm_enter_deep_sleep();
        break;
    case PM_SLEEP_MODE_STANDBY:
        //TO DO
        break;
    case PM_SLEEP_MODE_SHUTDOWN:
        break;
    default:
        RT_ASSERT(0);
        break;
    }
}

static void aic_run(struct rt_pm *pm, rt_uint8_t mode)
{
    static rt_uint8_t prev_mode = 0;

    RT_ASSERT(pm != RT_NULL);

    if(prev_mode == mode)
    {
        return;
    }

    switch(mode)
    {
    case PM_RUN_MODE_HIGH_SPEED: /* 480MHz */
    case PM_RUN_MODE_NORMAL_SPEED:
        hal_clk_set_parent(CLK_CPU, CLK_PLL_INT0);
        hal_clk_set_freq(CLK_PLL_INT0, 480000000);
        break;

    case PM_RUN_MODE_MEDIUM_SPEED: /* 240Mhz */
        hal_clk_set_parent(CLK_CPU, CLK_PLL_INT0);
        hal_clk_set_freq(CLK_PLL_INT0, 240000000);
        break;

    case PM_RUN_MODE_LOW_SPEED: /* 24Mhz */
        hal_clk_set_parent(CLK_CPU, CLK_OSC24M);
        break;

    default:
        return;
    }
}

/* timeout unit is rt_tick_t, but MTIMECMPH/L unit is HZ
 * one tick is 4000 counter
 */
static void aic_timer_start(struct rt_pm *pm, rt_uint32_t timeout)
{
    uint64_t tmp_counter;
    uint32_t tick_resolution = drv_get_sys_freq() / CONFIG_SYSTICK_HZ;

    sleep_counter = ((uint64_t)csi_coret_get_valueh() << 32) |
                    csi_coret_get_value();
    tmp_counter = (uint64_t)timeout * tick_resolution;

    csi_coret_set_load(tmp_counter + sleep_counter);
}

static void aic_timer_stop(struct rt_pm *pm)
{
    uint64_t tmp_counter = ((uint64_t)csi_coret_get_valueh() << 32) |
                           csi_coret_get_value();

    uint32_t tick_resolution = drv_get_sys_freq() / CONFIG_SYSTICK_HZ;

    csi_coret_set_load(tmp_counter + tick_resolution);
}

static rt_tick_t aic_timer_get_tick(struct rt_pm *pm)
{
    rt_tick_t delta_tick;
    uint32_t delta_counter;
    uint32_t tick_resolution = drv_get_sys_freq() / CONFIG_SYSTICK_HZ;

    resume_counter = ((uint64_t)csi_coret_get_valueh() << 32) |
                     csi_coret_get_value();
    delta_counter = resume_counter - sleep_counter;

    delta_tick = delta_counter / tick_resolution;

    return delta_tick;
}

static const struct rt_pm_ops aic_pm_ops =
{
    aic_sleep,
    aic_run,
    aic_timer_start,
    aic_timer_stop,
    aic_timer_get_tick,
};

/**
 * This function initialize the power manager
 */
int aic_pm_hw_init(void)
{
    rt_uint8_t timer_mask = 0;

#ifdef AIC_PM_POWER_DEFAULT_LIGHT_MODE
    rt_pm_default_set(PM_SLEEP_MODE_LIGHT);
#endif
    timer_mask = (1UL << PM_SLEEP_MODE_LIGHT);
    /* initialize system pm module */
    rt_system_pm_init(&aic_pm_ops, timer_mask, RT_NULL);

    return 0;
}
INIT_BOARD_EXPORT(aic_pm_hw_init);

