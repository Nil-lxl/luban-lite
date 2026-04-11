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

#ifndef _HCI_H5_H_
#define _HCI_H5_H_

#include <stdint.h>
#include <ipc/workqueue.h>

#define HCI_H5_NONE 0x00
#define HCI_H5_ACK  0x00
#define HCI_H5_CMD  0x01
#define HCI_H5_ACL  0x02
#define HCI_H5_EVT  0x04
#define HCI_H5_ISO  0x05
#define HCI_H5_LINK 0x0f

typedef void *(hci_h5_alloc_cmd)(void);
typedef void *(hci_h5_alloc_evt)(int);
typedef struct os_mbuf *(hci_h5_alloc_acl)(void);
typedef struct os_mbuf *(hci_h5_alloc_iso)(void);

struct hci_h5_allocators {
    hci_h5_alloc_cmd *cmd;
    hci_h5_alloc_acl *acl;
    hci_h5_alloc_evt *evt;
    hci_h5_alloc_iso *iso;
};

extern const struct hci_h5_allocators hci_h5_allocs_from_ll;
extern const struct hci_h5_allocators hci_h5_allocs_from_hs;

typedef int (hci_h5_frame_cb)(uint8_t pkt_type, void *data);

struct hci_h5_sm {
    uint8_t state;
    uint8_t pkt_type;
    uint8_t min_len;

    struct rt_work ack_work;
    struct rt_work retx_work;

    uint8_t tx_win;
    uint8_t tx_ack;
    uint8_t tx_seq;
    uint8_t rx_ack;
    enum {
        UNINIT,
        INIT,
        ACTIVE,
    } link_state;
    uint16_t len;
    uint16_t exp_len;
    uint8_t hdr[4];
    uint8_t idx;
    union {
        uint8_t *buf;
        struct os_mbuf *om;
    };

    const struct hci_h5_allocators *allocs;
    hci_h5_frame_cb *frame_cb;
};

void hci_h5_sm_init(struct hci_h5_sm *h5sm,
                    const struct hci_h5_allocators *allocs,
                    hci_h5_frame_cb *frame_cb);

int hci_h5_sm_rx(struct hci_h5_sm *h5sm, const uint8_t *buf, uint16_t len);
int hci_h5_sm_send(struct hci_h5_sm *h5sm, uint8_t pkt_type, const uint8_t *buf, uint16_t len);

#endif /* _HCI_H5_H_ */
