/**
 ****************************************************************************************
 *
 * @file uwifi_msg_rx.h
 *
 * @brief RX function declarations
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */

#ifndef _UWIFI_MSG_RX_H_
#define _UWIFI_MSG_RX_H_

//add by
#define MAC_HDR_OFT         24
#define MGMT_SSID_OFT       12

#define PROBE_RSP_SSID_OFT   (MAC_HDR_OFT + MGMT_SSID_OFT)   //36

/// 802.11 IE identifier

#define MAC_INFOELT_ID_OFT                0
#define MAC_INFOELT_LEN_OFT               1
#define MAC_INFOELT_EXT_ID_OFT            2
#define MAC_INFOELT_INFO_OFT              2
#define MAC_INFOELT_EXT_INFO_OFT          3


#define SSID_ELEM_ID                    0

#define MAC_EID_DS                      3
#define MAC_EID_DS_CHANNEL_OFT          2

#define WPA_ELEM_ID                     221
#define WPA_OUI                         0x0050F201
#define WPA_CCMP_OUI                    0x0050F204
#define WPA_TKIP_OUI                    0x0050F202

#define RSN_ELEM_ID                     48     //WPA2
#define RSN_OUI                         0x000FAC
#define RSN_OUI_CCMP                    0x000FAC04
#define RSN_OUI_TKIP                    0x000FAC02
#define RSN_CCMP_TYPE                   4
#define RSN_TKIP_TYPE                   2

#define RSNXE_ELEM_ID                   244

#define WPA_MULTI_CIPHER_OUI_OFT        8
#define WPA_UNICAST_CIPHER_COUNT_OFT    12
#define WPA_UNICAST_CIPHER_OUI_OFT      14
#define WPA_AFTER_CIPHER_OUI_OFT        18

#define RSN_GROUP_CIPHER_TYPE_OFT       7
#define RSN_PAIRWISE_CIPHER_COUNT_OFT   8
#define RSN_PAIRWISE_CIPHER_TYPE_OFT    10
#define RSN_AFTER_CIPHER_OUI_OFT        14

#define SSID_LEN_OFT_ELEM               1
#define SSID_OFT_ELEM                   2

#define SUPPORT_RATES                   10
#define DIRECT_SEQ                      3
#define COUNTRY                         8
#define ERP_INFO                        3
#define MGMT_WPA_RSN_OFT      (SUPPORT_RATES + DIRECT_SEQ + COUNTRY + ERP_INFO)

#if 0
#define WPA_GET_BE32(a) ((((uint32_t) (a)[0]) << 24) | (((uint32_t) (a)[1]) << 16) | \
             (((uint32_t) (a)[2]) << 8) | ((uint32_t) (a)[3]))

#define WPA_GET_BE24(a) ((((uint32_t) (a)[0]) << 16) | (((uint32_t) (a)[1]) << 8) | \
             (((uint32_t) (a)[2]) ))
#endif

/// Structure of a long control frame MAC header
struct mac_hdr
{
    /// Frame control
    uint16_t fctl;
    /// Duration/ID
    uint16_t durid;
    /// Address 1
    struct mac_addr addr1;
    /// Address 2
    struct mac_addr addr2;
    /// Address 3
    struct mac_addr addr3;
    /// Sequence control
    uint16_t seq;
};

struct ProbeRsp
{
    /// MAC Header
    struct mac_hdr h;
    /// Timestamp
    uint64_t tsf;
    /// Beacon interval
    uint16_t bcnint;
    /// Capability information
    uint16_t capa;
    /// Rest of the payload
    uint8_t variable[];
};

int asr_rx_sm_disconnect_ind(struct asr_hw *asr_hw,struct asr_cmd *cmd,struct ipc_e2a_msg *msg);


void asr_rx_handle_msg(struct asr_hw *asr_hw, struct ipc_e2a_msg *msg);
uint8_t* uwifi_mac_ie_find(uint8_t *addr, uint32_t buflen, uint32_t ie_id);
extern void uwifi_schedule_reconnect(struct config_info *pst_config_info);
uint8_t* uwifi_mac_ext_ie_find(uint8_t *addr, uint32_t buflen, uint32_t ie_id, uint8_t ext_ie_id);

#endif /* _UWIFI_MSG_RX_H_ */
