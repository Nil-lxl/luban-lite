/**
 ******************************************************************************
 *
 * @file uwifi_tx.c
 *
 * @brief tx related operation
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */

#include "uwifi_tx.h"
#include "uwifi_include.h"
#include "uwifi_rx.h"
#include "uwifi_msg_tx.h"
#include "uwifi_kernel.h"
#include "uwifi_txq.h"
#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
#include "uwifi_idle_mode.h"
#endif
#include "uwifi_sdio.h"
#include "uwifi_msg.h"
#include "uwifi_ops_adapter.h"
#define INVALID_TID 0xFF
#define INVALID_STA_IDX 0xFF

#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
//IDLE_HWQ_IDX:0-3
#define IDLE_HWQ_IDX    0
#endif
extern int flow_ctrl_high;

extern bool check_is_dhcp(struct asr_vif *asr_vif,bool is_tx, uint16_t eth_proto, uint8_t* type, uint8_t** d_mac, uint8_t* data,
    uint32_t data_len);

/******************************************************************************
 * TX functions
 *****************************************************************************/
#define PRIO_STA_NULL 0xAA

static const int asr_down_hwq2tid[3] = {
    [ASR_HWQ_BK] = 2,
    [ASR_HWQ_BE] = 3,
    [ASR_HWQ_VI] = 5,
};

/******************************************************************************
 * Power Save functions
 *****************************************************************************/
/**
 * asr_set_traffic_status - Inform FW if traffic is available for STA in PS
 *
 * @asr_hw: Driver main data
 * @sta: Sta in PS mode
 * @available: whether traffic is buffered for the STA
 * @ps_id: type of PS data requested (@LEGACY_PS_ID or @UAPSD_ID)
  */
void asr_set_traffic_status(struct asr_hw *asr_hw,
                             struct asr_sta *sta,
                             bool available,
                             uint8_t ps_id)
{
    bool uapsd = (ps_id != LEGACY_PS_ID);
    asr_send_me_traffic_ind(asr_hw, sta->sta_idx, uapsd, available);
}

/**
 * asr_ps_bh_enable - Enable/disable PS mode for one STA
 *
 * @asr_hw: Driver main data
 * @sta: Sta which enters/leaves PS mode
 * @enable: PS mode status
 *
 * This function will enable/disable PS mode for one STA.
 * When enabling PS mode:
 *  - Stop all STA's txq for ASR_TXQ_STOP_STA_PS reason
 *  - Count how many buffers are already ready for this STA
 *  - For BC/MC sta, update all queued SKB to use hw_queue BCMC
 *  - Update TIM if some packet are ready
 *
 * When disabling PS mode:
 *  - Start all STA's txq for ASR_TXQ_STOP_STA_PS reason
 *  - For BC/MC sta, update all queued SKB to use hw_queue AC_BE
 *  - Update TIM if some packet are ready (otherwise fw will not update TIM
 *    in beacon for this STA)
 *
 * All counter/skb updates are protected from TX path by taking tx_lock
 *
 * NOTE: _bh_ in function name indicates that this function is called
 * from a bottom_half tasklet.
 */
struct asr_traffic_status g_asr_traffic_sts;

void asr_update_sta_ps_pkt_num(struct asr_hw *asr_hw,struct asr_sta *sta)
{
    struct sk_buff *skb_pending;
    struct sk_buff *pnext;
    struct hostdesc * hostdesc_tmp = NULL;
    struct asr_vif *asr_vif_tmp = NULL;
    u8 tid_tmp ;

    if (sta == NULL)
        return ;

    // loop list and get avail skbs.
    skb_queue_walk_safe(&asr_hw->tx_sk_list, skb_pending, pnext) {
        hostdesc_tmp = (struct hostdesc *)skb_pending->data;
        if ((hostdesc_tmp->vif_idx < asr_hw->vif_max_num)) {
            asr_vif_tmp = asr_hw->vif_table[hostdesc_tmp->vif_idx];
                if (asr_vif_tmp && hostdesc_tmp->staid == sta->sta_idx) {
                    /*
                    hostdesc->tid = tid;
                    tid = 0xFF, non qos data or mgt frame.
                    tid TID_0~TID7,TID_MGT   qos data.

                    hostdesc->queue_idx = txq->hwq->id;
                    queue_idx : hw queue, BK/BE/VI/VO/Beacon.
                    */
                    tid_tmp = hostdesc_tmp->tid;

                    if (tid_tmp >= NX_NB_TXQ_PER_STA)
                        tid_tmp = 0;

                    if ((sta->uapsd_tids & tid_tmp) && !(is_multicast_sta(asr_hw,sta->sta_idx)))
                        sta->ps.pkt_ready[UAPSD_ID] ++;
                    else
                        sta->ps.pkt_ready[LEGACY_PS_ID] ++;

                }
        } else {
                dbg(D_ERR,D_UWIFI_CTRL," unlikely: mrole tx(vif_idx %d not valid)!!!\r\n",hostdesc_tmp->vif_idx);
        }
    }
}

void asr_ps_bh_enable(struct asr_hw *asr_hw, struct asr_sta *sta,
                       bool enable)
{
    //struct asr_txq *txq;

    if (enable) {
        //trace_ps_enable(sta);

        asr_rtos_lock_mutex(&asr_hw->tx_lock);
        sta->ps.active = true;
        sta->ps.sp_cnt[LEGACY_PS_ID] = 0;
        sta->ps.sp_cnt[UAPSD_ID] = 0;

        // remove sta per txq from hw_list
        // sdio mode not affect, instead ignore sta ps pkt in tx_sk_list.
        //asr_txq_sta_stop(sta, ASR_TXQ_STOP_STA_PS, asr_hw);

        if (is_multicast_sta(asr_hw,sta->sta_idx)) {
            //txq = asr_txq_sta_get(sta, 0, NULL,asr_hw);

            sta->ps.pkt_ready[LEGACY_PS_ID] = 0;  //skb_queue_len(&txq->sk_list);
            sta->ps.pkt_ready[UAPSD_ID] = 0;

            asr_update_sta_ps_pkt_num(asr_hw,sta);

            //txq->hwq = &asr_hw->hwq[ASR_HWQ_BCMC];
        } else {
            //int i;
            sta->ps.pkt_ready[LEGACY_PS_ID] = 0;
            sta->ps.pkt_ready[UAPSD_ID] = 0;

        #if 0
            foreach_sta_txq(sta, txq, i, asr_hw) {
                sta->ps.pkt_ready[txq->ps_id] += skb_queue_len(&txq->sk_list);
            }
        #else
            // txq will use LEGACY_PS_ID or UAPSD_ID.
            asr_update_sta_ps_pkt_num(asr_hw,sta);
        #endif
        }

        asr_rtos_unlock_mutex(&asr_hw->tx_lock);

    #if 0
        if (sta->ps.pkt_ready[LEGACY_PS_ID]) {
            asr_set_traffic_status(asr_hw, sta, true, LEGACY_PS_ID);
        }

        if (sta->ps.pkt_ready[UAPSD_ID])
            asr_set_traffic_status(asr_hw, sta, true, UAPSD_ID);

        if ( sta->ps.pkt_ready[LEGACY_PS_ID] ||  sta->ps.pkt_ready[UAPSD_ID])
        dev_err(asr_hw->dev," [ps]true: sta-%d, uapsd=0x%x, (%d , %d) \r\n",sta->sta_idx,sta->uapsd_tids,
                                                   sta->ps.pkt_ready[LEGACY_PS_ID],  sta->ps.pkt_ready[UAPSD_ID] );
     #else
         if (sta->ps.pkt_ready[LEGACY_PS_ID] || sta->ps.pkt_ready[UAPSD_ID]) {

               g_asr_traffic_sts.send = true;
               g_asr_traffic_sts.asr_sta_ps = sta;
               g_asr_traffic_sts.tx_ava = true;
               g_asr_traffic_sts.ps_id_bits = (sta->ps.pkt_ready[LEGACY_PS_ID] ? CO_BIT(LEGACY_PS_ID) : 0) |
                                              (sta->ps.pkt_ready[UAPSD_ID] ? CO_BIT(UAPSD_ID) : 0);
         } else {
               g_asr_traffic_sts.send = false;
         }
    #endif
    }
    else
    {
        //trace_ps_disable(sta->sta_idx);

        asr_rtos_lock_mutex(&asr_hw->tx_lock);
        sta->ps.active = false;

    #if 0  //not used for sdio mode.
        if (is_multicast_sta(asr_hw,sta->sta_idx)) {
            txq = asr_txq_sta_get(sta, 0, NULL,asr_hw);

            txq->hwq = &asr_hw->hwq[ASR_HWQ_BE];
            txq->push_limit = 0;

        } else {

            int i;
            foreach_sta_txq(sta, txq, i, asr_hw) {
                txq->push_limit = 0;
            }
        }


        // readd sta per txq to hw_list
        // sdio mode not affect, instead re-trigger tx task and sta pkt in tx_sk_list will process.
        asr_txq_sta_start(sta, ASR_TXQ_STOP_STA_PS, asr_hw);
    #endif

        asr_rtos_unlock_mutex(&asr_hw->tx_lock);

        // re-trigger tx task.
        //set_bit(ASR_FLAG_MAIN_TASK_BIT, &asr_hw->ulFlag);
        //wake_up_interruptible(&asr_hw->waitq_main_task_thead);
        uwifi_sdio_event_set(UWIFI_SDIO_EVENT_TX);

    #if 0
        if (sta->ps.pkt_ready[LEGACY_PS_ID])
            asr_set_traffic_status(asr_hw, sta, false, LEGACY_PS_ID);

        if (sta->ps.pkt_ready[UAPSD_ID])
            asr_set_traffic_status(asr_hw, sta, false, UAPSD_ID);

        if ( sta->ps.pkt_ready[LEGACY_PS_ID] ||  sta->ps.pkt_ready[UAPSD_ID])
        dev_err(asr_hw->dev," [ps]false:sta-%d, uapsd=0x%x, (%d , %d) \r\n",sta->sta_idx,sta->uapsd_tids,
                                                   sta->ps.pkt_ready[LEGACY_PS_ID],  sta->ps.pkt_ready[UAPSD_ID] );
    #else
         if (sta->ps.pkt_ready[LEGACY_PS_ID] || sta->ps.pkt_ready[UAPSD_ID]) {

               g_asr_traffic_sts.send = true;
               g_asr_traffic_sts.asr_sta_ps = sta;
               g_asr_traffic_sts.tx_ava = false;
               g_asr_traffic_sts.ps_id_bits = (sta->ps.pkt_ready[LEGACY_PS_ID] ? CO_BIT(LEGACY_PS_ID) : 0) |
                                              (sta->ps.pkt_ready[UAPSD_ID] ? CO_BIT(UAPSD_ID) : 0);
         } else {
               g_asr_traffic_sts.send = false;
         }
    #endif
    }
}


/**
 * asr_ps_bh_traffic_req - Handle traffic request for STA in PS mode
 *
 * @asr_hw: Driver main data
 * @sta: Sta which enters/leaves PS mode
 * @pkt_req: number of pkt to push
 * @ps_id: type of PS data requested (@LEGACY_PS_ID or @UAPSD_ID)
 *
 * This function will make sure that @pkt_req are pushed to fw
 * whereas the STA is in PS mode.
 * If request is 0, send all traffic
 * If request is greater than available pkt, reduce request
 * Note: request will also be reduce if txq credits are not available
 *
 * All counter updates are protected from TX path by taking tx_lock
 *
 * NOTE: _bh_ in function name indicates that this function is called
 * from the bottom_half tasklet.
 */
void asr_ps_bh_traffic_req(struct asr_hw *asr_hw, struct asr_sta *sta,
                            uint16_t pkt_req, uint8_t ps_id)
{
    int pkt_ready_all;
    struct asr_txq *txq;

    if(!sta->ps.active)
    {
        dbg(D_WRN,D_UWIFI_CTRL,"sta %pM is not in Power Save mode",sta->mac_addr);
        return;
    }

    asr_rtos_lock_mutex(&asr_hw->tx_lock);

    pkt_ready_all = (sta->ps.pkt_ready[ps_id] - sta->ps.sp_cnt[ps_id]);

    /* Don't start SP until previous one is finished or we don't have
       packet ready (which must not happen for U-APSD) */
    if (sta->ps.sp_cnt[ps_id] || pkt_ready_all <= 0) {
        goto done;
    }

    /* Adapt request to what is available. */
    if (pkt_req == 0 || pkt_req > pkt_ready_all) {
        pkt_req = pkt_ready_all;
    }

    /* Reset the SP counter */
    sta->ps.sp_cnt[ps_id] = 0;

    /* "dispatch" the request between txq */
    txq = asr_txq_sta_get(sta, NX_NB_TXQ_PER_STA - 1, NULL, asr_hw);

    if (is_multicast_sta(asr_hw, sta->sta_idx)) {
        if (txq->credits <= 0)
            goto done;
        if (pkt_req > txq->credits)
            pkt_req = txq->credits;
        txq->push_limit = pkt_req;
        sta->ps.sp_cnt[ps_id] = pkt_req;
        asr_txq_add_to_hw_list(txq);
    } else {
        int i;
        /* TODO: dispatch using correct txq priority */
        for (i = NX_NB_TXQ_PER_STA - 1; i >= 0; i--, txq--) {
            uint16_t txq_len = skb_queue_len(&txq->sk_list);

            if (txq->ps_id != ps_id)
                continue;

            if (txq_len > txq->credits)
                txq_len = txq->credits;

            if (txq_len > 0) {
                if (txq_len < pkt_req) {
                    /* Not enough pkt queued in this txq, add this
                       txq to hwq list and process next txq */
                    pkt_req -= txq_len;
                    txq->push_limit = txq_len;
                    sta->ps.sp_cnt[ps_id] += txq_len;
                    asr_txq_add_to_hw_list(txq);
                } else {
                    /* Enough pkt in this txq to comlete the request
                       add this txq to hwq list and stop processing txq */
                    txq->push_limit = pkt_req;
                    sta->ps.sp_cnt[ps_id] += pkt_req;
                    asr_txq_add_to_hw_list(txq);
                    break;
                }
            }
        }
    }

  done:
    asr_rtos_unlock_mutex(&asr_hw->tx_lock);
}

static void asr_downgrade_ac(struct asr_sta *sta, struct sk_buff *skb)
{
    int8_t ac = asr_tid2hwq[skb->priority];

    if(ac > ASR_HWQ_VO)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"Unexepcted ac %d for skb before downgrade", ac);
        ac = ASR_HWQ_VO;
    }

    while (sta->acm & BIT(ac)) {
        if (ac == ASR_HWQ_BK) {
            skb->priority = 1;
            return;
        }
        ac--;
        skb->priority = asr_down_hwq2tid[ac];
    }
}

/* Given a data frame determine dscp/tos to use. */
unsigned int asr_get_tid_from_skb_data(struct sk_buff *skb)
{
    uint8_t dscp;
    struct ethhdr *eth = (struct ethhdr *)skb->data;
    uint8_t *layer3hdr = skb->data + sizeof(struct ethhdr); // data+14(mac header size)

    if(eth->h_proto == wlan_htons(ETH_P_IP))
        dscp = ipv4_get_dsfield((struct iphdrs*)layer3hdr) & 0xfc;
    else if(eth->h_proto == wlan_htons(ETH_P_IPV6))
        dscp = ipv6_get_dsfield((uint16_t *)layer3hdr) & 0xfc;
    else if(eth->h_proto == wlan_htons(ETH_P_80221))
        return 7;
    else
        return 0;

    return dscp >> 5;
}

/**
 * uint16_t (*ndo_select_queue)(struct net_device *dev, struct sk_buff *skb,
 *                         void *accel_priv, select_queue_fallback_t fallback);
 *    Called to decide which queue to when device supports multiple
 *    transmit queues.
 */
uint16_t asr_select_queue(struct asr_vif *asr_vif, struct sk_buff *skb)
{
    struct asr_sta *sta = NULL;
    struct asr_txq *txq;
    uint16_t netdev_queue;
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    switch (asr_vif->iftype)
    {
        case NL80211_IFTYPE_STATION:
        {
            //struct ethhdr *eth;
            //eth = (struct ethhdr *)skb->data;
            sta = asr_vif->sta.ap;
            break;
        }
        case NL80211_IFTYPE_AP:
        {
            struct asr_sta *cur;
            struct ethhdr *eth = (struct ethhdr *)skb->data;

            if (is_multicast_ether_addr(eth->h_dest))
            {
                sta = &asr_hw->sta_table[asr_vif->ap.bcmc_index];
            }
            else
            {
                list_for_each_entry(cur, &asr_vif->ap.sta_list, list)
                {
                    if (!memcmp(cur->mac_addr, eth->h_dest, ETH_ALEN))
                    {
                        sta = cur;
                        break;
                    }
                }
            }
            break;
        }
        default:
            break;
    }

    if (sta && sta->qos)
    {

        //dbg(D_ERR, D_UWIFI_CTRL, "%s:sta(%d) is qos\n",__func__,sta->sta_idx);
        /* use the data classifier to determine what 802.1d tag the
        * data frame has */
        skb->priority = asr_get_tid_from_skb_data(skb) & IEEE80211_QOS_CTL_TAG1D_MASK;
        if (sta->acm)
            asr_downgrade_ac(sta, skb);

        txq = asr_txq_sta_get(sta, skb->priority, NULL, asr_hw);
        netdev_queue = txq->ndev_idx;
    }
    else if (sta)
    {
        skb->priority = 0xFF;
        txq = asr_txq_sta_get(sta, 0, NULL, asr_hw);
        netdev_queue = txq->ndev_idx;

        //dbg(D_ERR, D_UWIFI_CTRL, "%s:sta(%d),netdev_queue=%d\n",__func__,sta->sta_idx,netdev_queue);
    }
    else
    {
        /* This packet will be dropped in xmit function, still need to select
           an active queue for xmit to be called. As it most likely to happen
           for AP interface, select BCMC queue
           (TODO: select another queue if BCMC queue is stopped) */
        skb->priority = PRIO_STA_NULL;
        netdev_queue = NX_NB_TID_PER_STA * asr_hw->sta_max_num;

        //dbg(D_ERR, D_UWIFI_CTRL, "%s:sta is null,netdev_queue=%d\n",__func__,netdev_queue);
    }

    return netdev_queue;
}

/**
 * asr_get_tx_info - Get STA and tid for one skb
 *
 * @asr_vif: vif ptr
 * @skb: skb
 * @tid: pointer updated with the tid to use for this skb
 *
 * @return: pointer on the destination STA (may be NULL)
 *
 * skb has already been parsed in asr_select_queue function
 * simply re-read information form skb.
 */
static struct asr_sta *asr_get_tx_info(struct asr_vif *asr_vif,
                                         struct sk_buff *skb,
                                         uint8_t *tid)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;
    struct asr_sta *sta;
    int sta_idx;
    int ndev_idx;

    ndev_idx = asr_select_queue(asr_vif, skb);

    *tid = skb->priority;
    if (skb->priority == PRIO_STA_NULL)
    {
        return NULL;
    }
    else
    {
        if (ndev_idx == NX_NB_TID_PER_STA * asr_hw->sta_max_num)
            sta_idx = asr_hw->sta_max_num + master_vif_idx(asr_vif);
        else
            sta_idx = ndev_idx / NX_NB_TID_PER_STA;

        //dbg(D_ERR, D_UWIFI_CTRL, "%s:ndev_idx=%d,sta=%d\n",__func__,ndev_idx,sta_idx);
        sta = &asr_hw->sta_table[sta_idx];
    }
    return sta;
}

uint8_t* asr_tx_aggr_get_copy_addr(struct asr_hw *asr_hw, uint32_t len)
{
    uint8_t* addr;

    //as the tx buffer way have changed, the tx buffer may exist more little buf segment, so more tx msg, to avoid too much tx msg, limit it here
    //when there are more than 20 buffer is not processed in tx direction, pause send more tx msg to queue.
    #if 0 // remove ?? add flow control later.
    if (asr_hw->tx_agg_env.aggr_buf_cnt > TX_AGG_BUF_UNIT_CNT)
        return 0;
    #endif
    if (asr_hw->tx_agg_env.last_aggr_buf_next_addr >= asr_hw->tx_agg_env.cur_aggr_buf_next_addr) {
        if (asr_hw->tx_agg_env.last_aggr_buf_next_addr + len <  (uint8_t*)asr_hw->tx_agg_env.aggr_buf->data + TX_AGG_BUF_SIZE) {
            addr = asr_hw->tx_agg_env.last_aggr_buf_next_addr;
            asr_hw->tx_agg_env.last_aggr_buf_next_addr += len;
            asr_hw->tx_agg_env.last_aggr_buf_next_idx++;
            asr_hw->tx_agg_env.aggr_buf_cnt++;
        } else {
            if ((uint8_t*)asr_hw->tx_agg_env.aggr_buf->data + len < asr_hw->tx_agg_env.cur_aggr_buf_next_addr) {
                addr = (uint8_t*)asr_hw->tx_agg_env.aggr_buf->data;
                asr_hw->tx_agg_env.last_aggr_buf_next_addr = addr + len;
                asr_hw->tx_agg_env.last_aggr_buf_next_idx = 1;
                asr_hw->tx_agg_env.aggr_buf_cnt++;
            } else {
                return 0;
            }
        }
    } else {
        if (asr_hw->tx_agg_env.last_aggr_buf_next_addr + len < asr_hw->tx_agg_env.cur_aggr_buf_next_addr) {
            addr = asr_hw->tx_agg_env.last_aggr_buf_next_addr;
            asr_hw->tx_agg_env.last_aggr_buf_next_addr += len;
            asr_hw->tx_agg_env.last_aggr_buf_next_idx++;
            asr_hw->tx_agg_env.aggr_buf_cnt++;
        } else {
            return 0;
        }
    }
    /*
    if((asr_hw->tx_agg_env.last_aggr_buf_next_addr != 0 && asr_hw->tx_agg_env.last_aggr_buf_next_addr < asr_hw->tx_agg_env.cur_aggr_buf_next_addr && asr_hw->tx_agg_env.last_aggr_buf_next_addr + len > asr_hw->tx_agg_env.cur_aggr_buf_next_addr)||
        (asr_hw->tx_agg_env.last_aggr_buf_next_addr + len > (uint8_t*)asr_hw->tx_agg_env.aggr_buf->data + TX_AGG_BUF_SIZE &&  (uint8_t*)asr_hw->tx_agg_env.aggr_buf->data + len > asr_hw->tx_agg_env.cur_aggr_buf_next_addr) ||
        (asr_hw->tx_agg_env.last_aggr_buf_next_idx+5)%TX_AGG_BUF_UNIT_CNT == (asr_hw->tx_agg_env.cur_aggr_buf_next_idx)%TX_AGG_BUF_UNIT_CNT) //queue full
        return 0;

    addr = asr_hw->tx_agg_env.last_aggr_buf_next_addr;
    asr_hw->tx_agg_env.last_aggr_buf_next_idx++;
    if (asr_hw->tx_agg_env.last_aggr_buf_next_idx >= TX_AGG_BUF_UNIT_CNT) {
        asr_hw->tx_agg_env.last_aggr_buf_next_addr = (uint8_t*)asr_hw->tx_agg_env.aggr_buf->data;
        asr_hw->tx_agg_env.last_aggr_buf_next_idx = 0;
    }
    else {
        asr_hw->tx_agg_env.last_aggr_buf_next_addr += len;
    }
    */
    return addr;
}

bool asr_tx_ring_is_nearly_full(struct asr_hw * asr_hw)
{
    s32 empty_size_byte = 0;

    if (asr_hw->tx_agg_env.cur_aggr_buf_next_addr == asr_hw->tx_agg_env.last_aggr_buf_next_addr) {
        return false;
    }

    empty_size_byte =
        (s32) (asr_hw->tx_agg_env.cur_aggr_buf_next_addr - asr_hw->tx_agg_env.last_aggr_buf_next_addr);

    empty_size_byte = (empty_size_byte + TX_AGG_BUF_SIZE) % TX_AGG_BUF_SIZE;

    //dbg(D_ERR,D_UWIFI_DATA, "%s:empty_size_byte=%d,%d,cur=%p,last=%p.\n"
    //      , __func__, empty_size_byte, (TX_AGG_BUF_UNIT_CNT / 8 * TX_AGG_BUF_UNIT_SIZE), asr_hw->tx_agg_env.cur_aggr_buf_next_addr, asr_hw->tx_agg_env.last_aggr_buf_next_addr);

    if (empty_size_byte < (TX_AGG_BUF_UNIT_CNT / 8 * TX_AGG_BUF_UNIT_SIZE)) {
        return true;
    }

    return false;
}

bool asr_tx_ring_is_ready_push(struct asr_hw * asr_hw)
{
    s32 empty_size_byte = 0;

    if (asr_hw->tx_agg_env.cur_aggr_buf_next_addr == asr_hw->tx_agg_env.last_aggr_buf_next_addr) {
        return true;
    }

    empty_size_byte =
        (s32) ((u32) (asr_hw->tx_agg_env.cur_aggr_buf_next_addr) -
           (u32) (asr_hw->tx_agg_env.last_aggr_buf_next_addr));

    //dev_info(asr_hw->dev, "%s:empty_size_byte=%d.\n", __func__, empty_size_byte);
    empty_size_byte = (empty_size_byte + TX_AGG_BUF_SIZE) % TX_AGG_BUF_SIZE;

    if (empty_size_byte > (TX_AGG_BUF_UNIT_CNT * 4 / 5 * TX_AGG_BUF_UNIT_SIZE)) {
        return true;
    }

    return false;
}


extern bool mrole_enable;
extern bool asr_xmit_opt;

void asr_tx_opt_flow_ctrl(struct asr_hw * asr_hw,struct asr_vif *asr_vif,bool check_high_wm)
{
    if ((asr_hw == NULL) || (asr_vif == NULL)) {
        return;
    }

    if (check_high_wm) {
        // before vif xmit, check vif
        if (mrole_enable && (asr_hw->vif_started > 1)) {
            if (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_AP) {
                if ((asr_vif->tx_skb_cnt > CFG_VIF_CNT_HWM)       // 30 *0.5 * 0.8
                    //&& !netif_queue_stopped(asr_vif->ndev)
                    ) {
                        //dev_err(asr_hw->dev, "%s:tx buf full(%d),stop all cfg vif queue.\n", __func__,asr_vif->tx_skb_cnt);
                        //netif_tx_stop_all_queues(asr_vif->ndev);
                        //cfg_vif_disable_tx = true;
                        asr_vif->vif_disable_tx = true;
                } else
                    asr_vif->vif_disable_tx = false;
            } else {
                if ((asr_vif->tx_skb_cnt > TRA_VIF_CNT_HWM)     // 30 *0.5 * 0.8
                    //&& !netif_queue_stopped(asr_vif->ndev)
                    ) {
                        //dev_err(asr_hw->dev, "%s:tx buf full(%d),stop all traffic vif queue.\n", __func__,asr_vif->tx_skb_cnt);
                        //netif_tx_stop_all_queues(asr_vif->ndev);
                        //tra_vif_disable_tx = true;
                        asr_vif->vif_disable_tx = true;
                } else
                   asr_vif->vif_disable_tx = false;
            }
        } else {
            if ((asr_vif->tx_skb_cnt > UNIQUE_VIF_CNT_HWM)        // 30 * 0.8
                //&& !netif_queue_stopped(asr_vif->ndev)
                ) {
                //dev_info(asr_hw->dev, "%s:tx buf full,stop all unique vif queue.\n", __func__);
                //netif_tx_stop_all_queues(asr_vif->ndev);
                asr_vif->vif_disable_tx = true;
            } else
                asr_vif->vif_disable_tx = false;
        }
    }else {
        if (mrole_enable && (asr_hw->vif_started > 1)) {
            if (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_AP) {
                //    cfg vif flow ctrl.
                if ((asr_vif->tx_skb_cnt < CFG_VIF_CNT_LWM)      // 30 *0.5 * 0.2
                    //&& netif_queue_stopped(asr_vif->ndev)
                    ) {
                        //dev_err(asr_hw->dev, "%s:tx buf ready,wake all cfg vif.\n", __func__);
                        //netif_tx_wake_all_queues(asr_vif->ndev);
                        //cfg_vif_disable_tx = false;
                        asr_vif->vif_disable_tx = false;
                }
            } else {
                //    traffic vif flow ctrl.
                if ((asr_vif->tx_skb_cnt < TRA_VIF_CNT_LWM)    // 30 *0.5 * 0.2
                    //&& netif_queue_stopped(asr_vif->ndev)
                    ){
                        //dev_err(asr_hw->dev, "%s:tx buf ready,wake all cfg vif.\n", __func__);
                        //netif_tx_wake_all_queues(asr_vif->ndev);
                        //tra_vif_disable_tx = false;
                        asr_vif->vif_disable_tx = false;
                }
            }
        } else {
            if ((asr_vif->tx_skb_cnt < UNIQUE_VIF_CNT_LWM)       // 30  * 0.2
                //&& netif_queue_stopped(asr_vif->ndev)
                ) {
                //dev_info(asr_hw->dev, "%s:tx buf ready,wake all queue.\n", __func__);
                //netif_tx_wake_all_queues(asr_vif->ndev);
                asr_vif->vif_disable_tx = false;
            }
        }
    }
}


void asr_tx_agg_flow_ctrl(struct asr_hw * asr_hw,struct asr_vif *asr_vif,bool check_high_wm)
{
    if ((asr_hw == NULL) || (asr_vif == NULL)) {
        return;
    }

    if (check_high_wm) {
        // before vif xmit, check vif
        if (mrole_enable && (asr_hw->vif_started > 1)) {
            if (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_AP) {
                if ((asr_vif->txring_bytes > CFG_VIF_HWM)
                    //&& !netif_queue_stopped(asr_vif->ndev)
                    ) {
                        //dev_err(asr_hw->dev, "%s:tx buf full(%d),stop all cfg vif queue.\n", __func__,asr_vif->txring_bytes);
                        //netif_tx_stop_all_queues(asr_vif->ndev);
                        asr_vif->vif_disable_tx = true;
                } else
                   asr_vif->vif_disable_tx = false;
            } else {
                if ((asr_vif->txring_bytes > TRA_VIF_HWM)
                    //&& !netif_queue_stopped(asr_vif->ndev)
                    ) {
                        //dev_err(asr_hw->dev, "%s:tx buf full(%d),stop all traffic vif queue.\n", __func__,asr_vif->txring_bytes);
                        //netif_tx_stop_all_queues(asr_vif->ndev);
                        asr_vif->vif_disable_tx = true;
                } else
                    asr_vif->vif_disable_tx = false;
            }
        } else {
            if ((asr_vif->txring_bytes > UNIQUE_VIF_HWM)
                //&& !netif_queue_stopped(asr_vif->ndev)
                ) {
                //dev_info(asr_hw->dev, "%s:tx buf full,stop all unique vif queue.\n", __func__);
                //netif_tx_stop_all_queues(asr_vif->ndev);
                asr_vif->vif_disable_tx = true;
            } else
                asr_vif->vif_disable_tx = false;
        }
    }
    else {
        if (mrole_enable && (asr_hw->vif_started > 1)) {
            if (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_AP) {
                //  cfg vif flow ctrl.
                if ((asr_vif->txring_bytes < CFG_VIF_LWM)
                    //&& netif_queue_stopped(asr_vif->ndev)
                    ) {
                        //dev_err(asr_hw->dev, "%s:tx buf ready,wake all cfg vif.\n", __func__);
                        //netif_tx_wake_all_queues(asr_vif->ndev);
                        asr_vif->vif_disable_tx = false;
                }
            } else {
                //  traffic vif flow ctrl.
                if ((asr_vif->txring_bytes < TRA_VIF_LWM)
                    //&& netif_queue_stopped(asr_vif->ndev)
                    ){
                        //dev_err(asr_hw->dev, "%s:tx buf ready,wake all cfg vif.\n", __func__);
                        //netif_tx_wake_all_queues(asr_vif->ndev);
                        asr_vif->vif_disable_tx = false;
                }
            }
        } else {
            if ((asr_vif->txring_bytes < UNIQUE_VIF_LWM)
                //&& netif_queue_stopped(asr_vif->ndev)
                ) {
                //dev_info(asr_hw->dev, "%s:tx buf ready,wake all queue.\n", __func__);
                //netif_tx_wake_all_queues(asr_vif->ndev);
                asr_vif->vif_disable_tx = false;
            }
        }
    }
}


void asr_tx_flow_ctrl(struct asr_hw * asr_hw,struct asr_vif *asr_vif,bool check_high_wm)
{
      if (asr_xmit_opt)
          asr_tx_opt_flow_ctrl(asr_hw,asr_vif,check_high_wm);
      else
          asr_tx_agg_flow_ctrl(asr_hw,asr_vif,check_high_wm);
}



void asr_tx_agg_buf_reset(struct asr_hw *asr_hw)
{
    if (NULL == asr_hw)
        return;

    //clear the tx buffer, the buffered data will be dropped. there will no send data in tx direction
    asr_hw->tx_agg_env.last_aggr_buf_next_addr = (uint8_t *) asr_hw->tx_agg_env.aggr_buf->data;
    asr_hw->tx_agg_env.last_aggr_buf_next_idx = 0;
    asr_hw->tx_agg_env.cur_aggr_buf_next_addr = (uint8_t *) asr_hw->tx_agg_env.aggr_buf->data;
    asr_hw->tx_agg_env.cur_aggr_buf_next_idx = 0;
    asr_hw->tx_agg_env.aggr_buf_cnt = 0;

}

void asr_tx_agg_buf_mask_vif(struct asr_hw *asr_hw,struct asr_vif *asr_vif)
{
     // set tx pkt unvalid per vif and let fw hif to drop.
     struct hostdesc *hostdesc_temp;
     struct asr_tx_agg *tx_agg_env = &(asr_hw->tx_agg_env);
     u16 aggr_buf_cnt = tx_agg_env->aggr_buf_cnt;
     u8 *temp_cur_next_addr,*tx_agg_buf_wr_ptr;
     u16 tx_agg_buf_wr_idx;
     u16 temp_len;
     u16 vif_drop_cnt = 0;

     temp_cur_next_addr = tx_agg_env->cur_aggr_buf_next_addr;

     tx_agg_buf_wr_ptr  = tx_agg_env->last_aggr_buf_next_addr;
     tx_agg_buf_wr_idx  = tx_agg_env->last_aggr_buf_next_idx;

     while (aggr_buf_cnt)
     {
         temp_len = *((u16 *) temp_cur_next_addr) + 2;
         hostdesc_temp = (struct hostdesc *)(temp_cur_next_addr);

         if ((hostdesc_temp->vif_idx == asr_vif->vif_index)) {
              // set pkt unvalid.
              hostdesc_temp->flags |= TXU_CNTRL_DROP;
              vif_drop_cnt++;
         }

         temp_cur_next_addr += temp_len;

         aggr_buf_cnt--;

        // handle cur next addr close to txbuf end.
        if ((aggr_buf_cnt == tx_agg_buf_wr_idx) &&
            (tx_agg_buf_wr_ptr < temp_cur_next_addr)) {
            temp_cur_next_addr = (uint8_t *) tx_agg_env->aggr_buf->data;
        }
     }

     dbg(D_INF, D_UWIFI_CTRL, "%s (%d,%d) vif frame drop cnt %d...\n", __func__,asr_vif->vif_index, ASR_VIF_TYPE(asr_vif),vif_drop_cnt);

}


//============================tx opt skb start=====================================================


//============================tx opt skb end=======================================================
#if 0
bool asr_check_tcp_duplicate(uint8_t *xmit_buf,uint32_t xmit_len,struct asr_sta *sta,uint8_t tid)
{
    struct asr_pbuf* pbuf = NULL;
    struct ethhdr *eth = NULL;
    struct asr_ip_hdr *ip_hdr = NULL;
    struct asr_tcp_hdr *tcp_hdr = NULL;


    if (!xmit_buf ||  !sta || tid >= NX_NB_TID_PER_STA) {
        return false;
    }

    if (xmit_len != 54) {
        return false;
    }

    eth = (struct ethhdr *)xmit_buf;
    ip_hdr = (struct asr_ip_hdr*)((uint8_t*)xmit_buf + 14);
    tcp_hdr = (struct asr_tcp_hdr*)((uint8_t*)ip_hdr + sizeof(struct asr_ip_hdr));


    if ((wlan_ntohs(eth->h_proto)) == 0x800 && ip_hdr->_proto == 0x6) {

        if (sta->tcp_ack[tid].src == (wlan_ntohs(tcp_hdr->src)) &&
            sta->tcp_ack[tid].dest == (wlan_ntohs(tcp_hdr->dest)) &&
            sta->tcp_ack[tid].acknum == (wlan_ntohl(tcp_hdr->acknum)) &&
            ((wlan_ntohs(tcp_hdr->flags)) & 0x1FF) == 0x10  &&
            asr_rtos_get_time() - sta->tcp_ack[tid].time < 5 &&
            !memcmp(eth->h_dest,sta->tcp_ack[tid].h_dest,6) ) {

            // dbg(D_ERR, D_UWIFI_CTRL, "%s:%d,%d,%u,%u,%u\n",__func__,tid,sta->sta_idx
            //     ,sta->tcp_ack[tid].time,asr_rtos_get_time()
            //     ,wlan_ntohl(tcp_hdr->acknum));
            asr_msleep(1);
            return true;
        }

        memcpy(sta->tcp_ack[tid].h_dest,eth->h_dest,6);
        sta->tcp_ack[tid].time = asr_rtos_get_time();
        sta->tcp_ack[tid].acknum = wlan_ntohl(tcp_hdr->acknum);
        sta->tcp_ack[tid].src = wlan_ntohs(tcp_hdr->src);
        sta->tcp_ack[tid].dest = wlan_ntohs(tcp_hdr->dest);
    }


    return false;
}
#endif
/**
 * netdev_tx_t (*ndo_start_xmit)(struct sk_buff *skb,
 *                               struct net_device *dev);
 *    Called when a packet needs to be transmitted.
 *    Must return NETDEV_TX_OK , NETDEV_TX_BUSY.
 *        (can also return NETDEV_TX_LOCKED if NETIF_F_LLTX)
 *
 *  - Initialize the desciptor for this pkt (stored in skb before data)
 *  - Push the pkt in the corresponding Txq
 *  - If possible (i.e. credit available and not in PS) the pkt is pushed
 *    to fw
 */
extern bool arp_debug_log;
extern bool txlogen;
uint32_t g_tra_vif_drv_txcnt;
uint32_t g_cfg_vif_drv_txcnt;
uint32_t g_skb_expand_cnt;

int asr_start_xmit_aggbuf(struct asr_vif *asr_vif, struct sk_buff *skb)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;
    struct ethhdr *eth;
    struct asr_sta *sta;
    struct asr_txq *txq;
    int headroom;
    uint16_t frame_len;
    uint16_t frame_offset;
    uint8_t tid;
    uint32_t temp_len;
    struct hostdesc *hostdesc;
    uint32_t tx_data_end_token = 0xAECDBFCA;
    void *xmit_buf;
    uint32_t xmit_len;
    int ret = ASR_ERR_CONN;
    bool is_dhcp = false;
    struct asr_pbuf* pbuf = NULL;
    uint16_t pbuf_len = 0;
    uint8_t times = 0;
    uint16_t index = 0;


    if (skb->private[0] == 0xAABBCCDD) {
        pbuf = (struct asr_pbuf*)(skb->private[1]);
        xmit_buf = pbuf->payload;
        xmit_len = skb->private[2];
    } else if (skb->private[0] == 0xDEADBEAF) {
        xmit_buf = (void *)skb->private[1];
        xmit_len = skb->private[2];
    } else
    {
        xmit_buf = skb->data;
        xmit_len = skb->len;
    }
    /* Get the STA id and TID information */
    sta = asr_get_tx_info(asr_vif, skb, &tid);
    if (!sta) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:sta is NULL\n",__func__);
        goto free;
    }

    /* sta may be invalid for first arp from network stack after ap mode startup */
    if (!sta->valid)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:sta(%d) is not vaild yet\n",__func__, sta->sta_idx);
        goto free;
    }

    if (asr_test_bit(ASR_DEV_RESTARTING, &asr_hw->phy_flags)
        || (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_STATION
        && (asr_test_bit(ASR_DEV_STA_DISCONNECTING, &asr_vif->dev_flags)
            || !asr_test_bit(ASR_DEV_STA_CONNECTED, &asr_vif->dev_flags)
            || asr_test_bit(ASR_DEV_CLOSEING, &asr_vif->dev_flags)
            || asr_test_bit(ASR_DEV_PRECLOSEING, &asr_vif->dev_flags)))) {

        dbg(D_ERR, D_UWIFI_CTRL, "%s:drop,flags(%08X)\n", __func__, (unsigned int)asr_vif->dev_flags);
        goto free;
    }

    #if 0
    if (asr_check_tcp_duplicate(xmit_buf,pbuf?pbuf->len:xmit_len,sta,tid)) {
        ret = ASR_ERR_OK;
        goto free;
    }
    #endif

    txq = asr_txq_sta_get(sta, tid, NULL, asr_hw);

    /* Retrieve the pointer to the Ethernet data */
    eth = (struct ethhdr *)xmit_buf;

    headroom  = sizeof(struct asr_txhdr) + ASR_SWTXHDR_ALIGN_PADS((long)eth);

    frame_len = (((uint16_t)xmit_len) - sizeof(*eth));
    frame_offset = (headroom + sizeof(*eth));
    temp_len = ASR_ALIGN_BLKSZ_HI((frame_offset + frame_len) + 4);

    if (temp_len > ASR_SDIO_DATA_MAX_LEN) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s frame drop, temp_len=%u,hostdesc=%u,headrom=%d,eth=%u,skb_len=%d \n",
            __func__, temp_len,(unsigned int)sizeof(struct hostdesc),headroom,(unsigned int)sizeof(*eth),(u16) skb->len );
        goto free;
    }

    /************** add multi-vif txcnt for tx fc.. ****************************************/
    if (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_STATION)
            g_tra_vif_drv_txcnt++;

    if (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_AP)
            g_cfg_vif_drv_txcnt++;

    // tx flow ctrl.
    if (mrole_enable)
    {
       asr_tx_flow_ctrl(asr_hw,asr_vif,true);
       while (asr_vif->vif_disable_tx == true && ++times < 50) {
           // just delay to wait tx task free tx agg buf.
            #if defined(AWORKS) || defined(HX_SDK) || defined(JXC_SDK)
           asr_rtos_delay_milliseconds(2);
            #else
           asr_rtos_delay_milliseconds(10);
            #endif
           asr_tx_flow_ctrl(asr_hw,asr_vif,true);
       }
    } else {
        while (asr_tx_ring_is_nearly_full(asr_hw) && ++times < 50) {
            // just delay to wait tx task free tx agg buf.
            #if defined(AWORKS) || defined(HX_SDK) || defined(JXC_SDK)
                asr_rtos_delay_milliseconds(2);
            #else
                asr_rtos_delay_milliseconds(10);
            #endif
        }
    }

    asr_rtos_lock_mutex(&asr_hw->tx_agg_env_lock);

    if (asr_tx_ring_is_nearly_full(asr_hw) && !asr_vif->is_resending) {
        //dbg(D_ERR, D_UWIFI_CTRL, "%s:tx buf full,stop all queue.\n", __func__);

        // lwip handle this error ,stop xmit and driver drop current pkt.
        ret = ASR_ERR_MEM;
        asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);

        // just delay to wait tx task free tx agg buf.
        asr_rtos_delay_milliseconds(10);

        goto free;
    }

    hostdesc = (struct hostdesc*)asr_tx_aggr_get_copy_addr(asr_hw, temp_len);
    if(hostdesc)
    {
        if (skb->private[0] != 0xAABBCCDD) {
            is_dhcp = check_is_dhcp(asr_vif, true, wlan_ntohs(eth->h_proto), NULL, NULL,
                xmit_buf + sizeof(*eth), (u16) xmit_len - sizeof(*eth));
        }
        if (xmit_buf && is_dhcp) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s vif type%d tx dhcp: dst: %x:%x:%x:%x:%x:%x, src: %x:%x:%x:%x:%x:%x, 0x%x", __func__,
                    asr_vif->iftype,
                    eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5],
                    eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5],
                    eth->h_proto);
        } else {
                if (arp_debug_log || txlogen)
                    dbg(D_ERR, D_UWIFI_CTRL, "%s vif type%d tx eth header, dst: %x:%x:%x:%x:%x:%x, src: %x:%x:%x:%x:%x:%x, 0x%x", __func__,
                    asr_vif->iftype,
                        eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5],
                        eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5],
                        eth->h_proto);
        }
        hostdesc->queue_idx = txq->hwq->id;
        memcpy(&hostdesc->eth_dest_addr, eth->h_dest, ETH_ALEN);
        memcpy(&hostdesc->eth_src_addr, eth->h_source, ETH_ALEN);
        hostdesc->ethertype = eth->h_proto;
        hostdesc->staid = sta->sta_idx;
        hostdesc->tid = tid;
        hostdesc->vif_idx = asr_vif->vif_index;
        if (asr_vif->use_4addr && (sta->sta_idx < asr_hw->sta_max_num))
            hostdesc->flags = TXU_CNTRL_USE_4ADDR;
        else
            hostdesc->flags = 0;
        hostdesc->packet_len = frame_len;
        hostdesc->packet_addr = 0;
        hostdesc->packet_offset = frame_offset;
        hostdesc->sdio_tx_total_len = (frame_offset + frame_len);
        hostdesc->txq = (uint32_t)txq;
        hostdesc->agg_num = 0;
        hostdesc->sdio_tx_len = temp_len-2;
        if (skb->private[0] == 0xAABBCCDD) {
            pbuf_len = 0;
            for(; pbuf != NULL; pbuf = pbuf->next)
            {
                memcpy((uint8_t*)hostdesc + headroom + pbuf_len, pbuf->payload, pbuf->len);
                pbuf_len += pbuf->len;
            }
            memcpy((uint8_t*)hostdesc + temp_len -4, &tx_data_end_token, 4);
        } else {
            memcpy((uint8_t*)hostdesc + headroom, xmit_buf, xmit_len);
            memcpy((uint8_t*)hostdesc + temp_len -4, &tx_data_end_token, 4);
        }

        //tell to transfer, push to queue_front
        //uwifi_sdio_event_set(UWIFI_SDIO_EVENT_TX);

        asr_vif->txring_bytes += temp_len;

        if ((asr_hw->tx_agg_env.aggr_buf_cnt == 1 || is_dhcp)
             && ((asr_hw->tx_use_bitmap & 0xFFFE) == 0xFFFE)
             && (asr_hw->xmit_first_trigger_flag == false)
            ) {
            //asr_rtos_lock_mutex(&asr_hw->tx_agg_timer_lock);
            //stop the timer
            if (asr_hw->tx_aggr_timer.handle && asr_rtos_is_timer_running(&(asr_hw->tx_aggr_timer))){
                asr_rtos_stop_timer(&(asr_hw->tx_aggr_timer));
            }
            //asr_rtos_unlock_mutex(&asr_hw->tx_agg_timer_lock);
           //tell to transfer, push to queue_front

            asr_hw->xmit_first_trigger_flag = true;

            //dbg(D_ERR, D_UWIFI_CTRL,"%s:%d\n",__func__,asr_hw->tx_agg_env.aggr_buf_cnt);
            uwifi_sdio_event_set(UWIFI_SDIO_EVENT_TX);
        } else {
            //asr_rtos_lock_mutex(&asr_hw->tx_agg_timer_lock);
            //start a timer
            if (!asr_rtos_is_timer_running(&(asr_hw->tx_aggr_timer))){
                //asr_rtos_stop_timer(&(asr_hw->tx_aggr_timer));
                asr_rtos_start_timer(&(asr_hw->tx_aggr_timer));
            }
            //asr_rtos_unlock_mutex(&asr_hw->tx_agg_timer_lock);
        }

        ret = ASR_ERR_OK;
    }
    else
    {
        if (txlogen)
            dbg(D_ERR, D_UWIFI_CTRL,"%s: drop data,%d \n",__func__,asr_hw->tx_agg_env.aggr_buf_cnt);


        ret = ASR_ERR_MEM;
    }
    asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);

free:

    if (skb->private[0] == 0xAABBCCDD) {
        skb_reinit(skb);
    } else {
        if(skb_push(skb, PKT_BUF_RESERVE_HEAD))
        dev_kfree_skb_tx(skb);
    }

    return ret;
}


void asr_recycle_tx_skb(struct asr_hw *asr_hw,struct sk_buff *skb)
{
    if (!asr_hw || !skb) {
        return;
    }

    skb_reinit(skb);

    skb_queue_tail(&asr_hw->tx_sk_free_list, skb);
}

int asr_start_xmit_opt(struct asr_vif *asr_vif, struct sk_buff *skb)
{
    int headroom = 0;
    u16 frame_len = 0;
    u16 frame_offset = 0;
    u32 temp_len = 0;
    struct hostdesc *hostdesc = NULL, *hostdesc_new = NULL;
    uint32_t tx_data_end_token = 0xAECDBFCA;
    struct asr_sta *sta;
    struct asr_txq *txq;
    u8 tid;
    struct ethhdr *eth;
    bool is_dhcp = false;
    u8 dhcp_type = 0;
    u8* d_mac = NULL;
    int max_headroom;
    int tx_sk_list_cnt;
    u32 tail_align_len = 0;
    int ret = ASR_ERR_CONN;
    uint8_t times = 0;
    struct asr_hw *asr_hw = NULL;
    struct sk_buff *skb_expand = NULL;

    if (!skb) {
        return ret;
    }

    if (!asr_vif) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:vif is NULL\n", __func__);
        goto xmit_sdio_free;
    }

    asr_hw = asr_vif->asr_hw;

    if (!asr_hw) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:asr_hw is NULL\n", __func__);
        goto xmit_sdio_free;
    }

    max_headroom = sizeof(struct asr_txhdr) + 3;

    /* check whether the current skb can be used */
    if ((skb_headroom(skb) < max_headroom) ||
        (skb_tailroom(skb) < (32+4))) {

            dbg(D_ERR, D_UWIFI_CTRL, "%s:skb error %d <  %d , %d (%p - %p) (%p - %p)\n", __func__,skb_headroom(skb), max_headroom, skb->len,
                                                                                     skb->end,skb->tail,
                                                                                     skb->head,skb->data);
            goto xmit_sdio_free;
    }


    if ((skb->end - skb->head) < TX_AGG_BUF_UNIT_SIZE) {
        // call by uwifi_tx_skb_data case.
        // should do skb_copy_expand
        if (!skb_queue_empty(&asr_hw->tx_sk_free_list)) {

            struct sk_buff *newskb = skb_dequeue(&asr_hw->tx_sk_free_list);

            skb_reserve(newskb, PKT_BUF_RESERVE_HEAD);

            memcpy(newskb->data, skb->data, skb->len);
            //fill in pkt.
            skb_put(newskb,skb->len);

            skb_expand = skb;
            skb = newskb;
            g_skb_expand_cnt++;
        }else {

            dbg(D_ERR,D_UWIFI_DATA,"%s: tx_sk_free_list run out", __func__);

            return ASR_ERR_BUF;
        }
    }

    /* Get the STA id and TID information */
    sta = asr_get_tx_info(asr_vif, skb, &tid);
    if (!sta) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:sta is NULL , vif type (%d) , tid = %d\n", __func__,asr_vif->iftype, tid );
        goto xmit_sdio_free;
    }

    txq = asr_txq_sta_get(sta, tid, NULL, asr_hw);
    if (txq->idx == TXQ_INACTIVE) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:drop,vif type (%d) , phy_flags(0x%X),dev_flags(0x%X),tid=%d,sta_idx=%d\n",
             __func__, asr_vif->iftype, (unsigned int)asr_hw->phy_flags, (unsigned int)asr_vif->dev_flags,tid, sta->sta_idx);
        goto xmit_sdio_free;
    }

    if (!txq->hwq) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s frame drop, no hwq \n", __func__);
        goto xmit_sdio_free;
    }

    /*    add struct hostdesc and chain to tx_sk_list*/

    /* the real offset is :
     sizeof(struct txdesc_api)(44) + pad(4) + sizeof(*eth)(14) and
     it should not bellow the wireless header(46) + sdio_total_len_offset(4)
     */

    /* Retrieve the pointer to the Ethernet data */
    eth = (struct ethhdr *)skb->data;

    headroom = sizeof(struct asr_txhdr);
    frame_len = (u16) skb->len - sizeof(*eth);
    frame_offset = headroom + sizeof(*eth);

    temp_len = ASR_ALIGN_BLKSZ_HI(frame_offset + frame_len + 4);

    if (temp_len > ASR_SDIO_DATA_MAX_LEN) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s frame drop, temp_len=%u,hostdesc=%u,headrom=%d,eth=%u,skb_len=%d \n",
            __func__, temp_len,(unsigned int)sizeof(struct hostdesc),headroom,(unsigned int)sizeof(*eth),(u16) skb->len );
        goto xmit_sdio_free;
    }

    // tx flow ctrl.
    {
       asr_tx_flow_ctrl(asr_hw,asr_vif,true);
       while (asr_vif->vif_disable_tx == true && ++times < 50) {
           // just delay to wait tx task free tx agg buf.
           #if defined(AWORKS)
           asr_rtos_delay_milliseconds(2);
           #else
           asr_rtos_delay_milliseconds(10);
           #endif
           asr_tx_flow_ctrl(asr_hw,asr_vif,true);
       }
    }

    if (asr_vif->vif_disable_tx == true && !asr_vif->is_resending) {
        dbg(D_INF, D_UWIFI_CTRL, "%s:tx buf full,stop all queue.\n", __func__);
        // lwip handle this error ,stop xmit and driver drop current pkt.
        ret = ASR_ERR_MEM;

        // just delay to wait tx task free tx agg buf.
        asr_rtos_delay_milliseconds(10);
        goto xmit_sdio_free;
    }

    #if 0
    if (wlan_ntohs(eth->h_proto) == ETH_P_PAE) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s tx eapol, da(%x:%x:%x:%x:%x:%x), sa(%x:%x:%x:%x:%x:%x), sta_idx=%d, vif_idx=%d  \n", __func__,
                                     eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5],
                                     eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5],
                                     sta->sta_idx,
                                     asr_vif->vif_index );

       }
    #endif
    /****************************************************************************************************/
    is_dhcp = check_is_dhcp(asr_vif, true, wlan_ntohs(eth->h_proto), &dhcp_type, &d_mac,
                  skb->data + sizeof(*eth), (u16) skb->len - sizeof(*eth));

    /************** add multi-vif txcnt for tx fc.. ****************************************/
    if (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_STATION)
            g_tra_vif_drv_txcnt++;

    if (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_AP)
            g_cfg_vif_drv_txcnt++;

    /************************** hostdesc prepare . ******************************************************/

    /*
    hif data structure:
               hostdesc           data               tx_data_end_token
    len:       headroom           skb->len             4

    skb operation:
       skb_push(skb, headroom);   fill hostdesc.

       skb_put (skb,4)           ;  fill tx_data_end_token.

    */
    skb_push(skb, headroom);

    hostdesc = (struct hostdesc *)skb->data;

    // Fill-in the hostdesc
    hostdesc->queue_idx = txq->hwq->id;
    memcpy(&hostdesc->eth_dest_addr, eth->h_dest, ETH_ALEN);
    memcpy(&hostdesc->eth_src_addr, eth->h_source, ETH_ALEN);
    hostdesc->ethertype = eth->h_proto;
    hostdesc->staid = sta->sta_idx;
    hostdesc->tid = tid;
    hostdesc->vif_idx = asr_vif->vif_index;

    if (asr_vif->use_4addr && (sta->sta_idx < asr_hw->sta_max_num))
        hostdesc->flags = TXU_CNTRL_USE_4ADDR;
    else
        hostdesc->flags = 0;

    hostdesc->packet_len = frame_len;
    hostdesc->packet_addr = 0;
    hostdesc->packet_offset = frame_offset;
    hostdesc->sdio_tx_total_len = frame_offset + frame_len;
    hostdesc->txq = (u32) (uintptr_t) txq;
    hostdesc->agg_num = 0;
    hostdesc->sdio_tx_len = temp_len - 2;
    hostdesc->timestamp = 0;
    hostdesc->sn = 0xFFFF;     // set default value.

    //memcpy((u8 *) hostdesc + headroom, skb->data, skb->len);

    //add tx_data_end_token to skb.
    tail_align_len = temp_len - skb->len ;

    if (tail_align_len < 4 || (skb->tail + tail_align_len > skb->end)) {
        dbg(D_ERR, D_UWIFI_CTRL, "[%s] unlikely ,temp_len=%d ,skb_len=%d ,tail_len=%d, skb(0x%x ,0x%x)\n",__func__,temp_len,
                                                        skb->len,tail_align_len,
                                                        (u32)skb->tail, (u32)skb->end);
        goto xmit_sdio_free;
    }

    skb_put(skb,tail_align_len);
    
    memcpy((uint8_t*)hostdesc + temp_len -4, &tx_data_end_token, 4);

    // chain skb to tk_list.
    asr_rtos_lock_mutex(&asr_hw->tx_lock);
    skb_queue_tail(&asr_hw->tx_sk_list, skb);
    asr_vif->tx_skb_cnt ++;
    tx_sk_list_cnt = skb_queue_len(&asr_hw->tx_sk_list);
    asr_rtos_unlock_mutex(&asr_hw->tx_lock);

    /****************************************************************************************************/
    if (txlogen)
        dbg(D_ERR, D_UWIFI_CTRL, "[%s] type=%d ,sta_idx=%d ,vif_idx=%d ,frm_len=%d (%d %d %ld),eth_type=0x%x\n",__func__,
                                      ASR_VIF_TYPE(asr_vif), sta ? sta->sta_idx : 0xFF , asr_vif ? asr_vif->vif_index : 0xFF ,
                                      frame_len,tail_align_len,headroom,sizeof(*eth),hostdesc->ethertype);

    if ((tx_sk_list_cnt == 1 || is_dhcp)
         && ((asr_hw->tx_use_bitmap & 0xFFFE) == 0xFFFE)
         && (asr_hw->xmit_first_trigger_flag == false)
        ) {
        //asr_rtos_lock_mutex(&asr_hw->tx_agg_timer_lock);
        //stop the timer
        if (asr_hw->tx_aggr_timer.handle && asr_rtos_is_timer_running(&(asr_hw->tx_aggr_timer))){
            asr_rtos_stop_timer(&(asr_hw->tx_aggr_timer));
        }
        //asr_rtos_unlock_mutex(&asr_hw->tx_agg_timer_lock);
       //tell to transfer, push to queue_front

        asr_hw->xmit_first_trigger_flag = true;

        //dbg(D_ERR, D_UWIFI_CTRL,"%s:%d\n",__func__,asr_hw->tx_agg_env.aggr_buf_cnt);
        uwifi_sdio_event_set(UWIFI_SDIO_EVENT_TX);
    } else {
        //asr_rtos_lock_mutex(&asr_hw->tx_agg_timer_lock);
        //start a timer
        if (!asr_rtos_is_timer_running(&(asr_hw->tx_aggr_timer))){
            //asr_rtos_stop_timer(&(asr_hw->tx_aggr_timer));
            asr_rtos_start_timer(&(asr_hw->tx_aggr_timer));
        }
        //asr_rtos_unlock_mutex(&asr_hw->tx_agg_timer_lock);
    }

    ret = ASR_ERR_OK;

    // send success, free expand original skb here.
    if (skb_expand) {
        if(skb_push(skb_expand, PKT_BUF_RESERVE_HEAD))
            dev_kfree_skb_tx(skb_expand);
    }

    return ret;

xmit_sdio_free:
    if (asr_vif)
        asr_vif->net_stats.tx_dropped++;

    asr_recycle_tx_skb(asr_hw,skb);

    return ret;

}


int asr_start_xmit(struct asr_vif *asr_vif, struct sk_buff *skb){

    if (asr_xmit_opt) {
        return asr_start_xmit_opt(asr_vif, skb);
    } else
        return asr_start_xmit_aggbuf(asr_vif, skb);
}

/**
 * _ieee80211_is_robust_mgmt_frame - check if frame is a robust management frame
 * @hdr: the frame (buffer must include at least the first octet of payload)
 */
static inline bool _ieee80211_is_robust_mgmt_frame(struct ieee80211_hdr *hdr)
{
    if (ieee80211_is_disassoc(hdr->frame_control) ||
        ieee80211_is_deauth(hdr->frame_control))
        return true;

    if (ieee80211_is_action(hdr->frame_control)) {
        uint8_t *category;

        /*
         * Action frames, excluding Public Action frames, are Robust
         * Management Frames. However, if we are looking at a Protected
         * frame, skip the check since the data may be encrypted and
         * the frame has already been found to be a Robust Management
         * Frame (by the other end).
         */
        if (ieee80211_has_protected(hdr->frame_control))
            return true;
        category = ((uint8_t *) hdr) + 24;
        return *category != WLAN_CATEGORY_PUBLIC &&
            *category != WLAN_CATEGORY_HT &&
            *category != WLAN_CATEGORY_SELF_PROTECTED &&
            *category != WLAN_CATEGORY_VENDOR_SPECIFIC;
    }

    return false;
}

/**
 * ieee80211_is_robust_mgmt_frame - check if skb contains a robust mgmt frame
 * @skb: the skb containing the frame, length will be checked
 */
bool ieee80211_is_robust_mgmt_frame(uint16_t frame_len, const uint8_t *ieee80211_hdr_v)
{
    if (frame_len < 25)
        return false;
    return _ieee80211_is_robust_mgmt_frame((struct ieee80211_hdr *)ieee80211_hdr_v);
}

/**
 * asr_start_mgmt_xmit - Transmit a management frame
 *
 * @vif: Vif that send the frame
 * @sta: Destination of the frame. May be NULL if the destiantion is unknown
 *       to the AP.
 * @params: Mgmt frame parameters
 * @offchan: Indicate whether the frame must be send via the offchan TXQ.
 *           (is is redundant with params->offchan ?)
 * @cookie: updated with a unique value to identify the frame with upper layer
 *
 */
int asr_start_mgmt_xmit_aggbuf(struct asr_vif *vif, struct asr_sta *sta,
                         struct cfg80211_mgmt_tx_params *params, bool offchan,
                         uint64_t *cookie)
{
    struct asr_hw *asr_hw = vif->asr_hw;
    uint16_t frame_len;
    //uint8_t *data;
    struct asr_txq *txq;
    bool robust;
    uint16_t temp_len;
    uint16_t frame_offset;
    struct hostdesc *hostdesc;
    uint32_t tx_data_end_token = 0xAECDBFCA;

    frame_len = params->len;

    frame_offset = sizeof(struct txdesc_api) + HOST_PAD_LEN;
    temp_len = ASR_ALIGN_BLKSZ_HI(frame_offset + frame_len + 4);
    /* Set TID and Queues indexes */
#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
    if(asr_hw->monitor_vif_idx != 0xff)
        txq = asr_txq_vif_get_idle_mode(asr_hw,IDLE_HWQ_IDX,NULL);
    else if (sta)
    {
        txq = asr_txq_sta_get(sta, 8, NULL, asr_hw);  //VO
    } else
#else
    if (sta){
        txq = asr_txq_sta_get(sta, 8, NULL, asr_hw);  //VO
    } else
#endif
    {
        if (offchan)
            txq = &asr_hw->txq[asr_hw->sta_max_num * NX_NB_TXQ_PER_STA + asr_hw->vif_max_num * NX_NB_TXQ_PER_VIF];
        else
            txq = asr_txq_vif_get(vif, NX_UNK_TXQ_TYPE, NULL);
    }
    /*
    if (sta) {

        txq = asr_txq_sta_get(sta, 8, NULL, asr_hw);
    } else {
        if (offchan)
            txq = &asr_hw->txq[NX_OFF_CHAN_TXQ_IDX];
        else
            txq = asr_txq_vif_get(vif, NX_UNK_TXQ_TYPE, NULL);
    }
    */
    /* Ensure that TXQ is active */
    if (txq->idx == TXQ_INACTIVE) {
        dbg(D_ERR, D_UWIFI_CTRL, "TXQ inactive\n");
        return -EBUSY;
    }
#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
    /*
    if(params->cus_flags)
    {
        asr_rtos_free((uint8_t *)params->buf);
        params->buf = NULL;
    }
    */
#endif
    robust = ieee80211_is_robust_mgmt_frame(frame_len, params->buf);

    /* Update CSA counter if present */
    /*
    if (params->n_csa_offsets &&
            vif->iftype == NL80211_IFTYPE_AP &&
            vif->ap.csa) {
        int i;

        data = params->data;
        for (i = 0; i < params->n_csa_offsets ; i++) {
            data[params->csa_offsets[i]] = vif->ap.csa->count;
        }
    }
    */
    asr_rtos_lock_mutex(&asr_hw->tx_agg_env_lock);
    hostdesc = (struct hostdesc*)asr_tx_aggr_get_copy_addr(asr_hw, temp_len);
    if(hostdesc)
    {
        hostdesc->queue_idx = txq->hwq->id;
        hostdesc->staid = (sta) ? sta->sta_idx : 0xFF;
        hostdesc->vif_idx = vif->vif_index;
        hostdesc->tid = 0xFF;
        hostdesc->flags = TXU_CNTRL_MGMT;
        hostdesc->txq = (uint32_t)txq;
        if (robust)
            hostdesc->flags |= TXU_CNTRL_MGMT_ROBUST;

        hostdesc->packet_len = frame_len;

        if (params->no_cck)
        {
            hostdesc->flags |= TXU_CNTRL_MGMT_NO_CCK;
        }
        hostdesc->agg_num = 0;
        hostdesc->packet_addr = 0;
        hostdesc->packet_offset = frame_offset;
        hostdesc->sdio_tx_len = temp_len - 2;
        hostdesc->sdio_tx_total_len = frame_offset + frame_len;
        if (hostdesc->sdio_tx_total_len % 4)
            dbg(D_ERR, D_UWIFI_CTRL, "%s sdio_tx_total_len:%d", __func__, hostdesc->sdio_tx_total_len);
        /* Copy the provided data */
        memcpy((uint8_t*)hostdesc+frame_offset, params->buf, frame_len);
        memcpy((uint8_t*)hostdesc + temp_len -4, &tx_data_end_token, 4);

        vif->txring_bytes += temp_len;
    }
    else
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s frame drop\n",__func__);
    }
    asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);

    *cookie = 0xff;
    /* Confirm transmission to CFG80211 */
    //cfg80211_mgmt_tx_status(&vif->wdev, *cookie,
    //                        params->buf, frame_len, hostdesc?true:false, GFP_ATOMIC);
    cfg80211_mgmt_tx_comp_status(vif, params->buf, frame_len, (hostdesc?1:0));
    //tell to transfer
    uwifi_sdio_event_set(UWIFI_SDIO_EVENT_TX);

    return 0;
}


int asr_start_mgmt_xmit_opt(struct asr_vif *vif, struct asr_sta *sta,
                 struct cfg80211_mgmt_tx_params *params,
                 bool offchan, u64 * cookie)
{
    struct asr_hw *asr_hw = vif->asr_hw;
    u16 frame_len;
    u8 *data;
    struct asr_txq *txq;
    bool robust;
    u16 temp_len;
    u16 frame_offset;
    struct hostdesc *hostdesc = NULL;
    uint32_t tx_data_end_token = 0xAECDBFCA;
    struct ieee80211_mgmt *mgmt = NULL;
    struct sk_buff *skb_new = NULL;
    #ifdef CFG_ROAMING
    struct ieee80211_hdr *hdr = NULL;
    //uint32_t timeout_ms = 0;
    #endif

    //ASR_DBG(ASR_FN_ENTRY_STR);

    if (params == NULL) {
        return 0;
    }

    mgmt = (struct ieee80211_mgmt *)(params->buf);
    frame_len = params->len;

    frame_offset = sizeof(struct txdesc_api) + HOST_PAD_LEN;
    temp_len = ASR_ALIGN_BLKSZ_HI(frame_offset + frame_len + 4);

    /* Set TID and Queues indexes */
#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
    if (asr_hw->monitor_vif_idx != 0xff)
        txq = asr_txq_vif_get_idle_mode(asr_hw, IDLE_HWQ_IDX, NULL);
    else if (sta) {
        txq = asr_txq_sta_get(sta, 8, NULL, asr_hw);    //VO
    } else
#else
    if (sta) {
        txq = asr_txq_sta_get(sta, 8, NULL, asr_hw);
    } else
#endif
    {
        if (offchan)
            txq =  &asr_hw->txq[asr_hw->sta_max_num * NX_NB_TXQ_PER_STA +
                     asr_hw->vif_max_num * NX_NB_TXQ_PER_VIF];
        else
            txq = asr_txq_vif_get(vif, NX_UNK_TXQ_TYPE, NULL);
    }

    /* Ensure that TXQ is active */
    if (txq->idx == TXQ_INACTIVE) {
        dbg(D_ERR, D_UWIFI_CTRL, "TXQ inactive\n");
        return -EBUSY;
    }

    if (params->buf == NULL) {
        return 0;
    }

    robust = ieee80211_is_robust_mgmt_frame(frame_len, params->buf);

    /* Update CSA counter if present */
    /*
    if (unlikely(params->n_csa_offsets) && vif->wdev.iftype == NL80211_IFTYPE_AP && vif->ap.csa) {
        int i;
        data = (u8 *)(params->buf);
        for (i = 0; i < params->n_csa_offsets; i++) {
            data[params->csa_offsets[i]] = vif->ap.csa->count;
        }
    }
    */

    if (temp_len > ASR_SDIO_DATA_MAX_LEN) {
        dbg(D_ERR, D_UWIFI_CTRL,  "%s frame drop, temp_len=%d \n", __func__, temp_len);
    } else if (txq->hwq) {

        // malloc skb
        //skb_new = dev_alloc_skb(temp_len);

        if (!skb_queue_empty(&asr_hw->tx_sk_free_list)) {

            skb_new = skb_dequeue(&asr_hw->tx_sk_free_list);
        }

        if (skb_new) {
            memset(skb_new->data, 0, temp_len);
            hostdesc = (struct hostdesc *)skb_new->data;
            hostdesc->queue_idx = txq->hwq->id;
            hostdesc->staid = (sta) ? sta->sta_idx : 0xFF;
            hostdesc->vif_idx = vif->vif_index;
            hostdesc->tid = 0xFF;
            hostdesc->flags = TXU_CNTRL_MGMT;
            hostdesc->txq = (uint32_t)txq;

            if (robust)
                hostdesc->flags |= TXU_CNTRL_MGMT_ROBUST;

            hostdesc->packet_len = frame_len;

            if (params->no_cck)

            {
                hostdesc->flags |= TXU_CNTRL_MGMT_NO_CCK;
            }

            hostdesc->agg_num = 0;
            hostdesc->packet_addr = 0;
            hostdesc->packet_offset = frame_offset;
            hostdesc->sdio_tx_len = temp_len - 2;
            hostdesc->sdio_tx_total_len = frame_offset + frame_len;
            hostdesc->timestamp = 0;
            hostdesc->sn = 0xFFFF;
            //dev_err(asr_hw->dev, "asr_start_mgmt_xmit %d %d %d %d \n",frame_offset,frame_len,temp_len,hostdesc->sdio_tx_len);

            /* Copy the provided data */
            memcpy((u8 *) hostdesc + frame_offset, params->buf, frame_len);
            memcpy((uint8_t*)hostdesc + temp_len -4, &tx_data_end_token, 4);

            // chain skb to tk_list.
            asr_rtos_lock_mutex(&asr_hw->tx_lock);
            skb_queue_tail(&asr_hw->tx_sk_list, skb_new);
            // mrole add for vif fc.
            vif->tx_skb_cnt ++;
            asr_rtos_unlock_mutex(&asr_hw->tx_lock);

        } else {
            dbg(D_ERR, D_UWIFI_CTRL,  "%s:%d: skb alloc of size %u failed\n\n", __func__, __LINE__, temp_len);
        }

    } else {
        hostdesc = NULL;
    }

    *cookie = 0xff;

    /* Confirm transmission to CFG80211 */
    //cfg80211_mgmt_tx_status(&vif->wdev, *cookie,
    //                        params->buf, frame_len, hostdesc?true:false, GFP_ATOMIC);
    cfg80211_mgmt_tx_comp_status(vif, params->buf, frame_len, (hostdesc?1:0));

    dbg(D_ERR, D_UWIFI_CTRL,
             "[asr_tx_mgmt]iftype=%d,frame=0x%X,sta_idx=%d,vif_idx=%d ,da=%02X:%02X:%02X:%02X:%02X:%02X,len=%d,ret=%d\n",
             ASR_VIF_TYPE(vif), mgmt->frame_control,
                         (sta) ? sta->sta_idx : 0xFF,
                         (vif) ? vif->vif_index : 0xFF,
                         mgmt->da[0], mgmt->da[1],mgmt->da[2],
                         mgmt->da[3], mgmt->da[4], mgmt->da[5],
                         frame_len, hostdesc ? true : false);

    //tell to transfer
    uwifi_sdio_event_set(UWIFI_SDIO_EVENT_TX);

    return 0;
}


int asr_start_mgmt_xmit(struct asr_vif *vif, struct asr_sta *sta,
                         struct cfg80211_mgmt_tx_params *params, bool offchan,
                         uint64_t *cookie)
{
     if (asr_xmit_opt) {

         return asr_start_mgmt_xmit_opt(vif, sta, params, offchan, cookie);

     } else {
         return asr_start_mgmt_xmit_aggbuf(vif, sta, params, offchan, cookie);
     }
}



/**
 * asr_txq_credit_update - Update credit for one txq
 *
 * @asr_hw: Driver main data
 * @sta_idx: STA idx
 * @tid: TID
 * @update: offset to apply in txq credits
 *
 * Called when fw send ME_TX_CREDITS_UPDATE_IND message.
 * Apply @update to txq credits, and stop/start the txq if needed
 */
void asr_txq_credit_update(struct asr_hw *asr_hw, int sta_idx, uint8_t tid,
                            int8_t update)
{
    struct asr_sta *sta = &asr_hw->sta_table[sta_idx];
    struct asr_txq *txq;

    txq = asr_txq_sta_get(sta, tid, NULL, asr_hw);

    asr_rtos_lock_mutex(&asr_hw->tx_lock);

    if (txq->idx != TXQ_INACTIVE) {
        txq->credits += update;

        if (txq->credits <= 0)
            asr_txq_stop(txq, ASR_TXQ_STOP_FULL);
        else
            asr_txq_start(txq, ASR_TXQ_STOP_FULL);
    }

    asr_rtos_unlock_mutex(&asr_hw->tx_lock);
}
