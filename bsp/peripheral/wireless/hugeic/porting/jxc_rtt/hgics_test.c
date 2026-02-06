#include "hgic.h"
#include "os_porting.h"
#include <rtthread.h>
#include <string.h>

#if 0
#define AUTH_MODE WPA_AUTH_WPA2_AES

#define BLENC_STA   0//1
#define HGIC_BLE_TEST_MODE  0//2

#define CONFIG_BUFF_SIZE (1024)
#define REPLY_BUFF_SIZE (4096)

#ifndef MAC2STR
#define MAC2STR(a) (a)[0]&0xff, (a)[1]&0xff, (a)[2]&0xff, (a)[3]&0xff, (a)[4]&0xff, (a)[5]&0xff
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define COMPACT_MACSTR "%02x%02x%02x%02x%02x%02x"
#endif

#define hgic_dbg(fmt, ...) rt_kprintf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define hgic_err(fmt, ...) rt_kprintf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

extern void hgics_recv_blenc_data(unsigned char *data, unsigned int len);
#define NULL 0

typedef enum {
    WLAN_STATION = 0,
    WLAN_AP,
} WIFI_WORK_MODE;

typedef enum {
    WPA_AUTH_NONE = 0,
    WPA_AUTH_WPA1_TKIP,
    WPA_AUTH_WPA1_AES,
    WPA_AUTH_WPA2_TKIP,
    WPA_AUTH_WPA2_AES,
    WPA_AUTH_WPA2_TKIP_AES,
    WPA_AUTH_WPA3_SAE,
} WPA_AUTH_MODE;

struct umac_config {
    unsigned char  hg0[1024];
    unsigned char  hg1[1024];
};

struct wifi_state {
    unsigned int drv_inited;
    //unsigned int connected;
    unsigned int mode;
    struct umac_config umac_cfg;
};

static struct wifi_state g_wifi_state = {0};

static void str2mac(char *dst, char *src)
{
    int i = 0;
    while (i < 6) {
        if (' ' == *src || ':' == *src || '"' == *src || '\'' == *src) {
            src++;
            continue;
        }
        *(dst + i) = ((hex2num(*src) << 4) | hex2num(*(src + 1)));
        i++;
        src += 2;
    }
}

void *hgics_configs_get()
{
    return (void *)ZALLOC(128);
}

void hgics_configs_put(void *conf)
{
    if (conf) {
        FREE(conf);
    }
}

extern void tcpip_test(unsigned int mode);

void hgic_smac_event_cb(char * ifname, int event, int param1, int param2)
{
    switch (event) {
        case HGIC_EVENT_CONECTED:
            hgic_dbg("%s:"MACSTR" Connected,mode:%s\n",__FUNCTION__,MAC2STR((char *)param1),
                g_wifi_state.mode == WLAN_STATION? "STA" : "AP");
            if(g_wifi_state.mode == WLAN_STATION) {
                //sta
            } else {
                //dhcps_init();
            }
            break;
        case HGIC_EVENT_DISCONECTED:
            hgic_dbg("%s:Disconnected\n",__FUNCTION__);
            break;
        case HGIC_EVENT_BLENC_DATA:
            hgics_recv_blenc_data(param1, param2);
            break;
        case HGIC_EVENT_HGIC_DATA:
            hgics_proc_bt_data(param1, param2);
            break;
    }
}

void hgic_smac_init_cb(void *args)
{
    int ret = 0;
    if(g_wifi_state.mode == WLAN_STATION) {
        wpas_start("w0");
#if BLENC_STA
        hgic_blenc_init();
        hgics_blenc_test(HGIC_BLE_TEST_MODE, 100);
#endif

    } else if (g_wifi_state.mode == WLAN_AP) {
        hapd_start("w0");
    } else {
        hgic_err("Unsupport mode:%d\n",g_wifi_state.mode);
    }

    rt_kprinft("***%s:hgics init done!\r\n",__FUNCTION__);
}


static struct umac_config *sys_umaccfg_init(unsigned int auth_mode)
{
    struct umac_config *cfg = &g_wifi_state.umac_cfg;

    memset(cfg->hg0,0,sizeof(cfg->hg0));
    if(g_wifi_state.mode == WLAN_STATION) {
        strcpy(cfg->hg0, "update_config=1\n" \
           "network={\n" \
           "proto=WPA RSN\n"\
           "scan_ssid=1\n" \
           "ssid=\"JL_RTOS\"\n" \
           "key_mgmt=WPA-PSK\n" \
           "pairwise=CCMP TKIP\n" \
           "group=CCMP TKIP\n" \
           "psk=\"12345678\"\n" \
           "}\n");
    } else if (g_wifi_state.mode == WLAN_AP) {
        rt_kprinft("AP:Auth_mode:%d\n", auth_mode);
        if (auth_mode == WPA_AUTH_NONE) {
            strcpy(cfg->hg0, "country_code=CN\n" \
               "ssid=JLRTOS_AP\n" \
               "channel=5\n" \
               "wpa=0\n" \
               "hw_mode=g\n"\
               "ieee80211n=1\n" \
               "ht_capab=\n"\
              );
        } else if (auth_mode == WPA_AUTH_WPA2_AES) {
            strcpy(cfg->hg0, "country_code=CN\n" \
               "ssid=JLRTOS_AP\n" \
               "channel=9\n" \
               "wpa=2\n" \
               "wpa_key_mgmt=WPA-PSK\n" \
               "wpa_pairwise=CCMP\n" \
               "wpa_passphrase=12345678\n"\
               "hw_mode=g\n"\
               "ieee80211n=1\n" \
               "ht_capab=\n"\
              );
        }  else if(auth_mode == WPA_AUTH_WPA3_SAE) {
           strcpy(cfg->hg0, "country_code=CN\n" \
               "ssid=JLRTOS_AP\n" \
               "channel=5\n" \
               "wpa=2\n" \
               "wpa_key_mgmt=SAE\n" \
               "wpa_pairwise=CCMP\n" \
               "sae_password=12345678\n" \
               "hw_mode=g\n"\
               "ieee80211n=1\n" \
               "ieee80211w=2\n" \
               "ht_capab=\n"\
			);
        } else if (auth_mode == WPA_AUTH_WPA2_TKIP_AES) {
            strcpy(cfg->hg0, "update_config=1\n" \
               "country_code=CN\n" \
               "ssid=akailos_ap\n" \
               "channel=5\n" \
               "wpa=2\n" \
               "wpa_key_mgmt=WPA-PSK\n" \
               "wpa_pairwise=CCMP TKIP\n" \
               "wpa_passphrase=12345678\n"\
               "hw_mode=g\n"\
               "ieee80211n=1\n" \
               "ht_capab=\n" \
            );
       } else {
            strcpy(cfg->hg0, "update_config=1\n" \
               "country_code=CN\n" \
               "ssid=hgic_bgn_test\n" \
               "channel=5\n" \
               "wpa=0\n" \
               "hw_mode=g\n"\
               "ieee80211n=1\n" \
               "ht_capab=\n"\
              );
        }
    } else {
        hgic_err("Unsupport mode:%d\n",g_wifi_state.mode);

    }
    return cfg;
}

struct umac_config *umac_configs(void)
{
    return &g_wifi_state.umac_cfg;
}

int sys_save_umaccfg(struct umac_config *cfg)
{
    ;//do nothing
}

static unsigned int __wifi_init_check(void)
{
    if(g_wifi_state.drv_inited == 0) {
        hgic_err("WIFI Driver not inited!\n");
    }
    return g_wifi_state.drv_inited;
}

static void __wifi_init_done(void)
{
    g_wifi_state.drv_inited = 1;
}

static void __wifi_deinit_done(void)
{
    g_wifi_state.drv_inited = 0;
}

#if 0
#define wlan_dev        wlan_device
#define wlan_dev_ops    wlan_dev_ops

static unsigned int __wifi_init_check(void)
{
    if(g_wifi_state.drv_inited == 0) {
        hgic_err("WIFI Driver not inited!\n");
    }
    return g_wifi_state.drv_inited;
}

static void __wifi_init_done(void)
{
    g_wifi_state.drv_inited = 1;
}

static void __wifi_deinit_done(void)
{
    g_wifi_state.drv_inited = 0;
}

static void sta_connect_fail(void)
{
    g_wifi_state.connected=0;
	wlan_dev_indicate_event_handle(g_wifi_state.s_wlan_dev, WLAN_DEV_EVT_CONNECT_FAIL, NULL);
}

static void wifi_discon_report(void)
{
    g_wifi_state.connected=0;
    wlan_dev_indicate_event_handle(g_wifi_state.s_wlan_dev, WLAN_DEV_EVT_DISCONNECT, NULL);
}

static void ap_stop_indicate(void)
{
    wlan_dev_indicate_event_handle(g_wifi_state.s_wlan_dev, WLAN_DEV_EVT_AP_STOP, NULL);
}

static void wifi_key_error_indicate(void)
{
    struct wlan_buff buff;
    int reason=(1UL << 4);
    buff.len = sizeof(reason);
	buff.data = (void *)&reason;
    g_wifi_state.connected=0;
    wlan_dev_indicate_event_handle(g_wifi_state.s_wlan_dev, WLAN_DEV_EVT_CONNECT_FAIL, &buff);
}

int hgic_txw840x_get_status(char *ifname,char *pstatus,int buff_len)
{
    char *buff = NULL;
    if(!__wifi_init_check())
        return -1;

    buff = MALLOC(REPLY_BUFF_SIZE);
    if(buff == NULL) {
        hgic_err("Malloc buffer failed!\n");
        return -2;
    }
    memset(buff,0,REPLY_BUFF_SIZE);

    if(g_wifi_state.mode == WLAN_AP)
        hgics_hapdcli_get_status(ifname, buff, REPLY_BUFF_SIZE);
    else
        hgics_wpacli_get_status(ifname, buff, REPLY_BUFF_SIZE);
    if(pstatus) {
        strncpy(pstatus,buff,strlen(buff) > buff_len ? buff_len : strlen(buff));
    }
    FREE(buff);
    return 0;
}

/*
STATUS FORMART:

bssid=5c:de:34:79:4e:35
freq=2452
ssid=akoswpa2
id=0
mode=station
wifi_generation=4
pairwise_cipher=CCMP
group_cipher=CCMP
key_mgmt=WPA2-PSK
wpa_state=COMPLETED
ip_address=$PO
address=d8:83:32:c1:0c:58

*/
static int ak_alios_parse_wifi_state(const char *buff,struct wlan_info *info)
{
    char *pdata   = NULL;
    char *elem_buf = NULL;
    char *ptemp = NULL;

    if(buff == NULL || info == NULL || strlen(buff) == 0) {
        hgic_err("Input param error!\n");
        return -1;
    }

    elem_buf = MALLOC(STATE_STR_CUTDOWN_BUFFER_LEN);
    if(elem_buf == NULL) {
        hgic_err("Error,no memory!\n");
        return -1;
    }
    memset(elem_buf,0,STATE_STR_CUTDOWN_BUFFER_LEN);

    pdata = strstr(buff, "bssid=");
    if(pdata) {
        hgics_str_cutdown(pdata, '\n', elem_buf, STATE_STR_CUTDOWN_BUFFER_LEN);
        if(strlen(elem_buf)){
            str2mac(info->bssid, elem_buf + 6);
            hgic_dbg("bssid:"MACSTR"\n",MAC2STR(info->bssid));//format:bssid=AP_MAC
        }
    }

    pdata = strstr(buff,"freq=");
    if(pdata){
        hgics_str_cutdown(pdata, '\n', elem_buf, STATE_STR_CUTDOWN_BUFFER_LEN);
        if(strlen(elem_buf)){
            info->channel = atoi(elem_buf+5);//format:freq=2412...
            hgic_dbg("channel:%d\n",info->channel);
        }
    }

    ptemp = pdata;
    if(ptemp) {
        pdata = strstr(ptemp ,"ssid=");
        if(pdata){
            hgics_str_cutdown(pdata, '\n', elem_buf, STATE_STR_CUTDOWN_BUFFER_LEN);
            if(strlen(elem_buf)){
                strncpy(info->ssid.val, elem_buf + 5,
                    strlen(elem_buf + 5) > sizeof(info->ssid.val) ?
                    sizeof(info->ssid.val) : strlen(elem_buf + 5));
                info->ssid.len = strlen(info->ssid.val);//format:ssid=anyka_ap
                hgic_dbg("ssid:%s,len:%d\n",info->ssid.val,info->ssid.len);
            }
        }
    }

    FREE(elem_buf);
    return 0;
}

static void  wifi_connected_report()
{
    unsigned long time;
    unsigned char *result_buff = NULL;
    time = drv_hw_get_ms();
    struct wlan_buff buff = {0};
    struct wlan_info info = {0};

    result_buff = MALLOC(4096);
    if(!result_buff) {
        hgic_err("malloc buff failed!\n");
        return;
    }
    memset(result_buff,0,4096);
    hgics_wpacli_get_status("w0", result_buff, 4096);

    memset(&info,0,sizeof(struct wlan_info));
    hgics_wpacli_get_status("w0", result_buff, 4096);
    ak_alios_parse_wifi_state(result_buff, &info);

    buff.len = sizeof(struct wlan_info);//temp
	buff.data = &info;
    g_wifi_state.connected = 1;
    wlan_dev_indicate_event_handle(g_wifi_state.s_wlan_dev, WLAN_DEV_EVT_CONNECT, &buff);
    FREE(result_buff);
}

static void hgic_wifi_event_test_task(void *usrdata)
{
    struct wifi_state *wifi = (struct wifi_state *)usrdata;
    struct hgic_wifi_event *evt = NULL;
    int size = 0;
    int ret  = 0;

    while(1) {
        evt = NULL;
        ret = aos_queue_recv(&wifi->eventq, osWaitForever,(void *)&evt,&size);
        if (ret) {
            hgic_err("recv error!\n");
            continue;
        }
        if (!evt) {
            hgic_err("Invaild evt!\n");
            continue;
        }
        switch(evt->event) {
            case HGIC_EVENT_CONECTED:
                if(evt->mode == WLAN_STATION) {
                    hgic_dbg("STA MODE:%s Connected!bssid:"MACSTR"\r\n",
                        evt->ifname,MAC2STR((char *)evt->mac_addr));
                    wifi_connected_report(evt->mac_addr);
                } else if(evt->mode == WLAN_AP) {
                    hgic_dbg("AP MODE:STA %s Connected!bssid:"MACSTR"\r\n",
                        evt->ifname,MAC2STR((char *)evt->mac_addr));
                    ap_assoc_indicate(evt->mac_addr);
                } else {
                    hgic_err("Unknow state:%d\n",evt->mode);
                }
            break;
            case HGIC_EVENT_DISCONECTED:
                if(evt->mode == WLAN_STATION) {
                    rt_kprinft("STA MODE:disconnected!\r\n");
                    if(wifi->connected != 1){
				        sta_connect_fail();
			        } else {
				        wifi_discon_report();
			        }
                } else if(evt->mode == WLAN_AP) {
                    rt_kprinft("AP MODE:disconnected!\r\n");
                    ap_disassoc_indicate(evt->mac_addr);
                } else {
                    hgic_err("Unknow state:%d\n",evt->mode);
                }
                break;
            case HGIC_EVENT_CONECT_START:
                rt_kprinft("STA MODE:start connectting ...\r\n");
                break;
            case HGIC_EVENT_BLENC_DATA:
                //hgic_err("Recv blenc data!\n");
                hgics_recv_blenc_data(evt->data, evt->data_len);
                if(evt->data) {
                    FREE(evt->data);
                }
                break;
            case HGIC_EVENT_HGIC_DATA:
                hgics_proc_bt_data(evt->data, evt->data_len);
                if(evt->data) {
                    FREE(evt->data);
                }
            break;
            default:
                hgic_dbg("Recv unknow event:%d\n",evt->event);
                break;
        }
        if(evt) {
            FREE(evt);
        }
    }
}

static int hgic_wifi_test_init()
{
    int ret = 0;
    ret = aos_queue_new(&g_wifi_state.eventq, &g_wifi_state.eventq_data,
        sizeof(g_wifi_state.eventq_data), 4);
    ASSERT(ret == 0);

    ret = aos_task_new_ext(&g_wifi_state.event_task, "wifi_evt",
                          hgic_wifi_event_test_task, &g_wifi_state, 2048, 16);
    ASSERT(ret == 0);
    return 0;
}
#endif

int hgic_txw840x_set_mode(unsigned int mode)
{
/*
    if(!__wifi_init_check()) {
        hgic_err("wifi driver Not inited!");
        return -1;
    }
*/
    int ret = 0;
    hgic_dbg("***Set mode:%d\n",mode);

    if (mode == WLAN_AP) {
        g_wifi_state.mode = WLAN_AP;
        ret = hapd_init();
    } else if (mode == WLAN_STATION) {
        g_wifi_state.mode = WLAN_STATION;
        ret = wpas_init();
    } else {

    }
    if(ret) {
        hgic_err("Wifi app %s init error,mode:%d\n",
            mode == WLAN_AP ? "Hostadp" : "Wpa_supplicant",mode);
        return ret;
    }
    hgic_dbg("set mode %d \n", mode);
    sys_umaccfg_init(WPA_AUTH_NONE);
	return 0;
}


int hgic_txw840x_init(unsigned int mode)
{
    g_wifi_state.mode = mode;
	hgic_dbg("***hugeic wifi probe start,mode:%d***\n",mode);

    int ret = 0;

    if(!__wifi_init_check()) {
        hgic_dbg("Enter,for Anyka Alios test\n");
        hgic_param_ifname("w%d");             // set driver parameter: ifname, the default ifname is hg%d.
        hgic_param_initcb(hgic_smac_init_cb); // set driver parameter: init callback.
        hgic_param_eventcb(hgic_smac_event_cb); // set driver parameter: event callback.
        hgics_init();       // init hgics driver core lib.
        //hgic_wifi_test_init();
        hgic_txw840x_set_mode(mode);
        __wifi_init_done();
        hgic_dbg("Leave\n");
    } else {
        hgic_dbg("Already inited!\n");
    }

    //hgic_timer_test();

    return 0;
}

extern int hgics_blenc_init(void);

#if 1
static void hgic_timer_test1_callback(void *priv)
{
    struct wifi_state *wifi = (struct wifi_state *)priv;
    int ret = 0;
    if(wifi == NULL) {
        hgic_err("Input param error\n");
        return;
    }
    hgic_dbg("Enter HUGEIC timer1 test callback!\n");
    if(wifi->mode == WLAN_AP) {
        //hgloop_cleanup();
        //hgics_hapdcli_disable_ap("w0");
        //hgics_hapdcli_stop_ap("w0");
        ret = hapd_stop("w0");
        if(ret) {
            hgic_err("Stop hostapd error!\n");
        }
        //hapd_deinit();
    } else if(wifi->mode == WLAN_STATION) {
        //hgics_wpacli_disable_network("w0");
        ret = wpas_stop("w0");
        if(ret) {
            hgic_err("Stop wpa error!\n");
        }
    } else {
        hgic_err("Unknow mode:%d!\n",wifi->mode);
    }
}

static void hgic_timer_test2_callback(void *priv)
{
    struct wifi_state *wifi = (struct wifi_state *)priv;
    int ret = 0;
    if(wifi == NULL) {
        hgic_err("Input param error\n");
        return;
    }
    hgic_dbg("Enter HUGEIC timer test2 callback!\n");
    if(wifi->mode == WLAN_AP) {
        wifi->mode = WLAN_STATION;
        wpas_init();
        ret = hgic_txw840x_set_mode(wifi->mode);
        if(ret) {
            hgic_err("init wpa error!\n");
        }
        ret = wpas_start("w0");
        if(ret) {
            hgic_err("start wpa error!\n");
        }
        return;
    } else if(wifi->mode == WLAN_STATION) {
        wifi->mode = WLAN_AP;
        hapd_init();
        ret = hgic_txw840x_set_mode(wifi->mode);
        if(ret) {
            hgic_err("init hostapd error!\n");
        }
        ret = hapd_start("w0");
        if(ret) {
            hgic_err("start hapd error!\n");
        }
        return;
    } else {
        hgic_err("Unknow mode:%d!\n",wifi->mode);
    }
}

static int hgic_timer_test()
{
    int timer = 0;
    timer = sys_timeout_add_to_task("sys_timer",
        &g_wifi_state, hgic_timer_test1_callback, 20000);
    if(timer == 0) {
        hgic_err("Create hgic test1 timer failed!\n");
        return -1;
    }

    timer = sys_timeout_add_to_task("sys_timer",
        &g_wifi_state, hgic_timer_test2_callback, 30000);
    if(timer == 0) {
        hgic_err("Create hgic test2 timer failed!\n");
        return -1;
    }
    return 0;
}
#endif

#if 0

int hgic_txw840x_scan(struct wlan_device *wlan, struct scan_info *scan_info)
{
    char *buff = NULL;
    char cmd[128];
    int i = 0;

    if(!__wifi_init_check())
       return -1;

    buff = MALLOC(AP_INFO_BUFF_SIZE);
    if(buff == NULL) {
        hgic_err("Malloc buffer failed!\n");
        return -2;
    }
    memset(buff,0,AP_INFO_BUFF_SIZE);

    if(scan_info) {
        hgic_dbg("Use default scan...\n");
        memset(cmd,0,sizeof(cmd));
        for(i = 0;i < sizeof(scan_info->channel_set)/sizeof(scan_info->channel_set[0]);i++) {
            hgic_dbg("Scan freq:%d\n",scan_info->channel_set[i]);
        }

        if(scan_info->ssid.val != NULL && scan_info->ssid.len != 0) {
            snprintf(cmd, 128,"SCAN ssid=\"%s\"",scan_info->ssid.val);
            hgic_dbg("Set scan cmd:%s\n",cmd);
            wpas_cli("w0", cmd, buff, AP_INFO_BUFF_SIZE);
        } else {
            wpas_cli("w0", "SCAN", buff, AP_INFO_BUFF_SIZE);
        }
    } else {
        hgic_dbg("Use default scan...\n");
        wpas_cli("w0", "SCAN", buff, AP_INFO_BUFF_SIZE);
    }
    drv_os_msleep(1000);
    wpas_cli("w0", "SCAN_RESULTS", buff, AP_INFO_BUFF_SIZE);
    free(buff);
    return 0;
}

int hgic_txw840x_join(struct wlan_device *wlan, struct sta_info *sta_info)
{
    if(!__wifi_init_check())
       return -1;

    if(!wlan || !sta_info) {
        hgic_err("input param error!\n");
        return -1;
    }

    hgics_wpacli_set_ssid("w0", sta_info->ssid.val);
    if(sta_info->security || sta_info->key.len != 0) {
        hgic_dbg("Security ssid %s,key:%s\n",sta_info->ssid.val,sta_info->key.val);
        hgics_wpacli_set_keymgmt("w0", "WPA-PSK");
        hgics_wpacli_set_psk("w0", sta_info->key.val);
    } else {
        hgic_dbg("Non security ssid:%s,key:%s\n",sta_info->ssid.val,sta_info->key.val);
        hgics_wpacli_set_keymgmt("w0", "NONE");
    }
    hgics_wpacli_disable_network("w0");
    hgics_wpacli_enable_network("w0");

    return 0;
}

static int hgic_wifi_modify_config(char *ifname,struct umac_config *conf,char *cmd)
{

    int left = 0;

    if(conf == NULL || cmd == NULL || ifname == NULL) {
        hgic_err("Input param error!\n");
        return -1;
    }
    if(strcmp(conf->hg0,cmd) == 0) {
        hgic_dbg("cmd [%s] already exist!\n",cmd);
        return 0;
    }
    if(strlen(cmd) > sizeof(conf->hg0) - strlen(conf->hg0)) {
        hgic_err("No enought buffer!\n");
        return -1;
    }
    strcat(conf->hg0, cmd);
    //hgic_dbg("Modify conf:%s\n",conf->hg0);
    return 0;
}



int hgic_txw840x_softap(struct wlan_device *wlan, struct ap_info *ap_info)
{
    unsigned char ap_addr[12] = {0};
    struct wlan_buff buff = {0};
    struct umac_config *conf = NULL;
    char *cmd = NULL;
    char *out = NULL;
    int ret = 0;

    if(!__wifi_init_check())
        return -1;

    cmd = MALLOC(CONFIG_BUFF_SIZE);
    if(cmd == NULL) {
        hgic_err("ERROR:No memory!\n");
        return -1;
    }
    memset(cmd,0,CONFIG_BUFF_SIZE);

    out = MALLOC(CONFIG_BUFF_SIZE);
    if(out == NULL) {
        hgic_err("ERROR:No memory!\n");
        return -1;
    }
    memset(out,0,CONFIG_BUFF_SIZE);

    conf = umac_configs();
    if(conf == NULL) {
        hgic_err("ERROR:No memory!\n");
        return -1;
    }

    if(ap_info->security || ap_info->key.len != 0) {
        hgic_dbg("Security ssid %s,key:%s\n",ap_info->ssid.val,ap_info->key.val);

        hgic_wifi_modify_config("w0", conf, "wpa_key_mgmt=WPA-PSK\n");
        hgic_wifi_modify_config("w0", conf, "wpa_pairwise=CCMP\n");
        memset(cmd,0,CONFIG_BUFF_SIZE);
        snprintf(cmd, CONFIG_BUFF_SIZE,"ssid=%s\n",ap_info->ssid.val);
        hgic_wifi_modify_config("w0", conf, cmd);
        memset(cmd,0,CONFIG_BUFF_SIZE);
        snprintf(cmd, CONFIG_BUFF_SIZE,"wpa_passphrase=%s\n",ap_info->key.val);
        hgic_wifi_modify_config("w0", conf, cmd);
    } else {
        hgic_dbg("Non security ssid:%s,key:%s\n",ap_info->ssid.val,ap_info->key.val);
        memset(cmd,0,CONFIG_BUFF_SIZE);
        memset(conf->hg0,0,sizeof(conf->hg0));
        snprintf(cmd, CONFIG_BUFF_SIZE, "country_code=CN\n" \
               "ssid=%s\n" \
               "channel=5\n" \
               "wpa=0\n" \
               "hw_mode=g\n"\
               "ieee80211n=1\n" \
               "ht_capab=\n",ap_info->ssid.val);
        strcpy(conf->hg0,cmd);
    }

    ret = hgics_hapdcli_update_config("w0", out, CONFIG_BUFF_SIZE);
    if(ret < 0) {
        hgic_err("update umac config failed,ret:%d\n",ret);
    }

    memcpy(ap_addr, ap_info->ip_addr, sizeof(ap_info->ip_addr));
    memcpy(ap_addr + sizeof(ap_info->ip_addr), ap_info->mask, sizeof(ap_info->mask));
    memcpy(ap_addr + sizeof(ap_info->ip_addr) + sizeof(ap_info->mask), ap_info->gateway, sizeof(ap_info->gateway));
	buff.data = (void *)ap_addr;
    buff.len = sizeof(ap_addr);

	wlan_dev_indicate_event_handle(g_wifi_state.s_wlan_dev, WLAN_DEV_EVT_AP_START, &buff);

    //hgic_dbg("softap start!\n");

    FREE(cmd);
    FREE(out);

    return 0;
}


int hgic_txw840x_disconnect(struct wlan_device *wlan)
{
    if(!__wifi_init_check())
       return -1;
    hgics_wpacli_disable_network("w0");
    return 0;
}

int hgic_txw840x_ap_stop(struct wlan_device *wlan)
{
    if(!__wifi_init_check())
        return -1;
    hgics_hapdcli_stop_ap("w0");
    return 0;
}

int hgic_txw840x_ap_deauth(struct wlan_device *wlan, unsigned char mac[])
{
    char buff[128];
    if(!__wifi_init_check())
        return -1;
    hgics_hapdcli_deauth("w0");
    return 0;
}

int hgic_txw840x_scan_stop(struct wlan_device *wlan)
{
    if(!__wifi_init_check())
        return -1;

    hgics_wpacli_cancel_scan("w0");
    return 0;
}

extern int hgics_blenc_test(int mode,int adv_interval);
int hgic_txw840x_get_rssi(struct wlan_device *wlan)
{
    struct wlan_info info = {0};
    if(!__wifi_init_check())
        return -1;

    return hgics_wpacli_get_rssi("w0");
}

int hgic_txw840x_set_channel(struct wlan_device *wlan, int channel)
{
    struct umac_config *conf = NULL;
    char *chan[32] = {0};
    char cmd[128] = {0};
    char out[128] = {0};
    if(!__wifi_init_check())
        return -1;

    conf = umac_configs();
    if(conf == NULL) {
        hgic_err("ERROR:No memory!\n");
        return -1;
    }

    memset(cmd,0,sizeof(cmd));
    memset(out,0,sizeof(out));
    itoa(channel, chan, 10);
    snprintf(cmd,sizeof(cmd),"channel=%s\n",chan);
    hgic_wifi_modify_config("w0", conf, cmd);
    return hgics_hapdcli_update_config("w0", out, sizeof(out));
}

int hgic_txw840x_get_channel(struct wlan_device *wlan)
{
    if(!__wifi_init_check())
        return -1;
    hgic_err("Not support!\n");
    hgic_dbg("Use for blenc sta test...\n");
    return -1;
}

int hgic_txw840x_set_country(struct wlan_device *wlan, country_code_t country_code)
{
    int i = 0;
    char *country_str = NULL;
    if(!__wifi_init_check())
        return -1;
    for(i = 0; i < COUNTRY_MAX;i++) {
        if(country_code == country_trans[i].type_value) {
            country_str = country_trans[i].type;
        }
    }
    if(country_str == NULL) {

        hgic_err("country code %d not found!\n");
        return -1;
    }
    hgics_wpacli_set_country("w0", country_str);
    return 0;
}

country_code_t hgic_txw840x_get_country(struct wlan_device *wlan)
{
    country_code_t country_code = 0;
    int i = 0;
    char country[64];

    memset(country,0,sizeof(country));
    hgics_wpacli_get_country("w0", country, sizeof(country));

    for(i = 0; i < COUNTRY_MAX;i++) {
        if(strcmp(country,country_trans[i].type) == 0) {
            country_code = country_trans[i].type_value;
        }
    }
    return country_code;
}

int hgic_txw840x_set_mac(struct wlan_device *wlan, wlan_mode_t mode, unsigned char *mac)
{
    if(!__wifi_init_check())
        return -1;
    hgic_err("Not support!\n");
    return -1;
}

int hgic_txw840x_get_mac(struct wlan_device *wlan, wlan_mode_t mode, unsigned char *mac)
{
    return sys_get_ndev_addr(mac);
}

int hgic_txw840x_recv(struct wlan_device *wlan, void *buff, int len)
{
    if(!__wifi_init_check())
        return -1;
    hgic_err("Not support!\n");
    return -1;
}

extern void alios_pbuf_send(struct pbuf *pbuf);
extern void alios_wifi_send(struct pbuf *pbuf);
int hgic_txw840x_send(struct wlan_device *wlan, void *buff, int len)
{
    if(g_wifi_state.s_wlan_dev && (g_wifi_state.s_wlan_dev->flags != 0) )
    {
        struct pbuf *p = (struct pbuf *)buff;
        alios_wifi_send(p);
    }
    return 0;
}


int hgic_txw840x_msg_send(struct wlan_device *wlan, void *buff, int len)
{
    if(!__wifi_init_check())
        return -1;
    hgic_err("Not support!\n");
    return 0;
}


const struct wlan_dev_ops wlan_ops = {
    .wlan_init = hgic_txw840x_init,
    .wlan_mode = hgic_txw840x_set_mode,
    .wlan_scan = hgic_txw840x_scan,
    .wlan_join = hgic_txw840x_join,
    .wlan_softap = hgic_txw840x_softap,
    .wlan_disconnect = hgic_txw840x_disconnect,
    .wlan_ap_stop = hgic_txw840x_ap_stop,
    .wlan_ap_deauth = hgic_txw840x_ap_deauth,
    .wlan_scan_stop = hgic_txw840x_scan_stop,
    .wlan_get_rssi = hgic_txw840x_get_rssi,
    .wlan_set_channel = hgic_txw840x_set_channel,
    .wlan_get_channel = hgic_txw840x_get_channel,
    .wlan_set_country = hgic_txw840x_set_country,
    .wlan_get_country = hgic_txw840x_get_country,
    .wlan_set_mac = hgic_txw840x_set_mac,
    .wlan_get_mac = hgic_txw840x_get_mac,
    .wlan_recv = hgic_txw840x_recv,
    .wlan_send = hgic_txw840x_send,
    .wlan_msg_send = hgic_txw840x_msg_send,
};

void *ak_wlan_get_netif(void)
{
    return (void *)g_wifi_state.s_wlan_dev;
}

int ak_wlan_netif_ready()
{
    if(g_wifi_state.s_wlan_dev == NULL) {
        hgic_err("wlan_dev not inited!\n");
        return 0;
    }
    if(g_wifi_state.s_wlan_dev->flags != 0) {
        return 1;
    } else {
        return 0;
    }
}

int os_wlan_device_register(struct of_driver *driver, const struct wlan_dev_ops *wlan_ops)
{
	struct wlan_device *wlan_dev = drv_malloc(sizeof(struct wlan_device));
	memset(wlan_dev, 0 , sizeof(struct wlan_device));

    wlan_dev->dhcpc_disable = false;
    wlan_dev->dhcpd_disable = false;

    /* register wlan device */
	if (wlan_dev_register(wlan_dev, "wifi", wlan_ops, DRV_DEVICE_FLAG_RDWR, NULL) != 0)
	{
		drv_printk(C1, M_DRVSYS, "wlan device register failed\n");
		drv_free(wlan_dev);
		return -1;
	}

	driver->os_dev = (void *)wlan_dev;
	wlan_dev->device.user_data = (void *)driver;

    return 0;
}


int wifi_device_reg(void)
{
	struct ak_wlan_dev *ak_wlan;
	ver_register(&lib_info);

	ak_wlan = MALLOC(sizeof(struct ak_wlan_dev));
    ASSERT(ak_wlan);
	memset(ak_wlan, 0, sizeof(struct ak_wlan_dev));
	ak_wlan->driver = MALLOC(sizeof(struct of_driver));
    ASSERT(ak_wlan->driver);
	memset(ak_wlan->driver, 0, sizeof(struct of_driver));

	ak_wlan->driver->driver_data = (void *)ak_wlan;
	ak_wlan->status = 0;

	INIT_LIST_HEAD(&ak_wlan->recv_list);

	if(os_wlan_device_register(ak_wlan->driver, &wlan_ops)) {
		drv_free(ak_wlan->driver);
		drv_free(ak_wlan);
	    hgic_err("wlan register failed\n");
		return -1;
	}

	g_wifi_state.s_wlan_dev = ak_wlan->driver->os_dev;
	hgic_err("HUGEIC:wlan register success,netif:%p\n",g_wifi_state.s_wlan_dev);

	return 0;
}

OF_DEVICE_POST_INITCALL(wifi_device_reg);
#endif
#endif
