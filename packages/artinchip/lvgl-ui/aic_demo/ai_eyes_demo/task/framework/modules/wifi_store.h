/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#ifndef __WIFI_STORE_H__
#define __WIFI_STORE_H__

#include <rtthread.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_STORE_FILE     "/data/wifi_info.json"
#define WIFI_STORE_VER      1
#define WIFI_STORE_SSID_LEN 64
#define WIFI_STORE_PSK_LEN  64

typedef struct {
    int      ver;
    rt_bool_t ever_connected;
    char     ssid[WIFI_STORE_SSID_LEN];
    char     psk[WIFI_STORE_PSK_LEN];
    uint32_t last_ok_ts;
} wifi_store_t;

/**
 * Load from file; on failure set out to defaults (ver=WIFI_STORE_VER, others default-initialized)
 */
int wifi_store_load(wifi_store_t *out);

/**
 * Save to JSON file
 */
int wifi_store_save(const wifi_store_t *in);

/**
 * Write in-memory JSON text to file (useful when provisioning passes a raw JSON string)
 */
int wifi_store_save_raw(const char *json_str);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_STORE_H__ */
