/*
 * Copyright (C) 2022-2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "lvgl.h"
#include "player_ui.h"
#include "lv_aic_player.h"

static lv_obj_t *scr;
static lv_obj_t *player;
static lv_obj_t *gif;

void player_ui_init() {
    scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

#if 1
    gif = lv_gif_create(scr);
    lv_gif_set_src(gif, LVGL_PATH(black360.gif));
    lv_gif_set_loop_count(gif, 1);
    lv_obj_align(gif, LV_ALIGN_CENTER, 0, 0);
#else
    player = lv_aic_player_create(scr);
    lv_obj_center(player);
    lv_aic_player_set_src(player, LVGL_PATH(gif / black466.mp4));
    lv_aic_player_set_auto_restart(player, true);
    lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_START, NULL);
#endif

}

void ui_init(void) {
    player_ui_init();
}
