/*
 * Copyright (C) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "multi_lang.h"
#include "lang_item.h"
#include "lvgl.h"
#include "aic_ui.h"
#include "screen_white.h"
#include "screen_dark.h"

#define LANG_INI_PATH LVGL_STORAGE_PATH"/lang"

multi_lang_t *multi_ctx = NULL;

void init_lang_style()
{
    static lv_style_t style_chs;
    static lv_style_t style_eng;

    lv_style_init(&style_chs);
    lv_style_init(&style_eng);
#if LV_FONT_SIMSUN_16_CJK == 1
    lv_style_set_text_font(&style_chs, &lv_font_simsun_16_cjk);
#else
    lv_style_set_text_font(&style_chs, &lv_font_montserrat_14);
#endif
    multi_lang_add_style(multi_ctx, "chinese", &style_chs);
    multi_lang_add_style(multi_ctx, "english", &style_eng);
}

void lv_multi_lang_example(void)
{
    multi_ctx = multi_lang_create(LANG_INI_PATH);
    multi_lang_set_language(multi_ctx, "chinese");
    init_lang_style();

    init_screen_white();
    init_screen_dark();

    lv_scr_load(screen_white);
}

void ui_init(void)
{
    lv_multi_lang_example();
}
