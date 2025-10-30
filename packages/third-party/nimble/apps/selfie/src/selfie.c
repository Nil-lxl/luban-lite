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

#include "nimble/ble.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "selfie.h"

static uint8_t selfie_addr_type;
static bool ble_is_connected = false;
static bool notify_state = false;

static int selfie_gap_event(struct ble_gap_event *event, void *arg);

void selfie_adv_start(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    uint8_t addr_val[6] = {0};
    const char *name;
    int32_t adv_duration_ms = 180000; /* maximum possible duration for hid device(180s) */
    int rc;

    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &selfie_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(INFO, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Printing ADDR */
    rc = ble_hs_id_copy_addr(selfie_addr_type, addr_val, NULL);

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

    fields.appearance = SELFIE_APPEARANCE_GENERIC;
    fields.appearance_is_present = 1;

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
        BLE_UUID16_INIT(SELFIE_SERVICE_UUID)
    };
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    /* Initialize the security configuration */
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_our_key_dist = 0;
    ble_hs_cfg.sm_their_key_dist = 0;
    //ble_hs_cfg.sm_sc = 1;
    //ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ID | BLE_SM_PAIR_KEY_DIST_ENC;
    //ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ID | BLE_SM_PAIR_KEY_DIST_ENC;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising. */
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    /* Recommended interval 30ms to 150ms */
    adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(30);
    adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(150);
    rc = ble_gap_adv_start(selfie_addr_type, NULL, adv_duration_ms,
                           &adv_params, selfie_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d", rc);
        return;
    }
}

void selfie_adv_stop(void)
{
    ble_gap_adv_stop();
}

/* This functions simulates heart beat and notifies it to the client */
void selfie_shutter_event(struct ble_npl_event *ev)
{
    uint8_t key[2];
    int rc __attribute__((unused));

    if (!notify_state) {
        MODLOG_DFLT(INFO, "notify is not enabled.\n");
        return;
    }

    key[0] = 0xe9;
    key[1] = 0x00;
    rc = selfie_gatt_send_report(conn_handle, key, sizeof(key));
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "ble gatts notify custom failed. rc=%d\n", rc);

        assert(rc == 0);
    }

    aicos_mdelay(200);

    key[0] = 0x00;
    key[1] = 0x00;
    rc = selfie_gatt_send_report(conn_handle, key, sizeof(key));
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "ble gatts notify custom failed. rc=%d\n", rc);

        assert(rc == 0);
    }
}

static int selfie_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
        MODLOG_DFLT(INFO, "connection %s; status=%d\n",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);
        if (event->connect.status != 0) {
            /* Connection failed; resume advertising. */
            selfie_adv_start();
            conn_handle = BLE_HS_CONN_HANDLE_NONE;
            return 0;
        }

        ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        MODLOG_DFLT(INFO, "conn: itvl:%d us, latency:%d, to:%d ms\n", 
                    desc.conn_itvl * 1250, desc.conn_latency,
                    desc.supervision_timeout*10);

        conn_handle = event->connect.conn_handle;
        ble_gattc_exchange_mtu(event->connect.conn_handle, NULL, NULL);

        if (event->connect.status != 0) {
            /* Connection failed; resume advertising. */
            selfie_adv_start();
            conn_handle = 0;
        } else {
            conn_handle = event->connect.conn_handle;
        }
        return 0;
    case BLE_GAP_EVENT_DISCONNECT:
        MODLOG_DFLT(INFO, "disconnect; reason=%d\n", event->disconnect.reason);
        /* reset conn_handle */
        conn_handle = BLE_HS_CONN_HANDLE_NONE;
        /* Connection terminated; resume advertising */
        selfie_adv_start();
        return 0;
    case BLE_GAP_EVENT_CONN_UPDATE:
        /* The central has updated the connection parameters. */
        MODLOG_DFLT(INFO, "connection updated; status=%d\n", event->conn_update.status);
        ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        MODLOG_DFLT(INFO, "conn upd cmpl: conn_handle:%d, itvl:%d us, latency:%d, to:%d ms\n", 
                desc.conn_handle, desc.conn_itvl * 1250,
                desc.conn_latency, desc.supervision_timeout*10);
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        MODLOG_DFLT(INFO, "advertise complete; reason=%d\n", event->adv_complete.reason);
        selfie_adv_start();
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
        notify_state = event->subscribe.cur_notify;
        return 0;

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
        return 0;
    case BLE_GAP_EVENT_ENC_CHANGE:
        /* Encryption has been enabled or disabled for this connection. */
        MODLOG_DFLT(INFO, "encryption change event; status=%d\n", event->enc_change.status);
        rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        assert(rc == 0);
        return 0;
    case BLE_GAP_EVENT_NOTIFY_TX:
        MODLOG_DFLT(INFO, "notify_tx event; conn_handle=%d attr_handle=%d "
                    "status=%d is_indication=%d\n",
                    event->notify_tx.conn_handle,
                    event->notify_tx.attr_handle,
                    event->notify_tx.status,
                    event->notify_tx.indication);
        return 0;
    case BLE_GAP_EVENT_REPEAT_PAIRING:
        MODLOG_DFLT(INFO, "REPEAT_PAIRING_EVENT started\n");
        /* Delete the old bond. */
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    case BLE_GAP_EVENT_PAIRING_COMPLETE:
        MODLOG_DFLT(INFO, "PAIRING_COMPLETE_EVENT started\n");
        //if (event->pairing_complete.status == 0) {
        //    struct ble_store_value_sec *sec = &event->pairing_complete.sec;
        //}
        //rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        //assert(rc == 0);
        //ble_store_util_delete_peer(&desc.peer_id_addr);
        //return BLE_GAP_REPEAT_PAIRING_RETRY;
        return 0;
    case BLE_GAP_EVENT_PASSKEY_ACTION:
        MODLOG_DFLT(INFO, "PASSKEY_ACTION_EVENT started\n");
        struct ble_sm_io pkey = {0};
        int key = 0;

        if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
            pkey.action = event->passkey.params.action;
            /* This is the passkey to be entered on peer */
            pkey.passkey = 123456;
            MODLOG_DFLT(INFO, "Enter passkey %d on the peer side\n", pkey.passkey);
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            MODLOG_DFLT(INFO, "ble_sm_inject_io result: %d\n", rc);
        } else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
            MODLOG_DFLT(INFO, "Accepting passkey..");
            pkey.action = event->passkey.params.action;
            pkey.numcmp_accept = key;
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            MODLOG_DFLT(INFO, "ble_sm_inject_io result: %d\n", rc);
        } else if (event->passkey.params.action == BLE_SM_IOACT_OOB) {
            static uint8_t tem_oob[16] = {0};
            pkey.action = event->passkey.params.action;
            for (int i = 0; i < 16; i++) {
                pkey.oob[i] = tem_oob[i];
            }
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            MODLOG_DFLT(INFO, "ble_sm_inject_io result: %d\n", rc);
        } else if (event->passkey.params.action == BLE_SM_IOACT_INPUT) {
            MODLOG_DFLT(INFO, "Input not supported passing -> 123456\n");
            MODLOG_DFLT(INFO, "numcmp:%d\n", event->passkey.params.numcmp);
            pkey.action = event->passkey.params.action;
            //pkey.passkey = 123456;
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            MODLOG_DFLT(INFO, "ble_sm_inject_io result: %d\n", rc);
        }
        return 0;
    }
    return 0;
}

static void selfie_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void selfie_on_sync(void)
{
    extern struct ble_npl_event ble_hs_ev_shutter;
    ble_npl_event_init(&ble_hs_ev_shutter, selfie_shutter_event, NULL);
    selfie_adv_start();
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
int selfie(int argc, char **argv)
{
    int rc __attribute__((unused));
    static int init_flag = 0;

    if (init_flag)
        return 0;
    init_flag = 1;

    /* Initialize the NimBLE host configuration */
    ble_hs_cfg.reset_cb = selfie_on_reset;
    ble_hs_cfg.sync_cb = selfie_on_sync;
    ble_hs_cfg.gatts_register_cb = selfie_gatt_svr_register_cb;
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

    rc = selfie_gatt_svr_init();
    assert(rc == 0);

    /* Set the default device name */
    rc = ble_svc_gap_device_name_set(SELFIE_DEVICE_NAME);
    assert(rc == 0);

    /* XXX Need to have template for store */
    ble_store_config_init();

    /* startup bluetooth host stack*/
    ble_hs_thread_startup();

    return 0;
}
MSH_CMD_EXPORT_ALIAS(selfie, selfie, "bluetoooth selfie sample");
