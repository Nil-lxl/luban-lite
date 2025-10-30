/*
 * Copyright (C) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef SCREEN_WHITE_H
#define SCREEN_WHITE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t *screen_white;

lv_obj_t *init_screen_white(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // SCREEN_WHITE_H
