/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#include "wifi_store.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <dfs_posix.h>
#include "cJSON.h"
#include <stdlib.h>
#include <rtdbg.h>

#define DBG_SECTION_NAME "WIFI_STORE"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

static void wifi_store_default(wifi_store_t *o)
{
    memset(o, 0, sizeof(*o));
    o->ver = WIFI_STORE_VER;
    o->ever_connected = RT_FALSE;
}

int wifi_store_load(wifi_store_t *out)
{
    int fd;
    char buf[512];
    int n;
    cJSON *root;
    cJSON *it;

    if (!out)
        return -1;
    wifi_store_default(out);

    fd = open(WIFI_STORE_FILE, O_RDONLY);
    if (fd < 0) {
        LOG_W("wifi_store: no file %s", WIFI_STORE_FILE);
        return -1;
    }
    n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) {
        LOG_W("wifi_store: read empty");
        return -1;
    }
    buf[n] = '\0';

    root = cJSON_Parse(buf);
    if (!root) {
        LOG_E("wifi_store: JSON parse fail");
        return -1;
    }

    it = cJSON_GetObjectItem(root, "ver");
    if (it && cJSON_IsNumber(it))
        out->ver = it->valueint;

    it = cJSON_GetObjectItem(root, "ever_connected");
    if (it && cJSON_IsBool(it))
        out->ever_connected = cJSON_IsTrue(it) ? RT_TRUE : RT_FALSE;

    it = cJSON_GetObjectItem(root, "ssid");
    if (it && cJSON_IsString(it) && it->valuestring)
        strncpy(out->ssid, it->valuestring, sizeof(out->ssid) - 1);

    it = cJSON_GetObjectItem(root, "psk");
    if (it && cJSON_IsString(it) && it->valuestring)
        strncpy(out->psk, it->valuestring, sizeof(out->psk) - 1);

    it = cJSON_GetObjectItem(root, "last_ok_ts");
    if (it && cJSON_IsNumber(it))
        out->last_ok_ts = (uint32_t)it->valueint;

    cJSON_Delete(root);
    LOG_I("wifi_store: loaded ever_connected=%d ssid='%s'", out->ever_connected, out->ssid);
    return 0;
}

int wifi_store_save(const wifi_store_t *in)
{
    cJSON *root;
    char *str;
    int fd;
    int len;
    int w;

    if (!in)
        return -1;

    root = cJSON_CreateObject();
    if (!root)
        return -1;

    cJSON_AddNumberToObject(root, "ver", in->ver ? in->ver : WIFI_STORE_VER);
    cJSON_AddBoolToObject(root, "ever_connected", in->ever_connected ? 1 : 0);
    cJSON_AddStringToObject(root, "ssid", in->ssid);
    cJSON_AddStringToObject(root, "psk", in->psk);
    cJSON_AddNumberToObject(root, "last_ok_ts", (double)in->last_ok_ts);

    str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!str)
        return -1;

    len = strlen(str);
    fd = open(WIFI_STORE_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        free(str);
        LOG_E("wifi_store: open write fail");
        return -1;
    }
    w = write(fd, str, len);
    close(fd);
    free(str);

    if (w != len) {
        LOG_E("wifi_store: write incomplete");
        return -1;
    }
    LOG_I("wifi_store: saved ok");
    return 0;
}

int wifi_store_save_raw(const char *json_str)
{
    int fd;
    int len;
    int w;

    if (!json_str)
        return -1;
    len = strlen(json_str);
    fd = open(WIFI_STORE_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return -1;
    w = write(fd, json_str, len);
    close(fd);
    return (w == len) ? 0 : -1;
}
