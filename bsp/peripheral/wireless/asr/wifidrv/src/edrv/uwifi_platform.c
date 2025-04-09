/**
 ******************************************************************************
 *
 * @file uwifi_platform.c
 *
 * @brief platform related implement
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */
#include "uwifi_include.h"
#include "uwifi_rx.h"
#include "uwifi_msg_rx.h"
#include "uwifi_platform.h"
#include "uwifi_kernel.h"
#include "uwifi_tx.h"
#include "uwifi_msg.h"
#include "asr_rtos.h"
#include "asr_sdio.h"
#include "uwifi_sdio.h"
#include "uwifi_ops_adapter.h"
#include "uwifi_hif.h"
#include "tasks_info.h"

#ifdef THREADX
#include "loadtable.h"
#else
#include "firmware.h"
#endif

extern asr_thread_hand_t  lwifi_task_handle;
bool txlogen = 0;
bool rxlogen = 0;
int lalalaen = 0;
#ifdef CONFIG_ASR_KEY_DBG
int tx_status_debug = 20;    // default 20s.
#endif
//int setscheduler = 0;
//int dbg_type = 0;
//32*512 = 16384, so tx/rx aggr num can't exceed 16384/1696 = 9.66, so the maximum is 8
//int tx_aggr = 8;  //8 will error//7 is maximum as port0 as msg

//int rx_aggr_max_num = 8;//but when wrap exceed the maximum index 15, then 6 is the maximum rx_aggr_max_num, as port index 0 and 1 is reversed
//int rx_thread_timer = 800;//the rx_thread_timer smaller, the ipc host irq more, now msg/tx/rx all in sdio_irq_main process, so too much ipc host irq will be nosiy
int flow_ctrl_high = 150;
int flow_ctrl_low = 80;
//int tx_conserve = 1;   //if tx_conserve == 1, tx_aggr = 4 will be better

void ipc_host_init(struct ipc_host_env_tag *env,
                  struct ipc_host_cb_tag *cb, void *pthis)
{
    // Reset the IPC Host environment
    memset(env, 0, sizeof(struct ipc_host_env_tag));

    // Save the callbacks in our own environment
    env->cb = *cb;

    // Save the pointer to the register base
    env->pthis = pthis;

    // Initialize buffers numbers and buffers sizes needed for DMA Receptions
    env->rx_bufnb = IPC_RXBUF_CNT;
    env->rx_bufsz = IPC_RXBUF_SIZE;

    env->rx_msgbufnb = IPC_RXMSGBUF_CNT;
    env->rx_msgbufsz = IPC_RXMSGBUF_SIZE;

#ifdef SDIO_DEAGGR
    env->rx_bufnb_sdio_deagg = IPC_RXBUF_CNT_SDIO_DEAGG;
    env->rx_bufsz_sdio_deagg = IPC_RXBUF_SIZE_SDIO_DEAGG;
#endif

    #ifdef CFG_AMSDU_TEST
    env->rx_bufnb_split = IPC_RXBUF_CNT_SPLIT;
    env->rx_bufsz_split = IPC_RXBUF_SIZE_SPLIT;
    #endif
}

int asr_rxbuff_alloc(struct asr_hw *asr_hw, uint32_t len, struct sk_buff **skb)
{
    struct sk_buff *skb_new;
    skb_new = dev_alloc_skb_rx(len);

    if (!skb_new) {
        dbg(D_ERR,D_UWIFI_DATA," skb alloc of size %u failed\n\n", (unsigned int)len);
        return -ENOMEM;
    }
    *skb = skb_new;

    return 0;
}

static void asr_elems_deallocs(struct asr_hw *asr_hw)
{
    // Get first element
    struct sk_buff *skb;

    while (!skb_queue_empty(&asr_hw->rx_data_sk_list))
    {
        skb = __skb_dequeue(&asr_hw->rx_data_sk_list);
        if(skb)
        {
            dev_kfree_skb_rx(skb);
            skb = NULL;
        }
    }

    while (!skb_queue_empty(&asr_hw->rx_msg_sk_list))
    {
        skb = __skb_dequeue(&asr_hw->rx_msg_sk_list);
        if(skb)
        {
            dev_kfree_skb_rx(skb);
            skb = NULL;
        }
    }

    #ifdef CFG_AMSDU_TEST
    // used for reconstruct amsdu.
    while (!skb_queue_empty(&asr_hw->rx_sk_split_list))
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:%d: rx_sk_split_list deleted\n", __func__, __LINE__);
        skb = __skb_dequeue(&asr_hw->rx_sk_split_list);
        if(skb)
        {
            dev_kfree_skb_rx(skb);
            skb = NULL;
        }
    }

    #endif

    #ifdef SDIO_DEAGGR
    while (!skb_queue_empty(&asr_hw->rx_sk_sdio_deaggr_list)) {
        skb = skb_dequeue(&asr_hw->rx_sk_sdio_deaggr_list);
        if (skb) {
            dev_kfree_skb_rx(skb);
            skb = NULL;
        }
    }
    #endif

}


/**
 * @brief Allocate storage elements.
 *
 *  This function allocates all the elements required for communications with
LMAC,
 *  such as Rx Data elements, MSGs elements, ...
 *
 * This function should be called in correspondence with the deallocation
function.
 *
 * @param[in]   asr_hw   Pointer to main structure storing all the relevant
information
 */
static int asr_elems_allocs(struct asr_hw *asr_hw)
{
    struct sk_buff *skb;
    int i;
    dbg(D_ERR, D_UWIFI_CTRL, "%s, rx_bufnb = %d rx_msgbufnb = %d rx_bufsz = %d , rx_msgbufsz = %d\r\n",__func__,
                               asr_hw->ipc_env->rx_bufnb,asr_hw->ipc_env->rx_msgbufnb,
                               asr_hw->ipc_env->rx_bufsz,asr_hw->ipc_env->rx_msgbufsz);

    for (i = 0; i < asr_hw->ipc_env->rx_bufnb; i++) {

        // Allocate a new sk buff
        if (asr_rxbuff_alloc(asr_hw, asr_hw->ipc_env->rx_bufsz, &skb)) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s:%d: MEM ALLOC FAILED\n", __func__, __LINE__);
            goto err_alloc;
        }

        // Add the sk buffer structure in the table of rx buffer
        skb_queue_tail(&asr_hw->rx_data_sk_list, skb);
    }

    for (i = 0; i < asr_hw->ipc_env->rx_msgbufnb; i++) {

        // Allocate a new sk buff
        if (asr_rxbuff_alloc(asr_hw, asr_hw->ipc_env->rx_msgbufsz, &skb)) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s:%d: MEM ALLOC FAILED\n", __func__, __LINE__);
            goto err_alloc;
        }

        // Add the sk buffer structure in the table of rx buffer
        skb_queue_tail(&asr_hw->rx_msg_sk_list, skb);
    }

    #ifdef CFG_AMSDU_TEST
        for (i = 0; i < asr_hw->ipc_env->rx_bufnb_split; i++) {

        // Allocate a new sk buff
        if (asr_rxbuff_alloc(asr_hw, asr_hw->ipc_env->rx_bufsz_split, &skb)) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s:%d: MEM ALLOC FAILED\n", __func__, __LINE__);
            goto err_alloc;
        }

        // Add the sk buffer structure in the table of rx buffer
        dbg(D_INF, D_UWIFI_CTRL, "%s:%d: rx_sk_split_list added\n", __func__, __LINE__);
        skb_queue_tail(&asr_hw->rx_sk_split_list, skb);
    }
    #endif

    #ifdef SDIO_DEAGGR
    for (i = 0; i < asr_hw->ipc_env->rx_bufnb_sdio_deagg; i++) {

        // Allocate a new sk buff
        if (asr_rxbuff_alloc(asr_hw, asr_hw->ipc_env->rx_bufsz_sdio_deagg, &skb)) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s:%d: MEM ALLOC FAILED\n", __func__, __LINE__);
            goto err_alloc;
        }

        memset(skb->data, 0, asr_hw->ipc_env->rx_bufsz_sdio_deagg);
        // Add the sk buffer structure in the table of rx buffer
        skb_queue_tail(&asr_hw->rx_sk_sdio_deaggr_list, skb);
    }
    #endif


    return 0;

err_alloc:
    asr_elems_deallocs(asr_hw);
    return -ENOMEM;
}


// tx hif buf
int asr_txhifbuffs_alloc(struct asr_hw *asr_hw, u32 len, struct sk_buff **skb)
{
    struct sk_buff *skb_new;

    skb_new = dev_alloc_skb_tx(len);

    if (skb_new == NULL) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:%d: skb alloc of size %u failed\n\n", __func__, __LINE__, len);
        return -ENOMEM;
    }

    *skb = skb_new;

    return 0;
}

void asr_txhifbuffs_dealloc(struct asr_hw *asr_hw)
{
    // Get first element
    struct sk_buff *skb = NULL;

    while (!skb_queue_empty(&asr_hw->tx_sk_free_list)) {
        skb = __skb_dequeue(&asr_hw->tx_sk_free_list);
        if (skb) {
            dev_kfree_skb_tx(skb);
            skb = NULL;
        }
    }

    while (!skb_queue_empty(&asr_hw->tx_sk_list)) {
        skb = __skb_dequeue(&asr_hw->tx_sk_list);
        if (skb) {
            dev_kfree_skb_tx(skb);
            skb = NULL;
        }
    }

    while (!skb_queue_empty(&asr_hw->tx_hif_skb_list)) {
        skb = __skb_dequeue(&asr_hw->tx_hif_skb_list);
        if (skb) {
            dev_kfree_skb_tx(skb);
            skb = NULL;
        }
    }

    while (!skb_queue_empty(&asr_hw->tx_hif_free_buf_list)) {
        skb = __skb_dequeue(&asr_hw->tx_hif_free_buf_list);
        if (skb) {
            dev_kfree_skb_tx(skb);
            skb = NULL;
        }
    }
}


/**
 * WLAN driver call-back function for message reception indication
 */
extern struct asr_traffic_status g_asr_traffic_sts;
uint8_t asr_msgind(void *pthis, void *hostid)
{
    struct asr_hw *asr_hw = (struct asr_hw *)pthis;
    struct sk_buff *skb = (struct sk_buff*)hostid;
    struct ipc_e2a_msg *msg;
    uint8_t ret = 0;
    bool is_ps_change_ind_msg = false;

    /* Retrieve the message structure */
    msg = (struct ipc_e2a_msg *)skb->data;
    /* Relay further actions to the msg parser */
    asr_rx_handle_msg(asr_hw, msg);
    is_ps_change_ind_msg = ((MSG_T(msg->id) == TASK_MM) && (MSG_I(msg->id) == MM_PS_CHANGE_IND));

    skb_queue_tail(&asr_hw->rx_msg_sk_list, skb);

    //dbg(D_ERR, D_UWIFI_CTRL, "%s:%d: msg use cnt %d\n", __func__, __LINE__,msg_buf_cnt_use);
    if (is_ps_change_ind_msg == true) {
           // move traffic sts msg send here.
           if (g_asr_traffic_sts.send) {
                       struct asr_sta *asr_sta_tmp = g_asr_traffic_sts.asr_sta_ps;

                // send msg may schedule.
                if (g_asr_traffic_sts.ps_id_bits & LEGACY_PS_ID)
                    asr_set_traffic_status(asr_hw, asr_sta_tmp, g_asr_traffic_sts.tx_ava, LEGACY_PS_ID);

                if (g_asr_traffic_sts.ps_id_bits & UAPSD_ID)
                    asr_set_traffic_status(asr_hw, asr_sta_tmp, g_asr_traffic_sts.tx_ava, UAPSD_ID);

                 dbg(D_INF,D_UWIFI_CTRL," [ps]tx_ava=%d:sta-%d, uapsd=0x%x, (%d , %d) \r\n",
                                                        g_asr_traffic_sts.tx_ava,
                                                        asr_sta_tmp->sta_idx,asr_sta_tmp->uapsd_tids,
                                                        asr_sta_tmp->ps.pkt_ready[LEGACY_PS_ID],
                                                        asr_sta_tmp->ps.pkt_ready[UAPSD_ID]);
           }
    }

    return ret;
}

/**
 * WLAN driver call-back function for primary TBTT indication
 */
void asr_prim_tbtt_ind(void *pthis)
{
}
asr_semaphore_t g_asr_mgmt_sem;
int asr_ipc_init(struct asr_hw *asr_hw)
{
    struct ipc_host_cb_tag cb;

    /* initialize the API interface */
    cb.recv_data_ind   = asr_rxdataind;
    cb.recv_msg_ind    = asr_msgind;
    //cb.recv_msgack_ind = asr_msgackind;
    //cb.send_data_cfm   = asr_txdatacfm;
    //cb.prim_tbtt_ind   = asr_prim_tbtt_ind;
    cb.recv_dbg_ind = asr_dbgind;

    /* set the IPC environment */
    asr_hw->ipc_env = (struct ipc_host_env_tag *)
                       asr_rtos_malloc(sizeof(struct ipc_host_env_tag));
    if(NULL == asr_hw->ipc_env)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s:alloc ipc_host_env_tag failed",__func__);
        return -ENOMEM;
    }

    /* call the initialization of the IPC */
    ipc_host_init(asr_hw->ipc_env, &cb, asr_hw);
    /* rx mgmt semaphore */
    asr_rtos_init_semaphore(&g_asr_mgmt_sem, 1);
    if(asr_cmd_mgr_init(&asr_hw->cmd_mgr))
        return -1;
    else
        return asr_elems_allocs(asr_hw);
}

extern struct asr_hw g_asr_hw;
extern struct asr_hw *sp_asr_hw;

#ifdef CONFIG_ASR595X
int asr_download_bootloader(struct asr_hw *asr_hw)
{
    int ret;

    struct asr_firmware asr_bootld;
    request_firmware(&asr_bootld,ASR_BOOTLOADER);

    ret = asr_sdio_download_firmware(asr_hw, (uint8_t *)asr_bootld.data, asr_bootld.size);

    if(ret)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s download bootloader fail",__func__);
        return ret;
    }

    return ret;

}
#endif

int asr_download_fw(struct asr_hw *asr_hw)
{
    int ret;

    #ifdef THREADX
    uint32_t fw_img_ptr_loaded = get_asrbin_begin_address();//(unsigned char *)(0x81C00000 + 680 * 1024);
    #else
    struct asr_firmware asr_fw;
    #endif

    asr_sdio_set_block_size(SDIO_BLOCK_SIZE_DLD);//is a question, it should set the hardware of the block size

#ifdef CONFIG_ASR595X
    ret = asr_download_bootloader(asr_hw);
    if(ret)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s download bootloader fail",__func__);
        return ret;
    }
#endif

    #ifndef THREADX
    request_firmware(&asr_fw,ASR_FIRMWARE);
    #endif

    #ifdef THREADX
    ret = asr_sdio_download_firmware(asr_hw, (uint8_t *)fw_img_ptr_loaded, 163840);
    #else
    ret = asr_sdio_download_firmware(asr_hw, (uint8_t *)asr_fw.data, asr_fw.size);
    #endif
    if(ret)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s download fw fail",__func__);
        return ret;
    }

    return ret;

}

int asr_platform_on(struct asr_hw *asr_hw)
{
    int ret;

    //ipc_shenv = (struct ipc_shared_env_tag *)&ipc_shared_env;

    if ((ret = asr_ipc_init(asr_hw))) {
        dbg(D_ERR, D_UWIFI_CTRL, "asr ipc init failed\r\n");
        return ret;
    }
    //init sdio irq
    if (init_sdio_task(asr_hw) != kNoErr) {
        dbg(D_ERR, D_UWIFI_CTRL, "asr init rx uwifi failed\r\n");
        return -1;
    }
    //init rx to os task
    if (init_rx_to_os(asr_hw) != kNoErr) {
        dbg(D_ERR, D_UWIFI_CTRL, "asr init rx to os failed\r\n");
        return -1;
    }
    return 0;
}

void asr_ipc_deinit(struct asr_hw *asr_hw)
{
    asr_cmd_mgr_deinit(&asr_hw->cmd_mgr);
    asr_elems_deallocs(asr_hw);
    asr_rtos_deinit_semaphore(&g_asr_mgmt_sem);
    asr_rtos_free(asr_hw->ipc_env);
    asr_hw->ipc_env = NULL;
}

/**
 * asr_platform_off - Stop the platform
 *
 * @asr_hw Main driver data
 *
 * Called by 802.11 part
 */
void asr_platform_off(struct asr_hw *asr_hw)
{
    deinit_sdio_task();
    deinit_rx_to_os();
    asr_ipc_deinit(asr_hw);
}

//following tx_iq_comp_2_4G/rx_iq_comp_2_4G value need further check
//had checked, 01000000
int asr_parse_phy_configfile(struct asr_hw *asr_hw,
                              struct asr_phy_conf_file *config)
{

    /* Get Trident path mapping */
    config->trd.path_mapping = asr_hw->mod_params->phy_cfg;


    /* Get DC offset compensation */
    config->trd.tx_dc_off_comp = 0;


    /* Get Karst TX IQ compensation value for path0 on 2.4GHz */
    config->karst.tx_iq_comp_2_4G[0] = 0x01000000;

    /* Get Karst TX IQ compensation value for path1 on 2.4GHz */
    config->karst.tx_iq_comp_2_4G[1] = 0x01000000;

    /* Get Karst RX IQ compensation value for path0 on 2.4GHz */
    config->karst.rx_iq_comp_2_4G[0] = 0x01000000;

    /* Get Karst RX IQ compensation value for path1 on 2.4GHz */
    config->karst.rx_iq_comp_2_4G[1] = 0x01000000;
    /* Get Karst default path */
    config->karst.path_used = asr_hw->mod_params->phy_cfg;



    return 0;
}

static int asr_check_fw_hw_feature(struct asr_hw *asr_hw,
                                    struct wiphy *wiphy)
{
    uint32_t sys_feat = asr_hw->version_cfm.features;
    //uint32_t mac_feat = asr_hw->version_cfm.version_machw_1;
    uint32_t phy_feat = asr_hw->version_cfm.version_phy_1;

    if (!(sys_feat & BIT(MM_FEAT_UMAC_BIT))) {
        dbg(D_ERR,D_UWIFI_CTRL,"Loading softmac firmware with fullmac driver\n");
        return -1;
    }

    if (!(sys_feat & BIT(MM_FEAT_PS_BIT))) {
        asr_hw->mod_params->ps_on = false;
    }

    /* AMSDU (non)support implies different shared structure definition
       so insure that fw and drv have consistent compilation option */
    if (sys_feat & BIT(MM_FEAT_AMSDU_BIT)) {
#if !NX_AMSDU_TX
        dbg(D_ERR,D_UWIFI_CTRL,"AMSDU enabled in firmware but support not compiled in driver\n");
        return -1;
#else
        if (asr_hw->mod_params->amsdu_maxnb > NX_TX_PAYLOAD_MAX)
            asr_hw->mod_params->amsdu_maxnb = NX_TX_PAYLOAD_MAX;
#endif
    } else {
#if NX_AMSDU_TX
        dbg(D_ERR,D_UWIFI_CTRL,"AMSDU disabled in firmware but support compiled in driver\n");
        return -1;
#endif
    }

    if (!(sys_feat & BIT(MM_FEAT_UAPSD_BIT))) {
        asr_hw->mod_params->uapsd_timeout = 0;
    }

    if (sys_feat & BIT(MM_FEAT_BCN_BIT))
    {
    }else
    {
        dbg(D_ERR,D_UWIFI_CTRL,"disabled in firmware but support compiled in driver\n");
        return -1;
    }

    switch (__MDM_PHYCFG_FROM_VERS(phy_feat)) {
        case MDM_PHY_CONFIG_TRIDENT:
        case MDM_PHY_CONFIG_ELMA:
            asr_hw->mod_params->nss = 1;
            break;
        case MDM_PHY_CONFIG_KARST:
            {
                int nss_supp = (phy_feat & MDM_NSS_MASK) >> MDM_NSS_LSB;
                if (asr_hw->mod_params->nss > nss_supp)
                    asr_hw->mod_params->nss = nss_supp;
            }
            break;
        default:
            break;
    }

    if (asr_hw->mod_params->nss < 1 || asr_hw->mod_params->nss > 2)
        asr_hw->mod_params->nss = 1;

    return 0;
}

#ifdef CONFIG_ASR595X
static void asr_set_he_capa(struct asr_hw *asr_hw, struct wiphy *wiphy)
{
    struct ieee80211_supported_band *band_2GHz = wiphy->bands[NL80211_BAND_2GHZ];
    int i;
    int nss = asr_hw->mod_params->nss;
    struct ieee80211_sta_he_cap *he_cap;
    int mcs_map;

    if (!asr_hw->mod_params->he_on) {
        band_2GHz->iftype_data = NULL;
        band_2GHz->n_iftype_data = 0;
        return;
    }

    he_cap = (struct ieee80211_sta_he_cap *)&band_2GHz->iftype_data->he_cap;
    he_cap->has_he = true;

    if (asr_hw->mod_params->twt_request)
        he_cap->he_cap_elem.mac_cap_info[0] |= IEEE80211_HE_MAC_CAP0_TWT_REQ;

    he_cap->he_cap_elem.mac_cap_info[2] |= IEEE80211_HE_MAC_CAP2_ALL_ACK;
    if (asr_hw->mod_params->use_2040) {
        he_cap->he_cap_elem.phy_cap_info[0] |= IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_IN_2G;
        he_cap->ppe_thres[0] |= 0x10;
    }
    if (asr_hw->mod_params->use_80) {
        he_cap->he_cap_elem.phy_cap_info[0] |= IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_80MHZ_IN_5G;
    }
    if (asr_hw->mod_params->ldpc_on) {
        he_cap->he_cap_elem.phy_cap_info[1] |= IEEE80211_HE_PHY_CAP1_LDPC_CODING_IN_PAYLOAD;
    } else {
        // If no LDPC is supported, we have to limit to MCS0_9, as LDPC is mandatory
        // for MCS 10 and 11
        asr_hw->mod_params->he_mcs_map = min_t(int, asr_hw->mod_params->mcs_map, IEEE80211_HE_MCS_SUPPORT_0_9);
    }
    he_cap->he_cap_elem.phy_cap_info[1] |= IEEE80211_HE_PHY_CAP1_HE_LTF_AND_GI_FOR_HE_PPDUS_0_8US |

        IEEE80211_HE_PHY_CAP1_MIDAMBLE_RX_TX_MAX_NSTS;

    he_cap->he_cap_elem.phy_cap_info[2] |=
            IEEE80211_HE_PHY_CAP2_MIDAMBLE_RX_TX_MAX_NSTS |

        IEEE80211_HE_PHY_CAP2_NDP_4x_LTF_AND_3_2US | IEEE80211_HE_PHY_CAP2_DOPPLER_RX;
    if (asr_hw->mod_params->stbc_on)
        he_cap->he_cap_elem.phy_cap_info[2] |= IEEE80211_HE_PHY_CAP2_STBC_RX_UNDER_80MHZ;
    he_cap->he_cap_elem.phy_cap_info[3] |= IEEE80211_HE_PHY_CAP3_DCM_MAX_CONST_RX_16_QAM |
        IEEE80211_HE_PHY_CAP3_DCM_MAX_RX_NSS_1 | IEEE80211_HE_PHY_CAP3_RX_HE_MU_PPDU_FROM_NON_AP_STA;

    if (asr_hw->mod_params->bfmee) {
        he_cap->he_cap_elem.phy_cap_info[4] |= IEEE80211_HE_PHY_CAP4_SU_BEAMFORMEE;
        he_cap->he_cap_elem.phy_cap_info[4] |= IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_UNDER_80MHZ_4;
    }
    he_cap->he_cap_elem.phy_cap_info[5] |= IEEE80211_HE_PHY_CAP5_NG16_SU_FEEDBACK |
        IEEE80211_HE_PHY_CAP5_NG16_MU_FEEDBACK;
    he_cap->he_cap_elem.phy_cap_info[6] |= IEEE80211_HE_PHY_CAP6_CODEBOOK_SIZE_42_SU |
        IEEE80211_HE_PHY_CAP6_CODEBOOK_SIZE_75_MU |
        IEEE80211_HE_PHY_CAP6_TRIG_SU_BEAMFORMER_FB |
        IEEE80211_HE_PHY_CAP6_TRIG_MU_BEAMFORMER_FB |
        IEEE80211_HE_PHY_CAP6_PPE_THRESHOLD_PRESENT | IEEE80211_HE_PHY_CAP6_PARTIAL_BANDWIDTH_DL_MUMIMO;
    he_cap->he_cap_elem.phy_cap_info[7] |= IEEE80211_HE_PHY_CAP7_HE_SU_MU_PPDU_4XLTF_AND_08_US_GI;
    he_cap->he_cap_elem.phy_cap_info[8] |= IEEE80211_HE_PHY_CAP8_20MHZ_IN_40MHZ_HE_PPDU_IN_2G;
    he_cap->he_cap_elem.phy_cap_info[9] |= IEEE80211_HE_PHY_CAP9_RX_FULL_BW_SU_USING_MU_WITH_COMP_SIGB |
        IEEE80211_HE_PHY_CAP9_RX_FULL_BW_SU_USING_MU_WITH_NON_COMP_SIGB;

    mcs_map = asr_hw->mod_params->he_mcs_map;
    memset(&he_cap->he_mcs_nss_supp, 0, sizeof(he_cap->he_mcs_nss_supp));
    for (i = 0; i < nss; i++) {
        uint16_t unsup_for_ss = cpu_to_le16(IEEE80211_HE_MCS_NOT_SUPPORTED << (i * 2));
        he_cap->he_mcs_nss_supp.rx_mcs_80 |= cpu_to_le16(mcs_map << (i * 2));
        he_cap->he_mcs_nss_supp.rx_mcs_160 |= unsup_for_ss;
        he_cap->he_mcs_nss_supp.rx_mcs_80p80 |= unsup_for_ss;
        mcs_map = IEEE80211_HE_MCS_SUPPORT_0_9;
    }
    for (; i < 8; i++) {
        uint16_t unsup_for_ss = cpu_to_le16(IEEE80211_HE_MCS_NOT_SUPPORTED << (i * 2));
        he_cap->he_mcs_nss_supp.rx_mcs_80 |= unsup_for_ss;
        he_cap->he_mcs_nss_supp.rx_mcs_160 |= unsup_for_ss;
        he_cap->he_mcs_nss_supp.rx_mcs_80p80 |= unsup_for_ss;
    }
    mcs_map = asr_hw->mod_params->he_mcs_map;
    for (i = 0; i < nss; i++) {
        uint16_t unsup_for_ss = cpu_to_le16(IEEE80211_HE_MCS_NOT_SUPPORTED << (i * 2));
        he_cap->he_mcs_nss_supp.tx_mcs_80 |= cpu_to_le16(mcs_map << (i * 2));
        he_cap->he_mcs_nss_supp.tx_mcs_160 |= unsup_for_ss;
        he_cap->he_mcs_nss_supp.tx_mcs_80p80 |= unsup_for_ss;
        mcs_map = min_t(int, asr_hw->mod_params->he_mcs_map, IEEE80211_HE_MCS_SUPPORT_0_9);
    }
    for (; i < 8; i++) {
        uint16_t unsup_for_ss = cpu_to_le16(IEEE80211_HE_MCS_NOT_SUPPORTED << (i * 2));
        he_cap->he_mcs_nss_supp.tx_mcs_80 |= unsup_for_ss;
        he_cap->he_mcs_nss_supp.tx_mcs_160 |= unsup_for_ss;
        he_cap->he_mcs_nss_supp.tx_mcs_80p80 |= unsup_for_ss;
    }
}
#endif

int asr_handle_dynparams(struct asr_hw *asr_hw, struct wiphy *wiphy)
{
    struct ieee80211_supported_band *band_2GHz =
        wiphy->bands[IEEE80211_BAND_2GHZ];

    uint32_t mdm_phy_cfg;
    int i, ret;
    int nss;

    ret = asr_check_fw_hw_feature(asr_hw, wiphy);
    if (ret)
        return ret;

    if (asr_hw->mod_params->phy_cfg < 0 || asr_hw->mod_params->phy_cfg > 5)
        asr_hw->mod_params->phy_cfg = 2;

    if (asr_hw->mod_params->mcs_map < 0 || asr_hw->mod_params->mcs_map > 2)
        asr_hw->mod_params->mcs_map = 0;

    mdm_phy_cfg = __MDM_PHYCFG_FROM_VERS(asr_hw->version_cfm.version_phy_1);

    if (mdm_phy_cfg == MDM_PHY_CONFIG_TRIDENT)
    {
        struct asr_phy_conf_file phy_conf;
        // Retrieve the Trident configuration
        asr_parse_phy_configfile(asr_hw, &phy_conf);
        memcpy(&asr_hw->phy_config, &phy_conf.trd, sizeof(phy_conf.trd));
    } else if (mdm_phy_cfg == MDM_PHY_CONFIG_ELMA) {
    } else if (mdm_phy_cfg == MDM_PHY_CONFIG_KARST)
    {
        struct asr_phy_conf_file phy_conf;
        // We use the NSS parameter as is
        // Retrieve the Karst configuration
        asr_parse_phy_configfile(asr_hw, &phy_conf);

        memcpy(&asr_hw->phy_config, &phy_conf.karst, sizeof(phy_conf.karst));
    } else {

    }

    nss = asr_hw->mod_params->nss;


    /* VHT capabilities */
    /*
     * MCS map:
     * This capabilities are filled according to the mcs_map module parameter.
     * However currently we have some limitations due to FPGA clock constraints
     * that prevent always using the range of MCS that is defined by the
     * parameter:
     *   - in RX, 2SS, we support up to MCS7
     *   - in TX, 2SS, we support up to MCS8
     */

    for (i = 0; i < nss; i++)
        band_2GHz->ht_cap.mcs.rx_mask[i] = 0xFF;

    /* HT capabilities */
    band_2GHz->ht_cap.cap |= 1 << IEEE80211_HT_CAP_RX_STBC_SHIFT;

    if (asr_hw->mod_params->use_2040)
    {
        band_2GHz->ht_cap.mcs.rx_mask[4] = 0x1; /* MCS32 */
        band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_SUP_WIDTH_20_40;
        band_2GHz->ht_cap.mcs.rx_highest = cpu_to_le16(135 * nss);
    } else
    {
        band_2GHz->ht_cap.mcs.rx_highest = cpu_to_le16(65 * nss);
    }
    if (nss > 1)
        band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_TX_STBC;

    if (asr_hw->mod_params->sgi)
    {
        band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_SGI_20;
        if (asr_hw->mod_params->use_2040)
        {
            band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_SGI_40;
            band_2GHz->ht_cap.mcs.rx_highest = cpu_to_le16(150 * nss);
        } else
            band_2GHz->ht_cap.mcs.rx_highest = cpu_to_le16(72 * nss);
    }

    band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_GRN_FLD;
    if (!asr_hw->mod_params->ht_on)
        band_2GHz->ht_cap.ht_supported = false;

    /**
     * adjust caps with lower layers asr_hw->version_cfm
     */

#ifdef CONFIG_ASR595X
    /* Set HE capabilities */
    asr_set_he_capa(asr_hw, wiphy);
#endif

    return 0;
}

asr_thread_hand_t  uwifi_sdio_task_handle={0};
asr_thread_hand_t  uwifi_rx_to_os_task_handle={0};
OSStatus init_sdio_task(struct asr_hw *asr_hw)
{
    OSStatus status = kNoErr;
    asr_task_config_t cfg;

    status = uwifi_sdio_event_init();
    if(status != kNoErr)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"OS Error:asr_rtos_create_event");
        return status;
    }
    if (kNoErr != asr_rtos_task_cfg_get(ASR_TASK_CONFIG_UWIFI_SDIO, &cfg)) {
        dbg(D_ERR,D_UWIFI_CTRL,"get uwifi sdio task information fail");
        return -1;
    }
    status = asr_rtos_create_thread(&uwifi_sdio_task_handle, cfg.task_priority,UWIFI_SDIO_TASK_NAME, uwifi_sdio_main,
                                      cfg.stack_size, (asr_thread_arg_t)asr_hw);
    if(status != kNoErr)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"OS Error:asr_rtos_create_thread");
        return status;
    }

    return status;
}

OSStatus deinit_sdio_task(void)
{
    OSStatus status = kNoErr;

    status = asr_rtos_delete_thread(&uwifi_sdio_task_handle);
    if(status != kNoErr)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"[%s]OS Error:asr_rtos_delete_thread",__func__);
        return status;
    }
    dbg(D_ERR,D_UWIFI_CTRL,"thread uwifi_sdio_main deleted");

    status = uwifi_sdio_event_deinit();
    if(status != kNoErr)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"OS Error:asr_rtos_deinit_event");
        return status;
    }

    return status;
}
OSStatus init_rx_to_os(struct asr_hw *asr_hw)
{
    OSStatus status = kNoErr;
    asr_task_config_t cfg;

    status = uwifi_rx_to_os_msg_queue_init();
    if(status != kNoErr)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"OS Error:uwifi_rx_to_os_msg_queue_init");
        return status;
    }
    if (kNoErr != asr_rtos_task_cfg_get(ASR_TASK_CONFIG_UWIFI_RX_TO_OS, &cfg)) {
        dbg(D_ERR,D_UWIFI_CTRL,"get uwifi rx to os task information fail");
        return -1;
    }
    status = asr_rtos_create_thread(&uwifi_rx_to_os_task_handle, cfg.task_priority,UWIFI_RX_TO_OS_TASK_NAME, uwifi_rx_to_os_main,
                                      cfg.stack_size, (asr_thread_arg_t)asr_hw);
    if(status != kNoErr)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"OS Error:asr_rtos_create_thread");
        uwifi_rx_to_os_msg_queue_deinit();
        return status;
    }

    return status;

}
OSStatus deinit_rx_to_os(void)
{
    OSStatus status = kNoErr;

    status = asr_rtos_delete_thread(&uwifi_rx_to_os_task_handle);
    if(status != kNoErr)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"[%s]OS Error:asr_rtos_delete_thread",__func__);
        return status;
    }

    status = uwifi_rx_to_os_msg_queue_deinit();
    if(status != kNoErr)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"OS Error:uwifi_rx_to_os_msg_queue_deinit");
        return status;
    }

    return status;
}


