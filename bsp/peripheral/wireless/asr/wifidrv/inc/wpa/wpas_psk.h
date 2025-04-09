#ifndef __WPAS_PSK_H_
#define __WPAS_PSK_H_

#include <stdint.h>
#include <stdbool.h>
#include "asr_rtos.h"
#include "uwifi_common.h"
//#include "wpa_adapter.h"

#define GMK_LEN                     32
#define GTK_LEN                     32
#define PMK_LEN                     32
#define PTK_LEN                     32
#define KEY_NONCE_LEN               32
#define NumGroupKey                 4
#define KEY_RC_LEN                  8
#define KEY_NONCE_LEN               32
#define KEY_IV_LEN                  16
#define KEY_RSC_LEN                 8
#define KEY_ID_LEN                  8
#define KEY_MIC_LEN                 16
#define KEY_MATERIAL_LEN            2
#define PTK_LEN_EAPOLMIC            16
#define PTK_LEN_EAPOLENC            16
#define PTK_LEN_TKIP                64
#define PMKID_LEN                   16
#define GTK_LEN_CCMP                16

#define DescTypePos                 0
#define KeyInfoPos                  1
#define KeyLenPos                   3
#define ReplayCounterPos            5
#define KeyNoncePos                 13
#define KeyIVPos                    45
#define KeyRSCPos                   61
#define KeyIDPos                    69
#define KeyMICPos                   77
#define KeyDataLenPos               93
#define KeyDataPos                  95
#define LIB1X_EAPOL_VER             1   //00000001B
#define LIB1X_EAPOL_LOGOFF          2   //0000 0010B
#define LIB1X_EAPOL_EAPPKT          0   //0000 0000B
#define LIB1X_EAPOL_START           1   //0000 0001B
#define LIB1X_EAPOL_KEY             3   //0000 0011B
#define LIB1X_EAPOL_ENCASFALERT     4   //0000 0100B


#define PTK_LEN_CCMP        48

#define MAX_RESEND_NUM      15

#define RESEND_TIME         500 // 500ms, normal rsp should in 10ms
#define COMPLETE_TIME       5000// 5000ms for handshake complete timeout

#define A_SHA_DIGEST_LEN    20
#define ETHER_HDRLEN        14
#define LIB1X_EAPOL_HDRLEN  4
#define INFO_ELEMENT_SIZE   128
#define MAX_EAPOLMSG_LEN    512
#define MAX_EAPOLKEYMSG_LEN (MAX_EAPOLMSG_LEN-(ETHER_HDRLEN+LIB1X_EAPOL_HDRLEN))
#define EAPOLMSG_HDRLEN     95  //EAPOL-key payload length without KeyData
#define MAX_UNICAST_CIPHER  2
#define WPA_ELEMENT_ID      0xDD
#define WPA2_ELEMENT_ID     0x30

typedef enum    { desc_type_WPA2 = 2, desc_type_RSN = 254 } DescTypeRSN;
typedef enum    { type_Group = 0, type_Pairwise = 1 } KeyType;

#if (defined(CONFIG_IEEE80211W) || NX_SAE)
#define WPA_IGTK_MAX_LEN 32
// HMAC_MD5_RC4 = 1, HMAC_SHA1_AES = 2, AES_128_CMAC = 3
typedef enum    { key_desc_ver0 = 0, key_desc_ver1 = 1, key_desc_ver2 = 2, key_desc_ver3 = 3} KeyDescVer;

/// Authentication algorithm definition from fw.
#define MAC_AUTH_ALGO_OPEN                0
#define MAC_AUTH_ALGO_SHARED              1
#define MAC_AUTH_ALGO_FT                  2
#define MAC_AUTH_ALGO_SAE                 3

#else
typedef enum    { key_desc_ver1 = 1, key_desc_ver2 = 2 } KeyDescVer;
#endif

enum { PSK_WPA=1, PSK_WPA2=2,PSK_WPA3};

/**
 * enum mfp_options - Management frame protection (IEEE 802.11w) options
 */
enum mfp_options {
    NO_MGMT_FRAME_PROTECTION = 0,
    MGMT_FRAME_PROTECTION_OPTIONAL = 1,
    MGMT_FRAME_PROTECTION_REQUIRED = 2,
};

enum WPA3_SUPPORT{
    WPA3_NOT_SUPPORT = 0,
    WPA3_ONLY,
    WPA3_WPA2_BOTH,
};

enum {
    PSK_STATE_IDLE,
    PSK_STATE_PTKSTART,
    PSK_STATE_PTKINITNEGOTIATING,
    PSK_STATE_PTKINITDONE
};

enum {
    PSK_GSTATE_REKEYNEGOTIATING,
    PSK_GSTATE_REKEYESTABLISHED,
    PSK_GSTATE_KEYERROR
};

enum wlan_op_mode
{
    WLAN_OPMODE_AUTO=0,    /* Let the driver decides */
    WLAN_OPMODE_ADHOC,    /* Single cell network */
    WLAN_OPMODE_ADHOCMASTER,
    WLAN_OPMODE_INFRA,    /* Multi cell network, roaming, ... */
    WLAN_OPMODE_MASTER,    /* Synchronisation master or Access Point*/
    WLAN_OPMODE_MONITOR,    /* Passive monitor (listen only) */
    WLAN_OPMODE_MP
};

/*
 * Reason code for Disconnect
 */
typedef enum _ReasonCode{
    unspec_reason                   = 0x01,
    auth_not_valid                  = 0x02,
    deauth_lv_ss                    = 0x03,
    inactivity                      = 0x04,
    ap_overload                     = 0x05,
    class2_err                      = 0x06,
    class3_err                      = 0x07,
    disas_lv_ss                     = 0x08,
    asoc_not_auth                   = 0x09,
    RSN_invalid_info_element        = 13,
    RSN_MIC_failure                 = 14,
    RSN_4_way_handshake_timeout     = 15,
    RSN_diff_info_element           = 17,
    RSN_multicast_cipher_not_valid  = 18,
    RSN_unicast_cipher_not_valid    = 19,
    RSN_AKMP_not_valid              = 20,
    RSN_unsupported_RSNE_version    = 21,
    RSN_invalid_RSNE_capabilities   = 22,
    RSN_ieee_802dot1x_failed        = 23,

    RSN_PMK_not_avaliable           = 24,
    expire                          = 30,
    session_timeout                 = 31,
    acct_idle_timeout               = 32,
    acct_user_request               = 33
}ReasonCode;


typedef struct _OCTET_STRING {
    uint8_t   *Octet;
    int32_t   Length;
} OCTET_STRING;

typedef union _LARGE_INTEGER {
    uint8_t   charData[8];
    struct {
        uint32_t  HighPart;
        uint32_t  LowPart;
    } field;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef union _OCTET16_INTEGER {
    uint8_t   charData[16];
    struct {
        LARGE_INTEGER   HighPart;
        LARGE_INTEGER   LowPart;
    } field;
} OCTET16_INTEGER;

typedef union  _OCTET32_INTEGER {
    uint8_t   charData[32];
    struct {
        OCTET16_INTEGER HighPart;
        OCTET16_INTEGER LowPart;
    } field;
} OCTET32_INTEGER;

typedef struct _DOT11_WPA2_IE_HEADER {
    uint8_t   ElementID;
    uint8_t   Length;
    uint16_t  Version;
} DOT11_WPA2_IE_HEADER;


// group key info
typedef struct _wpa_global_info {
    OCTET32_INTEGER Counter;
    uint8_t   PSK[A_SHA_DIGEST_LEN*2];
    uint8_t   PSKGuest[A_SHA_DIGEST_LEN*2];
    int     GTKAuthenticator;
    int     GKeyDoneStations;
    int     GkeyReady;
    OCTET_STRING    AuthInfoElement;
    uint8_t   AuthInfoBuf[INFO_ELEMENT_SIZE];
    uint8_t   MulticastCipher;
    int     NumOfUnicastCipher;
    uint8_t   UnicastCipher[MAX_UNICAST_CIPHER];
    int     NumOfUnicastCipherWPA2;
    uint8_t   UnicastCipherWPA2[MAX_UNICAST_CIPHER];
    OCTET_STRING    GNonce;
    uint8_t   GNonceBuf[KEY_NONCE_LEN];
    uint8_t   GTK[NumGroupKey][GTK_LEN];
    uint8_t   GMK[GMK_LEN];
    uint8_t   keyrsc[KEY_RSC_LEN];

#ifdef CONFIG_IEEE80211W
    uint32_t  MgmtGroupCipher;
    uint8_t   IGTK[WPA_IGTK_MAX_LEN];
    uint32_t  IGTKLen;
    uint16_t  IGTKIdx;
#endif

    int     GN;
    int     GM;
    //int     GRekeyCounts;
    int     GResetCounter;

    int     IntegrityFailed;
    int     GTKRekey;
    int     GKeyFailure;
    asr_timer_t   GKRekeyTimer;
} WPA_GLOBAL_INFO;

typedef enum
{
    EAPOL_NO_TX = 0,
    EAPOL_TX_WAIT_CFM,
    EAPOL_TX_DONE,
}FOURTH_EAPOL_TX_STAT_E;
// wpa sta info
typedef struct _wpa_sta_info {
    int     state;
    int     gstate;
    int     RSNEnabled; // bit0-WPA, bit1-WPA2
    int     PMKCached;
    int     PInitAKeys;
    uint8_t   UnicastCipher;
    uint8_t   NumOfRxTSC;
    uint8_t   AuthKeyMethod;
    //int     isSuppSupportPreAuthentication;
    //int     isSuppSupportPairwiseAsDefaultKey;
    LARGE_INTEGER   CurrentReplayCounter;
    LARGE_INTEGER   ReplayCounterStarted;
    OCTET_STRING    ANonce;
    OCTET_STRING    SNonce;
    uint8_t   AnonceBuf[KEY_NONCE_LEN];
    uint8_t   SnonceBuf[KEY_NONCE_LEN];
    uint8_t   PMK[PMK_LEN];
    uint8_t   PTK[PTK_LEN_TKIP];
    OCTET_STRING    EAPOLMsgRecvd;
    OCTET_STRING    EAPOLMsgSend;
    OCTET_STRING    EapolKeyMsgRecvd;
    OCTET_STRING    EapolKeyMsgSend;
    //uint8_t     eapSendBuf[MAX_EAPOLMSG_LEN];
    //uint8_t     eapRecvdBuf[MAX_EAPOLMSG_LEN];
    int     resendCnt;
    int     isGuest;
    int     clientHndshkProcessing;
    int     clientHndshkDone;
    asr_timer_t   resendTimer;
    asr_timer_t   completeTimer;
    struct wpa_priv        *priv;
    uint8_t   wpa_ie[INFO_ELEMENT_SIZE];
    uint32_t  wpa_ie_len;
    int     GUpdateStationKeys;
    uint8_t   HandShakeSetGTK; //indicate if 4 way handshake should set gtk
    uint8_t   FourthEapolTxState; //sta mode 4th eapol tx state
    uint8_t   possible_attack; //possible attack for 4 way hardshark
#ifdef CONFIG_IEEE80211W
    uint32_t  KeyMgmt;
    uint32_t  MgmtGroupCipher;
#endif
} WPA_STA_INFO;

typedef struct _LIB1X_EAPOL_KEY
{
    uint8_t   key_desc_ver;
    uint8_t   key_info[2];
    uint8_t   key_len[2];
    uint8_t   key_replay_counter[KEY_RC_LEN];
    uint8_t   key_nounce[KEY_NONCE_LEN];
    uint8_t   key_iv[KEY_IV_LEN];
    uint8_t   key_rsc[KEY_RSC_LEN];
    uint8_t   key_id[KEY_ID_LEN];
    uint8_t   key_mic[KEY_MIC_LEN];
    uint8_t   key_data_len[KEY_MATERIAL_LEN];
    uint8_t   *key_data;
}__attribute__((packed)) lib1x_eapol_key;


struct lib1x_eapol
{
    uint8_t   protocol_version;
    uint8_t   packet_type;    // This makes it odd in number !
    uint16_t  packet_body_length;
}__attribute__((packed)) ;

#define SetSubStr(f,a,l)                memcpy(f->Octet+l,a.Octet,a.Length)
#define GetKeyInfo0(f, mask)            ((f->Octet[KeyInfoPos + 1] & mask) ? 1 :0)
#define SetKeyInfo0(f,mask,b)           (f->Octet[KeyInfoPos + 1] = (f->Octet[KeyInfoPos + 1] & ~mask) | ( b?mask:0x0) )
#define GetKeyInfo1(f, mask)            ((f->Octet[KeyInfoPos] & mask) ? 1 :0)
#define SetKeyInfo1(f,mask,b)           (f->Octet[KeyInfoPos] = (f->Octet[KeyInfoPos] & ~mask) | ( b?mask:0x0) )

// EAPOLKey
#define Message_DescType(f)             (f->Octet[DescTypePos])
#define Message_setDescType(f, type)    (f->Octet[DescTypePos] = type)
// Key Information Filed
#define Message_KeyDescVer(f)           (f->Octet[KeyInfoPos+1] & 0x07)//(f->Octet[KeyInfoPos+1] & 0x01) | (f->Octet[KeyInfoPos+1] & 0x02) <<1 | (f->Octet[KeyInfoPos+1] & 0x04) <<2
#define Message_setKeyDescVer(f, v)     (f->Octet[KeyInfoPos+1] &= 0xf8) , f->Octet[KeyInfoPos+1] |= (v & 0x07)//(f->Octet[KeyInfoPos+1] |= ((v&0x01)<<7 | (v&0x02)<<6 | (v&0x04)<<5) )
#define Message_KeyType(f)              GetKeyInfo0(f,0x08)
#define Message_setKeyType(f, b)        SetKeyInfo0(f,0x08,b)
#define Message_KeyIndex(f)             (((f->Octet[KeyInfoPos+1] & 0x30) >> 4) & 0x03) //(f->Octet[KeyInfoPos+1] & 0x20) | (f->Octet[KeyInfoPos+1] & 0x10) <<1
#define Message_setKeyIndex(f, v)       (f->Octet[KeyInfoPos+1] &= 0xcf), f->Octet[KeyInfoPos+1] |= ((v<<4) & 0x30)//(f->Octet[KeyInfoPos+1] |= ( (v&0x01)<<5 | (v&0x02)<<4)  )
#define Message_Install(f)              GetKeyInfo0(f,0x40)
#define Message_setInstall(f, b)        SetKeyInfo0(f,0x40,b)
#define Message_KeyAck(f)               GetKeyInfo0(f,0x80)
#define Message_setKeyAck(f, b)         SetKeyInfo0(f,0x80,b)

#define Message_KeyMIC(f)               GetKeyInfo1(f,0x01)
#define Message_setKeyMIC(f, b)         SetKeyInfo1(f,0x01,b)
#define Message_Secure(f)               GetKeyInfo1(f,0x02)
#define Message_setSecure(f, b)         SetKeyInfo1(f,0x02,b)
#define Message_Error(f)                GetKeyInfo1(f,0x04)
#define Message_setError(f, b)          SetKeyInfo1(f,0x04,b)
#define Message_Request(f)              GetKeyInfo1(f,0x08)
#define Message_setRequest(f, b)        SetKeyInfo1(f,0x08,b)
#define Message_Reserved(f)             (f->Octet[KeyInfoPos] & 0xf0)
#define Message_setReserved(f, v)       (f->Octet[KeyInfoPos] |= (v<<4&0xff))
#define Message_KeyLength(f)            ((uint16_t)(f->Octet[KeyLenPos] <<8) + (uint16_t)(f->Octet[KeyLenPos+1]))
#define Message_setKeyLength(f, v)      (f->Octet[KeyLenPos] = (v&0xff00) >>8 ,  f->Octet[KeyLenPos+1] = (v&0x00ff))

#define Message_KeyNonce(f)                 sub_str(f,KeyNoncePos,KEY_NONCE_LEN)
#define Message_setKeyNonce(f, v)           SetSubStr(f, v, KeyNoncePos)
#define Message_EqualKeyNonce(f1, f2)       !memcmp(f1->Octet + KeyNoncePos, f2->Octet, KEY_NONCE_LEN)
#define Message_KeyIV(f)                    sub_str(f, KeyIVPos, KEY_IV_LEN)
#define Message_setKeyIV(f, v)              SetSubStr(f, v, KeyIVPos)
#define Message_KeyRSC(f)                   sub_str(f, KeyRSCPos, KEY_RSC_LEN)
#define Message_setKeyRSC(f, v)             SetSubStr(f, v, KeyRSCPos)
#define Message_KeyID(f)                    sub_str(f, KeyIDPos, KEY_ID_LEN)
#define Message_setKeyID(f, v)              SetSubStr(f, v, KeyIDPos)
#define Message_MIC(f)                      sub_str(f, KeyMICPos, KEY_MIC_LEN)
#define Message_setMIC(f, v)                SetSubStr(f, v, KeyMICPos)
#define Message_clearMIC(f)                 memset(f->Octet+KeyMICPos, 0, KEY_MIC_LEN)
#define Message_KeyDataLength(f)            ((uint16_t)(f->Octet[KeyDataLenPos] <<8) + (uint16_t)(f->Octet[KeyDataLenPos+1]))
#define Message_setKeyDataLength(f, v)      (f->Octet[KeyDataLenPos] = (v&0xff00) >>8 ,  f->Octet[KeyDataLenPos+1] = (v&0x00ff))
#define Message_KeyData(f, l)               sub_str(f, KeyDataPos, l)
#define Message_setKeyData(f, v)            SetSubStr(f, v, KeyDataPos);
#define Message_EqualRSNIE(f1 , f2, l)      !memcmp(f1->Octet, f2->Octet, l)
#define Message_ReturnKeyDataLength(f)      f->Length - (ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN + EAPOLMSG_HDRLEN)

#define Message_CopyReplayCounter(f1, f2)   memcpy(f1->Octet + ReplayCounterPos, f2->Octet + ReplayCounterPos, KEY_RC_LEN)
#define Message_DefaultReplayCounter(li)    ((li.field.HighPart == 0xffffffff) && (li.field.LowPart == 0xffffffff) ) ?1:0

#define MAX_GTK_IDX 6

extern struct wpa_priv *g_pwpa_priv[COEX_MODE_MAX];

struct sta_info;
struct config_info;
struct pmkid_caching;
struct cfg80211_auth_request;
struct asr_vif;

void sm_stop_sa_query(struct config_info *config_info);
void wpas_sta_4way_handshake_done_handler(void);
void wpas_complete_timeout_callback(struct sta_info *pstat);
unsigned char search_by_mac_pmkid_cache(struct wpa_priv *priv, unsigned char *peermac);
unsigned char pmkid_cached(struct sta_info *pstat);
unsigned char add_pmkid_cache(struct pmkid_caching *pmkid_cache_ptr, unsigned char *pmkid,
    unsigned char *pmk, unsigned char *peermac);
void wpa_free_sae(void);
void pmkid_cache_del(struct pmkid_caching *pmkid_cache_ptr, int idx);

void sme_auth_start(struct asr_vif *asr_vif,struct cfg80211_auth_request *req);

void uwifi_wpa_handle_rx_mgmt(struct asr_vif *asr_vif, uint8_t *pframe, uint32_t len);

void wpas_derive_psk_callback(void* pcontext, uint8_t *data);

void wpas_priv_free(enum wlan_op_mode mode);

void wpas_psk_event_handler(struct wpa_priv *priv, int32_t event_id, uint8_t *mac, uint8_t *msg, int32_t msg_len);

void wpas_psk_init(struct wpa_priv *priv);

#endif //__WPAS_PSK_H_
