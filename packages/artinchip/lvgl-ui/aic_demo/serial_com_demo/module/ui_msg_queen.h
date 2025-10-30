/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  zequan liang <zequan liang@artinchip.com>
 */

#define SIMPLE_MSQ_SIZE 256
typedef struct {
    char msg[SIMPLE_MSQ_SIZE];
    int msg_len;
} ui_message_t;

int ui_msg_queen_init(void);
int ui_msg_queen_send(ui_message_t *msg);
int ui_msg_queen_recv(ui_message_t *msg);
