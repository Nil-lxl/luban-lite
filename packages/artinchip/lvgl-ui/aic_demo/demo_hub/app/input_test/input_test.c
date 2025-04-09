/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include "aic_ui.h"
#include "lvgl.h"
#include "input_button_adc_drv.h"

static void ui_input_test_cb(lv_event_t *e);
static void obj_move_x_init(lv_anim_t *anim, lv_obj_t *obj, uint32_t delay,
                       int32_t start_value, int32_t end_value, uint32_t duration);
static void obj_move_y_init(lv_anim_t *anim, lv_obj_t *obj, uint32_t delay,
                       int32_t start_value, int32_t end_value, uint32_t duration);

static lv_anim_t *anim;

#if defined(AIC_USING_GPAI) && defined(KERNEL_RTTHREAD)
static lv_timer_t *button_adc_timer;

static void read_adc_and_sent_event(lv_timer_t * timer)
{
    lv_obj_t *btn;
    lv_obj_t *input_test_ui = (lv_obj_t *)timer->user_data;
    int btn_id = -1;
    uint32_t btn_state;
    int obj_index = 0;

    button_adc_read(&btn_id, &btn_state);
    if (btn_state == 0 || btn_id < 0)
        return;

    obj_index = - btn_id;

    btn = lv_obj_get_child(input_test_ui, obj_index);
    lv_obj_send_event(btn, btn_state, NULL);
}
#endif

static void btn_change_bg_color_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_current_target_obj(e);

    if(code == LV_EVENT_PRESSED) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x2775D5), 0);
    } else if (code == LV_EVENT_LONG_PRESSED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF0000), 0);
    } else if (code == LV_EVENT_RELEASED) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xC4C4C4), 0);
    }
}

static lv_obj_t *test_btn_create(lv_obj_t *parent, char *label)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, LV_HOR_RES * 0.16, LV_VER_RES  * 0.5);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xC4C4C4), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, 0);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, label);
#if LVGL_VERSION_MAJOR == 8
    lv_label_set_recolor(btn_label, true);
#endif
    lv_obj_set_align(btn_label, LV_ALIGN_CENTER);
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xffffff), 0);

    lv_obj_update_layout(btn);
    lv_obj_add_event_cb(btn, btn_change_bg_color_handler, LV_EVENT_ALL, NULL);

    return btn;
}

lv_obj_t *input_test_ui_init(void)
{
    lv_obj_t *input_test_ui = lv_obj_create(NULL);
    lv_obj_clear_flag(input_test_ui, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bg = lv_obj_create(input_test_ui);
    lv_obj_remove_style_all(bg);
    lv_obj_set_size(bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(bg, LV_OPA_100, 0);

    lv_obj_t *title = lv_label_create(input_test_ui);
    lv_label_set_text(title, "Button input test");
#if LVGL_VERSION_MAJOR == 8
    lv_label_set_recolor(title, true);
#endif
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    lv_obj_set_style_text_color(title, lv_color_hex(0x0), 0);

    lv_obj_t *tip = lv_label_create(input_test_ui);
    lv_label_set_text(tip, "\nPlease configure the adc button or common button");
#if LVGL_VERSION_MAJOR == 8
    lv_label_set_recolor(tip, true);
#endif
    lv_obj_set_align(tip, LV_ALIGN_TOP_MID);
    lv_obj_set_style_text_color(tip, lv_color_hex(0x0), 0);

    anim = lv_mem_alloc(sizeof(lv_anim_t) * 6);
    obj_move_y_init(&anim[0], title, 0, -100, 0, 500);
    obj_move_y_init(&anim[1], tip, 200, -100, 0, 500);

    lv_obj_t *btn;
    int pos_x = 0;
    char num[2] = {0};

    for (int i = 0; i < 4; i++) {
        num[0] = '0' + i;
        num[1] = '\0';
        btn = test_btn_create(input_test_ui, num);
        lv_obj_set_align(btn, LV_ALIGN_LEFT_MID);

        pos_x = (((LV_HOR_RES - lv_obj_get_width(btn) * 4)) / 5) * (i + 1) + i * lv_obj_get_width(btn);
        lv_obj_set_pos(btn, pos_x, 0);
        obj_move_x_init(&anim[i+2], btn, 200 * i, -(pos_x + lv_obj_get_width(btn)), pos_x, 500);
    }

    lv_obj_add_event_cb(input_test_ui, ui_input_test_cb, LV_EVENT_ALL, NULL);
#if defined(AIC_USING_GPAI) && defined(KERNEL_RTTHREAD)
    if (button_adc_open() < 0)
        return input_test_ui;

    button_adc_timer = lv_timer_create(read_adc_and_sent_event, 30, input_test_ui);
#endif

    return input_test_ui;
}

static void ui_input_test_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);

    if (code == LV_EVENT_SCREEN_UNLOAD_START) {
#if defined(AIC_USING_GPAI) && defined(KERNEL_RTTHREAD)
        if (button_adc_timer)
            lv_timer_del(button_adc_timer);
        button_adc_close();
#endif
        lv_mem_free(anim);
    }
    if (code == LV_EVENT_DELETE) {;}
    if (code == LV_EVENT_SCREEN_LOADED) {;}
}


static void anim_move_x_cb(void * var, int32_t v)
{
    lv_obj_set_x(var, v);
}

static void anim_move_y_cb(void * var, int32_t v)
{
    lv_obj_set_y(var, v);
}

static void _base_anim_init(lv_anim_t *anim, lv_obj_t *obj, lv_anim_exec_xcb_t exec_cb, uint32_t delay,
                            int32_t start_value, int32_t end_value, uint32_t duration)
{
    lv_anim_init(anim);
    lv_anim_set_var(anim, obj);
    lv_anim_set_values(anim, start_value, end_value);
    lv_anim_set_time(anim, duration);
    lv_anim_set_exec_cb(anim, exec_cb);
    lv_anim_set_delay(anim, delay);
    lv_anim_set_path_cb(anim, lv_anim_path_ease_out);
}

static void obj_move_x_init(lv_anim_t *anim, lv_obj_t *obj, uint32_t delay,
                       int32_t start_value, int32_t end_value, uint32_t duration)
{
    _base_anim_init(anim, obj, anim_move_x_cb, delay, start_value, end_value, duration);
    lv_anim_start(anim);
}

static void obj_move_y_init(lv_anim_t *anim, lv_obj_t *obj, uint32_t delay,
                       int32_t start_value, int32_t end_value, uint32_t duration)
{
    _base_anim_init(anim, obj, anim_move_y_cb, delay, start_value, end_value, duration);
    lv_anim_start(anim);
}
