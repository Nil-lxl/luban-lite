/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#include "app_framework.h"
#include "core/module.h"
#include "core/msg_def.h"
#include "modules/wifi_module.h"
#include "modules/gpio_module.h"
#include "modules/ota_auth_module.h"
#include "modules/xiaozhi_module.h"
#include "modules/ui_module.h"
#include "modules/audio_module.h"
#include <rtdbg.h>

#define DBG_SECTION_NAME "APP_FW"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

/* External interfaces of each module */
extern module_t *wifi_module_get(void);
extern module_t *gpio_module_get(void);
extern module_t *ota_auth_module_get(void);
extern module_t *xiaozhi_module_get(void);
extern module_t *ui_module_get(void);
extern module_t *audio_module_get(void);

/* Module thread startup functions */
extern int wifi_module_thread_start(void);
extern int gpio_module_thread_start(void);
extern int ota_auth_module_thread_start(void);
extern int xiaozhi_module_thread_start(void);
extern int ui_module_thread_start(void);
extern int audio_module_thread_start(void);

/**
 * Initialize application framework
 */
int app_framework_init(void)
{
    int ret;

    LOG_I("Initializing application framework...");

    /* Register each module */
    ret = module_register(wifi_module_get());
    if (ret != RT_EOK) {
        LOG_E("Failed to register WiFi module");
        return ret;
    }

    ret = module_register(gpio_module_get());
    if (ret != RT_EOK) {
        LOG_E("Failed to register GPIO module");
        return ret;
    }

    ret = module_register(ota_auth_module_get());
    if (ret != RT_EOK) {
        LOG_E("Failed to register OTA Auth module");
        return ret;
    }

    ret = module_register(xiaozhi_module_get());
    if (ret != RT_EOK) {
        LOG_E("Failed to register XiaoZhi module");
        return ret;
    }

    ret = module_register(ui_module_get());
    if (ret != RT_EOK) {
        LOG_E("Failed to register UI module");
        return ret;
    }

    ret = module_register(audio_module_get());
    if (ret != RT_EOK) {
        LOG_E("Failed to register Audio module");
        return ret;
    }

    /* Start module threads */
    wifi_module_thread_start();
    gpio_module_thread_start();
    ota_auth_module_thread_start();
    xiaozhi_module_thread_start();
    ui_module_thread_start();
    audio_module_thread_start();

    /* Send system init message */
    app_message_t init_msg;
    rt_memset(&init_msg, 0, sizeof(init_msg));
    init_msg.header.type = MSG_TYPE_SYS_INIT;
    init_msg.header.src_module = MODULE_ID_SYS;
    init_msg.header.dst_module = 0;  /* Broadcast */
    init_msg.header.data_len = 0;
    module_send_msg(&init_msg);

    LOG_I("Application framework initialized successfully");

    return RT_EOK;
}

/**
 * Deinitialize application framework
 */
int app_framework_deinit(void)
{
    LOG_I("Deinitializing application framework...");

    /* Unregister all modules */
    module_unregister(MODULE_ID_WIFI);
    module_unregister(MODULE_ID_GPIO);
    module_unregister(MODULE_ID_OTA_AUTH);
    module_unregister(MODULE_ID_XIAOZHI);
    module_unregister(MODULE_ID_UI);
    module_unregister(MODULE_ID_AUDIO);

    LOG_I("Application framework deinitialized");

    return RT_EOK;
}

