/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2021-10-17     Meco Man      First version
 */
#include <lvgl.h>
#ifdef KERNEL_RTTHREAD
#include <rtthread.h>
#endif
#include "aic_core.h"
#include "aic_osal.h"
#include "aic_ui.h"
#include <dfs_fs.h>
#include "aic_time.h"

void lv_wait_sdcard_mounted(void)
{
    if (!strcmp(LVGL_STORAGE_PATH, "/sdcard")) {
        aicos_msleep(1000);
    }
}

void lv_user_gui_init(void)
{
#ifdef AIC_LVGL_DEMO
    aic_ui_init();
#endif
}

#ifdef KERNEL_RTTHREAD
extern int lvgl_thread_init(void);

INIT_LATE_APP_EXPORT(lvgl_thread_init);

#endif
