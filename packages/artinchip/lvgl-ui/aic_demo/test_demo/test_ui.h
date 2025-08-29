/*
 * Copyright (c) 2022-2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#pragma once

#include "lvgl.h"
#include "aic_ui.h"

typedef enum {
    LV_COLOR_WHITE = 0xffffff,
    LV_COLOR_BLACK = 0x000000,
    LV_COLOR_RED = 0xff0000,
    LV_COLOR_GREEN = 0x00ff00,
    LV_COLOR_BLUE = 0x0000ff,
    LV_COLOR_PINK = 0xff00f0,

};

void test_ui_init(void);
void create_gray_lvl(void);

void cir_thread_begin(void);

void keyadc_thread_begin(void);

