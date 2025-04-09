/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include <string.h>
#include "../components/custom_table_ui.h"
#include "layout_table_ui.h"

#include "lvgl/lvgl.h"
#include "aic_ui.h"

cook_book books;

void cook_book_init(cook_book *book)
{
    char name[128] = {0};
    memset(book, 0, sizeof(cook_book));

    for (int i = 0; i < 18; i++) {
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "name:%d", i);
        strncpy(book->name[i], name, sizeof(name));
        book->count++;
    }

    book->update = false;
}

void cook_book_set_ops(cook_book *book, char *name, int ops)
{
    int i = 0;
    for (i = 0; i < 128; i++) {
        if (strncmp(book->name[i], name, 127) == 0)
            break;
    }

    if (i == 128)
        return;

    book->ops[i] = book->ops[i] | COOK_BOOK_OPS_DELETE;
}

void cook_book_clear_ops(cook_book *book, char *name, int ops)
{
    int i = 0;
    for (i = 0; i < 128; i++) {
        if (strncmp(book->name[i], name, 127) == 0)
            break;
    }

    if (i == 128)
        return;

    book->ops[i] = book->ops[i] &(~COOK_BOOK_OPS_DELETE);
}

void cook_book_exec_ops(cook_book *book, int ops)
{
    int i;
    for (i = 0; i < 256; i++) {
        if (ops == COOK_BOOK_OPS_DELETE && book->ops[i] == COOK_BOOK_OPS_DELETE) {
            memset(book->name[i], 0, 128);
            book->ops[i] = 0;
            book->count--;
        }
    }
}

lv_obj_t *layout_table_ui_init(void)
{
    lv_obj_t *layout_ui = lv_obj_create(NULL);

    cook_book_init(&books);
    lv_obj_t *table = simple_table_create(layout_ui);
    for (int i = 0; i < 256; i++) {
        if (strlen(books.name[i]) != 0) {
            simple_table_add(table, books.name[i]);
        }
    }

    return layout_ui;
}
