/*
 * Copyright (C) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef MULTI_LANG_H
#define MULTI_LANG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "ini.h"

#define MAX_NAME_LENGTH 128
#define MAX_LANG_NAME 32

typedef struct _multi_lang_t {
    char ini_path[MAX_NAME_LENGTH];
    char lang[MAX_LANG_NAME];
    lv_ll_t list;
    ini_t *ini;
} multi_lang_t;

multi_lang_t *multi_lang_create(const char *ini_path);

int multi_lang_set_language(multi_lang_t *multi_ctx, const char *lang);

char *multi_lang_get_language(multi_lang_t *multi_ctx);

const char *multi_lang_get_text_by_key(multi_lang_t *multi_ctx, const char *key);

int multi_lang_add_style(multi_lang_t *multi_ctx, const char *lang, lv_style_t *style);

int multi_lang_remove_style_by_lang(multi_lang_t *multi_ctx, const char *lang);

lv_style_t *multi_lang_get_cur_style(multi_lang_t *multi_ctx);

void multi_lang_destroy(multi_lang_t *multi_ctx);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // MULTI_LANG_H
