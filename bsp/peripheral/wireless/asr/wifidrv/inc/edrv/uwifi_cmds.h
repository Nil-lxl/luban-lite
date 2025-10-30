/**
 ****************************************************************************************
 *
 * @file uwifi_cmds.h
 *
 * @brief uwifi cmds header
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */

#ifndef _UWIFI_CMDS_H_
#define _UWIFI_CMDS_H_

#include <stdint.h>
#include "asr_rtos.h"
#include "uwifi_wlan_list.h"
#include "wifi_types.h"
#include "ipc_shared.h"

#define ASR_80211_CMD_TIMEOUT_MS    500//20000//3000//3000//300   TODO_ASR
#define ASR_80211_CMD_TIMEOUT_RETRY    4


#define ASR_CMD_FLAG_NONBLOCK      BIT(0)
#define ASR_CMD_FLAG_REQ_CFM       BIT(1)
#define ASR_CMD_FLAG_WAIT_PUSH     BIT(2)
#define ASR_CMD_FLAG_WAIT_ACK      BIT(3)
#define ASR_CMD_FLAG_WAIT_CFM      BIT(4)
#define ASR_CMD_FLAG_DONE          BIT(5)
/* ATM IPC design makes it possible to get the CFM before the ACK,
 * otherwise this could have simply been a state enum */
#define ASR_CMD_WAIT_COMPLETE(flags) \
    (!(flags & (ASR_CMD_FLAG_WAIT_CFM)))

#define ASR_CMD_MAX_QUEUED         8

enum asr_cmd_mgr_state {
    ASR_CMD_MGR_STATE_DEINIT,
    ASR_CMD_MGR_STATE_INITED,
    ASR_CMD_MGR_STATE_CRASHED,
};

struct asr_hw;

struct asr_cmd {
    struct list_head list;
    lmac_msg_id_t id;
    lmac_msg_id_t reqid;
    struct lmac_msg *a2e_msg;
    char            *e2a_msg;
    uint32_t tkn;
    uint16_t flags;

    asr_semaphore_t semaphore;
    uint32_t result;
};

typedef int (*msg_cb_fct)(struct asr_hw *asr_hw, struct asr_cmd *cmd, struct ipc_e2a_msg *msg);

struct asr_cmd_mgr {
    enum asr_cmd_mgr_state state;
    asr_mutex_t lock;
    uint32_t next_tkn;
    uint32_t queue_sz;
    uint32_t max_queue_sz;

    struct list_head cmds;

    int  (*queue)(struct asr_cmd_mgr *, struct asr_cmd *);
    //int  (*llind)(struct asr_cmd_mgr *, struct asr_cmd *);
    int  (*msgind)(struct asr_cmd_mgr *, struct ipc_e2a_msg *, msg_cb_fct);
    //void (*print)(struct asr_cmd_mgr *);
    //void (*drain)(struct asr_cmd_mgr *);
};

int asr_cmd_mgr_init(struct asr_cmd_mgr *cmd_mgr);
void asr_cmd_mgr_deinit(struct asr_cmd_mgr *cmd_mgr);

void cmd_queue_crash_handle(struct asr_hw *asr_hw, const char *func, u32 line, u32 reason);

#endif /* _UWIFI_CMDS_H_ */
