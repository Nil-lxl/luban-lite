/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "lvgl.h"
#include "media_list.h"
#include "aic_player.h"

media_list_t *media_list_create(void)
{
    media_list_t *list = NULL;
    list = (media_list_t *)lv_mem_alloc(sizeof(media_list_t));
    if (list == NULL) {
        return NULL;
    }
    memset(list, 0, sizeof(media_list_t));

    return list;
}

int media_list_destroy(media_list_t *list)
{
    if (list) {
        lv_mem_free(list);
        return 0;
    }

    return -1;
}

int media_list_add_info(media_list_t *list, media_info_t *data)
{
    if (list == NULL || data == NULL) {
        return -1;
    }

    int num = list->num;
    if (num >= MEDIA_INFO_MAX) {
        return -1;
    }

    memcpy(&list->info[num], data, sizeof(media_info_t));
    list->num++;

    return 0;
}

int media_list_get_info(media_list_t *list, media_info_t *data, int pos)
{
    if (list == NULL || data == NULL) {
        return -1;
    }

    if (pos == MEDIA_TYPE_POS_LATEST) {
        memcpy(data, &list->info[list->num - 1], sizeof(media_info_t));
        return 0;
    }
    if (pos == MEDIA_TYPE_POS_OLDEST) {
        memcpy(data, &list->info[0], sizeof(media_info_t));
        return 0;
    }

    if (pos >= list->num || pos < 0)
        return -1;

    memcpy(data, &list->info[pos], sizeof(media_info_t));

    return 0;
}

int media_list_get_info_num(media_list_t *list)
{
    if (list == NULL) {
        return -1;
    }

    return list->num;
}

int media_list_set_randomly(media_list_t *list)
{
    if (list == NULL) {
        return -1;
    }

    srand(time(0));

    int num = list->num;
    for(int i = 0; i < num; i++) {
        int j = rand() % (i + 1);

        media_info_t tmp;
        memcpy(&tmp, &list->info[i], sizeof(media_info_t));

        /* swap */
        memcpy(&list->info[i], &list->info[j], sizeof(media_info_t));
        memcpy(&list->info[j], &tmp, sizeof(media_info_t));
    }

    return 0;
}
