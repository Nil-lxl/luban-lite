/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#include "wifi_module.h"
#include "wifi_store.h"
#include "wifi_connect_service.h"
#include "../core/module.h"
#include "../core/msg_def.h"
#include "../core/msg_bus.h"
#include <string.h>
#include <stdio.h>
#include <rtthread.h>
#include <rtdbg.h>

#define DBG_SECTION_NAME "WIFI_MOD"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#define WIFI_THREAD_NAME        "wifi_module"
#define WIFI_THREAD_STACK_SIZE  (8192)
#define WIFI_THREAD_PRIORITY    20
#define WIFI_THREAD_TICK        5

#define WIFI_CONNECT_TIMEOUT_MS 15000
#define WIFI_RSSI_POLL_MS       10000

static int wifi_msg_handler(const app_message_t *msg);

static int8_t wifi_rssi_to_level(int dbm)
{
    if (dbm >= -55)
        return 3;
    if (dbm >= -65)
        return 2;
    if (dbm >= -75)
        return 1;
    return 0;
}

typedef struct {
    wifi_state_t   state;
    wifi_store_t   store;
    char           mac_str[24];
    rt_bool_t      inited_stack;
    uint32_t       last_rssi_tick;
    int              connect_round; /* Attempts in current flow, max 2 */
} wifi_module_priv_t;

static wifi_module_priv_t wifi_priv;

static void wifi_send_msg_data(msg_type_t mtype, wifi_state_t st, const char *ssid,
                               const char *psk, int rssi_dbm, int8_t level,
                               const char *mac, uint8_t reason)
{
    app_message_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.header.type = mtype;
    msg.header.src_module = MODULE_ID_WIFI;
    msg.header.dst_module = 0;
    msg.header.data_len = sizeof(msg_wifi_data_t);
    msg.data.wifi.state = st;
    msg.data.wifi.rssi = rssi_dbm;
    msg.data.wifi.rssi_level = level;
    msg.data.wifi.provision_reason = reason;
    if (ssid)
        strncpy(msg.data.wifi.ssid, ssid, sizeof(msg.data.wifi.ssid) - 1);
    if (psk)
        strncpy(msg.data.wifi.password, psk, sizeof(msg.data.wifi.password) - 1);
    if (mac)
        strncpy(msg.data.wifi.mac_str, mac, sizeof(msg.data.wifi.mac_str) - 1);
    module_send_msg(&msg);
    wifi_priv.state = st;
}

static void wifi_send_connecting_ui(const char *ssid)
{
    wifi_send_msg_data(MSG_TYPE_WIFI_CONNECTING, WIFI_STATE_CONNECTING, ssid, NULL,
                       0, -1, NULL, 0);
}

static void wifi_send_need_provision_ui(uint8_t reason)
{
    wifi_send_msg_data(MSG_TYPE_WIFI_NEED_PROVISION, WIFI_STATE_NEED_PROVISION,
                       NULL, NULL, 0, -1, wifi_priv.mac_str, reason);
}

static void wifi_send_provisioning_ui(void)
{
    wifi_send_msg_data(MSG_TYPE_WIFI_PROVISIONING, WIFI_STATE_PROVISIONING,
                       NULL, NULL, 0, -1, wifi_priv.mac_str, WIFI_PROVISION_REASON_USER);
}

static void wifi_send_session_failed_ui(void)
{
    wifi_send_msg_data(MSG_TYPE_WIFI_SESSION_FAILED, WIFI_STATE_SESSION_FAILED,
                       NULL, NULL, 0, -1, wifi_priv.mac_str,
                       WIFI_PROVISION_REASON_SESSION_FAIL);
}

static void wifi_send_net_ready_ui(void)
{
    int dbm = wifi_svc_get_rssi_dbm();
    int8_t lv = wifi_rssi_to_level(dbm);

    wifi_send_msg_data(MSG_TYPE_NET_READY, WIFI_STATE_NET_READY, wifi_priv.store.ssid,
                       NULL, dbm, lv, NULL, 0);
}

static void wifi_send_rssi_only_if_connected(void)
{
    if (wifi_priv.state != WIFI_STATE_NET_READY && wifi_priv.state != WIFI_STATE_CONNECTED)
        return;
    if (!wifi_svc_is_ready())
        return;
    {
        int dbm = wifi_svc_get_rssi_dbm();
        int8_t lv = wifi_rssi_to_level(dbm);
        wifi_send_msg_data(MSG_TYPE_WIFI_RSSI_LEVEL, WIFI_STATE_NET_READY, wifi_priv.store.ssid,
                           NULL, dbm, lv, NULL, 0);
    }
}

/**
 * @return 0 on success, -1 on failure (after 2 attempts)
 */
static int wifi_try_connect_twice(wifi_store_t *st)
{
    int attempt;

    if (!st || st->ssid[0] == '\0')
        return -1;

    for (attempt = 0; attempt < 2; attempt++) {
        wifi_priv.connect_round = attempt + 1;
        LOG_I("WiFi try %d/2 ssid=%s", attempt + 1, st->ssid);
        wifi_send_connecting_ui(st->ssid);

        if (wifi_svc_connect(st->ssid, st->psk) != 0) {
            rt_thread_mdelay(500);
            continue;
        }
        if (wifi_svc_wait_ready(WIFI_CONNECT_TIMEOUT_MS)) {
            st->ever_connected = RT_TRUE;
            st->last_ok_ts = (uint32_t)rt_tick_get();
            wifi_store_save(st);
            wifi_priv.store = *st;
            wifi_send_net_ready_ui();
            LOG_I("WiFi connected");
            return 0;
        }
        LOG_W("WiFi attempt %d timeout", attempt + 1);
        rt_thread_mdelay(300);
    }
    return -1;
}

static void wifi_enter_provision(uint8_t reason)
{
    if (reason == WIFI_PROVISION_REASON_SESSION_FAIL)
        wifi_send_session_failed_ui();
    wifi_send_need_provision_ui(reason);
    wifi_svc_provision_start();
}

static void wifi_thread_entry(void *parameter)
{
    app_message_t msg;

    RT_UNUSED(parameter);
    LOG_I("WiFi thread start");
    rt_thread_mdelay(800);

    wifi_svc_get_mac_str(wifi_priv.mac_str, sizeof(wifi_priv.mac_str));
    wifi_svc_set_sta_mode();
    wifi_priv.inited_stack = RT_TRUE;

    if (wifi_store_load(&wifi_priv.store) != 0) {
        memset(&wifi_priv.store, 0, sizeof(wifi_priv.store));
        wifi_priv.store.ver = WIFI_STORE_VER;
    }

    if (wifi_priv.store.ssid[0] == '\0') {
        LOG_I("No SSID in store -> provision (never ok path)");
        wifi_enter_provision(WIFI_PROVISION_REASON_NEVER_OK);
    } else {
        if (wifi_try_connect_twice(&wifi_priv.store) != 0) {
            LOG_W("STA connect failed twice -> provision UI");
            wifi_enter_provision(WIFI_PROVISION_REASON_SESSION_FAIL);
        }
    }

    wifi_priv.last_rssi_tick = rt_tick_get();

    while (1) {
        while (msg_bus_try_receive(MODULE_ID_WIFI, &msg) == RT_EOK) {
            wifi_msg_handler(&msg);
        }

        if ((rt_tick_get() - wifi_priv.last_rssi_tick) > rt_tick_from_millisecond(WIFI_RSSI_POLL_MS)) {
            wifi_priv.last_rssi_tick = rt_tick_get();
            wifi_send_rssi_only_if_connected();
        }

        rt_thread_mdelay(50);
    }
}

static int wifi_msg_handler(const app_message_t *msg)
{
    if (!msg) {
        return -RT_EINVAL;
    }

    switch (msg->header.type) {
    case MSG_TYPE_SYS_INIT:
        LOG_I("WiFi module SYS_INIT");
        break;

    case MSG_TYPE_NET_READY:
        wifi_priv.state = WIFI_STATE_NET_READY;
        break;

    case MSG_TYPE_WIFI_PROVISION_REQUEST:
        LOG_I("WiFi provision request (key/other)");
        wifi_svc_provision_stop();
        rt_thread_mdelay(50);
        wifi_send_provisioning_ui();
        wifi_svc_provision_start();
        break;

    case MSG_TYPE_WIFI_PROVISION_RESULT: {
        const char *ns = msg->data.wifi.ssid;
        const char *np = msg->data.wifi.password;
        int ok = (msg->data.wifi.state != WIFI_STATE_CONNECT_FAILED);

        if (!ok || !ns || ns[0] == '\0') {
            LOG_W("provision result fail");
            break;
        }
        strncpy(wifi_priv.store.ssid, ns, sizeof(wifi_priv.store.ssid) - 1);
        strncpy(wifi_priv.store.psk, np ? np : "", sizeof(wifi_priv.store.psk) - 1);
        wifi_priv.store.ver = WIFI_STORE_VER;
        wifi_store_save(&wifi_priv.store);

        /*
         * Provisioning thread has already called rt_wlan_connect successfully.
         * Running wifi_try_connect_twice again causes duplicate connect attempts,
         * abnormal reconnect paths, and up to 15s x 2 wait_ready delay with possible aic8800 "node is null" spam.
         * If L2 is already associated, only wait for DHCP (ready) and skip reconnect.
         */
        if (wifi_svc_is_sta_connected()) {
            if (!wifi_svc_is_ready())
                (void)wifi_svc_wait_ready(WIFI_CONNECT_TIMEOUT_MS);
            wifi_priv.store.ever_connected = RT_TRUE;
            wifi_priv.store.last_ok_ts = (uint32_t)rt_tick_get();
            wifi_store_save(&wifi_priv.store);
            wifi_send_net_ready_ui();
            LOG_I("provision: STA already associated, skip duplicate connect");
            break;
        }

        if (wifi_try_connect_twice(&wifi_priv.store) != 0) {
            wifi_enter_provision(WIFI_PROVISION_REASON_SESSION_FAIL);
        }
        break;
    }

    default:
        break;
    }
    return RT_EOK;
}

static int wifi_init(void)
{
    memset(&wifi_priv, 0, sizeof(wifi_priv));
    wifi_priv.state = WIFI_STATE_WATE_OPEN;

    msg_bus_subscribe(MODULE_ID_WIFI, MSG_TYPE_NET_READY);
    msg_bus_subscribe(MODULE_ID_WIFI, MSG_TYPE_SYS_INIT);
    msg_bus_subscribe(MODULE_ID_WIFI, MSG_TYPE_WIFI_PROVISION_REQUEST);
    msg_bus_subscribe(MODULE_ID_WIFI, MSG_TYPE_WIFI_PROVISION_RESULT);

    LOG_I("WiFi module init");
    return RT_EOK;
}

static int wifi_deinit(void)
{
    msg_bus_unsubscribe(MODULE_ID_WIFI, MSG_TYPE_NET_READY);
    msg_bus_unsubscribe(MODULE_ID_WIFI, MSG_TYPE_SYS_INIT);
    msg_bus_unsubscribe(MODULE_ID_WIFI, MSG_TYPE_WIFI_PROVISION_REQUEST);
    msg_bus_unsubscribe(MODULE_ID_WIFI, MSG_TYPE_WIFI_PROVISION_RESULT);
    msg_bus_unsubscribe(MODULE_ID_WIFI, MSG_TYPE_SYS_UNKNOWN);
    return RT_EOK;
}

void wifi_module_request_provision(void)
{
    app_message_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_WIFI_PROVISION_REQUEST;
    msg.header.src_module = MODULE_ID_SYS;
    msg.header.dst_module = MODULE_ID_WIFI;
    msg.header.data_len = 0;
    module_send_msg(&msg);
}

void wifi_module_notify_provision_result(const char *ssid, const char *psk, int ok)
{
    app_message_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_WIFI_PROVISION_RESULT;
    msg.header.src_module = MODULE_ID_SYS;
    msg.header.dst_module = MODULE_ID_WIFI;
    msg.header.data_len = sizeof(msg_wifi_data_t);
    msg.data.wifi.state = ok ? WIFI_STATE_CONNECTED : WIFI_STATE_CONNECT_FAILED;
    if (ssid)
        strncpy(msg.data.wifi.ssid, ssid, sizeof(msg.data.wifi.ssid) - 1);
    if (psk)
        strncpy(msg.data.wifi.password, psk, sizeof(msg.data.wifi.password) - 1);
    module_send_msg(&msg);
}

/** Callback from provisioning service (wifi_connect_service.c) after UDP/STA completes; forward into message bus flow */
void wifi_svc_provision_done_cb(const char *ssid, const char *psk, int ok)
{
    wifi_module_notify_provision_result(ssid, psk, ok);
}

static module_ops_t wifi_ops = {
    .init = wifi_init,
    .deinit = wifi_deinit,
    .msg_handler = wifi_msg_handler,
    .name = "WiFi Module",
    .id = MODULE_ID_WIFI,
};

static module_t wifi_module = {
    .ops = &wifi_ops,
    .active = RT_FALSE,
    .thread = RT_NULL,
    .priv_data = &wifi_priv,
};

module_t *wifi_module_get(void)
{
    return &wifi_module;
}

int wifi_module_thread_start(void)
{
    wifi_module.thread = rt_thread_create(WIFI_THREAD_NAME,
                                          wifi_thread_entry,
                                          RT_NULL,
                                          WIFI_THREAD_STACK_SIZE,
                                          WIFI_THREAD_PRIORITY,
                                          WIFI_THREAD_TICK);
    if (wifi_module.thread != RT_NULL) {
        rt_thread_startup(wifi_module.thread);
        return RT_EOK;
    }
    return -RT_ERROR;
}
