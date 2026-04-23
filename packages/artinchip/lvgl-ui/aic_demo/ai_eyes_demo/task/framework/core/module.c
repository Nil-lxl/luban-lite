/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#include "module.h"
#include "msg_bus.h"
#include <string.h>
#include <rtdbg.h>

#define DBG_SECTION_NAME "MODULE"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#define MAX_MODULES 16

static module_t *registered_modules[MAX_MODULES];
static rt_bool_t module_system_inited = RT_FALSE;

/**
 * Initialize module system
 */
static int module_system_init(void)
{
    if (module_system_inited == RT_TRUE) {
        return RT_EOK;
    }

    rt_memset(registered_modules, 0, sizeof(registered_modules));

    /* Initialize message bus */
    if (msg_bus_init() != RT_EOK) {
        LOG_E("Failed to initialize message bus");
        return -RT_ERROR;
    }

    module_system_inited = RT_TRUE;
    LOG_I("Module system initialized");

    return RT_EOK;
}

/**
 * Register module
 */
int module_register(module_t *module)
{
    int i;

    if (module == RT_NULL || module->ops == RT_NULL) {
        LOG_E("Invalid module pointer");
        return -RT_EINVAL;
    }

    if (module_system_init() != RT_EOK) {
        return -RT_ERROR;
    }

    /* Check whether already registered */
    if (module->ops->id < MAX_MODULES &&
        registered_modules[module->ops->id] != RT_NULL) {
        LOG_W("Module %d already registered", module->ops->id);
        return -RT_EBUSY;
    }

    /* Find free slot */
    if (module->ops->id >= MAX_MODULES) {
        for (i = 0; i < MAX_MODULES; i++) {
            if (registered_modules[i] == RT_NULL) {
                module->ops->id = i;
                break;
            }
        }
        if (i >= MAX_MODULES) {
            LOG_E("No free slot for module registration");
            return -RT_EFULL;
        }
    }

    /* Initialize module */
    if (module->ops->init && module->ops->init() != RT_EOK) {
        LOG_E("Module %s init failed", module->ops->name);
        return -RT_ERROR;
    }

    /* Register module */
    registered_modules[module->ops->id] = module;
    module->active = RT_TRUE;

    /* Subscribe to all message types (module may later subscribe selectively) */
    msg_bus_subscribe(module->ops->id, MSG_TYPE_SYS_UNKNOWN);

    LOG_I("Module %s (ID:%d) registered successfully",
          module->ops->name, module->ops->id);

    return RT_EOK;
}

/**
 * Unregister module
 */
int module_unregister(module_id_t module_id)
{
    module_t *module;

    if (module_id >= MAX_MODULES) {
        LOG_E("Invalid module ID: %d", module_id);
        return -RT_EINVAL;
    }

    module = registered_modules[module_id];
    if (module == RT_NULL) {
        LOG_W("Module %d not registered", module_id);
        return -RT_ERROR;
    }

    /* Unsubscribe */
    msg_bus_unsubscribe(module_id, MSG_TYPE_SYS_UNKNOWN);

    /* Deinitialize module */
    if (module->ops->deinit && module->ops->deinit() != RT_EOK) {
        LOG_W("Module %s deinit failed", module->ops->name);
    }

    /* Unregister module */
    registered_modules[module_id] = RT_NULL;
    module->active = RT_FALSE;

    LOG_I("Module %d unregistered", module_id);

    return RT_EOK;
}

/**
 * Send message
 */
int module_send_msg(const app_message_t *msg)
{
    if (msg == RT_NULL) {
        LOG_E("Invalid message pointer");
        return -RT_EINVAL;
    }

    /* MODULE_ID_SYS (0xFF) is a valid system module ID */
    if (msg->header.src_module != MODULE_ID_SYS && msg->header.src_module >= MAX_MODULES) {
        LOG_E("Invalid source module ID: %d", msg->header.src_module);
        return -RT_EINVAL;
    }

    /* Set timestamp */
    app_message_t *msg_copy = (app_message_t *)msg;
    msg_copy->header.timestamp = rt_tick_get();

    return msg_bus_publish(msg);
}

/**
 * Send message (simplified interface)
 */
int module_send_msg_simple(msg_type_t type, module_id_t src_module,
                           module_id_t dst_module, void *data, uint32_t data_len)
{
    app_message_t msg;

    if (data_len > MSG_MAX_DATA_SIZE) {
        LOG_E("Data length too large: %d", data_len);
        return -RT_EINVAL;
    }

    rt_memset(&msg, 0, sizeof(msg));
    msg.header.type = type;
    msg.header.src_module = src_module;
    msg.header.dst_module = dst_module;
    msg.header.timestamp = rt_tick_get();
    msg.header.data_len = data_len;

    if (data != RT_NULL && data_len > 0) {
        rt_memcpy(msg.data.raw, data, data_len);
    }

    return module_send_msg(&msg);
}

/**
 * Get module instance
 */
module_t *module_get(module_id_t module_id)
{
    if (module_id >= MAX_MODULES) {
        return RT_NULL;
    }

    return registered_modules[module_id];
}

