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

void lv_wait_fs_mounted(void)
{
    struct dfs_filesystem *fs = NULL;

    for (int i = 0; i < 1000; i++) {
        fs = dfs_filesystem_lookup(LVGL_STORAGE_PATH);
        if (fs != NULL) {
            /* Continue to look up until fs->path is not equal to '/'. */
            if (strncmp(LVGL_STORAGE_PATH, fs->path, 3) == 0)
                return;
        }

        aicos_msleep(10);
    }
    LV_LOG_ERROR("Wait fs mound timeout, path: %s\n", LVGL_STORAGE_PATH);
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
