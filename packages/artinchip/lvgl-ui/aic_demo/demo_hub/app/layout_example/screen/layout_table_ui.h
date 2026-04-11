/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#ifndef _AIC_LAYOUT_EXAMPLE_TABLE_UI_H
#define _AIC_LAYOUT_EXAMPLE_TABLE_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "aic_ui.h"

#define COOK_BOOK_OPS_DELETE       (1 << 1)
#define COOK_BOOK_OPS_LIKE         (1 << 2)
#define COOK_BOOK_OPS_PUSH_SERVER  (1 << 3)
#define COOK_BOOK_OPS_CLONE_SERVER (1 << 4)

typedef struct _cook_book {
    char name[256][128];
    char cur_name[128];
    int ops[256];
    int count;
    bool update;
} cook_book;

extern cook_book books;

void cook_book_init(cook_book *book);
void cook_book_exec_ops(cook_book *book, int ops);
void cook_book_set_ops(cook_book *book, char *name, int ops);
void cook_book_clear_ops(cook_book *book, char *name, int ops);

lv_obj_t *layout_table_ui_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
