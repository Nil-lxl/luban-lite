/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "../photo_frame.h"
#include "photo_frame_anim.h"
#include "stdio.h"

#if LVGL_VERSION_MAJOR == 8
#define lv_img_header_t         lv_img_header_t
#else
#define lv_img_header_t         lv_image_header_t
#endif

static void anim_opa_cb(void * var, int32_t v);
static void anim_img_opa_cb(void * var, int32_t v);
static void anim_move_x_cb(void * var, int32_t v);
static void anim_move_y_cb(void * var, int32_t v);
static void anim_rotate_cb(void * var, int32_t v);
static void anim_zoom_cb(void * var, int32_t v);
static void anim_def_free_obj_cb(lv_anim_t *anim);

void base_anim_init(base_anim_t *base_anim)
{
    base_anim->pivot_x = 0;
    base_anim->pivot_y = 0;
}

base_anim_t *base_anim_create(void)
{
    base_anim_t *base = lv_mem_alloc(sizeof(base_anim_t));
    if (!base)
        return NULL;

    lv_memset(base, 0, sizeof(base_anim_t));

    base_anim_init(base);

    return base;
}

void base_anim_del(base_anim_t *base_anim)
{
    if (base_anim)
        lv_mem_free(base_anim);
}

static void _base_anim_init(lv_anim_t *anim, lv_obj_t *img_obj, lv_anim_exec_xcb_t exec_cb, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, uint32_t duration)
{
    lv_anim_init(anim);
    lv_anim_set_var(anim, img_obj);
    lv_anim_set_values(anim, start_value, end_value);
    lv_anim_set_time(anim, duration);
    lv_anim_set_exec_cb(anim, exec_cb);
    lv_anim_set_path_cb(anim, path_cb);
}

static void _base_anim_start(lv_anim_t *anim)
{
    lv_anim_start(anim);
}

void base_anim_cut_in_set(base_anim_t *base_anim, lv_obj_t *img_obj, bool en)
{
    if (en)
        _base_anim_init(&base_anim->cut_in, img_obj, anim_opa_cb, lv_anim_path_ease_in,
                    LV_OPA_100, LV_OPA_100, 1);
    else
        _base_anim_init(&base_anim->cut_in, img_obj, anim_opa_cb, lv_anim_path_ease_in,
                    LV_OPA_0, LV_OPA_0, 1);
}

void base_anim_cut_start(base_anim_t *base_anim)
{
    _base_anim_start(&base_anim->cut_in);
}

void base_anim_opa_set(base_anim_t *base_anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, uint32_t duration)
{
    _base_anim_init(&base_anim->opa, img_obj, anim_opa_cb, path_cb,
                    start_value, end_value, duration);
}

void base_anim_opa_start(base_anim_t *base_anim)
{
    _base_anim_start(&base_anim->opa);
}

void base_anim_img_opa_set(base_anim_t *base_anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, uint32_t duration)
{
    _base_anim_init(&base_anim->img_opa, img_obj, anim_img_opa_cb, path_cb,
                    start_value, end_value, duration);
}

void base_anim_img_opa_start(base_anim_t *base_anim)
{
    _base_anim_start(&base_anim->img_opa);
}

void base_anim_move_x_set(base_anim_t *base_anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, uint32_t duration)
{
    _base_anim_init(&base_anim->move_x, img_obj, anim_move_x_cb, path_cb,
                    start_value, end_value, duration);
}

void base_anim_move_x_start(base_anim_t *base_anim)
{
    _base_anim_start(&base_anim->move_x);
}

void base_anim_move_y_set(base_anim_t *base_anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, uint32_t duration)
{
    _base_anim_init(&base_anim->move_y, img_obj, anim_move_y_cb, path_cb,
                    start_value, end_value, duration);
}

void base_anim_move_y_start(base_anim_t *base_anim)
{
    _base_anim_start(&base_anim->move_y);
}

void base_anim_rotate_set(base_anim_t *base_anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, int32_t pivot_x, int32_t pivot_y, uint32_t duration)
{
    lv_img_set_pivot(img_obj, pivot_x, pivot_y);
    _base_anim_init(&base_anim->rotate, img_obj, anim_rotate_cb, path_cb,
                    start_value, end_value, duration);
}

void base_anim_rotate_start(base_anim_t *base_anim)
{
    _base_anim_start(&base_anim->rotate);
}

void base_anim_zoom_set(base_anim_t *base_anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, uint32_t duration)
{
    _base_anim_init(&base_anim->zoom, img_obj, anim_zoom_cb, path_cb,
                    start_value, end_value, duration);
}

void base_anim_zoom_start(base_anim_t *base_anim)
{
    _base_anim_start(&base_anim->zoom);
}

photo_switch_anim_t *photo_switch_anim_create(void)
{
    photo_switch_anim_t *photo_switch = lv_mem_alloc(sizeof(photo_switch_anim_t));

    if (!photo_switch)
        return NULL;

    lv_memset(photo_switch, 0, sizeof(photo_switch_anim_t));

    base_anim_init(&photo_switch->base_anim_1);
    base_anim_init(&photo_switch->base_anim_2);

    return photo_switch;
}

void photo_switch_anim_init(photo_switch_anim_t *photo_switch_anim)
{
    lv_memset(photo_switch_anim, 0, sizeof(photo_switch_anim_t));

    base_anim_init(&photo_switch_anim->base_anim_1);
    base_anim_init(&photo_switch_anim->base_anim_2);
}

void photo_switch_anim_del(photo_switch_anim_t *photo_switch_anim)
{
    if (photo_switch_anim)
        lv_mem_free(photo_switch_anim);
}

/* ----------------------none----------------------- */
void photo_switch_anim_none_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration) {;}

void photo_switch_anim_none_start(photo_switch_anim_t *photo_switch_anim) {;}

/* ----------------------push----------------------- */
void photo_switch_anim_push_from_bottom_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration)
{
    int16_t start_obj1 = 0;
    int16_t end_obj1 = - LV_VER_RES;

    int16_t start_obj2 = LV_VER_RES;
    int16_t end_obj2 = 0;

    base_anim_move_y_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, start_obj1, end_obj1, duration);
    base_anim_move_y_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, start_obj2, end_obj2, duration);
}

void photo_switch_anim_push_from_bottom_start(photo_switch_anim_t *photo_switch_anim)
{
    base_anim_move_y_start(&photo_switch_anim->base_anim_1);
    base_anim_move_y_start(&photo_switch_anim->base_anim_2);
}

void photo_switch_anim_push_from_top_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration)
{
    int16_t start_obj1 = 0;
    int16_t end_obj1 = LV_VER_RES;

    int16_t start_obj2 = -LV_VER_RES;
    int16_t end_obj2 = 0;

    base_anim_move_y_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, start_obj1, end_obj1, duration);
    base_anim_move_y_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, start_obj2, end_obj2, duration);
}

void photo_switch_anim_push_from_top_start(photo_switch_anim_t *photo_switch_anim)
{
    base_anim_move_y_start(&photo_switch_anim->base_anim_1);
    base_anim_move_y_start(&photo_switch_anim->base_anim_2);
}

/* ----------------------cover----------------------- */
void photo_switch_anim_cover_from_right_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration)
{
    int16_t start_obj2 = LV_HOR_RES;
    int16_t end_obj2 = 0;

    base_anim_move_x_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, start_obj2, end_obj2, duration);
}

void photo_switch_anim_cover_from_right_start(photo_switch_anim_t *photo_switch_anim)
{
    base_anim_move_x_start(&photo_switch_anim->base_anim_2);
}

void photo_switch_anim_cover_from_left_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration)
{
    int16_t start_obj2 = -LV_HOR_RES;
    int16_t end_obj2 = 0;

    base_anim_move_x_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, start_obj2, end_obj2, duration);
}

void photo_switch_anim_cover_from_left_start(photo_switch_anim_t *photo_switch_anim)
{
    base_anim_move_x_start(&photo_switch_anim->base_anim_2);
}

/* ----------------------uncover----------------------- */
void photo_switch_anim_uncover_from_right_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration)
{
    int32_t start_obj1, end_obj1;

    start_obj1 = 0;
    end_obj1 = - LV_HOR_RES;

    base_anim_move_x_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, start_obj1, end_obj1, duration);
}

void photo_switch_anim_uncover_from_right_start(photo_switch_anim_t *photo_switch_anim)
{
    base_anim_move_x_start(&photo_switch_anim->base_anim_1);
}

void photo_switch_anim_uncover_from_left_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration)
{
    int32_t start_obj1, end_obj1;

    start_obj1 = 0;
    end_obj1 = LV_HOR_RES;

    base_anim_move_x_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, start_obj1, end_obj1, duration);
}

void photo_switch_anim_uncover_from_left_start(photo_switch_anim_t *photo_switch_anim)
{
    base_anim_move_x_start(&photo_switch_anim->base_anim_1);
}

/* ----------------------fade----------------------- */
void photo_swith_fade_in_out_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration)
{
    base_anim_img_opa_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, LV_OPA_COVER, LV_OPA_TRANSP, duration);
    base_anim_img_opa_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, LV_OPA_TRANSP, LV_OPA_COVER, duration);
}

void photo_swith_fade_in_out_start(photo_switch_anim_t *photo_switch_anim)
{
    base_anim_img_opa_start(&photo_switch_anim->base_anim_1);
    base_anim_img_opa_start(&photo_switch_anim->base_anim_2);
}

/* ----------------------zoom----------------------- */
void photo_swith_zoom_enlarge_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration)
{
    lv_obj_t *img_obj1 = lv_obj_get_child(obj1, 0);
    lv_obj_t *img_obj2 = lv_obj_get_child(obj2, 0);
    int16_t ori_zoom_1 = lv_img_get_zoom(img_obj1);
    int16_t ori_zoom_2 = lv_img_get_zoom(img_obj2);

    float zoom_magnification = 2.0;
    base_anim_zoom_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, ori_zoom_1, ori_zoom_1 * zoom_magnification, duration);
    base_anim_zoom_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, ori_zoom_2 * (1 / zoom_magnification), ori_zoom_2, duration);
}

void photo_swith_zoom_enlarge_start(photo_switch_anim_t *photo_switch_anim)
{
    base_anim_zoom_start(&photo_switch_anim->base_anim_1);
    base_anim_zoom_start(&photo_switch_anim->base_anim_2);
}

void photo_swith_zoom_shrink_set(photo_switch_anim_t *photo_switch_anim,  lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration)
{
    lv_obj_t *img_obj1 = lv_obj_get_child(obj1, 0);
    lv_obj_t *img_obj2 = lv_obj_get_child(obj2, 0);
    int16_t ori_zoom_1 = lv_img_get_zoom(img_obj1);
    int16_t ori_zoom_2 = lv_img_get_zoom(img_obj2);

    float zoom_magnification = 2.0;
    base_anim_zoom_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, ori_zoom_1, ori_zoom_1 * (1 / zoom_magnification), duration);
    base_anim_zoom_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, ori_zoom_2 * zoom_magnification, ori_zoom_2, duration);
}

void photo_swith_zoom_shrink_start(photo_switch_anim_t *photo_switch_anim)
{
    base_anim_zoom_start(&photo_switch_anim->base_anim_1);
    base_anim_zoom_start(&photo_switch_anim->base_anim_2);
}

/* ----------------------fly pase----------------------- */
void photo_switch_anim_fly_pase_enlarge_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration)
{
    lv_obj_t *img_obj1 = lv_obj_get_child(obj1, 0);
    lv_obj_t *img_obj2 = lv_obj_get_child(obj2, 0);
    int16_t ori_zoom_1 = lv_img_get_zoom(img_obj1);
    int16_t ori_zoom_2 = lv_img_get_zoom(img_obj2);

    float zoom_magnification = 2.0;
    base_anim_zoom_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, ori_zoom_1, ori_zoom_1 * zoom_magnification, duration);
    base_anim_zoom_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, ori_zoom_2 * (1 / zoom_magnification), ori_zoom_2, duration);
    base_anim_img_opa_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, LV_OPA_COVER, LV_OPA_TRANSP, duration);
    base_anim_img_opa_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, LV_OPA_TRANSP, LV_OPA_COVER, duration);
}

void photo_switch_anim_fly_pase_enlarge_start(photo_switch_anim_t *photo_switch_anim)
{
    base_anim_zoom_start(&photo_switch_anim->base_anim_1);
    base_anim_zoom_start(&photo_switch_anim->base_anim_2);
    base_anim_img_opa_start(&photo_switch_anim->base_anim_1);
    base_anim_img_opa_start(&photo_switch_anim->base_anim_2);
}

void photo_switch_anim_fly_pase_shrink_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration)
{
    lv_obj_t *img_obj1 = lv_obj_get_child(obj1, 0);
    lv_obj_t *img_obj2 = lv_obj_get_child(obj2, 0);
    int16_t ori_zoom_1 = lv_img_get_zoom(img_obj1);
    int16_t ori_zoom_2 = lv_img_get_zoom(img_obj2);

    float zoom_magnification = 2.0;
    base_anim_zoom_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, ori_zoom_1, ori_zoom_1 * (1 / zoom_magnification), duration);
    base_anim_zoom_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, ori_zoom_2 * zoom_magnification, ori_zoom_2, duration);
    base_anim_img_opa_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, LV_OPA_COVER, LV_OPA_TRANSP, duration);
    base_anim_img_opa_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, LV_OPA_TRANSP, LV_OPA_COVER, duration);
}

void photo_switch_anim_fly_pase_shrink_start(photo_switch_anim_t *photo_switch_anim)
{
    base_anim_zoom_start(&photo_switch_anim->base_anim_1);
    base_anim_zoom_start(&photo_switch_anim->base_anim_2);
    base_anim_img_opa_start(&photo_switch_anim->base_anim_1);
    base_anim_img_opa_start(&photo_switch_anim->base_anim_2);
}
/* ----------------------comb----------------------- */
static void photo_switch_anim_comb_init(lv_anim_t *anim, lv_obj_t *img_obj, int32_t start_value, int32_t end_value, uint32_t duration, uint32_t delay)
{
    lv_anim_init(anim);
    lv_anim_set_var(anim, img_obj);
    lv_anim_set_values(anim, start_value, end_value);
    lv_anim_set_time(anim, duration);
    lv_anim_set_delay(anim, delay);
    lv_anim_set_exec_cb(anim, anim_move_x_cb);
    lv_anim_set_path_cb(anim, lv_anim_path_linear);
    lv_anim_set_deleted_cb(anim, anim_def_free_obj_cb);
}

/* Divide the image into six parts.
   pos: 0~7, from bottm to top
*/
static lv_obj_t *photo_switch_obj_comb_create(lv_obj_t *obj, int pos)
{
    lv_obj_t *container = lv_obj_create(obj);
    lv_obj_remove_style_all(container);
    lv_obj_set_style_bg_color(container, lv_color_hex(g_setting.bg_color), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_100, 0);
    lv_obj_set_size(container, LV_HOR_RES, LV_VER_RES / 8);
    lv_obj_set_align(container, LV_ALIGN_TOP_MID);
    lv_obj_set_pos(container, 0, pos * LV_VER_RES / 8);

    lv_obj_t *img_obj = lv_obj_get_child(obj, 0);

    lv_img_header_t header;
    lv_img_decoder_get_info(lv_img_get_src(img_obj), &header);
    int32_t img_actual_height = header.h;
    lv_coord_t part_img_pos_0 = (LV_VER_RES / 2) - (img_actual_height / 2);

    lv_obj_t *part_img_obj = lv_img_create(container);
    lv_img_set_src(part_img_obj, lv_img_get_src(img_obj));
    lv_img_set_zoom(part_img_obj, lv_img_get_zoom(img_obj));
    lv_obj_set_align(part_img_obj, LV_ALIGN_TOP_MID);
    lv_obj_set_pos(part_img_obj, 0, part_img_pos_0 - (pos * (LV_VER_RES / 8)));

    return container;
}

void photo_switch_anim_comb_horizontal_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration)
{
    int32_t delay;
    photo_switch_anmi_sout_out_t *sout_out;

    lv_obj_t *img_obj1 = lv_obj_get_child(obj1, 0);
    lv_obj_t *img_obj2 = lv_obj_get_child(obj2, 0);

    base_anim_cut_in_set(&photo_switch_anim->base_anim_1, img_obj1, 1);
    base_anim_cut_in_set(&photo_switch_anim->base_anim_2, img_obj2, 1);

    sout_out = &photo_switch_anim->sout_out_anim;
    sout_out->sout_out_img_obj0 = photo_switch_obj_comb_create(obj1, 0);
    lv_obj_set_parent(sout_out->sout_out_img_obj0, obj2); /*Let the object cover in front of obj2*/
    sout_out->sout_out_img_obj1 = photo_switch_obj_comb_create(obj1, 1);
    lv_obj_set_parent(sout_out->sout_out_img_obj1, obj2);
    sout_out->sout_out_img_obj2 = photo_switch_obj_comb_create(obj1, 2);
    lv_obj_set_parent(sout_out->sout_out_img_obj2, obj2);
    sout_out->sout_out_img_obj3 = photo_switch_obj_comb_create(obj1, 3);
    lv_obj_set_parent(sout_out->sout_out_img_obj3, obj2);
    sout_out->sout_out_img_obj4 = photo_switch_obj_comb_create(obj1, 4);
    lv_obj_set_parent(sout_out->sout_out_img_obj4, obj2);
    sout_out->sout_out_img_obj5 = photo_switch_obj_comb_create(obj1, 5);
    lv_obj_set_parent(sout_out->sout_out_img_obj5, obj2);
    sout_out->sout_out_img_obj6 = photo_switch_obj_comb_create(obj1, 6);
    lv_obj_set_parent(sout_out->sout_out_img_obj6, obj2);
    sout_out->sout_out_img_obj7 = photo_switch_obj_comb_create(obj1, 7);
    lv_obj_set_parent(sout_out->sout_out_img_obj7, obj2);

    delay = (int32_t)(duration * 7.0 / 8.0);
    photo_switch_anim_comb_init(&sout_out->anim_0, sout_out->sout_out_img_obj0, 0, -LV_HOR_RES, duration - delay, delay);
    delay = (int32_t)(duration * 6.0 / 8.0);
    photo_switch_anim_comb_init(&sout_out->anim_1, sout_out->sout_out_img_obj1, 0, LV_HOR_RES, duration - delay, delay);
    delay = (int32_t)(duration * 5.0 / 8.0);
    photo_switch_anim_comb_init(&sout_out->anim_2, sout_out->sout_out_img_obj2, 0, -LV_HOR_RES, duration - delay, delay);
    delay = (int32_t)(duration * 4.0 / 8.0);
    photo_switch_anim_comb_init(&sout_out->anim_3, sout_out->sout_out_img_obj3, 0, LV_HOR_RES, duration - delay, delay);
    delay = (int32_t)(duration * 3.0 / 8.0);
    photo_switch_anim_comb_init(&sout_out->anim_4, sout_out->sout_out_img_obj4, 0, -LV_HOR_RES, duration - delay, delay);
    delay = (int32_t)(duration * 2.0 / 8.0);
    photo_switch_anim_comb_init(&sout_out->anim_5, sout_out->sout_out_img_obj5, 0, LV_HOR_RES, duration - delay, delay);
    delay = (int32_t)(duration * 1.0 / 8.0);
    photo_switch_anim_comb_init(&sout_out->anim_6, sout_out->sout_out_img_obj6, 0, -LV_HOR_RES, duration - delay, delay);
    delay = (int32_t)(0);
    photo_switch_anim_comb_init(&sout_out->anim_7, sout_out->sout_out_img_obj7, 0, LV_HOR_RES, duration - delay, delay);
}

void photo_switch_anim_comb_horizontal_start(photo_switch_anim_t *photo_switch_anim)
{
    base_anim_cut_start(&photo_switch_anim->base_anim_1);
    base_anim_cut_start(&photo_switch_anim->base_anim_2);

    lv_anim_start(&photo_switch_anim->sout_out_anim.anim_0);
    lv_anim_start(&photo_switch_anim->sout_out_anim.anim_1);
    lv_anim_start(&photo_switch_anim->sout_out_anim.anim_3);
    lv_anim_start(&photo_switch_anim->sout_out_anim.anim_2);
    lv_anim_start(&photo_switch_anim->sout_out_anim.anim_4);
    lv_anim_start(&photo_switch_anim->sout_out_anim.anim_5);
    lv_anim_start(&photo_switch_anim->sout_out_anim.anim_6);
    lv_anim_start(&photo_switch_anim->sout_out_anim.anim_7);
}

void photo_switch_anim_enlarge_rotate_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration)
{
    lv_obj_t *img_obj1 = lv_obj_get_child(obj1, 0);
    lv_obj_t *img_obj2 = lv_obj_get_child(obj2, 0);
    int16_t ori_zoom_1 = lv_img_get_zoom(img_obj1);
    int16_t ori_zoom_2 = lv_img_get_zoom(img_obj2);

    float zoom_magnification = 2.0;
    base_anim_zoom_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, ori_zoom_1, ori_zoom_1 * zoom_magnification, duration);
    base_anim_zoom_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, ori_zoom_2 * (1 / zoom_magnification), ori_zoom_2, duration);
    base_anim_img_opa_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, LV_OPA_COVER, LV_OPA_TRANSP, duration);
    base_anim_img_opa_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, LV_OPA_TRANSP, LV_OPA_COVER, duration);
    base_anim_rotate_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_ease_in_out, 0, 1800, LV_HOR_RES / 2, LV_VER_RES / 2, duration);
    base_anim_rotate_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_ease_in_out, 1800, 0, LV_HOR_RES / 2, LV_VER_RES / 2, duration);
}

void photo_switch_anim_enlarge_rotate_start(photo_switch_anim_t *photo_switch_anim)
{
#if OPTIMIZE
    if (LV_HOR_RES < 1024) {
        base_anim_zoom_start(&photo_switch_anim->base_anim_1);
        base_anim_zoom_start(&photo_switch_anim->base_anim_2);
    }
#else
    base_anim_zoom_start(&photo_switch_anim->base_anim_1);
    base_anim_zoom_start(&photo_switch_anim->base_anim_2);
#endif
    base_anim_rotate_start(&photo_switch_anim->base_anim_1);
    base_anim_rotate_start(&photo_switch_anim->base_anim_2);
    base_anim_img_opa_start(&photo_switch_anim->base_anim_1);
    base_anim_img_opa_start(&photo_switch_anim->base_anim_2);
}

void photo_switch_anim_shrink_rotate_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration)
{
    lv_obj_t *img_obj1 = lv_obj_get_child(obj1, 0);
    lv_obj_t *img_obj2 = lv_obj_get_child(obj2, 0);
    int16_t ori_zoom_1 = lv_img_get_zoom(img_obj1);
    int16_t ori_zoom_2 = lv_img_get_zoom(img_obj2);

    float zoom_magnification = 2.0;
    base_anim_zoom_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, ori_zoom_1, ori_zoom_1 * (1 / zoom_magnification), duration);
    base_anim_zoom_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, ori_zoom_2 * zoom_magnification, ori_zoom_2, duration);
    base_anim_img_opa_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, LV_OPA_COVER, LV_OPA_TRANSP, duration);
    base_anim_img_opa_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, LV_OPA_TRANSP, LV_OPA_COVER, duration);
    base_anim_rotate_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_ease_in_out, 0, 1800, LV_HOR_RES / 2, LV_VER_RES / 2, duration);
    base_anim_rotate_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_ease_in_out, 1800, 0, LV_HOR_RES / 2, LV_VER_RES / 2, duration);
}

void photo_switch_anim_shrink_rotate_start(photo_switch_anim_t *photo_switch_anim)
{
#if OPTIMIZE
    if (LV_HOR_RES < 1024) {
        base_anim_zoom_start(&photo_switch_anim->base_anim_1);
        base_anim_zoom_start(&photo_switch_anim->base_anim_2);
    }
#else
    base_anim_zoom_start(&photo_switch_anim->base_anim_1);
    base_anim_zoom_start(&photo_switch_anim->base_anim_2);
#endif
    base_anim_rotate_start(&photo_switch_anim->base_anim_1);
    base_anim_rotate_start(&photo_switch_anim->base_anim_2);
    base_anim_img_opa_start(&photo_switch_anim->base_anim_1);
    base_anim_img_opa_start(&photo_switch_anim->base_anim_2);
}

void photo_switch_anim_rotate_move_right_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration)
{
    lv_obj_t *img_obj1 = lv_obj_get_child(obj1, 0);
    lv_obj_t *img_obj2 = lv_obj_get_child(obj2, 0);

    base_anim_img_opa_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, LV_OPA_COVER, LV_OPA_TRANSP, duration);
    base_anim_img_opa_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, LV_OPA_TRANSP, LV_OPA_COVER, duration);
    base_anim_rotate_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_ease_in_out, 0, 1800, LV_HOR_RES / 2, LV_VER_RES / 2, duration);
    base_anim_rotate_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_ease_in_out, 1800, 0, LV_HOR_RES / 2, LV_VER_RES / 2, duration);
    base_anim_move_x_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_ease_in_out, 0, LV_HOR_RES, duration);
    base_anim_move_x_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_ease_in_out, -LV_HOR_RES, 0, duration);
    base_anim_move_y_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_ease_in_out, 0, LV_VER_RES, duration);
    base_anim_move_y_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_ease_in_out, -LV_VER_RES, 0, duration);
}

void photo_switch_anim_rotate_move_right_start(photo_switch_anim_t *photo_switch_anim)
{
#if OPTIMIZE
    if (LV_HOR_RES < 1024) {
        base_anim_img_opa_start(&photo_switch_anim->base_anim_1);
        base_anim_img_opa_start(&photo_switch_anim->base_anim_2);
    }
#else
    base_anim_img_opa_start(&photo_switch_anim->base_anim_1);
    base_anim_img_opa_start(&photo_switch_anim->base_anim_2);
#endif
    base_anim_rotate_start(&photo_switch_anim->base_anim_1);
    base_anim_rotate_start(&photo_switch_anim->base_anim_2);
    base_anim_move_x_start(&photo_switch_anim->base_anim_1);
    base_anim_move_x_start(&photo_switch_anim->base_anim_2);
    base_anim_move_y_start(&photo_switch_anim->base_anim_1);
    base_anim_move_y_start(&photo_switch_anim->base_anim_2);
}

void photo_switch_anim_rotate_move_left_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration)
{
    lv_obj_t *img_obj1 = lv_obj_get_child(obj1, 0);
    lv_obj_t *img_obj2 = lv_obj_get_child(obj2, 0);

    base_anim_img_opa_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_linear, LV_OPA_COVER, LV_OPA_TRANSP, duration);
    base_anim_img_opa_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_linear, LV_OPA_TRANSP, LV_OPA_COVER, duration);
    base_anim_rotate_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_ease_in_out, 0, 1800, LV_HOR_RES / 2, LV_VER_RES / 2, duration);
    base_anim_rotate_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_ease_in_out, 1800, 0, LV_HOR_RES / 2, LV_VER_RES / 2, duration);
    base_anim_move_x_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_ease_in_out, 0, -LV_HOR_RES, duration);
    base_anim_move_x_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_ease_in_out, LV_HOR_RES, 0, duration);
    base_anim_move_y_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_ease_in_out, 0, -LV_VER_RES, duration);
    base_anim_move_y_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_ease_in_out, LV_VER_RES, 0, duration);
}

void photo_switch_anim_rotate_move_left_start(photo_switch_anim_t *photo_switch_anim)
{
#if OPTIMIZE
    if (LV_HOR_RES < 1024) {
        base_anim_img_opa_start(&photo_switch_anim->base_anim_1);
        base_anim_img_opa_start(&photo_switch_anim->base_anim_2);
    }
#else
    base_anim_img_opa_start(&photo_switch_anim->base_anim_1);
    base_anim_img_opa_start(&photo_switch_anim->base_anim_2);
#endif
    base_anim_rotate_start(&photo_switch_anim->base_anim_1);
    base_anim_rotate_start(&photo_switch_anim->base_anim_2);
    base_anim_move_x_start(&photo_switch_anim->base_anim_1);
    base_anim_move_x_start(&photo_switch_anim->base_anim_2);
    base_anim_move_y_start(&photo_switch_anim->base_anim_1);
    base_anim_move_y_start(&photo_switch_anim->base_anim_2);
}

void photo_switch_anim_float_in_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration)
{
    lv_obj_t *img_obj1 = lv_obj_get_child(obj1, 0);
    lv_obj_t *img_obj2 = lv_obj_get_child(obj2, 0);

    base_anim_img_opa_set(&photo_switch_anim->base_anim_1, img_obj1, lv_anim_path_ease_out, LV_OPA_COVER, LV_OPA_TRANSP, duration);
    base_anim_img_opa_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_ease_out, LV_OPA_TRANSP, LV_OPA_COVER, duration);
    base_anim_move_y_set(&photo_switch_anim->base_anim_2, img_obj2, lv_anim_path_ease_out, LV_VER_RES / 5, 0, duration);
}

void photo_switch_anim_float_in_start(photo_switch_anim_t *photo_switch_anim)
{
    base_anim_img_opa_start(&photo_switch_anim->base_anim_1);
    base_anim_img_opa_start(&photo_switch_anim->base_anim_2);
    base_anim_move_y_start(&photo_switch_anim->base_anim_2);
}

static void anim_opa_cb(void * var, int32_t v)
{
    lv_obj_set_style_opa(var, v, LV_PART_MAIN);
}

static void anim_img_opa_cb(void * var, int32_t v)
{
    lv_obj_set_style_img_opa(var, v, LV_PART_MAIN);
}

static void anim_move_x_cb(void * var, int32_t v)
{
    lv_obj_set_x(var, v);
}

static void anim_move_y_cb(void * var, int32_t v)
{
    lv_obj_set_y(var, v);
}

static void anim_rotate_cb(void * var, int32_t v)
{
    lv_img_set_angle(var, v);
}

static void anim_zoom_cb(void * var, int32_t v)
{
    lv_img_set_zoom(var, v);
}

static void anim_def_free_obj_cb(lv_anim_t *anim)
{
    lv_obj_del(anim->var);
}
