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
#include <stdint.h>
#include <string.h>
#include <syscfg/syscfg.h>
#include <os/os.h>
#include <os/os_mbuf.h>
#include <nimble/hci_common.h>
#include <nimble/transport.h>
#include <nimble/transport/hci_h5.h>
#include <aic_core.h>
#include <aic_utils.h>

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#define HCI_H5_SM_W4_PKT_DELIMITER 0
#define HCI_H5_SM_W4_PKT_TYPE      1
#define HCI_H5_SM_W4_HEADER        2
#define HCI_H5_SM_W4_PAYLOAD       3
#define HCI_H5_SM_COMPLETED        4

#define SLIP_DELIMITER  0xc0
#define SLIP_ESC        0xdb
#define SLIP_ESC_DELIM  0xdc
#define SLIP_ESC_ESC    0xdd

#define H5_HDR_SEQ(hdr)      ((hdr)[0] & 0x07)
#define H5_HDR_ACK(hdr)      (((hdr)[0] >> 3) & 0x07)
#define H5_HDR_CRC(hdr)      (((hdr)[0] >> 6) & 0x01)
#define H5_HDR_RELIABLE(hdr) (((hdr)[0] >> 7) & 0x01)
#define H5_HDR_PKT_TYPE(hdr) ((hdr)[1] & 0x0f)
#define H5_HDR_LEN(hdr)      ((((hdr)[1] >> 4) & 0x0f) + ((hdr)[2] << 4))

#define H5_SET_SEQ(hdr, seq)   ((hdr)[0] |= (seq))
#define H5_SET_ACK(hdr, ack)   ((hdr)[0] |= (ack) << 3)
#define H5_SET_RELIABLE(hdr)   ((hdr)[0] |= 1 << 7)
#define H5_SET_TYPE(hdr, type) ((hdr)[1] |= type)
#define H5_SET_LEN(hdr, len)   (((hdr)[1] |= ((len)&0x0f) << 4), ((hdr)[2] |= (len) >> 4))

#define H5_RX_ACK_TIMEOUT (250)
#define H5_TX_ACK_TIMEOUT (250)

const struct hci_h5_allocators hci_h5_allocs_from_ll = {
    .acl = ble_transport_alloc_acl_from_ll,
    .evt = ble_transport_alloc_evt,
};
const struct hci_h5_allocators hci_h5_allocs_from_hs = {
    .cmd = ble_transport_alloc_cmd,
    .acl = ble_transport_alloc_acl_from_hs,
    .iso = ble_transport_alloc_iso_from_hs,
};

struct hci_h5_input_buffer {
    const uint8_t *buf;
    uint16_t len;
};

struct sk_buff {
    /* These two members must be first. */
    struct sk_buff *next;
    struct sk_buff *prev;
    uint8_t *head;
    uint8_t *data;
    uint8_t *tail;
    uint8_t *end;

    uint8_t pkt_type;
#define H5SK_BUFF_OUTSIDE (1 << 0)
#define H5SK_CRC_CAL      (1 << 1)
    uint16_t flags;
    uint16_t crc;
    uint32_t buff_sz;
    uint32_t len;
};

static struct sk_buff *skb_alloc(unsigned int len, int reserved)
{
    struct sk_buff *skb = NULL;
    uint8_t *data = NULL;

    skb = malloc(sizeof(struct sk_buff) + len + reserved);
    if (skb) {
        memset(skb, 0, sizeof(struct sk_buff));

        data = (uint8_t *)skb + sizeof(struct sk_buff) + reserved;
        skb->head = data;
        skb->data = data;
        skb->tail = data;
        skb->end = data + len;
        skb->len = 0;
    } else {
        pr_err("Failed to alloc sk buff.\n");
        return NULL;
    }

    return skb;
}

static void skb_free(struct sk_buff *skb)
{
    if (skb) {
        free(skb);
    }
}

static uint8_t *skb_put(struct sk_buff *skb, uint32_t len)
{
    uint8_t *tmp = NULL;

    tmp = skb->tail;
    skb->tail += len;
    skb->len += len;

    return tmp;
}

static void h5_slip_delim(struct sk_buff *skb)
{
    const char delim = SLIP_DELIMITER;

    memcpy(skb_put(skb, 1), &delim, 1);
}

static void hci_h5_slip_one_byte(struct sk_buff *skb, uint8_t c)
{
    const char esc_delim[2] = { SLIP_ESC, SLIP_ESC_DELIM };
    const char esc_esc[2] = { SLIP_ESC, SLIP_ESC_ESC };

    switch (c) {
    case SLIP_DELIMITER:
        memcpy(skb_put(skb, 2), &esc_delim, 2);
        break;
    case SLIP_ESC:
        memcpy(skb_put(skb, 2), &esc_esc, 2);
        break;
    default:
        memcpy(skb_put(skb, 1), &c, 1);
    }
}

static void hci_h5_reset_rx(struct hci_h5_sm *h5sm)
{
    switch (h5sm->pkt_type) {
    case HCI_H5_LINK:
        if (h5sm->buf) {
            free(h5sm->buf);
            h5sm->buf = NULL;
        }
        break;
    case HCI_H5_CMD:
    case HCI_H5_EVT:
        if (h5sm->buf) {
            ble_transport_free(h5sm->buf);
            h5sm->buf = NULL;
        }
        break;
    case HCI_H5_ACL:
    case HCI_H5_ISO:
        if (h5sm->om) {
            os_mbuf_free_chain(h5sm->om);
            h5sm->om = NULL;
        }
        break;
    default:
        break;
    }
    h5sm->state = HCI_H5_SM_W4_PKT_DELIMITER;
}

static int hci_h5_unslip_one_byte(unsigned char *byte, unsigned char c)
{
    const uint8_t delim = SLIP_DELIMITER, esc = SLIP_ESC;
    static unsigned char last_byte = 0;

    if ((c != SLIP_ESC) && (last_byte != SLIP_ESC)) {
        *byte = c;
        return 0;
    } else if (c == SLIP_ESC) {
        last_byte = SLIP_ESC;
        return 1;
    }

    if (last_byte == SLIP_ESC) {
        last_byte = 0;
        switch (c) {
        case SLIP_ESC_DELIM:
            *byte = delim;
            break;
        case SLIP_ESC_ESC:
            *byte = esc;
            break;
        default:
            pr_err("Invalid esc byte 0x%02hhx", c);
            return -EIO;
        }
    }

    return 0;
}

static bool hci_h5_reliable_packet(uint8_t type)
{
	switch (type) {
	case HCI_H5_ACL:
	case HCI_H5_CMD:
	case HCI_H5_EVT:
	//case HCI_H5_SCO:
	case HCI_H5_ISO:
		return true;
	default:
		return false;
	}
}

static void hci_h5_print_header(const uint8_t *hdr, const char *str)
{
    if (H5_HDR_RELIABLE(hdr)) {
        pr_debug("%s REL: seq %u ack %u crc %u type %u len %u\n", str, H5_HDR_SEQ(hdr),
             H5_HDR_ACK(hdr), H5_HDR_CRC(hdr), H5_HDR_PKT_TYPE(hdr), H5_HDR_LEN(hdr));
    } else {
        pr_debug("%s UNREL: ack %u crc %u type %u len %u\n", str, H5_HDR_ACK(hdr),
            H5_HDR_CRC(hdr), H5_HDR_PKT_TYPE(hdr), H5_HDR_LEN(hdr));
    }
}

extern void rtthread_uart_tx(const uint8_t *buf, size_t len);
int hci_h5_sm_send(struct hci_h5_sm *h5sm, uint8_t type, const uint8_t *data, uint16_t len)
{
	struct sk_buff *nskb;
	uint8_t hdr[4] = { 0 };
	int i;

    /* Set ACK for outgoing packet and stop delayed work */
    H5_SET_ACK(hdr, h5sm->tx_ack);
    /* If cancel fails we may ack the same seq number twice, this is OK. */
    rt_work_cancel(&h5sm->ack_work);

	if (hci_h5_reliable_packet(type)) {
		H5_SET_RELIABLE(hdr);
        H5_SET_SEQ(hdr, h5sm->tx_seq);
        h5sm->tx_seq = (h5sm->tx_seq + 1) % 8;
	}
    H5_SET_TYPE(hdr, type);
    H5_SET_LEN(hdr, len);

    /* Calculate CRC */
	hdr[3] = ~((hdr[0] + hdr[1] + hdr[2]) & 0xff);

    hci_h5_print_header(hdr, "TX: >");
	/*
	 * Max len of packet: (original len + 4 (H5 hdr) + 2 (crc)) * 2
	 * (because bytes 0xc0 and 0xdb are escaped, worst case is when
	 * the packet is all made of 0xc0 and 0xdb) + 2 (0xc0
	 * delimiters at start and end).
	 */
	nskb = skb_alloc((len + 6) * 2 + 2, 0);
	if (!nskb) {
		pr_err("skb alloc failed.\n");
		return -1;
    }

	h5_slip_delim(nskb);

	pr_debug("tx: seq %u ack %u crc %u rel %u type %u len %u",
	       H5_HDR_SEQ(hdr), H5_HDR_ACK(hdr),
	       H5_HDR_CRC(hdr), H5_HDR_RELIABLE(hdr), H5_HDR_PKT_TYPE(hdr),
	       H5_HDR_LEN(hdr));

	for (i = 0; i < 4; i++)
		hci_h5_slip_one_byte(nskb, hdr[i]);

	for (i = 0; i < len; i++)
		hci_h5_slip_one_byte(nskb, data[i]);

	h5_slip_delim(nskb);

    rtthread_uart_tx(nskb->data, nskb->len);

    skb_free(nskb);

	return 0;
}

static void hci_h5_frame_start(struct hci_h5_sm *rxs, uint8_t pkt_type)
{
    rxs->pkt_type = pkt_type;
    rxs->len = 0;
    rxs->exp_len = 0;

    switch (rxs->pkt_type) {
    case HCI_H5_ACK:
    case HCI_H5_LINK:
        rxs->min_len = 0;
        break;
    case HCI_H5_CMD:
        rxs->min_len = 0;
        break;
    case HCI_H5_ACL:
    case HCI_H5_ISO:
        rxs->min_len = 0;
        break;
    case HCI_H5_EVT:
        rxs->min_len = 0;
        break;
    default:
        /* XXX sync loss */
        assert(0);
        break;
    }
}

static int hci_h5_ib_consume(struct hci_h5_input_buffer *ib, uint16_t len)
{
    assert(ib->len >= len);

    ib->buf += len;
    ib->len -= len;

    return len;
}

static int hci_h5_ib_pull_min_len(struct hci_h5_sm *rxs, struct hci_h5_input_buffer *ib)
{
    uint16_t len;

    len = min(ib->len, rxs->min_len - rxs->len);
    memcpy(&rxs->hdr[rxs->len], ib->buf, len);

    rxs->len += len;
    hci_h5_ib_consume(ib, len);

    return rxs->len != rxs->min_len;
}

static int hci_h5_sm_w4_header(struct hci_h5_sm *h5sm, struct hci_h5_input_buffer *ib)
{
    int rc;

    rc = hci_h5_ib_pull_min_len(h5sm, ib);
    if (rc) {
        /* need more data */
        return 1;
    }

    switch (h5sm->pkt_type) {
    case HCI_H5_ACK:
        break;
    case HCI_H5_LINK:
        h5sm->buf = malloc(4);
        if (!h5sm->buf) {
            return -1;
        }

        h5sm->exp_len = H5_HDR_LEN(h5sm->hdr);
        break;
    case HCI_H5_CMD:
        assert(h5sm->allocs && h5sm->allocs->cmd);
        h5sm->buf = h5sm->allocs->cmd();
        if (!h5sm->buf) {
            return -1;
        }

        memcpy(h5sm->buf, h5sm->hdr, h5sm->len);
        h5sm->exp_len = h5sm->hdr[2] + 3;
        break;
    case HCI_H5_ACL:
        assert(h5sm->allocs && h5sm->allocs->acl);
        h5sm->om = h5sm->allocs->acl();
        if (!h5sm->om) {
            return -1;
        }

        os_mbuf_append(h5sm->om, h5sm->hdr, h5sm->len);
        h5sm->exp_len = H5_HDR_LEN(h5sm->hdr);
        break;
    case HCI_H5_EVT:
        if (h5sm->hdr[0] == BLE_HCI_EVCODE_LE_META) {
            /* For LE Meta event we need 3 bytes to parse header */
            h5sm->min_len = 3;
            rc = hci_h5_ib_pull_min_len(h5sm, ib);
            if (rc) {
                /* need more data */
                return 1;
            }
        }

        assert(h5sm->allocs && h5sm->allocs->evt);

        /* We can drop legacy advertising events if there's no free buffer in
         * discardable pool.
         */
        if (h5sm->hdr[2] == BLE_HCI_LE_SUBEV_ADV_RPT) {
            h5sm->buf = h5sm->allocs->evt(1);
        } else {
            h5sm->buf = h5sm->allocs->evt(0);
            if (!h5sm->buf) {
                return -1;
            }
        }

        if (h5sm->buf) {
            memcpy(h5sm->buf, h5sm->hdr, h5sm->len);
        }

        h5sm->exp_len = H5_HDR_LEN(h5sm->hdr);
        break;
    case HCI_H5_ISO:
        assert(h5sm->allocs && h5sm->allocs->iso);
        h5sm->om = h5sm->allocs->iso();
        if (!h5sm->om) {
            return -1;
        }

        os_mbuf_append(h5sm->om, h5sm->hdr, h5sm->len);
        h5sm->exp_len = (get_le16(&h5sm->hdr[2]) & 0x7fff) + 4;
        break;
    default:
        assert(0);
        break;
    }

    return 0;
}

static int hci_h5_sm_w4_payload(struct hci_h5_sm *h5sm, struct hci_h5_input_buffer *ib)
{
    uint16_t mbuf_len;
    uint16_t len;
    int rc;

    len = min(ib->len, h5sm->exp_len - h5sm->len);

    switch (h5sm->pkt_type) {
    case HCI_H5_ACK:
        break;
    case HCI_H5_LINK:
        if (h5sm->buf) {
            memcpy(&h5sm->buf[h5sm->len], ib->buf, len);
        }
        break;
    case HCI_H5_CMD:
    case HCI_H5_EVT:
        if (h5sm->buf) {
            memcpy(&h5sm->buf[h5sm->len], ib->buf, len);
        }
        break;
    case HCI_H5_ACL:
    case HCI_H5_ISO:
        assert(h5sm->om);

        mbuf_len = OS_MBUF_PKTLEN(h5sm->om);
        rc = os_mbuf_append(h5sm->om, ib->buf, len);
        if (rc) {
            /* Some data may already be appended so need to adjust h5sm only by
             * the size of appended data.
             */
            len = OS_MBUF_PKTLEN(h5sm->om) - mbuf_len;
            h5sm->len += len;
            hci_h5_ib_consume(ib, len);

            return -1;
        }
        break;
    default:
        assert(0);
        break;
    }

    h5sm->len += len;
    hci_h5_ib_consume(ib, len);

    /* return 1 if need more data */
    return h5sm->len != h5sm->exp_len;
}

static void hci_h5_sm_retx_timeout(struct rt_work *work, void *data)
{
    /* TODO */
}

static void hci_h5_sm_ack_timeout(struct rt_work *work, void *data)
{
    struct hci_h5_sm *h5sm = (struct hci_h5_sm *)data;

    hci_h5_sm_send(h5sm, HCI_H5_ACK, NULL, 0);
}

static void hci_h5_sm_completed(struct hci_h5_sm *h5sm)
{
    int rc;

    /* rx_ack should be in every packet */
    h5sm->rx_ack =  H5_HDR_ACK(h5sm->hdr);

    if (hci_h5_reliable_packet(H5_HDR_PKT_TYPE(h5sm->hdr))) {
        /* For reliable packet increment next transmit ack number */
        h5sm->tx_ack = (h5sm->tx_ack + 1) % 8;
        rt_work_submit(&h5sm->ack_work, H5_RX_ACK_TIMEOUT);
    }

    hci_h5_print_header(h5sm->hdr, "RX: >");

    switch (h5sm->pkt_type) {
    case  HCI_H5_ACK:
        break;
    case HCI_H5_LINK:
        if (h5sm->buf) {
            assert(h5sm->frame_cb);
            rc = h5sm->frame_cb(h5sm->pkt_type, h5sm->buf);
            if (rc != 0) {
                free(h5sm->buf);
            }
            h5sm->buf = NULL;
        }
        break;
    case HCI_H5_CMD:
    case HCI_H5_EVT:
        if (h5sm->buf) {
            assert(h5sm->frame_cb);
            rc = h5sm->frame_cb(h5sm->pkt_type, h5sm->buf);
            if (rc != 0) {
                ble_transport_free(h5sm->buf);
            }
            h5sm->buf = NULL;
        }
        break;
    case HCI_H5_ACL:
    case HCI_H5_ISO:
        if (h5sm->om) {
            assert(h5sm->frame_cb);
            rc = h5sm->frame_cb(h5sm->pkt_type, h5sm->om);
            if (rc != 0) {
                os_mbuf_free_chain(h5sm->om);
            }
            h5sm->om = NULL;
        }
        break;
    default:
        assert(0);
        break;
    }

    memset(h5sm->hdr, 0, 4);
}

int hci_h5_sm_rx(struct hci_h5_sm *h5sm, const uint8_t *buf, uint16_t len)
{
    struct hci_h5_input_buffer ib = {
        .buf = buf,
        .len = len,
    };
    uint8_t *hdr;

    int rc = 0;
    while (ib.len && (rc >= 0)) {
        rc = 0;
        switch (h5sm->state) {
        case HCI_H5_SM_W4_PKT_DELIMITER:
            if (ib.buf[0] == SLIP_DELIMITER) {
                h5sm->state = HCI_H5_SM_W4_PKT_TYPE;
            }
            hci_h5_ib_consume(&ib, 1);
            break;
        case HCI_H5_SM_W4_PKT_TYPE:
            if (ib.buf[0] == SLIP_DELIMITER && h5sm->idx == 0) {
                hci_h5_ib_consume(&ib, 1);
                continue;
            }

            hdr = h5sm->hdr;
            rc = hci_h5_unslip_one_byte(&hdr[h5sm->idx], ib.buf[0]);
            if (rc < 0) {
                hci_h5_reset_rx(h5sm);
                break;
            } else if (rc == 1) {
                hci_h5_ib_consume(&ib, 1);
                continue;
            }
            hci_h5_ib_consume(&ib, 1);
            h5sm->idx++;

            if (h5sm->idx < 4)
                continue;
            else
                h5sm->idx = 0;

            if ((~(hdr[0] + hdr[1] + hdr[2]) & 0xFF) != hdr[3]) {
                pr_err("Invalid header checksum.\n");
                h5sm->state = HCI_H5_SM_W4_PKT_DELIMITER;
                continue;
            }

            hci_h5_frame_start(h5sm, H5_HDR_PKT_TYPE(hdr));
            h5sm->state = HCI_H5_SM_W4_HEADER;
            break;
        case HCI_H5_SM_W4_HEADER:
            rc = hci_h5_sm_w4_header(h5sm, &ib);
            assert(rc >= 0);
            if (rc) {
                break;
            }
            h5sm->state = HCI_H5_SM_W4_PAYLOAD;
            break;
        case HCI_H5_SM_W4_PAYLOAD:
            rc = hci_h5_sm_w4_payload(h5sm, &ib);
            assert(rc >= 0);
            if (rc) {
                break;
            }
            h5sm->state = HCI_H5_SM_COMPLETED;
            break;
        case HCI_H5_SM_COMPLETED:
            /*
             * Check when full packet is received, it can be done
             * when parsing packet header but we need to receive
             * full packet anyway to clear UART.
             */
            if (H5_HDR_RELIABLE(h5sm->hdr) && (H5_HDR_SEQ(h5sm->hdr) != h5sm->tx_ack)) {
                pr_warn("Seq expected %u got %u. Drop packet\n", h5sm->tx_ack,
                        H5_HDR_SEQ(h5sm->hdr));
                hci_h5_reset_rx(h5sm);
                break;
            }

            hci_h5_sm_completed(h5sm);
            h5sm->state = HCI_H5_SM_W4_PKT_DELIMITER;
            break;
        default:
            assert(0);
            break;
        }
    }

    /* Calculate consumed bytes
     *
     * Note: we should always consume some bytes unless there is an oom error.
     * It's also possible that we have an oom error but already consumed some
     * data, in such case just return success and error will be returned on next
     * pass.
     */
    len = len - ib.len;
    if (len == 0) {
        assert(rc < 0);
        return -1;
    }

    return len;
}

void hci_h5_sm_init(struct hci_h5_sm *h5sm, const struct hci_h5_allocators *allocs,
                    hci_h5_frame_cb *frame_cb)
{
    memset(h5sm, 0, sizeof(*h5sm));
    h5sm->tx_win = 4;
    h5sm->allocs = allocs;
    h5sm->frame_cb = frame_cb;

    rt_work_init(&h5sm->ack_work, hci_h5_sm_ack_timeout, h5sm);
    rt_work_init(&h5sm->retx_work, hci_h5_sm_retx_timeout, h5sm);
}
