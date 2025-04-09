#ifndef __UWIFI_TYPES_H__
#define __UWIFI_TYPES_H__
#include <stdint.h>
#include "uwifi_wlan_list.h"
#include "wifi_config.h"
#include "asr_types.h"
#include "asr_config.h"

#define __cpu_to_le32(x) ((uint32_t)(x))
#define __le32_to_cpu(x) ((uint32_t)(x))
#define __cpu_to_le16(x) ((uint16_t)(x))
#define __le16_to_cpu(x) ((uint16_t)(x))

#define cpu_to_le32 __cpu_to_le32
#define le32_to_cpu __le32_to_cpu
#define cpu_to_le16 __cpu_to_le16
#define le16_to_cpu __le16_to_cpu

#define ETH_ALEN     6

#define ETH_P_PAE    0x888E        /* Port Access Entity (IEEE 802.1X) */

/// SSID maximum length.
#define MAX_SSID_LEN 32
#define MAX_PASSWORD_LEN  64
#define MAX_KEY_LEN 32 // TKIP keys 256 bits (max length) with MIC keys
#define MAX_AP_NUM 32
#if NX_SAE
#define MAX_IE_LEN 64
#else
#define MAX_IE_LEN 32
#endif
#define MAX_RSNXE_LEN       6

#define CONFIG_USER_MAX     1
#define MAX_SAE_DATA_LEN    512

/*
 * Maximum number of payload addresses and lengths present in the descriptor
 */
#define NX_TX_PAYLOAD_MAX      6

/*
 * A-PMDU buffer sizes
 * According to IEEE802.11n spec size varies from 8K to 64K (in powers of 2)
 */
#define IEEE80211_MIN_AMPDU_BUF 0x8
#define IEEE80211_MAX_AMPDU_BUF 0x40

#if defined(CONFIG_ASR595X) || defined(AWORKS)                // wifi6 & wifi4 use same setting to share wpa lib.
#define PKT_BUF_RESERVE_HEAD    112 // 28 //96  //should be sizeof(struct asr_txhdr) + ASR_SWTXHDR_ALIGN_SZ; 28
#else
#define PKT_BUF_RESERVE_HEAD    64 // 28 //96  //should be sizeof(struct asr_txhdr) + ASR_SWTXHDR_ALIGN_SZ; 28
#endif

#define PKT_BUF_RESERVE_TAIL    64

#undef _TRUE
#define _TRUE        1

#undef _FALSE
#define _FALSE        0

#define LEGACY_PS_ID   0
#define UAPSD_ID       1

#define _SUCCESS    1
#define _FAIL       0


#define IPC_A2E_MSG_BUF_SIZE    127 // size in 4-byte words

/* Maximum number of rx buffer the fw may use at the same time */
#define ASR_RXBUFF_MAX  TOTAL_SKB_NUM //(64 * 10)

/**
 * Fullmac TXQ configuration:
 *  - STA: 1 TXQ per TID (limited to 8)
 *         1 txq for bufferable MGT frames
 *  - VIF: 1 tXQ for Multi/Broadcast +
 *         1 TXQ for MGT for unknown STAs or non-bufferable MGT frames
 *  - 1 TXQ for offchannel transmissions
 *
 *
 * Txq mapping looks like
 * for NX_REMOTE_STA_MAX=10 and NX_VIRT_DEV_MAX=4
 *
 * | TXQ | NDEV_ID | VIF |   STA |  TID | HWQ |
 * |-----+---------+-----+-------+------+-----|-
 * |   0 |       0 |     |     0 |    0 |   1 | 9 TXQ per STA
 * |   1 |       1 |     |     0 |    1 |   0 | (8 data + 1 mgmt)
 * |   2 |       2 |     |     0 |    2 |   0 |
 * |   3 |       3 |     |     0 |    3 |   1 |
 * |   4 |       4 |     |     0 |    4 |   2 |
 * |   5 |       5 |     |     0 |    5 |   2 |
 * |   6 |       6 |     |     0 |    6 |   3 |
 * |   7 |       7 |     |     0 |    7 |   3 |
 * |   8 |     N/A |     |     0 | MGMT |   3 |
 * |-----+---------+-----+-------+------+-----|-
 * | ... |         |     |       |      |     | Same for all STAs
 * |-----+---------+-----+-------+------+-----|-
 * |  90 |      80 |   0 | BC/MC |    0 | 1/4 | 1 TXQ for BC/MC per VIF
 * | ... |         |     |       |      |     |
 * |  93 |      80 |   3 | BC/MC |    0 | 1/4 |
 * |-----+---------+-----+-------+------+-----|-
 * |  94 |     N/A |   0 |   N/A | MGMT |   3 | 1 TXQ for unknown STA per VIF
 * | ... |         |     |       |      |     |
 * |  97 |     N/A |   3 |   N/A | MGMT |   3 |
 * |-----+---------+-----+-------+------+-----|-
 * |  98 |     N/A |     |   N/A | MGMT |   3 | 1 TXQ for offchannel frame
 */
#define NX_NB_TID_PER_STA 8
#define NX_NB_TXQ_PER_STA (NX_NB_TID_PER_STA + 1)
#define NX_NB_TXQ_PER_VIF 2
#define NX_NB_TXQ ((NX_NB_TXQ_PER_STA * NX_REMOTE_STA_MAX) +    \
                   (NX_NB_TXQ_PER_VIF * NX_VIRT_DEV_MAX) + 1)

/* A dma_addr_t can hold any valid DMA or bus address for the platform */
//typedef uint32_t dma_addr_t;

#endif //__UWIFI_TYPES_H__
