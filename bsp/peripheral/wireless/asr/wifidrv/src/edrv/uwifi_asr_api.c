/**
 ****************************************************************************************
 *
 * @file uwifi_asr_api.c
 *
 * @brief called by alios interfaces of wifi_port.c
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#include "uwifi_common.h"
//#include "asr_cm4.h"
//#include "asr_efuse.h"
#include "asr_dbg.h"
#include "asr_wlan_api.h"
#include "asr_wlan_api_aos.h"
#include "uwifi_ipc_host.h"
#include "uwifi_ops_adapter.h"
#include "uwifi_interface.h"
#include "uwifi_rx.h"
#ifdef LWIP
#include "lwipopts.h"
#include "netif.h"
//#include "lwip_comm_wifi.h"
#endif
#include "uwifi_msg.h"
#include "uwifi_wpa_api.h"
#include "uwifi_task.h"
#include "uwifi_notify.h"
#include "uwifi_msg_tx.h"
#include "wpas_psk.h"
#include "wpas_crypto.h"
#include "wpas_types.h"
//#include "sns_silib.h"
#ifdef ALIOS_SUPPORT
#include <k_api.h>
#endif
#ifdef CONFIG_IEEE80211W
#include "wpa_common.h"
#endif
#include "hostapd.h"

#include "uwifi_asr_api.h"

extern uint8_t wifi_ready;
uint32_t current_iftype = 0xFF;
//uint8_t g_user_close_wifi = 0;
//asr_mutex_t g_only_scan_mutex;
int g_sdio_reset_flag = 0;
extern int scan_done_flag;
//extern uint8_t wifi_psk_is_ture;
extern int32_t asr_malloc_cnt;
int g_asr_wlan_state_complete = 0;
uint8_t is_scan_adv = 0;


int asr_wlan_get_mac_address(uint8_t *puc_mac)
{
#if 0
    struct asr_hw  *pst_asr_hw    = uwifi_get_asr_hw();
    struct asr_vif *pst_sta_vif    = NULL;

    if (NULL == puc_mac)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "get_mac_address:input is null!");
        return -1;
    }

    if (NULL == pst_asr_hw)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "get_mac_address:pls open wifi first!");
        return -1;
    }

    if ((STA_MODE == current_iftype) && (pst_asr_hw->sta_vif_idx < NX_VIRT_DEV_MAX))
        pst_sta_vif = pst_asr_hw->vif_table[pst_asr_hw->sta_vif_idx];
    else if ((SAP_MODE == current_iftype) && (pst_asr_hw->ap_vif_idx < NX_VIRT_DEV_MAX))
        pst_sta_vif = pst_asr_hw->vif_table[pst_asr_hw->ap_vif_idx];
    else if ((SNIFFER_MODE == current_iftype) && (pst_asr_hw->monitor_vif_idx < NX_VIRT_DEV_MAX))
        pst_sta_vif = pst_asr_hw->vif_table[pst_asr_hw->monitor_vif_idx];

    if (NULL == pst_sta_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_get_mac_address:no vif!");
        return -1;
    }
    memcpy(puc_mac, pst_sta_vif->wlan_mac_add.mac_addr, ETH_ALEN);

    return 0;
#else
    if (NULL == puc_mac)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "get_mac_address:input is null!");
        return -1;
    }

    uwifi_get_mac(puc_mac);

    return 0;
#endif
}

int asr_save_sta_config_info(struct config_info *pst_config_info, asr_wlan_init_type_t* pst_network_init_info)
{
    dbg(D_INF, D_UWIFI_CTRL, "prepare to connect ap %s",pst_network_init_info->wifi_ssid);

    pst_config_info->wlan_ip_stat.dhcp = pst_network_init_info->dhcp_mode;

    if (pst_network_init_info->wifi_retry_interval != 0)
        pst_config_info->wifi_retry_interval = pst_network_init_info->wifi_retry_interval;
    else
        pst_config_info->wifi_retry_interval = AUTOCONNECT_INTERVAL_INIT;

    pst_config_info->wifi_retry_times = pst_network_init_info->wifi_retry_times;

    memcpy(pst_config_info->connect_ap_info.ssid, pst_network_init_info->wifi_ssid, MAX_SSID_LEN);
    memcpy(pst_config_info->connect_ap_info.pwd, pst_network_init_info->wifi_key, MAX_PASSWORD_LEN);
    memcpy(pst_config_info->wlan_ip_stat.ip, pst_network_init_info->local_ip_addr, 16);
    memcpy(pst_config_info->wlan_ip_stat.mask, pst_network_init_info->net_mask, 16);
    memcpy(pst_config_info->wlan_ip_stat.gate, pst_network_init_info->gateway_ip_addr, 16);
    memcpy(pst_config_info->wlan_ip_stat.dns, pst_network_init_info->dns_server_ip_addr, 16);
    pst_config_info->connect_ap_info.ssid_len = strlen((const char*)pst_config_info->connect_ap_info.ssid);
    if ((0 == pst_config_info->connect_ap_info.ssid_len) || (pst_config_info->connect_ap_info.ssid_len > MAX_SSID_LEN))
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_save_sta_config_info:ssid len error!");
        memset(pst_config_info, 0, sizeof(struct config_info));
        return -1;
    }

    pst_config_info->connect_ap_info.pwd_len = strlen((const char*)pst_config_info->connect_ap_info.pwd);
    if (pst_config_info->connect_ap_info.pwd_len> MAX_PASSWORD_LEN)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_save_sta_config_info:pwd len error!");
        memset(pst_config_info, 0, sizeof(struct config_info));
        return -1;
    }

    if((pst_network_init_info->channel > 0) && (pst_network_init_info->channel < 15))
    {
        pst_config_info->connect_ap_info.channel = pst_network_init_info->channel;
    }

    memcpy(pst_config_info->connect_ap_info.bssid, pst_network_init_info->mac_addr, MAC_ADDR_LEN);

    pst_config_info->en_autoconnect = pst_network_init_info->en_autoconnect ;  //AUTOCONNECT_SCAN_CONNECT;  /* scan 1st */
    if (0 == pst_config_info->connect_ap_info.pwd_len)
    {
        pst_config_info->connect_ap_info.security = WIFI_ENCRYP_OPEN;
    }
    else if (pst_network_init_info->security == 0)
    {
        pst_config_info->connect_ap_info.security = WIFI_ENCRYP_AUTO;
    }
    else
    {
        pst_config_info->connect_ap_info.security = pst_network_init_info->security;
    }

    return 0;
}

int asr_save_sta_config_info_adv(struct config_info *pst_config_info, asr_wlan_init_info_adv_st* pst_network_init_info)
{
    pst_config_info->wlan_ip_stat.dhcp = pst_network_init_info->dhcp_mode;
    pst_config_info->wifi_retry_interval = pst_network_init_info->wifi_retry_interval;
    pst_config_info->connect_ap_info.pwd_len = pst_network_init_info->key_len;
    pst_config_info->connect_ap_info.security = pst_network_init_info->ap_info.security;
    pst_config_info->connect_ap_info.ssid_len = strlen(pst_network_init_info->ap_info.ssid);
    if ((0 == pst_config_info->connect_ap_info.ssid_len) || (pst_config_info->connect_ap_info.ssid_len > MAX_SSID_LEN))
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_save_sta_config_info:ssid len error!");
        memset(pst_config_info, 0, sizeof(struct config_info));
        return -1;
    }

    memcpy(pst_config_info->connect_ap_info.ssid, pst_network_init_info->ap_info.ssid, MAX_SSID_LEN);
    memcpy(pst_config_info->connect_ap_info.pwd, pst_network_init_info->key, pst_network_init_info->key_len);
    memcpy(pst_config_info->wlan_ip_stat.ip, pst_network_init_info->local_ip_addr, 16);
    memcpy(pst_config_info->wlan_ip_stat.mask, pst_network_init_info->net_mask, 16);
    memcpy(pst_config_info->wlan_ip_stat.gate, pst_network_init_info->gateway_ip_addr, 16);
    memcpy(pst_config_info->wlan_ip_stat.dns, pst_network_init_info->dns_server_ip_addr, 16);

    return 0;
}

#ifdef CFG_STATION_SUPPORT
int asr_wlan_sta_start(asr_wlan_init_type_t* pst_network_init_info)
{
    asr_wlan_init_type_t* pst_network_info;
    pst_network_info = asr_rtos_malloc(sizeof(asr_wlan_init_type_t));
    memcpy(pst_network_info, pst_network_init_info, sizeof(asr_wlan_init_type_t));
    uwifi_msg_aos2u_sta_start((void*)pst_network_info);
    return 0;
}
#endif

#ifdef CFG_SOFTAP_SUPPORT
int asr_wlan_ap_start(asr_wlan_init_type_t* pst_network_init_info)
{
    struct softap_info_t* softap_info = &g_softap_info;

    #if !NX_STA_AP_COEX
    if(current_iftype != 0xFF)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "cur mode is %d, cannot start softap!",(int)current_iftype);
        return -1;
    }
    #endif

    if(softap_info->ssid_len != 0)
    {
        if(!memcmp(softap_info->ssid, pst_network_init_info->wifi_ssid, strlen(pst_network_init_info->wifi_ssid)))
        {
            dbg(D_ERR, D_UWIFI_CTRL, "error! open same ap twice");
            return -1;
        }
    }

    memset(softap_info, 0, sizeof(struct softap_info_t));

    softap_info->ssid_len   = strlen(pst_network_init_info->wifi_ssid);
    dbg(D_ERR, D_UWIFI_CTRL, "%s softap_info->ssid_len =%d\r\n",__func__,softap_info->ssid_len);

    memcpy(softap_info->ssid, pst_network_init_info->wifi_ssid, softap_info->ssid_len);
    dbg(D_ERR, D_UWIFI_CTRL, "%s softap_info->ssid =%s\r\n",__func__,softap_info->ssid );

    #ifdef CFG_ENCRYPT_SUPPORT
    softap_info->pwd_len    = strlen(pst_network_init_info->wifi_key);
    if (softap_info->pwd_len != 0)
    {
        softap_info->password   = asr_rtos_malloc(softap_info->pwd_len + 1);
        memset(softap_info->password,0,softap_info->pwd_len + 1);

        if(softap_info->password == NULL)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_open:password malloc failed");
            return -1;
        }
        memcpy(softap_info->password, pst_network_init_info->wifi_key, softap_info->pwd_len);
        dbg(D_ERR, D_UWIFI_CTRL, "%s softap_info->password =%s\r\n",__func__,softap_info->password );
    }
    #else
    softap_info->pwd_len = 0;
    dbg(D_ERR, D_UWIFI_CTRL, "%s: unsupport encrypt softap\r\n",__func__);
    #endif

    if(pst_network_init_info->channel)
    {
        softap_info->chan = (uint8_t)pst_network_init_info->channel;
    }
    else
    {
        softap_info->chan = 0;
    }

    softap_info->he_enable = (uint8_t)pst_network_init_info->he_enable;

    /* 1. start sta wifi */
    uwifi_msg_at2u_open_mode(SAP_MODE, softap_info);
    if (asr_rtos_get_semaphore(&asr_api_ctrl_param.open_semaphore, ASR_NEVER_TIMEOUT))
    {
        dbg(D_ERR,D_UWIFI_CTRL,"open cmd timed-out");
        return -1;
    }

    if(softap_info != NULL)
    {

        if (softap_info->password != NULL)
        {
            asr_rtos_free(softap_info->password);
            softap_info->password = NULL;
        }

        softap_info = NULL;
    }

    if(asr_api_ctrl_param.open_result)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_sta_start:start ap failed!");
        return -1;
    }

    current_iftype = SAP_MODE;

    set_wifi_ready_status(WIFI_IS_READY);

    return 0;
}
#endif

uint8_t user_pmk[PMK_LEN] = {0};
#ifdef CFG_ENCRYPT_SUPPORT

#if NX_SAE_PMK_CACHE
extern struct pmkid_caching user_pmkid_caching;
#endif
//this api is for softap mode, for midea
void asr_wlan_set_pmk(uint8_t *pmk)
{
    if(current_iftype != 0xFF)
    {
        //should set before open
        dbg(D_ERR, D_UWIFI_CTRL, "%s %d",__func__,(int)current_iftype);
        return;
    }

    memcpy(user_pmk, pmk, PMK_LEN);
}

//this api is for softap mode, for midea
int asr_wlan_get_pmk(uint8_t *ssid, uint8_t *password, uint8_t *pmk)
{
    uint8_t psk_temp[40] = {0};

    #if HW_PSK
    if(!encrypt_engine_init())
    {
        dbg(D_ERR, D_UWIFI_CTRL, "C310 init err");
        return 1;
    }
    #endif

    password_hash(password, ssid, strlen((const char*)ssid), psk_temp);
    memcpy(pmk, (const char*)psk_temp, PMK_LEN);

    #if HW_PSK
    encrypt_engine_deinit();
    #endif

    dbg(D_INF, D_UWIFI_CTRL,"%s %s",__func__,psk_temp);

    return 0;
}
#endif

void asr_wlan_clear_pmk(void)
{
    memset(user_pmk, 0, sizeof(user_pmk));

    #if NX_SAE_PMK_CACHE
    memset(&user_pmkid_caching, 0, sizeof(user_pmkid_caching));
    #endif
}

#ifdef CFG_SOFTAP_SUPPORT
asr_wlan_ip_stat_t g_dhcpd_ip_stat = {0};
extern const char *asr_get_wifi_version(void);
// asr_wlan_ap_open used for vendor wifi hal.
int asr_wlan_ap_open(asr_wlan_ap_init_t* init_info)
{
    struct softap_info_t* softap_info = &g_softap_info;

    if(current_iftype != 0xFF)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "cur mode is %d, cannot start softap!",(int)current_iftype);
        return -1;
    }

    if(softap_info->ssid_len != 0)
    {
        if(!memcmp(softap_info->ssid, init_info->ssid, strlen(init_info->ssid)))
        {
            dbg(D_ERR, D_UWIFI_CTRL, "error! open same ap twice");
            return -1;
        }
    }

    memset(softap_info, 0, sizeof(struct softap_info_t));

    dbg(D_INF, D_UWIFI_CTRL, "%s ssid1:%s pwd:%s chanel:%d interval:%d hide:%d ssid_len=%d\r\n", __func__,
                                  init_info->ssid, init_info->pwd, init_info->channel,
                                  init_info->interval, init_info->hide,strlen(init_info->ssid));

    softap_info->interval   = init_info->interval;
    softap_info->hide       = init_info->hide;
    softap_info->ssid_len   = strlen(init_info->ssid);

    memcpy(softap_info->ssid, init_info->ssid, softap_info->ssid_len);
    softap_info->ssid[softap_info->ssid_len] = 0;
    softap_info->pwd_len    = strlen(init_info->pwd);

    if (softap_info->pwd_len != 0)
    {
        softap_info->password   = asr_rtos_malloc(softap_info->pwd_len+1);

        if(softap_info->password == NULL)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_open:password malloc failed");

            return -1;
        }
        memcpy(softap_info->password, init_info->pwd, softap_info->pwd_len);
    }

    if(init_info->channel)
    {
        softap_info->chan = (uint8_t)init_info->channel;
    }
    else
    {
        softap_info->chan = 0;
    }

    /* 1. start softap */
    //dbg(D_ERR, D_UWIFI_CTRL, "%s softap_info->ssid_len=%d\r\n",__func__,softap_info->ssid_len);
    dbg(D_INF, D_UWIFI_CTRL, "%s ssid2:%s chanel:%d interval:%d hide:%d \r\n", __func__,softap_info->ssid, softap_info->chan, softap_info->interval, softap_info->hide);
    dbg(D_INF, D_UWIFI_CTRL, "%s ssid_len=%d len2=%d pwd_len=%d\r\n", __func__,strlen((char*)softap_info->ssid),softap_info->ssid_len,softap_info->pwd_len);
    if (softap_info->pwd_len != 0)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s  pwd:%s  \r\n", __func__,softap_info->password);
    }

    uwifi_msg_at2u_open_mode(SAP_MODE, softap_info);

    if (asr_rtos_get_semaphore(&asr_api_ctrl_param.open_semaphore, ASR_NEVER_TIMEOUT))
    {
        dbg(D_ERR,D_UWIFI_CTRL,"open cmd timed-out");
        return -1;
    }

    if(softap_info != NULL)
    {
        if (softap_info->password != NULL)
        {
            asr_rtos_free(softap_info->password);
            softap_info->password = NULL;
        }

        softap_info = NULL;
    }

    if(asr_api_ctrl_param.open_result)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_sta_start:start ap failed!");
        return -1;
    }

    current_iftype = SAP_MODE;

    set_wifi_ready_status(WIFI_IS_READY);

    return 0;
}
#endif

//extern uint8_t wifi_connect_err;
extern uint8_t wifi_conect_fail_code;
extern uint8_t wifi_softap_fail_code;
extern struct asr_hw* uwifi_get_asr_hw(void);
int asr_wlan_open(asr_wlan_init_type_t* init_info)
{
    int l_ret = -1;

    if (NULL == init_info)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_open:input is null!");
        return -1;
    }

    switch (init_info->wifi_mode)
    {
        #ifdef CFG_STATION_SUPPORT
        case STA:
            //wifi_connect_err = 0;
            //uint32_t _n = 2000;// 10 s
            if(strlen((const char*)init_info->wifi_ssid) || !is_zero_mac_addr((const uint8_t *)init_info->mac_addr)) {
                l_ret = asr_wlan_sta_start(init_info);
            } else
            {
                #if !NX_STA_AP_COEX
                if(current_iftype != 0xFF && current_iftype != STA_MODE)
                {
                    dbg(D_ERR, D_UWIFI_CTRL, "cur mode is %d, cannot start sta!",(int)current_iftype);
                    return -1;
                }
                #endif
                uwifi_msg_at2u_open_mode(STA_MODE, 0);

                #if NX_STA_AP_COEX
                if (current_iftype == SAP_MODE)
                {
                    current_iftype = STA_AP_MODE;
                }
                else {
                    current_iftype = STA_MODE;
                }
                #else
                current_iftype = STA_MODE;
                #endif

                l_ret = 0;

            }
            break;
        #endif
        #ifdef CFG_SOFTAP_SUPPORT
        case SOFTAP:

            #if !NX_STA_AP_COEX
            if((current_iftype == STA_MODE) && (init_info->wifi_mode == SOFTAP))
            {
                dbg(D_ERR, D_UWIFI_CTRL, "sta_ap_not_coexist cur mode is %d, cannot start ap!",(int)current_iftype);
                return -1;
            }
            #endif

            if (current_iftype == SNIFFER_MODE)
            {
                dbg(D_ERR, D_UWIFI_CTRL, "cur mode is sniffer, cannot start ap!");
                return -1;
            }

            //config ip gw netmask if setted by user
            memcpy(g_dhcpd_ip_stat.ip, init_info->local_ip_addr, 16);
            memcpy(g_dhcpd_ip_stat.gate, init_info->gateway_ip_addr, 16);
            memcpy(g_dhcpd_ip_stat.mask, init_info->net_mask, 16);
            memcpy(g_dhcpd_ip_stat.dns, init_info->dns_server_ip_addr, 16);
            dbg(D_INF, D_UWIFI_CTRL, "softap open ip %s, %s, %s",g_dhcpd_ip_stat.ip, g_dhcpd_ip_stat.gate, g_dhcpd_ip_stat.mask);

            l_ret = asr_wlan_ap_start(init_info);

            break;
        #endif
        default:
            dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_open:mode not support:%d", init_info->wifi_mode);
            l_ret = -1;
            break;
    }

    dbg(D_INF,D_UWIFI_CTRL,"%s version %s l_ret=%d \r\n",__func__,asr_get_wifi_version(),l_ret);

    return l_ret;
}

#if 0
int asr_wlan_start_adv(asr_wlan_init_info_adv_st* pst_network_init_info)  //TODO_ASR
{
    int l_ret = -1;
    struct wifi_conn_t st_wifi_connect_param;
    struct config_info *pst_config_info = uwifi_get_sta_config_info();
    dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_start_adv!");
    if (NULL == pst_config_info)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_start_adv:pst_config_info is null!");
        return -1;
    }

    if (NULL == pst_network_init_info)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_start_adv:input is null!");
        return -1;
    }

    if(current_iftype != 0xFF)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_open: cannot start twice!");
        return -1;
    }

    /* 1. start sta wifi */
    uwifi_msg_at2u_open_mode(STA_MODE, NULL);
    if (asr_rtos_get_semaphore(&asr_api_ctrl_param.open_semaphore, ASR_NEVER_TIMEOUT))
    {
        dbg(D_ERR,D_UWIFI_CTRL,"open cmd timed-out");
        return -1;
    }
    if(asr_api_ctrl_param.open_result)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_sta_start:start sta failed!");
        return -1;
    }

    /* 2. save config info to vif */
    l_ret = asr_save_sta_config_info_adv(pst_config_info, pst_network_init_info);
    if (0 != l_ret)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_start_adv:save config info failed!");
        return -1;
    }

    /* 3. start connect */
    memset(&st_wifi_connect_param, 0, sizeof(struct wifi_conn_t));
    st_wifi_connect_param.ssid_len   = pst_config_info->connect_ap_info.ssid_len;
    st_wifi_connect_param.ssid = asr_rtos_malloc(st_wifi_connect_param.ssid_len);
    if(st_wifi_connect_param.ssid == NULL)
        return -1;
    memcpy(st_wifi_connect_param.ssid, pst_config_info->connect_ap_info.ssid, st_wifi_connect_param.ssid_len);
    st_wifi_connect_param.pwd_len    = strlen((const char*)pst_config_info->connect_ap_info.pwd);
    st_wifi_connect_param.password = asr_rtos_malloc(st_wifi_connect_param.pwd_len);
    if(st_wifi_connect_param.password == NULL)
    {
        asr_rtos_free(st_wifi_connect_param.ssid);
        st_wifi_connect_param.ssid = NULL;
        return -1;
    }
    memcpy(st_wifi_connect_param.password, pst_config_info->connect_ap_info.pwd, st_wifi_connect_param.pwd_len);
    st_wifi_connect_param.encrypt    = pst_network_init_info->ap_info.security;
    memcpy(st_wifi_connect_param.bssid, pst_network_init_info->ap_info.bssid, ETH_ALEN);

    uwifi_msg_at2u_connect_adv_req(&st_wifi_connect_param);
    if (asr_rtos_get_semaphore(&asr_api_ctrl_param.connect_adv_semaphore, ASR_NEVER_TIMEOUT))
    {
        dbg(D_ERR,D_UWIFI_CTRL,"connect adv cmd timed-out");
        return -1;
    }
    asr_rtos_free(st_wifi_connect_param.password);
    st_wifi_connect_param.password = NULL;
    asr_rtos_free(st_wifi_connect_param.ssid);
    st_wifi_connect_param.ssid = NULL;
    if(asr_api_ctrl_param.connect_adv_result)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_sta_start:connect adv failed!");
        return -1;
    }

    current_iftype = STA_MODE;

    return l_ret;
}
#endif

static void hex_to_char(unsigned char data, char *h, char *l)
{
    unsigned char tmp = data;
    char h_tmp, l_tmp;

    tmp &= 0xff;
    l_tmp = (((tmp & 0xf) > 9) ? ('a' + ((tmp & 0xf) - 10) ) :
                                ( '0' + (tmp & 0xf) ) );
    h_tmp = ((((tmp >> 4) & 0xf) > 9) ? ('a' + (((tmp >> 4) & 0xf) - 10) ) :
                                ( '0' + ((tmp >> 4) & 0xf) ) );
    *h = h_tmp;
    *l = l_tmp;
    return;
}

int asr_wlan_get_mac_address_inchar(char *puc_mac)
{
    uint8_t auc_mac[ETH_ALEN];
    if (NULL == puc_mac)
    return -1;

    if (0 != asr_wlan_get_mac_address(auc_mac))
    return -1;

    hex_to_char(auc_mac[0], puc_mac, puc_mac+1);
    hex_to_char(auc_mac[1], puc_mac+2, puc_mac+3);
    hex_to_char(auc_mac[2], puc_mac+4, puc_mac+5);
    hex_to_char(auc_mac[3], puc_mac+6, puc_mac+7);
    hex_to_char(auc_mac[4], puc_mac+8, puc_mac+9);
    hex_to_char(auc_mac[5], puc_mac+10, puc_mac+11);
    return 0;
}

asr_wlan_ip_stat_t * asr_wlan_get_ip_status()
{
    struct config_info *pst_config_info;

#ifdef CFG_STATION_SUPPORT
    if(current_iftype == STA_MODE)
    {
        pst_config_info = uwifi_get_sta_config_info();

        if (NULL == pst_config_info)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_get_ip_status:pst_config_info is null!");
            return NULL;
        }

        if (!pst_config_info->is_connected)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_get_ip_status::ip address not got!");
            return NULL;
        }

        return &(pst_config_info->wlan_ip_stat);
    }
    else
#endif
#ifdef CFG_SOFTAP_SUPPORT
    if(current_iftype == SAP_MODE)
    {
        return &g_dhcpd_ip_stat;
    }
    else
#endif
    {
        dbg(D_ERR, D_UWIFI_CTRL, "cur mode %d, cannot get ip status",(int)current_iftype);

        return NULL;
    }
}

#ifdef CFG_STATION_SUPPORT
int asr_wlan_get_link_status(asr_wlan_link_stat_t *pst_link_status)
{
    struct asr_hw     *pst_asr_hw     = uwifi_get_asr_hw();
    struct asr_vif    *pst_asr_vif    = NULL;

    if (NULL == pst_asr_hw)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_get_link_status:no asr hw!");
        return -1;
    }

    pst_asr_vif    = pst_asr_hw->vif_table[pst_asr_hw->sta_vif_idx];
    if (NULL == pst_asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_get_link_status:no sta vif!");
        return -1;
    }

    pst_link_status->is_connected = pst_asr_vif->sta.configInfo.is_connected;
    if (pst_link_status->is_connected)
    {
        //rssi TBD
        pst_link_status->channel = phy_freq_to_channel(pst_asr_vif->sta.ap->band, pst_asr_vif->sta.ap->center_freq);
        memcpy(pst_link_status->ssid, pst_asr_vif->sta.configInfo.connect_ap_info.ssid, MAX_SSID_LEN);
        memcpy(pst_link_status->bssid, pst_asr_vif->sta.ap->mac_addr, ETH_ALEN);
    }
    return 0;
}

asr_wlan_ap_info_adv_t *asr_wlan_get_associated_apinfo()
{
    struct config_info *pst_config_info = uwifi_get_sta_config_info();
    if (NULL == pst_config_info)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_get_ip_status:pst_config_info is null!");
        return NULL;
    }

    return &(pst_config_info->connect_ap_info);
}
//int only_scan_done_flag = 0;
//extern asr_mutex_t g_only_scan_mutex;
extern int g_sdio_reset_flag;
//extern uint8_t scan_timeout;

int asr_wlan_start_scan(void)
{
    if(g_sdio_reset_flag)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:sdio reset \r\n", __func__);
        asr_rtos_delay_milliseconds(5);
        return -1;
    }

    struct wifi_scan_t *st_wifi_scan = asr_rtos_malloc(sizeof(struct wifi_scan_t));
    if (st_wifi_scan == NULL)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:malloc fail!", __func__);
        return -1;
    }
    st_wifi_scan->scanmode = SCAN_ALL_CHANNEL;//for scan channel to be 1
    st_wifi_scan->freq = 0;
    st_wifi_scan->ssid_len = 0;
    st_wifi_scan->ssid = NULL;

    is_scan_adv = UI_SCAN;
    uwifi_msg_at2u_scan_req(st_wifi_scan);

    return 0;
}

int asr_wlan_start_scan_adv(void)
{
    if(g_sdio_reset_flag)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:sdio reset \r\n", __func__);
        asr_rtos_delay_milliseconds(5);
        return -1;
    }

    struct wifi_scan_t *st_wifi_scan = asr_rtos_malloc(sizeof(struct wifi_scan_t));
    if (st_wifi_scan == NULL)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:malloc fail!", __func__);
        return -1;
    }
    st_wifi_scan->scanmode = SCAN_ALL_CHANNEL;
    st_wifi_scan->freq = 0;
    st_wifi_scan->ssid_len = 0;
    st_wifi_scan->ssid = NULL;
    is_scan_adv = ADV_SCAN;
    uwifi_msg_at2u_scan_req(st_wifi_scan);

    return 0;
}

int asr_wlan_start_scan_detail(char *ssid, int channel, char *bssid)
{
    int ret;

    if(g_sdio_reset_flag)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:sdio reset \r\n", __func__);
        asr_rtos_delay_milliseconds(5);
        return -1;
    }

    struct wifi_scan_t *st_wifi_scan = asr_rtos_malloc(sizeof(struct wifi_scan_t));
    if (st_wifi_scan == NULL)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "wlan_start_scan_adv malloc fail!");
        return -1;
    }
    memset(st_wifi_scan, 0, sizeof(struct wifi_scan_t));

    dbg(D_INF, D_UWIFI_CTRL, "scan_detail %d %s 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x",
        channel, ssid, bssid[0],bssid[1],bssid[2],bssid[3],bssid[4],bssid[5]);

    if(channel > 0)
    {
        ret = uwifi_check_legal_channel(channel);
        if(ret == 0)
        {
            if(channel == 14)
            {
                st_wifi_scan->freq = 2484;
            }
            else
            {
                st_wifi_scan->freq = 2407 + channel * 5;
            }
            st_wifi_scan->scanmode = SCAN_FREQ_SSID;
        }
        else
        {
            st_wifi_scan->scanmode = SCAN_ALL_CHANNEL;
            st_wifi_scan->freq = 0;
        }
    }
    else
    {
        st_wifi_scan->scanmode = SCAN_ALL_CHANNEL;
        st_wifi_scan->freq = 0;
    }

    if(bssid != 0)
    {
        memcpy(st_wifi_scan->bssid, bssid, MAC_ADDR_LEN);
    }

    if ((ssid != 0) && (strlen(ssid) <= IEEE80211_MAX_SSID_LEN))
    {
        st_wifi_scan->ssid_len = strlen(ssid);
        st_wifi_scan->ssid = asr_rtos_malloc(st_wifi_scan->ssid_len+1);
        if (st_wifi_scan->ssid == NULL)
        {
            st_wifi_scan->ssid_len = 0;
        }
        else
        {
            memcpy(st_wifi_scan->ssid, ssid, st_wifi_scan->ssid_len);
        }
    }
    else
    {
        st_wifi_scan->ssid_len = 0;
        st_wifi_scan->ssid = 0;
    }
    is_scan_adv = UI_SCAN;
    uwifi_msg_at2u_scan_req(st_wifi_scan);

    return 0;
}

int asr_wlan_start_scan_active(const char *ssid)
{
    if(g_sdio_reset_flag)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:sdio reset \r\n", __func__);
        asr_rtos_delay_milliseconds(5);
        return -1;
    }

    struct wifi_scan_t *st_wifi_scan = NULL;

    if (strlen(ssid) == 0 || strlen(ssid) > IEEE80211_MAX_SSID_LEN)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:ssid len %u is error!", __func__, strlen(ssid));
        return -1;
    }

    st_wifi_scan = asr_rtos_malloc(sizeof(struct wifi_scan_t));
    if (st_wifi_scan == NULL)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:scan malloc fail!", __func__);
        return -1;
    }

    st_wifi_scan->scanmode = SCAN_SSID;
    st_wifi_scan->freq = 0;
    st_wifi_scan->ssid_len = strlen(ssid);
    st_wifi_scan->ssid = asr_rtos_malloc(strlen(ssid));
    if (st_wifi_scan->ssid == NULL)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:ssid malloc fail!", __func__);
        asr_rtos_free(st_wifi_scan);
        st_wifi_scan = NULL;
        return -1;
    }
    memcpy(st_wifi_scan->ssid, ssid, strlen(ssid));
    is_scan_adv = DEFAULT_SCAN;
    uwifi_msg_at2u_scan_req(st_wifi_scan);

    return 0;
}
#endif

extern bool arp_debug_log;
int asr_wlan_close(void)
{
    if(current_iftype == 0xFF)
    {
        dbg(D_ERR, D_OS, "%s has been closed\r\n ",__func__);
        return -1;
    }
    dbg(D_INF, D_OS, "%s user close wifi itself\r\n ",__func__);

    asr_wlan_set_scan_status(SCAN_USER_CLOSE);


#if NX_STA_AP_COEX
    if(current_iftype == STA_AP_MODE)
    {
        uwifi_msg_at2u_close(STA_MODE);
        uwifi_msg_at2u_close(SAP_MODE);
    }
    else
#endif
    {
        uwifi_msg_at2u_close(current_iftype);
    }
    // waitting for close

    current_iftype = 0xFF;
    //arp_debug_log = 1;
    dbg(D_INF, D_OS, "close malloc cnt %d",(int)asr_malloc_cnt);

    return 0;
}

int asr_wlan_power_on(void)
{
    dbg(D_INF, D_UWIFI_CTRL, "power on success");
    return 0;
}

#ifdef CFG_STATION_SUPPORT
int asr_wlan_suspend_sta(void)
{
    if (current_iftype != STA_MODE)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_suspend_sta:it is not sta mode!");
        return 0;
    }
    uwifi_msg_at2u_disconnect(DISCONNECT_REASON_USER_GENERATE);
    if (asr_rtos_get_semaphore(&asr_api_ctrl_param.disconnect_semaphore, ASR_NEVER_TIMEOUT))
    {
        dbg(D_ERR,D_UWIFI_CTRL,"disconnect cmd timed-out");
        return -1;
    }
    if(asr_api_ctrl_param.disconnect_result)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_suspend_sta:disconnect failed!");
        return -1;
    }

    //wpa_disconnect();
    return 0;
}
#endif

extern void uwifi_hostapd_deauth_peer_sta(struct asr_hw *asr_hw, uint8_t *mac_addr);
int asr_wlan_suspend_ap(void)  //TODO_ASR
{
    struct asr_hw  *pst_asr_hw    = uwifi_get_asr_hw();
    struct asr_vif *pst_asr_vif    = pst_asr_hw->vif_table[pst_asr_hw->ap_vif_idx];
    struct uwifi_ap_peer_info *cur, *tmp;

    if (NULL == pst_asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_suspend_ap:no ap vif!");
        return -1;
    }

    list_for_each_entry_safe(cur, tmp, &pst_asr_vif->ap.peer_sta_list, list)
    {
        uwifi_hostapd_deauth_peer_sta(pst_asr_hw, cur->peer_addr);
    }

    return 0;
}

int asr_wlan_suspend(void)
{
#ifdef CFG_STATION_SUPPORT
    if (STA_MODE == current_iftype)
        return asr_wlan_suspend_sta();
    else
#endif
#ifdef CFG_SOFTAP_SUPPORT
    if (SAP_MODE == current_iftype)
        return asr_wlan_suspend_ap();
    else
#endif
    {
        dbg(D_ERR,D_UWIFI_CTRL,"asr_wlan_suspend not wifi opened");
        return -1;
    }
}

#ifdef CFG_STATION_SUPPORT
int asr_wlan_set_ps_options(uint8_t listen_bc_mc, uint16_t listen_interval)
{
#if NX_POWERSAVE
    return asr_send_set_ps_options(listen_bc_mc, listen_interval);
#else
    dbg(D_ERR, D_UWIFI_CTRL, "%s error, power save mode not supported!", __func__);
    return -1;
#endif
}

int asr_wlan_set_ps_mode(uint8_t ps_on)
{
#if NX_POWERSAVE
    uwifi_msg_at2u_set_ps_mode(ps_on);
    return 0;
#else
    dbg(D_ERR, D_UWIFI_CTRL, "%s error, power save mode not supported!", __func__);
    return -1;
#endif
}
#endif

#ifdef CFG_SNIFFER_SUPPORT
int asr_wlan_monitor_set_channel(int channel)
{
    if(current_iftype == SNIFFER_MODE)
    {
        uwifi_sniffer_handle_channel_change(channel);
    }
    return 0;
}

int asr_wlan_start_monitor(void)
{
    dbg(D_INF, D_UWIFI_CTRL, "asr malloc cnt %d",(int)asr_malloc_cnt);
    if(current_iftype == SNIFFER_MODE)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "monitor has been opened!");
        return 0;
    }

    /* 1. start monitor wifi */
    uwifi_msg_at2u_open_monitor_mode();
    if (asr_rtos_get_semaphore(&asr_api_ctrl_param.open_monitor_semaphore, ASR_NEVER_TIMEOUT))
    {
        dbg(D_ERR,D_UWIFI_CTRL,"open monitor cmd timed-out");
        return -1;
    }
    if(asr_api_ctrl_param.open_monitor_result)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr_wlan_start_monitor:start monitor failed!");
        return -1;
    }
    current_iftype = SNIFFER_MODE;
    return 0;
}

int asr_wlan_stop_monitor(void)
{
    if (current_iftype == SNIFFER_MODE)
    {
#ifdef CFG_MIMO_UF
        asr_rtos_declare_critical();
        asr_rtos_enter_critical();
        uf_rx_disable();
        asr_rtos_exit_critical();
#endif
        asr_wlan_close();
    }
    dbg(D_INF, D_UWIFI_CTRL, "asr_wlan_stop_monitor_end!");
    return 0;
}

int asr_wlan_register_monitor_cb(monitor_cb_t fn)
{
    uwifi_sniffer_handle_cb_register((uint32_t)fn);
    return 0;
}

void asr_wlan_register_mgmt_monitor_cb(monitor_cb_t fn)
{
    uwifi_sniffer_handle_mgmt_cb_register((uint32_t)fn);
}
#endif

int asr_wlan_send_raw_frame(uint8_t *buf, int len)
{
#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
    struct asr_hw  *pst_asr_hw = uwifi_get_asr_hw();
    struct asr_vif *pst_asr_vif;

    if (pst_asr_hw == NULL)
    {
        return -1;
    }

    if (pst_asr_hw->sta_vif_idx != 0xFF)
    {
        pst_asr_vif = pst_asr_hw->vif_table[pst_asr_hw->sta_vif_idx];
        if ((pst_asr_vif != NULL) && (pst_asr_vif->sta.connInfo.wifiState >= WIFI_CONNECTED))
        {
            return uwifi_tx_custom_mgmtframe(buf, len);
        }
    }
    else if (pst_asr_hw->monitor_vif_idx != 0xFF)
        return uwifi_tx_custom_mgmtframe(buf, len);
    else
#endif
    {
        dbg(D_ERR, D_UWIFI_CTRL, "can not send custom tx");
        return -2;
    }

    return 0;
}

int asr_wlan_reset()
{
    struct asr_hw  *pst_asr_hw = uwifi_get_asr_hw();
    struct asr_vif *pst_asr_vif;

    if (pst_asr_hw == NULL)
    {
        return -1;
    }

    if (pst_asr_hw->sta_vif_idx != 0xFF)
    {
        pst_asr_vif = pst_asr_hw->vif_table[pst_asr_hw->sta_vif_idx];
        if ((pst_asr_vif != NULL) && (pst_asr_vif->sta.connInfo.wifiState >= WIFI_CONNECTED))
        {
            dev_restart_work_func(pst_asr_vif);
            return 0;
        }
    }

    return 0;
}

int asr_wlan_init(void)
{
    if(!g_asr_wlan_state_complete)
    {
        asr_rtos_init_semaphore(&asr_api_ctrl_param.open_semaphore, 0);
        asr_rtos_init_semaphore(&asr_api_ctrl_param.open_monitor_semaphore, 0);
        asr_rtos_init_semaphore(&asr_api_ctrl_param.disconnect_semaphore, 0);
        asr_api_ctrl_param.open_result = 0;
        asr_api_ctrl_param.open_monitor_result = 0;
        asr_api_ctrl_param.disconnect_result = 0;

        memset( &g_softap_info, 0, sizeof(struct softap_info_t));

        init_all_flag();
        if(init_uwifi()!= kNoErr)
        {
            dbg(D_CRT,D_UWIFI_CTRL,"init uwifi task falied");
            return -1;
        }

        g_asr_wlan_state_complete = 1;
        //g_asr_wlan_init = 1;
    }
    else
    {
        dbg(D_INF, D_UWIFI_CTRL, "asr wlan already init");
    }

    return 0;
}

extern asr_thread_hand_t  uwifi_task_handle;
int asr_wlan_deinit(void)
{
    asr_wlan_close();

    //
    asr_rtos_delay_milliseconds(1000);

    if(g_asr_wlan_state_complete)
    {
        clear_all_flag();

        if(sp_asr_hw != NULL)
        {
            if( STA_MODE == current_iftype) {
                uwifi_platform_deinit(sp_asr_hw, sp_asr_hw->vif_table[sp_asr_hw->sta_vif_idx]);
            } else if((SAP_MODE == current_iftype) ||
                      ((sp_asr_hw->ap_vif_idx < NX_VIRT_DEV_MAX) && (NL80211_IFTYPE_AP == sp_asr_hw->vif_table[sp_asr_hw->ap_vif_idx]->iftype))) {
                uwifi_platform_deinit(sp_asr_hw, sp_asr_hw->vif_table[sp_asr_hw->ap_vif_idx]);
            } else {
                uwifi_platform_deinit(sp_asr_hw, NULL);
            }

            sp_asr_hw = NULL;
        }

        if(asr_api_ctrl_param.open_semaphore)
            asr_rtos_deinit_semaphore(&asr_api_ctrl_param.open_semaphore);
        if(asr_api_ctrl_param.open_monitor_semaphore)
            asr_rtos_deinit_semaphore(&asr_api_ctrl_param.open_monitor_semaphore);
        if(asr_api_ctrl_param.disconnect_semaphore)
            asr_rtos_deinit_semaphore(&asr_api_ctrl_param.disconnect_semaphore);

        if(uwifi_task_handle.thread)
        {
            deinit_uwifi();
            uwifi_task_handle.thread = 0;
        }

        asr_api_ctrl_param.open_result = 0;
        asr_api_ctrl_param.open_monitor_result = 0;
        asr_api_ctrl_param.disconnect_result = 0;

        //if(wifi_ready!=WIFI_NOT_READY)
        //    set_wifi_ready_status(WIFI_NOT_READY);

        g_asr_wlan_state_complete = 0;
        dbg(D_ERR, D_UWIFI_CTRL, "asr wlan deinit done");
    }
    else
    {
        dbg(D_ERR, D_UWIFI_CTRL, "asr wlan not init");
    }

    return 0;
}

int asr_wlan_start_debug_mode(void)
{
    GlobalDebugEn = 1;

    return 0;
}

int asr_wlan_stop_debug_mode(void)
{
    GlobalDebugEn = 0;

    return 0;
}

void asr_wlan_set_country_code(char *country)
{
    struct asr_hw  *pst_asr_hw = uwifi_get_asr_hw();
    if(NULL==pst_asr_hw)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "please init wifi first");
        return;
    }
    if((country[0] != pst_asr_hw->country[0]) || (country[1] != pst_asr_hw->country[1]))
    {
        pst_asr_hw->country[0] = country[0];
        pst_asr_hw->country[1] = country[1];
    }
}

asr_semaphore_t asr_wlan_vendor_scan_semaphore = 0;
asr_wlan_cb_scan_compeleted asr_wlan_vendor_scan_comp_callback = 0;

// FIXME: for compile on alios
//#ifndef ALIOS_SUPPORT
extern asr_wlan_event_cb_t asr_wlan_event_cb_handle;
int asr_wlan_ip_got_cb_register(asr_wlan_cb_ip_got fn)
{
    asr_wlan_event_cb_handle.ip_got = fn;

    return 0;
}

int asr_wlan_stat_chg_cb_register(asr_wlan_cb_stat_chg fn)
{
    asr_wlan_event_cb_handle.stat_chg = fn;

    return 0;
}

int asr_wlan_scan_compeleted_cb_register(asr_wlan_cb_scan_compeleted fn)
{
    asr_wlan_event_cb_handle.scan_compeleted = fn;

    return 0;
}

int asr_wlan_associated_ap_cb_register(asr_wlan_cb_associated_ap fn)
{
    asr_wlan_event_cb_handle.associated_ap = fn;

    return 0;
}

int asr_wlan_rssi_cb_register(asr_wlan_cb_rssi fn)
{
    asr_wlan_event_cb_handle.rssi_chg = fn;
    return 0;
}

int asr_wlan_err_stat_cb_register(asr_wlan_err_stat_handler fn)
{
    asr_wlan_event_cb_handle.error_status = fn;
    return 0;
}

int asr_wlan_ap_add_dev_cb_register(asr_wlan_ap_dev_handler fn)
{
    asr_wlan_event_cb_handle.ap_add_dev = fn;
    return 0;
}

int asr_wlan_ap_del_dev_cb_register(asr_wlan_ap_dev_handler fn)
{
    asr_wlan_event_cb_handle.ap_del_dev = fn;
    return 0;
}

#ifdef CFG_SNIFFER_SUPPORT
int asr_wlan_sniffer_mgmt_cb_register(uint32_t asr_wlan_mgmt_sniffer)
{
    uwifi_sniffer_handle_mgmt_cb_register(asr_wlan_mgmt_sniffer);

    return 0;
}
#endif

//#endif
#ifdef CFG_DBG
int asr_wlan_host_log_set(uint8_t log_switch)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    if (log_switch == 1)
        return asr_send_set_host_log_req(asr_hw, DBG_HOST_LOG_ON);
    else
        return asr_send_set_host_log_req(asr_hw, DBG_HOST_LOG_OFF);
}
#endif

int asr_wlan_get_err_code(uint8_t mode)
{
    uint8_t ret = -1;
    if( STA == mode)
    {
        ret = wifi_conect_fail_code;
    }
    else if ( SOFTAP == mode)
    {
        ret =  wifi_softap_fail_code;
    }
    dbg(D_INF, D_UWIFI_CTRL, "%s mode=%d ret=%d\r\n", __func__,mode,ret);
    return ret;
}


void asr_wlan_set_scan_status(asr_scan_status_e _status)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    //struct asr_vif *asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];
    asr_rtos_lock_mutex(&asr_hw->scan_mutex);
    scan_done_flag = _status;
    asr_rtos_unlock_mutex(&asr_hw->scan_mutex);
    if(SCAN_TIME_OUT ==_status)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s time out need restart card\r\n", __func__);
        //asr_vif->sta.connInfo.wifiState = WIFI_SCAN_FAILED;
                 //     dbg(D_ERR, D_UWIFI_CTRL, "%s reset card reg\r\n",__func__);
        // SDIO_CMD52(1, 0, 0x2, 0x0, NULL);//write IO_Enable 0x0, disable io
        // asr_rtos_delay_milliseconds(20);//delay 5ms
        // SDIO_CMD52(1, 0, 0x2, 0x2, NULL);///write IO_Enable 0x2, enable io
        // asr_rtos_delay_milliseconds(20);//delay 5ms
        // SDIO_CMD52(1, 0, 0x0, 0x4, NULL);///write H2C_INTEVENT 0x4, terminate cmd53 data
    }

    //asr_rtos_delay_milliseconds(100);

    dbg(D_INF, D_UWIFI_CTRL, "%s scan status=%d\n", __func__,_status);
}


int asr_wlan_get_scan_status(void)
{
    int ret = scan_done_flag;
    dbg(D_INF, D_UWIFI_CTRL, "%s  status=%d\r\n", __func__,ret);
    return ret;
}

void init_all_flag(void)
{

    if(SCAN_INIT != scan_done_flag)
    {
        scan_done_flag = SCAN_INIT;
    }

    //wifi_connect_err = 0;
    wifi_conect_fail_code = 0;
    //wifi_psk_is_ture = 0;

    if(WIFI_NOT_READY != wifi_ready)
    {
        wifi_ready = WIFI_NOT_READY;
    }

    //asr_rtos_init_mutex(&g_only_scan_mutex);
}

void clear_all_flag(void)
{
    //wifi_connect_err = 0;
    wifi_conect_fail_code = 0;
    //wifi_psk_is_ture = 0;

    if(WIFI_NOT_READY != wifi_ready)
        set_wifi_ready_status(WIFI_NOT_READY);

    if(SCAN_DEINIT != scan_done_flag)
        asr_wlan_set_scan_status(SCAN_DEINIT);

    //asr_rtos_deinit_mutex(&g_only_scan_mutex);
}

void set_wifi_ready_status(asr_wifi_ready_status_e _status)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    asr_rtos_lock_mutex(&asr_hw->wifi_ready_mutex);
    wifi_ready = _status;
    asr_rtos_unlock_mutex(&asr_hw->wifi_ready_mutex);
}

// record err status and handle err.
void asr_wlan_set_disconnect_status(asr_wlan_err_status_e _status)
{

    dbg(D_ERR, D_UWIFI_CTRL, "%s %d",__func__,_status);

    //wifi_connect_err = _status;
    wifi_conect_fail_code = _status;

#ifdef CFG_ENCRYPT_SUPPORT
    if(_status == WLAN_STA_MODE_PASSWORD_ERR)
    {
        asr_wlan_clear_pmk();
    }
#endif

    if (asr_wlan_event_cb_handle.error_status)
        asr_wlan_event_cb_handle.error_status(_status);

}


//extern int autoconnect_tries;
void asr_wlan_set_wifi_ready_status(asr_wifi_ready_status_e _status)
{
    extern uint8_t wifi_ready;
    if(_status)
    {
        //autoconnect_tries = 0;
        //wifi_connect_err = 0;
        wifi_conect_fail_code = 0;
        //wifi_psk_is_ture = 0;
        dbg(D_INF,D_UWIFI_CTRL,"%s:up...\r\n",__func__);
    }

    set_wifi_ready_status(_status);   // set wifi ready when got ip ok.

}

int asr_wlan_get_wifi_ready_status()
{
    int ret = wifi_ready;
    return ret;
}

int asr_wlan_get_wifi_mode(void)
{
    if(current_iftype == STA_MODE)
    {
        return STA;
    }
    else if(current_iftype == SAP_MODE)
    {
        return SOFTAP;
    }
    else if(current_iftype == SNIFFER_MODE)
    {
        return SNIFFER;
    }
    else
    {
        return current_iftype;
    }
}

///////////asr api sync method//////////////////////////////////////////////////////////////////////////////////////////////////////////

asr_semaphore_t asr_api_protect;

void asr_api_adapter_lock_init()
{
    asr_rtos_init_semaphore(&asr_api_protect, 0);
}

void asr_api_adapter_unlock()
{
    asr_rtos_set_semaphore(&asr_api_protect);
}

void asr_api_adapter_lock(void)
{
    if(asr_rtos_get_semaphore(&asr_api_protect, ASR_API_TIMEOUT))
        dbg(D_ERR,D_UWIFI_CTRL,"pre asr api is running\n");

    dbg(D_INF,D_UWIFI_CTRL,"LOCK \n");
}

void asr_api_adapter_try_unlock(asr_api_rsp_status_t status)
{

    asr_rtos_declare_critical();
    asr_rtos_enter_critical();

    if(status == CONFIG_OK)
    {
        dbg(D_INF,D_UWIFI_CTRL,"OK,UNLOCK \n");

        asr_rtos_set_semaphore(&asr_api_protect);
    }
    else if(status == WAIT_PEER_RSP)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"WAIT_PEER_RSP,UNLOCK \n");

        asr_rtos_set_semaphore(&asr_api_protect);
    }
    else if(status != RSP_NULL)
    {

        dbg(D_ERR,D_UWIFI_CTRL,"FAIL:-%d , UNLOCK \n",status);

        asr_rtos_set_semaphore(&asr_api_protect);
    }

    asr_rtos_exit_critical();

}


void asr_api_adapter_lock_deinit()
{
    asr_rtos_deinit_semaphore(&asr_api_protect);
}

/*
    asr api adater sync method (for asr AT cmd / user adater / ...).
    1)  asr api no wait resp, direct unlock.

    2)  unlock in asr_wifi_connect_state
    EVENT_AP_UP                      // softap up
    EVENT_AP_DOWN                    // softap down
    EVENT_STA_CLOSE                 //  station close, wifi_event_cb(WLAN_EVENT_STA_CLOSE,NULL);

    3) unlock for user call scan action finished.
       asr_wifi_scan_ind      // wifi_event_cb(WLAN_EVENT_SCAN_COMPLETED, (void *)&scan_results); in asr_rx_scanu_start_cfm

    4) unlock when got ip ok for user call connect sta in station mode.
       asr_wifi_get_ip_ind    // wifi_event_cb(WLAN_EVENT_IP_GOT, (void*)(msg->param1));  in LWIP2U_IP_GOT

    5) err stats during open (for case cannot wait to get ip).
    asr_wlan_err_stat_handler
        WLAN_STA_MODE_PASSWORD_ERR
                asr_api_adapter_try_unlock(CONN_EAPOL_FAIL);          // 4way handshake issue.
        WLAN_STA_MODE_DHCP_FAIL
                asr_api_adapter_try_unlock(CONN_DHCP_FAIL);           // dhcp get ip issue.
        WLAN_STA_MODE_CONN_RETRY_MAX
                asr_api_adapter_try_unlock(CONN_TIMEOUT);             // reconnect to max timer issue.
*/
bool user_scan_flag = false;
void asr_wifi_scan_result_ind(asr_wlan_scan_result_t *result)
{
    if (user_scan_flag  == true)
    {
        int i;
        for(i=0; i<result->ap_num; i++)
        {
            dbg(D_INF, D_UWIFI_CTRL,"scan ap=%2d | rssi=%3d | ch=%2d | secu=%d | bssid=%02x:%02x:%02x:%02x:%02x:%02x | ssid=%s\r\n",
                i,
                result->ap_list[i].ap_power,
                result->ap_list[i].channel,
                result->ap_list[i].security,
                result->ap_list[i].bssid[0],result->ap_list[i].bssid[1],result->ap_list[i].bssid[2],
                result->ap_list[i].bssid[3],result->ap_list[i].bssid[4],result->ap_list[i].bssid[5],
                result->ap_list[i].ssid);
        }
        user_scan_flag = false;
        asr_api_adapter_try_unlock(CONFIG_OK);
    }

}

void asr_wifi_connect_state(asr_wifi_event_e stat)
{
    switch (stat)
    {
        case EVENT_STATION_UP:
            dbg(D_INF, D_UWIFI_CTRL,"%s EVENT_STATION_UP\r\n",__func__);
            break;
        case EVENT_STATION_DOWN:
            dbg(D_INF, D_UWIFI_CTRL,"%s EVENT_STATION_DOWN\r\n",__func__);
            break;
        case EVENT_AP_UP:
            dbg(D_INF, D_UWIFI_CTRL,"%s EVENT_AP_UP\r\n",__func__);
            asr_api_adapter_try_unlock(CONFIG_OK);
            break;
        case EVENT_AP_DOWN:
            dbg(D_INF, D_UWIFI_CTRL,"%s EVENT_AP_DOWN\r\n",__func__);
            asr_api_adapter_try_unlock(CONFIG_OK);
            break;
        case EVENT_STA_CLOSE:
            dbg(D_INF, D_UWIFI_CTRL,"wifi_sta_close_done");
            asr_api_adapter_try_unlock(CONFIG_OK);
            break;
        default :
            dbg(D_ERR, D_UWIFI_CTRL,"unsupport wifi_event_e :%d",stat);
            break;
    }
}

void asr_wifi_rssi_chg(uint8_t rssi_level)
{
    dbg(D_INF, D_UWIFI_CTRL,"%s rssi=%d\r\n",__func__,rssi_level);
}

void asr_wifi_get_ip_ind(asr_wlan_ip_stat_t *pnet)
{
    if (pnet) {
        dbg(D_INF, D_UWIFI_CTRL,"Got ip : %s, gw : %s, mask : %s, mac : %s\r\n", pnet->ip, pnet->gate, pnet->mask, pnet->macaddr);
    } else {
        dbg(D_INF, D_UWIFI_CTRL, "%s: ip is null\n", __func__);
    }
    asr_api_adapter_try_unlock(CONFIG_OK);
}

void asr_wifi_err_stat_handler(asr_wlan_err_status_e err_info)
{
    dbg(D_INF, D_UWIFI_CTRL, "%s wlan err sta:%d", __func__,err_info);

    switch(err_info)
    {
        case WLAN_STA_MODE_PASSWORD_ERR:
            asr_api_adapter_try_unlock(CONN_EAPOL_FAIL);
            break;
        case WLAN_STA_MODE_DHCP_FAIL:
            asr_api_adapter_try_unlock(CONN_DHCP_FAIL);
            break;
        case WLAN_STA_MODE_CONN_RETRY_MAX:
            asr_api_adapter_try_unlock(CONN_TIMEOUT);
            break;
        default:
            break;
    }

}

void asr_wifi_event_cb_register(void)
{
    #ifdef CFG_STATION_SUPPORT
    asr_wlan_scan_compeleted_cb_register(asr_wifi_scan_result_ind);
    asr_wlan_ip_got_cb_register(asr_wifi_get_ip_ind);
    asr_wlan_rssi_cb_register(asr_wifi_rssi_chg);
    #endif
    asr_wlan_stat_chg_cb_register(asr_wifi_connect_state);
    asr_wlan_err_stat_cb_register(asr_wifi_err_stat_handler);
}

void asr_wifi_enable_rssi_level_indication()
{
   int ret;
   /*  5. enable rssi level indication*/
   struct asr_hw *asr_hw = uwifi_get_asr_hw();
   struct asr_vif *asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];
   ret =  uwifi_cfg80211_ops.set_cqm_rssi_level_config(asr_vif , (bool)1);
   if (0 != ret)
   {
       dbg(D_ERR, D_UWIFI_CTRL, "asr_send_cfg_rssi_level_req failed!");
   }
}


int asr_wifi_get_rssi(int8_t *rssi)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    struct asr_vif *asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];

    if (!rssi) {
        return -1;
    }

    //dbg(D_INF, D_UWIFI_CTRL, "vif_index=%d,sta_idx=%d\n",asr_vif->vif_index,asr_vif->sta.ap->sta_idx);

    return asr_send_get_rssi_req(asr_hw, asr_vif->sta.ap->sta_idx, rssi);
}

int asr_wlan_set_dbg_cmd(unsigned int host_dbg_cmd)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();

    dbg(D_ERR, D_UWIFI_CTRL, "host_dbg_cmd = 0x%x \n",host_dbg_cmd);

    return asr_send_me_host_dbg_cmd(asr_hw, host_dbg_cmd);
}


int asr_wifi_set_upload_fram(uint8_t fram_type, uint8_t enable)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();

    if (fram_type != IEEE80211_STYPE_BEACON && fram_type != IEEE80211_STYPE_PROBE_REQ
        && fram_type != IEEE80211_STYPE_PROBE_RESP) {
        return -1;
    }

    //dbg(D_INF, D_UWIFI_CTRL, "vif_index=%d,fram_type=0x%X,enable=%d\n",asr_hw->sta_vif_idx,fram_type,enable);

    return asr_send_upload_fram_req(asr_hw, asr_hw->sta_vif_idx, fram_type, enable);
}

int asr_wifi_set_appie(uint8_t fram_type, uint8_t *ie, uint16_t ie_len)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    uint8_t vif_idx = 0;

    if (!asr_hw || ie_len > FRAM_CUSTOM_APPIE_LEN || (!ie && ie_len != 0)) {
        return -1;
    }
    if (fram_type != IEEE80211_STYPE_BEACON && fram_type != IEEE80211_STYPE_PROBE_REQ
        && fram_type != IEEE80211_STYPE_PROBE_RESP) {
        return -1;
    }

    if (fram_type == IEEE80211_STYPE_BEACON || fram_type == IEEE80211_STYPE_PROBE_RESP) {
        vif_idx = asr_hw->ap_vif_idx;
    } else if (fram_type == IEEE80211_STYPE_PROBE_REQ) {
        vif_idx = asr_hw->sta_vif_idx;
    }

    //dbg(D_INF, D_UWIFI_CTRL, "vif_idx=%d,fram_type=0x%X,ie_len=%d\n",vif_idx,fram_type,ie_len);

    return asr_send_fram_appie_req(asr_hw, vif_idx, fram_type, ie, ie_len);
}


int asr_wifi_write_rf_efuse(uint8_t * txpwr, uint8_t * txevm, uint8_t *freq_err, bool iswrite, uint8_t *index)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    uint8_t vif_idx = 0;
    int ret = 0;

    if (!asr_hw) {
        return -1;
    }
    if (!asr_hw || !txpwr || !txevm) {
        return -EINVAL;
    }

    if ((ret = asr_send_efuse_txpwr_req(asr_hw, txpwr, txevm, freq_err, iswrite, index))) {
        dbg(D_INF, D_UWIFI_CTRL, "RDWR efuse: %s fail,%d\n",iswrite?"WR":"RD", ret);
        return -1;
    }

    dbg(D_INF, D_UWIFI_CTRL, "RDWR efuse:%u,%u(%02X,%02X,%02X,%02X,%02X,%02X),(%02X,%02X,%02X,%02X,%02X,%02X),0x%X\n"
        ,iswrite,*index,txpwr[0],txpwr[1],txpwr[2],txpwr[3],txpwr[4],txpwr[5]
        ,txevm[0],txevm[1],txevm[2],txevm[3],txevm[4],txevm[5],*freq_err);

    return ret;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////

