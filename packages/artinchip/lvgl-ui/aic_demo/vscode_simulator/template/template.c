/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "aic_ui.h"
#include "lvgl.h"

#define USE_FREETYPE_TEST

static void pic_test(void)
{
    lv_obj_t *hello_world = lv_img_create(lv_scr_act());
    lv_obj_center(hello_world);
    lv_img_set_src(hello_world, LVGL_PATH(template/assets/hello_world.png));
    lv_img_set_zoom(hello_world, 256);
}

#ifdef USE_FREETYPE_TEST
static void freetype_test(void)
{
    lv_font_t *font = NULL;
#if LVGL_VERSION_MAJOR == 8
    /*Create a font*/
    static lv_ft_info_t info;
    /*FreeType uses C standard file system, so no driver letter is required.*/
    info.name = LVGL_PATH_ORI(template/assets/Lato-Regular.ttf);
    info.weight = 24;
    info.style = FT_FONT_STYLE_NORMAL;
    info.mem = NULL;
    if(!lv_ft_font_init(&info)) {
        LV_LOG_ERROR("create failed.");
    }

    font = info.font;
#else
    font = lv_freetype_font_create(LVGL_PATH_ORI(template/assets/Lato-Regular.ttf),
                                    LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                    24,
                                    LV_FREETYPE_FONT_STYLE_NORMAL);
#endif //LVGL_VERSION_MAJOR


    /*Create a label with the new style*/
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello World!");

#if LVGL_VERSION_MAJOR == 8
    lv_label_set_recolor(label, true);
#endif
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label);

}
#endif

void template_ui_init()
{
    pic_test();
#ifdef USE_FREETYPE_TEST
    freetype_test();
#endif
}
