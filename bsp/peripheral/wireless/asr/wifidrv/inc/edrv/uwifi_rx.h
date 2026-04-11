/**
 ******************************************************************************
 *
 * @file uwifi_rx.h
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */
#ifndef _UWIFI_RX_H_
#define _UWIFI_RX_H_

#include <stdint.h>
#include "uwifi_types.h"
#include "uwifi_ipc_shared.h"
#include "uwifi_ops_adapter.h"

enum rx_status_bits
{
    /// The buffer can be forwarded to the networking stack
    RX_STAT_FORWARD = 1 << 0,
    /// A new buffer has to be allocated
    RX_STAT_ALLOC = 1 << 1,
    /// The buffer has to be deleted
    RX_STAT_DELETE = 1 << 2,
    /************************************* defrag used start*********************************/
    /// The length of the buffer has to be updated
    RX_STAT_LEN_UPDATE = 1 << 3,
    /// The length in the Ethernet header has to be updated
    RX_STAT_ETH_LEN_UPDATE = 1 << 4,
    /// Simple copy
    RX_STAT_COPY = 1 << 5,
    /************************************* defrag used end*********************************/
    /// Spurious frame (inform upper layer and discard)
    RX_STAT_SPURIOUS = 1 << 6,
    /// Frame for monitor interface
    RX_STAT_MONITOR = 1 << 7,
    /// unsupported frame
    RX_STAT_UF = 1 << 8,
};


/*
 * Decryption status subfields.
 * {
 */
#define ASR_RX_HD_DECR_UNENC           0 // Frame unencrypted
#define ASR_RX_HD_DECR_ICVFAIL         1 // WEP/TKIP ICV failure
#define ASR_RX_HD_DECR_CCMPFAIL        2 // CCMP failure
#define ASR_RX_HD_DECR_AMSDUDISCARD    3 // A-MSDU discarded at HW
#define ASR_RX_HD_DECR_NULLKEY         4 // NULL key found
#define ASR_RX_HD_DECR_WEPSUCCESS      5 // Security type WEP
#define ASR_RX_HD_DECR_TKIPSUCCESS     6 // Security type TKIP
#define ASR_RX_HD_DECR_CCMPSUCCESS     7 // Security type CCMP
// @}

#define NET_XMIT_SUCCESS    0x00
#define NET_XMIT_DROP        0x01    /* skb dropped            */

struct hw_vect {
    /** Total length for the MPDU transfer */
    uint32_t len                   :16;

    uint32_t reserved              : 8;

    /** AMPDU Status Information */
    uint32_t mpdu_cnt              : 6;
    uint32_t ampdu_cnt             : 2;


    /** TSF Low */
    uint32_t tsf_lo;
    /** TSF High */
    uint32_t tsf_hi;

    /** Receive Vector 1a */
    uint32_t    leg_length         :12;
    uint32_t    leg_rate           : 4;
    uint32_t    ht_length          :16;

    /** Receive Vector 1b */
    uint32_t    _ht_length         : 4; // FIXME
    uint32_t    short_gi           : 1;
    uint32_t    stbc               : 2;
    uint32_t    smoothing          : 1;
    uint32_t    mcs                : 7;
    uint32_t    pre_type           : 1;
    uint32_t    format_mod         : 3;
    uint32_t    ch_bw              : 2;
    uint32_t    n_sts              : 3;
    uint32_t    lsig_valid         : 1;
    uint32_t    sounding           : 1;
    uint32_t    num_extn_ss        : 2;
    uint32_t    aggregation        : 1;
    uint32_t    fec_coding         : 1;
    uint32_t    dyn_bw             : 1;
    uint32_t    doze_not_allowed   : 1;

    /** Receive Vector 1c */
    uint32_t    antenna_set        : 8;
    uint32_t    partial_aid        : 9;
    uint32_t    group_id           : 6;
    uint32_t    reserved_1c        : 1;
    int         rssi1              : 8;

    /** Receive Vector 1d */
    int    rssi2              : 8;
    int    rssi3              : 8;
    int    rssi4              : 8;
    uint32_t    reserved_1d        : 8;

    /** Receive Vector 2a */
    uint32_t    rcpi               : 8;
    uint32_t    evm1               : 8;
    uint32_t    evm2               : 8;
    uint32_t    evm3               : 8;

    /** Receive Vector 2b */
    uint32_t    evm4               : 8;
    uint32_t    reserved2b_1       : 8;
    uint32_t    reserved2b_2       : 8;
    uint32_t    reserved2b_3       : 8;

    /** Status **/
    uint32_t    rx_vect2_valid     : 1;
    uint32_t    resp_frame         : 1;
    /** Decryption Status */
    uint32_t    decr_status        : 3;
    uint32_t    rx_fifo_oflow      : 1;

    /** Frame Unsuccessful */
    uint32_t    undef_err          : 1;
    uint32_t    phy_err            : 1;
    uint32_t    fcs_err            : 1;
    uint32_t    addr_mismatch      : 1;
    uint32_t    ga_frame           : 1;
    uint32_t    current_ac         : 2;

    uint32_t    frm_successful_rx  : 1;
    /** Descriptor Done  */
    uint32_t    desc_done_rx       : 1;
    /** Key Storage RAM Index */
    uint32_t    key_sram_index     : 10;
    /** Key Storage RAM Index Valid */
    uint32_t    key_sram_v         : 1;
    uint32_t    type               : 2;
    uint32_t    subtype            : 4;
};

struct hw_rxhdr {
    /** RX vector */
    struct hw_vect hwvect;

    /** PHY channel information 1 */
    uint32_t    phy_band           : 8;
    uint32_t    phy_channel_type   : 8;
    uint32_t    phy_prim20_freq    : 16;
    /** PHY channel information 2 */
    uint32_t    phy_center1_freq   : 16;
    uint32_t    phy_center2_freq   : 16;
    /** RX flags */
    uint32_t    flags_is_amsdu     : 1;
    uint32_t    flags_is_80211_mpdu: 1;
    uint32_t    flags_is_4addr     : 1;
    uint32_t    flags_new_peer     : 1;
    uint32_t    flags_user_prio    : 3;
    uint32_t    flags_rsvd0        : 1;
    uint32_t    flags_vif_idx      : 8;    // 0xFF if invalid VIF index
    uint32_t    flags_sta_idx      : 8;    // 0xFF if invalid STA index
    uint32_t    flags_dst_idx      : 8;    // 0xFF if unknown destination STA
};
//sdio fw add
struct host_rx_desc
{
    //id to indicate data/log/msg
    uint16_t            id;          // data=0xffff, log=0xfffe, others is msg
    // payload offset
    uint16_t            pld_offset;
    /// Total length of the payload
    uint16_t            frmlen;
    /// AMPDU status information
    uint16_t            ampdu_stat_info;
    /// TSF Low
    uint16_t            sdio_rx_len;  //this pkt sdio transfer total len, n*blocksize
    /// TSF Low
    uint16_t            totol_frmlen;
    /// TSF High
    uint16_t            seq_num;
    /// TSF High
    uint16_t            fn_num;
    /// Contains the bytes 4 - 1 of Receive Vector 1
    uint32_t            recvec1a;
    /// Contains the bytes 8 - 5 of Receive Vector 1
    uint32_t            recvec1b;
    /// Contains the bytes 12 - 9 of Receive Vector 1
    uint32_t            recvec1c;
    /// Contains the bytes 16 - 13 of Receive Vector 1
    uint32_t            recvec1d;
    /// Contains the bytes 4 - 1 of Receive Vector 2
    uint32_t            recvec2a;
    ///  Contains the bytes 8 - 5 of Receive Vector 2
    uint32_t            recvec2b;
    /// MPDU status information
    uint32_t            statinfo;
    /// Structure containing the information about the PHY channel that was used for this RX
    struct phy_channel_info phy_info;
    /// Word containing some SW flags about the RX packet
    uint32_t flags;
    // sdio rx agg num.
    uint16_t num;
    // new add for ooo
    uint8_t            sta_idx;
    uint8_t            tid;
    uint16_t           rx_status;
};
#define HOST_RX_DESC_SIZE (sizeof(struct host_rx_desc))
#define HOST_RX_DESC_PART_LEN 38    //from ampdu_stat_info to statinfo
#define HOST_RX_DATA_ID (0xFFFF)
#define HOST_RX_DESC_ID (0x7FFE)


struct asr_e2arxdesc_elem {
    struct rxdesc_tag *rxdesc_ptr;
    dma_addr_t dma_addr;
};

/**
 * struct asr_skb_cb - Control Buffer structure for RX buffer
 *
 * @dma_addr: DMA address of the data buffer
 * @pattern: Known pattern (used to check pointer on skb)
 * @idx: Index in asr_hw.rxbuff_table to contians address of this buffer
 */
struct asr_skb_cb {
    dma_addr_t dma_addr;
    uint32_t pattern;
    uint32_t idx;
}__attribute__((__may_alias__));

#define ASR_RXBUFF_HOSTID_TO_IDX(hostid) ((hostid) - 1)
#define ASR_RXBUFF_VALID_IDX(idx) ((idx) < ASR_RXBUFF_MAX)


#define ASR_RXBUFF_PATTERN     (0xCAFEFADE)

#define ASR_RXBUFF_DMA_ADDR_SET(skbuff, addr)          \
    ((struct asr_skb_cb *)(skbuff->cb))->dma_addr = addr
#define ASR_RXBUFF_DMA_ADDR_GET(skbuff)                \
    ((struct asr_skb_cb *)(skbuff->cb))->dma_addr

#define ASR_RXBUFF_PATTERN_SET(skbuff, pat)                \
        ((struct asr_skb_cb *)(skbuff->cb))->pattern = pat
#define ASR_RXBUFF_PATTERN_GET(skbuff)         \
        ((struct asr_skb_cb *)(skbuff->cb))->pattern

#define ASR_RXBUFF_IDX_SET(skbuff, val)                \
        ((struct asr_skb_cb *)(skbuff->cb))->idx = val
#define ASR_RXBUFF_IDX_GET(skbuff)             \
        ((struct asr_skb_cb *)(skbuff->cb))->idx

#define ASR_RXBUFF_IDX_TO_HOSTID(idx) ((idx) + 1)


#define ASR_VIF_TYPE(asr_vif) (asr_vif->iftype)


/**
 * asr_rxbuff_pull - Extract a skb from asr_hw.rxbuff_table
 *
 * @asr_hw: Main driver data
 * @skb: Buffer to pull from the table
 *
 */
void asr_rxbuff_pull(struct asr_hw *asr_hw, struct sk_buff *skb);
uint8_t asr_rxdataind(void *pthis, void *hostid);
uint8_t asr_dbgind(void *pthis, void *hostid);

#if 0//def CFG_SNIFFER_SUPPORT
uint8_t asr_rxdataind_sniffer(void *pthis, void *hostid);
#endif
uint32_t asr_rxbuff_push(struct asr_hw *asr_hw, struct sk_buff *skb);
OSStatus init_rx_uwifi(struct asr_hw *asr_hw);
OSStatus deinit_rx_uwifi(void);

typedef struct {
    uint32_t packets;
    uint32_t time;
} rx_packet_timer_t;
typedef struct {
    uint32_t speed;//bytes per microseconds
    uint32_t time;
} rx_bytes_timer_t;
#define RX_TIMER_DYNAMIC_LEVELS     8
#define RX_TIMER_TRAIN_TIMES        100
#endif /* _UWIFI_RX_H_ */
