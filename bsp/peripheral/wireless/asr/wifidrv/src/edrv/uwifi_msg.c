/**
 ****************************************************************************************
 *
 * @file uwifi_msg.c
 *
 * @brief main uwifi task rx msg process
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */

#include "uwifi_msg.h"
#include "uwifi_common.h"
#include "uwifi_interface.h"
#include "asr_dbg.h"
#include "asr_sdio.h"
#include "asr_rtos_api.h"
#include "uwifi_hif.h"
#include "uwifi_txq.h"
#include "uwifi_platform.h"
#include "uwifi_ipc_host.h"
#include "uwifi_sdio.h"
#include "uwifi_types.h"
#include "ipc_compat.h"
#include "ipc_host.h"
extern int flow_ctrl_low;
extern int scan_done_flag;


/******************************************************
 *                      Macros
 ******************************************************/
#define UWIFI_QUEUE_SIZE  20*30  //these define uwifi task queue
#define UWIFI_RX_TO_OS_QUEUE_SIZE  10*30  //these define uwifi rx task queue


/******************************************************
 *               Variables Definitions
 ******************************************************/
asr_queue_stuct_t   uwifi_msg_queue;
asr_queue_stuct_t   uwifi_rx_to_os_msg_queue;
asr_event_t   uwifi_sdio_event;

#ifdef CFG_STA_AP_COEX
bool asr_xmit_opt = false;   // true: tx pkt use list instead of tx agg buf.
bool mrole_enable = true;   // true: support sta+softap coexist.
#else
bool asr_xmit_opt = false;
bool mrole_enable = false;
#endif

int tx_debug = 2;
uint32_t asr_data_tx_pkt_cnt;
uint32_t asr_rx_pkt_cnt;

/******************************************************
 *               Function Definitions
 ******************************************************/

u32 bitcount(u32 num)
{
    u32 count = 0;
    static u32 nibblebits[] = {
        0, 1, 1, 2, 1, 2, 2, 3,
        1, 2, 2, 3, 2, 3, 3, 4
    };
    for (; num != 0; num >>= 4)
        count += nibblebits[num & 0x0f];
    return count;
}

OSStatus uwifi_msg_queue_init(void)
{
    return asr_rtos_init_queue(&uwifi_msg_queue, "UWIFI_TASK_QUEUE", sizeof(struct uwifi_task_msg_t), UWIFI_QUEUE_SIZE);
}

OSStatus uwifi_msg_queue_deinit(void)
{
    return asr_rtos_deinit_queue(&uwifi_msg_queue);
}

OSStatus uwifi_rx_to_os_msg_queue_init(void)
{
    return asr_rtos_init_queue(&uwifi_rx_to_os_msg_queue, "UWIFI_RX_TO_OS_TASK_QUEUE", sizeof(struct uwifi_task_msg_t), UWIFI_RX_TO_OS_QUEUE_SIZE);
}

OSStatus uwifi_rx_to_os_msg_queue_deinit(void)
{
    return asr_rtos_deinit_queue(&uwifi_rx_to_os_msg_queue);
}

OSStatus uwifi_sdio_event_init(void)
{
    return asr_rtos_create_event(&uwifi_sdio_event);
}

OSStatus uwifi_sdio_event_deinit(void)
{
    return asr_rtos_deinit_event(&uwifi_sdio_event);
}

OSStatus uwifi_sdio_event_set(uint32_t mask)
{
    return asr_rtos_set_event(&uwifi_sdio_event, mask);
}

OSStatus uwifi_sdio_event_clear(uint32_t mask)
{
    return asr_rtos_clear_event(&uwifi_sdio_event, mask);
}

OSStatus uwifi_sdio_event_wait(uint32_t mask, uint32_t *rcv_flags, uint32_t timeout)
{
    return asr_rtos_wait_for_event(&uwifi_sdio_event, mask, rcv_flags, timeout);
}
//handle message which is from other task,such as AT/LWIP
void uwifi_msg_handle(void)
{
    uint32_t ret;
    if(sp_asr_hw == NULL)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "wifi open for the first time");
        sp_asr_hw = uwifi_platform_init();
    }

    while(SCAN_DEINIT == scan_done_flag)
        asr_rtos_delay_milliseconds(5);

    while(SCAN_DEINIT != scan_done_flag)
    {
        struct uwifi_task_msg_t msg={0};
        ret = asr_rtos_pop_from_queue(&uwifi_msg_queue, &msg, ASR_WAIT_FOREVER);
        if (ret)
            dbg(D_ERR, D_UWIFI_CTRL, "pop from queue failed\r\n");
        uwifi_main_process_msg(&msg);
    }
}

#ifdef APM_AGING_SUPPORT
void uwifi_msg_l2u_msg_deauth(uint8_t *buf)
{
    struct uwifi_task_msg_t l2u_msg={F2D_MSG_DEAUTH,0,0,0};
    l2u_msg.param1 = (uint32_t)buf;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NEVER_TIMEOUT);
}
#endif


/********************************************
lwip send to uwifi
*********************************************/
int uwifi_tx_skb_data(struct sk_buff *skb, struct asr_vif *asr_vif)
{
    return uwifi_dev_transmit_skb(skb, asr_vif);
}

/********************************************
msg private implement
*********************************************/
void uwifi_msg_lwip2u_ip_got(uint32_t ipstat)
{
    struct uwifi_task_msg_t lwip2u_msg={LWIP2U_IP_GOT, 0, 0, 0};
    lwip2u_msg.param1 = ipstat;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &lwip2u_msg, ASR_NEVER_TIMEOUT);
}

void uwifi_msg_lwip2u_tx_skb_data(struct sk_buff *skb)
{
    struct uwifi_task_msg_t l2u_msg={LWIP2D_DATA_TX_SKB,0,0, 0};
    l2u_msg.param1 = (uint32_t)skb;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NEVER_TIMEOUT);
}

void uwifi_msg_aos2u_sta_start(void* p_config)
{
    uint32_t ret;
    struct uwifi_task_msg_t aos2u_msg={AOS2U_STA_START,0,0, 0};
    aos2u_msg.param1 = (uint32_t)p_config;
    ret = asr_rtos_push_to_queue(&uwifi_msg_queue, &aos2u_msg, ASR_NEVER_TIMEOUT);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "push to queue failed\r\n");
        return;
    }
}

void uwifi_msg_rxu2u_wpa_info_clear(void* asr_vif, uint32_t security)
{
    struct uwifi_task_msg_t rxu2u_msg={RXU2U_WPA_INFO_CLEAR,0,0, 0};
    rxu2u_msg.param1 = (uint32_t)asr_vif;
    rxu2u_msg.param2 = security;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &rxu2u_msg, ASR_NEVER_TIMEOUT);
}

void uwifi_msg_at2u_open_mode(enum open_mode openmode,struct softap_info_t* softap_info)
{
    if(openmode==SAP_MODE)
    dbg(D_INF, D_UWIFI_CTRL, "%s done softap_info->ssid_len=%d\r\n",__func__,softap_info->ssid_len);
    struct uwifi_task_msg_t l2u_msg={AT2D_OPEN_MODE,0,0, 0};
    l2u_msg.param1 = openmode;
    l2u_msg.param2 = (uint32_t)softap_info;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NEVER_TIMEOUT);
}

#ifdef CFG_SNIFFER_SUPPORT
void uwifi_msg_at2u_open_monitor_mode(void)
{
    struct uwifi_task_msg_t at2u_msg={AT2D_OPEN_MONITOR_MODE,0,0, 0};
    asr_rtos_push_to_queue(&uwifi_msg_queue, &at2u_msg, ASR_NEVER_TIMEOUT);
}
#endif

//#ifdef CFG_STATION_SUPPORT
void uwifi_msg_at2u_set_ps_mode(uint8_t ps_on)
{
    struct uwifi_task_msg_t at2u_msg={AT2D_SET_PS_MODE, 0, 0, 0};
    at2u_msg.param1 = (uint32_t)ps_on;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &at2u_msg, ASR_NEVER_TIMEOUT);
}

void uwifi_msg_at2u_scan_req(struct wifi_scan_t* wifi_scan)
{
    struct uwifi_task_msg_t l2u_msg={AT2D_SCAN_REQ,0,0, 0};
    l2u_msg.param1 = (uint32_t)wifi_scan;
    if (wifi_scan->ssid)
    {
        l2u_msg.param2 = (uint32_t)(wifi_scan->ssid);
    }
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NEVER_TIMEOUT);
}

void uwifi_msg_at2u_connect_req(struct wifi_conn_t *wifi_con)
{
    struct uwifi_task_msg_t l2u_msg={AT2D_CONNECT_REQ,0,0, 0};
    l2u_msg.param1 = (uint32_t)wifi_con;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NEVER_TIMEOUT);
}

void uwifi_msg_at2u_connect_adv_req(struct wifi_conn_t *wifi_con)
{
    struct uwifi_task_msg_t l2u_msg={AT2D_CONNECT_ADV_REQ,0,0, 0};
    l2u_msg.param1 = (uint32_t)wifi_con;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NEVER_TIMEOUT);
}

void uwifi_msg_at2u_disconnect(enum reasoncode reason)
{
    struct uwifi_task_msg_t l2u_msg={AT2D_DISCONNECT,0,0, 0};
    l2u_msg.param1 = (uint32_t)reason;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NEVER_TIMEOUT);
}

void uwifi_msg_at2u_tx_power(uint8_t power, uint32_t iftype)
{
    struct uwifi_task_msg_t l2u_msg={AT2D_TX_POWER,0,0, 0};
    l2u_msg.param1 = power;
    l2u_msg.param2 = iftype;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NEVER_TIMEOUT);
}
//#endif


void uwifi_msg_lwip2u_peer_sta_ip_update(uint32_t ipstat,uint32_t macstat, uint32_t net_if)
{
    struct uwifi_task_msg_t lwip2u_msg={LWIP2U_STA_IP_UPDATE, 0, 0, 0};
    lwip2u_msg.param1 = ipstat;
    lwip2u_msg.param2 = macstat;
    lwip2u_msg.param3 = net_if;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &lwip2u_msg, ASR_NEVER_TIMEOUT);
}

void uwifi_msg_at2u_close(uint32_t iftype)
{
    struct uwifi_task_msg_t l2u_msg={AT2D_CLOSE,iftype,0, 0};
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NEVER_TIMEOUT);
}

#ifdef CFG_SOFTAP_SUPPORT
void uwifi_msg_rxu2u_assocrsp_tx_cfm(uint32_t is_success, uint32_t param)
{
    struct uwifi_task_msg_t urx2u_msg={RXU2U_ASSOCRSP_TX_CFM,0,0, 0};
    urx2u_msg.param1 = is_success;
    urx2u_msg.param2 = param;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &urx2u_msg, ASR_NEVER_TIMEOUT);
}

void uwifi_msg_rxu2u_deauth_tx_cfm(uint32_t is_success, uint32_t param)
{
    struct uwifi_task_msg_t urx2u_msg={RXU2U_DEAUTH_TX_CFM,0,0, 0};
    urx2u_msg.param1 = is_success;
    urx2u_msg.param2 = param;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &urx2u_msg, ASR_NEVER_TIMEOUT);
}

void uwifi_msg_rxu2u_rx_deauth(uint32_t param)
{
    struct uwifi_task_msg_t urx2u_msg={RXU2U_RX_DEAUTH,0,0, 0};
    urx2u_msg.param1 = param;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &urx2u_msg, ASR_NEVER_TIMEOUT);
}
#endif

void uwifi_msg_timer2u_autoconnect_sta(uint32_t param)
{
    struct uwifi_task_msg_t urx2u_msg = {TIMER2U_AUTOCONNECT_STA, 0, 0, 0};
    urx2u_msg.param1 = param;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &urx2u_msg, ASR_NO_WAIT);
}

#ifdef CFG_SNIFFER_SUPPORT
void uwifi_msg_at2u_sniffer_register_cb(monitor_cb_t p_fn)
{
    struct uwifi_task_msg_t urx2u_msg = {AT2D_SNIFFER_REGISTER_CB, 0, 0, 0};
    urx2u_msg.param1 = (uint32_t)p_fn;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &urx2u_msg, ASR_NEVER_TIMEOUT);
}

void uwifi_msg_at2u_set_filter_type(uint32_t type)
{
    struct uwifi_task_msg_t urx2u_msg = {AT2D_SET_FILTER_TYPE, 0, 0, 0};
    urx2u_msg.param1 = type;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &urx2u_msg, ASR_NEVER_TIMEOUT);
}
#endif

#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
void uwifi_msg_at2u_send_cus_mgmtframe(uint8_t *pframe, uint32_t len)
{
    struct uwifi_task_msg_t l2u_msg={AT2D_CUS_FRAME,0,0, 0};
    l2u_msg.param1 = (uint32_t)pframe;
    l2u_msg.param2= len;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NEVER_TIMEOUT);
}
#endif

#ifdef CFG_ENCRYPT_SUPPORT
void uwifi_msg_rxu2u_eapol_packet_process(uint8_t *data, uint32_t data_len, uint32_t mode)
{
    struct uwifi_task_msg_t urx2u_msg = {RXU2U_EAPOL_PACKET_PROCESS, 0, 0, 0};
    urx2u_msg.param1 = (uint32_t)data;
    urx2u_msg.param2 = data_len;
    urx2u_msg.param3 = mode;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &urx2u_msg, ASR_NEVER_TIMEOUT);
}
#endif

#ifdef CONFIG_SME
void uwifi_msg_rxu2u_sme_process(struct asr_vif *asr_vif, void *req, uint32_t mode)
{
    struct uwifi_task_msg_t urx2u_msg = {RXU2U_SME_PACKET_PROCESS, 0, 0, 0};
    urx2u_msg.param1 = (uint32_t)asr_vif;
    urx2u_msg.param2 = (uint32_t)req;
    urx2u_msg.param3 = mode;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &urx2u_msg, ASR_NEVER_TIMEOUT);
}
#endif

//#ifdef CFG_ENCRYPT_SUPPORT
void uwifi_msg_rxu2u_handshake_start(uint32_t mode)
{
    struct uwifi_task_msg_t urx2u_msg = {RXU2U_HANDSHAKE_START, 0, 0, 0};
    urx2u_msg.param1 = mode;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &urx2u_msg, ASR_NEVER_TIMEOUT);
}
void uwifi_msg_at2u_sta_handshake_done(void)
{
    struct uwifi_task_msg_t l2u_msg={RXU2U_STA_HANDSHAKE_DONE,0,0,0};
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NEVER_TIMEOUT);
}

void uwifi_msg_timer2u_eapol_resend(uint32_t arg)
{
    struct uwifi_task_msg_t l2u_msg={TIMER2U_EAPOL_RESEND,0,0, 0};
    l2u_msg.param1 = arg;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NO_WAIT);
}

void uwifi_msg_timer2u_wpa_complete_to(uint32_t arg)
{
    struct uwifi_task_msg_t l2u_msg={TIMER2U_WPA_COMPLETE_TO,0,0, 0};
    l2u_msg.param1 = arg;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NO_WAIT);
}
//#endif

#ifdef CFG_SOFTAP_SUPPORT
// new add softap auth/assoc timeout msg.
void uwifi_msg_at2u_softap_auth_to(uint32_t arg)
{
    struct uwifi_task_msg_t l2u_msg={TIMER2U_SOFTAP_AUTH_TO,0,0, 0};
    l2u_msg.param1 = arg;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NEVER_TIMEOUT);
}

void uwifi_msg_at2u_softap_assoc_to(uint32_t arg)
{
    struct uwifi_task_msg_t l2u_msg={TIMER2U_SOFTAP_ASSOC_TO,0,0, 0};
    l2u_msg.param1 = arg;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NEVER_TIMEOUT);
}
#endif

void uwifi_msg_at2u_sta_sa_query_to(uint32_t arg)
{
    struct uwifi_task_msg_t l2u_msg={TIMER2U_STA_SA_QUERY_TO,0,0, 0};
    l2u_msg.param1 = arg;
    asr_rtos_push_to_queue(&uwifi_msg_queue, &l2u_msg, ASR_NEVER_TIMEOUT);
}



/********************************************
AT/other app task send to uwifi
*********************************************/
void uwifi_open_wifi_mode(enum open_mode openmode,struct softap_info_t* softap_info)
{
#ifdef CFG_SNIFFER_SUPPORT
    if(openmode == SNIFFER_MODE)
        uwifi_msg_at2u_open_monitor_mode();
    else
#endif
        uwifi_msg_at2u_open_mode(openmode,softap_info);
}

void uwifi_close_wifi(uint32_t iftype)
{
    uwifi_msg_at2u_close(iftype);
}

#ifdef CFG_STATION_SUPPORT
void uwifi_scan_req(struct wifi_scan_t* wifi_scan)
{
    uwifi_msg_at2u_scan_req(wifi_scan);
}

void uwifi_connect_req(struct wifi_conn_t *wifi_con)
{
    uwifi_msg_at2u_connect_req(wifi_con);
}

void uwifi_disconnect(void)
{
    uwifi_msg_at2u_disconnect(DISCONNECT_REASON_USER_GENERATE);
}

void uwifi_set_tx_power(uint8_t power, uint32_t iftype)
{
    uwifi_msg_at2u_tx_power(power, iftype);
}
#endif

#ifdef CFG_SNIFFER_SUPPORT
void uwifi_sniffer_register_cb(monitor_cb_t p_fn)
{
    uwifi_msg_at2u_sniffer_register_cb(p_fn);
}

void uwifi_set_filter_type(uint32_t type)
{
    uwifi_msg_at2u_set_filter_type(type);
}
#endif

#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
uint8_t uwifi_tx_custom_mgmtframe(uint8_t *pframe, uint32_t len)
{
    uint8_t *uwifi_pframe;
    uwifi_pframe = asr_rtos_malloc(len);
    if(uwifi_pframe == NULL)
    {
        return -1;
    }
    memcpy(uwifi_pframe, pframe,len);
    uwifi_msg_at2u_send_cus_mgmtframe(uwifi_pframe,len);
    return 0;
}
#endif

void uwifi_msg_rx_to_os_queue(void)
{
    struct uwifi_task_msg_t msg = {0};
    asr_rtos_push_to_queue(&uwifi_rx_to_os_msg_queue,&msg,ASR_WAIT_FOREVER);
}

void asr_tx_flow_ctrl_stop(struct asr_hw *asr_hw, struct asr_txq *txq)
{
    /* restart netdev queue if number of queued buffer is below threshold */
    //pr_err("-flowctrl %d\n",txq->ndev_idx);
    txq->status &= ~ASR_TXQ_NDEV_FLOW_CTRL;
    //netif_wake_subqueue(txq->ndev, txq->ndev_idx);
}

void asr_tx_flow_ctrl_start(struct asr_hw *asr_hw, struct asr_txq *txq)
{
    //pr_err("+flowctrl %d\n",txq->ndev_idx);
    txq->status |= ASR_TXQ_NDEV_FLOW_CTRL;
    //netif_stop_subqueue(txq->ndev, txq->ndev_idx);
}

u16 asr_tx_aggr_get_ava_trans_num(struct asr_hw *asr_hw, uint16_t * total_ava_num)
{
    struct asr_tx_agg *tx_agg_env = &asr_hw->tx_agg_env;
    u16 ret = 0;

    asr_rtos_lock_mutex(&asr_hw->tx_agg_env_lock);
    *total_ava_num = (tx_agg_env->aggr_buf_cnt);

    if (asr_hw->tx_agg_env.last_aggr_buf_next_addr >= asr_hw->tx_agg_env.cur_aggr_buf_next_addr) {
        ret = *total_ava_num;
    } else {
        if (tx_agg_env->aggr_buf_cnt > tx_agg_env->last_aggr_buf_next_idx)
            ret = tx_agg_env->aggr_buf_cnt - tx_agg_env->last_aggr_buf_next_idx;
        else {
            ret = tx_agg_env->aggr_buf_cnt;
        }
    }

    if (tx_agg_env->aggr_buf_cnt == tx_agg_env->last_aggr_buf_next_idx &&
        asr_hw->tx_agg_env.last_aggr_buf_next_addr < asr_hw->tx_agg_env.cur_aggr_buf_next_addr) {
        tx_agg_env->cur_aggr_buf_next_idx = 0;
        tx_agg_env->cur_aggr_buf_next_addr = (uint8_t *) tx_agg_env->aggr_buf->data;
    }
    asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);

    return ret;

}
extern bool arp_debug_log;
extern bool txlogen;

#ifdef CONFIG_ASR_KEY_DBG
u32 tx_agg_port_num[8] = { 0 };
u32 rx_agg_port_num[8] = { 0 };
void tx_agg_port_stats(u8 avail_data_port)
{
    if (avail_data_port > 0 && avail_data_port < 9) {
        tx_agg_port_num[avail_data_port - 1]++;
    } else {
        //dbg(D_INF, D_UWIFI_CTRL, "wrong avail_data_port %d\n", avail_data_port);
    }
}

void rx_aggr_stat(u8 aggr_num)
{
    // legal value: 1~8
    if (aggr_num > 0 && aggr_num < 9) {
        rx_agg_port_num[aggr_num - 1]++;
    } else {
        dbg(D_ERR, D_UWIFI_CTRL, "wrong aggr_num %d\n", aggr_num);
    }
}
#endif

extern bool check_vif_block_flags(struct asr_vif *asr_vif);

bool check_sta_ps_state(struct asr_sta *sta)
{
      if (sta == NULL)
          return false;
      else
          return (sta->ps.active);
}

u16 asr_tx_get_ava_trans_skbs(struct asr_hw *asr_hw)
{
    struct sk_buff *skb_pending;
    struct sk_buff *pnext;
    struct hostdesc * hostdesc_tmp = NULL;
    struct asr_vif *asr_vif_tmp = NULL;
    struct asr_sta *asr_sta_tmp = NULL;
    u16 total_ava_skbs = 0;

    // loop list and get avail skbs.
    skb_queue_walk_safe(&asr_hw->tx_sk_list, skb_pending, pnext) {
        hostdesc_tmp = (struct hostdesc *)skb_pending->data;
        if ((hostdesc_tmp->vif_idx < asr_hw->vif_max_num)) {
            asr_vif_tmp = asr_hw->vif_table[hostdesc_tmp->vif_idx];
            asr_sta_tmp = (hostdesc_tmp->staid < (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX)) ? &asr_hw->sta_table[hostdesc_tmp->staid] : NULL;

            if (asr_vif_tmp && (check_vif_block_flags(asr_vif_tmp) == false)
                            && (check_sta_ps_state(asr_sta_tmp) == false)) {
                total_ava_skbs++;
            }
        } else {
                dbg(D_ERR, D_UWIFI_CTRL, " unlikely: mrole tx(vif_idx %d not valid)!!!\r\n",hostdesc_tmp->vif_idx);
        }
    }

    return total_ava_skbs;

}

void asr_tx_get_hif_skb_list(struct asr_hw *asr_hw,u8 port_num)
{
    struct sk_buff *skb_pending;
    struct sk_buff *pnext;
    struct hostdesc * hostdesc_tmp = NULL;
    struct asr_vif *asr_vif_tmp = NULL;
    struct asr_sta *asr_sta_tmp = NULL;

    skb_queue_walk_safe(&asr_hw->tx_sk_list, skb_pending, pnext) {
        hostdesc_tmp = (struct hostdesc *)skb_pending->data;

        if ((hostdesc_tmp->vif_idx < asr_hw->vif_max_num)) {
            asr_vif_tmp = asr_hw->vif_table[hostdesc_tmp->vif_idx];
            asr_sta_tmp = (hostdesc_tmp->staid < (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX)) ? &asr_hw->sta_table[hostdesc_tmp->staid] : NULL;
            if (asr_vif_tmp && (check_vif_block_flags(asr_vif_tmp) == false)
                            && (check_sta_ps_state(asr_sta_tmp) == false)) {
                skb_unlink(skb_pending, &asr_hw->tx_sk_list);
                skb_queue_tail(&asr_hw->tx_hif_skb_list, skb_pending);
                port_num-- ;

                if (port_num == 0)
                    break;
            } else {
                   if (hostdesc_tmp->flags & TXU_CNTRL_MGMT) {

                         dbg(D_ERR, D_UWIFI_CTRL," bypass mgt mrole tx(vif_idx = %d sta=%d, )(%d %d), vif_tmp=%p ,flags=0x%lx !!!\r\n",
                                                                              hostdesc_tmp->vif_idx,hostdesc_tmp->staid,
                                                                             check_sta_ps_state(asr_sta_tmp), check_vif_block_flags(asr_vif_tmp),
                                                                             asr_vif_tmp,
                                                                             asr_vif_tmp ? asr_vif_tmp->dev_flags: 0x0);
                   }
            }
        } else {
                dbg(D_ERR, D_UWIFI_CTRL," unlikely: mrole tx(vif_idx %d not valid)!!!\r\n",hostdesc_tmp->vif_idx);
        }

    }

    if (port_num)
        dbg(D_ERR, D_UWIFI_CTRL," unlikely: remain avail port error (%d) !!!\r\n",port_num);

}


u16 asr_hif_bufdata_prepare(struct asr_hw *asr_hw,struct sk_buff *hif_buf_skb,u8 port_num,u32 *tx_bytes)
{
    struct sk_buff *skb_pending;
    struct sk_buff *pnext;
    struct hostdesc * hostdesc_tmp = NULL;
    u16 trans_len = 0;
    u16 temp_len;
    //struct asr_vif *asr_vif_tmp = NULL;
    struct hostdesc *hostdesc_start = NULL;
    u8 hif_skb_copy_cnt = 0;

    asr_data_tx_pkt_cnt += port_num;

    skb_queue_walk_safe(&asr_hw->tx_hif_skb_list, skb_pending, pnext)
    {
        hostdesc_tmp = (struct hostdesc *)skb_pending->data;

        temp_len = hostdesc_tmp->sdio_tx_len + 2;

        if (temp_len % 32 || temp_len > ASR_SDIO_DATA_MAX_LEN || (hostdesc_tmp->vif_idx >= asr_hw->vif_max_num)) {
            dbg(D_ERR, D_UWIFI_CTRL,"unlikely: len:%d not 32 aligned, port_num=%d ,vif_idx=%d , drop \r\n",temp_len,port_num,hostdesc_tmp->vif_idx);

            //WARN_ON(1);

            // directly free : not rechain when sdio send fail.
            skb_unlink(skb_pending, &asr_hw->tx_hif_skb_list);
            //dev_kfree_skb_any(skb_pending);
            asr_recycle_tx_skb(asr_hw,skb_pending);
            break;

        } else {
            // copy to hifbuf.
            memcpy(hif_buf_skb->data + trans_len, skb_pending->data ,temp_len);
            trans_len += temp_len;
            *tx_bytes += hostdesc_tmp->packet_len;
            hif_skb_copy_cnt++;

            if (txlogen)
                dbg(D_ERR, D_UWIFI_CTRL,"hif_copy: %d %d ,%d %d \r\n",temp_len,trans_len,hif_skb_copy_cnt,port_num);

        }
    }

        if (port_num != hif_skb_copy_cnt) {
            dbg(D_ERR, D_UWIFI_CTRL," unlikely: port num mismatch(%d %d)!!!\r\n",port_num,hif_skb_copy_cnt);
            trans_len = 0;
            *tx_bytes = 0;
        } else {
        hostdesc_start = (struct hostdesc *)(hif_buf_skb->data);
        hostdesc_start->agg_num = hif_skb_copy_cnt;
    }

    return trans_len;

}


void asr_drop_tx_vif_skb(struct asr_hw *asr_hw,struct asr_vif *asr_vif_drop)
{
    struct sk_buff *skb_pending;
    struct sk_buff *pnext;
    struct hostdesc * hostdesc_tmp = NULL;
    //struct asr_vif *asr_vif_tmp = NULL;

    skb_queue_walk_safe(&asr_hw->tx_sk_list, skb_pending, pnext) {
        hostdesc_tmp = (struct hostdesc *)skb_pending->data;
        if ((asr_vif_drop == NULL) ||
            (asr_vif_drop && (hostdesc_tmp->vif_idx == asr_vif_drop->vif_index))) {
            skb_unlink(skb_pending, &asr_hw->tx_sk_list);
            //dev_kfree_skb_any(skb_pending);
            asr_recycle_tx_skb(asr_hw,skb_pending);
        }
    }
}

void asr_tx_skb_sta_deinit(struct asr_hw *asr_hw,struct asr_sta *asr_sta_drop)
{
    struct sk_buff *skb_pending;
    struct sk_buff *pnext;
    struct hostdesc * hostdesc_tmp = NULL;

    skb_queue_walk_safe(&asr_hw->tx_sk_list, skb_pending, pnext) {
        hostdesc_tmp = (struct hostdesc *)skb_pending->data;
        if (asr_sta_drop && hostdesc_tmp && (hostdesc_tmp->staid == asr_sta_drop->sta_idx)) {
            skb_unlink(skb_pending, &asr_hw->tx_sk_list);
            //dev_kfree_skb_any(skb_pending);
            asr_recycle_tx_skb(asr_hw,skb_pending);
        }
    }
}


int asr_opt_tx_task(struct asr_hw *asr_hw)
{
    u8 avail_data_port;
    int ret;
    u8 port_num = 0;
    u16 ava_skb_num = 0;
    u16 trans_len = 0;
    struct hostdesc *hostdesc_start = NULL;
    struct hostdesc *hostdesc_tmp = NULL;
    int total_tx_num = 0;

    struct sk_buff *hif_buf_skb = NULL;
    struct sk_buff *hif_tx_skb = NULL;
    struct sk_buff *pnext = NULL;

    unsigned int io_addr;
    u32 tx_bytes = 0;
    struct asr_vif *asr_traffic_vif = NULL;
    struct asr_vif *asr_ext_vif = NULL;
    struct asr_vif *asr_vif_tmp = NULL;

    u16 bitmap_record;

    if (asr_hw == NULL)
       return 0;

    //tx_task_run_cnt++;

    while (1) {
        /************************** phy status check. **************************************************************/
        // check traffic vif status.
        if (mrole_enable && (asr_hw->vif_started > 1)) {

            if (asr_hw->sta_vif_idx < asr_hw->vif_max_num) {
                asr_traffic_vif  = asr_hw->vif_table[asr_hw->sta_vif_idx];
            }

            if (asr_hw->ap_vif_idx < asr_hw->vif_max_num) {
                asr_ext_vif  = asr_hw->vif_table[asr_hw->ap_vif_idx];
            }

        } else {
            if (asr_hw->sta_vif_idx < asr_hw->vif_max_num) {
                asr_traffic_vif  = asr_hw->vif_table[asr_hw->sta_vif_idx];
            } else if (asr_hw->ap_vif_idx < asr_hw->vif_max_num) {
                asr_traffic_vif  = asr_hw->vif_table[asr_hw->ap_vif_idx];
            }
        }

        // wifi phy restarting, reset all tx agg buf.
        if (asr_test_bit(ASR_DEV_RESTARTING, &asr_hw->phy_flags)) {
            //struct sk_buff *skb_pending;
            //struct sk_buff *pnext;

            dbg(D_ERR, D_UWIFI_CTRL, "%s:phy drop,phy_flags(%08X),cnt(%u)\n", __func__, (unsigned int)asr_hw->phy_flags, skb_queue_len(&asr_hw->tx_sk_list));

            // todo. drop all skbs in asr_hw->tx_sk_list.
            asr_drop_tx_vif_skb(asr_hw,NULL);
            asr_rtos_lock_mutex(&asr_hw->tx_lock);
            list_for_each_entry(asr_vif_tmp,&asr_hw->vifs,list){
                asr_vif_tmp->tx_skb_cnt = 0;
            }
            asr_rtos_unlock_mutex(&asr_hw->tx_lock);
            break;
        }

        if (asr_traffic_vif && ASR_VIF_TYPE(asr_traffic_vif) == NL80211_IFTYPE_STATION &&
                 (asr_test_bit(ASR_DEV_STA_DISCONNECTING, &asr_traffic_vif->dev_flags) ||
                 (!asr_test_bit(ASR_DEV_STA_CONNECTED, &asr_traffic_vif->dev_flags))  ||         // P2P mask:will send mgt frame from host before connected.
                 asr_test_bit(ASR_DEV_CLOSEING, &asr_traffic_vif->dev_flags) ||
                 asr_test_bit(ASR_DEV_PRECLOSEING, &asr_traffic_vif->dev_flags))) {
                 dbg(D_ERR, D_UWIFI_CTRL, "%s:sta drop,dev_flags(%08X),vif_started_cnt=%d\n",
                     __func__, (unsigned int)asr_traffic_vif->dev_flags,asr_hw->vif_started);

                asr_rtos_lock_mutex(&asr_hw->tx_lock);
                asr_drop_tx_vif_skb(asr_hw,asr_traffic_vif);
                asr_traffic_vif->tx_skb_cnt = 0;
                asr_rtos_unlock_mutex(&asr_hw->tx_lock);

                if (asr_hw->vif_started <= 1)
                    break;
        }

        // check hif buf valid.
        if (skb_queue_empty(&asr_hw->tx_hif_free_buf_list)) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s: tx_hif_skb_list is empty ...\n", __func__);
            break;
        }

        /************************ tx cmd53 prepare and send.****************************************************************/
        asr_rtos_lock_mutex(&asr_hw->tx_lock);           // use lock to prevent asr_vif->dev_flags change.

        ava_skb_num = asr_tx_get_ava_trans_skbs(asr_hw);
        if (!ava_skb_num) {
            asr_rtos_unlock_mutex(&asr_hw->tx_lock);
            break;
        }

        avail_data_port = asr_sdio_tx_get_available_data_port(asr_hw, ava_skb_num, &port_num, &io_addr, &bitmap_record);
#ifdef CONFIG_ASR_KEY_DBG
        tx_agg_port_stats(avail_data_port);
#endif
        if (!avail_data_port) {
            asr_rtos_unlock_mutex(&asr_hw->tx_lock);
            break;
        }

        asr_tx_get_hif_skb_list(asr_hw,port_num);

        asr_rtos_unlock_mutex(&asr_hw->tx_lock);

        // pop free hif buf and move tx skb to hif buf.
        hif_buf_skb = skb_dequeue(&asr_hw->tx_hif_free_buf_list);

        if (hif_buf_skb == NULL) {
              // free asr_hw->tx_hif_skb_list and break
              dbg(D_ERR, D_UWIFI_CTRL, "%s: unlikely , tx_hif_skb_list is empty ...\n", __func__);

              skb_queue_walk_safe(&asr_hw->tx_hif_skb_list, hif_tx_skb, pnext) {
                  skb_unlink(hif_tx_skb, &asr_hw->tx_hif_skb_list);
                  //dev_kfree_skb_any(hif_tx_skb);
                  asr_recycle_tx_skb(asr_hw,hif_tx_skb);
              }
              break;
        }

        // to adapt to sdio aggr tx, better use scatter dma later.
        trans_len = asr_hif_bufdata_prepare(asr_hw,hif_buf_skb,port_num,&tx_bytes);

        if (trans_len) {
            hostdesc_start = (struct hostdesc *)hif_buf_skb->data;
            // send sdio .
            ret = asr_sdio_send_data(asr_hw, HIF_TX_DATA, (u8 *) hostdesc_start, trans_len, io_addr, bitmap_record);    // sleep api, not add in spin lock
            if (ret) {
                if (mrole_enable == false && asr_traffic_vif != NULL) {
                    asr_traffic_vif->net_stats.tx_dropped += hostdesc_start->agg_num;
                }
                dbg(D_ERR, D_UWIFI_CTRL, "%s: unlikely , sdio send fail ,rechain hif_tx_skb...\n", __func__);
            } else {
                if (txlogen)
                    dbg(D_ERR, D_UWIFI_CTRL, "%s: sdio send %d ok(%d %d %d) ,vif_idx=%d, (%d %d %d) 0x%x...\n", __func__,
                                                                          trans_len,
                                                                          ava_skb_num,avail_data_port,port_num,
                                                                          hostdesc_start->vif_idx,
                                                                          hostdesc_start->sdio_tx_len,hostdesc_start->sdio_tx_total_len,hostdesc_start->agg_num,
                                                                          hostdesc_start->ethertype);
            }
        } else {
            ret = -EBUSY;
            dbg(D_ERR, D_UWIFI_CTRL, "%s: unlikely , sdio send ebusy ...\n", __func__);

        }

        // tx_hif_skb_list free, if sdio send fail, need rechain to tx_sk_list
        asr_rtos_lock_mutex(&asr_hw->tx_lock);
        skb_queue_walk_safe(&asr_hw->tx_hif_skb_list, hif_tx_skb, pnext) {
            skb_unlink(hif_tx_skb, &asr_hw->tx_hif_skb_list);
            if (ret) {
                skb_queue_tail(&asr_hw->tx_sk_list, hif_tx_skb);          // sdio send fail
            } else {
                // flow ctrl update.
                hostdesc_tmp = (struct hostdesc *)hif_tx_skb->data;
                if ((hostdesc_tmp->vif_idx < asr_hw->vif_max_num)) {
                    asr_vif_tmp = asr_hw->vif_table[hostdesc_tmp->vif_idx];

                    if (asr_vif_tmp)
                        asr_vif_tmp->tx_skb_cnt --;
                }
                    //dev_kfree_skb_any(hif_tx_skb);
                    asr_recycle_tx_skb(asr_hw,hif_tx_skb);
            }
        }
        asr_rtos_unlock_mutex(&asr_hw->tx_lock);

            /************************ tx stats and flow ctrl.****************************************************************/
        if (mrole_enable == false && asr_traffic_vif != NULL)
        {
            asr_traffic_vif->net_stats.tx_packets += hostdesc_start->agg_num;
            asr_traffic_vif->net_stats.tx_bytes += tx_bytes;
        }

        asr_hw->stats.last_tx = asr_rtos_get_time();
#ifdef ASR_STATS_RATES_TIMER
        asr_hw->stats.tx_bytes += tx_bytes;
#endif

        // clear hif buf and rechain to free hif_buf list.
        memset(hif_buf_skb->data, 0, IPC_HIF_TXBUF_SIZE);
        // Add the sk buffer structure in the table of rx buffer
        skb_queue_tail(&asr_hw->tx_hif_free_buf_list, hif_buf_skb);

        // tx flow ctrl part.
        //list_for_each_entry(asr_vif_tmp, &asr_hw->vifs, list) {
        //    asr_tx_flow_ctrl(asr_hw,asr_vif_tmp,false);
        //}

        if (skb_queue_len(&asr_hw->tx_sk_list) == 0)
            asr_hw->xmit_first_trigger_flag = false;

        total_tx_num += hostdesc_start->agg_num;

    }

    return total_tx_num;
}

int asr_tx_task(struct asr_hw *asr_hw)
{
    u8 avail_data_port;
    int ret;
    struct asr_tx_agg *tx_agg_env = NULL;
    u8 port_num = 0;
    u16 ava_agg_num = 0;
    u16 trans_len = 0;
    u16 temp_len;
    struct hostdesc *hostdesc_start = NULL;
    u8 *temp_cur_next_addr;
    u16 temp_cur_next_idx;
    u16 total_ava_num;
    int total_tx_num = 0;
    unsigned int io_addr;
    u32 tx_bytes = 0;
    //uint8_t tx_cur_idx;
    struct hostdesc *hostdesc_temp = NULL;
    struct asr_vif *asr_vif_tmp = NULL;
    struct asr_vif *asr_traffic_vif = NULL;
    struct asr_vif *asr_ext_vif = NULL;
    uint32_t host_sdio_retry = 0;
    u16 bitmap_record;

    if (asr_hw == NULL)
       return 0;

    tx_agg_env = &(asr_hw->tx_agg_env);


    //tx_task_run_cnt++;

    while (1) {

        /************************** dev & vif stats check. **************************************************************/
        // check traffic vif status.
        if (mrole_enable && (asr_hw->vif_started > 1)) {

            if (asr_hw->sta_vif_idx < asr_hw->vif_max_num) {
                asr_traffic_vif  = asr_hw->vif_table[asr_hw->sta_vif_idx];
            }

            if (asr_hw->ap_vif_idx < asr_hw->vif_max_num) {
                asr_ext_vif  = asr_hw->vif_table[asr_hw->ap_vif_idx];
            }

        } else {
            if (asr_hw->sta_vif_idx < asr_hw->vif_max_num) {
                asr_traffic_vif  = asr_hw->vif_table[asr_hw->sta_vif_idx];
            } else if (asr_hw->ap_vif_idx < asr_hw->vif_max_num) {
                asr_traffic_vif  = asr_hw->vif_table[asr_hw->ap_vif_idx];
            }
        }

        // wifi phy restarting, reset all tx agg buf.
        if (asr_test_bit(ASR_DEV_RESTARTING, &asr_hw->phy_flags)) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s:phy drop,phy_flags(%08X),cnt(%u)\n",
                 __func__, (unsigned int)asr_hw->phy_flags, asr_hw->tx_agg_env.aggr_buf_cnt);

            asr_rtos_lock_mutex(&asr_hw->tx_agg_env_lock);
            asr_tx_agg_buf_reset(asr_hw);
            list_for_each_entry(asr_vif_tmp,&asr_hw->vifs,list){
                asr_vif_tmp->txring_bytes = 0;
            }
            asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);

            break;
        }

        // FIXME. p2p case will send mgt frame from host before connect.
        if ( asr_traffic_vif && ASR_VIF_TYPE(asr_traffic_vif) == NL80211_IFTYPE_STATION &&
             (asr_test_bit(ASR_DEV_STA_DISCONNECTING, &asr_traffic_vif->dev_flags) ||
             !asr_test_bit(ASR_DEV_STA_CONNECTED, &asr_traffic_vif->dev_flags)  ||
              asr_test_bit(ASR_DEV_CLOSEING, &asr_traffic_vif->dev_flags) ||
              asr_test_bit(ASR_DEV_PRECLOSEING, &asr_traffic_vif->dev_flags))) {

            dbg(D_INF, D_UWIFI_CTRL, "%s:sta drop,tra vif dev_flags(%08X),cnt(%u),vif_started_cnt=%d\n",
                 __func__, (unsigned int)asr_traffic_vif->dev_flags,asr_hw->tx_agg_env.aggr_buf_cnt,asr_hw->vif_started);

            asr_rtos_lock_mutex(&asr_hw->tx_agg_env_lock);
            if (asr_hw->vif_started > 1) {
                asr_tx_agg_buf_mask_vif(asr_hw,asr_traffic_vif);
            } else {
                asr_tx_agg_buf_reset(asr_hw);
                list_for_each_entry(asr_vif_tmp,&asr_hw->vifs,list){
                    asr_vif_tmp->txring_bytes = 0;
                }
            }

            asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);

            if (asr_hw->vif_started <= 1)
                break;
        }


#ifdef CONFIG_ASR595X
        // add stop tx flag check, just stop send data from drv to fw.
        if (asr_traffic_vif && ASR_VIF_TYPE(asr_traffic_vif) == NL80211_IFTYPE_STATION
            && (asr_test_bit(ASR_DEV_STA_OUT_TWTSP, &asr_traffic_vif->dev_flags) ||
                asr_test_bit(ASR_DEV_TXQ_STOP_CSA, &asr_traffic_vif->dev_flags) ||
                asr_test_bit(ASR_DEV_TXQ_STOP_VIF_PS, &asr_traffic_vif->dev_flags) ||
                asr_test_bit(ASR_DEV_TXQ_STOP_CHAN, &asr_traffic_vif->dev_flags))) {

            dbg(D_ERR, D_UWIFI_CTRL, " stop tx, tra vif dev_flags=0x%x!!!\r\n",(unsigned int)(asr_traffic_vif->dev_flags));

            break;
        }
#endif


        /************************ tx cmd53 prepare and send.****************************************************************/
        ava_agg_num = asr_tx_aggr_get_ava_trans_num(asr_hw, &total_ava_num);
        if (!ava_agg_num) {
            break;
        }

        avail_data_port =
            asr_sdio_tx_get_available_data_port(asr_hw, ava_agg_num, &port_num, &io_addr, &bitmap_record);

        #ifdef CONFIG_ASR_KEY_DBG
        tx_agg_port_stats(avail_data_port);
        #endif

        if (!avail_data_port) {
            break;
        }

        temp_cur_next_addr = tx_agg_env->cur_aggr_buf_next_addr;
        temp_cur_next_idx = tx_agg_env->cur_aggr_buf_next_idx;

        hostdesc_start = (struct hostdesc *)(temp_cur_next_addr);
        hostdesc_start->agg_num = port_num;

        asr_data_tx_pkt_cnt += port_num;
        while (port_num--) {
            tx_bytes += ((struct hostdesc *)(temp_cur_next_addr))->packet_len;

            temp_len = *((u16 *) temp_cur_next_addr) + 2;
            if (temp_len % SDIO_BLOCK_SIZE || temp_len > ASR_SDIO_DATA_MAX_LEN) {
                dbg(D_ERR, D_UWIFI_CTRL,
                    "unlikely: temp_cur_next_addr:0x%x len:%d not aligned\r\n",
                    (u32) (uintptr_t) temp_cur_next_addr, temp_len);

                dbg(D_ERR, D_UWIFI_CTRL,
                    "[%s %d] wr:0x%x(%d) rd:0x%x(%d) aggr_buf(%p) cnt:%d - agg_ava_num:%d (%d,%d)\n",
                    __func__, __LINE__, (u32) (uintptr_t)
                    tx_agg_env->last_aggr_buf_next_addr,
                    tx_agg_env->last_aggr_buf_next_idx, (u32) (uintptr_t)
                    tx_agg_env->cur_aggr_buf_next_addr,
                    tx_agg_env->cur_aggr_buf_next_idx,
                    asr_hw->tx_agg_env.aggr_buf->data,
                    tx_agg_env->aggr_buf_cnt, ava_agg_num, hostdesc_start->agg_num, port_num);

                //WARN_ON(1);

                asr_rtos_lock_mutex(&asr_hw->tx_agg_env_lock);
                asr_tx_agg_buf_reset(asr_hw);
                list_for_each_entry(asr_vif_tmp,&asr_hw->vifs,list){
                    asr_vif_tmp->txring_bytes = 0;
                }
                asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);

                return total_tx_num;
            } else {
                // multi vif tx ringbuf byte caculate for flow ctrl.
                hostdesc_temp = (struct hostdesc *)(temp_cur_next_addr);

                if ((hostdesc_temp->vif_idx < asr_hw->vif_max_num)) {
                    asr_vif_tmp = asr_hw->vif_table[hostdesc_temp->vif_idx];

                    asr_rtos_lock_mutex(&asr_hw->tx_agg_env_lock);
                    if (asr_vif_tmp && (asr_vif_tmp->txring_bytes >= temp_len)) {
                        asr_vif_tmp->txring_bytes -= temp_len;
                        //dbg(D_ERR, D_UWIFI_CTRL," mrole tx(vif_type %d): frm_len = %d \r\n",
                        //                        ASR_VIF_TYPE(asr_vif_tmp),hostdesc_temp->packet_len);
                    } else {
                        if (asr_vif_tmp)
                            dbg(D_ERR, D_UWIFI_CTRL," unlikely: mrole tx(vif_type %d): %d < %d !!!\r\n",
                                                ASR_VIF_TYPE(asr_vif_tmp),asr_vif_tmp->txring_bytes, temp_len);
                    }
                    asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);
                } else {
                        dbg(D_ERR, D_UWIFI_CTRL," unlikely: mrole tx(vif_idx %d not valid)!!!\r\n",hostdesc_temp->vif_idx);
                }
            }
            trans_len += temp_len;
            temp_cur_next_addr += temp_len;
            temp_cur_next_idx++;
        }

        ret = asr_sdio_send_data(asr_hw, HIF_TX_DATA, (u8 *) hostdesc_start, trans_len, io_addr, bitmap_record);    // sleep api, not add in spin lock
        if (ret) {
            dbg(D_ERR, D_UWIFI_CTRL,
                "asr_tx_push fail  with len:%d and src 0x%x ,ret=%d aggr_num=%d  ethertype=0x%x, bm=0x%x,io_addr=%X,tx_use_bitmap=0x%X\n",
                trans_len, (u32) (uintptr_t) hostdesc_start, ret, hostdesc_start->agg_num, hostdesc_start->ethertype,
                bitmap_record,io_addr,asr_hw->tx_use_bitmap);
            dbg(D_ERR, D_UWIFI_CTRL, "[%s] wr:0x%x(%d) rd:0x%x(%d) cnt:%d \n", __func__, (u32) (uintptr_t)
                tx_agg_env->last_aggr_buf_next_addr,
                tx_agg_env->last_aggr_buf_next_idx, (u32) (uintptr_t)
                tx_agg_env->cur_aggr_buf_next_addr,
                tx_agg_env->cur_aggr_buf_next_idx, tx_agg_env->aggr_buf_cnt);


            if (mrole_enable == false && asr_traffic_vif != NULL) {
                asr_traffic_vif->net_stats.tx_dropped += hostdesc_start->agg_num;
            }

            if (host_sdio_retry++ > 5) {
                break;
            }

            if (host_restart(asr_hw, NULL, hostdesc_start->agg_num, true)) {
                dbg(D_ERR, D_UWIFI_CTRL, "HOST_RESTART: SDIO write SUCCESS !!!!! \n");
                continue;
            } else {
                dbg(D_ERR, D_UWIFI_CTRL, "HOST_RESTART: SDIO write FAIL !!!!! \n");
                break;
            }

        }
        trans_len = 0;

        /************************ tx stats and flow ctrl.****************************************************************/

        asr_rtos_lock_mutex(&asr_hw->tx_agg_env_lock); //may only need to protect data from here

        if (mrole_enable == false && asr_traffic_vif != NULL)
        {
            asr_traffic_vif->net_stats.tx_packets += hostdesc_start->agg_num;
            asr_traffic_vif->net_stats.tx_bytes += tx_bytes;
        }

        asr_hw->stats.last_tx = asr_rtos_get_time();
#ifdef ASR_STATS_RATES_TIMER
        asr_hw->stats.tx_bytes += tx_bytes;
#endif

        if (tx_agg_env->aggr_buf_cnt < hostdesc_start->agg_num) {

            dbg(D_ERR, D_UWIFI_CTRL, "[%s] unlikely : wr:0x%x(%d) rd:0x%x(%d) cnt:%d agg_num:%d agg_ava_num:%d\n",
                __func__, (u32) (uintptr_t)
                tx_agg_env->last_aggr_buf_next_addr, tx_agg_env->last_aggr_buf_next_idx,
                (u32) (uintptr_t)
                tx_agg_env->cur_aggr_buf_next_addr, tx_agg_env->cur_aggr_buf_next_idx,
                tx_agg_env->aggr_buf_cnt, hostdesc_start->agg_num, ava_agg_num);

            asr_tx_agg_buf_reset(asr_hw);
            list_for_each_entry(asr_vif_tmp,&asr_hw->vifs,list){
                asr_vif_tmp->txring_bytes = 0;
            }
            asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);
            return total_tx_num;
        } else
            tx_agg_env->aggr_buf_cnt -= hostdesc_start->agg_num;

        if (tx_agg_env->aggr_buf_cnt == 0)
            asr_hw->xmit_first_trigger_flag = false;

        total_tx_num += hostdesc_start->agg_num;
        tx_agg_env->cur_aggr_buf_next_addr = temp_cur_next_addr;
        tx_agg_env->cur_aggr_buf_next_idx = temp_cur_next_idx;

        asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);

    }

    return total_tx_num;
}


int asr_main_task_run_cnt;
int asr_main_task(struct asr_hw *asr_hw, u8 main_task_from_type)
{
    int type = HIF_INVALID_TYPE;
    //unsigned long now = asr_rtos_get_time();
    struct asr_tx_agg *tx_agg_env = &(asr_hw->tx_agg_env);
    int ret = 0;
    //struct sdio_func *func;

#if 0
    spin_lock_bh(&asr_hw->pmain_proc_lock);
    /* Check if already processing */
    if (asr_hw->mlan_processing) {
        asr_hw->more_task_flag = true;
        ret = -1;
        spin_unlock_bh(&asr_hw->pmain_proc_lock);
        goto exit_main_proc;
    } else {
        asr_hw->mlan_processing = true;
        //asr_hw->restart_flag = false;
        spin_unlock_bh(&asr_hw->pmain_proc_lock);
    }

process_start:
#endif

    do {
        /* Handle pending SDIO interrupts if any */
        asr_main_task_run_cnt++;

        if (
            // if from thread, asr_hw->sdio_ireg always = 0
             asr_hw->sdio_ireg
            // never true.
            || (asr_hw->restart_flag)
            )
        {
            asr_process_int_status(asr_hw->ipc_env);
        }

        // handle tx task
        if ((asr_xmit_opt ? skb_queue_len(&asr_hw->tx_sk_list) : tx_agg_env->aggr_buf_cnt) && bitcount(asr_hw->tx_use_bitmap))
        //if( bitcount(asr_hw->tx_use_bitmap))
        {
            if (asr_xmit_opt)  {
                ret += asr_opt_tx_task(asr_hw);
            } else {
                ret += asr_tx_task(asr_hw);
            }
        }

    } while (0);

#if 0
    spin_lock_bh(&asr_hw->pmain_proc_lock);
    if (asr_hw->more_task_flag == true) {
        asr_hw->more_task_flag = false;
        asr_hw->restart_flag = true;
        spin_unlock_bh(&asr_hw->pmain_proc_lock);
        goto process_start;
    }
    asr_hw->mlan_processing = false;
    asr_hw->restart_flag = false;
    spin_unlock_bh(&asr_hw->pmain_proc_lock);

exit_main_proc:
#endif

    return ret;
}

int asr_main_tx_task(struct asr_hw *asr_hw, u8 main_task_from_type)
{
    int type = HIF_INVALID_TYPE;
    //unsigned long now = asr_rtos_get_time();
    struct asr_tx_agg *tx_agg_env = &(asr_hw->tx_agg_env);
    int ret = 0;

    do {
        // handle tx task
        if ((asr_xmit_opt ? skb_queue_len(&asr_hw->tx_sk_list) : tx_agg_env->aggr_buf_cnt) && bitcount(asr_hw->tx_use_bitmap))
        //if( bitcount(asr_hw->tx_use_bitmap))
        {
            if (asr_xmit_opt)  {
                ret += asr_opt_tx_task(asr_hw);
            } else {
                ret += asr_tx_task(asr_hw);
            }
        }

    } while (0);

    return ret;
}



u8 asr_count_bits(u16 data, u8 start_bit)
{
    u8 num = 0;

    while (start_bit < 16) {
        if (data & (1 << start_bit)) {
            num++;
        }
        start_bit++;
    }

    return num;
}

void dump_sdio_info(struct asr_hw *asr_hw, const char *func, u32 line)
{
    u8 sdio_int_reg;
    u8 sdio_int_reg_l;
    u16 rd_bitmap, wr_bitmap;
    u16 rd_bitmap_l, wr_bitmap_l;
    struct asr_tx_agg *tx_agg_env = &(asr_hw->tx_agg_env);

    sdio_int_reg = asr_hw->sdio_reg[HOST_INT_STATUS];
    rd_bitmap = asr_hw->sdio_reg[RD_BITMAP_L] | asr_hw->sdio_reg[RD_BITMAP_U] << 8;
    wr_bitmap = asr_hw->sdio_reg[WR_BITMAP_L] | asr_hw->sdio_reg[WR_BITMAP_U] << 8;

    sdio_int_reg_l = asr_hw->last_sdio_regs[HOST_INT_STATUS];
    rd_bitmap_l = asr_hw->last_sdio_regs[RD_BITMAP_L] | asr_hw->sdio_reg[RD_BITMAP_U] << 8;
    wr_bitmap_l = asr_hw->last_sdio_regs[WR_BITMAP_L] | asr_hw->sdio_reg[WR_BITMAP_U] << 8;

    dbg(D_ERR, D_UWIFI_CTRL,
        "%s,%d:trace sdio,%02X,(%04X,%02d,%02d),(%04X,%04X,%03d,%02d),(%02X,%04X,%04X)\n",
        func, line, sdio_int_reg, rd_bitmap, asr_count_bits(rd_bitmap, 2),
        asr_hw->rx_data_cur_idx, wr_bitmap, asr_hw->tx_use_bitmap,
        tx_agg_env->aggr_buf_cnt, asr_hw->tx_data_cur_idx, sdio_int_reg_l, rd_bitmap_l, wr_bitmap_l);
}

static int read_sdio_block(struct asr_hw *asr_hw, u8 * dst, unsigned int addr)
{
    int ret;

    /*  read newest wr_bitmap/rd_bitmap/rd_len to update sdio_reg */
    asr_sdio_claim_host();
    ret = sdio_read_cmd53_reg(addr, dst, SDIO_REG_READ_LENGTH);
    asr_sdio_release_host();

    return ret;
}

static int read_sdio_block_int(struct asr_hw *asr_hw, u8 *sdio_int_reg)
{
    u16 rd_bitmap, wr_bitmap, diff_bitmap = 0;
    int ret = 0;

    // read_sdio_block will clear host int status and get rd/wr bitmap at the same time.
    ret = read_sdio_block(asr_hw, asr_hw->sdio_reg, 0);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "[%s] reg rd fail (%d)!!! \n", __func__, ret);

        cmd_queue_crash_handle(asr_hw, __func__, __LINE__, ASR_RESTART_REASON_SDIO_ERR);

        return -1;
    }

    //spin_lock_bh(&asr_hw->int_reg_lock);
    //memcpy(asr_hw->sdio_reg, sdio_regs, SDIO_REG_READ_LENGTH);

    *sdio_int_reg = asr_hw->sdio_reg[HOST_INT_STATUS];
    rd_bitmap = asr_hw->sdio_reg[RD_BITMAP_L] | asr_hw->sdio_reg[RD_BITMAP_U] << 8;
    wr_bitmap = asr_hw->sdio_reg[WR_BITMAP_L] | asr_hw->sdio_reg[WR_BITMAP_U] << 8;
    diff_bitmap = ((asr_hw->tx_use_bitmap) ^ wr_bitmap);

    //dump_sdio_info(asr_hw, __func__, __LINE__);

    if (!(*sdio_int_reg & HOST_INT_UPLD_ST) && ((rd_bitmap & 0x1)
                           || asr_count_bits(rd_bitmap, 2) >= SDIO_RX_AGG_TRI_CNT)) {

        //dbg(D_ERR, D_UWIFI_CTRL,"%s: recover rx,0x%x\n", __func__, rd_bitmap);

        //dump_sdio_info(asr_hw, __func__, __LINE__);

        *sdio_int_reg |= HOST_INT_UPLD_ST;
        asr_hw->sdio_reg[HOST_INT_STATUS] |= HOST_INT_UPLD_ST;
    }

    if (!(*sdio_int_reg & HOST_INT_DNLD_ST) &&
        ((diff_bitmap & 0x1) || asr_count_bits(diff_bitmap, 1) >= SDIO_TX_AGG_TRI_CNT)) {

        //dbg(D_ERR, D_UWIFI_CTRL,"%s: recover tx,0x%X\n", __func__, diff_bitmap);

        *sdio_int_reg |= HOST_INT_DNLD_ST;
        asr_hw->sdio_reg[HOST_INT_STATUS] |= HOST_INT_DNLD_ST;
    }

    if (*sdio_int_reg) {
        /*
         * HOST_INT_DNLD_ST and/or HOST_INT_UPLD_ST
         * Clear the interrupt status register
         */
        if (txlogen || rxlogen)
            dbg(D_ERR, D_UWIFI_CTRL, "[%s]int:(0x%x 0x%x 0x%x)\n", __func__, *sdio_int_reg, wr_bitmap,
                rd_bitmap);

        asr_hw->sdio_ireg |= *sdio_int_reg;
        memcpy(asr_hw->last_sdio_regs, asr_hw->sdio_reg, SDIO_REG_READ_LENGTH);
        //spin_unlock_bh(&asr_hw->int_reg_lock);
    } else {
        //spin_unlock_bh(&asr_hw->int_reg_lock);
        return 0;
    }

    return 0;
}


int isr_write_clear_cnt;
int ipc_read_rx_bm_fail_cnt;
int ipc_read_tx_bm_fail_case0_cnt;
int ipc_read_tx_bm_fail_case2_cnt;
int ipc_read_tx_bm_fail_case3_cnt;
int ipc_isr_skip_main_task_cnt;
extern int tx_status_debug;

void asr_sdio_corner_case(struct asr_hw *asr_hw,u8 sdio_int_reg,u32 tx_len,bool is_tx_send,bool is_rx_receive,
            u16 rd_bitmap,u16 wr_bitmap,bool skip_main_task_flag,int tx_pkt_cnt_in_isr)
{
    bool write_clear_case1, write_clear_case2, write_clear_case3, write_clear_case4,write_clear_case0;
    bool is_tx=false,is_rx=false;
    int ret = -1;
    // struct sdio_func *func = asr_hw->plat->func;

    is_tx = (sdio_int_reg & HOST_INT_DNLD_ST) ? true : false;
    is_rx = (sdio_int_reg & HOST_INT_UPLD_ST) ? true : false;

    /*
       corner case:
       0:    current
       1:    tx ;
       2:    only sdio isr send;
       3:    bitmap data is 0
    */

    //write_clear_case0 = is_tx && !skip_main_task_flag && !(wr_bitmap >> 1)&& tx_len && !is_tx_send;
    //write_clear_case0 = is_tx && !skip_main_task_flag && (!(asr_hw->tx_use_bitmap>> 1)||!(wr_bitmap >> 1))&& tx_len && !is_tx_send;

    write_clear_case0 = is_tx && (!(asr_hw->tx_use_bitmap>> 1)||!(wr_bitmap >> 1))&& tx_len && !is_tx_send;

    // special case for write clear int status
    // corner case 1: last int read all bitmap.
    write_clear_case1 = is_rx && (!(rd_bitmap >> 2)) && !is_rx_receive;

    // corner case 2: may wr_bitmap = 0x1, but no data tx cmd53 will also cause clear fail.
    write_clear_case2 = is_tx && !skip_main_task_flag && !(wr_bitmap >> 1) && tx_len;

    // corner case 3: this case cmd53 write pkt cnt==0(maybe aggr_buf_cnt==0), use write clear
    write_clear_case3 = is_tx && !skip_main_task_flag && (wr_bitmap >> 1) && !tx_pkt_cnt_in_isr && tx_len;

    // corner case 4: this case skip cmd53 write/read, use write clear // may removed
    write_clear_case4 = (is_tx|| is_rx) && skip_main_task_flag;

    // interrupt and bitmap sync fail.
    if (write_clear_case0)
        ipc_read_tx_bm_fail_case0_cnt++;

    if (write_clear_case1)
        ipc_read_rx_bm_fail_cnt++;

    if (write_clear_case2)
        ipc_read_tx_bm_fail_case2_cnt++;

    if (write_clear_case3)
        ipc_read_tx_bm_fail_case3_cnt++;

    if (write_clear_case4)
        ipc_isr_skip_main_task_cnt++;

    // if no cmd53 write/read , clear int status will fail,use write-clear .
    if (write_clear_case1 || write_clear_case0 ||
        (write_clear_case2 && (tx_debug <= 0)) ||
        (write_clear_case3 && (tx_debug <= 0)) ||
        (write_clear_case4 && (tx_debug <= 0)) ||
        (tx_debug == 256 )
        ) {

        asr_sdio_claim_host();
        ret = sdio_writeb_cmd52(HOST_INT_STATUS, 0x0);
        asr_sdio_release_host();

        if (!ret) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s write clear HOST_INT_STATUS fail!!! (%d)\n", __func__, ret);
            return;
        }

        isr_write_clear_cnt++;

    } else {

#ifdef CONFIG_ASR_KEY_DBG
        if (tx_status_debug == 13) {
            if (!is_tx_send && is_tx && tx_len)
                dbg(D_ERR, D_UWIFI_CTRL, "%s tx :(reg:0x%x  ,%d %d ,0x%x : 0x%x %d)!!! \n",
                    __func__,
                    sdio_int_reg,
                    skip_main_task_flag,
                    tx_len,
                    wr_bitmap,asr_hw->tx_use_bitmap,
                    asr_hw->tx_data_cur_idx);

            if (!is_rx_receive && is_rx)
                dbg(D_ERR, D_UWIFI_CTRL, "%s rx :(0x%x 0x%x %d)!!! \n",
                    __func__, sdio_int_reg, rd_bitmap, skip_main_task_flag);
        }
#endif
    }

}

static void asr_sdio_dataworker(struct asr_hw *asr_hw)
{
    //struct sdio_func *func = asr_hw->plat->func;
    u8 sdio_int_reg;
    int ret = -1;
    u16 rd_bitmap, wr_bitmap;
    u32 pre_tx_pkt_cnt, after_tx_pkt_cnt;
    u32 pre_rx_pkt_cnt, after_rx_pkt_cnt;
    int skip_main_task_flag = 0;
    struct asr_tx_agg *tx_agg_env;
    bool write_clear_case1, write_clear_case2, write_clear_case3, write_clear_case4;
    int main_task_ret = 0;
    int tx_pkt_cnt_in_isr = 0;
    bool is_tx_send=false,is_rx_receive=false;
    u32 tx_len = 0;

    /* interrupt to read int status */
    if (!asr_hw) {
        return;
    }

    //func = asr_hw->plat->func;
    //asr_hw = sdio_get_drvdata(func);
    tx_agg_env = &(asr_hw->tx_agg_env);

    if (read_sdio_block_int(asr_hw, &sdio_int_reg)) {
        return;
    }

    rd_bitmap = asr_hw->sdio_reg[RD_BITMAP_L] | asr_hw->sdio_reg[RD_BITMAP_U] << 8;
    wr_bitmap = asr_hw->sdio_reg[WR_BITMAP_L] | asr_hw->sdio_reg[WR_BITMAP_U] << 8;

    // main process to update tx/rx bitmap and handle tx/rx task.
    //tx_evt_irq_times++;

    pre_tx_pkt_cnt = asr_data_tx_pkt_cnt;
    pre_rx_pkt_cnt = asr_rx_pkt_cnt;

    // asr main task in isr may skip .
    main_task_ret = asr_main_task(asr_hw, SDIO_ISR);
    if (main_task_ret < 0)
        skip_main_task_flag = 1;
    else
        tx_pkt_cnt_in_isr = main_task_ret;

    after_tx_pkt_cnt = asr_data_tx_pkt_cnt;
    after_rx_pkt_cnt = asr_rx_pkt_cnt;


    is_tx_send = (pre_tx_pkt_cnt == after_tx_pkt_cnt)? false : true;
    is_rx_receive = (pre_rx_pkt_cnt == after_rx_pkt_cnt)? false : true;

    //tx_len = asr_xmit_opt ? skb_queue_len(&asr_hw->tx_sk_list) : tx_agg_env->aggr_buf_cnt;
    tx_len = tx_agg_env->aggr_buf_cnt;

    asr_sdio_corner_case(asr_hw,sdio_int_reg,tx_len,is_tx_send,is_rx_receive,
                        rd_bitmap,wr_bitmap,skip_main_task_flag,tx_pkt_cnt_in_isr);

    #if 1
    //FIX ME : trigger more to avoid there is more data comming during read period
    if (rd_bitmap ||
        tx_agg_env->aggr_buf_cnt >= SDIO_TX_AGG_TRI_CNT)
    {

        //trigger rx will re-read
        uwifi_sdio_event_set(UWIFI_SDIO_EVENT_RX);
    }
    #endif

}



int sdio_tx_evt_cnt;
int sdio_rx_evt_cnt;

void uwifi_sdio_main(asr_thread_arg_t arg)
{
    struct asr_hw *asr_hw = (struct asr_hw *)arg;
    int ret;
    int type;
    uint32_t rcv_flags;


    while(SCAN_DEINIT != scan_done_flag)
    {
        rcv_flags = 0;
        uwifi_sdio_event_wait(UWIFI_SDIO_EVENT_RX | UWIFI_SDIO_EVENT_TX | UWIFI_SDIO_EVENT_MSG, &rcv_flags, ASR_WAIT_FOREVER);

        // clear wakeup event
        uwifi_sdio_event_clear(rcv_flags);

        if (txlogen)
            dbg(D_DBG, D_UWIFI_CTRL, "%s get flags:0x%x", __func__, rcv_flags);

        if (rcv_flags & UWIFI_SDIO_EVENT_RX) {
            // event trigger by sdio int: clr int stats/update bitmap/sdio tx&rx
            sdio_rx_evt_cnt++;
            asr_sdio_dataworker(asr_hw);
        }

        if (rcv_flags & UWIFI_SDIO_EVENT_TX) {
            // event trigger by xmit, sdio tx based on tx_use_bitmap
            sdio_tx_evt_cnt++;
            #ifndef AWORKS
            asr_main_task(asr_hw, SDIO_THREAD);
            #else
            asr_main_tx_task(asr_hw, SDIO_THREAD);
            #endif
        }

        #if 0//ndef MSG_REFINE
        //msg, first
        if (rcv_flags & UWIFI_SDIO_EVENT_MSG) {
            extern struct asr_cmd *g_cmd;
            extern asr_semaphore_t *g_sem;
            extern int g_cmd_ret;
            struct asr_cmd *cmd = g_cmd;
            asr_semaphore_t *sem = g_sem;
            if (cmd && sem) {

                g_cmd_ret = ipc_host_msg_push(asr_hw->ipc_env, (void *)cmd, sizeof(struct lmac_msg) + cmd->a2e_msg->param_len);
                if (g_cmd_ret) {
                    dbg(D_ERR,D_UWIFI_CTRL,"%s:retry tx msg,g_cmd=%p,g_sem=%p",__func__, g_cmd, g_sem);
                    uwifi_sdio_event_set(UWIFI_SDIO_EVENT_MSG);
                } else {
                    asr_rtos_set_semaphore(sem);
                }

            } else {
                dbg(D_ERR,D_UWIFI_CTRL,"%s:rcv_flags=0x%X,g_cmd=%p,g_sem=%p",__func__, rcv_flags, g_cmd, g_sem);
            }
        }
        #endif

        //dbg(D_DBG, D_UWIFI_CTRL, "%s: rcv_flags=0x%X,INT=0x%X,rdbitmap=0x%X",
        //    __func__, rcv_flags, asr_hw->sdio_reg[HOST_INT_STATUS], rdbitmap);
    }
}

void uwifi_rx_to_os_main(asr_thread_arg_t arg)
{
    struct uwifi_task_msg_t msg={0};
    struct asr_hw *asr_hw = (struct asr_hw *)arg;

    while(SCAN_DEINIT == scan_done_flag)
        asr_rtos_delay_milliseconds(5);

    while(SCAN_DEINIT != scan_done_flag)
    {
        asr_rtos_pop_from_queue(&uwifi_rx_to_os_msg_queue, &msg, ASR_WAIT_FOREVER);
        asr_rx_to_os_task(asr_hw);
    }
}

