/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "lvgl.h"
#include "aic_ui.h"

lv_font_t *montserratmedium_16;
lv_font_t *montserratmedium_20;
lv_font_t *montserratmedium_14;
lv_font_t *montserratmedium_24;

lv_font_t *noto_sanss_chinese_14;
lv_font_t *noto_sanss_chinese_16;
lv_font_t *noto_sanss_chinese_24;

static lv_font_t *_llm_font_create(char *path, int size) {
#if LVGL_VERSION_MAJOR == 8
    static lv_ft_info_t info;
    info.name = path;
    info.weight = size;
    info.style = FT_FONT_STYLE_NORMAL;
    info.mem = NULL;
    if (!lv_ft_font_init(&info))
        return NULL;
    return info.font;
#else
    lv_font_t *font = lv_freetype_font_create(path,
                                              LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                              size,
                                              LV_FREETYPE_FONT_STYLE_NORMAL);
    return font;
#endif //LVGL_VERSION_MAJOR
}

void llm_font_create(void)
{

    montserratmedium_14 = _llm_font_create(LVGL_PATH_ORI(montserratMedium.ttf), 14);
    if (!montserratmedium_14) {
    	LV_LOG_ERROR("Failed to init montserratmedium_14");
    	return;
    }

    montserratmedium_16 = _llm_font_create(LVGL_PATH_ORI(montserratMedium.ttf), 16);
    if (!montserratmedium_16) {
    	LV_LOG_ERROR("Failed to init montserratmedium_16");
    	return;
    }

    montserratmedium_20 = _llm_font_create(LVGL_PATH_ORI(montserratMedium.ttf), 20);
    if (!montserratmedium_20) {
    	LV_LOG_ERROR("Failed to init montserratmedium_20");
    	return;
    }

    montserratmedium_24= _llm_font_create(LVGL_PATH_ORI(montserratMedium.ttf), 24);
    if (!montserratmedium_24) {
    	LV_LOG_ERROR("Failed to init montserratmedium_14");
    	return;
    }

    noto_sanss_chinese_16 = _llm_font_create(LVGL_PATH_ORI(DroidSansFallback.ttf), 16);
    if (!noto_sanss_chinese_16) {
    	LV_LOG_ERROR("Failed to init montserratmedium_24");
    	return;
    }

    noto_sanss_chinese_14 = _llm_font_create(LVGL_PATH_ORI(DroidSansFallback.ttf), 24);
    if (!noto_sanss_chinese_14) {
    	LV_LOG_ERROR("Failed to init montserratmedium_24");
    	return;
    }

    noto_sanss_chinese_24 = _llm_font_create(LVGL_PATH_ORI(DroidSansFallback.ttf), 24);
    if (!noto_sanss_chinese_24) {
    	LV_LOG_ERROR("Failed to init noto_sanss_chinese");
    	return;
    }
}
