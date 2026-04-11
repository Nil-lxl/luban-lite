/**
  ******************************************************************************
  * @file    hgics_relay.c
  * @author  HUGE-IC Application Team
  * @version V1.0.0
  * @date    2022-05-18
  * @brief   hgic smac relay daemon.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2022 HUGE-IC</center></h2>
  *
  ******************************************************************************
  */

#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#ifdef RT_USING_SAL
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#endif
#include <netdb.h>
#include <poll.h>

#include "hgic.h"

#define BIT(nr) (1 << (nr))
#define SCAN_RESULT_LEN (10*1024)
#define MAC_EQU(a, b) (memcmp((a), (b), 6)== 0)
#define MACSTR        "%02x:%02x:%02x:%02x:%02x:%02x"
#define MACARG(a)     (a)[0]&0xff, (a)[1]&0xff, (a)[2]&0xff, (a)[3]&0xff, (a)[4]&0xff, (a)[5]&0xff

struct wpacli_status {
    char bssid[6];
    char addr[6];
    char ssid[36];
    int  freq;
    char pairwise_cipher[12];
    char group_cipher[12];
    char key_mgmt[12];
    int  wpa_state;
};

struct hgics_relay {
    struct wpacli_status wpastatus;
    int last_wpa_state, wifi_mode;
    int rssi_th;
    int hwscan_running, connecting;
    char cfg_ssid[40];
    char cur_ssid[40];
    time_t hwscan_time;
} g_hgrelay;

//2.4G channel list
static const int freq_list[14] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484};

static inline int hgics_is_relay(char *ssid)
{
    char temp[64];
    sprintf(temp, "%s#r#", g_hgrelay.cfg_ssid);
    return (strncmp(ssid, temp, strlen(temp)) == 0);
}

static inline int hgics_is_ap(char *ssid)
{
    return (strcmp(ssid, g_hgrelay.cfg_ssid) == 0);
}

static inline int hgics_is_suitable_ap(char *ssid)
{
    return (hgics_is_ap(ssid) || hgics_is_relay(ssid));
}

static int hgics_freq_to_channel(int freq)
{
    int i = 0;
    for (i = 0; i < 14; i++) {
        if (freq == freq_list[i]) {
            return i + 1;
        }
    }
    return 0;
}

static void hgics_gen_hostapd_conf()
{
#if 1 //sample code
    char *ssid;
    FILE *fp;
    int channel = hgics_freq_to_channel(g_hgrelay.wpastatus.freq);
    struct hgic_txq_param param[4];
    param[0].acm    = 0;
    param[0].aifs   = 4;
    param[0].cw_max = 1023;
    param[0].cw_min = 15;
    param[0].txop   = 0;
    param[1].acm    = 0;
    param[1].aifs   = 7;
    param[1].cw_max = 1023;
    param[1].cw_min = 15;
    param[1].txop   = 0;
    param[2].acm    = 0;
    param[2].aifs   = 2;
    param[2].cw_max = 15;
    param[2].cw_min = 7;
    param[2].txop   = 94;
    param[3].acm    = 0;
    param[3].aifs   = 2;
    param[3].cw_max = 7;
    param[3].cw_min = 3;
    param[3].txop   = 47;

    hgic_iwpriv_get_txq_param("wlan0", (char *)param, sizeof(param));
    fp = fopen("/etc/hostapd.conf", "w+");
    if (fp) {
        fprintf(fp, "ctrl_interface=/var/run/hostapd\n");
        fprintf(fp, "interface=wlan1\n");
        fprintf(fp, "driver=nl80211\n");
        fprintf(fp, "hw_mode=g\n");
        fprintf(fp, "ssid=%s#r#%02x%02x\n", g_hgrelay.cfg_ssid, (g_hgrelay.wpastatus.addr[4] & 0xff), (g_hgrelay.wpastatus.addr[5] & 0xff));
        fprintf(fp, "dtim_period=10\n");
        fprintf(fp, "beacon_int=100\n");
        fprintf(fp, "channel=%d\n", channel);
        fprintf(fp, "max_num_sta=255\n");
        fprintf(fp, "auth_algs=3\n");
        fprintf(fp, "ieee80211n=1\n");
        fprintf(fp, "wpa=2\n");
        fprintf(fp, "wpa_key_mgmt=WPA-PSK\n");
        fprintf(fp, "wpa_pairwise=CCMP\n");
        fprintf(fp, "rsn_pairwise=CCMP\n");
        fprintf(fp, "wpa_passphrase=12345678\n");
        fprintf(fp, "rts_threshold=1\n");
        fprintf(fp, "wmm_enabled=1\n");
        fprintf(fp, "wmm_ac_be_aifs=%d\n", param[0].aifs * 2);
        fprintf(fp, "wmm_ac_be_cwmin=%d\n", hgics_fls(param[0].cw_min));
        fprintf(fp, "wmm_ac_be_cwmax=%d\n", hgics_fls(param[0].cw_max));
        fprintf(fp, "wmm_ac_be_txop_limit=%d\n", param[0].txop);
        fprintf(fp, "wmm_ac_be_acm=%d\n", param[0].acm);
        fprintf(fp, "wmm_ac_bk_cwmin=%d\n", hgics_fls(param[1].cw_min));
        fprintf(fp, "wmm_ac_bk_cwmax=%d\n", hgics_fls(param[1].cw_max));
        fprintf(fp, "wmm_ac_bk_aifs=%d\n", param[1].aifs * 2);
        fprintf(fp, "wmm_ac_bk_txop_limit=%d\n", param[1].txop);
        fprintf(fp, "wmm_ac_bk_acm=%d\n", param[1].acm);
        fprintf(fp, "wmm_ac_vi_aifs=%d\n", param[2].aifs * 2);
        fprintf(fp, "wmm_ac_vi_cwmin=%d\n", hgics_fls(param[2].cw_min));
        fprintf(fp, "wmm_ac_vi_cwmax=%d\n", hgics_fls(param[2].cw_max));
        fprintf(fp, "wmm_ac_vi_txop_limit=%d\n", param[2].txop);
        fprintf(fp, "wmm_ac_vi_acm=%d\n", param[2].acm);
        fprintf(fp, "wmm_ac_vo_aifs=%d\n", param[3].aifs * 2);
        fprintf(fp, "wmm_ac_vo_cwmin=%d\n", hgics_fls(param[3].cw_min));
        fprintf(fp, "wmm_ac_vo_cwmax=%d\n", hgics_fls(param[3].cw_max));
        fprintf(fp, "wmm_ac_vo_txop_limit=%d\n", param[3].txop);
        fprintf(fp, "wmm_ac_vo_acm=%d\n", param[3].acm);
        fclose(fp);
    }
#endif
}

/*
static void hgics_wpacli_set_ssid(char *ssid)
{
    char cmd[128];
    char out[128];
    if (strcmp(ssid, g_hgrelay.cur_ssid)) {
        strcpy(g_hgrelay.cur_ssid, ssid);
        sprintf(cmd, "wpa_cli -iwlan0 set_network 0 ssid '\"%s\"'", ssid);
        hgics_do_system(cmd, out, 100);
        printf("wpa_cli set ssid %s: %s\r\n", ssid, out);
    }
}


static int hgics_wpacli_get_rssi()
{
    char *ptr;
    char buff[128];

    if (hgics_do_system("wpa_cli signal_poll", buff, 124) > 0) {
        ptr = strstr(buff, "AVG_RSSI=");
        if (ptr) {
            return atoi(ptr + 9);
        }
    }
    return 0;
}
*/

static void hgics_parse_wpacli_status(char *str)
{
    char *ptr = strstr(str, "bssid=");
    if (ptr) {
        hgic_str2mac(ptr + 6, g_hgrelay.wpastatus.bssid);
    }

    ptr = strstr(str, "freq=");
    if (ptr) {
        g_hgrelay.wpastatus.freq = atoi(ptr + 5);
    }

    ptr = strstr(str, "\nssid=");
    if (ptr) {
        hgics_strcpy(g_hgrelay.wpastatus.ssid, ptr + 6, 32);
    }

    ptr = strstr(str, "pairwise_cipher=");
    if (ptr) {
        hgics_strcpy(g_hgrelay.wpastatus.pairwise_cipher, ptr + 16, 10);
    }

    ptr = strstr(str, "group_cipher=");
    if (ptr) {
        hgics_strcpy(g_hgrelay.wpastatus.group_cipher, ptr + 13, 10);
    }

    ptr = strstr(str, "key_mgmt=");
    if (ptr) {
        hgics_strcpy(g_hgrelay.wpastatus.key_mgmt, ptr + 9, 10);
    }

    ptr = strstr(str, "address=");
    if (ptr) {
        hgic_str2mac(ptr + 8, g_hgrelay.wpastatus.addr);
    }

    g_hgrelay.wpastatus.wpa_state = strstr(str, "wpa_state=COMPLETED") ? 1 : 0;
}

static void hgics_check_signal()
{
    time_t t = time(NULL);
    int chan = hgics_freq_to_channel(g_hgrelay.wpastatus.freq);

    if (g_hgrelay.hwscan_running == 0 && hgics_wpacli_get_rssi() < g_hgrelay.rssi_th &&
        (g_hgrelay.hwscan_time == 0 || g_hgrelay.hwscan_time + 5 < t)) {
        if (!hgic_iwpriv_set_hwscan("wlan0", 100, 5, BIT(chan - 1))) {
            g_hgrelay.hwscan_time    = t;
            g_hgrelay.hwscan_running = 1;
        }
    }
}

static void hgics_check_wpascan_result()
{
    char *token;
    char  max_ssid[32], max_r_ssid[32];
    int   max_rssi = -255, max_r_rssi = -255;
    int   rssi;
    char *ssid, *signal;
    char *buff;

    if (g_hgrelay.wifi_mode != 2) {
        return;
    }

    max_ssid[0] = 0;
    max_r_ssid[0] = 0;
    buff = malloc(SCAN_RESULT_LEN);
    if (buff) {
        if (hgics_do_system("wpa_cli scan_results", buff, SCAN_RESULT_LEN - 1) > 0) {
            token = strtok(buff, "\n");
            token = strtok(NULL, "\n");
            while (token != NULL) {
                ssid   = hgics_strchr(token, '\t', 4);
                signal = hgics_strchr(token, '\t', 2);
                if (ssid && signal && hgics_is_suitable_ap(ssid)) {
                    rssi = atoi(signal);
                    if (hgics_is_ap(ssid)) {
                        if (rssi > max_rssi) {
                            hgics_strcpy(max_ssid, ssid, 32);
                            max_rssi = rssi;
                        }
                    } else {
                        if (rssi > max_r_rssi) {
                            hgics_strcpy(max_r_ssid, ssid, 32);
                            max_r_rssi = rssi;
                        }
                    }
                }
                token = strtok(NULL, "\n");
            }
        }
        free(buff);
    }

    if (max_ssid[0] || max_r_ssid[0]) {
        if (max_ssid[0] == 0)         ssid = max_r_ssid;
        else if (max_r_ssid[0] == 0)  ssid = max_ssid;
        else {
            ssid = ((max_rssi < -50 && max_r_rssi > max_rssi + 6) ? max_r_ssid : max_ssid);
        }
        hgics_wpacli_set_ssid(ssid);
        printf("WPASCAN: Find new AP, connect to %s\r\n", ssid);
    }
}

int hgics_relay_init(char *ssid, int mode)
{
    memset(&g_hgrelay, 0, sizeof(g_hgrelay));
    g_hgrelay.rssi_th = -65;

    if (ssid) {
        strcpy(g_hgrelay.cfg_ssid, ssid);
        strcpy(g_hgrelay.cur_ssid, ssid);
        g_hgrelay.wifi_mode = mode;
    }
}

void hgics_relay_check_status(void)
{
    char  buff[256];
    char *ptr;

    if (g_hgrelay.wifi_mode != 2) {
        return;
    }

    if (hgics_do_system("wpa_cli status", buff, 256) > 0) {
        memset(&g_hgrelay.wpastatus, 0, sizeof(g_hgrelay.wpastatus));
        hgics_parse_wpacli_status(buff);
        if (g_hgrelay.last_wpa_state != g_hgrelay.wpastatus.wpa_state) {
            g_hgrelay.hwscan_running = 0;
            if (g_hgrelay.wpastatus.wpa_state) {
                if (strstr(g_hgrelay.wpastatus.ssid, "#r#") == NULL) {
                    hgics_gen_hostapd_conf();
                    printf("connected AP, enable my AP function\r\n");
                    system("hostapd /etc/hostapd.conf &");
                } else {
                    printf("connect to repeater %s, disable my AP function\r\n", g_hgrelay.wpastatus.ssid);
                    hgic_iwpriv_set_rts_threshold("wlan0", 1);
                    system("killall hostapd");
                }
            } else {
                printf("disconnect, disable my AP function\r\n");
                system("killall hostapd");
            }
        } else {
            if (g_hgrelay.wpastatus.wpa_state) {
                hgics_check_signal();
            } else {
                g_hgrelay.hwscan_running = 0;
                if (g_hgrelay.connecting == 0) {
                    g_hgrelay.connecting = 20;
                    hgics_check_wpascan_result();
                } else {
                    g_hgrelay.connecting--;
                }
            }
        }
        g_hgrelay.last_wpa_state = g_hgrelay.wpastatus.wpa_state;
    }
}

static void hgics_hwscan_result_dump(char *data, int len)
{
    int i = 0;
    int count = len / sizeof(struct hgic_bss_info);
    struct hgic_bss_info *bssinfo = (struct hgic_bss_info *)data;

    printf("HWSCAN RESULTS:\n");
    for (i = 0; i < count; i++) {
        printf(MACSTR" ssid:%s, freq:%d, signal:%d \r\n", MACARG(bssinfo->bssid), bssinfo->ssid, bssinfo->freq, bssinfo->signal);
        bssinfo++;
    }
}

void hgics_relay_check_hwscan_result(char *data, int len)
{
    int i = 0;
    int count = len / sizeof(struct hgic_bss_info);
    struct hgic_bss_info *bssinfo = (struct hgic_bss_info *)data;
    struct hgic_bss_info *max   = NULL;
    struct hgic_bss_info *max_r = NULL;

    g_hgrelay.hwscan_running = 0;
    if (g_hgrelay.wifi_mode != 2) {
        hgics_hwscan_result_dump(data, len);
        return;
    }

    hgic_dbg("HWSCAN RESULTS:\n");
    for (i = 0; i < count; i++) {
        printf(MACSTR" ssid:%s, freq:%d, signal:%d \r\n", MACARG(bssinfo->bssid), bssinfo->ssid, bssinfo->freq, bssinfo->signal);
        if (hgics_is_suitable_ap(bssinfo->ssid) && !MAC_EQU(bssinfo->bssid, g_hgrelay.wpastatus.bssid)) {
            if(hgics_is_ap(bssinfo->ssid)){
                if (max == NULL || max->signal < bssinfo->signal) {
                    max = bssinfo;
                }
            }else{
                if (max_r == NULL || max_r->signal < bssinfo->signal) {
                    max_r = bssinfo;
                }
            }
        }
        bssinfo++;
    }

    if(max || max_r){
        if(max == NULL)        bssinfo = max_r;
        else if(max_r == NULL) bssinfo = max;
        else{
            bssinfo = ((max->signal < -50 && max_r->signal > max->signal + 6) ? max_r : max);
        }

        if (bssinfo->signal > (hgics_wpacli_get_rssi() + 5) && strlen(bssinfo->ssid) > 0) {
            hgics_wpacli_set_ssid(bssinfo->ssid);
            hgic_dbg("HWSCAN: Find new AP, switch to %s\r\n", bssinfo->ssid);
        }
    }
}

