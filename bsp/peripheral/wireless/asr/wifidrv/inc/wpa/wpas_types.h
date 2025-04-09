#ifndef __WPAS_TYPES_H__
#define __WPAS_TYPES_H__

#include "wpas_psk.h"
#include "uwifi_types.h"
#include "asr_dbg.h"

#define MAXRSNIELEN     128
#define MACADDRLEN      6

typedef void*   wpa_caddr_t;

#define OPMODE  (priv->op_mode)

#ifdef CFG_SOFTAP_SUPPORT
#define MAX_ASSOC_NUM   CFG_STA_MAX
#else
#define MAX_ASSOC_NUM   1
#endif //CFG_SOFTAP_SUPPORT

#define MAX_STA_INFO_NB MAX_ASSOC_NUM

#define NUM_WDS     4

typedef enum{
    DOT11_PortStatus_Unauthorized,
    DOT11_PortStatus_Authorized,
    DOT11_PortStatus_Guest
}DOT11_POWLAN_STATUS;

typedef enum{
    DOT11_AuthKeyType_RSN = 1,
    DOT11_AuthKeyType_PSK = 2,
    DOT11_AuthKeyType_NonRSN802dot1x = 3
} DOT11_AUTHKEY_TYPE;

typedef enum{
    DOT11_Ioctl_Query = 0,
    DOT11_Ioctl_Set = 1
} DOT11_Ioctl_Flag;

typedef struct {
    uint8_t   EventId;
    uint8_t   IsMoreEvent;
    uint8_t   MACAddr[MACADDRLEN];
    uint16_t  Status;
}DOT11_ASSOCIATIIN_RSP;

typedef struct _DOT11_DISCONNECT_REQ{
    uint8_t   EventId;
    uint8_t   IsMoreEvent;
    uint16_t  Reason;
    uint8_t   MACAddr[MACADDRLEN];
}DOT11_DISCONNECT_REQ;

typedef struct _DOT11_SET_RSNIE{
    uint8_t   EventId;
    uint8_t   IsMoreEvent;
    uint16_t  Flag;
    uint16_t  RSNIELen;
    uint8_t   RSNIE[MAXRSNIELEN];
    uint8_t   MACAddr[MACADDRLEN];
}DOT11_SET_RSNIE;

typedef struct _DOT11_SET_PORT{
    uint8_t EventId;
    uint8_t PortStatus;
    uint8_t PortType;
    uint8_t MACAddr[MACADDRLEN];
}DOT11_SET_PORT;

typedef struct _DOT11_MIC_FAILURE{
    uint8_t   EventId;
    uint8_t   IsMoreEvent;
    uint8_t   MACAddr[MACADDRLEN];
}DOT11_MIC_FAILURE;

typedef struct _DOT11_INIT_QUEUE
{
    uint8_t EventId;
    uint8_t IsMoreEvent;
} DOT11_INIT_QUEUE, *PDOT11_INIT_QUEUE;

typedef enum{
    ERROR_BUFFER_TOO_SMALL = -1,
    ERROR_INVALID_PARA = -2,
    ERROR_INVALID_RSNIE = -13,
    ERROR_INVALID_MULTICASTCIPHER = -18,
    ERROR_INVALID_UNICASTCIPHER = -19,
    ERROR_INVALID_AUTHKEYMANAGE = -20,
    ERROR_UNSUPPORTED_RSNEVERSION = -21,
    ERROR_INVALID_CAPABILITIES = -22
} INFO_ERROR;

//*---------- The followings are for processing of RSN Information Element------------*/
#define RSN_ELEMENT_ID                  0xDD
#define RSN_VER1                        0x01
#define DOT11_MAX_CIPHER_ALGORITHMS     0x0a

typedef struct _DOT11_RSN_IE_HEADER {
    uint8_t   ElementID;
    uint8_t   Length;
    uint8_t   OUI[4];
    uint16_t  Version;
}__attribute__((packed)) DOT11_RSN_IE_HEADER;


typedef struct _DOT11_RSN_IE_SUITE{
    uint8_t   OUI[3];
    uint8_t   Type;
}__attribute__((packed)) DOT11_RSN_IE_SUITE;


typedef struct _DOT11_RSN_IE_COUNT_SUITE{
    uint16_t  SuiteCount;
    DOT11_RSN_IE_SUITE  dot11RSNIESuite[DOT11_MAX_CIPHER_ALGORITHMS];
}__attribute__((packed)) DOT11_RSN_IE_COUNT_SUITE, *PDOT11_RSN_IE_COUNT_SUITE;

typedef union _DOT11_RSN_CAPABILITY{
    uint16_t  shortData;
    uint8_t   charData[2];

    struct
    {
        volatile uint32_t Reserved1:2; // B7 B6
        volatile uint32_t GtksaReplayCounter:2; // B5 B4
        volatile uint32_t PtksaReplayCounter:2; // B3 B2
        volatile uint32_t NoPairwise:1; // B1
        volatile uint32_t PreAuthentication:1; // B0
        volatile uint32_t Reserved2:8;
    }__attribute__((packed)) field;

}DOT11_RSN_CAPABILITY;

struct Dot1180211AuthEntry {
    uint32_t  dot11AuthAlgrthm;       // 802.11 auth, could be open, shared, auto
    uint32_t  dot11PrivacyAlgrthm;    // encryption algorithm, could be none, wep40, TKIP, CCMP, wep104
    uint32_t  dot11PrivacyKeyIndex;   // this is only valid for legendary wep, 0~3 for key id.
    uint32_t  dot11PrivacyKeyLen;     // this could be 40 or 104

    uint32_t  dot11EnablePSK;         // 0: disable, bit0: WPA, bit1: WPA2
    uint32_t  dot11WPACipher;         // bit0-wep64, bit1-tkip, bit2-wrap,bit3-ccmp, bit4-wep128
    uint32_t  dot11WPA2Cipher;        // bit0-wep64, bit1-tkip, bit2-wrap,bit3-ccmp, bit4-wep128
    uint32_t  dot11WPAGrpCipher;      // bit0-wep64, bit1-tkip, bit2-wrap,bit3-ccmp, bit4-wep128
    uint32_t  dot11WPA2GrpCipher;     // bit0-wep64, bit1-tkip, bit2-wrap,bit3-ccmp, bit4-wep128

    uint8_t   dot11PassPhrase[65];    // passphrase
    uint8_t   dot11PassPhraseGuest[65];   // passphrase of guest

    uint32_t  dot11GKRekeyTime;   // group key rekey time, 0 - disable

#ifdef CONFIG_IEEE80211W
    uint32_t  dot11KeyMgmt;
    uint32_t  dot11MgmtGroupCipher;
#endif
    /* just wep here */
    uint8_t   key_64bits_arr[4][5];
    uint8_t   key_128bits_arr[4][13];

    uint8_t   is_eap_sim;
#ifdef CONFIG_OUTER_WPASUPP
    WIFISUPP_WPA_EAP_TYPE_E eap_type;
    uint32_t  simcard_num;
    uint8_t   peap_identity[32+1];
    uint32_t  peap_identity_len;
    uint8_t   peap_password[32+1];
    uint32_t  peap_password_len;
#endif //CONFIG_OUTER_WPASUPP
};

#ifndef MACADDRLEN
#define MACADDRLEN      6
#endif

struct wdsEntry {
    uint8_t macAddr [MACADDRLEN];
    uint32_t txRate;
}__attribute__((packed));

struct wds_info {
    int     wdsEnabled;
    int     wdsPure;    // act as WDS bridge only, no AP function
    int     wdsPriority;    // WDS packets have higer priority
    struct wdsEntry entry[NUM_WDS];
    int     wdsNum;     // number of WDS used
    int     wdsPrivacy;
    uint8_t   wdsWepKey[32];
    uint8_t   wdsMapingKey[NUM_WDS][32];
    int     wdsMappingKeyLen[NUM_WDS];
    int     wdsKeyId;
    uint8_t   wdsPskPassPhrase[65];
};

struct Dot11RsnIE {
    uint8_t   rsnie[128];
    uint8_t   rsnielen;
};

struct wlan_ethhdr_t {
    uint8_t daddr[6];
    uint8_t saddr[6];
    uint16_t type;
}__attribute__((packed));

struct aid_obj {
    void*   station;
    uint32_t  used;   // used == TRUE => has been allocated, used == FALSE => can be allocated
};

union pn48
{
    uint64_t    val;
    struct
    {
        uint8_t TSC0;
        uint8_t TSC1;
        uint8_t TSC2;
        uint8_t TSC3;
        uint8_t TSC4;
        uint8_t TSC5;
        uint8_t TSC6;
        uint8_t TSC7;
    } _byte_;
};

struct Keytype
{
    uint8_t skey[16];
};

struct Dot11EncryptKey
{
    //uint32_t dot11TTKeyLen;
    //uint32_t dot11TMicKeyLen;
    //struct Keytype dot11TTKey;
    //struct Keytype dot11TMicKey1;
    //struct Keytype dot11TMicKey2;
    union pn48 dot11TXPN48;
    //union pn48 dot11RXPN48;
};


struct Dot11KeyMappingsEntry
{
    //uint32_t dot11Privacy;
    //uint32_t keyInCam;
    //uint32_t keyid;
    struct Dot11EncryptKey dot11EncryptKey;
};

struct wpa_priv
{
    WPA_GLOBAL_INFO wpa_global_info;

    uint8_t macaddr[ETH_ALEN];
    uint8_t op_mode;   //adhoc,master,infra mode

    uint8_t ssid_len;
    uint8_t ssid[33];

    uint8_t pwd_len;
    uint8_t pwd[65];
    /* these were moved here from mib */
    struct Dot1180211AuthEntry  dot1180211AuthEntry;
    /* just used in AP mode, left here just
     * for build use now, we will delete it
     * later */
    struct Dot11KeyMappingsEntry dot11GroupKeysTable;
    //struct wds_info    dot11WdsInfo;
    //struct Dot11RsnIE dot11RsnIE;
    //struct aid_obj  aidarray[MAX_ASSOC_NUM];
} ;

enum {
    DRV_STATE_INIT  = 1,    /* driver has been init */
    DRV_STATE_OPEN  = 2,    /* driver is opened */
#ifdef UNIVERSAL_REPEATER
    DRV_STATE_VXD_INIT = 4, /* vxd driver has been opened */
    DRV_STATE_VXD_AP_STARTED    = 8, /* vxd ap has been started */
#endif //UNIVERSAL_REPEATER
    DRV_STATE_MAX
};

inline static int32_t _atoi(uint8_t *s, int32_t base)
{
    int32_t k = 0;

    k = 0;
    if (base == 10) {
        while (*s != '\0' && *s >= '0' && *s <= '9') {
            k = 10 * k + (*s - '0');
            s++;
        }
    }
    else {
        while (*s != '\0') {
            int32_t v;
            if ( *s >= '0' && *s <= '9')
                v = *s - '0';
            else if ( *s >= 'a' && *s <= 'f')
                v = *s - 'a' + 10;
            else if ( *s >= 'A' && *s <= 'F')
                v = *s - 'A' + 10;
            else {
                dbg(D_ERR, D_UWIFI_SUPP, "error hex format!\n");
                return 0;
            }
            k = 16 * k + v;
            s++;
        }
    }
    return k;
}

inline static int32_t get_array_val(uint8_t *dst, uint8_t *src, int32_t len)
{
    uint8_t tmpbuf[4];
    int32_t num=0;

    while (len > 0) {
        memcpy(tmpbuf, src, 2);
        tmpbuf[2]='\0';
        *dst++ = (uint8_t)_atoi(tmpbuf, 16);
        len-=2;
        src+=2;
        num++;
    }
    return num;
}

#define SSID(priv)              (priv->ssid)
#define SSID_LEN(priv)          (priv->ssid_len)

#define GET_MY_HWADDR(priv)     (priv->macaddr)

#endif //__WPAS_TYPES_H__
