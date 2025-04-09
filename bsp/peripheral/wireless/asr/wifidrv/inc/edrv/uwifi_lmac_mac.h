/**
 ****************************************************************************************
 *
 * @file uwifi_lmac_mac.h
 *
 * @brief MAC related definitions.
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */

#ifndef _UWIFI_LMAC_H_
#define _UWIFI_LMAC_H_
#include <stdint.h>

/**
 ****************************************************************************************
 * @defgroup MAC MAC
 * @ingroup COMMON
 * @brief  Common defines,structures
 *
 * This module contains defines commonaly used for MAC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */
/// duration of a Time Unit in microseconds
#define TU_DURATION                     1024

/// max number of channels in the 2.4 GHZ band
#define MAC_DOMAINCHANNEL_24G_MAX       14

/// max number of channels in the 5 GHZ band
#define MAC_DOMAINCHANNEL_5G_MAX        45

/// Mask to test if it's a basic rate - BIT(7)
#define MAC_BASIC_RATE                  0x80
/// Mask for extracting/checking word alignment
#define WORD_ALIGN                      3

#define MAX_AMSDU_LENGTH                7935

/*
 * MACRO DEFINITIONS
 ****************************************************************************************
 */

/// MAC address length in bytes.
#define MAC_ADDR_LEN 6

/// MAC RATE-SET
#define MAC_OFDM_PHY_RATESET_LEN    8
#define MAC_EXT_RATES_OFF      8

/// IV/EIV data
#define MAC_IV_LEN  4
#define MAC_EIV_LEN 4
struct rx_seciv
{
    uint8_t iv[MAC_IV_LEN];
    uint8_t ext_iv[MAC_EIV_LEN];
};

struct mac_mcsset
{
    uint8_t length;
    uint8_t array[MAX_MCS_LEN];
};

/// MAC Secret Key
#define MAC_WEP_KEY_CNT          4  // Number of WEP keys per virtual device
#define MAC_WEP_KEY_LEN         13  // Max size of a WEP key (104/8 = 13)
struct mac_wep_key
{
    uint8_t array[MAC_WEP_KEY_LEN]; // Key material
};

/// MAC channel list
/// @todo: fix that number
#define MAC_MAX_CH 40
struct mac_ch_list
{
    /// Number of channels in channel list.
    uint16_t nbr;
    /// List of the channels.
    uint8_t list[MAC_MAX_CH];
};

struct mac_country_subband
{
    // First channel number of the triplet.
    uint8_t first_chn;
    // Max number of channel number for the triplet.
    uint8_t nbr_of_chn;
    // Maximum allowed transmit power.
    uint8_t max_tx_power;
};

#define MAX_COUNTRY_LEN         3
#define MAX_COUNTRY_SUBBAND     5
struct mac_country
{
    // Length of the country string
    uint8_t length;
    // Country  string 2 char.
    uint8_t string[MAX_COUNTRY_LEN];
    // channel info triplet
    struct mac_country_subband subband[MAX_COUNTRY_SUBBAND];
};

/// MAC QOS CAPABILITY
struct mac_qoscapability
{
    uint8_t  qos_info;
};

/// RSN information element
#define MAC_RAW_RSN_IE_LEN 34
struct mac_raw_rsn_ie
{
    uint8_t data[2 + MAC_RAW_RSN_IE_LEN];
};

#define MAC_RAW_ENC_LEN 0x1A
struct mac_wpa_frame
{
    uint8_t array[MAC_RAW_ENC_LEN];
};

#define MAC_WME_PARAM_LEN          16
struct mac_wmm_frame
{
    uint8_t array [MAC_WME_PARAM_LEN];
};

/// BSS load element
struct mac_bss_load
{
    uint16_t sta_cnt;
    uint8_t  ch_utilization;
    uint16_t avail_adm_capacity;
};

struct mac_twenty_fourty_bss
{
    uint8_t bss_coexistence;
};

/// MAC BA PARAMETERS
struct mac_ba_param
{
    struct mac_addr   peer_sta_address;     ///< Peer STA MAC Address to which BA is Setup
    uint16_t          buffer_size;          ///< Number of buffers available for this BA
    uint16_t          start_sequence_number;///< Start Sequence Number of BA
    uint16_t          ba_timeout;           ///< BA Setup timeout value
    uint8_t           dev_type;             ///< BA Device Type Originator/Responder
    uint8_t           block_ack_policy;     ///< BLOCK-ACK Policy Setup Immedaite/Delayed
    uint8_t           buffer_cnt;           ///< Number of buffers required for BA Setup
};

/// MAC TS INFO field
struct mac_ts_info
{
    uint8_t   traffic_type;
    uint8_t   ack_policy;
    uint8_t   access_policy;
    uint8_t   dir;
    uint8_t   tsid;
    uint8_t   user_priority;
    bool      aggregation;
    bool      apsd;
    bool      schedule;
};

/// MAC TSPEC PARAMETERS
struct mac_tspec_param
{
    struct mac_ts_info ts_info;
    uint16_t  nominal_msdu_size;
    uint16_t  max_msdu_size;
    uint32_t  min_service_interval;
    uint32_t  max_service_interval;
    uint32_t  inactivity_interval;
    uint32_t  short_inactivity_interval;
    uint32_t  service_start_time;
    uint32_t  max_burst_size;
    uint32_t  min_data_rate;
    uint32_t  mean_data_rate;
    uint32_t  min_phy_rate;
    uint32_t  peak_data_rate;
    uint32_t  delay_bound;
    uint16_t  medium_time;
    uint8_t   surplusbwallowance;
};

/// Structure containing the information required to perform a measurement request
struct mac_request_set
{

    uint8_t             mode;       ///<As specified by standard
    uint8_t             type;       ///< 0: Basic request, 1: CCA request, 2: RPI histogram request
    uint16_t            duration;   ///< In TU
    uint64_t            start_time; ///< TSF time
    uint8_t             ch_number;  ///< channel to be measured
};

/// Structure containing the information returned from a measurement process
struct mac_report_set
{
    uint8_t             mode;       ///<As specified by standard
    uint8_t             type;       ///< 0: Basic request, 1: CCA request, 2: RPI histogram request
    uint16_t            duration;   ///< In TU
    uint64_t            start_time; ///< TSF time
    uint8_t             ch_number;  ///< channel to be measured
    uint8_t             map;        ///< As specified by standard
    uint8_t             cca_busy_fraction;  ///<As specified by standard
    uint8_t             rpi_histogram[8];   ///<As specified by standard
};

/// Structure containing the MAC SW and MAC HW version information
struct mac_version
{
    char mac_sw_version[16];
    char mac_sw_version_date[48];
    char mac_sw_build_date[48];
    uint32_t mac_hw_version1;
    uint32_t mac_hw_version2;
};

/// Structure containing some of the properties of a BSS. @todo Add required fields during
/// AP/IBSS mode implementation
struct mac_bss_conf
{
    /// Flags (ERP, QoS, etc.).
    uint32_t flags;
    /// Beacon period
    uint16_t beacon_period;
};

/*
* GLOBAL VARIABLES
****************************************************************************************
*/
extern const uint8_t mac_tid2ac[];

extern const uint8_t mac_id2rate[];

extern const uint16_t mac_mcs_params_20[];

extern const uint16_t mac_mcs_params_40[];

/// @}

#endif // _UWIFI_LMAC_H_