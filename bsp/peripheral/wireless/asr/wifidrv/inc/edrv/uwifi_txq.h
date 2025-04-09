/**
 ****************************************************************************************
 *
 * @file uwifi_txq.h
 *
 * @brief api of txq operation
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */

#ifndef _UWIFI_TXQ_H_
#define _UWIFI_TXQ_H_

#include <stdint.h>
#include <stdbool.h>
#include "uwifi_types.h"
#include "uwifi_kernel.h"
#include "uwifi_ops_adapter.h"
#include "uwifi_tx.h"

//#define NX_FIRST_VIF_TXQ_IDX (NX_REMOTE_STA_MAX * NX_NB_TXQ_PER_STA)
//#define NX_FIRST_BCMC_TXQ_IDX  NX_FIRST_VIF_TXQ_IDX
//#define NX_FIRST_UNK_TXQ_IDX  (NX_FIRST_BCMC_TXQ_IDX + NX_VIRT_DEV_MAX)


#define NX_BCMC_TXQ_TYPE 0
#define NX_UNK_TXQ_TYPE  1

/**
 * Each data TXQ is a netdev queue. TXQ to send MGT are not data TXQ as
 * they did not recieved buffer from netdev interface.
 * Need to allocate the maximum case.
 * AP : all STAs + 1 BC/MC
 */
#define NX_NB_NDEV_TXQ ((NX_NB_TID_PER_STA * NX_REMOTE_STA_MAX) + 1 )
//#define NX_BCMC_TXQ_NDEV_IDX (NX_NB_TID_PER_STA * NX_REMOTE_STA_MAX)
#define NX_STA_NDEV_IDX(tid, sta_idx) ((tid) + (sta_idx) * NX_NB_TID_PER_STA)
#define NDEV_NO_TXQ 0xffff
#if (NX_NB_NDEV_TXQ >= NDEV_NO_TXQ)
#error("Need to increase struct asr_txq->ndev_idx size")
#endif

/* stop netdev queue when number of queued buffers if greater than this  */
#define ASR_NDEV_FLOW_CTRL_STOP    200
/* restart netdev queue when number of queued buffers is lower than this */
#define ASR_NDEV_FLOW_CTRL_RESTART 100

#define TXQ_INACTIVE 0xffff
#if (NX_NB_TXQ >= TXQ_INACTIVE)
#error("Need to increase struct asr_txq->idx size")
#endif

#define NX_TXQ_INITIAL_CREDITS 4

/**
 * enum asr_push_flags - Flags of pushed buffer
 *
 * @ASR_PUSH_RETRY Pushing a buffer for retry
 * @ASR_PUSH_IMMEDIATE Pushing a buffer without queuing it first
 */
enum asr_push_flags {
    ASR_PUSH_RETRY  = BIT(0),
    ASR_PUSH_IMMEDIATE = BIT(1),
};

/**
 * enum asr_txq_flags - TXQ status flag
 *
 * @ASR_TXQ_IN_HWQ_LIST The queue is scheduled for transmission
 * @ASR_TXQ_STOP_FULL No more credits for the queue
 * @ASR_TXQ_STOP_CSA CSA is in progress
 * @ASR_TXQ_STOP_STA_PS Destiniation sta is currently in power save mode
 * @ASR_TXQ_STOP_VIF_PS Vif owning this queue is currently in power save mode
 * @ASR_TXQ_STOP_CHAN Channel of this queue is not the current active channel
 * @ASR_TXQ_STOP_MU_POS TXQ is stopped waiting for all the buffers pushed to
 *                       fw to be confirmed
 * @ASR_TXQ_STOP All possible reason to have a txq stopped
 * @ASR_TXQ_NDEV_FLOW_CTRL associated netdev queue is currently stopped.
 *                          Note: when a TXQ is flowctrl it is NOT stopped
 */
enum asr_txq_flags {
    ASR_TXQ_IN_HWQ_LIST  = BIT(0),
    ASR_TXQ_STOP_FULL    = BIT(1),
    ASR_TXQ_STOP_CSA     = BIT(2),
    ASR_TXQ_STOP_STA_PS  = BIT(3),
    ASR_TXQ_STOP_VIF_PS  = BIT(4),
    ASR_TXQ_STOP_CHAN    = BIT(5),
#ifdef CONFIG_TWT
    ASR_TXQ_STOP_TWT = BIT(6),    // sta mode, txq is stopped when outside of TWT SP.
#else
    ASR_TXQ_STOP_MU_POS  = BIT(6),
#endif
    ASR_TXQ_STOP         = (ASR_TXQ_STOP_FULL | ASR_TXQ_STOP_CSA |
                             ASR_TXQ_STOP_STA_PS | ASR_TXQ_STOP_VIF_PS |
                             ASR_TXQ_STOP_CHAN
                            #ifdef CONFIG_TWT
                            | ASR_TXQ_STOP_TWT
                            #endif
                            ) ,
    ASR_TXQ_NDEV_FLOW_CTRL = BIT(7),
};

#define ASR_TXQ_GROUP_ID(txq) 0
#define ASR_TXQ_POS_ID(txq)   0

static inline bool asr_txq_is_stopped(struct asr_txq *txq)
{
    return (txq->status & ASR_TXQ_STOP);
}

static inline bool asr_txq_is_full(struct asr_txq *txq)
{
    return (txq->status & ASR_TXQ_STOP_FULL);
}

static inline bool asr_txq_is_scheduled(struct asr_txq *txq)
{
    return (txq->status & ASR_TXQ_IN_HWQ_LIST);
}

struct asr_txq *asr_txq_sta_get(struct asr_sta *sta, uint8_t tid, int *idx,
                                  struct asr_hw * asr_hw);
struct asr_txq *asr_txq_vif_get(struct asr_vif *vif, uint8_t type, int *idx);

/* return status bits related to the vif */
static inline uint8_t asr_txq_vif_get_status(struct asr_vif *asr_vif)
{
    struct asr_txq *txq = asr_txq_vif_get(asr_vif, 0, NULL);
    return (txq->status & (ASR_TXQ_STOP_CHAN | ASR_TXQ_STOP_VIF_PS));
}
void asr_txq_deinit(struct asr_hw *asr_hw, struct asr_txq *txq);

void asr_txq_vif_init(struct asr_hw * asr_hw, struct asr_vif *vif,
                       uint8_t status);
void asr_txq_vif_deinit(struct asr_hw * asr_hw, struct asr_vif *vif);
void asr_txq_sta_init(struct asr_hw * asr_hw, struct asr_sta *asr_sta,
                       uint8_t status);
void asr_txq_sta_deinit(struct asr_hw * asr_hw, struct asr_sta *asr_sta);

void asr_txq_offchan_init(struct asr_vif *asr_vif);
void asr_txq_offchan_deinit(struct asr_vif *asr_vif);

void asr_txq_add_to_hw_list(struct asr_txq *txq);
void asr_txq_del_from_hw_list(struct asr_txq *txq);
void asr_txq_stop(struct asr_txq *txq, uint16_t reason);
void asr_txq_start(struct asr_txq *txq, uint16_t reason);
void asr_txq_vif_start(struct asr_vif *vif, uint16_t reason,
                        struct asr_hw *asr_hw);
void asr_txq_vif_stop(struct asr_vif *vif, uint16_t reason,
                       struct asr_hw *asr_hw);
void asr_txq_sta_start(struct asr_sta *sta, uint16_t reason,
                        struct asr_hw *asr_hw);
void asr_txq_sta_stop(struct asr_sta *sta, uint16_t reason,
                       struct asr_hw *asr_hw);
void asr_txq_offchan_start(struct asr_hw *asr_hw);
void asr_txq_sta_switch_vif(struct asr_sta *sta, struct asr_vif *old_vif,
                             struct asr_vif *new_vif);

int asr_txq_queue_skb(struct sk_buff *skb, struct asr_txq *txq,
                       struct asr_hw *asr_hw,  bool retry);
void asr_txq_confirm_any(struct asr_hw *asr_hw, struct asr_txq *txq,
                                                              struct asr_hwq *hwq, struct asr_sw_txhdr *sw_txhdr);

void asr_hwq_init(struct asr_hw *asr_hw);
void asr_hwq_process(struct asr_hw *asr_hw, struct asr_hwq *hwq);
void asr_hwq_process_all(struct asr_hw *asr_hw);

struct sk_buff *skb_dequeue(struct sk_buff_head *list);

#endif /* _UWIFI_TXQ_H_ */
