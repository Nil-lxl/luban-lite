/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#if LV_AIC_VIDEO_WINDOW
#include "lv_aic_video_window.h"
#include "aic_ui.h"

static void lv_aic_video_window_event(const lv_obj_class_t * class_p, lv_event_t * e);

const lv_obj_class_t lv_aic_video_window_class = {
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .event_cb = lv_aic_video_window_event,
    .instance_size = sizeof(lv_aic_video_window_t),
#if LVGL_VERSION_MAJOR == 8
    .base_class = &lv_img_class
#else
    .base_class = &lv_image_class,
    .name = "lv_aic_video_window",
#endif
};

lv_obj_t * lv_aic_video_window_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(&lv_aic_video_window_class, parent);
    lv_obj_class_init_obj(obj);
#if LV_COLOR_DEPTH != 32
    LV_LOG_WARN("Color depth != 32 will not take effect!");
#endif
    return obj;
}

void lv_aic_video_window_set_size(lv_obj_t * obj, uint32_t width, uint32_t height)
{
    lv_aic_video_window_t *window = (lv_aic_video_window_t *)obj;
    window->width = width;
    window->height = height;
#if LVGL_VERSION_MAJOR == 8
    lv_event_send(obj, LV_EVENT_SIZE_CHANGED, NULL);
#else
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
#endif
}

void lv_aic_video_window_set_color(lv_obj_t * obj, lv_color_t color)
{
    lv_aic_video_window_t *window = (lv_aic_video_window_t *)obj;
    window->color = color;
#if LVGL_VERSION_MAJOR == 8
    lv_event_send(obj, LV_EVENT_STYLE_CHANGED, NULL);
#else
    lv_obj_send_event(obj, LV_EVENT_STYLE_CHANGED, NULL);
#endif
}

static void lv_aic_video_window_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);
    /*Call the ancestor's event handler*/
    lv_res_t res = lv_obj_event_base(&lv_aic_video_window_class, e);
    if(res != LV_RES_OK) return;
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SIZE_CHANGED || code == LV_EVENT_STYLE_CHANGED) {
        lv_obj_t * obj = lv_event_get_current_target(e);
        lv_aic_video_window_t *window = (lv_aic_video_window_t *)obj;
        char window_para[128] = {0};

        if (window->width == 0 || window->height == 0)
            return;
#if LVGL_VERSION_MAJOR == 8
        if (window->color.ch.red == window->last_color.ch.red &&
            window->color.ch.green == window->last_color.ch.green &&
            window->color.ch.blue == window->last_color.ch.blue &&
            window->width == window->last_width &&
            window->height == window->last_height)
            return;

        uint32_t color = (0 << 24) | (window->color.ch.red << 16) |
                         (window->color.ch.green << 8) | (window->color.ch.blue);
#else
        if (window->color.red == window->last_color.red &&
            window->color.green == window->last_color.green &&
            window->color.blue == window->last_color.blue &&
            window->width == window->last_width &&
            window->height == window->last_height)
            return;

        uint32_t color = (0 << 24) | (window->color.red << 16) |
                         (window->color.green << 8) | (window->color.blue);
#endif
        int alpha_en = 0;
        snprintf(window_para, sizeof(window_para), "L:/%dx%d_%d_%08x.fake",
                window->width, window->height, alpha_en, (unsigned int)color);
        lv_img_set_src(obj, window_para);

        window->last_color = window->color;
        window->last_height = window->height;
        window->last_width = window->width;
    }
}
#endif
