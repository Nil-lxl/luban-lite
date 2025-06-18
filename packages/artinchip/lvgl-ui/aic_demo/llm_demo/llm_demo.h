/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "lvgl.h"
#include "aic_ui.h"

#include "./module/llm_answer.h"

enum {
    LLM_DEEPSEEK_V3,
    LLM_DEEPSEEK_R1,
    LLM_DEEPSEEK_DISTALL,
    LLM_TONGYI_QWQ32B,
    LLM_DOUBAO_1_5_256k,
    LLM_DOUBAO_1_5_32k,
};
typedef int llm_type_t;

typedef struct _llm_config {
    llm_config_t cfg;
    void *logo;
    lv_color_t color;
} lv_llm_config_t;

void llm_ui_init(void);
int llm_config_load(llm_type_t type, const char *config_file);
int llm_request_submit(const char *input, lv_obj_t *obj);
lv_color_t llm_get_message_style_color(void);
void * lv_llm_get_message_logo_src(void);
void * lv_llm_get_message_user_src(void);
lv_color_t llm_get_message_user_color(void);
