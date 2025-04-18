/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_core.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <aic_hal.h>
#include <hal_rtc.h>
#include <wdt.h>

static int do_reset_boot(int argc, char *argv[])
{
#ifdef AIC_WDT_DRV
    wdt_init();
    printf("Restarting system ...\n");
#endif
#ifdef AIC_WRI_DRV
    aic_set_reboot_reason(REBOOT_REASON_CMD_REBOOT);
#endif
#ifdef AIC_WDT_DRV
    wdt_expire_now();
    while(1) {};
#endif
    return 0;
}

CONSOLE_CMD(reset, do_reset_boot,  "Reboot device.");
CONSOLE_CMD(reboot, do_reset_boot, "Reboot device.");

static int cmd_aicupg(int argc, char **argv)
{
#ifdef AIC_WRI_DRV
    if ((argc == 2) && !strcmp(argv[1], "gotobl"))
        aic_set_reboot_reason(REBOOT_REASON_BL_UPGRADE);
    else if ((argc == 2) && !strcmp(argv[1], "hid"))
        aic_set_reboot_reason(REBOOT_REASON_BL_HID_UPGRADE);
    else
        aic_set_reboot_reason(REBOOT_REASON_UPGRADE);
#endif
    do_reset_boot(0, NULL);
    return 0;
}

CONSOLE_CMD(aicupg, cmd_aicupg, "Reboot to the upgrade mode.");

