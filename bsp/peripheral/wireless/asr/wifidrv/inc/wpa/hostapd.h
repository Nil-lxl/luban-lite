/**
 ****************************************************************************************
 *
 * @file hostapd.h
 *
 * @brief external api or data structure of hostapd.
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#ifndef HOSTAPD_H
#define HOSTAPD_H

#include "asr_types.h"
#include "uwifi_ops_adapter.h"
#include "mac.h"
#include "uwifi_interface.h"
#include "wifi_config.h"
#include "ipc_compat.h"

#ifdef CFG_SOFTAP_SUPPORT

#define MAC_HDR_LEN            24

#define WIFI_PSK_LEN           64
#define BASIC_RATESET_LEN      12
#define BASIC_EXT_RATESET_LEN  16
#define WIFI_AP_REKEY_TIME_DEFAULT 36000 //default rekey time:3600s
#define GetFrameSubType(pbuf)    ((*(uint16_t *)(pbuf)) & (BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2)))

enum ssid_match_result
{
    NO_SSID_MATCH,
    EXACT_SSID_MATCH,
    WILDCARD_SSID_MATCH
};

//edca cfg parameters
struct ap_edca_cfg_param
{
    uint8_t  acm;
    uint8_t  aifsn;
    uint8_t  cwmin;
    uint8_t  cwmax;
    uint16_t txop;
};

//data structure to config ap mode
struct probe_req_elems {
    uint8_t *ssid;
    uint8_t *ssid_list;
    uint8_t  ssid_len;
    uint8_t  ssid_list_len;
};

//data structure to config ap mode
struct uwifi_ap_cfg_param {
    uint32_t                    dtim_period;
    uint16_t                    beacon_interval;
    uint16_t                    capabality;  //used in beacon & assoc_rsp capability information

    uint8_t                     wmm_enabled;
    uint8_t                     ht_enabled;
    uint8_t                     he_enabled;
    uint8_t                     encryption_protocol;

    struct ieee80211_channel    chan_basic;
    enum nl80211_chan_width     chan_width;
    uint32_t                    chan_center_freq1;
    uint32_t                    chan_center_freq2;

    bool                        crypto_control_port;
    bool                        crypto_control_port_no_encrypt;
    uint16_t                    crypto_control_port_ethertype;
    uint32_t                    crypto_cipher_group;
    uint32_t                    crypto_cipher_pairwise;

    uint8_t                     ssid[WIFI_SSID_LEN+1];
    uint8_t                     ssid_len;
    uint8_t                     psk[WIFI_PSK_LEN+1];
    uint8_t                     psk_len;

    //should be put into asr_hw
    uint8_t                     supported_rates[SUPPORT_RATESET_LEN];  // Set of 16 data rates
    uint8_t                     basic_rates[BASIC_RATESET_LEN];
    uint8_t                     basic_rates_len;
    uint8_t                     basic_rates_ext[BASIC_EXT_RATESET_LEN];
    uint8_t                     basic_rates_ext_len;

    //for dsss parameter set element
    uint8_t                     dsss_config;

    struct ap_edca_cfg_param    edca_param[AC_MAX];

    uint8_t                     ap_isolate;
    enum nl80211_hidden_ssid    hidden_ssid;
    uint8_t                     re_enable;

};


//connect state for ap mode
enum connect_state_e  //only for ap mode
{
    CONNECT_IDLE = 0,
    CONNECT_AUTH,              //after received auth
    CONNECT_ASSOC,              //after received assoc_req
    CONNECT_ASSOC_DONE,         //after tx assoc_rsp acked
    CONNECT_HANDSHAKE_DONE,
    CONNECT_GET_IP_DONE,
};

//used to store connected peer sta infor for user
struct peer_sta_user_info
{
    bool     valid;
    uint8_t  status;  //connect status
    uint16_t aid;                 /* association ID */
    uint8_t  mac_addr[MAC_ADDR_LEN];  /* MAC address of the station */
    uint32_t ip_addr;
    //bool     ps_active;           //true when sta in ps mode
};

//used to store ap infor for user
struct ap_user_info
{
    uint8_t bssid[MAC_ADDR_LEN];
    uint8_t ssid[WIFI_SSID_LEN];
    uint8_t ssid_len;
    enum ieee80211_band band;
    uint16_t center_freq;
    enum nl80211_chan_width  chan_width;
    int8_t connect_peer_num;
    struct peer_sta_user_info sta_table[AP_MAX_ASSOC_NUM];
};

struct ap_close_status
{
    uint8_t is_waiting_cfm;
    void *p_sta_start_info;
};

struct sta_close_status
{
    uint8_t is_waiting_close;
};
int uwifi_open_ap(struct asr_hw *asr_hw, struct softap_info_t *softap_info);
void uwifi_close_ap(struct asr_hw *asr_hw);
void uwifi_hostapd_handle_handshake_error(struct asr_vif *asr_vif, uint8_t *mac, uint16_t status);
void uwifi_hostapd_handle_handshake_done(uint8_t *mac);
void uwifi_hostapd_mgmt_tx_comp_status(struct asr_vif *asr_vif, const uint8_t *buf,
                                                                                const uint32_t len, const bool ack);
void uwifi_hostapd_handle_rx_mgmt(struct asr_vif *asr_vif, uint8_t *pframe, uint32_t len);
struct uwifi_ap_cfg_param* uwifi_hostapd_get_softap_cfg_param_t(void);
void uwifi_hostapd_handle_assocrsp_tx_comp_msg(uint32_t is_success, uint32_t param);
void uwifi_hostapd_handle_deauth_tx_comp_msg(uint32_t is_success, uint32_t param);
void uwifi_hostapd_handle_deauth_msg(uint32_t idx);

void uwifi_hostapd_aging_deauth_peer_sta(uint8_t *mac_addr);

uint8_t* uwifi_hostapd_set_ie(uint8_t *pbuf, uint32_t index, uint32_t len, uint8_t *source);
uint8_t* uwifi_init_mgmtframe_header(struct ieee80211_hdr *frame, uint8_t *addr1,
                                                                      uint8_t *addr2, uint8_t *addr3, uint16_t subtype);
uint8_t uwifi_hostapd_get_rateset_len(uint8_t *rateset);
uint8_t* uwifi_hostapd_set_wmm_info_element(uint8_t *pframe, struct uwifi_ap_cfg_param *cfg_param);
uint8_t* uwifi_hostapd_set_rsn_info_element(uint8_t *pframe);
uint8_t* uwifi_hostapd_set_ht_capa_info_element(uint8_t *pframe);
uint8_t* uwifi_hostapd_set_ht_opration_info_element(uint8_t *pframe);
uint8_t* uwifi_hostapd_set_he_capa_info_element(uint8_t *pframe);
uint8_t* uwifi_hostapd_set_he_operation_info_element(uint8_t *pframe);


void uwifi_hostapd_handle_auth_to(void *arg);
void uwifi_hostapd_handle_assoc_to(void *arg);

extern struct ap_user_info g_ap_user_info;
extern struct softap_info_t g_softap_info;
extern struct uwifi_ap_cfg_param g_ap_cfg_info;
#endif //#ifdef CFG_SOFTAP_SUPPORT

#endif //HOSTAPD

