/*
 * Copyright (C) 2022-2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "lvgl.h"
#include "test_ui.h"

static int end_flag = 0;

static void gif_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        end_flag = 1;
    }
}

int gif_check_finish()
{
    return end_flag;
}

void gif_ui_init()
{
    lv_obj_t * img;
    img = lv_gif_create(lv_scr_act());

    lv_gif_set_src(img, LVGL_PATH(gif/bulb.gif));

    printf("path:%s\n", LVGL_PATH(gif/bulb.gif));
    // must call after lv_gif_set_src
    lv_gif_set_loop_count(img, 1);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    lv_obj_add_event_cb(img, gif_event, LV_EVENT_ALL, NULL);
    printf("gif_ini_ok\n");
}

void ui_init(void)
{
    gif_ui_init();
}
