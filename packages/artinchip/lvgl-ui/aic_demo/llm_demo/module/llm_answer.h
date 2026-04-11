/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#ifndef __LLM_ANSWER_H__
#define __LLM_ANSWER_H__

#define WEB_HEADER_BUF_SIZE 512
#define WEB_SOCKET_BUF_SIZE 2048

/* message will be pushed to the callback function */
typedef void (*llm_response_callback)(const char *message, void *user_data);
typedef struct _llm_config_t {
    char *prompt;
    char *url;
    char *api_key;
    char *model;
    char *config_file;

    char auth_header[WEB_HEADER_BUF_SIZE];
    char response_buf[WEB_SOCKET_BUF_SIZE];
    char content_buffer[WEB_SOCKET_BUF_SIZE];

    llm_response_callback callback;
    void *callback_para;

    void *user_data;
} llm_config_t;

char *llm_generate_response(llm_config_t *config);
#endif
