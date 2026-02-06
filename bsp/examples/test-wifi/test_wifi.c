/*
 * Copyright (c) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#include <rtthread.h>
#include <rtconfig.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netdev.h>
#include <math.h>
#include "aic_core.h"

#if defined(RT_USING_SAL) && defined(SAL_INTERNET_CHECK)
#error "SAL_INTERNET_CHECK should disabled in test-wifi when RT_USING_SAL"
#endif

typedef enum {
    ERR_NONE = 0,
    ERR_WIFI_INIT,
    ERR_WIFI_SCAN,
    ERR_WIFI_SCAN_NUM,
    ERR_WIFI_JOIN,
    ERR_WIFI_DISC,
    ERR_WIFI_START_AP,
    ERR_WIFI_STOP_AP,
    ERR_UNKNOWN
} WifiErrorCode;

extern rt_err_t ping(char* target_name, rt_uint32_t times, rt_size_t size);

static const char sopts[] = "s::a::p::t::b::c::xdlkh";
static const struct option lopts[] = {
    {"sta_test",    optional_argument, NULL, 's'},
    {"ap_test",     optional_argument, NULL, 'a'},
    {"ap_tcp",      optional_argument, NULL, 'p'},
    {"sta_tcp",     optional_argument, NULL, 't'},
    {"ap_udp",      optional_argument, NULL, 'b'},
    {"sta_udp",     optional_argument, NULL, 'c'},
    {"stress",            no_argument, NULL, 'x'},
    {"delete",            no_argument, NULL, 'd'},
    {"load",              no_argument, NULL, 'l'},
    {"del_load",          no_argument, NULL, 'k'},
    {"usage",             no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

static void usage(char *program)
{
    rt_kprintf("Usage: %s [options]: \n", program);
    rt_kprintf("\t -s, --sta_test\t\tSTA mode basic function test.if not set, use RT_WLAN_DEVICE_STA_NAME.\n");
    rt_kprintf("\t -a, --ap_test\t\tAP mode basic function test.if not set, use RT_WLAN_DEVICE_AP_NAME.\n");
    rt_kprintf("\t -p, --ap_tcp\t\tDevice set to AP mode,and set to iperf server for tcp.\n");
    rt_kprintf("\t -t, --sta_tcp\t\tDevice set to STA mode,and set to iperf server/client for tcp.\n");
    rt_kprintf("\t -b, --ap_udp\t\tDevice set to AP mode,and set to iperf server for udp.\n");
    rt_kprintf("\t -c, --sta_udp\t\tDevice set to STA mode,and set to iperf server/client for udp.\n");
    rt_kprintf("\t -x, --stress\t\tSwitching between AP and STA modes for infinite loop testing.\n");
    rt_kprintf("\t -d, --del_stress\t\tDelete the stress test thread.\n");
    rt_kprintf("\t -l, --load\t\tCreating load thread increases CPU burden.\n");
    rt_kprintf("\t -k, --del_load\t\tDeleet the load thread.\n");
    rt_kprintf("\t -h, --usage \n");
    rt_kprintf("\n");
    rt_kprintf("Example: %s -s / %s -swlan0\n", program, program);
    rt_kprintf("Example: %s -a / %s -awlan0\n", program, program);
    rt_kprintf("Example: %s -p\n", program);
    rt_kprintf("Example: %s -t / %s -tserver\n", program, program);
}

static struct rt_wlan_scan_result scan_result;
static struct rt_wlan_info *scan_filter = RT_NULL;
static rt_int32_t record_scan_num = 0;
static bool g_sta_test_flag = 0;
static bool g_ap_test_flag = 0;
static u32 g_stress_cnt = 0;
static u32 g_wifi_errcode[ERR_UNKNOWN] = {0};
static rt_thread_t wifi_stress_thread = RT_NULL;
static rt_sem_t wifi_stress_exit_sem = RT_NULL;
static volatile rt_bool_t wifi_stress_exit_flag = RT_FALSE;
static rt_thread_t load_thread;

static rt_bool_t aic_wifi_info_isequ(struct rt_wlan_info *info1, struct rt_wlan_info *info2)
{
    rt_bool_t is_equ = 1;
    rt_uint8_t bssid_zero[RT_WLAN_BSSID_MAX_LENGTH] = { 0 };

    if (is_equ && (info1->security != SECURITY_UNKNOWN) && (info2->security != SECURITY_UNKNOWN)) {
        is_equ &= info2->security == info1->security;
    }
    if (is_equ && ((info1->ssid.len > 0) && (info2->ssid.len > 0))) {
        is_equ &= info1->ssid.len == info2->ssid.len;
        is_equ &= rt_memcmp(&info2->ssid.val[0], &info1->ssid.val[0], info1->ssid.len) == 0;
    }
    if (is_equ && (rt_memcmp(&info1->bssid[0], bssid_zero, RT_WLAN_BSSID_MAX_LENGTH)) &&
       (rt_memcmp(&info2->bssid[0], bssid_zero, RT_WLAN_BSSID_MAX_LENGTH))) {
        is_equ &= rt_memcmp(&info1->bssid[0], &info2->bssid[0], RT_WLAN_BSSID_MAX_LENGTH) == 0;
    }
    if (is_equ && info1->datarate && info2->datarate) {
        is_equ &= info1->datarate == info2->datarate;
    }
    if (is_equ && (info1->channel >= 0) && (info2->channel >= 0)) {
        is_equ &= info1->channel == info2->channel;
    }
    if (is_equ && (info1->rssi < 0) && (info2->rssi < 0)) {
        is_equ &= info1->rssi == info2->rssi;
    }
    return is_equ;
}

static rt_err_t aic_wifi_scan_result_cache(struct rt_wlan_info *info)
{
    struct rt_wlan_info *ptable;
    rt_err_t err = RT_EOK;
    int i, insert = -1;
    rt_base_t level;

    if ((info == RT_NULL) || (info->ssid.len == 0)) return -RT_EINVAL;

    pr_debug("ssid:%s len:%d mac:%02x:%02x:%02x:%02x:%02x:%02x", info->ssid.val, info->ssid.len,
                  info->bssid[0], info->bssid[1], info->bssid[2], info->bssid[3], info->bssid[4], info->bssid[5]);

    /* scanning result filtering */
    level = rt_hw_interrupt_disable();
    if (scan_filter) {
        struct rt_wlan_info _tmp_info = *scan_filter;
        rt_hw_interrupt_enable(level);
        if (aic_wifi_info_isequ(&_tmp_info, info) != RT_TRUE) {
            return RT_EOK;
        }
    } else {
        rt_hw_interrupt_enable(level);
    }

    /* de-duplicatio */
    for (i = 0; i < scan_result.num; i++) {
        if ((info->ssid.len == scan_result.info[i].ssid.len) &&
                (rt_memcmp(&info->bssid[0], &scan_result.info[i].bssid[0], RT_WLAN_BSSID_MAX_LENGTH) == 0))
            return RT_EOK;
#ifdef RT_WLAN_SCAN_SORT
        if (insert >= 0)
            continue;

        /* Signal intensity comparison */
        if ((info->rssi < 0) && (scan_result.info[i].rssi < 0)) {
            if (info->rssi > scan_result.info[i].rssi) {
                insert = i;
                continue;
            } else if (info->rssi < scan_result.info[i].rssi) {
                continue;
            }
        }
        /* Channel comparison */
        if (info->channel < scan_result.info[i].channel) {
            insert = i;
            continue;
        } else if (info->channel > scan_result.info[i].channel) {
            continue;
        }
        /* data rate comparison */
        if ((info->datarate > scan_result.info[i].datarate)) {
            insert = i;
            continue;
        } else if (info->datarate < scan_result.info[i].datarate) {
            continue;
        }
#endif
    }

    /* Insert the end */
    if (insert == -1)
        insert = scan_result.num;

    if (scan_result.num >= RT_WLAN_SCAN_CACHE_NUM)
        return RT_EOK;

    /* malloc memory */
    ptable = rt_malloc(sizeof(struct rt_wlan_info) * (scan_result.num + 1));
    if (ptable == RT_NULL) {
        pr_err("wlan info malloc failed!");
        return -RT_ENOMEM;
    }
    scan_result.num ++;

    /* copy info */
    for (i = 0; i < scan_result.num; i++) {
        if (i < insert)
            ptable[i] = scan_result.info[i];
        else if (i > insert)
            ptable[i] = scan_result.info[i - 1];
        else if (i == insert)
            ptable[i] = *info;
    }
    rt_free(scan_result.info);
    scan_result.info = ptable;
    return err;
}

static void aic_wifi_scan_result_clean(void)
{
    /* If there is data */
    if (scan_result.num) {
        scan_result.num = 0;
        rt_free(scan_result.info);
        scan_result.info = RT_NULL;
    }
}

static void aic_print_ap_info(struct rt_wlan_info *info,int index)
{
    char *security;

    if(index == 0) {
        rt_kprintf("             SSID                      MAC            security    rssi chn Mbps\n");
        rt_kprintf("------------------------------- -----------------  -------------- ---- --- ----\n");
    }
    {
        rt_kprintf("%-32.32s", &(info->ssid.val[0]));
        rt_kprintf("%02x:%02x:%02x:%02x:%02x:%02x  ",
                   info->bssid[0],
                   info->bssid[1],
                   info->bssid[2],
                   info->bssid[3],
                   info->bssid[4],
                   info->bssid[5]
                  );
        switch (info->security) {
        case SECURITY_OPEN:
            security = "OPEN";
            break;
        case SECURITY_WEP_PSK:
            security = "WEP_PSK";
            break;
        case SECURITY_WEP_SHARED:
            security = "WEP_SHARED";
            break;
        case SECURITY_WPA_TKIP_PSK:
            security = "WPA_TKIP_PSK";
            break;
        case SECURITY_WPA_AES_PSK:
            security = "WPA_AES_PSK";
            break;
        case SECURITY_WPA2_AES_PSK:
            security = "WPA2_AES_PSK";
            break;
        case SECURITY_WPA2_TKIP_PSK:
            security = "WPA2_TKIP_PSK";
            break;
        case SECURITY_WPA2_MIXED_PSK:
            security = "WPA2_MIXED_PSK";
            break;
        case SECURITY_WPS_OPEN:
            security = "WPS_OPEN";
            break;
        case SECURITY_WPS_SECURE:
            security = "WPS_SECURE";
            break;
        default:
            security = "UNKNOWN";
            break;
        }
        rt_kprintf("%-14.14s ", security);
        rt_kprintf("%-4d ", info->rssi);
        rt_kprintf("%3d ", info->channel);
        rt_kprintf("%4d\n", info->datarate / 1000000);
    }
}

static void aic_user_ap_info_callback(int event, struct rt_wlan_buff *buff, void *parameter)
{
    struct rt_wlan_info *info = RT_NULL;
    int index = 0;
    int ret = RT_EOK;

    RT_ASSERT(event == RT_WLAN_EVT_SCAN_REPORT);
    RT_ASSERT(buff != RT_NULL);
    RT_ASSERT(parameter != RT_NULL);

    info = (struct rt_wlan_info *)buff->data;
    index = *((int *)(parameter));

    ret = aic_wifi_scan_result_cache(info);
    if(ret == RT_EOK) {
        if(scan_filter == RT_NULL ||
                (scan_filter != RT_NULL &&
                 scan_filter->ssid.len == info->ssid.len &&
                 memcmp(&scan_filter->ssid.val[0], &info->ssid.val[0], scan_filter->ssid.len) == 0)) {
            /*Print the info*/
            aic_print_ap_info(info,index);

            index++;
            *((int *)(parameter)) = index;
        }
    }
}

static void aic_scan_done_callback(int event, struct rt_wlan_buff *buff, void *parameter)
{
    RT_ASSERT(event == RT_WLAN_EVT_SCAN_DONE);
    RT_ASSERT(parameter != RT_NULL);

    record_scan_num = scan_result.num;
}

static int aic_wifi_scan(char *ssid)
{
    struct rt_wlan_info *info = RT_NULL;
    struct rt_wlan_info filter;
    int ret = 0;
    int i = 0;

    if (ssid) {
        INVALID_INFO(&filter);
        SSID_SET(&filter, ssid);
        info = &filter;
    }

    ret = rt_wlan_register_event_handler(RT_WLAN_EVT_SCAN_REPORT, aic_user_ap_info_callback, &i);
    if(ret != RT_EOK) {
        pr_err("Scan register user callback error:%d!\n", ret);
        return -1;
    }

    ret = rt_wlan_register_event_handler(RT_WLAN_EVT_SCAN_DONE, aic_scan_done_callback, &i);
    if(ret != RT_EOK) {
        pr_err("Scan register user callback error:%d!\n", ret);
        return -1;
    }

    if(info)
        scan_filter = info;

    /*Todo: what can i do for it return val */
    ret = rt_wlan_scan_with_info(info);
    if(ret != RT_EOK) {
        pr_err("Scan with info error:%d!\n", ret);
    }

    /* clean scan result */
    aic_wifi_scan_result_clean();
    if(info)
        scan_filter = RT_NULL;

    return 0;
}

static int aic_wifi_init(char *dev_name, char *set_mode)
{
    rt_err_t err = RT_EOK;
    rt_wlan_mode_t mode;
    char device[16] = {0};

    if (strncmp("sta", set_mode, 3) == 0) {
        mode = RT_WLAN_STATION;
    } else if (strncmp("ap", set_mode, 2) == 0) {
        mode = RT_WLAN_AP;
    } else if (strncmp("none", set_mode, 4) == 0) {
        mode = RT_WLAN_NONE;
    } else {
        return -1;
    }

    // if the device name is not set, set the default value.
    if (dev_name == NULL) {
        if (mode == RT_WLAN_STATION)
            snprintf(device, sizeof(device), "%s", RT_WLAN_DEVICE_STA_NAME);
        else if (mode == RT_WLAN_AP)
            snprintf(device, sizeof(device), "%s", RT_WLAN_DEVICE_AP_NAME);
    } else {
        snprintf(device, sizeof(device), "%s", dev_name);
    }

    err = rt_wlan_set_mode((const char*)device, mode);
    if (err != RT_EOK) {
        pr_err("wifi init err! errcode:%d\n", err);
        return -1;
    }

    return 0;
}

extern int netdev_cmd_ping(char* target_name, char *netdev_name, rt_uint32_t times, rt_size_t size);
static int aic_wifi_connect(char *ssid, char *password)
{
    int try_cnt = 0;
    rt_err_t err = RT_EOK;
    struct netdev *dev = netdev_get_by_name("w0");
    if (dev == NULL) {
        pr_err("get the netdev failed!\n");
        return -RT_EINVAL;
    }

    err = rt_wlan_connect(ssid, password);
    if (err < 0) {
        pr_err("WiFi connect failed!\n");
        return err;
    }

    while (dev->ip_addr.addr == 0) {
        rt_thread_mdelay(500);
        try_cnt++;
        if (try_cnt >= 10) {
            pr_err("WiFi dhcpc timeout, get ip err!\n");
            return -RT_EINVAL;
        }
    }
    rt_kprintf("IP ADDR:%s GW ADDR: %s\n", inet_ntoa(dev->ip_addr.addr), inet_ntoa(dev->gw));

    err = netdev_cmd_ping(inet_ntoa(dev->gw), RT_NULL, 4, 0);
    if (err < 0) {
        pr_err("WiFi ping gate way failed!\n");
        return err;
    }

    return 0;
}

static int aic_wifi_sta_basic_test(char *dev_name)
{
    int ret = 0;

    g_sta_test_flag = 0;

    //1.init the wifi to sta mode
    ret = aic_wifi_init(dev_name , "sta");
    if (ret < 0) {
        pr_err("WiFi initialization failed!\n");
        g_wifi_errcode[ERR_WIFI_INIT]++;
        return ERR_WIFI_INIT;
    }

    //2.wifi scan
    record_scan_num = 0;
    ret = aic_wifi_scan(NULL);
    if (ret < 0) {
        pr_err("WiFi scan start failed!\n");
        g_wifi_errcode[ERR_WIFI_SCAN]++;
        return ERR_WIFI_SCAN;
    }

    rt_kprintf("%s:get the scan result nums:%d\n", __func__, record_scan_num);

    if (record_scan_num <= 0) {
        pr_err("WiFi scan result empty, please check the RF\n");
        g_wifi_errcode[ERR_WIFI_SCAN_NUM]++;
        return ERR_WIFI_SCAN_NUM;
    }

    //3.open network test
    ret = aic_wifi_connect(OPEN_NETWORK_SSID, NULL);
    if (ret < 0) {
        pr_err("WiFi connect failed!\n");
        g_wifi_errcode[ERR_WIFI_JOIN]++;
        ret= ERR_WIFI_JOIN;
        goto err_join;
    }

    //4.WPA2_AES_PSK network test
    ret = aic_wifi_connect(WPA2_AES_PSK_NETWORK_SSID, WPA2_AES_PSK_NETWORK_PASSWORD);
    if (ret < 0) {
        pr_err("WiFi connect failed!\n");
        g_wifi_errcode[ERR_WIFI_JOIN]++;
        ret= ERR_WIFI_JOIN;
        goto err_join;
    }

    g_sta_test_flag = 1;

err_join:
    //5.disconnect
    ret = rt_wlan_disconnect();
    if (ret < 0) {
        pr_err("WiFi disconnet failed!\n");
        g_wifi_errcode[ERR_WIFI_DISC]++;
        return ERR_WIFI_DISC;
    }

    return ret;
}

static int aic_wifi_ap_basic_test(char *dev_name)
{
    int ret = 0;

    g_ap_test_flag = 0;

    //1.init the wifi to ap mode
    ret = aic_wifi_init(dev_name , "ap");
    if (ret < 0) {
        pr_err("WiFi initialization failed!\n");
        g_wifi_errcode[ERR_WIFI_INIT]++;
        return ERR_WIFI_INIT;
    }

    //2.gen the ap for test
    ret = rt_wlan_start_ap(AP_TEST_SSID, AP_TEST_PASSWORD);
    if (ret < 0) {
        pr_err("WiFi start ap failed!\n");
        g_wifi_errcode[ERR_WIFI_START_AP]++;
        return ERR_WIFI_START_AP;
    }

    rt_thread_mdelay(200);

    //3.stop the ap
    ret = rt_wlan_ap_stop();
    if (ret < 0) {
        pr_err("WiFi stop ap failed!\n");
        g_wifi_errcode[ERR_WIFI_STOP_AP]++;
        return ERR_WIFI_STOP_AP;
    }

    g_ap_test_flag = 1;

    return ret;
}

static void aic_wifi_report_err(void)
{
    rt_kprintf("\n=== WiFi Test Status ===\n");
    rt_kprintf("Error Counts:\n");
    rt_kprintf("  Init Failures: %d\n", g_wifi_errcode[ERR_WIFI_INIT]);
    rt_kprintf("  Scan Failures: %d\n", g_wifi_errcode[ERR_WIFI_SCAN]);
    rt_kprintf("  Scan NULL Failures: %d\n", g_wifi_errcode[ERR_WIFI_SCAN_NUM]);
    rt_kprintf("  Join Failures: %d\n", g_wifi_errcode[ERR_WIFI_JOIN]);
    rt_kprintf("  Disc Failures: %d\n", g_wifi_errcode[ERR_WIFI_DISC]);
    rt_kprintf("  StartAP Failures: %d\n", g_wifi_errcode[ERR_WIFI_START_AP]);
    rt_kprintf("  StopAP Failures: %d\n", g_wifi_errcode[ERR_WIFI_STOP_AP]);
    rt_kprintf("=======================\n");

    if (g_sta_test_flag && g_ap_test_flag)
        rt_kprintf("wifi stress test success %d times.\n", ++g_stress_cnt);
}

static void aic_wifi_stress_test()
{
    // basic test device name use RT_WLAN_DEVICE
    while (1) {
        aic_wifi_sta_basic_test(RT_WLAN_DEVICE_STA_NAME);
        rt_thread_mdelay(200);
        aic_wifi_ap_basic_test(RT_WLAN_DEVICE_AP_NAME);
        rt_thread_mdelay(200);

        /* check the heap */
        system("free");
#ifdef RT_MEMTRACE_BRIEF_DUMP
        system("memheaptrace");
#endif

        aic_wifi_report_err();

        if (wifi_stress_exit_flag) {
            rt_kprintf("wifi stress thread exit.\n");
            break;
        }
    }
    rt_sem_release(wifi_stress_exit_sem);
}

static int create_stress_test_thread(void)
{
    wifi_stress_exit_sem = rt_sem_create("exit_sem", 0, RT_IPC_FLAG_FIFO);
    if (wifi_stress_exit_sem == NULL) {
        rt_kprintf("wifi stress thread sem create failed.\n");
        return -1;
    }

    wifi_stress_thread = rt_thread_create("wifi_stress", aic_wifi_stress_test, RT_NULL, 10240, 19, 20);
    if (wifi_stress_thread != RT_NULL) {
        rt_thread_startup(wifi_stress_thread);
        return 0;
    }

    rt_sem_delete(wifi_stress_exit_sem);
    rt_kprintf("%s err return.\n", __FUNCTION__);
    return -1;
}

static void delete_stress_test_thread(void)
{
    if (!wifi_stress_thread) {
        rt_kprintf("wifi stress thread already exited.\n");
        return;
    }

    wifi_stress_exit_flag = RT_TRUE;

    if (rt_sem_take(wifi_stress_exit_sem, RT_WAITING_FOREVER) == RT_EOK) {
        rt_kprintf("delete the wifi stress thread.\n");
        rt_thread_delete(wifi_stress_thread);
        wifi_stress_thread = RT_NULL;

        rt_sem_delete(wifi_stress_exit_sem);
        wifi_stress_exit_flag = RT_FALSE;
    }
}

static int aic_wifi_iperf_sta(char *para, u8 mode)
{
    int ret = 0;
    char iperf_cmd[64] = {0};

    //1.stop the primary mode
    rt_wlan_ap_stop();
    rt_wlan_disconnect();
    snprintf(iperf_cmd, sizeof(iperf_cmd), "iperf --stop");
    system(iperf_cmd);
    memset(iperf_cmd, 0, sizeof(iperf_cmd));

    //2.init the wifi to sta mode
    ret = aic_wifi_init(NULL , "sta");
    if (ret < 0) {
        pr_err("WiFi initialization failed!\n");
        return ERR_WIFI_INIT;
    }

    //3.WPA2_AES_PSK network
    ret = aic_wifi_connect(AP_TEST_SSID, AP_TEST_PASSWORD);
    if (ret < 0) {
        pr_err("WiFi connect failed!\n");
        return ERR_WIFI_JOIN;
    }
    struct netdev *dev = netdev_get_by_name("w0");
    if (dev == NULL) {
        pr_err("get the netdev failed!\n");
        return -RT_EINVAL;
    }

    //0 is tcp, 1 is udp
    if (mode == 0) {
        if (para && strncmp(para, "server", 6) == 0)
            snprintf(iperf_cmd, sizeof(iperf_cmd), "iperf -s");
        else
            snprintf(iperf_cmd, sizeof(iperf_cmd), "iperf -c %s", inet_ntoa(dev->gw));
    } else {
        if (para && strncmp(para, "server", 6) == 0)
            snprintf(iperf_cmd, sizeof(iperf_cmd), "iperf -u -s");
        else
            snprintf(iperf_cmd, sizeof(iperf_cmd), "iperf -u -c %s", inet_ntoa(dev->gw));
    }

    system(iperf_cmd);
    return 0;
}

static int aic_wifi_iperf_ap(char *para, u8 mode)
{
    int ret = 0;
    char iperf_cmd[64] = {0};

    //1.stop the primary mode
    rt_wlan_ap_stop();
    rt_wlan_disconnect();
    snprintf(iperf_cmd, sizeof(iperf_cmd), "iperf --stop");
    system(iperf_cmd);
    memset(iperf_cmd, 0, sizeof(iperf_cmd));

    //2.init the wifi to ap mode
    ret = aic_wifi_init(NULL , "ap");
    if (ret < 0) {
        pr_err("WiFi initialization failed!\n");
        return ERR_WIFI_INIT;
    }

    //3.gen the ap for test
    ret = rt_wlan_start_ap(AP_TEST_SSID, AP_TEST_PASSWORD);
    if (ret < 0) {
        pr_err("WiFi start ap failed!\n");
        return ERR_WIFI_START_AP;
    }

    //0 is tcp, 1 is udp
    if (mode == 0) {
        if (para == NULL)
            snprintf(iperf_cmd, sizeof(iperf_cmd), "iperf -s");
    } else {
        if (para == NULL)
            snprintf(iperf_cmd, sizeof(iperf_cmd), "iperf -u -s");
    }
    system(iperf_cmd);
    return 0;
}

static void load_thread_entry(void * parameter)
{
    while (1) {
        volatile float value = 100.0f;
        for (int i = 0; i < 100000; i++) {
            value = sqrtf(value);
        }
        rt_thread_delay(1);
    }
}

static int create_load_thread(void)
{
    load_thread = rt_thread_create("load_thread", load_thread_entry, RT_NULL, 2048, 19, 10);

    if (load_thread != RT_NULL) {
        rt_thread_startup(load_thread);
        return 0;
    }
    return -1;
}

static void delete_load_thread(void)
{
    if (load_thread) {
        rt_thread_delete(load_thread);
        load_thread = RT_NULL;
    }
}

static int cmd_test_wifi(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    int option = 0;

    if (argc == 1) {
        usage(argv[0]);
        return 0;
    }

    optind = 0;
    while ((option = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (option) {
        case 's':
            ret = aic_wifi_sta_basic_test(optarg);
            if (ret != 0)
                ret = -RT_ERROR;
            break;
        case 'a':
            ret = aic_wifi_ap_basic_test(optarg);
            if (ret != 0)
                ret = -RT_ERROR;
            break;
        case 'p':
            ret = aic_wifi_iperf_ap(optarg, 0);
            if (ret != 0)
                ret = -RT_ERROR;
            break;
        case 't':
            ret = aic_wifi_iperf_sta(optarg, 0);
            if (ret != 0)
                ret = -RT_ERROR;
            break;
        case 'b':
            ret = aic_wifi_iperf_ap(optarg, 1);
            if (ret != 0)
                ret = -RT_ERROR;
            break;
        case 'c':
            ret = aic_wifi_iperf_sta(optarg, 1);
            if (ret != 0)
                ret = -RT_ERROR;
            break;
        case 'x':
            ret = create_stress_test_thread();
            if (ret != 0)
                ret = -RT_ERROR;
            break;
        case 'd':
            delete_stress_test_thread();
            break;
        case 'l':
            ret = create_load_thread();
            if (ret != 0)
                ret = -RT_ERROR;
            break;
        case 'k':
            delete_load_thread();
            break;
        case 'h':
            usage(argv[0]);
            break;
        default:
            pr_err("Invalid argument\n");
            ret = -RT_ERROR;
            usage(argv[0]);
            break;
        }
    }
    return ret;
}
MSH_CMD_EXPORT_ALIAS(cmd_test_wifi, test_wifi, wifi test demo);

