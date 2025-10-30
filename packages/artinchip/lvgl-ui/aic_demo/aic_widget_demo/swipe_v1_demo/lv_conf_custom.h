/*
 * Copyright (C) 2025 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * lv_conf_custom.h for custom lv_conf.h file
 * example :
 *     #undef LV_USE_LOG
 *     #define LV_USE_LOG 1
 */

#ifndef LV_CONF_CUSTOM_H
#define LV_CONF_CUSTOM_H

/* code  begin */

#undef LV_USE_FREETYPE
#define LV_USE_FREETYPE 1
#define LV_FREETYPE_CACHE_FT_GLYPH_CNT 64

#if LV_COLOR_DEPTH == 32
#define LV_COLOR_SCREEN_TRANSP 1
#endif

#undef LV_SWIPE_V1
#define LV_SWIPE_V1 1
/* code end */

#endif  /* LV_CONF_CUSTOM_H */
