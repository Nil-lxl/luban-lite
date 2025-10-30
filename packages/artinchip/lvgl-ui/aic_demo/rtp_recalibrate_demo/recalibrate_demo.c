/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include "lvgl.h"
#include "lv_tpc_run.h"

#define COLOR_DEFAULT 0xc4c4c4
#define COLOR_FINISH  0x6dd401
#define COLOR_WAIT    0xb620e0

static void update_recalibrate_obj(lv_timer_t *t)
{
#if AIC_RTP_RECALIBRATE_ENABLE
    static int last_recalibrate_status = -1;
    int recalibrate_status = rtp_get_recalibrate_status();

    if (last_recalibrate_status == recalibrate_status)
        return;

    lv_obj_t *label = lv_obj_get_child(t->user_data, -1);
    lv_label_set_text_fmt(label, "status = 0x%x", rtp_get_recalibrate_status());

    if (rtp_get_recalibrate_status() == 0)
        lv_obj_set_style_bg_color(lv_obj_get_child(t->user_data, 0), lv_color_hex(COLOR_WAIT), 0);

    if (rtp_is_recalibrate_dir_data_stored(RTP_CAL_POINT_DIR_TOP_LEFT)) {
        lv_obj_set_style_bg_color(lv_obj_get_child(t->user_data, 0), lv_color_hex(COLOR_FINISH), 0);
        lv_obj_set_style_bg_color(lv_obj_get_child(t->user_data, 1), lv_color_hex(COLOR_WAIT), 0);
    }

    if (rtp_is_recalibrate_dir_data_stored(RTP_CAL_POINT_DIR_TOP_RIGHT)) {
        lv_obj_set_style_bg_color(lv_obj_get_child(t->user_data, 1), lv_color_hex(COLOR_FINISH), 0);
        lv_obj_set_style_bg_color(lv_obj_get_child(t->user_data, 2), lv_color_hex(COLOR_WAIT), 0);
    }

    if (rtp_is_recalibrate_dir_data_stored(RTP_CAL_POINT_DIR_BOT_RIGHT)) {
        lv_obj_set_style_bg_color(lv_obj_get_child(t->user_data, 2), lv_color_hex(COLOR_FINISH), 0);
        lv_obj_set_style_bg_color(lv_obj_get_child(t->user_data, 3), lv_color_hex(COLOR_WAIT), 0);
    }

    if (rtp_is_recalibrate_dir_data_stored(RTP_CAL_POINT_DIR_BOT_LEFT)) {
        lv_obj_set_style_bg_color(lv_obj_get_child(t->user_data, 3), lv_color_hex(COLOR_FINISH), 0);
        lv_obj_set_style_bg_color(lv_obj_get_child(t->user_data, 4), lv_color_hex(COLOR_WAIT), 0);
    }

    if (rtp_is_recalibrate_dir_data_stored(RTP_CAL_POINT_DIR_CENTER)) {
        lv_obj_set_style_bg_color(lv_obj_get_child(t->user_data, 4), lv_color_hex(COLOR_FINISH), 0);
    }

    last_recalibrate_status = recalibrate_status;
#endif
}

static void get_calibrate_point(rtp_cal_point_dir_t dir, int *x, int *y)
{
#if AIC_RTP_RECALIBRATE_ENABLE
    rtp_get_calibrate_point(dir, x, y);
#else
    if (dir == RTP_CAL_POINT_DIR_TOP_LEFT) {
        *x = 50;
        *y = 50;
    } else if (dir == RTP_CAL_POINT_DIR_TOP_RIGHT) {
        *x = LV_HOR_RES - 50;
        *y = 50;
    } else if (dir == RTP_CAL_POINT_DIR_BOT_RIGHT) {
        *x = LV_HOR_RES - 50;
        *y = LV_VER_RES - 50;
    } else if (dir == RTP_CAL_POINT_DIR_BOT_LEFT) {
        *x = 50;
        *y = LV_VER_RES - 50;
    } else if (dir == RTP_CAL_POINT_DIR_CENTER) {
        *x = LV_HOR_RES / 2;
        *y = LV_VER_RES / 2;
    }
#endif
}

static lv_obj_t *create_recalibrate_obj(lv_obj_t *parent, lv_color_t color, uint16_t width, uint16_t height, lv_coord_t center_x, lv_coord_t center_y)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_size(obj, width, height);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * label = lv_label_create(obj);
    lv_label_set_text(label, "+");
    lv_obj_center(label);

    lv_obj_t *scr = lv_obj_get_screen(parent);
    lv_coord_t align_x = center_x - width / 2;
    lv_coord_t align_y = center_y - height / 2;
    lv_obj_align_to(obj, scr, LV_ALIGN_TOP_LEFT, align_x, align_y);

    return obj;
}

static void recalibrate_create()
{
    lv_obj_t *screen = lv_obj_create(NULL);

    int i = 0;
    int x = 0;
    int y = 0;

    rtp_init_recalibrate();

    for (i = 0; i <= RTP_CAL_POINT_DIR_CENTER; i++) {
        get_calibrate_point(i, &x, &y);
        create_recalibrate_obj(screen, lv_color_hex(COLOR_DEFAULT), 50, 50, x, y);
    }

    lv_obj_t *obj = lv_btn_create(screen);
    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, - LV_VER_RES * 0.1);
    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, "Click Me");
    lv_obj_center(label);

    label = lv_label_create(screen);
#if AIC_RTP_RECALIBRATE_ENABLE
    lv_label_set_text_fmt(label, "status = 0x%x", rtp_get_recalibrate_status());
#else
    lv_label_set_text(label, "Please open AIC_RTP_RECALIBRATE_ENABLE!!!");
#endif
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, LV_VER_RES * 0.1);

    lv_screen_load(screen);

    lv_timer_create(update_recalibrate_obj, 50, screen);


#if AIC_RTP_RECALIBRATE_ENABLE
    rtp_init_recalibrate();
#endif
}

void ui_init(void)
{
    recalibrate_create();
}
