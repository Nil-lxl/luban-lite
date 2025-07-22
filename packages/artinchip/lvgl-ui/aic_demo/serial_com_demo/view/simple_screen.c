/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  zequan liang <zequan liang@artinchip.com>
 */
#include "simple_screen.h"
#include "string.h"
#include "stdio.h"

static uint32_t simple_screen_count = 0;

static void simple_screen_cb(lv_event_t *e);

lv_obj_t *simple_screen_create(void)
{
    char current_name[32] = {0};

    snprintf(current_name, sizeof(current_name), "simple_screen_%d:None", simple_screen_count);

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, LV_HOR_RES, LV_VER_RES);

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, current_name);
    lv_obj_center(label);

    uint32_t *simple_id = lv_malloc(sizeof(uint32_t));
    *simple_id = simple_screen_count++;

    lv_obj_add_event_cb(label, simple_screen_cb, LV_EVENT_ALL, simple_id);

    return screen;
}

lv_obj_t *simple_screen_get_label(lv_obj_t *screen)
{
    return lv_obj_get_child(screen, 0);
}

static void simple_screen_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    uint32_t *id = lv_event_get_user_data(e);
    char current_name[32] = {0};

    if (code == LV_EVENT_SCREEN_UNLOAD_START) {;}

    if (code == LV_EVENT_DELETE) {
        lv_free(id);
    }

    if (code == LV_EVENT_SCREEN_LOADED) {
        snprintf(current_name, sizeof(current_name), "simple_screen_%d", *id);
        extern void set_current_acvtive_screen_str(const char *name);
        set_current_acvtive_screen_str(current_name);
    }
}
