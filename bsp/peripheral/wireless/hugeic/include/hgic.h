#ifndef _HUGE_IC_H_
#define _HUGE_IC_H_
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __RTOS__
#define HGIC_CMD_START 100
#else
#define HGIC_CMD_START 0
#endif

#ifndef __packed
#define __packed          __attribute__((packed))
#endif

typedef void (*hgic_init_cb)(void *args);
typedef void (*hgic_event_cb)(char *ifname, int event, long param1, long param2);

struct hgic_bss_info {
    unsigned char bssid[6];
    unsigned char ssid[32];
    unsigned char encrypt;
    signed   char signal;
    unsigned short freq;
};

struct hgic_bss_info1 {
    unsigned char bssid[6];
    unsigned char ssid[32];
    unsigned char encrypt: 6, ver: 2;
    signed   char signal;
    unsigned short freq;
    unsigned char country_region[4];
    unsigned char bss_bw;
};

struct hgic_fw_info {
    unsigned int version;
    unsigned int svn_version;
    unsigned short chip_id;
    unsigned short cpu_id;
    unsigned char  mac[6];
    unsigned char  resv[2];
    unsigned int app_version;
    unsigned int  smt_dat;
};

struct hgic_sta_info {
    unsigned char aid;
    unsigned char ps:1;
    unsigned char addr[6];
    signed char rssi;
    signed char evm;
    signed char tx_snr;
    signed char rx_snr;
};

struct hgic_freqinfo {
    unsigned char  bss_bw, chan_cnt;
    unsigned short freq_start, freq_end;
    unsigned short chan_list[16];
};

struct hgic_module_hwinfo{
    union{
        struct{
            unsigned char type;
            unsigned char saw:1, rev:7;
        };
        unsigned short v;
    };
};

struct hgic_txq_param {
    unsigned short txop;
    unsigned short cw_min;
    unsigned short cw_max;
    unsigned char  aifs;
    unsigned char  acm;
};

struct hgic_mcast_txparam {
    unsigned char dupcnt;
    unsigned char tx_bw;
    unsigned char tx_mcs;
    unsigned char clearch;
};

struct hgic_rx_info {
    unsigned char  band;
    unsigned char  mcs: 4, bw: 4;
    char  evm;
    char  signal;
    unsigned short freq;
    short freq_off;
    unsigned char  rx_flags;
    unsigned char  antenna;
    unsigned char  nss : 4, s1g_nss : 4;
    unsigned char  vht_flag : 3, s1g_flag : 5;
};

struct hgic_tx_info {
    unsigned char  band;
    unsigned char  tx_bw;
    unsigned char  tx_mcs;
    unsigned char  freq_idx: 5, antenna: 3;
    unsigned int   tx_flags;
    unsigned short tx_flags2;
    unsigned char  priority;
    unsigned char  tx_power;
};

struct bt_rx_info {
    unsigned char   channel;    //current channel
    unsigned char   con_handle; //hci handle
    signed char     rssi;
    unsigned char   frm_type;
    unsigned int    rev;
};


/*data packet header*/
struct hgic_hdr {
    unsigned short magic;
    unsigned char  type;
    unsigned char  ifidx: 4, flags: 4;
    unsigned short length;
    unsigned short cookie;
} __packed;

struct hgic_frm_hdr {
    struct hgic_hdr hdr;
    union {
        struct hgic_rx_info rx_info;
        struct hgic_tx_info tx_info;
        unsigned char  rev[24];
    };
} __packed;

struct hgic_frm_hdr2 {
    struct hgic_hdr hdr;
} __packed;


/*contro pakcet header*/
struct hgic_ctrl_hdr {
    struct hgic_hdr hdr;
    union {
        struct {
            unsigned char  cmd_id;
            short status;
        } cmd;
        struct {
            unsigned short  cmd_id;
            short status;
        } cmd2;
        struct {
            unsigned char event_id;
            short value;
        } event;
        struct {
            unsigned short event_id;
            short value;
        } event2;
        struct {
            unsigned char  type;
        } hci;
        unsigned char info[4];
    };
} __packed;

struct hgic_key_conf {
    unsigned int cipher;
    unsigned int flags;
    unsigned char keyidx;
    unsigned char keylen;
    unsigned char key[0];
};

struct hgic_cca_ctl {
    char    start_th;
    char    mid_th;
    char    ed_th;
    /* CCA auto adjustment.
     * When the cca automatic adjustment takes effect, the
     * above three parameters are invalid.
     */
    char   auto_en     : 1;
};

struct hgic_acs_result{
    unsigned int  freq;//KHz
    unsigned char sync_cnt;
    signed char   min;
    signed char   max;
    signed char   avg;
};

struct hgic_fallback_mcs {
    unsigned char original_type;
    unsigned char original_mcs;
    unsigned char fallback_type;
    unsigned char fallback_mcs;
};

#define HDR_CMDID(ctl) ((ctl)->hdr.type==HGIC_HDR_TYPE_CMD2? le16_to_cpu((ctl)->cmd2.cmd_id):(ctl)->cmd.cmd_id)
#define HDR_EVTID(ctl) ((ctl)->hdr.type==HGIC_HDR_TYPE_EVENT2?le16_to_cpu((ctl)->event2.event_id):(ctl)->event.event_id)
#define HDR_CMDID_SET(ctl, id) if(id>255){\
        (ctl)->hdr.type =HGIC_HDR_TYPE_CMD2;\
        (ctl)->cmd2.cmd_id = cpu_to_le16(id);\
    }else{\
        (ctl)->hdr.type =HGIC_HDR_TYPE_CMD;\
        (ctl)->cmd.cmd_id = id;\
    }
#define HDR_EVTID_SET(ctl, id) if(id>255){\
        (ctl)->hdr.type =HGIC_HDR_TYPE_EVENT2;\
        (ctl)->event2.event_id = cpu_to_le16(id);\
    }else{\
        (ctl)->hdr.type =HGIC_HDR_TYPE_EVENT;\
        (ctl)->event.event_id = id;\
    }

enum hgic_hdr_type {
    HGIC_HDR_TYPE_ACK         = 1,
    HGIC_HDR_TYPE_FRM         = 2,
    HGIC_HDR_TYPE_CMD         = 3,
    HGIC_HDR_TYPE_EVENT       = 4,
    HGIC_HDR_TYPE_FIRMWARE    = 5,
    HGIC_HDR_TYPE_NLMSG       = 6,
    HGIC_HDR_TYPE_BOOTDL      = 7,
    HGIC_HDR_TYPE_TEST        = 8,
    HGIC_HDR_TYPE_FRM2        = 9,
    HGIC_HDR_TYPE_TEST2       = 10,
    HGIC_HDR_TYPE_SOFTFC      = 11,
    HGIC_HDR_TYPE_OTA         = 12,
    HGIC_HDR_TYPE_CMD2        = 13,
    HGIC_HDR_TYPE_EVENT2      = 14,
    HGIC_HDR_TYPE_BOOTDL_DATA = 15,
    HGIC_HDR_TYPE_IFBR        = 16,
    HGIC_HDR_TYPE_BEACON      = 17,
    HGIC_HDR_TYPE_AGGFRM      = 18,
    HGIC_HDR_TYPE_BLUETOOTH   = 19,

    HGIC_HDR_TYPE_MAX,
};

enum hgic_cmd {
    HGIC_CMD_DEV_OPEN               = HGIC_CMD_START + 1,   /* fmac/smac */
    HGIC_CMD_DEV_CLOSE              = HGIC_CMD_START + 2,   /* fmac/smac */
    HGIC_CMD_SET_MAC                = HGIC_CMD_START + 3,   /* fmac/smac */
    HGIC_CMD_SET_SSID               = HGIC_CMD_START + 4,   /* fmac */
    HGIC_CMD_SET_BSSID              = HGIC_CMD_START + 5,   /* fmac */
    HGIC_CMD_SET_COUNTRY            = HGIC_CMD_START + 6,   /* fmac */
    HGIC_CMD_SET_CHANNEL            = HGIC_CMD_START + 7,   /* fmac */
    HGIC_CMD_SET_CENTER_FREQ        = HGIC_CMD_START + 8,   /* smac */
    HGIC_CMD_SET_RTS_THRESHOLD      = HGIC_CMD_START + 9,   /* smac */
    HGIC_CMD_SET_FRG_THRESHOLD      = HGIC_CMD_START + 10,  /* smac */
    HGIC_CMD_SET_KEY_MGMT           = HGIC_CMD_START + 11,  /* fmac */
    HGIC_CMD_SET_WPA_PSK            = HGIC_CMD_START + 12,  /* fmac */
    HGIC_CMD_SET_KEY                = HGIC_CMD_START + 13,  /* smac */
    HGIC_CMD_SCAN                   = HGIC_CMD_START + 14,  /* fmac */
    HGIC_CMD_GET_SCAN_LIST          = HGIC_CMD_START + 15,  /* fmac */
    HGIC_CMD_SET_BSSID_FILTER       = HGIC_CMD_START + 16,  /* fmac */
    HGIC_CMD_DISCONNECT             = HGIC_CMD_START + 17,  /* fmac */
    HGIC_CMD_GET_BSSID              = HGIC_CMD_START + 18,  /* fmac */
    HGIC_CMD_SET_WBNAT              = HGIC_CMD_START + 19,  /* unused */
    HGIC_CMD_GET_STATUS             = HGIC_CMD_START + 20,  /* fmac */
    HGIC_CMD_SET_LISTEN_INTERVAL    = HGIC_CMD_START + 21,  /* smac */
    HGIC_CMD_SET_TX_POWER           = HGIC_CMD_START + 22,  /* fmac/smac */
    HGIC_CMD_GET_TX_POWER           = HGIC_CMD_START + 23,  /* fmac/smac */
    HGIC_CMD_SET_TX_LCOUNT          = HGIC_CMD_START + 24,  /* unused */
    HGIC_CMD_SET_TX_SCOUNT          = HGIC_CMD_START + 25,  /* unused */
    HGIC_CMD_ADD_STA                = HGIC_CMD_START + 26,  /* smac */
    HGIC_CMD_REMOVE_STA             = HGIC_CMD_START + 27,  /* smac */
    HGIC_CMD_SET_TX_BW              = HGIC_CMD_START + 28,  /* fmac */
    HGIC_CMD_SET_TX_MCS             = HGIC_CMD_START + 29,  /* fmac/smac */
    HGIC_CMD_SET_FREQ_RANGE         = HGIC_CMD_START + 30,  /* fmac */
    HGIC_CMD_ACS_ENABLE             = HGIC_CMD_START + 31,  /* fmac */
    HGIC_CMD_SET_PRIMARY_CHAN       = HGIC_CMD_START + 32,  /* fmac */
    HGIC_CMD_SET_BG_RSSI            = HGIC_CMD_START + 33,  /* fmac */
    HGIC_CMD_SET_BSS_BW             = HGIC_CMD_START + 34,  /* fmac/smac */
    HGIC_CMD_TESTMODE_CMD           = HGIC_CMD_START + 35,  /* fmac/smac */
    HGIC_CMD_SET_AID                = HGIC_CMD_START + 36,  /* smac */
    HGIC_CMD_GET_FW_STATE           = HGIC_CMD_START + 37,  /* unused */
    HGIC_CMD_SET_TXQ_PARAM          = HGIC_CMD_START + 38,  /* smac */
    HGIC_CMD_SET_CHAN_LIST          = HGIC_CMD_START + 39,  /* fmac */
    HGIC_CMD_GET_CONN_STATE         = HGIC_CMD_START + 40,  /* fmac */
    HGIC_CMD_SET_WORK_MODE          = HGIC_CMD_START + 41,  /* fmac */
    HGIC_CMD_SET_PAIRED_STATIONS    = HGIC_CMD_START + 42,  /* fmac */
    HGIC_CMD_GET_FW_INFO            = HGIC_CMD_START + 43,  /* fmac/smac */
    HGIC_CMD_PAIRING                = HGIC_CMD_START + 44,  /* fmac */
    HGIC_CMD_GET_TEMPERATURE        = HGIC_CMD_START + 45,  /* fmac/smac */
    HGIC_CMD_ENTER_SLEEP            = HGIC_CMD_START + 46,  /* fmac */
    HGIC_CMD_OTA                    = HGIC_CMD_START + 47,  /* fmac */
    HGIC_CMD_GET_SSID               = HGIC_CMD_START + 48,  /* fmac */
    HGIC_CMD_GET_WPA_PSK            = HGIC_CMD_START + 49,  /* fmac */
    HGIC_CMD_GET_SIGNAL             = HGIC_CMD_START + 50,  /* fmac */
    HGIC_CMD_GET_TX_BITRATE         = HGIC_CMD_START + 51,  /* fmac */
    HGIC_CMD_SET_BEACON_INT         = HGIC_CMD_START + 52,  /* fmac */
    HGIC_CMD_GET_STA_LIST           = HGIC_CMD_START + 53,  /* fmac */
    HGIC_CMD_SAVE_CFG               = HGIC_CMD_START + 54,  /* fmac */
    HGIC_CMD_JOIN_GROUP             = HGIC_CMD_START + 55,  /* unused */
    HGIC_CMD_SET_ETHER_TYPE         = HGIC_CMD_START + 56,  /* unused */
    HGIC_CMD_GET_STA_COUNT          = HGIC_CMD_START + 57,  /* fmac */
    HGIC_CMD_SET_HEARTBEAT_INT      = HGIC_CMD_START + 58,  /* fmac */
    HGIC_CMD_SET_MCAST_KEY          = HGIC_CMD_START + 59,  /* unused */
    HGIC_CMD_SET_AGG_CNT            = HGIC_CMD_START + 60,  /* fmac/smac */
    HGIC_CMD_GET_AGG_CNT            = HGIC_CMD_START + 61,  /* fmac/smac */
    HGIC_CMD_GET_BSS_BW             = HGIC_CMD_START + 62,  /* fmac/smac */
    HGIC_CMD_GET_FREQ_RANGE         = HGIC_CMD_START + 63,  /* fmac */
    HGIC_CMD_GET_CHAN_LIST          = HGIC_CMD_START + 64,  /* fmac */
    HGIC_CMD_RADIO_ONOFF            = HGIC_CMD_START + 65,  /* fmac/smac */
    HGIC_CMD_SET_PS_HEARTBEAT       = HGIC_CMD_START + 66,  /* fmac */
    HGIC_CMD_SET_WAKEUP_STA         = HGIC_CMD_START + 67,  /* fmac */
    HGIC_CMD_SET_PS_HEARTBEAT_RESP  = HGIC_CMD_START + 68,  /* fmac */
    HGIC_CMD_SET_PS_WAKEUP_DATA     = HGIC_CMD_START + 69,  /* fmac */
    HGIC_CMD_SET_PS_CONNECT         = HGIC_CMD_START + 70,  /* fmac */
    HGIC_CMD_SET_BSS_MAX_IDLE       = HGIC_CMD_START + 71,  /* fmac */
    HGIC_CMD_SET_WKIO_MODE          = HGIC_CMD_START + 72,  /* fmac */
    HGIC_CMD_SET_DTIM_PERIOD        = HGIC_CMD_START + 73,  /* fmac */
    HGIC_CMD_SET_PS_MODE            = HGIC_CMD_START + 74,  /* fmac */
    HGIC_CMD_LOAD_DEF               = HGIC_CMD_START + 75,  /* fmac */
    HGIC_CMD_DISASSOC_STA           = HGIC_CMD_START + 76,  /* fmac */
    HGIC_CMD_SET_APLOST_TIME        = HGIC_CMD_START + 77,  /* fmac */
    HGIC_CMD_GET_WAKEUP_REASON      = HGIC_CMD_START + 78,  /* fmac */
    HGIC_CMD_UNPAIR                 = HGIC_CMD_START + 79,  /* fmac */
    HGIC_CMD_SET_AUTO_CHAN_SWITCH   = HGIC_CMD_START + 80,  /* fmac */
    HGIC_CMD_SET_REASSOC_WKHOST     = HGIC_CMD_START + 81,  /* fmac */
    HGIC_CMD_SET_WAKEUP_IO          = HGIC_CMD_START + 82,  /* fmac */
    HGIC_CMD_DBGINFO_OUTPUT         = HGIC_CMD_START + 83,  /* fmac/smac */
    HGIC_CMD_SET_SYSDBG             = HGIC_CMD_START + 84,  /* fmac/smac */
    HGIC_CMD_SET_AUTO_SLEEP_TIME    = HGIC_CMD_START + 85,  /* fmac */
    HGIC_CMD_GET_KEY_MGMT           = HGIC_CMD_START + 86,  /* fmac */
    HGIC_CMD_SET_PAIR_AUTOSTOP      = HGIC_CMD_START + 87,  /* fmac */
    HGIC_CMD_SET_SUPER_PWR          = HGIC_CMD_START + 88,  /* fmac */
    HGIC_CMD_SET_REPEATER_SSID      = HGIC_CMD_START + 89,  /* fmac */
    HGIC_CMD_SET_REPEATER_PSK       = HGIC_CMD_START + 90,  /* fmac */
    HGIC_CMD_CFG_AUTO_SAVE          = HGIC_CMD_START + 91,  /* fmac */
    HGIC_CMD_SEND_CUST_MGMT         = HGIC_CMD_START + 92,  /* fmac */
    HGIC_CMD_GET_BATTERY_LEVEL      = HGIC_CMD_START + 93,  /* unused */
    HGIC_CMD_SET_DCDC13             = HGIC_CMD_START + 94,  /* fmac */
    HGIC_CMD_SET_ACKTMO             = HGIC_CMD_START + 95,  /* fmac/smac */
    HGIC_CMD_GET_MODULETYPE         = HGIC_CMD_START + 96,  /* fmac/smac */
    HGIC_CMD_PA_PWRCTRL_DIS         = HGIC_CMD_START + 97,  /* fmac */
    HGIC_CMD_SET_DHCPC              = HGIC_CMD_START + 98,  /* fmac */
    HGIC_CMD_GET_DHCPC_RESULT       = HGIC_CMD_START + 99,  /* fmac */
    HGIC_CMD_SET_WKUPDATA_MASK      = HGIC_CMD_START + 100, /* fmac */
    HGIC_CMD_GET_WKDATA_BUFF        = HGIC_CMD_START + 101, /* fmac */
    HGIC_CMD_GET_DISASSOC_REASON    = HGIC_CMD_START + 102, /* fmac */
    HGIC_CMD_SET_WKUPDATA_SAVEEN    = HGIC_CMD_START + 103, /* fmac */
    HGIC_CMD_SET_CUST_DRIVER_DATA   = HGIC_CMD_START + 104, /* fmac */
    HGIC_CMD_SET_MCAST_TXPARAM      = HGIC_CMD_START + 105, /* unused */
    HGIC_CMD_SET_STA_FREQINFO       = HGIC_CMD_START + 106, /* fmac */
    HGIC_CMD_SET_RESET_STA          = HGIC_CMD_START + 107, /* fmac */
    HGIC_CMD_SET_UART_FIXLEN        = HGIC_CMD_START + 108, /* unused */
    HGIC_CMD_GET_UART_FIXLEN        = HGIC_CMD_START + 109, /* unused */
    HGIC_CMD_SET_ANT_AUTO           = HGIC_CMD_START + 110, /* fmac */
    HGIC_CMD_SET_ANT_SEL            = HGIC_CMD_START + 111, /* fmac */
    HGIC_CMD_GET_ANT_SEL            = HGIC_CMD_START + 112, /* fmac */
    HGIC_CMD_SET_WKUP_HOST_REASON   = HGIC_CMD_START + 113, /* fmac */
    HGIC_CMD_SET_MAC_FILTER_EN      = HGIC_CMD_START + 114, /* unused */
    HGIC_CMD_SET_ATCMD              = HGIC_CMD_START + 115, /* fmac/smac */
    HGIC_CMD_SET_ROAMING            = HGIC_CMD_START + 116, /* fmac */
    HGIC_CMD_SET_AP_HIDE            = HGIC_CMD_START + 117, /* fmac */
    HGIC_CMD_SET_DUAL_ANT           = HGIC_CMD_START + 118, /* fmac */
    HGIC_CMD_SET_MAX_TCNT           = HGIC_CMD_START + 119, /* fmac/smac */
    HGIC_CMD_SET_ASSERT_HOLDUP      = HGIC_CMD_START + 120, /* fmac/smac */
    HGIC_CMD_SET_AP_PSMODE_EN       = HGIC_CMD_START + 121, /* fmac */
    HGIC_CMD_SET_DUPFILTER_EN       = HGIC_CMD_START + 122, /* fmac */
    HGIC_CMD_SET_DIS_1V1_M2U        = HGIC_CMD_START + 123, /* fmac */
    HGIC_CMD_SET_DIS_PSCONNECT      = HGIC_CMD_START + 124, /* fmac */
    HGIC_CMD_SET_RTC                = HGIC_CMD_START + 125, /* fmac */
    HGIC_CMD_GET_RTC                = HGIC_CMD_START + 126, /* fmac */
    HGIC_CMD_SET_KICK_ASSOC         = HGIC_CMD_START + 127, /* fmac */
    HGIC_CMD_START_ASSOC            = HGIC_CMD_START + 128, /* fmac */
    HGIC_CMD_SET_AUTOSLEEP          = HGIC_CMD_START + 129, /* fmac */
    HGIC_CMD_SEND_BLENC_DATA        = HGIC_CMD_START + 130, /* smac */
    HGIC_CMD_SET_BLENC_EN           = HGIC_CMD_START + 131, /* smac */
    HGIC_CMD_RESET                  = HGIC_CMD_START + 132, /* fmac/smac */
    HGIC_CMD_SET_HWSCAN             = HGIC_CMD_START + 133, /* smac */
    HGIC_CMD_GET_TXQ_PARAM          = HGIC_CMD_START + 134, /* smac */
    HGIC_CMD_SET_PROMISC            = HGIC_CMD_START + 135, /* smac */
    HGIC_CMD_SET_USER_EDCA          = HGIC_CMD_START + 136, /* fmac */
    HGIC_CMD_SET_FIX_TXRATE         = HGIC_CMD_START + 137, /* smac */
    HGIC_CMD_SET_NAV_MAX            = HGIC_CMD_START + 138, /* smac */
    HGIC_CMD_CLEAR_NAV              = HGIC_CMD_START + 139, /* smac */
    HGIC_CMD_SET_CCA_PARAM          = HGIC_CMD_START + 140, /* smac */
    HGIC_CMD_SET_TX_MODGAIN         = HGIC_CMD_START + 141, /* smac */
    HGIC_CMD_GET_NAV                = HGIC_CMD_START + 142, /* smac */
    HGIC_CMD_SET_BEACON_START       = HGIC_CMD_START + 143, /* smac */
    HGIC_CMD_SET_BLE_OPEN           = HGIC_CMD_START + 144, /* smac */
    HGIC_CMD_GET_MODE               = HGIC_CMD_START + 145, /* fmac */
    HGIC_CMD_GET_BGRSSI             = HGIC_CMD_START + 146, /* fmac */
    HGIC_CMD_SEND_BLENC_ADVDATA     = HGIC_CMD_START + 147, /* smac */
    HGIC_CMD_SEND_BLENC_SCANRESP    = HGIC_CMD_START + 148, /* smac */
    HGIC_CMD_SEND_BLENC_DEVADDR     = HGIC_CMD_START + 149, /* smac */
    HGIC_CMD_SEND_BLENC_ADVINTERVAL = HGIC_CMD_START + 150, /* smac */
    HGIC_CMD_SEND_BLENC_STARTADV    = HGIC_CMD_START + 151, /* smac */
    HGIC_CMD_SET_RTS_DURATION       = HGIC_CMD_START + 152, /* smac */
    HGIC_CMD_STANDBY_CFG            = HGIC_CMD_START + 153, /* fmac */
    HGIC_CMD_SET_CONNECT_PAIRONLY   = HGIC_CMD_START + 154, /* fmac */
    HGIC_CMD_SET_DIFFCUST_CONN      = HGIC_CMD_START + 155, /* fmac */
    HGIC_CMD_GET_CENTER_FREQ        = HGIC_CMD_START + 156, /* smac */
	HGIC_CMD_SET_WAIT_PSMODE        = HGIC_CMD_START + 157, /* fmac */
	HGIC_CMD_SET_AP_CHAN_SWITCH     = HGIC_CMD_START + 158, /* fmac */
	HGIC_CMD_SET_CCA_FOR_CE         = HGIC_CMD_START + 159, /* fmac */
	HGIC_CMD_SET_DISABLE_PRINT      = HGIC_CMD_START + 160, /* fmac/smac */	
	HGIC_CMD_SET_APEP_PADDING       = HGIC_CMD_START + 161, /* smac */
	HGIC_CMD_GET_ACS_RESULT         = HGIC_CMD_START + 162, /* fmac/smac */		
	HGIC_CMD_GET_WIFI_STATUS_CODE   = HGIC_CMD_START + 163, /* fmac */
	HGIC_CMD_GET_WIFI_REASON_CODE   = HGIC_CMD_START + 164, /* fmac */
	HGIC_CMD_SET_WATCHDOG           = HGIC_CMD_START + 165, /* fmac/smac */	
	HGIC_CMD_SET_RETRY_FALLBACK_CNT = HGIC_CMD_START + 166, /* smac */
	HGIC_CMD_SET_FALLBACK_MCS       = HGIC_CMD_START + 167, /* smac */
	HGIC_CMD_GET_XOSC_VALUE         = HGIC_CMD_START + 168, /* smac */
	HGIC_CMD_SET_XOSC_VALUE         = HGIC_CMD_START + 169, /* smac */
	HGIC_CMD_GET_FREQ_OFFSET        = HGIC_CMD_START + 170, /* smac */
	HGIC_CMD_SET_CALI_PERIOD        = HGIC_CMD_START + 171, /* smac */
    HGIC_CMD_SET_BLENC_ADVFILTER    = HGIC_CMD_START + 172, /* smac */
    HGIC_CMD_SET_MAX_TX_DELAY       = HGIC_CMD_START + 173, /* smac */
    HGIC_CMD_GET_STA_INFO           = HGIC_CMD_START + 174, /* fmac */
    HGIC_CMD_SEND_MGMTFRAME         = HGIC_CMD_START + 175, /* fmac */    
    HGIC_CMD_SET_POOL_MAXUSAGE      = HGIC_CMD_START + 176, /* fmac/smac */
    HGIC_CMD_SET_ICMP_MNTR          = HGIC_CMD_START + 177, /* fmac/smac */
    HGIC_CMD_SET_WIFI_PASSWD        = HGIC_CMD_START + 178, /* fmac */
    HGIC_CMD_SET_WIFI_RPASSWD       = HGIC_CMD_START + 179, /* fmac */
    HGIC_CMD_GET_CHIP_UUID          = HGIC_CMD_START + 180, /* fmac/smac */
    HGIC_CMD_SET_COEXIST_EN         = HGIC_CMD_START + 181, /* smac */
    HGIC_CMD_SET_THROUGHPUT_EN      = HGIC_CMD_START + 182, /* smac */
    HGIC_CMD_SET_PS_HBDATA_MASK     = HGIC_CMD_START + 183, /* fmac */
    HGIC_CMD_SET_MEM_RECYCLE        = HGIC_CMD_START + 184, /* smac/fmac */
    HGIC_CMD_SET_HOST_MONITOR       = HGIC_CMD_START + 185, /* smac/fmac */
    HGIC_CMD_SET_DMA_SCATTER        = HGIC_CMD_START + 186, /* smac/fmac */
    HGIC_CMD_SET_BLE_LLPKT_LEN      = HGIC_CMD_START + 187, /* smac/fmac */
    HGIC_CMD_SET_SLEEP_ROAMING      = HGIC_CMD_START + 188, /* fmac */
    HGIC_CMD_SET_EDCA_LIMIT_MAX     = HGIC_CMD_START + 189, /* smac/fmac */
    HGIC_CMD_SET_EDCA_SLOT_TIME     = HGIC_CMD_START + 190, /* smac/fmac */
    HGIC_CMD_SET_EDCA_LIMIT_MIN     = HGIC_CMD_START + 191, /* smac/fmac */
    HGIC_CMD_SET_SIGNAL_THRESHOLD   = HGIC_CMD_START + 192, /* fmac */

    HGIC_CMD_SEND_BLENC_HCI_DATA    = HGIC_CMD_START + 0xFFFF + 1,/*drv priv cmd,not real cmd*/

};

enum hgic_event {
    HGIC_EVENT_STATE_CHG           = 1,
    HGIC_EVENT_CH_SWICH            = 2,
    HGIC_EVENT_DISCONNECT_REASON   = 3,
    HGIC_EVENT_ASSOC_STATUS        = 4,
    HGIC_EVENT_SCANNING            = 5,
    HGIC_EVENT_SCAN_DONE           = 6,
    HGIC_EVENT_TX_BITRATE          = 7,
    HGIC_EVENT_PAIR_START          = 8,
    HGIC_EVENT_PAIR_SUCCESS        = 9,
    HGIC_EVENT_PAIR_DONE           = 10,
    HGIC_EVENT_CONECT_START        = 11,
    HGIC_EVENT_CONECTED            = 12,
    HGIC_EVENT_DISCONECTED         = 13,
    HGIC_EVENT_SIGNAL              = 14,
    HGIC_EVENT_DISCONNET_LOG       = 15,
    HGIC_EVENT_REQUEST_PARAM       = 16,
    HGIC_EVENT_TESTMODE_STATE      = 17,
    HGIC_EVENT_FWDBG_INFO          = 18,
    HGIC_EVENT_CUSTOMER_MGMT       = 19,
    HGIC_EVENT_SLEEP_FAIL          = 20,
    HGIC_EVENT_DHCPC_DONE          = 21,
    HGIC_EVENT_CONNECT_FAIL        = 22,
    HGIC_EVENT_CUST_DRIVER_DATA    = 23,
    HGIC_EVENT_UNPAIR_STA          = 24,
    HGIC_EVENT_BLENC_DATA          = 25,
    HGIC_EVENT_HWSCAN_RESULT       = 26,
    HGIC_EVENT_EXCEPTION_INFO      = 27,
	HGIC_EVENT_DSLEEP_WAKEUP       = 28,
    HGIC_EVENT_STA_MIC_ERROR       = 29,
    HGIC_EVENT_ACS_DONE            = 30,
    HGIC_EVENT_FW_INIT_DONE        = 31,
    HGIC_EVENT_ROAM_CONECTED       = 32,
    HGIC_EVENT_MGMT_FRAME          = 33,
    HGIC_EVENT_UNKNOWN_STA         = 34,
    HGIC_EVENT_ROAM_FAIL           = 35,
    HGIC_EVENT_P2P_DEV_FIND_STOP   = 36,
    HGIC_EVENT_P2P_DEV_FIND        = 37,
    HGIC_EVENT_P2P_DEV_LOST        = 38,
    HGIC_EVENT_P2P_GROUP_REMOVE    = 39,
    HGIC_EVENT_P2P_GO_NEG_REQ      = 40,
    HGIC_EVENT_P2P_INV_RESULT      = 41, 
    HGIC_EVENT_P2P_GROUP_FAIL      = 42,
    HGIC_EVENT_P2P_INV_RECV        = 43,
    
    /*add customer driver event, recommend start from 65000, max 65535.*/

    //////////////////////////////////////////////////////////////////
    /*virtual driver event, exceed 65535*/
    HGIC_EVENT_HGIC_DATA           = 0xFFFF,        
};

enum hgicf_hw_state {
    HGICF_HW_DISCONNECTED     = 0,
    HGICF_HW_DISABLED         = 1,
    HGICF_HW_INACTIVE         = 2,
    HGICF_HW_SCANNING         = 3,
    HGICF_HW_AUTHENTICATING   = 4,
    HGICF_HW_ASSOCIATING      = 5,
    HGICF_HW_ASSOCIATED       = 6,
    HGICF_HW_4WAY_HANDSHAKE   = 7,
    HGICF_HW_GROUP_HANDSHAKE  = 8,
    HGICF_HW_CONNECTED        = 9,
};

enum HGIC_EXCEPTION_NUM{
    HGIC_EXCEPTION_CPU_USED_OVERTOP         = 1,
    HGIC_EXCEPTION_HEAP_USED_OVERTOP        = 2,
    HGIC_EXCEPTION_WIFI_BUFFER_USED_OVERTOP = 3,
    HGIC_EXCEPTION_TX_BLOCKED               = 4,
    HGIC_EXCEPTION_TXDELAY_TOOLONG          = 5,
    HGIC_EXCEPTION_STRONG_BGRSSI            = 6,
    HGIC_EXCEPTION_TEMPERATURE_OVERTOP      = 7,
    HGIC_EXCEPTION_WRONG_PASSWORD           = 8,
};

struct hgic_exception_info {
    int num;
    union {
        struct  {
            int max, avg, min;
        } txdelay;
        struct  {
            int max, avg, min;
        } bgrssi;
        struct  {
            int total, used;
        } buffer_usage;
        struct  {
            int total, used;
        } heap_usage;
        struct  {
            int temp;
        } temperature;
    } info;
};

struct hgic_dhcp_result {
    unsigned int ipaddr, netmask, svrip, router, dns1, dns2;
};

#ifdef __RTOS__
struct firmware {
    unsigned char *data;
    unsigned int size;
};
int request_firmware(struct firmware **fw, const char *name, void *dev);
void release_firmware(struct firmware *fw);

extern int hgicf_init(void);
extern int hgicf_cmd(char *ifname, unsigned int cmd, long param1, long param2);
extern int hgics_cmd(char *ifname, unsigned int cmd, long param1, long param2);
extern int  hgics_init(void);
extern void hgics_exit(void);
extern int wpas_init(void);
extern int wpas_start(char *ifname);
extern int wpas_stop(char *ifname);
extern int wpas_cli(char *ifname, char *cmd, char *reply_buff, int reply_len);
extern int wpas_passphrase(char *ssid, char *passphrase, char psk[32]);
extern int hapd_init(void);
extern int hapd_start(char *ifname);
extern int hapd_stop(char *ifname);
extern int hapd_cli(char *ifname, char *cmd, char *reply_buff, int reply_len);
extern void hgic_param_iftest(int iftest);
extern const char *hgic_param_ifname(const char *name);
extern char *hgic_param_fwfile(const char *fw);
extern int hgic_param_ifcount(int count);
extern void hgic_param_initcb(hgic_init_cb cb);
extern void hgic_param_eventcb(hgic_event_cb cb);
extern int hgic_ota_start(char *ifname, char *fw_name);

void hgic_raw_init(void);
int hgic_raw_send(char *dest, char *data, int len);
int hgic_raw_rcev(char *buf, int size, char *src);

#ifdef __HGIC_SMAC__
#include "umac_config.h"
#endif
#endif

#ifdef __cplusplus
}
#endif
#endif
