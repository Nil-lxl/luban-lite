/**
 ****************************************************************************************
 *
 * @file uwifi_cmds.c
 *
 * @brief Handles commands from LMAC FW
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */

#include "uwifi_cmds.h"
#include "uwifi_include.h"
#include "uwifi_msg_tx.h"
#include "uwifi_msg.h"
#include "ipc_host.h"
/**
 *
 */
static void cmd_dump(const struct asr_cmd *cmd)
{
    dbg(D_ERR,D_UWIFI_CTRL,"tkn[%u]  flags:%04x  result:%d  cmd:%4u - reqcfm(%4u)\n",
                                                                (unsigned int)cmd->tkn,
                                                                cmd->flags,
                                                                (int)cmd->result,
                                                                (unsigned int)cmd->id,
                                                                (unsigned int)cmd->reqid);
}

static void cmd_mgr_drain(struct asr_cmd_mgr *cmd_mgr)
{
    struct asr_cmd *cur, *nxt;


    asr_rtos_lock_mutex(&cmd_mgr->lock);
    list_for_each_entry_safe(cur, nxt, &cmd_mgr->cmds, list) {
        list_del(&cur->list);
        cmd_mgr->queue_sz--;
        if (!(cur->flags & ASR_CMD_FLAG_NONBLOCK))  //block case
            asr_rtos_set_semaphore(&cur->semaphore);
    }
    asr_rtos_unlock_mutex(&cmd_mgr->lock);
}

void cmd_queue_crash_handle(struct asr_hw *asr_hw, const char *func, u32 line, u32 reason)
{
    struct asr_cmd_mgr *cmd_mgr = NULL;

    if (asr_hw == NULL) {
        return;
    }

    if (asr_test_bit(ASR_DEV_PRE_RESTARTING, &asr_hw->phy_flags) || asr_test_bit(ASR_DEV_RESTARTING, &asr_hw->phy_flags)) {
        dbg(D_ERR,D_UWIFI_CTRL, "%s:phy_flags=0X%X\n", __func__, (unsigned int)asr_hw->phy_flags);
        return;
    }

    asr_set_bit(ASR_DEV_PRE_RESTARTING, &asr_hw->phy_flags);

    cmd_mgr = &asr_hw->cmd_mgr;

    dump_sdio_info(asr_hw, func, line);

    //spin_lock_bh(&cmd_mgr->lock);
    cmd_mgr->state = ASR_CMD_MGR_STATE_CRASHED;
    //spin_unlock_bh(&cmd_mgr->lock);

    //send dev restart event
    asr_msleep(10);
    cmd_mgr_drain(cmd_mgr);

#ifdef ASR_MODULE_RESET_SUPPORT
    asr_hw->dev_restart_work.asr_hw = asr_hw;
    asr_hw->dev_restart_work.parm1 = reason;
    //schedule_work(&asr_hw->dev_restart_work.real_work);
#endif
}

/**
 *
 */
static void cmd_complete(struct asr_cmd_mgr *cmd_mgr, struct asr_cmd *cmd)
{
    list_del(&cmd->list);
    cmd_mgr->queue_sz--;
    cmd->flags |= ASR_CMD_FLAG_DONE;
    if (cmd->flags & ASR_CMD_FLAG_NONBLOCK) {
        asr_rtos_free(cmd);
        cmd = NULL;
    } else { //block case
        if (ASR_CMD_WAIT_COMPLETE(cmd->flags)) {
            cmd->result = 0;
            asr_rtos_set_semaphore(&cmd->semaphore);
        }
    }
}

/**
 *
 */
int g_cmd_ret;
struct asr_cmd *g_cmd;
asr_semaphore_t *g_sem;

static int cmd_mgr_queue(struct asr_cmd_mgr *cmd_mgr, struct asr_cmd *cmd)
{
    struct asr_hw *asr_hw = container_of(cmd_mgr, struct asr_hw, cmd_mgr);
    unsigned int tout;
    int ret = 0;
    asr_semaphore_t cmd_sem;
    struct asr_cmd *cmd_check = NULL;
    struct asr_cmd *cur = NULL, *nxt = NULL;

    dbg(D_ERR, D_UWIFI_CTRL, "dTX (%d,%d)->(%d,%d),flag=0x%x",MSG_T(cmd->id),MSG_I(cmd->id),MSG_T(cmd->reqid),MSG_I(cmd->reqid),cmd->flags);
    asr_rtos_init_semaphore(&cmd_sem,0);
    asr_rtos_lock_mutex(&cmd_mgr->lock);
    if (cmd_mgr->state == ASR_CMD_MGR_STATE_CRASHED)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"ASR_CMD_MGR_STATE_CRASHED\r\n");
        cmd->result = -EPIPE;
        asr_rtos_free(cmd->a2e_msg);
        cmd->a2e_msg = NULL;
        asr_rtos_unlock_mutex(&cmd_mgr->lock);
        asr_rtos_deinit_semaphore(&cmd_sem);
        return -EPIPE;
    }

    if (!list_empty(&cmd_mgr->cmds))
    {
        if (cmd_mgr->queue_sz == cmd_mgr->max_queue_sz)
        {
            dbg(D_ERR,D_UWIFI_CTRL,"ENOMEM\r\n");
            cmd->result = -ENOMEM;
            asr_rtos_free(cmd->a2e_msg);
            cmd->a2e_msg = NULL;
            asr_rtos_unlock_mutex(&cmd_mgr->lock);
            asr_rtos_deinit_semaphore(&cmd_sem);
            return -ENOMEM;
        }
    }

    if (cmd->flags & ASR_CMD_FLAG_REQ_CFM)
        cmd->flags |= ASR_CMD_FLAG_WAIT_CFM;

    cmd->tkn    = cmd_mgr->next_tkn++;
    cmd->result = -EINTR;

    if (!(cmd->flags & ASR_CMD_FLAG_NONBLOCK)) //block case
        asr_rtos_init_semaphore(&cmd->semaphore, 0);
    list_add_tail(&cmd->list, &cmd_mgr->cmds);
    cmd_mgr->queue_sz++;
    if (SM_DISCONNECT_CFM == cmd->reqid)
    {
        /* when disconnect, the deauth may cannot send out for 3second more,
           delete it when TX performance improve */
        tout = ASR_NEVER_TIMEOUT;
    }
    else
    {
        tout = (ASR_80211_CMD_TIMEOUT_MS * cmd_mgr->queue_sz);
    }
    asr_rtos_unlock_mutex(&cmd_mgr->lock);

    #if 0//ndef MSG_REFINE
    g_cmd = cmd;
    g_sem = &cmd_sem;
    uwifi_sdio_event_set(UWIFI_SDIO_EVENT_MSG);
    asr_rtos_get_semaphore(&cmd_sem,ASR_WAIT_FOREVER);
    g_cmd = NULL;
    g_sem = NULL;
    asr_rtos_deinit_semaphore(&cmd_sem);
    ret = g_cmd_ret;
    #else

    asr_rtos_deinit_semaphore(&cmd_sem);

    /* send cmd to FW, SDIO
       if error, return and release msg and cmd

       ipc_host_msg_push in uwifi task instead of uwifi sdio task.
     */
    ret = ipc_host_msg_push(asr_hw->ipc_env, (void *)cmd, sizeof(struct lmac_msg) + cmd->a2e_msg->param_len);

    #endif

    asr_rtos_free(cmd->a2e_msg);
    cmd->a2e_msg = NULL;

    if (ret) {      // has error
        asr_rtos_lock_mutex(&cmd_mgr->lock);
        list_for_each_entry_safe(cur, nxt, &cmd_mgr->cmds, list) {
            if (cmd == cur) {
                list_del(&cur->list);
                cmd_mgr->queue_sz--;
            }
        }
        asr_rtos_unlock_mutex(&cmd_mgr->lock);

        dbg(D_ERR,D_UWIFI_CTRL,"[%s] ipc msg push err,ret=%d\n", __func__,  ret);

        if (ret != -ENOSYS)
            cmd_queue_crash_handle(asr_hw, __func__, __LINE__, ASR_RESTART_REASON_TXMSG_FAIL);

        return -EBUSY;
    }

    if (!(cmd->flags & ASR_CMD_FLAG_NONBLOCK)) //block case
    {
        int rx_retry = ASR_80211_CMD_TIMEOUT_RETRY;
        while(rx_retry--)
        {
            if (asr_rtos_get_semaphore(&cmd->semaphore, tout))
            {
                if(rx_retry)
                {
                    tout = ASR_80211_CMD_TIMEOUT_MS;
                    dbg(D_ERR, D_UWIFI_CTRL, "rx msg retry(%d),(%d,%d)->(%d,%d)\n",rx_retry,MSG_T(cmd->id),MSG_I(cmd->id),MSG_T(cmd->reqid),MSG_I(cmd->reqid));
                    uwifi_sdio_event_set(UWIFI_SDIO_EVENT_RX);
                }
                else
                {
                    dbg(D_DBG, D_UWIFI_CTRL, "%s: flags=%d tout=%d reqid=%d queue_sz=%d\r\n", __func__,cmd->flags,tout,cmd->reqid,cmd_mgr->queue_sz);
                    dbg(D_ERR, D_UWIFI_CTRL, "cmd timed-out (%d,%d)->(%d,%d)\n",MSG_T(cmd->id),MSG_I(cmd->id),MSG_T(cmd->reqid),MSG_I(cmd->reqid));
                    asr_rtos_deinit_semaphore(&cmd->semaphore);
                    cmd_dump(cmd);
                    asr_rtos_lock_mutex(&cmd_mgr->lock);
                    cmd_mgr->state = ASR_CMD_MGR_STATE_CRASHED;
                    if (!(cmd->flags & ASR_CMD_FLAG_DONE)) {
                        cmd->result = -ETIMEDOUT;
                        cmd_complete(cmd_mgr, cmd);
                    }
                    asr_rtos_unlock_mutex(&cmd_mgr->lock);
                    return -ETIMEDOUT;
                }
            }
            else
            {
                asr_rtos_deinit_semaphore(&cmd->semaphore);
                break;
            }
        }
    } else {
        cmd->result = 0;
    }

    // cmd del when no req cfm,otherwise in cmd_complete for block case.
    if(!(cmd->flags & ASR_CMD_FLAG_REQ_CFM))
    {
        asr_rtos_lock_mutex(&cmd_mgr->lock);
        list_del(&cmd->list);
        cmd_mgr->queue_sz--;
        asr_rtos_unlock_mutex(&cmd_mgr->lock);
    }

    // ? check cmd del.
    asr_rtos_lock_mutex(&cmd_mgr->lock);
    list_for_each_entry(cmd_check, &cmd_mgr->cmds, list) {
        if (cmd_check == cmd) {

            // non-block but req cfm will del here. if del , cannot found in cmd_mgr_msgind. ex. ME_TRAFFIC_IND_REQ
            //list_del(&cmd->list);
            //cmd_mgr->queue_sz--;
            //dbg(D_ERR, D_UWIFI_CTRL, "ERROR: cmd check in! reqid=%d, flags=0x%x \n",cmd->reqid,cmd->flags);
            break;
        }
    }

    asr_rtos_unlock_mutex(&cmd_mgr->lock);

    return 0;
}

/**
 *
 */
static int cmd_mgr_msgind(struct asr_cmd_mgr *cmd_mgr, struct ipc_e2a_msg *msg, msg_cb_fct cb)
{
    struct asr_hw *asr_hw = container_of(cmd_mgr, struct asr_hw, cmd_mgr);
    struct asr_cmd *cmd;
    bool found = false;

    asr_rtos_lock_mutex(&cmd_mgr->lock);
    list_for_each_entry(cmd, &cmd_mgr->cmds, list) {
        if (cmd->reqid == msg->id &&
            (cmd->flags & ASR_CMD_FLAG_WAIT_CFM)) {
            if (!cb || (cb && !cb(asr_hw, cmd, msg))) {
                found = true;
                cmd->flags &= ~ASR_CMD_FLAG_WAIT_CFM;
                if (cmd->e2a_msg && msg->param_len)
                    memcpy(cmd->e2a_msg, &msg->param, msg->param_len);
                if (ASR_CMD_WAIT_COMPLETE(cmd->flags))
                    cmd_complete(cmd_mgr, cmd);

                break;
            }
        }
    }

    asr_rtos_unlock_mutex(&cmd_mgr->lock);

    if (!found && cb)
    {
        //dbg(D_ERR, D_UWIFI_CTRL, "cmd->flags:0x%x,(%d %d)", cmd->flags,MSG_T(msg->id),MSG_I(msg->id));
        cb(asr_hw, NULL, msg);
    }
    return 0;
}


/**
 *
 */
int asr_cmd_mgr_init(struct asr_cmd_mgr *cmd_mgr)
{
    INIT_LIST_HEAD(&cmd_mgr->cmds);
    if(asr_rtos_init_mutex(&cmd_mgr->lock))
        return -1;
    cmd_mgr->max_queue_sz = ASR_CMD_MAX_QUEUED;
    cmd_mgr->queue  = &cmd_mgr_queue;

    //cmd_mgr->print  = &cmd_mgr_print;
    //cmd_mgr->drain  = &cmd_mgr_drain;
    //cmd_mgr->llind  = &cmd_mgr_llind;

    cmd_mgr->msgind = &cmd_mgr_msgind;

    return 0;
}

/**
 *
 */
void asr_cmd_mgr_deinit(struct asr_cmd_mgr *cmd_mgr)
{
    //cmd_mgr->print(cmd_mgr);
    //cmd_mgr->drain(cmd_mgr);
    //cmd_mgr->print(cmd_mgr);
    asr_rtos_deinit_mutex(&cmd_mgr->lock);
    memset(cmd_mgr, 0, sizeof(*cmd_mgr));
}
