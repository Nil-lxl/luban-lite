/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "lvgl.h"
#include "../font/font_init.h"
#if 0
lv_obj_t * left_list_ui_create(lv_obj_t *parent);
lv_obj_t * create_setting(lv_obj_t *parent);
lv_obj_t * create_setting_wifi(lv_obj_t *parent);
lv_obj_t * create_setting_model(lv_obj_t *parent);

lv_obj_t *func_ui_create(lv_obj_t *parent)
{
    lv_obj_t *func_obj = lv_obj_create(parent);
    lv_obj_set_pos(func_obj, 0, 0);
    lv_obj_set_size(func_obj, 1024, 600);
    lv_obj_remove_style_all(func_obj);
    lv_obj_set_style_bg_color(func_obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(func_obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);

    left_list_ui_create(func_obj);
    create_setting(func_obj);
    create_setting_wifi(func_obj);
    create_setting_wifi(create_setting_model);
}

lv_obj_t * left_list_ui_create(lv_obj_t *parent)
{
    lv_obj_t *letf_list = lv_obj_create(parent);
    lv_obj_set_pos(letf_list, 0, 0);
    lv_obj_set_size(letf_list, 180, 600);
    lv_obj_remove_style_all(letf_list);

    // Init setting_img
    lv_obj_t *setting_img = lv_img_create(letf_list);
    lv_img_set_src(setting_img, LVGL_IMAGE_PATH(setting.png));
    lv_img_set_pivot(setting_img, 50, 50);
    lv_img_set_angle(setting_img, 0);
    lv_obj_set_style_img_opa(setting_img, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(setting_img, 12, 510);
    lv_obj_add_flag(setting_img, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *setting_label = lv_label_create(letf_list);
    lv_label_set_text(setting_label, "Setting");
    lv_label_set_long_mode(setting_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(setting_label, 58, 514);
    lv_obj_set_size(setting_label, 100, 32);
    lv_obj_add_flag(setting_img, LV_OBJ_FLAG_CLICKABLE);

    // Set style of setting_label
    lv_obj_set_style_text_font(setting_label, montserratmedium_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init info_img
    lv_obj_t *info_img = lv_img_create(letf_list);
    lv_img_set_src(info_img, LVGL_IMAGE_PATH(info.png));
    lv_img_set_pivot(info_img, 50, 50);
    lv_img_set_angle(info_img, 0);
    lv_obj_set_style_img_opa(info_img, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(info_img, 13, 552);

    // Init info_label
    lv_obj_t *info_label = lv_label_create(letf_list);
    lv_label_set_text(info_label, "Info");
    lv_label_set_long_mode(info_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(info_label, 59, 556);
    lv_obj_set_size(info_label, 100, 32);

    // Set style of info_label
    lv_obj_set_style_text_font(info_label, montserratmedium_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init func_line
    lv_obj_t *func_line = lv_line_create(letf_list);
#if LVGL_VERSION_MAJOR == 8
    static lv_point_t line_1_point[] ={{0, 0}, {170, 0}};
#else
    static lv_point_precise_t line_1_point[] ={{0, 0}, {170, 0}};
#endif
    lv_line_set_points(func_line, line_1_point, 2);
    lv_obj_set_pos(func_line, 5, 497);
    lv_obj_set_size(func_line, 176, 3);

    // Set style of func_line
    lv_obj_set_style_line_color(func_line, lv_color_hex(0xe3e1e1), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init history_label
    lv_obj_t *history_label = lv_label_create(letf_list);
    lv_label_set_text(history_label, "History");
    lv_label_set_long_mode(history_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(history_label, 6, 4);
    lv_obj_set_size(history_label, 100, 32);

    // Set style of history_label
    lv_obj_set_style_text_font(history_label, montserratmedium_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(history_label, lv_color_hex(0x919090), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init history_label_1
    lv_obj_t *history_label_1 = lv_label_create(letf_list);
    lv_label_set_text(history_label_1, "None");
    lv_label_set_long_mode(history_label_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(history_label_1, 5, 50);
    lv_obj_set_size(history_label_1, 100, 32);

    // Set style of history_label_1
    lv_obj_set_style_text_font(history_label_1, montserratmedium_24, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init history_label_2
    lv_obj_t *history_label_2 = lv_label_create(letf_list);
    lv_label_set_text(history_label_2, "None");
    lv_label_set_long_mode(history_label_2, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(history_label_2, 5, 102);
    lv_obj_set_size(history_label_2, 100, 32);

    // Set style of history_label_2
    lv_obj_set_style_text_font(history_label_2, montserratmedium_24, LV_PART_MAIN | LV_STATE_DEFAULT);

    return letf_list;
}

lv_obj_t *create_setting(lv_obj_t *parent)
{
    lv_obj_t *setting_container = lv_obj_create(parent);
    lv_obj_set_pos(setting_container, 81, 94);
    lv_obj_set_size(setting_container, 700, 450);
    lv_obj_set_scrollbar_mode(setting_container, LV_SCROLLBAR_MODE_AUTO);

    // Set style of setting_container
    lv_obj_set_style_bg_color(setting_container, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(setting_container, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(setting_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(setting_container, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(setting_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(setting_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(setting_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(setting_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init setting_label
    lv_obj_t *setting_label = lv_label_create(setting_container);
    lv_label_set_text(setting_label, "Setting");
    lv_label_set_long_mode(setting_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(setting_label, 319, 12);
    lv_obj_set_size(setting_label, 63, 22);

    // Set style of setting_label
    lv_obj_set_style_text_font(setting_label, montserratmedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init wifi_btn
    lv_obj_t *wifi_btn = lv_btn_create(setting_container);
    lv_obj_t *wifi_btn_label = lv_label_create(wifi_btn);
    lv_label_set_text(wifi_btn_label, "WIFI");
    lv_obj_align(wifi_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(wifi_btn, 21, 71);
    lv_obj_set_size(wifi_btn, 317, 43);

    // Set style of wifi_btn
    lv_obj_set_style_bg_color(wifi_btn, lv_color_hex(0x50cae9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(wifi_btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(wifi_btn, montserratmedium_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(wifi_btn, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(wifi_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init model
    lv_obj_t *model = lv_btn_create(setting_container);
    lv_obj_t *model_btn_label = lv_label_create(model);
    lv_label_set_text(model_btn_label, "MODEL");
    lv_obj_align(model_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(model, 352, 72);
    lv_obj_set_size(model, 329, 43);

    // Set style of model
    lv_obj_set_style_bg_color(model, lv_color_hex(0x50cae9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(model, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(model, montserratmedium_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(model, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(model, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init close_img_btn
    lv_obj_t *close_img_btn = lv_imgbtn_create(setting_container);
    lv_imgbtn_set_src(close_img_btn, LV_IMGBTN_STATE_RELEASED, NULL, LVGL_IMAGE_PATH(close.png), NULL);
    lv_imgbtn_set_src(close_img_btn, LV_IMGBTN_STATE_PRESSED, NULL, LVGL_IMAGE_PATH(closed.png), NULL);
    lv_obj_t *screen_close_imgbtn_label = lv_label_create(close_img_btn);
    lv_label_set_text(screen_close_imgbtn_label, "");
    lv_obj_align(screen_close_imgbtn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(close_img_btn, 646, 18);
    lv_obj_set_size(close_img_btn, 36, 36);

    // Set style of close_img_btn
    lv_obj_set_style_text_font(close_img_btn, montserratmedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);
}

lv_obj_t *create_setting_wifi(lv_obj_t *parent)
{
    // Init name_textarea
    lv_obj_t *name_textarea = lv_textarea_create(parent);
    lv_textarea_set_one_line(name_textarea, false);
    lv_textarea_set_text(name_textarea, "AIC-Guest");
    lv_textarea_set_placeholder_text(name_textarea, "");
    lv_obj_set_pos(name_textarea, 18, 155);
    lv_obj_set_size(name_textarea, 664, 47);

    // Set style of name_textarea
    lv_obj_set_style_text_font(name_textarea, montserratmedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init wifi_name
    lv_obj_t *wifi_name = lv_label_create(parent);
    lv_label_set_text(wifi_name, "Name");
    lv_label_set_long_mode(wifi_name, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(wifi_name, 21, 126);
    lv_obj_set_size(wifi_name, 100, 20);

    // Set style of wifi_name
    lv_obj_set_style_text_font(wifi_name, montserratmedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init password_textarea
    lv_obj_t *password_textarea = lv_textarea_create(parent);
    lv_textarea_set_one_line(password_textarea, false);
    lv_textarea_set_text(password_textarea, "AIC-Guest");
    lv_textarea_set_placeholder_text(password_textarea, "");
    lv_obj_set_pos(password_textarea, 18, 240);
    lv_obj_set_size(password_textarea, 664, 47);

    // Set style of password_textarea
    lv_obj_set_style_text_font(password_textarea, montserratmedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init wifi_password
    lv_obj_t *wifi_password = lv_label_create(parent);
    lv_label_set_text(wifi_password, "Password");
    lv_label_set_long_mode(wifi_password, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(wifi_password, 22, 216);
    lv_obj_set_size(wifi_password, 100, 20);

    // Set style of wifi_password
    lv_obj_set_style_text_font(wifi_password, montserratmedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init wifi_tip
    lv_obj_t *wifi_tip = lv_label_create(parent);
    lv_label_set_text(wifi_tip, "Tip:");
    lv_label_set_long_mode(wifi_tip, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(wifi_tip, 26, 304);
    lv_obj_set_size(wifi_tip, 581, 32);

    // Set style of wifi_tip
    lv_obj_set_style_text_font(wifi_tip, montserratmedium_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init connect_imgbtn
    lv_obj_t *connect_imgbtn = lv_imgbtn_create(parent);
    lv_imgbtn_set_src(connect_imgbtn, LV_IMGBTN_STATE_RELEASED, NULL, LVGL_IMAGE_PATH(connect.png), NULL);
    lv_imgbtn_set_src(connect_imgbtn, LV_IMGBTN_STATE_PRESSED, NULL, LVGL_IMAGE_PATH(connected.png), NULL);
    lv_obj_t *screen_connect_btn_label = lv_label_create(connect_imgbtn);
    lv_label_set_text(screen_connect_btn_label, "");
    lv_obj_align(screen_connect_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(connect_imgbtn, 301, 341);
    lv_obj_set_size(connect_imgbtn, 100, 100);

    // Set style of connect_imgbtn
    lv_obj_set_style_text_font(connect_imgbtn, montserratmedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);
}

lv_obj_t *create_setting_model(lv_obj_t *parent)
{
   // Init model_name
    lv_obj_t *model_name = lv_label_create(parent);
    lv_label_set_text(model_name, "Model");
    lv_label_set_long_mode(model_name, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(model_name, 28, 136);
    lv_obj_set_size(model_name, 100, 20);

    // Set style of model_name
    lv_obj_set_style_text_font(model_name, montserratmedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Init model_dropdown
    lv_obj_t *model_dropdown = lv_dropdown_create(parent);
    lv_obj_set_style_max_height(lv_dropdown_get_list(model_dropdown), 120, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_dropdown_set_options(model_dropdown, "Deepseek\nDoubao\nTongyi");
    lv_obj_set_pos(model_dropdown, 18, 164);
    lv_obj_set_size(model_dropdown, 665, 46);

    // Set style of model_dropdown
    lv_obj_set_style_text_font(model_dropdown, montserratmedium_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(model_dropdown, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lv_dropdown_get_list(model_dropdown), montserratmedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(lv_dropdown_get_list(model_dropdown), lv_color_hex(0x50cae9), LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(model_dropdown, LV_FONT_DEFAULT, LV_PART_INDICATOR | LV_STATE_DEFAULT);
}
#endif
