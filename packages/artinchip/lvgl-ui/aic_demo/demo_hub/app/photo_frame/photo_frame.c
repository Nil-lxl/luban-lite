/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "aic_ui.h"
#include "photo_frame.h"

extern lv_obj_t *photo_frame_main_create(lv_obj_t *parent);
extern void photo_frame_screen_deinit(void);

img_list *g_list;
photo_frame_setting_t g_setting;
const char *setting_auto_play_desc =  "False\n"
                                      "True";
const char *setting_cur_anim_desc =   "None\n"
                                      "Fake in out\n"
                                      "Push\n"
                                      "Cover\n"
                                      "Uncover\n"
#if USE_SWITCH_ANIM_ZOOM
                                      "Zoom\n"
#endif
                                      "Fly pase\n"
#if USE_SWITCH_ANIM_COMB
                                      "Comb\n"
#endif
                                      "Flow\n"
#if USE_SWITCH_ANIM_ROTATE
                                      "Rotate\n"
                                      "Rotate move\n"
#endif
                                      "Random";
const char *setting_full_mode_desc =  "Normal\n"
                                      "Original";
                                      //"Fill cover"; /* Only V9 supports correct scaling */
static void ui_photo_frame_cb(lv_event_t *e);
static void photo_frame_ui_setting(void);
static void ui_photo_frame_cb(lv_event_t *e);
static void photo_frame_ui_setting(void);

lv_obj_t *photo_frame_ui_init(void)
{
    srand(time(0));

    photo_frame_screen_deinit();

    lv_obj_t *ui_photo_frame = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_photo_frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_img_cache_set_size(12);

    photo_frame_ui_setting();

    photo_frame_main_create(ui_photo_frame);

#if OPTIMIZE
#if LVGL_VERSION_MAJOR == 8
    lv_disp_t *disp = lv_disp_get_default();
    disp->bg_opa = LV_OPA_0;
    disp->bg_color = lv_color_hex(g_setting.bg_color);
#endif
#else
    lv_obj_set_style_bg_color(ui_photo_frame, lv_color_hex(g_setting.bg_color), 0);
    lv_obj_set_style_bg_opa(ui_photo_frame, LV_OPA_100, 0);
#endif

    lv_obj_add_event_cb(ui_photo_frame, ui_photo_frame_cb, LV_EVENT_ALL, NULL);

    return ui_photo_frame;
}

static void photo_frame_img_list_init(void)
{
    g_list = lv_mem_alloc(sizeof(img_list));
    lv_memset(g_list, 0, sizeof(img_list));

    strncpy(g_list->img_src[0], LVGL_PATH(photo_frame/bg.jpg), 256);
    strncpy(g_list->img_src[1], LVGL_PATH(photo_frame/picture/car.jpg), 256);
    strncpy(g_list->img_src[2], LVGL_PATH(photo_frame/picture/ice.jpg), 256);
    strncpy(g_list->img_src[3], LVGL_PATH(photo_frame/picture/xunfei.jpg), 256);
    strncpy(g_list->img_src[4], LVGL_PATH(photo_frame/picture/machine.jpg), 256);
    strncpy(g_list->img_src[5], LVGL_PATH(photo_frame/picture/ui.jpg), 256);
    strncpy(g_list->img_src[6], LVGL_PATH(photo_frame/picture/beach.jpg), 256);
    

    for (int i = 0; i < 20; i++) {
        if (strlen(g_list->img_src[i]) != 0)
            g_list->img_num++;
    }
    g_list->img_pos = 0;
}

static void photo_frame_ui_setting()
{
    photo_frame_img_list_init();

    g_setting.auto_play = true;
    g_setting.cur_anmi = PHOTO_SWITCH_ANIM_RANDOM;
    g_setting.cur_photo_full_mode = FULL_MODE_NONRMAL;
    g_setting.bg_color = 0xffffff;
}

static void ui_photo_frame_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_current_target(e);

    if (code == LV_EVENT_SCREEN_UNLOAD_START) {
        lv_mem_free(g_list);
        lv_obj_clean(obj);
    }

    if (code == LV_EVENT_DELETE) {;}

    if (code == LV_EVENT_SCREEN_LOADED) {;}
}

int adaptive_width(int width)
{
    float src_width = LV_HOR_RES;
    int align_width = (int)((src_width / 1024.0) * width);
    return align_width;
}

int adaptive_height(int height)
{
    float scr_height = LV_VER_RES;
    int align_height = (int)((scr_height / 600.0) * height);
    return align_height;
}
