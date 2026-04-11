/**
 ******************************************************************************
 *
 * @file uwifi_txq.c
 *
 * @brief txq related operation
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */
#include "uwifi_txq.h"
#include "uwifi_include.h"
#include "uwifi_rx.h"
#include "uwifi_tx.h"
#include "uwifi_kernel.h"

#define ALL_HWQ_MASK  ((1 << CONFIG_USER_MAX) - 1)

/******************************************************************************
 * Utils inline functions
 *****************************************************************************/

struct asr_txq *asr_txq_sta_get(struct asr_sta *sta, uint8_t tid, int *idx,
                                  struct asr_hw * asr_hw)
{
    int id;

    if (tid >= NX_NB_TXQ_PER_STA)
        tid = 0;

    if (is_multicast_sta(asr_hw, sta->sta_idx))
        id = (asr_hw->sta_max_num * NX_NB_TXQ_PER_STA) + sta->vif_idx;
    else
        id = (sta->sta_idx * NX_NB_TXQ_PER_STA) + tid;

    if (idx)
        *idx = id;

    return &asr_hw->txq[id];
}

struct asr_txq *asr_txq_vif_get(struct asr_vif *vif, uint8_t type, int *idx)
{
    int id;

    id = vif->asr_hw->sta_max_num * NX_NB_TXQ_PER_STA + master_vif_idx(vif) +
        (type * vif->asr_hw->vif_max_num);

    if (idx)
        *idx = id;

    return &vif->asr_hw->txq[id];
}

#if 0
static struct asr_sta *asr_txq_2_sta(struct asr_txq *txq)
{
    return txq->sta;
}
#endif


/******************************************************************************
 * Init/Deinit functions
 *****************************************************************************/
/**
 * asr_txq_init - Initialize a TX queue
 *
 * @txq: TX queue to be initialized
 * @idx: TX queue index
 * @status: TX queue initial status
 * @hwq: Associated HW queue
 * @ndev: Net device this queue belongs to
 *        (may be null for non netdev txq)
 *
 * Each queue is initialized with the credit of @NX_TXQ_INITIAL_CREDITS.
 */
static void asr_txq_init(struct asr_hw *asr_hw, struct asr_txq *txq, int idx, uint8_t status,
                          struct asr_hwq *hwq,
                          struct asr_sta *sta)
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
    txq->sta = sta;
    for (i = 0; i < CONFIG_USER_MAX ; i++)
        txq->pkt_pushed[i] = 0;


    txq->ps_id = LEGACY_PS_ID;
    txq->push_limit = 0;
    if (idx < asr_hw->sta_max_num * NX_NB_TXQ_PER_STA) {
        int sta_idx = sta->sta_idx;
        int tid = idx - (sta_idx * NX_NB_TXQ_PER_STA);
        if (tid < NX_NB_TID_PER_STA)
            txq->ndev_idx = NX_STA_NDEV_IDX(tid, sta_idx);
        else
            txq->ndev_idx = NDEV_NO_TXQ;
    } else if (idx < asr_hw->sta_max_num * NX_NB_TXQ_PER_STA + asr_hw->vif_max_num) {
        txq->ndev_idx = NX_NB_TID_PER_STA * asr_hw->sta_max_num;
    } else {
        txq->ndev_idx = NDEV_NO_TXQ;
    }
}

/**
 *    skb_dequeue - remove from the head of the queue
 *    @list: list to dequeue from
 *
 *    Remove the head of the list. The list lock is taken so the function
 *    may be used safely with other locking list functions. The head item is
 *    returned or %NULL if the list is empty.
 */

struct sk_buff *skb_dequeue(struct sk_buff_head *list)
{
    struct sk_buff *result;

    asr_rtos_lock_mutex(&list->lock);
    result = __skb_dequeue(list);
    asr_rtos_unlock_mutex(&list->lock);
    return result;
}


/**
 * asr_txq_flush - Flush all buffers queued for a TXQ
 *
 * @asr_hw: main driver data
 * @txq: txq to flush
 */
void asr_txq_flush(struct asr_hw *asr_hw, struct asr_txq *txq)
{
    struct sk_buff *skb;
    //struct asr_txhdr *txhdr;
    //struct asr_sw_txhdr *sw_hdr;
    while(!skb_queue_empty(&txq->sk_list) && (skb = skb_dequeue(&txq->sk_list)) != NULL) {
        //struct asr_sw_txhdr *hdr = ((struct asr_txhdr *)skb->data)->sw_hdr;

        //dma_unmap_single(asr_hw->dev, hdr->dma_addr, hdr->map_len,
        //                 DMA_TO_DEVICE);

        dbg(D_ERR, D_UWIFI_CTRL, "txq flush\r\n");
        //txhdr = (struct asr_txhdr *)skb->data;
        //sw_hdr = txhdr->sw_hdr;
        //skb_pull(skb, sw_hdr->headroom);
        //asr_rtos_free(sw_hdr);
        //txhdr->sw_hdr = NULL;
        dev_kfree_skb_tx(skb);
    }

    //try to deinit mutex
    asr_rtos_deinit_mutex(&txq->sk_list.lock);
}

/**
 * asr_txq_deinit - De-initialize a TX queue
 *
 * @asr_hw: Driver main data
 * @txq: TX queue to be de-initialized
 * Any buffer stuck in a queue will be freed.
 */
void asr_txq_deinit(struct asr_hw *asr_hw, struct asr_txq *txq)
{
    asr_rtos_lock_mutex(&asr_hw->tx_lock);
    asr_txq_del_from_hw_list(txq);
    txq->idx = TXQ_INACTIVE;
    asr_rtos_unlock_mutex(&asr_hw->tx_lock);

    asr_txq_flush(asr_hw, txq);
}

/**
 * asr_txq_vif_init - Initialize all TXQ linked to a vif
 *
 * @asr_hw: main driver data
 * @asr_vif: Pointer on VIF
 * @status: Intial txq status
 *
 * Softmac : 1 VIF TXQ per HWQ
 *
 * Fullmac : 1 VIF TXQ for BC/MC
 *           1 VIF TXQ for MGMT to unknown STA
 */
void asr_txq_vif_init(struct asr_hw *asr_hw, struct asr_vif *asr_vif,
                       uint8_t status)
{
    struct asr_txq *txq;
    int idx;



    txq = asr_txq_vif_get(asr_vif, NX_BCMC_TXQ_TYPE, &idx);
    asr_txq_init(asr_hw, txq, idx, status, &asr_hw->hwq[ASR_HWQ_BE],
                  &asr_hw->sta_table[asr_vif->ap.bcmc_index]);

    txq = asr_txq_vif_get(asr_vif, NX_UNK_TXQ_TYPE, &idx);
    asr_txq_init(asr_hw, txq, idx, status, &asr_hw->hwq[ASR_HWQ_VO],
                  NULL);

}

/**
 * asr_txq_vif_deinit - Deinitialize all TXQ linked to a vif
 *
 * @asr_hw: main driver data
 * @asr_vif: Pointer on VIF
 */
void asr_txq_vif_deinit(struct asr_hw * asr_hw, struct asr_vif *asr_vif)
{
    struct asr_txq *txq;

    txq = asr_txq_vif_get(asr_vif, NX_BCMC_TXQ_TYPE, NULL);
    asr_txq_deinit(asr_hw, txq);

    txq = asr_txq_vif_get(asr_vif, NX_UNK_TXQ_TYPE, NULL);
    asr_txq_deinit(asr_hw, txq);
}


/**
 * asr_txq_sta_init - Initialize TX queues for a STA
 *
 * @asr_hw: Main driver data
 * @asr_sta: STA for which tx queues need to be initialized
 * @status: Intial txq status
 *
 * This function initialize all the TXQ associated to a STA.
 * Softmac : 1 TXQ per TID
 *
 * Fullmac : 1 TXQ per TID (limited to 8)
 *           1 TXQ for MGMT
 */
void asr_txq_sta_init(struct asr_hw * asr_hw, struct asr_sta *asr_sta,
                       uint8_t status)
{
    struct asr_txq *txq;
    int tid, idx;


    //struct asr_vif *asr_vif = asr_hw->vif_table[asr_sta->vif_idx];

    txq = asr_txq_sta_get(asr_sta, 0, &idx, asr_hw);
    for (tid = 0; tid < NX_NB_TXQ_PER_STA; tid++, txq++, idx++) {
        asr_txq_init(asr_hw, txq, idx, status, &asr_hw->hwq[asr_tid2hwq[tid]],
                      asr_sta);
        txq->ps_id = asr_sta->uapsd_tids & (1 << tid) ? UAPSD_ID : LEGACY_PS_ID;
    }

}

/**
 * asr_txq_sta_deinit - Deinitialize TX queues for a STA
 *
 * @asr_hw: Main driver data
 * @asr_sta: STA for which tx queues need to be deinitialized
 */
void asr_txq_sta_deinit(struct asr_hw *asr_hw, struct asr_sta *asr_sta)
{
    struct asr_txq *txq;
    int i;


    txq = asr_txq_sta_get(asr_sta, 0, NULL, asr_hw);

    for (i = 0; i < NX_NB_TXQ_PER_STA; i++, txq++) {
        asr_txq_deinit(asr_hw, txq);
    }
}


/**
 * asr_init_unk_txq - Initialize TX queue for the transmission on a offchannel
 *
 * @vif: Interface for which the queue has to be initialized
 *
 * NOTE: Offchannel txq is only active for the duration of the ROC
 */
void asr_txq_offchan_init(struct asr_vif *asr_vif)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;
    struct asr_txq *txq;

    txq = &asr_hw->txq[asr_hw->sta_max_num * NX_NB_TXQ_PER_STA + asr_hw->vif_max_num * NX_NB_TXQ_PER_VIF];
    asr_txq_init(asr_hw, txq, asr_hw->sta_max_num * NX_NB_TXQ_PER_STA + asr_hw->vif_max_num * NX_NB_TXQ_PER_VIF, ASR_TXQ_STOP_CHAN,
                  &asr_hw->hwq[ASR_HWQ_VO], NULL);
}

/**
 * asr_deinit_offchan_txq - Deinitialize TX queue for offchannel
 *
 * @vif: Interface that manages the STA
 *
 * This function deintialize txq for one STA.
 * Any buffer stuck in a queue will be freed.
 */
void asr_txq_offchan_deinit(struct asr_vif *asr_vif)
{
    struct asr_txq *txq;
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    txq = &asr_vif->asr_hw->txq[asr_hw->sta_max_num * NX_NB_TXQ_PER_STA + asr_hw->vif_max_num * NX_NB_TXQ_PER_VIF];
    asr_txq_deinit(asr_vif->asr_hw, txq);
}


/******************************************************************************
 * Start/Stop functions
 *****************************************************************************/
/**
 * asr_txq_add_to_hw_list - Add TX queue to a HW queue schedule list.
 *
 * @txq: TX queue to add
 *
 * Add the TX queue if not already present in the HW queue list.
 * To be called with tx_lock hold
 */
void asr_txq_add_to_hw_list(struct asr_txq *txq)
{
    if (!(txq->status & ASR_TXQ_IN_HWQ_LIST)) {
        txq->status |= ASR_TXQ_IN_HWQ_LIST;
        list_add_tail(&txq->sched_list, &txq->hwq->list);
        txq->hwq->need_processing = true;
    }
}

/**
 * asr_txq_del_from_hw_list - Delete TX queue from a HW queue schedule list.
 *
 * @txq: TX queue to delete
 *
 * Remove the TX queue from the HW queue list if present.
 * To be called with tx_lock hold
 */
void asr_txq_del_from_hw_list(struct asr_txq *txq)
{

    if (txq->status & ASR_TXQ_IN_HWQ_LIST) {

        txq->status &= ~ASR_TXQ_IN_HWQ_LIST;
        list_del(&txq->sched_list);
    }
}

/**
 * asr_txq_start - Try to Start one TX queue
 *
 * @txq: TX queue to start
 * @reason: reason why the TX queue is started (among ASR_TXQ_STOP_xxx)
 *
 * Re-start the TX queue for one reason.
 * If after this the txq is no longer stopped and some buffers are ready,
 * the TX queue is also added to HW queue list.
 * To be called with tx_lock hold
 */
void asr_txq_start(struct asr_txq *txq, uint16_t reason)
{
    if (txq->idx != TXQ_INACTIVE && (txq->status & reason))
    {
        txq->status &= ~reason;
        if (!asr_txq_is_stopped(txq) &&
            !skb_queue_empty(&txq->sk_list)) {
            asr_txq_add_to_hw_list(txq);
        }
    }
}

/**
 * asr_txq_stop - Stop one TX queue
 *
 * @txq: TX queue to stop
 * @reason: reason why the TX queue is stopped (among ASR_TXQ_STOP_xxx)
 *
 * Stop the TX queue. It will remove the TX queue from HW queue list
 * To be called with tx_lock hold
 */
void asr_txq_stop(struct asr_txq *txq, uint16_t reason)
{


    if(txq==NULL)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s-txq is NULL\n", __func__);
    }

    if (txq->idx != TXQ_INACTIVE)
    {
        txq->status |= reason;
        asr_txq_del_from_hw_list(txq);
    }
}


/**
 * asr_txq_sta_start - Start all the TX queue linked to a STA
 *
 * @sta: STA whose TX queues must be re-started
 * @reason: Reason why the TX queue are restarted (among ASR_TXQ_STOP_xxx)
 * @asr_hw: Driver main data
 *
 * This function will re-start all the TX queues of the STA for the reason
 * specified. It can be :
 * - ASR_TXQ_STOP_STA_PS: the STA is no longer in power save mode
 * - ASR_TXQ_STOP_VIF_PS: the VIF is in power save mode (p2p absence)
 * - ASR_TXQ_STOP_CHAN: the STA's VIF is now on the current active channel
 *
 * Any TX queue with buffer ready and not Stopped for other reasons, will be
 * added to the HW queue list
 * To be called with tx_lock hold
 */
void asr_txq_sta_start(struct asr_sta *asr_sta, uint16_t reason,
                                    struct asr_hw *asr_hw)
{
    struct asr_txq *txq;
    int i;
    int nb_txq;

    txq = asr_txq_sta_get(asr_sta, 0, NULL, asr_hw);

    if (is_multicast_sta(asr_hw, asr_sta->sta_idx))
        nb_txq = 1;
    else
        nb_txq = NX_NB_TXQ_PER_STA;

    for (i = 0; i < nb_txq; i++, txq++)
        asr_txq_start(txq, reason);
}


/**
 * asr_stop_sta_txq - Stop all the TX queue linked to a STA
 *
 * @sta: STA whose TX queues must be stopped
 * @reason: Reason why the TX queue are stopped (among ASR_TX_STOP_xxx)
 * @asr_hw: Driver main data
 *
 * This function will stop all the TX queues of the STA for the reason
 * specified. It can be :
 * - ASR_TXQ_STOP_STA_PS: the STA is in power save mode
 * - ASR_TXQ_STOP_VIF_PS: the VIF is in power save mode (p2p absence)
 * - ASR_TXQ_STOP_CHAN: the STA's VIF is not on the current active channel
 *
 * Any TX queue present in a HW queue list will be removed from this list.
 * To be called with tx_lock hold
 */
void asr_txq_sta_stop(struct asr_sta *asr_sta, uint16_t reason
                       , struct asr_hw *asr_hw)
{
    struct asr_txq *txq;
    int i;
    int nb_txq;

    if (!asr_sta)
        return;


    txq = asr_txq_sta_get(asr_sta, 0, NULL, asr_hw);

    if (is_multicast_sta(asr_hw, asr_sta->sta_idx))
        nb_txq = 1;
    else
        nb_txq = NX_NB_TXQ_PER_STA;

    for (i = 0; i < nb_txq; i++, txq++)
        asr_txq_stop(txq, reason);

}

static void asr_txq_vif_for_each_sta(struct asr_hw *asr_hw, struct asr_vif *asr_vif,
                               void (*f)(struct asr_sta *, uint16_t, struct asr_hw *),
                               uint16_t reason)
{
    switch (ASR_VIF_TYPE(asr_vif)) {
    case NL80211_IFTYPE_STATION:
    {
        f(asr_vif->sta.ap, reason, asr_hw);
        break;
    }
    case NL80211_IFTYPE_AP:
    {
        struct asr_sta *sta;
        list_for_each_entry(sta, &asr_vif->ap.sta_list, list) {
            f(sta, reason, asr_hw);
        }
        break;
    }
    default:
        break;
    }
}

bool check_vif_block_flags(struct asr_vif *asr_vif)
{
    if (NULL == asr_vif)
        return true;
    else
    return (asr_test_bit(ASR_DEV_STA_OUT_TWTSP,   &asr_vif->dev_flags) ||
           asr_test_bit(ASR_DEV_TXQ_STOP_CSA,    &asr_vif->dev_flags) ||
           asr_test_bit(ASR_DEV_TXQ_STOP_VIF_PS, &asr_vif->dev_flags) ||
           asr_test_bit(ASR_DEV_TXQ_STOP_CHAN,   &asr_vif->dev_flags));
}


/**
 * asr_txq_vif_start - START TX queues of all STA associated to the vif
 *                      and vif's TXQ
 *
 * @vif: Interface to start
 * @reason: Start reason (ASR_TXQ_STOP_CHAN or ASR_TXQ_STOP_VIF_PS)
 * @asr_hw: Driver main data
 *
 * Iterate over all the STA associated to the vif and re-start them for the
 * reason @reason
 * Take tx_lock
 */
void asr_txq_vif_start(struct asr_vif *asr_vif, uint16_t reason,
                        struct asr_hw *asr_hw)
{
    struct asr_txq *txq;

    asr_rtos_lock_mutex(&asr_hw->tx_lock);

    asr_txq_vif_for_each_sta(asr_hw, asr_vif, asr_txq_sta_start, reason);

    txq = asr_txq_vif_get(asr_vif, NX_BCMC_TXQ_TYPE, NULL);
    asr_txq_start(txq, reason);
    txq = asr_txq_vif_get(asr_vif, NX_UNK_TXQ_TYPE, NULL);
    asr_txq_start(txq, reason);

    // sdio mode used: clr drv flag for tx task.
#ifdef CONFIG_TWT
    if (ASR_TXQ_STOP_TWT == reason) {
        asr_clear_bit(ASR_DEV_STA_OUT_TWTSP,&asr_vif->dev_flags);
    }
#endif
    if (ASR_TXQ_STOP_CSA == reason) {
        asr_clear_bit(ASR_DEV_TXQ_STOP_CSA,&asr_vif->dev_flags);
    }

    if (ASR_TXQ_STOP_CHAN == reason) {
        asr_clear_bit(ASR_DEV_TXQ_STOP_CHAN,&asr_vif->dev_flags);
    }

    if (ASR_TXQ_STOP_VIF_PS == reason) {
        asr_clear_bit(ASR_DEV_TXQ_STOP_VIF_PS,&asr_vif->dev_flags);
    }

    asr_rtos_unlock_mutex(&asr_hw->tx_lock);

    // add tx task trigger.
    if ( (check_vif_block_flags(asr_vif) == false) ) {
        uwifi_sdio_event_set(UWIFI_SDIO_EVENT_TX);
    }

}



/**
 * asr_txq_vif_stop - STOP TX queues of all STA associated to the vif
 *
 * @vif: Interface to stop
 * @arg: Stop reason (ASR_TXQ_STOP_CHAN or ASR_TXQ_STOP_VIF_PS)
 * @asr_hw: Driver main data
 *
 * Iterate over all the STA associated to the vif and stop them for the
 * reason ASR_TXQ_STOP_CHAN or ASR_TXQ_STOP_VIF_PS
 * Take tx_lock
 */
void asr_txq_vif_stop(struct asr_vif *asr_vif, uint16_t reason,
                       struct asr_hw *asr_hw)
{
    struct asr_txq *txq;

    asr_rtos_lock_mutex(&asr_hw->tx_lock);

    asr_txq_vif_for_each_sta(asr_hw, asr_vif, asr_txq_sta_stop, reason);

    txq = asr_txq_vif_get(asr_vif, NX_BCMC_TXQ_TYPE, NULL);
    asr_txq_stop(txq, reason);
    txq = asr_txq_vif_get(asr_vif, NX_UNK_TXQ_TYPE, NULL);
    asr_txq_stop(txq, reason);

    // sdio mode used: set drv flag for tx task stop.
#ifdef CONFIG_TWT
    if (ASR_TXQ_STOP_TWT == reason) {
        asr_set_bit(ASR_DEV_STA_OUT_TWTSP,&asr_vif->dev_flags);
    }
#endif
    if (ASR_TXQ_STOP_CSA == reason) {
        asr_set_bit(ASR_DEV_TXQ_STOP_CSA,&asr_vif->dev_flags);
    }

    if (ASR_TXQ_STOP_CHAN == reason) {
        asr_set_bit(ASR_DEV_TXQ_STOP_CHAN,&asr_vif->dev_flags);
    }

    if (ASR_TXQ_STOP_VIF_PS == reason) {
        asr_set_bit(ASR_DEV_TXQ_STOP_VIF_PS,&asr_vif->dev_flags);
    }

    asr_rtos_unlock_mutex(&asr_hw->tx_lock);
}

/**
 * asr_start_offchan_txq - START TX queue for offchannel frame
 *
 * @asr_hw: Driver main data
 */
void asr_txq_offchan_start(struct asr_hw *asr_hw)
{
    struct asr_txq *txq;

    txq = &asr_hw->txq[asr_hw->sta_max_num * NX_NB_TXQ_PER_STA + asr_hw->vif_max_num * NX_NB_TXQ_PER_VIF];
    asr_txq_start(txq, ASR_TXQ_STOP_CHAN);
}

/**
 * asr_switch_vif_sta_txq - Associate TXQ linked to a STA to a new vif
 *
 * @sta: STA whose txq must be switched
 * @old_vif: Vif currently associated to the STA (may no longer be active)
 * @new_vif: vif which should be associated to the STA for now on
 *
 * This function will switch the vif (i.e. the netdev) associated to all STA's
 * TXQ. This is used when AP_VLAN interface are created.
 * If one STA is associated to an AP_vlan vif, it will be moved from the master
 * AP vif to the AP_vlan vif.
 * If an AP_vlan vif is removed, then STA will be moved back to mastert AP vif.
 *
 */
void asr_txq_sta_switch_vif(struct asr_sta *sta, struct asr_vif *old_vif,
                             struct asr_vif *new_vif)
{
    struct asr_hw *asr_hw = new_vif->asr_hw;
    struct asr_txq *txq;
    int i;

    txq = asr_txq_sta_get(sta, 0, NULL, asr_hw);
    for (i = 0; i < NX_NB_TID_PER_STA; i++, txq++) {

    }
}

/******************************************************************************
 * TXQ queue/schedule functions
 *****************************************************************************/
/**
 * asr_txq_queue_skb - Queue a buffer in a TX queue
 *
 * @skb: Buffer to queue
 * @txq: TX Queue in which the buffer must be added
 * @asr_hw: Driver main data
 * @retry: Should it be queued in the retry list
 *
 * @return: Retrun 1 if txq has been added to hwq list, 0 otherwise
 *
 * Add a buffer in the buffer list of the TX queue
 * and add this TX queue in the HW queue list if the txq is not stopped.
 * If this is a retry packet it is added after the last retry packet or at the
 * beginning if there is no retry packet queued.
 *
 * If the STA is in PS mode and this is the first packet queued for this txq
 * update TIM.
 *
 * To be called with tx_lock hold
 */
int asr_txq_queue_skb(struct sk_buff *skb, struct asr_txq *txq,
                       struct asr_hw *asr_hw,  bool retry)
{
    if (txq->sta && txq->sta->ps.active)
    {
        txq->sta->ps.pkt_ready[txq->ps_id]++;

        if (txq->sta->ps.pkt_ready[txq->ps_id] == 1)
        {
            asr_set_traffic_status(asr_hw, txq->sta, true, txq->ps_id);
        }
    }

    if (!retry) {
        /* add buffer in the sk_list */
        skb_queue_tail(&txq->sk_list, skb);
    } else {
        if (txq->last_retry_skb)
            skb_append(txq->last_retry_skb, skb, &txq->sk_list);
        else
            skb_queue_head(&txq->sk_list, skb);

        txq->last_retry_skb = skb;
        txq->nb_retry++;
    }


    /* Flowctrl corresponding netdev queue if needed */
    /* If too many buffer are queued for this TXQ stop netdev queue */
    if ((txq->ndev_idx != NDEV_NO_TXQ) &&
        (skb_queue_len(&txq->sk_list) > ASR_NDEV_FLOW_CTRL_STOP)) {
        txq->status |= ASR_TXQ_NDEV_FLOW_CTRL;
        //netif_stop_subqueue(txq->ndev, txq->ndev_idx);
    }

    /* add it in the hwq list if not stopped and not yet present */
    if (!(txq->status & ASR_TXQ_STOP)) {
        asr_txq_add_to_hw_list(txq);
        return 1;
    }

    return 0;
}

/**
 * asr_txq_confirm_any - Process buffer confirmed by fw
 *
 * @asr_hw: Driver main data
 * @txq: TX Queue
 * @hwq: HW Queue
 * @sw_txhdr: software descriptor of the confirmed packet
 *
 * Process a buffer returned by the fw. It doesn't check buffer status
 * and only does systematic counter update:
 * - hw credit
 * - buffer pushed to fw
 *
 * To be called with tx_lock hold
 */
void asr_txq_confirm_any(struct asr_hw *asr_hw, struct asr_txq *txq,
                          struct asr_hwq *hwq, struct asr_sw_txhdr *sw_txhdr)
{
    int user = 0;

    if (txq->pkt_pushed[user])
        txq->pkt_pushed[user]--;

    hwq->credits[user]++;
    hwq->need_processing = true;
    //asr_hw->stats.cfm_balance[hwq->id]--;
}

/******************************************************************************
 * HWQ processing
 *****************************************************************************/
#if 0
static int8_t asr_txq_get_credits(struct asr_txq *txq)
{
    int8_t cred = txq->credits;

    /* if destination is in PS mode, push_limit indicates the maximum
       number of packet that can be pushed on this txq. */
    if (txq->push_limit && (cred > txq->push_limit)) {
        cred = txq->push_limit;
    }
    return cred;
}
#endif

/**
 * extract the first @nb_elt of @list and append them to @head
 * It is assume that:
 * - @list contains more that @nb_elt
 * - There is no need to take @list nor @head lock to modify them
 */
static inline void skb_queue_extract(struct sk_buff_head *list,
                                     struct sk_buff_head *head, int nb_elt)
{
    int i;
    struct sk_buff *first, *last, *ptr;

    first = ptr = list->next;
    for (i = 0; i < nb_elt; i++) {
        ptr = ptr->next;
    }
    last = ptr->prev;

    /* unlink nb_elt in list */
    list->qlen -= nb_elt;
    list->next = ptr;
    ptr->prev = (struct sk_buff *)list;

    /* append nb_elt at end of head */
    head->qlen += nb_elt;
    last->next = (struct sk_buff *)head;
    head->prev->next = first;
    first->prev = head->prev;
    head->prev = last;
}

/**
 * asr_txq_get_skb_to_push - Get list of buffer to push for one txq
 *
 * @asr_hw: main driver data
 * @hwq: HWQ on wich buffers will be pushed
 * @txq: TXQ to get buffers from
 * @user: user postion to use
 * @sk_list_push: list to update
 *
 *
 * This function will returned a list of buffer to push for one txq.
 * It will take into account the number of credit of the HWQ for this user
 * position and TXQ (and push_limit for fullmac).
 * This allow to get a list that can be pushed without having to test for
 * hwq/txq status after each push
 *
 * If a MU group has been selected for this txq, it will also update the
 * counter for the group
 *
 * @return true if txq no longer have buffer ready after the ones returned.
 *         false otherwise
 */
#if 0
static
bool asr_txq_get_skb_to_push(struct asr_hw *asr_hw, struct asr_hwq *hwq,
                              struct asr_txq *txq, int user,
                              struct sk_buff_head *sk_list_push)
{
    int nb_ready = skb_queue_len(&txq->sk_list);
    int credits = min_t(int, asr_txq_get_credits(txq), hwq->credits[user]);
    bool res = false;

    __skb_queue_head_init(sk_list_push);

    if (credits >= nb_ready) {
        skb_queue_splice_init(&txq->sk_list, sk_list_push);
        credits = nb_ready;

        res = true;
    } else {
        skb_queue_extract(&txq->sk_list, sk_list_push, credits);

        /* When processing PS service period (i.e. push_limit != 0), no longer
           process this txq if this is a asrcy PS service period (even if no
           packet is pushed) or the SP is complete for this txq */
        if (txq->push_limit &&
            ((txq->ps_id == LEGACY_PS_ID) ||
             (credits >= txq->push_limit)))
            res = true;
    }

    //asr_mu_set_active_sta(asr_hw, asr_txq_2_sta(txq), credits);

    return res;
}
#endif

/**
 * asr_hwq_process - Process one HW queue list
 *
 * @asr_hw: Driver main data
 * @hw_queue: HW queue index to process
 *
 * The function will iterate over all the TX queues linked in this HW queue
 * list. For each TX queue, push as many buffers as possible in the HW queue.
 * (NB: TX queue have at least 1 buffer, otherwise it wouldn't be in the list)
 * - If TX queue no longer have buffer, remove it from the list and check next
 *   TX queue
 * - If TX queue no longer have credits or has a push_limit (PS mode) and it
 *   is reached , remove it from the list and check next TX queue
 * - If HW queue is full, update list head to start with the next TX queue on
 *   next call if current TX queue already pushed "too many" pkt in a row, and
 *   return
 *
 * To be called when HW queue list is modified:
 * - when a buffer is pushed on a TX queue
 * - when new credits are received
 * - when a STA returns from Power Save mode or receives traffic request.
 * - when Channel context change
 *
 * To be called with tx_lock hold
 */
#if 0
void asr_hwq_process(struct asr_hw *asr_hw, struct asr_hwq *hwq)
{
    struct asr_txq *txq, *next;
    int user = 0;
    int credit_map = 0;
    //bool mu_enable = false;

    hwq->need_processing = false;
    credit_map = ALL_HWQ_MASK - 1;

    list_for_each_entry_safe(txq, next, &hwq->list, sched_list) {
        struct asr_txhdr *txhdr = NULL;
        struct sk_buff_head sk_list_push;
        struct sk_buff *skb;
        bool txq_empty;

        if (!hwq->credits[user]) {
            credit_map |= BIT(user);
            if (credit_map == ALL_HWQ_MASK)
                break;
            continue;
        }

        txq_empty = asr_txq_get_skb_to_push(asr_hw, hwq, txq, user,
                                             &sk_list_push);

        while ((skb = __skb_dequeue(&sk_list_push)) != NULL) {
            txhdr = (struct asr_txhdr *)skb->data;
            asr_tx_push(asr_hw, txhdr, 0);
        }

        if (txq_empty) {
            asr_txq_del_from_hw_list(txq);
            txq->pkt_sent = 0;
        } else if ((hwq->credits[user] == 0) &&
                   asr_txq_is_scheduled(txq)) {
            /* txq not empty,
               - To avoid starving need to process other txq in the list
               - For better aggregation, need to send "as many consecutive
               pkt as possible" for he same txq
               ==> Add counter to trigger txq switch
            */
            if (txq->pkt_sent > hwq->size) {
                txq->pkt_sent = 0;
                list_rotate_left(&hwq->list);
            }
        }

        /* Unable to complete PS traffic request because of hwq credit */
        if (txq->push_limit && txq->sta) {
            if (txq->ps_id == LEGACY_PS_ID) {
                /* for asrcy PS abort SP and wait next ps-poll */
                txq->sta->ps.sp_cnt[txq->ps_id] -= txq->push_limit;
                txq->push_limit = 0;
            }
            /* for u-apsd need to complete the SP to send EOSP frame */
        }

        /* restart netdev queue if number of queued buffer is below threshold */
        if ((txq->status & ASR_TXQ_NDEV_FLOW_CTRL) &&
            skb_queue_len(&txq->sk_list) < ASR_NDEV_FLOW_CTRL_RESTART) {
            txq->status &= ~ASR_TXQ_NDEV_FLOW_CTRL;
            //netif_wake_subqueue(txq->ndev, txq->ndev_idx);
        }

    }

}
#endif
/**
 * asr_hwq_process_all - Process all HW queue list
 *
 * @asr_hw: Driver main data
 *
 * Loop over all HWQ, and process them if needed
 * To be called with tx_lock hold
 */
 #if 0
void asr_hwq_process_all(struct asr_hw *asr_hw)
{
    int id;

    for (id = ARRAY_SIZE(asr_hw->hwq) - 1; id >= 0 ; id--)
    {
        if (asr_hw->hwq[id].need_processing)
        {
            asr_hwq_process(asr_hw, &asr_hw->hwq[id]);
        }
    }
}
#endif

/**
 * asr_hwq_init - Initialize all hwq structures
 *
 * @asr_hw: Driver main data
 *
 */
void asr_hwq_init(struct asr_hw *asr_hw)
{
    int i, j;

    for (i = 0; i < ARRAY_SIZE(asr_hw->hwq); i++) {
        struct asr_hwq *hwq = &asr_hw->hwq[i];

        for (j = 0 ; j < CONFIG_USER_MAX; j++)
            hwq->credits[j] = nx_txdesc_cnt[i];
        hwq->id = i;
        hwq->size = nx_txdesc_cnt[i];
        INIT_LIST_HEAD(&hwq->list);
    }
}
