/**
 ******************************************************************************
 *
 * @file uwifi_notify.cpp
 *
 * @brief uwifi_notify impletion
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */
#include "uwifi_ieee80211.h"
#include "uwifi_notify.h"
//#include "at_callback.h"
#include "wifi_config.h"
#if UWIFI_TEST
#include "uwifi_msg.h"
#include "uwifi_basic_test.h"
#endif
#include "asr_dbg.h"
#include "uwifi_msg.h"
#ifdef LWIP
#include "lwipopts.h"
//#include "lwip_comm_wifi.h"
#endif
#include "asr_wlan_api_aos.h"
#include "hostapd.h"

extern void dhcps_record_client_wifi_status(uint8_t ip, uint8_t *client_mac);

#ifdef WIFI_TEST_LINUX
extern int wifi_test_result;
extern sem_t open_close_completion;
extern sem_t scan_result_completion;
extern sem_t connect_result_completion;

void wifi_test_linux_scan_cb(asr_wlan_scan_result_t *pApList)
{
    uint8_t i;
    printf("wifi_test_linux_scan_cb:AP num:%d\n", pApList->ap_num);
    for(i=0;i<pApList->ap_num;i++)
    {
         printf("[ap:%d]  name = %s  | rssi=%d \r\n",
            i,pApList->ap_list[i].ssid, (int8_t)pApList->ap_list[i].ap_power);
    }

    if (0 == pApList->ap_num)
    {
        wifi_test_result = -1;
    }
    else if (0 != memcmp(pApList->ap_list[0].ssid, "000", 3))
    {
        wifi_test_result = -1;
    }
    else
    {
        wifi_test_result = 0;
    }

    sem_post(&scan_result_completion);
}
#endif

void IOpenCloseStateCallback(enum wifi_open_close_state state)
{
    #ifdef WIFI_TEST_LINUX
    wifi_test_result = state;
    sem_post(&open_close_completion);
    #endif
}

// FIXME:
//#ifndef ALIOS_SUPPORT
asr_wlan_event_cb_t asr_wlan_event_cb_handle = {0};
int g_wifi_sta_up_down_flag = 0;// 1,up ;2 down

void wifi_event_cb(asr_wlan_event_e evt, void* info)
{
    switch (evt)
    {
#ifdef CFG_STATION_SUPPORT
        case WLAN_EVENT_SCAN_COMPLETED:
            if(asr_wlan_event_cb_handle.scan_compeleted)
                asr_wlan_event_cb_handle.scan_compeleted((asr_wlan_scan_result_t *)info);
            break;
        case WLAN_EVENT_AUTH:
            if(asr_wlan_event_cb_handle.stat_chg)
            {
                asr_wlan_event_cb_handle.stat_chg(EVENT_STATION_AUTH);
                dbg(D_ERR,D_UWIFI_CTRL,"%s:STA AUTH\r\n",__func__);
            }
            break;
        case WLAN_EVENT_ASSOCIATED:
            {
                asr_wlan_ap_info_adv_t *ap_info = asr_wlan_get_associated_apinfo();
                if(asr_wlan_event_cb_handle.associated_ap)
                    asr_wlan_event_cb_handle.associated_ap(ap_info);
            }
            break;
#endif
        case WLAN_EVENT_4WAY_HANDSHAKE:
            if(asr_wlan_event_cb_handle.stat_chg)
            {
                asr_wlan_event_cb_handle.stat_chg(EVENT_STATION_4WAY_HANDSHAKE);
            }
            break;
        case WLAN_EVENT_4WAY_HANDSHAKE_DONE:
            if(asr_wlan_event_cb_handle.stat_chg)
            {
                asr_wlan_event_cb_handle.stat_chg(EVENT_STATION_4WAY_HANDSHAKE_DONE);
            }
            break;
#ifdef CFG_STATION_SUPPORT
        case WLAN_EVENT_CONNECTED:
            if(g_wifi_sta_up_down_flag == 1)
                return;

            if(asr_wlan_event_cb_handle.stat_chg)
            {
                asr_wlan_event_cb_handle.stat_chg(EVENT_STATION_UP);
                dbg(D_ERR,D_UWIFI_CTRL,"%s:EVENT_STATION_UP uped\r\n",__func__);
                g_wifi_sta_up_down_flag = 1;
            }
            break;
        case WLAN_EVENT_IP_GOT:
            if(asr_wlan_event_cb_handle.ip_got)
                asr_wlan_event_cb_handle.ip_got((asr_wlan_ip_stat_t *)info);
            break;
        case WLAN_EVENT_DISCONNECTED:
            if(g_wifi_sta_up_down_flag == 2)
                return;

            if(asr_wlan_event_cb_handle.stat_chg)
            {
                asr_wlan_event_cb_handle.stat_chg(EVENT_STATION_DOWN);
                dbg(D_ERR,D_UWIFI_CTRL,"%s:EVENT_STATION_DOWN downed\r\n",__func__);
                g_wifi_sta_up_down_flag = 2;
            }
            break;
#endif
        case WLAN_EVENT_AP_UP:
            if(asr_wlan_event_cb_handle.stat_chg)
                asr_wlan_event_cb_handle.stat_chg(EVENT_AP_UP);
            break;
        case WLAN_EVENT_AP_DOWN:
            if(asr_wlan_event_cb_handle.stat_chg)
                asr_wlan_event_cb_handle.stat_chg(EVENT_AP_DOWN);
            break;
        case WLAN_EVENT_RSSI_LEVEL:
            if (asr_wlan_event_cb_handle.rssi_chg)
                asr_wlan_event_cb_handle.rssi_chg(*(uint8_t *)info);
            break;
        case WLAN_EVENT_STA_CLOSE:
             if(asr_wlan_event_cb_handle.stat_chg)
                asr_wlan_event_cb_handle.stat_chg(EVENT_STA_CLOSE);
            break;
        case WLAN_EVENT_AP_PEER_UP:
            if(asr_wlan_event_cb_handle.ap_add_dev)
                asr_wlan_event_cb_handle.ap_add_dev(((struct peer_sta_user_info *)info)->mac_addr);
            break;
        case WLAN_EVENT_AP_PEER_DOWN:
            if(asr_wlan_event_cb_handle.ap_del_dev)
                asr_wlan_event_cb_handle.ap_del_dev(((struct peer_sta_user_info *)info)->mac_addr);
            #ifndef JXC_SDK
            uint8_t *mac = ((struct peer_sta_user_info *)info)->mac_addr;
            dbg(D_INF,D_UWIFI_CTRL,"%s:AP_PEER_DOWN,mac=%X:%X:%X:%X:%X:%X\r\n",__func__,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
            dhcps_record_client_wifi_status(0, ((struct peer_sta_user_info *)info)->mac_addr);
            #endif
            break;
        default:
            break;
    }
}
//#endif
