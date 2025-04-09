/**
 ******************************************************************************
 *
 * @file uwifi_msg_tx.c
 *
 * @brief TX function definitions
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */
#include "uwifi_include.h"
#include "uwifi_msg_tx.h"
#include "phy.h"
#include "wifi_config.h"

#if UWIFI_TEST
#include "uwifi_basic_test.h"
#endif
#ifdef CFG_SNIFFER_SUPPORT
#include "reg_mac_core.h"
#endif

#include "asr_rtos_api.h"
#include "asr_wlan_api_aos.h"
#include "uwifi_sdio.h"
#include "wpas_psk.h"

const struct mac_addr mac_addr_bcst = {{0xFFFF, 0xFFFF, 0xFFFF}};

extern struct asr_mod_params asr_module_params;
#ifdef CFG_SNIFFER_SUPPORT
/* Default MAC Rx filters that cannot be changed by mac80211 */
#define ASR_MAC80211_NOT_CHANGEABLE    (                                       \
                                         NXMAC_ACCEPT_QO_S_NULL_BIT           | \
                                         NXMAC_ACCEPT_Q_DATA_BIT              | \
                                         NXMAC_ACCEPT_DATA_BIT                | \
                                         NXMAC_ACCEPT_OTHER_MGMT_FRAMES_BIT   | \
                                         NXMAC_ACCEPT_MY_UNICAST_BIT          | \
                                         NXMAC_ACCEPT_BROADCAST_BIT           | \
                                         NXMAC_ACCEPT_BEACON_BIT              | \
                                         NXMAC_ACCEPT_PROBE_RESP_BIT            \
                                        )
#define ASR_MAC80211_FILTER_MONITOR  (0xFFFFFFFF & ~(NXMAC_ACCEPT_ERROR_FRAMES_BIT | NXMAC_EXC_UNENCRYPTED_BIT |                      \
                                                  NXMAC_EN_DUPLICATE_DETECTION_BIT))

#endif

static const int bw2chnl[] = {
    [NL80211_CHAN_WIDTH_20_NOHT] = PHY_CHNL_BW_20,
    [NL80211_CHAN_WIDTH_20]      = PHY_CHNL_BW_20,
    [NL80211_CHAN_WIDTH_40]      = PHY_CHNL_BW_40,
    [NL80211_CHAN_WIDTH_80]      = PHY_CHNL_BW_80,
    [NL80211_CHAN_WIDTH_160]     = PHY_CHNL_BW_160,
    [NL80211_CHAN_WIDTH_80P80]   = PHY_CHNL_BW_80P80,
};

#if 0
static const int chnl2bw[] = {
    [PHY_CHNL_BW_20]      = NL80211_CHAN_WIDTH_20,
    [PHY_CHNL_BW_40]      = NL80211_CHAN_WIDTH_40,
    [PHY_CHNL_BW_80]      = NL80211_CHAN_WIDTH_80,
    [PHY_CHNL_BW_160]     = NL80211_CHAN_WIDTH_160,
    [PHY_CHNL_BW_80P80]   = NL80211_CHAN_WIDTH_80P80,
};
#endif

#if 0
static uint8_t asr_ampdudensity2usec(uint8_t ampdudensity)
{
    switch (ampdudensity) {
    case IEEE80211_HT_MPDU_DENSITY_NONE:
        return 0;
        /* 1 microsecond is our granularity */
    case IEEE80211_HT_MPDU_DENSITY_0_25:
    case IEEE80211_HT_MPDU_DENSITY_0_5:
    case IEEE80211_HT_MPDU_DENSITY_1:
        return 1;
    case IEEE80211_HT_MPDU_DENSITY_2:
        return 2;
    case IEEE80211_HT_MPDU_DENSITY_4:
        return 4;
    case IEEE80211_HT_MPDU_DENSITY_8:
        return 8;
    case IEEE80211_HT_MPDU_DENSITY_16:
        return 16;
    default:
        return 0;
    }
}
#endif

static  bool use_pairwise_key(struct cfg80211_crypto_settings *crypto)
{
    if ((crypto->cipher_group ==  WLAN_CIPHER_SUITE_WEP40) ||
        (crypto->cipher_group ==  WLAN_CIPHER_SUITE_WEP104))
        return false;

    return true;
}

static  inline bool is_non_blocking_msg(int id) {
    return ((id == MM_TIM_UPDATE_REQ) || (id == ME_RC_SET_RATE_REQ) ||
            (id == ME_TRAFFIC_IND_REQ));
}

/**
 ******************************************************************************
 * @brief Allocate memory for a message
 *
 * This primitive allocates memory for a message that has to be sent. The memory
 * is allocated dynamically on the heap and the length of the variable parameter
 * structure has to be provided in order to allocate the correct size.
 *
 * Several additional parameters are provided which will be preset in the message
 * and which may be used internally to choose the kind of memory to allocate.
 *
 * The memory allocated will be automatically freed by the kernel, after the
 * pointer has been sent to ke_msg_send(). If the message is not sent, it must
 * be freed explicitly with ke_msg_free().
 *
 * Allocation failure is considered critical and should not happen.
 *
 * @param[in] id        Message identifier
 * @param[in] dest_id   Destination Task Identifier
 * @param[in] src_id    Source Task Identifier
 * @param[in] param_len Size of the message parameters to be allocated
 *
 * @return Pointer to the parameter member of the ke_msg. If the parameter
 *         structure is empty, the pointer will point to the end of the message
 *         and should not be used (except to retrieve the message pointer or to
 *         send the message)
 ******************************************************************************
 */
static  void *asr_msg_zalloc(uint16_t const id,
                                    uint16_t const dest_id,
                                    uint16_t const src_id,
                                    uint16_t const param_len)
{
    struct lmac_msg *msg;
    uint32_t tx_data_end_token = 0xAECDBFCA;

    msg = (struct lmac_msg *)asr_rtos_malloc(sizeof(struct lmac_msg) + param_len + SDIO_BLOCK_SIZE);
    if (msg == NULL) {
        dbg(D_ERR,D_UWIFI_CTRL,"%s:alloc asr_msg failed",__func__);
        return NULL;
    }

    msg->id = id;
    msg->dest_id = dest_id;
    msg->src_id = src_id;
    msg->param_len = param_len;
    uint32_t temp_len = ASR_ALIGN_BLKSZ_HI(sizeof(struct lmac_msg) + param_len + 4);
    memcpy((uint8_t*)msg + temp_len - 4, &tx_data_end_token, 4);
    return msg->param;
}

static void asr_msg_free(struct asr_hw *asr_hw, const void *msg_params)
{
    struct lmac_msg *msg = container_of((void *)msg_params,
                                        struct lmac_msg, param);
    /* Free the message */
    asr_rtos_free(msg);
    msg = NULL;
}
int asr_msg_push(struct ipc_host_env_tag *env, void *msg_buf, uint16_t len)
{
    int i;
    uint32_t *src, *dst;

    src = (uint32_t*)((struct asr_cmd *)msg_buf)->a2e_msg;
    dst = (uint32_t*)&(env->shared->msg_a2e_buf.msg);

    for (i=0; i<len; i+=4)
    {
        *dst++ = *src++;
    }
    env->msga2e_hostid = msg_buf;

    return (0);
}
static int asr_send_msg(struct asr_hw *asr_hw, const void *msg_params,
                         int reqcfm, uint16_t reqid, void *cfm)
{
    struct lmac_msg *msg;
    struct asr_cmd *cmd;
    bool nonblock;
    int ret;

    msg = container_of((void *)msg_params, struct lmac_msg, param);

    if (!asr_test_bit(ASR_DEV_STARTED, &asr_hw->phy_flags) &&
        reqid != MM_RESET_CFM && reqid != MM_VERSION_CFM &&
        reqid != MM_START_CFM && reqid != MM_SET_IDLE_CFM &&
        reqid != ME_CONFIG_CFM && reqid != MM_SET_PS_MODE_CFM &&
        reqid != ME_CHAN_CONFIG_CFM && reqid != MM_FW_SOFTVERSION_CFM &&
        reqid != MM_GET_INFO_CFM && reqid != MM_HIF_SDIO_INFO_IND && 
        reqid != MM_FW_MAC_ADDR_CFM && reqid != MM_SET_FW_MAC_ADDR_CFM)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s: bypassing (ASR_DEV_RESTARTING set) 0x%02x\n",__func__,reqid);
        asr_rtos_free(msg);
        msg = NULL;
        return -EBUSY;
    }else if (!asr_hw->ipc_env) {
        dbg(D_ERR,D_UWIFI_CTRL,"%s: bypassing (restart must have failed)\n", __func__);
        asr_rtos_free(msg);
        msg = NULL;
        return -EBUSY;
    }

    nonblock = is_non_blocking_msg(msg->id);
    if(nonblock)
        dbg(D_INF, D_UWIFI_CTRL, "%s reqid=%d,nonblock=%d\r\n", __func__,reqid,nonblock);

    cmd = asr_rtos_malloc(sizeof(struct asr_cmd));
    if(NULL == cmd)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s:alloc asr_cmd failed",__func__);
        asr_rtos_free(msg);
        msg = NULL;
        return -ENOMEM;
    }

    cmd->result  = -EINTR;
    cmd->id      = msg->id;
    cmd->reqid   = reqid;
    cmd->a2e_msg = msg;
    cmd->e2a_msg = cfm;

    if (nonblock||(!reqcfm))
        cmd->flags = ASR_CMD_FLAG_NONBLOCK;

    if (reqcfm)
        cmd->flags |= ASR_CMD_FLAG_REQ_CFM;

    //let last crashed not effect the next cmd msg transfer
    //asr_hw->cmd_mgr.state = ASR_CMD_MGR_STATE_INITED;
    ret = asr_hw->cmd_mgr.queue(&asr_hw->cmd_mgr, cmd);   //cmd_mgr_queue
    if(ret == -EBUSY) {
        asr_rtos_free(cmd);
        return ret;
    }

    if ((!nonblock)||(!reqcfm)) // block case or no_req cfm
        asr_rtos_free(cmd);
    else {
        ret = cmd->result;
        dbg(D_INF, D_UWIFI_CTRL, "%s non-block & req cfm case, req id = %d \r\n", __func__,cmd->reqid);
        //non-block & req cfm , free later in cmd_complete
    }

    return ret;
}

/******************************************************************************
 *    Control messages handling functions (SOFTMAC and  FULLMAC)
 *****************************************************************************/
int asr_send_reset(struct asr_hw *asr_hw)
{
    void *void_param;


    /* RESET REQ has no parameter */
    void_param = asr_msg_zalloc(MM_RESET_REQ, TASK_MM, DRV_TASK_ID, 0);
    if (!void_param)
        return -ENOMEM;
    return asr_send_msg(asr_hw, void_param, 1, MM_RESET_CFM, NULL);
}

int asr_send_start(struct asr_hw *asr_hw)
{
    struct mm_start_req *start_req_param;

    start_req_param = asr_msg_zalloc(MM_START_REQ, TASK_MM, DRV_TASK_ID,
                                      sizeof(struct mm_start_req));
    if (!start_req_param)
        return -ENOMEM;

    memcpy(&start_req_param->phy_cfg, &asr_hw->phy_config, sizeof(asr_hw->phy_config));
    start_req_param->uapsd_timeout = (uint32_t)asr_hw->mod_params->uapsd_timeout;
    start_req_param->lp_clk_accuracy = (uint16_t)asr_hw->mod_params->lp_clk_ppm;

    return asr_send_msg(asr_hw, start_req_param, 1, MM_START_CFM, NULL);
}

int asr_send_set_ps_options(uint8_t listen_bc_mc, uint16_t listen_interval)
{
    asr_module_params.listen_bcmc = listen_bc_mc;
    asr_module_params.listen_itv = listen_interval;

    return 0;
}

int asr_send_set_ps_mode(uint32_t ps_on)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    struct mm_set_ps_mode_req *set_ps_mode_req_param;

    if(asr_hw==NULL)
        return -1;

    asr_hw->ps_on = ps_on;
    set_ps_mode_req_param = asr_msg_zalloc(MM_SET_PS_MODE_REQ, TASK_MM, DRV_TASK_ID,
                                            sizeof(struct mm_set_ps_mode_req));
    if (!set_ps_mode_req_param)
        return -ENOMEM;

    if (ps_on)
    {
        set_ps_mode_req_param->new_state = PS_MODE_ON_DYN;
    }
    else
    {
        set_ps_mode_req_param->new_state = PS_MODE_OFF;
    }

    return asr_send_msg(asr_hw, set_ps_mode_req_param, 1, MM_SET_PS_MODE_CFM, NULL);
}

#ifdef CFG_SNIFFER_SUPPORT
int asr_send_set_idle(struct asr_hw *asr_hw, int idle)
{
    struct mm_set_idle_req *set_idle_req_param;

    set_idle_req_param = asr_msg_zalloc(MM_SET_IDLE_REQ, TASK_MM, DRV_TASK_ID,
                                         sizeof(struct mm_set_idle_req));
    if (!set_idle_req_param)
        return -ENOMEM;

    set_idle_req_param->hw_idle = idle;

    return asr_send_msg(asr_hw, set_idle_req_param, 1, MM_SET_IDLE_CFM, NULL);
}
#endif

#ifdef CFG_DBG
int asr_send_mem_read_req(struct asr_hw *asr_hw, uint32_t mem_addr)
{
    struct dbg_mem_read_req *mem_read_req;

    mem_read_req = asr_msg_zalloc(DBG_MEM_READ_REQ, TASK_DBG, DRV_TASK_ID,
                                    sizeof(struct dbg_mem_read_req));
    if (!mem_read_req)
        return -ENOMEM;
    mem_read_req->memaddr = mem_addr;

    return asr_send_msg(asr_hw, mem_read_req, 1, DBG_MEM_READ_CFM, NULL);
}
int asr_send_mem_write_req(struct asr_hw *asr_hw, uint32_t mem_addr, uint32_t mem_data)
{
    struct dbg_mem_write_req *mem_write_req;

    mem_write_req = asr_msg_zalloc(DBG_MEM_WRITE_REQ, TASK_DBG, DRV_TASK_ID,
                                    sizeof(struct dbg_mem_write_req));
    if (!mem_write_req)
        return -ENOMEM;
    mem_write_req->memaddr = mem_addr;
    mem_write_req->
memdata = mem_data;

    return asr_send_msg(asr_hw, mem_write_req, 1, DBG_MEM_WRITE_CFM, NULL);
}

int asr_send_mod_filter_req(struct asr_hw *asr_hw, uint32_t mod_filter)
{
    struct dbg_set_mod_filter_req *mod_filter_req;

    mod_filter_req = asr_msg_zalloc(DBG_SET_MOD_FILTER_REQ, TASK_DBG, DRV_TASK_ID,
                                    sizeof(struct dbg_set_mod_filter_req));
    if (!mod_filter_req)
        return -ENOMEM;
    mod_filter_req->mod_filter= mod_filter;

    return asr_send_msg(asr_hw, mod_filter_req, 1, DBG_SET_MOD_FILTER_CFM, NULL);
}

int asr_send_sev_filter_req(struct asr_hw *asr_hw, uint32_t sev_filter)
{
    struct dbg_set_sev_filter_req *sev_filter_req;

    sev_filter_req = asr_msg_zalloc(DBG_SET_SEV_FILTER_REQ, TASK_DBG, DRV_TASK_ID,
                                    sizeof(struct dbg_set_sev_filter_req));
    if (!sev_filter_req)
        return -ENOMEM;
    sev_filter_req->sev_filter = sev_filter;

    return asr_send_msg(asr_hw, sev_filter_req, 1, DBG_SET_SEV_FILTER_CFM, NULL);

}

int asr_send_get_sys_stat_req(struct asr_hw *asr_hw)
{
    void *void_param;

    void_param = asr_msg_zalloc(DBG_GET_SYS_STAT_REQ, TASK_DBG, DRV_TASK_ID, 0);
    if (!void_param)
        return -ENOMEM;

    return asr_send_msg(asr_hw, void_param, 1, DBG_GET_SYS_STAT_CFM, NULL);
}
int asr_send_set_host_log_req(struct asr_hw *asr_hw, dbg_host_log_switch_t log_switch)
{
    struct dbg_set_host_log_req *set_host_log_req;

    set_host_log_req = asr_msg_zalloc(DBG_SET_HOST_LOG_REQ, TASK_DBG, DRV_TASK_ID,
                                    sizeof(struct dbg_set_host_log_req));
    if (!set_host_log_req)
        return -ENOMEM;
    set_host_log_req->log_switch = log_switch;

    return asr_send_msg(asr_hw, set_host_log_req, 1, DBG_SET_HOST_LOG_CFM, NULL);
}
#endif
int asr_send_version_req(struct asr_hw *asr_hw, struct mm_version_cfm *cfm)
{
    void *void_param;


    /* VERSION REQ has no parameter */
    void_param = asr_msg_zalloc(MM_VERSION_REQ, TASK_MM, DRV_TASK_ID, 0);
    if (!void_param)
        return -ENOMEM;
    return asr_send_msg(asr_hw, void_param, 1, MM_VERSION_CFM, cfm);
}

int asr_send_fw_softversion_req(struct asr_hw *asr_hw,struct mm_fw_softversion_cfm* cfm)
{
    void* get_fw_param = NULL;
    get_fw_param = asr_msg_zalloc(MM_FW_SOFTVERSION_REQ,TASK_MM,DRV_TASK_ID,
        sizeof(struct mm_fw_softversion_cfm));
    if(!get_fw_param)
        return -ENOMEM;
    return asr_send_msg(asr_hw,get_fw_param,1,MM_FW_SOFTVERSION_CFM,cfm);
}

int asr_send_fw_macaddr_req(struct asr_hw *asr_hw, struct mm_fw_macaddr_cfm *cfm)
{
    void *msg = NULL;
    int ret = 0;

    /* VERSION REQ has no parameter */
    msg = asr_msg_zalloc(MM_FW_MAC_ADDR_REQ, TASK_MM, DRV_TASK_ID, 0);
    if (!msg)
        return -ENOMEM;

    return asr_send_msg(asr_hw, msg, 1, MM_FW_MAC_ADDR_CFM, cfm);
}

int asr_send_set_fw_macaddr_req(struct asr_hw *asr_hw, const uint8_t * macaddr)
{
    struct mm_set_fw_macaddr_req *req = NULL;
    struct mm_fw_macaddr_cfm cfm;

    /* Build the MM_CFG_RSSI_REQ message */
    req = asr_msg_zalloc(MM_SET_FW_MAC_ADDR_REQ, TASK_MM, DRV_TASK_ID, sizeof(struct mm_set_fw_macaddr_req));
    if (!req)
        return -ENOMEM;

    memcpy(req->mac, macaddr, MAC_ADDR_LEN);
    memset(&cfm, 0, sizeof(struct mm_fw_macaddr_cfm));

    /* Send the MM_CFG_RSSI_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, MM_SET_FW_MAC_ADDR_CFM, &cfm);
}

int asr_send_mm_get_info(struct asr_hw *asr_hw, struct mm_get_info_cfm *cfm)
{
    void *void_param;

    /* Build the ADD_IF_REQ message */
    void_param = asr_msg_zalloc(MM_GET_INFO_REQ, TASK_MM, DRV_TASK_ID, 0);
    if (!void_param)
        return -ENOMEM;

    /* Send the ADD_IF_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, void_param, 1, MM_GET_INFO_CFM, cfm);
}

int asr_send_add_if(struct asr_hw *asr_hw, const unsigned char *mac,
                     enum nl80211_iftype iftype, bool p2p, struct mm_add_if_cfm *cfm)
{
    struct mm_add_if_req *add_if_req_param;


    /* Build the ADD_IF_REQ message */
    add_if_req_param = asr_msg_zalloc(MM_ADD_IF_REQ, TASK_MM, DRV_TASK_ID,
                                       sizeof(struct mm_add_if_req));
    if (!add_if_req_param)
        return -ENOMEM;

    /* Set parameters for the ADD_IF_REQ message */
    memcpy(&(add_if_req_param->addr.array[0]), mac, 6);

    switch (iftype) {
#ifdef CFG_STATION_SUPPORT
    case NL80211_IFTYPE_STATION:
        add_if_req_param->type = MM_STA;
        break;
#endif
#ifdef CFG_SOFTAP_SUPPORT
    case NL80211_IFTYPE_AP:
        add_if_req_param->type = MM_AP;
        break;
#endif
#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
    case NL80211_IFTYPE_MONITOR:
        add_if_req_param->type = MM_MONITOR_MODE;
        break;
#endif
    default:
        return -1;
        break;
    }
    /* Send the ADD_IF_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, add_if_req_param, 1, MM_ADD_IF_CFM, cfm);
}

int asr_send_remove_if(struct asr_hw *asr_hw, uint8_t vif_index)
{
    struct mm_remove_if_req *remove_if_req;



    /* Build the MM_REMOVE_IF_REQ message */
    remove_if_req = asr_msg_zalloc(MM_REMOVE_IF_REQ, TASK_MM, DRV_TASK_ID,
                                    sizeof(struct mm_remove_if_req));
    if (!remove_if_req)
        return -ENOMEM;

    /* Set parameters for the MM_REMOVE_IF_REQ message */
    remove_if_req->inst_nbr = vif_index;

    /* Send the MM_REMOVE_IF_REQ message to LMAC FW */
    int ret = asr_send_msg(asr_hw, remove_if_req, 1, MM_REMOVE_IF_CFM, NULL);
    dbg(D_ERR, D_UWIFI_CTRL, "%s ret=%d\r\n", __func__,ret);
    return ret;
}


int asr_send_key_add(struct asr_hw *asr_hw, uint8_t vif_idx, uint8_t sta_idx, bool pairwise,
                      uint8_t *key, uint8_t key_len, uint8_t key_idx, uint8_t cipher_suite,
                      struct mm_key_add_cfm *cfm)
{
    struct mm_key_add_req *key_add_req;


    /* Build the MM_KEY_ADD_REQ message */
    key_add_req = asr_msg_zalloc(MM_KEY_ADD_REQ, TASK_MM, DRV_TASK_ID,
                                  sizeof(struct mm_key_add_req));
    if (!key_add_req)
        return -ENOMEM;

    /* Set parameters for the MM_KEY_ADD_REQ message */
    if (sta_idx != 0xFF) {
        /* Pairwise key */
        key_add_req->sta_idx = sta_idx;
    } else {
        /* Default key */
        key_add_req->sta_idx = sta_idx;
        key_add_req->key_idx = (uint8_t)key_idx; /* only useful for default keys */
    }
    key_add_req->pairwise = pairwise;
    key_add_req->inst_nbr = vif_idx;
    key_add_req->key.length = key_len;
    memcpy(&(key_add_req->key.array[0]), key, key_len);

    key_add_req->cipher_suite = cipher_suite;

    dbg(D_ERR,D_UWIFI_CTRL,"sta_idx:%d key_idx:%d inst_nbr:%d cipher:%d key_len:%d\n",
             key_add_req->sta_idx, key_add_req->key_idx, key_add_req->inst_nbr,
             key_add_req->cipher_suite, key_add_req->key.length);

    /* Send the MM_KEY_ADD_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, key_add_req, 1, MM_KEY_ADD_CFM, cfm);
}

int asr_send_key_del(struct asr_hw *asr_hw, uint8_t hw_key_idx)
{
    struct mm_key_del_req *key_del_req;

    /* Build the MM_KEY_DEL_REQ message */
    key_del_req = asr_msg_zalloc(MM_KEY_DEL_REQ, TASK_MM, DRV_TASK_ID,
                                  sizeof(struct mm_key_del_req));
    if (!key_del_req)
        return -ENOMEM;

    /* Set parameters for the MM_KEY_DEL_REQ message */
    key_del_req->hw_key_idx = hw_key_idx;

    /* Send the MM_KEY_DEL_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, key_del_req, 1, MM_KEY_DEL_CFM, NULL);
}

int asr_send_bcn_change(struct asr_hw *asr_hw, uint8_t vif_idx, uint32_t bcn_addr,
                         uint16_t bcn_len, uint16_t tim_oft, uint16_t tim_len, uint16_t *csa_oft)
{
    struct mm_bcn_change_req *req;



    /* Build the MM_BCN_CHANGE_REQ message */
    req = asr_msg_zalloc(MM_BCN_CHANGE_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_bcn_change_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_BCN_CHANGE_REQ message */
    req->bcn_ptr = bcn_addr;
    req->bcn_len = bcn_len;
    req->tim_oft = tim_oft;
    req->tim_len = tim_len;
    req->inst_nbr = vif_idx;

    if (csa_oft) {
        int i;
        for (i = 0; i < BCN_MAX_CSA_CPT; i++) {
            req->csa_oft[i] = csa_oft[i];
        }
    }

    /* Send the MM_BCN_CHANGE_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, MM_BCN_CHANGE_CFM, NULL);
}

void cfg80211_chandef_create(struct cfg80211_chan_def *chandef,
                 struct ieee80211_channel *chan,
                 enum nl80211_channel_type chan_type)
{
    chandef->chan = chan;
    chandef->center_freq2 = 0;

    switch (chan_type) {
    case NL80211_CHAN_NO_HT:
        chandef->width = NL80211_CHAN_WIDTH_20_NOHT;
        chandef->center_freq1 = chan->center_freq;
        break;
    case NL80211_CHAN_HT20:
        chandef->width = NL80211_CHAN_WIDTH_20;
        chandef->center_freq1 = chan->center_freq;
        break;
    case NL80211_CHAN_HT40PLUS:
        chandef->width = NL80211_CHAN_WIDTH_40;
        chandef->center_freq1 = chan->center_freq + 10;
        break;
    case NL80211_CHAN_HT40MINUS:
        chandef->width = NL80211_CHAN_WIDTH_40;
        chandef->center_freq1 = chan->center_freq - 10;
        break;
    default:
        break;
    }
}

#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME || defined CFG_ADD_API)
int asr_send_set_channel(struct asr_hw *asr_hw, struct ieee80211_channel *pst_chan, enum nl80211_channel_type chan_type)
{
    struct mm_set_channel_req  *set_chnl_par;
    struct cfg80211_chan_def    st_chandef;


    set_chnl_par = asr_msg_zalloc(MM_SET_CHANNEL_REQ, TASK_MM, DRV_TASK_ID,
                               sizeof(struct mm_set_channel_req));
    if (!set_chnl_par)
        return -ENOMEM;

    memset(&st_chandef, 0, sizeof(struct cfg80211_chan_def));

    /* Create channel definition structure */
    cfg80211_chandef_create(&st_chandef, pst_chan, chan_type);

#ifdef CONFIG_ASR595X
    set_chnl_par->chan.band = NL80211_BAND_2GHZ;
    set_chnl_par->chan.type = bw2chnl[st_chandef.width];;
    set_chnl_par->chan.prim20_freq = pst_chan->center_freq;
    set_chnl_par->chan.center1_freq = st_chandef.center_freq1;
    set_chnl_par->chan.center2_freq = st_chandef.center_freq2;
    set_chnl_par->index = 0;
    set_chnl_par->chan.tx_power = pst_chan->max_power;


    dbg(D_ERR, D_UWIFI_CTRL, "mac80211:     freq=%d(c1:%d - c2:%d)/width=%d - band=%d\n"
        "    hw(%d): prim20=%d(c1:%d - c2:%d)/ type=%d - band=%d\n",
        pst_chan->center_freq, st_chandef.center_freq1,
        st_chandef.center_freq2, st_chandef.width, NL80211_BAND_2GHZ,
        0, set_chnl_par->chan.prim20_freq, set_chnl_par->chan.center1_freq,
        set_chnl_par->chan.center2_freq, set_chnl_par->chan.type, set_chnl_par->chan.band);

#else

    set_chnl_par->band          = NL80211_BAND_2GHZ;
    set_chnl_par->type          = bw2chnl[st_chandef.width];
    set_chnl_par->prim20_freq   = pst_chan->center_freq;
    set_chnl_par->center1_freq  = st_chandef.center_freq1;
    set_chnl_par->center2_freq  = st_chandef.center_freq2;
    set_chnl_par->index         = 0;
    set_chnl_par->tx_power      = pst_chan->max_power;
#endif

    /* Send the MM_SET_CHANNEL_REQ REQ message to LMAC FW */
    return asr_send_msg(asr_hw, set_chnl_par, 1, MM_SET_CHANNEL_CFM, NULL);
}

int asr_send_set_filter(struct asr_hw *asr_hw, uint32_t filter)
{
    struct mm_set_filter_req *set_filter_req_param;
    struct asr_vif *asr_vif   = asr_hw->vif_table[asr_hw->monitor_vif_idx];
    uint32_t new_filter         = asr_vif->sniffer.rx_filter;

    /* Build the MM_SET_FILTER_REQ message */
    set_filter_req_param =
        asr_msg_zalloc(MM_SET_FILTER_REQ, TASK_MM, DRV_TASK_ID,
                        sizeof(struct mm_set_filter_req));
    if (!set_filter_req_param)
        return -ENOMEM;
    if (0 == filter)
        new_filter = 0x0;
    /* Set parameters for the MM_SET_FILTER_REQ message */
    if(filter & BIT(WLAN_RX_BEACON))
    {
        new_filter |= NXMAC_ACCEPT_BROADCAST_BIT;
        new_filter |= NXMAC_ACCEPT_ALL_BEACON_BIT;
        new_filter |= NXMAC_ACCEPT_OTHER_MGMT_FRAMES_BIT;
    }
    if(filter & BIT(WLAN_RX_PROBE_REQ))
    {
        new_filter |= NXMAC_ACCEPT_PROBE_REQ_BIT;
        new_filter |= NXMAC_ACCEPT_UNICAST_BIT;
        new_filter |= NXMAC_ACCEPT_OTHER_MGMT_FRAMES_BIT;
    }
    if(filter & BIT(WLAN_RX_PROBE_RES))
    {
        new_filter |= NXMAC_ACCEPT_UNICAST_BIT;
        new_filter |= NXMAC_ACCEPT_OTHER_MGMT_FRAMES_BIT;
    }
    if(filter & BIT(WLAN_RX_ACTION))
    {
        /* fw did not support action only */
        new_filter |= NXMAC_ACCEPT_OTHER_MGMT_FRAMES_BIT;
    }
    if(filter & BIT(WLAN_RX_MANAGEMENT))
    {
        new_filter |= NXMAC_ACCEPT_PROBE_REQ_BIT;
        new_filter |= NXMAC_ACCEPT_OTHER_MGMT_FRAMES_BIT;
    }
    if(filter & BIT(WLAN_RX_DATA))
    {
        new_filter |= NXMAC_ACCEPT_OTHER_DATA_FRAMES_BIT;
        new_filter |= NXMAC_ACCEPT_UNICAST_BIT;
        new_filter |= NXMAC_ACCEPT_MULTICAST_BIT;
        new_filter |= NXMAC_ACCEPT_OTHER_BSSID_BIT;
    }
    if(filter & BIT(WLAN_RX_MCAST_DATA))
    {
        new_filter |= NXMAC_ACCEPT_OTHER_DATA_FRAMES_BIT;
        new_filter |= NXMAC_ACCEPT_MULTICAST_BIT;
        new_filter |= NXMAC_ACCEPT_OTHER_BSSID_BIT;
    }
    if(filter & BIT(WLAN_RX_MONITOR_DEFAULT))
    {
        new_filter |= NXMAC_DONT_DECRYPT_BIT;
        new_filter |= NXMAC_ACCEPT_BROADCAST_BIT;
        new_filter |= NXMAC_ACCEPT_OTHER_BSSID_BIT;
        new_filter |= NXMAC_ACCEPT_MY_UNICAST_BIT;
        new_filter |= NXMAC_ACCEPT_PROBE_RESP_BIT;
        new_filter |= NXMAC_ACCEPT_PROBE_REQ_BIT;
        new_filter |= NXMAC_ACCEPT_ALL_BEACON_BIT;
        new_filter |= NXMAC_ACCEPT_DATA_BIT;
        new_filter |= NXMAC_ACCEPT_Q_DATA_BIT;
    }
    if (filter & BIT(WLAN_RX_ALL))
        new_filter |= ASR_MAC80211_FILTER_MONITOR;
    if (filter & BIT(WLAN_RX_SMART_CONFIG)) {
        new_filter |= NXMAC_DONT_DECRYPT_BIT;
        new_filter |= NXMAC_ACCEPT_BROADCAST_BIT;
        new_filter |= NXMAC_ACCEPT_MULTICAST_BIT;
        new_filter |= NXMAC_ACCEPT_OTHER_BSSID_BIT;
        new_filter |= NXMAC_ACCEPT_UNICAST_BIT;
        new_filter |= NXMAC_ACCEPT_DATA_BIT;
        new_filter |= NXMAC_ACCEPT_CFWO_DATA_BIT;
        new_filter |= NXMAC_ACCEPT_Q_DATA_BIT;
        new_filter |= NXMAC_ACCEPT_QCFWO_DATA_BIT;
        new_filter |= NXMAC_ACCEPT_QO_S_NULL_BIT;
        new_filter |= NXMAC_ACCEPT_OTHER_DATA_FRAMES_BIT;
    }
    if (filter & BIT(WLAN_RX_SMART_CONFIG_MC)) {
        new_filter |= NXMAC_DONT_DECRYPT_BIT;
        new_filter |= NXMAC_ACCEPT_BROADCAST_BIT;
        new_filter |= NXMAC_ACCEPT_MY_UNICAST_BIT;
        //new_filter |= NXMAC_ACCEPT_ALL_BEACON_BIT;
        new_filter |= NXMAC_ACCEPT_MULTICAST_BIT;
        new_filter |= NXMAC_ACCEPT_OTHER_BSSID_BIT;
        new_filter |= NXMAC_ACCEPT_UNICAST_BIT;
        //new_filter |= NXMAC_ACCEPT_DATA_BIT;
        //new_filter |= NXMAC_ACCEPT_CFWO_DATA_BIT;
        new_filter |= NXMAC_ACCEPT_Q_DATA_BIT;
        //new_filter |= NXMAC_ACCEPT_QCFWO_DATA_BIT;
        //new_filter |= NXMAC_ACCEPT_QO_S_NULL_BIT;
        new_filter |= NXMAC_ACCEPT_OTHER_DATA_FRAMES_BIT;
    }
    if (new_filter != asr_vif->sniffer.rx_filter)
    {
        /* Now copy all the flags into the message parameter */
        asr_vif->sniffer.rx_filter     = new_filter;
        set_filter_req_param->filter    = new_filter;
        /* Send the MM_SET_FILTER_REQ message to LMAC FW */
        return asr_send_msg(asr_hw, set_filter_req_param, 1, MM_SET_FILTER_CFM, NULL);
    }

    return 0;
}
#endif

int asr_send_roc(struct asr_hw *asr_hw, struct asr_vif *vif,
                  struct ieee80211_channel *chan, unsigned  int duration)
{
    struct mm_remain_on_channel_req *req;
    struct cfg80211_chan_def chandef;



    /* Create channel definition structure */
    cfg80211_chandef_create(&chandef, chan, NL80211_CHAN_NO_HT);

    /* Build the MM_REMAIN_ON_CHANNEL_REQ message */
    req = asr_msg_zalloc(MM_REMAIN_ON_CHANNEL_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_remain_on_channel_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_REMAIN_ON_CHANNEL_REQ message */
    req->op_code      = MM_ROC_OP_START;
    req->vif_index    = vif->vif_index;
    req->duration_ms  = duration;

#ifdef CONFIG_ASR595X
    req->chan.band = chan->band;
    req->chan.type = bw2chnl[chandef.width];
    req->chan.prim20_freq = chan->center_freq;
    req->chan.center1_freq = chandef.center_freq1;
    req->chan.center2_freq = chandef.center_freq2;
    req->chan.tx_power = chan->max_power;
#else
    req->band         = chan->band;
    req->type         = bw2chnl[chandef.width];
    req->prim20_freq  = chan->center_freq;
    req->center1_freq = chandef.center_freq1;
    req->center2_freq = chandef.center_freq2;
    req->tx_power     = chan->max_power;
#endif

    /* Send the MM_REMAIN_ON_CHANNEL_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, MM_REMAIN_ON_CHANNEL_CFM, NULL);
}

int asr_send_cancel_roc(struct asr_hw *asr_hw)
{
    struct mm_remain_on_channel_req *req;



    /* Build the MM_REMAIN_ON_CHANNEL_REQ message */
    req = asr_msg_zalloc(MM_REMAIN_ON_CHANNEL_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_remain_on_channel_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_REMAIN_ON_CHANNEL_REQ message */
    req->op_code = MM_ROC_OP_CANCEL;

    /* Send the MM_REMAIN_ON_CHANNEL_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 0, 0, NULL);
}

int asr_send_set_power(struct asr_hw *asr_hw, uint8_t vif_idx, int8_t pwr,
                        struct mm_set_power_cfm *cfm)
{
    struct mm_set_power_req *req;



    /* Build the MM_SET_POWER_REQ message */
    req = asr_msg_zalloc(MM_SET_POWER_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_set_power_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_SET_POWER_REQ message */
    req->inst_nbr = vif_idx;
    req->power = pwr;

    /* Send the MM_SET_POWER_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, MM_SET_POWER_CFM, cfm);
}

int asr_send_set_edca(struct asr_hw *asr_hw, uint8_t hw_queue, uint32_t param,
                       bool uapsd, uint8_t inst_nbr)
{
    struct mm_set_edca_req *set_edca_req;



    /* Build the MM_SET_EDCA_REQ message */
    set_edca_req = asr_msg_zalloc(MM_SET_EDCA_REQ, TASK_MM, DRV_TASK_ID,
                                   sizeof(struct mm_set_edca_req));
    if (!set_edca_req)
        return -ENOMEM;

    /* Set parameters for the MM_SET_EDCA_REQ message */
    set_edca_req->ac_param = param;
    set_edca_req->uapsd = uapsd;
    set_edca_req->hw_queue = hw_queue;
    set_edca_req->inst_nbr = inst_nbr;

    /* Send the MM_SET_EDCA_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, set_edca_req, 1, MM_SET_EDCA_CFM, NULL);
}

/******************************************************************************
 *    Control messages handling functions (FULLMAC only)
 *****************************************************************************/
int asr_send_me_config_req(struct asr_hw *asr_hw)
{
    struct me_config_req *req;
    struct wiphy *wiphy = asr_hw->wiphy;
    struct ieee80211_sta_ht_cap *ht_cap =
                            &wiphy->bands[IEEE80211_BAND_2GHZ]->ht_cap;

#ifdef CONFIG_ASR595X
    struct ieee80211_sta_he_cap const *he_cap;
#endif

    uint8_t *ht_mcs = (uint8_t *)&ht_cap->mcs;
    int i;



    /* Build the ME_CONFIG_REQ message */
    req = asr_msg_zalloc(ME_CONFIG_REQ, TASK_ME, DRV_TASK_ID,
                                   sizeof(struct me_config_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_CONFIG_REQ message */


    req->ht_supp = ht_cap->ht_supported;
    req->vht_supp = false;
    req->ht_cap.ht_capa_info = cpu_to_le16(ht_cap->cap);
    req->ht_cap.a_mpdu_param = ht_cap->ampdu_factor |
                                     (ht_cap->ampdu_density <<
                                         IEEE80211_HT_AMPDU_PARM_DENSITY_SHIFT);
    for (i = 0; i < sizeof(ht_cap->mcs); i++)
        req->ht_cap.mcs_rate[i] = ht_mcs[i];
    req->ht_cap.ht_extended_capa = 0;
    req->ht_cap.tx_beamforming_capa = 0;
    req->ht_cap.asel_capa = 0;

    req->vht_cap.vht_capa_info = 0;
    req->vht_cap.rx_highest = 0;
    req->vht_cap.rx_mcs_map = 0;
    req->vht_cap.tx_highest = 0;
    req->vht_cap.tx_mcs_map = 0;

#ifdef CONFIG_ASR595X
    if (wiphy->bands[NL80211_BAND_2GHZ]->iftype_data != NULL) {
        he_cap = &wiphy->bands[NL80211_BAND_2GHZ]->iftype_data->he_cap;

        req->he_supp = he_cap->has_he;
        for (i = 0; i < ARRAY_SIZE(he_cap->he_cap_elem.mac_cap_info); i++) {
            req->he_cap.mac_cap_info[i] = he_cap->he_cap_elem.mac_cap_info[i];
        }
        for (i = 0; i < ARRAY_SIZE(he_cap->he_cap_elem.phy_cap_info); i++) {
            req->he_cap.phy_cap_info[i] = he_cap->he_cap_elem.phy_cap_info[i];
        }
        req->he_cap.mcs_supp.rx_mcs_80 = cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_80);
        req->he_cap.mcs_supp.tx_mcs_80 = cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_80);
        req->he_cap.mcs_supp.rx_mcs_160 = cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_160);
        req->he_cap.mcs_supp.tx_mcs_160 = cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_160);
        req->he_cap.mcs_supp.rx_mcs_80p80 = cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_80p80);
        req->he_cap.mcs_supp.tx_mcs_80p80 = cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_80p80);
        for (i = 0; i < MAC_HE_PPE_THRES_MAX_LEN; i++) {
            req->he_cap.ppe_thres[i] = he_cap->ppe_thres[i];
        }
        req->he_ul_on = asr_hw->mod_params->he_ul_on;
    }
    if (asr_hw->mod_params->use_80)
        req->phy_bw_max = PHY_CHNL_BW_80;
    else if (asr_hw->mod_params->use_2040)
        req->phy_bw_max = PHY_CHNL_BW_40;
    else
        req->phy_bw_max = PHY_CHNL_BW_20;
#endif

    req->ps_on = asr_hw->mod_params->ps_on;
    req->tx_lft = asr_hw->mod_params->tx_lft;

    /* Send the ME_CONFIG_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, ME_CONFIG_CFM, NULL);
}

int asr_send_me_chan_config_req(struct asr_hw *asr_hw)
{
    struct me_chan_config_req *req;
    struct wiphy *wiphy = asr_hw->wiphy;
    int i;



    /* Build the ME_CHAN_CONFIG_REQ message */
    req = asr_msg_zalloc(ME_CHAN_CONFIG_REQ, TASK_ME, DRV_TASK_ID,
                                            sizeof(struct me_chan_config_req));
    if (!req)
        return -ENOMEM;

    req->chan2G4_cnt=  0;
    if (wiphy->bands[IEEE80211_BAND_2GHZ] != NULL) {
        struct ieee80211_supported_band *b = wiphy->bands[IEEE80211_BAND_2GHZ];
        for (i = 0; i < b->n_channels; i++) {
            req->chan2G4[req->chan2G4_cnt].flags = 0;
            if (b->channels[i].flags & IEEE80211_CHAN_DISABLED)
                req->chan2G4[req->chan2G4_cnt].flags |= SCAN_DISABLED_BIT;
            req->chan2G4[req->chan2G4_cnt].flags |= passive_scan_flag(b->channels[i].flags);
            req->chan2G4[req->chan2G4_cnt].band = IEEE80211_BAND_2GHZ;
            req->chan2G4[req->chan2G4_cnt].freq = b->channels[i].center_freq;
            req->chan2G4[req->chan2G4_cnt].tx_power = b->channels[i].max_power;
            req->chan2G4_cnt++;
            if (req->chan2G4_cnt == SCAN_CHANNEL_2G4)
                break;
        }
    }

    req->chan5G_cnt = 0;

    return asr_send_msg(asr_hw, req, 1, ME_CHAN_CONFIG_CFM, NULL);
}

int asr_send_me_set_control_port_req(struct asr_hw *asr_hw, bool opened, uint8_t sta_idx)
{
    struct me_set_control_port_req *req;

    /* Build the ME_SET_CONTROL_PORT_REQ message */
    req = asr_msg_zalloc(ME_SET_CONTROL_PORT_REQ, TASK_ME, DRV_TASK_ID,
                                   sizeof(struct me_set_control_port_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_SET_CONTROL_PORT_REQ message */
    req->sta_idx = sta_idx;
    req->control_port_open = opened;

    /* Send the ME_SET_CONTROL_PORT_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, ME_SET_CONTROL_PORT_CFM, NULL);
}

int asr_send_me_sta_add(struct asr_hw *asr_hw, struct station_parameters *params,
                         const uint8_t *mac, uint8_t inst_nbr, struct me_sta_add_cfm *cfm)
{
    struct me_sta_add_req *req;
    uint8_t *ht_mcs = (uint8_t *)&params->ht_capa->mcs;
    int i;

    /* Build the MM_STA_ADD_REQ message */
    req = asr_msg_zalloc(ME_STA_ADD_REQ, TASK_ME, DRV_TASK_ID,
                                  sizeof(struct me_sta_add_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_STA_ADD_REQ message */
    memcpy(&(req->mac_addr.array[0]), mac, ETH_ALEN);

    req->rate_set.length = params->supported_rates_len;
    for (i = 0; i < params->supported_rates_len; i++)
        req->rate_set.array[i] = params->supported_rates[i];

    req->flags = 0;
    if (params->ht_capa) {
        const struct ieee80211_ht_cap *ht_capa = params->ht_capa;

        req->flags |= STA_HT_CAPA;
        req->ht_cap.ht_capa_info = cpu_to_le16(ht_capa->cap_info);
        req->ht_cap.a_mpdu_param = ht_capa->ampdu_params_info;
        for (i = 0; i < sizeof(ht_capa->mcs); i++)
            req->ht_cap.mcs_rate[i] = ht_mcs[i];
        req->ht_cap.ht_extended_capa = cpu_to_le16(ht_capa->extended_ht_cap_info);
        req->ht_cap.tx_beamforming_capa = cpu_to_le32(ht_capa->tx_BF_cap_info);
        req->ht_cap.asel_capa = ht_capa->antenna_selection_info;
    }

#ifdef CONFIG_ASR595X
    if (params->he_capa) {
        const struct ieee80211_he_cap_elem *he_capa = params->he_capa;
        const struct ieee80211_he_mcs_nss_supp *mcs_nss_supp = &(params->he_capa->he_mcs_nss_supp);

        req->flags |= STA_HE_CAPA;
        for (i = 0; i < sizeof(he_capa->mac_cap_info); i++)
            req->he_cap.mac_cap_info[i] = he_capa->mac_cap_info[i];
        for (i = 0; i < sizeof(he_capa->phy_cap_info); i++)
            req->he_cap.phy_cap_info[i] = he_capa->phy_cap_info[i];
        req->he_cap.mcs_supp.rx_mcs_80 = mcs_nss_supp->rx_mcs_80;
        req->he_cap.mcs_supp.tx_mcs_80 = mcs_nss_supp->tx_mcs_80;

        /* not support above 80MHz */
        //req->he_cap.mcs_supp.rx_mcs_160 = mcs_nss_supp->rx_mcs_160;
        //req->he_cap.mcs_supp.tx_mcs_160 = mcs_nss_supp->tx_mcs_160;
        //req->he_cap.mcs_supp.rx_mcs_80p80 = mcs_nss_supp->rx_mcs_80p80;
        //req->he_cap.mcs_supp.tx_mcs_80p80 = mcs_nss_supp->tx_mcs_80p80;
    }
#endif

    if (params->sta_flags_set & BIT(NL80211_STA_FLAG_WME))
        req->flags |= STA_QOS_CAPA;

    if (params->sta_flags_set & BIT(NL80211_STA_FLAG_MFP))
        req->flags |= STA_MFP_CAPA;

    if (params->opmode_notif_used) {
        req->flags |= STA_OPMOD_NOTIF;
        req->opmode = params->opmode_notif;
    }

    req->aid = cpu_to_le16(params->aid);
    req->uapsd_queues = params->uapsd_queues;
    req->max_sp_len = params->max_sp * 2;
    req->vif_idx = inst_nbr;

    /* Send the ME_STA_ADD_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, ME_STA_ADD_CFM, cfm);
}

int asr_send_me_sta_del(struct asr_hw *asr_hw, uint8_t sta_idx)
{
    struct me_sta_del_req *req;

    /* Build the MM_STA_DEL_REQ message */
    req = asr_msg_zalloc(ME_STA_DEL_REQ, TASK_ME, DRV_TASK_ID,
                          sizeof(struct me_sta_del_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_STA_DEL_REQ message */
    req->sta_idx = sta_idx;

    /* Send the ME_STA_DEL_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, ME_STA_DEL_CFM, NULL);
}

int asr_send_me_traffic_ind(struct asr_hw *asr_hw, uint8_t sta_idx, bool uapsd, uint8_t tx_status)
{
    struct me_traffic_ind_req *req;

    #if 1//def CONFIG_ASR595X

    /* Build the ME_UTRAFFIC_IND_REQ message */
    req = asr_msg_zalloc(ME_TRAFFIC_IND_REQ, TASK_ME, DRV_TASK_ID,
                          sizeof(struct me_traffic_ind_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_TRAFFIC_IND_REQ message */
    req->sta_idx = sta_idx;
    req->tx_avail = tx_status;
    req->uapsd = uapsd;

    /* Send the ME_TRAFFIC_IND_REQ to UMAC FW */
    return asr_send_msg(asr_hw, req, 1, ME_TRAFFIC_IND_CFM, NULL);
    #else

    // fw not support except 595x.
    return 0;
    #endif


}

int asr_send_me_rc_stats(struct asr_hw *asr_hw,
                          uint8_t sta_idx,
                          struct me_rc_stats_cfm *cfm)
{
    struct me_rc_stats_req *req;



    /* Build the ME_RC_STATS_REQ message */
    req = asr_msg_zalloc(ME_RC_STATS_REQ, TASK_ME, DRV_TASK_ID,
                                  sizeof(struct me_rc_stats_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_RC_STATS_REQ message */
    req->sta_idx = sta_idx;

    /* Send the ME_RC_STATS_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, ME_RC_STATS_CFM, cfm);
}

int asr_send_me_rc_set_rate(struct asr_hw *asr_hw,
                             uint8_t sta_idx,
                             uint16_t rate_cfg)
{
    struct me_rc_set_rate_req *req;



    /* Build the ME_RC_SET_RATE_REQ message */
    req = asr_msg_zalloc(ME_RC_SET_RATE_REQ, TASK_ME, DRV_TASK_ID,
                          sizeof(struct me_rc_set_rate_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_RC_SET_RATE_REQ message */
    req->sta_idx = sta_idx;
    req->fixed_rate_cfg = rate_cfg;

    /* Send the ME_RC_SET_RATE_REQ message to FW */
    return asr_send_msg(asr_hw, req, 0, 0, NULL);
}

int asr_send_me_host_dbg_cmd(struct asr_hw *asr_hw, unsigned int host_dbg_cmd)
{
    struct me_host_dbg_cmd_req *req = NULL;

    /* Build the ME_RC_SET_RATE_REQ message */
    req = asr_msg_zalloc(ME_HOST_DBG_CMD_REQ, TASK_ME, DRV_TASK_ID, sizeof(struct me_host_dbg_cmd_req));

    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_HOST_DBG_CMD_REQ message */
    req->host_dbg_cmd = host_dbg_cmd;

    /* Send the ME_HOST_DBG_CMD_REQ message to FW */
    return asr_send_msg(asr_hw, req, 0, 0, NULL);
}

#ifdef CONFIG_SME
int asr_send_sm_auth_req(struct asr_vif *asr_vif, struct cfg80211_auth_request *auth_req, struct sm_auth_cfm *cfm)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;
    struct sm_auth_req *req = NULL;
    int i;
    uint32_t flags = 0;

    if (!asr_vif || !asr_hw) {
        return -EBUSY;
    }

    /* Build the SM_CONNECT_REQ message */
    req = asr_msg_zalloc(SM_AUTH_REQ, TASK_SM, DRV_TASK_ID,
                                   sizeof(struct sm_auth_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the SM_AUTH_REQ message */
    // ssid / bssid/ channel.channel
    if (!auth_req->bssid[0] &&  !auth_req->bssid[1] &&  !auth_req->bssid[2]
        &&  !auth_req->bssid[3] &&  !auth_req->bssid[4] &&  !auth_req->bssid[5]){
        req->bssid = mac_addr_bcst;
    } else {
        memcpy(&req->bssid, auth_req->bssid, ETH_ALEN);
    }

    req->chan.band = auth_req->channel.band;
    req->chan.freq = auth_req->channel.center_freq;
    req->chan.flags = passive_scan_flag(auth_req->channel.flags);

    for (i = 0; i < auth_req->ssid_len; i++)
        req->ssid.array[i] = auth_req->ssid[i];
    req->ssid.length = auth_req->ssid_len;

    if (auth_req->crypto.n_ciphers_pairwise && ((auth_req->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_WEP40)
                             || (auth_req->crypto.ciphers_pairwise[0] ==
                             WLAN_CIPHER_SUITE_TKIP)
                             || (auth_req->crypto.ciphers_pairwise[0] ==
                             WLAN_CIPHER_SUITE_WEP104)))
        flags |= DISABLE_HT;

    req->flags = flags;

    asr_vif->sta.auth_type = auth_req->auth_type;

    /// Authentication type
    /* Set auth_type */
    if (auth_req->auth_type == NL80211_AUTHTYPE_AUTOMATIC) {
        req->auth_type = NL80211_AUTHTYPE_OPEN_SYSTEM;
    } else
    #if NX_SAE
    if (auth_req->auth_type == NL80211_AUTHTYPE_SAE) {
        req->auth_type = MAC_AUTH_ALGO_SAE;
    } else
    #endif
    {
        req->auth_type = auth_req->auth_type;
    }

    /// VIF index
    req->vif_idx = asr_vif->vif_index;

    dbg(D_ERR, D_UWIFI_CTRL, "  * ssid=%s,(band=%d  freq=%d flags=%d),flags=0x%x,auth_type=%d,vif_idx=%d  \n",
        req->ssid.array, req->chan.band, req->chan.freq, req->chan.flags, req->flags, req->auth_type, req->vif_idx);
    dbg(D_ERR, D_UWIFI_CTRL, "  * bssid=%02X:%02X:%02X:%02X:%02X:%02X\n"
        ,auth_req->bssid[0],auth_req->bssid[1],auth_req->bssid[2]
        ,auth_req->bssid[3],auth_req->bssid[4],auth_req->bssid[5]);

    if (auth_req->ie_len)
        memcpy(req->ie_buf, auth_req->ie, auth_req->ie_len);

    req->ie_len = auth_req->ie_len;
    dbg(D_ERR, D_UWIFI_CTRL, "  * ch :ie_len=%d,total_len=%u  \n", req->ie_len, sizeof(struct sm_auth_req));

#if NX_SAE
    // Non-IE data to use with SAE or %NULL.
    if (auth_req->sae_data_len > sizeof(req->sae_data)) {
        // for now max sae data len =512, which is enough for ecc group19.
        dbg(D_ERR, D_UWIFI_CTRL, "  * sae data len out of range!!!(%u > %d)  \n",
            (unsigned int)auth_req->sae_data_len, req->sae_data_len);

        asr_msg_free(asr_hw, req);
        return -EINVAL;

    } else {
        if (auth_req->sae_data_len)
            memcpy(req->sae_data, auth_req->sae_data, auth_req->sae_data_len);
        req->sae_data_len = auth_req->sae_data_len;
        dbg(D_ERR, D_UWIFI_CTRL, "  * ch :sae_len=%d,total_len=%u  \n", req->sae_data_len, sizeof(struct sm_auth_req));
    }
#endif

    asr_set_bit(ASR_DEV_STA_AUTH, &asr_vif->dev_flags);

    /* Send the SM_AUTH_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, SM_AUTH_CFM, cfm);
}

int asr_send_sm_assoc_req(struct asr_vif *asr_vif, struct cfg80211_assoc_request *assoc_req, struct sm_assoc_cfm *cfm)
{
    struct sm_assoc_req *req = NULL;
    // int i;
    u32 flags = 0;
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    //ASR_DBG(ASR_FN_ENTRY_STR);

    if (!asr_vif || !asr_hw) {
        return -EBUSY;
    }

    /* Build the SM_CONNECT_REQ message */
    req = asr_msg_zalloc(SM_ASSOC_REQ, TASK_SM, DRV_TASK_ID,
                                   sizeof(struct sm_assoc_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the SM_ASSOC_REQ message */
    /// BSSID to assoc to (if not specified, set this field to WILDCARD BSSID)
    //if (assoc_req->bss->bssid)
    memcpy(&req->bssid, assoc_req->bssid, ETH_ALEN);
    //else
    //      req->bssid = mac_addr_bcst;

    /// Listen interval to be used for this connection
    req->listen_interval = asr_module_params.listen_itv;

    /// Flag indicating if the we have to wait for the BC/MC traffic after beacon or not
    req->dont_wait_bcmc = !asr_module_params.listen_bcmc;

    if (assoc_req->crypto.n_ciphers_pairwise && ((assoc_req->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_WEP40)
                             || (assoc_req->crypto.ciphers_pairwise[0] ==
                             WLAN_CIPHER_SUITE_TKIP)
                             || (assoc_req->crypto.ciphers_pairwise[0] ==
                             WLAN_CIPHER_SUITE_WEP104)))
        flags |= DISABLE_HT;

    if (assoc_req->crypto.control_port)
        flags |= CONTROL_PORT_HOST;

    //if (assoc_req->crypto.control_port_no_encrypt) //temp mask to prevent tx fourth epol after send add key cmd in fw.
    flags |= CONTROL_PORT_NO_ENC;

    if (use_pairwise_key(&assoc_req->crypto))
        flags |= WPA_WPA2_IN_USE;

    if (assoc_req->use_mfp)
        flags |= MFP_IN_USE;

    /// Control port Ethertype
    if (assoc_req->crypto.control_port_ethertype)
        req->ctrl_port_ethertype = assoc_req->crypto.control_port_ethertype;
    else
        req->ctrl_port_ethertype = ETH_P_PAE;

    /// associate flags
    req->flags = flags;

    /// UAPSD queues (bit0: VO, bit1: VI, bit2: BE, bit3: BK)
    req->uapsd_queues = asr_module_params.uapsd_queues;

    //Use management frame protection (IEEE 802.11w) in this association
    req->use_mfp = assoc_req->use_mfp;

    /// VIF index
    req->vif_idx = asr_vif->vif_index;

    /// Extra IEs to add to (Re)Association Request frame or %NULL
    if ((assoc_req->ie_len > sizeof(req->ie_buf))) {
        asr_msg_free(asr_hw, req);
        return -EINVAL;
    }

    if (assoc_req->ie_len)
        memcpy(req->ie_buf, assoc_req->ie, assoc_req->ie_len);
    req->ie_len = assoc_req->ie_len;

    /* Send the SM_ASSOC_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, SM_ASSOC_CFM, cfm);
}
#endif

int asr_send_sm_connect_req(struct asr_vif *asr_vif,
                             struct cfg80211_connect_params *sme,
                             struct sm_connect_cfm *cfm)
{
    struct sm_connect_req *req;
    int i;
    uint32_t flags = 0;
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    dbg(D_ERR, D_UWIFI_CTRL, "%s\r\n", __func__);
    /* Build the SM_CONNECT_REQ message */
    req = asr_msg_zalloc(SM_CONNECT_REQ, TASK_SM, DRV_TASK_ID,
                                   sizeof(struct sm_connect_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the SM_CONNECT_REQ message */
    if (sme->crypto.n_ciphers_pairwise &&
        ((sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_WEP40) ||
         (sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_TKIP) ||
         (sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_WEP104)))
        flags |= DISABLE_HT;

    if (sme->crypto.control_port)
        flags |= CONTROL_PORT_HOST;

    if (sme->crypto.control_port_no_encrypt)
        flags |= CONTROL_PORT_NO_ENC;

    if (use_pairwise_key(&sme->crypto))
        flags |= WPA_WPA2_IN_USE;

    if (sme->mfp != NL80211_MFP_NO)
        flags |= MFP_IN_USE;

    req->ctrl_port_ethertype = sme->crypto.control_port_ethertype;

    if (!sme->bssid[0] && !sme->bssid[1] && !sme->bssid[2]
        && !sme->bssid[3] && !sme->bssid[4] && !sme->bssid[5]){
        req->bssid = mac_addr_bcst;
    } else {
        memcpy(&req->bssid, sme->bssid, ETH_ALEN);
    }
    req->vif_idx = asr_vif->vif_index;
    if (sme->channel) {
        req->chan.band = sme->channel->band;
        req->chan.freq = sme->channel->center_freq;
        req->chan.flags = passive_scan_flag(sme->channel->flags);
    } else {
        req->chan.freq = (uint16_t)-1;
    }
    for (i = 0; i < sme->ssid_len; i++)
        req->ssid.array[i] = sme->ssid[i];
    req->ssid.length = sme->ssid_len;
    req->flags = flags;
    if (sme->ie_len > sizeof(req->ie_buf)){
        asr_msg_free(asr_hw, req);
        return -EINVAL;
    }

    if (sme->ie_len)
        memcpy(req->ie_buf, sme->ie, sme->ie_len);
    req->ie_len = sme->ie_len;
    req->listen_interval = asr_module_params.listen_itv;
    req->dont_wait_bcmc = !asr_module_params.listen_bcmc;

    /* Set auth_type */
    if (sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC)
        req->auth_type = NL80211_AUTHTYPE_OPEN_SYSTEM;
    else
        req->auth_type = sme->auth_type;

    /* Set UAPSD queues */
    req->uapsd_queues = asr_module_params.uapsd_queues;

    /* Send the SM_CONNECT_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, SM_CONNECT_CFM, cfm);
}

int asr_send_sm_disconnect_req(struct asr_hw *asr_hw,
                                struct asr_vif *asr_vif,
                                uint16_t reason)
{
    struct sm_disconnect_req *req;
    int ret = 0;
    uint32_t timeout = 300;

    if (!asr_vif || !asr_hw) {
        return -EBUSY;
    }

    if (asr_test_bit(ASR_DEV_CLOSEING, &asr_vif->dev_flags)
        || !asr_test_bit(ASR_DEV_STARTED, &asr_hw->phy_flags)) {
        return -EBUSY;
    }

    asr_set_bit(ASR_DEV_STA_DISCONNECTING, &asr_vif->dev_flags);

    while (asr_test_bit(ASR_DEV_SCANING, &asr_vif->dev_flags) && timeout--) {
        asr_rtos_delay_milliseconds(10);
    }

    if (asr_test_bit(ASR_DEV_SCANING, &asr_vif->dev_flags)) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s: warnning disconnect in scan\n", __func__);
    }

    /* Build the SM_DISCONNECT_REQ message */
    req = asr_msg_zalloc(SM_DISCONNECT_REQ, TASK_SM, DRV_TASK_ID,
                                   sizeof(struct sm_disconnect_req));
    if (!req) {
        asr_clear_bit(ASR_DEV_STA_DISCONNECTING, &asr_vif->dev_flags);
        return -ENOMEM;
    }

    /* Set parameters for the SM_DISCONNECT_REQ message */
    req->reason_code = reason;
    req->vif_idx = asr_vif->vif_index;

    /* Send the SM_DISCONNECT_REQ message to LMAC FW */
    //return asr_send_msg(asr_hw, req, 1, SM_DISCONNECT_IND, NULL);
    ret = asr_send_msg(asr_hw, req, 1, SM_DISCONNECT_CFM, NULL);
    if (ret || !asr_test_bit(ASR_DEV_STA_CONNECTED, &asr_vif->dev_flags)) {
        asr_clear_bit(ASR_DEV_STA_DISCONNECTING, &asr_vif->dev_flags);
    }

    return ret;
}

#ifdef CFG_STATION_SUPPORT
//scan request can refer wlan_send_probe_req in rk_iot code
int asr_send_scanu_req(struct asr_vif *asr_vif,
                        struct cfg80211_scan_request *param)
{
    int i;
    uint8_t chan_flags = 0;
    struct scanu_start_req *req;
    struct asr_hw *asr_hw = asr_vif->asr_hw;
    uint8_t null_mac[6] = {0};
    int ret = 0;

    if (asr_test_bit(ASR_DEV_CLOSEING, &asr_vif->dev_flags)
        || !asr_test_bit(ASR_DEV_STARTED, &asr_hw->phy_flags)
        || asr_test_bit(ASR_DEV_STA_DISCONNECTING, &asr_vif->dev_flags)
        || asr_test_bit(ASR_DEV_STA_CONNECTING, &asr_vif->dev_flags)
        || asr_test_bit(ASR_DEV_RESTARTING, &asr_hw->phy_flags)) {

        dbg(D_ERR, D_UWIFI_CTRL, "ASR: %s,drv_flags=0X%X.\n", __func__, (unsigned int)asr_vif->dev_flags);

        return -EBUSY;
    }

    asr_set_bit(ASR_DEV_SCANING, &asr_vif->dev_flags);

    /* Build the SCANU_START_REQ message */
    req = asr_msg_zalloc(SCANU_START_REQ, TASK_SCANU, DRV_TASK_ID,
                          sizeof(struct scanu_start_req));
    if (!req) {
    asr_clear_bit(ASR_DEV_SCANING, &asr_vif->dev_flags);
        return -ENOMEM;
    }

    /* Set parameters */
    req->vif_idx = asr_vif->vif_index;
    req->chan_cnt = (uint8_t)min_t(int, SCAN_CHANNEL_MAX, param->n_channels);
    req->ssid_cnt = (uint8_t)min_t(int, SCAN_SSID_MAX, param->n_ssids);

    if(memcmp(param->bssid, null_mac, MAC_ADDR_LEN) != 0)
        memcpy((void*)req->bssid.array, param->bssid, MAC_ADDR_LEN);
    else
        req->bssid = mac_addr_bcst;

    req->no_cck = param->no_cck;

    if (req->ssid_cnt == 0)
        chan_flags |= SCAN_PASSIVE_BIT;
    for (i = 0; i < req->ssid_cnt; i++) {
        int j;
        for (j = 0; j < param->ssids[i].ssid_len; j++)
            req->ssid[i].array[j] = param->ssids[i].ssid[j];
        req->ssid[i].length = param->ssids[i].ssid_len;
    }
    if (param->ie)
    {
        memcpy(req->add_ie, param->ie, param->ie_len);
        req->add_ie_len = param->ie_len;
    }
    else
    {
        req->add_ie_len = 0;
    }

    for (i = 0; i < req->chan_cnt; i++) {
        struct ieee80211_channel *chan = param->channels[i];

        req->chan[i].band = chan->band;
        req->chan[i].freq = chan->center_freq;
        req->chan[i].flags = chan_flags | passive_scan_flag(chan->flags);
        req->chan[i].tx_power = chan->max_reg_power;
        #ifdef CONFIG_ASR595X
        req->chan[i].scan_additional_cnt = SCAN_ADDITIONAL_TIMES;
        #endif
    }

    /* Send the SCANU_START_REQ message to LMAC FW */

    ret = asr_send_msg(asr_hw, req, 0, 0, NULL);
    if (ret) {
        asr_clear_bit(ASR_DEV_SCANING, &asr_vif->dev_flags);
    }

    if (asr_test_bit(ASR_DEV_INITED, &asr_hw->phy_flags)
        && !asr_test_bit(ASR_DEV_RESTARTING, &asr_hw->phy_flags)
        && asr_test_bit(ASR_DEV_SCANING, &asr_vif->dev_flags)) {

        asr_rtos_update_timer(&(asr_hw->scan_cmd_timer), ASR_SCAN_CMD_TIMEOUT_MS);
        asr_rtos_start_timer(&(asr_hw->scan_cmd_timer));
    }

    return ret;
}
#endif

static const uint8_t *cfg80211_find_ie(uint8_t eid, const uint8_t *ies, int len)
{
    while (len > 2 && ies[0] != eid) {
        len -= ies[1] + 2;
        ies += ies[1] + 2;
    }
    if (len < 2)
        return NULL;
    if (len < 2 + ies[1])
        return NULL;
    return ies;
}

int asr_send_apm_start_req(struct asr_hw *asr_hw, struct asr_vif *vif,
                            struct cfg80211_ap_settings *settings,
                            struct apm_start_cfm *cfm, struct asr_dma_elem *elem)
{
    struct apm_start_req *req;
    struct asr_bcn *bcn = &vif->ap.bcn;
    uint8_t *buf;
    uint32_t flags = 0;
    const uint8_t *rate_ie;
    uint8_t rate_len = 0;
    int var_offset = offsetof(struct ieee80211_mgmt, u.beacon.variable);
    const uint8_t *var_pos;
    int len, i;

    /* Build the APM_START_REQ message */
    req = asr_msg_zalloc(APM_START_REQ, TASK_APM, DRV_TASK_ID,
                                   sizeof(struct apm_start_req));
    if (!req)
        return -ENOMEM;

    // Build the beacon
    bcn->dtim = (uint8_t)settings->dtim_period;
    buf = asr_build_bcn(bcn, &settings->beacon);
    if (!buf) {
        asr_msg_free(asr_hw, req);
        return -ENOMEM;
    }

    // Retrieve the basic rate set from the beacon buffer
    len = bcn->len - var_offset;
    var_pos = buf + var_offset;

    rate_ie = cfg80211_find_ie(WLAN_EID_SUPP_RATES, var_pos, len);
    if (rate_ie) {
        const uint8_t *rates = rate_ie + 2;
        for (i = 0; i < rate_ie[1]; i++) {
            if (rates[i] & 0x80)
                req->basic_rates.array[rate_len++] = rates[i];
        }
    }
    rate_ie = cfg80211_find_ie(WLAN_EID_EXT_SUPP_RATES, var_pos, len);
    if (rate_ie) {
        const uint8_t *rates = rate_ie + 2;
        for (i = 0; i < rate_ie[1]; i++) {
            if (rates[i] & 0x80)
                req->basic_rates.array[rate_len++] = rates[i];
        }
    }
    req->basic_rates.length = rate_len;

    // Fill in the DMA structure
    elem->buf = buf;
    elem->len = bcn->len;
    memcpy(req->bcn, buf, bcn->len);

    /* Set parameters for the APM_START_REQ message */
    req->vif_idx = vif->vif_index;
    req->bcn_len = bcn->len;
    req->tim_oft = bcn->head_len;
    req->tim_len = bcn->tim_len;

    req->chan.flags = 0;

#ifdef CONFIG_ASR595X
    req->chan.band = settings->chandef.chan->band;
    req->chan.prim20_freq = settings->chandef.chan->center_freq;
    //req->chan.flags = 0;
    req->chan.tx_power = settings->chandef.chan->max_power;
    req->chan.center1_freq = settings->chandef.center_freq1;
    req->chan.center2_freq = settings->chandef.center_freq2;
    //req->ch_width = bw2chnl[settings->chandef.width];
#else
    req->chan.band = settings->chandef.chan->band;
    req->chan.freq = settings->chandef.chan->center_freq;
    req->chan.tx_power = settings->chandef.chan->max_power;
    req->center_freq1 = settings->chandef.center_freq1;
    req->center_freq2 = settings->chandef.center_freq2;
    req->ch_width = bw2chnl[settings->chandef.width];
#endif

    req->bcn_int = settings->beacon_interval;
    dbg(D_ERR, D_UWIFI_CTRL, "vif_idx:%d bcn_len:%d tim_oft:%d tim_len:%d", req->vif_idx, req->bcn_len, req->tim_oft, req->tim_len);
    if (settings->crypto.control_port)
        flags |= CONTROL_PORT_HOST;

    if (settings->crypto.control_port_no_encrypt)
        flags |= CONTROL_PORT_NO_ENC;

    if (use_pairwise_key(&settings->crypto))
        flags |= WPA_WPA2_IN_USE;

    if (settings->crypto.control_port_ethertype)
        req->ctrl_port_ethertype = settings->crypto.control_port_ethertype;
    else
        req->ctrl_port_ethertype = ETH_P_PAE;
    req->flags = flags;

    req->ssid.length = settings->ssid_len;
    memcpy(req->ssid.array, settings->ssid, settings->ssid_len);

    /* Send the APM_START_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, APM_START_CFM, cfm);
}

int asr_send_apm_stop_req(struct asr_hw *asr_hw, struct asr_vif *vif)
{
    struct apm_stop_req *req;



    /* Build the APM_STOP_REQ message */
    req = asr_msg_zalloc(APM_STOP_REQ, TASK_APM, DRV_TASK_ID,
                                   sizeof(struct apm_stop_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the APM_STOP_REQ message */
    req->vif_idx = vif->vif_index;

    /* Send the APM_STOP_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, APM_STOP_CFM, NULL);
}



int asr_send_apm_start_cac_req(struct asr_hw *asr_hw, struct asr_vif *vif,
                                struct cfg80211_chan_def *chandef,
                                struct apm_start_cac_cfm *cfm)
{
    struct apm_start_cac_req *req;



    /* Build the APM_START_CAC_REQ message */
    req = asr_msg_zalloc(APM_START_CAC_REQ, TASK_APM, DRV_TASK_ID,
                          sizeof(struct apm_start_cac_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the APM_START_CAC_REQ message */
    req->vif_idx = vif->vif_index;
    req->chan.band = chandef->chan->band;

#ifdef CONFIG_ASR595X
    req->chan.prim20_freq = chandef->chan->center_freq;
    req->chan.flags = 0;
    req->chan.center1_freq = chandef->center_freq1;
    req->chan.center2_freq = chandef->center_freq2;
    //req->ch_width = bw2chnl[chandef->width];
#else
    req->chan.freq = chandef->chan->center_freq;
    req->chan.flags = 0;
    req->center_freq1 = chandef->center_freq1;
    req->center_freq2 = chandef->center_freq2;
    req->ch_width = bw2chnl[chandef->width];
#endif

    /* Send the APM_START_CAC_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, APM_START_CAC_CFM, cfm);
}

int asr_send_apm_stop_cac_req(struct asr_hw *asr_hw, struct asr_vif *vif)
{
    struct apm_stop_cac_req *req;



    /* Build the APM_STOP_CAC_REQ message */
    req = asr_msg_zalloc(APM_STOP_CAC_REQ, TASK_APM, DRV_TASK_ID,
                          sizeof(struct apm_stop_cac_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the APM_STOP_CAC_REQ message */
    req->vif_idx = vif->vif_index;

    /* Send the APM_STOP_CAC_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 1, APM_STOP_CAC_CFM, NULL);
}

int asr_send_cfg_rssi_req(struct asr_hw *asr_hw, uint8_t vif_index, int rssi_thold, uint32_t rssi_hyst)
{
    struct mm_cfg_rssi_req *req;



    /* Build the MM_CFG_RSSI_REQ message */
    req = asr_msg_zalloc(MM_CFG_RSSI_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_cfg_rssi_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_CFG_RSSI_REQ message */
    req->vif_index = vif_index;
    req->rssi_thold = (int8_t)rssi_thold;
    req->rssi_hyst = (uint8_t)rssi_hyst;

    /* Send the MM_CFG_RSSI_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 0, 0, NULL);
}
int asr_send_cfg_rssi_level_req(struct asr_hw *asr_hw, bool enable)
{
    struct mm_cfg_rssi_level_req *req;
    struct asr_vif *asr_vif;

    asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];


    /* Build the MM_CFG_RSSI_LEVEL_REQ message */
    req = asr_msg_zalloc(MM_CFG_RSSI_LEVEL_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_cfg_rssi_level_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_CFG_RSSI_LEVEL_REQ message */
    req->vif_index = asr_vif->vif_index;
    req->rssi_level_enable = enable;

    /* Send the MM_CFG_RSSI_LEVEL_REQ message to LMAC FW */
    return asr_send_msg(asr_hw, req, 0, 0, NULL);
}



int asr_send_get_rssi_req(struct asr_hw *asr_hw, u8 staid, s8 *rssi)
{
    struct mm_get_rssi_cfm *req = NULL;
    struct mm_get_rssi_cfm cfm;
    int ret = 0;

    if (!asr_hw || !rssi) {
        return -1;
    }

    *rssi = 0;

    /* Build the MM_SET_POWER_REQ message */
    req = asr_msg_zalloc(MM_GET_RSSI_REQ, TASK_MM, DRV_TASK_ID, sizeof(struct mm_get_rssi_cfm));
    if (!req)
        return -ENOMEM;

    req->staid = staid;

    /* Send the MM_GET_RSSI_CFM message to LMAC FW */
    ret = asr_send_msg(asr_hw, req, 1, MM_GET_RSSI_CFM, &cfm);

    if (cfm.status == CO_OK) {
        *rssi = cfm.rssi;
    }

    dbg(D_INF, D_UWIFI_CTRL, "%s: %d,%d,%d=%d\n", __func__, ret, cfm.status,
        cfm.staid, cfm.rssi);

    //kfree(msg);
    return ret;
}

int asr_send_upload_fram_req(struct asr_hw *asr_hw, u8 vif_idx, u16 fram_type, u8 enable)
{
    struct mm_set_upload_fram_cfm *req = NULL;
    struct mm_set_upload_fram_cfm cfm;
    int ret = 0;

    if (!asr_hw) {
        return -1;
    }

    /* Build the MM_SET_POWER_REQ message */
    req = asr_msg_zalloc(MM_SET_UPLOAD_FRAM_REQ, TASK_MM, DRV_TASK_ID, sizeof(struct mm_set_upload_fram_cfm) + 32);
    if (!req)
        return -ENOMEM;

    req->vif_idx = vif_idx;
    req->fram_type = (u8)fram_type;
    req->enable = enable;

    /* Send the MM_GET_RSSI_CFM message to LMAC FW */
    ret = asr_send_msg(asr_hw, req, 1, MM_SET_UPLOAD_FRAM_CFM, &cfm);

    dbg(D_INF, D_UWIFI_CTRL, "%s:vif_idx=%d,fram_type=0x%02X,enable=%d,%d,%d\n",
        __func__, req->vif_idx, req->fram_type, req->enable, ret, cfm.status);

    if (cfm.status) {
        ret = -2;
    }

    //kfree(msg);
    return ret;
}

int asr_send_fram_appie_req(struct asr_hw *asr_hw, u8 vif_idx, u16 fram_type, u8 *ie, u8 ie_len)
{
    struct mm_set_fram_appie_req *req = NULL;
    struct mm_set_fram_appie_req cfm;
    int ret = 0;

    if (!asr_hw || ie_len > FRAM_CUSTOM_APPIE_LEN) {
        return -1;
    }


    /* Build the MM_SET_POWER_REQ message */
    req = asr_msg_zalloc(MM_SET_FRAM_APPIE_REQ, TASK_MM, DRV_TASK_ID, sizeof(struct mm_set_fram_appie_req));
    if (!req)
        return -ENOMEM;


    req->vif_idx = vif_idx;
    req->fram_type = (u8)fram_type;
    req->ie_len = ie_len;
    if (req->ie_len) {
        memcpy(req->appie, ie, ie_len);
    }

    /* Send the MM_GET_RSSI_CFM message to LMAC FW */
    ret = asr_send_msg(asr_hw, req, 1, MM_SET_FRAM_APPIE_CFM, &cfm);

    dbg(D_INF, D_UWIFI_CTRL, "%s: %d,%d\n", __func__, ret, cfm.status);

    if (cfm.status) {
        ret = -2;
    }

    //kfree(msg);
    return ret;
}

int asr_send_efuse_txpwr_req(struct asr_hw *asr_hw, uint8_t * txpwr, uint8_t * txevm, uint8_t *freq_err, bool iswrite, uint8_t *index)
{
    struct mm_efuse_txpwr_info *req = NULL;
    struct mm_efuse_txpwr_info cfm;
    int ret = 0;

    if (!asr_hw || !txpwr || !txevm || !freq_err) {
        return -EINVAL;
    }

    req = asr_msg_zalloc(MM_SET_EFUSE_TXPWR_REQ, TASK_MM, DRV_TASK_ID, sizeof(struct mm_efuse_txpwr_info));
    if (!req) {
        return -ENOMEM;
    }

    memset(&cfm, 0, sizeof(struct mm_efuse_txpwr_info));

    if (iswrite) {
        req->iswrite = iswrite;
        memcpy(req->txpwr, txpwr, 6);
        memcpy(req->txevm, txevm, 6);
        req->freq_err = *freq_err;
    }

    /* Send the MM_CFG_RSSI_REQ message to LMAC FW */
    ret = asr_send_msg(asr_hw, req, 1, MM_SET_EFUSE_TXPWR_CFM, &cfm);

    if (!iswrite) {
        memcpy(txpwr, cfm.txpwr, 6);
        memcpy(txevm, cfm.txevm, 6);
        *freq_err = cfm.freq_err;
    }

    *index = cfm.index;

    dbg(D_INF, D_UWIFI_CTRL, "%s: %d,%d,%d,0x%X(%02X,%02X,%02X,%02X,%02X,%02X),(%02X,%02X,%02X,%02X,%02X,%02X),%u\n"
        , __func__, ret, req->iswrite, cfm.status, cfm.freq_err
        ,cfm.txpwr[0],cfm.txpwr[1],cfm.txpwr[2],cfm.txpwr[3],cfm.txpwr[4],cfm.txpwr[5]
        ,cfm.txevm[0],cfm.txevm[1],cfm.txevm[2],cfm.txevm[3],cfm.txevm[4],cfm.txevm[5],cfm.index);

    ret = cfm.status;

    return ret;
}

int asr_send_hif_sdio_info_req(struct asr_hw *asr_hw)
{
    void *msg = NULL;
    int ret = 0;

    /* VERSION REQ has no parameter */
    msg = asr_msg_zalloc(MM_HIF_SDIO_INFO_REQ, TASK_MM, DRV_TASK_ID, 0);
    if (!msg)
        return -ENOMEM;

    ret = asr_send_msg(asr_hw, msg, 1, MM_HIF_SDIO_INFO_IND, NULL);

    //kfree(msg);
    return ret;
}
#ifdef CONFIG_TWT
int asr_send_itwt_config(struct asr_hw *asr_hw, wifi_twt_config_t * wifi_twt_param)
{
    struct me_itwt_config_req *req = NULL;
    struct asr_vif *asr_vif = NULL;

    /* Build the ME_CHAN_CONFIG_REQ message */
    req = asr_msg_zalloc(ME_ITWT_CONFIG_REQ, TASK_ME, DRV_TASK_ID, sizeof(struct me_itwt_config_req));
    if (!req)
        return -ENOMEM;

    if (asr_hw->sta_vif_idx < asr_hw->vif_max_num) {
        asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];
    }

    // clear twt drv flag first.
    if (asr_vif)
        asr_clear_bit(ASR_DEV_STA_OUT_TWTSP, &asr_vif->dev_flags);

    /* Set parameters for the ME_ITWT_CONFIG_REQ message */
    req->setup_cmd = wifi_twt_param->setup_cmd;    //0 :request 1:suggest 2:demand
    req->trigger = 1;
    req->implicit = 1;
    req->flow_type = wifi_twt_param->flow_type;
    req->twt_channel = 0;
    req->twt_prot = 0;
    req->wake_interval_exponent = wifi_twt_param->wake_interval_exponent;
    req->wake_interval_mantissa = wifi_twt_param->wake_interval_mantissa;
    req->target_wake_time_lo = 250000;

    req->monimal_min_wake_duration = wifi_twt_param->monimal_min_wake_duration;
    req->twt_upload_wait_trigger = wifi_twt_param->twt_upload_wait_trigger;

    /* Send the ME_ITWT_CONFIG_REQ message to FW */
    return asr_send_msg(asr_hw, req, 0, 0, NULL);
}

int asr_send_itwt_del(struct asr_hw *asr_hw, uint8_t flow_id)
{
    struct me_itwt_del_req *req = NULL;
    struct asr_vif *asr_vif = NULL;

    /* Build the ME_CHAN_CONFIG_REQ message */
    req = asr_msg_zalloc(ME_ITWT_DEL_REQ, TASK_ME, DRV_TASK_ID, sizeof(struct me_itwt_del_req));
    if (!req)
        return -ENOMEM;

    if (asr_hw->sta_vif_idx < asr_hw->vif_max_num) {
        asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];
    }

    // clear twt drv flag first.
    if (asr_vif)
        asr_clear_bit(ASR_DEV_STA_OUT_TWTSP, &asr_vif->dev_flags);

    /* Set parameters for the ME_ITWT_CONFIG_REQ message */
    req->flow_id = flow_id;

    /* Send the ME_ITWT_DEL_REQ message to FW */
    return asr_send_msg(asr_hw, req, 0, 0, NULL);
}
#endif//ifdef CONFIG_TWT

int asr_send_set_tx_pwr_rate(struct asr_hw *asr_hw, struct mm_set_tx_pwr_rate_cfm *cfm,
                 struct mm_set_tx_pwr_rate_req *tx_pwr)
{
    struct mm_set_tx_pwr_rate_req *req = NULL;
    int ret = 0;

    if (!asr_hw || !cfm || !tx_pwr) {
        return -1;
    }


    /* Build the MM_SET_POWER_REQ message */
    req = asr_msg_zalloc(MM_SET_TX_PWR_RATE_REQ, TASK_MM, DRV_TASK_ID, sizeof(struct mm_set_tx_pwr_rate_req));
    if (!req)
        return -ENOMEM;

    cfm->status = CO_OK;

    memcpy(req, tx_pwr, sizeof(struct mm_set_tx_pwr_rate_req));

    /* Send the MM_SET_TX_PWR_RATE_REQ message to LMAC FW */
    ret = asr_send_msg(asr_hw, req, 1, MM_SET_TX_PWR_RATE_CFM, cfm);

    dbg(D_INF, D_UWIFI_CTRL, "%s: %d,%d\n", __func__, ret, cfm->status);

    return ret;
}
