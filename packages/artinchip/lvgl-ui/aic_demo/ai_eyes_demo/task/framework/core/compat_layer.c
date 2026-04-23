/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#include "compat_layer.h"
#include "msg_def.h"
#include "msg_bus.h"
#include "module.h"
#include <rtthread.h>
#include <rtdbg.h>

#define DBG_SECTION_NAME "COMPAT"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

/**
 * Send WiFi connecting message (legacy framework compatibility)
 */
int send_ui_wifi_connecting_msg(void)
{
    app_message_t msg;
    rt_memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_WIFI_CONNECTING;
    msg.header.src_module = MODULE_ID_SYS;
    msg.header.dst_module = MODULE_ID_UI;
    msg.header.data_len = sizeof(msg_wifi_data_t);
    msg.data.wifi.state = WIFI_STATE_CONNECTING;
    module_send_msg(&msg);
    return RT_EOK;
}

/**
 * Send WiFi connected message (legacy framework compatibility)
 */
int send_thread_wifi_connected_msg(void)
{
    app_message_t msg;
    rt_memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_WIFI_CONNECTED;
    msg.header.src_module = MODULE_ID_SYS;
    msg.header.dst_module = 0;  /* Broadcast */
    msg.header.data_len = sizeof(msg_wifi_data_t);
    msg.data.wifi.state = WIFI_STATE_CONNECTED;
    module_send_msg(&msg);
    return RT_EOK;
}

/**
 * Send WiFi connect-failed/disconnected message (legacy framework compatibility)
 * Note: current state uses WIFI_STATE_CONNECT_FAILED.
 */
int send_thread_wifi_disconnected_msg(void)
{
    app_message_t msg;
    rt_memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_WIFI_DISCONNECTED;
    msg.header.src_module = MODULE_ID_SYS;
    msg.header.dst_module = 0;  /* Broadcast */
    msg.header.data_len = sizeof(msg_wifi_data_t);
    msg.data.wifi.state = WIFI_STATE_CONNECT_FAILED;
    module_send_msg(&msg);
    return RT_EOK;
}

/**
 * Send network-ready message (legacy framework compatibility)
 */
int send_thread_net_ready_msg(void)
{
    app_message_t msg;
    rt_memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_NET_READY;
    msg.header.src_module = MODULE_ID_SYS;
    msg.header.dst_module = 0;  /* Broadcast */
    msg.header.data_len = 0;
    module_send_msg(&msg);
    return RT_EOK;
}

