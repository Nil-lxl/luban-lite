/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "aic_reboot_reason.h"
#include "hal_wri.h"

#define WRI_FLAG_RTC_POR            BIT(31)
#define WRI_FLAG_PRCM_RST_AP_CPU    BIT(29)
#define WRI_FLAG_PRCM_RST_AP_CORE   BIT(28)
#define WRI_FLAG_THS_RST            BIT(11)
#define WRI_FLAG_AP_WDOG_RST        BIT(10)
#define WRI_FLAG_CPU_DM_RST         BIT(9)
#define WRI_FLAG_PIN_RST            BIT(8)
#define WRI_FLAG_RT_WDOG_RST        BIT(6)
#define WRI_FLAG_VCC33_BOR          BIT(3)
#define WRI_FLAG_VDD09_CPU_POR      BIT(2)
#define WRI_FLAG_VDD09_AP_POR       BIT(1)
#define WRI_FLAG_VDD09_RT_POR       BIT(0)

u32 wr_bit[WRI_TYPE_MAX] = {WRI_FLAG_VDD09_RT_POR,
                            WRI_FLAG_VDD09_AP_POR,
                            WRI_FLAG_VDD09_CPU_POR,
                            WRI_FLAG_VCC33_BOR,
                            WRI_FLAG_RT_WDOG_RST,
                            WRI_FLAG_PIN_RST,
                            WRI_FLAG_CPU_DM_RST,
                            WRI_FLAG_AP_WDOG_RST,
                            WRI_FLAG_THS_RST,
                            WRI_FLAG_PRCM_RST_AP_CORE,
                            WRI_FLAG_PRCM_RST_AP_CPU,
                            WRI_FLAG_RTC_POR};

static enum aic_warm_reset_type get_reset_type(enum aic_warm_reset_type hw)
{
    u16 hw_reason[] = {WRI_TYPE_RT_WDOG_RST, WRI_TYPE_AP_WDOG_RST,};

    for (int i = 0; i < ARRAY_SIZE(hw_reason); i++) {
        if (hw == hw_reason[i])
            return hw_reason[i];
    }
    return WRI_TYPE_ERR;
}

static void get_hw_reboot_action(enum aic_warm_reset_type hw)
{
    switch (hw) {
    case WRI_TYPE_RT_WDOG_RST:
        pr_info("Reboot action: RTSS Watchdog-system-Reset\n");
    break;
    case WRI_TYPE_AP_WDOG_RST:
        pr_info("Reboot action: APSS Watchdog-system-Reset\n");
    break;
    default:
        printf("Reboot action: Unknown(%d)\n", hw);
        break;
    }
}

static void get_sw_reboot_reason(enum aic_reboot_reason sw,
                                 enum aic_reboot_reason r)
{
    switch (sw) {
    case REBOOT_REASON_UPGRADE:
        printf("Reboot reason: Upgrade-Mode\n");
        break;
    case REBOOT_REASON_SW_LOCKUP:
        printf("Reboot reason: Software-Lockup\n");
        break;
    case REBOOT_REASON_HW_LOCKUP:
        printf("Reboot reason: Hardware-Lockup\n");
        break;
    case REBOOT_REASON_PANIC:
        printf("Reboot reason: Kernel-Panic\n");
        break;
    case REBOOT_REASON_RT_CMD_REBOOT:
        pr_info("Reboot reason: RTSS Command-Reboot\n");
        break;
    case REBOOT_REASON_AP_CMD_REBOOT:
        pr_info("Reboot reason: APSS Command-Reboot\n");
        break;
    case REBOOT_REASON_RAMDUMP:
        printf("Ramdump\n");
        break;
    default:
        printf("Unknown(%d)\n", r);
        break;
    }
}

static void get_warm_reboot_reason(enum aic_warm_reset_type hw,
                                    enum aic_reboot_reason **r)
{
    switch (hw) {
    case WRI_TYPE_RTC_POR:
        pr_info("Reboot reason: RTC-Power-Down\n");
        **r = REBOOT_REASON_RTC_POR;
        break;
    case WRI_TYPE_PIN_RST:
        pr_info("Reboot reason: Extend-Reset\n");
        **r = REBOOT_REASON_PIN_RST;
        break;
    case WRI_TYPE_THS_RST:
        pr_info("Reboot reason: OTP-Reset\n");
        **r = REBOOT_REASON_THS_RST;
        break;
    case WRI_TYPE_CPU_DM_RST:
        pr_info("Reboot reason: CPU's DBG MODULE Reset\n");
        **r = REBOOT_REASON_CPU_DM_RST;
        break;
    case WRI_TYPE_PRCM_RST_AP_CPU:
        pr_info("Reboot reason: APSS CPU Reset\n");
        **r = REBOOT_REASON_PRCM_RST_AP_CPU;
        break;
    case WRI_TYPE_PRCM_RST_AP_CORE:
        pr_info("Reboot reason: APSS system-Reset\n");
        **r = REBOOT_REASON_PRCM_RST_AP_CORE;
        break;
    default:
        printf("Unknown(%d)\n", hw);
        break;
    }
}

static int get_hw_reboot_reason(enum aic_warm_reset_type hw,
                                enum aic_reboot_reason *r, u32 sw)
{
    if (*r == REBOOT_REASON_CMD_SHUTDOWN) {
        printf("Reboot reason: Command-Poweroff\n");
        return *r;
    }

    if (*r == REBOOT_REASON_SUSPEND) {
        printf("Reboot reason: Suspend\n");
        return *r;
    }

    if (*r == REBOOT_REASON_COLD) {
        if (hw == WRI_TYPE_VDD09_RT_POR) {
            pr_info("Startup reason: RTSS Power-On-Reset\n");
            return (enum aic_reboot_reason)sw;
        }

        if (hw == WRI_TYPE_VDD09_AP_POR) {
            pr_info("Startup reason: APSS Power-On-Reset\n");
            return (enum aic_reboot_reason)sw;
        }

        if (hw == WRI_TYPE_VDD09_CPU_POR) {
            pr_info("Startup reason: C907 Power-On-Reset\n");
            return (enum aic_reboot_reason)sw;
        }

        if (hw == WRI_TYPE_VCC33_BOR) {
            pr_info("Startup reason: VCC33 Power-Down-Reset\n");
            return (enum aic_reboot_reason)sw;
        }

        printf("Reboot action: Warm-Reset, reason: ");
        get_warm_reboot_reason(hw, &r);
        return *r;
    }
    return -EINVAL;
}

const struct aic_wri_ops wri_ops = {
    .wri_bit = wr_bit,
    .reset_type = get_reset_type,
    .hw_reboot_action = get_hw_reboot_action,
    .sw_reboot_reason = get_sw_reboot_reason,
    .hw_reboot_reason = get_hw_reboot_reason,
};
