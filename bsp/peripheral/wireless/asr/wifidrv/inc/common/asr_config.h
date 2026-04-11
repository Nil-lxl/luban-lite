/**
 ****************************************************************************************
 *
 * @file asr_config.h
 *
 * @brief Inclusion of appropriate config files.
 *
 * This file should be included each time an NX_xxx definition is needed.
 * CFG_xxx macros must no be used in the code, only NX_xxx, defined in
 * the included config files.
 *
 * Copyright (C) ASR 2011-2016
 *
 ****************************************************************************************
 */

#ifndef _ASR_CONFIG_H_
#define _ASR_CONFIG_H_

/**
 ****************************************************************************************
 * @addtogroup MACSW
 * @{
 * The CFG_xxx macros should be added on the compilation command line
 * by the SCons scripts. Because it has been not implemented yet, any
 * undefined option fall back on the default behavior.
 ****************************************************************************************
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "mac.h"
#include "wifi_config.h"

//LWIFI TASK TEST
#ifdef CFG_LWIFI_TEST
  #define LWIFI_TEST 1
#else
  #define LWIFI_TEST 0
#endif

// Features that must be enabled for MESH support
#ifdef CFG_MESH
  #undef CFG_MROLE
  #define CFG_MROLE
  #undef CFG_MFP
  #define CFG_MFP
  #undef CFG_PS
  #define CFG_PS
#endif //defined CFG_MESH

// Features that must be enabled for UMAC support
/// Whether UMAC is enable or not
#undef CFG_CMON
#define CFG_CMON
#undef CFG_MROLE
#define CFG_MROLE

#ifdef CFG_STA_AP_COEX
    #define NX_STA_AP_COEX 1
#else
    #define NX_STA_AP_COEX 0
#endif


/**
 *******************************************************************************
 *   @name General Configuration
 *******************************************************************************
 */
/// Max number of virtual interfaces managed. MAC HW is limited to 6, but the LMAC assumes
/// that this number is a power of 2, thus only 1, 2 or 4 are allowed
#if NX_STA_AP_COEX
#define NX_VIRT_DEV_MAX 2
#else
#define NX_VIRT_DEV_MAX 1
#endif


/// Heap size
#define NX_HEAP_SIZE (2048 + 256 * NX_VIRT_DEV_MAX + 64 * NX_REMOTE_STA_MAX)

/** @} General */

/**
 *******************************************************************************
 *   @name Beacon Configuration
 *******************************************************************************
 */

#ifndef WIFI_TEST_LINUX
#define CFG_PS
//#define CFG_UAPSD
#define CFG_DPSM
#endif
/**
 *******************************************************************************
 *   @name Power Save Configuration
 *******************************************************************************
 */
/// UAPSD support
#ifdef CFG_UAPSD
  #define NX_UAPSD 1
  // If UAPSD is enabled, we force the asrcy PS mode to be enabled
  #undef CFG_PS
  #define CFG_PS
#else
  #define NX_UAPSD 0
#endif

/// DPSM support
#ifdef CFG_DPSM
  #define NX_DPSM 1
  // If DPSM is enabled, we force the asrcy PS mode to be enabled
  #undef CFG_PS
  #define CFG_PS
#else
  #define NX_DPSM 0
#endif

/// Legacy power save support
#ifdef CFG_PS
  #define NX_POWERSAVE 1
#else
  #define NX_POWERSAVE 0
#endif
/** @} Power Save */

/**
 *******************************************************************************
 * @name A-MSDU Configuration
 *******************************************************************************
 */
/// Define the A-MSDU option for TX
#ifdef CFG_AMSDU
  #define NX_AMSDU_TX 1
  // Force the aggregation to be supported
  #undef CFG_AGG
  #define CFG_AGG
  /// Number of payloads per TX descriptor
  #define NX_TX_PAYLOAD_MAX 6
#else
  #define NX_AMSDU_TX 0
#endif

/// Maximum size of A-MSDU supported in reception
#ifdef CFG_AMSDU_4K
  #define ASR_MAX_AMSDU_RX    4096
#elif defined CFG_AMSDU_8K
  #define ASR_MAX_AMSDU_RX    8192
#elif defined CFG_AMSDU_12K
  #define ASR_MAX_AMSDU_RX    12288
#else
  #define ASR_MAX_AMSDU_RX    IPC_RXBUF_SIZE
#endif
/** @} A-MSDU */

/**
 *******************************************************************************
 * @name A-MPDU Configuration
 *******************************************************************************
 */
#ifdef CFG_AGG
  /// A-MPDU TX support
  #define NX_AMPDU_TX 1
  /// Maximum number of TX Block Ack
  #define NX_MAX_BA_TX CFG_BATX
  #if (NX_MAX_BA_TX == 0)
    #error "At least one BA TX agreement shall be allowed"
  #endif
#else  // !CFG_AGG
  #define NX_AMPDU_TX  0
  #define NX_MAX_BA_TX 0
  #undef CFG_BWLEN
#endif // CFG_AGG

/// RX Packet Reordering support
#define NX_REORD CFG_BARX
/// RX Packet Reordering Buffer Size
#define NX_REORD_BUF_SIZE CFG_REORD_BUF
#if (NX_REORD && ((NX_REORD_BUF_SIZE < 4) || (NX_REORD_BUF_SIZE > 64)))
  #error "Incorrect reordering buffer size"
#endif
/** @} A-MPDU */

/**
 *******************************************************************************
 * @name Modem Configuration
 *******************************************************************************
 */
/// Modem version
#ifdef CFG_MDM_VER_V10
  #define NX_MDM_VER 10
#elif defined CFG_MDM_VER_V11
  #define NX_MDM_VER 11
#elif defined CFG_MDM_VER_V20
  #define NX_MDM_VER 20
#elif defined CFG_MDM_VER_V21
  #define NX_MDM_VER 21
#endif
/** @} Modem */

/**
 *******************************************************************************
 * @name BeamForming Configuration
 *******************************************************************************
 */
/// Beamformee support
#ifdef CFG_BFMEE
  #define RW_BFMEE_EN 1
#else
  #define RW_BFMEE_EN 0
  // Disable MU-MIMO RX if Beamformee is not supported
  #undef CFG_MU_RX
#endif

//these macros are for beamformer originally
/// Number of users supported
#define RW_USER_MAX            1
//these macros are used to disable related codes in header files which is created by xls and scripts.
#define RW_BFMER_EN     0
/// MU-MIMO TX support
#define RW_MUMIMO_TX_EN        0  //(RW_USER_MAX > 1)
/// Support for up to one secondary user
#define RW_MUMIMO_SEC_USER1_EN  0 //(RW_USER_MAX > 1)
/// Support for up to two secondary users
#define RW_MUMIMO_SEC_USER2_EN  0  //(RW_USER_MAX > 2)
/// Support for up to three secondary users
#define RW_MUMIMO_SEC_USER3_EN  0  //(RW_USER_MAX > 3)

/// MU-MIMO RX support
#ifdef CFG_MU_RX
  #define RW_MUMIMO_RX_EN 1
#else
  #define RW_MUMIMO_RX_EN 0
#endif
/** @} BeamForming */

/**
 *******************************************************************************
 * @name P2P Configuration
 *******************************************************************************
 */
#if CFG_P2P
  /// P2P support
  #define NX_P2P             1
  /// Maximum number of simultaneous P2P connections
  #define NX_P2P_VIF_MAX     CFG_P2P

  /// P2P GO Support
  #ifdef CFG_P2P_GO
    #define NX_P2P_GO 1
  #else
    #define NX_P2P_GO 0
  #endif //(GFG_P2P_GO)
#else
  #define NX_P2P         0
  #define NX_P2P_VIF_MAX 0
  #define NX_P2P_GO      0
#endif //(CFG_P2P)
/** @} P2P */

/**
 *******************************************************************************
 * @name MESH Configuration
 *******************************************************************************
 */
#ifdef CFG_MESH
  /// Wireless Mesh Networking support
  #define RW_MESH_EN       (1)
  /// UMAC support for MESH
  #define RW_UMESH_EN      (1)
  /// Maximum Number of
  #define RW_MESH_VIF_NB   (CFG_MESH_VIF)
  /// Maximum number of MESH link
  #define RW_MESH_LINK_NB  (CFG_MESH_LINK)
  /// Maximum number of MESH path
  #define RW_MESH_PATH_NB  (CFG_MESH_PATH)
  /// Maximum number of MESH proxy
  #define RW_MESH_PROXY_NB (CFG_MESH_PROXY)
#else
  #define RW_MESH_EN       (0)
  #define RW_UMESH_EN      (0)
  #define RW_MESH_VIF_NB   (0)
  #define RW_MESH_LINK_NB  (0)
  #define RW_MESH_PATH_NB  (0)
  #define RW_MESH_PROXY_NB (0)
#endif //defined CFG_MESH
/** @} MESH */

/**
 *******************************************************************************
 *   @name TX Configuration
 *******************************************************************************
 */
/// Minimal MPDU spacing we support in TX
#define NX_TX_MPDU_SPACING   CFG_SPC

/// Number of TX queues in the LMAC
#define NX_TXQ_CNT          (AC_MAX + 1)

/// Number of TX descriptors available in the system (BK)
#define NX_TXDESC_CNT0       CFG_TXDESC0
#if (NX_TXDESC_CNT0 & (NX_TXDESC_CNT0 - 1))
  #error "Not a power of 2"
#endif
/// Number of TX descriptors available in the system (BE)
#define NX_TXDESC_CNT1       CFG_TXDESC1
#if (NX_TXDESC_CNT1 & (NX_TXDESC_CNT1 - 1))
  #error "Not a power of 2"
#endif
/// Number of TX descriptors available in the system (VI)
#define NX_TXDESC_CNT2       CFG_TXDESC2
#if (NX_TXDESC_CNT2 & (NX_TXDESC_CNT2 - 1))
  #error "Not a power of 2"
#endif
/// Number of TX descriptors available in the system (VO)
#define NX_TXDESC_CNT3       CFG_TXDESC3
#if (NX_TXDESC_CNT3 & (NX_TXDESC_CNT3 - 1))
  #error "Not a power of 2"
#endif

/// Number of TX descriptors available in the system (BCN)
#define NX_TXDESC_CNT4       CFG_TXDESC4
#if (NX_TXDESC_CNT4 & (NX_TXDESC_CNT4 - 1))
  #error "Not a power of 2"
#endif

/// Number of TX frame descriptors and buffers available for frames generated internally
#define NX_TXFRAME_CNT (NX_VIRT_DEV_MAX*2+2)

/// Maximum size of a TX frame generated internally
#if (NX_P2P)
  #define NX_TXFRAME_LEN 384
#else
  #define NX_TXFRAME_LEN 256
#endif //(NX_P2P)

/// Number of TX flow control credits allocated by default per RA/TID (UMAC only)
#define NX_DEFAULT_TX_CREDIT_CNT 4
/** @} TX */

#define FLASH_LWIFI_SEG __attribute__((section("seg_flash_driver")))
/**
 *******************************************************************************
 *   @name RX Configuration
 *******************************************************************************
 */
/// RX Payload buffer size
#define NX_RX_PAYLOAD_LEN    512

/// Maximum number of the longest A-MSDUs that can be stored at the same time
#define NX_RX_LONG_MPDU_CNT  2


/// Number of RX payload descriptors - defined to be n times the maximum A-MSDU size
/// plus one extra one used for HW flow control
#define NX_RX_PAYLOAD_DESC_CNT  (21+1)//((ASR_MAX_AMSDU_RX / NX_RX_PAYLOAD_LEN) * NX_RX_LONG_MPDU_CNT + 1)

/// Number of RX descriptors (SW and Header descriptors)
#define NX_RXDESC_CNT NX_RX_PAYLOAD_DESC_CNT

/** @} RX */

// be used to disable radar related implementations of header files which is created by xls and script.
#define RW_RADAR_EN     0

/**
 *******************************************************************************
 * @name Debug Configuration
 *******************************************************************************
 */
/// Debug dump forwarding
#ifdef CFG_DBGDUMP
  #define NX_DEBUG_DUMP 1
#else
  #define NX_DEBUG_DUMP 0
#endif

/// Debug support
#define NX_DEBUG  CFG_DBG

/// Debug print output
#if NX_DEBUG
  #define NX_PRINT 1
#else
  #define NX_PRINT 0
#endif

/// Trace Buffer Support
#ifdef CFG_TRACE
  #define NX_TRACE 1
#else
  #define NX_TRACE 0
#endif

/// Profiling support
#ifdef CFG_PROF
  #define NX_PROFILING_ON 1
#else
  #define NX_PROFILING_ON 0
#endif

/// System statistics support
#ifdef CFG_STATS
  #define NX_SYS_STAT 1
#else
  #define NX_SYS_STAT 0
#endif
/** @} Debug */

/**
 *******************************************************************************
 * @name Extra Configuration
 *******************************************************************************
 */
/// Recovery support
#ifdef CFG_REC
  #define NX_RECOVERY 1
#else
  #define NX_RECOVERY 0
#endif

/// Connection monitoring support
#ifdef CFG_CMON
  #define NX_CONNECTION_MONITOR 1
#else
  #define NX_CONNECTION_MONITOR 0
#endif

/// Multi-role support (AP+STA, STA+STA)
#ifdef CFG_MROLE
  #define NX_MULTI_ROLE 1
#else
  #define NX_MULTI_ROLE 0
#endif

/// MFP support (for UMAC only)
#define CFG_MFP
#ifdef CFG_MFP
  #define NX_MFP 1
#else
  #define NX_MFP 0
#endif

#ifdef CFG_SAE
  #define NX_SAE 1// 1
  #define NX_SAE_PMK_CACHE 1
  #define NX_SAE_DEBUG 1
  #define NX_SAE_H2E 1
  #define NX_SAE_TRANS_DISABLE 0
  #if defined (ASR_REL) || defined (ALIOS_SUPPORT) || defined (JL_SDK)
  #else
  //#include "settings.h"
  #endif
#else
  #define NX_SAE 0
#endif

/// WAPI support
#ifdef CFG_WAPI
  #define RW_WAPI_EN 1
#else
  #define RW_WAPI_EN 0
#endif

/// WLAN coexistence support
#ifdef CFG_COEX
  #define RW_WLAN_COEX_EN 1
#else
  #define RW_WLAN_COEX_EN 0
#endif

/// Compilation for a HW supporting Key RAM configuration
#ifdef CFG_KEYCFG
  #define NX_KEY_RAM_CONFIG 1
#else
  #define NX_KEY_RAM_CONFIG 0
#endif

/// TDLS support
#ifdef CFG_TDLS
  #define TDLS_ENABLE 1
#else
  #define TDLS_ENABLE 0
#endif

#ifdef CFG_BWLEN
  /// per-BW length adaptation support
  #define NX_BW_LEN_ADAPT 1
  /// Number of steps for BW length adaptation
  #define NX_BW_LEN_STEPS 4
#else
  #define NX_BW_LEN_ADAPT 0
  #define NX_BW_LEN_STEPS 1
#endif

/// HSU support. Possible values are:
/// - 0: Don't use HSU, use software implementation.
/// - 1: Use HSU and fallback to software implementation if not available.
/// - 2: Only use HSU. (runtime error is generated if HSU is not available)
#ifdef CFG_HSU
  #define NX_HSU (CFG_HSU)
#else
  #define NX_HSU 0
#endif

/** @} Extra */

/**
 *******************************************************************************
 * @name Fw Features Configuration
 * Features automatically enabled if required by the selected configuration
 *******************************************************************************
 */
/// Maximum number of operating channel contexts
#define NX_CHAN_CTXT_CNT 3

/// General Purpose DMA module
#define NX_GP_DMA 1

/// Internal handling of received frame
#define NX_RX_FRAME_HANDLING 1

#ifdef APM_AGING_SUPPORT
/// Traffic Detection per STA support
#define NX_TD_STA  1  //(RW_BFMER_EN)
#else
/// Traffic Detection per STA support
#define NX_TD_STA  0  //(RW_BFMER_EN)
#endif
/// Traffic Detection support
#define NX_TD 1

/** @} Features */

/**
 *******************************************************************************
 * @name Misc
 *******************************************************************************
 */
/// This macro appears in some generated header files, define it to avoid warning
#define RW_NX_LDPC_DEC                  1

/// This macro appears in some generated header files, define it to avoid warning
#define RW_NX_AGC_SNR_EN                1

/// This macro appears in some generated header files, define it to avoid warning
#define RW_NX_IQ_COMP_EN                1

/// This macro appears in some generated header files, define it to avoid warning
#define RW_NX_FIQ_COMP_EN               1

/// This macro appears in some generated header files, define it to avoid warning
#define RW_NX_DERIV_80211B              1
/// @} misc

#ifdef WIFI_TEST_LINUX
#define HW_PSK 0  //0
#else
//if use c310 HW to accelerate psk calculation
//in yzs sdio solution, there is no c310 can be used in yzs side
#define HW_PSK 0  //1
#endif

#endif // _ASR_CONFIG_H_
/// @} MACSW


