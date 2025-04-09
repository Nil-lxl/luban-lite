/**
 ****************************************************************************************
 *
 * @file uwifi_interface.h
 *
 * @brief api providing interface for AT/other app task, and lwip task
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#ifndef _UWIFI_INTERFACE_H_
#define _UWIFI_INTERFACE_H_

#include "uwifi_types.h"
#include "uwifi_kernel.h"
#include "uwifi_ops_adapter.h"

enum open_mode
{
    STA_MODE=0x01,
    SAP_MODE,
    SNIFFER_MODE,
    STA_AP_MODE,
    NON_MODE_=0xFF,
};

enum scan_mode
{
    SCAN_ALL_CHANNEL=0x0,
    SCAN_SSID,
    SCAN_FREQ_SSID
};

struct wifi_scan_t
{
    enum scan_mode scanmode;
    uint8_t* ssid;
    uint8_t  ssid_len;
    uint16_t freq;
    uint8_t  bssid[MAC_ADDR_LEN];
};

//if password is NULL, will open as open mode, else will use ssid/password work as wpa2/ccmp ap
struct softap_info_t
{
    uint8_t  ssid[WIFI_SSID_LEN];
    uint8_t  ssid_len;
    uint8_t* password;
    uint8_t  pwd_len;
    uint8_t  chan;
    uint32_t interval;
    uint8_t  hide;
    uint8_t  he_enable;
};

struct wifi_conn_t
{
    uint8_t* ssid;
    uint8_t* password;
    uint32_t cipher;
    uint8_t  bssid[ETH_ALEN];
    uint8_t  ssid_len;
    uint8_t  pwd_len;
    uint8_t  encrypt;
    uint8_t  channel;
    uint8_t  ie[MAX_IE_LEN];
    uint16_t ie_len;
};

/********************************************
wpa/other app call tx data
*********************************************/
int uwifi_tx_skb_data(struct sk_buff *skb, struct asr_vif *asr_vif);

/********************************************
AT/other app task send to uwifi
*********************************************/
#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
uint8_t uwifi_tx_custom_mgmtframe(uint8_t *pframe, uint32_t len);
#endif

void uwifi_open_wifi_mode(enum open_mode openmode, struct softap_info_t* softap_info);
void uwifi_close_wifi(uint32_t iftype);
void uwifi_scan_req(struct wifi_scan_t* wifi_scan);
void uwifi_connect_req(struct wifi_conn_t* wifi_con);
void uwifi_disconnect(void);
void uwifi_set_tx_power(uint8_t power, uint32_t iftype);  //dbm
#ifdef CFG_SNIFFER_SUPPORT
void uwifi_sniffer_register_cb(monitor_cb_t p_fn);
void uwifi_set_filter_type(uint32_t type);
#endif

/********************************************
Other extended API
*********************************************/
#if 0
enum nl80211_iftype uwifi_get_work_mode(void);
#endif

#endif //_UWIFI_INTERFACE_H_
