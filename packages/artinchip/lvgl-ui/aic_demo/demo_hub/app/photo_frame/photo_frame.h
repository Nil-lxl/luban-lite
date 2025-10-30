/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "lvgl.h"

#define OPTIMIZE               1
#define USE_SWITCH_ANIM_COMB   1
#define USE_SWITCH_ANIM_ZOOM   0
#define USE_SWITCH_ANIM_ROTATE 0

enum {
    /* ordinary */
    PHOTO_SWITCH_ANIM_NONE = 0,
    PHOTO_SWITCH_ANIM_FAKE_IN_OUT,
    PHOTO_SWITCH_ANIM_PUSH,
    PHOTO_SWITCH_ANIM_COVER,
    PHOTO_SWITCH_ANIM_UNCOVER,
#if USE_SWITCH_ANIM_ZOOM
    PHOTO_SWITCH_ANIM_ZOOM,
#endif
    /* gorgeous */
    PHOTO_SWITCH_ANIM_FLY_PASE,
#if USE_SWITCH_ANIM_COMB
    PHOTO_SWITCH_ANIM_COMB,
#endif
    PHOTO_SWITCH_ANIM_FLOAT,
#if USE_SWITCH_ANIM_ROTATE
    PHOTO_SWITCH_ANIM_ROTATE,
    PHOTO_SWITCH_ANIM_ROTATE_MOVE,
#endif
    /* random */

    PHOTO_SWITCH_ANIM_RANDOM,

    /* unrealized */
    PHOTO_SWITCH_ANIM_SCALE_BACK,
    PHOTO_SWITCH_ANIM_DISPLAY,
    PHOTO_SWITCH_ANIM_FLOT_IN,
    PHOTO_SWITCH_ANIM_FLIP,
};
typedef int photo_switch_anim;

enum {
    FULL_MODE_NONRMAL = 0, /* Maintain the original horizontal to vertical ratio */
    FULL_MODE_ORIGINAL, /* The dimensions of the original image, Maximum screen size */
    FULL_MODE_FILL_COVER, /* Only V9 supports correct scaling */
};
typedef int photo_full_mode;

typedef struct _photo_frame_setting
{
    int32_t bg_color; /* default: 0xffffff */

    bool auto_play;
    photo_switch_anim cur_anmi;
    photo_full_mode cur_photo_full_mode;
} photo_frame_setting_t;

typedef struct _img_list
{
    char img_src[20][256];
    int16_t img_num;
    int16_t img_pos;
} img_list;

extern img_list *g_list;
extern photo_frame_setting_t g_setting;
extern const char *setting_auto_play_desc;
extern const char *setting_cur_anim_desc;
extern const char *setting_full_mode_desc;

int adaptive_width(int width);
int adaptive_height(int height);
