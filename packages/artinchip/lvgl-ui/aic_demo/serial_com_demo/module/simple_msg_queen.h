/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  zequan liang <zequan liang@artinchip.com>
 */

typedef struct {
    char *msg;
    int msg_len;
} ui_message_t;

int simple_msg_queen_init(void);
int simple_msg_queen_send(ui_message_t *msg);
int simple_msg_queen_recv(ui_message_t *msg);
