/**
 ****************************************************************************************
 *
 * @file wpa_adapter.h
 *
 * @brief adaptor for wpa_supplicant module
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#ifndef _ADAPTOR_TYPES_H_
#define _ADAPTOR_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include "wpas_types.h"
#include "uwifi_common.h"
#include "wlan_cfg.h"

typedef enum{
    DOT11_EVENT_NO_EVENT = 1,
    DOT11_EVENT_REQUEST = 2,
    DOT11_EVENT_ASSOCIATION_IND = 3,
    DOT11_EVENT_ASSOCIATION_RSP = 4,
    DOT11_EVENT_AUTHENTICATION_IND = 5,
    DOT11_EVENT_REAUTHENTICATION_IND = 6,
    DOT11_EVENT_DEAUTHENTICATION_IND = 7,
    DOT11_EVENT_DISASSOCIATION_IND = 8,
    DOT11_EVENT_DISCONNECT_REQ = 9,
    DOT11_EVENT_SET_802DOT11 = 10,
    DOT11_EVENT_SET_KEY = 11,
    DOT11_EVENT_SET_PORT = 12,
    DOT11_EVENT_DELETE_KEY = 13,
    DOT11_EVENT_SET_RSNIE = 14,
    DOT11_EVENT_GKEY_TSC = 15,
    DOT11_EVENT_MIC_FAILURE = 16,
    DOT11_EVENT_ASSOCIATION_INFO = 17,
    DOT11_EVENT_INIT_QUEUE = 18,
    DOT11_EVENT_EAPOLSTART = 19,

    DOT11_EVENT_ACC_SET_EXPIREDTIME = 31,
    DOT11_EVENT_ACC_QUERY_STATS = 32,
    DOT11_EVENT_ACC_QUERY_STATS_ALL = 33,
    DOT11_EVENT_REASSOCIATION_IND = 34,
    DOT11_EVENT_REASSOCIATION_RSP = 35,
    DOT11_EVENT_STA_QUERY_BSSID = 36,
    DOT11_EVENT_STA_QUERY_SSID = 37,
    DOT11_EVENT_EAP_PACKET = 41,

    DOT11_EVENT_EAPOLSTART_PREAUTH = 45,
    DOT11_EVENT_EAP_PACKET_PREAUTH = 46,

    DOT11_EVENT_WPA2_MULTICAST_CIPHER = 47,
    DOT11_EVENT_WPA_MULTICAST_CIPHER = 48,

    DOT11_EVENT_MAX
} DOT11_EVENT;

struct recv_frame_hdr{
    uint32_t  len;
    uint8_t *rx_data;
};

#define RECVFRAME_HDR_ALIGN 128
struct recv_frame{
    union
    {
        struct recv_frame_hdr hdr;
        uint32_t mem[RECVFRAME_HDR_ALIGN>>2];
    }u;
};

struct sta_info {
    uint16_t  used;  //false(0):sta_info is not used yet; true(1):sta_info is used.
    uint8_t    hwaddr[ETH_ALEN];
    /* before we set key we should set it to true,
     * to block hw to enc frame, but after 4-way handshake
     * we should set it to false to enc all data frames */
    //uint32_t ieee8021x_blocked;   //0: allowed, 1:blocked
    //uint32_t dot118021XPrivacy; //aes, tkip...
    //struct Keytype dot11tkiptxmickey;
    //struct Keytype dot11tkiprxmickey;
    //struct Keytype dot118021x_UncstKey;

#ifdef INCLUDE_WPA_WPA2_PSK
    /* wpa psk ---> */
    /* this struct just used in supp_psk.c */
    WPA_STA_INFO wpa_sta_info;
    //struct Dot11KeyMappingsEntry dot11KeyMapping;
    /* wpa psk <--- */
#endif //INCLUDE_WPA_WPA2_PSK
    unsigned char is_sae_sta;
#if NX_SAE
    /*when rx peer's SAE confirem save PMK here clean these when release_stainfo*/
    unsigned char sae_pmk[PMK_LEN];
    unsigned char sae_pmkid[PMKID_LEN];
    unsigned char pmkid_caching_idx;
    uint8_t *ap_rsnxe;
    uint32_t ap_rsnxe_len;
#endif
};

typedef enum
{
    DOT11_KeyType_Group = 0,
    DOT11_KeyType_Pairwise = 1
} DOT11_KEY_TYPE;

#define __STRING_FMT(x) #x
#define STRING_FMT(x) __STRING_FMT(x)
//#define __FUNCTION__ "[Function@" __FILE__ " (" STRING_FMT(__LINE__) ")]"
#define __WLAN_FUNC__     __FUNCTION__


struct iw_point {
    void     *pointer;    /* Pointer to the data */
    uint16_t    length;        /* number of fields or size in bytes */
    uint16_t    flags;        /* Optional params */
};

typedef enum
{
    WIFISUPP_WEP_KEY_TYPE_64BITS,       //64bits_type WEP Key
    WIFISUPP_WEP_KEY_TYPE_128BITS,      //128bits_type WEP Key
    WIFISUPP_WEP_KEY_TYPE_MAX
}WIFISUPP_WEP_KEY_TYPE_E;

enum dot11AuthAlgrthmNum {
    dot11AuthAlgrthm_Open = 0,
    dot11AuthAlgrthm_Shared,
    dot11AuthAlgrthm_8021X,
    dot11AuthAlgrthm_Auto,
    dot11AuthAlgrthm_MaxNum
};

//encryp_protocol
typedef enum
{
    WIFISUPP_ENCRYP_PROTOCOL_OPENSYS,   //open system
    WIFISUPP_ENCRYP_PROTOCOL_WEP,       //WEP
    WIFISUPP_ENCRYP_PROTOCOL_WPA,       //WPA
    WIFISUPP_ENCRYP_PROTOCOL_WPA2,      //WPA2
    WIFISUPP_ENCRYP_PROTOCOL_WAPI,      //WAPI: Not support in this version
    WIFISUPP_ENCRYP_PROTOCOL_EAP,      //WAPI
    WIFISUPP_ENCRYP_PROTOCOL_MAX
}WIFISUPP_ENCRYP_PROTOCOL_E;

//group_cipher
typedef enum
{
    WIFISUPP_CIPHER_TKIP,       //TKIP
    WIFISUPP_CIPHER_CCMP,       //CCMP
    WIFISUPP_CIPHER_WEP,
    WIFISUPP_CIPHER_SMS4,       //WAPI SMS-4
    WIFISUPP_CIPHER_MAX
} WIFISUPP_CIPHER_E;

//PrivacyAlgrthm
typedef enum{
    DOT11_ENC_NONE  = 0,
    DOT11_ENC_WEP40 = 1,
    DOT11_ENC_TKIP  = 2,
    DOT11_ENC_WRAP  = 3,
    DOT11_ENC_CCMP  = 4,
    DOT11_ENC_WEP104= 5,
    DOT11_ENC_WAPI    = 6
} DOT11_ENC_ALGO;

void wifi_recv_indicatepkt_eapol(struct recv_frame *precv_frame, uint32_t mode);

void wpa_connect_init(struct wpa_psk *psk, enum wlan_op_mode mode);

int32_t asr_get_random_bytes(void* dst, uint32_t size);

void wpas_psk_deinit(struct wpa_priv *priv);

void reset_sta_info(struct wpa_priv *priv, struct sta_info *pstat);

void wifi_indicate_handshake_start(uint32_t mode);

#ifdef CFG_SOFTAP_SUPPORT
void enc_gtk(struct wpa_priv *priv, struct sta_info *pstat,
                uint8_t *kek, int32_t keklen, uint8_t *key,
                int32_t keylen, uint8_t *out, uint16_t *outlen);
#endif //CFG_SOFTAP_SUPPORT

extern struct sta_info g_wpa_sta_info[MAX_STA_INFO_NB];

#endif //_ADAPTOR_TYPES_H_
