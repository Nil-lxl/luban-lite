/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#ifndef _AIC_DEMO_MEDIA_H
#define _AIC_DEMO_MEDIA_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PATH_LEN                      64
#define MEDIA_INFO_MAX                    12

#define MEDIA_TYPE_POS_LATEST             MEDIA_INFO_MAX + 1
#define MEDIA_TYPE_POS_OLDEST             MEDIA_INFO_MAX + 2

typedef enum {
    MEDIA_TYPE_AUDIO = 1,
    MEDIA_TYPE_VIDEO,
} media_type;

typedef struct _media_info {
    char name[MAX_PATH_LEN];
    char source[MAX_PATH_LEN];
    media_type type;
    long duration_ms;
    long file_size_bytes;
} media_info_t;

typedef struct _media_list {
    int num;
    media_info_t info[MEDIA_INFO_MAX]; /* media_list */
} media_list_t;

media_list_t *media_list_create(void);
int media_list_destroy(media_list_t *list);
int media_list_add_info(media_list_t *list, media_info_t *data);
int media_list_get_info(media_list_t *list, media_info_t *data, int pos);
int media_list_get_info_num(media_list_t *list);
int media_list_set_randomly(media_list_t *list);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
