/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#ifndef __OTA_AUTH_MODULE_H__
#define __OTA_AUTH_MODULE_H__

#include "../core/module.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get OTA auth module instance
 */
module_t *ota_auth_module_get(void);

/**
 * OTA authentication info (for external modules)
 */
typedef struct {
    char mqtt_endpoint[256];
    char mqtt_client_id[64];
    char mqtt_username[128];
    char mqtt_password[128];
    char mqtt_publish_topic[256];
    char mqtt_subscribe_topic[256];
    char ws_url[256];
    char ws_token[256];
    char firmware_version[32];
    rt_bool_t valid;
} ota_auth_info_t;

/**
 * Get OTA authentication info
 * @return Pointer to OTA info struct, or NULL if not authenticated
 */
ota_auth_info_t *ota_auth_get_info(void);

/**
 * Check whether OTA authentication is completed
 * @return RT_TRUE if authenticated, RT_FALSE otherwise
 */
rt_bool_t ota_auth_is_authenticated(void);

/**
 * Get device MAC address (for external modules)
 */
const char *ota_auth_get_device_mac(void);

/**
 * Get device Client-ID (for external modules)
 */
const char *ota_auth_get_device_client_id(void);

#ifdef __cplusplus
}
#endif

#endif /* __OTA_AUTH_MODULE_H__ */

