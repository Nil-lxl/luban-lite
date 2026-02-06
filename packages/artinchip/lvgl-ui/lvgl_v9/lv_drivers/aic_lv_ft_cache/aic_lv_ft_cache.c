/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Cui Jiawei <jiawei.cui@artinchip.com>
 */

/**
 * @file aic_lv_ft_cache.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "aic_lv_ft_cache.h"

#if LV_USE_FREETYPE

#include "lv_freetype.h"
#include "lv_freetype_private.h"
#include "../../core/lv_global.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    char *pathname;
    int ref_cnt;
} face_id_node_t;

typedef uint32_t(get_data_size_cb_t)(const void *data);

struct _lv_lru_rb_t {
    lv_cache_t cache;

    lv_rb_t rb;
    lv_ll_t ll;

    get_data_size_cb_t *get_data_size_cb;
};
typedef struct _lv_lru_rb_t lv_lru_rb_t_;
/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_result_t aic_lv_ft_cache_print_stats(void)
{
    lv_freetype_context_t *ctx = lv_freetype_get_context();
    if (!ctx) {
        LV_LOG_ERROR("FreeType context not initialized");
        return LV_RESULT_INVALID;
    }
    face_id_node_t *node;
    uint32_t face_index = 0;

    printf("====================================\n");
    printf(" FreeType Cache Statistics          \n");
    printf("====================================\n");
    printf("Total Cache Size:      %zu\n", lv_cache_get_size(ctx->cache_node_cache, NULL));
    printf("Free Cache Size:       %zu\n", lv_cache_get_free_size(ctx->cache_node_cache, NULL));
    printf("\nFont Faces Details:\n");
    _LV_LL_READ(&ctx->face_id_ll, node)
    {
        printf("  %2" PRIu32 ": %s (ref_cnt: %d)\n", face_index++, node->pathname, node->ref_cnt);
    }

    lv_lru_rb_t_ *lru = (lv_lru_rb_t_ *)ctx->cache_node_cache;
    lv_rb_node_t **rb_node;
    _LV_LL_READ(&lru->ll, rb_node)
    {
        void *search_key = (*rb_node)->data;
        lv_cache_entry_t *entry =
            lv_cache_entry_get_entry(search_key, ctx->cache_node_cache->node_size);

        lv_freetype_cache_node_t *font_cache_node = lv_cache_entry_get_data(entry);

        printf("------------------------------------\n");
        printf("%s\n", font_cache_node->pathname);
        printf("Total ref:                       %" PRId32 "\n", lv_cache_entry_get_ref(entry));
        printf("Total glyph_cache Size:          %zu\n",
               lv_cache_get_size(font_cache_node->glyph_cache, NULL));
        printf("Free glyph_cache Size:           %zu\n\n",
               lv_cache_get_free_size(font_cache_node->glyph_cache, NULL));
        printf("Total draw_data_cache Size:      %zu\n",
               lv_cache_get_size(font_cache_node->draw_data_cache, NULL));
        printf("Free draw_data_cache Size:       %zu\n\n",
               lv_cache_get_free_size(font_cache_node->draw_data_cache, NULL));
        printf("------------------------------------\n");
    }

    if (face_index == 0) {
        printf("  No font faces loaded\n");
    }

    printf("====================================\n");

    return LV_RESULT_OK;
}

lv_result_t aic_lv_ft_cache_drop_all(aic_lv_ft_cache_type_t cache_type)
{
    lv_freetype_context_t *ctx = lv_freetype_get_context();
    if (!ctx) {
        LV_LOG_ERROR("FreeType context not initialized");
        return LV_RESULT_INVALID;
    }

    lv_lru_rb_t_ *lru = (lv_lru_rb_t_ *)ctx->cache_node_cache;
    lv_rb_node_t **rb_node;

    _LV_LL_READ(&lru->ll, rb_node)
    {
        void *search_key = (*rb_node)->data;
        lv_cache_entry_t *entry =
            lv_cache_entry_get_entry(search_key, ctx->cache_node_cache->node_size);
        lv_freetype_cache_node_t *font_cache_node = lv_cache_entry_get_data(entry);
        if ((cache_type & AIC_LV_FT_CACHE_TYPE_GLYPH) && font_cache_node->glyph_cache)
            lv_cache_drop_all(font_cache_node->glyph_cache, NULL);
        if ((cache_type & AIC_LV_FT_CACHE_TYPE_DRAW_DATA) && font_cache_node->draw_data_cache)
            lv_cache_drop_all(font_cache_node->draw_data_cache, NULL);
    }
    return LV_RESULT_OK;
}

lv_result_t aic_lv_ft_cache_drop_specific(const char *pathname, lv_freetype_font_style_t style,
                                          lv_freetype_font_render_mode_t render_mode,
                                          aic_lv_ft_cache_type_t cache_type)
{
    lv_freetype_context_t *ctx = lv_freetype_get_context();
    if (!ctx) {
        LV_LOG_ERROR("FreeType context not initialized");
        return LV_RESULT_INVALID;
    }

    lv_lru_rb_t_ *lru = (lv_lru_rb_t_ *)ctx->cache_node_cache;
    lv_rb_node_t **rb_node;

    _LV_LL_READ(&lru->ll, rb_node)
    {
        void *search_key = (*rb_node)->data;
        lv_cache_entry_t *entry =
            lv_cache_entry_get_entry(search_key, ctx->cache_node_cache->node_size);
        lv_freetype_cache_node_t *font_cache_node = lv_cache_entry_get_data(entry);
        bool match = true;

        if (pathname != NULL)
            if (font_cache_node->pathname == NULL ||
                strcmp(font_cache_node->pathname, pathname) != 0)
                match = false;

        if (match && style != LV_FREETYPE_FONT_STYLE_ANY)
            if (style != font_cache_node->style)
                match = false;

        if (match && render_mode != LV_FREETYPE_FONT_RENDER_MODE_ANY)
            if (render_mode != font_cache_node->render_mode)
                match = false;

        if (match) {
            if ((cache_type & AIC_LV_FT_CACHE_TYPE_GLYPH) && font_cache_node->glyph_cache)
                lv_cache_drop_all(font_cache_node->glyph_cache, NULL);
            if ((cache_type & AIC_LV_FT_CACHE_TYPE_DRAW_DATA) && font_cache_node->draw_data_cache)
                lv_cache_drop_all(font_cache_node->draw_data_cache, NULL);
            LV_LOG_INFO("Dropped font cache entries, path: %s", font_cache_node->pathname);
        }
    }

    return LV_RESULT_OK;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /* LV_USE_FREETYPE */
