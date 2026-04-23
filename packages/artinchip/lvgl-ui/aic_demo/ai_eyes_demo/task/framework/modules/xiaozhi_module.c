/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#include "xiaozhi_module.h"
#include "../core/module.h"
#include "../core/msg_def.h"
#include "../core/msg_bus.h"
#include "../core/msg_utils.h"
#include "ota_auth_module.h"
#include <rtthread.h>
#include <librws.h>
#include <string.h>
#include <stdlib.h>
#include <cJSON.h>
#include <rtdbg.h>

#define DBG_SECTION_NAME "XIAOZHI_MOD"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

/* XiaoZhi WebSocket connection parameters */
#define XIAOZHI_WS_URL      "api.tenclass.net"
#define XIAOZHI_WS_PORT     443
#define XIAOZHI_WS_PATH     "/xiaozhi/v1/"

#define AUTHORIZATION_HEADER    "Authorization"
#define AUTHORIZATION_VALUE     "Bearer test-token"
#define PROTOCOL_VERSION_HEADER "Protocol-Version"
#define PROTOCOL_VERSION_VALUE  "2"
#define DEVICE_ID_HEADER        "Device-Id"
#define CLIENT_ID_HEADER        "Client-Id"

/* MCP protocol version */
#define MCP_PROTOCOL_VERSION "2024-11-05"
#define SERVER_VERSION "1.7.2"

/* OPUS parameters (for message sending) */
#define OPUS_SAMPLE_RATE    24000
#define OPUS_CHANNELS       1

/* Auto-connect thread parameters */
#define AUTO_CONNECT_THREAD_STACK_SIZE  4096
#define AUTO_CONNECT_THREAD_PRIORITY    15
#define RECONNECT_INTERVAL              5000

/* XiaoZhi application struct */
struct xiaozhi_app {
    rt_bool_t init;
    rws_socket socket;
    char session_id[32];
    rt_bool_t connected;
    rt_bool_t connecting;  /* Connection-in-progress state */
    rt_bool_t running;
    rt_bool_t tts_end_received;
    struct rt_thread *auto_connect_thread;
};

typedef struct xiaozhi_app *xiaozhi_app_t;

/* XiaoZhi module private data */
typedef struct {
    xiaozhi_app_t app;
    rt_bool_t initialized;
    rt_bool_t ota_auth_requested;  /* Whether OTA auth request has been sent */
    rt_bool_t ota_auth_completed;  /* Whether OTA auth is completed */
} xiaozhi_module_priv_t;

static xiaozhi_module_priv_t xiaozhi_priv = {0};

/* xiaozhi_is_ok variable definition (used by Audio module readiness check) */
uint8_t xiaozhi_is_ok = 0;

/* ========== MCP protocol handling functions ========== */

/* Tool callback function type */
typedef int (*mcp_tool_handler_t)(cJSON *arguments, const char *session_id, void *socket, char *result_buffer, int result_buffer_size);

/* MCP tool definition */
typedef struct {
    const char *name;
    const char *description;
    const char *input_schema_json;
    mcp_tool_handler_t handler;
} mcp_tool_t;

/* Volume-set tool handler */
static int xiaozhi_mcp_tool_set_volume(cJSON *arguments, const char *session_id, void *socket, char *result_buffer, int result_buffer_size)
{
    if (arguments == RT_NULL || result_buffer == RT_NULL || result_buffer_size <= 0) {
        return -RT_ERROR;
    }

    cJSON *volume = cJSON_GetObjectItemCaseSensitive(arguments, "volume");
    if (!cJSON_IsNumber(volume)) {
        return -RT_ERROR;
    }

    int volume_value = volume->valueint;
    if (volume_value < 0 || volume_value > 100) {
        return -RT_ERROR;
    }

    LOG_I("MCP: Sending message to set audio volume to %d", volume_value);

    /* Send volume-set message to Audio module via message bus */
    app_message_t msg;
    rt_memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_AUDIO_SET_VOLUME;
    msg.header.src_module = MODULE_ID_XIAOZHI;
    msg.header.dst_module = MODULE_ID_AUDIO;
    msg.header.data_len = sizeof(msg_audio_volume_data_t);
    msg.data.audio_volume.volume = volume_value;

    if (module_send_msg(&msg) == RT_EOK) {
        rt_snprintf(result_buffer, result_buffer_size,
            "{\"content\":[{\"type\":\"text\",\"text\":\"Volume set to %d%%\"}]}", volume_value);
        return RT_EOK;
    }

    LOG_E("MCP: Failed to send volume set message");
    return -RT_ERROR;
}

/* MCP tool list */
static const mcp_tool_t xiaozhi_mcp_tools[] = {
    {
        .name = "self.audio_speaker.set_volume",
        .description = "Set the volume of the audio speaker. "
                      "The volume value should be between 0 and 100. "
                      "If the current volume is unknown, you must call `self.get_device_status` tool first and then call this tool.",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{"
                            "\"volume\":{\"type\":\"integer\",\"description\":\"Volume level (0-100)\",\"minimum\":0,\"maximum\":100}},"
                            "\"required\":[\"volume\"]}",
        .handler = xiaozhi_mcp_tool_set_volume
    }
};

#define XIAOZHI_MCP_TOOLS_COUNT (sizeof(xiaozhi_mcp_tools) / sizeof(xiaozhi_mcp_tools[0]))

/* Send MCP response message */
static int xiaozhi_mcp_send_response(const char *session_id, int id, const char *result, rt_bool_t is_error, void *socket)
{
    char response_msg[1024];
    rws_socket rws_sock = (rws_socket)socket;

    if (is_error) {
        rt_snprintf(response_msg, sizeof(response_msg),
            "{\"session_id\":\"%s\",\"type\":\"mcp\",\"payload\":{\"jsonrpc\":\"2.0\",\"id\":%d,\"error\":%s}}",
            session_id, id, result);
    } else {
        rt_snprintf(response_msg, sizeof(response_msg),
            "{\"session_id\":\"%s\",\"type\":\"mcp\",\"payload\":{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":%s}}",
            session_id, id, result);
    }

    if (rws_sock != RT_NULL) {
        rws_socket_send_text(rws_sock, response_msg);
        return RT_EOK;
    }

    return -RT_ERROR;
}

/* Handle initialize method */
static int xiaozhi_mcp_handle_initialize(cJSON *payload, const char *session_id, void *socket)
{
    cJSON *id = cJSON_GetObjectItemCaseSensitive(payload, "id");
    if (!cJSON_IsNumber(id)) {
        return -RT_ERROR;
    }

    char result_json[512];
    rt_snprintf(result_json, sizeof(result_json),
        "{\"protocolVersion\":\"%s\",\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":\"\",\"version\":\"%s\"}}",
        MCP_PROTOCOL_VERSION, SERVER_VERSION);

    xiaozhi_mcp_send_response(session_id, id->valueint, result_json, RT_FALSE, socket);
    LOG_I("MCP: Sent initialize response");

    return RT_EOK;
}

/* Handle tools/list method */
static int xiaozhi_mcp_handle_tools_list(cJSON *payload, const char *session_id, void *socket)
{
    cJSON *id = cJSON_GetObjectItemCaseSensitive(payload, "id");
    if (!cJSON_IsNumber(id)) {
        return -RT_ERROR;
    }

    /* Build tool-list JSON */
    cJSON *tools_array = cJSON_CreateArray();
    if (tools_array == RT_NULL) {
        return -RT_ENOMEM;
    }

    for (int i = 0; i < XIAOZHI_MCP_TOOLS_COUNT; i++) {
        cJSON *tool = cJSON_CreateObject();
        if (tool == RT_NULL) {
            cJSON_Delete(tools_array);
            return -RT_ENOMEM;
        }

        cJSON_AddStringToObject(tool, "name", xiaozhi_mcp_tools[i].name);
        cJSON_AddStringToObject(tool, "description", xiaozhi_mcp_tools[i].description);

        /* Parse input_schema JSON string */
        cJSON *input_schema = cJSON_Parse(xiaozhi_mcp_tools[i].input_schema_json);
        if (input_schema != RT_NULL) {
            cJSON_AddItemToObject(tool, "inputSchema", input_schema);
        }

        cJSON_AddItemToArray(tools_array, tool);
    }

    char *tools_json_str = cJSON_Print(tools_array);
    cJSON_Delete(tools_array);

    if (tools_json_str == RT_NULL) {
        return -RT_ENOMEM;
    }

    char result_json[1536];
    rt_snprintf(result_json, sizeof(result_json), "{\"tools\":%s}", tools_json_str);

    xiaozhi_mcp_send_response(session_id, id->valueint, result_json, RT_FALSE, socket);
    LOG_I("MCP: Sent tools/list response");

    free(tools_json_str);
    return RT_EOK;
}

/* Handle tools/call method */
static int xiaozhi_mcp_handle_tools_call(cJSON *payload, const char *session_id, void *socket)
{
    cJSON *id = cJSON_GetObjectItemCaseSensitive(payload, "id");
    if (!cJSON_IsNumber(id)) {
        return -RT_ERROR;
    }

    cJSON *params = cJSON_GetObjectItemCaseSensitive(payload, "params");
    if (!cJSON_IsObject(params)) {
        char error_json[256];
        rt_snprintf(error_json, sizeof(error_json),
            "{\"code\":-32602,\"message\":\"Missing params\"}");
        xiaozhi_mcp_send_response(session_id, id->valueint, error_json, RT_TRUE, socket);
        return -RT_ERROR;
    }

    cJSON *tool_name = cJSON_GetObjectItemCaseSensitive(params, "name");
    if (!cJSON_IsString(tool_name)) {
        char error_json[256];
        rt_snprintf(error_json, sizeof(error_json),
            "{\"code\":-32602,\"message\":\"Missing tool name\"}");
        xiaozhi_mcp_send_response(session_id, id->valueint, error_json, RT_TRUE, socket);
        return -RT_ERROR;
    }

    cJSON *tool_arguments = cJSON_GetObjectItemCaseSensitive(params, "arguments");
    if (tool_arguments != RT_NULL && !cJSON_IsObject(tool_arguments)) {
        char error_json[256];
        rt_snprintf(error_json, sizeof(error_json),
            "{\"code\":-32602,\"message\":\"Invalid arguments\"}");
        xiaozhi_mcp_send_response(session_id, id->valueint, error_json, RT_TRUE, socket);
        return -RT_ERROR;
    }

    /* Find tool */
    const char *tool_name_str = tool_name->valuestring;
    const mcp_tool_t *tool = RT_NULL;

    for (int i = 0; i < XIAOZHI_MCP_TOOLS_COUNT; i++) {
        if (strcmp(xiaozhi_mcp_tools[i].name, tool_name_str) == 0) {
            tool = &xiaozhi_mcp_tools[i];
            break;
        }
    }

    if (tool == RT_NULL) {
        char error_json[256];
        rt_snprintf(error_json, sizeof(error_json),
            "{\"code\":-32601,\"message\":\"Unknown tool: %s\"}", tool_name_str);
        xiaozhi_mcp_send_response(session_id, id->valueint, error_json, RT_TRUE, socket);
        LOG_W("MCP: Unknown tool: %s", tool_name_str);
        return -RT_ERROR;
    }

    /* Invoke tool handler */
    LOG_I("MCP: Calling tool: %s", tool_name_str);

    if (tool_arguments == RT_NULL) {
        tool_arguments = cJSON_CreateObject();
    }

    char result_buffer[512];
    if (tool->handler(tool_arguments, session_id, socket, result_buffer, sizeof(result_buffer)) == RT_EOK) {
        /* Tool execution succeeded, send success response */
        xiaozhi_mcp_send_response(session_id, id->valueint, result_buffer, RT_FALSE, socket);
        LOG_I("MCP: Tool %s executed successfully", tool_name_str);
    } else {
        /* Tool execution failed */
        char error_json[256];
        rt_snprintf(error_json, sizeof(error_json),
            "{\"code\":-1,\"message\":\"Tool execution failed\"}");
        xiaozhi_mcp_send_response(session_id, id->valueint, error_json, RT_TRUE, socket);
        LOG_E("MCP: Tool %s execution failed", tool_name_str);
    }

    return RT_EOK;
}

/* Handle MCP message (JSON object format) */
static int xiaozhi_mcp_process_json(cJSON *json, const char *session_id, void *socket)
{
    if (json == RT_NULL || session_id == RT_NULL || socket == RT_NULL) {
        return -RT_EINVAL;
    }

    /* Check JSONRPC version */
    cJSON *version = cJSON_GetObjectItemCaseSensitive(json, "jsonrpc");
    if (version == RT_NULL || !cJSON_IsString(version) || strcmp(version->valuestring, "2.0") != 0) {
        LOG_E("MCP: Invalid JSONRPC version");
        return -RT_ERROR;
    }

    /* Check method */
    cJSON *method = cJSON_GetObjectItemCaseSensitive(json, "method");
    if (method == RT_NULL || !cJSON_IsString(method)) {
        LOG_E("MCP: Missing method");
        return -RT_ERROR;
    }

    const char *method_str = method->valuestring;

    /* Ignore notification messages */
    if (strstr(method_str, "notifications") != RT_NULL) {
        return RT_EOK;
    }

    /* Get payload */
    cJSON *payload = json;

    /* Handle different methods */
    if (strcmp(method_str, "initialize") == 0) {
        return xiaozhi_mcp_handle_initialize(payload, session_id, socket);
    } else if (strcmp(method_str, "tools/list") == 0) {
        return xiaozhi_mcp_handle_tools_list(payload, session_id, socket);
    } else if (strcmp(method_str, "tools/call") == 0) {
        return xiaozhi_mcp_handle_tools_call(payload, session_id, socket);
    } else {
        LOG_E("MCP: Method not implemented: %s", method_str);
        cJSON *id = cJSON_GetObjectItemCaseSensitive(payload, "id");
        if (cJSON_IsNumber(id)) {
            char error_json[256];
            rt_snprintf(error_json, sizeof(error_json),
                "{\"code\":-32601,\"message\":\"Method not implemented: %s\"}", method_str);
            xiaozhi_mcp_send_response(session_id, id->valueint, error_json, RT_TRUE, socket);
        }
        return -RT_ERROR;
    }
}


/* Send emotion message to UI module */
static void xiaozhi_send_emotion_msg(emotion_status_t emotion)
{
    app_message_t msg;
    rt_memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_XIAOZHI_EMOTION;
    msg.header.src_module = MODULE_ID_XIAOZHI;
    msg.header.dst_module = MODULE_ID_UI;
    msg.header.data_len = sizeof(msg_emotion_data_t);
    msg.data.emotion.emotion = emotion;
    rt_strncpy(msg.data.emotion.desc, "xiaozhi_emotion", sizeof(msg.data.emotion.desc) - 1);
    module_send_msg(&msg);
}

/* WebSocket on-open callback */
static void on_open(rws_socket socket)
{
    LOG_D("WebSocket connected to XiaoZhi server.");

    if (xiaozhi_priv.app) {
        xiaozhi_priv.app->connected = RT_TRUE;
        xiaozhi_priv.app->connecting = RT_FALSE;  /* Connected successfully, clear connecting state */
        xiaozhi_priv.app->tts_end_received = RT_FALSE;
    }

    /* Send hello message */
    const char *hello_msg = "{\"type\":\"hello\",\"version\":2,\"features\":{\"aec\":true,\"mcp\":true},\"transport\":\"websocket\",\"audio_params\":{\"format\":\"opus\",\"sample_rate\":16000,\"channels\":1,\"frame_duration\":60}}";
    rws_socket_send_text(socket, hello_msg);
    LOG_D("Sent hello message: %s", hello_msg);

    /* Send connection-success message */
    app_message_t msg;
    rt_memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_XIAOZHI_CONNECTED;
    msg.header.src_module = MODULE_ID_XIAOZHI;
    msg.header.dst_module = 0;  /* Broadcast */
    msg.header.data_len = 0;
    module_send_msg(&msg);
}

/* WebSocket on-close callback */
static void on_close(rws_socket socket)
{
    rws_error error = rws_socket_get_error(socket);

    if (error) {
        LOG_E("WebSocket disconnected, error: %i, %s",
              rws_error_get_code(error), rws_error_get_description(error));
    } else {
        LOG_D("WebSocket disconnected!");
    }

    if (xiaozhi_priv.app) {
        xiaozhi_priv.app->connected = RT_FALSE;
        xiaozhi_priv.app->connecting = RT_FALSE;  /* Connection closed, clear connecting state */
    }

    /* Send disconnected message */
    app_message_t msg;
    rt_memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_XIAOZHI_DISCONNECTED;
    msg.header.src_module = MODULE_ID_XIAOZHI;
    msg.header.dst_module = 0;  /* Broadcast */
    msg.header.data_len = 0;
    module_send_msg(&msg);
}

/* Handle text message */
static void on_message_text(rws_socket socket, const char *text, const unsigned int len)
{
    cJSON *root = NULL;
    cJSON *type = NULL;
    char *buff = RT_NULL;

    buff = (char *)rt_malloc(len + 1);
    if (buff == RT_NULL) {
        return;
    }

    rt_memset(buff, 0x00, len + 1);
    rt_memcpy(buff, text, len);

    LOG_I("Received message (len=%d): %s", len, buff);

    root = cJSON_Parse(buff);
    if (root == NULL) {
        LOG_E("Failed to parse JSON message: %s", buff);
        rt_free(buff);
        return;
    }

    type = cJSON_GetObjectItemCaseSensitive(root, "type");
    if (cJSON_IsString(type) && (type->valuestring != NULL)) {
        if (strcmp(type->valuestring, "hello") == 0) {
            LOG_I("Received hello response from server");

            cJSON *session_id = cJSON_GetObjectItemCaseSensitive(root, "session_id");
            if (cJSON_IsString(session_id) && (session_id->valuestring != NULL) && xiaozhi_priv.app) {
                rt_strncpy(xiaozhi_priv.app->session_id, session_id->valuestring,
                          sizeof(xiaozhi_priv.app->session_id) - 1);
                xiaozhi_priv.app->session_id[sizeof(xiaozhi_priv.app->session_id) - 1] = '\0';
                LOG_I("Session ID: %s", xiaozhi_priv.app->session_id);

                /* Send start-listen command */
                char start_listen_msg[128];
                rt_snprintf(start_listen_msg, sizeof(start_listen_msg),
                    "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"start\",\"mode\":\"realtime\"}",
                    xiaozhi_priv.app->session_id);
                rws_socket_send_text(socket, start_listen_msg);
                LOG_I("Sent start listen command: %s", start_listen_msg);
            }
        } else if (strcmp(type->valuestring, "mcp") == 0) {
            /* Handle MCP message */
            cJSON *payload = cJSON_GetObjectItemCaseSensitive(root, "payload");
            if (cJSON_IsObject(payload) && xiaozhi_priv.app) {
                /* Check if tools/list completed to mark system ready */
                cJSON *method = cJSON_GetObjectItemCaseSensitive(payload, "method");
                if (cJSON_IsString(method) && strcmp(method->valuestring, "tools/list") == 0) {
                    /* Process MCP message first */
                    xiaozhi_mcp_process_json(payload, xiaozhi_priv.app->session_id, socket);

                    /* Tool list loaded, mark as ready */
                    xiaozhi_is_ok = 1;
                    LOG_I("XiaoZhi tools/list completed, system ready");
                    /* Send probe message (listen/detect, text=hello) */
                    if (xiaozhi_priv.app->session_id[0] != '\0' && xiaozhi_priv.app->socket != RT_NULL) {
                        char hello_msg[128];
                        rt_snprintf(hello_msg, sizeof(hello_msg),
                            "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"detect\",\"text\":\"hello\"}",
                            xiaozhi_priv.app->session_id);
                        rws_socket_send_text(xiaozhi_priv.app->socket, hello_msg);
                        LOG_I("Sent detect listen message: %s", hello_msg);
                    }
                } else {
                    /* Other MCP messages (initialize, tools/call, etc.) */
                    xiaozhi_mcp_process_json(payload, xiaozhi_priv.app->session_id, socket);
                }
            }
        } else if (strcmp(type->valuestring, "llm") == 0) {
            /* Handle LLM message: extract emotion and forward to UI */
            cJSON *emotion = cJSON_GetObjectItemCaseSensitive(root, "emotion");
            if (cJSON_IsString(emotion) && (emotion->valuestring != NULL)) {
                emotion_status_t emotion_enum = msg_emotion_str_to_enum(emotion->valuestring);
                xiaozhi_send_emotion_msg(emotion_enum);
            }
        }
        /* Handling for other message types... */
    }

    cJSON_Delete(root);
    rt_free(buff);
}

/* Handle binary audio message */
static void on_message_bin(rws_socket socket, const void *data, const unsigned int len)
{
    /* Audio packet header size is 16 bytes */
    const unsigned int AUDIO_PACKET_HEADER_SIZE = 16;

    if (xiaozhi_priv.app && xiaozhi_priv.app->connected == RT_TRUE && data && len > 0) {

        if (len < AUDIO_PACKET_HEADER_SIZE) {
            LOG_E("Received data too short to contain header: %d bytes", len);
            return;
        }

        /* Send raw OPUS data (with header) directly to Audio module for decoding */
        unsigned char *msg_data = (unsigned char *)rt_malloc(len);
        if (msg_data == RT_NULL) {
            LOG_E("Failed to allocate memory for OPUS packet");
            return;
        }

        rt_memcpy(msg_data, data, len);

        /* Send OPUS packet to Audio module */
        app_message_t msg;
        rt_memset(&msg, 0, sizeof(msg));
        msg.header.type = MSG_TYPE_AUDIO_DECODE_OPUS;
        msg.header.src_module = MODULE_ID_XIAOZHI;
        msg.header.dst_module = MODULE_ID_AUDIO;
        msg.header.data_len = sizeof(msg_audio_data_t);
        msg.data.audio.data = msg_data;  /* Note: must be freed by Audio module */
        msg.data.audio.data_len = len;
        msg.data.audio.sample_rate = OPUS_SAMPLE_RATE;
        msg.data.audio.channels = OPUS_CHANNELS;
        msg.data.audio.bits = 16;  /* Internal OPUS bit depth */
        module_send_msg(&msg);
    }
}

/* Send OPUS audio data to server */
int xiaozhi_send_binary_opus(const unsigned char *opus_data, unsigned int data_size)
{
    unsigned char *packet;
    unsigned int packet_size;
    unsigned short version = 2;
    unsigned short type = 0;
    unsigned int reserved = 0;
    unsigned int timestamp = 0;
    unsigned int payload_size;

    if (opus_data == RT_NULL || data_size == 0) {
        return -1;
    }

    if (!xiaozhi_priv.app || xiaozhi_priv.app->connected == RT_FALSE) {
        return -1;
    }

    payload_size = data_size;
    packet_size = 16 + data_size;

    packet = (unsigned char *)rt_malloc(packet_size);
    if (packet == RT_NULL) {
        return -1;
    }

    /* Fill packet header (big-endian) */
    packet[0] = (version >> 8) & 0xFF;
    packet[1] = version & 0xFF;
    packet[2] = (type >> 8) & 0xFF;
    packet[3] = type & 0xFF;
    packet[4] = (reserved >> 24) & 0xFF;
    packet[5] = (reserved >> 16) & 0xFF;
    packet[6] = (reserved >> 8) & 0xFF;
    packet[7] = reserved & 0xFF;
    packet[8] = (timestamp >> 24) & 0xFF;
    packet[9] = (timestamp >> 16) & 0xFF;
    packet[10] = (timestamp >> 8) & 0xFF;
    packet[11] = timestamp & 0xFF;
    packet[12] = (payload_size >> 24) & 0xFF;
    packet[13] = (payload_size >> 16) & 0xFF;
    packet[14] = (payload_size >> 8) & 0xFF;
    packet[15] = payload_size & 0xFF;

    rt_memcpy(packet + 16, opus_data, data_size);

    if (rws_socket_send_bin(xiaozhi_priv.app->socket, packet, packet_size, 2, rws_true) != rws_true) {
        rt_free(packet);
        return -1;
    }

    rt_free(packet);
    return 0;
}

/* Force-clean socket to avoid hang in rws_socket_disconnect_and_release */
static void xiaozhi_force_cleanup_socket(void)
{
    if (!xiaozhi_priv.app || !xiaozhi_priv.app->socket) {
        return;
    }

    LOG_W("Force cleaning up socket (avoiding potential deadlock)...");
    xiaozhi_priv.app->connected = RT_FALSE;
    xiaozhi_priv.app->connecting = RT_FALSE;  /* Also clear connecting state during force-clean */
    xiaozhi_priv.app->socket = RT_NULL;

    /* Give worker thread some time to exit, without waiting on mutex */
    rt_thread_mdelay(1000);

    LOG_W("Socket force cleanup completed.");
}

/* Connect to XiaoZhi server */
static int xiaozhi_connect_internal(void)
{
    rws_bool ret = rws_false;

    if (xiaozhi_priv.app != RT_NULL) {
        if (xiaozhi_priv.app->connected == RT_TRUE) {
            LOG_E("The WebSocket connection has been opened.");
            return (-RT_EBUSY);
        }
        if (xiaozhi_priv.app->connecting == RT_TRUE) {
            LOG_W("WebSocket connection is already in progress.");
            return (-RT_EBUSY);
        }
    }

    if (xiaozhi_priv.app == RT_NULL) {
        xiaozhi_priv.app = (xiaozhi_app_t)rt_malloc_align(sizeof(struct xiaozhi_app), 4);
        if (xiaozhi_priv.app == RT_NULL) {
            LOG_E("Memory malloc failed.");
            return (-RT_ENOMEM);
        }
        rt_memset(xiaozhi_priv.app, 0x00, sizeof(struct xiaozhi_app));
    }

    /* If old socket exists, use force-clean to avoid hang in rws_socket_disconnect_and_release */
    if (xiaozhi_priv.app->socket != RT_NULL) {
        LOG_W("Old socket still exists, force cleaning up before reconnect");
        xiaozhi_force_cleanup_socket();
    }

    xiaozhi_priv.app->socket = rws_socket_create();
    if (xiaozhi_priv.app->socket == RT_NULL) {
        LOG_E("Socket create failed.");
        return (-RT_ERROR);
    }

    xiaozhi_priv.app->connected = RT_FALSE;
    xiaozhi_priv.app->connecting = RT_TRUE;  /* Mark as connecting */

    /* Set connection URL */
    rws_socket_set_url(xiaozhi_priv.app->socket, "wss", XIAOZHI_WS_URL, XIAOZHI_WS_PORT, XIAOZHI_WS_PATH);

    /* Add header fields */
    rws_socket_add_header(xiaozhi_priv.app->socket, AUTHORIZATION_HEADER, AUTHORIZATION_VALUE);
    rws_socket_add_header(xiaozhi_priv.app->socket, PROTOCOL_VERSION_HEADER, PROTOCOL_VERSION_VALUE);

    /* Obtain device information from OTA auth module interface */
    rws_socket_add_header(xiaozhi_priv.app->socket, DEVICE_ID_HEADER, ota_auth_get_device_mac());
    rws_socket_add_header(xiaozhi_priv.app->socket, CLIENT_ID_HEADER, ota_auth_get_device_client_id());

    /* Set callback functions */
    rws_socket_set_on_connected(xiaozhi_priv.app->socket, &on_open);
    rws_socket_set_on_disconnected(xiaozhi_priv.app->socket, &on_close);
    rws_socket_set_on_received_text(xiaozhi_priv.app->socket, &on_message_text);
    rws_socket_set_on_received_bin(xiaozhi_priv.app->socket, &on_message_bin);
    rws_socket_set_custom_mode(xiaozhi_priv.app->socket);

    /* Connect to server */
    ret = rws_socket_connect(xiaozhi_priv.app->socket);
    if (ret == rws_false) {
        LOG_E("Connect to %s:%d%s failed.", XIAOZHI_WS_URL, XIAOZHI_WS_PORT, XIAOZHI_WS_PATH);
        if (xiaozhi_priv.app->socket) {
            /* Connection failed, use force-clean */
            xiaozhi_force_cleanup_socket();
        }
        xiaozhi_priv.app->connected = RT_FALSE;
        xiaozhi_priv.app->connecting = RT_FALSE;
        return (-RT_ERROR);
    }

    LOG_D("Try to connect to %s:%d%s", XIAOZHI_WS_URL, XIAOZHI_WS_PORT, XIAOZHI_WS_PATH);
    return RT_EOK;
}

/* Abort speaking */
static void xiaozhi_abort_speaking_internal(uint8_t reason)
{
    char send_msg[128]={0};
    if (xiaozhi_priv.app && xiaozhi_priv.app->socket && xiaozhi_priv.app->connected) {
        rt_snprintf(send_msg, sizeof(send_msg),
                   "{\"session_id\":\"%s\",\"type\":\"abort\",\"reason\":\"user_request\"}",
                   xiaozhi_priv.app->session_id);
        rws_socket_send_text(xiaozhi_priv.app->socket, send_msg);
    }
}

/* Start listening */
static void xiaozhi_start_listen_internal(void)
{
    char send_msg[128]={0};
    if (xiaozhi_priv.app && xiaozhi_priv.app->socket && xiaozhi_priv.app->connected) {
        rt_snprintf(send_msg, sizeof(send_msg),
                   "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"start\",\"mode\":\"auto\"}",
                   xiaozhi_priv.app->session_id);
        rws_socket_send_text(xiaozhi_priv.app->socket, send_msg);
    }
}

/* Auto-connect thread */
static void xiaozhi_auto_connect_thread(void *parameter)
{
    LOG_I("XiaoZhi auto connect thread started");

    while (xiaozhi_priv.app && xiaozhi_priv.app->running) {
        if (!xiaozhi_priv.app->connected) {
            /* If currently connecting, wait briefly */
            if (xiaozhi_priv.app->connecting) {
                LOG_D("Connection in progress (connected=%d, connecting=%d), waiting...",
                      xiaozhi_priv.app->connected, xiaozhi_priv.app->connecting);
                rt_thread_mdelay(1000);
                continue;
            }

            LOG_I("Attempting to connect to XiaoZhi server...");
            if (xiaozhi_connect_internal() != RT_EOK) {
                LOG_E("Failed to connect, will retry in %d ms", RECONNECT_INTERVAL);
                rt_thread_mdelay(RECONNECT_INTERVAL);
                continue;
            }
        }
        rt_thread_mdelay(1000);
    }

    LOG_I("XiaoZhi auto connect thread stopped");
}

/* Start auto-connect */
static int xiaozhi_auto_connect_start(void)
{
    if (xiaozhi_priv.app == RT_NULL) {
        xiaozhi_priv.app = (xiaozhi_app_t)rt_malloc_align(sizeof(struct xiaozhi_app), 4);
        if (xiaozhi_priv.app == RT_NULL) {
            LOG_E("Memory malloc failed.");
            return (-RT_ENOMEM);
        }
        rt_memset(xiaozhi_priv.app, 0x00, sizeof(struct xiaozhi_app));
    }

    /* Check whether thread already exists and is running */
    if (xiaozhi_priv.app->auto_connect_thread != RT_NULL && xiaozhi_priv.app->running == RT_TRUE) {
        LOG_W("Auto connect thread is already running");
        return RT_EOK;
    }

    /* If thread exists but is not running, clean up resources */
    if (xiaozhi_priv.app->auto_connect_thread != RT_NULL) {
        rt_thread_delete(xiaozhi_priv.app->auto_connect_thread);
        xiaozhi_priv.app->auto_connect_thread = RT_NULL;
    }

    /* Create new connection thread */
    xiaozhi_priv.app->running = RT_TRUE;
    xiaozhi_priv.app->auto_connect_thread = rt_thread_create("xiaozhi_ws",
                              xiaozhi_auto_connect_thread,
                              RT_NULL,
                              AUTO_CONNECT_THREAD_STACK_SIZE,
                              AUTO_CONNECT_THREAD_PRIORITY,
                              10);

    if (xiaozhi_priv.app->auto_connect_thread != RT_NULL) {
        rt_thread_startup(xiaozhi_priv.app->auto_connect_thread);
        LOG_I("XiaoZhi auto connect thread started successfully");
        return RT_EOK;
    } else {
        LOG_E("Failed to create XiaoZhi auto connect thread");
        xiaozhi_priv.app->running = RT_FALSE;
        return (-RT_ERROR);
    }
}

/* XiaoZhi module message handler */
static int xiaozhi_msg_handler(const app_message_t *msg)
{
    if (msg == RT_NULL) {
        return -RT_EINVAL;
    }

    switch (msg->header.type) {
        case MSG_TYPE_SYS_INIT:
            LOG_I("XiaoZhi module received init message");
            break;

        case MSG_TYPE_NET_READY:
            /* Send request only when not requested before and auth not completed */
            if (!xiaozhi_priv.ota_auth_requested && !xiaozhi_priv.ota_auth_completed) {
                LOG_I("Network ready, requesting OTA authentication");
                xiaozhi_priv.ota_auth_requested = RT_TRUE;
                /* Send OTA auth request message */
                {
                    app_message_t auth_req_msg;
                    rt_memset(&auth_req_msg, 0, sizeof(auth_req_msg));
                    auth_req_msg.header.type = MSG_TYPE_OTA_AUTH_REQUEST;
                    auth_req_msg.header.src_module = MODULE_ID_XIAOZHI;
                    auth_req_msg.header.dst_module = MODULE_ID_OTA_AUTH;
                    auth_req_msg.header.data_len = 0;
                    module_send_msg(&auth_req_msg);
                }
            } else {
                LOG_D("OTA auth already requested or completed, ignore NET_READY");
            }
            break;

        case MSG_TYPE_OTA_AUTH_SUCCESS:
            /* Handle only when auth not completed, avoid duplicate handling */
            if (!xiaozhi_priv.ota_auth_completed) {
                LOG_I("OTA authentication success, starting XiaoZhi connection");
                xiaozhi_priv.ota_auth_completed = RT_TRUE;
                /* OTA auth success: start auto-connect (function checks running state internally) */
                xiaozhi_auto_connect_start();
            } else {
                LOG_D("OTA auth already completed, ignore duplicate success message");
            }
            break;

        case MSG_TYPE_OTA_AUTH_FAILED:
            LOG_E("OTA authentication failed, cannot connect to XiaoZhi");
            xiaozhi_priv.ota_auth_requested = RT_FALSE;  /* Allow retry */
            /* OTA auth failed; retry logic can be implemented here */
            break;

        case MSG_TYPE_XIAOZHI_ABORT_SPEAK:
            LOG_I("Received abort speak message");
            xiaozhi_abort_speaking_internal(0);
            break;

        case MSG_TYPE_XIAOZHI_START_LISTEN:
            LOG_I("Received start listen message");
            xiaozhi_start_listen_internal();
            break;

        case MSG_TYPE_AUDIO_ENCODE_OPUS:
            /* Receive OPUS data encoded by Audio module and send to server */
            if (msg->data.audio.data != RT_NULL && msg->data.audio.data_len > 0) {
                xiaozhi_send_binary_opus((const unsigned char *)msg->data.audio.data,
                                         msg->data.audio.data_len);
                /* Audio buffer is freed by XiaoZhi module in this branch */
                rt_free(msg->data.audio.data);
            }
            break;

        default:
            LOG_D("XiaoZhi module received unhandled msg type: 0x%02X", msg->header.type);
            break;
    }

    return RT_EOK;
}

/* XiaoZhi module init */
static int xiaozhi_init(void)
{
    rt_memset(&xiaozhi_priv, 0, sizeof(xiaozhi_priv));
    xiaozhi_priv.initialized = RT_FALSE;


    LOG_I("XiaoZhi module initialized");

    /* Subscribe messages */
    msg_bus_subscribe(MODULE_ID_XIAOZHI, MSG_TYPE_NET_READY);
    msg_bus_subscribe(MODULE_ID_XIAOZHI, MSG_TYPE_OTA_AUTH_SUCCESS);
    msg_bus_subscribe(MODULE_ID_XIAOZHI, MSG_TYPE_OTA_AUTH_FAILED);
    msg_bus_subscribe(MODULE_ID_XIAOZHI, MSG_TYPE_XIAOZHI_ABORT_SPEAK);
    msg_bus_subscribe(MODULE_ID_XIAOZHI, MSG_TYPE_XIAOZHI_START_LISTEN);
    msg_bus_subscribe(MODULE_ID_XIAOZHI, MSG_TYPE_AUDIO_ENCODE_OPUS);
    msg_bus_subscribe(MODULE_ID_XIAOZHI, MSG_TYPE_SYS_INIT);

    return RT_EOK;
}

/* XiaoZhi module deinit */
static int xiaozhi_deinit(void)
{
    if (xiaozhi_priv.app) {
        xiaozhi_priv.app->running = RT_FALSE;
        if (xiaozhi_priv.app->auto_connect_thread != RT_NULL) {
            rt_thread_delete(xiaozhi_priv.app->auto_connect_thread);
            xiaozhi_priv.app->auto_connect_thread = RT_NULL;
        }
        if (xiaozhi_priv.app->socket != RT_NULL) {
            rws_socket_disconnect_and_release(xiaozhi_priv.app->socket);
            xiaozhi_priv.app->socket = RT_NULL;
        }
        rt_free(xiaozhi_priv.app);
        xiaozhi_priv.app = RT_NULL;
    }

    msg_bus_unsubscribe(MODULE_ID_XIAOZHI, MSG_TYPE_NET_READY);
    msg_bus_unsubscribe(MODULE_ID_XIAOZHI, MSG_TYPE_OTA_AUTH_SUCCESS);
    msg_bus_unsubscribe(MODULE_ID_XIAOZHI, MSG_TYPE_OTA_AUTH_FAILED);
    msg_bus_unsubscribe(MODULE_ID_XIAOZHI, MSG_TYPE_XIAOZHI_ABORT_SPEAK);
    msg_bus_unsubscribe(MODULE_ID_XIAOZHI, MSG_TYPE_XIAOZHI_START_LISTEN);
    msg_bus_unsubscribe(MODULE_ID_XIAOZHI, MSG_TYPE_AUDIO_ENCODE_OPUS);
    msg_bus_unsubscribe(MODULE_ID_XIAOZHI, MSG_TYPE_SYS_UNKNOWN);

    LOG_I("XiaoZhi module deinitialized");
    return RT_EOK;
}

/* XiaoZhi module thread entry */
static void xiaozhi_thread_entry(void *parameter)
{
    app_message_t msg;

    LOG_I("XiaoZhi module thread started");

    while (1) {
        /* Receive message */
        if (msg_bus_receive(MODULE_ID_XIAOZHI, &msg, RT_WAITING_FOREVER) == RT_EOK) {
            xiaozhi_msg_handler(&msg);
        }
    }
}

/* XiaoZhi module operations */
static module_ops_t xiaozhi_ops = {
    .init = xiaozhi_init,
    .deinit = xiaozhi_deinit,
    .msg_handler = xiaozhi_msg_handler,
    .name = "XiaoZhi Module",
    .id = MODULE_ID_XIAOZHI,
};

/* XiaoZhi module instance */
static module_t xiaozhi_module = {
    .ops = &xiaozhi_ops,
    .active = RT_FALSE,
    .thread = RT_NULL,
    .priv_data = &xiaozhi_priv,
};

/* Get XiaoZhi module instance */
module_t *xiaozhi_module_get(void)
{
    return &xiaozhi_module;
}

/* Initialize XiaoZhi module thread */
int xiaozhi_module_thread_start(void)
{
    xiaozhi_module.thread = rt_thread_create("xiaozhi_module",
                                              xiaozhi_thread_entry,
                                              RT_NULL,
                                              8192,
                                              15,
                                              10);
    if (xiaozhi_module.thread != RT_NULL) {
        rt_thread_startup(xiaozhi_module.thread);
        return RT_EOK;
    }
    return -RT_ERROR;
}

