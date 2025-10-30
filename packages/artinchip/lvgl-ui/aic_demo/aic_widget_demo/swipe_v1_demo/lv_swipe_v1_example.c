/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lv_swipe_v1.h"
#include "aic_ui.h"

/* Font pointers for UI elements */
static lv_font_t *font_24 = NULL;
static lv_font_t *font_48 = NULL;

/* Initialize required fonts */
void swipe_v1_font_init()
{
    const char *font_path = LVGL_PATH_ORI(Lato-Regular.ttf);

#if LVGL_VERSION_MAJOR == 8
    static lv_ft_info_t font_init = {0};
    /*FreeType uses C standard file system, so no driver letter is required.*/
    font_init.name = font_path;
    font_init.weight = 24;
    font_init.style = FT_FONT_STYLE_NORMAL;
    if(!lv_ft_font_init(&font_init)) {
        LV_LOG_ERROR("create failed.");
    }
    font_24 = font_init.font;

    font_init.weight = 48;
    if(!lv_ft_font_init(&font_init)) {
        LV_LOG_ERROR("create failed.");
    }

    font_48 = font_init.font;
#else
    font_24 = lv_freetype_font_create(font_path,
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP, 24, 0);
    font_48 = lv_freetype_font_create(font_path,
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP, 48, 0);
#endif
}

/* Callback for toggle switch button */
static void toggle_switch_cb(lv_event_t *e)
{
    lv_obj_t *label = lv_event_get_user_data(e);
    lv_obj_t *swipe = lv_obj_get_parent(
        lv_event_get_current_target(e));
    int active_idx = lv_swipe_v1_get_active_child(swipe);

    bool is_on = lv_swipe_v1_get_child_active_src(swipe, active_idx);
    lv_label_set_text(label, is_on ? "ON" : "OFF");
}

/* Callback for mode selection button */
static void mode_select_cb(lv_event_t *e)
{
    lv_obj_t *label = lv_event_get_user_data(e);
    lv_obj_t *swipe = lv_obj_get_parent(
        lv_event_get_current_target(e));
    int active_idx = lv_swipe_v1_get_active_child(swipe);
    int mode = lv_swipe_v1_get_child_active_src(swipe, active_idx);

    const char *modes[] = {"Volume 0", "Volume 1", "Volume 2"};
    lv_label_set_text(label, modes[mode]);
}

/* Update current active item display */
static void update_active_display(lv_event_t *e)
{
    lv_obj_t *label = lv_event_get_user_data(e);
    lv_obj_t *swipe = lv_event_get_current_target(e);
    lv_label_set_text_fmt(label, "Active %d",
        lv_swipe_v1_get_active_child(swipe));
}

/* Next button click handler */
static void next_button_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t *swipe = lv_event_get_user_data(e);
        lv_swipe_v1_set_next(swipe, LV_ANIM_ON);
    }
}

/* Previous button click handler */
static void prev_button_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t *swipe = lv_event_get_user_data(e);
        lv_swipe_v1_set_prev(swipe, LV_ANIM_ON);
    }
}

/* Switch button click handler */
static void switch_button_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t *swipe = lv_event_get_user_data(e);
        lv_swipe_v1_toggle_child_active(swipe);
    }
}

/* Create navigation button with common settings */
static lv_obj_t* create_nav_button(lv_obj_t *parent, const char *text,
    int x, lv_event_cb_t cb, lv_obj_t *swipe)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, x, -100);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, swipe);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    return btn;
}

/* Add swipe item with common configuration */
static void add_swipe_item(lv_obj_t *swipe, int index,
    void *img1, void *img2, int x, int y,
    lv_event_cb_t cb, const char *default_text)
{
    // Add state resources
    lv_swipe_v1_child_add_state_src(swipe, index, img1, img2);

    // Configure child object
    lv_obj_t *child = lv_swipe_v1_get_child(swipe, index);
    lv_obj_align(child, LV_ALIGN_CENTER, x, y);
    lv_obj_set_style_transform_zoom(child, 180, 0);
    lv_swipe_v1_set_child_deactive_src(swipe, index, 0);

    // Add label if needed
    if (default_text) {
        lv_obj_t *label = lv_label_create(child);
        lv_label_set_text(label, default_text);
        lv_obj_set_style_text_font(label, font_24, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -80);

        // Add callback if specified
        if (cb) {
            lv_obj_add_event_cb(child, cb, LV_EVENT_CLICKED, label);
        }
    }
}

/* Main initialization function */
void swipe_v1_init(void)
{
    // Initialize fonts first
    swipe_v1_font_init();

    // Create swipe component
    lv_obj_t *swipe = lv_swipe_v1_create(lv_scr_act());
    lv_obj_set_size(swipe, 1024, 600);
    lv_swipe_v1_set_anim_params(swipe, 300, lv_anim_path_linear);

    // Add toggle switch item
    add_swipe_item(swipe, 0,
        LVGL_PATH(power_1.png),
        LVGL_PATH(power_1_3.png),
        -180, 90, toggle_switch_cb, "OFF");
    add_swipe_item(swipe, 0,
        LVGL_PATH(power_0.png),
        LVGL_PATH(power_0_3.png),
        -180, 90, NULL, NULL);

    // Add water/solution selector
    add_swipe_item(swipe, 1,
        LVGL_PATH(light_0.png),
        LVGL_PATH(light_0_3.png),
        60, 90, NULL, "");
    add_swipe_item(swipe, 1,
        LVGL_PATH(light_1.png),
        LVGL_PATH(light_1_3.png),
        60, 90, NULL, "");

    // Add mode selector
    add_swipe_item(swipe, 2,
        LVGL_PATH(volume_0.png),
        LVGL_PATH(volume_0_3.png),
        300, 90, mode_select_cb, "Volume 0");
    add_swipe_item(swipe, 2,
        LVGL_PATH(volume_1.png),
        LVGL_PATH(volume_1_3.png),
        300, 90, NULL, NULL);
    add_swipe_item(swipe, 2,
        LVGL_PATH(volume_2.png),
        LVGL_PATH(volume_2_3.png),
        300, 90, NULL, NULL);

    // Add center decorative item
    lv_swipe_v1_child_add_state_src(swipe, 3,
        LVGL_PATH(duck_0.png),
        LVGL_PATH(duck_0_3.png));
    lv_obj_t *center = lv_swipe_v1_get_child(swipe, 3);
    lv_obj_align(center, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_transform_zoom(center, 256, 0);

    // Create navigation buttons
    create_nav_button(lv_scr_act(), "last", -100, prev_button_cb, swipe);
    create_nav_button(lv_scr_act(), "next", 100, next_button_cb, swipe);
    create_nav_button(lv_scr_act(), "switch", 0, switch_button_cb, swipe);

    // Create active item display
    lv_obj_t *active_label = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(active_label, "Active %d",
        lv_swipe_v1_get_active_child(swipe));
    lv_obj_set_style_text_font(active_label, font_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(active_label, lv_color_hex(0x0A51A9), 0);
    lv_obj_align(active_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(swipe, update_active_display, LV_EVENT_VALUE_CHANGED, active_label);
}
