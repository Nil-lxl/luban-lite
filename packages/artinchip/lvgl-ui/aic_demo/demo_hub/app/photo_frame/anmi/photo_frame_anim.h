/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#ifndef PHTOTO_FRAME_ANIM_H
#define PHTOTO_FRAME_ANIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/* base anim */
typedef struct _base_anim {
    lv_anim_t cut_in;

    lv_anim_t opa; /* include: style set opa */

    lv_anim_t img_opa;

    lv_anim_t move_x;

    lv_anim_t move_y;

    lv_anim_t rotate;
    lv_coord_t pivot_x;
    lv_coord_t pivot_y;

    lv_anim_t zoom;
    /* The following is only supported by LVGL V9 */
    lv_anim_t zoom_x;

    lv_anim_t zoom_y;
} base_anim_t;

/* photo switch anim */
typedef struct _photo_switch_anmi_sout_out {
    lv_obj_t *sout_out_img_obj0;
    lv_obj_t *sout_out_img_obj1;
    lv_obj_t *sout_out_img_obj2;
    lv_obj_t *sout_out_img_obj3;
    lv_obj_t *sout_out_img_obj4;
    lv_obj_t *sout_out_img_obj5;
    lv_obj_t *sout_out_img_obj6;
    lv_obj_t *sout_out_img_obj7;

    lv_anim_t anim_0;
    lv_anim_t anim_1;
    lv_anim_t anim_2;
    lv_anim_t anim_3;
    lv_anim_t anim_4;
    lv_anim_t anim_5;
    lv_anim_t anim_6;
    lv_anim_t anim_7;
} photo_switch_anmi_sout_out_t;

typedef struct _photo_switch_anim {
    /* anim execpt sout out */
    base_anim_t base_anim_1;
    base_anim_t base_anim_2;

    /* sout out */
    photo_switch_anmi_sout_out_t sout_out_anim;
} photo_switch_anim_t;

/* base anim */
void base_anim_init(base_anim_t *base_anim);
base_anim_t *base_anim_create(void);
void base_anim_del(base_anim_t *base_anim);

void base_anim_cut_in_set(base_anim_t *base_anim, lv_obj_t *img_obj, bool en);
void base_anim_cut_start(base_anim_t *base_anim);

void base_anim_opa_set(base_anim_t *base_anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, uint32_t duration);
void base_anim_opa_start(base_anim_t *base_anim);

void base_anim_img_opa_set(base_anim_t *base_anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, uint32_t duration);
void base_anim_img_opa_start(base_anim_t *base_anim);

void base_anim_move_x_set(base_anim_t *base_anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                          int32_t start_value, int32_t end_value, uint32_t duration);
void base_anim_move_x_start(base_anim_t *base_anim);

void base_anim_move_y_set(base_anim_t *base_anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, uint32_t duration);
void base_anim_move_y_start(base_anim_t *base_anim);

void base_anim_rotate_set(base_anim_t *base_anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, int32_t pivot_x, int32_t pivot_y, uint32_t duration);
void base_anim_rotate_start(base_anim_t *base_anim);

void base_anim_zoom_set(base_anim_t *base_anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, uint32_t duration);
void base_anim_zoom_start(base_anim_t *base_anim);

/* The following is only supported by LVGL V9 */
void base_anim_zoom_x_set(base_anim_t *base_anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, uint32_t duration);
void base_anim_zoom_x_start(base_anim_t *base_anim);

void base_anim_zoom_y_set(base_anim_t *base_anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, uint32_t duration);
void base_anim_zoom_y_start(base_anim_t *base_anim);

/* photo switch anim */
void photo_switch_anim_init(photo_switch_anim_t *photo_switch_anim);
photo_switch_anim_t *photo_switch_anim_create(void);
void photo_switch_anim_del(photo_switch_anim_t *photo_switch_anim);

/* ordinary, img_obj1->img_obj2 */
void photo_switch_anim_none_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_none_start(photo_switch_anim_t *photo_switch_anim);

/* Realized */
void photo_switch_anim_push_from_bottom_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_push_from_bottom_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_push_from_top_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_push_from_top_start(photo_switch_anim_t *photo_switch_anim);

/* unrealized */
void photo_switch_anim_push_from_left_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_push_from_left_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_push_from_right_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_push_from_right_start(photo_switch_anim_t *photo_switch_anim);

/* Realized */
void photo_switch_anim_cover_from_right_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_cover_from_right_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_cover_from_left_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_cover_from_left_start(photo_switch_anim_t *photo_switch_anim);

/* unrealized */
void photo_switch_anim_cover_from_bottom_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_cover_from_bottom_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_cover_from_top_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_cover_from_top_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_cover_from_left_top_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_cover_from_left_top_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_cover_from_left_bottom_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_cover_from_left_bottom_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_cover_from_right_bottom_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_cover_from_right_bottom_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_cover_from_right_top_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_cover_from_right_top_start(photo_switch_anim_t *photo_switch_anim);

/* Realized */
void photo_switch_anim_uncover_from_right_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_uncover_from_right_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_uncover_from_left_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_uncover_from_left_start(photo_switch_anim_t *photo_switch_anim);

/* unrealized */
void photo_switch_anim_uncover_from_bottom_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_uncover_from_bottom_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_uncover_from_top_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_uncover_from_top_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_uncover_from_left_top_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_uncover_from_left_top_start(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_uncover_from_left_bottom_set(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_uncover_from_left_bottom_start(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_uncover_from_right_top_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_uncover_from_right_top_start(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_uncover_from_right_bottom_set(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_uncover_from_right_bottom_start(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);

/* Realized */
void photo_swith_fade_in_out_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_swith_fade_in_out_start(photo_switch_anim_t *photo_switch_anim);
/* unrealized */
void photo_swith_fade_in_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_swith_fade_in_start(photo_switch_anim_t *photo_switch_anim);
void photo_swith_fade_out_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_swith_fade_out_start(photo_switch_anim_t *photo_switch_anim);

/* Realized */
void photo_swith_zoom_enlarge_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_swith_zoom_enlarge_start(photo_switch_anim_t *photo_switch_anim);
void photo_swith_zoom_shrink_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_swith_zoom_shrink_start(photo_switch_anim_t *photo_switch_anim);

/* gorgeous */
void photo_switch_anim_fly_pase_enlarge_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_fly_pase_enlarge_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_fly_pase_shrink_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_fly_pase_shrink_start(photo_switch_anim_t *photo_switch_anim);

void photo_switch_anim_comb_horizontal_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration);
void photo_switch_anim_comb_horizontal_start(photo_switch_anim_t *photo_switch_anim);

void photo_switch_anim_enlarge_rotate_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration);
void photo_switch_anim_enlarge_rotate_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_shrink_rotate_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration);
void photo_switch_anim_shrink_rotate_start(photo_switch_anim_t *photo_switch_anim);

void photo_switch_anim_rotate_move_right_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration);
void photo_switch_anim_rotate_move_right_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_rotate_move_left_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *obj1, lv_obj_t *obj2, uint32_t duration);
void photo_switch_anim_rotate_move_left_start(photo_switch_anim_t *photo_switch_anim);

/* unrealized */
void photo_switch_anim_comb_vertical_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_comb_vertical_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_scale_back_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_scale_back_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_display_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_display_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_float_in_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_float_in_start(photo_switch_anim_t *photo_switch_anim);
void photo_switch_anim_flip_set(photo_switch_anim_t *photo_switch_anim, lv_obj_t *img_obj1, lv_obj_t *img_obj2, uint32_t duration);
void photo_switch_anim_flip_start(photo_switch_anim_t *photo_switch_anim);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //PHTOTO_FRAME_ANIM_H
