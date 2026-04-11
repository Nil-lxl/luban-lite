/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <stdio.h>
#include "lvgl.h"
#include "aic_ui.h"
#include "lv_img_roller.h"

static int img_num = 6;
static lv_obj_t *label_name;

static char *img_path[] = {
        LVGL_IMAGE_PATH(00.png),
        LVGL_IMAGE_PATH(01.png),
        LVGL_IMAGE_PATH(02.png),
        LVGL_IMAGE_PATH(03.png),
        LVGL_IMAGE_PATH(04.png),
        LVGL_IMAGE_PATH(05.png),
};

static char *img_name[] = {
        "Drumsticks",
        "Western-style food",
        "Steamed rice",
        "Hamburger",
        "Fish",
        "Stuffed bun",
};

static void img_roller_event(lv_event_t * e)
{
    lv_obj_t *target = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        int index = lv_img_get_active_id(target);
        printf("cur_index:%d\n", index);
        lv_label_set_text(label_name, img_name[index]);
    }
}

void aic_img_roller(void)
{
    int init_index = 1;

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    label_name = lv_label_create(lv_scr_act());
    lv_label_set_text(label_name, img_name[init_index]);
    lv_label_set_long_mode(label_name, LV_LABEL_LONG_WRAP);
    lv_obj_center(label_name);
    lv_obj_set_y(label_name, 120);
    lv_obj_set_style_text_color(label_name, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *img_roller = lv_img_roller_create(lv_scr_act());
    lv_obj_set_size(img_roller, 300, 100);
    lv_obj_center(img_roller);

    // lv_img_roller_set_transform_ratio(img_roller, 128);
    lv_img_roller_set_direction(img_roller, LV_DIR_HOR);

    for (int i = 0; i < img_num; i++) {
        lv_img_roller_add_child(img_roller, img_path[i]);
    }

    lv_img_roller_set_loop_mode(img_roller, LV_ROLL_LOOP_OFF);

    // set active index to init_index
    lv_img_roller_set_active(img_roller, init_index, LV_ANIM_OFF);
    lv_obj_add_event_cb(img_roller, img_roller_event, LV_EVENT_ALL, NULL);

    // make img roller ready
    lv_img_roller_ready(img_roller);
}
