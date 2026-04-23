/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#ifndef __WIFI_MODULE_H__
#define __WIFI_MODULE_H__

#include "../core/module.h"

#ifdef __cplusplus
extern "C" {
#endif

module_t *wifi_module_get(void);

/**
 * Callable from any context: request immediate provisioning (handled by WiFi thread)
 */
void wifi_module_request_provision(void);

/**
 * Called by provisioning service after obtaining SSID/password; non-zero ok means success
 * On success, JSON is saved and STA connection is attempted (up to 2 times)
 */
void wifi_module_notify_provision_result(const char *ssid, const char *psk, int ok);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_MODULE_H__ */
