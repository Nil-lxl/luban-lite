/**
 ******************************************************************************
 *
 * @file rwnx_rx.h
 *
 * Copyright (C) RivieraWaves 2012-2019
 *
 ******************************************************************************
 */
#ifndef _RWNX_RX_H_
#define _RWNX_RX_H_

enum rx_status_bits
{
    /// The buffer can be forwarded to the networking stack
    RX_STAT_FORWARD = 1 << 0,
    /// A new buffer has to be allocated
    RX_STAT_ALLOC = 1 << 1,
    /// The buffer has to be deleted
    RX_STAT_DELETE = 1 << 2,
    /// The length of the buffer has to be updated
    RX_STAT_LEN_UPDATE = 1 << 3,
    /// The length in the Ethernet header has to be updated
    RX_STAT_ETH_LEN_UPDATE = 1 << 4,
    /// Simple copy
    RX_STAT_COPY = 1 << 5,
    /// Spurious frame (inform upper layer and discard)
    RX_STAT_SPURIOUS = 1 << 6,
    /// packet for monitor interface
    RX_STAT_MONITOR = 1 << 7,
};


/*
 * Decryption status subfields.
 * {
 */
#define RWNX_RX_HD_DECR_UNENC           0 // Frame unencrypted
#define RWNX_RX_HD_DECR_ICVFAIL         1 // WEP/TKIP ICV failure
#define RWNX_RX_HD_DECR_CCMPFAIL        2 // CCMP failure
#define RWNX_RX_HD_DECR_AMSDUDISCARD    3 // A-MSDU discarded at HW
#define RWNX_RX_HD_DECR_NULLKEY         4 // NULL key found
#define RWNX_RX_HD_DECR_WEPSUCCESS      5 // Security type WEP
#define RWNX_RX_HD_DECR_TKIPSUCCESS     6 // Security type TKIP
#define RWNX_RX_HD_DECR_CCMPSUCCESS     7 // Security type CCMP (or WPI)
// @}


/// Structure containing information about the received frame (length, timestamp, rate, etc.)
struct rx_vector
{
    /// Total length of the received MPDU
    uint16_t            frmlen;
    /// AMPDU status information
    uint16_t            ampdu_stat_info;
    /// TSF Low
    uint32_t            tsflo;
    /// TSF High
    uint32_t            tsfhi;
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
};

/// Structure containing the information about the PHY channel that was used for this RX
struct phy_channel_info
{
    /// PHY channel information 1
    uint32_t info1;
    /// PHY channel information 2
    uint32_t info2;
};

/// Structure containing the information about the received payload
struct rx_info
{
    /// Rx header descriptor (this element MUST be the first of the structure)
    struct rx_vector vect;
    /// Structure containing the information about the PHY channel that was used for this RX
    struct phy_channel_info phy_info;
    /// Word containing some SW flags about the RX packet
    uint32_t flags;
    #if NX_AMSDU_DEAGG
    /// Array of host buffer identifiers for the other A-MSDU subframes
    uint32_t amsdu_hostids[NX_MAX_MSDU_PER_RX_AMSDU - 1];
    #endif
    /// Spare room for LMAC FW to write a pattern when last DMA is sent
    uint32_t pattern;

    uint16_t reserved[2];
};


#endif /* _RWNX_RX_H_ */
