/**
 ******************************************************************************
 *
 * @file uwifi_tx.h
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */
#ifndef _UWIFI_TX_H_
#define _UWIFI_TX_H_

#include <stdint.h>
#include <stdbool.h>
#include "uwifi_wlan_list.h"
#include "uwifi_common.h"
#include "uwifi_ipc_shared.h"
#include "uwifi_types.h"
#include "uwifi_ieee80211.h"
#include "uwifi_ops_adapter.h"
#include "uwifi_defs.h"

/* These are the defined Ethernet Protocol ID's. */
#define ETH_P_IP            0x0800        /* Internet Protocol packet    */
#define ETH_P_ARP            0x0806        /* Address Resolution packet    */
#define ETH_P_RARP            0x8035        /* Reverse Addr Res packet    */
#define ETH_P_AARP            0x80F3        /* Appletalk AARP        */
#define ETH_P_IPX            0x8137        /* IPX over DIX            */
#define ETH_P_PAE            0x888E         /* Port Access Entity (IEEE 802.1X) */
#define ETH_P_IPV6            0x86DD        /* IPv6 over bluebook        */
#define ETH_P_MPLS_UC        0x8847        /* MPLS Unicast traffic        */
#define ETH_P_MPLS_MC        0x8848        /* MPLS Multicast traffic    */
#define ETH_P_80221            0x8917        /* IEEE 802.21 Media Independent Handover Protocol */


#define ASR_HWQ_BK                     0
#define ASR_HWQ_BE                     1
#define ASR_HWQ_VI                     2
#define ASR_HWQ_VO                     3
#define ASR_HWQ_BCMC                   4
#define ASR_HWQ_NB                     NX_TXQ_CNT
#define ASR_HWQ_ALL_ACS (ASR_HWQ_BK | ASR_HWQ_BE | ASR_HWQ_VI | ASR_HWQ_VO)
#define ASR_HWQ_ALL_ACS_BIT ( BIT(ASR_HWQ_BK) | BIT(ASR_HWQ_BE) |    \
                               BIT(ASR_HWQ_VI) | BIT(ASR_HWQ_VO) )

#define ASR_SWTXHDR_ALIGN_SZ           4
#define ASR_SWTXHDR_ALIGN_MSK (ASR_SWTXHDR_ALIGN_SZ - 1)
#define ASR_SWTXHDR_ALIGN_PADS(x) \
                    ((ASR_SWTXHDR_ALIGN_SZ - ((x) & ASR_SWTXHDR_ALIGN_MSK)) \
                     & ASR_SWTXHDR_ALIGN_MSK)


#if defined(CONFIG_ASR595X) || defined(AWORKS)          // wifi6 & wifi4 use same setting to share wpa lib.
#define HOST_PAD_LEN 62           // (2+60) , 60 is max mac extend hdr len(include wapi)
#else
#define HOST_PAD_LEN 4 //need adjust
#endif

#define AMSDU_PADDING(x) ((4 - ((x) & 0x3)) & 0x3)

#define TXU_CNTRL_RETRY        BIT(0)
#define TXU_CNTRL_MORE_DATA    BIT(2)
#define TXU_CNTRL_MGMT         BIT(3)
#define TXU_CNTRL_MGMT_NO_CCK  BIT(4)
#define TXU_CNTRL_AMSDU        BIT(6)
#define TXU_CNTRL_MGMT_ROBUST  BIT(7)
#define TXU_CNTRL_USE_4ADDR    BIT(8)
#define TXU_CNTRL_EOSP         BIT(9)
#define TXU_CNTRL_MESH_FWD     BIT(10)
#define TXU_CNTRL_TDLS         BIT(11)

/// This frame is postponed internally because of PS. (only for AP)
#define TXU_CNTRL_POSTPONE_PS   BIT(12)
/// This frame is probably respond to probe request from other channels
#define TXU_CNTRL_LOWEST_RATE   BIT(13)
/// Internal flag indicating that this packet should use the trial rate as first or second rate
#define TXU_CNTRL_RC_TRIAL      BIT(14)

// new add for mrole
#define TXU_CNTRL_DROP          BIT(15)


extern const int asr_tid2hwq[IEEE80211_NUM_TIDS];

#define IPC_TXQUEUE_CNT     NX_TXQ_CNT

enum netdev_tx {
    NETDEV_TX_OK     = 0x00,    /* driver took care of packet */
    NETDEV_TX_BUSY     = 0x10,    /* driver tx path was busy*/
    NETDEV_TX_LOCKED = 0x20,    /* driver tx lock was already taken */
};

/**
 * struct asr_amsdu_txhdr - Structure added in skb headroom (instead of
 * asr_txhdr) for amsdu subframe buffer (except for the first subframe
 * that has a normal asr_txhdr)
 *
 * @list     List of other amsdu subframe (asr_sw_txhdr.amsdu.hdrs)
 * @map_len  Length to be downloaded for this subframe
 * @dma_addr Buffer address form embedded point of view
 * @skb      skb
 * @pad      padding added before this subframe
 *           (only use when amsdu must be dismantled)
 */
struct asr_amsdu_txhdr {
    struct list_head list;
    size_t map_len;
    dma_addr_t dma_addr;
    struct sk_buff *skb;
    uint16_t pad;
};

#if 0
/**
 * struct asr_sw_txhdr - Software part of tx header
 *
 * @asr_sta sta to which this buffer is addressed
 * @asr_vif vif that send the buffer
 * @txq pointer to TXQ used to send the buffer
 * @hw_queue Index of the HWQ used to push the buffer.
 *           May be different than txq->hwq->id on confirmation.
 * @frame_len Size of the frame (doesn't not include mac header)
 *            (Only used to update stat, can't we use skb->len instead ?)
 * @headroom Headroom added in skb to add asr_txhdr
 *           (Only used to remove it before freeing skb, is it needed ?)
 * @amsdu Description of amsdu whose first subframe is this buffer
 *        (amsdu.nb = 0 means this buffer is not part of amsdu)
 * @skb skb received from transmission
 * @map_len  Length mapped for DMA (only asr_hw_txhdr and data are mapped)
 * @dma_addr DMA address after mapping
 * @desc Buffer description that will be copied in shared mem for FW
 */
struct asr_sw_txhdr {
    struct asr_sta *asr_sta;
    struct asr_vif *asr_vif;
    struct asr_txq *txq;
    uint8_t hw_queue;
    uint16_t frame_len;
    uint16_t headroom;
    struct sk_buff *skb;

    size_t map_len;
    dma_addr_t dma_addr;
    struct txdesc_api desc;
};
#endif

/**
 * struct asr_hw_txstatus - Bitfield of confirmation status
 *
 * @tx_done: packet has been sucessfully transmitted
 * @retry_required: packet has been transmitted but not acknoledged.
 * Driver must repush it.
 * @sw_retry_required: packet has not been transmitted (FW wasn't able to push
 * it when it received it: not active channel ...). Driver must repush it.
 */
union asr_hw_txstatus {
    struct {
        uint32_t tx_done            : 1;
        uint32_t retry_required     : 1;
        uint32_t sw_retry_required  : 1;
        uint32_t reserved           :29;
    };
    uint32_t value;
};

/**
 * struct tx_cfm_tag - Structure indicating the status and other
 * information about the transmission
 *
 * @pn: PN that was used for the transmission
 * @sn: Sequence number of the packet
 * @timestamp: Timestamp of first transmission of this MPDU
 * @credits: Number of credits to be reallocated for the txq that push this
 * buffer (can be 0 or 1)
 * @ampdu_size: Size of the ampdu in which the frame has been transmitted if
 * this was the last frame of the a-mpdu, and 0 if the frame is not the last
 * frame on a a-mdpu.
 * 1 means that the frame has been transmitted as a singleton.
 * @amsdu_size: Size, in bytes, allowed to create a-msdu.
 * @status: transmission status
 */
struct tx_cfm_tag
{
    uint16_t pn[4];
    uint16_t sn;
    uint16_t timestamp;
    int8_t credits;
    uint8_t ampdu_size;
#ifdef CFG_AMSDU
    uint16_t amsdu_size;
#endif
    union asr_hw_txstatus status;
};

/**
 * struct asr_hw_txhdr - Hardware part of tx header
 *
 * @cfm: Information updated by fw/hardware after sending a frame
 */
struct asr_hw_txhdr {
    struct tx_cfm_tag cfm;
};

struct asr_sw_txhdr {
    //uint16_t headroom;
    struct txdesc_api desc;
};


/**
 * struct asr_txhdr - Stucture to control transimission of packet
 * (Added in skb headroom)
 *
 * @sw_hdr: Information from driver
 * @cache_guard:
 * @hw_hdr: Information for/from hardware
 */
struct asr_txhdr {
    struct asr_sw_txhdr sw_hdr;
    uint8_t host_pad[HOST_PAD_LEN];
};

struct asr_vif;
struct cfg80211_mgmt_tx_params;
struct asr_sta;

uint16_t asr_select_queue(struct asr_vif *asr_vif, struct sk_buff *skb);
int asr_start_xmit(struct asr_vif *asr_vif, struct sk_buff *skb);
int asr_start_mgmt_xmit(struct asr_vif *vif, struct asr_sta *sta,struct cfg80211_mgmt_tx_params *params,
                                                                                        bool offchan, uint64_t *cookie);
//int asr_txdatacfm(void *pthis, void *host_id);

void asr_set_traffic_status(struct asr_hw *asr_hw,
                             struct asr_sta *sta,
                             bool available,
                             uint8_t ps_id);
void asr_ps_bh_enable(struct asr_hw *asr_hw, struct asr_sta *sta,
                       bool enable);
void asr_ps_bh_traffic_req(struct asr_hw *asr_hw, struct asr_sta *sta,
                            uint16_t pkt_req, uint8_t ps_id);

void asr_switch_vif_sta_txq(struct asr_sta *sta, struct asr_vif *old_vif,
                             struct asr_vif *new_vif);

int asr_dbgfs_print_sta(char *buf, size_t size, struct asr_sta *sta,
                         struct asr_hw *asr_hw);
void asr_txq_credit_update(struct asr_hw *asr_hw, int sta_idx, uint8_t tid,
                            int8_t update);
void asr_tx_push(struct asr_hw *asr_hw, struct asr_txhdr *txhdr, int flags);
bool ieee80211_is_robust_mgmt_frame(uint16_t frame_len, const uint8_t *ieee80211_hdr_v);
int asr_prep_tx(struct asr_txhdr *txhdr);

void asr_tx_agg_buf_reset(struct asr_hw *asr_hw);
void asr_tx_agg_buf_mask_vif(struct asr_hw *asr_hw,struct asr_vif *asr_vif);
void asr_tx_agg_buf_reset(struct asr_hw *asr_hw);

#define TRAFFIC_VIF_FC_LEVEL       (64)  //(96)
#define MROLE_VIF_FC_DIV           (128)


// tx agg buf
#define CFG_VIF_MAX_BYTES       (TX_AGG_BUF_UNIT_CNT * TX_AGG_BUF_UNIT_SIZE * (MROLE_VIF_FC_DIV - TRAFFIC_VIF_FC_LEVEL) / MROLE_VIF_FC_DIV)
#define TRAFFIC_VIF_MAX_BYTES   (TX_AGG_BUF_UNIT_CNT * TX_AGG_BUF_UNIT_SIZE * TRAFFIC_VIF_FC_LEVEL / MROLE_VIF_FC_DIV)
#define UNIQUE_VIF_MAX_BYTES    (TX_AGG_BUF_UNIT_CNT * TX_AGG_BUF_UNIT_SIZE)

#define CFG_VIF_LWM   (CFG_VIF_MAX_BYTES / 8)
#define CFG_VIF_HWM   (CFG_VIF_MAX_BYTES * 7 / 8)

#define TRA_VIF_LWM   (TRAFFIC_VIF_MAX_BYTES / 8)
#define TRA_VIF_HWM   (TRAFFIC_VIF_MAX_BYTES * 7 / 8)

#define UNIQUE_VIF_LWM   (UNIQUE_VIF_MAX_BYTES / 8)
#define UNIQUE_VIF_HWM   (UNIQUE_VIF_MAX_BYTES * 7 / 8)

// tx list
#define CFG_VIF_MAX_CNTS       (TX_AGG_BUF_UNIT_CNT  * (MROLE_VIF_FC_DIV - TRAFFIC_VIF_FC_LEVEL) / MROLE_VIF_FC_DIV)
#define TRAFFIC_VIF_MAX_CNTS   (TX_AGG_BUF_UNIT_CNT  * TRAFFIC_VIF_FC_LEVEL / MROLE_VIF_FC_DIV)
#define UNIQUE_VIF_MAX_CNTS    (TX_AGG_BUF_UNIT_CNT )

#define CFG_VIF_CNT_LWM   (CFG_VIF_MAX_CNTS / 8)
#define CFG_VIF_CNT_HWM   (CFG_VIF_MAX_CNTS * 7 / 8)

#define TRA_VIF_CNT_LWM   (TRAFFIC_VIF_MAX_CNTS / 8)
#define TRA_VIF_CNT_HWM   (TRAFFIC_VIF_MAX_CNTS * 7 / 8)

#define UNIQUE_VIF_CNT_LWM   (UNIQUE_VIF_MAX_CNTS / 8)
#define UNIQUE_VIF_CNT_HWM   (UNIQUE_VIF_MAX_CNTS * 7 / 8)


#endif /* _UWIFI_TX_H_ */
