#include <rthw.h>
#include <rtdef.h>
#include <rtthread.h>
#include <lwipopts.h>

#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "netif/ethernetif.h"
#include "os_porting.h"
#include "aic_core.h"
#include "hgic.h"
#include "umac_config.h"
#include <string.h>

#define AP_INFO_BUFF_SIZE   (2048 * 10)
#define CONFIG_BUFF_SIZE    (1024)
#define REPLY_BUFF_SIZE     (4096)

#ifndef MAC2STR
#define MAC2STR(a) (a)[0]&0xff, (a)[1]&0xff, (a)[2]&0xff, (a)[3]&0xff, (a)[4]&0xff, (a)[5]&0xff
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define COMPACT_MACSTR "%02x%02x%02x%02x%02x%02x"
#endif

#define WIFIDEV_STATE_STOP      0
#define WIFIDEV_STATE_RUNNING   1
#define HGIC_WIFI_RXQ_SIZE 32

#define HGIC_IF_NAME "hg0"

struct hgic_wifi {
    struct eth_device  dev;
    unsigned int state;
    void *ndev;
};

struct hgic_ble_mb {
    u32 event;
    u8 *data;
    u32 data_len;
};

struct hgic_netdev_scatter_data {
    void   *addr;
    unsigned int  size;
};

struct wifi_state {
    unsigned int drv_inited;
    rt_wlan_mode_t mode;
    struct umac_config umac_cfg;
};

struct wifi_state g_wifi_state = {0};
struct hgic_wifi *g_hgic_wifi_dev = NULL;
struct rt_wlan_device * g_hgics_wlan_dev = NULL;
struct rt_wlan_device * g_hgics_ap_dev = NULL;
struct rt_wlan_device * g_hgics_current_dev = NULL;
static rt_mutex_t g_hgics_init_sem = NULL;
static rt_mutex_t g_hgics_scan_sem = NULL;
#if BLENC_STA
static u8 ble_data[BLENC_DATA_SIZE] = {0};
#endif
extern rt_mailbox_t g_hgics_ble_mb;
extern int hgic_ble_proc_init();
extern int net_device_xmit_scatter(void *dev, void *scat_info, int count, int total_len);


struct umac_config *umac_configs(void)
{
    return &g_wifi_state.umac_cfg;
}

int sys_save_umaccfg(struct umac_config *cfg)
{
}

char *malloc_skb_buff(unsigned int len, int *buff_priv)
{
    char * buf = NULL;

    buf = aicos_malloc_align(0, len, CACHE_LINE_SIZE);
    if (!buf) {
        hgic_err("malloc skb buffer failed!, len:%d\n", len);
        return NULL;
    }
    *buff_priv = (void *)buf;
    return buf;
}

void free_skb_buff(char *buff, void *buff_priv)
{
    if(buff_priv) {
        aicos_free_align(0, buff_priv);
    }
}

void sys_netif_recv(void *priv, char *data, int len, void *buff_priv)
{
    struct hgic_wifi *wifi = (struct hgic_wifi *)priv;
    struct pbuf *p = NULL;

    if (wifi == NULL || data == NULL || len == 0) {
        hgic_err("Input param error!\n");
        return;
    }
    if (wifi->state != WIFIDEV_STATE_RUNNING) {
        hgic_err("Wifi not running!\n");
        return;
    }

    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p == NULL) {
        hgic_err("\n\rCannot allocate pbuf to receive packet length %d,l%d.\n", len);
        return;
    }
    pbuf_take(p, data, len);

    if (rt_wlan_dev_report_data(g_hgics_current_dev, p, p->len)) {
        hgic_err("input pkt fail\n");
        pbuf_free(p);
    }
}

void *sys_netif_register(void *dev, char *name)
{
    int ret = 0;
    struct hgic_wifi *wifi = NULL;
    unsigned char mac[6] = {0};

    hgic_dbg("First time to init netif:%s\n", name);

    wifi = MALLOC(sizeof(struct hgic_wifi));
    if (!wifi) {
        hgic_err("Malloc failed!\n");
        return NULL;
    }
    memset(wifi, 0, sizeof(struct hgic_wifi));

    wifi->ndev = dev;
    wifi->state = WIFIDEV_STATE_RUNNING;
    ret = net_device_get_addr(wifi->ndev, mac);
    if (ret < 0) {
        hgic_dbg("get hgic wifi dev mac failed! register failed!\n");
        return NULL;
    }
    g_hgic_wifi_dev = wifi;
    hgic_dbg("register hgic wifi dev success, mac:"MACSTR"\n",MAC2STR(mac));

    hgic_dbg("hgic wifi rtt eth_dev will set when set mode!\n");
    return wifi;
}

int sys_netif_unregister(void *priv)
{
    struct hgic_wifi *wifi = (struct hgic_wifi *)priv;
    if (wifi) {
        hgic_dbg("Unregister netdev\n");
        wifi->state = WIFIDEV_STATE_STOP;
    }
    return 0;
}

extern void hgics_recv_blenc_data(unsigned char *data, unsigned int len);

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

static rt_802_11_band_t freq_to_band(int freq) {
    if (freq >= 2412 && freq <= 2484) {
        return RT_802_11_BAND_2_4GHZ;
    } else if (freq >= 4900 && freq <= 5925) {
        return RT_802_11_BAND_5GHZ;
    }
    return RT_802_11_BAND_UNKNOWN;
}

static rt_int16_t freq_to_channel(int freq) {
    // 2.4GHz
    if (freq >= 2412 && freq <= 2484) {
        if (freq == 2484) return 14;
        return (freq - 2407) / 5;
    }
    // 5GHz
    if (freq >= 5180 && freq <= 5825) {
        return (freq - 5000) / 5;
    }
    return -1;
}

static rt_wlan_security_t parse_security(const char *flags) {
    int has_wpa = 0, has_wpa2 = 0, has_tkip = 0, has_ccmp = 0, has_wep = 0;

    if (strstr(flags, "WPA2")) has_wpa2 = 1;
    if (strstr(flags, "WPA")) has_wpa = 1;
    if (strstr(flags, "TKIP")) has_tkip = 1;
    if (strstr(flags, "CCMP")) has_ccmp = 1;
    if (strstr(flags, "WEP")) has_wep = 1;

    if (has_wpa2) {
        if (has_tkip && has_ccmp) return SECURITY_WPA2_MIXED_PSK;
        if (has_ccmp) return SECURITY_WPA2_AES_PSK;
        if (has_tkip) return SECURITY_WPA2_TKIP_PSK;
    }

    if (has_wpa) {
        if (has_ccmp) return SECURITY_WPA_AES_PSK;
        if (has_tkip) return SECURITY_WPA_TKIP_PSK;
    }

    if (has_wep) {
        return SECURITY_WEP_PSK;
    }

    if (strstr(flags, "ESS") || strstr(flags, "IBSS")) {
        return SECURITY_OPEN;
    }

    return SECURITY_UNKNOWN;
}

static void parse_ssid(const char *src, rt_wlan_ssid_t *dest) {
    int len = 0;
    const char *p = src;

    while (*p && len < RT_WLAN_SSID_MAX_LENGTH) {
        if (p[0] == '\\' && p[1] == 'x' && isxdigit(p[2]) && isxdigit(p[3])) {
            char hex[3] = {p[2], p[3], '\0'};
            dest->val[len++] = (rt_uint8_t)strtol(hex, NULL, 16);
            p += 4;
        } else {
            dest->val[len++] = *p++;
        }
    }

    dest->len = len;
    dest->val[len] = '\0';
}

int parse_wpas_scan_info(const char *data) {
    char *line = strdup(data);
    char *saveptr = NULL;
    char *token = strtok_r(line, "\n", &saveptr);
    int count = 0;
    struct rt_wlan_buff buff = {0};
    struct rt_wlan_info info = {0};

    token = strtok_r(NULL, "\n", &saveptr);

    while (token) {
        char *fields[5];
        char *field_save = NULL;
        char *field_token = strtok_r(token, "\t", &field_save);
        int field_count = 0;

        while (field_token && field_count < 5) {
            fields[field_count++] = field_token;
            field_token = strtok_r(NULL, "\t", &field_save);
        }

        if (field_count == 5) {
            memset(&info, 0, sizeof(struct rt_wlan_info));
            unsigned int bssid[6];
            if (sscanf(fields[0], "%x:%x:%x:%x:%x:%x",
                       &bssid[0], &bssid[1], &bssid[2],
                       &bssid[3], &bssid[4], &bssid[5]) == 6) {
                for (int i = 0; i < 6; i++) {
                    info.bssid[i] = (rt_uint8_t)bssid[i];
                }
            }

            info.channel = freq_to_channel(atoi(fields[1]));
            info.band = RT_802_11_BAND_2_4GHZ;//freq_to_band(atoi(fields[1]));
            info.rssi = (int8_t)atoi(fields[2]);
            info.security = parse_security(fields[3]);
            parse_ssid(fields[4], &info.ssid);
            info.datarate = 0;
            info.hidden = 0;

            buff.len = sizeof(struct rt_wlan_info);
            buff.data = (void *)&info;

            rt_wlan_dev_indicate_event_handle(g_hgics_wlan_dev, RT_WLAN_DEV_EVT_SCAN_REPORT, &buff);

            count++;
        }

        token = strtok_r(NULL, "\n", &saveptr);
    }

    rt_wlan_dev_indicate_event_handle(g_hgics_wlan_dev, RT_WLAN_DEV_EVT_SCAN_DONE, NULL);

    free(line);
    return count;
}

static int hgic_wifi_wifi_send(struct pbuf *pbuf)
{
    struct hgic_wifi *wifi = NULL;
    struct hgic_netdev_scatter_data scatter_info[10];
    struct pbuf *p = NULL;
    unsigned int frag_num = 0;
    unsigned int copy_len = 0;
    int ret = 0;

    if (pbuf == NULL || pbuf->payload == NULL) {
        hgic_err("input param error!\n");
        return -1;
    }

    wifi = g_hgic_wifi_dev;

    if (wifi == NULL) {
        hgic_err("Wifi not inited!\n");
        return -2;
    }
    if (wifi->state != WIFIDEV_STATE_RUNNING) {
        hgic_err("Wifi not running!\n");
        return -3;
    }

    memset(&scatter_info, 0, sizeof(scatter_info));
    frag_num = 0;
    for (p = pbuf; p != NULL; p = p->next) {
        scatter_info[frag_num].addr = (void *)p->payload;
        scatter_info[frag_num].size = p->len;
        frag_num ++;
        copy_len += p->len;
        if (frag_num > (sizeof(scatter_info) / sizeof(scatter_info[0]))) {
            hgic_err("Too much fragment!\n");
            break;
        }
    }
    if (copy_len != pbuf->tot_len) {
        hgic_err("Copy error,please check!\n");
        return -4;
    }

    ret = net_device_xmit_scatter(wifi->ndev, (void *)&scatter_info, frag_num, pbuf->tot_len);
    if (ret < 0) {
        hgic_err("Send to wifi failed,ret:%d\n", ret);
    }
    return ret;
}

static unsigned int __wifi_init_check(void)
{
    if(g_wifi_state.drv_inited == 0) {
        hgic_dbg("WIFI Driver not inited!\n");
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

void hgic_smac_init_cb(void *args)
{
    int ret = 0;

    /* callback release the sem for hugeic delay init */
    rt_sem_release(g_hgics_init_sem);

    hgic_dbg("hgics init done!\r\n");
}

void hgic_smac_event_cb(char * ifname, int event, long param1, long param2)
{
    hgic_dbg("event id:%d\n", event);
    switch (event) {
    case HGIC_EVENT_CONECTED:
        hgic_dbg(""MACSTR" Connected,mode:%s\n", MAC2STR((char *)param1),
            g_wifi_state.mode == RT_WLAN_STATION? "STA" : "AP");
        if(g_wifi_state.mode == RT_WLAN_STATION) {
            rt_wlan_dev_indicate_event_handle(g_hgics_wlan_dev, RT_WLAN_DEV_EVT_CONNECT, NULL);
        } else {
            struct rt_wlan_buff buff;
            struct rt_wlan_info sta_info;
            memset(&sta_info, 0, sizeof(sta_info));
            memcpy(sta_info.bssid, (char *)param1, 6);
            buff.len = sizeof(sta_info);
            buff.data = (void *)&sta_info;
            rt_wlan_dev_indicate_event_handle(g_hgics_ap_dev, RT_WLAN_DEV_EVT_AP_ASSOCIATED, &buff);
        }
        break;
    case HGIC_EVENT_DISCONECTED:
        hgic_dbg(""MACSTR" Disconnected,mode:%s\n", MAC2STR((char *)param1),
            g_wifi_state.mode == RT_WLAN_STATION? "STA" : "AP");
        if(g_wifi_state.mode == RT_WLAN_STATION) {
            rt_wlan_dev_indicate_event_handle(g_hgics_wlan_dev, RT_WLAN_DEV_EVT_DISCONNECT, NULL);
        } else {
            struct rt_wlan_buff buff;
            struct rt_wlan_info sta_info;
            memset(&sta_info, 0, sizeof(sta_info));
            memcpy(sta_info.bssid, (char *)param1, 6);
            buff.len = sizeof(sta_info);
            buff.data = (void *)&sta_info;
            rt_wlan_dev_indicate_event_handle(g_hgics_ap_dev, RT_WLAN_DEV_EVT_AP_DISASSOCIATED, &buff);
        }
        break;
    case HGIC_EVENT_SCAN_DONE:
        if ((g_wifi_state.mode == RT_WLAN_NONE) && g_hgics_scan_sem != NULL)
            rt_sem_release(g_hgics_scan_sem);
        break;
#if BLENC_STA
    case HGIC_EVENT_BLENC_DATA:
    case HGIC_EVENT_HGIC_DATA:
        {
            struct hgic_ble_mb *ble_mb = rt_malloc(sizeof(struct hgic_ble_mb));
            if (ble_mb == NULL) {
                hgic_err("alloc ble_mb failed!\n");
                return;
            }

            ble_mb->event = event;
            ble_mb->data_len = (u32)param2;
            ble_mb->data = ble_data;
            if(ble_mb->data_len > BLENC_DATA_SIZE) {
                hgic_err("please increase BLENC_DATA_SIZE!\n");
                return;
            }
            memcpy(ble_mb->data, (void *)param1,(u32)param2);
            rt_mb_send(g_hgics_ble_mb, (rt_ubase_t)ble_mb);
            break;
        }
#endif
    default:
        break;
    }
}

rt_err_t hgic_wifi_init(struct rt_wlan_device *wlan)
{
    hgic_dbg("[rtt] wlan init probe start.\n");

    g_hgics_init_sem = rt_sem_create("hgics_init_sem", 0, RT_IPC_FLAG_FIFO);
    if (g_hgics_init_sem == NULL) {
        rt_kprintf("hgics_init_mutex create failed!\n");
        return -RT_ERROR;
    }

    if(!__wifi_init_check()) {
        hgic_param_ifname("hg%d");             // set driver parameter: ifname, the default ifname is hg%d.
        hgic_param_initcb(hgic_smac_init_cb); // set driver parameter: init callback.
        hgic_param_eventcb(hgic_smac_event_cb); // set driver parameter: event callback.
        hgics_init();       // init hgics driver core lib.
        hapd_init();
        wpas_init();
#if BLENC_STA
        hgic_ble_proc_init();
#endif
        /* waiting for hugeic delay init */
        if (rt_sem_take(g_hgics_init_sem, rt_tick_from_millisecond(2000)) == -RT_ETIMEOUT) {
            rt_kprintf("hgic_smac_init_cb timeout!\n");
            return -RT_ETIMEOUT;
        };
        __wifi_init_done();
        hgic_dbg("Leave\n");
    } else {
        hgic_dbg("Already inited!\n");
    }

    rt_sem_delete(g_hgics_init_sem);

    return RT_EOK;
}

rt_err_t hgic_wifi_set_mode(struct rt_wlan_device *wlan, rt_wlan_mode_t mode)
{
    if(!__wifi_init_check())
       return -RT_ERROR;

    hgic_dbg("[rtt] wlan set mode :%d\n",mode);

    g_wifi_state.mode = mode;

    if (mode ==RT_WLAN_AP) {
        g_hgics_current_dev = g_hgics_ap_dev;
    } else if (mode == RT_WLAN_STATION) {
        g_hgics_current_dev = g_hgics_wlan_dev;
    } else {
        hgic_err("Unsupport mode:%d\n",mode);
        g_hgics_current_dev = NULL;
        return -RT_ERROR;
    }

    return RT_EOK;
}

static int hgic_wlan_scan(struct rt_scan_info *scan_info)
{
    char *buff = NULL;
    char cmd[128];
    int i = 0;

    g_hgics_scan_sem = rt_sem_create("hgics_scan_sem", 0, RT_IPC_FLAG_FIFO);
    if (g_hgics_scan_sem == NULL) {
        rt_kprintf("hgic_wlan_scan create failed!\n");
        return -RT_ERROR;
    }

    buff = MALLOC(AP_INFO_BUFF_SIZE);
    if(buff == NULL) {
        hgic_err("Malloc buffer failed!\n");
        return -RT_ENOMEM;
    }
    memset(buff, 0, AP_INFO_BUFF_SIZE);

    if (scan_info) {
        hgic_dbg("Use default scan...\n");
        memset(cmd, 0, sizeof(cmd));

        if(scan_info->ssid.val != NULL && scan_info->ssid.len != 0) {
            snprintf(cmd, 128,"SCAN ssid=\"%s\"",scan_info->ssid.val);
            hgic_dbg("Set scan cmd:%s\n", cmd);
            wpas_cli(HGIC_IF_NAME, cmd, buff, AP_INFO_BUFF_SIZE);
        } else {
            wpas_cli(HGIC_IF_NAME, "SCAN", buff, AP_INFO_BUFF_SIZE);
        }
    } else {
        hgic_dbg("Use default scan...\n");
        wpas_cli(HGIC_IF_NAME, "SCAN", buff, AP_INFO_BUFF_SIZE);
    }

    if (rt_sem_take(g_hgics_scan_sem, rt_tick_from_millisecond(5000)) == -RT_ETIMEOUT) {
        rt_kprintf("hgic_wlan_scan timeout!\n");
        return -RT_ETIMEOUT;
    };

    wpas_cli(HGIC_IF_NAME, "SCAN_RESULTS", buff, AP_INFO_BUFF_SIZE);

    rt_sem_delete(g_hgics_scan_sem);
    g_hgics_scan_sem = NULL;

    parse_wpas_scan_info(buff);

    FREE(buff);
    return RT_EOK;
}

rt_err_t hgic_wifi_scan(struct rt_wlan_device *wlan, struct rt_scan_info *scan_info)
{
    hgic_dbg("[rtt] wlan scan...................\n");
    struct umac_config *conf = NULL;
    rt_err_t ret = RT_EOK;

    if(!__wifi_init_check())
       return -RT_ERROR;

    /* due to rtt may not call wlan_mode */
    g_wifi_state.mode = RT_WLAN_NONE;
    g_hgics_current_dev = g_hgics_wlan_dev;

    conf = umac_configs();
    if (conf == NULL) {
        hgic_err("ERROR:Get umac configuration failed!\n");
        return -RT_ENOMEM;
    }
    memset(conf->hg0, 0, sizeof(conf->hg0));

    wpas_start(HGIC_IF_NAME);

    if (scan_info) {
        hgic_dbg("scan with info:ssid:%s, len:%d\n", scan_info->ssid.val, scan_info->ssid.len);
        hgic_dbg("bssid=%02X:%02X:%02X:%02X:%02X:%02X\n",
            scan_info->bssid[0], scan_info->bssid[1], scan_info->bssid[2],
            scan_info->bssid[3], scan_info->bssid[4], scan_info->bssid[5]);
        hgic_dbg("channel:%d->%d, passive=%d\n", scan_info->channel_min, scan_info->channel_max, scan_info->passive);
    }
    ret = hgic_wlan_scan(scan_info);
    if (ret < 0) {
        hgic_err("ERROR:hgic_wlan_scan!, ret:%d\n", ret);
    }

    wpas_stop(HGIC_IF_NAME);

    return ret;
}

rt_err_t hgic_wifi_join(struct rt_wlan_device *wlan, struct rt_sta_info *sta_info)
{
    struct umac_config *conf = NULL;

    hgic_dbg("[rtt] wlan join:SSID:%s\n", sta_info->ssid.val);

    /* due to rtt may not call wlan_mode */
    g_wifi_state.mode = RT_WLAN_STATION;
    g_hgics_current_dev = g_hgics_wlan_dev;

    if(!__wifi_init_check())
       return -RT_ERROR;

    if(!wlan || !sta_info) {
        hgic_err("input param error!\n");
        return -RT_ERROR;
    }

    conf = umac_configs();
    if (conf == NULL) {
        hgic_err("ERROR:Get umac configuration failed!\n");
        return -RT_ENOMEM;
    }
    memset(conf->hg0, 0, sizeof(conf->hg0));

    strcpy(conf->hg0, "update_config=1\n" \
       "network={\n" \
       "proto=WPA RSN\n"\
       "scan_ssid=1\n" \
       "pairwise=CCMP TKIP\n" \
       "group=CCMP TKIP\n" \
       "}\n");

    wpas_start(HGIC_IF_NAME);

    hgic_iwpriv_set_max_txcnt("hg0", 15);

    hgics_wpacli_set_ssid(HGIC_IF_NAME, sta_info->ssid.val);
    if(sta_info->key.len != 0) {
        hgic_dbg("Security ssid %s,key:%s\n",sta_info->ssid.val,sta_info->key.val);
        hgics_wpacli_set_keymgmt(HGIC_IF_NAME, "WPA-PSK");
        hgics_wpacli_set_psk(HGIC_IF_NAME, sta_info->key.val);
    } else {
        hgic_dbg("Non security ssid:%s,key:%s\n",sta_info->ssid.val,sta_info->key.val);
        hgics_wpacli_set_keymgmt(HGIC_IF_NAME, "NONE");
    }

    hgics_wpacli_disable_network(HGIC_IF_NAME);
    hgics_wpacli_enable_network(HGIC_IF_NAME);

    return RT_EOK;
}

static int hgic_wifi_modify_config(char *ifname, struct umac_config *conf, char *cmd)
{
    int left = 0;

    if(conf == NULL || cmd == NULL || ifname == NULL) {
        hgic_err("Input param error!\n");
        return -1;
    }
    if(strcmp(conf->hg0, cmd) == 0) {
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

rt_err_t hgic_wifi_softap(struct rt_wlan_device *wlan, struct rt_ap_info *ap_info)
{
    rt_err_t ret = RT_EOK;
    unsigned char ap_addr[12] = {0};
    struct umac_config *conf = NULL;
    char *cmd = NULL;

    /* due to rtt may not call wlan_mode */
    g_wifi_state.mode = RT_WLAN_AP;
    g_hgics_current_dev = g_hgics_ap_dev;

    if(!__wifi_init_check())
        return -RT_ERROR;

    cmd = MALLOC(CONFIG_BUFF_SIZE);
    if (cmd == NULL) {
        hgic_err("ERROR:No memory!\n");
        return -RT_ENOMEM;
    }

    conf = umac_configs();
    if (conf == NULL) {
        hgic_err("ERROR:Get umac configuration failed!\n");
        ret = -RT_ERROR;
        goto err_ret;
    }

    if (ap_info->security || ap_info->key.len != 0) {
        hgic_dbg("Security ssid %s,key:%s\n",ap_info->ssid.val,ap_info->key.val);

        /* set the default para */
        strcpy(conf->hg0, "country_code=CN\n" \
           "channel=6\n" \
           "wpa=2\n" \
           "wpa_key_mgmt=WPA-PSK\n" \
           "wpa_pairwise=CCMP\n" \
           "hw_mode=g\n"\
           "ieee80211n=1\n" \
           "ht_capab=\n" \
          );

        memset(cmd, 0, CONFIG_BUFF_SIZE);
        snprintf(cmd, CONFIG_BUFF_SIZE, "ssid=%s\n", ap_info->ssid.val);
        hgic_wifi_modify_config(HGIC_IF_NAME, conf, cmd);

        memset(cmd, 0, CONFIG_BUFF_SIZE);
        snprintf(cmd, CONFIG_BUFF_SIZE, "wpa_passphrase=%s\n", ap_info->key.val);
        hgic_wifi_modify_config(HGIC_IF_NAME, conf, cmd);
    } else {
        hgic_dbg("Non security ssid:%s,key:%s\n",ap_info->ssid.val, ap_info->key.val);
        memset(conf->hg0, 0, sizeof(conf->hg0));
        snprintf(cmd, CONFIG_BUFF_SIZE, "country_code=CN\n" \
               "ssid=%s\n" \
               "channel=6\n" \
               "wpa=0\n" \
               "hw_mode=g\n"\
               "ieee80211n=1\n" \
               "ht_capab=\n",ap_info->ssid.val);
        strcpy(conf->hg0, cmd);
    }

    if (hapd_start(HGIC_IF_NAME) < 0) {
        hgic_err("hapd_start failed!\n");
        ret = -RT_ERROR;
        goto err_ret;
    }

    hgic_iwpriv_set_max_txcnt("hg0", 15);

    rt_wlan_dev_indicate_event_handle(g_hgics_ap_dev, RT_WLAN_DEV_EVT_AP_START, NULL);

err_ret:
    FREE(cmd);

    return ret;
}

rt_err_t hgic_wifi_disconnect(struct rt_wlan_device *wlan)
{
    hgic_dbg("[rtt] wlan disconnect.\n");

    g_wifi_state.mode = RT_WLAN_STATION;
    g_hgics_current_dev = g_hgics_wlan_dev;

    if(!__wifi_init_check())
       return -RT_ERROR;

    hgics_wpacli_disable_network(HGIC_IF_NAME);
    wpas_stop(HGIC_IF_NAME);

    return RT_EOK;
}

rt_err_t hgic_wifi_ap_stop(struct rt_wlan_device *wlan)
{
    hgic_dbg("[rtt] wlan ap stop.\n");

    g_wifi_state.mode = RT_WLAN_AP;
    g_hgics_current_dev = g_hgics_ap_dev;

    if(!__wifi_init_check())
       return -RT_ERROR;

    hgics_hapdcli_stop_ap(HGIC_IF_NAME);
    rt_thread_mdelay(200);
    hapd_stop(HGIC_IF_NAME);

    rt_wlan_dev_indicate_event_handle(g_hgics_ap_dev, RT_WLAN_DEV_EVT_AP_STOP, NULL);

    return RT_EOK;
}

rt_err_t hgic_wifi_ap_deauth(struct rt_wlan_device *wlan, rt_uint8_t mac[])
{
    if(!__wifi_init_check())
        return -RT_ERROR;

    hgics_hapdcli_deauth(HGIC_IF_NAME);

    return RT_EOK;
}

rt_err_t hgic_wifi_scan_stop(struct rt_wlan_device *wlan)
{
    rt_err_t ret = RT_EOK;
    return ret;
}

int hgic_wifi_get_rssi(struct rt_wlan_device *wlan)
{
    hgic_dbg("[rtt] wlan get rssi.\n");

    if(!__wifi_init_check())
        return -RT_ERROR;

    return hgics_wpacli_get_rssi(HGIC_IF_NAME);
}

rt_err_t hgic_wifi_set_channel(struct rt_wlan_device *wlan, int channel)
{
    hgic_dbg("[rtt] wlan set channel.\n");

    struct umac_config *conf = NULL;
    char *chan[32] = {0};
    char cmd[128] = {0};
    char out[128] = {0};
    if(!__wifi_init_check())
        return -RT_ERROR;

    conf = umac_configs();
    if(conf == NULL) {
        hgic_err("ERROR:No memory!\n");
        return -RT_ENOMEM;
    }

    memset(cmd,0,sizeof(cmd));
    memset(out,0,sizeof(out));
    itoa(channel, chan, 10);
    snprintf(cmd,sizeof(cmd),"channel=%s\n",chan);
    hgic_wifi_modify_config(HGIC_IF_NAME, conf, cmd);
    return hgics_hapdcli_update_config(HGIC_IF_NAME, out, sizeof(out));
}

int hgic_wifi_get_channel(struct rt_wlan_device *wlan)
{
    hgic_dbg("[rtt] wlan get channel.\n");

    if(!__wifi_init_check())
        return -RT_ERROR;

    return -RT_ERROR;
}

rt_err_t hgic_wifi_set_country(struct rt_wlan_device *wlan, rt_country_code_t country_code)
{
    hgic_dbg("[rtt] wlan set country.\n");

    if(!__wifi_init_check())
        return -RT_ERROR;

    return -RT_ERROR;
}

rt_country_code_t hgic_wifi_get_country(struct rt_wlan_device *wlan)
{
    hgic_dbg("[rtt] wlan get country.\n");

    if(!__wifi_init_check())
        return -RT_ERROR;

    return -RT_ERROR;
}

rt_err_t hgic_wifi_set_mac(struct rt_wlan_device *wlan, rt_uint8_t mac[])
{
    hgic_dbg("[rtt] wlan set mac\n");

    hgic_dbg("%02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    rt_err_t ret = RT_EOK;
    return ret;
}

rt_err_t hgic_wifi_get_mac(struct rt_wlan_device *wlan, rt_uint8_t mac[])
{
    hgic_dbg("[rtt] wlan get mac\n");

    rt_err_t ret = RT_EOK;
    if (mac) {
        if (g_hgic_wifi_dev == NULL) {
             hgic_dbg("g_hgic_wifi_dev not regist yet!\n");
            return -RT_ERROR;
        }
        ret = net_device_get_addr(g_hgic_wifi_dev->ndev, mac);
        if (ret < 0) {
            hgic_dbg("get hgic wifi dev mac failed!\n");
            return -RT_ERROR;
        }
        hgic_dbg("mac:"MACSTR"\n",MAC2STR(mac));
    }

    return ret;
}

int hgic_wifi_recv(struct rt_wlan_device *wlan, void *buff, int len)
{
    hgic_dbg("[rtt] recv.\n");
    return -1;
}

int hgic_wifi_send(struct rt_wlan_device *wlan, void *buff, int len)
{
    struct pbuf *p = buff;
    int ret = 0;

    //net_device_xmit(g_hgic_wifi_dev->ndev, buff, len);
    ret = hgic_wifi_wifi_send((struct pbuf *)buff);
    if(ret) {
        return ret;
    }
    return RT_EOK;
}

const struct rt_wlan_dev_ops wlan_ops = {
    .wlan_init = hgic_wifi_init,
    .wlan_mode = hgic_wifi_set_mode,
    .wlan_scan = hgic_wifi_scan,
    .wlan_join = hgic_wifi_join,
    .wlan_softap = hgic_wifi_softap,
    .wlan_disconnect = hgic_wifi_disconnect,
    .wlan_ap_stop = hgic_wifi_ap_stop,
    .wlan_ap_deauth = hgic_wifi_ap_deauth,
    .wlan_scan_stop = hgic_wifi_scan_stop,
    .wlan_get_rssi = hgic_wifi_get_rssi,
    .wlan_set_channel = hgic_wifi_set_channel,
    .wlan_get_channel = hgic_wifi_get_channel,
    .wlan_set_country = hgic_wifi_set_country,
    .wlan_get_country = hgic_wifi_get_country,
    .wlan_set_mac = hgic_wifi_set_mac,
    .wlan_get_mac = hgic_wifi_get_mac,
    .wlan_recv = hgic_wifi_recv,
    .wlan_send = hgic_wifi_send,
};

int hgic_wlan_device_reg(void)
{
    hgic_dbg("ctrl power key\n");
    platform_pwr_wifi_pin_init();
    platform_pwr_wifi_pin_disable();
    rt_thread_mdelay(10);
    platform_pwr_wifi_pin_enable();

    g_hgics_wlan_dev = rt_malloc(sizeof(struct rt_wlan_device));
    if (!g_hgics_wlan_dev) {
        rt_kprintf("%s devcie malloc fail!\n", RT_WLAN_FLAG_STA_ONLY);
        return -1;
    }
    rt_wlan_dev_register(g_hgics_wlan_dev, RT_WLAN_DEVICE_STA_NAME, &wlan_ops, RT_WLAN_FLAG_STA_ONLY, NULL);

    g_hgics_ap_dev = rt_malloc(sizeof(struct rt_wlan_device));
    if (!g_hgics_ap_dev) {
        rt_kprintf("%s devcie malloc fail!\n", RT_WLAN_FLAG_AP_ONLY);
        return -1;
    }
    rt_wlan_dev_register(g_hgics_ap_dev, RT_WLAN_DEVICE_AP_NAME, &wlan_ops, RT_WLAN_FLAG_AP_ONLY, NULL);

    return 0;
}
INIT_COMPONENT_EXPORT(hgic_wlan_device_reg);

