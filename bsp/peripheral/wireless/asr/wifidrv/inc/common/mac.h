/**
 ****************************************************************************************
 *
 * @file mac.h
 *
 * @brief MAC related definitions.
 *
 * Copyright (C) ASR 2011-2016
 *
 ****************************************************************************************
 */

#ifndef _MAC_H_
#define _MAC_H_

/**
 ****************************************************************************************
 * @defgroup CO_MAC CO_MAC
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
// standard includes
// for memcmp
#include <string.h>
#include "asr_types.h"


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

/*
 * MACRO DEFINITIONS
 ****************************************************************************************
 */

/// Access Category enumeration
enum
{
    /// Background
    AC_BK = 0,
    /// Best-effort
    AC_BE,
    /// Video
    AC_VI,
    /// Voice
    AC_VO,
    /// Number of access categories
    AC_MAX
};

/// Traffic ID enumeration
enum
{
    /// TID_0. Mapped to @ref AC_BE as per 802.11 standard.
    TID_0,
    /// TID_1. Mapped to @ref AC_BK as per 802.11 standard.
    TID_1,
    /// TID_2. Mapped to @ref AC_BK as per 802.11 standard.
    TID_2,
    /// TID_3. Mapped to @ref AC_BE as per 802.11 standard.
    TID_3,
    /// TID_4. Mapped to @ref AC_VI as per 802.11 standard.
    TID_4,
    /// TID_5. Mapped to @ref AC_VI as per 802.11 standard.
    TID_5,
    /// TID_6. Mapped to @ref AC_VO as per 802.11 standard.
    TID_6,
    /// TID_7. Mapped to @ref AC_VO as per 802.11 standard.
    TID_7,
    /// Non standard Management TID used internally
    TID_MGT,
    /// Number of TID supported
    TID_MAX
};

/// SCAN type
enum
{
    /// Passive scanning
    SCAN_PASSIVE,
    /// Active scanning
    SCAN_ACTIVE
};

/// Legacy rate 802.11 definitions
enum
{
    /// DSSS/CCK 1Mbps
    MAC_RATE_1MBPS   =   2,
    /// DSSS/CCK 2Mbps
    MAC_RATE_2MBPS   =   4,
    /// DSSS/CCK 5.5Mbps
    MAC_RATE_5_5MBPS =  11,
    /// OFDM 6Mbps
    MAC_RATE_6MBPS   =  12,
    /// OFDM 9Mbps
    MAC_RATE_9MBPS   =  18,
    /// DSSS/CCK 11Mbps
    MAC_RATE_11MBPS  =  22,
    /// OFDM 12Mbps
    MAC_RATE_12MBPS  =  24,
    /// OFDM 18Mbps
    MAC_RATE_18MBPS  =  36,
    /// OFDM 24Mbps
    MAC_RATE_24MBPS  =  48,
    /// OFDM 36Mbps
    MAC_RATE_36MBPS  =  72,
    /// OFDM 48Mbps
    MAC_RATE_48MBPS  =  96,
    /// OFDM 54Mbps
    MAC_RATE_54MBPS  = 108
};

/**
 ****************************************************************************************
 * Compare two MAC addresses.
 * The MAC addresses MUST be 16 bit aligned.
 * @param[in] addr1_ptr Pointer to the first MAC address.
 * @param[in] addr2_ptr Pointer to the second MAC address.
 * @return True if equal, false if not.
 ****************************************************************************************
 */
#define MAC_ADDR_CMP(addr1_ptr, addr2_ptr)                                              \
    ((*(((uint16_t*)(addr1_ptr)) + 0) == *(((uint16_t*)(addr2_ptr)) + 0)) &&            \
     (*(((uint16_t*)(addr1_ptr)) + 1) == *(((uint16_t*)(addr2_ptr)) + 1)) &&            \
     (*(((uint16_t*)(addr1_ptr)) + 2) == *(((uint16_t*)(addr2_ptr)) + 2)))

/**
 ****************************************************************************************
 * Compare two MAC addresses whose alignment is not known.
 * @param[in] __a1 Pointer to the first MAC address.
 * @param[in] __a2 Pointer to the second MAC address.
 * @return True if equal, false if not.
 ****************************************************************************************
 */
#define MAC_ADDR_CMP_PACKED(__a1, __a2)                                                 \
    (memcmp(__a1, __a2, MAC_ADDR_LEN) == 0)

/**
 ****************************************************************************************
 * Copy a MAC address.
 * The MAC addresses MUST be 16 bit aligned.
 * @param[in] addr1_ptr Pointer to the destination MAC address.
 * @param[in] addr2_ptr Pointer to the source MAC address.
 ****************************************************************************************
 */
#define MAC_ADDR_CPY(addr1_ptr, addr2_ptr)                                              \
    *(((uint16_t*)(addr1_ptr)) + 0) = *(((uint16_t*)(addr2_ptr)) + 0);                  \
    *(((uint16_t*)(addr1_ptr)) + 1) = *(((uint16_t*)(addr2_ptr)) + 1);                  \
    *(((uint16_t*)(addr1_ptr)) + 2) = *(((uint16_t*)(addr2_ptr)) + 2)

/**
 ****************************************************************************************
 * Compare two SSID.
 * @param[in] ssid1_ptr Pointer to the first SSID structure.
 * @param[in] ssid2_ptr Pointer to the second SSID structure.
 * @return True if equal, false if not.
 ****************************************************************************************
 */
#define MAC_SSID_CMP(ssid1_ptr,ssid2_ptr)                                               \
    (((ssid1_ptr)->length == (ssid2_ptr)->length) &&                                    \
     (memcmp((&(ssid1_ptr)->array[0]), (&(ssid2_ptr)->array[0]), (ssid1_ptr)->length) == 0))

/**
 ****************************************************************************************
 * Check if MAC address is a group address: test the multicast bit.
 * @param[in] mac_addr_ptr Pointer to the MAC address to be checked.
 * @return 0 if unicast address, 1 otherwise
 ****************************************************************************************
 */
#define MAC_ADDR_GROUP(mac_addr_ptr) ((*((uint8_t *)(mac_addr_ptr))) & 1)

/// MAC address length in bytes.
#define MAC_ADDR_LEN 6

/// MAC address structure.
struct mac_addr
{
    /// Array of 16-bit words that make up the MAC address.
    uint16_t array[MAC_ADDR_LEN/2];
};

/// SSID maximum length.
#define MAC_SSID_LEN 32

/// SSID.
struct mac_ssid
{
    /// Actual length of the SSID.
    uint8_t length;
    /// Array containing the SSID name.
    uint8_t array[MAC_SSID_LEN];
};

/// MAC rateset maximum length
#define MAC_RATESET_LEN             12
/// Maximum length of the supported rates IE
#define MAC_SUPP_RATES_IE_LEN       8

/// Structure containing the asrcy rateset of a station
struct mac_rateset
{
    /// Number of asrcy rates supported
    uint8_t     length;
    /// Array of asrcy rates
    uint8_t     array[MAC_RATESET_LEN];
};

/// HT MCS number of words
#define MAC_MCS_WORD_CNT            3
/// Structure containing the HT MCS supported by a station
struct mac_rates
{
    /// MCS 0 to 76
    uint32_t mcs[MAC_MCS_WORD_CNT];
    /// Legacy rates (1Mbps to 54Mbps)
    uint16_t asrcy;
};

/// Max number of default keys for given VIF
#define MAC_DEFAULT_KEY_COUNT 4
/// Max number of MFP key for given VIF
#define MAC_DEFAULT_MFP_KEY_COUNT 6

/// Structure containing the information about a key
struct key_info_tag
{
    /// Replay counters for RX
    uint64_t rx_pn[TID_MAX];
    /// Replay counter for TX
    uint64_t tx_pn;
    /// Union of MIC and MFP keys
    union
    {
        struct
        {
            /// TX MIC key
            uint32_t tx_key[2];
            /// RX MIC key
            uint32_t rx_key[2];
        } mic;
        struct
        {
            uint32_t key[4];
        } mfp;
    }u;
    /// Type of security currently used
    uint8_t cipher;
    /// Key index as specified by the authenticator/supplicant
    uint8_t key_idx;
    /// Index of the key in the HW memory
    uint8_t hw_key_idx;
    /// Flag indicating if the key is valid
    bool valid;
};

/// MAC Security Key maximum length
#define MAC_SEC_KEY_LEN         32  // TKIP keys 256 bits (max length) with MIC keys

/// Structure defining a security key
struct mac_sec_key
{
    /// Key material length
    uint8_t length;
    /// Key material
    uint32_t array[MAC_SEC_KEY_LEN/4];
};

/// MCS bitfield maximum size (in bytes)
#define MAX_MCS_LEN 16 // 16 * 8 = 128
/// MAC HT capability information element
struct mac_htcapability
{
    /// HT capability information
    uint16_t         ht_capa_info;
    /// A-MPDU parameters
    uint8_t          a_mpdu_param;
    /// Supported MCS
    uint8_t          mcs_rate[MAX_MCS_LEN];
    /// HT extended capability information
    uint16_t         ht_extended_capa;
    /// Beamforming capability information
    uint32_t         tx_beamforming_capa;
    /// Antenna selection capability information
    uint8_t          asel_capa;
};


/// MAC VHT capability information element
struct mac_vhtcapability
{
    /// VHT capability information
    uint32_t         vht_capa_info;
    /// RX MCS map
    uint16_t         rx_mcs_map;
    /// RX highest data rate
    uint16_t         rx_highest;
    /// TX MCS map
    uint16_t         tx_mcs_map;
    /// TX highest data rate
    uint16_t         tx_highest;
};

/// Length (in bytes) of the MAC HE capability field
#define MAC_HE_MAC_CAPA_LEN 6
/// Length (in bytes) of the PHY HE capability field
#define MAC_HE_PHY_CAPA_LEN 11
/// Maximum length (in bytes) of the PPE threshold data
#define MAC_HE_PPE_THRES_MAX_LEN 25

/// Structure listing the per-NSS, per-BW supported MCS combinations
struct mac_he_mcs_nss_supp {
    /// per-NSS supported MCS in RX, for BW <= 80MHz
    uint16_t rx_mcs_80;
    /// per-NSS supported MCS in TX, for BW <= 80MHz
    uint16_t tx_mcs_80;
    /// per-NSS supported MCS in RX, for BW = 160MHz
    uint16_t rx_mcs_160;
    /// per-NSS supported MCS in TX, for BW = 160MHz
    uint16_t tx_mcs_160;
    /// per-NSS supported MCS in RX, for BW = 80+80MHz
    uint16_t rx_mcs_80p80;
    /// per-NSS supported MCS in TX, for BW = 80+80MHz
    uint16_t tx_mcs_80p80;
};

/// MAC HE capability information element
struct mac_hecapability {
    /// MAC HE capabilities
    uint8_t mac_cap_info[MAC_HE_MAC_CAPA_LEN];
    /// PHY HE capabilities
    uint8_t phy_cap_info[MAC_HE_PHY_CAPA_LEN];
    /// Supported MCS combinations
    struct mac_he_mcs_nss_supp mcs_supp;
    /// PPE Thresholds data
    uint8_t ppe_thres[MAC_HE_PPE_THRES_MAX_LEN];
};


/// MAC HT operation element
struct mac_htoprnelmt
{
    /// Primary channel information
    uint8_t     prim_channel;
    /// HT operation information 1
    uint8_t     ht_oper_1;
    /// HT operation information 2
    uint16_t    ht_oper_2;
    /// HT operation information 3
    uint16_t    ht_oper_3;
    /// Basic MCS set
    uint8_t     mcs_rate[MAX_MCS_LEN];

};

///EDCA Parameter Set Element
struct  mac_edca_param_set
{
    /// QoS information
    uint8_t         qos_info;
    /// Admission Control Mandatory bitfield
    uint8_t         acm;
    /// Per-AC EDCA parameters
    uint32_t        ac_param[AC_MAX];
};

/// UAPSD enabled on VO
#define MAC_QOS_INFO_STA_UAPSD_ENABLED_VO      CO_BIT(0)
/// UAPSD enabled on VI
#define MAC_QOS_INFO_STA_UAPSD_ENABLED_VI      CO_BIT(1)
/// UAPSD enabled on BK
#define MAC_QOS_INFO_STA_UAPSD_ENABLED_BK      CO_BIT(2)
/// UAPSD enabled on BE
#define MAC_QOS_INFO_STA_UAPSD_ENABLED_BE      CO_BIT(3)
/// UAPSD enabled on all access categories
#define MAC_QOS_INFO_STA_UAPSD_ENABLED_ALL     0x0F
/// UAPSD enabled in AP
#define MAC_QOS_INFO_AP_UAPSD_ENABLED          CO_BIT(7)

/// Scan result element, parsed from beacon or probe response frames.
struct mac_scan_result
{
    /// Network BSSID.
    struct mac_addr bssid;
    /// Network name.
    struct mac_ssid ssid;
    /// Network type (IBSS or ESS).
    uint16_t bsstype;
    /// Network channel number.
    struct scan_chan_tag *chan;
    /// Network beacon period.
    uint16_t beacon_period;
    /// Capability information
    uint16_t cap_info;
    /// Scan result is valid
    bool valid_flag;
    /// RSSI of the scanned BSS
    int8_t rssi;
};

/// BSS capabilities flags
enum
{
    /// BSS is QoS capable
    BSS_QOS_VALID = CO_BIT(0),
    /// BSS is HT capable
    BSS_HT_VALID = CO_BIT(1),
    /// BSS is VHT capable
    BSS_VHT_VALID = CO_BIT(2),
    /// Information about the BSS are valid
    BSS_INFO_VALID = CO_BIT(31),
};

// Protection Status field (Bit indexes, Masks, Offsets)
/// Non-ERP stations are present
#define MAC_PROT_NONERP_PRESENT_BIT        CO_BIT(0)
/// ERP protection shall be used
#define MAC_PROT_USE_PROTECTION_BIT        CO_BIT(1)
/// Barker preamble protection shall be used
#define MAC_PROT_BARKER_PREAMB_BIT         CO_BIT(2)
/// ERP protection status mask
#define MAC_PROT_ERP_STATUS_MASK           (MAC_PROT_NONERP_PRESENT_BIT |                \
                                            MAC_PROT_USE_PROTECTION_BIT |                \
                                            MAC_PROT_BARKER_PREAMB_BIT)

/// Station flags
enum
{
    /// Bit indicating that a STA has QoS (WMM) capability
    STA_QOS_CAPA = CO_BIT(0),
    /// Bit indicating that a STA has HT capability
    STA_HT_CAPA = CO_BIT(1),
    /// Bit indicating that a STA has VHT capability
    STA_VHT_CAPA = CO_BIT(2),
    /// Bit indicating that a STA has MFP capability
    STA_MFP_CAPA = CO_BIT(3),
    /// Bit indicating that the STA included the Operation Notification IE
    STA_OPMOD_NOTIF = CO_BIT(4),
    /// Bit indicating that a STA has HE capability
    STA_HE_CAPA = CO_BIT(5),
};

/// Connection flags
enum
{
    /// Flag indicating whether the control port is controlled by host or not
    CONTROL_PORT_HOST = CO_BIT(0),
    /// Flag indicating whether the control port frame shall be sent unencrypted
    CONTROL_PORT_NO_ENC = CO_BIT(1),
    /// Flag indicating whether HT and VHT shall be disabled or not
    DISABLE_HT = CO_BIT(2),
    /// Flag indicating whether WPA or WPA2 authentication is in use
    WPA_WPA2_IN_USE = CO_BIT(3),
    /// Flag indicating whether MFP is in use
    MFP_IN_USE = CO_BIT(4),
};

/// Scan result element, parsed from beacon or probe response frames.
struct mac_sta_info
{
    /// Legacy rate set supported by the STA
    struct mac_rateset rate_set;
    /// HT capabilities
    struct mac_htcapability ht_cap;
    /// VHT capabilities
    //struct mac_vhtcapability vht_cap;
    /// Bitfield showing some capabilities of the STA (@ref STA_QOS_CAPA, @ref STA_HT_CAPA,
    /// @ref STA_VHT_CAPA, @ref STA_MFP_CAPA)
    uint32_t capa_flags;
    /// Maximum PHY channel bandwidth supported by the STA
    uint8_t phy_bw_max;
    /// Current channel bandwidth for the STA
    uint8_t bw_cur;
    /// Bit field indicating which queues have to be delivered upon U-APSD trigger
    uint8_t uapsd_queues;
    /// Maximum size, in frames, of a APSD service period
    uint8_t max_sp_len;
    /// Maximum number of spatial streams supported for STBC reception
    uint8_t stbc_nss;
};

/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */
/// Array converting a TID to its associated AC
extern const uint8_t mac_tid2ac[];

/// Array converting an AC to its corresponding bit in the QoS Information field
extern const uint8_t mac_ac2uapsd[AC_MAX];

/// Array converting a MAC HW rate id into its corresponding standard rate value
extern const uint8_t mac_id2rate[];

/// Constant value corresponding to the Broadcast MAC address
extern const struct mac_addr mac_addr_bcst;


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Compute the PAID/GID to be put in the THD for frames that need to be sent to
 * an AP or a Mesh peer.
 * @see 802.11ac-2013, table 9-5b for details about the computation.
 *
 * @param[in] p_mac_addr Pointer to the BSSID of the AP or the MAC address of the mesh
 *                       peer
 *
 * @return The PAID/GID
 ****************************************************************************************
 */
uint32_t mac_paid_gid_sta_compute(struct mac_addr const *p_mac_addr);

/**
 ****************************************************************************************
 * @brief Compute the PAID/GID to be put in the THD for frames that need to be sent to
 * a STA connected to an AP or a TDLS peer.
 * @see 802.11ac-2013, table 9-5b for details about the computation.
 *
 * @param[in] p_mac_addr Pointer to the BSSID of the AP
 * @param[in] aid        Association ID of the STA
 *
 * @return The PAID/GID
 ****************************************************************************************
 */
uint32_t mac_paid_gid_ap_compute(struct mac_addr const *p_mac_addr, uint16_t aid);

/// @}

#endif // _MAC_H_
