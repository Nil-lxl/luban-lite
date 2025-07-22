/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
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
#include "sp_ddr3_init.h"

uint64_t sleep_counter;
uint64_t resume_counter;

extern void aic_suspend_resume();
extern u32 aic_suspend_resume_size;
static void (*aic_suspend_resume_fn)();
extern size_t __sram_start;

#define PRCM_DDR_WAKEUP_STATUS              0x88000108
#define CMU_APB0_REG                        0x98020120
#define CMU_APB2_REG                        0x98020128
#define SRAM_SUSPEND_RESUME_START           0x30040000

void aic_pm_enter_idle(void)
{
    __WFI();
}

void aic_ddr_sr_code_on_ddr(void)
{
    pr_debug("aic_suspend_resume_size: %d\n", aic_suspend_resume_size);
    pr_debug("suspend_resume_addr: 0x%08x\n", SRAM_SUSPEND_RESUME_START);
    pr_debug("__sram_start: 0x%08x\n", (uint32_t)&__sram_start);

    aic_suspend_resume_fn = (void *)SRAM_SUSPEND_RESUME_START;

    rt_memcpy((void *)SRAM_SUSPEND_RESUME_START,
              aic_suspend_resume, aic_suspend_resume_size);

    aicos_icache_invalid();
    aicos_dcache_clean_invalid();
    aic_suspend_resume_fn();
}

void aic_pm_enter_deep_sleep(void)
{
    rt_base_t level;
    uint32_t i, reg_val;
    uint32_t cmu_pll_freq[5];
    uint32_t cmu_bus_freq[3];

    level = rt_hw_interrupt_disable();
    /*
     * After VDD1.1 power domain reset, the CMU will also reset.
     * So save the CMU pll and bus register value before the VDD1.1 domain
     * power down.
     */
    for (i = 0; i < ARRAY_SIZE(cmu_pll_freq); i++)
        cmu_pll_freq[i] = hal_clk_get_freq(CLK_AP_PLL_INT0 + i);

    for (i = 0; i < ARRAY_SIZE(cmu_bus_freq); i++)
        cmu_bus_freq[i] = hal_clk_get_freq(CLK_AP_AXI + i);

    /* change r_cpu frequency to 24M */
    hal_clk_set_parent(CLK_CPU, CLK_24M);

    /* Indicate DDR will enter self-refresh */
    reg_val = readl(PRCM_DDR_WAKEUP_STATUS);
    reg_val |= 1;
    writel(reg_val, PRCM_DDR_WAKEUP_STATUS);
#ifdef FPGA_BOARD_ARTINCHIP
    #define CMU_AP_CPU0_REG             0x18020180
    #define CMU_AP_CPU1_REG             0x18020190
    #define CMU_WR_CFG_REG              0x18020FE8

    // reset AP CPU0
    writel(0xA1C00180, CMU_WR_CFG_REG);
    while (!(readl(CMU_WR_CFG_REG) & 0x10000))
        continue;
    writel(0, CMU_AP_CPU0_REG);
    // reset AP CPU1
    writel(0xA1C00190, CMU_WR_CFG_REG);
    while (!(readl(CMU_WR_CFG_REG) & 0x10000))
        continue;
    writel(0, CMU_AP_CPU1_REG);
#endif
    /* reset all pins */
    //TO DO
    aic_ddr_sr_code_on_ddr();

    /* wakeup flow */
    aicos_icache_invalid();
    aicos_dcache_clean_invalid();

    /* restore CMU pll and bus freqency */
    for (i = 0; i < ARRAY_SIZE(cmu_pll_freq); i++)
        hal_clk_set_freq(CLK_AP_PLL_INT0 + i, cmu_pll_freq[i]);

    for (i = 0; i < ARRAY_SIZE(cmu_bus_freq); i++)
        hal_clk_set_freq(CLK_AP_AXI + i, cmu_bus_freq[i]);

    /* indicate DDR has exited self-refresh and is ready */
    reg_val = readl(PRCM_DDR_WAKEUP_STATUS);
    reg_val &= ~1;
    writel(reg_val, PRCM_DDR_WAKEUP_STATUS);
#ifdef FPGA_BOARD_ARTINCHIP
    // #define CMU_AP_CPU0_REG             0x18020180
    // #define CMU_AP_CPU1_REG             0x18020190
    // #define CMU_WR_CFG_REG              0x18020FE8

    // release AP CPU0
    writel(0xA1C00180, CMU_WR_CFG_REG);
    while (!(readl(CMU_WR_CFG_REG) & 0x10000))
        continue;
    writel(0x01010000, CMU_AP_CPU0_REG);
    // release AP CPU1
    writel(0xA1C00190, CMU_WR_CFG_REG);
    while (!(readl(CMU_WR_CFG_REG) & 0x10000))
        continue;
    writel(0x01010000, CMU_AP_CPU1_REG);
#endif
    /* change cpu frequency to pll */
    hal_clk_set_parent(CLK_CPU, CLK_PLL_INT0);
    rt_pm_request(PM_SLEEP_MODE_NONE);
    rt_hw_interrupt_enable(level);
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
    uint64_t delta_counter;
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
    NULL,
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

    timer_mask = 1UL << PM_SLEEP_MODE_DEEP;
    /* initialize system pm module */
    rt_system_pm_init(&aic_pm_ops, timer_mask, RT_NULL);

    return 0;
}
INIT_BOARD_EXPORT(aic_pm_hw_init);
