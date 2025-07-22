/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  zequan liang <zequan liang@artinchip.com>
 */
#include "simple_msg_queen.h"
#include <rtthread.h>

static rt_mq_t g_ui_mq;

int simple_msg_queen_init(void)
{
    g_ui_mq = rt_mq_create("ui_mq", sizeof(ui_message_t), 128, RT_IPC_FLAG_FIFO);
    if (g_ui_mq == RT_NULL) {
        rt_kprintf("create message queue failed!\n");
        return -1;
    }
    return 0;
}

int simple_msg_queen_send(ui_message_t *msg)
{
    if(rt_mq_send(g_ui_mq, msg, sizeof(ui_message_t)) == RT_EOK)
        return 0;
    return -1;
}

int simple_msg_queen_recv(ui_message_t * msg)
{
    if(rt_mq_recv(g_ui_mq, msg, sizeof(ui_message_t), 0) == RT_EOK) {
        return 0;
    }

    return -1;
}
