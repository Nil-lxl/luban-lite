/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#ifndef __LLM_DEMO_VIEW_H
#define __LLM_DEMO_VIEW_H

#include "lvgl.h"
#include "./font/font_init.h"

enum {
    UI_CHAT_SIDE_LEFT,
    UI_CHAT_SIDE_RIGHT
};
typedef int ui_chat_side;

#define MSG_TYPE_LLM_TEXT   0
#define MSG_TYPE_WIFI       1

#define WIFI_START_MSG      "WIFI_START"
#define WIFI_CONNECT_MSG    "WIFI_CONNECT"
#define WIFI_CONNECTED_MSG  "WIFI_CONNECTED"
#define WIFI_CONNECTING_MSG "WIFI_CONNECTING"
#define WIFI_DISCONNECT_MSG "WIFI_DISCONNECT"
#define WIFI_ERROR_MSG      "WIFI_ERROR"

lv_obj_t* chat_ui_create(lv_obj_t* parent);
void chat_ui_message_set_text(lv_obj_t *label, const char *text);
void handle_ui_message(char *msg);

#endif
