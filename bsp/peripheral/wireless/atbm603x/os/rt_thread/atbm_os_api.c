/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#include "atbm_hal.h"
/******** Functions below is Wlan API **********/
#include "atbm_sysops.h"
#include <stdint.h>
#include "wlan_dev.h"

int g_wifi_is_connected=0;
extern struct rt_wlan_device * s_wlan_dev;
extern struct rt_wlan_device * s_ap_dev;

atbm_uint32 atbm_os_random()
{
    atbm_uint32 data = atbm_random()/3;
    return (data>>1);
}
char *atbm_strdup(const char *s)
{
    int len = rt_strlen(s) + 1;
    char *tmp = (char *)rt_malloc(len);

    if (!tmp)
        return NULL;

    atbm_memcpy(tmp, s, len);

    return tmp;
}

static rt_wlan_security_t atbm2wlan_security(atbm_int8 atbm)
{
    rt_wlan_security_t type = SECURITY_UNKNOWN;
    switch(atbm)
    {
    case ATBM_KEY_NONE:
        type = SECURITY_OPEN;
        break;
    case ATBM_KEY_WEP:
        type = SECURITY_WEP_PSK;
        break;
    case ATBM_KEY_WEP_SHARE:
        type = SECURITY_WEP_SHARED;
        break;
    case ATBM_KEY_WPA:
        type = SECURITY_WPA_AES_PSK;
        break;
    case ATBM_KEY_WPA2:
        type = SECURITY_WPA2_AES_PSK;
        break;
    case ATBM_KEY_MIX:
        type = SECURITY_WPA2_MIXED_PSK;
        break;
    default:
        type = SECURITY_UNKNOWN;
        break;
    }
    return type;
}

int atbm_wifi_report_scan_results(struct atbmwifi_vif *priv)
{
    struct rt_wlan_buff buff;
    struct rt_wlan_info scan_info;

    int ap_count=priv->scan_ret.len;
    struct atbmwifi_scan_result_info *info = priv->scan_ret.info;

    wifi_printk(WIFI_ALWAYS,"ap num:%d\n",ap_count);

    for(int i = 0; i < ap_count; i++) {
        atbm_memset(&scan_info, 0, sizeof(struct rt_wlan_info));
        scan_info.security = atbm2wlan_security(info->security);
        scan_info.band = RT_802_11_BAND_2_4GHZ;
        scan_info.channel = (short)info->channel;
        scan_info.rssi = info->rssi;
        atbm_memcpy(scan_info.ssid.val, info->ssid, info->ssidlen);
        scan_info.ssid.val[info->ssidlen] = 0;
        scan_info.ssid.len = info->ssidlen;
        atbm_memcpy(scan_info.bssid, info->BSSID, 6);

        buff.len = sizeof(struct rt_wlan_info);
        buff.data = (void *)&scan_info;
        rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_SCAN_REPORT, &buff);
        info++;
    }
    rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_SCAN_DONE, NULL);

    if (priv->scan_ret.info)
    {
        atbm_kfree(priv->scan_ret.info);
        priv->scan_ret.info = NULL;
    }
    return 0;
}

int atbmwifi_event_OsCallback(atbm_void *prv,int eventid,atbm_void *param)
{
    struct atbmwifi_vif *priv = prv;
    iot_printf("Event:(%d),if_type:(%d)\n", eventid,priv->iftype);

    if(atbmwifi_is_ap_mode(priv->iftype)){
        switch(eventid){
            case ATBM_WIFI_DEASSOC_EVENT:
            case ATBM_WIFI_DEAUTH_EVENT:
            {
                struct rt_wlan_buff buff;
                struct rt_wlan_info sta_info;
                atbm_uint8 *sta_mac=(atbm_uint8*)param;
                atbm_memcpy(sta_info.bssid,sta_mac,6);
                buff.len = sizeof(sta_info);
                buff.data = (void *)&sta_info;
                wifi_printk(WIFI_ALWAYS,"del sta: "MACSTR"\n",MAC2STR(sta_mac));
                rt_wlan_dev_indicate_event_handle(s_ap_dev, RT_WLAN_DEV_EVT_AP_DISASSOCIATED, &buff);
                break;
            }
            case ATBM_WIFI_JOIN_EVENT:
            {
                struct rt_wlan_buff buff;
                struct rt_wlan_info sta_info;

                atbm_uint8 *sta_mac=(atbm_uint8*)param;
                atbm_memcpy(sta_info.bssid,sta_mac,6);
                buff.len = sizeof(sta_info);
                buff.data = (void *)&sta_info;
                wifi_printk(WIFI_ALWAYS,"add sta: "MACSTR"\n",MAC2STR(sta_mac));
                rt_wlan_dev_indicate_event_handle(s_ap_dev, RT_WLAN_DEV_EVT_AP_ASSOCIATED, &buff);
                break;
            }
            case ATBM_WIFI_DISENABLE_NET_EVENT:
                rt_wlan_dev_indicate_event_handle(s_ap_dev, RT_WLAN_DEV_EVT_AP_STOP, NULL);
                break;
            case ATBM_WIFI_ENABLE_NET_EVENT:
            {
                rt_wlan_dev_indicate_event_handle(s_ap_dev, RT_WLAN_DEV_EVT_AP_START, NULL);
                break;
            }
            default:
                break;
        }
    }
    else {
        switch(eventid){
            case ATBM_WIFI_SCANDONE_EVENT:
            {
                 if((priv->scan_no_connect) && (priv->iftype==ATBM_NL80211_IFTYPE_STATION)){
                     atbm_wifi_report_scan_results(priv);
                 }
                break;
            }
            case ATBM_WIFI_ENABLE_NET_EVENT:
                if(g_wifi_is_connected !=1){
                    g_wifi_is_connected=1;
                    priv->auto_connect_when_lost = 0;
                    rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_CONNECT, NULL);
                }
                break;
            case ATBM_WIFI_DISENABLE_NET_EVENT:
            {
                g_wifi_is_connected=0;
                rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_DISCONNECT, NULL);
                break;
            }
            case ATBM_WIFI_WPS_SUCCESS:
                break;
            default:
                break;
        }
    }
    return 0;
}
