/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * GATT Client Demo - Peer Management
 *
 * Manages discovered services, characteristics, and descriptors
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "host/ble_hs.h"
#include "gatt_client.h"

static void *peer_svc_mem;
static struct os_mempool peer_svc_pool;

static void *peer_chr_mem;
static struct os_mempool peer_chr_pool;

static void *peer_dsc_mem;
static struct os_mempool peer_dsc_pool;

static void *peer_mem;
static struct os_mempool peer_pool;
static SLIST_HEAD(, peer) peers;

static struct peer_svc *peer_svc_find_range(struct peer *peer, uint16_t attr_handle);
static struct peer_svc *peer_svc_find(struct peer *peer, uint16_t svc_start_handle,
                                      struct peer_svc **out_prev);
int peer_svc_is_empty(const struct peer_svc *svc);

uint16_t chr_end_handle(const struct peer_svc *svc, const struct peer_chr *chr);
int chr_is_empty(const struct peer_svc *svc, const struct peer_chr *chr);
static struct peer_chr *peer_chr_find(const struct peer_svc *svc, uint16_t chr_def_handle,
                                      struct peer_chr **out_prev);
static void peer_disc_chrs(struct peer *peer);

static int peer_dsc_disced(uint16_t conn_handle, const struct ble_gatt_error *error,
                           uint16_t chr_val_handle, const struct ble_gatt_dsc *dsc,
                           void *arg);

static struct peer *
peer_find(uint16_t conn_handle)
{
    struct peer *peer;

    SLIST_FOREACH(peer, &peers, next) {
        if (peer->conn_handle == conn_handle) {
            return peer;
        }
    }

    return NULL;
}

static void
peer_disc_complete(struct peer *peer, int rc)
{
    peer->disc_prev_chr_val = 0;

    if (peer->disc_cb != NULL) {
        peer->disc_cb(peer, rc, peer->disc_cb_arg);
    }
}

static struct peer_dsc *
peer_dsc_find_prev(const struct peer_chr *chr, uint16_t dsc_handle)
{
    struct peer_dsc *prev;
    struct peer_dsc *dsc;

    prev = NULL;
    SLIST_FOREACH(dsc, &chr->dscs, next) {
        if (dsc->dsc.handle >= dsc_handle) {
            break;
        }
        prev = dsc;
    }

    return prev;
}

static struct peer_dsc *
peer_dsc_find(const struct peer_chr *chr, uint16_t dsc_handle,
              struct peer_dsc **out_prev)
{
    struct peer_dsc *prev;
    struct peer_dsc *dsc;

    prev = peer_dsc_find_prev(chr, dsc_handle);
    if (prev == NULL) {
        dsc = SLIST_FIRST(&chr->dscs);
    } else {
        dsc = SLIST_NEXT(prev, next);
    }

    if (dsc != NULL && dsc->dsc.handle != dsc_handle) {
        dsc = NULL;
    }

    if (out_prev != NULL) {
        *out_prev = prev;
    }
    return dsc;
}

static int
peer_dsc_add(struct peer *peer, uint16_t chr_val_handle,
             const struct ble_gatt_dsc *gatt_dsc)
{
    struct peer_dsc *prev;
    struct peer_dsc *dsc;
    struct peer_svc *svc;
    struct peer_chr *chr;

    svc = peer_svc_find_range(peer, chr_val_handle);
    if (svc == NULL) {
        assert(0);
        return BLE_HS_EUNKNOWN;
    }

    chr = peer_chr_find(svc, chr_val_handle, NULL);
    if (chr == NULL) {
        assert(0);
        return BLE_HS_EUNKNOWN;
    }

    dsc = peer_dsc_find(chr, gatt_dsc->handle, &prev);
    if (dsc != NULL) {
        return 0;
    }

    dsc = os_memblock_get(&peer_dsc_pool);
    if (dsc == NULL) {
        return BLE_HS_ENOMEM;
    }
    memset(dsc, 0, sizeof *dsc);

    dsc->dsc = *gatt_dsc;

    if (prev == NULL) {
        SLIST_INSERT_HEAD(&chr->dscs, dsc, next);
    } else {
        SLIST_NEXT(prev, next) = dsc;
    }

    return 0;
}

static void
peer_disc_dscs(struct peer *peer)
{
    struct peer_chr *chr;
    struct peer_svc *svc;
    int rc;

    SLIST_FOREACH(svc, &peer->svcs, next) {
        SLIST_FOREACH(chr, &svc->chrs, next) {
            if (!chr_is_empty(svc, chr) &&
                SLIST_EMPTY(&chr->dscs) &&
                peer->disc_prev_chr_val <= chr->chr.def_handle) {

                rc = ble_gattc_disc_all_dscs(peer->conn_handle,
                                             chr->chr.val_handle,
                                             chr_end_handle(svc, chr),
                                             peer_dsc_disced, peer);
                if (rc != 0) {
                    peer_disc_complete(peer, rc);
                }

                peer->disc_prev_chr_val = chr->chr.val_handle;
                return;
            }
        }
    }

    peer_disc_complete(peer, 0);
}

static int
peer_dsc_disced(uint16_t conn_handle, const struct ble_gatt_error *error,
                uint16_t chr_val_handle, const struct ble_gatt_dsc *dsc,
                void *arg)
{
    struct peer *peer;
    int rc;

    peer = arg;
    assert(peer->conn_handle == conn_handle);

    switch (error->status) {
    case 0:
        rc = peer_dsc_add(peer, chr_val_handle, dsc);
        break;

    case BLE_HS_EDONE:
        if (peer->disc_prev_chr_val > 0) {
            peer_disc_dscs(peer);
        }
        rc = 0;
        break;

    default:
        rc = error->status;
        break;
    }

    if (rc != 0) {
        peer_disc_complete(peer, rc);
    }

    return rc;
}

uint16_t
chr_end_handle(const struct peer_svc *svc, const struct peer_chr *chr)
{
    const struct peer_chr *next_chr;

    next_chr = SLIST_NEXT(chr, next);
    if (next_chr != NULL) {
        return next_chr->chr.def_handle - 1;
    } else {
        return svc->svc.end_handle;
    }
}

int
chr_is_empty(const struct peer_svc *svc, const struct peer_chr *chr)
{
    return chr_end_handle(svc, chr) <= chr->chr.val_handle;
}

static struct peer_chr *
peer_chr_find_prev(const struct peer_svc *svc, uint16_t chr_val_handle)
{
    struct peer_chr *prev;
    struct peer_chr *chr;

    prev = NULL;
    SLIST_FOREACH(chr, &svc->chrs, next) {
        if (chr->chr.val_handle >= chr_val_handle) {
            break;
        }
        prev = chr;
    }

    return prev;
}

static struct peer_chr *
peer_chr_find(const struct peer_svc *svc, uint16_t chr_val_handle,
              struct peer_chr **out_prev)
{
    struct peer_chr *prev;
    struct peer_chr *chr;

    prev = peer_chr_find_prev(svc, chr_val_handle);
    if (prev == NULL) {
        chr = SLIST_FIRST(&svc->chrs);
    } else {
        chr = SLIST_NEXT(prev, next);
    }

    if (chr != NULL && chr->chr.val_handle != chr_val_handle) {
        chr = NULL;
    }

    if (out_prev != NULL) {
        *out_prev = prev;
    }
    return chr;
}

static void
peer_chr_delete(struct peer_chr *chr)
{
    struct peer_dsc *dsc;

    while ((dsc = SLIST_FIRST(&chr->dscs)) != NULL) {
        SLIST_REMOVE_HEAD(&chr->dscs, next);
        os_memblock_put(&peer_dsc_pool, dsc);
    }

    os_memblock_put(&peer_chr_pool, chr);
}

static int
peer_chr_add(struct peer *peer, uint16_t svc_start_handle,
             const struct ble_gatt_chr *gatt_chr)
{
    struct peer_chr *prev;
    struct peer_chr *chr;
    struct peer_svc *svc;

    svc = peer_svc_find(peer, svc_start_handle, NULL);
    if (svc == NULL) {
        assert(0);
        return BLE_HS_EUNKNOWN;
    }

    chr = peer_chr_find(svc, gatt_chr->def_handle, &prev);
    if (chr != NULL) {
        return 0;
    }

    chr = os_memblock_get(&peer_chr_pool);
    if (chr == NULL) {
        return BLE_HS_ENOMEM;
    }
    memset(chr, 0, sizeof *chr);

    chr->chr = *gatt_chr;

    if (prev == NULL) {
        SLIST_INSERT_HEAD(&svc->chrs, chr, next);
    } else {
        SLIST_NEXT(prev, next) = chr;
    }

    return 0;
}

static int
peer_chr_disced(uint16_t conn_handle, const struct ble_gatt_error *error,
                const struct ble_gatt_chr *chr, void *arg)
{
    struct peer *peer;
    int rc;

    peer = arg;
    assert(peer->conn_handle == conn_handle);

    switch (error->status) {
    case 0:
        rc = peer_chr_add(peer, peer->cur_svc->svc.start_handle, chr);
        break;

    case BLE_HS_EDONE:
        if (peer->disc_prev_chr_val > 0) {
            peer_disc_chrs(peer);
        }
        rc = 0;
        break;

    default:
        rc = error->status;
        break;
    }

    if (rc != 0) {
        peer_disc_complete(peer, rc);
    }

    return rc;
}

static void
peer_disc_chrs(struct peer *peer)
{
    struct peer_svc *svc;
    int rc;

    SLIST_FOREACH(svc, &peer->svcs, next) {
        if (!peer_svc_is_empty(svc) && SLIST_EMPTY(&svc->chrs)) {
            peer->cur_svc = svc;
            rc = ble_gattc_disc_all_chrs(peer->conn_handle,
                                         svc->svc.start_handle,
                                         svc->svc.end_handle,
                                         peer_chr_disced, peer);
            if (rc != 0) {
                peer_disc_complete(peer, rc);
            }
            return;
        }
    }

    peer_disc_dscs(peer);
}

int
peer_svc_is_empty(const struct peer_svc *svc)
{
    return svc->svc.end_handle <= svc->svc.start_handle;
}

static struct peer_svc *
peer_svc_find_prev(struct peer *peer, uint16_t svc_start_handle)
{
    struct peer_svc *prev;
    struct peer_svc *svc;

    prev = NULL;
    SLIST_FOREACH(svc, &peer->svcs, next) {
        if (svc->svc.start_handle >= svc_start_handle) {
            break;
        }
        prev = svc;
    }

    return prev;
}

static struct peer_svc *
peer_svc_find(struct peer *peer, uint16_t svc_start_handle,
              struct peer_svc **out_prev)
{
    struct peer_svc *prev;
    struct peer_svc *svc;

    prev = peer_svc_find_prev(peer, svc_start_handle);
    if (prev == NULL) {
        svc = SLIST_FIRST(&peer->svcs);
    } else {
        svc = SLIST_NEXT(prev, next);
    }

    if (svc != NULL && svc->svc.start_handle != svc_start_handle) {
        svc = NULL;
    }

    if (out_prev != NULL) {
        *out_prev = prev;
    }
    return svc;
}

static struct peer_svc *
peer_svc_find_range(struct peer *peer, uint16_t attr_handle)
{
    struct peer_svc *svc;

    SLIST_FOREACH(svc, &peer->svcs, next) {
        if (svc->svc.start_handle <= attr_handle &&
            svc->svc.end_handle >= attr_handle) {
            return svc;
        }
    }

    return NULL;
}

const struct peer_svc *
peer_svc_find_uuid(const struct peer *peer, const ble_uuid_t *uuid)
{
    const struct peer_svc *svc;

    SLIST_FOREACH(svc, &peer->svcs, next) {
        if (ble_uuid_cmp(&svc->svc.uuid.u, uuid) == 0) {
            return svc;
        }
    }

    return NULL;
}

const struct peer_chr *
peer_chr_find_uuid(const struct peer *peer, const ble_uuid_t *svc_uuid,
                   const ble_uuid_t *chr_uuid)
{
    const struct peer_svc *svc;
    const struct peer_chr *chr;

    svc = peer_svc_find_uuid(peer, svc_uuid);
    if (svc == NULL) {
        return NULL;
    }

    SLIST_FOREACH(chr, &svc->chrs, next) {
        if (ble_uuid_cmp(&chr->chr.uuid.u, chr_uuid) == 0) {
            return chr;
        }
    }

    return NULL;
}

const struct peer_dsc *
peer_dsc_find_uuid(const struct peer *peer, const ble_uuid_t *svc_uuid,
                   const ble_uuid_t *chr_uuid, const ble_uuid_t *dsc_uuid)
{
    const struct peer_chr *chr;
    const struct peer_dsc *dsc;

    chr = peer_chr_find_uuid(peer, svc_uuid, chr_uuid);
    if (chr == NULL) {
        return NULL;
    }

    SLIST_FOREACH(dsc, &chr->dscs, next) {
        if (ble_uuid_cmp(&dsc->dsc.uuid.u, dsc_uuid) == 0) {
            return dsc;
        }
    }

    return NULL;
}

static int
peer_svc_add(struct peer *peer, const struct ble_gatt_svc *gatt_svc)
{
    struct peer_svc *prev;
    struct peer_svc *svc;

    svc = peer_svc_find(peer, gatt_svc->start_handle, &prev);
    if (svc != NULL) {
        return 0;
    }

    svc = os_memblock_get(&peer_svc_pool);
    if (svc == NULL) {
        return BLE_HS_ENOMEM;
    }
    memset(svc, 0, sizeof *svc);

    svc->svc = *gatt_svc;
    SLIST_INIT(&svc->chrs);

    if (prev == NULL) {
        SLIST_INSERT_HEAD(&peer->svcs, svc, next);
    } else {
        SLIST_INSERT_AFTER(prev, svc, next);
    }

    return 0;
}

static void
peer_svc_delete(struct peer_svc *svc)
{
    struct peer_chr *chr;

    while ((chr = SLIST_FIRST(&svc->chrs)) != NULL) {
        SLIST_REMOVE_HEAD(&svc->chrs, next);
        peer_chr_delete(chr);
    }

    os_memblock_put(&peer_svc_pool, svc);
}

static int
peer_svc_disced(uint16_t conn_handle, const struct ble_gatt_error *error,
                const struct ble_gatt_svc *service, void *arg)
{
    struct peer *peer;
    int rc;

    peer = arg;
    assert(peer->conn_handle == conn_handle);

    switch (error->status) {
    case 0:
        rc = peer_svc_add(peer, service);
        break;

    case BLE_HS_EDONE:
        if (peer->disc_prev_chr_val > 0) {
            peer_disc_chrs(peer);
        }
        rc = 0;
        break;

    default:
        rc = error->status;
        break;
    }

    if (rc != 0) {
        peer_disc_complete(peer, rc);
    }

    return rc;
}

int
peer_disc_all(uint16_t conn_handle, peer_disc_fn *disc_cb, void *disc_cb_arg)
{
    struct peer_svc *svc;
    struct peer *peer;
    int rc;

    peer = peer_find(conn_handle);
    if (peer == NULL) {
        return BLE_HS_ENOTCONN;
    }

    while ((svc = SLIST_FIRST(&peer->svcs)) != NULL) {
        SLIST_REMOVE_HEAD(&peer->svcs, next);
        peer_svc_delete(svc);
    }

    peer->disc_prev_chr_val = 1;
    peer->disc_cb = disc_cb;
    peer->disc_cb_arg = disc_cb_arg;

    rc = ble_gattc_disc_all_svcs(conn_handle, peer_svc_disced, peer);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int
peer_delete(uint16_t conn_handle)
{
    struct peer_svc *svc;
    struct peer *peer;
    int rc;

    peer = peer_find(conn_handle);
    if (peer == NULL) {
        return BLE_HS_ENOTCONN;
    }

    SLIST_REMOVE(&peers, peer, peer, next);

    while ((svc = SLIST_FIRST(&peer->svcs)) != NULL) {
        SLIST_REMOVE_HEAD(&peer->svcs, next);
        peer_svc_delete(svc);
    }

    rc = os_memblock_put(&peer_pool, peer);
    if (rc != 0) {
        return BLE_HS_EOS;
    }

    return 0;
}

int
peer_add(uint16_t conn_handle)
{
    struct peer *peer;

    peer = peer_find(conn_handle);
    if (peer != NULL) {
        return BLE_HS_EALREADY;
    }

    peer = os_memblock_get(&peer_pool);
    if (peer == NULL) {
        return BLE_HS_ENOMEM;
    }

    memset(peer, 0, sizeof *peer);
    peer->conn_handle = conn_handle;

    SLIST_INSERT_HEAD(&peers, peer, next);

    return 0;
}

static void
peer_free_mem(void)
{
    free(peer_mem);
    peer_mem = NULL;

    free(peer_svc_mem);
    peer_svc_mem = NULL;

    free(peer_chr_mem);
    peer_chr_mem = NULL;

    free(peer_dsc_mem);
    peer_dsc_mem = NULL;
}

int
peer_init(int max_peers, int max_svcs, int max_chrs, int max_dscs)
{
    int rc;

    peer_free_mem();

    peer_mem = malloc(OS_MEMPOOL_BYTES(max_peers, sizeof(struct peer)));
    if (peer_mem == NULL) {
        rc = BLE_HS_ENOMEM;
        goto err;
    }

    rc = os_mempool_init(&peer_pool, max_peers, sizeof(struct peer), peer_mem, "peer_pool");
    if (rc != 0) {
        rc = BLE_HS_EOS;
        goto err;
    }

    peer_svc_mem = malloc(OS_MEMPOOL_BYTES(max_svcs, sizeof(struct peer_svc)));
    if (peer_svc_mem == NULL) {
        rc = BLE_HS_ENOMEM;
        goto err;
    }

    rc = os_mempool_init(&peer_svc_pool, max_svcs, sizeof(struct peer_svc), peer_svc_mem, "peer_svc_pool");
    if (rc != 0) {
        rc = BLE_HS_EOS;
        goto err;
    }

    peer_chr_mem = malloc(OS_MEMPOOL_BYTES(max_chrs, sizeof(struct peer_chr)));
    if (peer_chr_mem == NULL) {
        rc = BLE_HS_ENOMEM;
        goto err;
    }

    rc = os_mempool_init(&peer_chr_pool, max_chrs, sizeof(struct peer_chr), peer_chr_mem, "peer_chr_pool");
    if (rc != 0) {
        rc = BLE_HS_EOS;
        goto err;
    }

    peer_dsc_mem = malloc(OS_MEMPOOL_BYTES(max_dscs, sizeof(struct peer_dsc)));
    if (peer_dsc_mem == NULL) {
        rc = BLE_HS_ENOMEM;
        goto err;
    }

    rc = os_mempool_init(&peer_dsc_pool, max_dscs, sizeof(struct peer_dsc), peer_dsc_mem, "peer_dsc_pool");
    if (rc != 0) {
        rc = BLE_HS_EOS;
        goto err;
    }

    return 0;

err:
    peer_free_mem();
    return rc;
}

void
peer_print_all(const struct peer *peer)
{
    const struct peer_svc *svc;
    const struct peer_chr *chr;
    const struct peer_dsc *dsc;
    char uuid_buf[BLE_UUID_STR_LEN];
    int svc_count = 0, chr_count = 0, dsc_count = 0;

    MODLOG_DFLT(INFO, "\n========================================\n");
    MODLOG_DFLT(INFO, "Discovered Services and Characteristics\n");
    MODLOG_DFLT(INFO, "========================================\n");

    SLIST_FOREACH(svc, &peer->svcs, next) {
        svc_count++;
        MODLOG_DFLT(INFO, "\n[Service %d] UUID: %s\n", svc_count,
                    ble_uuid_to_str(&svc->svc.uuid.u, uuid_buf));
        MODLOG_DFLT(INFO, "  Handle range: 0x%04x - 0x%04x\n",
                    svc->svc.start_handle, svc->svc.end_handle);

        SLIST_FOREACH(chr, &svc->chrs, next) {
            chr_count++;
            MODLOG_DFLT(INFO, "  [Characteristic %d] UUID: %s\n", chr_count,
                        ble_uuid_to_str(&chr->chr.uuid.u, uuid_buf));
            MODLOG_DFLT(INFO, "    Def handle: 0x%04x, Val handle: 0x%04x\n",
                        chr->chr.def_handle, chr->chr.val_handle);

            MODLOG_DFLT(INFO, "    Properties: ");
            if (chr->chr.properties & BLE_GATT_CHR_PROP_BROADCAST)
                MODLOG_DFLT(INFO, "BROADCAST ");
            if (chr->chr.properties & BLE_GATT_CHR_PROP_READ)
                MODLOG_DFLT(INFO, "READ ");
            if (chr->chr.properties & BLE_GATT_CHR_PROP_WRITE_NO_RSP)
                MODLOG_DFLT(INFO, "WRITE_NO_RSP ");
            if (chr->chr.properties & BLE_GATT_CHR_PROP_WRITE)
                MODLOG_DFLT(INFO, "WRITE ");
            if (chr->chr.properties & BLE_GATT_CHR_PROP_NOTIFY)
                MODLOG_DFLT(INFO, "NOTIFY ");
            if (chr->chr.properties & BLE_GATT_CHR_PROP_INDICATE)
                MODLOG_DFLT(INFO, "INDICATE ");
            if (chr->chr.properties & BLE_GATT_CHR_PROP_AUTH_SIGN_WRITE)
                MODLOG_DFLT(INFO, "AUTH_SIGN_WRITE ");
            if (chr->chr.properties & BLE_GATT_CHR_PROP_EXTENDED)
                MODLOG_DFLT(INFO, "EXTENDED ");
            MODLOG_DFLT(INFO, "\n");

            SLIST_FOREACH(dsc, &chr->dscs, next) {
                dsc_count++;
                MODLOG_DFLT(INFO, "    [Descriptor %d] UUID: %s, Handle: 0x%04x\n",
                            dsc_count, ble_uuid_to_str(&dsc->dsc.uuid.u, uuid_buf),
                            dsc->dsc.handle);
            }
            dsc_count = 0;
        }
        chr_count = 0;
    }

    MODLOG_DFLT(INFO, "\n========================================\n");
}
