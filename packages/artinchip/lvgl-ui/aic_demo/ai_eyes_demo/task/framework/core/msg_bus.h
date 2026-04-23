/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#ifndef __MSG_BUS_H__
#define __MSG_BUS_H__

#include "msg_def.h"
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Message bus interface ==================== */

/**
 * Initialize message bus
 * @return 0 on success, <0 on failure
 */
int msg_bus_init(void);

/**
 * Deinitialize message bus
 * @return 0 on success, <0 on failure
 */
int msg_bus_deinit(void);

/**
 * Subscribe to messages
 * @param module_id Module ID
 * @param msg_type Message type (use MSG_TYPE_SYS_UNKNOWN to subscribe all)
 * @return 0 on success, <0 on failure
 */
int msg_bus_subscribe(uint32_t module_id, msg_type_t msg_type);

/**
 * Unsubscribe from messages
 * @param module_id Module ID
 * @param msg_type Message type
 * @return 0 on success, <0 on failure
 */
int msg_bus_unsubscribe(uint32_t module_id, msg_type_t msg_type);

/**
 * Publish a message to the bus
 * @param msg Message pointer
 * @return 0 on success, <0 on failure
 */
int msg_bus_publish(const app_message_t *msg);

/**
 * Receive a message from the bus (timeout behavior: 0 non-blocking, >0 bounded wait, RT_WAITING_FOREVER blocking)
 * @param module_id Module ID
 * @param msg Output message pointer
 * @param timeout Timeout in ticks
 * @return 0 on success, <0 on failure
 */
int msg_bus_receive(uint32_t module_id, app_message_t *msg, rt_int32_t timeout);

/**
 * Receive a message from the bus (non-blocking)
 * @param module_id Module ID
 * @param msg Output message pointer
 * @return 0 on success, <0 on failure
 */
int msg_bus_try_receive(uint32_t module_id, app_message_t *msg);

/**
 * Clear message queue of the specified module
 * @param module_id Module ID
 * @return 0 on success, <0 on failure
 */
int msg_bus_clear(uint32_t module_id);

/**
 * Set message queue size for a module (call before module subscribes)
 * @param module_id Module ID
 * @param queue_size Queue size (0 means use default)
 * @return 0 on success, <0 on failure
 */
int msg_bus_set_queue_size(uint32_t module_id, uint32_t queue_size);

/**
 * Get message queue size of a module
 * @param module_id Module ID
 * @return Queue size, or 0 on failure
 */
uint32_t msg_bus_get_queue_size(uint32_t module_id);

#ifdef __cplusplus
}
#endif

#endif /* __MSG_BUS_H__ */

