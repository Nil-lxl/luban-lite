/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "lvgl.h"
#include "aic_ui.h"
#include "camera_ui.h"

extern int msh_exec(char *cmd, rt_size_t length);
static void ui_camera_cb(lv_event_t *e);

extern int check_lvgl_de_config(void);
extern void fb_dev_layer_set(void);
extern void fb_dev_layer_reset(void);

#if defined (AIC_DVP_TEST) && (defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X))
static int lv_msh_exec(char *command, int length)
{
    char cmd[256] = {0};
    strncpy(cmd, command, sizeof(cmd) - 1);
    return msh_exec(cmd, strlen(cmd));
}
#endif

lv_obj_t *camera_ui_init(void)
{
    lv_obj_t *camera_ui = lv_obj_create(NULL);
#if defined (AIC_DVP_TEST) && (defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X))
    lv_msh_exec("test_dvp -c 0", rt_strlen("test_dvp -c 0"));
#endif
    if (check_lvgl_de_config() == -1) {
        LV_LOG_INFO("Please check defined AICFB_ARGB8888 and LV_COLOR_DEPTH is 32 or\n");
        LV_LOG_INFO("check defined AICFB_RGB565 and LV_COLOR_DEPTH is 16\n");
        return camera_ui;
    }
    fb_dev_layer_set();

    lv_obj_add_event_cb(camera_ui, ui_camera_cb, LV_EVENT_ALL, NULL);
    return camera_ui;
}

static void ui_camera_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_SCREEN_UNLOAD_START) {
#if defined (AIC_DVP_TEST) && (defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X))
        //release dvp resource
        extern void dvp_thread_stop();
        dvp_thread_stop();
#endif
        fb_dev_layer_reset();
    }
    if (code == LV_EVENT_DELETE) {;}
    if (code == LV_EVENT_SCREEN_LOADED) {;}
}

