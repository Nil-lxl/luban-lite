/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-03-22     Murphy       the first version
 */
#include <rtconfig.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <finsh.h>
#include <string.h>
#include "aic_core.h"
#include "webclient.h"
#include <env.h>
#include <ota.h>
#include <absystem_os.h>

#ifdef LPKG_USING_HTTP_OTA

#define GET_HEADER_BUFSZ  512
#define GET_RESP_BUFSZ    512
#define HTTP_OTA_DL_DELAY (10 * RT_TICK_PER_SECOND)

#define HTTP_OTA_URL PKG_HTTP_OTA_URL

static int ota_shard_download_fun_handle(char *buffer, int size)
{
    int ret;

    ret = ota_shard_download_fun(buffer, size);

    web_free(buffer);

    return ret;
}

static int http_ota_fw_download(const char *uri)
{
    int ret = RT_EOK;
    struct webclient_session *session = RT_NULL;
    int file_size;

    ret = ota_init();
    if (ret != RT_EOK) {
        LOG_E("ota initialization failed.");
        goto __exit;
    }

    /* create webclient session and set header response size */
    session = webclient_session_create(GET_HEADER_BUFSZ);
    if (!session) {
        LOG_E("open uri failed.");
        ret = -RT_ERROR;
        goto __exit;
    }

    /* get the real data length */
    webclient_shard_head_function(session, uri, &file_size);

    if (file_size == 0) {
        LOG_E("Request file size is 0!");
        LOG_I("Sample: http_ota http://192.168.31.22/ota.cpio");
        ret = -RT_ERROR;
        goto __exit;
    } else if (file_size < 0) {
        LOG_E("webclient GET request type is chunked.");
        ret = -RT_ERROR;
        goto __exit;
    }
    LOG_I("OTA file size is (%d)", file_size);
    LOG_I("\033[1A");

    /* register the handle function, you can handle data in the function */
    webclient_register_shard_position_function(session,
                                               ota_shard_download_fun_handle);

    /* the "memory size" that you can provide in the project and uri */
    ret = webclient_shard_position_function(session, uri, 0,
                                            file_size, OTA_BUFF_LEN);

    /* clear the handle function */
    webclient_register_shard_position_function(session, RT_NULL);

    if (ret == RT_EOK) {
        if (session != RT_NULL) {
            webclient_close(session);
            session = RT_NULL;
        }

        LOG_I("\033[0B");
        LOG_I("Download firmware to flash success.");
        LOG_I("System now will restart...");

        rt_thread_delay(rt_tick_from_millisecond(5));

        ret = aic_upgrade_end();
        if (ret) {
            LOG_E("Aic upgrade end");
        }

        /* Reset the device, Start new firmware */
        extern void rt_hw_cpu_reset(void);
        rt_hw_cpu_reset();
    } else {
        LOG_E("Download firmware failed.");
    }

__exit:
    if (session != RT_NULL)
        webclient_close(session);

    ota_deinit();

    return ret;
}

void http_ota(uint8_t argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("using uri: " HTTP_OTA_URL "\n");
        http_ota_fw_download(HTTP_OTA_URL);
    } else {
        http_ota_fw_download(argv[1]);
    }
}
/**
 * msh />http_ota [url]
*/
MSH_CMD_EXPORT(http_ota, Use HTTP to download the firmware);

#endif /* LPKG_USING_HTTP_OTA */
