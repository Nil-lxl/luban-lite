/**
 ******************************************************************************
 *
 * @file uwifi_ieee80211.h
 *
 * @brief IEEE 802.11 defines
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */

#ifndef _UWIFI_IEEE80211_H
#define _UWIFI_IEEE80211_H

#include <stdbool.h>
#include <stdint.h>
#include "uwifi_types.h"
#include "ipc_shared.h"

/*
 * DS bit usage
 *
 * TA = transmitter address
 * RA = receiver address
 * DA = destination address
 * SA = source address
 *
 * ToDS    FromDS  A1(RA)  A2(TA)  A3      A4      Use
 * -----------------------------------------------------------------
 *  0       0       DA      SA      BSSID   -       IBSS/DLS
 *  0       1       DA      BSSID   SA      -       AP -> STA
 *  1       0       BSSID   SA      DA      -       AP <- STA
 *  1       1       RA      TA      DA      SA      unspecified (WDS)
 */

#define FCS_LEN 4

#define IEEE80211_FCTL_VERS        0x0003
#define IEEE80211_FCTL_FTYPE        0x000c
#define IEEE80211_FCTL_STYPE        0x00f0
#define IEEE80211_FCTL_TODS        0x0100
#define IEEE80211_FCTL_FROMDS        0x0200
#define IEEE80211_FCTL_MOREFRAGS    0x0400
#define IEEE80211_FCTL_RETRY        0x0800
#define IEEE80211_FCTL_PM        0x1000
#define IEEE80211_FCTL_MOREDATA        0x2000
#define IEEE80211_FCTL_PROTECTED    0x4000
#define IEEE80211_FCTL_ORDER        0x8000
#define IEEE80211_FCTL_CTL_EXT        0x0f00

#define IEEE80211_SCTL_FRAG        0x000F
#define IEEE80211_SCTL_SEQ        0xFFF0

#define IEEE80211_FTYPE_MGMT        0x0000
#define IEEE80211_FTYPE_CTL        0x0004
#define IEEE80211_FTYPE_DATA        0x0008
#define IEEE80211_FTYPE_EXT        0x000c

/* management */
#define IEEE80211_STYPE_ASSOC_REQ    0x0000
#define IEEE80211_STYPE_ASSOC_RESP    0x0010
#define IEEE80211_STYPE_REASSOC_REQ    0x0020
#define IEEE80211_STYPE_REASSOC_RESP    0x0030
#define IEEE80211_STYPE_PROBE_REQ    0x0040
#define IEEE80211_STYPE_PROBE_RESP    0x0050
#define IEEE80211_STYPE_BEACON        0x0080
#define IEEE80211_STYPE_ATIM        0x0090
#define IEEE80211_STYPE_DISASSOC    0x00A0
#define IEEE80211_STYPE_AUTH        0x00B0
#define IEEE80211_STYPE_DEAUTH        0x00C0
#define IEEE80211_STYPE_ACTION        0x00D0

/* control */
#define IEEE80211_STYPE_CTL_EXT        0x0060
#define IEEE80211_STYPE_BACK_REQ    0x0080
#define IEEE80211_STYPE_BACK        0x0090
#define IEEE80211_STYPE_PSPOLL        0x00A0
#define IEEE80211_STYPE_RTS        0x00B0
#define IEEE80211_STYPE_CTS        0x00C0
#define IEEE80211_STYPE_ACK        0x00D0
#define IEEE80211_STYPE_CFEND        0x00E0
#define IEEE80211_STYPE_CFENDACK    0x00F0

/* data */
#define IEEE80211_STYPE_DATA            0x0000
#define IEEE80211_STYPE_DATA_CFACK        0x0010
#define IEEE80211_STYPE_DATA_CFPOLL        0x0020
#define IEEE80211_STYPE_DATA_CFACKPOLL        0x0030
#define IEEE80211_STYPE_NULLFUNC        0x0040
#define IEEE80211_STYPE_CFACK            0x0050
#define IEEE80211_STYPE_CFPOLL            0x0060
#define IEEE80211_STYPE_CFACKPOLL        0x0070
#define IEEE80211_STYPE_QOS_DATA        0x0080
#define IEEE80211_STYPE_QOS_DATA_CFACK        0x0090
#define IEEE80211_STYPE_QOS_DATA_CFPOLL        0x00A0
#define IEEE80211_STYPE_QOS_DATA_CFACKPOLL    0x00B0
#define IEEE80211_STYPE_QOS_NULLFUNC        0x00C0
#define IEEE80211_STYPE_QOS_CFACK        0x00D0
#define IEEE80211_STYPE_QOS_CFPOLL        0x00E0
#define IEEE80211_STYPE_QOS_CFACKPOLL        0x00F0

/* extension, added by 802.11ad */
#define IEEE80211_STYPE_DMG_BEACON        0x0000

/* control extension - for IEEE80211_FTYPE_CTL | IEEE80211_STYPE_CTL_EXT */
#define IEEE80211_CTL_EXT_POLL        0x2000
#define IEEE80211_CTL_EXT_SPR        0x3000
#define IEEE80211_CTL_EXT_GRANT    0x4000
#define IEEE80211_CTL_EXT_DMG_CTS    0x5000
#define IEEE80211_CTL_EXT_DMG_DTS    0x6000
#define IEEE80211_CTL_EXT_SSW        0x8000
#define IEEE80211_CTL_EXT_SSW_FBACK    0x9000
#define IEEE80211_CTL_EXT_SSW_ACK    0xa000


#define IEEE80211_SN_MASK        ((IEEE80211_SCTL_SEQ) >> 4)
#define IEEE80211_MAX_SN        IEEE80211_SN_MASK
#define IEEE80211_SN_MODULO        (IEEE80211_MAX_SN + 1)

#define IEEE80211_SEQ_TO_SN(seq)    (((seq) & IEEE80211_SCTL_SEQ) >> 4)
#define IEEE80211_SN_TO_SEQ(ssn)    (((ssn) << 4) & IEEE80211_SCTL_SEQ)

/* miscellaneous IEEE 802.11 constants */
#define IEEE80211_MAX_FRAG_THRESHOLD    2352
#define IEEE80211_MAX_RTS_THRESHOLD    2353
#define IEEE80211_MAX_AID        2007
#define IEEE80211_MAX_TIM_LEN        251
#define IEEE80211_MAX_MESH_PEERINGS    63
/* Maximum size for the MA-UNITDATA primitive, 802.11 standard section
   6.2.1.1.2.

   802.11e clarifies the figure in section 7.1.2. The frame body is
   up to 2304 octets long (maximum MSDU size) plus any crypt overhead. */
#define IEEE80211_MAX_DATA_LEN        2304
/* 802.11ad extends maximum MSDU size for DMG (freq > 40Ghz) networks
 * to 7920 bytes, see 8.2.3 General frame format
 */
#define IEEE80211_MAX_DATA_LEN_DMG    7920
/* 30 byte 4 addr hdr, 2 byte QoS, 2304 byte MSDU, 12 byte crypt, 4 byte FCS */
#define IEEE80211_MAX_FRAME_LEN        2352

#define IEEE80211_MAX_SSID_LEN        32

#define IEEE80211_MAX_MESH_ID_LEN    32

#define IEEE80211_FIRST_TSPEC_TSID    8
#define IEEE80211_NUM_TIDS        16

/* number of user priorities 802.11 uses */
#define IEEE80211_NUM_UPS        8

#define IEEE80211_QOS_CTL_LEN        2
/* 1d tag mask */
#define IEEE80211_QOS_CTL_TAG1D_MASK        0x0007
/* TID mask */
#define IEEE80211_QOS_CTL_TID_MASK        0x000f
/* EOSP */
#define IEEE80211_QOS_CTL_EOSP            0x0010
/* ACK policy */
#define IEEE80211_QOS_CTL_ACK_POLICY_NORMAL    0x0000
#define IEEE80211_QOS_CTL_ACK_POLICY_NOACK    0x0020
#define IEEE80211_QOS_CTL_ACK_POLICY_NO_EXPL    0x0040
#define IEEE80211_QOS_CTL_ACK_POLICY_BLOCKACK    0x0060
#define IEEE80211_QOS_CTL_ACK_POLICY_MASK    0x0060
/* A-MSDU 802.11n */
#define IEEE80211_QOS_CTL_A_MSDU_PRESENT    0x0080
/* Mesh Control 802.11s */
#define IEEE80211_QOS_CTL_MESH_CONTROL_PRESENT  0x0100

/* Mesh Power Save Level */
#define IEEE80211_QOS_CTL_MESH_PS_LEVEL        0x0200
/* Mesh Receiver Service Period Initiated */
#define IEEE80211_QOS_CTL_RSPI            0x0400

/* U-APSD queue for WMM IEs sent by AP */
#define IEEE80211_WMM_IE_AP_QOSINFO_UAPSD    (1<<7)
#define IEEE80211_WMM_IE_AP_QOSINFO_PARAM_SET_CNT_MASK    0x0f

/* U-APSD queues for WMM IEs sent by STA */
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_VO    (1<<0)
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_VI    (1<<1)
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_BK    (1<<2)
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_BE    (1<<3)
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_MASK    0x0f

/* U-APSD max SP length for WMM IEs sent by STA */
#define IEEE80211_WMM_IE_STA_QOSINFO_SP_ALL    0x00
#define IEEE80211_WMM_IE_STA_QOSINFO_SP_2    0x01
#define IEEE80211_WMM_IE_STA_QOSINFO_SP_4    0x02
#define IEEE80211_WMM_IE_STA_QOSINFO_SP_6    0x03
#define IEEE80211_WMM_IE_STA_QOSINFO_SP_MASK    0x03
#define IEEE80211_WMM_IE_STA_QOSINFO_SP_SHIFT    5

#define IEEE80211_HT_CTL_LEN        4

/**
 * enum survey_info_flags - survey information flags
 *
 * @SURVEY_INFO_NOISE_DBM: noise (in dBm) was filled in
 * @SURVEY_INFO_IN_USE: channel is currently being used
 * @SURVEY_INFO_CHANNEL_TIME: channel active time (in ms) was filled in
 * @SURVEY_INFO_CHANNEL_TIME_BUSY: channel busy time was filled in
 * @SURVEY_INFO_CHANNEL_TIME_EXT_BUSY: extension channel busy time was filled in
 * @SURVEY_INFO_CHANNEL_TIME_RX: channel receive time was filled in
 * @SURVEY_INFO_CHANNEL_TIME_TX: channel transmit time was filled in
 *
 * Used by the driver to indicate which info in &struct survey_info
 * it has filled in during the get_survey().
 */
enum survey_info_flags {
    SURVEY_INFO_NOISE_DBM = 1<<0,
    SURVEY_INFO_IN_USE = 1<<1,
    SURVEY_INFO_CHANNEL_TIME = 1<<2,
    SURVEY_INFO_CHANNEL_TIME_BUSY = 1<<3,
    SURVEY_INFO_CHANNEL_TIME_EXT_BUSY = 1<<4,
    SURVEY_INFO_CHANNEL_TIME_RX = 1<<5,
    SURVEY_INFO_CHANNEL_TIME_TX = 1<<6,
};

/**
 * enum nl80211_channel_type - channel type
 * @NL80211_CHAN_NO_HT: 20 MHz, non-HT channel
 * @NL80211_CHAN_HT20: 20 MHz HT channel
 * @NL80211_CHAN_HT40MINUS: HT40 channel, secondary channel
 *    below the control channel
 * @NL80211_CHAN_HT40PLUS: HT40 channel, secondary channel
 *    above the control channel
 */
enum nl80211_channel_type {
    NL80211_CHAN_NO_HT,
    NL80211_CHAN_HT20,
    NL80211_CHAN_HT40MINUS,
    NL80211_CHAN_HT40PLUS
};

/**
 * enum nl80211_sta_flags - station flags
 *
 * Station flags. When a station is added to an AP interface, it is
 * assumed to be already associated (and hence authenticated.)
 *
 * @__NL80211_STA_FLAG_INVALID: attribute number 0 is reserved
 * @NL80211_STA_FLAG_AUTHORIZED: station is authorized (802.1X)
 * @NL80211_STA_FLAG_SHORT_PREAMBLE: station is capable of receiving frames
 *    with short barker preamble
 * @NL80211_STA_FLAG_WME: station is WME/QoS capable
 * @NL80211_STA_FLAG_MFP: station uses management frame protection
 * @NL80211_STA_FLAG_AUTHENTICATED: station is authenticated
 * @NL80211_STA_FLAG_TDLS_PEER: station is a TDLS peer -- this flag should
 *    only be used in managed mode (even in the flags mask). Note that the
 *    flag can't be changed, it is only valid while adding a station, and
 *    attempts to change it will silently be ignored (rather than rejected
 *    as errors.)
 * @NL80211_STA_FLAG_ASSOCIATED: station is associated; used with drivers
 *    that support %NL80211_FEATURE_FULL_AP_CLIENT_STATE to transition a
 *    previously added station into associated state
 * @NL80211_STA_FLAG_MAX: highest station flag number currently defined
 * @__NL80211_STA_FLAG_AFTER_LAST: internal use
 */
enum nl80211_sta_flags {
    __NL80211_STA_FLAG_INVALID,
    NL80211_STA_FLAG_AUTHORIZED,
    NL80211_STA_FLAG_SHORT_PREAMBLE,
    NL80211_STA_FLAG_WME,
    NL80211_STA_FLAG_MFP,
    NL80211_STA_FLAG_AUTHENTICATED,
    NL80211_STA_FLAG_TDLS_PEER,
    NL80211_STA_FLAG_ASSOCIATED,

    /* keep last */
    __NL80211_STA_FLAG_AFTER_LAST,
    NL80211_STA_FLAG_MAX = __NL80211_STA_FLAG_AFTER_LAST - 1
};

struct ieee80211_hdr {
    uint16_t frame_control;
    uint16_t duration_id;
    uint8_t addr1[ETH_ALEN];
    uint8_t addr2[ETH_ALEN];
    uint8_t addr3[ETH_ALEN];
    uint16_t seq_ctrl;
    uint8_t addr4[ETH_ALEN];
} __attribute__ ((__packed__)) __attribute__((aligned(2)));

struct ieee80211_hdr_3addr {
    uint16_t frame_control;
    uint16_t duration_id;
    uint8_t addr1[ETH_ALEN];
    uint8_t addr2[ETH_ALEN];
    uint8_t addr3[ETH_ALEN];
    uint16_t seq_ctrl;
} __attribute__ ((__packed__)) __attribute__((aligned(2)));

/**
 * ieee80211_has_protected - check if IEEE80211_FCTL_PROTECTED is set
 * @fc: frame control bytes in little-endian byteorder
 */
static inline int ieee80211_has_protected(uint16_t fc)
{
    return (fc & cpu_to_le16(IEEE80211_FCTL_PROTECTED)) != 0;
}

/**
 * ieee80211_is_beacon - check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_BEACON
 * @fc: frame control bytes in little-endian byteorder
 */
static inline int ieee80211_is_beacon(uint16_t fc)
{
    return (fc & cpu_to_le16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
           cpu_to_le16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_BEACON);
}

/**
 * ieee80211_is_probe_req - check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_PROBE_REQ
 * @fc: frame control bytes in little-endian byteorder
 */
static inline bool ieee80211_is_probe_req(uint16_t fc)
{
    return (fc & cpu_to_le16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
           cpu_to_le16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_PROBE_REQ);
}

/**
 * ieee80211_is_probe_resp - check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_PROBE_RESP
 * @fc: frame control bytes in little-endian byteorder
 */
static inline bool ieee80211_is_probe_resp(uint16_t fc)
{
    return (fc & cpu_to_le16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
           cpu_to_le16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_PROBE_RESP);
}


/**
 * ieee80211_is_disassoc - check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_DISASSOC
 * @fc: frame control bytes in little-endian byteorder
 */
static inline int ieee80211_is_disassoc(uint16_t fc)
{
    return (fc & cpu_to_le16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
           cpu_to_le16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_DISASSOC);
}

/**
 * ieee80211_is_auth - check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_AUTH
 * @fc: frame control bytes in little-endian byteorder
 */
static inline int ieee80211_is_auth(uint16_t fc)
{
    return (fc & cpu_to_le16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        cpu_to_le16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_AUTH);
}

/**
 * ieee80211_is_deauth - check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_DEAUTH
 * @fc: frame control bytes in little-endian byteorder
 */
static inline int ieee80211_is_deauth(uint16_t fc)
{
    return (fc & cpu_to_le16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
           cpu_to_le16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_DEAUTH);
}

/**
 * ieee80211_is_action - check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_ACTION
 * @fc: frame control bytes in little-endian byteorder
 */
static inline int ieee80211_is_action(uint16_t fc)
{
    return (fc & cpu_to_le16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
           cpu_to_le16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_ACTION);
}

/**
 * ieee80211_is_action - check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_ACTION
 * @fc: frame control bytes in little-endian byteorder
 */
static inline int ieee80211_is_mgmt(uint16_t fc)
{
    return (fc & cpu_to_le16(IEEE80211_FCTL_FTYPE)) == cpu_to_le16(IEEE80211_FTYPE_MGMT);
}

/**
 * ieee80211_is_assoc_resp - check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_ASSOC_RESP
 * @fc: frame control bytes in little-endian byteorder
 */
static inline int ieee80211_is_assoc_resp(uint16_t fc)
{
    return (fc & cpu_to_le16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        cpu_to_le16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_ASSOC_RESP);
}

/**
 * enum ieee80211_preq_flags - mesh PREQ element flags
 *
 * @IEEE80211_PREQ_PROACTIVE_PREP_FLAG: proactive PREP subfield
 */
enum ieee80211_preq_flags {
    IEEE80211_PREQ_PROACTIVE_PREP_FLAG    = 1<<2,
};

/**
 * enum ieee80211_preq_target_flags - mesh PREQ element per target flags
 *
 * @IEEE80211_PREQ_TO_FLAG: target only subfield
 * @IEEE80211_PREQ_USN_FLAG: unknown target HWMP sequence number subfield
 */
enum ieee80211_preq_target_flags {
    IEEE80211_PREQ_TO_FLAG    = 1<<0,
    IEEE80211_PREQ_USN_FLAG    = 1<<2,
};

/**
 * struct ieee80211_msrment_ie
 *
 * This structure refers to "Measurement Request/Report information element"
 */
struct ieee80211_msrment_ie {
    uint8_t token;
    uint8_t mode;
    uint8_t type;
    uint8_t request[0];
} __attribute__ ((__packed__));

/**
 * struct ieee80211_ext_chansw_ie
 *
 * This structure represents the "Extended Channel Switch Announcement element"
 */
struct ieee80211_ext_chansw_ie {
    uint8_t mode;
    uint8_t new_operating_class;
    uint8_t new_ch_num;
    uint8_t count;
} __attribute__ ((__packed__));

enum ieee80211_rann_flags {
    RANN_FLAG_IS_GATE = 1 << 0,
};

enum ieee80211_ht_chanwidth_values {
    IEEE80211_HT_CHANWIDTH_20MHZ = 0,
    IEEE80211_HT_CHANWIDTH_ANY = 1,
};

/**
 * enum ieee80211_opmode_bits - VHT operating mode field bits
 * @IEEE80211_OPMODE_NOTIF_CHANWIDTH_MASK: channel width mask
 * @IEEE80211_OPMODE_NOTIF_CHANWIDTH_20MHZ: 20 MHz channel width
 * @IEEE80211_OPMODE_NOTIF_CHANWIDTH_40MHZ: 40 MHz channel width
 * @IEEE80211_OPMODE_NOTIF_CHANWIDTH_80MHZ: 80 MHz channel width
 * @IEEE80211_OPMODE_NOTIF_CHANWIDTH_160MHZ: 160 MHz or 80+80 MHz channel width
 * @IEEE80211_OPMODE_NOTIF_RX_NSS_MASK: number of spatial streams mask
 *    (the NSS value is the value of this field + 1)
 * @IEEE80211_OPMODE_NOTIF_RX_NSS_SHIFT: number of spatial streams shift
 * @IEEE80211_OPMODE_NOTIF_RX_NSS_TYPE_BF: indicates streams in SU-MIMO PPDU
 *    using a beamforming steering matrix
 */
enum ieee80211_vht_opmode_bits {
    IEEE80211_OPMODE_NOTIF_CHANWIDTH_MASK    = 3,
    IEEE80211_OPMODE_NOTIF_CHANWIDTH_20MHZ    = 0,
    IEEE80211_OPMODE_NOTIF_CHANWIDTH_40MHZ    = 1,
    IEEE80211_OPMODE_NOTIF_CHANWIDTH_80MHZ    = 2,
    IEEE80211_OPMODE_NOTIF_CHANWIDTH_160MHZ    = 3,
    IEEE80211_OPMODE_NOTIF_RX_NSS_MASK    = 0x70,
    IEEE80211_OPMODE_NOTIF_RX_NSS_SHIFT    = 4,
    IEEE80211_OPMODE_NOTIF_RX_NSS_TYPE_BF    = 0x80,
};

#define WLAN_SA_QUERY_TR_ID_LEN 2

/**
 * struct ieee80211_tpc_report_ie
 *
 * This structure refers to "TPC Report element"
 */
struct ieee80211_tpc_report_ie {
    uint8_t tx_power;
    uint8_t link_margin;
} __attribute__ ((__packed__));

struct ieee80211_mgmt {
    uint16_t frame_control;
    uint16_t duration;
    uint8_t da[ETH_ALEN];
    uint8_t sa[ETH_ALEN];
    uint8_t bssid[ETH_ALEN];
    uint16_t seq_ctrl;
    union {
        struct {
            uint16_t auth_alg;
            uint16_t auth_transaction;
            uint16_t status_code;
            /* possibly followed by Challenge text */
            uint8_t variable[0];
        } __attribute__ ((__packed__)) auth;
        struct {
            uint16_t reason_code;
        } __attribute__ ((__packed__)) deauth;
        struct {
            uint16_t capab_info;
            uint16_t listen_interval;
            /* followed by SSID and Supported rates */
            uint8_t variable[0];
        } __attribute__ ((__packed__)) assoc_req;
        struct {
            uint16_t capab_info;
            uint16_t status_code;
            uint16_t aid;
            /* followed by Supported rates */
            uint8_t variable[0];
        } __attribute__ ((__packed__)) assoc_resp, reassoc_resp;
        struct {
            uint16_t capab_info;
            uint16_t listen_interval;
            uint8_t current_ap[ETH_ALEN];
            /* followed by SSID and Supported rates */
            uint8_t variable[0];
        } __attribute__ ((__packed__)) reassoc_req;
        struct {
            uint16_t reason_code;
        } __attribute__ ((__packed__)) disassoc;
        struct {
            uint64_t timestamp;
            uint16_t beacon_int;
            uint16_t capab_info;
            /* followed by some of SSID, Supported rates,
             * FH Params, DS Params, CF Params, IBSS Params, TIM */
            uint8_t variable[0];
        } __attribute__ ((__packed__)) beacon;
        struct {
            /* only variable items: SSID, Supported rates */
            uint8_t variable[0];
        } __attribute__ ((__packed__)) probe_req;
        struct {
            uint64_t timestamp;
            uint16_t beacon_int;
            uint16_t capab_info;
            /* followed by some of SSID, Supported rates,
             * FH Params, DS Params, CF Params, IBSS Params */
            uint8_t variable[0];
        } __attribute__ ((__packed__)) probe_resp;
        struct {
            uint8_t category;
            union {
                struct {
                    uint8_t action_code;
                    uint8_t dialog_token;
                    uint8_t status_code;
                    uint8_t variable[0];
                } __attribute__ ((__packed__)) wme_action;
                struct{
                    uint8_t action_code;
                    uint8_t variable[0];
                } __attribute__ ((__packed__)) chan_switch;
                struct{
                    uint8_t action_code;
                    struct ieee80211_ext_chansw_ie data;
                    uint8_t variable[0];
                } __attribute__ ((__packed__)) ext_chan_switch;
                struct{
                    uint8_t action_code;
                    uint8_t dialog_token;
                    uint8_t element_id;
                    uint8_t length;
                    struct ieee80211_msrment_ie msr_elem;
                } __attribute__ ((__packed__)) measurement;
                struct{
                    uint8_t action_code;
                    uint8_t dialog_token;
                    uint16_t capab;
                    uint16_t timeout;
                    uint16_t start_seq_num;
                } __attribute__ ((__packed__)) addba_req;
                struct{
                    uint8_t action_code;
                    uint8_t dialog_token;
                    uint16_t status;
                    uint16_t capab;
                    uint16_t timeout;
                } __attribute__ ((__packed__)) addba_resp;
                struct{
                    uint8_t action_code;
                    uint16_t params;
                    uint16_t reason_code;
                } __attribute__ ((__packed__)) delba;
                struct {
                    uint8_t action_code;
                    uint8_t variable[0];
                } __attribute__ ((__packed__)) self_prot;
                struct{
                    uint8_t action_code;
                    uint8_t variable[0];
                } __attribute__ ((__packed__)) mesh_action;
                struct {
                    uint8_t action;
                    uint8_t trans_id[WLAN_SA_QUERY_TR_ID_LEN];
                } __attribute__ ((__packed__)) sa_query;
                struct {
                    uint8_t action;
                    uint8_t smps_control;
                } __attribute__ ((__packed__)) ht_smps;
                struct {
                    uint8_t action_code;
                    uint8_t chanwidth;
                } __attribute__ ((__packed__)) ht_notify_cw;
                struct {
                    uint8_t action_code;
                    uint8_t dialog_token;
                    uint16_t capability;
                    uint8_t variable[0];
                } __attribute__ ((__packed__)) tdls_discover_resp;
                struct {
                    uint8_t action_code;
                    uint8_t operating_mode;
                } __attribute__ ((__packed__)) vht_opmode_notif;
                struct {
                    uint8_t action_code;
                    uint8_t dialog_token;
                    uint8_t tpc_elem_id;
                    uint8_t tpc_elem_length;
                    struct ieee80211_tpc_report_ie tpc;
                } __attribute__ ((__packed__)) tpc_report;
            } u;
        } __attribute__ ((__packed__)) action;
    } u;
} __attribute__ ((__packed__)) __attribute__((aligned(2)));

/* Supported Rates value encodings in 802.11n-2009 7.3.2.2 */
#define BSS_MEMBERSHIP_SELECTOR_HT_PHY    127

/*
 * Peer-to-Peer IE attribute related definitions.
 */
/**
 * enum ieee80211_p2p_attr_id - identifies type of peer-to-peer attribute.
 */
enum ieee80211_p2p_attr_id {
    IEEE80211_P2P_ATTR_STATUS = 0,
    IEEE80211_P2P_ATTR_MINOR_REASON,
    IEEE80211_P2P_ATTR_CAPABILITY,
    IEEE80211_P2P_ATTR_DEVICE_ID,
    IEEE80211_P2P_ATTR_GO_INTENT,
    IEEE80211_P2P_ATTR_GO_CONFIG_TIMEOUT,
    IEEE80211_P2P_ATTR_LISTEN_CHANNEL,
    IEEE80211_P2P_ATTR_GROUP_BSSID,
    IEEE80211_P2P_ATTR_EXT_LISTEN_TIMING,
    IEEE80211_P2P_ATTR_INTENDED_IFACE_ADDR,
    IEEE80211_P2P_ATTR_MANAGABILITY,
    IEEE80211_P2P_ATTR_CHANNEL_LIST,
    IEEE80211_P2P_ATTR_ABSENCE_NOTICE,
    IEEE80211_P2P_ATTR_DEVICE_INFO,
    IEEE80211_P2P_ATTR_GROUP_INFO,
    IEEE80211_P2P_ATTR_GROUP_ID,
    IEEE80211_P2P_ATTR_INTERFACE,
    IEEE80211_P2P_ATTR_OPER_CHANNEL,
    IEEE80211_P2P_ATTR_INVITE_FLAGS,
    /* 19 - 220: Reserved */
    IEEE80211_P2P_ATTR_VENDOR_SPECIFIC = 221,

    IEEE80211_P2P_ATTR_MAX
};

/* Notice of Absence attribute - described in P2P spec 4.1.14 */
/* Typical max value used here */
#define IEEE80211_P2P_NOA_DESC_MAX    4

#define IEEE80211_P2P_OPPPS_ENABLE_BIT        BIT(7)
#define IEEE80211_P2P_OPPPS_CTWINDOW_MASK    0x7F

/* 802.11 BAR control masks */
#define IEEE80211_BAR_CTRL_ACK_POLICY_NORMAL    0x0000
#define IEEE80211_BAR_CTRL_MULTI_TID        0x0002
#define IEEE80211_BAR_CTRL_CBMTID_COMPRESSED_BA    0x0004
#define IEEE80211_BAR_CTRL_TID_INFO_MASK    0xf000
#define IEEE80211_BAR_CTRL_TID_INFO_SHIFT    12

/* 802.11n HT capability MSC set */
#define IEEE80211_HT_MCS_RX_HIGHEST_MASK    0x3ff
#define IEEE80211_HT_MCS_TX_DEFINED        0x01
#define IEEE80211_HT_MCS_TX_RX_DIFF        0x02
/* value 0 == 1 stream etc */
#define IEEE80211_HT_MCS_TX_MAX_STREAMS_MASK    0x0C
#define IEEE80211_HT_MCS_TX_MAX_STREAMS_SHIFT    2
#define        IEEE80211_HT_MCS_TX_MAX_STREAMS    4
#define IEEE80211_HT_MCS_TX_UNEQUAL_MODULATION    0x10

/*
 * 802.11n D5.0 20.3.5 / 20.6 says:
 * - indices 0 to 7 and 32 are single spatial stream
 * - 8 to 31 are multiple spatial streams using equal modulation
 *   [8..15 for two streams, 16..23 for three and 24..31 for four]
 * - remainder are multiple spatial streams using unequal modulation
 */
#define IEEE80211_HT_MCS_UNEQUAL_MODULATION_START 33
#define IEEE80211_HT_MCS_UNEQUAL_MODULATION_START_BYTE \
    (IEEE80211_HT_MCS_UNEQUAL_MODULATION_START / 8)

/* 802.11n HT capabilities masks (for cap_info) */
#define IEEE80211_HT_CAP_LDPC_CODING        0x0001
#define IEEE80211_HT_CAP_SUP_WIDTH_20_40    0x0002
#define IEEE80211_HT_CAP_SM_PS            0x000C
#define    IEEE80211_HT_CAP_SM_PS_SHIFT    2
#define IEEE80211_HT_CAP_GRN_FLD        0x0010
#define IEEE80211_HT_CAP_SGI_20            0x0020
#define IEEE80211_HT_CAP_SGI_40            0x0040
#define IEEE80211_HT_CAP_TX_STBC        0x0080
#define IEEE80211_HT_CAP_RX_STBC        0x0300
#define    IEEE80211_HT_CAP_RX_STBC_SHIFT    8
#define IEEE80211_HT_CAP_DELAY_BA        0x0400
#define IEEE80211_HT_CAP_MAX_AMSDU        0x0800
#define IEEE80211_HT_CAP_DSSSCCK40        0x1000
#define IEEE80211_HT_CAP_RESERVED        0x2000
#define IEEE80211_HT_CAP_40MHZ_INTOLERANT    0x4000
#define IEEE80211_HT_CAP_LSIG_TXOP_PROT        0x8000

/* 802.11n HT extended capabilities masks (for extended_ht_cap_info) */
#define IEEE80211_HT_EXT_CAP_PCO        0x0001
#define IEEE80211_HT_EXT_CAP_PCO_TIME        0x0006
#define    IEEE80211_HT_EXT_CAP_PCO_TIME_SHIFT    1
#define IEEE80211_HT_EXT_CAP_MCS_FB        0x0300
#define    IEEE80211_HT_EXT_CAP_MCS_FB_SHIFT    8
#define IEEE80211_HT_EXT_CAP_HTC_SUP        0x0400
#define IEEE80211_HT_EXT_CAP_RD_RESPONDER    0x0800

/* 802.11n HT capability AMPDU settings (for ampdu_params_info) */
#define IEEE80211_HT_AMPDU_PARM_FACTOR        0x03
#define IEEE80211_HT_AMPDU_PARM_DENSITY        0x1C
#define    IEEE80211_HT_AMPDU_PARM_DENSITY_SHIFT    2

/*
 * Maximum length of AMPDU that the STA can receive in high-throughput (HT).
 * Length = 2 ^ (13 + max_ampdu_length_exp) - 1 (octets)
 */
enum ieee80211_max_ampdu_length_exp {
    IEEE80211_HT_MAX_AMPDU_8K = 0,
    IEEE80211_HT_MAX_AMPDU_16K = 1,
    IEEE80211_HT_MAX_AMPDU_32K = 2,
    IEEE80211_HT_MAX_AMPDU_64K = 3
};

/*
 * Maximum length of AMPDU that the STA can receive in VHT.
 * Length = 2 ^ (13 + max_ampdu_length_exp) - 1 (octets)
 */
enum ieee80211_vht_max_ampdu_length_exp {
    IEEE80211_VHT_MAX_AMPDU_8K = 0,
    IEEE80211_VHT_MAX_AMPDU_16K = 1,
    IEEE80211_VHT_MAX_AMPDU_32K = 2,
    IEEE80211_VHT_MAX_AMPDU_64K = 3,
    IEEE80211_VHT_MAX_AMPDU_128K = 4,
    IEEE80211_VHT_MAX_AMPDU_256K = 5,
    IEEE80211_VHT_MAX_AMPDU_512K = 6,
    IEEE80211_VHT_MAX_AMPDU_1024K = 7
};

#define IEEE80211_HT_MAX_AMPDU_FACTOR 13

/* Minimum MPDU start spacing */
enum ieee80211_min_mpdu_spacing {
    IEEE80211_HT_MPDU_DENSITY_NONE = 0,    /* No restriction */
    IEEE80211_HT_MPDU_DENSITY_0_25 = 1,    /* 1/4 usec */
    IEEE80211_HT_MPDU_DENSITY_0_5 = 2,    /* 1/2 usec */
    IEEE80211_HT_MPDU_DENSITY_1 = 3,    /* 1 usec */
    IEEE80211_HT_MPDU_DENSITY_2 = 4,    /* 2 usec */
    IEEE80211_HT_MPDU_DENSITY_4 = 5,    /* 4 usec */
    IEEE80211_HT_MPDU_DENSITY_8 = 6,    /* 8 usec */
    IEEE80211_HT_MPDU_DENSITY_16 = 7    /* 16 usec */
};

/* for ht_param */
#define IEEE80211_HT_PARAM_CHA_SEC_OFFSET        0x03
#define    IEEE80211_HT_PARAM_CHA_SEC_NONE        0x00
#define    IEEE80211_HT_PARAM_CHA_SEC_ABOVE    0x01
#define    IEEE80211_HT_PARAM_CHA_SEC_BELOW    0x03
#define IEEE80211_HT_PARAM_CHAN_WIDTH_ANY        0x04
#define IEEE80211_HT_PARAM_RIFS_MODE            0x08

/* for operation_mode */
#define IEEE80211_HT_OP_MODE_PROTECTION            0x0003
#define    IEEE80211_HT_OP_MODE_PROTECTION_NONE        0
#define    IEEE80211_HT_OP_MODE_PROTECTION_NONMEMBER    1
#define    IEEE80211_HT_OP_MODE_PROTECTION_20MHZ        2
#define    IEEE80211_HT_OP_MODE_PROTECTION_NONHT_MIXED    3
#define IEEE80211_HT_OP_MODE_NON_GF_STA_PRSNT        0x0004
#define IEEE80211_HT_OP_MODE_NON_HT_STA_PRSNT        0x0010

/* for stbc_param */
#define IEEE80211_HT_STBC_PARAM_DUAL_BEACON        0x0040
#define IEEE80211_HT_STBC_PARAM_DUAL_CTS_PROT        0x0080
#define IEEE80211_HT_STBC_PARAM_STBC_BEACON        0x0100
#define IEEE80211_HT_STBC_PARAM_LSIG_TXOP_FULLPROT    0x0200
#define IEEE80211_HT_STBC_PARAM_PCO_ACTIVE        0x0400
#define IEEE80211_HT_STBC_PARAM_PCO_PHASE        0x0800

/* block-ack parameters */
#define IEEE80211_ADDBA_PARAM_POLICY_MASK 0x0002
#define IEEE80211_ADDBA_PARAM_TID_MASK 0x003C
#define IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK 0xFFC0
#define IEEE80211_DELBA_PARAM_TID_MASK 0xF000
#define IEEE80211_DELBA_PARAM_INITIATOR_MASK 0x0800

/* Spatial Multiplexing Power Save Modes (for capability) */
#define WLAN_HT_CAP_SM_PS_STATIC    0
#define WLAN_HT_CAP_SM_PS_DYNAMIC    1
#define WLAN_HT_CAP_SM_PS_INVALID    2
#define WLAN_HT_CAP_SM_PS_DISABLED    3

/* for SM power control field lower two bits */
#define WLAN_HT_SMPS_CONTROL_DISABLED    0
#define WLAN_HT_SMPS_CONTROL_STATIC    1
#define WLAN_HT_SMPS_CONTROL_DYNAMIC    3

/**
 * enum ieee80211_vht_mcs_support - VHT MCS support definitions
 * @IEEE80211_VHT_MCS_SUPPORT_0_7: MCSes 0-7 are supported for the
 *    number of streams
 * @IEEE80211_VHT_MCS_SUPPORT_0_8: MCSes 0-8 are supported
 * @IEEE80211_VHT_MCS_SUPPORT_0_9: MCSes 0-9 are supported
 * @IEEE80211_VHT_MCS_NOT_SUPPORTED: This number of streams isn't supported
 *
 * These definitions are used in each 2-bit subfield of the @rx_mcs_map
 * and @tx_mcs_map fields of &struct ieee80211_vht_mcs_info, which are
 * both split into 8 subfields by number of streams. These values indicate
 * which MCSes are supported for the number of streams the value appears
 * for.
 */
enum ieee80211_vht_mcs_support {
    IEEE80211_VHT_MCS_SUPPORT_0_7    = 0,
    IEEE80211_VHT_MCS_SUPPORT_0_8    = 1,
    IEEE80211_VHT_MCS_SUPPORT_0_9    = 2,
    IEEE80211_VHT_MCS_NOT_SUPPORTED    = 3,
};

/**
 * enum ieee80211_vht_chanwidth - VHT channel width
 * @IEEE80211_VHT_CHANWIDTH_USE_HT: use the HT operation IE to
 *    determine the channel width (20 or 40 MHz)
 * @IEEE80211_VHT_CHANWIDTH_80MHZ: 80 MHz bandwidth
 * @IEEE80211_VHT_CHANWIDTH_160MHZ: 160 MHz bandwidth
 * @IEEE80211_VHT_CHANWIDTH_80P80MHZ: 80+80 MHz bandwidth
 */
enum ieee80211_vht_chanwidth {
    IEEE80211_VHT_CHANWIDTH_USE_HT        = 0,
    IEEE80211_VHT_CHANWIDTH_80MHZ        = 1,
    IEEE80211_VHT_CHANWIDTH_160MHZ        = 2,
    IEEE80211_VHT_CHANWIDTH_80P80MHZ    = 3,
};


/**
 * struct ieee80211_he_cap_elem - HE capabilities element
 *
 * This structure is the "HE capabilities element" fixed fields as
 * described in P802.11ax_D3.0 section 9.4.2.237.2 and 9.4.2.237.3
 */
struct ieee80211_he_cap_elem {
    uint8_t mac_cap_info[6];
    uint8_t phy_cap_info[11];
} __packed;

#define IEEE80211_TX_RX_MCS_NSS_DESC_MAX_LEN    5

/**
 * enum ieee80211_he_mcs_support - HE MCS support definitions
 * @IEEE80211_HE_MCS_SUPPORT_0_7: MCSes 0-7 are supported for the
 *    number of streams
 * @IEEE80211_HE_MCS_SUPPORT_0_9: MCSes 0-9 are supported
 * @IEEE80211_HE_MCS_SUPPORT_0_11: MCSes 0-11 are supported
 * @IEEE80211_HE_MCS_NOT_SUPPORTED: This number of streams isn't supported
 *
 * These definitions are used in each 2-bit subfield of the rx_mcs_*
 * and tx_mcs_* fields of &struct ieee80211_he_mcs_nss_supp, which are
 * both split into 8 subfields by number of streams. These values indicate
 * which MCSes are supported for the number of streams the value appears
 * for.
 */
enum ieee80211_he_mcs_support {
    IEEE80211_HE_MCS_SUPPORT_0_7    = 0,
    IEEE80211_HE_MCS_SUPPORT_0_9    = 1,
    IEEE80211_HE_MCS_SUPPORT_0_11    = 2,
    IEEE80211_HE_MCS_NOT_SUPPORTED    = 3,
};

/**
 * struct ieee80211_he_mcs_nss_supp - HE Tx/Rx HE MCS NSS Support Field
 *
 * This structure holds the data required for the Tx/Rx HE MCS NSS Support Field
 * described in P802.11ax_D2.0 section 9.4.2.237.4
 *
 * @rx_mcs_80: Rx MCS map 2 bits for each stream, total 8 streams, for channel
 *     widths less than 80MHz.
 * @tx_mcs_80: Tx MCS map 2 bits for each stream, total 8 streams, for channel
 *     widths less than 80MHz.
 * @rx_mcs_160: Rx MCS map 2 bits for each stream, total 8 streams, for channel
 *     width 160MHz.
 * @tx_mcs_160: Tx MCS map 2 bits for each stream, total 8 streams, for channel
 *     width 160MHz.
 * @rx_mcs_80p80: Rx MCS map 2 bits for each stream, total 8 streams, for
 *     channel width 80p80MHz.
 * @tx_mcs_80p80: Tx MCS map 2 bits for each stream, total 8 streams, for
 *     channel width 80p80MHz.
 */
struct ieee80211_he_mcs_nss_supp {
    uint16_t rx_mcs_80;
    uint16_t tx_mcs_80;
    uint16_t rx_mcs_160;
    uint16_t tx_mcs_160;
    uint16_t rx_mcs_80p80;
    uint16_t tx_mcs_80p80;
} __packed;

/**
 * struct ieee80211_he_operation - HE capabilities element
 *
 * This structure is the "HE operation element" fields as
 * described in P802.11ax_D2.0 section 9.4.2.238
 */
struct ieee80211_he_operation {
    uint32_t he_oper_params;
    uint16_t he_mcs_nss_set;
    /* Optional 0,1,3 or 4 bytes: depends on @he_oper_params */
    uint8_t optional[0];
} __packed;

/**
 * struct ieee80211_he_mu_edca_param_ac_rec - MU AC Parameter Record field
 *
 * This structure is the "MU AC Parameter Record" fields as
 * described in P802.11ax_D2.0 section 9.4.2.240
 */
struct ieee80211_he_mu_edca_param_ac_rec {
    uint8_t aifsn;
    uint8_t ecw_min_max;
    uint8_t mu_edca_timer;
} __packed;

/**
 * struct ieee80211_mu_edca_param_set - MU EDCA Parameter Set element
 *
 * This structure is the "MU EDCA Parameter Set element" fields as
 * described in P802.11ax_D2.0 section 9.4.2.240
 */
struct ieee80211_mu_edca_param_set {
    uint8_t mu_qos_info;
    struct ieee80211_he_mu_edca_param_ac_rec ac_be;
    struct ieee80211_he_mu_edca_param_ac_rec ac_bk;
    struct ieee80211_he_mu_edca_param_ac_rec ac_vi;
    struct ieee80211_he_mu_edca_param_ac_rec ac_vo;
} __packed;


/* 802.11ac VHT Capabilities */
#define IEEE80211_VHT_CAP_MAX_MPDU_LENGTH_3895            0x00000000
#define IEEE80211_VHT_CAP_MAX_MPDU_LENGTH_7991            0x00000001
#define IEEE80211_VHT_CAP_MAX_MPDU_LENGTH_11454            0x00000002
#define IEEE80211_VHT_CAP_SUPP_CHAN_WIDTH_160MHZ        0x00000004
#define IEEE80211_VHT_CAP_SUPP_CHAN_WIDTH_160_80PLUS80MHZ    0x00000008
#define IEEE80211_VHT_CAP_SUPP_CHAN_WIDTH_MASK            0x0000000C
#define IEEE80211_VHT_CAP_RXLDPC                0x00000010
#define IEEE80211_VHT_CAP_SHORT_GI_80                0x00000020
#define IEEE80211_VHT_CAP_SHORT_GI_160                0x00000040
#define IEEE80211_VHT_CAP_TXSTBC                0x00000080
#define IEEE80211_VHT_CAP_RXSTBC_1                0x00000100
#define IEEE80211_VHT_CAP_RXSTBC_2                0x00000200
#define IEEE80211_VHT_CAP_RXSTBC_3                0x00000300
#define IEEE80211_VHT_CAP_RXSTBC_4                0x00000400
#define IEEE80211_VHT_CAP_RXSTBC_MASK                0x00000700
#define IEEE80211_VHT_CAP_SU_BEAMFORMER_CAPABLE            0x00000800
#define IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE            0x00001000
#define IEEE80211_VHT_CAP_BEAMFORMEE_STS_SHIFT                  13
#define IEEE80211_VHT_CAP_BEAMFORMEE_STS_MASK            \
        (7 << IEEE80211_VHT_CAP_BEAMFORMEE_STS_SHIFT)
#define IEEE80211_VHT_CAP_SOUNDING_DIMENSIONS_SHIFT        16
#define IEEE80211_VHT_CAP_SOUNDING_DIMENSIONS_MASK        \
        (7 << IEEE80211_VHT_CAP_SOUNDING_DIMENSIONS_SHIFT)
#define IEEE80211_VHT_CAP_MU_BEAMFORMER_CAPABLE            0x00080000
#define IEEE80211_VHT_CAP_MU_BEAMFORMEE_CAPABLE            0x00100000
#define IEEE80211_VHT_CAP_VHT_TXOP_PS                0x00200000
#define IEEE80211_VHT_CAP_HTC_VHT                0x00400000
#define IEEE80211_VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_SHIFT    23
#define IEEE80211_VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_MASK    \
        (7 << IEEE80211_VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_SHIFT)
#define IEEE80211_VHT_CAP_VHT_LINK_ADAPTATION_VHT_UNSOL_MFB    0x08000000
#define IEEE80211_VHT_CAP_VHT_LINK_ADAPTATION_VHT_MRQ_MFB    0x0c000000
#define IEEE80211_VHT_CAP_RX_ANTENNA_PATTERN            0x10000000
#define IEEE80211_VHT_CAP_TX_ANTENNA_PATTERN            0x20000000
#define IEEE80211_VHT_CAP_NOT_SUP_WIDTH_80            0x80000000

/* 802.11ax HE MAC capabilities */
#define IEEE80211_HE_MAC_CAP0_HTC_HE                0x01
#define IEEE80211_HE_MAC_CAP0_TWT_REQ                0x02
#define IEEE80211_HE_MAC_CAP0_TWT_RES                0x04
#define IEEE80211_HE_MAC_CAP0_DYNAMIC_FRAG_NOT_SUPP        0x00
#define IEEE80211_HE_MAC_CAP0_DYNAMIC_FRAG_LEVEL_1        0x08
#define IEEE80211_HE_MAC_CAP0_DYNAMIC_FRAG_LEVEL_2        0x10
#define IEEE80211_HE_MAC_CAP0_DYNAMIC_FRAG_LEVEL_3        0x18
#define IEEE80211_HE_MAC_CAP0_DYNAMIC_FRAG_MASK            0x18
#define IEEE80211_HE_MAC_CAP0_MAX_NUM_FRAG_MSDU_1        0x00
#define IEEE80211_HE_MAC_CAP0_MAX_NUM_FRAG_MSDU_2        0x20
#define IEEE80211_HE_MAC_CAP0_MAX_NUM_FRAG_MSDU_4        0x40
#define IEEE80211_HE_MAC_CAP0_MAX_NUM_FRAG_MSDU_8        0x60
#define IEEE80211_HE_MAC_CAP0_MAX_NUM_FRAG_MSDU_16        0x80
#define IEEE80211_HE_MAC_CAP0_MAX_NUM_FRAG_MSDU_32        0xa0
#define IEEE80211_HE_MAC_CAP0_MAX_NUM_FRAG_MSDU_64        0xc0
#define IEEE80211_HE_MAC_CAP0_MAX_NUM_FRAG_MSDU_UNLIMITED    0xe0
#define IEEE80211_HE_MAC_CAP0_MAX_NUM_FRAG_MSDU_MASK        0xe0

#define IEEE80211_HE_MAC_CAP1_MIN_FRAG_SIZE_UNLIMITED        0x00
#define IEEE80211_HE_MAC_CAP1_MIN_FRAG_SIZE_128            0x01
#define IEEE80211_HE_MAC_CAP1_MIN_FRAG_SIZE_256            0x02
#define IEEE80211_HE_MAC_CAP1_MIN_FRAG_SIZE_512            0x03
#define IEEE80211_HE_MAC_CAP1_MIN_FRAG_SIZE_MASK        0x03
#define IEEE80211_HE_MAC_CAP1_TF_MAC_PAD_DUR_0US        0x00
#define IEEE80211_HE_MAC_CAP1_TF_MAC_PAD_DUR_8US        0x04
#define IEEE80211_HE_MAC_CAP1_TF_MAC_PAD_DUR_16US        0x08
#define IEEE80211_HE_MAC_CAP1_TF_MAC_PAD_DUR_MASK        0x0c
#define IEEE80211_HE_MAC_CAP1_MULTI_TID_AGG_RX_QOS_1        0x00
#define IEEE80211_HE_MAC_CAP1_MULTI_TID_AGG_RX_QOS_2        0x10
#define IEEE80211_HE_MAC_CAP1_MULTI_TID_AGG_RX_QOS_3        0x20
#define IEEE80211_HE_MAC_CAP1_MULTI_TID_AGG_RX_QOS_4        0x30
#define IEEE80211_HE_MAC_CAP1_MULTI_TID_AGG_RX_QOS_5        0x40
#define IEEE80211_HE_MAC_CAP1_MULTI_TID_AGG_RX_QOS_6        0x50
#define IEEE80211_HE_MAC_CAP1_MULTI_TID_AGG_RX_QOS_7        0x60
#define IEEE80211_HE_MAC_CAP1_MULTI_TID_AGG_RX_QOS_8        0x70
#define IEEE80211_HE_MAC_CAP1_MULTI_TID_AGG_RX_QOS_MASK        0x70

/* Link adaptation is split between byte HE_MAC_CAP1 and
 * HE_MAC_CAP2. It should be set only if IEEE80211_HE_MAC_CAP0_HTC_HE
 * in which case the following values apply:
 * 0 = No feedback.
 * 1 = reserved.
 * 2 = Unsolicited feedback.
 * 3 = both
 */
#define IEEE80211_HE_MAC_CAP1_LINK_ADAPTATION            0x80

#define IEEE80211_HE_MAC_CAP2_LINK_ADAPTATION            0x01
#define IEEE80211_HE_MAC_CAP2_ALL_ACK                0x02
#define IEEE80211_HE_MAC_CAP2_TRS                0x04
#define IEEE80211_HE_MAC_CAP2_BSR                0x08
#define IEEE80211_HE_MAC_CAP2_BCAST_TWT                0x10
#define IEEE80211_HE_MAC_CAP2_32BIT_BA_BITMAP            0x20
#define IEEE80211_HE_MAC_CAP2_MU_CASCADING            0x40
#define IEEE80211_HE_MAC_CAP2_ACK_EN                0x80

#define IEEE80211_HE_MAC_CAP3_OMI_CONTROL            0x02
#define IEEE80211_HE_MAC_CAP3_OFDMA_RA                0x04

/* The maximum length of an A-MDPU is defined by the combination of the Maximum
 * A-MDPU Length Exponent field in the HT capabilities, VHT capabilities and the
 * same field in the HE capabilities.
 */
#define IEEE80211_HE_MAC_CAP3_MAX_AMPDU_LEN_EXP_USE_VHT    0x00
#define IEEE80211_HE_MAC_CAP3_MAX_AMPDU_LEN_EXP_VHT_1        0x08
#define IEEE80211_HE_MAC_CAP3_MAX_AMPDU_LEN_EXP_VHT_2        0x10
#define IEEE80211_HE_MAC_CAP3_MAX_AMPDU_LEN_EXP_RESERVED    0x18
#define IEEE80211_HE_MAC_CAP3_MAX_AMPDU_LEN_EXP_MASK        0x18
#define IEEE80211_HE_MAC_CAP3_AMSDU_FRAG            0x20
#define IEEE80211_HE_MAC_CAP3_FLEX_TWT_SCHED            0x40
#define IEEE80211_HE_MAC_CAP3_RX_CTRL_FRAME_TO_MULTIBSS        0x80

#define IEEE80211_HE_MAC_CAP4_BSRP_BQRP_A_MPDU_AGG        0x01
#define IEEE80211_HE_MAC_CAP4_QTP                0x02
#define IEEE80211_HE_MAC_CAP4_BQR                0x04
#define IEEE80211_HE_MAC_CAP4_SRP_RESP                0x08
#define IEEE80211_HE_MAC_CAP4_NDP_FB_REP            0x10
#define IEEE80211_HE_MAC_CAP4_OPS                0x20
#define IEEE80211_HE_MAC_CAP4_AMDSU_IN_AMPDU            0x40
/* Multi TID agg TX is split between byte #4 and #5
 * The value is a combination of B39,B40,B41
 */
#define IEEE80211_HE_MAC_CAP4_MULTI_TID_AGG_TX_QOS_B39        0x80

#define IEEE80211_HE_MAC_CAP5_MULTI_TID_AGG_TX_QOS_B40        0x01
#define IEEE80211_HE_MAC_CAP5_MULTI_TID_AGG_TX_QOS_B41        0x02
#define IEEE80211_HE_MAC_CAP5_SUBCHAN_SELECVITE_TRANSMISSION    0x04
#define IEEE80211_HE_MAC_CAP5_UL_2x996_TONE_RU            0x08
#define IEEE80211_HE_MAC_CAP5_OM_CTRL_UL_MU_DATA_DIS_RX        0x10

/* 802.11ax HE PHY capabilities */
#define IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_IN_2G        0x02
#define IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_80MHZ_IN_5G    0x04
#define IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_160MHZ_IN_5G        0x08
#define IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_80PLUS80_MHZ_IN_5G    0x10
#define IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_RU_MAPPING_IN_2G    0x20
#define IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_RU_MAPPING_IN_5G    0x40
#define IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_MASK            0xfe

#define IEEE80211_HE_PHY_CAP1_PREAMBLE_PUNC_RX_80MHZ_ONLY_SECOND_20MHZ    0x01
#define IEEE80211_HE_PHY_CAP1_PREAMBLE_PUNC_RX_80MHZ_ONLY_SECOND_40MHZ    0x02
#define IEEE80211_HE_PHY_CAP1_PREAMBLE_PUNC_RX_160MHZ_ONLY_SECOND_20MHZ    0x04
#define IEEE80211_HE_PHY_CAP1_PREAMBLE_PUNC_RX_160MHZ_ONLY_SECOND_40MHZ    0x08
#define IEEE80211_HE_PHY_CAP1_PREAMBLE_PUNC_RX_MASK            0x0f
#define IEEE80211_HE_PHY_CAP1_DEVICE_CLASS_A                0x10
#define IEEE80211_HE_PHY_CAP1_LDPC_CODING_IN_PAYLOAD            0x20
#define IEEE80211_HE_PHY_CAP1_HE_LTF_AND_GI_FOR_HE_PPDUS_0_8US        0x40
/* Midamble RX/TX Max NSTS is split between byte #2 and byte #3 */
#define IEEE80211_HE_PHY_CAP1_MIDAMBLE_RX_TX_MAX_NSTS            0x80

#define IEEE80211_HE_PHY_CAP2_MIDAMBLE_RX_TX_MAX_NSTS            0x01
#define IEEE80211_HE_PHY_CAP2_NDP_4x_LTF_AND_3_2US            0x02
#define IEEE80211_HE_PHY_CAP2_STBC_TX_UNDER_80MHZ            0x04
#define IEEE80211_HE_PHY_CAP2_STBC_RX_UNDER_80MHZ            0x08
#define IEEE80211_HE_PHY_CAP2_DOPPLER_TX                0x10
#define IEEE80211_HE_PHY_CAP2_DOPPLER_RX                0x20

/* Note that the meaning of UL MU below is different between an AP and a non-AP
 * sta, where in the AP case it indicates support for Rx and in the non-AP sta
 * case it indicates support for Tx.
 */
#define IEEE80211_HE_PHY_CAP2_UL_MU_FULL_MU_MIMO            0x40
#define IEEE80211_HE_PHY_CAP2_UL_MU_PARTIAL_MU_MIMO            0x80

#define IEEE80211_HE_PHY_CAP3_DCM_MAX_CONST_TX_NO_DCM            0x00
#define IEEE80211_HE_PHY_CAP3_DCM_MAX_CONST_TX_BPSK            0x01
#define IEEE80211_HE_PHY_CAP3_DCM_MAX_CONST_TX_QPSK            0x02
#define IEEE80211_HE_PHY_CAP3_DCM_MAX_CONST_TX_16_QAM            0x03
#define IEEE80211_HE_PHY_CAP3_DCM_MAX_CONST_TX_MASK            0x03
#define IEEE80211_HE_PHY_CAP3_DCM_MAX_TX_NSS_1                0x00
#define IEEE80211_HE_PHY_CAP3_DCM_MAX_TX_NSS_2                0x04
#define IEEE80211_HE_PHY_CAP3_DCM_MAX_CONST_RX_NO_DCM            0x00
#define IEEE80211_HE_PHY_CAP3_DCM_MAX_CONST_RX_BPSK            0x08
#define IEEE80211_HE_PHY_CAP3_DCM_MAX_CONST_RX_QPSK            0x10
#define IEEE80211_HE_PHY_CAP3_DCM_MAX_CONST_RX_16_QAM            0x18
#define IEEE80211_HE_PHY_CAP3_DCM_MAX_CONST_RX_MASK            0x18
#define IEEE80211_HE_PHY_CAP3_DCM_MAX_RX_NSS_1                0x00
#define IEEE80211_HE_PHY_CAP3_DCM_MAX_RX_NSS_2                0x20
#define IEEE80211_HE_PHY_CAP3_RX_HE_MU_PPDU_FROM_NON_AP_STA        0x40
#define IEEE80211_HE_PHY_CAP3_SU_BEAMFORMER                0x80

#define IEEE80211_HE_PHY_CAP4_SU_BEAMFORMEE                0x01
#define IEEE80211_HE_PHY_CAP4_MU_BEAMFORMER                0x02

/* Minimal allowed value of Max STS under 80MHz is 3 */
#define IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_UNDER_80MHZ_4        0x0c
#define IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_UNDER_80MHZ_5        0x10
#define IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_UNDER_80MHZ_6        0x14
#define IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_UNDER_80MHZ_7        0x18
#define IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_UNDER_80MHZ_8        0x1c
#define IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_UNDER_80MHZ_MASK    0x1c

/* Minimal allowed value of Max STS above 80MHz is 3 */
#define IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_ABOVE_80MHZ_4        0x60
#define IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_ABOVE_80MHZ_5        0x80
#define IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_ABOVE_80MHZ_6        0xa0
#define IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_ABOVE_80MHZ_7        0xc0
#define IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_ABOVE_80MHZ_8        0xe0
#define IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_ABOVE_80MHZ_MASK    0xe0

#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_UNDER_80MHZ_1    0x00
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_UNDER_80MHZ_2    0x01
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_UNDER_80MHZ_3    0x02
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_UNDER_80MHZ_4    0x03
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_UNDER_80MHZ_5    0x04
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_UNDER_80MHZ_6    0x05
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_UNDER_80MHZ_7    0x06
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_UNDER_80MHZ_8    0x07
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_UNDER_80MHZ_MASK    0x07

#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_ABOVE_80MHZ_1    0x00
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_ABOVE_80MHZ_2    0x08
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_ABOVE_80MHZ_3    0x10
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_ABOVE_80MHZ_4    0x18
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_ABOVE_80MHZ_5    0x20
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_ABOVE_80MHZ_6    0x28
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_ABOVE_80MHZ_7    0x30
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_ABOVE_80MHZ_8    0x38
#define IEEE80211_HE_PHY_CAP5_BEAMFORMEE_NUM_SND_DIM_ABOVE_80MHZ_MASK    0x38

#define IEEE80211_HE_PHY_CAP5_NG16_SU_FEEDBACK                0x40
#define IEEE80211_HE_PHY_CAP5_NG16_MU_FEEDBACK                0x80

#define IEEE80211_HE_PHY_CAP6_CODEBOOK_SIZE_42_SU            0x01
#define IEEE80211_HE_PHY_CAP6_CODEBOOK_SIZE_75_MU            0x02
#define IEEE80211_HE_PHY_CAP6_TRIG_SU_BEAMFORMER_FB            0x04
#define IEEE80211_HE_PHY_CAP6_TRIG_MU_BEAMFORMER_FB            0x08
#define IEEE80211_HE_PHY_CAP6_TRIG_CQI_FB                0x10
#define IEEE80211_HE_PHY_CAP6_PARTIAL_BW_EXT_RANGE            0x20
#define IEEE80211_HE_PHY_CAP6_PARTIAL_BANDWIDTH_DL_MUMIMO        0x40
#define IEEE80211_HE_PHY_CAP6_PPE_THRESHOLD_PRESENT            0x80

#define IEEE80211_HE_PHY_CAP7_SRP_BASED_SR                0x01
#define IEEE80211_HE_PHY_CAP7_POWER_BOOST_FACTOR_AR            0x02
#define IEEE80211_HE_PHY_CAP7_HE_SU_MU_PPDU_4XLTF_AND_08_US_GI        0x04
#define IEEE80211_HE_PHY_CAP7_MAX_NC_1                    0x08
#define IEEE80211_HE_PHY_CAP7_MAX_NC_2                    0x10
#define IEEE80211_HE_PHY_CAP7_MAX_NC_3                    0x18
#define IEEE80211_HE_PHY_CAP7_MAX_NC_4                    0x20
#define IEEE80211_HE_PHY_CAP7_MAX_NC_5                    0x28
#define IEEE80211_HE_PHY_CAP7_MAX_NC_6                    0x30
#define IEEE80211_HE_PHY_CAP7_MAX_NC_7                    0x38
#define IEEE80211_HE_PHY_CAP7_MAX_NC_MASK                0x38
#define IEEE80211_HE_PHY_CAP7_STBC_TX_ABOVE_80MHZ            0x40
#define IEEE80211_HE_PHY_CAP7_STBC_RX_ABOVE_80MHZ            0x80

#define IEEE80211_HE_PHY_CAP8_HE_ER_SU_PPDU_4XLTF_AND_08_US_GI        0x01
#define IEEE80211_HE_PHY_CAP8_20MHZ_IN_40MHZ_HE_PPDU_IN_2G        0x02
#define IEEE80211_HE_PHY_CAP8_20MHZ_IN_160MHZ_HE_PPDU            0x04
#define IEEE80211_HE_PHY_CAP8_80MHZ_IN_160MHZ_HE_PPDU            0x08
#define IEEE80211_HE_PHY_CAP8_HE_ER_SU_1XLTF_AND_08_US_GI        0x10
#define IEEE80211_HE_PHY_CAP8_MIDAMBLE_RX_TX_2X_AND_1XLTF        0x20
#define IEEE80211_HE_PHY_CAP8_DCM_MAX_BW_20MHZ                0x00
#define IEEE80211_HE_PHY_CAP8_DCM_MAX_BW_40MHZ                0x40
#define IEEE80211_HE_PHY_CAP8_DCM_MAX_BW_80MHZ                0x80
#define IEEE80211_HE_PHY_CAP8_DCM_MAX_BW_160_OR_80P80_MHZ        0xc0
#define IEEE80211_HE_PHY_CAP8_DCM_MAX_BW_MASK                0xc0

#define IEEE80211_HE_PHY_CAP9_LONGER_THAN_16_SIGB_OFDM_SYM        0x01
#define IEEE80211_HE_PHY_CAP9_NON_TRIGGERED_CQI_FEEDBACK        0x02
#define IEEE80211_HE_PHY_CAP9_TX_1024_QAM_LESS_THAN_242_TONE_RU        0x04
#define IEEE80211_HE_PHY_CAP9_RX_1024_QAM_LESS_THAN_242_TONE_RU        0x08
#define IEEE80211_HE_PHY_CAP9_RX_FULL_BW_SU_USING_MU_WITH_COMP_SIGB    0x10
#define IEEE80211_HE_PHY_CAP9_RX_FULL_BW_SU_USING_MU_WITH_NON_COMP_SIGB    0x20

/* 802.11ax HE TX/RX MCS NSS Support  */
#define IEEE80211_TX_RX_MCS_NSS_SUPP_HIGHEST_MCS_POS            (3)
#define IEEE80211_TX_RX_MCS_NSS_SUPP_TX_BITMAP_POS            (6)
#define IEEE80211_TX_RX_MCS_NSS_SUPP_RX_BITMAP_POS            (11)
#define IEEE80211_TX_RX_MCS_NSS_SUPP_TX_BITMAP_MASK            0x07c0
#define IEEE80211_TX_RX_MCS_NSS_SUPP_RX_BITMAP_MASK            0xf800

/* TX/RX HE MCS Support field Highest MCS subfield encoding */
enum ieee80211_he_highest_mcs_supported_subfield_enc {
    HIGHEST_MCS_SUPPORTED_MCS7 = 0,
    HIGHEST_MCS_SUPPORTED_MCS8,
    HIGHEST_MCS_SUPPORTED_MCS9,
    HIGHEST_MCS_SUPPORTED_MCS10,
    HIGHEST_MCS_SUPPORTED_MCS11,
};

/* Calculate 802.11ax HE capabilities IE Tx/Rx HE MCS NSS Support Field size */
static inline uint8_t
ieee80211_he_mcs_nss_size(const struct ieee80211_he_cap_elem *he_cap)
{
    uint8_t count = 4;

    if (he_cap->phy_cap_info[0] &
        IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_160MHZ_IN_5G)
        count += 4;

    if (he_cap->phy_cap_info[0] &
        IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_80PLUS80_MHZ_IN_5G)
        count += 4;

    return count;
}

/* 802.11ax HE PPE Thresholds */
#define IEEE80211_PPE_THRES_NSS_SUPPORT_2NSS            (1)
#define IEEE80211_PPE_THRES_NSS_POS                (0)
#define IEEE80211_PPE_THRES_NSS_MASK                (7)
#define IEEE80211_PPE_THRES_RU_INDEX_BITMASK_2x966_AND_966_RU    \
    (BIT(5) | BIT(6))
#define IEEE80211_PPE_THRES_RU_INDEX_BITMASK_MASK        0x78
#define IEEE80211_PPE_THRES_RU_INDEX_BITMASK_POS        (3)
#define IEEE80211_PPE_THRES_INFO_PPET_SIZE            (3)


#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define __const_hweight8(w)             \
        ((unsigned int)                 \
         ((!!((w) & (1ULL << 0))) +     \
          (!!((w) & (1ULL << 1))) +     \
          (!!((w) & (1ULL << 2))) +     \
          (!!((w) & (1ULL << 3))) +     \
          (!!((w) & (1ULL << 4))) +     \
          (!!((w) & (1ULL << 5))) +     \
          (!!((w) & (1ULL << 6))) +     \
          (!!((w) & (1ULL << 7)))))

/*
 * Calculate 802.11ax HE capabilities IE PPE field size
 * Input: Header byte of ppe_thres (first byte), and HE capa IE's PHY cap u8*
 */
static inline uint8_t
ieee80211_he_ppe_size(uint8_t ppe_thres_hdr, const uint8_t *phy_cap_info)
{
    uint8_t n;

    if ((phy_cap_info[6] &
         IEEE80211_HE_PHY_CAP6_PPE_THRESHOLD_PRESENT) == 0)
        return 0;

    n = __const_hweight8(ppe_thres_hdr &
             IEEE80211_PPE_THRES_RU_INDEX_BITMASK_MASK);
    n *= (1 + ((ppe_thres_hdr & IEEE80211_PPE_THRES_NSS_MASK) >>
           IEEE80211_PPE_THRES_NSS_POS));

    /*
     * Each pair is 6 bits, and we need to add the 7 "header" bits to the
     * total size.
     */
    n = (n * IEEE80211_PPE_THRES_INFO_PPET_SIZE * 2) + 7;
    n = DIV_ROUND_UP(n, 8);

    return n;
}

/* HE Operation defines */
#define IEEE80211_HE_OPERATION_BSS_COLOR_MASK            0x0000003f
#define IEEE80211_HE_OPERATION_DFLT_PE_DURATION_MASK        0x000001c0
#define IEEE80211_HE_OPERATION_DFLT_PE_DURATION_OFFSET        6
#define IEEE80211_HE_OPERATION_TWT_REQUIRED            0x00000200
#define IEEE80211_HE_OPERATION_RTS_THRESHOLD_MASK        0x000ffc00
#define IEEE80211_HE_OPERATION_RTS_THRESHOLD_OFFSET        10
#define IEEE80211_HE_OPERATION_PARTIAL_BSS_COLOR        0x00100000
#define IEEE80211_HE_OPERATION_VHT_OPER_INFO            0x00200000
#define IEEE80211_HE_OPERATION_MULTI_BSSID_AP            0x10000000
#define IEEE80211_HE_OPERATION_TX_BSSID_INDICATOR        0x20000000
#define IEEE80211_HE_OPERATION_BSS_COLOR_DISABLED        0x40000000

/*
 * ieee80211_he_oper_size - calculate 802.11ax HE Operations IE size
 * @he_oper_ie: byte data of the He Operations IE, stating from the the byte
 *    after the ext ID byte. It is assumed that he_oper_ie has at least
 *    sizeof(struct ieee80211_he_operation) bytes, checked already in
 *    ieee802_11_parse_elems_crc()
 * @return the actual size of the IE data (not including header), or 0 on error
 */
static inline uint8_t
ieee80211_he_oper_size(const uint8_t *he_oper_ie)
{
    struct ieee80211_he_operation *he_oper = (void *)he_oper_ie;
    uint8_t oper_len = sizeof(struct ieee80211_he_operation);
    uint32_t he_oper_params;

    /* Make sure the input is not NULL */
    if (!he_oper_ie)
        return 0;

    /* Calc required length */
    he_oper_params = le32_to_cpu(he_oper->he_oper_params);
    if (he_oper_params & IEEE80211_HE_OPERATION_VHT_OPER_INFO)
        oper_len += 3;
    if (he_oper_params & IEEE80211_HE_OPERATION_MULTI_BSSID_AP)
        oper_len++;

    /* Add the first byte (extension ID) to the total length */
    oper_len++;

    return oper_len;
}


/* Authentication algorithms */
#define WLAN_AUTH_OPEN 0
#define WLAN_AUTH_SHARED_KEY 1
#define WLAN_AUTH_FT 2
#define WLAN_AUTH_SAE 3
#define WLAN_AUTH_LEAP 128

#define WLAN_AUTH_CHALLENGE_LEN 128

#define WLAN_CAPABILITY_ESS        (1<<0)
#define WLAN_CAPABILITY_IBSS        (1<<1)

/*
 * A mesh STA sets the ESS and IBSS capability bits to zero.
 * however, this holds true for p2p probe responses (in the p2p_find
 * phase) as well.
 */
#define WLAN_CAPABILITY_IS_STA_BSS(cap)    \
    (!((cap) & (WLAN_CAPABILITY_ESS | WLAN_CAPABILITY_IBSS)))

#define WLAN_CAPABILITY_CF_POLLABLE    (1<<2)
#define WLAN_CAPABILITY_CF_POLL_REQUEST    (1<<3)
#define WLAN_CAPABILITY_PRIVACY        (1<<4)
#define WLAN_CAPABILITY_SHORT_PREAMBLE    (1<<5)
#define WLAN_CAPABILITY_PBCC        (1<<6)
#define WLAN_CAPABILITY_CHANNEL_AGILITY    (1<<7)

/* 802.11h */
#define WLAN_CAPABILITY_SPECTRUM_MGMT    (1<<8)
#define WLAN_CAPABILITY_QOS        (1<<9)
#define WLAN_CAPABILITY_SHORT_SLOT_TIME    (1<<10)
#define WLAN_CAPABILITY_APSD        (1<<11)
#define WLAN_CAPABILITY_RADIO_MEASURE    (1<<12)
#define WLAN_CAPABILITY_DSSS_OFDM    (1<<13)
#define WLAN_CAPABILITY_DEL_BACK    (1<<14)
#define WLAN_CAPABILITY_IMM_BACK    (1<<15)

/* measurement */
#define IEEE80211_SPCT_MSR_RPRT_MODE_LATE    (1<<0)
#define IEEE80211_SPCT_MSR_RPRT_MODE_INCAPABLE    (1<<1)
#define IEEE80211_SPCT_MSR_RPRT_MODE_REFUSED    (1<<2)

#define IEEE80211_SPCT_MSR_RPRT_TYPE_BASIC    0
#define IEEE80211_SPCT_MSR_RPRT_TYPE_CCA    1
#define IEEE80211_SPCT_MSR_RPRT_TYPE_RPI    2

/* 802.11g ERP information element */
#define WLAN_ERP_NON_ERP_PRESENT (1<<0)
#define WLAN_ERP_USE_PROTECTION (1<<1)
#define WLAN_ERP_BARKER_PREAMBLE (1<<2)

#define IEEE80211_CCK_RATE_1MB          0x02
#define IEEE80211_CCK_RATE_2MB          0x04
#define IEEE80211_CCK_RATE_5MB          0x0B
#define IEEE80211_CCK_RATE_11MB         0x16
#define IEEE80211_OFDM_RATE_LEN         8
#define IEEE80211_OFDM_RATE_6MB         0x0C
#define IEEE80211_OFDM_RATE_9MB         0x12
#define IEEE80211_OFDM_RATE_12MB        0x18
#define IEEE80211_OFDM_RATE_18MB        0x24
#define IEEE80211_OFDM_RATE_24MB        0x30
#define IEEE80211_OFDM_RATE_36MB        0x48
#define IEEE80211_OFDM_RATE_48MB        0x60
#define IEEE80211_OFDM_RATE_54MB        0x6C
#define IEEE80211_BASIC_RATE_MASK       0x80

/* WLAN_ERP_BARKER_PREAMBLE values */
enum {
    WLAN_ERP_PREAMBLE_SHORT = 0,
    WLAN_ERP_PREAMBLE_LONG = 1,
};

/* Band ID, 802.11ad #8.4.1.45 */
enum {
    IEEE80211_BANDID_TV_WS = 0, /* TV white spaces */
    IEEE80211_BANDID_SUB1  = 1, /* Sub-1 GHz (excluding TV white spaces) */
    IEEE80211_BANDID_2G    = 2, /* 2.4 GHz */
    IEEE80211_BANDID_3G    = 3, /* 3.6 GHz */
    IEEE80211_BANDID_5G    = 4, /* 4.9 and 5 GHz */
    IEEE80211_BANDID_60G   = 5, /* 60 GHz */
};

/* Status codes */
enum ieee80211_statuscode {
    WLAN_STATUS_SUCCESS = 0,
    WLAN_STATUS_UNSPECIFIED_FAILURE = 1,
    WLAN_STATUS_CAPS_UNSUPPORTED = 10,
    WLAN_STATUS_REASSOC_NO_ASSOC = 11,
    WLAN_STATUS_ASSOC_DENIED_UNSPEC = 12,
    WLAN_STATUS_NOT_SUPPORTED_AUTH_ALG = 13,
    WLAN_STATUS_UNKNOWN_AUTH_TRANSACTION = 14,
    WLAN_STATUS_CHALLENGE_FAIL = 15,
    WLAN_STATUS_AUTH_TIMEOUT = 16,
    WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA = 17,
    WLAN_STATUS_ASSOC_DENIED_RATES = 18,
    /* 802.11b */
    WLAN_STATUS_ASSOC_DENIED_NOSHORTPREAMBLE = 19,
    WLAN_STATUS_ASSOC_DENIED_NOPBCC = 20,
    WLAN_STATUS_ASSOC_DENIED_NOAGILITY = 21,
    /* 802.11h */
    WLAN_STATUS_ASSOC_DENIED_NOSPECTRUM = 22,
    WLAN_STATUS_ASSOC_REJECTED_BAD_POWER = 23,
    WLAN_STATUS_ASSOC_REJECTED_BAD_SUPP_CHAN = 24,
    /* 802.11g */
    WLAN_STATUS_ASSOC_DENIED_NOSHORTTIME = 25,
    WLAN_STATUS_ASSOC_DENIED_NODSSSOFDM = 26,
    /* 802.11w */
    WLAN_STATUS_ASSOC_REJECTED_TEMPORARILY = 30,
    WLAN_STATUS_ROBUST_MGMT_FRAME_POLICY_VIOLATION = 31,
    /* 802.11i */
    WLAN_STATUS_INVALID_IE = 40,
    WLAN_STATUS_INVALID_GROUP_CIPHER = 41,
    WLAN_STATUS_INVALID_PAIRWISE_CIPHER = 42,
    WLAN_STATUS_INVALID_AKMP = 43,
    WLAN_STATUS_UNSUPP_RSN_VERSION = 44,
    WLAN_STATUS_INVALID_RSN_IE_CAP = 45,
    WLAN_STATUS_CIPHER_SUITE_REJECTED = 46,
    WLAN_STATUS_EAPOL_FAILED = 128,
    /* 802.11e */
    WLAN_STATUS_UNSPECIFIED_QOS = 32,
    WLAN_STATUS_ASSOC_DENIED_NOBANDWIDTH = 33,
    WLAN_STATUS_ASSOC_DENIED_LOWACK = 34,
    WLAN_STATUS_ASSOC_DENIED_UNSUPP_QOS = 35,
    WLAN_STATUS_REQUEST_DECLINED = 37,
    WLAN_STATUS_INVALID_QOS_PARAM = 38,
    WLAN_STATUS_CHANGE_TSPEC = 39,
    WLAN_STATUS_WAIT_TS_DELAY = 47,
    WLAN_STATUS_NO_DIRECT_LINK = 48,
    WLAN_STATUS_STA_NOT_PRESENT = 49,
    WLAN_STATUS_STA_NOT_QSTA = 50,
    WLAN_STATUS_INVALID_PMKID = 53,
    /* 802.11s */
    WLAN_STATUS_ANTI_CLOG_REQUIRED = 76,
    WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED = 77,
    WLAN_STATUS_STA_NO_TBTT = 78,
    /* 802.11ad */
    WLAN_STATUS_REJECTED_WITH_SUGGESTED_CHANGES = 39,
    WLAN_STATUS_REJECTED_FOR_DELAY_PERIOD = 47,
    WLAN_STATUS_REJECT_WITH_SCHEDULE = 83,
    WLAN_STATUS_PENDING_ADMITTING_FST_SESSION = 86,
    WLAN_STATUS_PERFORMING_FST_NOW = 87,
    WLAN_STATUS_PENDING_GAP_IN_BA_WINDOW = 88,
    WLAN_STATUS_REJECT_U_PID_SETTING = 89,
    WLAN_STATUS_REJECT_DSE_BAND = 96,
    WLAN_STATUS_DENIED_WITH_SUGGESTED_BAND_AND_CHANNEL = 99,
    WLAN_STATUS_DENIED_DUE_TO_SPECTRUM_MANAGEMENT = 103,
    WLAN_STATUS_UNKNOWN_PASSWORD_IDENTIFIER = 123,
    WLAN_STATUS_DENIED_HE_NOT_SUPPORTED = 124,
    WLAN_STATUS_SAE_HASH_TO_ELEMENT = 126,
    WLAN_STATUS_SAE_PK = 127
};

/* Reason codes */
enum ieee80211_reasoncode {
    WLAN_REASON_UNSPECIFIED = 1,
    WLAN_REASON_PREV_AUTH_NOT_VALID = 2,
    WLAN_REASON_DEAUTH_LEAVING = 3,
    WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY = 4,
    WLAN_REASON_DISASSOC_AP_BUSY = 5,
    WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA = 6,
    WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA = 7,
    WLAN_REASON_DISASSOC_STA_HAS_LEFT = 8,
    WLAN_REASON_STA_REQ_ASSOC_WITHOUT_AUTH = 9,
    /* 802.11h */
    WLAN_REASON_DISASSOC_BAD_POWER = 10,
    WLAN_REASON_DISASSOC_BAD_SUPP_CHAN = 11,
    /* 802.11i */
    WLAN_REASON_INVALID_IE = 13,
    WLAN_REASON_MIC_FAILURE = 14,
    WLAN_REASON_4WAY_HANDSHAKE_TIMEOUT = 15,
    WLAN_REASON_GROUP_KEY_HANDSHAKE_TIMEOUT = 16,
    WLAN_REASON_IE_DIFFERENT = 17,
    WLAN_REASON_INVALID_GROUP_CIPHER = 18,
    WLAN_REASON_INVALID_PAIRWISE_CIPHER = 19,
    WLAN_REASON_INVALID_AKMP = 20,
    WLAN_REASON_UNSUPP_RSN_VERSION = 21,
    WLAN_REASON_INVALID_RSN_IE_CAP = 22,
    WLAN_REASON_IEEE8021X_FAILED = 23,
    WLAN_REASON_CIPHER_SUITE_REJECTED = 24,
    /* TDLS (802.11z) */
    WLAN_REASON_TDLS_TEARDOWN_UNREACHABLE = 25,
    WLAN_REASON_TDLS_TEARDOWN_UNSPECIFIED = 26,
    /* 802.11e */
    WLAN_REASON_DISASSOC_UNSPECIFIED_QOS = 32,
    WLAN_REASON_DISASSOC_QAP_NO_BANDWIDTH = 33,
    WLAN_REASON_DISASSOC_LOW_ACK = 34,
    WLAN_REASON_DISASSOC_QAP_EXCEED_TXOP = 35,
    WLAN_REASON_QSTA_LEAVE_QBSS = 36,
    WLAN_REASON_QSTA_NOT_USE = 37,
    WLAN_REASON_QSTA_REQUIRE_SETUP = 38,
    WLAN_REASON_QSTA_TIMEOUT = 39,
    WLAN_REASON_QSTA_CIPHER_NOT_SUPP = 45,
    /* 802.11s */
    WLAN_REASON_MESH_PEER_CANCELED = 52,
    WLAN_REASON_MESH_MAX_PEERS = 53,
    WLAN_REASON_MESH_CONFIG = 54,
    WLAN_REASON_MESH_CLOSE = 55,
    WLAN_REASON_MESH_MAX_RETRIES = 56,
    WLAN_REASON_MESH_CONFIRM_TIMEOUT = 57,
    WLAN_REASON_MESH_INVALID_GTK = 58,
    WLAN_REASON_MESH_INCONSISTENT_PARAM = 59,
    WLAN_REASON_MESH_INVALID_SECURITY = 60,
    WLAN_REASON_MESH_PATH_ERROR = 61,
    WLAN_REASON_MESH_PATH_NOFORWARD = 62,
    WLAN_REASON_MESH_PATH_DEST_UNREACHABLE = 63,
    WLAN_REASON_MAC_EXISTS_IN_MBSS = 64,
    WLAN_REASON_MESH_CHAN_REGULATORY = 65,
    WLAN_REASON_MESH_CHAN = 66,
};

/* Information Element IDs */
enum ieee80211_eid {
    WLAN_EID_SSID = 0,
    WLAN_EID_SUPP_RATES = 1,
    WLAN_EID_FH_PARAMS = 2, /* reserved now */
    WLAN_EID_DS_PARAMS = 3,
    WLAN_EID_CF_PARAMS = 4,
    WLAN_EID_TIM = 5,
    WLAN_EID_IBSS_PARAMS = 6,
    WLAN_EID_COUNTRY = 7,
    WLAN_EID_HP_PARAMS = 8,
    WLAN_EID_HP_TABLE = 9,
    WLAN_EID_REQUEST = 10,
    WLAN_EID_QBSS_LOAD = 11,
    WLAN_EID_EDCA_PARAM_SET = 12,
    WLAN_EID_TSPEC = 13,
    WLAN_EID_TCLAS = 14,
    WLAN_EID_SCHEDULE = 15,
    WLAN_EID_CHALLENGE = 16,
    /* 17-31 reserved for challenge text extension */
    WLAN_EID_PWR_CONSTRAINT = 32,
    WLAN_EID_PWR_CAPABILITY = 33,
    WLAN_EID_TPC_REQUEST = 34,
    WLAN_EID_TPC_REPORT = 35,
    WLAN_EID_SUPPORTED_CHANNELS = 36,
    WLAN_EID_CHANNEL_SWITCH = 37,
    WLAN_EID_MEASURE_REQUEST = 38,
    WLAN_EID_MEASURE_REPORT = 39,
    WLAN_EID_QUIET = 40,
    WLAN_EID_IBSS_DFS = 41,
    WLAN_EID_ERP_INFO = 42,
    WLAN_EID_TS_DELAY = 43,
    WLAN_EID_TCLAS_PROCESSING = 44,
    WLAN_EID_HT_CAPABILITY = 45,
    WLAN_EID_QOS_CAPA = 46,
    /* 47 reserved for Broadcom */
    WLAN_EID_RSN = 48,
    WLAN_EID_802_15_COEX = 49,
    WLAN_EID_EXT_SUPP_RATES = 50,
    WLAN_EID_AP_CHAN_REPORT = 51,
    WLAN_EID_NEIGHBOR_REPORT = 52,
    WLAN_EID_RCPI = 53,
    WLAN_EID_MOBILITY_DOMAIN = 54,
    WLAN_EID_FAST_BSS_TRANSITION = 55,
    WLAN_EID_TIMEOUT_INTERVAL = 56,
    WLAN_EID_RIC_DATA = 57,
    WLAN_EID_DSE_REGISTERED_LOCATION = 58,
    WLAN_EID_SUPPORTED_REGULATORY_CLASSES = 59,
    WLAN_EID_EXT_CHANSWITCH_ANN = 60,
    WLAN_EID_HT_OPERATION = 61,
    WLAN_EID_SECONDARY_CHANNEL_OFFSET = 62,
    WLAN_EID_BSS_AVG_ACCESS_DELAY = 63,
    WLAN_EID_ANTENNA_INFO = 64,
    WLAN_EID_RSNI = 65,
    WLAN_EID_MEASUREMENT_PILOT_TX_INFO = 66,
    WLAN_EID_BSS_AVAILABLE_CAPACITY = 67,
    WLAN_EID_BSS_AC_ACCESS_DELAY = 68,
    WLAN_EID_TIME_ADVERTISEMENT = 69,
    WLAN_EID_RRM_ENABLED_CAPABILITIES = 70,
    WLAN_EID_MULTIPLE_BSSID = 71,
    WLAN_EID_BSS_COEX_2040 = 72,
    WLAN_EID_BSS_INTOLERANT_CHL_REPORT = 73,
    WLAN_EID_OVERLAP_BSS_SCAN_PARAM = 74,
    WLAN_EID_RIC_DESCRIPTOR = 75,
    WLAN_EID_MMIE = 76,
    WLAN_EID_ASSOC_COMEBACK_TIME = 77,
    WLAN_EID_EVENT_REQUEST = 78,
    WLAN_EID_EVENT_REPORT = 79,
    WLAN_EID_DIAGNOSTIC_REQUEST = 80,
    WLAN_EID_DIAGNOSTIC_REPORT = 81,
    WLAN_EID_LOCATION_PARAMS = 82,
    WLAN_EID_NON_TX_BSSID_CAP =  83,
    WLAN_EID_SSID_LIST = 84,
    WLAN_EID_MULTI_BSSID_IDX = 85,
    WLAN_EID_FMS_DESCRIPTOR = 86,
    WLAN_EID_FMS_REQUEST = 87,
    WLAN_EID_FMS_RESPONSE = 88,
    WLAN_EID_QOS_TRAFFIC_CAPA = 89,
    WLAN_EID_BSS_MAX_IDLE_PERIOD = 90,
    WLAN_EID_TSF_REQUEST = 91,
    WLAN_EID_TSF_RESPOSNE = 92,
    WLAN_EID_WNM_SLEEP_MODE = 93,
    WLAN_EID_TIM_BCAST_REQ = 94,
    WLAN_EID_TIM_BCAST_RESP = 95,
    WLAN_EID_COLL_IF_REPORT = 96,
    WLAN_EID_CHANNEL_USAGE = 97,
    WLAN_EID_TIME_ZONE = 98,
    WLAN_EID_DMS_REQUEST = 99,
    WLAN_EID_DMS_RESPONSE = 100,
    WLAN_EID_LINK_ID = 101,
    WLAN_EID_WAKEUP_SCHEDUL = 102,
    /* 103 reserved */
    WLAN_EID_CHAN_SWITCH_TIMING = 104,
    WLAN_EID_PTI_CONTROL = 105,
    WLAN_EID_PU_BUFFER_STATUS = 106,
    WLAN_EID_INTERWORKING = 107,
    WLAN_EID_ADVERTISEMENT_PROTOCOL = 108,
    WLAN_EID_EXPEDITED_BW_REQ = 109,
    WLAN_EID_QOS_MAP_SET = 110,
    WLAN_EID_ROAMING_CONSORTIUM = 111,
    WLAN_EID_EMERGENCY_ALERT = 112,
    WLAN_EID_MESH_CONFIG = 113,
    WLAN_EID_MESH_ID = 114,
    WLAN_EID_LINK_METRIC_REPORT = 115,
    WLAN_EID_CONGESTION_NOTIFICATION = 116,
    WLAN_EID_PEER_MGMT = 117,
    WLAN_EID_CHAN_SWITCH_PARAM = 118,
    WLAN_EID_MESH_AWAKE_WINDOW = 119,
    WLAN_EID_BEACON_TIMING = 120,
    WLAN_EID_MCCAOP_SETUP_REQ = 121,
    WLAN_EID_MCCAOP_SETUP_RESP = 122,
    WLAN_EID_MCCAOP_ADVERT = 123,
    WLAN_EID_MCCAOP_TEARDOWN = 124,
    WLAN_EID_GANN = 125,
    WLAN_EID_RANN = 126,
    WLAN_EID_EXT_CAPABILITY = 127,
    /* 128, 129 reserved for Agere */
    WLAN_EID_PREQ = 130,
    WLAN_EID_PREP = 131,
    WLAN_EID_PERR = 132,
    /* 133-136 reserved for Cisco */
    WLAN_EID_PXU = 137,
    WLAN_EID_PXUC = 138,
    WLAN_EID_AUTH_MESH_PEER_EXCH = 139,
    WLAN_EID_MIC = 140,
    WLAN_EID_DESTINATION_URI = 141,
    WLAN_EID_UAPSD_COEX = 142,
    WLAN_EID_WAKEUP_SCHEDULE = 143,
    WLAN_EID_EXT_SCHEDULE = 144,
    WLAN_EID_STA_AVAILABILITY = 145,
    WLAN_EID_DMG_TSPEC = 146,
    WLAN_EID_DMG_AT = 147,
    WLAN_EID_DMG_CAP = 148,
    /* 149 reserved for Cisco */
    WLAN_EID_CISCO_VENDOR_SPECIFIC = 150,
    WLAN_EID_DMG_OPERATION = 151,
    WLAN_EID_DMG_BSS_PARAM_CHANGE = 152,
    WLAN_EID_DMG_BEAM_REFINEMENT = 153,
    WLAN_EID_CHANNEL_MEASURE_FEEDBACK = 154,
    /* 155-156 reserved for Cisco */
    WLAN_EID_AWAKE_WINDOW = 157,
    WLAN_EID_MULTI_BAND = 158,
    WLAN_EID_ADDBA_EXT = 159,
    WLAN_EID_NEXT_PCP_LIST = 160,
    WLAN_EID_PCP_HANDOVER = 161,
    WLAN_EID_DMG_LINK_MARGIN = 162,
    WLAN_EID_SWITCHING_STREAM = 163,
    WLAN_EID_SESSION_TRANSITION = 164,
    WLAN_EID_DYN_TONE_PAIRING_REPORT = 165,
    WLAN_EID_CLUSTER_REPORT = 166,
    WLAN_EID_RELAY_CAP = 167,
    WLAN_EID_RELAY_XFER_PARAM_SET = 168,
    WLAN_EID_BEAM_LINK_MAINT = 169,
    WLAN_EID_MULTIPLE_MAC_ADDR = 170,
    WLAN_EID_U_PID = 171,
    WLAN_EID_DMG_LINK_ADAPT_ACK = 172,
    /* 173 reserved for Symbol */
    WLAN_EID_MCCAOP_ADV_OVERVIEW = 174,
    WLAN_EID_QUIET_PERIOD_REQ = 175,
    /* 176 reserved for Symbol */
    WLAN_EID_QUIET_PERIOD_RESP = 177,
    /* 178-179 reserved for Symbol */
    /* 180 reserved for ISO/IEC 20011 */
    WLAN_EID_EPAC_POLICY = 182,
    WLAN_EID_CLISTER_TIME_OFF = 183,
    WLAN_EID_INTER_AC_PRIO = 184,
    WLAN_EID_SCS_DESCRIPTOR = 185,
    WLAN_EID_QLOAD_REPORT = 186,
    WLAN_EID_HCCA_TXOP_UPDATE_COUNT = 187,
    WLAN_EID_HL_STREAM_ID = 188,
    WLAN_EID_GCR_GROUP_ADDR = 189,
    WLAN_EID_ANTENNA_SECTOR_ID_PATTERN = 190,
    WLAN_EID_VHT_CAPABILITY = 191,
    WLAN_EID_VHT_OPERATION = 192,
    WLAN_EID_EXTENDED_BSS_LOAD = 193,
    WLAN_EID_WIDE_BW_CHANNEL_SWITCH = 194,
    WLAN_EID_VHT_TX_POWER_ENVELOPE = 195,
    WLAN_EID_CHANNEL_SWITCH_WRAPPER = 196,
    WLAN_EID_AID = 197,
    WLAN_EID_QUIET_CHANNEL = 198,
    WLAN_EID_OPMODE_NOTIF = 199,

    WLAN_EID_VENDOR_SPECIFIC = 221,
    WLAN_EID_QOS_PARAMETER = 222,
    WLAN_EID_RSNX = 244,
    WLAN_EID_EXTENSION = 255,
};

/* Element ID Extension (EID 255) values */
#define WLAN_EID_EXT_ASSOC_DELAY_INFO 1
#define WLAN_EID_EXT_FILS_REQ_PARAMS 2
#define WLAN_EID_EXT_FILS_KEY_CONFIRM 3
#define WLAN_EID_EXT_FILS_SESSION 4
#define WLAN_EID_EXT_FILS_HLP_CONTAINER 5
#define WLAN_EID_EXT_FILS_IP_ADDR_ASSIGN 6
#define WLAN_EID_EXT_KEY_DELIVERY 7
#define WLAN_EID_EXT_FILS_WRAPPED_DATA 8
#define WLAN_EID_EXT_FTM_SYNC_INFO 9
#define WLAN_EID_EXT_EXTENDED_REQUEST 10
#define WLAN_EID_EXT_ESTIMATED_SERVICE_PARAMS 11
#define WLAN_EID_EXT_FILS_PUBLIC_KEY 12
#define WLAN_EID_EXT_FILS_NONCE 13
#define WLAN_EID_EXT_FUTURE_CHANNEL_GUIDANCE 14
#define WLAN_EID_EXT_OWE_DH_PARAM 32
#define WLAN_EID_EXT_PASSWORD_IDENTIFIER 33
#define WLAN_EID_EXT_HE_CAPABILITIES 35
#define WLAN_EID_EXT_HE_OPERATION 36
#define WLAN_EID_EXT_HE_MU_EDCA_PARAMS 38
#define WLAN_EID_EXT_SPATIAL_REUSE 39
#define WLAN_EID_EXT_OCV_OCI 54
#define WLAN_EID_EXT_HE_6GHZ_BAND_CAP 59
#define WLAN_EID_EXT_REJECTED_GROUPS 92
#define WLAN_EID_EXT_ANTI_CLOGGING_TOKEN 93

/* Action category code */
enum ieee80211_category {
    WLAN_CATEGORY_SPECTRUM_MGMT = 0,
    WLAN_CATEGORY_QOS = 1,
    WLAN_CATEGORY_DLS = 2,
    WLAN_CATEGORY_BACK = 3,
    WLAN_CATEGORY_PUBLIC = 4,
    WLAN_CATEGORY_RADIO_MEASUREMENT = 5,
    WLAN_CATEGORY_FAST_BSS = 6,
    WLAN_CATEGORY_HT = 7,
    WLAN_CATEGORY_SA_QUERY = 8,
    WLAN_CATEGORY_PROTECTED_DUAL_OF_ACTION = 9,
    WLAN_CATEGORY_TDLS = 12,
    WLAN_CATEGORY_MESH_ACTION = 13,
    WLAN_CATEGORY_MULTIHOP_ACTION = 14,
    WLAN_CATEGORY_SELF_PROTECTED = 15,
    WLAN_CATEGORY_DMG = 16,
    WLAN_CATEGORY_WMM = 17,
    WLAN_CATEGORY_FST = 18,
    WLAN_CATEGORY_UNPROT_DMG = 20,
    WLAN_CATEGORY_VHT = 21,
    WLAN_CATEGORY_VENDOR_SPECIFIC_PROTECTED = 126,
    WLAN_CATEGORY_VENDOR_SPECIFIC = 127,
};

/* Self Protected Action codes */
enum ieee80211_self_protected_actioncode {
    WLAN_SP_RESERVED = 0,
    WLAN_SP_MESH_PEERING_OPEN = 1,
    WLAN_SP_MESH_PEERING_CONFIRM = 2,
    WLAN_SP_MESH_PEERING_CLOSE = 3,
    WLAN_SP_MGK_INFORM = 4,
    WLAN_SP_MGK_ACK = 5,
};

/* Security key length */
enum ieee80211_key_len {
    WLAN_KEY_LEN_WEP40 = 5,
    WLAN_KEY_LEN_WEP104 = 13,
    WLAN_KEY_LEN_CCMP = 16,
    WLAN_KEY_LEN_TKIP = 32,
    WLAN_KEY_LEN_AES_CMAC = 16,
    WLAN_KEY_LEN_SMS4 = 32,
};

#define IEEE80211_WEP_IV_LEN        4
#define IEEE80211_WEP_ICV_LEN        4
#define IEEE80211_CCMP_HDR_LEN        8
#define IEEE80211_CCMP_MIC_LEN        8
#define IEEE80211_CCMP_PN_LEN        6
#define IEEE80211_TKIP_IV_LEN        8
#define IEEE80211_TKIP_ICV_LEN        4
#define IEEE80211_CMAC_PN_LEN        6

/* Public action codes */
enum ieee80211_pub_actioncode {
    WLAN_PUB_ACTION_EXT_CHANSW_ANN = 4,
    WLAN_PUB_ACTION_TDLS_DISCOVER_RES = 14,
};

/* Extended Channel Switching capability to be set in the 1st byte of
 * the @WLAN_EID_EXT_CAPABILITY information element
 */
#define WLAN_EXT_CAPA1_EXT_CHANNEL_SWITCHING    BIT(2)

/* TDLS capabilities in the the 4th byte of @WLAN_EID_EXT_CAPABILITY */
#define WLAN_EXT_CAPA4_TDLS_BUFFER_STA        BIT(4)
#define WLAN_EXT_CAPA4_TDLS_PEER_PSM        BIT(5)
#define WLAN_EXT_CAPA4_TDLS_CHAN_SWITCH        BIT(6)

/* Interworking capabilities are set in 7th bit of 4th byte of the
 * @WLAN_EID_EXT_CAPABILITY information element
 */
#define WLAN_EXT_CAPA4_INTERWORKING_ENABLED    BIT(7)

/*
 * TDLS capabililites to be enabled in the 5th byte of the
 * @WLAN_EID_EXT_CAPABILITY information element
 */
#define WLAN_EXT_CAPA5_TDLS_ENABLED    BIT(5)
#define WLAN_EXT_CAPA5_TDLS_PROHIBITED    BIT(6)
#define WLAN_EXT_CAPA5_TDLS_CH_SW_PROHIBITED    BIT(7)

#define WLAN_EXT_CAPA8_OPMODE_NOTIF    BIT(6)
#define WLAN_EXT_CAPA8_TDLS_WIDE_BW_ENABLED    BIT(7)

/* TDLS specific payload type in the LLC/SNAP header */
#define WLAN_TDLS_SNAP_RFTYPE    0x2

/* BSS Coex IE information field bits */
#define WLAN_BSS_COEX_INFORMATION_REQUEST    BIT(0)

/**
 * enum - mesh synchronization method identifier
 *
 * @IEEE80211_SYNC_METHOD_NEIGHBOR_OFFSET: the default synchronization method
 * @IEEE80211_SYNC_METHOD_VENDOR: a vendor specific synchronization method
 *    that will be specified in a vendor specific information element
 */
enum {
    IEEE80211_SYNC_METHOD_NEIGHBOR_OFFSET = 1,
    IEEE80211_SYNC_METHOD_VENDOR = 255,
};

/**
 * enum - mesh path selection protocol identifier
 *
 * @IEEE80211_PATH_PROTOCOL_HWMP: the default path selection protocol
 * @IEEE80211_PATH_PROTOCOL_VENDOR: a vendor specific protocol that will
 *    be specified in a vendor specific information element
 */
enum {
    IEEE80211_PATH_PROTOCOL_HWMP = 1,
    IEEE80211_PATH_PROTOCOL_VENDOR = 255,
};

/**
 * enum - mesh path selection metric identifier
 *
 * @IEEE80211_PATH_METRIC_AIRTIME: the default path selection metric
 * @IEEE80211_PATH_METRIC_VENDOR: a vendor specific metric that will be
 *    specified in a vendor specific information element
 */
enum {
    IEEE80211_PATH_METRIC_AIRTIME = 1,
    IEEE80211_PATH_METRIC_VENDOR = 255,
};

/**
 * enum ieee80211_root_mode_identifier - root mesh STA mode identifier
 *
 * These attribute are used by dot11MeshHWMPRootMode to set root mesh STA mode
 *
 * @IEEE80211_ROOTMODE_NO_ROOT: the mesh STA is not a root mesh STA (default)
 * @IEEE80211_ROOTMODE_ROOT: the mesh STA is a root mesh STA if greater than
 *    this value
 * @IEEE80211_PROACTIVE_PREQ_NO_PREP: the mesh STA is a root mesh STA supports
 *    the proactive PREQ with proactive PREP subfield set to 0
 * @IEEE80211_PROACTIVE_PREQ_WITH_PREP: the mesh STA is a root mesh STA
 *    supports the proactive PREQ with proactive PREP subfield set to 1
 * @IEEE80211_PROACTIVE_RANN: the mesh STA is a root mesh STA supports
 *    the proactive RANN
 */
enum ieee80211_root_mode_identifier {
    IEEE80211_ROOTMODE_NO_ROOT = 0,
    IEEE80211_ROOTMODE_ROOT = 1,
    IEEE80211_PROACTIVE_PREQ_NO_PREP = 2,
    IEEE80211_PROACTIVE_PREQ_WITH_PREP = 3,
    IEEE80211_PROACTIVE_RANN = 4,
};

/*
 * IEEE 802.11-2007 7.3.2.9 Country information element
 *
 * Minimum length is 8 octets, ie len must be evenly
 * divisible by 2
 */

/* Although the spec says 8 I'm seeing 6 in practice */
#define IEEE80211_COUNTRY_IE_MIN_LEN    6

/* The Country String field of the element shall be 3 octets in length */
#define IEEE80211_COUNTRY_STRING_LEN    3

/*
 * For regulatory extension stuff see IEEE 802.11-2007
 * Annex I (page 1141) and Annex J (page 1147). Also
 * review 7.3.2.9.
 *
 * When dot11RegulatoryClassesRequired is true and the
 * first_channel/reg_extension_id is >= 201 then the IE
 * compromises of the 'ext' struct represented below:
 *
 *  - Regulatory extension ID - when generating IE this just needs
 *    to be monotonically increasing for each triplet passed in
 *    the IE
 *  - Regulatory class - index into set of rules
 *  - Coverage class - index into air propagation time (Table 7-27),
 *    in microseconds, you can compute the air propagation time from
 *    the index by multiplying by 3, so index 10 yields a propagation
 *    of 10 us. Valid values are 0-31, values 32-255 are not defined
 *    yet. A value of 0 inicates air propagation of <= 1 us.
 *
 *  See also Table I.2 for Emission limit sets and table
 *  I.3 for Behavior limit sets. Table J.1 indicates how to map
 *  a reg_class to an emission limit set and behavior limit set.
 */
#define IEEE80211_COUNTRY_EXTENSION_ID 201

enum ieee80211_timeout_interval_type {
    WLAN_TIMEOUT_REASSOC_DEADLINE = 1 /* 802.11r */,
    WLAN_TIMEOUT_KEY_LIFETIME = 2 /* 802.11r */,
    WLAN_TIMEOUT_ASSOC_COMEBACK = 3 /* 802.11w */,
};

/* BACK action code */
enum ieee80211_back_actioncode {
    WLAN_ACTION_ADDBA_REQ = 0,
    WLAN_ACTION_ADDBA_RESP = 1,
    WLAN_ACTION_DELBA = 2,
};

/* BACK (block-ack) parties */
enum ieee80211_back_parties {
    WLAN_BACK_RECIPIENT = 0,
    WLAN_BACK_INITIATOR = 1,
};

/* SA Query action */
enum ieee80211_sa_query_action {
    WLAN_ACTION_SA_QUERY_REQUEST = 0,
    WLAN_ACTION_SA_QUERY_RESPONSE = 1,
};

/* cipher suite selectors */
#define WLAN_CIPHER_SUITE_USE_GROUP    0x000FAC00
#define WLAN_CIPHER_SUITE_WEP40        0x000FAC01
#define WLAN_CIPHER_SUITE_TKIP        0x000FAC02
/* reserved:                 0x000FAC03 */
#define WLAN_CIPHER_SUITE_CCMP        0x000FAC04
#define WLAN_CIPHER_SUITE_WEP104    0x000FAC05
#define WLAN_CIPHER_SUITE_AES_CMAC    0x000FAC06
#define WLAN_CIPHER_SUITE_GCMP        0x000FAC08

#define WLAN_CIPHER_SUITE_SMS4        0x00147201

/* AKM suite selectors */
#define WLAN_AKM_SUITE_8021X        0x000FAC01
#define WLAN_AKM_SUITE_PSK        0x000FAC02
#define WLAN_AKM_SUITE_8021X_SHA256    0x000FAC05
#define WLAN_AKM_SUITE_PSK_SHA256    0x000FAC06
#define WLAN_AKM_SUITE_TDLS        0x000FAC07
#define WLAN_AKM_SUITE_SAE        0x000FAC08
#define WLAN_AKM_SUITE_FT_OVER_SAE    0x000FAC09

#define WLAN_MAX_KEY_LEN        32

#define WLAN_PMKID_LEN            16

#define WLAN_OUI_WFA            0x506f9a
#define WLAN_OUI_TYPE_WFA_P2P        9
#define WLAN_OUI_MICROSOFT        0x0050f2
#define WLAN_OUI_TYPE_MICROSOFT_WPA    1
#define WLAN_OUI_TYPE_MICROSOFT_WMM    2
#define WLAN_OUI_TYPE_MICROSOFT_WPS    4

/*
 * WMM/802.11e Tspec Element
 */
#define IEEE80211_WMM_IE_TSPEC_TID_MASK        0x0F
#define IEEE80211_WMM_IE_TSPEC_TID_SHIFT    1

enum ieee80211_tspec_status_code {
    IEEE80211_TSPEC_STATUS_ADMISS_ACCEPTED = 0,
    IEEE80211_TSPEC_STATUS_ADDTS_INVAL_PARAMS = 0x1,
};
#endif /* _UWIFI_IEEE80211_H */
