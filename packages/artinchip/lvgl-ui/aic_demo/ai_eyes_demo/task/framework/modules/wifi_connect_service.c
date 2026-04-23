/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 *
 * Merged from packages/artinchip/wifi-connect (UDP provisioning + SoftAP),
 * integrated with wifi_module via wifi_svc_provision_done_cb().
 */

#include "wifi_connect_service.h"
#include <string.h>
#include <stdio.h>
#include <rtthread.h>

#ifdef RT_USING_WIFI
#include <wlan_mgnt.h>
#endif

#if defined(RT_USING_WIFI)
#include <poll.h>
/* RT-Thread + lwIP: use lwip headers instead of netinet/in.h (BSD headers may be absent in toolchain) */
#include <lwip/sockets.h>
#endif

#include <rtdbg.h>
#define DBG_SECTION_NAME "WIFI_SVC"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

/* ========== Protocol/state aligned with aic_wifi_core.h ========== */

typedef enum {
    STA_STATE_CONNECT = 0,
    STA_STATE_CONNECTING,
    STA_STATE_DISCONNECT,
    STA_STATE_CONNECT_FAIL,
    STA_STATE_PROBE_ACK = 4,
    STA_STATE_CRED_RECEIVED = 5,
} sta_conns_state_t;

typedef struct {
    int32_t msg_id;
    char ssid[32];
    char password[32];
    sta_conns_state_t state;
} network_ap_t;

typedef struct {
    char ssid[32];
    char password[32];
} aic_wifi_ap_t;

#define WIFI_SOCK_THREAD_NAME     "wifi_prov_sock"
#define CON_CHECK_PERIOD_MS       500
#define CONNECT_TIMEOUT_MS        15000
/* Slightly reduced count from original wifi-connect to lower SDIO RX/WPA concurrency pressure */
#define SUCCESS_SEND_COUNT        3
#define SUCCESS_SEND_DELAY_MS     120
#define AP_STOP_DELAY_MS          300
#define PROV_RX_BUF_SZ            1024
#define PROV_MQ_MSG_MAX           3
#define PROV_THREAD_STACK         4096
#define PROV_THREAD_PRIO          10
#define PROV_SOCK_THREAD_STACK    4096
#define PROV_SOCK_THREAD_PRIO     10
#define PROV_MQ_RECV_MS           500

#ifdef RT_USING_WIFI

static struct rt_timer s_con_check_timer;
static volatile sta_conns_state_t s_link_state = STA_STATE_DISCONNECT;
static rt_uint32_t s_connect_start_tick;
static rt_mq_t s_prov_mq = RT_NULL;
static rt_sem_t s_prov_done_sem = RT_NULL;
static volatile rt_bool_t s_prov_stop_req = RT_FALSE;
static volatile rt_bool_t s_prov_running = RT_FALSE;
static rt_thread_t s_prov_tid = RT_NULL;
static rt_thread_t s_sock_tid = RT_NULL;

static int s_sock_fd = -1;
static struct sockaddr_in s_client_addr;
static socklen_t s_client_len;
static uint8_t s_rx_buf[PROV_RX_BUF_SZ];
static uint8_t s_tx_buf[64];

static char s_last_ssid[32];
static char s_last_psk[32];

/** Allow only one successful AP stop per provisioning round to avoid repeated rt_wlan_ap_stop and driver "node is null" logs */
static volatile rt_bool_t s_prov_ap_stop_done = RT_FALSE;
/** After STA provisioning success, skip rt_wlan_ap_stop temporarily (helps keep SoftAP in dual-band); reset by wifi_svc_provision_start */
static volatile rt_bool_t s_prov_keep_ap_after_success = RT_FALSE;

static void wifi_prov_connection_check(void *parameter);

/**
 * Safely stop SoftAP: deduplicate and check is_active first.
 * After STA switches to target AP, SoftAP may already be removed by driver; extra ap_stop can trigger "node is null".
 */
static void wifi_prov_ap_stop_safe(void)
{
    if (s_prov_keep_ap_after_success) {
        LOG_I("prov: keep SoftAP (no rt_wlan_ap_stop)");
        s_prov_ap_stop_done = RT_TRUE;
        return;
    }
    if (s_prov_ap_stop_done)
        return;
    if (!rt_wlan_ap_is_active()) {
        s_prov_ap_stop_done = RT_TRUE;
        return;
    }
    rt_wlan_ap_stop();
    s_prov_ap_stop_done = RT_TRUE;
}

static int wifi_prov_parse_ap(const uint8_t *rx_buffer, network_ap_t *nw_ap)
{
    int offset = 0;
    int32_t msg_id_raw;

    memcpy(&msg_id_raw, rx_buffer + offset, sizeof(msg_id_raw));
    nw_ap->msg_id = (int32_t)ntohl((uint32_t)msg_id_raw);
    offset += sizeof(msg_id_raw);

    memset(nw_ap->ssid, 0, sizeof(nw_ap->ssid));
    memcpy(nw_ap->ssid, rx_buffer + offset, sizeof(nw_ap->ssid));
    nw_ap->ssid[sizeof(nw_ap->ssid) - 1] = '\0';
    offset += sizeof(nw_ap->ssid);

    memset(nw_ap->password, 0, sizeof(nw_ap->password));
    memcpy(nw_ap->password, rx_buffer + offset, sizeof(nw_ap->password));
    nw_ap->password[sizeof(nw_ap->password) - 1] = '\0';
    offset += sizeof(nw_ap->password);

    return offset;
}

static void wifi_prov_socket_send(sta_conns_state_t st)
{
    int offset = 0;

    if (s_sock_fd < 0 || s_client_len == 0) {
        LOG_W("prov socket_send: no client");
        return;
    }
    memset(s_tx_buf, 0, sizeof(s_tx_buf));
    memcpy(s_tx_buf + offset, &st, sizeof(st));
    offset += sizeof(st);
    sendto(s_sock_fd, s_tx_buf, offset, 0, (struct sockaddr *)&s_client_addr, s_client_len);
}

static void wifi_prov_sock_entry(void *para)
{
    struct sockaddr_in recv_addr;
    socklen_t addr_len = sizeof(recv_addr);
    struct pollfd pollfds[1];
    network_ap_t nw_ap;
    aic_wifi_ap_t ap;
    uint32_t last_print = rt_tick_get();

    RT_UNUSED(para);

    s_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s_sock_fd < 0) {
        LOG_E("prov socket create failed");
        return;
    }

    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_port = htons(WIFI_SVC_PROV_UDP_PORT);
    srv.sin_addr.s_addr = INADDR_ANY;

    if (bind(s_sock_fd, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
        LOG_E("prov bind failed");
        closesocket(s_sock_fd);
        s_sock_fd = -1;
        return;
    }

    LOG_I("UDP provision on port %d", WIFI_SVC_PROV_UDP_PORT);

    pollfds[0].fd = s_sock_fd;
    pollfds[0].events = POLLIN;

    /* Check stop first: in dual-band, AP may still live after STA success; use s_prov_stop_req to end thread before reclaiming mq */
    while (!s_prov_stop_req) {
        int ret;

        if (!rt_wlan_ap_is_active())
            break;

        pollfds[0].revents = 0;
        ret = poll(pollfds, 1, 10);
        if (ret > 0 && (pollfds[0].revents & POLLIN)) {
            int recv_len = recvfrom(s_sock_fd, s_rx_buf, sizeof(s_rx_buf), 0,
                                    (struct sockaddr *)&recv_addr, &addr_len);
            if (recv_len > 0) {
                last_print = rt_tick_get();
                memcpy(&s_client_addr, &recv_addr, sizeof(s_client_addr));
                s_client_len = addr_len;
                memset(&nw_ap, 0, sizeof(nw_ap));
                wifi_prov_parse_ap(s_rx_buf, &nw_ap);
                if (nw_ap.ssid[0] != '\0') {
                    memset(&ap, 0, sizeof(ap));
                    memcpy(ap.ssid, nw_ap.ssid, sizeof(ap.ssid));
                    memcpy(ap.password, nw_ap.password, sizeof(ap.password));
                    wifi_prov_socket_send(STA_STATE_CRED_RECEIVED);
                    rt_thread_mdelay(100);
                    if (s_prov_mq != RT_NULL)
                        rt_mq_send(s_prov_mq, &ap, sizeof(ap));
                } else {
                    wifi_prov_socket_send(STA_STATE_PROBE_ACK);
                }
            }
        } else if (ret == 0) {
            if ((rt_tick_get() - last_print) > rt_tick_from_millisecond(10000)) {
                LOG_I("waiting UDP on port %d\n", WIFI_SVC_PROV_UDP_PORT);
                last_print = rt_tick_get();
            }
            rt_thread_yield();
        } else {
            rt_thread_yield();
        }
    }

    if (s_sock_fd >= 0) {
        closesocket(s_sock_fd);
        s_sock_fd = -1;
    }
    LOG_I("prov sock thread exit");
}

static void wifi_prov_connection_check(void *parameter)
{
    int i;

    RT_UNUSED(parameter);

    if (s_prov_stop_req)
        return;

    if (s_link_state != STA_STATE_CONNECTING)
        return;

    /*
     * Do not call rt_thread_mdelay in soft-timer context: it can block the whole soft-timer queue,
     * delaying SDIO RX/protocol handling and causing sdio_buf_alloc failure with "node is null" spam.
     * Only do fast sends here; delays and done_cb are handled in wifi_prov thread.
     */
    if (rt_wlan_is_connected()) {
        LOG_I("STA connected, notify provisioning client");
        s_link_state = STA_STATE_CONNECT;
        s_prov_keep_ap_after_success = RT_TRUE;
        for (i = 0; i < SUCCESS_SEND_COUNT; i++)
            wifi_prov_socket_send(STA_STATE_CONNECT);
        rt_timer_stop(&s_con_check_timer);
        if (s_prov_done_sem != RT_NULL)
            rt_sem_release(s_prov_done_sem);
        return;
    }

    if ((rt_tick_get() - s_connect_start_tick) > rt_tick_from_millisecond(CONNECT_TIMEOUT_MS)) {
        LOG_W("STA connect timeout");
        s_link_state = STA_STATE_CONNECT_FAIL;
        for (i = 0; i < SUCCESS_SEND_COUNT; i++)
            wifi_prov_socket_send(STA_STATE_CONNECT_FAIL);
        wifi_prov_ap_stop_safe();
        rt_timer_stop(&s_con_check_timer);
        if (s_prov_done_sem != RT_NULL)
            rt_sem_release(s_prov_done_sem);
    }
}

static int wifi_prov_start_ap(void)
{
    rt_err_t ret;

    ret = rt_wlan_set_mode(WIFI_SVC_AP_DEVICE_NAME, RT_WLAN_AP);
    if (ret != RT_EOK) {
        LOG_W("set_mode AP failed: %d", ret);
        return -1;
    }
    ret = rt_wlan_start_ap(WIFI_SVC_PROV_AP_SSID, WIFI_SVC_PROV_AP_PSK);
    if (ret != RT_EOK) {
        LOG_W("start_ap failed: %d", ret);
        return -1;
    }
    return 0;
}

static void wifi_prov_cleanup_resources(void)
{
    rt_timer_stop(&s_con_check_timer);
    rt_timer_detach(&s_con_check_timer);

    if (s_sock_fd >= 0) {
        closesocket(s_sock_fd);
        s_sock_fd = -1;
    }

    /* On STA success, s_prov_keep_ap_after_success is set by timer; wifi_prov_ap_stop_safe will skip ap_stop */
    wifi_prov_ap_stop_safe();

    if (s_prov_mq != RT_NULL) {
        rt_mq_delete(s_prov_mq);
        s_prov_mq = RT_NULL;
    }

    if (s_prov_done_sem != RT_NULL) {
        rt_sem_delete(s_prov_done_sem);
        s_prov_done_sem = RT_NULL;
    }

    s_sock_tid = RT_NULL;
    s_prov_tid = RT_NULL;
    s_prov_running = RT_FALSE;
    s_client_len = 0;
    s_link_state = STA_STATE_DISCONNECT;
    s_prov_stop_req = RT_FALSE;
}

static void wifi_prov_thread_entry(void *param)
{
    aic_wifi_ap_t ap;
    rt_err_t err;

    RT_UNUSED(param);

    s_prov_ap_stop_done = RT_FALSE;
    s_prov_keep_ap_after_success = RT_FALSE;
    s_prov_stop_req = RT_FALSE;
    s_client_len = 0;
    memset(s_last_ssid, 0, sizeof(s_last_ssid));
    memset(s_last_psk, 0, sizeof(s_last_psk));

    s_prov_done_sem = rt_sem_create("wifi_prov_done", 0, RT_IPC_FLAG_FIFO);
    s_prov_mq = rt_mq_create("wifi_prov", sizeof(aic_wifi_ap_t), PROV_MQ_MSG_MAX, RT_IPC_FLAG_FIFO);
    if (s_prov_mq == RT_NULL || s_prov_done_sem == RT_NULL) {
        LOG_E("prov mq/sem create failed");
        if (s_prov_done_sem != RT_NULL) {
            rt_sem_delete(s_prov_done_sem);
            s_prov_done_sem = RT_NULL;
        }
        if (s_prov_mq != RT_NULL) {
            rt_mq_delete(s_prov_mq);
            s_prov_mq = RT_NULL;
        }
        s_prov_running = RT_FALSE;
        return;
    }

    rt_timer_init(&s_con_check_timer, "wifi_prov_ck", wifi_prov_connection_check, RT_NULL,
                  rt_tick_from_millisecond(CON_CHECK_PERIOD_MS),
                  RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);

    if (wifi_prov_start_ap() != 0) {
        LOG_E("SoftAP start failed");
        rt_timer_detach(&s_con_check_timer);
        rt_mq_delete(s_prov_mq);
        s_prov_mq = RT_NULL;
        rt_sem_delete(s_prov_done_sem);
        s_prov_done_sem = RT_NULL;
        s_prov_running = RT_FALSE;
        return;
    }

    rt_timer_start(&s_con_check_timer);

    s_sock_tid = rt_thread_create(WIFI_SOCK_THREAD_NAME, wifi_prov_sock_entry, RT_NULL,
                                  PROV_SOCK_THREAD_STACK, PROV_SOCK_THREAD_PRIO, 20);
    if (s_sock_tid == RT_NULL) {
        LOG_E("sock thread create failed");
        rt_timer_stop(&s_con_check_timer);
        rt_timer_detach(&s_con_check_timer);
        wifi_prov_ap_stop_safe();
        rt_mq_delete(s_prov_mq);
        s_prov_mq = RT_NULL;
        rt_sem_delete(s_prov_done_sem);
        s_prov_done_sem = RT_NULL;
        s_prov_running = RT_FALSE;
        return;
    }
    rt_thread_startup(s_sock_tid);

    while (!s_prov_stop_req) {
        int got = 0;

        if (rt_mq_recv(s_prov_mq, &ap, sizeof(ap), rt_tick_from_millisecond(PROV_MQ_RECV_MS)) == RT_EOK)
            got = 1;

        if (s_prov_stop_req)
            break;

        if (!got)
            continue;

        if (ap.ssid[0] == '\0')
            continue;

        LOG_I("prov credentials: ssid=%s", ap.ssid);
        strncpy(s_last_ssid, ap.ssid, sizeof(s_last_ssid) - 1);
        strncpy(s_last_psk, ap.password, sizeof(s_last_psk) - 1);

        rt_wlan_set_mode(WIFI_SVC_DEVICE_NAME, RT_WLAN_STATION);
        err = rt_wlan_connect(ap.ssid, ap.password);
        if (err != RT_EOK) {
            LOG_W("rt_wlan_connect failed: %d", err);
            wifi_svc_provision_done_cb(s_last_ssid, s_last_psk, 0);
            break;
        }

        s_link_state = STA_STATE_CONNECTING;
        s_connect_start_tick = rt_tick_get();

        err = rt_sem_take(s_prov_done_sem, rt_tick_from_millisecond(CONNECT_TIMEOUT_MS + 2000));
        if (s_prov_stop_req) {
            LOG_I("prov stopped by user");
            break;
        }
        if (err != RT_EOK) {
            LOG_W("prov wait done timeout");
            wifi_svc_provision_done_cb(s_last_ssid, s_last_psk, 0);
        } else {
            /* UDP was sent quickly in timer; add delay here for peer reception without blocking soft-timer thread */
            if (s_link_state == STA_STATE_CONNECT) {
                rt_thread_mdelay(SUCCESS_SEND_DELAY_MS);
                rt_thread_mdelay(AP_STOP_DELAY_MS);
            }
            rt_thread_mdelay(120);
            if (s_link_state == STA_STATE_CONNECT) {
                wifi_svc_provision_done_cb(s_last_ssid, s_last_psk, 1);
            } else if (s_link_state == STA_STATE_CONNECT_FAIL) {
                wifi_svc_provision_done_cb(s_last_ssid, s_last_psk, 0);
            } else {
                LOG_W("prov: unexpected link_state=%d", (int)s_link_state);
                wifi_svc_provision_done_cb(s_last_ssid, s_last_psk, 0);
            }
        }
        break;
    }

    /* Notify socket thread to exit (required in dual-band even if AP remains, otherwise mq cannot be reclaimed) */
    s_prov_stop_req = RT_TRUE;
    rt_thread_mdelay(450);
    /* Allow SDIO RX and lwIP to release before teardown */
    rt_thread_mdelay(200);
    wifi_prov_cleanup_resources();
    LOG_I("provision thread exit");
}

#endif /* RT_USING_WIFI */

int wifi_svc_set_sta_mode(void)
{
#ifdef RT_USING_WIFI
    rt_err_t e = rt_wlan_set_mode(WIFI_SVC_DEVICE_NAME, RT_WLAN_STATION);
    if (e != RT_EOK) {
        LOG_W("wifi_svc_set_sta_mode: %d", e);
        return -1;
    }
    return 0;
#else
    LOG_W("RT_USING_WIFI off");
    return -1;
#endif
}

int wifi_svc_connect(const char *ssid, const char *psk)
{
#ifdef RT_USING_WIFI
    rt_err_t e;
    if (!ssid)
        return -1;
    e = rt_wlan_connect((char *)ssid, (char *)(psk ? psk : ""));
    if (e != RT_EOK) {
        LOG_W("wifi_svc_connect failed: %d", e);
        return -1;
    }
    return 0;
#else
    return -1;
#endif
}

rt_bool_t wifi_svc_wait_ready(uint32_t timeout_ms)
{
#ifdef RT_USING_WIFI
    rt_tick_t deadline = rt_tick_get() + rt_tick_from_millisecond(timeout_ms);
    while (rt_tick_get() < deadline) {
        if (rt_wlan_is_ready())
            return RT_TRUE;
        rt_thread_mdelay(100);
    }
    return rt_wlan_is_ready() ? RT_TRUE : RT_FALSE;
#else
    (void)timeout_ms;
    return RT_FALSE;
#endif
}

void wifi_svc_get_mac_str(char *out, size_t len)
{
    uint8_t mac[8]={0};

    if (!out || len < 4) {
        return;
    }
    memset(out, 0, len);
#ifdef RT_USING_WIFI
    if (rt_wlan_get_mac(mac) == RT_EOK) {
        rt_snprintf(out, len, "%02x:%02x:%02x:%02x:%02x:%02x",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return;
    }
#endif
    strncpy(out, "--:--:--:--:--:--", len - 1);
}

int wifi_svc_get_rssi_dbm(void)
{
#ifdef RT_USING_WIFI
    int r = rt_wlan_get_rssi();
    return r;
#else
    return -127;
#endif
}

rt_bool_t wifi_svc_is_ready(void)
{
#ifdef RT_USING_WIFI
    return rt_wlan_is_ready() ? RT_TRUE : RT_FALSE;
#else
    return RT_FALSE;
#endif
}

rt_bool_t wifi_svc_is_sta_connected(void)
{
#ifdef RT_USING_WIFI
    return rt_wlan_is_connected() ? RT_TRUE : RT_FALSE;
#else
    return RT_FALSE;
#endif
}

void wifi_svc_provision_start(void)
{
#ifndef RT_USING_WIFI
    LOG_W("provision: RT_USING_WIFI off");
    return;
#else
    if (s_prov_running) {
        LOG_W("provision already running");
        return;
    }

    s_prov_ap_stop_done = RT_FALSE;
    s_prov_keep_ap_after_success = RT_FALSE;
    s_prov_running = RT_TRUE;
    s_prov_tid = rt_thread_create("wifi_prov", wifi_prov_thread_entry, RT_NULL,
                                  PROV_THREAD_STACK, PROV_THREAD_PRIO, 20);
    if (s_prov_tid == RT_NULL) {
        LOG_E("provision thread create failed");
        s_prov_running = RT_FALSE;
        return;
    }
    rt_thread_startup(s_prov_tid);
#endif
}

void wifi_svc_provision_stop(void)
{
#ifndef RT_USING_WIFI
    return;
#else
    s_prov_stop_req = RT_TRUE;

    rt_timer_stop(&s_con_check_timer);

    if (s_prov_done_sem != RT_NULL)
        rt_sem_release(s_prov_done_sem);

    if (!s_prov_keep_ap_after_success)
        wifi_prov_ap_stop_safe();

    if (s_sock_fd >= 0) {
        closesocket(s_sock_fd);
        s_sock_fd = -1;
    }

    /* Thread self-cleans s_prov_mq etc.; if still blocked on mq, socket thread exits after AP/stop */
#endif
}

/**
 * Default empty implementation; wifi_module.c provides strong symbol and forwards MSG_TYPE_WIFI_PROVISION_RESULT.
 */
RT_WEAK void wifi_svc_provision_done_cb(const char *ssid, const char *psk, int ok)
{
    (void)ssid;
    (void)psk;
    (void)ok;
}
