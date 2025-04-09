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

typedef struct _lang_style_t {
    char *name;
    lv_style_t *style;
} lang_style_t;

multi_lang_t *multi_lang_create(const char *ini_path)
{
    int length;
    multi_lang_t *mult = malloc(sizeof(multi_lang_t));

    if (!mult) {
        goto fail;
    }

    memset(mult, 0, sizeof(multi_lang_t));
    _lv_ll_init(&mult->list, sizeof(lang_style_t));

    length = strlen(ini_path);
    if (length > MAX_NAME_LENGTH)
        length = MAX_NAME_LENGTH - 1;

    strncpy(mult->ini_path, ini_path, length);
    return mult;

fail:
    return NULL;
}

int multi_lang_set_language(multi_lang_t *multi_ctx, const char *lang)
{
    int length;
    char path[256];
    multi_lang_t *mult = multi_ctx;

    if (!mult) {
        goto fail;
    }

    if (mult->ini) {
        ini_free(mult->ini);
        mult->ini = NULL;
    }

    length = strlen(lang);
    if (length > MAX_LANG_NAME)
        length = MAX_LANG_NAME - 1;

    strncpy(mult->lang, lang, length);
    snprintf(path, 255, "%s/%s.ini", mult->ini_path, lang);
    printf("lang ini path:%s\n", path);
    mult->ini = ini_load(path);
    if (!mult->ini) {
        LV_LOG_ERROR("ini_load failed");
        goto fail;
    }
    return 0;

fail:
    return -1;
}

char *multi_lang_get_language(multi_lang_t *multi_ctx)
{
    multi_lang_t *mult = multi_ctx;

    if (!mult) {
        goto fail;
    }

    return mult->lang;

fail:
    return NULL;
}

const char *multi_lang_get_text_by_key(multi_lang_t *multi_ctx, const char *key)
{
    multi_lang_t *mult = multi_ctx;

    if (!mult) {
        goto fail;
    }

    if (mult->ini) {
        return ini_get(mult->ini, "lang", key);
    }

fail:
    return NULL;
}

int multi_lang_add_style(multi_lang_t *multi_ctx, const char *lang, lv_style_t *style)
{
    multi_lang_t *mult = multi_ctx;
    char * name = NULL;

    if (!mult) {
        goto fail;
    }

    name = lv_mem_alloc(strlen(lang) + 1);
    if(name == NULL)
        goto fail;

    lang_style_t *item = (lang_style_t *)_lv_ll_ins_head(&mult->list);
    if (item) {
        strcpy(name, lang);
        item->name = name;
        item->style = style;
    }

    return 0;
fail:
    if(name)
        lv_mem_free((void *)name);

    return -1;
}

int multi_lang_remove_style_by_lang(multi_lang_t *multi_ctx, const char *lang)
{
    multi_lang_t *mult = multi_ctx;
    lang_style_t *cur_style;

    if (!mult) {
        goto fail;
    }

    _LV_LL_READ_BACK(&mult->list, cur_style) {
        if (!strcmp(lang, cur_style->name)) {
            _lv_ll_remove(&mult->list, cur_style);
            lv_mem_free(cur_style->name);
            lv_mem_free(cur_style);
            return 0;
        }
    }

fail:
    return -1;
}

lv_style_t *multi_lang_get_cur_style(multi_lang_t *multi_ctx)
{
    multi_lang_t *mult = multi_ctx;
    lang_style_t *cur_style;

    if (!mult) {
        goto fail;
    }

    _LV_LL_READ_BACK(&mult->list, cur_style) {
        if (!strcmp(mult->lang, cur_style->name)) {
            return cur_style->style;
        }
    }

fail:
    return NULL;
}

void multi_lang_destroy(multi_lang_t *multi_ctx)
{
    multi_lang_t *mult = multi_ctx;

    if (!mult) {
        return;
    }

    if (mult->ini) {
        ini_free(mult->ini);
        mult->ini = NULL;
    }

    lang_style_t * cur_style;
    while(mult->list.head) {
        cur_style = _lv_ll_get_head(&mult->list);
        _lv_ll_remove(&mult->list, cur_style);
        lv_mem_free(cur_style->name);
        lv_mem_free(cur_style);
    }
    _lv_ll_clear(&mult->list);

    free(mult);

    return;
}
