/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: jiji.chen <jiji.chen@artinchip.com>
 */

#ifndef _AIC_HAL_SYSCFG_REGS_H_
#define _AIC_HAL_SYSCFG_REGS_H_
#include <aic_common.h>
#include <aic_soc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The register definition of SYSCFG V10 */
#define LDO30_CFG               (SYSCFG_BASE + 0x020)
#define LDO25_CFG               (SYSCFG_BASE + 0x024)
#define LDO1x_CFG               (SYSCFG_BASE + 0x028)
#define DDR_VREF                (SYSCFG_BASE + 0x040)
#define DDR_REXT                (SYSCFG_BASE + 0x044)
#define USB0_REXT               (SYSCFG_BASE + 0x048)
#define USB1_REXT               (SYSCFG_BASE + 0x04C)
#define EPHY_REXT               (SYSCFG_BASE + 0x050)
#define PSEN_CFG                (SYSCFG_BASE + 0x0C0)
#define PSEN_CNT_VAL            (SYSCFG_BASE + 0x0C4)
#define SYS_SRAM_PAR            (SYSCFG_BASE + 0x100)
#define CPU_SRAM_PAR            (SYSCFG_BASE + 0x104)
#define DDR_SRAM_PAR            (SYSCFG_BASE + 0x108)
#define VE_SRAM_PAR             (SYSCFG_BASE + 0x10C)
#define GE_SRAM_PAR             (SYSCFG_BASE + 0x110)
#define SRAM_CLK_CFG            (SYSCFG_BASE + 0x140)
#define USB0_CFG                (SYSCFG_BASE + 0x40C)
#define GMAC0_CFG               (SYSCFG_BASE + 0x410)
#define GMAC1_CFG               (SYSCFG_BASE + 0x414)
#define EPHY_CFG0               (SYSCFG_BASE + 0x418)
#define EPHY_CFG1               (SYSCFG_BASE + 0x41C)
#define DBG_CFG                 (SYSCFG_BASE + 0xF00)
#define SYSCFG_VER              (SYSCFG_BASE + 0xFFC)

/* The field definition of LDO30_CFG */
#define LDO30_CFG_ATB_ANA_EN                    BIT(27)
#define LDO30_CFG_ATB_ANA_SEL_MASK              GENMASK(26, 24)
#define LDO30_CFG_ATB_ANA_SEL_SHIFT             (24)
#define LDO30_CFG_ATB_BIAS_EN                   BIT(22)
#define LDO30_CFG_ATB_BIAS_SEL_MASK             GENMASK(21, 20)
#define LDO30_CFG_ATB_BIAS_SEL_SHIFT            (20)
#define LDO30_CFG_LVDS0_IBIAS_EN                BIT(18)
#define LDO30_CFG_LVDS1_IBIAS_EN                BIT(17)
#define LDO30_CFG_BAK_IBIAS_EN                  BIT(16)
#define LDO30_CFG_BG_CTRL_MASK                  GENMASK(15, 8)
#define LDO30_CFG_BG_CTRL_SHIFT                 (8)
#define LDO30_CFG_RTC_VAL_SEL_MASK              GENMASK(6, 4)
#define LDO30_CFG_RTC_VAL_SEL_SHIFT             (4)
#define LDO30_CFG_LDO_AVCC_EN                   BIT(3)
#define LDO30_CFG_LDO_VAL_MASK                  GENMASK(2, 0)
#define LDO30_CFG_LDO_VAL_SHIFT                 (0)

/* The field definition of LDO25_CFG */
#define LDO25_CFG_ATB_ANA_EN                    BIT(27)
#define LDO25_CFG_ATB_ANA_SEL_MASK              GENMASK(26, 24)
#define LDO25_CFG_ATB_ANA_SEL_SHIFT             (24)
#define LDO25_CFG_ATB_BIAS_EN                   BIT(22)
#define LDO25_CFG_ATB_BIAS_SEL_MASK             GENMASK(21, 20)
#define LDO25_CFG_ATB_BIAS_SEL_SHIFT            (20)
#define LDO25_CFG_BG_CTRL_MASK                  GENMASK(15, 8)
#define LDO25_CFG_BG_CTRL_SHIFT                 (8)
#define LDO25_CFG_LDO25_EN                      BIT(3)
#define LDO25_CFG_LDO25_VAL_MASK                GENMASK(2, 0)
#define LDO25_CFG_LDO25_VAL_SHIFT               (0)

/* The field definition of LDO1x_CFG */
#define LDO1x_CFG_LDO1X_VAL_FB_MASK             GENMASK(6, 4)
#define LDO1x_CFG_LDO1X_VAL_FB_SHIFT            (4)
#define LDO1x_CFG_LDO1X_EN                      BIT(3)
#define LDO1x_CFG_LDO1X_VAL_MASK                GENMASK(2, 0)
#define LDO1x_CFG_LDO1X_VAL_SHIFT               (0)

/* The field definition of DDR_VREF */
#define DDR_VREF_RES1_SEL_MASK                  GENMASK(11, 8)
#define DDR_VREF_RES1_SEL_SHIFT                 (8)
#define DDR_VREF_RES0_SEL_MASK                  GENMASK(3, 0)
#define DDR_VREF_RES0_SEL_SHIFT                 (0)

/* The field definition of DDR_REXT */
#define DDR_REXT_RES_CAL_EN                     BIT(8)
#define DDR_REXT_RES_CAL_VAL_MASK               GENMASK(7, 0)
#define DDR_REXT_RES_CAL_VAL_SHIFT              (0)

/* The field definition of USB0_REXT */
#define USB0_REXT_RES_CAL_EN                    BIT(8)
#define USB0_REXT_RES_CAL_VAL_MASK              GENMASK(7, 0)
#define USB0_REXT_RES_CAL_VAL_SHIFT             (0)
#define USB0_REXT_RES_EN_VAL(v)                 (((v) << USB0_REXT_RES_CAL_VAL_SHIFT) & USB0_REXT_RES_CAL_VAL_MASK)

/* The field definition of USB1_REXT */
#define USB1_REXT_RES_CAL_EN                    BIT(8)
#define USB1_REXT_RES_CAL_VAL_MASK              GENMASK(7, 0)
#define USB1_REXT_RES_CAL_VAL_SHIFT             (0)
#define USB1_REXT_RES_EN_VAL(v)                 (((v) << USB1_REXT_RES_CAL_VAL_SHIFT) & USB1_REXT_RES_CAL_VAL_MASK)

/* The field definition of EPHY_REXT */
#define EPHY_REXT_RES_CAL_EN                    BIT(8)
#define EPHY_REXT_RES_CAL_VAL_MASK              GENMASK(7, 0)
#define EPHY_REXT_RES_CAL_VAL_SHIFT             (0)

/* The field definition of PSEN_CFG */
#define PSEN_CFG_CNT_TIME_MASK                  GENMASK(31, 16)
#define PSEN_CFG_CNT_TIME_SHIFT                 (16)
#define PSEN_CFG_RO_SEL_MASK                    GENMASK(3, 1)
#define PSEN_CFG_RO_SEL_SHIFT                   (1)
#define PSEN_CFG_PSEN_START                     BIT(0)

/* The field definition of PSEN_CNT_VAL */
#define PSEN_CNT_VAL_CNT_VAL_MASK               GENMASK(15, 0)
#define PSEN_CNT_VAL_CNT_VAL_SHIFT              (0)

/* The field definition of SYS_SRAM_PAR */
#define SYS_SRAM_PAR_SRAM_PAR_MASK              GENMASK(31, 0)
#define SYS_SRAM_PAR_SRAM_PAR_SHIFT             (0)

/* The field definition of CPU_SRAM_PAR */
#define CPU_SRAM_PAR_SRAM_PAR_MASK              GENMASK(31, 0)
#define CPU_SRAM_PAR_SRAM_PAR_SHIFT             (0)

/* The field definition of DDR_SRAM_PAR */
#define DDR_SRAM_PAR_SRAM_PAR_MASK              GENMASK(31, 0)
#define DDR_SRAM_PAR_SRAM_PAR_SHIFT             (0)

/* The field definition of VE_SRAM_PAR */
#define VE_SRAM_PAR_SRAM_PAR_MASK               GENMASK(31, 0)
#define VE_SRAM_PAR_SRAM_PAR_SHIFT              (0)

/* The field definition of GE_SRAM_PAR */
#define GE_SRAM_PAR_SRAM_PAR_MASK               GENMASK(31, 0)
#define GE_SRAM_PAR_SRAM_PAR_SHIFT              (0)

/* The field definition of SRAM_CLK_CFG */
#define SRAM_CLK_CFG_SRAM_CLK_UNGATE_MASK       GENMASK(15, 0)
#define SRAM_CLK_CFG_SRAM_CLK_UNGATE_SHIFT      (0)

/* The field definition of USB0_CFG */
#define USB0_CFG_DRD_MODE                       BIT(0)

/* The field definition of GMAC0_CFG */
#define GMAC0_CFG_GMAC_REFCLK_INV               BIT(29)
#define GMAC0_CFG_GMAC_REFDLY_SEL_MASK          GENMASK(28, 24)
#define GMAC0_CFG_GMAC_REFDLY_SEL_SHIFT         (24)
#define GMAC0_CFG_GMAC_RXCLK_INV                BIT(23)
#define GMAC0_CFG_GMAC_RXDLY_SEL_MASK           GENMASK(22, 18)
#define GMAC0_CFG_GMAC_RXDLY_SEL_SHIFT          (18)
#define GMAC0_CFG_GMAC_RXDLY_EN_VAL(v)          (((v) << GMAC0_CFG_GMAC_RXDLY_SEL_SHIFT) & GMAC0_CFG_GMAC_RXDLY_SEL_MASK)
#define GMAC0_CFG_GMAC_TXCLK_INV                BIT(17)
#define GMAC0_CFG_GMAC_TXDLY_SEL_MASK           GENMASK(16, 12)
#define GMAC0_CFG_GMAC_TXDLY_SEL_SHIFT          (12)
#define GMAC0_CFG_GMAC_TXDLY_EN_VAL(v)          (((v) << GMAC0_CFG_GMAC_TXDLY_SEL_SHIFT) & GMAC0_CFG_GMAC_TXDLY_SEL_MASK)
#define GMAC0_CFG_SW_TXCLK_DIV2_MASK            GENMASK(11, 8)
#define GMAC0_CFG_SW_TXCLK_DIV2_SHIFT           (8)
#define GMAC0_CFG_SW_TXCLK_DIV1_MASK            GENMASK(7, 4)
#define GMAC0_CFG_SW_TXCLK_DIV1_SHIFT           (4)
#define GMAC0_CFG_SW_TXCLK_DIV_EN               BIT(2)
#define GMAC0_CFG_RMII_EXTCLK_SEL               BIT(1)
#define GMAC0_CFG_PHY_RGMII_SEL                 BIT(0)

/* The field definition of GMAC1_CFG */
#define GMAC1_CFG_GMAC_REFCLK_INV               BIT(29)
#define GMAC1_CFG_GMAC_REFDLY_SEL_MASK          GENMASK(28, 24)
#define GMAC1_CFG_GMAC_REFDLY_SEL_SHIFT         (24)
#define GMAC1_CFG_GMAC_RXCLK_INV                BIT(23)
#define GMAC1_CFG_GMAC_RXDLY_SEL_MASK           GENMASK(22, 18)
#define GMAC1_CFG_GMAC_RXDLY_SEL_SHIFT          (18)
#define GMAC1_CFG_GMAC_RXDLY_EN_VAL(v)          (((v) << GMAC1_CFG_GMAC_RXDLY_SEL_SHIFT) & GMAC1_CFG_GMAC_RXDLY_SEL_MASK)
#define GMAC1_CFG_GMAC_TXCLK_INV                BIT(17)
#define GMAC1_CFG_GMAC_TXDLY_SEL_MASK           GENMASK(16, 12)
#define GMAC1_CFG_GMAC_TXDLY_SEL_SHIFT          (12)
#define GMAC1_CFG_GMAC_TXDLY_EN_VAL(v)          (((v) << GMAC1_CFG_GMAC_TXDLY_SEL_SHIFT) & GMAC1_CFG_GMAC_TXDLY_SEL_MASK)
#define GMAC1_CFG_SW_TXCLK_DIV2_MASK            GENMASK(11, 8)
#define GMAC1_CFG_SW_TXCLK_DIV2_SHIFT           (8)
#define GMAC1_CFG_SW_TXCLK_DIV1_MASK            GENMASK(7, 4)
#define GMAC1_CFG_SW_TXCLK_DIV1_SHIFT           (4)
#define GMAC1_CFG_SW_TXCLK_DIV_EN               BIT(2)
#define GMAC1_CFG_RMII_EXTCLK_SEL               BIT(1)
#define GMAC1_CFG_PHY_RGMII_SEL                 BIT(0)

/* The field definition of EPHY_CFG0 */
#define EPHY_CFG0_LED1_CYCLE_HIGH_MASK          GENMASK(31, 28)
#define EPHY_CFG0_LED1_CYCLE_HIGH_SHIFT         (28)
#define EPHY_CFG0_LED0_CYCLE_HIGH_MASK          GENMASK(27, 24)
#define EPHY_CFG0_LED0_CYCLE_HIGH_SHIFT         (24)
#define EPHY_CFG0_LED1_MODE                     BIT(9)
#define EPHY_CFG0_LED0_MODE                     BIT(8)
#define EPHY_CFG0_LED1_SEL_MASK                 GENMASK(6, 4)
#define EPHY_CFG0_LED1_SEL_SHIFT                (4)
#define EPHY_CFG0_LED0_SEL_MASK                 GENMASK(2, 0)
#define EPHY_CFG0_LED0_SEL_SHIFT                (0)

/* The field definition of EPHY_CFG1 */
#define EPHY_CFG1_LED1_CYCLE_LOW_MASK           GENMASK(31, 16)
#define EPHY_CFG1_LED1_CYCLE_LOW_SHIFT          (16)
#define EPHY_CFG1_LED0_CYCLE_LOW_MASK           GENMASK(15, 0)
#define EPHY_CFG1_LED0_CYCLE_LOW_SHIFT          (0)

/* The field definition of DBG_CFG */
#define DBG_CFG_DBG_KEY_MASK                    GENMASK(31, 16)
#define DBG_CFG_DBG_KEY_SHIFT                   (16)
#define DBG_CFG_DBG_MODE_SEL_MASK               GENMASK(15, 14)
#define DBG_CFG_DBG_MODE_SEL_SHIFT              (14)
#define DBG_CFG_DBG_CLK_SEL_MASK                GENMASK(13, 8)
#define DBG_CFG_DBG_CLK_SEL_SHIFT               (8)
#define DBG_CFG_DBG_OUT_SEL_MASK                GENMASK(7, 0)
#define DBG_CFG_DBG_OUT_SEL_SHIFT               (0)

/* The field definition of SYSCFG_VER */
#define SYSCFG_VER_VERSION_MASK                 GENMASK(31, 0)
#define SYSCFG_VER_VERSION_SHIFT                (0)

#define REFERENCE_VOLTAGE           28000
#define VOLTAGE_SPACING             500

static inline u32 syscfg_hw_read_ldo_cfg(void)
{
    u32 ldo30_val;

    ldo30_val = readl(LDO30_CFG);
    ldo30_val &= LDO30_CFG_LDO_VAL_MASK;

    return ldo30_val * VOLTAGE_SPACING + REFERENCE_VOLTAGE;
}

static inline void syscfg_hw_usb_init(u32 res_val)
{
    s32 __attribute__((unused)) val;

#if defined(AIC_USING_USB0_HOST) || defined(AIC_USING_USB0_OTG) || defined(AICUPG_UDISK_ENABLE)
    val = readl(USB0_REXT);
    val &= ~USB0_REXT_RES_CAL_VAL_MASK;
    val |= USB0_REXT_RES_EN_VAL(res_val);
    val |= USB0_REXT_RES_CAL_EN;
    writel(val, USB0_REXT);
#endif

#if defined(AIC_USING_USB1_HOST) || defined(AIC_USING_USB1_OTG) || defined(AICUPG_UDISK_ENABLE)
    val = readl(USB1_REXT);
    val &= ~USB1_REXT_RES_CAL_VAL_MASK;
    val |= USB1_REXT_RES_EN_VAL(res_val);
    val |= USB1_REXT_RES_CAL_EN;
    writel(val, USB1_REXT);
#endif
}

static inline void syscfg_hw_usb_phy0_set_host(void)
{
    u32 val;

    val = readl(USB0_CFG);
    val &= ~USB0_CFG_DRD_MODE;
    writel(val, USB0_CFG);
}

static inline void syscfg_hw_usb_phy0_set_device(void)
{
    u32 val;

    val = readl(USB0_CFG);
    val |= USB0_CFG_DRD_MODE;
    writel(val, USB0_CFG);
}

static inline void syscfg_hw_gmac0_init(void)
{
    u32 val;

    val = readl(GMAC0_CFG);

#ifdef AIC_DEV_GMAC0_RGMII
    val |= GMAC0_CFG_PHY_RGMII_SEL;
#else
    val &= ~GMAC0_CFG_PHY_RGMII_SEL;
#endif

#ifdef AIC_DEV_GMAC0_PHY_EXTCLK
    val |= GMAC0_CFG_RMII_EXTCLK_SEL;
#endif
#if AIC_DEV_GMAC0_TXDELAY
    val |= GMAC0_CFG_GMAC_TXDLY_EN_VAL(AIC_DEV_GMAC0_TXDELAY);
#endif
#if AIC_DEV_GMAC0_RXDELAY
    val |= GMAC0_CFG_GMAC_RXDLY_EN_VAL(AIC_DEV_GMAC0_RXDELAY);
#endif

    writel(val, GMAC0_CFG);
}

static inline void syscfg_hw_gmac1_init(void)
{
    u32 val;

    val = readl(GMAC1_CFG);

#ifdef AIC_DEV_GMAC1_RGMII
    val |= GMAC1_CFG_PHY_RGMII_SEL;
#else
    val &= ~GMAC1_CFG_PHY_RGMII_SEL;
#endif

#ifdef AIC_DEV_GMAC1_PHY_EXTCLK
    val |= GMAC1_CFG_RMII_EXTCLK_SEL;
#endif
#if AIC_DEV_GMAC1_TXDELAY
    val |= GMAC1_CFG_GMAC_TXDLY_EN_VAL(AIC_DEV_GMAC1_TXDELAY);
#endif
#if AIC_DEV_GMAC1_RXDELAY
    val |= GMAC1_CFG_GMAC_RXDLY_EN_VAL(AIC_DEV_GMAC1_RXDELAY);
#endif

    writel(val, GMAC1_CFG);
}


static inline void syscfg_hw_gmac_init(u32 gmac)
{
    if (gmac == 0) {
        syscfg_hw_gmac0_init();
    } else if (gmac == 1) {
        syscfg_hw_gmac1_init();
    }
}

#ifdef __cplusplus
}
#endif

#endif
