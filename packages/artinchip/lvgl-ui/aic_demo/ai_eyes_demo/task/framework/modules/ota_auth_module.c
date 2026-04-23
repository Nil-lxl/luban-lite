/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#include "ota_auth_module.h"
#include "../core/module.h"
#include "../core/msg_def.h"
#include "../core/msg_bus.h"
#include <rtthread.h>
#include <webclient.h>
#include <cJSON.h>
#include <string.h>
#include <stdlib.h>
#include <lwip/netif.h>
#include <rtdbg.h>

#define DBG_SECTION_NAME "OTA_AUTH_MOD"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

/* OTA server config (default values, can be overridden by config) */
#ifndef XIAOZHI_OTA_SERVER_URL
#define XIAOZHI_OTA_SERVER_URL              "https://api.tenclass.net/xiaozhi/ota/"
#endif

#ifndef XIAOZHI_OTA_RESPONSE_BUFFER_SIZE
#define XIAOZHI_OTA_RESPONSE_BUFFER_SIZE    4096
#endif

#ifndef XIAOZHI_OTA_HEADER_BUFFER_SIZE
#define XIAOZHI_OTA_HEADER_BUFFER_SIZE      512
#endif

/* Device info config (default values) */
#ifndef XIAOZHI_DEVICE_VERSION
#define XIAOZHI_DEVICE_VERSION              "1.7.2"
#endif

#ifndef XIAOZHI_DEVICE_BOARD_NAME
#define XIAOZHI_DEVICE_BOARD_NAME           "xiaozhi-test"
#endif

#ifndef XIAOZHI_DEVICE_MAC_ADDRESS
#define XIAOZHI_DEVICE_MAC_ADDRESS          "00:22:44:67:88:25"
#endif

#ifndef XIAOZHI_DEVICE_CLIENT_ID
#define XIAOZHI_DEVICE_CLIENT_ID            "3d905bc5-c80a-4062-824e-ca2f64ee6198"
#endif

// Use configured definitions
#define XIAOZHI_OTA_URL             XIAOZHI_OTA_SERVER_URL
#define XIAOZHI_OTA_RESPONSE_BUFSZ  XIAOZHI_OTA_RESPONSE_BUFFER_SIZE
#define XIAOZHI_OTA_HEADER_BUFSZ    XIAOZHI_OTA_HEADER_BUFFER_SIZE
#define DEVICE_VERSION              XIAOZHI_DEVICE_VERSION
#define DEVICE_BOARD_NAME           XIAOZHI_DEVICE_BOARD_NAME
#define DEVICE_MAC_ADDRESS_DEFAULT  XIAOZHI_DEVICE_MAC_ADDRESS
#define DEVICE_CLIENT_ID_DEFAULT    XIAOZHI_DEVICE_CLIENT_ID

/* OTA auth module private data */
typedef struct {
    ota_auth_info_t ota_info;
    char device_mac_address[18];
    char device_client_id[40];
    rt_bool_t initialized;
    rt_bool_t authenticating;  /* Authentication in progress */
    rt_bool_t authenticated;   /* Authentication completed successfully */
} ota_auth_module_priv_t;

static ota_auth_module_priv_t ota_auth_priv = {0};

/* Get MAC address from WiFi netif */
static int ota_auth_get_wifi_mac_address(char *mac_str, size_t mac_str_len)
{
    struct netif *netif = NULL;

    if (!mac_str || mac_str_len < 18) {
        return -1;
    }

    for (netif = netif_list; netif != NULL; netif = netif->next) {
        if (netif->name[0] == 'w' && netif->hwaddr_len == 6) {
            rt_snprintf(mac_str, mac_str_len,
                       "%02x:%02x:%02x:%02x:%02x:%02x",
                       netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2],
                       netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]);

            LOG_I("WiFi MAC address obtained: %s", mac_str);
            return 0;
        }
    }

    LOG_W("WiFi interface not found, will use default MAC address");
    return -1;
}

/* Generate UUID-format Client-ID from MAC */
static int ota_auth_generate_uuid_from_mac(const char *mac_str, char *uuid_str, size_t uuid_str_len)
{
    unsigned char mac_bytes[6];
    unsigned int hash1, hash2;

    if (!mac_str || !uuid_str || uuid_str_len < 37) {
        return -1;
    }

    if (sscanf(mac_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
               &mac_bytes[0], &mac_bytes[1], &mac_bytes[2],
               &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]) != 6) {
        LOG_E("Failed to parse MAC address: %s", mac_str);
        return -1;
    }

    hash1 = (mac_bytes[0] << 24) | (mac_bytes[1] << 16) | (mac_bytes[2] << 8) | mac_bytes[3];
    hash2 = (mac_bytes[4] << 8) | mac_bytes[5];

    rt_snprintf(uuid_str, uuid_str_len,
               "%02x%02x%02x%02x-%02x%02x-4%02x%01x-%02x%02x-%02x%02x%02x%02x%02x%02x",
               mac_bytes[0], mac_bytes[1], mac_bytes[2], mac_bytes[3],
               mac_bytes[4], mac_bytes[5],
               (hash1 >> 8) & 0xFF, (hash1 >> 4) & 0x0F,
               0x80 | ((hash2 >> 8) & 0x3F), hash2 & 0xFF,
               mac_bytes[0], mac_bytes[1], mac_bytes[2], mac_bytes[3],
               mac_bytes[4], mac_bytes[5]);

    LOG_I("Generated Client-ID from MAC: %s", uuid_str);
    return 0;
}

/* Get device MAC address */
const char *ota_auth_get_device_mac(void)
{
    if (ota_auth_priv.device_mac_address[0] == '\0') {
        if (ota_auth_get_wifi_mac_address(ota_auth_priv.device_mac_address,
                                          sizeof(ota_auth_priv.device_mac_address)) != 0) {
            rt_strncpy(ota_auth_priv.device_mac_address, DEVICE_MAC_ADDRESS_DEFAULT,
                      sizeof(ota_auth_priv.device_mac_address) - 1);
            LOG_I("Using default MAC address: %s", ota_auth_priv.device_mac_address);
        }
    }
    return ota_auth_priv.device_mac_address;
}

/* Get device Client-ID */
const char *ota_auth_get_device_client_id(void)
{
    if (ota_auth_priv.device_client_id[0] == '\0') {
        const char *mac = ota_auth_get_device_mac();
        if (ota_auth_generate_uuid_from_mac(mac, ota_auth_priv.device_client_id,
                                            sizeof(ota_auth_priv.device_client_id)) != 0) {
            rt_strncpy(ota_auth_priv.device_client_id, DEVICE_CLIENT_ID_DEFAULT,
                      sizeof(ota_auth_priv.device_client_id) - 1);
            LOG_I("Using default Client-ID: %s", ota_auth_priv.device_client_id);
        }
    }
    return ota_auth_priv.device_client_id;
}

/* Parse OTA response JSON */
static int ota_auth_parse_response(const char *json_str, ota_auth_info_t *ota_info)
{
    cJSON *root = NULL;
    cJSON *data = NULL;
    cJSON *mqtt = NULL;
    cJSON *websocket = NULL;
    cJSON *firmware = NULL;
    int ret = -1;

    if (!json_str || !ota_info) {
        return -1;
    }

    root = cJSON_Parse(json_str);
    if (!root) {
        LOG_E("Failed to parse JSON response");
        return -1;
    }

    /* Check response format: may contain 'data' wrapper or direct root fields */
    data = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (cJSON_IsObject(data)) {
        /* New format: data under 'data' field */
        LOG_D("Response uses 'data' wrapper format");
    } else {
        /* Legacy format: data directly in root object */
        LOG_D("Response uses direct format (no 'data' wrapper)");
        data = root;  /* Use root object as data source */
    }

    /* Parse MQTT config */
    mqtt = cJSON_GetObjectItemCaseSensitive(data, "mqtt");
    if (!cJSON_IsObject(mqtt)) {
        LOG_W("Response does not contain 'mqtt' object");
        /* Continue parsing other fields */
    }

    if (cJSON_IsObject(mqtt)) {
        cJSON *item = NULL;

        item = cJSON_GetObjectItemCaseSensitive(mqtt, "endpoint");
        if (cJSON_IsString(item)) {
            rt_strncpy(ota_info->mqtt_endpoint, item->valuestring,
                      sizeof(ota_info->mqtt_endpoint) - 1);
        }

        item = cJSON_GetObjectItemCaseSensitive(mqtt, "client_id");
        if (cJSON_IsString(item)) {
            rt_strncpy(ota_info->mqtt_client_id, item->valuestring,
                      sizeof(ota_info->mqtt_client_id) - 1);
        }

        item = cJSON_GetObjectItemCaseSensitive(mqtt, "username");
        if (cJSON_IsString(item)) {
            rt_strncpy(ota_info->mqtt_username, item->valuestring,
                      sizeof(ota_info->mqtt_username) - 1);
        }

        item = cJSON_GetObjectItemCaseSensitive(mqtt, "password");
        if (cJSON_IsString(item)) {
            rt_strncpy(ota_info->mqtt_password, item->valuestring,
                      sizeof(ota_info->mqtt_password) - 1);
        }

        item = cJSON_GetObjectItemCaseSensitive(mqtt, "publish_topic");
        if (cJSON_IsString(item)) {
            rt_strncpy(ota_info->mqtt_publish_topic, item->valuestring,
                      sizeof(ota_info->mqtt_publish_topic) - 1);
        }

        item = cJSON_GetObjectItemCaseSensitive(mqtt, "subscribe_topic");
        if (cJSON_IsString(item)) {
            rt_strncpy(ota_info->mqtt_subscribe_topic, item->valuestring,
                      sizeof(ota_info->mqtt_subscribe_topic) - 1);
        }
    }

    /* Parse WebSocket config */
    websocket = cJSON_GetObjectItemCaseSensitive(data, "websocket");
    if (cJSON_IsObject(websocket)) {
        cJSON *item = NULL;

        item = cJSON_GetObjectItemCaseSensitive(websocket, "url");
        if (cJSON_IsString(item)) {
            rt_strncpy(ota_info->ws_url, item->valuestring,
                      sizeof(ota_info->ws_url) - 1);
        }

        item = cJSON_GetObjectItemCaseSensitive(websocket, "token");
        if (cJSON_IsString(item)) {
            rt_strncpy(ota_info->ws_token, item->valuestring,
                      sizeof(ota_info->ws_token) - 1);
        }
    } else {
        /* Backward compatibility: websocket config may be flattened in data */
        cJSON *ws_url_item = cJSON_GetObjectItemCaseSensitive(data, "websocket_url");
        cJSON *ws_token_item = cJSON_GetObjectItemCaseSensitive(data, "websocket_token");
        if (cJSON_IsString(ws_url_item)) {
            rt_strncpy(ota_info->ws_url, ws_url_item->valuestring,
                      sizeof(ota_info->ws_url) - 1);
        }
        if (cJSON_IsString(ws_token_item)) {
            rt_strncpy(ota_info->ws_token, ws_token_item->valuestring,
                      sizeof(ota_info->ws_token) - 1);
        }
    }

    /* Parse firmware info */
    firmware = cJSON_GetObjectItemCaseSensitive(data, "firmware");
    if (cJSON_IsObject(firmware)) {
        cJSON *item = cJSON_GetObjectItemCaseSensitive(firmware, "version");
        if (cJSON_IsString(item)) {
            rt_strncpy(ota_info->firmware_version, item->valuestring,
                      sizeof(ota_info->firmware_version) - 1);
        }
    }

    ota_info->valid = RT_TRUE;
    ret = 0;

    if (root) {
        cJSON_Delete(root);
    }

    return ret;
}

/* Execute OTA authentication */
static int ota_auth_authenticate_internal(void)
{
    struct webclient_session *session = RT_NULL;
    unsigned char *response_buf = RT_NULL;
    char *request_data = RT_NULL;
    cJSON *root = NULL;
    cJSON *application = NULL;
    cJSON *board = NULL;
    int ret = -1;
    int resp_status = 0;
    size_t bytes_read = 0;
    size_t total_read = 0;

    const char *device_mac = ota_auth_get_device_mac();
    const char *device_client_id = ota_auth_get_device_client_id();

    LOG_I("=== Starting OTA Authentication ===");
    LOG_I("MAC Address: %s", device_mac);
    LOG_I("Client ID: %s", device_client_id);
    LOG_I("OTA URL: %s", XIAOZHI_OTA_URL);

    response_buf = (unsigned char *)web_malloc(XIAOZHI_OTA_RESPONSE_BUFSZ);
    if (!response_buf) {
        LOG_E("Failed to allocate response buffer");
        goto __exit;
    }
    rt_memset(response_buf, 0, XIAOZHI_OTA_RESPONSE_BUFSZ);

    root = cJSON_CreateObject();
    if (!root) {
        LOG_E("Failed to create JSON object");
        goto __exit;
    }

    application = cJSON_CreateObject();
    cJSON_AddStringToObject(application, "version", DEVICE_VERSION);
    cJSON_AddItemToObject(root, "application", application);

    board = cJSON_CreateObject();
    cJSON_AddStringToObject(board, "name", DEVICE_BOARD_NAME);
    cJSON_AddItemToObject(root, "board", board);

    request_data = cJSON_PrintUnformatted(root);
    if (!request_data) {
        LOG_E("Failed to create JSON string");
        goto __exit;
    }

    LOG_I("Request Body: %s", request_data);

    session = webclient_session_create(XIAOZHI_OTA_HEADER_BUFSZ);
    if (!session) {
        LOG_E("Failed to create webclient session");
        goto __exit;
    }

    /* Add request headers (using dynamically obtained device info) */
    webclient_header_fields_add(session, "Device-Id: %s\r\n", device_mac);
    webclient_header_fields_add(session, "Client-Id: %s\r\n", device_client_id);
    webclient_header_fields_add(session, "Content-Type: application/json\r\n");
    webclient_header_fields_add(session, "Content-Length: %d\r\n", rt_strlen(request_data));

    LOG_I("Sending POST request...");
    resp_status = webclient_post(session, XIAOZHI_OTA_URL, request_data, rt_strlen(request_data));
    if (resp_status != 200) {
        LOG_E("POST request failed, response status: %d", resp_status);
        goto __exit;
    }

    LOG_I("POST request successful, reading response...");

    /* Read response payload */
    while (total_read < XIAOZHI_OTA_RESPONSE_BUFSZ - 1) {
        bytes_read = webclient_read(session,
                                   response_buf + total_read,
                                   XIAOZHI_OTA_RESPONSE_BUFSZ - total_read - 1);
        if (bytes_read <= 0) {
            break;
        }
        total_read += bytes_read;
    }

    if (total_read == 0) {
        LOG_E("No response data received");
        goto __exit;
    }

    response_buf[total_read] = '\0';
    LOG_I("Response received (%d bytes): %s", total_read, response_buf);

    if (ota_auth_parse_response((const char *)response_buf, &ota_auth_priv.ota_info) == 0) {
        LOG_I("=== OTA Authentication Success ===");
        LOG_I("MQTT Endpoint: %s", ota_auth_priv.ota_info.mqtt_endpoint);
        LOG_I("MQTT Client ID: %s", ota_auth_priv.ota_info.mqtt_client_id);
        LOG_I("MQTT Username: %s", ota_auth_priv.ota_info.mqtt_username);
        LOG_I("WebSocket URL: %s", ota_auth_priv.ota_info.ws_url);
        LOG_I("WebSocket Token: %s", ota_auth_priv.ota_info.ws_token);
        LOG_I("Firmware Version: %s", ota_auth_priv.ota_info.firmware_version);

        ret = 0;
    } else {
        LOG_E("Failed to parse OTA response");
    }

__exit:
    if (session) {
        webclient_close(session);
    }

    if (response_buf) {
        web_free(response_buf);
    }

    if (request_data) {
        cJSON_free(request_data);
    }

    if (root) {
        cJSON_Delete(root);
    }

    return ret;
}

/* Send authentication success message */
static void ota_auth_send_success_msg(void)
{
    app_message_t msg;
    rt_memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_OTA_AUTH_SUCCESS;
    msg.header.src_module = MODULE_ID_OTA_AUTH;
    msg.header.dst_module = 0;  /* Broadcast */
    msg.header.data_len = 0;
    module_send_msg(&msg);
}

/* Send authentication failure message */
static void ota_auth_send_failed_msg(void)
{
    app_message_t msg;
    rt_memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_OTA_AUTH_FAILED;
    msg.header.src_module = MODULE_ID_OTA_AUTH;
    msg.header.dst_module = 0;  /* Broadcast */
    msg.header.data_len = 0;
    module_send_msg(&msg);
}

/* OTA auth module message handler */
static int ota_auth_msg_handler(const app_message_t *msg)
{
    if (msg == RT_NULL) {
        return -RT_EINVAL;
    }

    switch (msg->header.type) {
        case MSG_TYPE_SYS_INIT:
            LOG_I("OTA Auth module received init message");
            break;

        case MSG_TYPE_OTA_AUTH_REQUEST:
            LOG_I("OTA Auth module received authentication request");
            /* If already authenticated, send success and skip duplicate auth */
            if (ota_auth_priv.authenticated) {
                LOG_I("Already authenticated, skip duplicate request");
                ota_auth_send_success_msg();
                break;
            }
            /* If authentication is in progress, ignore duplicate request */
            if (ota_auth_priv.authenticating) {
                LOG_W("Authentication already in progress, ignore duplicate request");
                break;
            }
            /* Start authentication */
            ota_auth_priv.authenticating = RT_TRUE;
            if (ota_auth_authenticate_internal() == 0) {
                ota_auth_priv.authenticated = RT_TRUE;
                ota_auth_priv.authenticating = RT_FALSE;
                ota_auth_send_success_msg();
            } else {
                ota_auth_priv.authenticating = RT_FALSE;
                ota_auth_send_failed_msg();
            }
            break;

        default:
            LOG_D("OTA Auth module received unhandled msg type: 0x%02X", msg->header.type);
            break;
    }

    return RT_EOK;
}

/* OTA auth module init */
static int ota_auth_init(void)
{
    rt_memset(&ota_auth_priv, 0, sizeof(ota_auth_priv));
    ota_auth_priv.initialized = RT_FALSE;

    LOG_I("OTA Auth module initialized");

    /* Subscribe messages */
    msg_bus_subscribe(MODULE_ID_OTA_AUTH, MSG_TYPE_OTA_AUTH_REQUEST);
    msg_bus_subscribe(MODULE_ID_OTA_AUTH, MSG_TYPE_SYS_INIT);

    return RT_EOK;
}

/* OTA auth module deinit */
static int ota_auth_deinit(void)
{
    msg_bus_unsubscribe(MODULE_ID_OTA_AUTH, MSG_TYPE_OTA_AUTH_REQUEST);
    msg_bus_unsubscribe(MODULE_ID_OTA_AUTH, MSG_TYPE_SYS_UNKNOWN);

    LOG_I("OTA Auth module deinitialized");
    return RT_EOK;
}

/* OTA auth module thread entry */
static void ota_auth_thread_entry(void *parameter)
{
    app_message_t msg;

    LOG_I("OTA Auth module thread started");

    while (1) {
        if (msg_bus_receive(MODULE_ID_OTA_AUTH, &msg, RT_WAITING_FOREVER) == RT_EOK) {
            ota_auth_msg_handler(&msg);
        }
    }
}

/* OTA auth module operations */
static module_ops_t ota_auth_ops = {
    .init = ota_auth_init,
    .deinit = ota_auth_deinit,
    .msg_handler = ota_auth_msg_handler,
    .name = "OTA Auth Module",
    .id = MODULE_ID_OTA_AUTH,
};

/* OTA auth module instance */
static module_t ota_auth_module = {
    .ops = &ota_auth_ops,
    .active = RT_FALSE,
    .thread = RT_NULL,
    .priv_data = &ota_auth_priv,
};

/* Get OTA auth module instance */
module_t *ota_auth_module_get(void)
{
    return &ota_auth_module;
}

/* Initialize OTA auth module thread */
int ota_auth_module_thread_start(void)
{
    ota_auth_module.thread = rt_thread_create("ota_auth_module",
                                               ota_auth_thread_entry,
                                               RT_NULL,
                                               4096,
                                               19,
                                               10);
    if (ota_auth_module.thread != RT_NULL) {
        rt_thread_startup(ota_auth_module.thread);
        return RT_EOK;
    }
    return -RT_ERROR;
}

/* External API implementations */
ota_auth_info_t *ota_auth_get_info(void)
{
    if (ota_auth_priv.ota_info.valid) {
        return &ota_auth_priv.ota_info;
    }
    return RT_NULL;
}

rt_bool_t ota_auth_is_authenticated(void)
{
    return ota_auth_priv.ota_info.valid;
}

