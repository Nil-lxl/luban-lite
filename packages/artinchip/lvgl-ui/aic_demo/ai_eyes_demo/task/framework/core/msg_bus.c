/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#include "msg_bus.h"
#include <string.h>
#include <rtdbg.h>

#define DBG_SECTION_NAME "MSG_BUS"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#define MAX_MODULES           16      /* Max module count */
#define MSG_QUEUE_SIZE_DEFAULT 8      /* Default queue size (further reduced to save memory) */
#define MAX_SUBSCRIBERS       8       /* Max subscribers per message type */
#define SUBSCRIPTION_SLOT_MAX 64      /* Must match subscription_table[] length */
#define DIRECT_QUEUE_FULL_LOG_INTERVAL 10U  /* Rate-limit direct-dst queue-full warnings */

/* Subscription table: records subscribing modules per message type */
typedef struct {
    msg_type_t msg_type;
    uint32_t   subscribers[MAX_SUBSCRIBERS];
    uint32_t   count;
} subscription_table_t;

/* Module message queues */
static rt_mq_t module_msg_queues[MAX_MODULES];
static subscription_table_t subscription_table[SUBSCRIPTION_SLOT_MAX];  /* Supports 64 message types */
static rt_bool_t msg_bus_inited = RT_FALSE;

/* Module queue-size config table (0 means default value) */
static uint32_t module_queue_sizes[MAX_MODULES] = {
    0,                              /* [0] MODULE_ID_NONE - unused */
    MSG_QUEUE_SIZE_DEFAULT,        /* [1] MODULE_ID_WIFI - default */
    32,                             /* [2] MODULE_ID_XIAOZHI - higher traffic, increase queue to 32 */
    MSG_QUEUE_SIZE_DEFAULT,        /* [3] MODULE_ID_UI - default */
    32,                             /* [4] MODULE_ID_AUDIO - more OPUS decode messages, increase queue to 32 */
    MSG_QUEUE_SIZE_DEFAULT,        /* [5] MODULE_ID_GPIO - default */
    MSG_QUEUE_SIZE_DEFAULT,        /* [6] MODULE_ID_OTA_AUTH - default */
    /* Other modules [7-15] default to 0, using MSG_QUEUE_SIZE_DEFAULT */
};

/* Forward declaration for logging module queue depth */
uint32_t msg_bus_get_queue_size(uint32_t module_id);

/* Rate-limited LOG for direct dst when queue stays full after optional audio drain */
static uint32_t s_direct_dst_queue_full_warn_count[MAX_MODULES];

/**
 * Create message queue for module (lazy creation)
 */
static int msg_bus_create_module_queue(uint32_t module_id)
{
    char name[16];
    uint32_t queue_size;

    if (module_id == 0 || module_id >= MAX_MODULES) {
        return -RT_EINVAL;
    }

    /* If queue already exists, return success directly */
    if (module_msg_queues[module_id] != RT_NULL) {
        return RT_EOK;
    }

    /* Get module queue-size config (0 means default value) */
    queue_size = module_queue_sizes[module_id];
    if (queue_size == 0) {
        queue_size = MSG_QUEUE_SIZE_DEFAULT;
    }

    rt_snprintf(name, sizeof(name), "mq_mod_%02d", module_id);

    LOG_D("Creating message queue for module %d: name=%s, msg_size=%d bytes, max_msgs=%d",
          module_id, name, MSG_TOTAL_SIZE, queue_size);

    /* rt_mq_create argument order: (name, msg_size, max_msgs, flag) */
    module_msg_queues[module_id] = rt_mq_create(name,
                                                MSG_TOTAL_SIZE,  /* Size of each message */
                                                queue_size,      /* Max message count */
                                                RT_IPC_FLAG_FIFO);
    if (module_msg_queues[module_id] == RT_NULL) {
        LOG_E("Failed to create message queue for module %d (msg_size=%d bytes, max_msgs=%d)",
              module_id, MSG_TOTAL_SIZE, queue_size);
        return -RT_ERROR;
    }

    LOG_I("Created message queue for module %d successfully (msg_size=%d bytes, max_msgs=%d)",
          module_id, MSG_TOTAL_SIZE, queue_size);
    return RT_EOK;
}

/**
 * Get subscription-table index
 */
static int get_subscription_index(msg_type_t msg_type)
{
    int i;
    /* First check whether it already exists */
    for (i = 0; i < SUBSCRIPTION_SLOT_MAX; i++) {
        if (subscription_table[i].msg_type == msg_type) {
            return i;
        }
    }
    /* Find free slot */
    for (i = 0; i < SUBSCRIPTION_SLOT_MAX; i++) {
        if (subscription_table[i].msg_type == MSG_TYPE_SYS_UNKNOWN &&
            subscription_table[i].count == 0) {
            /* Found free slot */
            subscription_table[i].msg_type = msg_type;
            return i;
        }
    }
    return -1;
}

/**
 * Initialize message bus (lazy mode: do not pre-create queues)
 */
int msg_bus_init(void)
{
    int i;

    if (msg_bus_inited == RT_TRUE) {
        LOG_W("Message bus already initialized");
        return 0;
    }

    /* Initialize queue array (all NULL, lazy creation) */
    rt_memset(module_msg_queues, 0, sizeof(module_msg_queues));

    /* module 0 (MODULE_ID_NONE) does not use message queue */
    module_msg_queues[0] = RT_NULL;

    /* Initialize subscription table */
    rt_memset(subscription_table, 0, sizeof(subscription_table));
    for (i = 0; i < SUBSCRIPTION_SLOT_MAX; i++) {
        subscription_table[i].msg_type = MSG_TYPE_SYS_UNKNOWN;
    }

    msg_bus_inited = RT_TRUE;
    LOG_I("Message bus initialized successfully (lazy queue creation enabled)");

    return RT_EOK;
}

/**
 * Deinitialize message bus
 */
int msg_bus_deinit(void)
{
    int i;

    if (msg_bus_inited == RT_FALSE) {
        return 0;
    }

    /* Delete all message queues (skip MODULE_ID_NONE = 0) */
    for (i = 1; i < MAX_MODULES; i++) {
        if (module_msg_queues[i] != RT_NULL) {
            rt_mq_delete(module_msg_queues[i]);
            module_msg_queues[i] = RT_NULL;
        }
    }
    module_msg_queues[0] = RT_NULL;

    rt_memset(subscription_table, 0, sizeof(subscription_table));
    msg_bus_inited = RT_FALSE;

    LOG_I("Message bus deinitialized");

    return RT_EOK;
}

/**
 * Subscribe message
 */
int msg_bus_subscribe(uint32_t module_id, msg_type_t msg_type)
{
    int idx;
    int ret;

    if (module_id == 0 || module_id >= MAX_MODULES) {
        LOG_E("Invalid module ID: %d (0 is reserved for MODULE_ID_NONE)", module_id);
        return -RT_EINVAL;
    }

    /* If message queue does not exist, create it (lazy creation) */
    if (module_msg_queues[module_id] == RT_NULL) {
        ret = msg_bus_create_module_queue(module_id);
        if (ret != RT_EOK) {
            LOG_E("Failed to create message queue for module %d during subscribe", module_id);
            return ret;
        }
    }

    idx = get_subscription_index(msg_type);
    if (idx < 0) {
        LOG_E("Subscription table full");
        return -RT_EFULL;
    }

    /* Check whether already subscribed */
    for (uint32_t i = 0; i < subscription_table[idx].count; i++) {
        if (subscription_table[idx].subscribers[i] == module_id) {
            LOG_D("Module %d already subscribed to msg type 0x%02X", module_id, msg_type);
            return RT_EOK;
        }
    }

    /* Add to subscriber list */
    if (subscription_table[idx].count >= MAX_SUBSCRIBERS) {
        LOG_E("Too many subscribers for msg type 0x%02X", msg_type);
        return -RT_EFULL;
    }

    subscription_table[idx].subscribers[subscription_table[idx].count] = module_id;
    subscription_table[idx].count++;

    LOG_D("Module %d subscribed to msg type 0x%02X", module_id, msg_type);

    return RT_EOK;
}

/**
 * Unsubscribe message
 */
int msg_bus_unsubscribe(uint32_t module_id, msg_type_t msg_type)
{
    int idx;
    uint32_t i, j;

    if (module_id == 0 || module_id >= MAX_MODULES) {
        LOG_E("Invalid module ID: %d (0 is reserved for MODULE_ID_NONE)", module_id);
        return -RT_EINVAL;
    }

    idx = get_subscription_index(msg_type);
    if (idx < 0) {
        LOG_W("Msg type 0x%02X not found in subscription table", msg_type);
        return -RT_ERROR;
    }

    /* Remove from subscriber list */
    for (i = 0; i < subscription_table[idx].count; i++) {
        if (subscription_table[idx].subscribers[i] == module_id) {
            /* Shift subsequent elements forward */
            for (j = i; j < subscription_table[idx].count - 1; j++) {
                subscription_table[idx].subscribers[j] =
                    subscription_table[idx].subscribers[j + 1];
            }
            subscription_table[idx].count--;
            LOG_D("Module %d unsubscribed from msg type 0x%02X", module_id, msg_type);
            return RT_EOK;
        }
    }

    LOG_W("Module %d not subscribed to msg type 0x%02X", module_id, msg_type);
    return -RT_ERROR;
}

static rt_bool_t msg_bus_msg_type_is_audio_opus(msg_type_t t)
{
    return (t == MSG_TYPE_AUDIO_ENCODE_OPUS || t == MSG_TYPE_AUDIO_DECODE_OPUS) ? RT_TRUE : RT_FALSE;
}

/**
 * Log rt_mq_send failure for broadcast paths (type subscribers vs MSG_TYPE_SYS_UNKNOWN slot).
 */
static void msg_bus_log_broadcast_send_failure(rt_err_t ret, uint32_t module_id,
                                               const app_message_t *msg, rt_bool_t is_sys_unknown_list)
{
    if (ret == -RT_EFULL) {
        uint32_t qsize = msg_bus_get_queue_size(module_id);

        if (is_sys_unknown_list) {
            LOG_W("Broadcast(all) msg queue FULL for module %d (src=%d, type=0x%02X, "
                  "ret=%d, queue_size=%d)",
                  module_id, msg->header.src_module, msg->header.type, ret, qsize);
        } else {
            LOG_W("Broadcast msg queue FULL for module %d (src=%d, type=0x%02X, "
                  "ret=%d, queue_size=%d)",
                  module_id, msg->header.src_module, msg->header.type, ret, qsize);
        }
    } else {
        if (is_sys_unknown_list) {
            LOG_W("Failed to send broadcast(all) msg to module %d (src=%d, type=0x%02X, ret=%d)",
                  module_id, msg->header.src_module, msg->header.type, ret);
        } else {
            LOG_W("Failed to send broadcast msg to module %d (src=%d, type=0x%02X, ret=%d)",
                  module_id, msg->header.src_module, msg->header.type, ret);
        }
    }
}

/**
 * Send to one subscriber list (shared by type-broadcast and SYS_UNKNOWN broadcast).
 */
static void msg_bus_publish_to_subscriber_list(const app_message_t *msg, int idx,
                                               rt_bool_t is_sys_unknown_list)
{
    uint32_t i;
    rt_err_t ret;

    if (idx < 0 || subscription_table[idx].count == 0) {
        return;
    }

    for (i = 0; i < subscription_table[idx].count; i++) {
        uint32_t module_id = subscription_table[idx].subscribers[i];

        if (module_id >= MAX_MODULES || module_msg_queues[module_id] == RT_NULL ||
            module_id == msg->header.src_module) {
            continue;
        }

        ret = rt_mq_send(module_msg_queues[module_id], msg, MSG_TOTAL_SIZE);
        if (ret != RT_EOK) {
            msg_bus_log_broadcast_send_failure(ret, module_id, msg, is_sys_unknown_list);
        }
    }
}

/**
 * Point-to-point publish when dst_module is set.
 * Note: cannot free msg->data.audio.data here because msg is const; caller handles on failure.
 */
static rt_err_t msg_bus_publish_direct_dst(const app_message_t *msg)
{
    uint32_t dst = msg->header.dst_module;
    rt_err_t ret;
    uint32_t qsize;

    if (module_msg_queues[dst] == RT_NULL) {
        LOG_D("Message queue for module %d not exists, message type 0x%02X dropped",
              dst, msg->header.type);
        return -RT_ERROR;
    }

    ret = rt_mq_send(module_msg_queues[dst], msg, MSG_TOTAL_SIZE);
    if (ret == RT_EOK) {
        return RT_EOK;
    }

    if (ret == -RT_EFULL) {
        qsize = msg_bus_get_queue_size(dst);
        if (msg_bus_msg_type_is_audio_opus(msg->header.type)) {
            app_message_t dummy_msg;

            if (rt_mq_recv(module_msg_queues[dst], &dummy_msg, MSG_TOTAL_SIZE, 0) == RT_EOK) {
                if (msg_bus_msg_type_is_audio_opus(dummy_msg.header.type) &&
                    dummy_msg.data.audio.data != RT_NULL) {
                    rt_free(dummy_msg.data.audio.data);
                }
                ret = rt_mq_send(module_msg_queues[dst], msg, MSG_TOTAL_SIZE);
                if (ret == RT_EOK) {
                    return RT_EOK;
                }
            }
        }

        s_direct_dst_queue_full_warn_count[dst]++;
        if (s_direct_dst_queue_full_warn_count[dst] % DIRECT_QUEUE_FULL_LOG_INTERVAL == 1U) {
            LOG_W("Msg queue FULL when sending to module %d (src=%d, type=0x%02X, "
                  "ret=%d, dropped=%d, queue_size=%d)",
                  dst, msg->header.src_module, msg->header.type, ret,
                  s_direct_dst_queue_full_warn_count[dst], qsize);
        }
    } else {
        LOG_W("Failed to send msg to module %d (src=%d, type=0x%02X, ret=%d)",
              dst, msg->header.src_module, msg->header.type, ret);
    }

    return ret;
}

/**
 * Broadcast by msg->header.type subscribers, then first MSG_TYPE_SYS_UNKNOWN subscriber list.
 */
static void msg_bus_publish_broadcast(const app_message_t *msg)
{
    int idx;
    int i;

    idx = get_subscription_index(msg->header.type);
    msg_bus_publish_to_subscriber_list(msg, idx, RT_FALSE);

    for (i = 0; i < SUBSCRIPTION_SLOT_MAX; i++) {
        if (subscription_table[i].msg_type == MSG_TYPE_SYS_UNKNOWN &&
            subscription_table[i].count > 0) {
            msg_bus_publish_to_subscriber_list(msg, i, RT_TRUE);
            break;
        }
    }
}

/**
 * Publish message to bus
 */
int msg_bus_publish(const app_message_t *msg)
{
    if (msg == RT_NULL) {
        LOG_E("Invalid message pointer");
        return -RT_EINVAL;
    }

    if (msg_bus_inited == RT_FALSE) {
        LOG_E("Message bus not initialized");
        return -RT_ERROR;
    }

    LOG_D("Publishing msg type 0x%02X from module %d to module %d",
          msg->header.type, msg->header.src_module, msg->header.dst_module);

    if (msg->header.dst_module != 0 && msg->header.dst_module < MAX_MODULES) {
        return msg_bus_publish_direct_dst(msg);
    }

    msg_bus_publish_broadcast(msg);
    return RT_EOK;
}

/**
 * Receive message from bus (blocking)
 */
int msg_bus_receive(uint32_t module_id, app_message_t *msg, rt_int32_t timeout)
{
    if (module_id == 0 || module_id >= MAX_MODULES || msg == RT_NULL) {
        LOG_E("Invalid parameters: module_id=%d", module_id);
        return -RT_EINVAL;
    }

    if (module_msg_queues[module_id] == RT_NULL) {
        LOG_E("Message queue not created for module %d", module_id);
        return -RT_ERROR;
    }

    rt_err_t ret = rt_mq_recv(module_msg_queues[module_id],
                              msg, MSG_TOTAL_SIZE, timeout);

    if (ret == RT_EOK) {
        LOG_D("Module %d received msg type 0x%02X", module_id, msg->header.type);
        return RT_EOK;
    }

    return ret;
}

/**
 * Receive message from bus (non-blocking)
 */
int msg_bus_try_receive(uint32_t module_id, app_message_t *msg)
{
    return msg_bus_receive(module_id, msg, 0);
}

/**
 * Clear message queue of specified module
 */
int msg_bus_clear(uint32_t module_id)
{
    if (module_id == 0 || module_id >= MAX_MODULES) {
        return -RT_EINVAL;
    }

    if (module_msg_queues[module_id] != RT_NULL) {
        rt_mq_control(module_msg_queues[module_id], RT_IPC_CMD_RESET, RT_NULL);
    }

    return RT_EOK;
}

/**
 * Set module message queue size (call before module subscribes)
 */
int msg_bus_set_queue_size(uint32_t module_id, uint32_t queue_size)
{
    if (module_id == 0 || module_id >= MAX_MODULES) {
        LOG_E("Invalid module ID: %d", module_id);
        return -RT_EINVAL;
    }

    /* Queue size cannot be changed after queue creation */
    if (module_msg_queues[module_id] != RT_NULL) {
        LOG_W("Message queue for module %d already exists, cannot change size", module_id);
        return -RT_EBUSY;
    }

    module_queue_sizes[module_id] = queue_size;
    LOG_I("Set queue size for module %d to %d", module_id, queue_size);

    return RT_EOK;
}

/**
 * Get configured module queue size
 */
uint32_t msg_bus_get_queue_size(uint32_t module_id)
{
    if (module_id == 0 || module_id >= MAX_MODULES) {
        return 0;
    }

    uint32_t queue_size = module_queue_sizes[module_id];
    return (queue_size == 0) ? MSG_QUEUE_SIZE_DEFAULT : queue_size;
}

