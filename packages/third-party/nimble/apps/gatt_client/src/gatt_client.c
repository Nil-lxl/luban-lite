/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * GATT Client Demo
 *
 * This demo demonstrates GATT client functionality:
 * - Scan for BLE devices
 * - Connect to a target device
 * - Discover services, characteristics, and descriptors
 * - Read/write characteristics
 * - Read/write descriptors (including CCCD)
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <rtthread.h>

#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/util/util.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "gatt_client.h"

extern struct ble_npl_eventq *nimble_port_get_dflt_eventq(void);

static int gatt_client_gap_event(struct ble_gap_event *event, void *arg);

static uint8_t gatt_client_addr_type;
static struct peer *g_current_peer;
static uint16_t g_conn_handle;

static struct ble_npl_callout g_test_timer;
static int g_test_step;

static char g_target_addr_str[18];
static int g_auto_connect;

static void gatt_client_scan(void);
static void gatt_client_connect(const ble_addr_t *addr);

static int
on_chr_read(uint16_t conn_handle,
            const struct ble_gatt_error *error,
            struct ble_gatt_attr *attr,
            void *arg)
{
    MODLOG_DFLT(INFO, "Characteristic read complete; status=%d conn_handle=%d\n",
                error->status, conn_handle);

    if (error->status == 0) {
        MODLOG_DFLT(INFO, "  attr_handle=%d value=", attr->handle);
        print_mbuf(attr->om);
        MODLOG_DFLT(INFO, "\n");
    }

    return 0;
}

static int
on_chr_write(uint16_t conn_handle,
             const struct ble_gatt_error *error,
             struct ble_gatt_attr *attr,
             void *arg)
{
    MODLOG_DFLT(INFO, "Characteristic write complete; status=%d conn_handle=%d attr_handle=%d\n",
                error->status, conn_handle, attr->handle);

    return 0;
}

static int
on_dsc_read(uint16_t conn_handle,
            const struct ble_gatt_error *error,
            struct ble_gatt_attr *attr,
            void *arg)
{
    MODLOG_DFLT(INFO, "Descriptor read complete; status=%d conn_handle=%d\n",
                error->status, conn_handle);

    if (error->status == 0) {
        MODLOG_DFLT(INFO, "  attr_handle=%d value=", attr->handle);
        print_mbuf(attr->om);
        MODLOG_DFLT(INFO, "\n");
    }

    return 0;
}

static int
on_dsc_write(uint16_t conn_handle,
             const struct ble_gatt_error *error,
             struct ble_gatt_attr *attr,
             void *arg)
{
    MODLOG_DFLT(INFO, "Descriptor write complete; status=%d conn_handle=%d attr_handle=%d\n",
                error->status, conn_handle, attr->handle);

    return 0;
}

static int
on_subscribe(uint16_t conn_handle,
             const struct ble_gatt_error *error,
             struct ble_gatt_attr *attr,
             void *arg)
{
    MODLOG_DFLT(INFO, "Subscribe complete; status=%d conn_handle=%d attr_handle=%d\n",
                error->status, conn_handle, attr->handle);

    return 0;
}

static void
gatt_client_test_read_chr(const struct peer *peer)
{
    const struct peer_chr *chr;
    int rc;

    MODLOG_DFLT(INFO, "\n=== Test: Read Characteristic ===\n");

    if (SLIST_EMPTY(&peer->svcs)) {
        MODLOG_DFLT(ERROR, "No services discovered\n");
        return;
    }

    struct peer_svc *svc = SLIST_FIRST(&peer->svcs);
    if (SLIST_EMPTY(&svc->chrs)) {
        MODLOG_DFLT(ERROR, "No characteristics in first service\n");
        return;
    }

    chr = SLIST_FIRST(&svc->chrs);

    MODLOG_DFLT(INFO, "Reading characteristic with handle=%d\n", chr->chr.val_handle);

    rc = ble_gattc_read(peer->conn_handle, chr->chr.val_handle, on_chr_read, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Failed to read characteristic; rc=%d\n", rc);
    }
}

static void
gatt_client_test_write_chr(const struct peer *peer)
{
    const struct peer_chr *chr;
    uint8_t value[4] = {0x01, 0x02, 0x03, 0x04};
    int rc;

    MODLOG_DFLT(INFO, "\n=== Test: Write Characteristic ===\n");

    if (SLIST_EMPTY(&peer->svcs)) {
        MODLOG_DFLT(ERROR, "No services discovered\n");
        return;
    }

    struct peer_svc *svc = SLIST_FIRST(&peer->svcs);
    if (SLIST_EMPTY(&svc->chrs)) {
        MODLOG_DFLT(ERROR, "No characteristics in first service\n");
        return;
    }

    chr = SLIST_FIRST(&svc->chrs);

    if (!(chr->chr.properties & BLE_GATT_CHR_PROP_WRITE) &&
        !(chr->chr.properties & BLE_GATT_CHR_PROP_WRITE_NO_RSP)) {
        MODLOG_DFLT(INFO, "Characteristic does not support write, skipping\n");
        return;
    }

    MODLOG_DFLT(INFO, "Writing to characteristic with handle=%d\n", chr->chr.val_handle);

    rc = ble_gattc_write_flat(peer->conn_handle, chr->chr.val_handle,
                              value, sizeof(value), on_chr_write, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Failed to write characteristic; rc=%d\n", rc);
    }
}

static void
gatt_client_test_read_dsc(const struct peer *peer)
{
    const struct peer_dsc *dsc;
    int rc;

    MODLOG_DFLT(INFO, "\n=== Test: Read Descriptor ===\n");

    if (SLIST_EMPTY(&peer->svcs)) {
        MODLOG_DFLT(ERROR, "No services discovered\n");
        return;
    }

    struct peer_svc *svc = SLIST_FIRST(&peer->svcs);
    if (SLIST_EMPTY(&svc->chrs)) {
        MODLOG_DFLT(ERROR, "No characteristics in first service\n");
        return;
    }

    struct peer_chr *chr = SLIST_FIRST(&svc->chrs);
    if (SLIST_EMPTY(&chr->dscs)) {
        MODLOG_DFLT(INFO, "No descriptors for this characteristic\n");
        return;
    }

    dsc = SLIST_FIRST(&chr->dscs);

    MODLOG_DFLT(INFO, "Reading descriptor with handle=%d\n", dsc->dsc.handle);

    rc = ble_gattc_read(peer->conn_handle, dsc->dsc.handle, on_dsc_read, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Failed to read descriptor; rc=%d\n", rc);
    }
}

static void
gatt_client_test_write_cccd(const struct peer *peer)
{
    const struct peer_dsc *dsc;
    uint8_t value[2] = {0x01, 0x00};
    int rc;

    MODLOG_DFLT(INFO, "\n=== Test: Write CCCD (Enable Notification) ===\n");

    dsc = peer_dsc_find_uuid(peer, BLE_UUID16_DECLARE(0x180D),
                             BLE_UUID16_DECLARE(0x2A37),
                             BLE_UUID16_DECLARE(BLE_GATT_DSC_CLT_CFG_UUID16));

    if (dsc == NULL) {
        MODLOG_DFLT(INFO, "No CCCD found for Heart Rate Measurement, trying generic search...\n");

        struct peer_svc *svc;
        SLIST_FOREACH(svc, &peer->svcs, next) {
            struct peer_chr *chr;
            SLIST_FOREACH(chr, &svc->chrs, next) {
                if (chr->chr.properties & BLE_GATT_CHR_PROP_NOTIFY) {
                    struct peer_dsc *cd;
                    SLIST_FOREACH(cd, &chr->dscs, next) {
                        if (ble_uuid_u16(&cd->dsc.uuid.u) == BLE_GATT_DSC_CLT_CFG_UUID16) {
                            MODLOG_DFLT(INFO, "Found CCCD at handle=%d\n", cd->dsc.handle);

                            rc = ble_gattc_write_flat(peer->conn_handle, cd->dsc.handle,
                                                      value, sizeof(value), on_subscribe, NULL);
                            if (rc != 0) {
                                MODLOG_DFLT(ERROR, "Failed to write CCCD; rc=%d\n", rc);
                            }
                            return;
                        }
                    }
                }
            }
        }

        MODLOG_DFLT(INFO, "No CCCD found for any notifiable characteristic\n");
        return;
    }

    MODLOG_DFLT(INFO, "Writing CCCD at handle=%d to enable notifications\n", dsc->dsc.handle);

    rc = ble_gattc_write_flat(peer->conn_handle, dsc->dsc.handle,
                              value, sizeof(value), on_subscribe, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Failed to write CCCD; rc=%d\n", rc);
    }
}

static void
gatt_client_test_next_step(struct ble_npl_event *ev)
{
    if (g_current_peer == NULL) {
        MODLOG_DFLT(ERROR, "No peer connected\n");
        return;
    }

    switch (g_test_step) {
    case 0:
        gatt_client_test_read_chr(g_current_peer);
        break;
    case 1:
        gatt_client_test_write_chr(g_current_peer);
        break;
    case 2:
        gatt_client_test_read_dsc(g_current_peer);
        break;
    case 3:
        gatt_client_test_write_cccd(g_current_peer);
        break;
    default:
        MODLOG_DFLT(INFO, "\n=== All GATT Client Tests Completed ===\n");
        return;
    }

    g_test_step++;
    ble_npl_callout_reset(&g_test_timer, RT_TICK_PER_SECOND * 2);
}

static void
gatt_client_on_disc_complete(const struct peer *peer, int status, void *arg)
{
    if (status != 0) {
        MODLOG_DFLT(ERROR, "Service discovery failed; status=%d conn_handle=%d\n",
                    status, peer->conn_handle);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }

    MODLOG_DFLT(INFO, "\n=== Service Discovery Complete ===\n");
    MODLOG_DFLT(INFO, "conn_handle=%d\n", peer->conn_handle);

    g_current_peer = (struct peer *)peer;

    peer_print_all(peer);

    MODLOG_DFLT(INFO, "\n=== Starting GATT Client Tests ===\n");
    g_test_step = 0;
    ble_npl_callout_reset(&g_test_timer, RT_TICK_PER_SECOND);
}

static void
gatt_client_scan(void)
{
    struct ble_gap_disc_params disc_params;
    int rc;

    MODLOG_DFLT(INFO, "=== Starting BLE Scan ===\n");

    /* Check if BLE host is enabled */
    if (!ble_hs_is_enabled()) {
        MODLOG_DFLT(ERROR, "BLE host not enabled. Please run 'gatt_client' first to initialize.\n");
        return;
    }

    rc = ble_hs_id_infer_auto(0, &gatt_client_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error determining address type; rc=%d\n", rc);
        return;
    }

    disc_params.filter_duplicates = 1;
    disc_params.passive = 0;
    disc_params.itvl = 0;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    rc = ble_gap_disc(gatt_client_addr_type, BLE_HS_FOREVER, &disc_params,
                      gatt_client_gap_event, NULL);
    if (rc == BLE_HS_EALREADY) {
        /* Scan already in progress, cancel it first */
        MODLOG_DFLT(INFO, "Scan already in progress, restarting...\n");
        rc = ble_gap_disc_cancel();
        if (rc == 0) {
            rc = ble_gap_disc(gatt_client_addr_type, BLE_HS_FOREVER, &disc_params,
                              gatt_client_gap_event, NULL);
        }
    }
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error initiating GAP discovery; rc=%d\n", rc);
    }
}

static void
gatt_client_connect(const ble_addr_t *addr)
{
    int rc;

    MODLOG_DFLT(INFO, "=== Connecting to device ===\n");
    MODLOG_DFLT(INFO, "Address: %s\n", addr_str(addr->val));

    rc = ble_gap_connect(gatt_client_addr_type, addr, 30000, NULL,
                         gatt_client_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error: Failed to connect to device; rc=%d\n", rc);
        return;
    }
}

static int
gatt_client_should_connect(const struct ble_gap_disc_desc *disc)
{
    if (g_auto_connect && strlen(g_target_addr_str) > 0) {
        char disc_addr_str[18];
        snprintf(disc_addr_str, sizeof(disc_addr_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                 disc->addr.val[5], disc->addr.val[4], disc->addr.val[3],
                 disc->addr.val[2], disc->addr.val[1], disc->addr.val[0]);

        if (strcmp(disc_addr_str, g_target_addr_str) == 0) {
            return 1;
        }
        return 0;
    }

    if (disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND &&
        disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
        return 0;
    }

    return 0;
}

static void
gatt_client_connect_if_interesting(const struct ble_gap_disc_desc *disc)
{
    int rc;

    if (!gatt_client_should_connect(disc)) {
        return;
    }

    rc = ble_gap_disc_cancel();
    if (rc != 0) {
        MODLOG_DFLT(DEBUG, "Failed to cancel scan; rc=%d\n", rc);
        return;
    }

    gatt_client_connect(&disc->addr);
}

static int
gatt_client_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    struct ble_hs_adv_fields fields;
    int rc;

    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        rc = ble_hs_adv_parse_fields(&fields, event->disc.data,
                                     event->disc.length_data);
        if (rc != 0) {
            return 0;
        }

        MODLOG_DFLT(INFO, "Device found: %s, RSSI=%d\n",
                    addr_str(event->disc.addr.val), event->disc.rssi);

        if (fields.name != NULL && fields.name_len > 0) {
            char name[32];
            int len = fields.name_len < sizeof(name) - 1 ? fields.name_len : sizeof(name) - 1;
            memcpy(name, fields.name, len);
            name[len] = '\0';
            MODLOG_DFLT(INFO, "  Name: %s\n", name);
        }

        gatt_client_connect_if_interesting(&event->disc);
        return 0;

    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            MODLOG_DFLT(INFO, "=== Connection established ===\n");

            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            print_conn_desc(&desc);
            MODLOG_DFLT(INFO, "\n");

            g_conn_handle = event->connect.conn_handle;

            rc = peer_add(event->connect.conn_handle);
            if (rc != 0) {
                MODLOG_DFLT(ERROR, "Failed to add peer; rc=%d\n", rc);
                return 0;
            }

            MODLOG_DFLT(INFO, "=== Starting Service Discovery ===\n");
            rc = peer_disc_all(event->connect.conn_handle,
                               gatt_client_on_disc_complete, NULL);
            if (rc != 0) {
                MODLOG_DFLT(ERROR, "Failed to discover services; rc=%d\n", rc);
                return 0;
            }
        } else {
            MODLOG_DFLT(ERROR, "Connection failed; status=%d\n",
                        event->connect.status);
            gatt_client_scan();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        MODLOG_DFLT(INFO, "=== Disconnected ===\n");
        MODLOG_DFLT(INFO, "reason=%d ", event->disconnect.reason);
        print_conn_desc(&event->disconnect.conn);
        MODLOG_DFLT(INFO, "\n");

        g_current_peer = NULL;
        g_conn_handle = 0;
        g_test_step = 0;
        ble_npl_callout_stop(&g_test_timer);

        peer_delete(event->disconnect.conn.conn_handle);

        gatt_client_scan();
        return 0;

    case BLE_GAP_EVENT_DISC_COMPLETE:
        MODLOG_DFLT(INFO, "Discovery complete; reason=%d\n",
                    event->disc_complete.reason);
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        MODLOG_DFLT(INFO, "Encryption change event; status=%d\n",
                    event->enc_change.status);
        rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        assert(rc == 0);
        print_conn_desc(&desc);
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX:
        MODLOG_DFLT(INFO, "=== Received %s ===\n",
                    event->notify_rx.indication ? "indication" : "notification");
        MODLOG_DFLT(INFO, "conn_handle=%d attr_handle=%d attr_len=%d\n",
                    event->notify_rx.conn_handle,
                    event->notify_rx.attr_handle,
                    OS_MBUF_PKTLEN(event->notify_rx.om));
        MODLOG_DFLT(INFO, "Data: ");
        print_mbuf(event->notify_rx.om);
        MODLOG_DFLT(INFO, "\n");
        return 0;

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "MTU update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);
        return BLE_GAP_REPEAT_PAIRING_RETRY;

    default:
        return 0;
    }
}

static void
gatt_client_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void
gatt_client_on_sync(void)
{
    int rc;

    MODLOG_DFLT(INFO, "=== BLE Host Sync ===\n");

    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    gatt_client_scan();
}

static int
gatt_client_entry(void)
{
    int rc;

    MODLOG_DFLT(INFO, "=== GATT Client Demo Starting ===\n");

    ble_hs_cfg.reset_cb = gatt_client_on_reset;
    ble_hs_cfg.sync_cb = gatt_client_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    rc = peer_init(4, 32, 64, 64);
    assert(rc == 0);

    rc = ble_svc_gap_device_name_set("nimble-gatt-client");
    assert(rc == 0);

    ble_npl_callout_init(&g_test_timer, nimble_port_get_dflt_eventq(),
                         gatt_client_test_next_step, NULL);

    ble_hs_thread_startup();

    return 0;
}

static void
gatt_client_scan_cmd(int argc, char **argv)
{
    g_auto_connect = 0;
    g_target_addr_str[0] = '\0';
    gatt_client_scan();
}

static void
gatt_client_connect_cmd(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: gattc_connect <addr>\n");
        rt_kprintf("  addr: XX:XX:XX:XX:XX:XX format\n");
        return;
    }

    strncpy(g_target_addr_str, argv[1], sizeof(g_target_addr_str) - 1);
    g_auto_connect = 1;

    rt_kprintf("Will connect to device: %s\n", g_target_addr_str);
    gatt_client_scan();
}

static void
gatt_client_disconnect_cmd(int argc, char **argv)
{
    int rc;

    if (g_conn_handle == 0) {
        rt_kprintf("Not connected\n");
        return;
    }

    rc = ble_gap_terminate(g_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    if (rc != 0) {
        rt_kprintf("Failed to disconnect; rc=%d\n", rc);
    }
}

static void
gatt_client_read_chr_cmd(int argc, char **argv)
{
    uint16_t handle;
    int rc;

    if (g_current_peer == NULL) {
        rt_kprintf("Not connected\n");
        return;
    }

    if (argc < 2) {
        rt_kprintf("Usage: gattc_read_chr <handle>\n");
        return;
    }

    handle = strtoul(argv[1], NULL, 0);

    rt_kprintf("Reading characteristic handle=%d\n", handle);
    rc = ble_gattc_read(g_conn_handle, handle, on_chr_read, NULL);
    if (rc != 0) {
        rt_kprintf("Failed to read; rc=%d\n", rc);
    }
}

static void
gatt_client_write_chr_cmd(int argc, char **argv)
{
    uint16_t handle;
    uint8_t value[32];
    int value_len;
    int rc;
    int i;

    if (g_current_peer == NULL) {
        rt_kprintf("Not connected\n");
        return;
    }

    if (argc < 3) {
        rt_kprintf("Usage: gattc_write_chr <handle> <hex_data>\n");
        rt_kprintf("  Example: gattc_write_chr 0x0012 01020304\n");
        return;
    }

    handle = strtoul(argv[1], NULL, 0);
    value_len = strlen(argv[2]) / 2;
    if (value_len > sizeof(value)) {
        value_len = sizeof(value);
    }

    for (i = 0; i < value_len; i++) {
        char hex[3] = {argv[2][i*2], argv[2][i*2+1], 0};
        value[i] = strtoul(hex, NULL, 16);
    }

    rt_kprintf("Writing to handle=%d, len=%d\n", handle, value_len);
    rc = ble_gattc_write_flat(g_conn_handle, handle, value, value_len, on_chr_write, NULL);
    if (rc != 0) {
        rt_kprintf("Failed to write; rc=%d\n", rc);
    }
}

static void
gatt_client_read_dsc_cmd(int argc, char **argv)
{
    uint16_t handle;
    int rc;

    if (g_current_peer == NULL) {
        rt_kprintf("Not connected\n");
        return;
    }

    if (argc < 2) {
        rt_kprintf("Usage: gattc_read_dsc <handle>\n");
        return;
    }

    handle = strtoul(argv[1], NULL, 0);

    rt_kprintf("Reading descriptor handle=%d\n", handle);
    rc = ble_gattc_read(g_conn_handle, handle, on_dsc_read, NULL);
    if (rc != 0) {
        rt_kprintf("Failed to read; rc=%d\n", rc);
    }
}

static void
gatt_client_write_dsc_cmd(int argc, char **argv)
{
    uint16_t handle;
    uint8_t value[32];
    int value_len;
    int rc;
    int i;

    if (g_current_peer == NULL) {
        rt_kprintf("Not connected\n");
        return;
    }

    if (argc < 3) {
        rt_kprintf("Usage: gattc_write_dsc <handle> <hex_data>\n");
        rt_kprintf("  Example: gattc_write_dsc 0x0013 0100\n");
        return;
    }

    handle = strtoul(argv[1], NULL, 0);
    value_len = strlen(argv[2]) / 2;
    if (value_len > sizeof(value)) {
        value_len = sizeof(value);
    }

    for (i = 0; i < value_len; i++) {
        char hex[3] = {argv[2][i*2], argv[2][i*2+1], 0};
        value[i] = strtoul(hex, NULL, 16);
    }

    rt_kprintf("Writing to descriptor handle=%d, len=%d\n", handle, value_len);
    rc = ble_gattc_write_flat(g_conn_handle, handle, value, value_len, on_dsc_write, NULL);
    if (rc != 0) {
        rt_kprintf("Failed to write; rc=%d\n", rc);
    }
}

static void
gatt_client_enable_notify_cmd(int argc, char **argv)
{
    uint16_t handle;
    uint8_t value[2] = {0x01, 0x00};
    int rc;

    if (g_current_peer == NULL) {
        rt_kprintf("Not connected\n");
        return;
    }

    if (argc < 2) {
        rt_kprintf("Usage: gattc_enable_notify <cccd_handle>\n");
        return;
    }

    handle = strtoul(argv[1], NULL, 0);

    rt_kprintf("Enabling notifications on CCCD handle=%d\n", handle);
    rc = ble_gattc_write_flat(g_conn_handle, handle, value, sizeof(value), on_subscribe, NULL);
    if (rc != 0) {
        rt_kprintf("Failed to enable notifications; rc=%d\n", rc);
    }
}

static void
gatt_client_enable_indicate_cmd(int argc, char **argv)
{
    uint16_t handle;
    uint8_t value[2] = {0x02, 0x00};
    int rc;

    if (g_current_peer == NULL) {
        rt_kprintf("Not connected\n");
        return;
    }

    if (argc < 2) {
        rt_kprintf("Usage: gattc_enable_indicate <cccd_handle>\n");
        return;
    }

    handle = strtoul(argv[1], NULL, 0);

    rt_kprintf("Enabling indications on CCCD handle=%d\n", handle);
    rc = ble_gattc_write_flat(g_conn_handle, handle, value, sizeof(value), on_subscribe, NULL);
    if (rc != 0) {
        rt_kprintf("Failed to enable indications; rc=%d\n", rc);
    }
}

static void
gatt_client_discover_cmd(int argc, char **argv)
{
    int rc;

    if (g_conn_handle == 0) {
        rt_kprintf("Not connected\n");
        return;
    }

    rt_kprintf("Starting service discovery...\n");
    rc = peer_disc_all(g_conn_handle, gatt_client_on_disc_complete, NULL);
    if (rc != 0) {
        rt_kprintf("Failed to discover services; rc=%d\n", rc);
    }
}

static void
gatt_client_show_peer_cmd(int argc, char **argv)
{
    if (g_current_peer == NULL) {
        rt_kprintf("No peer connected\n");
        return;
    }

    peer_print_all(g_current_peer);
}

MSH_CMD_EXPORT_ALIAS(gatt_client_entry, gatt_client, "GATT client demo - auto scan and test");
MSH_CMD_EXPORT_ALIAS(gatt_client_scan_cmd, gattc_scan, "Start BLE scan");
MSH_CMD_EXPORT_ALIAS(gatt_client_connect_cmd, gattc_connect, "Connect to a BLE device by address");
MSH_CMD_EXPORT_ALIAS(gatt_client_disconnect_cmd, gattc_disconnect, "Disconnect from current device");
MSH_CMD_EXPORT_ALIAS(gatt_client_discover_cmd, gattc_discover, "Discover all services/characteristics/descriptors");
MSH_CMD_EXPORT_ALIAS(gatt_client_show_peer_cmd, gattc_show, "Show discovered services and characteristics");
MSH_CMD_EXPORT_ALIAS(gatt_client_read_chr_cmd, gattc_read_chr, "Read a characteristic by handle");
MSH_CMD_EXPORT_ALIAS(gatt_client_write_chr_cmd, gattc_write_chr, "Write to a characteristic by handle");
MSH_CMD_EXPORT_ALIAS(gatt_client_read_dsc_cmd, gattc_read_dsc, "Read a descriptor by handle");
MSH_CMD_EXPORT_ALIAS(gatt_client_write_dsc_cmd, gattc_write_dsc, "Write to a descriptor by handle");
MSH_CMD_EXPORT_ALIAS(gatt_client_enable_notify_cmd, gattc_enable_notify, "Enable notifications on a CCCD handle");
MSH_CMD_EXPORT_ALIAS(gatt_client_enable_indicate_cmd, gattc_enable_indicate, "Enable indications on a CCCD handle");
