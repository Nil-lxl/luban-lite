/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "aic_hal_clk.h"
#include "hal_efuse.h"
#include "hal_syscfg.h"

#if defined(AIC_SYSCFG_DRV_V11)
#include "syscfg_hw_v1.1.h"
#elif defined(AIC_SYSCFG_DRV_V12)
#include "syscfg_hw_v1.2.h"
#else
#include "syscfg_hw_v1.0.h"
#endif

#define RES_CAL_VAL_DEF             0x40

#define GMAC_INIT_NUM0              0
#define GMAC_INIT_NUM1              1

u32 hal_syscfg_read_ldo_cfg(void)
{
    return syscfg_hw_read_ldo_cfg();
}

void hal_syscfg_usb_phy0_sw_host(s32 host_mode)
{
#ifndef AIC_SYSCFG_DRV_V12
    if (host_mode)
        syscfg_hw_usb_phy0_set_host();
    else
        syscfg_hw_usb_phy0_set_device();
#endif
}

#ifndef AIC_SYSCFG_DRV_V12
static s32 syscfg_usb_init(void)
{
    syscfg_hw_usb_init(RES_CAL_VAL_DEF);

    return 0;
}

const u32 gmac_init_table[] = {
#ifdef AIC_USING_GMAC0
    GMAC_INIT_NUM0,
#endif
#ifdef AIC_USING_GMAC1
    GMAC_INIT_NUM1,
#endif
};

static void syscfg_gmac_init(void)
{
    u32 i;
    for (i = 0; i < ARRAY_SIZE(gmac_init_table); i++) {
        syscfg_hw_gmac_init(gmac_init_table[i]);
    }
}
#endif  /* nodef AIC_SYSCFG_DRV_V12 */

static void syscfg_sip_flash_init(void)
{
#if defined(AIC_SYSCFG_SIP_FLASH_ENABLE)
    syscfg_hw_sip_flash_init();
#endif
}

#if defined(AIC_SYSCFG_DRV_V11) && defined(AIC_XSPI_DRV)
/* ldo25 BIT16, BIT17 ctrl the XSPI */
static void syscfg_ldo25_xspi_init(void)
{
    syscfg_hw_ldo25_xspi_init();
}
#endif

#if defined(AIC_SYSCFG_DRV_V11) || defined(AIC_SYSCFG_DRV_V12)
static void syscfg_ldo1x_init(void)
{
#ifdef AIC_SYSCFG_LDO1X_ENABLE
    syscfg_hw_ldo1x_enable(AIC_SYSCFG_LDO1X_VOL_VAL);
#else
    syscfg_hw_ldo1x_disable();
#endif

    aicos_udelay(100);
}
#endif /* AIC_SYSCFG_DRV_V10 */

s32 hal_syscfg_probe(void)
{
    s32 ret = 0;

    ret = hal_clk_enable(CLK_SYSCFG);
    if (ret < 0)
        return -1;

    ret = hal_clk_enable_deassertrst(CLK_SYSCFG);
    if (ret < 0)
        return -1;

    syscfg_sip_flash_init();
#ifndef AIC_SYSCFG_DRV_V12
    syscfg_usb_init();
    syscfg_gmac_init();
#endif

#if defined(AIC_SYSCFG_DRV_V11) && defined(AIC_XSPI_DRV)
    syscfg_ldo25_xspi_init();
#endif

#if defined(AIC_SYSCFG_DRV_V11) || defined(AIC_SYSCFG_DRV_V12)
    syscfg_ldo1x_init();
#endif

    return 0;
}
