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
#include "syscfg_hw_fpga.h"

#if defined(AIC_SYSCFG_DRV_V10)
#include "syscfg_hw_v1.0.h"
#elif defined(AIC_SYSCFG_DRV_V11)
#include "syscfg_hw_v1.1.h"
#elif defined(AIC_SYSCFG_DRV_V12)
#include "syscfg_hw_v1.2.h"
#elif defined(AIC_SYSCFG_DRV_V20)
#include "syscfg_hw_v2.0.h"
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
#ifndef AIC_SYSCFG_DRV_V20
    if (host_mode)
        syscfg_hw_usb_phy0_set_host();
    else
        syscfg_hw_usb_phy0_set_device();
#endif
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

#ifndef AIC_SYSCFG_DRV_V20
static void syscfg_gmac_init(void)
{
    u32 i;
    for (i = 0; i < ARRAY_SIZE(gmac_init_table); i++) {
        syscfg_hw_gmac_init(gmac_init_table[i]);
    }
}
#endif  /* nodef AIC_SYSCFG_DRV_V20 */
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

static s32 syscfg_fpga_drp_wr(u8 addr, u16 data)
{
    syscfg_hw_mmcm2_reset_start();

    aicos_udelay(20);
    syscfg_hw_mmcm2_data_write(addr, data);
    aicos_udelay(20);

    syscfg_hw_mmcm2_reset_end();

    return 0;
}

static u16 syscfg_fpga_drp_rd(u16 addr)
{
    syscfg_hw_mmcm2_addr_write(addr);

    return syscfg_hw_mmcm2_sta_data_read();
}

static s32 syscfg_fpga_cfg_vco(u32 freq)
{
    u8  cntr;
    u16 data;

    cntr = freq / 2;
    if ((freq % 2) == 0)
        data = 1 << 12 | cntr << 6 | cntr;
    else
        data = 1 << 12 | cntr << 6 | (cntr + 1);

    if (cntr > 0) {
        syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_VCO_M_CTL0, data);
        if (syscfg_fpga_drp_rd(FPGA_MMCM_DADDR_VCO_M_CTL0) != data)
            return -1;
    } else {
        syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_VCO_M_CTL1, 0x40);
    }

    return 0;
}

s32 syscfg_fpga_de_clk_sel_by_div(u8 sclk, u8 pixclk)
{
    u8  cntr;
    u16 data;

    cntr = sclk / 2;
    data = (1 << 12) | (cntr << 6) | cntr;

    if (cntr > 0) {
        syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_CLKOUT2_CTL0, data);
        if (syscfg_fpga_drp_rd(FPGA_MMCM_DADDR_CLKOUT2_CTL0) != data)
            return -1;
    } else {
        syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_CLKOUT2_CTL1, 0x40);
    }

    cntr = pixclk / 2;
    data = (1 << 12) | (cntr << 6) | cntr;

    if (cntr > 0) {
        syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_CLKOUT3_CTL0, data);
        if (syscfg_fpga_drp_rd(FPGA_MMCM_DADDR_CLKOUT3_CTL0) != data)
            return -1;
    } else {
        syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_CLKOUT3_CTL1, 0x40);
    }

    return 0;
}

static s32 syscfg_fpga_de_clk_sel(enum fpga_disp_clk type)
{
#if !defined(CONFIG_AIC_DISP_V12) && !defined(CONFIG_AIC_DISP_V11)
    u8 sclk_div[] = { 10,  8,  4,  4,  4,  4,  4,  4,  4,  4,  3,  3,   8,
        3,  6,   6,   3,   6,  1,   2,   1,  2,  4,  6};
    u8 pixclk_div[] = {120, 32, 28, 14, 24, 18, 16, 32, 48, 96, 30, 30, 120,
        60, 60, 120,  90, 120, 96, 128, 108, 72, 96, 96};
#else
    u8 sclk_div[] = {20,  16,  4,  4,  4,  4,  4,  4,  4,  4,  3,  3,   8,
        3,  6,   6,   4,   6,  1,   2,   1,  2,  4,  6};
    u8 pixclk_div[] = {120, 32, 28, 14, 24, 18, 16, 32, 48, 96, 30, 30, 120,
        60, 60, 120, 120, 120, 96, 128, 108, 72, 96, 96};
#endif

    if (type > 17)
        syscfg_fpga_cfg_vco(3);

    if (type == FPGA_DISP_CLK_MIPI_2LANE_RGB888)
        syscfg_fpga_cfg_vco(12);

    return syscfg_fpga_de_clk_sel_by_div(sclk_div[type], pixclk_div[type]);
}

#ifndef AIC_SYSCFG_DRV_V12
static void syscfg_fpga_gmac_clk_sel(u32 id)
{
    u8 fpga_mmcm2_div_gmac_clk[] = {40,  8};
    u8  cntr;
    u16 data;

    cntr = fpga_mmcm2_div_gmac_clk[id] / 2;
    data = 1 << 12 | cntr << 6 | cntr;

    syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_CLKOUT0_CTL0, data);
    if (syscfg_fpga_drp_rd(FPGA_MMCM_DADDR_CLKOUT0_CTL0) != data)
        return;

    syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_CLKOUT1_CTL0, data);
    if (syscfg_fpga_drp_rd(FPGA_MMCM_DADDR_CLKOUT1_CTL0) != data)
        return;
}
#endif

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
#ifndef AIC_SYSCFG_DRV_V20
    syscfg_gmac_init();
#endif
#endif

#if defined(AIC_SYSCFG_DRV_V11) && defined(AIC_XSPI_DRV)
    syscfg_ldo25_xspi_init();
#endif

#if defined(AIC_SYSCFG_DRV_V11) || defined(AIC_SYSCFG_DRV_V12)
    syscfg_ldo1x_init();
#endif

    syscfg_fpga_de_clk_sel(FPGA_DISP_CLK_RGB_PARALLEL);
#ifndef AIC_SYSCFG_DRV_V12
    //If use GMAC, set to 125M
    syscfg_fpga_gmac_clk_sel(FPGA_GMAC_CLK_25M);
#endif

    return 0;
}
