/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "lvgl.h"
#include "aic_ui.h"
#include "aic_core.h"
#include "mpp_ge.h"
#include "lv_aic_canvas.h"

#define USE_HW_IMG_BLIT

char *song_text[13] = {
"离开真的残酷吗",
"或者温柔才是可耻的",
"或者孤独的人无所谓",
"无日无夜无条件",
"前面真的危险吗",
"或者背叛才是体贴的",
"或者逃避比较容易吧",
"风言风语风吹沙",
"往前一步是黄昏",
"退后一步是人生",
"风不平浪不静",
"心还不安稳",
"一个岛锁住一个人"
};

static int line_num = 13;
static int canvas_width = 480;
static int canvas_height = 40;
static lv_obj_t *canvas[2] = { NULL };
static lv_font_t * font = NULL;

static int cur_line = 0;
static int cur_id = 0;

static int lv_set_line_txt(int id, int line, const lv_font_t *font);
static void lv_anim_deleted_cb(struct _lv_anim_t *a);

static void anim_angle(void * var, int32_t v)
{
    lv_img_set_angle(var, v * 10);
}

static void anim_x_cb(void * var, int32_t v)
{
    lv_obj_set_x(var, v);
}

static void anim_y_cb(void * var, int32_t v)
{
    lv_obj_set_y(var, v);
}

static void lv_song_txt_in(int id, int start_delay)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, canvas[id]);
    lv_anim_set_delay(&a, start_delay);
    lv_anim_set_time(&a, 1000);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);

    lv_anim_set_exec_cb(&a, anim_x_cb);
    lv_anim_set_values(&a, 10, 300);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, anim_y_cb);
    lv_anim_set_values(&a, 750, 400);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, anim_angle);
    lv_anim_set_values(&a, 320, 360);
    lv_anim_set_deleted_cb(&a, lv_anim_deleted_cb);
    lv_anim_start(&a);

    lv_obj_clear_flag(canvas[id], LV_OBJ_FLAG_HIDDEN);
}

static void lv_song_txt_out(int id)
{
    lv_anim_t amim;
    lv_anim_init(&amim);
    lv_anim_set_var(&amim, canvas[id]);
    lv_anim_set_delay(&amim, 1000);
    lv_anim_set_time(&amim, 1000);
    lv_anim_set_path_cb(&amim, lv_anim_path_linear);
    lv_anim_set_exec_cb(&amim, anim_y_cb);
    lv_anim_set_values(&amim, 400, 50);
    lv_anim_start(&amim);
    lv_obj_clear_flag(canvas[id], LV_OBJ_FLAG_HIDDEN);
}

void lv_song_txt_anim()
{
    int swap_id = cur_id == 0 ? 1: 0;

    cur_line++;
    if (cur_line >= line_num) {
        cur_line = 0;
    }

    lv_set_line_txt(swap_id, cur_line, font);
    lv_song_txt_out(cur_id);
    cur_id = swap_id;
    lv_song_txt_in(swap_id, 1000);
}

static void lv_anim_deleted_cb(struct _lv_anim_t *a)
{
    (void)a;
    //lv_anim_del_all();
    lv_song_txt_anim();
    return;
}

static int lv_set_line_txt(int id, int line, const lv_font_t *font)
{
    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = lv_palette_main(LV_PALETTE_ORANGE);

    if (font) {
        label_dsc.font = font;
    }

    lv_aic_canvas_draw_text_to_center(canvas[id], &label_dsc, song_text[line]);
    return 0;
}

int lv_create_canvas_image(lv_obj_t *parent, int w, int h)
{
    for(int i = 0; i < 2; i++) {
        canvas[i] = lv_aic_canvas_create(parent);
        lv_aic_canvas_alloc_buffer(canvas[i], canvas_width, canvas_height);
        lv_obj_add_flag(canvas[i], LV_OBJ_FLAG_HIDDEN);
    }

    return 0;
}

void lv_canvas_image_text(void)
{
    lv_obj_t *img_bg = lv_img_create(lv_scr_act());

#if LV_COLOR_DEPTH == 32
    lv_img_set_src(img_bg, LVGL_PATH(night.png));
#else
    lv_img_set_src(img_bg, LVGL_PATH(1024x600_night.jpg));
#endif

    lv_obj_set_pos(img_bg, 0, 0);
    lv_obj_center(img_bg);

    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);

#if LV_USE_FREETYPE ==  1
#if LVGL_VERSION_MAJOR == 8
    static lv_ft_info_t info;
    info.name = "/rodata/lvgl_data/font/DroidSansFallback.ttf";
    info.weight = 40;
    info.style = FT_FONT_STYLE_NORMAL;
    info.mem = NULL;
    if(!lv_ft_font_init(&info)) {
        LV_LOG_ERROR("create failed.");
        return;
    }
    font = info.font;
#else
    font = lv_freetype_font_create("/rodata/lvgl_data/font/DroidSansFallback.ttf",
                                    LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                    40,
                                    LV_FREETYPE_FONT_STYLE_NORMAL);
#endif //LVGL_VERSION_MAJOR
#endif //LV_USE_FREETYPE

    lv_create_canvas_image(lv_scr_act(), canvas_width, canvas_height);

    lv_set_line_txt(0, 0, font);
    lv_obj_clear_flag(canvas[0], LV_OBJ_FLAG_HIDDEN);

    lv_song_txt_in(0, 0);
    return;
}
