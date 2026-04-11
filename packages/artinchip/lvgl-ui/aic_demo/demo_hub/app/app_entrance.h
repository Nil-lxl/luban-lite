/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

enum {
    APP_NAVIGATION = 0,
    APP_DASHBOARD,
    APP_AUDIO,
    APP_VIDEO,
    APP_ELEVATOR,
    APP_BENCHMARK,
    APP_CAMERA,
    APP_COFFEE,
    APP_KEYPAD_ENCODER,
    APP_LAYOUT_LIST_EXAMPLE,
    APP_LAYOUT_TABLE_EXAMPLE,
    APP_STEAM_BOX,
    APP_86BOX,
    APP_STRESS,
    APP_WIDGETS,
    APP_PHOTO_FRAME,
    APP_INPUT_TEST,
    APP_NONE,
};
typedef int app_index_t;

void app_entrance(app_index_t index, int auto_del);
app_index_t app_running(void);
