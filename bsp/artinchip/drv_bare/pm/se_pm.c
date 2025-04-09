/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */

#include <aic_core.h>
#include <aic_drv_irq.h>
#include <aic_clk_id.h>

#define PRCM_SE_WAKEUP_ADDR         0x88000100
extern void se_save_context_and_suspend();
extern void se_restore_context_and_resume();

void aic_se_suspend_and_resume(void)
{
    uint8_t i;
    uint32_t se_save_vic_iser[4] = {0};
    uint32_t se_save_context[36] = {0};

    /* save resume code address to PRCM */
    writel(&se_restore_context_and_resume, (void *)PRCM_SE_WAKEUP_ADDR);

    for (i = 0; i < 4; i++) {
        se_save_vic_iser[i] = VIC->ISER[i];
        /* disable all enabled interrupt */
        if (se_save_vic_iser[i])
            VIC->ISER[i] = 0;
    }

    se_save_context_and_suspend(&se_save_context);

    /* wakeup flow */
    /* restore vic interrupt enable */
    for (i = 0; i < 4; i++)
        VIC->ISER[i] = se_save_vic_iser[i];

    csi_icache_enable();
    csi_dcache_enable();
    /* clear resume code address in PRCM */
    writel(0, (void *)PRCM_SE_WAKEUP_ADDR);

    csi_coret_config(drv_get_sys_freq() / CONFIG_SYSTICK_HZ, 0);
}


