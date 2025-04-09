/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "netif/etharp.h"
#include "lwip/tcpip.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <rtthread.h>
#include "wlan_dev.h"

#include "wifi/wifi_conf.h"
#include "net_stack_intf.h"

#include "aic_hal.h"

#define MAX_ETH_DRV_SG  16
#define MAX_ETH_MSG     1518

static struct rt_wlan_device *s_wlan_dev = NULL;
static struct rt_wlan_device *s_ap_dev = NULL;
static struct rt_wlan_device *s_current_dev = NULL;
static char rtw_mac[6];

rt_err_t aic_realtek_init(struct rt_wlan_device *wlan)
{
    return realtek_init();
}

rt_err_t aic_realtek_set_mode(struct rt_wlan_device *wlan, rt_wlan_mode_t mode)
{
    if(mode == RT_WLAN_AP) {
        if (!wifi_is_up(RTW_AP_INTERFACE)) {
            wifi_off();
            msleep(20);
            aicos_msleep(100);
            if (wifi_on(RTW_MODE_AP) < 0) {
                pr_err("WIFI mode AP start Failed\n");
                return -EIO;
            }
            s_current_dev = s_ap_dev;
            rt_wlan_dev_indicate_event_handle(s_ap_dev, RT_WLAN_DEV_EVT_AP_START, NULL);
        }
    } else if (mode == RT_WLAN_STATION) {
        if (!wifi_is_up(RTW_STA_INTERFACE)) {
            wifi_off();
            msleep(20);
            if (wifi_on(RTW_MODE_STA) < 0)
                return -EIO;
        }
        s_current_dev = s_wlan_dev;
    } else {
        wifi_off();
        s_current_dev = NULL;
    }

    return 0;
}

static rt_wlan_security_t _security_rtw_2_rtt(rtw_security_t rtw_type)
{
    switch (rtw_type) {
        case RTW_SECURITY_WPA_TKIP_8021X:
        case RTW_SECURITY_WPA_AES_8021X:
        case RTW_SECURITY_WPA2_AES_8021X:
        case RTW_SECURITY_WPA2_TKIP_8021X:
        case RTW_SECURITY_WPA_WPA2_MIXED_8021X:
        case RTW_SECURITY_WPA2_AES_CMAC:
        case RTW_SECURITY_WPA3_AES_PSK:
        case RTW_SECURITY_FORCE_32_BIT:
            return SECURITY_UNKNOWN;

        case RTW_SECURITY_WPA_WPA2_MIXED_PSK:
            return SECURITY_WPA2_MIXED_PSK;
        default :
            return rtw_type;
    }
    return rtw_type;
}

static rtw_result_t _scan_result_handler( rtw_scan_handler_result_t* malloced_scan_result )
{
    // struct rt_scan_info *scan_info = malloced_scan_result->user_data;
    struct rt_wlan_info info;
    struct rt_wlan_buff buff;

    if (malloced_scan_result->scan_complete != RTW_TRUE) {
        rtw_scan_result_t* record = &malloced_scan_result->ap_details;
        record->SSID.val[record->SSID.len] = 0; /* Ensure the SSID is null terminated */

        memset(&info, 0, sizeof(struct rt_scan_info));
        info.security = _security_rtw_2_rtt(record->security);
        info.band = record->band;
        info.datarate = 0;
        info.channel = record->channel;
        info.rssi = record->signal_strength;
        memcpy(&info.ssid, &record->SSID, sizeof(rt_wlan_ssid_t));
        memcpy(&info.bssid, record->BSSID.octet, 6);
        info.hidden = 0;

        buff.data = &info;
        buff.len = sizeof(struct rt_wlan_info);
        rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_SCAN_REPORT, &buff);
    } else {
        rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_SCAN_DONE, NULL);
    }
    return RTW_SUCCESS;
}

rt_err_t aic_realtek_scan(struct rt_wlan_device *wlan, struct rt_scan_info *scan_info)
{
    return wifi_scan_networks(_scan_result_handler, scan_info);
}

rt_err_t aic_realtek_join(struct rt_wlan_device *wlan, struct rt_sta_info *sta_info)
{
    int ret = RTW_ERROR;

    ret = wifi_connect(sta_info->ssid.val,
                        RTW_SECURITY_WPA2_AES_PSK,
                        sta_info->key.val,
                        sta_info->ssid.len,
                        sta_info->key.len,
                        0, NULL);

    rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_CONNECT, NULL); // todo: is block?

    return ret;
}


rt_err_t aic_realtek_softap(struct rt_wlan_device *wlan, struct rt_ap_info *ap_info)
{
    if (!wifi_is_up(RTW_AP_INTERFACE)) {
        wifi_off();
        msleep(20);
        aicos_msleep(100);
        if (wifi_on(RTW_MODE_AP) < 0) {
            pr_err("WIFI mode AP start Failed\n");
            return -EIO;
        }
    }

    if(wifi_start_ap(ap_info->ssid.val,
                    ap_info->security,
                    ap_info->key.val,
                    ap_info->ssid.len,
                    ap_info->key.len,
                    ap_info->channel
                    ) != RTW_SUCCESS) {
        pr_info("ERROR: Operation failed!\n");
        return -EIO;
    }

    rt_wlan_dev_indicate_event_handle(s_ap_dev, RT_WLAN_DEV_EVT_AP_START, NULL);

    return 0;
}

rt_err_t aic_realtek_disconnect(struct rt_wlan_device *wlan)
{
    int timeout = 20;
    char essid[33];

    pr_info("Deassociating AP ...\n");

    if(wext_get_ssid(WLAN0_NAME, (unsigned char *) essid) < 0) {
        pr_info("WIFI disconnected\n");
        return -1;
    }

    if(wifi_disconnect() < 0) {
        pr_info("ERROR: Operation failed!\n");
        return -1;
    }

    while(1) {
        if(wext_get_ssid(WLAN0_NAME, (unsigned char *) essid) < 0) {
            pr_info("WIFI disconnected\n");
            break;
        }

        if(timeout == 0) {
            pr_info("ERROR: Deassoc timeout!\n");
            break;
        }

        rtw_mdelay_os(1000);

        timeout --;
    }

    rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_DISCONNECT, NULL);

    return 0;
}

rt_err_t aic_realtek_ap_stop(struct rt_wlan_device *wlan)
{
    if (wifi_is_up(RTW_AP_INTERFACE)) {
        wifi_off();
    }

    rt_wlan_dev_indicate_event_handle(s_ap_dev, RT_WLAN_DEV_EVT_AP_STOP, NULL);

    return 0;
}

rt_err_t aic_realtek_ap_deauth(struct rt_wlan_device *wlan, unsigned char mac[])
{
    return -1;
}

rt_err_t aic_realtek_scan_stop(struct rt_wlan_device *wlan)
{
    return -1;
}

int aic_realtek_get_rssi(struct rt_wlan_device *wlan)
{
    int val = 0;
    wifi_get_rssi(&val);

    return val;
}

rt_err_t aic_realtek_set_channel(struct rt_wlan_device *wlan, int channel)
{
    return wifi_set_channel(channel);
}

int aic_realtek_get_channel(struct rt_wlan_device *wlan)
{
    int channel = 0;
    wifi_get_channel(&channel);

    return channel;
}

rt_err_t aic_realtek_set_country(struct rt_wlan_device *wlan, rt_country_code_t country_code)
{
    return -1;
}

rt_country_code_t aic_realtek_get_country(struct rt_wlan_device *wlan)
{
    return -1;
}

rt_err_t aic_realtek_set_mac(struct rt_wlan_device *wlan, rt_uint8_t *mac)
{
    return wifi_set_mac_address(mac);
}

void rtw_init_macaddr(char *mac)
{
    memcpy(rtw_mac, mac, 6);
}

rt_err_t aic_realtek_get_mac(struct rt_wlan_device *wlan, rt_uint8_t *mac)
{
    memcpy(mac, rtw_mac, 6);
    return 0;
}

int aic_realtek_recv(struct rt_wlan_device *wlan, void *buff, int len)
{
    return -1;
}

int aic_realtek_send(struct rt_wlan_device *wlan, void *buff, int len)
{
    /* initialize the one struct eth_drv_sg array */
    struct eth_drv_sg sg_list[MAX_ETH_DRV_SG];
    int sg_len = 0;
    struct pbuf *p = buff;
    struct pbuf *q;

    if(!rltk_wlan_running(0))
        return ERR_IF;

    /* packet is stored in one list composed by several pbuf. */
    for (q = p; q != NULL && sg_len < MAX_ETH_DRV_SG; q = q->next) {
        sg_list[sg_len].buf = (unsigned int) q->payload;
        sg_list[sg_len++].len = q->len;
    }

    if (sg_len)
        rltk_wlan_send(0, sg_list, sg_len, p->tot_len);

    return ERR_OK;
}

void wlan_recv(int idx, unsigned int len)
{
    int errcode;

    struct eth_drv_sg sg_list[MAX_ETH_DRV_SG];
    struct pbuf *p, *q;
    int sg_len = 0;

    /* WIFI chip is running */
    if(!rltk_wlan_running(idx))
        return;

    if ((len > MAX_ETH_MSG) || (len < 0))
        len = MAX_ETH_MSG;

    /* Allocate buffer to store received packet */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p == NULL) {
        rt_kprintf("\n\rCannot allocate pbuf to receive packet length %d,l%d.\n", len,__LINE__);
        return;
    }

    /* Create scatter list */
    for (q = p; q != NULL && sg_len < MAX_ETH_DRV_SG; q = q->next) {
        sg_list[sg_len].buf = (unsigned int) q->payload;
        sg_list[sg_len++].len = q->len;
    }

    /* Copy received packet to scatter list from wrapper RX skb */
    rltk_wlan_recv(idx, sg_list, sg_len);

    if (rt_wlan_dev_report_data(s_current_dev, p, p->len)) {
        rt_kprintf("if[%d] input pkt fail\n",idx);
        pbuf_free(p);
    }
}

const struct rt_wlan_dev_ops wlan_ops = {
    .wlan_init = aic_realtek_init,
    .wlan_mode = aic_realtek_set_mode,
    .wlan_scan = aic_realtek_scan,
    .wlan_join = aic_realtek_join,
    .wlan_softap = aic_realtek_softap,
    .wlan_disconnect = aic_realtek_disconnect,
    .wlan_ap_stop = aic_realtek_ap_stop,
    .wlan_ap_deauth = aic_realtek_ap_deauth,
    .wlan_scan_stop = aic_realtek_scan_stop,
    .wlan_get_rssi = aic_realtek_get_rssi,
    .wlan_set_channel = aic_realtek_set_channel,
    .wlan_get_channel = aic_realtek_get_channel,
    .wlan_set_country = aic_realtek_set_country,
    .wlan_get_country = aic_realtek_get_country,
    .wlan_set_mac = aic_realtek_set_mac,
    .wlan_get_mac = aic_realtek_get_mac,
    .wlan_recv = aic_realtek_recv,
    .wlan_send = aic_realtek_send,
};

int aic_realtek_wifi_device_reg(void)
{
    extern void realtek_pin_init();

    realtek_pin_init();

    s_wlan_dev = rt_malloc(sizeof(struct rt_wlan_device));
    if (!s_wlan_dev) {
        rt_kprintf("%s devcie malloc fail!\n", RT_WLAN_FLAG_STA_ONLY);
        return -1;
    }

    rt_wlan_dev_register(s_wlan_dev, RT_WLAN_DEVICE_STA_NAME, &wlan_ops, RT_WLAN_FLAG_STA_ONLY, NULL);

    s_ap_dev = rt_malloc(sizeof(struct rt_wlan_device));
    if (!s_ap_dev) {
        rt_kprintf("%s devcie malloc fail!\n", RT_WLAN_FLAG_AP_ONLY);
        return -1;
    }
    rt_wlan_dev_register(s_ap_dev, RT_WLAN_DEVICE_AP_NAME, &wlan_ops, RT_WLAN_FLAG_AP_ONLY, NULL);

    return 0;
}

INIT_COMPONENT_EXPORT(aic_realtek_wifi_device_reg);
