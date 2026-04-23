/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#ifndef __COMPAT_LAYER_H__
#define __COMPAT_LAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Send WiFi connecting message (legacy framework compatibility)
 */
int send_ui_wifi_connecting_msg(void);

/**
 * Send WiFi connected message (legacy framework compatibility)
 */
int send_thread_wifi_connected_msg(void);

/**
 * Send WiFi connect-failed/disconnected message (legacy framework compatibility)
 */
int send_thread_wifi_disconnected_msg(void);

/**
 * Send network-ready message (legacy framework compatibility)
 */
int send_thread_net_ready_msg(void);

#ifdef __cplusplus
}
#endif

#endif /* __COMPAT_LAYER_H__ */

