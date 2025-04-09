#include "asr_rtos_api.h"
#include "asr_wlan_api.h"
#include "asr_sdio.h"
#include "uwifi_defs.h"
#include <wlan_mgnt.h>
#include <rtdbg.h>
//#include "debug.h"

#define LOG_TAG "ASR_OS_API"

static int wifi_init_state = 0;
static asr_wifi_event_e wifi_cur_state = EVENT_NONE;

struct rt_wlan_device * s_wlan_dev = NULL;
struct rt_wlan_device * s_ap_dev = NULL;
static int g_rssi_avg;

static rt_err_t asr5825_disconnect(struct rt_wlan_device *wlan);

static rt_wlan_security_t asr_security2wlan(uint8_t security)
{
    rt_wlan_security_t wlan_sec = 0;

    switch(security){
        case WIFI_ENCRYP_OPEN:
            wlan_sec = SECURITY_OPEN;
            break;
        case WIFI_ENCRYP_WEP:
            wlan_sec = SECURITY_WEP_SHARED;
            break;
        case WIFI_ENCRYP_WPA:
            wlan_sec = SECURITY_WPA_AES_PSK;
            break;
        case WIFI_ENCRYP_WPA2:
            wlan_sec = SECURITY_WPA2_MIXED_PSK;
            break;
        default:
            wlan_sec = SECURITY_UNKNOWN;
            break;
    };

    //LOG_I( "security=%d,wlan_sec=0x%X", security, wlan_sec);

    return wlan_sec;
}

char *asr5825_wifi_version(void)
{
	return (char*)asr_get_wifi_version();
}

void asr_wifi_set_init_done()
{
    wifi_init_state = 1;
}

void asr_wifi_set_deinit()
{
    wifi_init_state = 0;
}

static int __wifi_init_check(void)
{
    if(wifi_init_state != 0) {
		return 1;
    }
    return 0;
}

static void asr5825_wifi_connected_report(void)
{
    LOG_I("%s: wifi connected!", __func__);
    // rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_CONNECT, NULL);
    //保存wifi info信息到flash
    //atbm_wifi_get_linkinfo_noscan((FAST_LINK_INFO *)&wifi_info_saved.bss_descriptor);
    //wifi_backup_usr_data(1, &wifi_info_saved, sizeof(wifi_info_saved));
    //LOG_I( "wifi backup usr data ok!");
}
/* EVENT PROCESS */
static void asr5825_wlan_stat_chg_event_callback(asr_wifi_event_e wlan_event)
{
    switch(wlan_event)
    {
        case EVENT_STATION_UP:
            wifi_cur_state = EVENT_STATION_UP;
            asr5825_wifi_connected_report();
            break;

        case EVENT_STATION_DOWN:
            wifi_cur_state = EVENT_STATION_DOWN;
            LOG_I("%s: EVENT_STATION_DOWN", __func__);
            rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_DISCONNECT, NULL);
            break;

        case EVENT_STA_CLOSE:
            wifi_cur_state = EVENT_STA_CLOSE;
            LOG_I("%s: EVENT_STA_CLOSE", __func__);
#if 0
            asr_gpio_init();
            asr_wlan_set_gpio_reset_pin(1);
            asr_wlan_set_gpio_reset_pin(0);
#endif
            break;

        case EVENT_AP_UP:
            wifi_cur_state = EVENT_AP_UP;
            LOG_I("%s: EVENT_AP_UP", __func__);
            rt_wlan_dev_indicate_event_handle(s_ap_dev, RT_WLAN_DEV_EVT_AP_START, NULL);
            break;

        case EVENT_AP_DOWN:
            wifi_cur_state = EVENT_AP_DOWN;
            LOG_I("%s: EVENT_AP_DOWN", __func__);
            rt_wlan_dev_indicate_event_handle(s_ap_dev, RT_WLAN_DEV_EVT_AP_STOP, NULL);
            break;

        default:
            break;
    }
}

static void asr5825_wlan_err_stat_callback(asr_wlan_err_status_e err_info)
{
    LOG_I("%s: err_info = %d", __func__, err_info);
    switch(err_info)
    {
        case WLAN_STA_MODE_USER_CLOSE:
            break;
        case WLAN_STA_MODE_BEACON_LOSS:
            break;
        case WLAN_STA_MODE_AUTH_FAIL:
            break;
        case WLAN_STA_MODE_ASSOC_FAIL:
            break;
        case WLAN_STA_MODE_PASSWORD_ERR:
            break;
        case WLAN_STA_MODE_NO_AP_FOUND:
            break;
        case WLAN_STA_MODE_DHCP_FAIL:
            break;
        case WLAN_STA_MODE_CONN_RETRY_MAX:
            break;
        case WLAN_STA_MODE_CONN_REASON_0:
            break;
        case WLAN_STA_MODE_CONN_TIMEOUT:
            break;
        case WLAN_STA_MODE_CONN_AP_CHG_PSK:
            break;
        case WLAN_STA_MODE_CONN_ENCRY_ERR:
            break;
        default:
            break;
    }
}

static void asr5825_callback_ip_got(asr_wlan_ip_stat_t *ip_status)
{
    LOG_I("%s", __func__);
}

static void asr5825_associated_ap(asr_wlan_ap_info_adv_t *ap_info)
{
    struct rt_wlan_buff buff;
    struct rt_wlan_info sta_info;

    memset(&sta_info, 0, sizeof(sta_info));
    memcpy(sta_info.bssid, ap_info->bssid, 6);
    memcpy(sta_info.ssid.val, ap_info->ssid, ap_info->ssid_len);
    sta_info.channel = ap_info->channel;
    sta_info.band = RT_802_11_BAND_2_4GHZ;
    sta_info.rssi = g_rssi_avg;
    sta_info.security = asr_security2wlan(ap_info->security);

    buff.len = sizeof(sta_info);
    buff.data = (void *)&sta_info;

    LOG_I("%s: %s,%02X:%02X:%02X:%02X:%02X:%02X,%d,%d",
        __func__, ap_info->ssid, sta_info.bssid[0], sta_info.bssid[1], sta_info.bssid[2],
        sta_info.bssid[3], sta_info.bssid[4], sta_info.bssid[5], ap_info->channel, ap_info->security);

    rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_CONNECT, &buff);
}

static void asr5825_ap_add_dev(char* mac)
{
    struct rt_wlan_buff buff;
    struct rt_wlan_info sta_info;

    memset(&sta_info, 0, sizeof(sta_info));
    memcpy(sta_info.bssid, mac, 6);

    buff.len = sizeof(sta_info);
    buff.data = (void *)&sta_info;

    LOG_I("%s: %02X:%02X:%02X:%02X:%02X:%02X",
        __func__,  sta_info.bssid[0], sta_info.bssid[1], sta_info.bssid[2], sta_info.bssid[3], sta_info.bssid[4], sta_info.bssid[5]);

    rt_wlan_dev_indicate_event_handle(s_ap_dev, RT_WLAN_DEV_EVT_AP_ASSOCIATED, &buff);
}

static void asr5825_ap_del_dev(char* mac)
{
    struct rt_wlan_buff buff;
    struct rt_wlan_info sta_info;

    memset(&sta_info, 0, sizeof(sta_info));
    memcpy(sta_info.bssid, mac, 6);

    buff.len = sizeof(sta_info);
    buff.data = (void *)&sta_info;

    LOG_I("%s: %02X:%02X:%02X:%02X:%02X:%02X",
        __func__,  sta_info.bssid[0], sta_info.bssid[1], sta_info.bssid[2], sta_info.bssid[3], sta_info.bssid[4], sta_info.bssid[5]);

    rt_wlan_dev_indicate_event_handle(s_ap_dev, RT_WLAN_DEV_EVT_AP_DISASSOCIATED, &buff);
}

static void asr5825_get_rssi_callback(uint8_t rssi_level)
{
    g_rssi_avg = rssi_level;
}

static int asr5825_get_rssi(struct rt_wlan_device *wlan)
{
    if(!__wifi_init_check())
        return -1;

    LOG_I( "%s: rssi=%d", __func__, g_rssi_avg);
    return g_rssi_avg;
}

static void asr5825_scan_compeleted_callback(asr_wlan_scan_result_t *result)
{
    struct rt_wlan_buff buff;
	struct rt_wlan_info *scan_list = (struct rt_wlan_info *)asr_rtos_malloc(sizeof(struct rt_wlan_info));
    int i;

    for(i = 0; i < result->ap_num; i++) {
        memset(scan_list, 0, sizeof(struct rt_wlan_info));
        scan_list->band = RT_802_11_BAND_2_4GHZ;
        scan_list->channel  = (result->ap_list + i)->channel;
        scan_list->rssi     = (result->ap_list + i)->ap_power;
        scan_list->security = asr_security2wlan((result->ap_list + i)->security);
        memcpy((char *)scan_list->bssid, (result->ap_list + i)->bssid, RT_WLAN_BSSID_MAX_LENGTH);

        scan_list->ssid.len = strlen((result->ap_list + i)->ssid);
        memcpy(scan_list->ssid.val, (result->ap_list + i)->ssid, RT_WLAN_SSID_MAX_LENGTH);

        LOG_I( "ap[%d] channel=%d,rssi=%d,security=%d,ssid=%s",
            i, scan_list->channel, scan_list->rssi, (result->ap_list + i)->security, scan_list->ssid.val);

        buff.len = sizeof(struct rt_wlan_info);
		buff.data = (void *)scan_list;
		rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_SCAN_REPORT, &buff);
    }

    rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_SCAN_DONE, NULL);
    asr_rtos_free(scan_list);
}

static rt_err_t asr5825_wlan_init(struct rt_wlan_device *wlan)
{
    int ret;

    LOG_I( "asr5825s wifi version:%s", (char *)asr5825_wifi_version());

    if(__wifi_init_check()) {
        LOG_E(" wifi has inited ");
        return 0;
    }

	ret = asr_sdio_register_driver();
    if (ret) {
        LOG_E("asr wifi init failed!");
        return 0;
    }

    asr_wlan_rssi_cb_register(asr5825_get_rssi_callback);
    asr_wlan_scan_compeleted_cb_register(asr5825_scan_compeleted_callback);
    asr_wlan_stat_chg_cb_register(asr5825_wlan_stat_chg_event_callback);
    asr_wlan_ip_got_cb_register(asr5825_callback_ip_got);
    asr_wlan_err_stat_cb_register(asr5825_wlan_err_stat_callback);
    asr_wlan_associated_ap_cb_register(asr5825_associated_ap);
    asr_wlan_ap_add_dev_cb_register(asr5825_ap_add_dev);
    asr_wlan_ap_del_dev_cb_register(asr5825_ap_del_dev);

    asr_wifi_set_init_done();

    LOG_I( "%s: success.", __func__);

	return 0;
}

static rt_err_t asr5825_wlan_mode(struct rt_wlan_device *wlan, rt_wlan_mode_t mode)
{
    int ret;
    asr_wlan_init_type_t init_info = {0};

    if(!__wifi_init_check())
            return -1;

    if (mode == RT_WLAN_AP) {
        init_info.wifi_mode = SOFTAP;
        /* not trigger lower interface, this action only change upper layer app mode status. */
        LOG_I( "%s: change to SOFTAP mode", __func__);
        return 0;
    } else if (mode == RT_WLAN_STATION) {
        init_info.wifi_mode = STA;
    } else if (mode == RT_WLAN_NONE) {
        init_info.wifi_mode = NONE;
        asr5825_disconnect(wlan);

        return 0;
    } else {
        LOG_E("%s: other mode(%d)", __func__, mode);
        return -1;
    }

    ret = asr_wlan_open(&init_info);
    if (ret) {
        LOG_E("%s: mode=%d failed", __func__, mode);
        return -1;
    }
    LOG_I( "%s: mode=%d success", __func__, mode);
	return 0;
}

static rt_err_t asr5825_scan(struct rt_wlan_device *wlan, struct rt_scan_info *scan_info)
{
    int ret;

    if(scan_info){
        rt_kprintf("scan ssid:%s, len:%d",scan_info->ssid.val,scan_info->ssid.len);
    }

    if(!__wifi_init_check())
        return -1;

    ret = asr_wlan_start_scan();
    if(ret) {
        LOG_E("asr5825 scan failed");
        return -1;
    }
    return 0;
}

static rt_err_t asr5825_wlan_join(struct rt_wlan_device *wlan, struct rt_sta_info *sta_info)
{
    int ret;
    asr_wlan_init_type_t init_info = {0};

    if(!__wifi_init_check())
        return -1;

    LOG_I( "%s: ssid=%s", __func__, sta_info->ssid.val);

    //判断是否已连接，若是，是否需要先断开连接？
    //asr_wlan_get_link_status
    if (wifi_cur_state != EVENT_STA_CLOSE) {
        LOG_I( "disconnect first");
        asr_wlan_suspend_sta();
    }

    memset(&init_info, 0, sizeof(asr_wlan_init_type_t));
    init_info.wifi_mode = STA;
    strncpy(init_info.wifi_key, (char *)sta_info->key.val, sta_info->key.len);
    strncpy(init_info.wifi_ssid, (char *)sta_info->ssid.val, sta_info->ssid.len);
    strcpy(init_info.mac_addr, (char *)sta_info->bssid);
    init_info.channel = sta_info->channel;
    init_info.security = WLAN_SECURITY_AUTO;
    init_info.en_autoconnect = AUTOCONNECT_SCAN_CONNECT;
    // init_info.dhcp_mode = WLAN_DHCP_DISABLE;
    // init_info.wifi_retry_times = UWIFI_AUTOCONNECT_DEFAULT_TRIES;
    ret = asr_wlan_open(&init_info);
    if (ret) {
        LOG_E("%s: failed ret=%d", __func__, ret);
        return -1;
    }
    LOG_I( "%s: success", __func__);
    return 0;
}

static rt_err_t asr5825_wlan_softap(struct rt_wlan_device *wlan, struct rt_ap_info *ap_info)
{
    int ret;
    asr_wlan_ap_init_t init_ap_info = {0};

    if(!__wifi_init_check())
        return -1;

    strncpy(init_ap_info.ssid, (char *)ap_info->ssid.val, ap_info->ssid.len);
    strncpy(init_ap_info.pwd, (char *)ap_info->key.val, ap_info->key.len);
    init_ap_info.interval = 100;
    init_ap_info.hide = ap_info->hidden;
    init_ap_info.channel = ap_info->channel;

    ret = asr_wlan_ap_open(&init_ap_info);
    if (ret) {
        LOG_E("%s: start failed", __func__);
        return -1;
    }
    LOG_I("%s: start success", __func__);

	rt_wlan_dev_indicate_event_handle(s_ap_dev, RT_WLAN_DEV_EVT_AP_START, NULL);

    return 0;
}

static rt_err_t asr5825_disconnect(struct rt_wlan_device *wlan)
{

    if(!__wifi_init_check())
        return -1;

    asr_wlan_close();

    rt_wlan_dev_indicate_event_handle(s_wlan_dev, RT_WLAN_DEV_EVT_DISCONNECT, NULL);

    return 0;
}

static rt_err_t asr5825_ap_stop(struct rt_wlan_device *wlan)
{
    int ret;

    if(!__wifi_init_check())
        return -1;

    ret = asr_wlan_close();
    if (ret) {
        LOG_I( "%s: stop failed", __func__);
        return -1;
    }
    rt_wlan_dev_indicate_event_handle(s_ap_dev, RT_WLAN_DEV_EVT_AP_STOP, NULL);
	return 0;
}

static rt_err_t asr5825_ap_deauth(struct rt_wlan_device *wlan, unsigned char mac[])
{
    //return atbm_ap_deauth_sta(mac);
    return -1;
}

static rt_err_t asr5825_scan_stop(struct rt_wlan_device *wlan)
{
    LOG_E(" %s not support ", __FUNCTION__);
    return -1;
}

static rt_err_t asr5825_set_channel(struct rt_wlan_device *wlan, int channel)
{
    LOG_E(" %s not support ", __FUNCTION__);
    return -1;
}

static int asr5825_get_channel(struct rt_wlan_device *wlan)
{
    LOG_E(" %s not support ", __FUNCTION__);
    return -1;
}

static rt_err_t asr5825_set_country(struct rt_wlan_device *wlan, rt_country_code_t country_code)
{
    char code_str[20];

    if(!__wifi_init_check())
        return -1;

    //itoa(country_code, code_str, 10);
    sprintf(code_str, "%d", country_code);
    asr_wlan_set_country_code(code_str);
    return 0;
}

static rt_country_code_t asr5825_get_country(struct rt_wlan_device *wlan)
{
    return -1;
}

static rt_err_t asr5825_set_mac(struct rt_wlan_device *wlan, unsigned char *mac)
{
    if(!__wifi_init_check())
        return -1;

    asr_wlan_set_mac_address(mac);
    return 0;
}

extern struct asr_hw g_asr_hw;

static rt_err_t asr5825_get_mac(struct rt_wlan_device *wlan, unsigned char *mac)
{
    if(!__wifi_init_check())
        return -1;

    memcpy(mac, g_asr_hw.mac_addr, sizeof(g_asr_hw.mac_addr));

    LOG_I("%s: %02X:%02X:%02X:%02X:%02X:%02X",
        __func__,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

    return 0;
}

static int asr5825_recv(struct rt_wlan_device *wlan, void *buff, int len)
{
    LOG_I("%s: not support ", __func__);
    return -1;
}

static int asr5825_send(struct rt_wlan_device *wlan, void *buff, int len)
{
#ifdef WLAN_PROT_LWIP_PBUF_FORCE
    struct pbuf *p = (struct pbuf *)buff;
    unsigned char *frame;
    int ret = 0;

    /* sending data directly */
    if (p->len == p->tot_len)
    {
        frame = (unsigned char *)p->payload;
        ret = uwifi_xmit_buf((void *)frame, p->tot_len);

        //LOG_E("%s: direct tx,len=%d", __func__, p->tot_len);
        return ret;
    }

    frame = malloc(p->tot_len);
    if (frame == NULL)
    {
        LOG_E("%s: malloc buf fail,len=%d", __func__, p->tot_len);
        return -1;
    }

    /*copy pbuf -> data dat*/
    pbuf_copy_partial(p, frame, p->tot_len, 0);
    ret = uwifi_xmit_buf((void *)frame, p->tot_len);

    //LOG_I( "%s: tx,len=%d", __func__, p->tot_len);
    free(frame);

    return ret;
#else
    return uwifi_xmit_buf(buff, len);
#endif
}

static const struct rt_wlan_dev_ops wlan_ops = {
    .wlan_init = asr5825_wlan_init,
    .wlan_mode = asr5825_wlan_mode,
    .wlan_scan = asr5825_scan,
    .wlan_join = asr5825_wlan_join,
    .wlan_softap = asr5825_wlan_softap,
    .wlan_disconnect = asr5825_disconnect,
    .wlan_ap_stop = asr5825_ap_stop,
    .wlan_ap_deauth = asr5825_ap_deauth,
    .wlan_scan_stop = asr5825_scan_stop,
    .wlan_get_rssi = asr5825_get_rssi,
    .wlan_set_channel = asr5825_set_channel,
    .wlan_get_channel = asr5825_get_channel,
    .wlan_set_country = asr5825_set_country,
    .wlan_get_country = asr5825_get_country,
    .wlan_set_mac = asr5825_set_mac,
    .wlan_get_mac = asr5825_get_mac,
    .wlan_recv = asr5825_recv,
    .wlan_send = asr5825_send,
};

int rt_wifi_device_reg(void)
{
    int ret = -1;

    asr_gpio_init();

    asr_wlan_set_gpio_reset_pin(0);
    asr_msleep(10);
    asr_wlan_set_gpio_reset_pin(1);
    asr_msleep(10);

    if(!s_wlan_dev)
    {
        s_wlan_dev = rt_malloc(sizeof(struct rt_wlan_device));
        if(!s_wlan_dev)
        {
            LOG_E("wlan0 devcie malloc fail!");
            return -1;
        }
    }

    ret = rt_wlan_dev_register(s_wlan_dev, "e2", &wlan_ops, RT_WLAN_FLAG_STA_ONLY, NULL);
    if(ret != 0)
    {
        LOG_E("rt_wlan_dev_register wl fail!");
        return -1;
    }
    LOG_I( "F:%s L:rt_wlan_dev_register e2 success!!! ", __FUNCTION__);

    if(!s_ap_dev)
    {
        s_ap_dev = rt_malloc(sizeof(struct rt_wlan_device));
        if(!s_ap_dev)
        {
            LOG_E("ap0 devcie malloc fail!");
            return -1;
        }
    }

    ret = rt_wlan_dev_register(s_ap_dev, "ap", &wlan_ops, RT_WLAN_FLAG_AP_ONLY, NULL);
    if(ret != 0)
    {
        LOG_E("rt_wlan_dev_register ap fail!");
        return -1;
    }
    LOG_I( "F:%s L:rt_wlan_dev_register ap success!!! ", __FUNCTION__);
	return 0;
}
INIT_COMPONENT_EXPORT(rt_wifi_device_reg);

void asr_srand(unsigned seed)
{

}

int asr_rand(void)
{
    return rt_tick_get();
}
