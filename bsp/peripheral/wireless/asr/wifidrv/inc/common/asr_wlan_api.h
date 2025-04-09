/**
 ****************************************************************************************
 *
 * @file asr_wifi_api.h
 *
 * @brief WiFi API.
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#ifndef _ASR_WIFI_API_H_
#define _ASR_WIFI_API_H_

#include <stdint.h>
#ifdef AWORKS
#include <stdbool.h>
#endif
#ifdef ALIOS_SUPPORT
#include "typedef.h"
#endif

#define AP_MAX_BLACK_NUM 16
#define AP_MAX_WHITE_NUM 16
#define ETH_ALEN     6
#define MAX_PASSWORD_LEN  64
#define MAX_SSID_LEN 32

typedef enum
{
    CONFIG_OK,          /* indicate asr api set success and response OK */
    PARAM_RANGE,        /* indicate some asr api param is out of range */
    PARAM_MISS,         /* indicate asr api param is less than needed count */
    CONFIG_FAIL,        /* indicate asr api set failed, or execute fail */
    CONN_TIMEOUT,       /* indicate connect timeout in station mode */
    CONN_EAPOL_FAIL,    /* indicate 4-way handshake failed in station mode */
    CONN_DHCP_FAIL,     /* indicate got ip by dhcp failed in station mode */
    WAIT_PEER_RSP,
    RSP_NULL=0xFF
}asr_api_rsp_status_t;

#define ASR_STR_TO_INT_ERR  0xFFFFFFFF

#ifndef UINT8
typedef unsigned char  UINT8;
#endif
#ifndef UINT16
typedef unsigned short  UINT16;
#endif

#ifndef ALIOS_SUPPORT

#ifndef UINT32
typedef unsigned long  UINT32;
#endif
#ifndef INT32
typedef signed long INT32;
#endif
#endif

/** Character, 1 byte */
typedef char t_s8;
/** Unsigned character, 1 byte */
typedef unsigned char t_u8;


/** Short integer */
typedef signed short t_s16;
/** Unsigned short integer */
typedef unsigned short t_u16;

/** Long integer */
typedef signed long t_s32;

/** Unsigned long integer */
typedef unsigned long t_u32;


/** Long long integer */
typedef signed long long t_s64;
/** Unsigned long integer */
typedef unsigned long long t_u64;


//#define TARGETOS_nucleus
//#include "wifi_adapter_marvell1802.h"

#define MAX_CHANNELS        64
#define MAX_NUM_CLIENTS     10
#define MAX_COUNTRY         4
#define MAX_WEP_LEN         26

typedef  uint8_t uap_802_11_mac_addr[ETH_ALEN];

typedef struct _black_list{
    uint16_t count;
    uap_802_11_mac_addr peer_addr[AP_MAX_BLACK_NUM];
    uint8_t onoff;
}blacklist;

typedef struct _white_list{
    uint16_t count;
    uap_802_11_mac_addr peer_addr[AP_MAX_WHITE_NUM];
    uint8_t onoff;
}whitelist;

typedef struct _uap_config_scan_chan
{
    UINT16 action; /* 0: get, 1: set */
    UINT16 band;
    UINT16 total_chan; /* Total no. of channels in below array */
    UINT8 chan_num[MAX_CHANNELS]; /* Array of channel nos. for the given band */
} uap_config_scan_chan;


typedef struct _sta_info {
    t_u8  mac_address[ETH_ALEN];
    t_u8  power_mfg_status;
    t_s8  rssi;
} uap_sta_info;

typedef struct _sta_list {
    UINT16    sta_count;
    uap_sta_info  info[MAX_NUM_CLIENTS];
} sta_list;

typedef struct _uap_mac_filter
{
    t_u16 filter_mode;
    t_u16 mac_count;
    uap_802_11_mac_addr mac_list[AP_MAX_BLACK_NUM];
} uap_mac_filter;

typedef struct _uap_wpa_param
{
    t_u8 pairwise_cipher_wpa;
    t_u8 pairwise_cipher_wpa2;
    t_u8 group_cipher;
    t_u8 rsn_protection;
    t_u32 length;
    t_u8 passphrase[MAX_PASSWORD_LEN];
    t_u32 gk_rekey_time;
} uap_wpa_param;


typedef struct _uap_wep_key
{
    t_u8 key_index;
    t_u8 is_default;
    t_u16 length;
    t_u8 key[MAX_WEP_LEN];
} uap_wep_key;


typedef struct _uap_wep_param
{
    uap_wep_key key0;
    uap_wep_key key1;
    uap_wep_key key2;
    uap_wep_key key3;
} uap_wep_param;


typedef struct _uap_ps_sleep_param
{
    t_u32 ctrl_bitmap;
    t_u32 min_sleep;
    t_u32 max_sleep;
} uap_ps_sleep_param;

typedef struct _uap_inact_sleep_param
{
    t_u32 inactivity_to;
    t_u32 min_awake;
    t_u32 max_awake;
} uap_inact_sleep_param;

typedef struct _uap_ps_mgmt
{
    t_u16 flags;
    t_u16 ps_mode;
    uap_ps_sleep_param       sleep_param;
    uap_inact_sleep_param    inact_param;
} uap_ps_mgmt;

typedef struct _uap_config_param
{
    UINT16 action; /* 0: get, 1: set */
    UINT32 ssid_len;
    UINT8 ssid[MAX_SSID_LEN];
    UINT8 bcast_ssid_ctrl;
    UINT8 mode;
    UINT8 channel;
    UINT8 acs_mode;
    UINT8 rf_band;
    UINT8 channel_bandwidth;
    UINT8 sec_chan;
    UINT16 max_sta_count;
    UINT32 sta_ageout_timer;
    UINT32 ps_sta_ageout_timer;
    UINT16 auth_mode;
    UINT16 protocol;
    UINT16 ht_cap_info;
    UINT8 ampdu_param;
    UINT8 tx_power;
    UINT8 enable_2040coex;
    uap_mac_filter filter;
    uap_wpa_param wpa_cfg;
    uap_wep_param wep_cfg;
    UINT8 state_802_11d;
    char country_code[MAX_COUNTRY];
    uap_ps_mgmt power_mode;
} uap_config_param;

// UINT16 pkt_fwd_ctl;
// Bit 0: Packet forwarding handled by Host (0) or Firmware (1)
// Bit 1: Intra-BSS broadcast packets are allowed (0) or denied (1)
// Bit 2: Intra-BSS unicast packets are allowed (0) or denied (1)
// Bit 3: Inter-BSS unicast packets are allowed (0) or denied (1)

typedef struct _uap_pkt_fwd_ctl
{
    UINT16 action; /* 0: get, 1: set */
    UINT16 pkt_fwd_ctl;
} uap_pkt_fwd_ctl;

void Set_Scan_Chan(uap_config_scan_chan _chan);
uap_config_scan_chan Get_Scan_Chan(void);

void Set_Scan_Flag(uint8_t _flag);
uint8_t Get_Scan_Flag(void);

/**
 * info:    get sta list in ap mode
 * output:  sta_list *list
 * return:  0:success;else failed
*/
INT32 UAP_Stalist(sta_list *list);

/**
 * info:    config blacklist; 0:get,1:set
 * output:  uap_pkt_fwd_ctl *param
 * return:  0:success;else failed
*/
INT32 UAP_PktFwd_Ctl(uap_pkt_fwd_ctl *param);

/**
 * info:    set or get ap channel
 * output:  uap_config_scan_chan *param
 * return:  0:success;else failed
*/
INT32 UAP_Scan_Channels_Config(uap_config_scan_chan *param);


/**
 * info:   open or close blacklist
 * input:   uint8_t onoff;1:on;0:off
 * return:  0:success;else failed
*/
INT32 UAP_Black_List_Onoff(uint8_t onoff);

/**
 * info:    get blacklist
 * output:  blacklist *black_l
 * return:  0:success;else failed
*/
INT32 UAP_Black_List_Get(blacklist *black_l);

/**
 * info:    add mac to blacklist
 * input:   uap_802_11_mac_addr blackmac
 * return:  0:success;else failed
*/
INT32 UAP_Black_List_Add( uap_802_11_mac_addr blackmac);

/**
 * info:   delete mac from blacklist
 * input:  uap_802_11_mac_addr blackmac
 * return: 0:success;else failed
*/
INT32 UAP_Black_List_Del( uap_802_11_mac_addr blackmac);

/**
 * info:   clear all mac from blacklist
 * input:  none
 * return: 0:success;else failed
*/
INT32 UAP_Black_List_Clear(void);



/**
 * info:    open or close whitelist
 * input:   uint8_t onoff;1:on;0:off
 * return:  0:success;else failed
*/
INT32 UAP_White_List_Onoff(uint8_t onoff);

/**
 * info:    get whitelist
 * output:  whitelist *white_l
 * return:  0:success;else failed
*/
INT32 UAP_White_List_Get(whitelist *white_l);

/*
 * info:    add mac to whitelist
 * input:   uap_802_11_mac_addr whitemac
 * return:  0:success;else failed
*/
INT32 UAP_White_List_Add( uap_802_11_mac_addr whitemac);

/**
 * info:   delete mac from whitelist
 * input:  uap_802_11_mac_addr whitemac
 * return: 0:success;else failed
*/
INT32 UAP_White_List_Del( uap_802_11_mac_addr whitemac);


/**
 * info:   clear all mac from whitelist
 * input:  none
 * return: 0:success;else failed
*/
INT32 UAP_White_List_Clear(void);


/**
 * info:   deauth connected mac
 * input:  uap_802_11_mac_addr mac
 * return: 0:success;else failed
*/
INT32 UAP_Sta_Deauth(uap_802_11_mac_addr mac);


typedef enum _Set_Get_Action
{
    GET_ACTION = 0,
    SET_ACTION
}Set_Get_Action;

typedef enum _Filter
{
    DISABLE_FILTER = 0,
    WHITE_FILTER,
    BLACK_FILTER
}Filter;

typedef enum _Onoff
{
    DOFF = 0,
    DON
}Onoff;


INT32 UAP_Params_Config(uap_config_param *  uap_param);

INT32 UAP_GetCurrentChannel(void);


INT32 UAP_BSS_Config(INT32 start_stop);

#ifdef AWORKS
#undef NONE
#endif
/**
 *  @brief  wlan network interface enumeration definition.
 */
typedef enum {
  SOFTAP,   /*Act as an access point, and other station can connect, 4 stations Max*/
  STA,      /*Act as a station which can connect to an access point*/
  SNIFFER,        /*Act as a sniffer*/
  NONE,
} asr_wlan_type_e;

typedef enum {
    WLAN_DHCP_DISABLE = 0,  //use static ip address in station mode
    WLAN_DHCP_CLIENT,
    WLAN_DHCP_SERVER,
} asr_wlan_dhcp_mode_e;

typedef enum {
    EVENT_NONE,
    EVENT_STATION_UP = 1,  /*used in station mode,
                            indicate station associated in open mode or 4-way-handshake done in WPA/WPA2*/
    EVENT_STATION_AUTH,
    EVENT_STATION_4WAY_HANDSHAKE,
    EVENT_STATION_4WAY_HANDSHAKE_DONE,
    EVENT_STATION_DOWN,    /*used in station mode, indicate station deauthed*/
    EVENT_STA_CLOSE,       /*used in station mode, indicate station close done */
    EVENT_AP_UP,           /*used in softap mode, indicate softap enabled*/
    EVENT_AP_DOWN,         /*used in softap mode, indicate softap disabled*/
} asr_wifi_event_e;

/**
 *  @brief  Scan result using normal scan.
 */
typedef struct {
  uint8_t is_scan_adv;
  char ap_num;       /**< The number of access points found in scanning. */
  struct {
    char    ssid[32+1];  /*ssid max len:32. +1 is for '\0'. when ssidlen is 32  */
    signed char    ap_power;   /**< Signal strength, min:0, max:100. */
    uint8_t    bssid[6];     /* The BSSID of an access point. */
    char    channel;      /* The RF frequency, 1-13 */
    uint8_t security;     /* Security type, @ref wlan_sec_type_t */
  } * ap_list;
} asr_wlan_scan_result_t;

typedef enum {
    WLAN_SECURITY_OPEN,                   //NONE
    WLAN_SECURITY_WEP,                    //WEP
    WLAN_SECURITY_WPA,                    //WPA
    WLAN_SECURITY_WPA2,                   //WPA2
    WLAN_SECURITY_AUTO,                   //WPA or WPA2
    WLAN_SECURITY_MAX,
} asr_wlan_security_e;

/*WLAN error status*/
typedef enum{
    WLAN_STA_MODE_USER_CLOSE = 0,            //in sta mode, connect fail as reason is user close wifi
    WLAN_STA_MODE_BEACON_LOSS = 1,           //in sta mode, cannot receive beacon of peer connected AP for a long time
    WLAN_STA_MODE_SCAN_FAIL,                 //in sta mode, scan fail
    WLAN_STA_MODE_AUTH_FAIL,                 //in sta mode, connect fail during auth
    WLAN_STA_MODE_ASSOC_FAIL,                //in sta mode, connect fail during association
    WLAN_STA_MODE_PASSWORD_ERR,              //in sta mode, connect fail as password error
    WLAN_STA_MODE_NO_AP_FOUND,               //in sta mode, connect fail as cannot find the connecting AP during scan
    WLAN_STA_MODE_DHCP_FAIL,                 //in sta mode, connect fail as dhcp fail
    WLAN_STA_MODE_CONN_RETRY_MAX,            //in sta mode, connect fail as reach the max connect retry times
    WLAN_STA_MODE_CONN_REASON_0,             //in sta mode, connect fail as reason is 0
    WLAN_STA_MODE_CONN_TIMEOUT,              //in sta mode, connect fail as reason is timeout
    WLAN_STA_MODE_CONN_AP_CHG_PSK,           //in sta mode, when connected ap and ap change psk
    WLAN_STA_MODE_CONN_ENCRY_ERR,           //in sta mode, when connected ap and ap change psk

}asr_wlan_err_status_e;

/*WIFI READY status*/
typedef enum{
    WIFI_NOT_READY = 0,               // wifi is not ready
    WIFI_IS_READY = 1,                   // wifi is ready
}asr_wifi_ready_status_e;

/*SCAN status*/
typedef enum{
    SCAN_INIT = 0,                   // scan init value is 0
    SCAN_FINISH_DOWN = 1,            //scan finished and scan done
    SCAN_OTHER_ERR,                  //other link err
    SCAN_SCANING_ERR,                //is in scaning mode now,do not scan again
    SCAN_MODE_ERR,                  //scan mode error,need in ap or sta mode
    SCAN_CONNECT_ERR,               //when in connect mode ,do not scan
    SCAN_START,                     //when start scan set  start
    SCAN_DEINIT ,                   // wifi deinit then set scan deinit
    SCAN_USER_CLOSE,                // wifi close because user close wifi
    SCAN_TIME_OUT,                // scan time out for 10s
}asr_scan_status_e;

typedef enum{
    DEFAULT_SCAN = 0,           // default scan
    ADV_SCAN = 1,               // adv scan no use
    UI_SCAN = 2,                // ui call scan api
    RECONNECT_SCAN = 3,         // call scan when reconnect
    OPEN_STA_SCAN = 4,          // call scan api when open sta
}asr_scan_source_e;


/*used in event callback of station mode, indicate softap informatino which is connected*/
typedef struct {
    char    ssid[32+1];     /* ssid max len:32. +1 is for '\0' when ssidlen is 32  */
    char    pwd[64+1];      /* pwd max len:64. +1 is for '\0' when pwdlen is 64 */
    char    bssid[6];       /* BSSID of the wlan needs to be connected.*/
    char    ssid_len;       /*ssid length*/
    char    pwd_len;        /*password length*/
    char    channel;       /* wifi channel 0-13.*/
    char    security;      /*refer to asr_wlan_security_e*/
} asr_wlan_ap_info_adv_t;

/*only used in station mode*/
typedef struct {
    char    dhcp;             /* no use currently */
    char    macaddr[16];      /* mac address on the target wlan interface, ASCII*/
    char    ip[16];           /* Local IP address on the target wlan interface, ASCII*/
    char    gate[16];         /* Router IP address on the target wlan interface, ASCII */
    char    mask[16];         /* Netmask on the target wlan interface, ASCII*/
    char    dns[16];          /* no use currently , ASCII*/
    char    broadcastip[16];  /* no use currently , ASCII*/
} asr_wlan_ip_stat_t;

/*only used in station mode*/
typedef struct {
    int     is_connected;  /* The link to wlan is established or not, 0: disconnected, 1: connected. */
    int     wifi_strength; /* Signal strength of the current connected AP */
    char    ssid[32+1];      /* ssid max len:32. +1 is for '\0'. when ssidlen is 32  */
    char    bssid[6];      /* BSSID of the current connected wlan */
    int     channel;       /* Channel of the current connected wlan */
} asr_wlan_link_stat_t;

/* used in open cmd for AP mode */
typedef struct {
    char    ssid[32+1];     /* ssid max len:32. +1 is for '\0' when ssidlen is 32  */
    char    pwd[64+1];      /* pwd max len:64. +1 is for '\0' when pwdlen is 64 */
    int     interval;       /* beacon listen interval */
    int     hide;           /* hidden SSID */
    int     channel;       /* Channel*/
} asr_wlan_ap_init_t;

enum autoconnect_type {
    AUTOCONNECT_DISABLED = 0,
    AUTOCONNECT_SCAN_CONNECT,
    AUTOCONNECT_DIRECT_CONNECT,
    AUTOCONNECT_TYPE_MAX
};

/*used in open cmd of hal_wifi_module_t*/
typedef struct {
    char    wifi_mode;              /* refer to hal_wifi_type_t*/
    char    security;               /* security mode */
    char    wifi_ssid[33];          /* in station mode, indicate SSID of the wlan needs to be connected.
                                       in softap mode, indicate softap SSID*/
    char    wifi_key[65];           /* in station mode, indicate Security key of the wlan needs to be connected,
                                       in softap mode, indicate softap password.(ignored in an open system.) */
    char    local_ip_addr[16];      /* used in softap mode to config ip for dut */
    char    net_mask[16];           /* used in softap mode to config gateway for dut */
    char    gateway_ip_addr[16];    /* used in softap mode to config netmask for dut */
    char    dns_server_ip_addr[16]; /* no use currently */
    char    dhcp_mode;              /* no use currently */
    char    channel;                /* softap channel in softap mode; connect channel in sta mode*/
    char    mac_addr[6];            /* connect bssid in sta mode*/
    char    reserved[32];           /* no use currently */
    int     wifi_retry_interval;    /* no use currently */
    int     wifi_retry_times;       /* used in station mode to config reconnecting times after disconnected*/
    int     interval;               /* used in softap mode to config beacon listen interval */
    int     hide;                   /* used in softap mode to config hidden SSID */
    int     he_enable;              /* used in softap mode to config hidden SSID */
    enum autoconnect_type en_autoconnect;         /* 0: close autoconnect;1: scan 1st; 2. direct connect*/
} asr_wlan_init_type_t;

typedef struct {
    uint8_t vif_index;
    bool status;
} asr_wlan_rssi_t;
#define WIFI_MAC_ADDR_LEN    6
/// MAC address structure.
typedef struct wifi_mac_addr
{
    /// Array of 16-bit words that make up the MAC address.
    uint16_t array[WIFI_MAC_ADDR_LEN/2];
} mac_addr_t;

/** @brief  wifi init functin, user should call it before use any wifi cmd
 *  @return    0       : on success.
 *  @return    other   : error occurred
 */
int asr_wlan_init(void);

/** @brief  wifi deinit functin, call it when donot use wifi any more to free resources
 *  @return    0       : on success.
 *  @return    other   : error occurred
 */
int asr_wlan_deinit(void);

/** @brief  used in station and softap mode, open wifi cmd
 *
 * @param init_info    : refer to asr_wlan_init_type_t
 *
 * @return    0       : on success.
 * @return    other   : error occurred
 */
int asr_wlan_open(asr_wlan_init_type_t* init_info);
void asr_wlan_clear_pmk(void);

/** @brief  used in softap mode, open wifi cmd
 *
 * @param init_info    : refer to asr_wlan_ap_init_t
 *
 * @return    0       : on success.
 * @return    other   : error occurred
 */
int asr_wlan_ap_open(asr_wlan_ap_init_t* init_info);

/** @brief  used in station and softap mode, close wifi cmd
 *
 * @return    0       : on success.
 * @return    other   : error occurred
 */
int asr_wlan_close(void);

/** @brief  used in station mode, scan cmd
 *  @return    0       : on success.
 *  @return    other   : error occurred
 */
int asr_wlan_start_scan(void);

/** @brief  used in station mode, scan cmd
 *  @return    0       : on success.
 *  @return    other   : error occurred
 */
int asr_wlan_start_scan_adv(void);

/** @brief  used in station mode, scan cmd
 *
 * @param ssid    : target ssid to scan
 *
 *  @return    0       : on success.
 *  @return    other   : error occurred
 */
int asr_wlan_start_scan_active(const char *ssid);

/** @brief  used in station and softap mode, get mac address(in hex mode) of WIFI device
 *
 * @param mac_addr    : pointer to get the mac address
 *
 *  @return    0       : on success.
 *  @return    other   : error occurred
 */
int asr_wlan_get_mac_address(uint8_t *mac_addr);

/** @brief  used in station and softap mode, set mac address for WIFI device
 *
 *  @param mac_addr    : src mac address pointer to set
 *
 */
void asr_wlan_set_mac_address(uint8_t *mac_addr);


/** @brief  used in station mode, get the ip information
 *
 * @param void
 * @return    NULL    : error occurred.
 * @return    pointer : ip status got.
 */
asr_wlan_ip_stat_t * asr_wlan_get_ip_status(void);


/** @brief  used in station mode, get link status information
 *
 * @param link_status    : refer to asr_wlan_link_stat_t
 *
 * @return    0       : on success.
 * @return    other   : error occurred
 */
int asr_wlan_get_link_status(asr_wlan_link_stat_t *link_status);

/** @brief  used in station mode, get the associated ap information
 *
 * @param void
 * @return    NULL    : error occurred.
 * @return    pointer : associated ap info got.
 */
asr_wlan_ap_info_adv_t *asr_wlan_get_associated_apinfo(void);

/*used in sniffer mode, open sniffer mode
*  @return    0       : on success.
*  @return    other   : error occurred
*/
int asr_wlan_start_monitor(void);

/*used in sniffer mode, close sniffer mode
*  @return    0       : on success.
*  @return    other   : error occurred
*/
int asr_wlan_stop_monitor(void);

/** @brief  used in sniffer mode, set the sniffer channel, should call this cmd after call start_monitor cmd
 *
 * @param channel    : WIFI channel(1-13)
 *  @return    0       : on success.
 *  @return    other   : error occurred
 */
int asr_wlan_monitor_set_channel(int channel);

/** @brief  used in sta mode, set the ps bc mc and listen interval, called before connect to ap.
 *
 * @param listen_bc_mc    : true or false
 * @param listen_interval :1, 3, 10
 *  @return    0       : on success.
 *  @return    other   : error occurred
 */
int asr_wlan_set_ps_options(uint8_t listen_bc_mc, uint16_t listen_interval);

/** @brief  used in sta mode, set ps mode on/off
 *
 * @param ps_on    : true or false
 *  @return    0       : on success.
 *  @return    other   : error occurred
 */
int asr_wlan_set_ps_mode(uint8_t ps_on);
int asr_wlan_set_dbg_cmd(unsigned int host_dbg_cmd);

/*when use monitor mode, user should register this type of callback function to get the received MPDU*/
typedef void (*monitor_cb_t)(uint8_t*data, int len, uint8_t channel);

/** @brief  used in sniffer mode, callback function to get received MPDU, should register before start_monitor
 *
 * @param fn    : refer to monitor_data_cb_t
 *
 *  @return    0       : on success.
 *  @return    other   : error occurred
 */
int asr_wlan_register_monitor_cb(monitor_cb_t fn);

/** @brief  used in station mode or sniffer mode, call this cmd to send a MPDU constructed by user
 *
 * @param buf    :  mac header pointer of the MPDU
 * @param len    :  length of the MPDU
 *
 * @return    0       : on success.
 * @return    other   : error occurred
 */
int asr_wlan_send_raw_frame(uint8_t *buf, int len);

/*enable WIFI stack log, will be output by uart
*
* @return    0       : on success.
* @return    other   : error occurred
*/
int asr_wlan_start_debug_mode(void);

/*disable WIFI stack log
*
* @return    0       : on success.
* @return    other   : error occurred
*/
int asr_wlan_stop_debug_mode(void);

/*
 * The event callback function called at specific wifi events occurred by wifi stack.
 * user should register these callback if want to use the informatin.
 *
 * @note For HAL implementors, these callbacks must be
 *       called under normal task context, not from interrupt.
 */
typedef void (*asr_wlan_cb_ip_got)(asr_wlan_ip_stat_t *ip_status);
/** @brief  used in station mode, WIFI stack call this cb when get ip
 *
 * @param fn    : cb function type, refer to asr_wlan_ip_stat_t
 *
 * @return    0       : on success.
 * @return    other   : error occurred
 */
int asr_wlan_ip_got_cb_register(asr_wlan_cb_ip_got fn);

typedef void (*asr_wlan_cb_stat_chg)(asr_wifi_event_e wlan_event);
/** @brief  used in station and softap mode,
 *          WIFI stack call this cb when status change, refer to asr_wifi_event_e
 *
 * @param fn    : cb function type
 *
 * @return    0       : on success.
 * @return    other   : error occurred
 */
int asr_wlan_stat_chg_cb_register(asr_wlan_cb_stat_chg fn);

typedef void (*asr_wlan_cb_scan_compeleted)(asr_wlan_scan_result_t *result);
/** @brief  used in station mode,
 *          WIFI stack call this cb when scan complete
 *
 * @param fn    : cb function type
 *
 * @return    0       : on success.
 * @return    other   : error occurred
 */
int asr_wlan_scan_compeleted_cb_register(asr_wlan_cb_scan_compeleted fn);

typedef void (*asr_wlan_cb_associated_ap)(asr_wlan_ap_info_adv_t *ap_info);
/** @brief  used in station mode,
 *          WIFI stack call this cb when associated with an AP, and tell the AP information
 *
 * @param fn    : cb function type
 *
 * @return    0       : on success.
 * @return    other   : error occurred
 */
int asr_wlan_associated_ap_cb_register(asr_wlan_cb_associated_ap fn);

typedef void (*asr_wlan_err_stat_handler)(asr_wlan_err_status_e err_info);
/** @brief  user use to register err status callback function,
 *          WIFI stack call this cb when some error occured, refer to asr_wlan_err_status_e
 *
 * @param fn    : cb function type
 *
 * @return    0       : on success.
 * @return    other   : error occurred
 */
int asr_wlan_err_stat_cb_register(asr_wlan_err_stat_handler fn);

typedef void (*asr_wlan_ap_dev_handler)(char* mac);
int asr_wlan_ap_add_dev_cb_register(asr_wlan_ap_dev_handler fn);
int asr_wlan_ap_del_dev_cb_register(asr_wlan_ap_dev_handler fn);

int asr_wlan_sniffer_mgmt_cb_register(uint32_t asr_wlan_mgmt_sniffer);

// typedef void (*asr_wlan_cb_rssi)(asr_wlan_rssi_t *rssi);
// int asr_wlan_rssi_cb_register(asr_wlan_cb_rssi fn);
typedef void (*asr_wlan_cb_rssi)(uint8_t rssi_level);
int asr_wlan_rssi_cb_register(asr_wlan_cb_rssi fn);
/** @brief  calibration RCO clock for RTC
 *
 */
void asr_drv_rco_cal(void);

/** @brief  config to close DCDC PFM mode
 *
 */
void asr_drv_close_dcdc_pfm(void);

/** @brief  config to support smartconfig in MIMO scenario
 *
 */
void asr_wlan_smartconfig_mimo_enable(void);
/*


*/
/** @brief  set country code to update country code, different country may have different channel list
 *   called after hal_wifi_init
 */
 // 1
void asr_wlan_set_country_code(char *country);

/** @brief query the sta list which connected to the AP

*/

typedef struct {
    uint8_t sta_nums;
    mac_addr_t mac_addr [12];//FIXME
} ap_sta_info;
// 2
bool asr_wlan_get_sta_list(ap_sta_info *sta);

/** @brief setup unicast/broadcast packet would be forward or not

    @input pkt_fwd_setting
        bit 0: packet forwarding handled by host(0) or firmware(1)
        bit 1: intra-bss broadcast are allowed(0) or denied(1)
        bit 2: intra-bss unicast are allowed(0) or denied(1)
        bit 3: intra-bss unicast are allowed(0) or denied(1)
*/
// 3
void asr_wlan_set_pkt_forward(uint8_t pkt_fwd_setting);

/** @brief ap channel set by a list, detect channel signal and determine the best channel
*/
// 4
void asr_wlan_ap_chan_config(uint8_t *chan_list);

/** @brief setup the ap's white and black list
*/
// 5
void asr_wlan_set_white_list(mac_addr_t *white_list);
// 6
void asr_wlan_set_black_list(mac_addr_t *black_list);
/** @brief do deauth a specific station in softap mode
*/
// 7
void asr_wlan_deauth_sta(mac_addr_t *mac_addr);
#ifdef CFG_DBG
/** @brief  Set if fw log output to Host side
 *  @input log_switch
 *      1 enable fw log output to Host
 *      0 dissable fw log output to Host
 *  enable fw log output to Host will consume one port from card to host, so for performance consideration, it
 *  is disabled in default.
 *
 */
int asr_wlan_host_log_set(uint8_t log_switch);
#endif

int asr_wlan_get_err_code(uint8_t mode);


void asr_wifi_event_cb_register(void);
void asr_wifi_enable_rssi_level_indication(void);

static inline int is_zero_mac_addr(const uint8_t *a)
{
    return !(a[0] | a[1] | a[2] | a[3] | a[4] | a[5]);
}

const char *asr_get_wifi_version(void);


int asr_wlan_suspend_sta(void);

int asr_wifi_get_rssi(int8_t *rssi);
int asr_wifi_set_upload_fram(uint8_t fram_type, uint8_t enable);
int asr_wifi_set_appie(uint8_t fram_type, uint8_t *ie, uint16_t ie_len);

//int asr_wifi_write_rf_efuse(uint8_t * txpwr, uint8_t * txevm, uint8_t *freq_err, bool iswrite, uint8_t *index);

#endif  //_ASR_WIFI_API_H_
