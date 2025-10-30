#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <rtthread.h>
#include "wlan_dev.h"
#include "atbm_config.h"
#include "atbm_hal.h"
#include "atbm_wifi_driver_api.h"

extern struct atbmwifi_common g_hw_prv;
struct rt_wlan_device * s_wlan_dev = NULL;
struct rt_wlan_device * s_ap_dev = NULL;

rt_err_t atbm6031_init(struct rt_wlan_device *wlan)
{
    if(g_hw_prv.init_done == 1){
        return 0;
    }
	return atbm_wifi_hw_init();
}

rt_err_t atbm6031_set_mode(struct rt_wlan_device *wlan, rt_wlan_mode_t mode)
{
    if(mode == RT_WLAN_AP) {
        atbm_wifi_on(ATBM_WIFI_AP_MODE);
    } else if ( mode == RT_WLAN_STATION){
        atbm_wifi_on(ATBM_WIFI_STA_MODE);
    }
    iot_printf("atbm6031_set_mode %d \n", mode);
    return 0;
}

static ATBM_SECURITY_TYPE wlan_security2atbm(rt_wlan_security_t security)
{
    ATBM_SECURITY_TYPE type = ATBM_KEY_NONE;
    switch(security)
    {
    case SECURITY_OPEN:
        type = ATBM_KEY_NONE;
        break;
    case SECURITY_WEP_PSK:
        type = ATBM_KEY_WEP;
        break;
    case SECURITY_WEP_SHARED:
        type = ATBM_KEY_WEP_SHARE;
        break;
    case SECURITY_WPA_AES_PSK:
        type = ATBM_KEY_WPA;
        break;
    case SECURITY_WPA2_AES_PSK:
        type = ATBM_KEY_WPA2;
        break;
    case SECURITY_WPA2_MIXED_PSK:
        type = ATBM_KEY_MIX;
        break;
    default:
        type = ATBM_KEY_NONE;
        break;
    }
    return type;
}

rt_err_t atbm6031_scan(struct rt_wlan_device *wlan, struct rt_scan_info *scan_info)
{
    struct atbmwifi_vif *priv=_atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
    atbm_wifi_on(ATBM_WIFI_STA_MODE);
    if(scan_info){
        iot_printf("scan ssid:%s, len:%d\n",scan_info->ssid.val,scan_info->ssid.len);
        priv->ssid_length=scan_info->ssid.len;
        rt_memcpy(priv->ssid,scan_info->ssid.val,priv->ssid_length);
    }
    return atbmwifi_scan_process(priv);
}

rt_err_t atbm6031_join(struct rt_wlan_device *wlan, struct rt_sta_info *sta_info)
{
    wifi_printk(WIFI_ALWAYS, "atbm603x_join...\n");
    atbm_wifi_on(ATBM_WIFI_STA_MODE);
    return atbm_wifi_sta_join_ap(sta_info->ssid.val, sta_info->bssid, WLAN_MIX_AUTH_PSK, 0,sta_info->key.val);
}


rt_err_t atbm6031_softap(struct rt_wlan_device *wlan, struct rt_ap_info *ap_info)
{
    int ssidlen = ap_info->ssid.len;
    int passwdlen = ap_info->key.len;
    int key_mgmt=wlan_security2atbm(ap_info->security);
    atbm_wifi_on(ATBM_WIFI_AP_MODE);
    wifi_StartAP_vif(1,(atbm_uint8 *)ap_info->ssid.val, ssidlen, (atbm_uint8 *)ap_info->key.val, passwdlen, ap_info->channel, key_mgmt,
            ap_info->hidden);

    wifi_printk(WIFI_ALWAYS, " %s softap start ...... \n", __FUNCTION__);
    return 0;
}

rt_err_t atbm6031_disconnect(struct rt_wlan_device *wlan)
{
    atbm_wifi_sta_disjoin_ap();
    rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_DISCONNECT, NULL);
    return 0;
}

rt_err_t atbm6031_ap_stop(struct rt_wlan_device *wlan)
{
    atbm_wifi_off(ATBM_WIFI_AP_MODE);
	return 0;
}

rt_err_t atbm6031_ap_deauth(struct rt_wlan_device *wlan, unsigned char mac[])
{
    return -1;
}

rt_err_t atbm6031_scan_stop(struct rt_wlan_device *wlan)
{
    return -1;
}

extern atbm_int32 atbm_wifi_get_rssi_avg(atbm_void);
int atbm6031_get_rssi(struct rt_wlan_device *wlan)
{
    return atbm_wifi_get_rssi_avg();
}

rt_err_t atbm6031_set_channel(struct rt_wlan_device *wlan, int channel)
{
    return -1;
}

int atbm6031_get_channel(struct rt_wlan_device *wlan)
{
	struct atbmwifi_vif *priv=_atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
    return priv->bss.channel_num;
}

rt_err_t atbm6031_set_country(struct rt_wlan_device *wlan, rt_country_code_t country_code)
{
    return -1;
}

rt_country_code_t atbm6031_get_country(struct rt_wlan_device *wlan)
{
    return -1;
}

rt_err_t atbm6031_set_mac(struct rt_wlan_device *wlan, rt_uint8_t *mac)
{
    return -1;
}
extern atbm_void atbm_wifi_get_mac_address_vif(atbm_uint8 if_id,unsigned char *addr);
rt_err_t atbm6031_get_mac(struct rt_wlan_device *wlan, rt_uint8_t *mac)
{
    atbm_wifi_get_mac_address_vif(wlan->mode == RT_WLAN_AP ? 1:0, mac);
    return 0;
}

void atbm6031_wifi_input(void *buf, int len, int if_id)
{
    struct rt_wlan_device *wlan_dev=NULL;
    wlan_dev=if_id ? s_ap_dev:s_wlan_dev;

    if (rt_wlan_dev_report_data(wlan_dev, buf, len)){
        rt_kprintf("if[%d] input pkt fail\n",if_id);
    }
}

int atbm6031_recv(struct rt_wlan_device *wlan, void *buff, int len)
{
    return -1;
}

int atbm6031_send(struct rt_wlan_device *wlan, void *buff, int len)
{
    return atbm_wifi_tx_pkt((atbm_int8 *)buff, len, wlan->mode == RT_WLAN_STATION ? 0 : 1);
}

const struct rt_wlan_dev_ops wlan_ops = {
    .wlan_init = atbm6031_init,
    .wlan_mode = atbm6031_set_mode,
    .wlan_scan = atbm6031_scan,
    .wlan_join = atbm6031_join,
    .wlan_softap = atbm6031_softap,
    .wlan_disconnect = atbm6031_disconnect,
    .wlan_ap_stop = atbm6031_ap_stop,
    .wlan_ap_deauth = atbm6031_ap_deauth,
    .wlan_scan_stop = atbm6031_scan_stop,
    .wlan_get_rssi = atbm6031_get_rssi,
    .wlan_set_channel = atbm6031_set_channel,
    .wlan_get_channel = atbm6031_get_channel,
    .wlan_set_country = atbm6031_set_country,
    .wlan_get_country = atbm6031_get_country,
    .wlan_set_mac = atbm6031_set_mac,
    .wlan_get_mac = atbm6031_get_mac,
    .wlan_recv = atbm6031_recv,
    .wlan_send = atbm6031_send,
};

int wifi_device_reg(void)
{
    s_wlan_dev = rt_malloc(sizeof(struct rt_wlan_device));
    if (!s_wlan_dev){
        rt_kprintf("wlan0 devcie malloc fail!");
        return -1;
    }

    rt_wlan_dev_register(s_wlan_dev, "wlan0", &wlan_ops, RT_WLAN_FLAG_STA_ONLY, NULL);

    s_ap_dev = rt_malloc(sizeof(struct rt_wlan_device));
    if (!s_ap_dev){
        rt_kprintf("ap0 devcie malloc fail!");
        return -1;
    }
    rt_wlan_dev_register(s_ap_dev, "ap0", &wlan_ops, RT_WLAN_FLAG_AP_ONLY, NULL);
    return 0;
}

INIT_COMPONENT_EXPORT(wifi_device_reg);

