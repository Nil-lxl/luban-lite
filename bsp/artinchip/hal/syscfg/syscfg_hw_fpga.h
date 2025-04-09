/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: jiji.chen <jiji.chen@artinchip.com>
 */

#ifndef _AIC_HAL_SYSCFG_FPGA_REGS_H_
#define _AIC_HAL_SYSCFG_FPGA_REGS_H_
#include <aic_common.h>
#include <aic_soc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MMCM2_CTL                               (SYSCFG_BASE + 0x0F0)
#define FPGA_MMCM2_STA                          (SYSCFG_BASE + 0x0F4)

/* The field definition of MMCM2_CTL */
#define MMCM2_CTL_DRP_DIN_MASK                  GENMASK(31, 16)
#define MMCM2_CTL_DRP_DIN_SHIFT                 (16)
#define MMCM2_CTL_DRP_DADDR_MASK                GENMASK(15, 9)
#define MMCM2_CTL_DRP_DADDR_SHIFT               (9)
#define MMCM2_CTL_DRP_DWE                       BIT(8)
#define MMCM2_CTL_DRP_START                     BIT(4)
#define MMCM2_CTL_DRP_SEL_MASK                  GENMASK(2, 1)
#define MMCM2_CTL_DRP_SEL_SHIFT                 (1)
#define MMCM2_CTL_DRP_RESET                     BIT(0)

#define MMCM2_CTL_DRP_DIN_VAL(v)                (((v) << MMCM2_CTL_DRP_DIN_SHIFT) & MMCM2_CTL_DRP_DIN_MASK)
#define MMCM2_CTL_DRP_DADDR_VAL(v)              (((v) << MMCM2_CTL_DRP_DADDR_SHIFT) & MMCM2_CTL_DRP_DADDR_MASK)

/* The field definition of FPGA_MMCM2_STA */
#define FPGA_MMCM2_STA_DRP_START_MASK           GENMASK(31, 16)
#define FPGA_MMCM2_STA_DRP_START_SHIFT          (16)


enum fpga_disp_clk {
    FPGA_DISP_CLK_RGB_SERIAL            = 0,
    FPGA_DISP_CLK_RGB_PARALLEL          = 1,
    FPGA_DISP_CLK_LVDS_SINGLE           = 2,
    FPGA_DISP_CLK_LVDS_DUAL             = 3,
    FPGA_DISP_CLK_MIPI_4LANE_RGB888     = 4,
    FPGA_DISP_CLK_MIPI_4LANE_RGB666     = 5,
    FPGA_DISP_CLK_MIPI_4LANE_RGB565     = 6,
    FPGA_DISP_CLK_MIPI_3LANE_RGB888     = 7,
    FPGA_DISP_CLK_MIPI_2LANE_RGB888     = 8,
    FPGA_DISP_CLK_MIPI_1LANE_RGB888     = 9,
    FPGA_DISP_CLK_I8080_24BIT           = 10,
    FPGA_DISP_CLK_I8080_18BIT           = 11,
    FPGA_DISP_CLK_I8080_16BIT_666_1     = 12,
    FPGA_DISP_CLK_I8080_16BIT_666_2     = 13,
    FPGA_DISP_CLK_I8080_16BIT_565       = 14,
    FPGA_DISP_CLK_I8080_9BIT            = 15,
    FPGA_DISP_CLK_I8080_8BIT_666        = 16,
    FPGA_DISP_CLK_I8080_8BIT_565        = 17,
    FPGA_DISP_CLK_SPI_4LINE_RGB888_OR_RGB666 = 18,
    FPGA_DISP_CLK_SPI_4LINE_RG565       = 19,
    FPGA_DISP_CLK_SPI_3LINE_RGB888_OR_RGB666 = 20,
    FPGA_DISP_CLK_SPI_3LINE_RG565       = 21,
    FPGA_DISP_CLK_SPI_4SDA_RGB888_OR_RGB666  = 22,
    FPGA_DISP_CLK_SPI_4SDA_RGB565       = 23,
};

enum fpga_mmcm_daddr {
    FPGA_MMCM_DADDR_CLKOUT0_CTL0        = 0x8,
    FPGA_MMCM_DADDR_CLKOUT0_CTL1        = 0x9,
    FPGA_MMCM_DADDR_CLKOUT1_CTL0        = 0xA,
    FPGA_MMCM_DADDR_CLKOUT1_CTL1        = 0xB,
    FPGA_MMCM_DADDR_CLKOUT2_CTL0        = 0xC,
    FPGA_MMCM_DADDR_CLKOUT2_CTL1        = 0xD,
    FPGA_MMCM_DADDR_CLKOUT3_CTL0        = 0xE,
    FPGA_MMCM_DADDR_CLKOUT3_CTL1        = 0xF,
    FPGA_MMCM_DADDR_CLKOUT4_CTL0        = 0x10,
    FPGA_MMCM_DADDR_CLKOUT4_CTL1        = 0x11,
    FPGA_MMCM_DADDR_CLKOUT5_CTL0        = 0x06,
    FPGA_MMCM_DADDR_CLKOUT5_CTL1        = 0x07,
    FPGA_MMCM_DADDR_CLKOUT6_CTL0        = 0x12,
    FPGA_MMCM_DADDR_CLKOUT6_CTL1        = 0x13,
    FPGA_MMCM_DADDR_VCO_M_CTL0          = 0x14,
    FPGA_MMCM_DADDR_VCO_M_CTL1          = 0x15,
};

enum fpga_gmac_clk_t {
    FPGA_GMAC_CLK_25M           = 0,
    FPGA_GMAC_CLK_125M          = 1,
};

static inline void syscfg_hw_mmcm2_reset_start(void)
{
    u32 val;

    val = readl(MMCM2_CTL);
    val |= MMCM2_CTL_DRP_RESET;

    writel(val, MMCM2_CTL);
}

static inline void syscfg_hw_mmcm2_reset_end(void)
{
    u32 val;

    val = readl(MMCM2_CTL);
    val &= ~MMCM2_CTL_DRP_RESET;

    writel(val, MMCM2_CTL);
}

static inline void syscfg_hw_mmcm2_data_write(u32 addr, u32 data)
{
    u32 val = 0;

    val |= MMCM2_CTL_DRP_DIN_VAL(data);
    val |= MMCM2_CTL_DRP_DADDR_VAL(addr);
    val |= MMCM2_CTL_DRP_DWE;
    val |= MMCM2_CTL_DRP_START;
    val |= MMCM2_CTL_DRP_RESET;

    writel(val, MMCM2_CTL);
    while (readl(MMCM2_CTL) & MMCM2_CTL_DRP_START) {
        ;
    }
}

static inline void syscfg_hw_mmcm2_addr_write(u16 addr)
{

    u32 val = readl(MMCM2_CTL);

    val &= ~MMCM2_CTL_DRP_DWE;
    val &= ~MMCM2_CTL_DRP_DADDR_MASK;
    val |= (addr << MMCM2_CTL_DRP_DADDR_SHIFT)
            | MMCM2_CTL_DRP_START;

    writel(val, MMCM2_CTL);
    while (readl(MMCM2_CTL) & MMCM2_CTL_DRP_START) {
        ;
    }
}

static inline u16 syscfg_hw_mmcm2_sta_data_read()
{

    u32 val = readl(FPGA_MMCM2_STA);

    return val >> FPGA_MMCM2_STA_DRP_START_SHIFT;
}

static inline void syscfg_hw_wait(u32 addr, u32 data)
{
    u32 val = 0;

    val |= MMCM2_CTL_DRP_DIN_VAL(data);
    val |= MMCM2_CTL_DRP_DADDR_VAL(addr);
    val |= MMCM2_CTL_DRP_DWE;
    val |= MMCM2_CTL_DRP_START;
    val |= MMCM2_CTL_DRP_RESET;

    writel(val, MMCM2_CTL);
}

#ifdef __cplusplus
}
#endif

#endif
