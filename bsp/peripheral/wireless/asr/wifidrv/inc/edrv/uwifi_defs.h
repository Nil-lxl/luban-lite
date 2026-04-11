/**
 ******************************************************************************
 *
 * @file asr_defs.h
 *
 * @brief Main driver structure declarations for fullmac driver
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */

#ifndef _ASR_DEFS_H_
#define _ASR_DEFS_H_

//#include "asr_mod_params.h"
#include "uwifi_tx.h"
#include "uwifi_rx.h"
//#include "uwif_utils.h"
//#include "uwifi_platform.h"
#include "uwifi_ipc_host.h"
#include "asr_wlan_api.h"
#include "asr_rtos_api.h"

#ifdef ALIOS_SUPPORT
// FIXME: rename atomic tp asr_atomic_t
#include "asm/atomic.h"
#endif


#define ASR_CH_NOT_SET 0xFF
#define ASR_TX_LIFETIME_MS             100

#define ASR_SCAN_CMD_TIMEOUT_MS     500

#define WPI_HDR_LEN    18
#define WPI_PN_LEN     16
#define WPI_PN_OFST     2
#define WPI_MIC_LEN    16
#define WPI_KEY_LEN    32
#define WPI_SUBKEY_LEN 16 // WPI key is actually two 16bytes key

#define LEGACY_PS_ID   0
#define UAPSD_ID       1

#define ASR_FLAG_TX_BIT 1
#define ASR_FLAG_RX_BIT 2
#define ASR_FLAG_RX_TX_BIT 3
#define ASR_FLAG_RX_TO_OS_BIT 4

#define ASR_FLAG_TX (1<<1)
#define ASR_FLAG_RX (1<<2)
#define ASR_FLAG_RX_TX (1<<3)
#define ASR_FLAG_RX_TO_OS (1<<4)
#define ASR_FLAG_TX_PROCESS (ASR_FLAG_TX|ASR_FLAG_RX_TX)

#define SDIO_REG_READ_LENGTH 64

#define SDIO_RX_AGG_TRI_CNT     6
#define SDIO_TX_AGG_TRI_CNT     7

#define MAC_RSNIE_CIPHER_WEP40    0x00
#define MAC_RSNIE_CIPHER_TKIP     0x01
#define MAC_RSNIE_CIPHER_CCMP     0x02
#define MAC_RSNIE_CIPHER_WEP104   0x03
#define MAC_RSNIE_CIPHER_SMS4     0x04
#define MAC_RSNIE_CIPHER_AES_CMAC 0x05

#define WPA_CIPHER_NONE     BIT(0)
#define WPA_CIPHER_WEP40    BIT(1)
#define WPA_CIPHER_WEP104   BIT(2)
#define WPA_CIPHER_TKIP     BIT(3)
#define WPA_CIPHER_CCMP     BIT(4)


#define ASR_SDIO_DATA_MAX_LEN TX_AGG_BUF_UNIT_SIZE

/** main task source */
enum {
    SDIO_THREAD,
    SDIO_ISR
};

enum {
    ASR_RESTART_REASON_CMD_CRASH = 0,
    ASR_RESTART_REASON_SCAN_FAIL = 1,
    ASR_RESTART_REASON_SDIO_ERR = 2,
    ASR_RESTART_REASON_TXMSG_FAIL = 3,

    ASR_RESTART_REASON_MAX,
};

/** Definitions for error constants. */
typedef enum {
    /** No error, everything OK. */
    ASR_ERR_OK         = 0,
    /** Out of memory error.     */
    ASR_ERR_MEM        = -1,
    /** Buffer error.            */
    ASR_ERR_BUF        = -2,
    /** Timeout.                 */
    ASR_ERR_TIMEOUT    = -3,
    /** Routing problem.         */
    ASR_ERR_RTE        = -4,
    /** Operation in progress    */
    ASR_ERR_INPROGRESS = -5,
    /** Illegal value.           */
    ASR_ERR_VAL        = -6,
    /** Operation would block.   */
    ASR_ERR_WOULDBLOCK = -7,
    /** Address in use.          */
    ASR_ERR_USE        = -8,
    /** Already connecting.      */
    ASR_ERR_ALREADY    = -9,
    /** Conn already established.*/
    ASR_ERR_ISCONN     = -10,
    /** Not connected.           */
    ASR_ERR_CONN       = -11,
    /** Low-level netif error    */
    ASR_ERR_IF         = -12,

    /** Connection aborted.      */
    ASR_ERR_ABRT       = -13,
    /** Connection reset.        */
    ASR_ERR_RST        = -14,
    /** Connection closed.       */
    ASR_ERR_CLSD       = -15,
    /** Illegal argument.        */
    ASR_ERR_ARG        = -16
} asr_netif_err;

enum
{
    WIFI_ENCRYP_OPEN,           //NONE
    WIFI_ENCRYP_WEP,           //WEP
    WIFI_ENCRYP_WPA,           //WPA
    WIFI_ENCRYP_WPA2,          //WPA2
    //WIFISUPP_ENCRYP_PROTOCOL_WAPI,     //WAPI: Not support in this version
    //WIFISUPP_ENCRYP_PROTOCOL_EAP,      //WAPI
    WIFI_ENCRYP_AUTO,          // used when alios did not tell us the encryption.
    WIFI_ENCRYP_MAX
};

/**
 * enum asr_ap_flags - AP flags
 *
 * @ASR_AP_ISOLATE Isolate clients (i.e. Don't brige packets transmitted by
 *                                   one client for another one)
 */
enum asr_ap_flags {
    ASR_AP_ISOLATE = BIT(0),
};

/**
 * enum nl80211_tx_power_setting - TX power adjustment
 * @NL80211_TX_POWER_AUTOMATIC: automatically determine transmit power
 * @NL80211_TX_POWER_LIMITED: limit TX power by the mBm parameter
 * @NL80211_TX_POWER_FIXED: fix TX power to the mBm parameter
 */
enum nl80211_tx_power_setting {
    NL80211_TX_POWER_AUTOMATIC,
    NL80211_TX_POWER_LIMITED,
    NL80211_TX_POWER_FIXED,
};

enum nl80211_auth_type {
    NL80211_AUTHTYPE_OPEN_SYSTEM,
    NL80211_AUTHTYPE_SHARED_KEY,
    NL80211_AUTHTYPE_FT,
    NL80211_AUTHTYPE_NETWORK_EAP,
    NL80211_AUTHTYPE_SAE,

    /* keep last */
    __NL80211_AUTHTYPE_NUM,
    NL80211_AUTHTYPE_MAX = __NL80211_AUTHTYPE_NUM - 1,
    NL80211_AUTHTYPE_AUTOMATIC
};

enum nl80211_mfp {
    NL80211_MFP_NO,
    NL80211_MFP_CAPABLE,
    NL80211_MFP_REQUIRED,
};

/**
 * enum ieee80211_band - supported frequency bands
 *
 * The bands are assigned this way because the supported
 * bitrates differ in these bands.
 *
 * @IEEE80211_BAND_2GHZ: 2.4GHz ISM band
 * @IEEE80211_BAND_5GHZ: around 5GHz band (4.9-5.7)
 * @IEEE80211_BAND_60GHZ: around 60 GHz band (58.32 - 64.80 GHz)
 * @IEEE80211_NUM_BANDS: number of defined bands
 */
enum ieee80211_band {
    IEEE80211_BAND_2GHZ,
    /* keep last */
    IEEE80211_NUM_BANDS
};

/**
 * enum nl80211_chan_width - channel width definitions
 *
 * These values are used with the %NL80211_ATTR_CHANNEL_WIDTH
 * attribute.
 *
 * @NL80211_CHAN_WIDTH_20_NOHT: 20 MHz, non-HT channel
 * @NL80211_CHAN_WIDTH_20: 20 MHz HT channel
 * @NL80211_CHAN_WIDTH_40: 40 MHz channel, the %NL80211_ATTR_CENTER_FREQ1
 *    attribute must be provided as well
 * @NL80211_CHAN_WIDTH_80: 80 MHz channel, the %NL80211_ATTR_CENTER_FREQ1
 *    attribute must be provided as well
 * @NL80211_CHAN_WIDTH_80P80: 80+80 MHz channel, the %NL80211_ATTR_CENTER_FREQ1
 *    and %NL80211_ATTR_CENTER_FREQ2 attributes must be provided as well
 * @NL80211_CHAN_WIDTH_160: 160 MHz channel, the %NL80211_ATTR_CENTER_FREQ1
 *    attribute must be provided as well
 * @NL80211_CHAN_WIDTH_5: 5 MHz OFDM channel
 * @NL80211_CHAN_WIDTH_10: 10 MHz OFDM channel
 */
enum nl80211_chan_width {
    NL80211_CHAN_WIDTH_20_NOHT,
    NL80211_CHAN_WIDTH_20,
    NL80211_CHAN_WIDTH_40,
    NL80211_CHAN_WIDTH_80,
    NL80211_CHAN_WIDTH_80P80,
    NL80211_CHAN_WIDTH_160,
    NL80211_CHAN_WIDTH_5,
    NL80211_CHAN_WIDTH_10,
};

/**
 * enum nl80211_hidden_ssid - values for %NL80211_ATTR_HIDDEN_SSID
 * @NL80211_HIDDEN_SSID_NOT_IN_USE: do not hide SSID (i.e., broadcast it in
 *    Beacon frames)
 * @NL80211_HIDDEN_SSID_ZERO_LEN: hide SSID by using zero-length SSID element
 *    in Beacon frames
 * @NL80211_HIDDEN_SSID_ZERO_CONTENTS: hide SSID by using correct length of SSID
 *    element in Beacon frames but zero out each byte in the SSID
 */
enum nl80211_hidden_ssid {
    NL80211_HIDDEN_SSID_NOT_IN_USE,
    NL80211_HIDDEN_SSID_ZERO_LEN,
    NL80211_HIDDEN_SSID_ZERO_CONTENTS
};

/**
 * enum nl80211_smps_mode - SMPS mode
 *
 * Requested SMPS mode (for AP mode)
 *
 * @NL80211_SMPS_OFF: SMPS off (use all antennas).
 * @NL80211_SMPS_STATIC: static SMPS (use a single antenna)
 * @NL80211_SMPS_DYNAMIC: dynamic smps (start with a single antenna and
 *    turn on other antennas after CTS/RTS).
 */
enum nl80211_smps_mode {
    NL80211_SMPS_OFF,
    NL80211_SMPS_STATIC,
    NL80211_SMPS_DYNAMIC,

    __NL80211_SMPS_AFTER_LAST,
    NL80211_SMPS_MAX = __NL80211_SMPS_AFTER_LAST - 1
};

/**
 * enum nl80211_bss_scan_width - control channel width for a BSS
 *
 * These values are used with the %NL80211_BSS_CHAN_WIDTH attribute.
 *
 * @NL80211_BSS_CHAN_WIDTH_20: control channel is 20 MHz wide or compatible
 * @NL80211_BSS_CHAN_WIDTH_10: control channel is 10 MHz wide
 * @NL80211_BSS_CHAN_WIDTH_5: control channel is 5 MHz wide
 */
enum nl80211_bss_scan_width {
    NL80211_BSS_CHAN_WIDTH_20,
    NL80211_BSS_CHAN_WIDTH_10,
    NL80211_BSS_CHAN_WIDTH_5,
};

/**
 * enum nl80211_band - Frequency band
 * @NL80211_BAND_2GHZ: 2.4 GHz ISM band
 * @NL80211_BAND_5GHZ: around 5 GHz band (4.9 - 5.7 GHz)
 * @NL80211_BAND_60GHZ: around 60 GHz band (58.32 - 64.80 GHz)
 */
enum nl80211_band {
    NL80211_BAND_2GHZ,
    NL80211_BAND_5GHZ,
    NL80211_BAND_60GHZ,
};

/**
 * enum nl80211_acl_policy - access control policy
 *
 * Access control policy is applied on a MAC list set by
 * %NL80211_CMD_START_AP and %NL80211_CMD_SET_MAC_ACL, to
 * be used with %NL80211_ATTR_ACL_POLICY.
 *
 * @NL80211_ACL_POLICY_ACCEPT_UNLESS_LISTED: Deny stations which are
 *    listed in ACL, i.e. allow all the stations which are not listed
 *    in ACL to authenticate.
 * @NL80211_ACL_POLICY_DENY_UNLESS_LISTED: Allow the stations which are listed
 *    in ACL, i.e. deny all the stations which are not listed in ACL.
 */
enum nl80211_acl_policy {
    NL80211_ACL_POLICY_ACCEPT_UNLESS_LISTED,
    NL80211_ACL_POLICY_DENY_UNLESS_LISTED,
};

/**
 * enum nl80211_txq_attr - TX queue parameter attributes
 * @__NL80211_TXQ_ATTR_INVALID: Attribute number 0 is reserved
 * @NL80211_TXQ_ATTR_AC: AC identifier (NL80211_AC_*)
 * @NL80211_TXQ_ATTR_TXOP: Maximum burst time in units of 32 usecs, 0 meaning
 *    disabled
 * @NL80211_TXQ_ATTR_CWMIN: Minimum contention window [a value of the form
 *    2^n-1 in the range 1..32767]
 * @NL80211_TXQ_ATTR_CWMAX: Maximum contention window [a value of the form
 *    2^n-1 in the range 1..32767]
 * @NL80211_TXQ_ATTR_AIFS: Arbitration interframe space [0..255]
 * @__NL80211_TXQ_ATTR_AFTER_LAST: Internal
 * @NL80211_TXQ_ATTR_MAX: Maximum TXQ attribute number
 */
enum nl80211_txq_attr {
    __NL80211_TXQ_ATTR_INVALID,
    NL80211_TXQ_ATTR_AC,
    NL80211_TXQ_ATTR_TXOP,
    NL80211_TXQ_ATTR_CWMIN,
    NL80211_TXQ_ATTR_CWMAX,
    NL80211_TXQ_ATTR_AIFS,

    /* keep last */
    __NL80211_TXQ_ATTR_AFTER_LAST,
    NL80211_TXQ_ATTR_MAX = __NL80211_TXQ_ATTR_AFTER_LAST - 1
};

enum nl80211_ac {
    NL80211_AC_VO,
    NL80211_AC_VI,
    NL80211_AC_BE,
    NL80211_AC_BK,
    NL80211_NUM_ACS
};

/* backward compat */
#define NL80211_TXQ_ATTR_QUEUE    NL80211_TXQ_ATTR_AC
#define NL80211_TXQ_Q_VO    NL80211_AC_VO
#define NL80211_TXQ_Q_VI    NL80211_AC_VI
#define NL80211_TXQ_Q_BE    NL80211_AC_BE
#define NL80211_TXQ_Q_BK    NL80211_AC_BK


struct asr_ip_hdr {
  /* version / header length */
  uint8_t _v_hl;
  /* type of service */
  uint8_t _tos;
  /* total length */
  uint16_t _len;
  /* identification */
  uint16_t _id;
  /* fragment offset field */
  uint16_t _offset;
  /* time to live */
  uint8_t _ttl;
  /* protocol*/
  uint8_t _proto;
  /* checksum */
  uint16_t _chksum;
  /* source and destination IP addresses */
  uint32_t src;
  uint32_t dest;
};

struct asr_udp_hdr {
  uint16_t src;
  uint16_t dest;  /* src/dest UDP ports */
  uint16_t len;
  uint16_t chksum;
};

struct asr_tcp_hdr {
  uint16_t src;
  uint16_t dest;  /* src/dest UDP ports */
  uint32_t seqnum;
  uint32_t acknum;
  uint16_t flags;
  uint16_t window;
  uint16_t checksum;
  uint16_t urgent;
};

struct asr_tcp_ack{
    uint32_t time;
    uint32_t acknum;
    uint16_t src;
    uint16_t dest;
    uint8_t h_dest[ETH_ALEN];
};

#define ASR_TCP_DEBUG_NUM   5000
struct asr_tcp_debug{
    uint32_t time;
    uint32_t seqnum;
    uint32_t acknum;
    uint16_t wifiseq;
    uint16_t framlen;
};

/**
 * struct asr_rx_rate_stats - Store statistics for RX rates
 *
 * @table: Table indicating how many frame has been receive which each
 * rate index. Rate index is the same as the one used by RC algo for TX
 * @size: Size of the table array
 * @cpt: number of frames received
 */
struct asr_rx_rate_stats {
    int *table;
    int size;
    int cpt;
};

/**
 * struct asr_sta_stats - Structure Used to store statistics specific to a STA
 *
 * @last_rx: Hardware vector of the last received frame
 * @rx_rate: Statistics of the received rates
 */
struct asr_sta_stats {
#ifdef CONFIG_ASR_DEBUGFS
    struct hw_vect last_rx;
    struct asr_rx_rate_stats rx_rate;
#endif
};


struct asr_sec_phy_chan {
    uint16_t prim20_freq;
    uint16_t center_freq1;
    uint16_t center_freq2;
    enum ieee80211_band band;
    uint8_t type;
};

#define ASR_CH_NOT_SET 0xFF

struct asr_tx_agg {
    struct sk_buff *aggr_buf;
    uint16_t last_aggr_buf_next_idx;
    uint16_t cur_aggr_buf_next_idx;
    uint8_t *last_aggr_buf_next_addr;
    uint8_t *cur_aggr_buf_next_addr;
    uint16_t aggr_buf_cnt;
};


enum wiphy_flags {
    /* use hole at 0 */
    /* use hole at 1 */
    /* use hole at 2 */
    WIPHY_FLAG_NETNS_OK            = BIT(3),
    WIPHY_FLAG_PS_ON_BY_DEFAULT        = BIT(4),
    WIPHY_FLAG_4ADDR_AP            = BIT(5),
    WIPHY_FLAG_4ADDR_STATION        = BIT(6),
    WIPHY_FLAG_CONTROL_PORT_PROTOCOL    = BIT(7),
    WIPHY_FLAG_IBSS_RSN            = BIT(8),
    WIPHY_FLAG_MESH_AUTH            = BIT(10),
    WIPHY_FLAG_SUPPORTS_SCHED_SCAN        = BIT(11),
    /* use hole at 12 */
    WIPHY_FLAG_SUPPORTS_FW_ROAM        = BIT(13),
    WIPHY_FLAG_AP_UAPSD            = BIT(14),
    WIPHY_FLAG_SUPPORTS_TDLS        = BIT(15),
    WIPHY_FLAG_TDLS_EXTERNAL_SETUP        = BIT(16),
    WIPHY_FLAG_HAVE_AP_SME            = BIT(17),
    WIPHY_FLAG_REPORTS_OBSS            = BIT(18),
    WIPHY_FLAG_AP_PROBE_RESP_OFFLOAD    = BIT(19),
    WIPHY_FLAG_OFFCHAN_TX            = BIT(20),
    WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL    = BIT(21),
    WIPHY_FLAG_SUPPORTS_5_10_MHZ        = BIT(22),
    WIPHY_FLAG_HAS_CHANNEL_SWITCH        = BIT(23),
};

#if 1//def LWIP
enum lwip_dhcp_event{
    DHCP_CLIENT_UP,
    DHCP_CLIENT_DOWN,
    DHCP_SERVER_UP,
    DHCP_SERVER_DOWN,
    DHCP_EVENT_MAX
};
#endif

struct net_device_stats {
    unsigned long   rx_packets;
    unsigned long   tx_packets;
    unsigned long   rx_bytes;
    unsigned long   tx_bytes;
    unsigned long   rx_errors;
    unsigned long   tx_errors;
    unsigned long   rx_dropped;
    unsigned long   tx_dropped;
    unsigned long   multicast;
    unsigned long   ollisions;
    unsigned long   rx_length_errors;
    unsigned long   rx_over_errors;
    unsigned long   rx_crc_errors;
    unsigned long   rx_frame_errors;
    unsigned long   rx_fifo_errors;
    unsigned long   rx_missed_errors;
    unsigned long   tx_aborted_errors;
    unsigned long   tx_carrier_errors;
    unsigned long   tx_fifo_errors;
    unsigned long   tx_heartbeat_errors;
    unsigned long   tx_window_errors;
    unsigned long   rx_compressed;
    unsigned long   tx_compressed;
};

struct asr_vif;

struct net_device_ops {
    int         (*ndo_open)(struct asr_hw *asr_hw, struct asr_vif *asr_vif);
    int         (*ndo_stop)(struct asr_hw *asr_hw, struct asr_vif *asr_vif);
    int         (*ndo_start_xmit) (struct asr_vif *asr_vif, struct sk_buff *skb);
    //struct net_device_stats* (*ndo_get_stats)(struct asr_vif *asr_vif);
    uint16_t    (*ndo_select_queue)(struct asr_vif *asr_vif, struct sk_buff *skb);
};

/**
 * struct cfg80211_ft_event - FT Information Elements
 * @ies: FT IEs
 * @ies_len: length of the FT IE in bytes
 * @target_ap: target AP's MAC address
 * @ric_ies: RIC IE
 * @ric_ies_len: length of the RIC IE in bytes
 */
struct cfg80211_ft_event_params {
    const uint8_t *ies;
    size_t ies_len;
    const uint8_t *target_ap;
    const uint8_t *ric_ies;
    size_t ric_ies_len;
};

#define IEEE80211_HT_MCS_MASK_LEN        10

/**
 * struct ieee80211_mcs_info - MCS information
 * @rx_mask: RX mask
 * @rx_highest: highest supported RX rate. If set represents
 *    the highest supported RX data rate in units of 1 Mbps.
 *    If this field is 0 this value should not be used to
 *    consider the highest RX data rate supported.
 * @tx_params: TX parameters
 */
struct ieee80211_mcs_info {
    uint8_t rx_mask[IEEE80211_HT_MCS_MASK_LEN];
    uint16_t rx_highest;
    uint8_t tx_params;
    uint8_t reserved[3];
} __attribute__ ((__packed__));

/**
 * struct ieee80211_ht_cap - HT capabilities
 *
 * This structure is the "HT capabilities element" as
 * described in 802.11n D5.0 7.3.2.57
 */
struct ieee80211_ht_cap {
    uint16_t cap_info;
    uint8_t ampdu_params_info;

    /* 16 bytes MCS information */
    struct ieee80211_mcs_info mcs;

    uint16_t extended_ht_cap_info;
    uint32_t tx_BF_cap_info;
    uint8_t antenna_selection_info;
} __attribute__ ((__packed__));

/**
 * struct ieee80211_sta_ht_cap - STA's HT capabilities
 *
 * This structure describes most essential parameters needed
 * to describe 802.11n HT capabilities for an STA.
 *
 * @ht_supported: is HT supported by the STA
 * @cap: HT capabilities map as described in 802.11n spec
 * @ampdu_factor: Maximum A-MPDU length factor
 * @ampdu_density: Minimum A-MPDU spacing
 * @mcs: Supported MCS rates
 */
struct ieee80211_sta_ht_cap {
    uint16_t cap; /* use IEEE80211_HT_CAP_ */
    bool ht_supported;
    uint8_t ampdu_factor;
    uint8_t ampdu_density;
    struct ieee80211_mcs_info mcs;
};

/**
 *  ieee80211 need replace by wpa
**/
struct ieee80211_channel {
    enum ieee80211_band band;
    uint16_t center_freq;
    //uint16_t hw_value;
    uint32_t flags;
    //int max_antenna_gain;
    int max_power;
    int max_reg_power;
    //bool beacon_found;
    //uint32_t orig_flags;
    //int orig_mag, orig_mpwr;
};

/**
 * struct cfg80211_mgmt_tx_params - mgmt tx parameters
 *
 * This structure provides information needed to transmit a mgmt frame
 *
 * @chan: channel to use
 * @offchan: indicates wether off channel operation is required
 * @wait: duration for ROC
 * @buf: buffer to transmit
 * @len: buffer length
 * @no_cck: don't use cck rates for this frame
 * @dont_wait_for_ack: tells the low level not to wait for an ack
 * @n_csa_offsets: length of csa_offsets array
 * @csa_offsets: array of all the csa offsets in the frame
 */
struct cfg80211_mgmt_tx_params {
    struct ieee80211_channel *chan;
    bool offchan;
    unsigned int wait;
    const uint8_t *buf;
    size_t len;
    bool no_cck;
    bool dont_wait_for_ack;
    int n_csa_offsets;
    const uint16_t *csa_offsets;
#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
    uint8_t cus_flags;
#endif
};

#define NL80211_MAX_NR_CIPHER_SUITES        1
#define NL80211_MAX_NR_AKM_SUITES        2

struct cfg80211_crypto_settings {
    //uint32_t wpa_versions;
    uint32_t cipher_group;
    int n_ciphers_pairwise;
    uint32_t ciphers_pairwise[NL80211_MAX_NR_CIPHER_SUITES];
    //int n_akm_suites;
    //uint32_t akm_suites[NL80211_MAX_NR_AKM_SUITES];
    bool control_port;
    uint16_t control_port_ethertype;
    bool control_port_no_encrypt;
#if (defined(CONFIG_IEEE80211W) || NX_SAE)
    uint32_t cipher_mgmt_group;
    uint32_t akm_suites[NL80211_MAX_NR_AKM_SUITES];
    enum nl80211_mfp mfp_option;
#endif
};

//add by
struct cfg80211_connect_params {
    struct ieee80211_channel *channel;
    uint8_t bssid[ETH_ALEN];
    uint8_t ssid[MAX_SSID_LEN];
    uint16_t ssid_len;
    enum nl80211_auth_type auth_type;
    uint16_t ie[MAX_IE_LEN];               // wpa 24,wpa2 22, wep?    try 100
    uint16_t ie_len;
    enum nl80211_mfp mfp;
    struct cfg80211_crypto_settings crypto;
    uint8_t key[MAX_KEY_LEN];
    uint8_t key_len, key_idx;
#if NX_SAE
    uint8_t password[MAX_PASSWORD_LEN];
    uint8_t pwd_len;
#endif // if 0
};

#ifdef CONFIG_SME
struct cfg80211_auth_request {
    struct ieee80211_channel channel;
    uint8_t bssid[ETH_ALEN];
    uint8_t ssid[MAX_SSID_LEN];
    uint16_t ssid_len;
    enum nl80211_auth_type auth_type;
    uint16_t ie[MAX_IE_LEN];               // wpa 24,wpa2 22, wep?      try 100
    uint16_t ie_len;
    enum nl80211_mfp mfp;
    struct cfg80211_crypto_settings crypto;
    uint8_t key[MAX_KEY_LEN];
    uint8_t key_len, key_idx;
#if NX_SAE
    uint8_t password[MAX_PASSWORD_LEN];
    uint8_t pwd_len;
#endif
    const u8 sae_data[MAX_SAE_DATA_LEN];
    uint16_t sae_data_len;
};

struct cfg80211_assoc_request {
    uint8_t bssid[ETH_ALEN];
    uint16_t ie[MAX_IE_LEN];               // wpa 24,wpa2 22, wep?    try 100
    uint16_t ie_len;
    struct cfg80211_crypto_settings crypto;
    bool use_mfp;
};

struct cfg80211_deauth_request {
    uint16_t reason_code;
};
#endif

struct wiphy {
    struct ieee80211_supported_band *bands[IEEE80211_NUM_BANDS];
    char fw_version[32];
};


/**
 * struct ieee80211_rate - bitrate definition
 *
 * This structure describes a bitrate that an 802.11 PHY can
 * operate with. The two values @hw_value and @hw_value_short
 * are only for driver use when pointers to this structure are
 * passed around.
 *
 * @flags: rate-specific flags
 * @bitrate: bitrate in units of 100 Kbps
 * @hw_value: driver/hardware value for this rate
 * @hw_value_short: driver/hardware value for this rate when
 *    short preamble is used
 */
struct ieee80211_rate {
    uint32_t flags;
    uint16_t bitrate;
    uint16_t hw_value, hw_value_short;
};

#define IEEE80211_HE_PPE_THRES_MAX_LEN        25

struct ieee80211_he_cap{
    struct ieee80211_he_cap_elem he_cap_elem;
    struct ieee80211_he_mcs_nss_supp he_mcs_nss_supp;
    uint8_t ppe_thres[IEEE80211_HE_PPE_THRES_MAX_LEN];
} __packed;
/**
 * struct ieee80211_sta_he_cap - STA's HE capabilities
 *
 * This structure describes most essential parameters needed
 * to describe 802.11ax HE capabilities for a STA.
 *
 * @has_he: true iff HE data is valid.
 * @he_cap_elem: Fixed portion of the HE capabilities element.
 * @he_mcs_nss_supp: The supported NSS/MCS combinations.
 * @ppe_thres: Holds the PPE Thresholds data.
 */
struct ieee80211_sta_he_cap {
    bool has_he;
    struct ieee80211_he_cap_elem he_cap_elem;
    struct ieee80211_he_mcs_nss_supp he_mcs_nss_supp;
    u8 ppe_thres[IEEE80211_HE_PPE_THRES_MAX_LEN];
};

/**
 * struct ieee80211_sband_iftype_data
 *
 * This structure encapsulates sband data that is relevant for the
 * interface types defined in @types_mask.  Each type in the
 * @types_mask must be unique across all instances of iftype_data.
 *
 * @types_mask: interface types mask
 * @he_cap: holds the HE capabilities
 */
struct ieee80211_sband_iftype_data {
    u16 types_mask;
    struct ieee80211_sta_he_cap he_cap;
};

struct ieee80211_supported_band {
    const struct ieee80211_channel *channels;
    const struct ieee80211_rate *bitrates;
    enum ieee80211_band band;
    int n_channels;
    int n_bitrates;
    struct ieee80211_sta_ht_cap ht_cap;
    u16 n_iftype_data;
    const struct ieee80211_sband_iftype_data *iftype_data;
};

struct ap_ssid
{
    uint8_t ssid[MAX_SSID_LEN+1];
    uint8_t ssid_len;
};

struct scan_result
{
    struct scan_result              *next_ptr;
    struct ap_ssid                  ssid;
    uint8_t                         bssid[ETH_ALEN];
    uint16_t                        chn;                 //the channel using
    struct ieee80211_channel        channel;
    int8_t                          rssi;
    uint8_t                         encryp_protocol;    //the protocol used by encryption
    struct cfg80211_crypto_settings crypto;
#ifdef CONFIG_IEEE80211W
    enum nl80211_mfp                mfp;
#endif
    //uint16_t                        rate;               //the rate
    uint8_t                         ie[MAX_IE_LEN];
    uint16_t                        ie_len;
    uint8_t                         rsnxe[MAX_RSNXE_LEN];
    uint16_t                        rsnxe_len;
    uint8_t                         sae_h2e;

    //uint16_t                      network_mode;       //network mode
    //uint16_t                      noise;              //SNR: signal noise ratio
    //uint16_t                      beacon_interval;    // the BEACON interval
    //WIFISUPP_CREDENTIAL_TYPE_E    credential_type;    //AKM type
};  //the AP info scanned

struct cfg80211_ssid {
    uint8_t ssid[IEEE80211_MAX_SSID_LEN];
    uint8_t ssid_len;
};

//add by
struct cfg80211_scan_request {
    struct cfg80211_ssid *ssids;
    int n_ssids;
    uint32_t n_channels;
    uint32_t flags;
    bool no_cck;
    const uint8_t *ie;
    uint16_t ie_len;
    uint8_t  bssid[MAC_ADDR_LEN];
    /* keep last */
    struct ieee80211_channel *channels[0];
#if 0
    enum nl80211_bss_scan_width scan_width;
    uint32_t rates[IEEE80211_NUM_BANDS];

    uint8_t mac_addr[ETH_ALEN] __aligned(2);
    uint8_t mac_addr_mask[ETH_ALEN] __aligned(2);

    /* internal */
    unsigned long scan_start;
    bool aborted, notified;
#endif // if 0
};

/**
 * struct cfg80211_beacon_data - beacon data
 * @head: head portion of beacon (before TIM IE)
 *    or %NULL if not changed
 * @tail: tail portion of beacon (after TIM IE)
 *    or %NULL if not changed
 * @head_len: length of @head
 * @tail_len: length of @tail
 * @beacon_ies: extra information element(s) to add into Beacon frames or %NULL
 * @beacon_ies_len: length of beacon_ies in octets
 * @proberesp_ies: extra information element(s) to add into Probe Response
 *    frames or %NULL
 * @proberesp_ies_len: length of proberesp_ies in octets
 * @assocresp_ies: extra information element(s) to add into (Re)Association
 *    Response frames or %NULL
 * @assocresp_ies_len: length of assocresp_ies in octets
 * @probe_resp_len: length of probe response template (@probe_resp)
 * @probe_resp: probe response template (AP mode only)
 */
struct cfg80211_beacon_data {
    const uint8_t *head, *tail;
    const uint8_t *beacon_ies;
    const uint8_t *proberesp_ies;
    const uint8_t *assocresp_ies;
    const uint8_t *probe_resp;

    size_t head_len, tail_len;
    size_t beacon_ies_len;
    size_t proberesp_ies_len;
    size_t assocresp_ies_len;
    size_t probe_resp_len;
};

struct cfg80211_chan_def {
    struct ieee80211_channel *chan;
    enum nl80211_chan_width width;
    uint32_t center_freq1;
    uint32_t center_freq2;
};

/**
 * struct cfg80211_ap_settings - AP configuration
 *
 * Used to configure an AP interface.
 *
 * @chandef: defines the channel to use
 * @beacon: beacon data
 * @beacon_interval: beacon interval
 * @dtim_period: DTIM period
 * @ssid: SSID to be used in the BSS (note: may be %NULL if not provided from
 *    user space)
 * @ssid_len: length of @ssid
 * @hidden_ssid: whether to hide the SSID in Beacon/Probe Response frames
 * @crypto: crypto settings
 * @privacy: the BSS uses privacy
 * @auth_type: Authentication type (algorithm)
 * @smps_mode: SMPS mode
 * @inactivity_timeout: time in seconds to determine station's inactivity.
 * @p2p_ctwindow: P2P CT Window
 * @p2p_opp_ps: P2P opportunistic PS
 * @acl: ACL configuration used by the drivers which has support for
 *    MAC address based access control
 */
struct cfg80211_ap_settings {
    struct cfg80211_chan_def chandef;

    struct cfg80211_beacon_data beacon;

    int beacon_interval, dtim_period;
    const uint8_t *ssid;
    size_t ssid_len;
    enum nl80211_hidden_ssid hidden_ssid;
    struct cfg80211_crypto_settings crypto;
    bool privacy;
    enum nl80211_auth_type auth_type;
    enum nl80211_smps_mode smps_mode;
    int inactivity_timeout;
    uint8_t p2p_ctwindow;
    bool p2p_opp_ps;
    const struct cfg80211_acl_data *acl;
};

struct mac_address {
    uint8_t addr[ETH_ALEN];
};

/**
 * struct cfg80211_acl_data - Access control list data
 *
 * @acl_policy: ACL policy to be applied on the station's
 *    entry specified by mac_addr
 * @n_acl_entries: Number of MAC address entries passed
 * @mac_addrs: List of MAC addresses of stations to be used for ACL
 */
struct cfg80211_acl_data {
    enum nl80211_acl_policy acl_policy;
    int n_acl_entries;

    /* Keep it last */
    struct mac_address mac_addrs[];
};

/**
 * struct cfg80211_csa_settings - channel switch settings
 *
 * Used for channel switch
 *
 * @chandef: defines the channel to use after the switch
 * @beacon_csa: beacon data while performing the switch
 * @counter_offsets_beacon: offsets of the counters within the beacon (tail)
 * @counter_offsets_presp: offsets of the counters within the probe response
 * @n_counter_offsets_beacon: number of csa counters the beacon (tail)
 * @n_counter_offsets_presp: number of csa counters in the probe response
 * @beacon_after: beacon data to be used on the new channel
 * @radar_required: whether radar detection is required on the new channel
 * @block_tx: whether transmissions should be blocked while changing
 * @count: number of beacons until switch
 */
struct cfg80211_csa_settings {
    struct cfg80211_chan_def chandef;
    struct cfg80211_beacon_data beacon_csa;
    const uint16_t *counter_offsets_beacon;
    const uint16_t *counter_offsets_presp;
    unsigned int n_counter_offsets_beacon;
    unsigned int n_counter_offsets_presp;
    struct cfg80211_beacon_data beacon_after;
    bool radar_required;
    bool block_tx;
    uint8_t count;
};


/**
 * struct ieee80211_txq_params - TX queue parameters
 * @ac: AC identifier
 * @txop: Maximum burst time in units of 32 usecs, 0 meaning disabled
 * @cwmin: Minimum contention window [a value of the form 2^n-1 in the range
 *    1..32767]
 * @cwmax: Maximum contention window [a value of the form 2^n-1 in the range
 *    1..32767]
 * @aifs: Arbitration interframe space [0..255]
 */
struct ieee80211_txq_params {
    enum nl80211_ac ac;
    uint16_t txop;
    uint16_t cwmin;
    uint16_t cwmax;
    uint8_t aifs;
};

/**
 * struct survey_info - channel survey response
 *
 * @channel: the channel this survey record reports, mandatory
 * @filled: bitflag of flags from &enum survey_info_flags
 * @noise: channel noise in dBm. This and all following fields are
 *    optional
 * @channel_time: amount of time in ms the radio spent on the channel
 * @channel_time_busy: amount of time the primary channel was sensed busy
 * @channel_time_ext_busy: amount of time the extension channel was sensed busy
 * @channel_time_rx: amount of time the radio spent receiving data
 * @channel_time_tx: amount of time the radio spent transmitting data
 *
 * Used by dump_survey() to report back per-channel survey information.
 *
 * This structure can later be expanded with things like
 * channel duty cycle etc.
 */
struct survey_info {
    struct ieee80211_channel *channel;
    uint64_t channel_time;
    uint64_t channel_time_busy;
    uint64_t channel_time_ext_busy;
    uint64_t channel_time_rx;
    uint64_t channel_time_tx;
    uint32_t filled;
    int8_t noise;
};

/**
 * struct bss_parameters - BSS parameters
 *
 * Used to change BSS parameters (mainly for AP mode).
 *
 * @use_cts_prot: Whether to use CTS protection
 *    (0 = no, 1 = yes, -1 = do not change)
 * @use_short_preamble: Whether the use of short preambles is allowed
 *    (0 = no, 1 = yes, -1 = do not change)
 * @use_short_slot_time: Whether the use of short slot time is allowed
 *    (0 = no, 1 = yes, -1 = do not change)
 * @basic_rates: basic rates in IEEE 802.11 format
 *    (or NULL for no change)
 * @basic_rates_len: number of basic rates
 * @ap_isolate: do not forward packets between connected stations
 * @ht_opmode: HT Operation mode
 *     (uint16_t = opmode, -1 = do not change)
 * @p2p_ctwindow: P2P CT Window (-1 = no change)
 * @p2p_opp_ps: P2P opportunistic PS (-1 = no change)
 */
struct bss_parameters {
    int use_cts_prot;
    int use_short_preamble;
    int use_short_slot_time;
    const uint8_t *basic_rates;
    uint8_t basic_rates_len;
    int ap_isolate;
    int ht_opmode;
    int8_t p2p_ctwindow, p2p_opp_ps;
};

/**
 * struct key_params - key information
 *
 * Information about a key
 *
 * @key: key material
 * @key_len: length of key material
 * @cipher: cipher suite selector
 * @seq: sequence counter (IV/PN) for TKIP and CCMP keys, only used
 *    with the get_key() callback, must be in little endian,
 *    length given by @seq_len.
 * @seq_len: length of @seq.
 */
struct key_params {
    uint8_t *key;
    uint8_t *seq;
    int key_len;
    int seq_len;
    uint32_t cipher;
};

/**
 * struct station_parameters - station parameters
 *
 * Used to change and create a new station.
 *
 * @vlan: vlan interface station should belong to
 * @supported_rates: supported rates in IEEE 802.11 format
 *    (or NULL for no change)
 * @supported_rates_len: number of supported rates
 * @sta_flags_mask: station flags that changed
 *    (bitmask of BIT(NL80211_STA_FLAG_...))
 * @sta_flags_set: station flags values
 *    (bitmask of BIT(NL80211_STA_FLAG_...))
 * @listen_interval: listen interval or -1 for no change
 * @aid: AID or zero for no change
 * @plink_action: plink action to take
 * @plink_state: set the peer link state for a station
 * @ht_capa: HT capabilities of station
 * @vht_capa: VHT capabilities of station
 * @uapsd_queues: bitmap of queues configured for uapsd. same format
 *    as the AC bitmap in the QoS info field
 * @max_sp: max Service Period. same format as the MAX_SP in the
 *    QoS info field (but already shifted down)
 * @sta_modify_mask: bitmap indicating which parameters changed
 *    (for those that don't have a natural "no change" value),
 *    see &enum station_parameters_apply_mask
 * @local_pm: local link-specific mesh power save mode (no change when set
 *    to unknown)
 * @capability: station capability
 * @ext_capab: extended capabilities of the station
 * @ext_capab_len: number of extended capabilities
 * @supported_channels: supported channels in IEEE 802.11 format
 * @supported_channels_len: number of supported channels
 * @supported_oper_classes: supported oper classes in IEEE 802.11 format
 * @supported_oper_classes_len: number of supported operating classes
 * @opmode_notif: operating mode field from Operating Mode Notification
 * @opmode_notif_used: information if operating mode field is used
 */
struct station_parameters {
    const uint8_t *supported_rates;
    uint32_t sta_flags_mask, sta_flags_set;
    uint32_t sta_modify_mask;
    int listen_interval;
    uint16_t aid;
    uint8_t supported_rates_len;
    uint8_t plink_action;
    uint8_t plink_state;
    const struct ieee80211_ht_cap *ht_capa;
    const struct ieee80211_he_cap *he_capa;
    uint8_t uapsd_queues;
    uint8_t max_sp;
    uint16_t capability;
    const uint8_t *ext_capab;
    uint8_t ext_capab_len;
    const uint8_t *supported_channels;
    uint8_t supported_channels_len;
    const uint8_t *supported_oper_classes;
    uint8_t supported_oper_classes_len;
    uint8_t opmode_notif;
    bool opmode_notif_used;
};

/**
 * struct station_del_parameters - station deletion parameters
 *
 * Used to delete a station entry (or all stations).
 *
 * @mac: MAC address of the station to remove or NULL to remove all stations
 * @subtype: Management frame subtype to use for indicating removal
 *    (10 = Disassociation, 12 = Deauthentication)
 * @reason_code: Reason code for the Disassociation/Deauthentication frame
 */
struct station_del_parameters {
    const uint8_t *mac;
    uint8_t subtype;
    uint16_t reason_code;
};

struct asr_mod_params {
    bool ht_on;
    int mcs_map;
    bool ldpc_on;
    int phy_cfg;
    int uapsd_timeout;
    bool ap_uapsd_on;
    bool sgi;
    bool sgi80;
    bool use_2040;
    bool use_80;
    bool custregd;
    int nss;
    bool murx;
    bool mutx;
    bool mutx_on;
    unsigned int roc_dur_max;
    int listen_itv;
    bool listen_bcmc;
    int lp_clk_ppm;
    bool ps_on;
    int tx_lft;
    int amsdu_maxnb;
    int uapsd_queues;
#ifdef CONFIG_ASR595X
    bool he_on;
    int he_mcs_map;
    bool he_ul_on;
    bool stbc_on;
    bool bfmee;
    bool twt_request;
#endif

};

#define COMMON_PARAM(name, default_fullmac)    \
    .name = default_fullmac,

/* Structure containing channel context information */
struct asr_chanctx {
    struct cfg80211_chan_def chan_def; /* channel description */
    uint8_t count;                     /* number of vif using this ctxt */
};

/* Structure that will contains all RoC information received from cfg80211 */
struct asr_roc_elem {
    struct ieee80211_channel *chan;
    unsigned int duration;
    /* Used to avoid call of CFG80211 callback upon expiration of RoC */
    bool mgmt_roc;
    /* Indicate if we have switch on the RoC channel */
    bool on_chan;

    struct asr_vif *asr_vif;
};

/**
 * struct asr_key - Key information
 *
 * @hw_idx: Idx of the key from hardware point of view
 */
struct asr_key {
    uint8_t hw_idx;
};

/**
 * struct vif_params - describes virtual interface parameters
 * @use_4addr: use 4-address frames
 * @macaddr: address to use for this virtual interface.
 *    If this parameter is set to zero address the driver may
 *    determine the address as needed.
 *    This feature is only fully supported by drivers that enable the
 *    %NL80211_FEATURE_MAC_ON_CREATE flag.  Others may support creating
 **    only p2p devices with specified MAC.
 */
struct vif_params {
       int use_4addr;
       uint8_t macaddr[ETH_ALEN];
};

/**
 * Structure used to store information relative to PS mode.
 *
 * @active: True when the sta is in PS mode.
 *          If false, other values should be ignored
 * @pkt_ready: Number of packets buffered for the sta in drv's txq
 *             (1 counter for Legacy PS and 1 for U-APSD)
 * @sp_cnt: Number of packets that remain to be pushed in the service period.
 *          0 means that no service period is in progress
 *          (1 counter for Legacy PS and 1 for U-APSD)
 */
struct asr_sta_ps {
    bool active;
    uint16_t pkt_ready[2];
    uint16_t sp_cnt[2];
};

/* Structure containing channel survey information received from MAC */
struct asr_survey_info {
    // Filled
    uint32_t filled;
    // Amount of time in ms the radio spent on the channel
    uint32_t chan_time_ms;
    // Amount of time the primary channel was sensed busy
    uint32_t chan_time_busy_ms;
    // Noise in dbm
    int8_t noise_dbm;
};

/*
 * Structure used to save information relative to the managed stations.
 */
struct asr_sta {
    struct list_head list;
    uint16_t aid;                /* association ID */
    uint8_t sta_idx;             /* Identifier of the station */
    uint8_t vif_idx;             /* Identifier of the VIF (fw id) the station
                               belongs to */
    uint8_t vlan_idx;            /* Identifier of the VLAN VIF (fw id) the station
                               belongs to (= vif_idx if no vlan in used) */
    enum ieee80211_band band; /* Band */
    enum nl80211_chan_width width; /* Channel width */
    uint16_t center_freq;        /* Center frequency */
    uint32_t center_freq1;       /* Center frequency 1 */
    uint32_t center_freq2;       /* Center frequency 2 */
    uint8_t ch_idx;              /* Identifier of the channel
                               context the station belongs to */
    bool qos;               /* Flag indicating if the station
                               supports QoS */
    uint8_t acm;                 /* Bitfield indicating which queues
                               have AC mandatory */
    uint16_t uapsd_tids;         /* Bitfield indicating which tids are subject to
                               UAPSD */
    uint8_t mac_addr[ETH_ALEN];  /* MAC address of the station */
    struct asr_key key;
    bool valid;             /* Flag indicating if the entry is valid */
    struct asr_sta_ps ps;  /* Information when STA is in PS (AP only) */

    bool ht;               /* Flag indicating if the station
                               supports HT */
    uint32_t ac_param[AC_MAX];  /* EDCA parameters */
    #ifdef THREADX_STM32
    struct asr_tcp_ack tcp_ack[NX_NB_TID_PER_STA];
    #endif
};

/**
 * struct asr_bcn - Information of the beacon in used (AP mode)
 *
 * @head: head portion of beacon (before TIM IE)
 * @tail: tail portion of beacon (after TIM IE)
 * @ies: extra IEs (not used ?)
 * @head_len: length of head data
 * @tail_len: length of tail data
 * @ies_len: length of extra IEs data
 * @tim_len: length of TIM IE
 * @len: Total beacon len (head + tim + tail + extra)
 * @dtim: dtim period
 */
struct asr_bcn {
    uint8_t *head;
    uint8_t *tail;
    uint8_t *ies;
    size_t head_len;
    size_t tail_len;
    size_t ies_len;
    size_t tim_len;
    size_t len;
    uint8_t dtim;
};

struct asr_dma_elem {
    uint8_t *buf;
    dma_addr_t dma_addr;
    int len;
};

/**
 * struct asr_csa - Information for CSA (Channel Switch Announcement)
 *
 * @vif: Pointer to the vif doing the CSA
 * @bcn: Beacon to use after CSA
 * @dma: DMA descriptor to send the new beacon to the fw
 * @chandef: defines the channel to use after the switch
 * @count: Current csa counter
 * @status: Status of the CSA at fw level
 * @ch_idx: Index of the new channel context
 * @work: work scheduled at the end of CSA
 */
struct asr_csa {
    struct asr_vif *vif;
    struct asr_bcn bcn;
    struct asr_dma_elem dma;
    struct cfg80211_chan_def chandef;
    int count;
    int status;
    int ch_idx;
    //struct work_struct work;
};

enum nl80211_iftype {
    NL80211_IFTYPE_UNSPECIFIED,//0
    NL80211_IFTYPE_STATION,
    NL80211_IFTYPE_AP,
#ifdef CFG_SNIFFER_SUPPORT
    NL80211_IFTYPE_MONITOR,
#endif
    /* keep last */
    NUM_NL80211_IFTYPES,
    NL80211_IFTYPE_MAX = NUM_NL80211_IFTYPES - 1
};



#define AUTOCONNECT_INTERVAL_INIT   50      //50ms
#define AUTOCONNECT_INTERVAL_MID    500     //500ms
#define AUTOCONNECT_INTERVAL_MAX    3000    //3000ms
#define AUTOCONNECT_INIT_STEP       50      //50ms
#define AUTOCONNECT_MAX_STEP        200     //200ms

#if NX_AMSDU_TX
struct asr_amsdu_stats {
    int done;
    int failed;
};
#endif

struct asr_stats {
    int cfm_balance[NX_TXQ_CNT];
    unsigned long last_rx, last_tx; /* jiffies */
    int ampdus_tx[IEEE80211_MAX_AMPDU_BUF];
    int ampdus_rx[IEEE80211_MAX_AMPDU_BUF];
    int ampdus_rx_map[4];
    int ampdus_rx_miss;
    int amsdus_rx[64];
    uint32_t tx_ctlpkts;
    uint32_t tx_ctlerrs;
    uint32_t rx_ctlpkts;
    uint32_t rx_ctlerrs;
#ifdef ASR_STATS_RATES_TIMER
    struct stats_rate txrx_rates[TXRX_RATES_NUM];
    unsigned long tx_bytes;
    unsigned long rx_bytes;
    unsigned long last_rx_times;
#endif
};

struct asr_preq_ie_elem {
    uint8_t *buf;
    dma_addr_t dma_addr;
    int bufsz;
};

struct asr_patternbuf {
    uint32_t *buf;
    uint32_t addr;
    int bufsz;
};

#define SA_QUERY_RETRY_MAX_CNT     5

struct config_info {
    asr_wlan_ip_stat_t wlan_ip_stat;
    asr_wlan_ap_info_adv_t connect_ap_info;
    asr_timer_t        wifi_retry_timer;
    int                 wifi_retry_interval;
    int                 wifi_retry_times;
    uint32_t            cipher_group;
    uint32_t            cipher_pairwise;
    uint16_t            control_port_ethertype;
    int                 is_connected;
    uint8_t             autoconnect_tries;
    uint8_t             autoconnect_count;
    uint8_t             ie[MAX_IE_LEN];
    uint16_t            ie_len;
    bool                control_port;
    enum autoconnect_type en_autoconnect;         /* 0: close autoconnect;1: scan 1st; 2. direct connect*/
#if (defined(CONFIG_IEEE80211W) || NX_SAE)
    uint32_t cipher_mgmt_group;
    uint32_t akm_suite;
    enum nl80211_mfp mfp_option;
    // new add for mfp sa query process.
    uint32_t last_unprot_disconnect_time;
    uint32_t sa_query_start_time;
    uint8_t sa_query_trans_id[SA_QUERY_RETRY_MAX_CNT][2];
    asr_timer_t sa_query_timer;
    uint8_t sa_query_count;
    uint8_t rsnxe;
#endif

};

//add by will adjust later
enum wifi_status
{
    WIFI_INACTIVE = 0,
    WIFI_OPEN,
    WIFI_CLOSE,
    WIFI_SCANNING,
    WIFI_WAITING_SCAN_RES,
    WIFI_SCAN_DONE,
    WIFI_SCAN_FAILED,
    WIFI_CONNECT_FAILED,
    WIFI_DISCONNECTING,
    WIFI_DISCONNECTED,
    WIFI_DISCONNECT_FAILED,
    WIFI_CONNECTING,
    WIFI_ASSOCIATED, //Associated
    WIFI_CONNECTED,  //wep and open connect succ, wpa/wpa2 need finish 4 eapol handshake
    LINKED_SCAN,
    LINKED_WAITING_SCAN_RES,
    LINKED_SCAN_DONE,
    LINKED_SCAN_FAILED,
    LINKED_DHCP,
    LINKED_CONFIGED, //connected and ip get
    STATUS_NUM
};
#ifdef ASR_A0V1
enum rf_init_status
{
    RF_UNINITIAL = 0,
    RF_CLOCK_INIT,
    RF_TX_RX_CAL,
    RF_STATUS_NUM
};
#endif
/********************************************
Structure definition
*********************************************/
struct wifi_conn_info
{
    enum wifi_status wifiState;
    //uint8_t          ap_ssid[MAX_SSID_LEN];
    //uint8_t          ssid_len;
    //uint8_t          encrypt;
    //uint8_t          ap_bssid[ETH_ALEN];
    //uint32_t         ipaddr;
    uint16_t         channel;
};

struct wlan_mac
{
    uint8_t mac_addr[ETH_ALEN];
};

/**
 * struct asr_hwq - Structure used to save information relative to
 *                   an AC TX queue (aka HW queue)
 * @list: List of TXQ, that have buffers ready for this HWQ
 * @credits: available credit for the queue (i.e. nb of buffers that
 *           can be pushed to FW )
 * @id Id of the HWQ among ASR_HWQ_....
 * @size size of the queue
 * @need_processing Indicate if hwq should be processed
 * @len number of packet ready to be pushed to fw for this HW queue
 * @len_stop threshold to stop mac80211(i.e. netdev) queues. Stop queue when
 *           driver has more than @len_stop packets ready.
 * @len_start threshold to wake mac8011 queues. Wake queue when driver has
 *            less than @len_start packets ready.
 */
struct asr_hwq {
    struct list_head list;
    uint8_t credits[CONFIG_USER_MAX];
    uint8_t size;
    uint8_t id;
    bool need_processing;
};

/**
 * struct asr_txq - Structure used to save information relative to
 *                   a RA/TID TX queue
 *
 * @idx: Unique txq idx. Set to TXQ_INACTIVE if txq is not used.
 * @status: bitfield of @asr_txq_flags.
 * @credits: available credit for the queue (i.e. nb of buffers that
 *           can be pushed to FW).
 * @pkt_sent: number of consecutive pkt sent without leaving HW queue list
 * @pkt_pushed: number of pkt currently pending for transmission confirmation
 * @sched_list: list node for HW queue schedule list (asr_hwq.list)
 * @sk_list: list of buffers to push to fw
 * @last_retry_skb: pointer on the last skb in @sk_list that is a retry.
 *                  (retry skb are stored at the beginning of the list)
 *                  NULL if no retry skb is queued in @sk_list
 * @nb_retry: Number of retry packet queued.
 * @hwq: Pointer on the associated HW queue.
 *
 * SOFTMAC specific:
 * @baw: Block Ack window information
 * @amsdu_anchor: pointer to asr_sw_txhdr of the first subframe of the A-MSDU.
 *                NULL if no A-MSDU frame is in construction
 * @amsdu_ht_len_cap:
 * @amsdu_vht_len_cap:
 * @tid:
 *
 * FULLMAC specific
 * @ps_id: Index to use for Power save mode (ASRCY or UAPSD)
 * @push_limit: number of packet to push before removing the txq from hwq list.
 *              (we always have push_limit < skb_queue_len(sk_list))
 * @ndev_idx: txq idx from netdev point of view (0xFF for non netdev queue)
 * @ndev: pointer to ndev of the corresponding vif
 * @amsdu: pointer to asr_sw_txhdr of the first subframe of the A-MSDU.
 *         NULL if no A-MSDU frame is in construction
 * @amsdu_len: Maximum size allowed for an A-MSDU. 0 means A-MSDU not allowed
 */
struct asr_txq {
    uint16_t idx;
    uint8_t status;
    int8_t credits;
    uint8_t pkt_sent;
    uint8_t pkt_pushed[CONFIG_USER_MAX];
    struct list_head sched_list;
    struct sk_buff_head sk_list;
    struct sk_buff *last_retry_skb;
    struct asr_hwq *hwq;
    int nb_retry;

    struct asr_sta *sta;
    uint8_t ps_id;
    uint8_t push_limit;
    uint16_t ndev_idx;
};

struct cfg80211_ops {
    int (*add_virtual_intf)(struct asr_hw *asr_hw, enum nl80211_iftype type,
                                                                struct vif_params *params, struct asr_vif **asr_vif);
    int    (*del_virtual_intf)(struct asr_vif *asr_vif);
    int    (*change_virtual_intf)(struct asr_vif *asr_vif, enum nl80211_iftype type, struct vif_params *params);
    int    (*scan)(struct asr_vif *asr_vif, struct cfg80211_scan_request *request);
    int    (*connect)(struct asr_vif *asr_vif, struct cfg80211_connect_params *sme);
    int    (*disconnect)(struct asr_vif *asr_vif, uint16_t reason_code);
    int    (*add_key)(struct asr_vif *asr_vif, uint8_t key_index, bool pairwise,
                                                                    const uint8_t *mac_addr, struct key_params *params);
    int    (*del_key)(struct asr_vif *asr_vif, uint8_t key_index, bool pairwise, const uint8_t *mac_addr);
    int    (*add_station)(struct asr_vif *asr_vif, const uint8_t *mac, struct station_parameters *params);
    int    (*del_station)(struct asr_vif *asr_vif, struct station_del_parameters *params);
    int    (*change_station)(struct asr_hw *asr_hw, const uint8_t *mac, struct station_parameters *params);
    int    (*mgmt_tx)(struct asr_vif *asr_vif, struct cfg80211_mgmt_tx_params *params, uint64_t *cookie);
    int    (*start_ap)(struct asr_vif *asr_vif, struct cfg80211_ap_settings *settings);
    int    (*stop_ap)(struct asr_vif *asr_vif);
    int    (*change_beacon)(struct asr_vif *asr_vif, struct cfg80211_beacon_data *info);
    int    (*set_txq_params)(struct asr_vif *asr_vif, struct ieee80211_txq_params *params);
    int    (*set_tx_power)(struct asr_vif *asr_vif, enum nl80211_tx_power_setting type, int dbm);
    int    (*remain_on_channel)(struct asr_vif *asr_vif, struct ieee80211_channel *chan,
                                                                               unsigned int duration, uint64_t *cookie);
    int    (*cancel_remain_on_channel)(struct asr_hw *asr_hw, uint64_t cookie);
    int    (*get_channel)(struct asr_vif *asr_vif, struct cfg80211_chan_def *chandef);
    int    (*set_cqm_rssi_config)(struct asr_vif *asr_vif, int32_t rssi_thold, uint32_t rssi_hyst);
    int    (*set_cqm_rssi_level_config)(struct asr_vif *asr_vif, bool enable);
    int    (*channel_switch)(struct asr_vif *asr_vif, struct cfg80211_csa_settings *params);
    int    (*change_bss)(struct asr_vif *asr_vif, struct bss_parameters *params);
};

struct countrycode_list {
    const char* ccode;
    uint8_t channelcount;
};

// phy_flag saved in asr_hw.
enum asr_phy_flag {
    ASR_DEV_INITED,
    ASR_DEV_RESTARTING,
    ASR_DEV_STARTED,
    ASR_DEV_PRE_RESTARTING,
};

// dev_flag saved in asr_vif
enum asr_dev_flag {
    ASR_DEV_PRECLOSEING,
    ASR_DEV_CLOSEING,
    ASR_DEV_SCANING,
    ASR_DEV_STA_CONNECTING,
    ASR_DEV_STA_AUTH,
    ASR_DEV_STA_CONNECTED,
    ASR_DEV_STA_DHCPEND,
    ASR_DEV_STA_DISCONNECTING,
    ASR_DEV_STA_OUT_TWTSP,
    ASR_DEV_TXQ_STOP_CSA,
    ASR_DEV_TXQ_STOP_CHAN,
    ASR_DEV_TXQ_STOP_VIF_PS,

    /*total number of flags -- must be at the end*/
    ASR_DEV_NUM_FLAGS,
};

struct asr_hw {
    struct asr_mod_params *mod_params;
    struct wiphy *wiphy;
    struct list_head vifs;
    struct asr_vif *vif_table[NX_VIRT_DEV_MAX + NX_REMOTE_STA_MAX]; /* indexed with fw id */
    struct asr_sta sta_table[NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX];
    struct asr_survey_info survey[SCAN_CHANNEL_MAX];
    struct cfg80211_scan_request *scan_request;
    struct asr_chanctx chanctx_table[NX_CHAN_CTXT_CNT];
    uint8_t cur_chanctx;

    /* RoC Management */
    struct asr_roc_elem *roc_elem;             /* Information provided by cfg80211 in its remain on channel request */
    uint32_t roc_cookie_cnt;                         /* Counter used to identify RoC request sent by cfg80211 */

    struct asr_cmd_mgr cmd_mgr;

    asr_atomic_t phy_flags;

    struct ipc_host_env_tag *ipc_env;           /* store the IPC environment */

    asr_mutex_t tx_lock;

    //spinlock_t cb_lock;
    asr_mutex_t mutex;                         /* per-device perimeter lock */
    asr_mutex_t scan_mutex;                    /* scan lock */
    asr_mutex_t wifi_ready_mutex;              /* wifi_ready lock */

    struct mm_version_cfm version_cfm;          /* Lower layers versions - obtained via MM_VERSION_REQ */

    //struct asr_dbginfo     dbginfo;            /* Debug information from FW */

    struct asr_stats       stats;

    struct asr_preq_ie_elem preq_ie;

    struct asr_txq txq[NX_NB_TXQ];
    struct asr_hwq hwq[NX_TXQ_CNT];
    //struct asr_sec_phy_chan sec_phy_chan;
    uint8_t avail_idx_map;
    uint8_t vif_started;
    bool adding_sta;
    struct phy_cfg_tag phy_config;

    //extend
    char country[2];
    uint8_t sta_vif_idx;
    uint8_t ap_vif_idx;
//#ifdef CFG_SNIFFER_SUPPORT
    uint8_t monitor_vif_idx;
//#endif
    //add by scan module
    uint8_t                 scan_count;
    uint8_t                 ap_num;
#ifdef CFG_STATION_SUPPORT
    struct scan_result      scan_list[MAX_AP_NUM];
    struct scan_result*     phead_scan_result;//phead for sorted scan result
#endif
    struct sk_buff_head rx_data_sk_list;
    struct sk_buff_head rx_msg_sk_list;
    #ifdef CFG_AMSDU_TEST
    struct sk_buff_head rx_sk_split_list;
    #endif

    #ifdef SDIO_DEAGGR
    struct sk_buff_head rx_sk_sdio_deaggr_list;
    #endif

    struct sk_buff_head rx_to_os_skb_list;
    struct sk_buff_head rx_pending_skb_list;

    volatile uint16_t tx_use_bitmap;
    volatile uint8_t tx_data_cur_idx;
    volatile uint8_t rx_data_cur_idx;
    int ioport;
    uint8_t *sdio_reg;   // use malloc
    uint8_t last_sdio_regs[SDIO_REG_READ_LENGTH];
    uint8_t sdio_ireg;

    struct asr_tx_agg tx_agg_env;
    asr_mutex_t tx_agg_env_lock;

    // tx opt use tx_sk_list
    struct sk_buff_head tx_sk_free_list;             // use tx_lock to protect.
    struct sk_buff_head tx_sk_list;             // use tx_lock to protect.
    struct sk_buff_head tx_hif_free_buf_list;
    struct sk_buff_head tx_hif_skb_list;

    asr_mutex_t tx_agg_timer_lock;

    asr_timer_t amsdu_malloc_free_timer;

    asr_timer_t tx_status_timer;
    asr_timer_t rx_thread_timer;
#ifdef RX_TRIGGER_TIMER_ENABLE
    asr_timer_t rx_trigger_timer;
#endif

    asr_timer_t tx_aggr_timer;
    bool xmit_first_trigger_flag;

    asr_timer_t sdio_debug_timer;
    struct sk_buff *pskb;

    bool more_task_flag;
    bool restart_flag;

    bool ps_on;
    uint32_t rx_packet_num;
    uint32_t last_rx_packet_num;
    uint32_t rx_thread_time;
    uint32_t rx_trigger_time;
    uint32_t rx_bytes;
    uint32_t last_rx_bytes;

    struct mm_fw_softversion_cfm fw_softversion_cfm;

    uint8_t mac_addr[ETH_ALEN];
    enum nl80211_iftype iftype;

#ifdef CFG_STATION_SUPPORT
    asr_timer_t scan_cmd_timer;
#endif

    u8 vif_max_num;
    u8 sta_max_num;

    struct mm_hif_sdio_info_ind hif_sdio_info_ind;
    asr_mutex_t tx_msg_lock;
};


#ifdef CFG_STA_AP_COEX
#define AP_MAX_ASSOC_NUM       (CFG_STA_MAX-1)
#else
#define AP_MAX_ASSOC_NUM       CFG_STA_MAX
#endif


#define SUPPORT_RATESET_LEN    16
#define WPA_WPA2_IE_LEN        34 //NOT SUPPORT FT,PKMID is zero
//used to store peer sta info, used in hostapd
struct uwifi_ap_peer_info
{
    struct list_head list;
    uint8_t      supported_rates[SUPPORT_RATESET_LEN];
    uint32_t     sta_flags_set;
    uint16_t     aid;
    // Bit field indicating which queues have to be delivered upon U-APSD trigger
    uint8_t      uapsd_queues;
    uint8_t      max_sp;
    uint8_t      supported_rates_len;
    uint8_t      short_preamble;
    uint8_t      peer_addr[MAC_ADDR_LEN];
    uint8_t      wpa_wpa2_ie[WPA_WPA2_IE_LEN];  //for 4 way handshake, include ie_id and ie_len field
    uint8_t      wpa_wpa2_ie_len;
    uint8_t      qos;
    uint8_t      ht;
    uint8_t      he;
    uint8_t      encryption_protocol;
    struct ieee80211_ht_cap   ht_cap;
    struct ieee80211_he_cap   he_cap;
    uint8_t he_cap_len;
    uint8_t connect_status;  //during connecting one peer sta, connect request from other sta canot be handled.
    asr_timer_t auth_timer;  //timer during auth
    asr_timer_t assoc_timer;  //timer during assoc
};

#ifdef CFG_ADD_API
#define AP_MAX_BLACK_NUM 16
#define AP_MAX_WHITE_NUM 16
struct list_mac
{
    struct list_head list;
    uint8_t  peer_addr[MAC_ADDR_LEN];
};
#endif

typedef void (*cb_stop_ap_ptr)(void);
/*
 * Structure used to save information relative to the managed interfaces.
 * This is also linked within the asr_hw vifs list.
 *
 */
struct asr_vif {
    struct list_head list;
    struct asr_key key[6];
    enum nl80211_iftype iftype;
    uint8_t drv_vif_index;           /* Identifier of the VIF in driver */
    uint8_t vif_index;               /* Identifier of the station in FW */
    uint8_t ch_index;                /* Channel context identifier */
    bool up;                    /* Indicate if associated netdev is up
                                   (i.e. Interface is created at fw level) */
    bool use_4addr;             /* Should we use 4addresses mode */
    bool is_resending;          /* Indicate if a frame is being resent on this interface */

    union
    {
        struct
        {
            struct asr_sta *ap; /* Pointer to the peer STA entry allocated for the AP */
            struct config_info configInfo;
            struct wifi_conn_info connInfo;
            u8 auth_type;
            u8 rsnxe;
        } sta;
        struct
        {
            uint16_t flags;                 /* see asr_ap_flags */
            struct list_head sta_list; /* List of STA connected to the AP */
            struct asr_bcn bcn;       /* beacon */
            uint8_t bcmc_index;             /* Index of the BCMC sta to use */
            struct asr_csa *csa;
            //struct list_head proxy_list; /* List of Proxies Information used on this interface */

            cb_stop_ap_ptr   stop_ap_cb;     /*used in hostapd*/
            uint16_t     aid_bitmap;         /*used in hostapd*/
            uint8_t  *bcn_frame;             /*used in hostapd*/
            uint8_t  *probe_rsp;             /*used in hostapd*/
            uint16_t probe_rsp_len;          /*used in hostapd*/
            struct list_head peer_sta_list;  /*used in hostapd*/
            struct uwifi_ap_peer_info peer_sta[AP_MAX_ASSOC_NUM]; /*used in hostapd*/
            uint8_t      connect_sta_enable; /*used in hostapd*/

            #ifdef CFG_ADD_API
            struct list_head peer_black_list;  /*used in hostapd for blacklist*/
            struct list_mac peer_black[AP_MAX_BLACK_NUM]; /*used in hostapd*/
            uint8_t black_state;

            struct list_head peer_white_list;  /*used in hostapd for blacklist*/
            struct list_mac peer_white[AP_MAX_WHITE_NUM]; /*used in hostapd*/
            uint8_t white_state;

            // set channel
            struct ieee80211_channel    st_chan;
            enum nl80211_channel_type   chan_type;
            uint8_t                     chan_num;
            #endif
        bool closing;
        } ap;
#ifdef CFG_SNIFFER_SUPPORT
        struct
        {
            struct ieee80211_channel    st_chan;
            uint32_t                    rx_filter;
            enum nl80211_channel_type   chan_type;
            uint8_t                     chan_num;
        } sniffer;
#endif
    };

    //struct asr_hw asr_hw;
    struct asr_hw *asr_hw;
    //extend
    struct wlan_mac wlan_mac_add;
    void   *net_if;

    uint32_t txring_bytes;
    uint32_t tx_skb_cnt;       // skb cnt used for flow ctrl in tx opt list
    bool vif_disable_tx;
    asr_atomic_t dev_flags;

    struct net_device_stats net_stats;
};

extern struct cfg80211_ops uwifi_cfg80211_ops;

struct asr_traffic_status {
    bool send;
    struct asr_sta *asr_sta_ps;
    bool tx_ava;
    u8 ps_id_bits;
};

uint8_t *asr_build_bcn(struct asr_bcn *bcn, struct cfg80211_beacon_data *new);

void asr_chanctx_link(struct asr_vif *vif, uint8_t idx,
                        struct cfg80211_chan_def *chandef);
void asr_chanctx_unlink(struct asr_vif *vif);
int  asr_chanctx_valid(struct asr_hw *asr_hw, uint8_t idx);

static inline bool is_multicast_sta(struct asr_hw *asr_hw, int sta_idx)
{
    return (sta_idx >= asr_hw->sta_max_num);
}

static inline uint8_t master_vif_idx(struct asr_vif *vif)
{
    return vif->vif_index;
}

#endif /* _ASR_DEFS_H_ */
