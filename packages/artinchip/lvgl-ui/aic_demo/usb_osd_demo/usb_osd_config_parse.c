/*
 * Copyright (C) 2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui Mai <huahui.mai@artinchip.com>
 */

#include <fcntl.h>
#include <pthread.h>
#include <rthw.h>
#include <rtthread.h>
#include <shell.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#include "cJSON.h"
#include "usb_osd_settings.h"

static char *read_file(const char *filename)
{
    FILE *file = NULL;
    long length = 0;
    char *content = NULL;
    size_t read_chars = 0;

    /* open in read binary mode */
    file = fopen(filename, "rb");
    if (file == NULL) {
        goto cleanup;
    }

    /* get the length */
    if (fseek(file, 0, SEEK_END) != 0) {
        goto cleanup;
    }
    length = ftell(file);
    if (length < 0) {
        goto cleanup;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        goto cleanup;
    }

    /* allocate content buffer */
    content = (char *)malloc((size_t)length + sizeof(""));
    if (content == NULL) {
        goto cleanup;
    }

    /* read the file into memory */
    read_chars = fread(content, sizeof(char), (size_t)length, file);
    if ((long)read_chars != length) {
        free(content);
        content = NULL;
        goto cleanup;
    }
    content[read_chars] = '\0';

cleanup:
    if (file != NULL) {
        fclose(file);
    }

    return content;
}

static cJSON *parse_file(const char *filename)
{
    cJSON *parsed = NULL;
    char *content = NULL;

    content = read_file(filename);
    if (content == NULL) {
        printf("error: read osd config file failed!!!\n");
        return NULL;
    }

    parsed = cJSON_Parse(content);
    if (content != NULL)
        free(content);

    return parsed;
}

static char *default_json = "{\r    \
    \"pwm\":80,\r    \
    \"lock\": {\r    \
    \"lock_mode\":0,\r    \
    \"lock_time\":0,\r    \
    \"blank_time\":0\r    \
    },\r \
    \"image\": {\r    \
    \"brightness\":50,\r    \
    \"contrast\":50,\r    \
    \"saturation\":50,\r    \
    \"hue\":50\r    \
    }\r \
    \r} \0";

static void lv_settings_context_parse(cJSON *root, struct osd_settings_context *ctx, bool default_config)
{
    cJSON *subcjson, *cjson;

    cjson = cJSON_GetObjectItem(root, "pwm");
    ctx->pwm.cjson = cjson;
    ctx->pwm.value = cjson->valueint;

    cjson = cJSON_GetObjectItem(root, "lock");

    subcjson = cJSON_GetObjectItem(cjson, "lock_mode");
    ctx->lock_mode.cjson = subcjson;
    ctx->lock_mode.value = subcjson->valueint;
    if (default_config)
        cJSON_SetIntValue(subcjson, LV_USB_OSD_SCREEN_LOCK_MODE);

    subcjson = cJSON_GetObjectItem(cjson, "lock_time");
    ctx->lock_time.cjson = subcjson;
    ctx->lock_time.value = subcjson->valueint;
    if (default_config)
        cJSON_SetIntValue(subcjson, LV_USB_OSD_SCREEN_LOCK_TIME * 1000);

    subcjson = cJSON_GetObjectItem(cjson, "blank_time");
    ctx->blank_time.cjson = subcjson;
    ctx->blank_time.value = subcjson->valueint;
#ifdef LV_USB_OSD_SCREEN_BLANK_TIME_AFTER_LOCK
    if (default_config) {
        cJSON_SetIntValue(subcjson, LV_USB_OSD_SCREEN_BLANK_TIME_AFTER_LOCK * 1000);
        ctx->blank_time.value = subcjson->valueint;
    }
#endif

    cjson = cJSON_GetObjectItem(root, "image");

    subcjson = cJSON_GetObjectItem(cjson, "brightness");
    ctx->brightness.cjson = subcjson;
    ctx->brightness.value = subcjson->valueint;

    subcjson = cJSON_GetObjectItem(cjson, "contrast");
    ctx->contrast.cjson = subcjson;
    ctx->contrast.value = subcjson->valueint;

    subcjson = cJSON_GetObjectItem(cjson, "saturation");
    ctx->saturation.cjson = subcjson;
    ctx->saturation.value = subcjson->valueint;

    subcjson = cJSON_GetObjectItem(cjson, "hue");
    ctx->hue.cjson = subcjson;
    ctx->hue.value = subcjson->valueint;
}

static int save_default_config(char *config_file, struct osd_settings_context *settings_cxt)
{
    FILE *file = NULL;
    cJSON *root = NULL;
    char *string = NULL;

    file = fopen(config_file, "wb");
    if (file == NULL) {
        printf("fopen config file %s failed\n", config_file);
        return -1;
    }

    root = cJSON_Parse(default_json);
    if (!root) {
        printf("parse default cJSON String failed\n");
        fclose(file);
        return -1;
    }
    settings_cxt->root = root;

    lv_settings_context_parse(root, settings_cxt, true);

    string = cJSON_Print(root);
    if (string == NULL)
        printf("failed to print default cjson\n");
    else
        fwrite(string, strlen(string), 1, file);

    fclose(file);
    return 0;
}

int lv_parse_config_file(char *config_file, struct osd_settings_context *settings_cxt)
{
    int ret = 0;
    cJSON *root = NULL;
    FILE *file = NULL;

    if (!config_file || !settings_cxt) {
        ret = -1;
        goto _EXIT;
    }

    file = fopen(config_file, "rb");
    if (file == NULL) {
        ret = save_default_config(config_file, settings_cxt);
        if (ret)
            printf("save default config file error\n");
    } else {
        root = parse_file(config_file);
        if (!root) {
            printf("parse %s file error\n", config_file);
            ret = -1;
            goto _EXIT;
        }

        lv_settings_context_parse(root, settings_cxt, false);
        settings_cxt->root = root;
    }

_EXIT:

    if (file)
        fclose(file);
    return ret;
}
