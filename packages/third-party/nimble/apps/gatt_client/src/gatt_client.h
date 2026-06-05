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

#ifndef H_GATT_CLIENT_
#define H_GATT_CLIENT_

#include "nimble/ble.h"
#include "modlog/modlog.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ble_hs_adv_fields;
struct ble_gap_conn_desc;
struct ble_hs_cfg;
union ble_store_value;
union ble_store_key;

void print_bytes(const uint8_t *bytes, int len);
void print_mbuf(const struct os_mbuf *om);
char *addr_str(const void *addr);
void print_uuid(const ble_uuid_t *uuid);
void print_conn_desc(const struct ble_gap_conn_desc *desc);
void print_adv_fields(const struct ble_hs_adv_fields *fields);

struct peer_dsc {
    SLIST_ENTRY(peer_dsc) next;
    struct ble_gatt_dsc dsc;
};
SLIST_HEAD(peer_dsc_list, peer_dsc);

struct peer_chr {
    SLIST_ENTRY(peer_chr) next;
    struct ble_gatt_chr chr;
    struct peer_dsc_list dscs;
};
SLIST_HEAD(peer_chr_list, peer_chr);

struct peer_svc {
    SLIST_ENTRY(peer_svc) next;
    struct ble_gatt_svc svc;
    struct peer_chr_list chrs;
};
SLIST_HEAD(peer_svc_list, peer_svc);

struct peer;
typedef void peer_disc_fn(const struct peer *peer, int status, void *arg);

struct peer {
    SLIST_ENTRY(peer) next;
    uint16_t conn_handle;
    struct peer_svc_list svcs;
    uint16_t disc_prev_chr_val;
    struct peer_svc *cur_svc;
    peer_disc_fn *disc_cb;
    void *disc_cb_arg;
};

int peer_disc_all(uint16_t conn_handle, peer_disc_fn *disc_cb, void *disc_cb_arg);
const struct peer_dsc *peer_dsc_find_uuid(const struct peer *peer, const ble_uuid_t *svc_uuid,
                                          const ble_uuid_t *chr_uuid, const ble_uuid_t *dsc_uuid);
const struct peer_chr *peer_chr_find_uuid(const struct peer *peer, const ble_uuid_t *svc_uuid,
                                          const ble_uuid_t *chr_uuid);
const struct peer_svc *peer_svc_find_uuid(const struct peer *peer, const ble_uuid_t *uuid);
int peer_delete(uint16_t conn_handle);
int peer_add(uint16_t conn_handle);
int peer_init(int max_peers, int max_svcs, int max_chrs, int max_dscs);

void peer_print_all(const struct peer *peer);

#ifdef __cplusplus
}
#endif

#endif
