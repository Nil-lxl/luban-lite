/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include "menu.h"
#include "global.h"
#include "details.h"
#include "production.h"
#include "stdio.h"
#include "lvgl.h"
#include "coffee.h"

extern int pageNum;

static void dips_size_init();

disp_size_t disp_size;

int pos_width_conversion(int width)
{
    float scr_width = LV_HOR_RES;
    int out = (int)((scr_width / 800.0) * width);
    return out;
}

int pos_height_conversion(int height)
{
    float scr_height = LV_VER_RES;
    return (int)((scr_height / 480.0) * (float)height);
}

lv_obj_t *coffee_ui_init(void)
{
    dips_size_init();
    menuInit();
    detailsInit();
    productionInit();
    return getNowMenuScr();
}

void coffee_ui_del(void)
{
    lv_anim_del_all();
    detailDel();
    menuDel();
    globalSrcDel();
    lv_img_cache_invalidate_src(NULL);
}

static void dips_size_init()
{
    if(LV_HOR_RES <= 480) disp_size = DISP_SMALL;
    else if(LV_HOR_RES <= 800) disp_size = DISP_MEDIUM;
    else disp_size = DISP_LARGE;
}

