/**
 ****************************************************************************************
 *
 * @file uwifi_process_msg.c
 *
 * @brief uwifi process AT/APP cmd
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#include <string.h>
#include <time.h>
#include "ethernetif_wifi.h"
#include "uwifi_include.h"
#include "wpa_adapter.h"
#include "uwifi_msg.h"
#include "hostapd.h"
#include "uwifi_common.h"
#include "uwifi_ops_adapter.h"
#include "uwifi_msg_tx.h"
#include "uwifi_msg_rx.h"
#include "uwifi_platform.h"
#include "uwifi_wpa_api.h"
#include "uwifi_notify.h"
#include "uwifi_rx.h"
#include "uwifi_sdio.h"
#include "uwifi_txq.h"
#include "asr_sdio.h"
#include "asr_gpio.h"
//#include "asr_efuse.h"

#include "uwifi_interface.h"
#if UWIFI_TEST
#include "uwifi_basic_test.h"
#endif

#include "wpas_psk.h"
#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
#include "uwifi_idle_mode.h"
#endif
#ifdef LWIP
#include "netif.h"
#include "dhcp.h"
//#include "lwip_comm_wifi.h"
#include "inet.h"
#endif
#include "asr_rtos_api.h"
#include "uwifi_asr_api.h"

#ifdef ALIOS_SUPPORT
//#include "dhcps.h"
#ifdef AOS_COMP_KVSET
#define CONFIG_KV_BUFFER_SIZE KV_CONFIG_TOTAL_SIZE
#else
#define CONFIG_KV_BUFFER_SIZE 0
#endif
#else
#ifdef LWIP_DHCPD
#include "dhcpd.h"
#endif
#define CONFIG_KV_BUFFER_SIZE 0
#endif
#include <string.h>
#define UWIFI_MAX_SCAN_INTERVAL_IN_SEC 180  //max interval in second when scan tries over the max
#define UWIFI_SCAN_TIMER_INC_MAX_TRIES 6    //max tries for scan timer incresing
#define UWIFI_AUTO_DIRECTCONNECT_MAX_TRIES   1
#define UWIFI_AOS_SCAN_MAX_TRIES 2  //6
#define IEEE80211_RATE_SHORT_PREAMBLE BIT(0)

#define BCN_FRAME_LEN  256
#define PROBE_RSP_FRAME_LEN  256
#define AP_DEFAULT_CHAN_FREQ   2437  //channel 6

#define ASR_HT_CAPABILITIES                                    \
{                                                               \
    .ht_supported   = true,                                     \
    .cap            = 0,                                        \
    .ampdu_factor   = IEEE80211_HT_MAX_AMPDU_8K,                \
    .ampdu_density  = IEEE80211_HT_MPDU_DENSITY_16,             \
    .mcs        = {                                             \
        .rx_mask = { 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, },        \
        .rx_highest = cpu_to_le16(65),                          \
        .tx_params = IEEE80211_HT_MCS_TX_DEFINED,               \
    },                                                          \
}

#define RATE(_bitrate, _hw_rate, _flags) {      \
    .bitrate    = (_bitrate),                   \
    .flags      = (_flags),                     \
    .hw_value   = (_hw_rate),                   \
}

#define CHAN(_freq) {                           \
    .center_freq    = (_freq),                  \
    .max_power  = 30, /* FIXME */               \
}

static const struct ieee80211_rate asr_ratetable[] = {
    RATE(10,  0x00, 0),
    RATE(20,  0x01, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(55,  0x02, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(110, 0x03, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(60,  0x04, 0),
    RATE(90,  0x05, 0),
    RATE(120, 0x06, 0),
    RATE(180, 0x07, 0),
    RATE(240, 0x08, 0),
    RATE(360, 0x09, 0),
    RATE(480, 0x0A, 0),
    RATE(540, 0x0B, 0),
};

/* The channels indexes here are not used anymore */
static const struct ieee80211_channel asr_2ghz_channels[] = {
    CHAN(2412),
    CHAN(2417),
    CHAN(2422),
    CHAN(2427),
    CHAN(2432),
    CHAN(2437),
    CHAN(2442),
    CHAN(2447),
    CHAN(2452),
    CHAN(2457),
    CHAN(2462),
    CHAN(2467),
    CHAN(2472),
    CHAN(2484),
};

#define ASR_HE_CAPABILITIES                                    \
{                                                               \
    .has_he = false,                                            \
    .he_cap_elem = {                                            \
        .mac_cap_info[0] = 0,                                   \
        .mac_cap_info[1] = 0,                                   \
        .mac_cap_info[2] = 0,                                   \
        .mac_cap_info[3] = 0,                                   \
        .mac_cap_info[4] = 0,                                   \
        .mac_cap_info[5] = 0,                                   \
        .phy_cap_info[0] = 0,                                   \
        .phy_cap_info[1] = 0,                                   \
        .phy_cap_info[2] = 0,                                   \
        .phy_cap_info[3] = 0,                                   \
        .phy_cap_info[4] = 0,                                   \
        .phy_cap_info[5] = 0,                                   \
        .phy_cap_info[6] = 0,                                   \
        .phy_cap_info[7] = 0,                                   \
        .phy_cap_info[8] = 0,                                   \
        .phy_cap_info[9] = 0,                                   \
        .phy_cap_info[10] = 0,                                  \
    },                                                          \
    .he_mcs_nss_supp = {                                        \
        .rx_mcs_80 = cpu_to_le16(0xfffa),                       \
        .tx_mcs_80 = cpu_to_le16(0xfffa),                       \
        .rx_mcs_160 = cpu_to_le16(0xffff),                      \
        .tx_mcs_160 = cpu_to_le16(0xffff),                      \
        .rx_mcs_80p80 = cpu_to_le16(0xffff),                    \
        .tx_mcs_80p80 = cpu_to_le16(0xffff),                    \
    },                                                          \
    .ppe_thres = {0x08, 0x1c, 0x07},                            \
}

#ifdef CONFIG_ASR595X
static struct ieee80211_sband_iftype_data asr_he_capa = {
    .types_mask = (BIT(NL80211_IFTYPE_STATION) | BIT(NL80211_IFTYPE_AP)),
    .he_cap = ASR_HE_CAPABILITIES,
};
#endif

static struct ieee80211_supported_band asr_band_2GHz = {
    .channels   = asr_2ghz_channels,
    .n_channels = ARRAY_SIZE(asr_2ghz_channels),
    .bitrates   = asr_ratetable,
    .n_bitrates = ARRAY_SIZE(asr_ratetable),
    .ht_cap     = ASR_HT_CAPABILITIES,
#ifdef CONFIG_ASR595X
    .iftype_data = &asr_he_capa,
    .n_iftype_data = 1,
#endif
};

struct asr_mod_params asr_module_params = {
    /* common parameters */
    COMMON_PARAM(ht_on, true)
#ifdef CONFIG_ASR595X
    COMMON_PARAM(mcs_map, IEEE80211_VHT_MCS_SUPPORT_0_9)
    #ifdef BASS_SUPPORT
    COMMON_PARAM(ldpc_on, false)
    #else
    COMMON_PARAM(ldpc_on, true)
    #endif
#else
    COMMON_PARAM(mcs_map, IEEE80211_VHT_MCS_SUPPORT_0_7)
    COMMON_PARAM(ldpc_on, false)
#endif
    COMMON_PARAM(phy_cfg, 0)
    COMMON_PARAM(uapsd_timeout, 300)
    COMMON_PARAM(ap_uapsd_on, true)
    COMMON_PARAM(sgi, true)
    COMMON_PARAM(sgi80, true)
    COMMON_PARAM(use_2040, false)
    COMMON_PARAM(use_80, false)
    COMMON_PARAM(custregd, false)
    COMMON_PARAM(nss, 1)
    COMMON_PARAM(murx, false)
    COMMON_PARAM(mutx, false)
    COMMON_PARAM(mutx_on, false)
    COMMON_PARAM(roc_dur_max, 500)
    COMMON_PARAM(listen_itv, 1)
    COMMON_PARAM(listen_bcmc, true)
    COMMON_PARAM(lp_clk_ppm, 20)
    COMMON_PARAM(ps_on, false)
    COMMON_PARAM(tx_lft, ASR_TX_LIFETIME_MS)
    COMMON_PARAM(amsdu_maxnb, NX_TX_PAYLOAD_MAX)
    // By default, only enable UAPSD for Voice queue (see
    // IEEE80211_DEFAULT_UAPSD_QUEUE comment)
    COMMON_PARAM(uapsd_queues, IEEE80211_WMM_IE_STA_QOSINFO_AC_VO)
#ifdef CONFIG_ASR595X
    COMMON_PARAM(he_on, true)
    COMMON_PARAM(he_mcs_map, IEEE80211_HE_MCS_SUPPORT_0_9)
    COMMON_PARAM(he_ul_on, false)
    COMMON_PARAM(stbc_on, false)
    COMMON_PARAM(bfmee, false)
    COMMON_PARAM(twt_request, true)
#endif
};

struct asr_hw g_asr_hw;
struct wiphy g_wiphy;
asr_api_ctrl_param_t asr_api_ctrl_param = {0};
int scan_done_flag = SCAN_INIT;
extern uint8_t wifi_ready;

#if NX_DEBUG
struct asr_hw *sp_asr_hw = NULL;
#else
struct asr_hw *sp_asr_hw = NULL;
#endif

#ifndef THREADX_STM32
struct uwifi_ap_cfg_param g_ap_cfg_info =
{
    1,  //dtim_period
    100,   //beacon_interval,
    0x0401,
    1,  //wmm enabled
    1,  //ht enabled
    #ifdef CONFIG_ASR595X
    0,    //he enabled
    #else
    0,    //he enabled
    #endif
    WIFI_ENCRYP_OPEN,  //encryption enable
    {
        IEEE80211_BAND_2GHZ,
        AP_DEFAULT_CHAN_FREQ,  //center_freq
        //0,
        0,
        //0,
        25,   //tx_power,
        0,
        //0,
        //0,
        //0,
        //0
    },
    NL80211_CHAN_WIDTH_20_NOHT,
    AP_DEFAULT_CHAN_FREQ,
    0,
    1,  //crypto_control_port;
    0,  //crypto_control_port_ethertype;  use 802.1x
    0,  //rypto_control_port_no_encrypt;
    WLAN_CIPHER_SUITE_CCMP,  //crypto_cipher_group;
    WLAN_CIPHER_SUITE_CCMP,  //crypto_cipher_pairwise;
    //should be init and put into asr_hw
    {0},
    0,
    {0},
    0,
    {
        IEEE80211_CCK_RATE_1MB  | IEEE80211_BASIC_RATE_MASK,
        IEEE80211_CCK_RATE_2MB  | IEEE80211_BASIC_RATE_MASK,
        IEEE80211_CCK_RATE_5MB  | IEEE80211_BASIC_RATE_MASK,
        IEEE80211_CCK_RATE_11MB | IEEE80211_BASIC_RATE_MASK,
        IEEE80211_OFDM_RATE_6MB,
        IEEE80211_OFDM_RATE_9MB,
        IEEE80211_OFDM_RATE_12MB,
        IEEE80211_OFDM_RATE_18MB,
        IEEE80211_OFDM_RATE_24MB,
        IEEE80211_OFDM_RATE_36MB,
        IEEE80211_OFDM_RATE_48MB,
        IEEE80211_OFDM_RATE_54MB,
        0,0,0,0
    },
    {
        IEEE80211_BASIC_RATE_MASK | IEEE80211_CCK_RATE_1MB,
        IEEE80211_BASIC_RATE_MASK | IEEE80211_CCK_RATE_2MB,
        IEEE80211_BASIC_RATE_MASK | IEEE80211_CCK_RATE_5MB,
        IEEE80211_BASIC_RATE_MASK | IEEE80211_CCK_RATE_11MB,
        0,0,0,0,0,0,0,0
    },
    4,
    {
        IEEE80211_OFDM_RATE_6MB,
        IEEE80211_OFDM_RATE_9MB,
        IEEE80211_OFDM_RATE_12MB,
        IEEE80211_OFDM_RATE_18MB,
        IEEE80211_OFDM_RATE_24MB,
        IEEE80211_OFDM_RATE_36MB,
        IEEE80211_OFDM_RATE_48MB,
        IEEE80211_OFDM_RATE_54MB,
        0,0,0,0,0,0,0,0
    },
    8,
    //dsss parameter
    6,
    //edca param
    {
        {0, 7, 4, 10, 0},  //BK
        {0, 3, 4, 10, 0},  //BE
        {0, 2, 3, 4, 0},  //VI
        {0, 2, 2, 3, 0}   //VO
    },
    1,  //ap_isolate 1:enable softap isolation, don't resend. 0:close softap isolation,resend. thers is error in transponding multicast frame currently.
    0,  //enable softap isolation, thers is error in transponding multicast frame currently.
    0   //default disable hidden ssid.
};
#endif

struct asr_wifi_config g_wifi_config = {
    {0},//mac_addr
    {
        50,50,50,50,//11b 1M 2M 5.5M 11M
        50,50,50,40,40,20,20,20,//11g 6M 9M 12M 18M 24M 36M 48M 54M
        50,50,50,40,30,30,20,20,//11n mcs0 mcs1 mcs2 mcs3 mcs4 mcs5 mcs6 mcs7
        30,30,30,20,20,20,20,20,//11n HT40 mcs0 mcs1 mcs2 mcs3 mcs4 mcs5 mcs6 mcs7
    },//tx_pwr
    false,//pwr_config
    {0},//cca
    {0},//edca_bk
    {0},//edca_be
    {0},//edca_vi
    {0},//edca_vo
};


extern struct net_device_ops uwifi_netdev_ops;
extern uint32_t current_iftype;
#ifdef CFG_SNIFFER_SUPPORT
extern struct net_device_ops uwifi_sniffer_netdev_ops;
extern monitor_cb_t sniffer_rx_cb;
extern monitor_cb_t sniffer_rx_mgmt_cb;
#endif
extern struct ap_close_status g_ap_close_status;
//extern efuse_info_t g_efuse_info;
extern struct ethernetif g_ethernetif[COEX_MODE_MAX];

extern int asr_save_sta_config_info(struct config_info *pst_config_info, asr_wlan_init_type_t* pst_network_init_info);
extern void wpas_send_timeout_callback(struct sta_info *pstat);
extern void uwifi_hostapd_deauth_peer_sta(struct asr_hw *asr_hw, uint8_t *mac_addr);

extern const char *asr_get_wifi_version(void);

#define FLASH_MAC_ADDR_TOKEN      (0xACBDEFFE)
#define FLASH_MAC_ADDR_TOKEN_LEN  (4)
typedef struct
{
    uint8_t   mac[MAC_ADDR_LEN];
    uint8_t   resv[2];
    uint32_t  token;
}flash_mac_addr_t;

struct asr_hw* uwifi_get_asr_hw(void)
{
    return sp_asr_hw;
}

void asr_wlan_set_mac_address(uint8_t *mac_addr)
{
#if 0
    flash_mac_addr_t flash_mac_addr;
    uint32_t offset = CONFIG_KV_BUFFER_SIZE;

    memcpy(flash_mac_addr.mac, mac_addr, MAC_ADDR_LEN);
    flash_mac_addr.token = FLASH_MAC_ADDR_TOKEN;
#endif
    //asr_rtos_declare_critical();
    //asr_rtos_enter_critical();
    //asr_flash_erase_write(PARTITION_PARAMETER_2, &offset, (void *)(&flash_mac_addr), sizeof(flash_mac_addr));
    //asr_rtos_exit_critical();
}


int asr_get_random_int_c()
{
    int i;
    srand(asr_rtos_get_time());
    i = rand();
    return i;
}

extern void asr_wlan_default_mac_addr(uint8_t *mac_addr);
#ifdef THREADX
extern void getWiFiMACAddr(char *macaddr);
#endif
void uwifi_read_mac_from_flash(uint8_t* mac_addr)
{
    #if 0
    flash_mac_addr_t flash_mac_addr;

    //read mac addr from flash
    //sdio can't directly operate the flash to get the mac addr
    //memcpy(&flash_mac_addr, (uint32_t *)(KV_FLASH_START_ADDR+CONFIG_KV_BUFFER_SIZE), sizeof(flash_mac_addr));

    if(flash_mac_addr.token != FLASH_MAC_ADDR_TOKEN)
    {
        asr_wlan_default_mac_addr(mac_addr);
        asr_wlan_set_mac_address(mac_addr);
    }
    else
    {
        memcpy(mac_addr, flash_mac_addr.mac, MAC_ADDR_LEN);
    }
    #else
        #ifdef THREADX
            getWiFiMACAddr(mac_addr);
            #else
            #ifdef LOAD_MAC_ADDR_FROM_FW
            struct asr_hw *asr_hw = uwifi_get_asr_hw();
            struct mm_fw_macaddr_cfm cfm;
            asr_send_fw_macaddr_req(asr_hw,&cfm);
            memcpy(mac_addr, &(cfm.mac), MAC_ADDR_LEN);
            #else
            if((mac_addr[0]==0) && (mac_addr[1]==0) && (mac_addr[2]==0) &&
               (mac_addr[3]==0) && (mac_addr[4]==0) && (mac_addr[5]==0))
            {
                //pr_info("failed read mac address from flash, will use random address\n");
                mac_addr[0] = 0x8c;
                mac_addr[1] = 0x59;
                mac_addr[2] = 0xdc;
                mac_addr[3] = (uint8_t)((asr_get_random_int_c()>>16)&0xff);
                mac_addr[4] = (uint8_t)((asr_get_random_int_c()>>8)&0xff);
                mac_addr[5] = (uint8_t)((asr_get_random_int_c()>>0)&0xff);
            }
            #endif
        #endif
    #endif
}
#if 0
int uwifi_read_mac_from_efuse(uint8_t* mac_addr)
{
    uint8_t efuse_default_val[MAC_ADDR_LEN] = {0};

    //read mac address from efuse
    memcpy(mac_addr, g_efuse_info.mac_addr, MAC_ADDR_LEN);

    if(memcmp(mac_addr, efuse_default_val, MAC_ADDR_LEN) == 0)
    {
        return -1;
    }
    else
    {
        if(mac_addr[0] & 0x1)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "efuse mac group address");
        }

        return 0;
    }
}

//#define EFUSE_FREQ_CALIB_VAL_LEN  (1)
void uwifi_read_freq_calib_val_from_efuse(uint16_t *freq_calib_val)
{
    //read XO frequence calibration value from efuse
    *freq_calib_val = g_efuse_info.freq_err;
    //asr_efuse_multi_read(EFUSE_FREQ_CALIB_VAL, EFUSE_FREQ_CALIB_VAL_LEN, (uint8_t*)freq_calib_val);
}
#endif
void uwifi_read_country_code_from_flash(char* country)
{

}

void uwifi_read_configurated_ap_from_flash(struct config_info *configInfo)
{

}

void uwifi_get_device_mac(uint8_t *buf)
{
    int efuse_ret = 1;

    //efuse_ret = uwifi_read_mac_from_efuse(buf);

    if(efuse_ret)
    {
        uwifi_read_mac_from_flash(buf);
    }

    #ifdef AWORKS
    aw_board_get_enet_mac_address(0, buf);
    #endif

    dbg(D_ERR,D_UWIFI_CTRL,"efuse %d, device mac %02X:%02X:%02X:%02X:%02X:%02X",efuse_ret,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);


}

void uwifi_get_mac(uint8_t *buf)
{
    if(sp_asr_hw->sta_vif_idx == 0xFF && sp_asr_hw->ap_vif_idx == 0xFF)
    {
        uwifi_get_device_mac(buf);
    }
    else
    {
        uint8_t *macaddr;
        if(sp_asr_hw->sta_vif_idx != 0xFF)
        {
            macaddr = sp_asr_hw->vif_table[sp_asr_hw->sta_vif_idx]->wlan_mac_add.mac_addr;
        }
        else
        {
            macaddr = sp_asr_hw->vif_table[sp_asr_hw->ap_vif_idx]->wlan_mac_add.mac_addr;
        }
        buf[0] = macaddr[0];
        buf[1] = macaddr[1];
        buf[2] = macaddr[2];
        buf[3] = macaddr[3];
        buf[4] = macaddr[4];
        buf[5] = macaddr[5];
    }

    //show current mac address we used
    dbg(D_ERR,D_UWIFI_CTRL,"mac address is %02X:%02X:%02X:%02X:%02X:%02X (%d , %d)",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],
                                                                          sp_asr_hw->sta_vif_idx,sp_asr_hw->ap_vif_idx);
}

void uwifi_get_country_code(char* country)
{
    uwifi_read_country_code_from_flash(country);

    dbg(D_INF,D_UWIFI_CTRL,"flash country code is %c:%c",country[0],country[1]);
    if((country[0]==0) && (country[1]==0))
    {
        dbg(D_ERR,D_UWIFI_CTRL,"failed read country code from flash,will use default CN");

        //default CN
        country[0] = 'C';
        country[1] = 'N';
    }
}

void uwifi_get_saved_config(struct config_info *configInfo)
{
    uwifi_read_configurated_ap_from_flash(configInfo);
    if(configInfo->connect_ap_info.ssid_len == 0)
        dbg(D_ERR,D_UWIFI_CTRL,"%s: not find saved ap",__func__);
}

///#ifdef CFG_STATION_SUPPORT
struct config_info* uwifi_get_sta_config_info(void)
{
    struct asr_hw     *pst_asr_hw     = uwifi_get_asr_hw();
    struct asr_vif    *pst_asr_vif    = NULL;

    if (NULL == pst_asr_hw)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "uwifi_get_sta_config_info:no asr hw!");
        return NULL;
    }

    if (pst_asr_hw->sta_vif_idx < pst_asr_hw->vif_max_num)
        pst_asr_vif = pst_asr_hw->vif_table[pst_asr_hw->sta_vif_idx];

    if (NULL == pst_asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "uwifi_get_sta_config_info:no sta vif!");
        return NULL;
    }

    return (&(pst_asr_vif->sta.configInfo));
}

#ifdef CFG_STATION_SUPPORT
void uwifi_clear_autoconnect_status(struct config_info *pst_config_info)
{
    if (NULL == pst_config_info)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "uwifi_clear_autoconnect_status:config info NULL!");
        return;
    }

    pst_config_info->autoconnect_tries = 0;
    pst_config_info->autoconnect_count = 0;
    pst_config_info->wifi_retry_interval = AUTOCONNECT_INTERVAL_INIT;

    /* in case, there are some unstoped timer */
    if (NULL != pst_config_info->wifi_retry_timer.handle)
    {
        /* deinit the timer */
        asr_rtos_deinit_timer(&(pst_config_info->wifi_retry_timer));
    }
}
#endif

int uwifi_dev_transmit_skb(struct sk_buff* skb, struct asr_vif *asr_vif)
{
    return uwifi_netdev_ops.ndo_start_xmit(asr_vif, skb);
}

/*
    @uwifi_xmit_buf

    return value
        0 - OK
      < 0 - No enough resource for transfer
*/
extern bool arp_debug_log;
extern bool txlogen;
extern bool mrole_enable;
#ifdef CFG_STA_AP_COEX
int uwifi_xmit_buf(void *buf, uint32_t len, void *ethernetif)
#else
int uwifi_xmit_buf(void *buf, uint32_t len)
#endif
{
    struct sk_buff *skb = NULL;
    int res = 0;
    uint8_t *p_skbdata;
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    struct asr_vif *asr_vif = NULL;
    struct asr_vif *cur = NULL;
    uint8_t found = 0;

   /*
        lwip_iflist_value(ifname, "ifmode", &value) to get the mode attribute of the netif
   */
   if(NULL == asr_hw)
   {
       dbg(D_ERR,D_UWIFI_CTRL,"%s:sp_hw is NULL",__func__);
       return ASR_ERR_CLSD;
   }

   #ifdef CFG_STA_AP_COEX
   if (mrole_enable && (asr_hw->vif_started > 1)) {

        list_for_each_entry(cur, &asr_hw->vifs, list)
        {
            if(!memcmp(cur->wlan_mac_add.mac_addr, ((struct ethernetif*)ethernetif)->ethaddr->mac_addr, MAC_ADDR_LEN))
            {
                found++;
                asr_vif = cur;
                break;
            }
        }
        if(!found)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "not find matched ethernetif");
            return ASR_ERR_CLSD;
        }
    } else
    #endif
    {
        if (asr_hw->sta_vif_idx != 0xFF)  //being opened as sta mode
            asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];
        else if (asr_hw->ap_vif_idx != 0xFF)
            asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
        else {
            dbg(D_ERR, D_UWIFI_CTRL, "not find matched ethernetif");
            return ASR_ERR_CLSD;
        }
    }

    skb = dev_alloc_skb_tx(sizeof(struct ethhdr) + sizeof(struct iphdrs) + PKT_BUF_RESERVE_HEAD);
    if (skb == NULL) {
        dbg(D_ERR,D_UWIFI_DATA,"%s: malloc skb failed", __func__);
        return ASR_ERR_BUF;
    }
    skb->len = len + PKT_BUF_RESERVE_HEAD;
    skb_pull(skb, PKT_BUF_RESERVE_HEAD);

    p_skbdata = skb->data;
    memcpy(p_skbdata, buf, sizeof(struct ethhdr) + sizeof(struct iphdrs));

    skb->private[0] = 0xDEADBEAF;
    skb->private[1] = (uint32_t)((void *)buf);
    skb->private[2] = skb->len;

    if (arp_debug_log || txlogen)
        dbg(D_ERR,D_UWIFI_DATA,"%s will resend(%u) arp request", __func__,asr_vif->is_resending);

    res = uwifi_netdev_ops.ndo_start_xmit(asr_vif,(struct sk_buff*)skb);
    //if(res)
    //    dbg(D_ERR,D_UWIFI_DATA,"tx failed, res=%d buf:0x%x len:0x%x",res, buf, len);

    return res;
}

int uwifi_xmit_asr_pbuf(struct asr_pbuf *pbuf, void *ethernetif)
{
    struct sk_buff *skb = NULL;
    int res = 0;
    uint8_t *p_skbdata = NULL;
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    struct asr_vif *asr_vif = NULL;
    struct asr_vif *cur = NULL;
    uint8_t found = 0;
    uint16_t pbuf_len = 0;
    uint8_t *data_temp;
    struct asr_pbuf *q;
    uint16_t packet_len = 0;
    uint8_t times = 0;

   if(NULL == asr_hw)
   {
       dbg(D_ERR,D_UWIFI_CTRL,"%s:sp_hw is NULL",__func__);
       return ASR_ERR_CLSD;
   }

   if (mrole_enable && (asr_hw->vif_started > 1)) {
        list_for_each_entry(cur, &asr_hw->vifs, list)
        {
            if(!memcmp(cur->wlan_mac_add.mac_addr, ((struct ethernetif*)ethernetif)->ethaddr->mac_addr, MAC_ADDR_LEN))
            {
                found++;
                asr_vif = cur;
                break;
            }
        }
        if(!found)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "not find matched ethernetif");
            return ASR_ERR_CLSD;
        }
    } else {
        if (asr_hw->sta_vif_idx != 0xFF)//being opened as sta mode
            asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];
        else if (asr_hw->ap_vif_idx != 0xFF)
            asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
        else {
            dbg(D_ERR, D_UWIFI_CTRL, "not find matched ethernetif");
            return ASR_ERR_CLSD;
        }
    }

    // temp add.
    #if 0
    while (skb_queue_empty(&asr_hw->tx_sk_free_list) && ++times < 50) {
        // just delay to wait tx task free tx agg buf.
        asr_rtos_delay_milliseconds(10);
    }
    #endif

    // convert asr pbuf pkt to skb pkt.
    if (!skb_queue_empty(&asr_hw->tx_sk_free_list)) {

        skb = skb_dequeue(&asr_hw->tx_sk_free_list);

        skb_reserve(skb, PKT_BUF_RESERVE_HEAD);

        data_temp = skb->data;
        for(q = pbuf; q != NULL; q = q->next)
        {
            memcpy(data_temp, q->payload, q->len);
            data_temp += q->len;
            packet_len += q->len;
        }

        //skb->len = packet_len;
        //fill in pkt.
        skb_put(skb,packet_len);

        if (skb->len > TX_AGG_BUF_UNIT_SIZE)
            dbg(D_ERR,D_UWIFI_DATA,"%s: skb oversize %d", __func__,skb->len);

    }else {

        dbg(D_ERR,D_UWIFI_DATA,"%s: tx_sk_free_list run out", __func__);

        return ASR_ERR_BUF;
    }
    /****************************************************************************************************************/

    skb->private[0] = 0xDEADBEAF;
    skb->private[1] = (uint32_t)((void *)skb->data);
    skb->private[2] = skb->len;

    if (arp_debug_log || txlogen)
        dbg(D_ERR,D_UWIFI_DATA,"%s will resend(%u) arp request", __func__,asr_vif->is_resending);

    res = uwifi_netdev_ops.ndo_start_xmit(asr_vif,(struct sk_buff*)skb);
    //if(res)
    //    dbg(D_ERR,D_UWIFI_DATA,"tx failed, res=%d buf:0x%x len:0x%x",res, buf, len);

    return res;
}


#ifdef CFG_STA_AP_COEX
int uwifi_xmit_pbuf(struct asr_pbuf *pbuf, void *ethernetif)
#else
int uwifi_xmit_pbuf(struct asr_pbuf *pbuf)
#endif
{
    struct sk_buff *skb = NULL;
    int res = 0;
    uint8_t *p_skbdata = NULL;
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    struct asr_vif *asr_vif = NULL;
    static struct sk_buff *xmit_skb = NULL;
    struct asr_vif *cur = NULL;
    uint8_t found = 0;

   /*
        lwip_iflist_value(ifname, "ifmode", &value) to get the mode attribute of the netif
    */

   if(NULL == asr_hw)
   {
       dbg(D_ERR,D_UWIFI_CTRL,"%s:sp_hw is NULL",__func__);
       return ASR_ERR_CLSD;
   }

#ifdef CFG_STA_AP_COEX
    if (mrole_enable && (asr_hw->vif_started > 1)) {

            list_for_each_entry(cur, &asr_hw->vifs, list)
            {
                if(!memcmp(cur->wlan_mac_add.mac_addr, ((struct ethernetif*)ethernetif)->ethaddr->mac_addr, MAC_ADDR_LEN))
                {
                    found++;
                    asr_vif = cur;
                    break;
                }
            }
            if(!found)
            {
                dbg(D_ERR, D_UWIFI_CTRL, "not find matched ethernetif");
                return ASR_ERR_CLSD;
            }
    } else
 #endif
       {
        if (asr_hw->sta_vif_idx != 0xFF)//being opened as sta mode
            asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];
        else if (asr_hw->ap_vif_idx != 0xFF)
            asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
        else {
            dbg(D_ERR, D_UWIFI_CTRL, "not find matched ethernetif");
            return ASR_ERR_CLSD;
        }
       }

    if (!xmit_skb) {
        xmit_skb = skb = dev_alloc_skb_tx(sizeof(struct ethhdr) + sizeof(struct iphdrs) + PKT_BUF_RESERVE_HEAD);
        if (skb == NULL) {
            dbg(D_ERR,D_UWIFI_DATA,"%s: malloc skb failed", __func__);
            return ASR_ERR_BUF;
        }
    } else {
        skb = xmit_skb;
    }

    skb->len = PKT_BUF_RESERVE_HEAD;
    skb_pull(skb, PKT_BUF_RESERVE_HEAD);

    skb->len = pbuf->tot_len;
    skb->data = pbuf->payload;//warning: not use skb pull,push ...

    skb->private[0] = 0xAABBCCDD;
    skb->private[1] = (uint32_t)((void *)pbuf);
    skb->private[2] = skb->len;
    if (arp_debug_log || txlogen)
        dbg(D_ERR,D_UWIFI_DATA,"%s will resend(%u) arp request", __func__,asr_vif->is_resending);
    res = uwifi_netdev_ops.ndo_start_xmit(asr_vif,(struct sk_buff*)skb);
    //if(res)
    //    dbg(D_ERR,D_UWIFI_DATA,"tx failed, res=%d buf:0x%x len:0x%x",res, buf, len);

    return res;
}

//update these parameters later
rx_packet_timer_t rx_packets_speed_table[RX_TIMER_DYNAMIC_LEVELS] = {
    {100, 20},
    {80, 50},
    {50, 100},
    {30, 100},
    {15, 100},
    {10, 100},
    {5, 100},
    {0, 100},
};

rx_bytes_timer_t rx_bytes_speed_table[RX_TIMER_DYNAMIC_LEVELS] = {
    {1000, 20},
    {800, 50},
    {500, 100},
    {300, 100},
    {150, 100},
    {100, 100},
    {50, 100},
    {0, 100},
};
//may we need update the rx_packets_speed_table and rx_bytes_speed_table by statistic in RX_TIMER_TRAIN_TIMES times
int rx_packets[RX_TIMER_DYNAMIC_LEVELS] = {0};
int rx_bytes[RX_TIMER_DYNAMIC_LEVELS] = {0};
int rx_times[RX_TIMER_DYNAMIC_LEVELS] = {0};
void amsdu_malloc_free_handler(void* arg)
{
//extern int g_amsdu_free_cnt;
//extern int g_amsdu_malloc_cnt;
    //printf("malloc:%d free:%d\r\n",g_amsdu_malloc_cnt,g_amsdu_free_cnt);
}

#define GET_BIT(x,pos)      ((x)>>(pos)&1)
extern uint8_t wifi_ready;
uint8_t old_wifiState = 0;
uint32_t rx_polling_level[RX_TIMER_DYNAMIC_LEVELS];
extern bool asr_xmit_opt;

#ifdef CONFIG_ASR_KEY_DBG
extern u32 tx_agg_port_num[8];
extern u32 rx_agg_port_num[8];
extern u32 pending_data_cnt;
extern u32 direct_fwd_cnt;
extern u32 rxdesc_refwd_cnt;
extern u32 rxdesc_del_cnt;
extern u32 min_deaggr_free_cnt;
extern int asr_rxdataind_run_cnt;
extern int asr_rxdataind_fn_cnt;
extern int tx_status_debug;
extern int asr_wait_rx_sk_buf_cnt;
extern int asr_wait_rx_sk_msgbuf_cnt;
extern int asr_total_int_rx_cnt;


#ifdef SDIO_DEAGGR

extern int asr_total_rx_sk_deaggrbuf_cnt;
extern int asr_wait_rx_sk_deaggrbuf_cnt;
extern int to_os_less8_cnt;
extern int to_os_less16_cnt;
extern int to_os_more16_cnt;
extern int max_to_os_delay_cnt;
#endif


extern int isr_write_clear_cnt;
extern int ipc_read_rx_bm_fail_cnt;
extern int ipc_read_tx_bm_fail_case0_cnt;
extern int ipc_read_tx_bm_fail_case2_cnt;
extern int ipc_read_tx_bm_fail_case3_cnt;
extern int ipc_isr_skip_main_task_cnt;

extern  int sdio_tx_evt_cnt;
extern  int sdio_rx_evt_cnt;
extern uint32_t g_tra_vif_drv_txcnt;
extern uint32_t g_cfg_vif_drv_txcnt;
extern uint32_t g_skb_expand_cnt;

void tx_status_timer_handler(void* arg)
{
    struct asr_hw *asr_hw = (struct asr_hw*)arg;
    struct asr_tx_agg *tx_agg_env = NULL ;
    u32 tx_agg_num_total;
    int i;
    int tx_skb_free_cnt,rx_skb_free_cnt;
    int rx_msgskb_free_cnt;
    int rx_skb_to_os_cnt;
    #ifdef SDIO_DEAGGR
    int rx_skb_total_cnt;
    int rx_deaggr_skb_free_cnt = 0;
    #endif
    u16 rd_bitmap,last_rd_bitmap,wr_bitmap;
    struct asr_vif *asr_traffic_vif = NULL;
    struct asr_vif *asr_ext_vif = NULL;
    u32 tra_txring_bytes = 0;
    u32 cfg_txring_bytes = 0;
    u32 tra_skb_cnt = 0;
    u32 cfg_skb_cnt = 0;

    if (asr_hw) {
        // tx debug
        tx_agg_num_total = (tx_agg_port_num[0] + tx_agg_port_num[1] + tx_agg_port_num[2] + tx_agg_port_num[3] +
                            tx_agg_port_num[4] + tx_agg_port_num[5] + tx_agg_port_num[6] + tx_agg_port_num[7]);

        tx_agg_env = &(asr_hw->tx_agg_env);
        if (tx_agg_env) {
            dbg(D_ERR,D_UWIFI_CTRL,"[%s] tx (cnt:%d,use_bm:0x%x,cur_idx=%d) agg_port_st(%d %d %d %d %d %d %d %d, %d - %d) ,evt_cnt(%d-%d)\r\n",
                __func__,
                tx_agg_env->aggr_buf_cnt,
                asr_hw->tx_use_bitmap,
                asr_hw->tx_data_cur_idx,
                tx_agg_port_num[0],tx_agg_port_num[1], tx_agg_port_num[2], tx_agg_port_num[3],
                tx_agg_port_num[4], tx_agg_port_num[5], tx_agg_port_num[6],tx_agg_port_num[7],
                tx_agg_num_total,(tx_agg_num_total / (tx_agg_port_num[0] + 1)),
                sdio_tx_evt_cnt,sdio_rx_evt_cnt);
        }

        // mrole tx debug
        if (asr_hw->sta_vif_idx < asr_hw->vif_max_num) {
            asr_traffic_vif  = asr_hw->vif_table[asr_hw->sta_vif_idx];
           if (asr_traffic_vif) {
               tra_txring_bytes = asr_traffic_vif->txring_bytes;
               tra_skb_cnt        = asr_traffic_vif->tx_skb_cnt;
           }
        }

        if (asr_hw->ap_vif_idx < asr_hw->vif_max_num) {
            asr_ext_vif  = asr_hw->vif_table[asr_hw->ap_vif_idx];
           if (asr_ext_vif) {
               cfg_txring_bytes  = asr_ext_vif->txring_bytes;
               cfg_skb_cnt         = asr_ext_vif->tx_skb_cnt;
           }
        }

        tx_skb_free_cnt = skb_queue_len(&asr_hw->tx_sk_free_list);

        dbg(D_ERR,D_UWIFI_CTRL,"[%s] mrole(%d %d %d) tx :skb_cnt(%d) ,cfg(%p,%d: bytes:%d[%d-%d], cnts:%d[%d-%d], %d) tra(%p,%d: bytes:%d[%d-%d] , cnts:%d[%d-%d] ,%d) \r\n" ,__func__,
            mrole_enable,asr_xmit_opt,g_skb_expand_cnt,
            tx_skb_free_cnt,
            asr_ext_vif,    asr_hw->ap_vif_idx, cfg_txring_bytes,CFG_VIF_LWM,CFG_VIF_HWM, cfg_skb_cnt,CFG_VIF_CNT_LWM,CFG_VIF_CNT_HWM,g_cfg_vif_drv_txcnt,
            asr_traffic_vif,asr_hw->sta_vif_idx,tra_txring_bytes,TRA_VIF_LWM,TRA_VIF_HWM, tra_skb_cnt,TRA_VIF_CNT_LWM,TRA_VIF_CNT_HWM,g_tra_vif_drv_txcnt);

        g_cfg_vif_drv_txcnt = g_tra_vif_drv_txcnt = g_skb_expand_cnt = 0;

        // rx debug
        rx_skb_free_cnt    = skb_queue_len(&asr_hw->rx_data_sk_list);
        rx_msgskb_free_cnt = skb_queue_len(&asr_hw->rx_msg_sk_list);
        rx_skb_to_os_cnt   = skb_queue_len(&asr_hw->rx_to_os_skb_list);

        dbg(D_ERR,D_UWIFI_CTRL,
            "[%s] rx:  sdio_aggr (%d %d %d %d %d %d %d %d) free skb cnt(%d %d) (%d) (%d %d - %d)\r\n",
            __func__,
            rx_agg_port_num[0], rx_agg_port_num[1], rx_agg_port_num[2],rx_agg_port_num[3],
            rx_agg_port_num[4], rx_agg_port_num[5], rx_agg_port_num[6], rx_agg_port_num[7],
            rx_skb_free_cnt, rx_msgskb_free_cnt, rx_skb_to_os_cnt,
            asr_wait_rx_sk_buf_cnt,asr_wait_rx_sk_msgbuf_cnt,asr_total_int_rx_cnt);

        #ifdef SDIO_DEAGGR
        //  rx deaggr debug
        rx_deaggr_skb_free_cnt = skb_queue_len(&asr_hw->rx_sk_sdio_deaggr_list);

        rx_skb_total_cnt = pending_data_cnt + direct_fwd_cnt;
        dbg(D_ERR,D_UWIFI_CTRL,
            "[%s]  rx deaggr debug (%d %d %d %d) -> (%d == %d) fn_skb(%d) min_free_deaggr(%d) cur_free_deaggr(%d), wait deaggr(%d - %d),(%d %d %d)(%d)\r\n",
            __func__,
            pending_data_cnt,direct_fwd_cnt,rxdesc_refwd_cnt,rxdesc_del_cnt,
            asr_rxdataind_run_cnt,rx_skb_total_cnt,
            asr_rxdataind_fn_cnt,
            min_deaggr_free_cnt,
            rx_deaggr_skb_free_cnt,
            asr_wait_rx_sk_deaggrbuf_cnt,asr_total_rx_sk_deaggrbuf_cnt,
            to_os_less8_cnt,to_os_less16_cnt,to_os_more16_cnt,
            max_to_os_delay_cnt);
        #endif

        rd_bitmap = last_rd_bitmap = wr_bitmap = 0;

        rd_bitmap = asr_hw->sdio_reg[RD_BITMAP_L];
        rd_bitmap |= asr_hw->sdio_reg[RD_BITMAP_U] << 8;

        last_rd_bitmap = asr_hw->last_sdio_regs[RD_BITMAP_L];
        last_rd_bitmap |= asr_hw->last_sdio_regs[RD_BITMAP_U] << 8;

        wr_bitmap = asr_hw->sdio_reg[WR_BITMAP_L];
        wr_bitmap |= asr_hw->sdio_reg[WR_BITMAP_U] << 8;

        dbg(D_ERR,D_UWIFI_CTRL,
            "[%s] sdio wr clr debug (%d :%d,%d %d %d %d) sdio_reg(0x%x, 0x%x) (0x%x,0x%x) poll_lv(%d,%d,%d,%d,%d,%d,%d,%d)\r\n",
            __func__,
            isr_write_clear_cnt,
            ipc_read_rx_bm_fail_cnt,
            ipc_read_tx_bm_fail_case0_cnt,ipc_read_tx_bm_fail_case2_cnt,ipc_read_tx_bm_fail_case3_cnt,ipc_isr_skip_main_task_cnt,
            last_rd_bitmap, rd_bitmap,wr_bitmap,asr_hw->tx_use_bitmap,
            rx_polling_level[0],rx_polling_level[1],rx_polling_level[2],rx_polling_level[3],
            rx_polling_level[4],rx_polling_level[5],rx_polling_level[6],rx_polling_level[7]
        );

        //clear
        for (i = 0; i < 8; i++) {
            tx_agg_port_num[i] = 0;
            rx_agg_port_num[i] = 0;
        }

        for (i = 0; i < RX_TIMER_DYNAMIC_LEVELS; i++) {
            rx_polling_level[i] = 0;
        }

        asr_wait_rx_sk_buf_cnt = asr_wait_rx_sk_msgbuf_cnt = asr_total_int_rx_cnt = 0;

        #ifdef SDIO_DEAGGR
        min_deaggr_free_cnt = 0xFF;
        asr_wait_rx_sk_deaggrbuf_cnt = asr_total_rx_sk_deaggrbuf_cnt = 0;
        to_os_less8_cnt = to_os_less16_cnt = to_os_more16_cnt = max_to_os_delay_cnt = 0;
        #endif

        isr_write_clear_cnt = ipc_read_rx_bm_fail_cnt = ipc_read_tx_bm_fail_case0_cnt = ipc_read_tx_bm_fail_case2_cnt = ipc_read_tx_bm_fail_case3_cnt = ipc_isr_skip_main_task_cnt = 0;
        sdio_tx_evt_cnt = sdio_rx_evt_cnt = 0;
    }

    if (tx_status_debug)
    {
        asr_rtos_update_timer(&(asr_hw->tx_status_timer), tx_status_debug * 1000);
        asr_rtos_start_timer(&(asr_hw->tx_status_timer));
    }
}
#endif

#ifdef RX_TRIGGER_TIMER_ENABLE
// this timer used to replace sdio int trigger.
void rx_trigger_timer_handler(void* arg)
{
    struct asr_hw *asr_hw = (struct asr_hw*)arg;
    uint16_t r_bitmap = 0;
    uint8_t r_bitmap_tmp[3] = {0};
    int ret;

    if (asr_hw->rx_trigger_time <= 0)
        asr_hw->rx_trigger_time = RX_TRIGGER_TIMEOUT_MS;

    asr_rtos_update_timer(&(asr_hw->rx_trigger_timer), asr_hw->rx_trigger_time);
    asr_rtos_start_timer(&(asr_hw->rx_trigger_timer));

    uwifi_sdio_event_set(UWIFI_SDIO_EVENT_RX);

}
#endif




void rx_thread_timer_handler(void* arg)
{
    struct asr_hw *asr_hw = (struct asr_hw*)arg;
    uint32_t period_packet_num, period_rx_bytes, rx_speed;
    int i, j;
    static int rx_thread_train_times = 0;
    //int timer_period = asr_hw->rx_thread_time;

    //compute the number of the received packets during the rx thread timer period, and the number of bytes too.
    period_packet_num = asr_hw->rx_packet_num - asr_hw->last_rx_packet_num;
    period_rx_bytes = asr_hw->rx_bytes - asr_hw->last_rx_bytes;
    rx_speed = period_rx_bytes/asr_hw->rx_thread_time;

    for (i  = 0; i < RX_TIMER_DYNAMIC_LEVELS; i++) {
        if (period_packet_num >= rx_packets_speed_table[i].packets ||  rx_speed >= rx_bytes_speed_table[i].speed) {

            if (period_packet_num >= rx_packets_speed_table[i].packets)
                asr_hw->rx_thread_time = rx_packets_speed_table[i].time;

            if (rx_speed >= rx_bytes_speed_table[i].speed)
                asr_hw->rx_thread_time = rx_bytes_speed_table[i].time;

            rx_packets[i] += period_packet_num;
            rx_bytes[i] += period_rx_bytes;
            rx_times[i]++;

            rx_thread_train_times++;

            if (rx_thread_train_times == RX_TIMER_TRAIN_TIMES) {
                //dbg(D_ERR, D_UWIFI_CTRL,"train one time");

                for (j = 0; j < RX_TIMER_DYNAMIC_LEVELS; j++) {
                    //dynamic update the speed table
                    //rx_packets_speed_table[j].packets = rx_packets[j]/rx_times[j];
                    //rx_bytes_speed_table[j].bytes = rx_bytes[j]/rx_times[j];
                    rx_packets[j] = 0;
                    rx_bytes[j] = 0;
                    rx_times[j] = 0;

                }
                rx_thread_train_times = 0;
            }

            /*when it is in sniffer mode, give more timer trigger way*/
            if (asr_hw->rx_thread_time > 0
                #ifdef CFG_SNIFFER_SUPPORT
                || asr_hw->monitor_vif_idx != 0xFF
                #endif
                ) {
                #ifdef CFG_SNIFFER_SUPPORT
                if (asr_hw->monitor_vif_idx != 0xFF)
                    asr_hw->rx_thread_time = 400;
                #endif
                asr_rtos_update_timer(&(asr_hw->rx_thread_timer), asr_hw->rx_thread_time);
                asr_rtos_start_timer(&(asr_hw->rx_thread_timer));

                asr_hw->last_rx_packet_num = asr_hw->rx_packet_num;
                asr_hw->last_rx_bytes = asr_hw->rx_bytes;


                rx_polling_level[i]++;

                if (asr_hw->rx_thread_time >= 200)
                    dbg(D_ERR, D_UWIFI_CTRL,"%s: rx_thread_time %d  level=%d period_packet_num=%d,rx_speed =%d \n",__func__,asr_hw->rx_thread_time , i , period_packet_num, rx_speed);

                uwifi_sdio_event_set(UWIFI_SDIO_EVENT_RX);

            } else {
                asr_hw->rx_packet_num = asr_hw->last_rx_packet_num = 0;
                asr_hw->rx_bytes = asr_hw->last_rx_bytes = 0;

                for (j = 0; j < RX_TIMER_DYNAMIC_LEVELS; j++) {
                    rx_packets[j] = 0;
                    rx_bytes[j] = 0;
                    rx_times[j] = 0;
                }
            }

            break;
        }
    }

}
uint8_t is_fw_downloaded = 0;

void tx_aggr_timer_handler(void* arg)
{
    struct asr_hw *asr_hw = (struct asr_hw*)arg;

    //dbg(D_ERR, D_UWIFI_CTRL,"%s:%d\n",__func__,asr_hw->tx_agg_env.aggr_buf_cnt);

    if(asr_hw->tx_agg_env.aggr_buf_cnt > 0 ) {//&& !asr_hw->main_task_tx_flag){

        uwifi_sdio_event_set(UWIFI_SDIO_EVENT_RX);

    }

}

extern bool fw_download_done;

#ifdef SDIO_DEBUG
extern int count_sdio_card_gpio_isr;
extern int count_sdio_card_gpio_evt;
void sdio_debug_timer_handler(void* arg)
{
    struct asr_hw *asr_hw = (struct asr_hw*)arg;
    struct asr_vif *asr_vif = NULL;
    uint16_t w_bitmap,r_bitmap;

    if (asr_hw->sta_vif_idx < asr_hw->vif_max_num) {
        asr_vif  = asr_hw->vif_table[asr_hw->sta_vif_idx];
    } else if (asr_hw->ap_vif_idx < asr_hw->vif_max_num) {
        asr_vif  = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    asr_rtos_update_timer(&(asr_hw->sdio_debug_timer), SDIO_DEBUG_TIMEOUT_MS);
    asr_rtos_start_timer(&(asr_hw->sdio_debug_timer));
    w_bitmap = get_sdio_reg(WR_BITMAP_L);
    w_bitmap |= get_sdio_reg(WR_BITMAP_U) << 8;
    r_bitmap = get_sdio_reg(RD_BITMAP_L);
    r_bitmap |= get_sdio_reg(RD_BITMAP_U) << 8;
    dbg(D_ERR, D_UWIFI_CTRL,"sdio_debug: (%d %d) (%d,%d),%02X(%d %d 0x%04X)(%d 0x%04X),drv_flag=0x%lx\n"
        ,count_sdio_card_gpio_isr, count_sdio_card_gpio_evt
    ,fw_download_done,is_fw_downloaded
    ,get_sdio_reg(HOST_INT_STATUS)
        ,asr_hw->tx_agg_env.aggr_buf_cnt,asr_hw->tx_data_cur_idx, w_bitmap
        ,asr_hw->rx_data_cur_idx, r_bitmap
    ,asr_vif?asr_vif->dev_flags:0);

    count_sdio_card_gpio_isr = count_sdio_card_gpio_evt = 0;
}
#endif

#ifdef CFG_STATION_SUPPORT
void scan_cmd_timer_handle(void* arg)
{
    struct asr_hw *asr_hw = (struct asr_hw*)arg;
    struct asr_vif *asr_vif = NULL;

    if (asr_hw->sta_vif_idx < asr_hw->vif_max_num) {
        asr_vif  = asr_hw->vif_table[asr_hw->sta_vif_idx];
    } else if (asr_hw->ap_vif_idx < asr_hw->vif_max_num) {
        asr_vif  = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if (asr_vif && asr_test_bit(ASR_DEV_SCANING, &asr_vif->dev_flags)) {
        asr_rtos_update_timer(&(asr_hw->sdio_debug_timer), SDIO_DEBUG_TIMEOUT_MS);
        asr_rtos_start_timer(&(asr_hw->sdio_debug_timer));
    }

    dbg(D_ERR, D_UWIFI_CTRL, "%s\n",__func__);

    uwifi_sdio_event_set(UWIFI_SDIO_EVENT_RX);
}
#endif

static int asr_read_mm_info(struct asr_hw *asr_hw)
{
    int ret = -1;
    struct mm_get_info_cfm mm_info_cfm;

    #if 0
    if (asr_hw->driver_mode == DRIVER_MODE_ATE) {
        asr_hw->vif_max_num = 1;
        asr_hw->sta_max_num = 1;

        return 0;
    }
    #endif

    memset(&mm_info_cfm, 0, sizeof(struct mm_get_info_cfm));
#ifdef CONFIG_FW_HAVE_NOT_MM_INFO_MSG
    // fw version not the latest
    ret = 0;
    asr_hw->vif_max_num = NX_VIRT_DEV_MAX;
    asr_hw->sta_max_num = NX_REMOTE_STA_MAX;
#else
    if ((ret = asr_send_mm_get_info(asr_hw, &mm_info_cfm))) {
        return ret;
    }
    if (!mm_info_cfm.status) {
        #if 0
        if (mm_info_cfm.vif_num > NX_VIRT_DEV_MAX || mm_info_cfm.sta_num > NX_REMOTE_STA_MAX) {
            dbg(D_ERR, D_UWIFI_CTRL,"%s:ERROR,too large param, vif_num=%d,sta_num=%d\n",
                __func__, mm_info_cfm.vif_num, mm_info_cfm.sta_num);
            return -1;
        }
        #endif

        ret = 0;

        dbg(D_INF, D_UWIFI_CTRL, "%s:vif_num=%d,sta_num=%d\n", __func__, mm_info_cfm.vif_num, mm_info_cfm.sta_num);
        asr_hw->vif_max_num = mm_info_cfm.vif_num;
        asr_hw->sta_max_num = mm_info_cfm.sta_num;
    } else {
        ret = -1;
    }
#endif

    return ret;
}

int asr_read_sdio_txrx_info(struct asr_hw *asr_hw)
{
    struct mm_hif_sdio_info_ind *sdio_info_ind = &(asr_hw->hif_sdio_info_ind);
    int ret = 0;

    //check fw tx rx idx
    memset(sdio_info_ind, 0, sizeof(struct mm_hif_sdio_info_ind));
    ret = asr_send_hif_sdio_info_req(asr_hw);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "ASR: error hif sdio info read fail %d\n", ret);
        return ret;
    }

    if (sdio_info_ind->flag) {
        if (sdio_info_ind->host_tx_data_cur_idx >= 1 && sdio_info_ind->host_tx_data_cur_idx <= 15) {
            asr_hw->tx_data_cur_idx = sdio_info_ind->host_tx_data_cur_idx;
        }

        if (sdio_info_ind->rx_data_cur_idx >= 2 && sdio_info_ind->rx_data_cur_idx <= 15) {
            asr_hw->rx_data_cur_idx = sdio_info_ind->rx_data_cur_idx;
        }

        dbg(D_ERR, D_UWIFI_CTRL, "ASR: hif sdio info (%d %d %d)\n", sdio_info_ind->flag,
            sdio_info_ind->host_tx_data_cur_idx, sdio_info_ind->rx_data_cur_idx);
    } else {

        dbg(D_ERR, D_UWIFI_CTRL, "ASR: error hif sdio info read fail %d,%d\n", sdio_info_ind->flag, ret);

        return -1;
    }

    return ret;
}

static void asr_set_tx_rate_power(struct asr_hw *asr_hw)
{
    struct mm_set_tx_pwr_rate_cfm tx_pwr_cfm;
    struct mm_set_tx_pwr_rate_req tx_pwr_req;

    if (g_wifi_config.pwr_config) {
        dbg(D_ERR, D_UWIFI_CTRL, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
            g_wifi_config.tx_pwr[0],g_wifi_config.tx_pwr[4],g_wifi_config.tx_pwr[12],g_wifi_config.tx_pwr[13],
            g_wifi_config.tx_pwr[14],g_wifi_config.tx_pwr[15],g_wifi_config.tx_pwr[16],
            g_wifi_config.tx_pwr[17],g_wifi_config.tx_pwr[18],g_wifi_config.tx_pwr[19]);
        memset(&tx_pwr_cfm, 0, sizeof(tx_pwr_cfm));
        memcpy(&tx_pwr_req, g_wifi_config.tx_pwr, sizeof(tx_pwr_req));
        asr_send_set_tx_pwr_rate(asr_hw, &tx_pwr_cfm, &tx_pwr_req);
    }
}

extern int wpa_debug_level;
extern void sdio_card_gpio_hisr_func();

#ifdef SDIO_LOOPBACK_TEST
extern int g_use_irq_flag;
extern int asr_sdio_function_irq();
extern void sdio_loopback_init();
extern int sdio_loopback_test();
#endif

struct asr_hw* uwifi_platform_init(void)
{
    int i;
    int ret = 0;
    struct asr_hw *asr_hw = NULL;

    if(_FAIL == wlan_init_skb_priv())
    {
        dbg(D_ERR,D_UWIFI_CTRL,"wlan_init_skb_priv failed");
        return NULL;
    }
    asr_hw = &g_asr_hw;
    sp_asr_hw = asr_hw;
    memset(asr_hw, 0, sizeof(g_asr_hw));

    asr_hw->sta_vif_idx        = 0xff;
    asr_hw->ap_vif_idx         = 0xff;
#ifdef CFG_SNIFFER_SUPPORT
    asr_hw->monitor_vif_idx    = 0xff;
#endif

    asr_hw->wiphy = &g_wiphy;
    asr_hw->wiphy->bands[IEEE80211_BAND_2GHZ] = &asr_band_2GHz;

    asr_hw->mod_params = &asr_module_params;
    //get country code
    uwifi_get_country_code(asr_hw->country);

    asr_hw->vif_started = 0;
    asr_hw->adding_sta = false;    //asr_rx_ps_change_ind ,reserve
    asr_hw->preq_ie.buf = NULL;

    for (i = 0; i < NX_VIRT_DEV_MAX + NX_REMOTE_STA_MAX; i++)
        asr_hw->avail_idx_map |= BIT(i);
    asr_hwq_init(asr_hw);

    for (i = 0; i < NX_NB_TXQ; i++) {
        asr_hw->txq[i].idx = TXQ_INACTIVE;
    }
    /* Init RoC element pointer to NULL, indicate that RoC can be started */
    asr_hw->roc_elem = NULL;
    /* Cookie can not be 0 */
    asr_hw->roc_cookie_cnt = 1;

    skb_queue_head_init(&asr_hw->rx_data_sk_list);
    skb_queue_head_init(&asr_hw->rx_msg_sk_list);

    #ifdef SDIO_DEAGGR
    skb_queue_head_init(&asr_hw->rx_sk_sdio_deaggr_list);
    #endif

    #ifdef CFG_OOO_UPLOAD
    skb_queue_head_init(&asr_hw->rx_pending_skb_list);
    #endif

    skb_queue_head_init(&asr_hw->tx_sk_list);

    skb_queue_head_init(&asr_hw->rx_to_os_skb_list);

    #ifdef CFG_AMSDU_TEST
    skb_queue_head_init(&asr_hw->rx_sk_split_list);
    #endif

    INIT_LIST_HEAD(&asr_hw->vifs);

    //clear cmd crash flag
    asr_hw->cmd_mgr.state = 0;
    asr_hw->tx_data_cur_idx = 1;
    asr_hw->rx_data_cur_idx = 2;
    asr_hw->tx_use_bitmap = 1;    //for msg tx

    if(asr_rtos_init_mutex(&asr_hw->tx_msg_lock))
        goto free_wlan_init_skb_priv;

    if(asr_rtos_init_mutex(&asr_hw->mutex))
        goto free_wlan_init_skb_priv;

    if(asr_rtos_init_mutex(&asr_hw->tx_lock))
        goto free_mutex;

    if(asr_rtos_init_mutex(&asr_hw->scan_mutex))
        goto free_tx_lock;
    if(asr_rtos_init_mutex(&asr_hw->wifi_ready_mutex))
        goto free_scan_mutex;


     if(asr_rtos_init_mutex(&asr_hw->tx_agg_env_lock))
        goto free_wifi_ready_mutex;
    #ifdef TX_ARRG_TEST
    if(asr_rtos_init_mutex(&asr_hw->tx_agg_timer_lock))
        goto free_tx_agg_env_lock;

    asr_hw->main_task_tx_flag = false;
    #endif

    asr_sdio_enable_func(1);
    asr_sdio_init_config(asr_hw);
    sdio_host_enable_isr(0);
    //download fw into sdio card

#ifndef LOAD_MAC_ADDR_FROM_FW
    uwifi_get_mac(asr_hw->mac_addr);
#endif

    #if 1   // when download fw img, set 1
    if (0 == is_fw_downloaded)
    {
        ret = asr_download_fw(asr_hw);
        if (ret)
            goto free_tx_lock;
        is_fw_downloaded = 1;
    }
    #else
      is_fw_downloaded = 1;
      fw_download_done = 1;
    #endif

    dbg(D_ERR, D_UWIFI_CTRL, "%s: version:%s\r\n", __func__, asr_get_wifi_version());

    #ifdef THREADX
    asr_rtos_create_hisr(&sdio_card_gpio_hisr, "sdio_card_hisr", sdio_card_gpio_hisr_func);
    #else
    // zephyr
#ifndef SDIO_LOOPBACK_TEST
    asr_wlan_irq_subscribe(sdio_card_gpio_hisr_func);
#else
    sdio_loopback_init();
    if (g_use_irq_flag == 1)
    {
        asr_wlan_irq_subscribe(asr_sdio_function_irq);
    }
#endif
    #endif

    // when use 1bit mode, no need to enable card int.
    sdio_host_enable_isr(1);

    ret = asr_sdio_enable_int(HOST_INT_DNLD_EN | HOST_INT_UPLD_EN);
    if(!ret) {

        goto free_tx_lock;
    }
    /*set block size SDIO_BLOCK_SIZE*/
    asr_sdio_set_block_size(SDIO_BLOCK_SIZE);

    //dbg(D_ERR, D_UWIFI_CTRL, "%s step 7\r\n", __func__);

#ifdef RX_TRIGGER_TIMER_ENABLE
    asr_rtos_init_timer(&(asr_hw->rx_trigger_timer), RX_TRIGGER_TIMEOUT_MS, rx_trigger_timer_handler, asr_hw);
    asr_rtos_start_timer(&(asr_hw->rx_trigger_timer));
#endif

    asr_hw->pskb = asr_rtos_malloc(sizeof(struct sk_buff));
    if (NULL == asr_hw->pskb)
    {
        dbg(D_ERR, D_UWIFI_DATA,"skb malloc from stack failed\n");
        goto free_tx_lock;
    }

    if (asr_xmit_opt) {
        struct sk_buff *hif_buf_skb = NULL;
        int i;

        skb_queue_head_init(&asr_hw->tx_sk_free_list);

        // init tx_sk_free_list instead of tx agg buf for tx pkt save.
        for (i = 0; i < TX_AGG_BUF_UNIT_CNT; i++) {
            // Allocate a new hif tx buff
            if (asr_txhifbuffs_alloc(asr_hw, TX_AGG_BUF_UNIT_SIZE, &hif_buf_skb)) {
                dbg(D_ERR, D_UWIFI_DATA,"%s:%d: hif_buf(%d) ALLOC FAILED\n", __func__, __LINE__,i);
            } else {
                if (hif_buf_skb) {
                    memset(hif_buf_skb->data, 0, TX_AGG_BUF_UNIT_SIZE);
                    skb_queue_tail(&asr_hw->tx_sk_free_list, hif_buf_skb);
                }
            }
        }

        skb_queue_head_init(&asr_hw->tx_hif_free_buf_list);
        skb_queue_head_init(&asr_hw->tx_hif_skb_list);

        // init hif buf skb list,used for gen cmd53 agg tx.
        for (i = 0; i < 2; i++) {
            // Allocate a new hif tx buff
            if (asr_txhifbuffs_alloc(asr_hw, IPC_HIF_TXBUF_SIZE, &hif_buf_skb)) {
                dbg(D_ERR, D_UWIFI_DATA,"%s:%d: hif_buf(%d) ALLOC FAILED\n", __func__, __LINE__,i);
            } else {
                if (hif_buf_skb) {
                    memset(hif_buf_skb->data, 0, IPC_HIF_TXBUF_SIZE);

                    skb_queue_tail(&asr_hw->tx_hif_free_buf_list, hif_buf_skb);
                }
            }
        }

        dbg(D_ERR, D_UWIFI_DATA,"ASR: tx list free cnt(%d).\n", TX_AGG_BUF_UNIT_CNT);

    } else {

        asr_hw->tx_agg_env.aggr_buf = dev_alloc_skb_tx(TX_AGG_BUF_SIZE);
        if (!asr_hw->tx_agg_env.aggr_buf)
            goto free_pskb;

        asr_rtos_lock_mutex(&asr_hw->tx_agg_env_lock);
        asr_tx_agg_buf_reset(asr_hw);
        asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);

        dbg(D_ERR, D_UWIFI_DATA,"ASR: aggr_buf data (%p) size(%d).\n", asr_hw->tx_agg_env.aggr_buf->data,TX_AGG_BUF_SIZE);

    }

    asr_hw->sdio_reg = (uint8_t*)asr_rtos_malloc(SDIO_REG_READ_LENGTH);
    if (!asr_hw->sdio_reg)
        goto free_aggr_buf;

    dbg(D_ERR, D_UWIFI_DATA," asr_hw->sdio_reg addr = %p \n",asr_hw->sdio_reg);

    asr_hw->rx_packet_num = 0;
    asr_hw->last_rx_packet_num = 0;
    asr_hw->rx_bytes = asr_hw->last_rx_bytes = 0;
    asr_hw->rx_thread_time = 800;

#ifndef SDIO_LOOPBACK_TEST
    if ((ret = asr_platform_on(asr_hw)))
    {
        dbg(D_ERR, D_UWIFI_CTRL,"%s,asr_platform_on fail\r\n ",__func__);
        goto free_sdio_reg;
    }

    if ((ret = asr_send_reset(asr_hw)))
    {
        dbg(D_ERR, D_UWIFI_CTRL,"%s,asr_send_reset fail\r\n ",__func__);
        goto platform_off;
    }

    if ((ret = asr_send_version_req(asr_hw, &asr_hw->version_cfm)))
    {
        dbg(D_ERR, D_UWIFI_CTRL,"%s,asr_send_version_req fail\r\n ",__func__);
        goto platform_off;
    }

    if ((ret = asr_send_fw_softversion_req(asr_hw, &asr_hw->fw_softversion_cfm)))
    {
        dbg(D_ERR, D_UWIFI_CTRL,"%s,asr_send_fw_softversion_req fail\r\n ",__func__);
        goto platform_off;
    }

    dbg(D_ERR, D_UWIFI_CTRL, " fw version is:%s\n", asr_hw->fw_softversion_cfm.fw_softversion);

    ret = asr_read_mm_info(asr_hw);
    if (ret) {
        goto platform_off;
    }

    ret = asr_read_sdio_txrx_info(asr_hw);
    if (ret) {
        goto platform_off;
    }
#ifdef LOAD_MAC_ADDR_FROM_FW
    uwifi_get_mac(&asr_hw->mac_addr);
#endif

    if ((ret = asr_handle_dynparams(asr_hw, asr_hw->wiphy)))
        goto platform_off;
    /* Set parameters to firmware */
    asr_send_me_config_req(asr_hw);
    asr_send_me_chan_config_req(asr_hw);

    asr_set_tx_rate_power(asr_hw);

    // Check if it is the first opened VIF
    if (asr_hw->vif_started == 0)
    {
       // Start the FW
       asr_send_start(asr_hw);
       /* Device is now started */
       //asr_vif->dev_flags = true;
       asr_set_bit(ASR_DEV_STARTED, &asr_hw->phy_flags);
    }

    //asr_rtos_init_timer(&(asr_hw->amsdu_malloc_free_timer), 800, amsdu_malloc_free_handler, asr_hw);
    //asr_rtos_start_timer(&(asr_hw->amsdu_malloc_free_timer));

#ifdef CONFIG_ASR_KEY_DBG
    // tx_status_timer used to detect tx status
    if (tx_status_debug)
    {
        asr_rtos_init_timer(&(asr_hw->tx_status_timer), (tx_status_debug * 1000), tx_status_timer_handler, asr_hw);
        asr_rtos_start_timer(&(asr_hw->tx_status_timer));
    }
#endif

    asr_rtos_init_timer(&(asr_hw->rx_thread_timer), 800, rx_thread_timer_handler, asr_hw);
    #ifdef AWORKS
    asr_rtos_start_timer(&(asr_hw->rx_thread_timer));
    #endif
    asr_rtos_init_timer(&(asr_hw->tx_aggr_timer), TX_AGGR_TIMEOUT_MS, tx_aggr_timer_handler, asr_hw);

    // wifi debug init.
    #ifdef SDIO_DEBUG
    asr_rtos_init_timer(&(asr_hw->sdio_debug_timer), SDIO_DEBUG_TIMEOUT_MS, sdio_debug_timer_handler, asr_hw);
    asr_rtos_start_timer(&(asr_hw->sdio_debug_timer));
    #endif
    wpa_debug_level = 5;   //default wpa log is err level.

#ifdef CFG_STATION_SUPPORT
    asr_rtos_init_timer(&(asr_hw->scan_cmd_timer), ASR_SCAN_CMD_TIMEOUT_MS, scan_cmd_timer_handle, asr_hw);
#endif

    dbg(D_ERR, D_UWIFI_CTRL, "%s done,phy_flags=0x%x", __func__,(unsigned int)asr_hw->phy_flags);

#else
    sdio_loopback_test();

#endif

    return asr_hw;
platform_off:
    asr_platform_off(asr_hw);
free_sdio_reg:
    asr_rtos_free(asr_hw->sdio_reg);
free_aggr_buf:
    dev_kfree_skb_tx(asr_hw->tx_agg_env.aggr_buf);
free_pskb:
    asr_rtos_free(asr_hw->pskb);

free_tx_agg_timer_lock:
    asr_rtos_deinit_mutex(&asr_hw->tx_agg_timer_lock);

free_tx_agg_env_lock:
    asr_rtos_deinit_mutex(&asr_hw->tx_agg_env_lock);

free_wifi_ready_mutex:
    asr_rtos_deinit_mutex(&asr_hw->wifi_ready_mutex);
free_scan_mutex:
    asr_rtos_deinit_mutex(&asr_hw->scan_mutex);
free_tx_lock:
    asr_rtos_deinit_mutex(&asr_hw->tx_lock);
free_mutex:
    asr_rtos_deinit_mutex(&asr_hw->mutex);
free_wlan_init_skb_priv:
    wlan_free_skb_priv();
    asr_hw = NULL;

    return NULL;
}

void uwifi_platform_deinit(struct asr_hw *asr_hw, struct asr_vif *asr_vif)
{
    //dbg(D_DBG, D_UWIFI_CTRL, "%s asr_hw->vif_started1=%d\r\n", __func__,asr_hw->vif_started);
    if (asr_hw->vif_started != 0)
    {
        //dbg(D_DBG, D_UWIFI_CTRL, "%s asr_hw->aaaaaa=%d\r\n", __func__,asr_hw->vif_started);
        if(asr_vif){

            //dbg(D_DBG, D_UWIFI_CTRL, "%s asr_hw->bbbbbbb=%d\r\n", __func__,asr_hw->vif_started);
            uwifi_netdev_ops.ndo_stop(asr_hw, asr_vif);
            //dbg(D_DBG, D_UWIFI_CTRL, "%s asr_hw->vif_started2=%d\r\n", __func__,asr_hw->vif_started);
        }
    }
    //dbg(D_DBG, D_UWIFI_CTRL, "%s asr_hw->vif_started3=%d\r\n", __func__,asr_hw->vif_started);
    if(asr_hw->vif_started == 0)
    {
        //asr_vif->dev_flags = false;
        asr_clear_bit(ASR_DEV_INITED, &asr_hw->phy_flags);
        asr_rtos_deinit_mutex(&asr_hw->tx_lock);
        asr_rtos_deinit_mutex(&asr_hw->mutex);
        asr_rtos_deinit_mutex(&asr_hw->wifi_ready_mutex);
        asr_rtos_deinit_mutex(&asr_hw->scan_mutex);
        asr_rtos_deinit_mutex(&asr_hw->tx_agg_env_lock);

        asr_rtos_deinit_mutex(&asr_hw->tx_agg_timer_lock);


        if (NULL != asr_hw->amsdu_malloc_free_timer.handle)
            asr_rtos_deinit_timer(&(asr_hw->amsdu_malloc_free_timer));
        #ifdef RX_TRIGGER_TIMER_ENABLE
        if (NULL != asr_hw->rx_trigger_timer.handle)
            asr_rtos_deinit_timer(&(asr_hw->rx_trigger_timer));
        #endif
        if (NULL != asr_hw->rx_thread_timer.handle)
            asr_rtos_deinit_timer(&(asr_hw->rx_thread_timer));

        if (NULL != asr_hw->tx_aggr_timer.handle)
            asr_rtos_deinit_timer(&(asr_hw->tx_aggr_timer));


        #ifdef SDIO_DEBUG
        if (NULL != asr_hw->sdio_debug_timer.handle)
            asr_rtos_deinit_timer(&(asr_hw->sdio_debug_timer));
        #endif

        asr_platform_off(asr_hw);
        //if(asr_hw->pskb->data)
        //    asr_rtos_free(asr_hw->pskb->data);

        asr_rtos_free(asr_hw->sdio_reg);

        if (asr_xmit_opt) {
            asr_txhifbuffs_dealloc(asr_hw);
        } else {
            dev_kfree_skb_tx(asr_hw->tx_agg_env.aggr_buf);
        }

        asr_rtos_free(asr_hw->pskb);
        //#ifdef SDIO_CLOSE_PS
        #ifdef THREADX
        extern asr_hisr_t sdio_card_gpio_hisr;
        asr_rtos_delete_hisr(&sdio_card_gpio_hisr);
        #endif
        //#endif
        sp_asr_hw = NULL;
        wlan_free_skb_priv();
        //#ifdef SDIO_CLOSE_PS
        asr_sdio_disable_int(HOST_INT_DNLD_EN | HOST_INT_UPLD_EN);
        sdio_host_enable_isr(0);
        //#endif
        #ifdef ASR_A0V1
        //wifi_deinit_config();
        #elif defined ASR_A0V2
        wifi_deinit_config();
        #endif
        //#ifdef SDIO_CLOSE_PS
        //is_fw_downloaded = 0;
        //#endif
        dbg(D_ERR, D_UWIFI_CTRL, "%s is_fw_downloaded=%d\r\n", __func__,is_fw_downloaded);
        IOpenCloseStateCallback(WIFI_CLOSE_SUCCESS);
    }
}

void uwifi_platform_deinit_api(void)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();

    if(asr_hw != NULL) {
        if( STA_MODE == current_iftype)
            uwifi_platform_deinit(asr_hw, asr_hw->vif_table[asr_hw->sta_vif_idx]);
        else if((SAP_MODE == current_iftype)||(NL80211_IFTYPE_AP == asr_hw->vif_table[asr_hw->ap_vif_idx]->iftype))
            uwifi_platform_deinit(asr_hw, asr_hw->vif_table[asr_hw->ap_vif_idx]);
        else
            uwifi_platform_deinit(asr_hw, NULL);
    }
}

#ifdef CFG_STATION_SUPPORT
#define WAIT_CONNECT_TIME_UNIT 200  //us
#define WAIT_CONNECT_TIMES  15
int uwifi_wpa_clear_wifi_status(bool autoconnect)
{
    struct asr_hw          *pst_asr_hw     = uwifi_get_asr_hw();
    struct asr_vif         *pst_asr_vif    = NULL;
    struct wifi_conn_info   *pst_conn_info   = NULL;
    struct config_info      *pst_config_info = NULL;

    int l_ret = -1;

    if (NULL == pst_asr_hw)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "uwifi_wpa_clear_wifi_status:no asr hw!");
        return -1;
    }

    if (pst_asr_hw->sta_vif_idx < pst_asr_hw->vif_max_num)
        pst_asr_vif = pst_asr_hw->vif_table[pst_asr_hw->sta_vif_idx];

    if (NULL == pst_asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "uwifi_wpa_clear_wifi_status:no sta vif!");
        return -1;
    }

    pst_conn_info = &(pst_asr_vif->sta.connInfo);
    // if being connecting, wait a while for connecting done.
    if (pst_conn_info->wifiState == WIFI_CONNECTING)
    {
        uint8_t i = 0;

        dbg(D_ERR, D_UWIFI_CTRL, "uwcws:CONNECTING!");

        while((i++ < WAIT_CONNECT_TIMES) && (pst_conn_info->wifiState != WIFI_ASSOCIATED) )
        {
            if(SCAN_DEINIT == scan_done_flag) return 0;
            asr_rtos_delay_milliseconds(WAIT_CONNECT_TIME_UNIT);
        }

        if(pst_conn_info->wifiState == WIFI_CONNECTING)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "uwcws:%d %d", i, pst_conn_info->wifiState);
            //dbg(D_ERR, D_UWIFI_CTRL, "%s: time out ready=%d,err=%d\r\n",__func__,wifi_ready,wifi_connect_err);
            //dbg(D_ERR, D_UWIFI_CTRL, "%s: time out scan_done=%d,fail=%d,psk=%d\r\n",__func__,scan_done_flag,wifi_conect_fail_code,wifi_psk_is_ture);
            //asr_wlan_set_disconnect_status(WLAN_STA_MODE_CONN_TIMEOUT);
            dbg(D_ERR, D_UWIFI_CTRL, "uwcws:%d %d", i, pst_conn_info->wifiState);
            return -1;
        }
    }

    /* disable auto connect */
    pst_config_info = &(pst_asr_vif->sta.configInfo);

    if (autoconnect == false)
        pst_config_info->en_autoconnect = AUTOCONNECT_DISABLED;

    if (pst_conn_info->wifiState >= WIFI_ASSOCIATED)
    {
        l_ret = wpa_disconnect();
        if (0 != l_ret)
        {
            dbg(D_ERR,D_UWIFI_CTRL,"%s:wpa_disconnect failed:%d",__func__, l_ret);
            return -1;
        }
        if (AUTOCONNECT_DISABLED == pst_config_info->en_autoconnect)
        {
            /* In user disconnect or user close wifi case, delete key here */
            if (pst_config_info->connect_ap_info.security == WIFI_ENCRYP_WEP)
            {
                uwifi_cfg80211_ops.del_key(pst_asr_vif, 0, false, NULL);
            }
            else if (pst_config_info->connect_ap_info.security != WIFI_ENCRYP_OPEN)
            {
                wpas_psk_deinit(g_pwpa_priv[COEX_MODE_STA]);
                wpas_priv_free(WLAN_OPMODE_INFRA);
            }
        }

        if (asr_test_bit(ASR_DEV_STA_DISCONNECTING, &pst_asr_vif->dev_flags)){
            asr_msleep(20);
        }
    }

    if (autoconnect == true)
        return 0;

    if (NULL != pst_config_info->wifi_retry_timer.handle)
    {
        /* deinit the wifi_retry timer */
        asr_rtos_deinit_timer(&(pst_config_info->wifi_retry_timer));
    }

    /* clear config info */
    memset(pst_config_info, 0, sizeof(struct config_info));
    return 0;
}
#endif

extern uint8_t user_pmk[PMK_LEN];

void uwifi_close(uint32_t iftype)  //TODO_ASR: return value
{
    struct asr_hw *asr_hw = sp_asr_hw;
    struct asr_vif *asr_vif = NULL;
    struct sk_buff *skb = NULL;
    uint16_t bitmap;
    int ret;
    int timeout = 500;
    struct asr_vif *asr_vif_tmp = NULL;

    //asr_wlan_set_wifi_ready_status(WIFI_NOT_READY);
    //g_sta_close_status.is_waiting_close = 1;
    if(NULL == asr_hw)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"wifi is closed, do not close again!\n");
        return;
    }

#ifdef CFG_SOFTAP_SUPPORT
    if(iftype == NL80211_IFTYPE_AP)
    {
        if(asr_hw->ap_vif_idx >= asr_hw->vif_max_num)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "softap has been closed");
            return;
        }

        asr_vif  = asr_hw->vif_table[asr_hw->ap_vif_idx];

        asr_set_bit(ASR_DEV_CLOSEING, &asr_vif->dev_flags);
        uwifi_close_ap(asr_hw);
        asr_clear_bit(ASR_DEV_CLOSEING, &asr_vif->dev_flags);
    }
    else
#endif
#ifdef CFG_STATION_SUPPORT
    if(iftype == NL80211_IFTYPE_STATION)
    {
        if(asr_hw->sta_vif_idx >= asr_hw->vif_max_num)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "sta has been closed");
            return;
        }
        asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];

        asr_set_bit(ASR_DEV_PRECLOSEING, &asr_vif->dev_flags);

        uwifi_wpa_clear_wifi_status(false);

        /* Ensure that we won't process disconnect ind */
        asr_rtos_lock_mutex(&asr_hw->cmd_mgr.lock);
        if((asr_vif->up) && (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_STATION))  //COEX_TODO
        {
            //IwifiStatusHandler((WiFiEvent)EVENT_STATION_DOWN);//WiFiEvent status
            memset(&asr_vif->sta.connInfo,0,sizeof(struct wifi_conn_info));
            asr_vif->sta.connInfo.wifiState = WIFI_DISCONNECTED;
            //other code del key
        }
        asr_rtos_unlock_mutex(&asr_hw->cmd_mgr.lock);

        asr_set_bit(ASR_DEV_CLOSEING, &asr_vif->dev_flags);
        asr_clear_bit(ASR_DEV_PRECLOSEING, &asr_vif->dev_flags);

        while (asr_test_bit(ASR_DEV_SCANING, &asr_vif->dev_flags) && timeout--) {
            asr_rtos_delay_milliseconds(10);
        }

        asr_clear_bit(ASR_DEV_SCANING, &asr_vif->dev_flags);
        asr_clear_bit(ASR_DEV_STA_CONNECTING, &asr_vif->dev_flags);
        asr_clear_bit(ASR_DEV_STA_DISCONNECTING, &asr_vif->dev_flags);

        /* Abort scan request on the vif */
        if (asr_hw->scan_request) {
            int i;
            //cfg80211_scan_done(asr_hw->scan_request, true);
            for(i=0;i<asr_hw->scan_request->n_channels;i++)
            {
                asr_rtos_free(asr_hw->scan_request->channels[i]);
                asr_hw->scan_request->channels[i] = NULL;
            }
            asr_rtos_free(asr_hw->scan_request);
            asr_hw->scan_request = NULL;
        }
        asr_hw->sta_vif_idx = 0xff;

        uwifi_netdev_ops.ndo_stop(asr_hw, asr_vif);

        dbg(D_CRT, D_UWIFI_CTRL, "%s wifi closed",__func__);
        asr_clear_bit(ASR_DEV_CLOSEING, &asr_vif->dev_flags);
        asr_clear_bit(ASR_DEV_STA_AUTH, &asr_vif->dev_flags);
        asr_clear_bit(ASR_DEV_STA_CONNECTED, &asr_vif->dev_flags);
        asr_clear_bit(ASR_DEV_STA_DHCPEND, &asr_vif->dev_flags);

    } else
#endif
#ifdef CFG_SNIFFER_SUPPORT
    if (iftype == NL80211_IFTYPE_MONITOR)
    {
        if(asr_hw->monitor_vif_idx == 0xff)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "monitor has been closed");
            return;
        }
        asr_vif = asr_hw->vif_table[asr_hw->monitor_vif_idx];
        asr_hw->monitor_vif_idx    = 0xff;
        asr_txq_vif_deinit_idle_mode(asr_hw,NULL);
        asr_send_set_idle(asr_hw, 1);

        uwifi_netdev_ops.ndo_stop(asr_hw, asr_vif);

        // sniffer reuse asr_rxdataind
        //asr_hw->ipc_env->cb.recv_data_ind  = asr_rxdataind;
    } else
#endif
    {
        dbg(D_ERR, D_UWIFI_CTRL, "iftype error:%d",(int)iftype);
        return;
    }

    {
        ///////////// common free code. ////////////////////////////////////////////////////////////
        //clear the tx buffer, the buffered data will be dropped. there will no send data in tx direction
        if (asr_xmit_opt) {
            asr_rtos_lock_mutex(&asr_hw->tx_lock);
            asr_drop_tx_vif_skb(asr_hw,asr_vif);
            asr_vif->tx_skb_cnt = 0;
            asr_rtos_unlock_mutex(&asr_hw->tx_lock);
        } else {
            asr_rtos_lock_mutex(&asr_hw->tx_agg_env_lock);
            asr_tx_agg_buf_mask_vif(asr_hw,asr_vif);
            asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);
        }

        if ((asr_xmit_opt == false) && asr_hw->vif_started == 0) {

            asr_rtos_lock_mutex(&asr_hw->tx_agg_env_lock);
            asr_tx_agg_buf_reset(asr_hw);
            list_for_each_entry(asr_vif_tmp,&asr_hw->vifs,list){
                asr_vif_tmp->txring_bytes = 0;
            }
            asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);
        }

        //if there are still data in rx direction, read it out
        uwifi_sdio_event_set(UWIFI_SDIO_EVENT_RX);

        asr_sdio_claim_host();
        ret = asr_sdio_read_cmd53(0, asr_hw->sdio_reg, 0x40);
        asr_sdio_release_host();
        if (ret){
            dbg(D_ERR, D_UWIFI_CTRL, "%s get HOST_INT_STATUS failed(0x8) ret:%d\n", __func__, ret);
        }
        bitmap = get_sdio_reg(RD_BITMAP_L);
        bitmap |= get_sdio_reg(RD_BITMAP_U) << 8;
        while (bitmap != 0x0) {
            asr_rtos_delay_milliseconds(5);
            bitmap = get_sdio_reg(RD_BITMAP_L);
            bitmap |= get_sdio_reg(RD_BITMAP_U) << 8;
            dbg(D_CRT, D_UWIFI_CTRL, "%s bitmap:0x%x", __func__, bitmap);
        }

        /////////////////////////////////////////////////////////////////////////

        //clear rx_to_os_skb_list be empty when close
        while (!skb_queue_empty(&asr_hw->rx_to_os_skb_list)) {
            skb = skb_dequeue(&asr_hw->rx_to_os_skb_list);
            #ifndef SDIO_DEAGGR
            memset(skb->data, 0, asr_hw->ipc_env->rx_bufsz);
            // Add the sk buffer structure in the table of rx buffer
            skb_queue_tail(&asr_hw->rx_data_sk_list, skb);
            #else
            memset(skb->data, 0, asr_hw->ipc_env->rx_bufsz_sdio_deagg);
            // Add the sk buffer structure in the table of rx buffer
            skb_queue_tail(&asr_hw->rx_sk_sdio_deaggr_list, skb);
            #endif
        }

        #ifdef CFG_OOO_UPLOAD
        while (!skb_queue_empty(&asr_hw->rx_pending_skb_list)) {
            skb = skb_dequeue(&asr_hw->rx_pending_skb_list);

            memset(skb->data, 0, asr_hw->ipc_env->rx_bufsz_sdio_deagg);
            // Add the sk buffer structure in the table of rx buffer
            skb_queue_tail(&asr_hw->rx_sk_sdio_deaggr_list, skb);
        }
        #endif
    }
    if ((asr_hw->amsdu_malloc_free_timer.handle != NULL) && asr_rtos_is_timer_running(&(asr_hw->amsdu_malloc_free_timer)))
        asr_rtos_stop_timer(&(asr_hw->amsdu_malloc_free_timer));
    if ((asr_hw->rx_thread_timer.handle != NULL) && asr_rtos_is_timer_running(&(asr_hw->rx_thread_timer)))
        asr_rtos_stop_timer(&(asr_hw->rx_thread_timer));

    if ((asr_hw->tx_aggr_timer.handle != NULL) && asr_rtos_is_timer_running(&(asr_hw->tx_aggr_timer)))
        asr_rtos_stop_timer(&(asr_hw->tx_aggr_timer));


    #if 0//def SDIO_DEBUG
    if ((asr_hw->sdio_debug_timer.handle != NULL) && asr_rtos_is_timer_running(&(asr_hw->sdio_debug_timer)))
        asr_rtos_stop_timer(&(asr_hw->sdio_debug_timer));
    #endif

    if(iftype == NL80211_IFTYPE_STATION)
        wifi_event_cb(WLAN_EVENT_STA_CLOSE,NULL);

#ifdef CFG_ENCRYPT_SUPPORT
    asr_wlan_clear_pmk();
#endif
    //moved to asr_wlan_open
    //current_iftype = 0xFF;
}

#ifdef CFG_SNIFFER_SUPPORT
int uwifi_start_sniffer_mode(void)
{
    struct asr_hw  *asr_hw;
    struct asr_vif *asr_vif;
    if (current_iftype != 0xff)
        uwifi_close(current_iftype);
    if(sp_asr_hw == NULL)
    {
        sp_asr_hw = uwifi_platform_init();
        if (NULL == sp_asr_hw)
        {
            dbg(D_ERR,D_UWIFI_CTRL,"uwifi_start_sniffer_mode::platform init failed");
        }
    }
    else
    {
        dbg(D_CRT,D_UWIFI_CTRL,"wifi open for the second time");
    }

    asr_hw = sp_asr_hw;
    if(asr_hw->monitor_vif_idx != 0xff)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"monitor cannot open twice");
        return -1;
    }

    if (asr_interface_add(asr_hw, NL80211_IFTYPE_MONITOR, NULL, &asr_vif))
    {
        dbg(D_ERR, D_UWIFI_CTRL, "uwifi add monitor vif interface fail");
        return -1;
    }

    #if 0  // reuse  asr_rxdataind
    asr_hw->ipc_env->cb.recv_data_ind = asr_rxdataind_sniffer;
    #endif

    if(uwifi_netdev_ops.ndo_open(asr_hw, asr_vif) == 0)
    {
        asr_txq_vif_init_idle_mode(asr_hw, 0);
    }
    else
    {
        dbg(D_ERR, D_UWIFI_CTRL, "uwifi sniffer ndo_open fail");
        return -1;
    }
    //asr_rtos_start_timer(&(asr_hw->rx_thread_timer));
    #ifdef CFG_MIMO_UF
    uf_rx_enable();
    #endif

    return 0;
}
#endif


extern uint8_t lwip_comm_init(enum open_mode openmode);

int wifi_open_ap_check(struct asr_hw  *asr_hw)
{
    if(asr_hw->ap_vif_idx >= 1 && asr_hw->ap_vif_idx != 0xFF)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"exceed max softap number %x", asr_hw->ap_vif_idx);
        return -1;
    }
    else if(asr_hw->ap_vif_idx < 1)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"second softap open");
    }

    if(asr_hw->ap_vif_idx != 0xFF)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"cannot open twice!");
        return -1;
    }

    return 0;
}

int uwifi_start_mode(enum open_mode openmode,struct softap_info_t* softap_info)
{
    struct asr_hw *asr_hw;
    int result = 0;

    if(sp_asr_hw == NULL)
    {
        sp_asr_hw = uwifi_platform_init();
    }
    asr_hw = sp_asr_hw;

    if(asr_hw != NULL)//platform init success
    {
        struct asr_vif *asr_vif;
        switch(openmode)
        {
#ifdef CFG_STATION_SUPPORT
            case STA_MODE:
                if(asr_hw->sta_vif_idx != 0xff)
                {
                    dbg(D_ERR,D_UWIFI_CTRL,"sta cannot open twice");
                    result = -1;
                    break;
                }

                //   init ethernetif.
                lwip_comm_init(STA_MODE);

                /* Add an initial station interface */
                if(asr_interface_add(asr_hw, NL80211_IFTYPE_STATION, NULL, &asr_vif))
                {
                    dbg(D_ERR, D_UWIFI_CTRL, "uwifi add vif interface fail");
                    result = -1;
                    break;
                }


                if(uwifi_netdev_ops.ndo_open(asr_hw, asr_vif) == 0)
                {
                    IOpenCloseStateCallback(WIFI_OPEN_SUCCESS);
                }
                else
                {
                    result = -1;
                }
                break;
#endif
#ifdef CFG_SOFTAP_SUPPORT
            case SAP_MODE:
            {
                result = wifi_open_ap_check(asr_hw);
                if(result == -1)
                    break;

                dbg(D_ERR, D_UWIFI_CTRL, "start sap mode\r\n");
                lwip_comm_init(SAP_MODE);
                if(asr_interface_add(asr_hw, NL80211_IFTYPE_AP, NULL, &asr_vif))
                {
                    dbg(D_ERR, D_UWIFI_CTRL, "uwifi add vif interface fail");
                    result = -1;
                    break;
                }
                dbg(D_ERR, D_UWIFI_CTRL, "asr_interface_add\r\n");
                if(uwifi_netdev_ops.ndo_open(asr_hw, asr_vif))
                {
                    dbg(D_ERR, D_UWIFI_CTRL, "uwifi open cmd fail");
                    result = -1;
                    break;
                }
                //dbg(D_ERR, D_UWIFI_CTRL, "ndo_open done softap_info->ssid_len=%d\r\n",softap_info->ssid_len);
                //dbg(D_ERR, D_UWIFI_CTRL, "%s ssid:%s chanel:%d interval:%d hide:%d ssid_len=%d len2=%d\r\n", __func__,softap_info->ssid, softap_info->chan, softap_info->interval, softap_info->hide,strlen(softap_info->ssid),softap_info->ssid_len);
                if(softap_info->pwd_len)
                {
                    dbg(D_ERR, D_UWIFI_CTRL, "%s  pwd:%s \r\n", __func__, softap_info->password);
                }
                if(softap_info->ssid_len > WIFI_SSID_LEN)
                {
                    result = -1;
                    dbg(D_ERR, D_UWIFI_CTRL, "uwifi_open_ap failed,len=%d oversize\r\n",softap_info->ssid_len);
                    break;
                }
                if(uwifi_open_ap(asr_hw, softap_info) == 0)
                {
                    IOpenCloseStateCallback(WIFI_OPEN_SUCCESS);

                    #if NX_STA_AP_COEX
                    if(current_iftype == STA_MODE)
                    {
                        current_iftype = STA_AP_MODE;
                    }
                    else if(current_iftype == 0xff)
                    {
                        current_iftype = SAP_MODE;
                    }
                    #else
                    current_iftype = SAP_MODE;
                    #endif
                }
                else
                {
                    result = -1;
                    dbg(D_ERR, D_UWIFI_CTRL, "uwifi_open_ap failed\r\n");
                    break;
                }
                dbg(D_ERR, D_UWIFI_CTRL, "uwifi_open_ap done,result=%d\r\n",result);
                asr_dhcps_start();
                wifi_event_cb(WLAN_EVENT_AP_UP, NULL);

                dbg(D_ERR, D_UWIFI_CTRL, "uwifi start mode sap done\r\n");
                break;
            }
#endif
            //case TEST_MODE: //suppport?
            //    break;
            default: //default STA mode
                #ifdef CFG_STATION_SUPPORT
                if(asr_hw->sta_vif_idx != 0xff)
                {
                    dbg(D_ERR,D_UWIFI_CTRL,"sta cannot open twice");
                    result = -1;
                    break;
                }
                /* Add an initial station interface */
                if(asr_interface_add(asr_hw, NL80211_IFTYPE_STATION, NULL, &asr_vif))
                {
                    dbg(D_ERR, D_UWIFI_CTRL, "uwifi add vif interface fail");
                    result = -1;
                    break;
                }
                if(uwifi_netdev_ops.ndo_open(asr_hw, asr_vif)==0)
                {
                    IOpenCloseStateCallback(WIFI_OPEN_SUCCESS);
                }
                else
                #endif
                {
                    result = -1;
                }
                break;
        }
    }
    else
    {
        dbg(D_ERR,D_UWIFI_CTRL,"wifi open failed");
        result = -1;
    }
    //asr_rtos_start_timer(&(asr_hw->rx_thread_timer));
    return result;
}

#ifdef CFG_STATION_SUPPORT
/*************************************************
Function: uwifi_wifi_retry_timer_callback
Description: start a autoconnect process when timer timeout
Input:  struct config_info *pst_config_info: config info saved in VIF.
Output: nothing
Return: void
Author: BaoshunFang
Date:   2018.4.17
Others:
*************************************************/
static void uwifi_wifi_retry_timer_callback(void* arg)
{
    struct config_info *pst_config_info = (struct config_info*)arg;

    if (NULL != pst_config_info->wifi_retry_timer.handle && asr_rtos_is_timer_running(&(pst_config_info->wifi_retry_timer)))
    {
        /* just stop timer*/
        asr_rtos_stop_timer(&(pst_config_info->wifi_retry_timer));
    }
    if (pst_config_info->en_autoconnect == AUTOCONNECT_DISABLED)
    {
        return;
    }
    uwifi_msg_timer2u_autoconnect_sta((uint32_t)arg);
}

struct scan_result* asr_wlan_get_scan_ap(char* ssid,char ssid_len, char ap_security)
{
    struct asr_hw  *pst_asr_hw    = uwifi_get_asr_hw();
    struct scan_result* current = pst_asr_hw->phead_scan_result;

    while(NULL != current)
    {
        if(SCAN_DEINIT == scan_done_flag)
            return current;

        dbg(D_INF, D_UWIFI_CTRL, "%s ssid:%s current->encryp_protocol=%d ap_security=%d bssid:%02x:%02x:%02x:%02x:%02x:%02x \r\n", __func__, current->ssid.ssid,current->encryp_protocol,ap_security
        ,current->bssid[0],current->bssid[1],current->bssid[2],current->bssid[3],current->bssid[4],current->bssid[5]);
        if ((current->ssid.ssid_len != ssid_len) ||
            (0 != memcmp(current->ssid.ssid, ssid, ssid_len)))
        {
            /* ssid didn't match */
            current = current->next_ptr;
            continue;
        }

        if (current->encryp_protocol == ap_security)
        {
            /* find matched ap, break */
            break;
        }

        if ((WIFI_ENCRYP_AUTO == ap_security) &&
            (WIFI_ENCRYP_OPEN != current->encryp_protocol))
        {
            /* find matched ap, update security to config info and break */
            break;
        }
        current = current->next_ptr;
    }
    return current;
}

struct scan_result* asr_wlan_get_scan_ap_by_bssid(char* bssid, char ap_security)
{
    struct asr_hw  *pst_asr_hw    = uwifi_get_asr_hw();
    struct scan_result* current = pst_asr_hw->phead_scan_result;

    while(NULL != current)
    {
        if (0 != memcmp(current->bssid, bssid, ETH_ALEN))
        {
            /* ssid didn't match */
            current = current->next_ptr;
            continue;
        }

        if(uwifi_check_legal_channel(phy_freq_to_channel(0, current->channel.center_freq)))
        {
            dbg(D_ERR, D_UWIFI_CTRL, "illegal freq %d",current->channel.center_freq);
            current = current->next_ptr;
            continue;
        }

        if (current->encryp_protocol == ap_security)
        {
            /* find matched ap, break */
            break;
        }

        if ((WIFI_ENCRYP_AUTO == ap_security) &&
            (WIFI_ENCRYP_OPEN != current->encryp_protocol))
        {
            /* find matched ap, update security to config info and break */
            break;
        }
        current = current->next_ptr;
    }
    return current;
}


int uwifi_scan_for_further_info(struct config_info *pst_config_info)
{
    struct wifi_scan_t  st_wifi_scan;
    struct scan_result *pst_dst_scan_info = NULL;
    int l_ret       = -1;
    int scan_try    = 0;
    memset(&st_wifi_scan, 0, sizeof(struct wifi_scan_t));
    st_wifi_scan.scanmode = SCAN_SSID;
    st_wifi_scan.freq = 0;
    if(pst_config_info->connect_ap_info.channel)
    {
        if(uwifi_check_legal_channel(pst_config_info->connect_ap_info.channel))
        {
            dbg(D_ERR, D_UWIFI_CTRL, "illegal channel %d",pst_config_info->connect_ap_info.channel);
            return -1;
        }
        st_wifi_scan.scanmode = SCAN_FREQ_SSID;
        st_wifi_scan.freq = phy_channel_to_freq(PHY_BAND_2G4, pst_config_info->connect_ap_info.channel);
    }

    memcpy(st_wifi_scan.bssid, pst_config_info->connect_ap_info.bssid, MAC_ADDR_LEN);

    st_wifi_scan.ssid_len = pst_config_info->connect_ap_info.ssid_len;

    if(st_wifi_scan.ssid_len) {
        st_wifi_scan.ssid = asr_rtos_malloc(st_wifi_scan.ssid_len + 1);
        if(st_wifi_scan.ssid == NULL)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "uwifi:malloc fail");
            return -1;
        }
        memcpy(st_wifi_scan.ssid, pst_config_info->connect_ap_info.ssid, st_wifi_scan.ssid_len);
    }

    if(SCAN_START!=scan_done_flag)
        asr_wlan_set_scan_status(SCAN_START);

    while (scan_try++ < UWIFI_AOS_SCAN_MAX_TRIES )
    {
        if( SCAN_DEINIT == scan_done_flag)
        {
            if(st_wifi_scan.ssid) {
                asr_rtos_free(st_wifi_scan.ssid);
                st_wifi_scan.ssid = NULL;
            }
            return 0;
        }

        l_ret = wpa_start_scan(&st_wifi_scan);
        if (0 != l_ret)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "[%s]uwifi:scan fail",__func__);
            continue;
        }

        dbg(D_INF, D_UWIFI_CTRL, "%s wpa_start_scan end, scan_done_flag=%d",__func__,scan_done_flag);

        uint32_t _n = 700;
        while(SCAN_START == scan_done_flag )
        {
            if( --_n )
            {
                if(SCAN_DEINIT == scan_done_flag)
                    break;
                asr_rtos_delay_milliseconds(30);
            }
            else
            {
                dbg(D_ERR, D_UWIFI_CTRL, "%s SCAN_TIME_OUT\r\n",__func__);
                asr_wlan_set_scan_status(SCAN_TIME_OUT);
                if(st_wifi_scan.ssid) {
                    asr_rtos_free(st_wifi_scan.ssid);
                    st_wifi_scan.ssid = NULL;
                }
                return -1;
            }
        }

        if (pst_config_info->connect_ap_info.ssid_len)
            pst_dst_scan_info = asr_wlan_get_scan_ap(pst_config_info->connect_ap_info.ssid, pst_config_info->connect_ap_info.ssid_len,
                                                   pst_config_info->connect_ap_info.security);
        if (NULL == pst_dst_scan_info)
        {
            if(!is_zero_mac_addr((const uint8_t *)pst_config_info->connect_ap_info.bssid))
                pst_dst_scan_info = asr_wlan_get_scan_ap_by_bssid(pst_config_info->connect_ap_info.bssid, pst_config_info->connect_ap_info.security);

            if (NULL == pst_dst_scan_info)
            {
                if(scan_try == UWIFI_AOS_SCAN_MAX_TRIES)
                    dbg(D_ERR, D_UWIFI_CTRL, "uwifi:no ap");

                continue;
            }

        }
        break;
    }

    if(st_wifi_scan.ssid) {
        asr_rtos_free(st_wifi_scan.ssid);
        st_wifi_scan.ssid = NULL;
    }

    if (NULL == pst_dst_scan_info)
    {
        asr_wlan_set_disconnect_status(WLAN_STA_MODE_NO_AP_FOUND);
        return -1;
    }
    /* update config info according to scan result */
    wpa_update_config_info(pst_config_info, pst_dst_scan_info);
    return 0;
}

/*************************************************
Function: uwifi_wpa_handle_disconnect
Description: handle disconnect event: stop dhcp and decide if we need a scan immediately.
Input:  uint32_t ul_param: the value of the pointer to config_info,
        uint32_t ul_center_freq: center frequency of the former connected ap.
Output: nothing
Return: void
Author: BaoshunFang
Date:   2017.12.18
Others:
*************************************************/
void uwifi_schedule_reconnect(struct config_info *pst_config_info)
{
    int l_ret = -1;

    dbg(D_INF, D_UWIFI_CTRL, "%s: sched reconn en_autoconnect=%d , interval=%d\r\n",__func__,pst_config_info->en_autoconnect,pst_config_info->wifi_retry_interval);

    if (AUTOCONNECT_DISABLED == pst_config_info->en_autoconnect)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "sched reconn autoconn disable");
        return;
    }

    if (NULL != pst_config_info->wifi_retry_timer.handle)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "sched reconn dup, retry timer deinit");
        asr_rtos_deinit_timer(&(pst_config_info->wifi_retry_timer));
        //return;
    }

    l_ret = asr_rtos_init_timer(&(pst_config_info->wifi_retry_timer), pst_config_info->wifi_retry_interval,
        uwifi_wifi_retry_timer_callback, (void *)pst_config_info);
    if (0 != l_ret)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "sched reconn init timer fail");
        return;
    }

    l_ret = asr_rtos_start_timer(&(pst_config_info->wifi_retry_timer));
    if (0 != l_ret)
    {
        /* deinit the wifi_retry timer */
        if (NULL != pst_config_info->wifi_retry_timer.handle)
            asr_rtos_deinit_timer(&(pst_config_info->wifi_retry_timer));

        dbg(D_ERR, D_UWIFI_CTRL, "sched reconn start timer fail");
        return;
    }
}

static bool uwifi_check_is_connecting(void)
{
    struct asr_vif *asr_vif = NULL;
    struct asr_hw *asr_hw = uwifi_get_asr_hw();


    if(asr_hw->sta_vif_idx != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];

        if (asr_vif->sta.connInfo.wifiState == WIFI_CONNECTING
            || asr_vif->sta.connInfo.wifiState == WIFI_ASSOCIATED) {
            return true;
        }
    }

    return false;
}

//int autoconnect_tries = 0;
static void uwifi_wpa_handle_autoconnect(void *arg)
{
    int32_t             l_ret           = -1;
    struct config_info *pst_config_info = (struct config_info*)arg;

    dbg(D_DBG, D_UWIFI_CTRL, "<<<%s", __func__);
    if (pst_config_info->en_autoconnect == AUTOCONNECT_DISABLED)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "handle autoconn ,autoconn disable");
        return;
    }

    if (uwifi_check_is_connecting()) {
        return;
    }

#ifdef _DOUBLE_85_RF_EN_
    asr_rf_tmmt_recal();
#endif

    /* auto connect time MAX, so start a scan first,then connect */

    dbg(D_INF, D_UWIFI_CTRL, "%s autoconnect_tries=%d,count=%d,max retry times=%d\r\n", __func__,pst_config_info->autoconnect_tries,
                   pst_config_info->autoconnect_count,
           pst_config_info->wifi_retry_times );

    if (pst_config_info->autoconnect_tries >= UWIFI_AUTO_DIRECTCONNECT_MAX_TRIES)
    {
        pst_config_info->en_autoconnect = AUTOCONNECT_SCAN_CONNECT;    /* scan 1st */
        pst_config_info->connect_ap_info.channel = 0;
        //support connect same ssid/password AP when origin connect ap disappear
        //if (NULL != pst_config_info->wifi_retry_timer.handle)
        //    asr_rtos_deinit_timer(&(pst_config_info->wifi_retry_timer));

        if(0 == pst_config_info->connect_ap_info.pwd_len)
        {
            pst_config_info->connect_ap_info.security = WIFI_ENCRYP_OPEN;
        }
        else
            pst_config_info->connect_ap_info.security = WIFI_ENCRYP_AUTO;
            //support some ap send error security(not match ap web set)

        memset(pst_config_info->connect_ap_info.bssid, 0, MAC_ADDR_LEN);
    }

    if (pst_config_info->autoconnect_count > 10) {
        pst_config_info->wifi_retry_interval = 1000*60; // set timer_interv to 60s when reconnect count more than 10.
    } else {
        if(pst_config_info->wifi_retry_interval < AUTOCONNECT_INTERVAL_MID)
            pst_config_info->wifi_retry_interval += AUTOCONNECT_INIT_STEP;
        else if(pst_config_info->wifi_retry_interval < AUTOCONNECT_INTERVAL_MAX)
            pst_config_info->wifi_retry_interval += AUTOCONNECT_MAX_STEP;
    }

    if((pst_config_info->wifi_retry_times) && (pst_config_info->autoconnect_tries >= pst_config_info->wifi_retry_times))
    {
    asr_wlan_set_disconnect_status(WLAN_STA_MODE_CONN_RETRY_MAX);
        dbg(D_ERR, D_UWIFI_CTRL,"autoconnect retry max %u times!\n", pst_config_info->wifi_retry_times);

        if (NULL != pst_config_info->wifi_retry_timer.handle)
            asr_rtos_deinit_timer(&(pst_config_info->wifi_retry_timer));

        return;
    }

    pst_config_info->autoconnect_tries++;
    pst_config_info->autoconnect_count++;  // record reconnect count


    if (pst_config_info->en_autoconnect != AUTOCONNECT_DIRECT_CONNECT)
    {
        /* scan first */
        l_ret = uwifi_scan_for_further_info(pst_config_info);
        if (0 != l_ret)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "handle autoconn scan fail");
            uwifi_schedule_reconnect(pst_config_info);
            return;
        }

    }

    dbg(D_DBG, D_UWIFI_CTRL, "%s:handle \r\n",__func__);
    l_ret = wpa_start_connect_adv(pst_config_info);
    if (0 != l_ret)
    {
        //TBD: exception handling
        dbg(D_ERR, D_UWIFI_CTRL, "handle autoconn connect fail");

        pst_config_info->en_autoconnect = AUTOCONNECT_SCAN_CONNECT;    /* scan 1st */
        pst_config_info->connect_ap_info.channel = 0;
        pst_config_info->connect_ap_info.security = WIFI_ENCRYP_AUTO;
        //support connect same ssid/password AP when origin connect ap disappear
        memset(pst_config_info->connect_ap_info.bssid, 0, MAC_ADDR_LEN);
        /* scan first */
        uwifi_scan_for_further_info(pst_config_info);
        uwifi_schedule_reconnect(pst_config_info);

        return;
    }
    return;
}
#endif

#ifdef CFG_STATION_SUPPORT
static void uwifi_wpa_handle_sa_query_to(void *arg)
{
    struct asr_vif *param = (struct asr_vif *)arg;
    struct config_info *pst_config_info = &(param->sta.configInfo);

    sm_stop_sa_query(pst_config_info);

    // Process to the disconnection
    uwifi_msg_at2u_disconnect(DISCONNECT_REASON_STA_SA_QUERY_FAIL);

}
#endif

#ifdef CFG_ADD_API
void uwifi_ap_channel_change(uint8_t chan_num)
{
    struct asr_hw  *asr_hw    = uwifi_get_asr_hw();
    struct asr_vif *asr_vif   = asr_hw->vif_table[asr_hw->ap_vif_idx];

    struct ieee80211_supported_band    *pst_band    = NULL;
    enum nl80211_channel_type           chan_type   = NL80211_CHAN_NO_HT;

    struct ieee80211_channel *pst_chan = &asr_vif->ap.st_chan;
    memset(pst_chan, 0, sizeof(struct ieee80211_channel));

    pst_band = asr_hw->wiphy->bands[IEEE80211_BAND_2GHZ];
    if (pst_band == NULL)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "ap set chan no support");
        return;
    }

    if (chan_num < 5)
    {
        /* channel only support 40+ */
        chan_type = NL80211_CHAN_HT40PLUS;
    }
    else if (chan_num > 9)
    {
        /* channel only support 40- */
        chan_type = NL80211_CHAN_HT40MINUS;
    }
    else
    {
        /* channel both support 40+ and 40-, 1st set 40+, then set 40- */
        if (NL80211_CHAN_HT40PLUS == asr_vif->ap.chan_type)
        {
            chan_type = NL80211_CHAN_HT40MINUS;
        }
        else
        {
            chan_type = NL80211_CHAN_HT40PLUS;
        }
    }

    if ((asr_vif->ap.chan_num == chan_num) && (asr_vif->ap.chan_type == chan_type))
    {
        dbg(D_ERR, D_UWIFI_CTRL, "ap set same chan");
        return;
    }

    pst_chan->band = IEEE80211_BAND_2GHZ;
    pst_chan->center_freq = pst_band->channels[chan_num-1].center_freq;
    pst_chan->max_power   = pst_band->channels[chan_num-1].max_power;
    asr_send_set_channel(asr_hw, pst_chan, chan_type);
    /* save the channel info to VIF */
    asr_vif->ap.chan_num   = chan_num;
    asr_vif->ap.chan_type = chan_type;
    dbg(D_ERR, D_UWIFI_CTRL, "set chan_num:%d chan_type:%d", asr_vif->ap.chan_num, asr_vif->ap.chan_type);
    return;
}
#endif

#ifdef CFG_SNIFFER_SUPPORT
/*************************************************
Function: uwifi_sniffer_handle_channel_change
Description: handle channel_change event
Input:  uint8_t channel: channel num needed to set.
Output: nothing
Return: void
Author: BaoshunFang
Date:   2017.1.8
Others:
*************************************************/
void uwifi_sniffer_handle_channel_change(uint8_t chan_num)
{
    struct asr_hw  *asr_hw    = uwifi_get_asr_hw();
    struct asr_vif *asr_vif   = asr_hw->vif_table[asr_hw->monitor_vif_idx];

    struct ieee80211_supported_band    *pst_band    = NULL;
    enum nl80211_channel_type           chan_type   = NL80211_CHAN_NO_HT;

    struct ieee80211_channel *pst_chan = &asr_vif->sniffer.st_chan;
    memset(pst_chan, 0, sizeof(struct ieee80211_channel));

    pst_band = asr_hw->wiphy->bands[IEEE80211_BAND_2GHZ];
    if (pst_band == NULL)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "sniffer set chan no support");
        return;
    }

    if (chan_num < 5)
    {
        /* channel only support 40+ */
        chan_type = NL80211_CHAN_HT40PLUS;
    }
    else if (chan_num > 9)
    {
        /* channel only support 40- */
        chan_type = NL80211_CHAN_HT40MINUS;
    }
    else
    {
        /* channel both support 40+ and 40-, 1st set 40+, then set 40- */
        if (NL80211_CHAN_HT40PLUS == asr_vif->sniffer.chan_type)
        {
            chan_type = NL80211_CHAN_HT40MINUS;
        }
        else
        {
            chan_type = NL80211_CHAN_HT40PLUS;
        }
    }

    if ((asr_vif->sniffer.chan_num == chan_num) && (asr_vif->sniffer.chan_type == chan_type))
    {
        dbg(D_ERR, D_UWIFI_CTRL, "sniffer set same chan");
        return;
    }

    pst_chan->band = IEEE80211_BAND_2GHZ;
    pst_chan->center_freq = pst_band->channels[chan_num-1].center_freq;
    pst_chan->max_power   = pst_band->channels[chan_num-1].max_power;
    asr_send_set_channel(asr_hw, pst_chan, chan_type);
    /* save the channel info to VIF */
    asr_vif->sniffer.chan_num   = chan_num;
    asr_vif->sniffer.chan_type = chan_type;
    dbg(D_ERR, D_UWIFI_CTRL, "set chan_num:%d chan_type:%d", asr_vif->sniffer.chan_num, asr_vif->sniffer.chan_type);
    return;
}

/*************************************************
Function: uwifi_sniffer_handle_cb_register
Description: handle cb_register event
Input:  uint32_t p_fn:pointer to the callback function.
Output: nothing
Return: void
Author: BaoshunFang
Date:   2018.1.9
Others:
*************************************************/
int uwifi_sniffer_handle_cb_register(uint32_t fn)
{
    sniffer_rx_cb = (monitor_cb_t)fn;

    return 0;
}

/*************************************************
Function: uwifi_sniffer_handle_mgmt_cb_register
Description: handle cb_register event
Input:  uint32_t p_fn:pointer to the callback function.
Output: nothing
Return: void
Author: BaoshunFang
Date:   2018.10.29
Others:
*************************************************/
void uwifi_sniffer_handle_mgmt_cb_register(uint32_t fn)
{
    sniffer_rx_mgmt_cb = (monitor_cb_t)fn;
}

/*************************************************
Function: uwifi_sniffer_handle_filter_change
Description: handle filter_change event
Input:  uint32_t type:filter type
Output: nothing
Return: void
Author: BaoshunFang
Date:   2017.1.9
Others:
*************************************************/
void uwifi_sniffer_handle_filter_change(uint32_t type)
{
    struct asr_hw  *asr_hw    = uwifi_get_asr_hw();

    asr_send_set_filter(asr_hw, type);
}
#endif

static void uwifi_wpa_eapol_handle(uint8_t *data,uint32_t len, uint32_t mode)
{
    struct recv_frame rec_frame;

    rec_frame.u.hdr.len = len;
    rec_frame.u.hdr.rx_data = data;

    wifi_recv_indicatepkt_eapol(&rec_frame, mode);
    asr_rtos_free(rec_frame.u.hdr.rx_data);
    rec_frame.u.hdr.rx_data = NULL;
}

#ifdef CONFIG_SME
static void uwifi_sme_evt_handle(struct asr_vif *asr_vif,void *req, uint32_t mode)
{
    dbg(D_ERR,D_UWIFI_CTRL,"%s %d",__func__, mode);

    switch (mode)
    {
        case SME_AUTH:
            if (asr_cfg80211_auth(asr_vif,(struct cfg80211_auth_request *)req) < 0){
                dbg(D_ERR, D_LWIFI, "SME: Authentication request to the driver failed");
            }
            break;
        case SME_ASSOC:
            if (asr_cfg80211_assoc(asr_vif,(struct cfg80211_assoc_request *)req) < 0){
                dbg(D_ERR, D_LWIFI, "SME: Association request to the driver failed");
            };   //sme_associate
            break;
        case SME_DEAUTH:
            if (asr_cfg80211_deauth(asr_vif,(struct cfg80211_deauth_request *)req) < 0){
                dbg(D_ERR, D_LWIFI, "SME: deauth request to the driver failed");
            };
            break;
        default :
            dbg(D_ERR,D_UWIFI_CTRL,"%s:mode not support:%d",__func__, mode);
            break;
    }
}
#endif

#ifdef CFG_STATION_SUPPORT
static void uwifi_wpa_info_clear(struct asr_vif *asr_vif, uint32_t security)
{
    struct config_info *config_info = &(asr_vif->sta.configInfo);

    /* In case, wifi close and internal generated disconnect
    occur at the same time. delete key @uwifi_wpa_clear_wifi_status */
    if (AUTOCONNECT_DISABLED == config_info->en_autoconnect)
        return;

    if (security == WIFI_ENCRYP_WEP)
    {
        uwifi_cfg80211_ops.del_key(asr_vif, 0, false, NULL);
    }
    else
    {
        wpas_psk_deinit(g_pwpa_priv[COEX_MODE_STA]);
        wpas_priv_free(WLAN_OPMODE_INFRA);
    }
}
#endif

int uwifi_check_legal_channel(int channel)
{
    struct asr_hw *pst_asr_hw = uwifi_get_asr_hw();
    int n_channels;

    if (pst_asr_hw == NULL)
    return -1;

    n_channels = wpa_get_channels_from_country_code(pst_asr_hw);
    if(channel <= n_channels)
        return 0;
    else
        return -1;
}

#ifdef CFG_STATION_SUPPORT
static void uwifi_handle_aos_sta_start(asr_wlan_init_type_t* pst_network_init_info)
{
    int l_ret = -1;
    struct config_info *pst_config_info;

#if NX_STA_AP_COEX
    if ((STA_MODE == current_iftype) || (STA_AP_MODE == current_iftype))
#else
    /* 1. start sta wifi */
    if (STA_MODE == current_iftype)
#endif
    {
        dbg(D_ERR, D_UWIFI_CTRL, "STA has been created,jump to scan\n");
    }
#if NX_STA_AP_COEX
    else if ((SAP_MODE == current_iftype) || (0xFF == current_iftype))
#else
    else if (0xFF == current_iftype)
#endif
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s uwifi_start_mode\r\n",__func__);
        l_ret = uwifi_start_mode(STA_MODE, NULL);
        if (0 != l_ret)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "STA create failed\n");
            return;
        }
    }
    else
    {
        dbg(D_ERR, D_UWIFI_CTRL, "current mode is %d,cannot start sta\n", (int)current_iftype);
        return;
    }

#if NX_STA_AP_COEX
    if (current_iftype == 0xFF)
    {
        current_iftype = STA_MODE;
    }
    else
    {
        if (current_iftype == SAP_MODE)
        {
            current_iftype = STA_AP_MODE;
        }
    }
#else
    current_iftype = STA_MODE;
#endif

    pst_config_info = uwifi_get_sta_config_info();
    if (NULL == pst_config_info)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "uwifi_handle_aos_sta_start:pst_config_info is null!");
        return;
    }
    /* if wifi associated, clear connect status first */
    l_ret = uwifi_wpa_clear_wifi_status(false);
    if (0 != l_ret)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "connect is ongoing, return\n");
        return;
    }
    /* 2.save config info to vif */
    l_ret = asr_save_sta_config_info(pst_config_info, pst_network_init_info);
    if (0 != l_ret)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "uwifi_handle_aos_sta_start:save config info failed!");
        return;
    }

    /* 3. check whether legal channel */
    l_ret = uwifi_check_legal_channel(pst_config_info->connect_ap_info.channel);
    if ((0 != l_ret) && (0 != pst_config_info->connect_ap_info.channel))
    {
        dbg(D_ERR, D_UWIFI_CTRL, "uwifi_handle_aos_sta_start:illegal channel %d!",pst_config_info->connect_ap_info.channel);
        return;
    }

    extern uint8_t is_scan_adv;
    is_scan_adv = OPEN_STA_SCAN;// asr_wlan_open scan
    /* 4. start scan */
    l_ret = uwifi_scan_for_further_info(pst_config_info);
    if (0 != l_ret)
    {
        uwifi_schedule_reconnect(pst_config_info);
        dbg(D_ERR, D_UWIFI_CTRL, "%s:scan failed!",__func__);

        if (AUTOCONNECT_DISABLED == pst_config_info->en_autoconnect) {
            dbg(D_ERR, D_UWIFI_CTRL,"%s: disable autoconnect, set retry max err here!\n",__func__);
            asr_wlan_set_disconnect_status(WLAN_STA_MODE_CONN_RETRY_MAX);
        }
        return;
    }

    /*  5. start connect after received scan results */
    dbg(D_ERR, D_UWIFI_CTRL, "%s:sta \r\n",__func__);
    l_ret = wpa_start_connect_adv(pst_config_info);
    if (0 != l_ret)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "uwifi_handle_aos_sta_start:connect failed!");

        pst_config_info->en_autoconnect = AUTOCONNECT_SCAN_CONNECT;    /* scan 1st */
        pst_config_info->connect_ap_info.channel = 0;
        pst_config_info->connect_ap_info.security = WIFI_ENCRYP_AUTO;
        //support connect same ssid/password AP when origin connect ap disappear
        memset(pst_config_info->connect_ap_info.bssid, 0, MAC_ADDR_LEN);
        /* scan first */
        uwifi_scan_for_further_info(pst_config_info);
        uwifi_schedule_reconnect(pst_config_info);
    }
}
#endif

#if 0

struct asr_hw* uwifi_get_asr_hw(void)
{
    return sp_asr_hw;
}


enum nl80211_iftype uwifi_get_work_mode(void)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    if(NULL == asr_hw)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s:asr_hw is NULL",__func__);
        return NL80211_IFTYPE_UNSPECIFIED;
    }

    struct asr_vif *vif = container_of(asr_hw,struct asr_vif,asr_hw);
    return vif->iftype;
}
#endif

#ifdef CFG_ENCRYPT_SUPPORT
void uwifi_wpa_handshake_start(uint32_t mode)
{
    wifi_event_cb(WLAN_EVENT_4WAY_HANDSHAKE, NULL);
    wifi_indicate_handshake_start(mode);
}

void uwifi_sta_handshake_done(void)
{
    wpas_sta_4way_handshake_done_handler();
    wifi_event_cb(WLAN_EVENT_4WAY_HANDSHAKE_DONE, NULL);
}

void uwifi_wpa_resend_eapol(uint32_t arg)
{
    wpas_send_timeout_callback((struct sta_info *)arg);
}

void uwifi_wpa_complete_to(uint32_t arg)
{
    wpas_complete_timeout_callback((struct sta_info *)arg);
}
#endif

#ifdef CFG_STATION_SUPPORT
void uwifi_handle_disconnect(enum reasoncode reason)
{
    dbg(D_ERR,D_UWIFI_CTRL,"%s %d",__func__, reason);

    switch (reason)
    {
        case DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT:
    case DISCONNECT_REASON_STA_SA_QUERY_FAIL:
            asr_wlan_set_disconnect_status(WLAN_STA_MODE_PASSWORD_ERR);
            uwifi_wpa_clear_wifi_status(true);
            break;
        case DISCONNECT_REASON_DHCP_TIMEOUT:
            asr_wlan_set_disconnect_status(WLAN_STA_MODE_DHCP_FAIL);
            uwifi_wpa_clear_wifi_status(true);
            break;
        case DISCONNECT_REASON_INTERFACE_INVALID:
        case DISCONNECT_REASON_STA_KRACK:
        case DISCONNECT_REASON_STA_4TH_EAPOL_TX_FAIL:
            uwifi_wpa_clear_wifi_status(true);
            break;
        case DISCONNECT_REASON_USER_GENERATE:
            asr_api_ctrl_param.disconnect_result = uwifi_wpa_clear_wifi_status(false);
            asr_rtos_set_semaphore(&asr_api_ctrl_param.disconnect_semaphore);
            break;
        default :
            dbg(D_ERR,D_UWIFI_CTRL,"%s:reason not support:%d",__func__, reason);
            break;
    }
}
#endif

#ifdef CFG_SOFTAP_SUPPORT
void uwifi_hostapd_softap_reenable()
{
    struct uwifi_ap_cfg_param *ap_cfg_param = &g_ap_cfg_info;
    struct asr_hw *asr_hw = uwifi_get_asr_hw();

    if(++ap_cfg_param->re_enable == 3)
    {
        struct softap_info_t* softap_info = &g_softap_info;

        memset(softap_info, 0, sizeof(struct softap_info_t));

        softap_info->ssid_len   = ap_cfg_param->ssid_len;
        memcpy(softap_info->ssid, ap_cfg_param->ssid, softap_info->ssid_len);
        softap_info->pwd_len    = ap_cfg_param->psk_len;

        if (softap_info->pwd_len != 0)
        {
            softap_info->password   = asr_rtos_malloc(softap_info->pwd_len);

            if(softap_info->password == NULL)
            {
                dbg(D_ERR, D_UWIFI_CTRL, "wlan_open:password malloc failed");
                asr_rtos_free(softap_info->password);
                return;
            }
            memcpy(softap_info->password, ap_cfg_param->psk, softap_info->pwd_len);
        }

        if(asr_hw->vif_table[asr_hw->sta_vif_idx]->sta.ap->center_freq == 2484)
        {
            softap_info->chan = 14;
        }
        else
        {
            softap_info->chan = (asr_hw->vif_table[asr_hw->sta_vif_idx]->sta.ap->center_freq-2407)/5;
        }
        softap_info->interval   = ap_cfg_param->beacon_interval;
        softap_info->hide       = ap_cfg_param->hidden_ssid;

        uwifi_msg_at2u_open_mode(SAP_MODE, softap_info);

        ap_cfg_param->re_enable = 0;
    }
    else
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s %d",__func__,ap_cfg_param->re_enable);
    }
}

void uwifi_ap_peer_sta_ip_update(uint32_t param1, uint8_t* sta_mac_addr)
{
    int8_t index= 0;
    uint8_t found = 0;
    uint32_t *client_ip = (uint32_t *)param1;
    struct asr_vif *cur;
    struct ap_user_info *ap_info;
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    struct ethernetif *ethernetif = &g_ethernetif[COEX_MODE_AP];
    struct uwifi_ap_peer_info *peer_cur, *peer_tmp;

    if(!asr_hw || ethernetif == NULL)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "wifi is closed");
        return;
    }

    list_for_each_entry(cur, &asr_hw->vifs, list)
    {
        if(!memcmp(cur->wlan_mac_add.mac_addr, ethernetif->ethaddr->mac_addr, MAC_ADDR_LEN))
        {
            found++;
            break;
        }
    }
    if(!found)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "not find matched vifs");
        return;
    }

    found = 0;
    ap_info = &g_ap_user_info;

    for(index = 0 ;index < AP_MAX_ASSOC_NUM; index++)
    {
        if (!memcmp(ap_info->sta_table[index].mac_addr, sta_mac_addr, MAC_ADDR_LEN))
        {
          found ++;
          break;
        }
    }

    if(!found)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "mac address cannot found");
    }
    else if (index < AP_MAX_ASSOC_NUM)
    {
        if(ap_info->sta_table[index].ip_addr != *client_ip)
        {
            ap_info->sta_table[index].ip_addr = *client_ip;
            wifi_event_cb(WLAN_EVENT_AP_PEER_UP, (void*)&(ap_info->sta_table[index]));
        }
    }

    found = 0;
    list_for_each_entry_safe(peer_cur, peer_tmp, &cur->ap.peer_sta_list, list)
    {
        if (!memcmp(peer_cur->peer_addr, sta_mac_addr, MAC_ADDR_LEN))
        {
            found ++;
            break;
        }
    }
    if(found)
        peer_cur->connect_status = CONNECT_GET_IP_DONE;

}
#endif

//extern asr_mutex_t g_only_scan_mutex;
extern int g_sdio_reset_flag;
//extern int only_scan_done_flag;
void uwifi_main_process_msg(struct uwifi_task_msg_t *msg)
{
    //struct asr_hw *asr_hw = uwifi_get_asr_hw();
    int l_ret = -1;
    switch(msg->id)
    {
        case AT2D_OPEN_MODE:{                         //STA mode, AP mode, sta&ap coexist mode, sniffer mode, test mode
            //while (g_sta_close_status.is_waiting_close) {//waitting until the last opened sta's close operation is done
            //    asr_rtos_delay_milliseconds(3);
            //}
            dbg(D_ERR, D_UWIFI_CTRL, "%s uwifi_start_mode\r\n",__func__);
            asr_api_ctrl_param.open_result = uwifi_start_mode((enum open_mode)msg->param1,(struct softap_info_t*)msg->param2);

            //if(asr_api_ctrl_param.open_result ==0)
                asr_rtos_set_semaphore(&asr_api_ctrl_param.open_semaphore);
            break;
            }
#ifdef CFG_SNIFFER_SUPPORT
        case AT2D_OPEN_MONITOR_MODE:                         //sniffer mode
            asr_api_ctrl_param.open_monitor_result = uwifi_start_sniffer_mode();
            asr_rtos_set_semaphore(&asr_api_ctrl_param.open_monitor_semaphore);
            break;
#endif
#ifdef CFG_STATION_SUPPORT
        case AT2D_SET_PS_MODE:
            asr_send_set_ps_mode(msg->param1);
            break;
        case AT2D_SCAN_REQ:                          //"AT+SCAN" to fw task
            // if(SCAN_START!=scan_done_flag)
            //     asr_wlan_set_scan_status(SCAN_START);
            if(g_sdio_reset_flag)
            {
                dbg(D_ERR, D_UWIFI_CTRL, "%s uwifi:sdio reset\r\n",__func__);
            }
            l_ret = wpa_start_scan((struct wifi_scan_t*)msg->param1);
            if (0 != l_ret)
            {
                dbg(D_ERR, D_UWIFI_CTRL, "%s uwifi:scan fail\r\n",__func__);
                asr_wlan_set_disconnect_status(WLAN_STA_MODE_SCAN_FAIL);
            }

            asr_rtos_free((struct wifi_scan_t*)msg->param1);
            msg->param1 = 0;
            if (msg->param2)
            {
                asr_rtos_free((void *)msg->param2);
                msg->param2 = 0;
            }
            break;
        case AT2D_CONNECT_REQ:                       //"AT+JOIN" to fw task
            wpa_start_connect((struct wifi_conn_t*)msg->param1);
            break;
        case AT2D_CONNECT_ADV_REQ:
            asr_api_ctrl_param.connect_adv_result = wpa_start_connect_adv((struct config_info*)msg->param1);
            asr_rtos_set_semaphore(&asr_api_ctrl_param.connect_adv_semaphore);
            break;
        case AT2D_DISCONNECT:
            uwifi_handle_disconnect((enum reasoncode)msg->param1);
            break;
        case AT2D_TX_POWER:                           //AT cmd write "AT+TX_POWER"
            wpa_set_power((uint8_t)msg->param1, msg->param2);
            break;
#endif
        case LWIP2U_IP_GOT:
            wifi_event_cb(WLAN_EVENT_IP_GOT, (void*)(msg->param1));
            {
                #ifdef CFG_SOFTAP_SUPPORT
                struct uwifi_ap_cfg_param *ap_cfg_param = uwifi_hostapd_get_softap_cfg_param_t();
                if(ap_cfg_param->re_enable)
                {
                    uwifi_hostapd_softap_reenable();
                }
                #endif
            }
            break;
        case LWIP2U_STA_IP_UPDATE:
            uwifi_ap_peer_sta_ip_update(msg->param1,(uint8_t *)msg->param2);
            break;
        case AT2D_CLOSE:
            uwifi_close(msg->param1);
            current_iftype = 0xFF;
            break;
#ifdef CFG_SNIFFER_SUPPORT
        case AT2D_SET_FILTER_TYPE:
            uwifi_sniffer_handle_filter_change(msg->param1);
            break;
        case AT2D_SNIFFER_REGISTER_CB:
            uwifi_sniffer_handle_cb_register(msg->param1);
            break;
#endif
#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
        case AT2D_CUS_FRAME:
            uwifi_tx_dev_custom_mgmtframe((uint8_t *)msg->param1,msg->param2);
            break;
#endif
#ifdef CFG_SOFTAP_SUPPORT
        case RXU2U_ASSOCRSP_TX_CFM:
            uwifi_hostapd_handle_assocrsp_tx_comp_msg(msg->param1, msg->param2);
            break;
        case RXU2U_DEAUTH_TX_CFM:
            uwifi_hostapd_handle_deauth_tx_comp_msg(msg->param1, msg->param2);
            break;
        case RXU2U_RX_DEAUTH:
            uwifi_hostapd_handle_deauth_msg(msg->param1);
            break;
#endif
#ifdef CFG_ENCRYPT_SUPPORT
        case RXU2U_EAPOL_PACKET_PROCESS:
            uwifi_wpa_eapol_handle((uint8_t*)msg->param1,msg->param2,msg->param3);
            break;
#ifdef CONFIG_SME
        case RXU2U_SME_PACKET_PROCESS:
            uwifi_sme_evt_handle((struct asr_vif *)msg->param1,(void*)msg->param2,msg->param3);
            break;
#endif
#endif
#ifdef CFG_ENCRYPT_SUPPORT
        case RXU2U_WPA_INFO_CLEAR:
            uwifi_wpa_info_clear((struct asr_vif *)msg->param1, msg->param2);
            break;
#endif
#ifdef CFG_STATION_SUPPORT
        case AOS2U_STA_START:
            if (g_ap_close_status.is_waiting_cfm)
            {
                /* ap close is ongoing, save the msg, handle it after ap closed */
                g_ap_close_status.p_sta_start_info = (void *)msg->param1;
                break;
            }
            uwifi_handle_aos_sta_start((asr_wlan_init_type_t *)msg->param1);
            asr_rtos_free((asr_wlan_init_type_t *)msg->param1);
            msg->param1 = 0;
            g_ap_close_status.p_sta_start_info = 0;
            dbg(D_ERR,D_UWIFI_CTRL,"%s:uwifi_handle_aos_sta_start over \r\n",__func__);
            break;
#endif
#ifdef CFG_ENCRYPT_SUPPORT
        case RXU2U_HANDSHAKE_START:
            uwifi_wpa_handshake_start(msg->param1);
            break;
        case RXU2U_STA_HANDSHAKE_DONE:
            uwifi_sta_handshake_done();
            break;
        case TIMER2U_EAPOL_RESEND:
            uwifi_wpa_resend_eapol(msg->param1);
            break;
        case TIMER2U_WPA_COMPLETE_TO:
            uwifi_wpa_complete_to(msg->param1);
            break;
#endif
#ifdef CFG_STATION_SUPPORT
        case TIMER2U_AUTOCONNECT_STA:
            uwifi_wpa_handle_autoconnect((void*)msg->param1);
            break;
#endif
#ifdef APM_AGING_SUPPORT
        case F2D_MSG_DEAUTH:
            uwifi_hostapd_aging_deauth_peer_sta((uint8_t *)msg->param1);
            break;
#endif
#ifdef CFG_SOFTAP_SUPPORT
        case TIMER2U_SOFTAP_AUTH_TO:
            uwifi_hostapd_handle_auth_to((void*)msg->param1);
            break;
        case TIMER2U_SOFTAP_ASSOC_TO:
            uwifi_hostapd_handle_assoc_to((void*)msg->param1);
            break;
#endif
#ifdef CFG_STATION_SUPPORT
        case TIMER2U_STA_SA_QUERY_TO:
            uwifi_wpa_handle_sa_query_to((void*)msg->param1);
            break;
#endif
        default:
            break;
    }
}

static int dev_recover_link_up(struct asr_hw *asr_hw, struct asr_vif *asr_vif )
{
    int ret = -1;
    struct ipc_e2a_msg *msg = NULL;
    struct sm_disconnect_ind *disc_ind = NULL;
    struct mm_add_if_cfm add_if_cfm;

    dbg(D_ERR,D_UWIFI_CTRL,"%s: asr_vif=%p,up=%d\n", __func__, asr_vif, asr_vif ? asr_vif->up : 0);

    if (asr_vif && asr_vif->up) {

        dbg(D_ERR,D_UWIFI_CTRL,"%s: recover up\n", __func__);

        // Start the FW
        if ((ret = asr_send_start(asr_hw))) {
            dbg(D_ERR,D_UWIFI_CTRL,"%s: asr_send_start fail\n", __func__);
            return ret;
        }

        /* Device is now started */
        asr_set_bit(ASR_DEV_STARTED, &asr_hw->phy_flags);

        /* Forward the information to the LMAC,
         *     p2p value not used in FMAC configuration, iftype is sufficient */
        if ((ret = asr_send_add_if(asr_hw, asr_vif->wlan_mac_add.mac_addr, ASR_VIF_TYPE(asr_vif), false, &add_if_cfm))) {
            dbg(D_ERR,D_UWIFI_CTRL,"%s: asr_send_add_if fail\n", __func__);
            return ret;
        }
        if (add_if_cfm.status != 0) {
            dbg(D_ERR,D_UWIFI_CTRL,"%s: asr_send_add_if fail status=%d\n", __func__, add_if_cfm.status);
            return ret;
        }

#ifdef CFG_STATION_SUPPORT
        /* Abort scan request on the vif */
        #if 0
        if (asr_hw->scan_request && asr_hw->scan_request->dev == asr_vif->wdev.netdev) {
            cfg80211_scan_done(asr_hw->scan_request, true);
            asr_hw->scan_request = NULL;
        }
        #endif

        /* Abort scan request on the vif */
        if (asr_hw->scan_request) {
            int i;
            //cfg80211_scan_done(asr_hw->scan_request, true);
            for(i=0;i<asr_hw->scan_request->n_channels;i++)
            {
                asr_rtos_free(asr_hw->scan_request->channels[i]);
                asr_hw->scan_request->channels[i] = NULL;
            }
            asr_rtos_free(asr_hw->scan_request);
            asr_hw->scan_request = NULL;
        }

        //sync disconnect to wpa
        dbg(D_ERR,D_UWIFI_CTRL,"%s: sync disconnect to wpa\n", __func__);

        //fix kernel net core status not sync to driver bug
        asr_set_bit(ASR_DEV_STA_CONNECTED, &asr_vif->dev_flags);

        msg = (struct ipc_e2a_msg *)asr_rtos_malloc(sizeof(struct ipc_e2a_msg));
        if (msg) {
            disc_ind = (struct sm_disconnect_ind *)(msg->param);
            memset(disc_ind, 0, sizeof(struct sm_disconnect_ind));
            disc_ind->vif_idx = asr_vif->vif_index;
            disc_ind->reason_code = 1;
            disc_ind->ft_over_ds = 0;
            asr_rx_sm_disconnect_ind(asr_hw, NULL, msg);
            asr_rtos_free(msg);
        }
#endif
    }

    return 0;
}

int asr_set_wifi_reset()
{
    asr_wlan_gpio_power(0);
    asr_wlan_gpio_power(1);
    return 0;
}

static void asr_set_vers(struct asr_hw *asr_hw)
{
    uint32_t vers = asr_hw->version_cfm.version_lmac;

    snprintf(asr_hw->wiphy->fw_version,
         sizeof(asr_hw->wiphy->fw_version), "%d.%d.%d.%d",
         (vers & (0xff << 24)) >> 24, (vers & (0xff << 16)) >> 16,
         (vers & (0xff << 8)) >> 8, (vers & (0xff << 0)) >> 0);
}

/// asr sdio wifi reset.
void dev_restart_work_func(struct asr_vif *asr_vif)
{
    struct asr_hw *asr_hw = NULL;
    int ret = 0;
    struct asr_vif *asr_vif_tmp = NULL;

    if (NULL == asr_vif)
        return;

    asr_hw = asr_vif->asr_hw;

    if (NULL == asr_hw)
        return;

    //if (g_asr_para.dev_driver_remove) {
    //    dbg(D_ERR,D_UWIFI_CTRL,"%s: cancel in dev_driver_remove\n", __func__);
    //    return;
    //}

    //delay for other thread run end
    asr_rtos_delay_milliseconds(100);

    dbg(D_ERR,D_UWIFI_CTRL,"%s: reset wifi,drv_flags=0X%X\n",__func__, (unsigned int)asr_vif->dev_flags);

    //if (asr_hw->vif_index < asr_hw->vif_max_num + asr_hw->sta_max_num) {
    //    asr_vif = asr_hw->vif_table[asr_hw->vif_index];
    //}

    if (!asr_test_and_set_bit(ASR_DEV_RESTARTING, &asr_hw->phy_flags))
    {
        #ifdef RX_TRIGGER_TIMER_ENABLE
        if ((asr_hw->rx_trigger_timer.handle != NULL) && asr_rtos_is_timer_running(&(asr_hw->rx_trigger_timer)))
            asr_rtos_stop_timer(&(asr_hw->rx_trigger_timer));
        #endif
        if ((asr_hw->rx_thread_timer.handle != NULL) && asr_rtos_is_timer_running(&(asr_hw->rx_thread_timer)))
            asr_rtos_stop_timer(&(asr_hw->rx_thread_timer));
        asr_clear_bit(ASR_DEV_STARTED, &asr_hw->phy_flags);
        asr_clear_bit(ASR_DEV_CLOSEING, &asr_vif->dev_flags);
        asr_clear_bit(ASR_DEV_SCANING, &asr_vif->dev_flags);
        asr_clear_bit(ASR_DEV_STA_CONNECTING, &asr_vif->dev_flags);
        asr_clear_bit(ASR_DEV_STA_CONNECTED, &asr_vif->dev_flags);
        asr_clear_bit(ASR_DEV_STA_DHCPEND, &asr_vif->dev_flags);
        asr_clear_bit(ASR_DEV_STA_DISCONNECTING, &asr_vif->dev_flags);


        //deinit sdio
                asr_sdio_disable_int(HOST_INT_DNLD_EN | HOST_INT_UPLD_EN);

        //disable rx dataready.
        asr_wlan_irq_unsubscribe();

        //sdio_claim_host(asr_plat->func);
        ret = asr_sdio_release_irq();
        //sdio_release_host(asr_plat->func);

        if (ret) {
            dbg(D_ERR,D_UWIFI_CTRL,"ERROR: release sdio irq fail %d.\n", ret);
        } else {
            dbg(D_ERR,D_UWIFI_CTRL,"ASR: release sdio irq success.\n");
        }

dev_reset_retry:
        //wifi module reset
        if (asr_set_wifi_reset()) {
            dbg(D_ERR,D_UWIFI_CTRL,"%s: asr_set_wifi_reset fail\n", __func__);
            goto dev_restart_end;
        }

        dbg(D_ERR,D_UWIFI_CTRL,"ASR: sdio init start.\n");

        #if 0
        if (!asr_test_and_set_bit(ASR_DEV_INITED, &asr_hw->phy_flags)) {
            dbg(D_ERR,D_UWIFI_CTRL,"%s: handler in nornal process\n", __func__);

            g_asr_para.dev_reset_start = false;
            asr_sdio_detect_change();

            goto dev_restart_end;
        }

        g_asr_para.dev_reset_start = true;
        #endif

        ret = asr_sdio_detect_change();

        if (ret) {
            dbg(D_ERR,D_UWIFI_CTRL,"%s: asr_sdio_detect_change fail\n", __func__);
            goto dev_restart_end;
        }

        //sdio_set_drvdata(asr_plat->func, asr_hw);

        //int sdio
        if (asr_sdio_init_config(asr_hw)) {
            dbg(D_ERR,D_UWIFI_CTRL,"%s: asr_sdio_init_config fail\n", __func__);
            goto dev_restart_end;
        }
        //disable host interrupt first, using polling method when download fw
        asr_sdio_disable_int(HOST_INT_DNLD_EN | HOST_INT_UPLD_EN);

        if (asr_download_fw(asr_hw)) {
            dbg(D_ERR,D_UWIFI_CTRL,"%s: asr_plat_lmac_load fail\n", __func__);
            asr_rtos_delay_milliseconds(3000);
            goto dev_reset_retry;
        }

        //sdio_claim_host(asr_plat->func);
        /*set block size SDIO_BLOCK_SIZE */
        asr_sdio_set_block_size(SDIO_BLOCK_SIZE);
        // enable data ready.
        //asr_wlan_irq_enable();

        ret = asr_sdio_claim_irq();
        //sdio_release_host(asr_plat->func);
        if (ret) {
            dbg(D_ERR,D_UWIFI_CTRL,"%s: asr_sdio_claim_irq fail\n", __func__);
            goto dev_restart_end;
        }
        //enable sdio host interrupt
        ret = asr_sdio_enable_int(HOST_INT_DNLD_EN | HOST_INT_UPLD_EN);
        if (!ret) {
            dbg(D_ERR,D_UWIFI_CTRL,"%s: asr_sdio_enable_int fail\n", __func__);
            goto dev_restart_end;
        }

        if (asr_xmit_opt) {
            asr_rtos_lock_mutex(&asr_hw->tx_lock);
            asr_drop_tx_vif_skb(asr_hw,NULL);
            list_for_each_entry(asr_vif_tmp, &asr_hw->vifs, list) {
                asr_vif_tmp->tx_skb_cnt = 0;
            }
            asr_rtos_unlock_mutex(&asr_hw->tx_lock);
        } else {
            asr_rtos_lock_mutex(&asr_hw->tx_agg_env_lock);
            asr_tx_agg_buf_reset(asr_hw);
            list_for_each_entry(asr_vif_tmp, &asr_hw->vifs, list) {
                asr_vif_tmp->txring_bytes = 0;
            }
            asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);
        }

        //clear cmd crash flag
        asr_hw->cmd_mgr.state = 0;
        asr_hw->tx_data_cur_idx = 1;
        asr_hw->rx_data_cur_idx = 2;
        asr_hw->tx_use_bitmap = 1;    //for msg tx

        //g_asr_para.sdio_send_times = 0;

        asr_rtos_delay_milliseconds(20);

        /* Reset FW */
        if ((ret = asr_send_reset(asr_hw))) {
            dbg(D_ERR,D_UWIFI_CTRL,"%s: asr_send_reset fail\n", __func__);
            goto dev_restart_end;
        }
        if ((ret = asr_send_version_req(asr_hw, &asr_hw->version_cfm))) {
            dbg(D_ERR,D_UWIFI_CTRL,"%s: asr_send_version_req fail\n", __func__);
            goto dev_restart_end;
        }

        asr_set_vers(asr_hw);

        if ((ret = asr_send_fw_softversion_req(asr_hw, &asr_hw->fw_softversion_cfm))) {
            dbg(D_ERR,D_UWIFI_CTRL,"%s: asr_send_fw_softversion_req fail\n", __func__);
            goto dev_restart_end;
        }
        dbg(D_ERR,D_UWIFI_CTRL,"%s: fw version (%s).\n", __func__, asr_hw->fw_softversion_cfm.fw_softversion);

        /* Set parameters to firmware */
        asr_send_me_config_req(asr_hw);

        /* Set channel parameters to firmware (must be done after WiPHY registration) */
        asr_send_me_chan_config_req(asr_hw);

        if (dev_recover_link_up(asr_hw, asr_vif)) {
            dbg(D_ERR,D_UWIFI_CTRL,"%s: dev_recover_link_up fail\n", __func__);
            goto dev_restart_end;
        }

        dbg(D_ERR,D_UWIFI_CTRL,"%s: dev restart successful!\n", __func__);
    }


dev_restart_end:

    asr_clear_bit(ASR_DEV_RESTARTING, &asr_hw->phy_flags);

    dbg(D_ERR,D_UWIFI_CTRL,"%s: dev restart end!,drv_flags=0x%x\n", __func__,(unsigned int)asr_vif->dev_flags);

    return;
}

#ifndef THREADX_STM32
#ifdef CFG_SOFTAP_SUPPORT
uint8_t* uwifi_hostapd_set_ie_ssid(uint8_t *pbuf, uint32_t index, uint32_t len, uint8_t *source, enum nl80211_hidden_ssid hide)
{
    *pbuf++ = (uint8_t)index;
    if (NL80211_HIDDEN_SSID_ZERO_CONTENTS == hide)
    {
        *pbuf++ = (uint8_t)len;
        memset(pbuf, 0, len);
        pbuf += len;
    }
    else if (hide)
    {
        *pbuf++ = 0;
    }
    else
    {
        *pbuf++ = (uint8_t)len;
        memcpy(pbuf, source, len);
        pbuf += len;
    }

    return (pbuf);
}

static void uwifi_hostapd_prepare_beacon(struct asr_vif *asr_vif,
                    struct uwifi_ap_cfg_param *cfg_param, uint8_t *bcn_frame, struct cfg80211_beacon_data *beacon)
{
    uint8_t *pframe;
    uint8_t addr1[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint8_t rate_len = 0;
    uint8_t tim_ie_len = 4;
    uint8_t tim_ie[4] = {0};

    beacon->head = bcn_frame;
    pframe = uwifi_init_mgmtframe_header((struct ieee80211_hdr*)bcn_frame, addr1, asr_vif->wlan_mac_add.mac_addr,
                                        asr_vif->wlan_mac_add.mac_addr, IEEE80211_STYPE_BEACON);

    //timestamp will be inserted by hardware
    pframe += 8;

    // beacon interval: 2 bytes
    *(pframe++) = cfg_param->beacon_interval & 0xff;
    *(pframe++) = (cfg_param->beacon_interval & 0xff00) >> 8;

    // capability info: 2 bytes
    *(pframe++) = cfg_param->capabality & 0xff;
    *(pframe++) = (cfg_param->capabality & 0xff00) >> 8;

    // SSID
    pframe = uwifi_hostapd_set_ie_ssid(pframe, WLAN_EID_SSID, cfg_param->ssid_len, cfg_param->ssid, cfg_param->hidden_ssid);

    // supported rates...
    rate_len = uwifi_hostapd_get_rateset_len(cfg_param->supported_rates);
    pframe = uwifi_hostapd_set_ie(pframe, WLAN_EID_SUPP_RATES, ((rate_len > 8)? 8: rate_len), cfg_param->supported_rates);

    // DS parameter set
    pframe = uwifi_hostapd_set_ie(pframe, WLAN_EID_DS_PARAMS, 1, &cfg_param->dsss_config);

    // EXTERNDED SUPPORTED RATE
    if (rate_len > 8)
    {
        pframe = uwifi_hostapd_set_ie(pframe, WLAN_EID_EXT_SUPP_RATES, (rate_len - 8), (cfg_param->supported_rates + 8));
    }
    beacon->head_len = pframe - beacon->head;

    //TIM
    tim_ie[1] = cfg_param->dtim_period;
    pframe = uwifi_hostapd_set_ie(pframe, WLAN_EID_TIM, tim_ie_len, tim_ie);

    beacon->tail = pframe;

    //rsn
    if(cfg_param->encryption_protocol == WIFI_ENCRYP_WPA2)
    {
        pframe = uwifi_hostapd_set_rsn_info_element(pframe);
    }

    //wmm
    if(cfg_param->wmm_enabled)
    {
        pframe = uwifi_hostapd_set_wmm_info_element(pframe, cfg_param);
    }

    if(cfg_param->ht_enabled)
    {
        //ht capability
        pframe = uwifi_hostapd_set_ht_capa_info_element(pframe);

        //ht operation element
        pframe = uwifi_hostapd_set_ht_opration_info_element(pframe);
    }

    if(cfg_param->he_enabled)
    {
        pframe = uwifi_hostapd_set_he_capa_info_element(pframe);
        pframe = uwifi_hostapd_set_he_operation_info_element(pframe);
    }


    beacon->tail_len = pframe - beacon->tail;
}

static uint32_t uwifi_hostapd_prepare_init_probe_rsp(struct asr_vif *asr_vif,
                                                            struct uwifi_ap_cfg_param *cfg_param, uint8_t *probe_rsp)
{
    uint8_t *pframe;
    uint8_t addr1[MAC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t rate_len = 0;

    pframe = uwifi_init_mgmtframe_header((struct ieee80211_hdr*)probe_rsp, addr1, asr_vif->wlan_mac_add.mac_addr,
                                        asr_vif->wlan_mac_add.mac_addr, IEEE80211_STYPE_PROBE_RESP);

    //timestamp will be inserted by hardware
    pframe += 8;

    // beacon interval: 2 bytes
    *(pframe++) = cfg_param->beacon_interval & 0xff;
    *(pframe++) = (cfg_param->beacon_interval & 0xff00) >> 8;

    // capability info: 2 bytes
    *(pframe++) = cfg_param->capabality & 0xff;
    *(pframe++) = (cfg_param->capabality & 0xff00) >> 8;

    // SSID
    pframe = uwifi_hostapd_set_ie(pframe, WLAN_EID_SSID, cfg_param->ssid_len, cfg_param->ssid);

    // supported rates...
    rate_len = uwifi_hostapd_get_rateset_len(cfg_param->supported_rates);
    pframe = uwifi_hostapd_set_ie(pframe, WLAN_EID_SUPP_RATES, ((rate_len > 8)? 8: rate_len), cfg_param->supported_rates);

    // DS parameter set
    pframe = uwifi_hostapd_set_ie(pframe, WLAN_EID_DS_PARAMS, 1, &(cfg_param->dsss_config));

    // EXTERNDED SUPPORTED RATE
    if (rate_len > 8)
    {
        pframe = uwifi_hostapd_set_ie(pframe, WLAN_EID_EXT_SUPP_RATES, (rate_len - 8), (cfg_param->supported_rates + 8));
    }

    //rsn
    if(cfg_param->encryption_protocol == WIFI_ENCRYP_WPA2)
    {
        pframe = uwifi_hostapd_set_rsn_info_element(pframe);
    }

    //wmm
    if(cfg_param->wmm_enabled)
    {
        pframe = uwifi_hostapd_set_wmm_info_element(pframe, cfg_param);
    }

    if(cfg_param->ht_enabled)
    {
        //ht capability
        pframe = uwifi_hostapd_set_ht_capa_info_element(pframe);

        //ht operation element
        pframe = uwifi_hostapd_set_ht_opration_info_element(pframe);
    }

    if(cfg_param->he_enabled)
    {
        //he capa
        pframe = uwifi_hostapd_set_he_capa_info_element(pframe);
        //he op
        pframe = uwifi_hostapd_set_he_operation_info_element(pframe);
    }

    return (pframe-probe_rsp);
}


static void uwifi_hostapd_prepare_start_info(struct uwifi_ap_cfg_param *cfg_param,
                                           struct cfg80211_ap_settings *drv_settings, struct softap_info_t *softap_info)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    if (softap_info->interval)
    {
        cfg_param->beacon_interval                = softap_info->interval;
        drv_settings->beacon_interval             = softap_info->interval;
    }
    else
        drv_settings->beacon_interval             = cfg_param->beacon_interval;

    drv_settings->dtim_period                     = cfg_param->dtim_period;
    drv_settings->chandef.center_freq1            = cfg_param->chan_center_freq1;
    drv_settings->chandef.center_freq2            = cfg_param->chan_center_freq2;
    drv_settings->chandef.width                   = cfg_param->chan_width;
    drv_settings->chandef.chan                    = &cfg_param->chan_basic;
    cfg_param->hidden_ssid                        = softap_info->hide;
    //when sta mode open firstly and connected, channel parameter of softap should follow that of sta mode.
    if((asr_hw->sta_vif_idx != 0xff)
        &&(asr_hw->vif_table[asr_hw->sta_vif_idx]->sta.connInfo.wifiState == WIFI_CONNECTED))
    {
        drv_settings->chandef.chan->center_freq = asr_hw->vif_table[asr_hw->sta_vif_idx]->sta.ap->center_freq;
        drv_settings->chandef.center_freq1 = asr_hw->vif_table[asr_hw->sta_vif_idx]->sta.ap->center_freq;
        cfg_param->chan_center_freq1 = drv_settings->chandef.center_freq1;
        cfg_param->chan_basic.center_freq = drv_settings->chandef.chan->center_freq;
        if(drv_settings->chandef.chan->center_freq == 2484)
        {
            cfg_param->dsss_config = 14;
        }
        else
        {
            cfg_param->dsss_config = (drv_settings->chandef.chan->center_freq - 2407)/5;
        }
    }

    drv_settings->crypto.control_port             = cfg_param->crypto_control_port;
    drv_settings->crypto.control_port_ethertype   = cfg_param->crypto_control_port_ethertype;
    drv_settings->crypto.control_port_no_encrypt  = cfg_param->crypto_control_port_no_encrypt;
    drv_settings->crypto.cipher_group             = cfg_param->crypto_cipher_group;

    if(softap_info->pwd_len == 0)  //open mode
    {
        cfg_param->encryption_protocol = WIFI_ENCRYP_OPEN;
        memset(cfg_param->psk, 0, WIFI_PSK_LEN+1);
        cfg_param->psk_len = 0;
        cfg_param->capabality = 0x0401; //G mode short slot time/privacy disable/ess type network
    }
    else  //wpa2(CCMP), not support wpa now
    {
        cfg_param->encryption_protocol = WIFI_ENCRYP_WPA2;
        memset(cfg_param->psk, 0, WIFI_PSK_LEN+1);
        memcpy(cfg_param->psk, softap_info->password, softap_info->pwd_len);
        cfg_param->psk_len = softap_info->pwd_len;
        cfg_param->capabality = 0x0411;  //G mode short slot time/privacy enable/ess type network
    }
    memset(cfg_param->ssid, 0, WIFI_SSID_LEN+1);
    memcpy(cfg_param->ssid, softap_info->ssid, softap_info->ssid_len);
    cfg_param->ssid_len = softap_info->ssid_len;
}
static void uwifi_hostapd_set_edca_params(struct asr_vif *asr_vif, struct uwifi_ap_cfg_param *ap_cfg_info)
{
   struct ieee80211_txq_params txq_params = {0};
   uint8_t i = 0;

   for(i = 0; i < AC_MAX; i++)
   {
       txq_params.ac = AC_MAX -1 - i;  //enum which is include AC_MAX is different from enum nl80211_ac
       txq_params.aifs = ap_cfg_info->edca_param[i].aifsn;
       txq_params.cwmin = ap_cfg_info->edca_param[i].cwmin;
       txq_params.cwmax= ap_cfg_info->edca_param[i].cwmax;
       txq_params.txop= ap_cfg_info->edca_param[i].txop;
       if(uwifi_cfg80211_ops.set_txq_params(asr_vif, &txq_params))
       {
           dbg(D_ERR, D_UWIFI_CTRL, "send edca param error, ac:%d",i);
       }
   }
}

static void uwifi_hostapd_update_start_ap_user_info(struct asr_vif *asr_vif)
{
   struct ap_user_info *ap_info = &g_ap_user_info;
   struct uwifi_ap_cfg_param *ap_cfg_info = &g_ap_cfg_info;

   memcpy(ap_info->bssid, asr_vif->wlan_mac_add.mac_addr, MAC_ADDR_LEN);
   ap_info->ssid_len = ap_cfg_info->ssid_len;
   memcpy(ap_info->ssid, ap_cfg_info->ssid, ap_info->ssid_len);
   ap_info->band = ap_cfg_info->chan_basic.band;
   ap_info->chan_width = ap_cfg_info->chan_width;
   ap_info->connect_peer_num = 0;
   ap_info->center_freq = ap_cfg_info->chan_basic.center_freq;
}

extern struct net_device_ops uwifi_netdev_ops;
uint8_t g_bcn_frame[BCN_FRAME_LEN];
uint8_t g_probersp_frame[PROBE_RSP_FRAME_LEN];
int uwifi_open_ap(struct asr_hw *asr_hw, struct softap_info_t *softap_info)
{
    struct cfg80211_ap_settings ap_setting;
    int error = 0;
    struct uwifi_ap_cfg_param *ap_cfg_param = &g_ap_cfg_info;
    struct asr_vif *asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    struct wiphy *wiphy     = asr_hw->wiphy;
    struct ieee80211_supported_band *band = wiphy->bands[IEEE80211_BAND_2GHZ];
    struct wpa_psk psk;

    memset(&ap_setting, 0, sizeof(struct cfg80211_ap_settings));
    memset(&psk, 0, sizeof(struct wpa_psk));
    dbg(D_ERR, D_UWIFI_CTRL, "%s enter...", __func__);
    if((softap_info->ssid_len > MAX_SSID_LEN) || (softap_info->pwd_len > MAX_PASSWORD_LEN))
    {
        dbg(D_CRT, D_UWIFI_CTRL, "open softap param error: ssid_len:%d, pwd_len:%d",
                                                                           softap_info->ssid_len, softap_info->pwd_len);
        error = -1;
        goto end;
    }

    if(ap_cfg_param->ap_isolate)
    {
        asr_vif->ap.flags |= ASR_AP_ISOLATE;
    }
    else
    {
        asr_vif->ap.flags &= ~ASR_AP_ISOLATE;
    }

    asr_vif->ap.connect_sta_enable = 0;
    asr_vif->ap.stop_ap_cb = 0;
    asr_vif->ap.bcn_frame = g_bcn_frame;
    asr_vif->ap.probe_rsp = g_probersp_frame;
    INIT_LIST_HEAD(&asr_vif->ap.peer_sta_list);

    #ifdef CFG_ADD_API
    asr_vif->ap.black_state = 0;
    asr_vif->ap.white_state = 0;

    INIT_LIST_HEAD(&asr_vif->ap.peer_black_list);
    INIT_LIST_HEAD(&asr_vif->ap.peer_white_list);
    asr_vif->ap.chan_num = softap_info->chan;
    #endif

    //under concurrecy scenario, this param may be changed, so set default value when open ap again.
    ap_cfg_param->chan_center_freq1        = AP_DEFAULT_CHAN_FREQ;
    ap_cfg_param->chan_basic.center_freq   = AP_DEFAULT_CHAN_FREQ;
    ap_cfg_param->crypto_control_port_ethertype = wlan_htons(ETH_P_PAE);

    // he enable set.
    ap_cfg_param->he_enabled = softap_info->he_enable;

    error = uwifi_check_legal_channel(softap_info->chan);
    if (0 != error)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "illegal channel %d, use default 6",softap_info->chan);
        softap_info->chan = 6;
    }

    if ((softap_info->chan > 0) && (softap_info->chan < 15))
    {
        ap_cfg_param->chan_center_freq1 = band->channels[softap_info->chan-1].center_freq;
        ap_cfg_param->chan_basic.center_freq = band->channels[softap_info->chan-1].center_freq;
        ap_cfg_param->dsss_config = softap_info->chan;
    }

    uwifi_hostapd_prepare_start_info(ap_cfg_param, &ap_setting, softap_info);
    dbg(D_ERR, D_UWIFI_CTRL, "%s uwifi_hostapd_prepare_start_info", __func__);
    uwifi_hostapd_prepare_beacon(asr_vif, ap_cfg_param, asr_vif->ap.bcn_frame, &(ap_setting.beacon));
    dbg(D_ERR, D_UWIFI_CTRL, "%s uwifi_hostapd_prepare_beacon", __func__);
    asr_vif->ap.probe_rsp_len  = uwifi_hostapd_prepare_init_probe_rsp(asr_vif, ap_cfg_param, asr_vif->ap.probe_rsp);
    dbg(D_ERR, D_UWIFI_CTRL, "%s uwifi_hostapd_prepare_init_probe_rsp", __func__);
    error = uwifi_cfg80211_ops.start_ap(asr_vif, &ap_setting);

    if(error)
    {
        dbg(D_CRT, D_UWIFI_CTRL, "start ap error in uwifi_cfg_80211_ops");
        goto end;

    }

    uwifi_hostapd_update_start_ap_user_info(asr_vif);
    dbg(D_ERR, D_UWIFI_CTRL, "%s uwifi_hostapd_update_start_ap_user_info", __func__);
    #ifdef CFG_ENCRYPT_SUPPORT
    if(ap_cfg_param->encryption_protocol != WIFI_ENCRYP_OPEN)
    {
        memcpy(psk.macaddr, asr_vif->wlan_mac_add.mac_addr, MAC_ADDR_LEN);
        psk.ssid_len = ap_cfg_param->ssid_len;
        memcpy(psk.ssid, ap_cfg_param->ssid, ap_cfg_param->ssid_len);
        psk.pwd_len = ap_cfg_param->psk_len;
        memcpy(psk.password, ap_cfg_param->psk, ap_cfg_param->psk_len);
        psk.encrpy_protocol = ap_cfg_param->encryption_protocol;
        psk.cipher_group = cipher_suite_to_wpa_cipher(ap_cfg_param->crypto_cipher_group);
        psk.ciphers_pairwise = cipher_suite_to_wpa_cipher(ap_cfg_param->crypto_cipher_pairwise);
        dbg(D_ERR, D_UWIFI_CTRL, "%s wpa_connect_init start ", __func__);
        wpa_connect_init(&psk, WLAN_OPMODE_MASTER);
        dbg(D_ERR, D_UWIFI_CTRL, "%s wpa_connect_init end", __func__);
    }
    #endif
#if 0
    else
    {
        uwifi_hostapd_add_group_key_open(asr_hw);
    }
#endif

    //set edca parameter(vo/vi/bk/be)
    uwifi_hostapd_set_edca_params(asr_vif, ap_cfg_param);
    dbg(D_ERR, D_UWIFI_CTRL, "%s uwifi_hostapd_set_edca_params", __func__);
    asr_vif->ap.connect_sta_enable = 1;

end:

    if (softap_info->password != NULL)
    {
        asr_rtos_free(softap_info->password);
        softap_info->password = NULL;
    }

    if (error)
    {
        memset(softap_info, 0, sizeof(struct softap_info_t));
        if (uwifi_netdev_ops.ndo_stop(asr_hw, asr_vif))
        {
            dbg(D_ERR,D_UWIFI_CTRL,"ap open cfg and para reset failed");
        }
    }

    dbg(D_ERR, D_UWIFI_CTRL, "%s exit", __func__);
    return error;
}

static void uwifi_close_ap_cb(void)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    struct asr_vif *asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    struct ap_user_info *ap_info = &g_ap_user_info;
    struct uwifi_ap_cfg_param *ap_cfg_param = &g_ap_cfg_info;
    struct softap_info_t* softap_info = &g_softap_info;

    if(ap_info->connect_peer_num <= 0)
    {
        dbg(D_CRT, D_UWIFI_CTRL, "hostapd deauth all sta, close ap now");
        asr_vif->ap.stop_ap_cb = 0;
        asr_vif->ap.aid_bitmap = 0;
#ifdef CFG_ENCRYPT_SUPPORT
        if(ap_cfg_param->encryption_protocol != WIFI_ENCRYP_OPEN)
        {
            wpas_psk_deinit(g_pwpa_priv[COEX_MODE_AP]);
        }
#endif
        //stop ap
        uwifi_cfg80211_ops.stop_ap(asr_vif);
#ifdef ALIOS_SUPPORT
        dhcps_stop();
#else
        #if LWIP_DHCPD
        dhcpd_stop();
        #else
        asr_dhcps_stop();
        #endif
#endif
        g_ap_close_status.is_waiting_cfm = 0;
        if (g_ap_close_status.p_sta_start_info)
            uwifi_msg_aos2u_sta_start(g_ap_close_status.p_sta_start_info);

        memset(&(asr_vif->ap), 0, sizeof(asr_vif->ap));

        memset(softap_info, 0, sizeof(struct softap_info_t));
        wifi_event_cb(WLAN_EVENT_AP_DOWN, NULL);
        asr_vif->ap.closing = false;

        if(ap_cfg_param->re_enable)
        {
            dbg(D_CRT, D_UWIFI_CTRL, "renable");
            uwifi_hostapd_softap_reenable();
        }

    }
}

void uwifi_close_ap(struct asr_hw *asr_hw)
{
    struct asr_vif *asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    struct uwifi_ap_peer_info *cur, *tmp;
    uint8_t found = 0;

    if(asr_vif != NULL)
    {
        g_ap_close_status.is_waiting_cfm = 1;
        asr_vif->ap.stop_ap_cb = uwifi_close_ap_cb;
        asr_vif->ap.connect_sta_enable = 0;
        asr_vif->ap.closing = true;
        int i;
        for (i = 0; i < AP_MAX_ASSOC_NUM; i++)
        {
            if (asr_vif->ap.peer_sta[i].auth_timer.handle)
            {
                asr_rtos_deinit_timer(&(asr_vif->ap.peer_sta[i].auth_timer));
            }
            if (asr_vif->ap.peer_sta[i].assoc_timer.handle)
            {
                asr_rtos_deinit_timer(&(asr_vif->ap.peer_sta[i].assoc_timer));
            }
        }

        list_for_each_entry_safe(cur, tmp, &asr_vif->ap.peer_sta_list, list)
        {
            uwifi_hostapd_deauth_peer_sta(asr_hw, cur->peer_addr);
            found++;
        }

        if(found == 0)
        {
            uwifi_close_ap_cb();
            #ifdef CFG_ENCRYPT_SUPPORT
            asr_wlan_clear_pmk();
            #endif
        }
    }
    else
    {
        dbg(D_ERR, D_UWIFI_CTRL, "closing vif is NULL");
    }
}
struct uwifi_ap_cfg_param* uwifi_hostapd_get_softap_cfg_param_t(void)
{
    return &g_ap_cfg_info;
}
#endif //CFG_SOFTAP_SUPPORT
#endif
