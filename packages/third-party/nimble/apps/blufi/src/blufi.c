/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <rtthread.h>
#include <wlan_cfg.h>

#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "blufi_protocol.h"
#include "blufi_api.h"
#include "blufi_prf.h"
#include "blufi.h"

struct blufi_wlan_cfg_info
{
    rt_wlan_mode_t mode;
    struct rt_wlan_cfg_info cfg;
};

struct blufi_wlan_cfg_info wlan_cfg_info;
static uint8_t blufi_addr_type;
static bool ble_is_connected = false;
static struct rt_work set_mode_work;
static struct rt_work wlan_connect_work;
static struct rt_work wlan_disconnect_work;

static int blufi_gap_event(struct ble_gap_event *event, void *arg);

void blufi_adv_start(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    uint8_t addr_val[6] = {0};
    const char *name;
    int rc;

    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &blufi_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(INFO, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Printing ADDR */
    rc = ble_hs_id_copy_addr(blufi_addr_type, addr_val, NULL);

    /**
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info).
     *     o Advertising tx power.
     *     o Device name.
     *     o 16-bit service UUIDs (alert notifications).
     */

    memset(&fields, 0, sizeof fields);

    /* Advertise two flags:
    *     o Discoverability in forthcoming advertisement (general)
     *     o BLE-only (BR/EDR unsupported).
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    /* Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    fields.uuids16 = (ble_uuid16_t[]) {
        BLE_UUID16_INIT(BLUFI_APP_UUID)
    };
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;
    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d", rc);
        return;
    }

    /* Begin advertising. */
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(blufi_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, blufi_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d", rc);
        return;
    }
}

void blufi_adv_stop(void)
{
    ble_gap_adv_stop();
}

void blufi_disconnect(void)
{
    ble_gap_terminate(blufi_env.conn_id, BLE_ERR_REM_USER_CONN_TERM);
}

void blufi_wlan_set_mode(struct rt_work *work, void *data)
{
        struct blufi_wlan_cfg_info *wlan_cfg_info = data;

        rt_wlan_set_mode("wlan0", wlan_cfg_info->mode); 
}

void blufi_wlan_connect(struct rt_work *work, void *data)
{
        struct blufi_wlan_cfg_info *wlan_cfg_info = data;

        rt_wlan_cfg_save(&wlan_cfg_info->cfg);
        rt_wlan_disconnect();
        rt_wlan_connect(wlan_cfg_info->cfg.info.ssid.val, wlan_cfg_info->cfg.key.val);
}

void blufi_wlan_disconnect(struct rt_work *work, void *data)
{
        rt_wlan_disconnect();
}

static void blufi_event_callback(blufi_cb_event_t event, blufi_cb_param_t *param)
{
    /* actually, should post to blufi_task handle the procedure,
     * now, as a example, we do it more simply */
    switch (event) {
    case BLUFI_EVENT_INIT_FINISH:
        MODLOG_DFLT_INFO("BLUFI init finish\n");
        blufi_adv_start();
        break;
    case BLUFI_EVENT_DEINIT_FINISH:
        MODLOG_DFLT_INFO("BLUFI deinit finish\n");
        break;
    case BLUFI_EVENT_BLE_CONNECT:
        MODLOG_DFLT_INFO("BLUFI ble connect\n");
        ble_is_connected = true;
        blufi_adv_stop();
        break;
    case BLUFI_EVENT_BLE_DISCONNECT:
        MODLOG_DFLT_INFO("BLUFI ble disconnect\n");
        ble_is_connected = false;
        blufi_adv_start();
        break;
    case BLUFI_EVENT_SET_WIFI_OPMODE:
        MODLOG_DFLT_INFO("BLUFI Set WIFI opmode %d\n", param->wifi_mode.op_mode);
        wlan_cfg_info.mode = param->wifi_mode.op_mode;
        rt_work_submit(&set_mode_work, 2500);
        break;
    case BLUFI_EVENT_REQ_CONNECT_TO_AP:
        MODLOG_DFLT_INFO("BLUFI request wifi connect to AP\n");
        /* there is no wifi callback when the device has already connected to this wifi
        so disconnect wifi before connection.
        */
#ifdef RT_WLAN_CFG_ENABLE
        rt_work_submit(&wlan_connect_work, 2500);
#endif
        break;
    case BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
        MODLOG_DFLT_INFO("BLUFI request wifi disconnect from AP\n");
        rt_work_submit(&wlan_disconnect_work, 2500);
        break;
    case BLUFI_EVENT_REPORT_ERROR:
        MODLOG_DFLT_ERROR("BLUFI report error, error code %d\n", param->report_error.state);
        blufi_send_error_info(param->report_error.state);
        break;
    case BLUFI_EVENT_GET_WIFI_STATUS: {
        MODLOG_DFLT_INFO("BLUFI get wifi status from AP\n");
        break;
    }
    case BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
        MODLOG_DFLT_INFO("blufi close a gatt connection");
        rt_work_submit(&wlan_disconnect_work, 2500);
        break;
    case BLUFI_EVENT_DEAUTHENTICATE_STA:
        /* TODO */
        break;
	case BLUFI_EVENT_RECV_STA_BSSID:
        MODLOG_DFLT_INFO("Recv STA BSSID %s\n", param->sta_bssid.bssid);
        break;
	case BLUFI_EVENT_RECV_STA_SSID:
        if (param->sta_ssid.ssid_len >= RT_WLAN_SSID_MAX_LENGTH) {
            MODLOG_DFLT_ERROR("STA SSID to long\n");
            break;
        }
        rt_memcpy(&wlan_cfg_info.cfg.info.ssid.val[0], param->sta_ssid.ssid, param->sta_ssid.ssid_len);
        wlan_cfg_info.cfg.info.ssid.len = param->sta_ssid.ssid_len;
        MODLOG_DFLT_INFO("Recv STA SSID %s\n", wlan_cfg_info.cfg.info.ssid.val);
        break;
	case BLUFI_EVENT_RECV_STA_PASSWD:
        if (param->sta_passwd.passwd_len >= RT_WLAN_PASSWORD_MAX_LENGTH) {
            MODLOG_DFLT_INFO("STA PASSWORD to long\n");
            break;
        }
        rt_memcpy(&wlan_cfg_info.cfg.key.val[0], param->sta_passwd.passwd, param->sta_passwd.passwd_len);
        wlan_cfg_info.cfg.key.len = param->sta_passwd.passwd_len;
        MODLOG_DFLT_INFO("Recv STA PASSWORD %s\n", wlan_cfg_info.cfg.key.val);
        break;
	case BLUFI_EVENT_RECV_SOFTAP_SSID:
        break;
	case BLUFI_EVENT_RECV_SOFTAP_PASSWD:
        break;
	case BLUFI_EVENT_RECV_SOFTAP_MAX_CONN_NUM:
        break;
	case BLUFI_EVENT_RECV_SOFTAP_AUTH_MODE:
        break;
	case BLUFI_EVENT_RECV_SOFTAP_CHANNEL:
        break;
    case BLUFI_EVENT_GET_WIFI_LIST:
        break;
    case BLUFI_EVENT_RECV_CUSTOM_DATA:
        MODLOG_DFLT_INFO("Recv Custom Data %" PRIu32 "\n", param->custom_data.data_len);
        MODLOG_DFLT_LOG_BUFFER_HEX("Custom Data", param->custom_data.data, param->custom_data.data_len);
        break;
	case BLUFI_EVENT_RECV_USERNAME:
        /* Not handle currently */
        break;
	case BLUFI_EVENT_RECV_CA_CERT:
        /* Not handle currently */
        break;
	case BLUFI_EVENT_RECV_CLIENT_CERT:
        /* Not handle currently */
        break;
	case BLUFI_EVENT_RECV_SERVER_CERT:
        /* Not handle currently */
        break;
	case BLUFI_EVENT_RECV_CLIENT_PRIV_KEY:
        /* Not handle currently */
        break;;
	case BLUFI_EVENT_RECV_SERVER_PRIV_KEY:
        /* Not handle currently */
        break;
    default:
        break;
    }

}

static blufi_callbacks_t blufi_callbacks = {
    .event_cb = blufi_event_callback,
};

static int blufi_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    blufi_cb_param_t param;
    int rc;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
            MODLOG_DFLT(INFO, "connection %s; status=%d\n",
                        event->connect.status == 0 ? "established" : "failed",
                        event->connect.status);
            if (event->connect.status == 0) {
                blufi_env.is_connected = true;
                blufi_env.recv_seq = blufi_env.send_seq = 0;

                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);
                memcpy(param.connect.remote_bda, desc.peer_id_addr.val, BLUFI_BD_ADDR_LEN);

                param.connect.conn_id = event->connect.conn_handle;
                /* save connection handle */
                blufi_env.conn_id = event->connect.conn_handle;

                blufi_event_callback(BLUFI_EVENT_BLE_CONNECT, &param);
        }
        if (event->connect.status != 0) {
            /* Connection failed; resume advertising. */
            blufi_adv_start();
        }
        return 0;
    case BLE_GAP_EVENT_DISCONNECT:
        MODLOG_DFLT(INFO, "disconnect; reason=%d\n", event->disconnect.reason);
        memcpy(blufi_env.remote_bda, event->disconnect.conn.peer_id_addr.val, BLUFI_BD_ADDR_LEN);

        blufi_env.is_connected = false;
        blufi_env.recv_seq = blufi_env.send_seq = 0;
        blufi_env.sec_mode = 0x0;
        blufi_env.offset = 0;

        if (blufi_env.aggr_buf != NULL) {
            free(blufi_env.aggr_buf);
            blufi_env.aggr_buf = NULL;
        }

        memcpy(param.disconnect.remote_bda, event->disconnect.conn.peer_id_addr.val, BLUFI_BD_ADDR_LEN);
        blufi_event_callback(BLUFI_EVENT_BLE_DISCONNECT, &param);

        return 0;
    case BLE_GAP_EVENT_CONN_UPDATE:
        /* The central has updated the connection parameters. */
        MODLOG_DFLT(INFO, "connection updated; status=%d\n", event->conn_update.status);
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        MODLOG_DFLT(INFO, "advertise complete; reason=%d\n", event->adv_complete.reason);
        blufi_adv_start();
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        MODLOG_DFLT(INFO, "subscribe event; conn_handle=%d attr_handle=%d "
                    "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
                    event->subscribe.conn_handle,
                    event->subscribe.attr_handle,
                    event->subscribe.reason,
                    event->subscribe.prev_notify,
                    event->subscribe.cur_notify,
                    event->subscribe.prev_indicate,
                    event->subscribe.cur_indicate);
        return 0;

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
        blufi_env.frag_size = (event->mtu.value < BLUFI_MAX_DATA_LEN ? event->mtu.value : BLUFI_MAX_DATA_LEN) - BLUFI_MTU_RESERVED_SIZE;
        return 0;

    }
    return 0;
}

static void blufi_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void blufi_on_sync(void)
{
    blufi_profile_init();
}

/*
 * main
 *
 * The main task for the project. This function initializes the packages,
 * then starts serving events from default event queue.
 *
 * @return int NOTE: this function should never return!
 */
extern void ble_store_config_init(void);
extern int nimble_ble_enable(void);
extern struct ble_npl_eventq *nimble_port_get_dflt_eventq(void);
int blufi(int argc, char **argv)
{
    int rc __attribute__((unused));
    static int init_flag = 0;

    if (init_flag)
        return 0;
    init_flag = 1;
    blufi_register_callbacks(&blufi_callbacks);

    rt_work_init(&set_mode_work, blufi_wlan_set_mode, &wlan_cfg_info);
    rt_work_init(&wlan_connect_work, blufi_wlan_connect, &wlan_cfg_info);
    rt_work_init(&wlan_disconnect_work, blufi_wlan_disconnect, &wlan_cfg_info);

    /* Initialize the NimBLE host configuration */
    ble_hs_cfg.reset_cb = blufi_on_reset;
    ble_hs_cfg.sync_cb = blufi_on_sync;
    ble_hs_cfg.gatts_register_cb = blufi_gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_hs_cfg.sm_io_cap = 4;
#ifdef CONFIG_EXAMPLE_BONDING
    ble_hs_cfg.sm_bonding = 1;
#endif
#ifdef CONFIG_EXAMPLE_MITM
    ble_hs_cfg.sm_mitm = 1;
#endif
#ifdef CONFIG_EXAMPLE_USE_SC
    ble_hs_cfg.sm_sc = 1;
#else
    ble_hs_cfg.sm_sc = 0;
#ifdef CONFIG_EXAMPLE_BONDING
    ble_hs_cfg.sm_our_key_dist = 1;
    ble_hs_cfg.sm_their_key_dist = 1;
#endif
#endif

    rc = blufi_gatt_svr_init();
    assert(rc == 0);

    /* Set the default device name */
    rc = ble_svc_gap_device_name_set(BLUFI_DEVICE_NAME);
    assert(rc == 0);

    /* XXX Need to have template for store */
    ble_store_config_init();

    /* startup bluetooth host stack*/
    ble_hs_thread_startup();

    return 0;
}
MSH_CMD_EXPORT_ALIAS(blufi, blufi, "bluetoooth config wifi sample");
