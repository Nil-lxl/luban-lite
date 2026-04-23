/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 *
 * STA connect, MAC/RSSI, and UDP provisioning utilities (derived from packages/artinchip/wifi-connect).
 * After provisioning, notify upper layer via wifi_svc_provision_done_cb (weak by default, strong symbol in wifi_module).
 */

#ifndef __WIFI_CONNECT_SERVICE_H__
#define __WIFI_CONNECT_SERVICE_H__

#include <rtdef.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** STA netdev name (must match wlan driver) */
#ifndef WIFI_SVC_DEVICE_NAME
#define WIFI_SVC_DEVICE_NAME  "wlan0"
#endif

/** Netdev name for provisioning SoftAP (often wlan1 in dual-band; use wlan0 in single-band projects) */
#ifndef WIFI_SVC_AP_DEVICE_NAME
#define WIFI_SVC_AP_DEVICE_NAME  "wlan1"
#endif

#ifndef WIFI_SVC_PROV_AP_SSID
#define WIFI_SVC_PROV_AP_SSID  "peiwang"
#endif

#ifndef WIFI_SVC_PROV_AP_PSK
#define WIFI_SVC_PROV_AP_PSK   "12345678"
#endif

#ifndef WIFI_SVC_PROV_UDP_PORT
#define WIFI_SVC_PROV_UDP_PORT  5000
#endif

/**
 * Set STA mode (idempotent)
 */
int wifi_svc_set_sta_mode(void);

/**
 * Connect to AP (executed by wlan stack)
 */
int wifi_svc_connect(const char *ssid, const char *psk);

/**
 * Block until link is ready; return RT_FALSE on timeout
 */
rt_bool_t wifi_svc_wait_ready(uint32_t timeout_ms);

/**
 * Fill MAC string in aa:bb:... format
 */
void wifi_svc_get_mac_str(char *out, size_t len);

/**
 * Current RSSI (dBm), returns -127 on failure
 */
int wifi_svc_get_rssi_dbm(void);

/**
 * Whether STA is associated and ready (IP availability depends on stack)
 */
rt_bool_t wifi_svc_is_ready(void);

/**
 * Whether STA is associated with AP (link layer only; usually true after provisioning to avoid duplicate connect)
 */
rt_bool_t wifi_svc_is_sta_connected(void);

/**
 * Start provisioning: bring up SoftAP + UDP service, switch to STA and connect after receiving SSID/password.
 */
void wifi_svc_provision_start(void);

/**
 * Stop provisioning: stop timers and close socket.
 * SoftAP is closed by default; if "keep AP after success" is set, AP stop is skipped.
 */
void wifi_svc_provision_stop(void);

/**
 * Provisioning completion callback: non-zero ok means STA connected to external AP.
 * Default weak empty implementation; wifi_module.c provides strong symbol and forwards MSG_TYPE_WIFI_PROVISION_RESULT.
 */
void wifi_svc_provision_done_cb(const char *ssid, const char *psk, int ok);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_CONNECT_SERVICE_H__ */
