/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Cui Jiawei <jiawei.cui@artinchip.com>
 */

/**
 * @file aic_lv_ft_cache.h
 *
 */

#ifndef AIC_LV_FT_CACHE_H
#define AIC_LV_FT_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/
#if LV_USE_FREETYPE

#define LV_FREETYPE_FONT_STYLE_ANY       0xFFFF /* Match any font style */
#define LV_FREETYPE_FONT_RENDER_MODE_ANY 0xFFFF /* Match any render mode */

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    AIC_LV_FT_CACHE_TYPE_GLYPH = 0x01,
    AIC_LV_FT_CACHE_TYPE_DRAW_DATA = 0x02,
    AIC_LV_FT_CACHE_TYPE_ALL = 0x03
} aic_lv_ft_cache_type_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * @brief Print statistics of the FreeType font cache.
 *
 * This function retrieves and prints detailed information about the FreeType font cache,
 * including total and free cache sizes, loaded font faces with their reference counts,
 * and detailed cache information for each font face.
 *
 * @return LV_RESULT_OK if successful, LV_RESULT_INVALID if FreeType context is not initialized.
 */
lv_result_t aic_lv_ft_cache_print_stats(void);

/**
 * @brief Drop cached data from FreeType cache based on cache type.
 *
 * This function clears glyph and/or draw data caches for all font faces currently loaded
 * based on the specified cache type. It preserves the font face references.
 *
 * @param cache_type Type of cache to drop
 * @return LV_RESULT_OK if successful, LV_RESULT_INVALID if FreeType context is not initialized.
 */
lv_result_t aic_lv_ft_cache_drop_all(aic_lv_ft_cache_type_t cache_type);

/**
 * @brief Drop specific font cache entries based on matching criteria
 * 
 * This function clears glyph and/or draw data caches for font faces that match the specified criteria.
 * Non-NULL parameters are used as matching conditions with AND logic.
 *
 * @param pathname Font file path to match, or NULL to match all paths
 * @param style Font style to match, or LV_FREETYPE_FONT_STYLE_ANY to match all styles
 * @param render_mode Font render mode to match, or LV_FREETYPE_FONT_RENDER_MODE_ANY to match all modes
 * @param cache_type Type of cache to drop
 * @return LV_RESULT_OK on success, LV_RESULT_INVALID if FreeType context is not initialized
 */
lv_result_t aic_lv_ft_cache_drop_specific(const char *pathname, lv_freetype_font_style_t style,
                                          lv_freetype_font_render_mode_t render_mode,
                                          aic_lv_ft_cache_type_t cache_type);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_USE_FREETYPE */

#endif /* AIC_LV_FT_CACHE_H */
