/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_WRI_H_
#define _ARTINCHIP_HAL_WRI_H_

#include "aic_common.h"

/* Register of WRI */
#define WRI_RST_FLAG            (WRI_BASE + 0x0)
#define WRI_BOOT_INFO           (WRI_BASE + 0x100)
#define WRI_SYS_BAK             (WRI_BASE + 0x104)
#define WRI_VERSION             (WRI_BASE + 0xFFC)

#if defined(AIC_WRI_DRV_V12)
#define REG_BOOT_INFO                           WRI_BOOT_INFO
#define GET_REG_STATUS(c)                       readl(c)
#define BOOT_INFO_SET(val, mask, shift, cur)    writel_bits(val, mask, shift, REG_BOOT_INFO)
#define BOOT_INFO_GET(mask, shift, cur)         ({ cur = readl_bits(mask, shift, REG_BOOT_INFO); })
#define BOOT_INFO_WRITEB(cur)                   writel(cur, REG_BOOT_INFO)
#else
#define RTC_WR_EN_KEY           0xAC
#define RTC_REG_WR_EN           (0x00FC)
#define RTC_BOOT_INFO           (RTC_BASE + 0x100)
#define REG_BOOT_INFO           RTC_BOOT_INFO

#define RTC_WRITE_ENABLE        writeb(RTC_WR_EN_KEY, RTC_BASE + RTC_REG_WR_EN)
#define RTC_WRITE_DISABLE       writeb(0, RTC_BASE + RTC_REG_WR_EN)
#define GET_REG_STATUS(c)       readb(c)
#define BOOT_INFO_WRITEB(cur) \
    ({ \
        RTC_WRITE_ENABLE; \
        writeb((cur) & 0xFF, REG_BOOT_INFO); \
        RTC_WRITE_DISABLE; \
    })
#define BOOT_INFO_SET(val, mask, shift, cur) \
    ({ \
        setbits(val, mask, shift, cur); \
        BOOT_INFO_WRITEB(cur); \
    })
#define BOOT_INFO_GET(mask, shift, cur) \
    ({ \
        cur = readb(REG_BOOT_INFO); \
        cur = getbits(mask, shift, cur); \
    })
#endif

#define WRI_REBOOT_REASON_MASK  GENMASK(7, 4)
#define WRI_REBOOT_REASON_SHIFT 4

#if defined(AIC_WRI_DRV_V12) || defined(AIC_WRI_DRV_V11) || defined(AIC_WRI_DRV_V10)
enum aic_warm_reset_type {
    WRI_TYPE_ERR = 0,
    WRI_TYPE_POR = WRI_TYPE_ERR,
    WRI_TYPE_RTC,
    WRI_TYPE_EXT,
    WRI_TYPE_DM,
    WRI_TYPE_WDT,
    WRI_TYPE_TSEN,
    WRI_TYPE_CMP,
    WRI_TYPE_MAX
};
#endif

#if defined(AIC_WRI_DRV_V13)
enum aic_warm_reset_type {
    WRI_TYPE_ERR = 0,
    WRI_TYPE_VDD11_SP_POR = WRI_TYPE_ERR,
    WRI_TYPE_VDD11_SW_POR,
    WRI_TYPE_VDD11_C908_POR,
    WRI_TYPE_RTC_POR,
    WRI_TYPE_PIN_RST,
    WRI_TYPE_THS_RST,
    WRI_TYPE_SP_WDOG_SYS_RST,
    WRI_TYPE_SP_WDOG_CPU_RST,
    WRI_TYPE_SP_DM_NDM_RST,
    WRI_TYPE_SP_DM_CPU_RST,
    WRI_TYPE_CS_WDOG_SYS_RST,
    WRI_TYPE_CS_WDOG_CPU_RST,
    WRI_TYPE_CS_DM_NDM_RST,
    WRI_TYPE_CS_DM_CPU_RST,
    WRI_TYPE_SC_WDOG_SYS_RST,
    WRI_TYPE_SC_WDOG_CPU_RST,
    WRI_TYPE_SC_DM_NDM_RST,
    WRI_TYPE_SC_DM_CPU_RST,
    WRI_TYPE_SE_WDOG_SYS_RST,
    WRI_TYPE_SE_WDOG_CPU_RST,
    WRI_TYPE_SE_DM_NDM_RST,
    WRI_TYPE_MAX
};
#endif

struct aic_wri_ops {
    u32 *wri_bit;
    enum aic_warm_reset_type (*reset_type)(enum aic_warm_reset_type hw);
    void (*hw_reboot_action)(enum aic_warm_reset_type hw);
    void (*sw_reboot_reason)(enum aic_reboot_reason sw,
                             enum aic_reboot_reason r);
    int (*hw_reboot_reason)(enum aic_warm_reset_type hw,
                            enum aic_reboot_reason *r, u32 sw);
};

extern const struct aic_wri_ops wri_ops;
enum aic_warm_reset_type aic_wr_type_get(void);
enum aic_reboot_reason aic_judge_reboot_reason(enum aic_warm_reset_type hw, u32 sw);

#endif
