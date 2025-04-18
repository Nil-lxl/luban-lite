#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
#include "uwifi_idle_mode.h"
#include "uwifi_platform.h"

struct asr_txq *asr_txq_vif_get_idle_mode(struct asr_hw *asr_hw, uint8_t ac, int *idx)
{
    if (idx)
        *idx = ac;

    return &asr_hw->txq[ac];
}

void asr_txq_init_idle_mode(struct asr_txq *txq, int idx, uint8_t status,
                          struct asr_hwq *hwq, int tid)
{
    int i;

    txq->idx = idx;
    txq->status = status;
    txq->credits = NX_TXQ_INITIAL_CREDITS;
    txq->pkt_sent = 0;
    skb_queue_head_init(&txq->sk_list);
    txq->last_retry_skb = NULL;
    txq->nb_retry = 0;
    txq->hwq = hwq;
    for (i = 0; i < CONFIG_USER_MAX ; i++)
        txq->pkt_pushed[i] = 0;

    //txq->baw.agg_on = false;
}

/* called after idle mode add interface */
void asr_txq_vif_init_idle_mode(struct asr_hw *asr_hw, uint8_t status)
{
    int i;
    int idx;
    struct asr_txq *txq;
    txq = asr_txq_vif_get_idle_mode(asr_hw, 0, &idx);
    for (i = 0; i < NX_NB_TXQ_MAX_IDLE_MODE; i++, idx++, txq++)
    {
        asr_txq_init_idle_mode(txq, idx, status, &asr_hw->hwq[i], -1);
    }
}
/**
 * asr_txq_sta_deinit - Deinitialize TX queues for a STA
 *
 * @asr_hw: Main driver data
 * @asr_sta: STA for which tx queues need to be deinitialized
 */
void asr_txq_vif_deinit_idle_mode(struct asr_hw *asr_hw, struct asr_vif *asr_vif)
{
    int i;
    int idx;
    struct asr_txq *txq;
    txq = asr_txq_vif_get_idle_mode(asr_hw, 0, &idx);

    for (i = 0; i < NX_NB_TXQ_MAX_IDLE_MODE; i++, txq++) {
        asr_txq_deinit(asr_hw, txq);
    }
}

/*start after channel switch*/
void asr_txq_vif_start_idle_mode(struct asr_vif *asr_vif, u16 reason,
                        struct asr_hw *asr_hw)
{
    struct asr_txq *txq;
    int i;

    asr_rtos_lock_mutex(&asr_hw->tx_lock);

    txq = asr_txq_vif_get_idle_mode(asr_hw, 0, NULL);
    for (i = 0; i < NX_NB_TXQ_MAX_IDLE_MODE; i++, txq++)
    {
        asr_txq_start(txq, reason);
    }

    asr_rtos_unlock_mutex(&asr_hw->tx_lock);
}

/*stop before channel switch*/
void asr_txq_vif_stop_idle_mode(struct asr_vif *asr_vif, uint16_t reason,
                       struct asr_hw *asr_hw)
{
    struct asr_txq *txq;
    int i;

    asr_rtos_lock_mutex(&asr_hw->tx_lock);

    txq = asr_txq_vif_get_idle_mode(asr_hw, 0, NULL);
    for (i = 0; i < NX_NB_TXQ_MAX_IDLE_MODE; i++, txq++)
    {
        asr_txq_stop(txq, reason);
    }

    asr_rtos_unlock_mutex(&asr_hw->tx_lock);
}
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
void asr_start_xmit_idle(uint8_t *frame, uint32_t len)
{
    //uint16_t freq;
    uint64_t cookie;  //useless?
    struct asr_vif *asr_vif;
    struct cfg80211_mgmt_tx_params tx_params = {0};

    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL",__func__);
        return;
    }

    // 1. get asr_vif and freq
    if(asr_hw->monitor_vif_idx != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->monitor_vif_idx];
        //if(asr_vif->sniffer.chan_num > 0)
        //    freq = 2407 + 5*(asr_vif->sniffer.chan_num);
        //else
        //    freq = 2437;
        tx_params.chan = &asr_vif->sniffer.st_chan;
    }
    else if(asr_hw->sta_vif_idx != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];
        tx_params.chan = asr_vif->asr_hw->chanctx_table[asr_vif->ch_index].chan_def.chan;
    }
    else if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
        tx_params.chan = asr_vif->asr_hw->chanctx_table[asr_vif->ch_index].chan_def.chan;
    }
    else
    {
        dbg(D_CRT, D_UWIFI_CTRL, "vif not support, return");
        return;
    }

    // 2. get tx_params
    tx_params.buf = frame;
    tx_params.len = len;
    tx_params.cus_flags = 1;

    // 3. send mgmt frame
    if(uwifi_cfg80211_ops.mgmt_tx(asr_vif, &tx_params, &cookie))
    {
        dbg(D_CRT, D_UWIFI_CTRL, "custom mgmt frame tx fail");
        asr_rtos_free((uint8_t *)tx_params.buf);
        tx_params.buf = NULL;
        return;
    }

}
void uwifi_tx_dev_custom_mgmtframe(uint8_t *pframe, uint32_t len)
{
    asr_start_xmit_idle(pframe,len);
}
#endif
