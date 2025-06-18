/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
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
#include "lv_aic_camera.h"

static void ui_camera_cb(lv_event_t *e);
#if defined(AIC_USING_CAMERA) && defined(AIC_DISPLAY_DRV) && defined(AIC_DVP_DRV)
static void start_cb(lv_event_t * e);
static void stop_cb(lv_event_t * e);
static void delete_cb(lv_event_t * e);
#endif
extern int check_lvgl_de_config(void);
extern void fb_dev_layer_set(void);
extern void fb_dev_layer_reset(void);

static int delete = 0;
lv_obj_t *camera_ui_init(void)
{
    lv_obj_t *camera_ui = lv_obj_create(NULL);
    lv_obj_set_style_bg_opa(camera_ui, LV_OPA_100, LV_PART_MAIN); /* set back ground opa to avoid display errors */
    lv_obj_set_style_bg_color(camera_ui, lv_color_hex(0xffffff), LV_PART_MAIN);
    if (check_lvgl_de_config() == -1) {
        LV_LOG_INFO("Please check defined AICFB_ARGB8888 and LV_COLOR_DEPTH is 32 or\n");
        LV_LOG_INFO("check defined AICFB_RGB565 and LV_COLOR_DEPTH is 16\n");
        return camera_ui;
    }

#if defined(AIC_USING_CAMERA) && defined(AIC_DISPLAY_DRV) && defined(AIC_DVP_DRV)
    lv_obj_t *camera = lv_aic_camera_create(camera_ui);
    lv_obj_set_style_bg_opa(camera, LV_OPA_100, LV_PART_MAIN); /* set back ground opa to avoid display errors */
    lv_obj_set_style_bg_color(camera, lv_color_hex(0x606060), LV_PART_MAIN);
    lv_obj_set_style_outline_width(camera, 2, LV_PART_MAIN);
    lv_obj_set_style_outline_opa(camera, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_size(camera, LV_HOR_RES / 2, LV_VER_RES / 2);
    lv_obj_center(camera);

    lv_aic_camera_set_format(camera, LV_AIC_CAMERA_FORMAT_NV12);
    if (lv_aic_camera_open(camera) == LV_RES_INV) {
        LV_LOG_ERROR("lv_aic_camera_open failed");
        return camera_ui;
    }

    lv_obj_t * start_btn = lv_button_create(camera_ui);
    lv_obj_add_event_cb(start_btn, start_cb, LV_EVENT_ALL, camera);
    lv_obj_align(start_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(start_btn, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_t *label = lv_label_create(start_btn);
    lv_label_set_text(label, "Start");
    lv_obj_center(label);

    lv_obj_t * stop_btn = lv_button_create(camera_ui);
    lv_obj_add_event_cb(stop_btn, delete_cb, LV_EVENT_ALL, camera);
    lv_obj_align(stop_btn, LV_ALIGN_CENTER, 80, 0);
    label = lv_label_create(stop_btn);
    lv_label_set_text(label, "dele");
    lv_obj_center(label);

    lv_obj_t * delete_btn = lv_button_create(camera_ui);
    lv_obj_add_event_cb(delete_btn, stop_cb, LV_EVENT_ALL, camera);
    lv_obj_align(delete_btn, LV_ALIGN_CENTER, -80, 0);
    label = lv_label_create(delete_btn);
    lv_label_set_text(label, "Stop");
    lv_obj_center(label);

    lv_obj_t *title = lv_label_create(camera_ui);
    lv_label_set_text(title, "Camera test");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(title, lv_color_hex(0x0), 0);
#endif
    fb_dev_layer_set();
    lv_obj_add_event_cb(camera_ui, ui_camera_cb, LV_EVENT_ALL, NULL);
    return camera_ui;
}

static void ui_camera_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_SCREEN_UNLOAD_START) {
        delete = 0;
        fb_dev_layer_reset();
    }
    if (code == LV_EVENT_DELETE) {;}
    if (code == LV_EVENT_SCREEN_LOADED) {;}
}

#if defined(AIC_USING_CAMERA) && defined(AIC_DISPLAY_DRV) && defined(AIC_DVP_DRV)
static void start_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *camera = lv_event_get_user_data(e);

    if(code == LV_EVENT_CLICKED) {
        if (delete)
            return;
        if (lv_aic_camera_start(camera) == LV_RES_OK) {
            /* clear background opa, display the video */
            lv_obj_set_style_bg_opa(camera, LV_OPA_0, LV_PART_MAIN);
        }
    }
}

static void stop_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *camera = lv_event_get_user_data(e);

    if(code == LV_EVENT_CLICKED) {
        if (delete)
            return;
        lv_obj_set_style_bg_opa(camera, LV_OPA_100, LV_PART_MAIN);
        lv_aic_camera_stop(camera);
    }
}

static void delete_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *camera = lv_event_get_user_data(e);

    if(code == LV_EVENT_CLICKED) {
        if (delete)
            return;
        lv_obj_del(camera);
        delete = 1;
    }
}
#endif
