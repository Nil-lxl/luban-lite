/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "llm_answer.h"
#include "webclient.h"
#include "stdio.h"
#include <string.h>
#include <cJSON.h>

#define DEFAULT_SYSTEM_PROMPT "Answer with chinese"
cJSON *read_llm_config(const char *file, const char *inputText, const char *model_name);
cJSON *read_llm_config_default(const char *inputText, const char *model_name);
static const char* read_http_status_message(int status_code);

char *llm_generate_response(llm_config_t *config)
{
    struct webclient_session *webSession = NULL;
    char *allContent = NULL;
    int bytesRead, responseStatus;

    if (config->prompt == NULL || config->url == NULL ||
        config->api_key == NULL || config->model == NULL) {
        printf("API key/URL/model is NULL\n");
        return NULL;
    }

    memset(config->response_buf, 0, WEB_SOCKET_BUF_SIZE);
    memset(config->content_buffer, 0, WEB_SOCKET_BUF_SIZE);
    // Create JSON payload
    cJSON *llm_config = read_llm_config(config->config_file, config->prompt, config->model);
    if (!llm_config)
        llm_config = read_llm_config_default(config->prompt, config->model);

    char *payload = cJSON_PrintUnformatted(llm_config);
    if (payload == NULL) {
        printf("Failed to create JSON payload.\n");
        goto cleanup;
    }

    // Create web session
    webSession = webclient_session_create(WEB_SOCKET_BUF_SIZE);
    if (webSession == NULL) {
        printf("Failed to create webclient session.\n");
        goto cleanup;
    }

    // Prepare authorization header
    snprintf(config->auth_header, sizeof(config->auth_header), "Authorization: Bearer %s\r\n", config->api_key);

    // Add headers
    webclient_header_fields_add(webSession, "Content-Type: application/json\r\n");
    webclient_header_fields_add(webSession, config->auth_header);
    webclient_header_fields_add(webSession, "Content-Length: %d\r\n", (int)strlen(payload));
    webclient_header_fields_add(webSession, "Accept-Charset: utf-8\r\n");

    // Send POST request
    responseStatus = webclient_post(webSession, config->url, payload, strlen(payload));
    if (responseStatus != 200) {
        static char debug_info[512] = {0};
        const char *http_status = read_http_status_message(responseStatus);
        snprintf(debug_info, sizeof(debug_info), "http code = %s, info %d\n", (char *)http_status, responseStatus);
        if (config->callback)
            config->callback(debug_info, config->callback_para);
        printf("config = %s\n", payload);
        goto cleanup;
    }

    // Read and process response
    while ((bytesRead = webclient_read(webSession, config->response_buf, WEB_SOCKET_BUF_SIZE)) > 0) {
        int inContent = 0;
        for (int i = 0; i < bytesRead; i++)  {
            if (inContent) {
                if (config->response_buf[i] == '"') {
                    inContent = 0;

                    // Append content to allContent
                    char *oldAllContent = allContent;
                    size_t oldLen = oldAllContent ? strlen(oldAllContent) : 0;
                    size_t newLen = strlen(config->content_buffer);
                    size_t totalLen = oldLen + newLen + 1;

                    char *newAllContent = (char *)web_malloc(totalLen);
                    if (newAllContent) {
                        newAllContent[0] = '\0';
                        if (oldAllContent) {
                            strcpy(newAllContent, oldAllContent);
                        }
                        strcat(newAllContent, config->content_buffer);
                        allContent = newAllContent;
                        // printf("%s", config->content_buffer);
                        free(oldAllContent);

                        if (config->callback)
                            config->callback(allContent, config->callback_para);
                    } else {
                        printf("Memory allocation failed, content truncated!\n");
                    }

                    config->content_buffer[0] = '\0'; // Reset content buffer
                } else {
                    strncat(config->content_buffer, &config->response_buf[i], 1);
                }
            } else if (config->response_buf[i] == '"' && i > 8 &&
                    strncmp(&config->response_buf[i - 10], "\"content\":\"", 10) == 0) {
                inContent = 1;
            }
        }
    }

cleanup:
    // Cleanup resources
    if (webSession)
        webclient_close(webSession);

    if (llm_config)
        cJSON_Delete(llm_config);


    if (payload)
        cJSON_free(payload);

    return allContent;
}

cJSON *read_llm_config(const char *file, const char *inputText, const char *model_name)
{
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        printf("Failed to open JSON config file: %s\n", file);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    if (length == -1L) {
        printf("Failed to get the file size\n");
    }
    fseek(fp, 0, SEEK_SET);
    char *data = (char *)malloc(length + 1);
    size_t read_len = fread(data, 1, length, fp);
    if (read_len != (size_t)length) {
        printf("Failed to read full file\n");
        free(data);
        fclose(fp);
        return NULL;
    }
    data[length] = '\0';
    fclose(fp);

    /* json parse*/
    cJSON *llm_config = cJSON_Parse(data);
    free(data);

    if (llm_config == NULL)
        goto clean_json;

    cJSON *model = cJSON_GetObjectItem(llm_config, "model");
    if (model == NULL)
        goto clean_json;
    cJSON_SetValuestring(model, model_name);

    cJSON *messages = cJSON_GetObjectItem(llm_config, "messages");
    if (messages == NULL || !cJSON_IsArray(messages))
        goto clean_json;

    cJSON *message = NULL;
    cJSON_ArrayForEach(message, messages) {
        cJSON *role = cJSON_GetObjectItem(message, "role");
        if (role && cJSON_IsString(role) && strcmp(role->valuestring, "user") == 0) {
            cJSON *content = cJSON_GetObjectItem(message, "content");
            if (content && cJSON_IsString(content)) {
                cJSON *new_context = cJSON_CreateString(inputText);
                cJSON_ReplaceItemInObject(message, "content", new_context);
            } else {
                cJSON_AddStringToObject(message, "content", inputText);
            }
        }
    }
    return llm_config;

clean_json:
    if (llm_config)
        cJSON_Delete(llm_config);
    return NULL;
}

cJSON *read_llm_config_default(const char *inputText, const char *model_name)
{
    cJSON *llm_config = cJSON_CreateObject();
    cJSON *model = cJSON_CreateString(model_name);
    cJSON *messages = cJSON_CreateArray();
    cJSON *systemMessage = cJSON_CreateObject();
    cJSON *userMessage = cJSON_CreateObject();

    cJSON_AddItemToObject(llm_config, "model", model);
    cJSON_AddItemToObject(llm_config, "messages", messages);
    cJSON_AddBoolToObject(llm_config, "stream", 1);

    cJSON_AddItemToArray(messages, systemMessage);
    cJSON_AddItemToArray(messages, userMessage);

    cJSON_AddStringToObject(systemMessage, "role", "system");
    cJSON_AddStringToObject(systemMessage, "content", DEFAULT_SYSTEM_PROMPT);

    cJSON_AddStringToObject(userMessage, "role", "user");
    cJSON_AddStringToObject(userMessage, "content", inputText);
    return llm_config;
}

static const char* read_http_status_message(int status_code)
{
    switch (status_code) {
        case 100: return "http status code 100: Continue";
        case 101: return "http status code 101: Switching Protocols";
        case 200: return "http status code 200: OK";
        case 201: return "http status code 201: Created";
        case 202: return "http status code 202: Accepted";
        case 204: return "http status code 204: No Content";
        case 301: return "http status code 301: Moved Permanently";
        case 302: return "http status code 302: Found";
        case 304: return "http status code 304: Not Modified";
        case 400: return "http status code 400: Bad Request";
        case 401: return "http status code 401: Unauthorized";
        case 403: return "http status code 403: Forbidden";
        case 404: return "http status code 404: Not Found";
        case 405: return "http status code 405: Method Not Allowed";
        case 500: return "http status code 500: Internal Server Error";
        case 501: return "http status code 501: Not Implemented";
        case 502: return "http status code 502: Bad Gateway";
        case 503: return "http status code 503: Service Unavailable";
        case 504: return "http status code 504: Gateway Timeout";
        case 505: return "http status code 505: HTTP Version Not Supported";
        default: return "http status code unkown";
    }
}
