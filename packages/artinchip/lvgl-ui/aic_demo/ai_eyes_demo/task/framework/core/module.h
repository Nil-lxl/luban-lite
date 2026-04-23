/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#ifndef __MODULE_H__
#define __MODULE_H__

#include "msg_def.h"
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Module ID definitions ==================== */

/**
 * Module ID enum
 * Each module has a unique ID for message routing
 */
typedef enum {
    MODULE_ID_NONE         = 0x00,
    MODULE_ID_WIFI         = 0x01,
    MODULE_ID_XIAOZHI      = 0x02,
    MODULE_ID_UI           = 0x03,
    MODULE_ID_AUDIO        = 0x04,
    MODULE_ID_GPIO         = 0x05,
    MODULE_ID_OTA_AUTH     = 0x06,
    MODULE_ID_SYS          = 0xFF,  /* System module */
} module_id_t;

/* ==================== Module interface definitions ==================== */

/**
 * Module operations struct
 * Each module must implement these interfaces
 */
typedef struct module_ops {
    /**
     * Module init
     * @return 0 on success, <0 on failure
     */
    int (*init)(void);

    /**
     * Module deinit
     * @return 0 on success, <0 on failure
     */
    int (*deinit)(void);

    /**
     * Message handler
     * @param msg Received message
     * @return 0 on success, <0 on failure
     */
    int (*msg_handler)(const app_message_t *msg);

    /**
     * Module name
     */
    const char *name;

    /**
     * Module ID
     */
    module_id_t id;
} module_ops_t;

/**
 * Module struct
 */
typedef struct module {
    module_ops_t      *ops;       /* Module operation interface */
    rt_bool_t          active;    /* Active state */
    rt_thread_t        thread;    /* Module thread handle */
    void              *priv_data; /* Private data */
} module_t;

/* ==================== Module registration and message send APIs ==================== */

/**
 * Register module
 * @param module Module struct pointer
 * @return 0 on success, <0 on failure
 */
int module_register(module_t *module);

/**
 * Unregister module
 * @param module_id Module ID
 * @return 0 on success, <0 on failure
 */
int module_unregister(module_id_t module_id);

/**
 * Send message
 * @param msg Message pointer
 * @return 0 on success, <0 on failure
 */
int module_send_msg(const app_message_t *msg);

/**
 * Send message (simplified interface)
 * @param type Message type
 * @param src_module Source module ID
 * @param dst_module Destination module ID (0 means broadcast)
 * @param data Message data pointer
 * @param data_len Data length
 * @return 0 on success, <0 on failure
 */
int module_send_msg_simple(msg_type_t type, module_id_t src_module,
                           module_id_t dst_module, void *data, uint32_t data_len);

/**
 * Get module instance
 * @param module_id Module ID
 * @return Module pointer, NULL on failure
 */
module_t *module_get(module_id_t module_id);

#ifdef __cplusplus
}
#endif

#endif /* __MODULE_H__ */

