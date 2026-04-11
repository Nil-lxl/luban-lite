/**
 ****************************************************************************************
 *
 * @file ipc_shared.h
 *
 * @brief Shared data between both IPC modules.
 *
 * Copyright (C) ASR 2011-2016
 *
 ****************************************************************************************
 */

#ifndef _IPC_SHARED_H_
#define _IPC_SHARED_H_

/**
 ****************************************************************************************
 * @defgroup IPC IPC
 * @ingroup PLATFORM_DRIVERS
 * @brief Inter Processor Communication module.
 *
 * The IPC module implements the protocol to communicate between the Host CPU
 * and the Embedded CPU.
 *
 * A typical use case of the IPC Tx path API:
 * @msc
 * hscale = "2";
 *
 * a [label=Driver],
 * b [label="IPC host"],
 * c [label="IPC emb"],
 * d [label=Firmware];
 *
 * ---   [label="Tx descriptor queue example"];
 * a=>a  [label="Driver receives a Tx packet from OS"];
 * a=>b  [label="ipc_host_txdesc_get()"];
 * a<<b  [label="struct txdesc_host *"];
 * a=>a  [label="Driver fill the descriptor"];
 * a=>b  [label="ipc_host_txdesc_push()"];
 * ...   [label="(several Tx desc can be pushed)"];
 * b:>c  [label="Tx desc queue filled IRQ"];
 * c=>>d [label="IPC emb Tx desc callback"];
 * ...   [label="(several Tx desc can be popped)"];
 * d=>d  [label="Packets are sent or discarded"];
 * ---   [label="Tx confirm queue example"];
 * c<=d  [label="ipc_emb_txcfm_push()"];
 * c>>d  [label="Request accepted"];
 * ...   [label="(several Tx cfm can be pushed)"];
 * b<:c  [label="Tx cfm queue filled IRQ"];
 * a<<=b [label="Driver's Tx Confirm callback"];
 * a=>b  [label="ipc_host_txcfm_pop()"];
 * a<<b  [label="struct ipc_txcfm"];
 * a<=a  [label="Packets are freed by the driver"];
 * @endmsc
 *
 * A typical use case of the IPC Rx path API:
 * @msc
 * hscale = "2";
 *
 * a [label=Firmware],
 * b [label="IPC emb"],
 * c [label="IPC host"],
 * d [label=Driver];
 *
 * ---   [label="Rx buffer and desc queues usage example"];
 * d=>c  [label="ipc_host_rxbuf_push()"];
 * d=>c  [label="ipc_host_rxbuf_push()"];
 * d=>c  [label="ipc_host_rxbuf_push()"];
 * ...   [label="(several Rx buffer are pushed)"];
 * a=>a  [label=" Frame is received\n from the medium"];
 * a<<b  [label="struct ipc_rxbuf"];
 * a=>a  [label=" Firmware fill the buffer\n with received frame"];
 * a<<b  [label="Push accepted"];
 * ...   [label="(several Rx desc can be pushed)"];
 * b:>c  [label="Rx desc queue filled IRQ"];
 * c=>>d [label="Driver Rx packet callback"];
 * c<=d  [label="ipc_host_rxdesc_pop()"];
 * d=>d  [label="Rx packet is handed \nover to the OS "];
 * ...   [label="(several Rx desc can be poped)"];
 * ---   [label="Rx buffer request exemple"];
 * b:>c  [label="Low Rx buffer count IRQ"];
 * a<<b  [label="struct ipc_rxbuf"];
 * c=>>d [label="Driver Rx buffer callback"];
 * d=>c  [label="ipc_host_rxbuf_push()"];
 * d=>c  [label="ipc_host_rxbuf_push()"];
 * d=>c  [label="ipc_host_rxbuf_push()"];
 * ...   [label="(several Rx buffer are pushed)"];
 * @endmsc
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
//#include "co_int.h"
//#include "dma.h"
#include "asr_config.h"

/*
 * DEFINES AND MACROS
 ****************************************************************************************
 */
/// Number of IPC TX queues
#define IPC_TXQUEUE_CNT     NX_TXQ_CNT

#define IPC_RXDESC_CNT      15

//but when wrap exceed the maximum index 15, then 6 is the maximum rx_aggr_max_num, as port index 0 and 1 is reversed for log and msg
//
#define RX_AGGR_BUF_NUM  8 // 6

/*
 * Number of Host buffers available for Data Rx handling
 */
#ifdef SDIO_DEAGGR
#define IPC_RXBUF_CNT   1 //
#else
#define IPC_RXBUF_CNT   5 // 7 // 8 //    8*6
#endif

/*
 * RX Data buffers size (in bytes) need adjust
*/

#define IPC_RXBUF_SIZE      ASR_ALIGN_BLKSZ_HI(1696)*(RX_AGGR_BUF_NUM)    //13568 //1696*8

#define IPC_RXMSGBUF_CNT    2
#define IPC_RXMSGBUF_SIZE   ASR_ALIGN_BLKSZ_HI(1696)
/*
 * Number of Host buffers available for Data Rx handling
 */
#define IPC_RXBUF_CNT_SPLIT       1

/*
 * RX Data buffers size (in bytes) need adjust
 */
#define IPC_RXBUF_SIZE_SPLIT      (WLAN_AMSDU_RX_LEN)

#define IPC_HIF_TXBUF_SIZE  ASR_ALIGN_BLKSZ_HI(13568)    //1696*8
#ifdef SDIO_DEAGGR

#define IPC_RXBUF_CNT_SDIO_DEAGG       (30) // (26) // (26)

#define IPC_RXBUF_SIZE_SDIO_DEAGG      (1696)
#endif


/// Number of Host buffers available for Emb->App MSGs sending (through DMA)
#define IPC_MSGE2A_BUF_CNT      5 //64

/// Length used in APP2EMB MSGs structures
#define IPC_A2E_MSG_BUF_SIZE    127 // size in 4-byte words


/// Length used in EMB2APP MSGs structures
#define IPC_E2A_MSG_PARAM_SIZE   256 // size in 4-byte words
/// Length for dbg log
#define IPC_DBG_PARAM_SIZE       512

/**
 * Define used for MSG buffers validity.
 * This value will be written only when a MSG buffer is used for sending from Emb to App.
 */
#define IPC_MSGE2A_VALID_PATTERN 0xADDEDE2A


#ifndef __packed
#define __packed __attribute__ ((__packed__)) __attribute__((aligned(1)))
#endif

/// Message structure for MSGs from Emb to App
struct ipc_e2a_msg
{
    uint16_t id;                ///< Message id.
    uint16_t dummy_dest_id;     ///< Not used
    uint16_t dummy_src_id;      ///< Not used
    uint16_t param_len;         ///< Parameter embedded struct length.
    uint32_t param[IPC_E2A_MSG_PARAM_SIZE];  ///< Parameter embedded struct. Must be word-aligned.
};
/// Message structure for Debug messages from Emb to App

struct ipc_dbg_msg
{
    uint16_t  id;
    uint8_t string[IPC_DBG_PARAM_SIZE]; ///< Debug string
};


/// Message structure for MSGs from App to Emb.
/// Actually a sub-structure will be used when filling the messages.
struct ipc_a2e_msg
{
    uint32_t dummy_word;                ///< used to cope with kernel message structure
    uint32_t msg[IPC_A2E_MSG_BUF_SIZE]; ///< body of the msg
};

struct ipc_ooo_rxdesc_buf_hdr
{
    uint16_t           id;
    //#ifdef CONFIG_ASR_SDIO
    uint16_t           sdio_rx_len;
    //#endif
    uint8_t            rxu_stat_desc_cnt;
    /*The rxu_stat_val start here. */
} __packed;

/// RX status value structure (as expected by Upper Layers)
struct rxu_stat_val
{
    // out-of-order value.
    //struct rxu_cntrl_ooo_val ooo_val;
    // use (sta_idx/tid/seq_num/fn_num) to identify one out-of-order pkt.
    uint8_t   sta_idx;
    uint8_t   tid;
    uint16_t  seq_num;
    uint16_t  amsdu_fn_num;
    /// Status (@ref rx_status_bits)
    uint8_t status;
} __packed;


/// Information provided by host to indentify RX buffer
struct ipc_shared_rx_buf
{
    /// ptr to hostbuf client (ipc_host client) structure
    volatile uint32_t hostid;
    /// ptr to real hostbuf dma address
    volatile uint32_t dma_addr;
};

/// Information provided by host to indentify RX desc
struct ipc_shared_rx_desc
{
    /// DMA Address
    volatile uint32_t dma_addr;
};

/*
 * TYPE and STRUCT DEFINITIONS
 ****************************************************************************************
 */

/// Descriptor filled by the Host
struct hostdesc
{

    // tx len filled by host when use tx aggregation mode
    uint16_t sdio_tx_len;
    // tx total len: used for host
    uint16_t sdio_tx_total_len;
    // queue id
    uint8_t  queue_idx; //VO VI BE BK
    /// Pointer to packet payload
    uint8_t  packet_offset;
    /// Size of the payload
    uint16_t packet_len;
    /// Packet TID
    uint8_t  tid;
    /// VIF index
    uint8_t  vif_idx;
    /// Station Id (0xFF if station is unknown)
    uint8_t  staid;
    // agg_num
    uint8_t  agg_num;
    /// TX flags
    uint16_t flags;
    /// Timestamp of first transmission of this MPDU
    uint16_t timestamp;
    /// address of buffer from hostdesc in fw
    uint32_t packet_addr;
    /// PN that was used for first transmission of this MPDU
    uint32_t txq;
    uint32_t reserved;
    /// Sequence Number used for first transmission of this MPDU
    uint16_t sn;
    /// Ethernet Type
    uint16_t ethertype;
    /// Destination Address
    struct mac_addr eth_dest_addr;
    /// Source Address
    struct mac_addr eth_src_addr;
};


/// Description of the TX API
struct txdesc_api
{
    /// Information provided by Host
    struct hostdesc host;
};


/// Descriptor used for Host/Emb TX frame information exchange
struct txdesc_host
{
    /// Flag indicating if the TX descriptor is ready (different from 0) or not (equal to 0)
    uint32_t ready;

    /// API of the embedded part
    struct txdesc_api api;
};

// Indexes are defined in the MIB shared structure
/// Structure describing the IPC data shared with the host CPU
struct ipc_shared_env_tag
{
    /// Room for MSG to be sent from App to Emb
    volatile struct ipc_a2e_msg msg_a2e_buf;

    /// Room to build the Emb->App MSGs Xferred
    volatile struct    ipc_e2a_msg msg_e2a_buf;
    /// DMA descriptor for Emb->App MSGs Xfers
    //volatile struct    dma_desc msg_dma_desc;
    /// Host buffer addresses for Emb->App MSGs DMA Xfers
    //volatile uint32_t  msg_e2a_hostbuf_addr [IPC_MSGE2A_BUF_CNT];

    #if NX_DEBUG_DUMP
    /// Host buffer address for the debug dump information
    volatile uint32_t  la_dbginfo_addr;
    #endif

    /// Host buffer address for the TX payload descriptor pattern
    volatile uint32_t  pattern_addr;

    /// Array of TX descriptors for the BK queue
    volatile struct txdesc_host txdesc0[RW_USER_MAX][NX_TXDESC_CNT0];
    /// Array of TX descriptors for the BE queue
    volatile struct txdesc_host txdesc1[RW_USER_MAX][NX_TXDESC_CNT1];
    /// Array of TX descriptors for the VI queue
    volatile struct txdesc_host txdesc2[RW_USER_MAX][NX_TXDESC_CNT2];
    /// Array of TX descriptors for the VO queue
    volatile struct txdesc_host txdesc3[RW_USER_MAX][NX_TXDESC_CNT3];
    /// Array of TX descriptors for the BC/MC queue
    volatile struct txdesc_host txdesc4[1][NX_TXDESC_CNT4];

    /// RX Descriptors Array
    volatile struct ipc_shared_rx_desc host_rxdesc[IPC_RXDESC_CNT];
    /// RX Buffers Array
    volatile struct ipc_shared_rx_buf  host_rxbuf[IPC_RXBUF_CNT];

    #if NX_TRACE
    /// Pattern for host to check if trace is initialized
    volatile uint16_t trace_pattern;
    /// Index (in 16bits word) of the first trace within the trace buffer
    volatile uint32_t trace_start;
    /// Index (in 16bits word) of the last trace within the trace buffer
    volatile uint32_t trace_end;
    /// Size (in 16bits word) of the trace buffer
    volatile uint32_t trace_size;
    /// Offset (in bytes) from here to the start of the trace buffer
    volatile uint32_t trace_offset;
    #endif
};

/// IPC Shared environment
//extern struct ipc_shared_env_tag ipc_shared_env;

/*
 * TYPE and STRUCT DEFINITIONS
 ****************************************************************************************
 */

#endif // _IPC_SHARED_H_

