/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtconfig.h>
#include "aic_core.h"
#include "aic_reboot_reason.h"
#include <aic_hal.h>

#if defined(AIC_CONSOLE_BARE_DRV)
#include "console.h"
#endif

#ifdef RT_USING_FINSH
#include <string.h>
#include <stdlib.h>
#include <finsh.h>
#include <drivers/watchdog.h>
#include "aic_drv_wdt.h"

#define TIMEOUT         0

#endif /* RT_USING_FINSH */

#if defined(RT_USING_FINSH) || defined(AIC_CONSOLE_BARE_DRV)
static int clk_mod_reset(char *name)
{
    int32_t clk_id;
    clk_id = hal_clk_get_id(name);
    if (clk_id < 0)
        printf("error! get clk-id failed!\n");
    printf("Reset module :%s(id:%d)\n", name, (u32)clk_id);

    if (hal_clk_disable_assertrst(clk_id) < 0)
        goto __err;
    if (hal_clk_enable_deassertrst(clk_id) < 0)
        goto __err;
    return 0;
__err:
    return -1;
}

static int do_clk_mod_reset(int argc, char *argv[])
{
    int next;

    if (argc < 2) {
        pr_err("reset [clk-name]\n");
        return -1;
    }

    for (next = 1; next < argc; next++) {
        if (clk_mod_reset(argv[next]) < 0)
            printf("error! get clk failed!\n");
    }

    return 0;
}
#endif

#if defined(RT_USING_FINSH)
MSH_CMD_EXPORT_ALIAS(do_clk_mod_reset, reset, Reset the device moudule);
#elif defined(AIC_CONSOLE_BARE_DRV)
CONSOLE_CMD(reset_mod, do_clk_mod_reset,  "Reboot device moudule.");
#endif

#if defined(RT_USING_FINSH)
void rt_hw_cpu_reset()
{
    u32 timeout = TIMEOUT;
    rt_device_t wdt_dev = RT_NULL;

    wdt_dev =  rt_device_find("wdt");
    rt_device_init(wdt_dev);

    LOG_I("Restarting system ...\n");
    aicos_msleep(100);
    #ifdef AIC_WRI_DRV
    aic_set_reboot_reason(REBOOT_REASON_CMD_REBOOT);
    #endif
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);

    aicos_msleep(1000);
    LOG_W("Watchdog doesn't work!");
}

void cmd_reboot(int argc, char **argv)
{
    rt_hw_cpu_reset();
}

MSH_CMD_EXPORT_ALIAS(cmd_reboot, reboot, Reboot the system);
#endif /* RT_USING_FINSH */
