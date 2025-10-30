
#ifdef __RTOS__
#include <linux/types.h>
#include <linux/unaligned.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/hrtimer.h>
#else
#include <linux/version.h>
#include <linux/module.h>
#include <linux/etherdevice.h>
#include <linux/sched.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#endif

#include "hgics.h"
#include "util.h"
#include "../utils/utils.h"

static struct sk_buff *hgics_trans_q_get(struct hgics_wdev *hg, u16 cookie)
{
    ulong flags = 0;
    struct sk_buff *found = NULL;
    struct sk_buff *skb = NULL;
    struct hgic_hdr *hdr = NULL;

    spin_lock_irqsave(&hg->trans_q.lock, flags);
    if (!skb_queue_empty(&hg->trans_q)) {
        skb_queue_walk(&hg->trans_q, skb) {
            hdr = (struct hgic_hdr *)(skb->data);
            if (hdr->cookie == cpu_to_le16(cookie)) {
                found = skb;
                break;
            }
        }
    }
    spin_unlock_irqrestore(&hg->trans_q.lock, flags);
    return found;
}

static void hgics_rx_ack(struct hgics_wdev *hg, struct hgics_txstatus *status)
{
    unsigned long flags;
    struct sk_buff *skb = NULL;
    struct sk_buff *found = NULL;
    struct hgic_frm_hdr *frm = NULL;
    struct ieee80211_tx_info *txi = NULL;

    spin_lock_irqsave(&hg->ack_q.lock, flags);
    if (!skb_queue_empty(&hg->ack_q)) {
        skb_queue_walk(&hg->ack_q, skb) {
            frm = (struct hgic_frm_hdr *)(skb->data);
            if (frm->hdr.cookie == cpu_to_le16(status->cookie)) {
                __skb_unlink(skb, &hg->ack_q);
                found = skb;
                break;
            }
        }
    }
    if (!found) {
        found = hgics_trans_q_get(hg, status->cookie);
        if (found) {
            txi = IEEE80211_SKB_CB(found);
            txi->flags |= IEEE80211_TX_CTL_NO_ACK;
        }
        found = NULL;
    }
    spin_unlock_irqrestore(&hg->ack_q.lock, flags);

    if (found) {
        txi = IEEE80211_SKB_CB(found);
        if (status->acked) {
            txi->flags |= IEEE80211_TX_STAT_ACK;
        }
        skb_pull(found, sizeof(struct hgic_frm_hdr));
        ieee80211_tx_status_irqsafe(hg->hw, found);
    }
}

static void hgics_print_hex(char *buf, int len)
{
    int i = 0;

    for (i = 0; i < len; i++) {
        if (i > 0 && i % 16 == 0) { PRINTF("\r\n"); }
        else if (i > 0 && i % 8 == 0) { PRINTF("  "); }
        PRINTF("%02x ", buf[i] & 0xff);
    }
    PRINTF("\r\n\r\n");
}

static void hgics_rx_sigfrm(struct hgics_wdev *hg, u8 *data, int len)
{
    struct ieee80211_rx_status rx_status;
    struct hgic_frm_hdr *hdr = (struct hgic_frm_hdr *)data;
    struct sk_buff *skb = dev_alloc_skb(len);

    if (skb == NULL) {
        hgic_err("alloc skb fail, len=%d\r\n", len);
        return;
    }

    if ((u16)(hg->rx_cookie + 1) != hdr->hdr.cookie) {
        hgic_err("cookie:%d-%d\r\n", hg->rx_cookie, hdr->hdr.cookie);
    }
    if(len <= (sizeof(struct hgic_frm_hdr) + hdr->hdr.flags)) {
        hgic_err("invaild recv len:%d,drop pkt!\n",len);
        dev_kfree_skb(skb);
        return;
    }

    data += sizeof(struct hgic_frm_hdr);
    len  -= (sizeof(struct hgic_frm_hdr) + hdr->hdr.flags);
    hg->rx_bytes += len;
    hg->rx_cookie = hdr->hdr.cookie;

    memset(&rx_status, 0, sizeof(rx_status));
    rx_status.mactime = ktime_to_us(ktime_get_real());
    rx_status.flag |= RX_FLAG_MACTIME_START | RX_FLAG_DECRYPTED | RX_FLAG_MMIC_STRIPPED | RX_FLAG_IV_STRIPPED;
    rx_status.freq = le16_to_cpu(hdr->rx_info.freq) ? le16_to_cpu(hdr->rx_info.freq) : hg->fw_freq;
    rx_status.band = hdr->rx_info.band;
    rx_status.rate_idx = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0) && defined(CONFIG_HGIC_AH)
    rx_status.vht_nss = hdr->rx_info.nss;
#endif
#ifdef CONFIG_HGIC_AH
    rx_status.vht_flag = hdr->rx_info.vht_flag;
    rx_status.s1g_nss  = hdr->rx_info.s1g_nss;
#endif
    rx_status.signal = hdr->rx_info.signal;

    skb_reserve(skb, sizeof(struct hgic_frm_hdr));
    memcpy(skb->data, data, len);
    skb_put(skb, len);
    memcpy(IEEE80211_SKB_RXCB(skb), &rx_status, sizeof(struct ieee80211_rx_status));
#ifndef __RTOS__
    hgics_icmp_monitor(hg->ctrl.icmp_mntr, skb, 0);
#endif

//    struct ieee80211_hdr *iehdr = data;
//    __le16 fc = iehdr->frame_control;
//    if (ieee80211_is_data(fc)) {
//        hgic_dbg("Recv data pkt:len:%d\n",len);
//        hgics_print_hex(iehdr, sizeof(struct ieee80211_hdr));
//    }

    ieee80211_rx_irqsafe(hg->hw, skb);
}

static void hgics_rx_agg_frm(struct hgics_wdev *hg, u8 *data, int len)
{
    struct hgic_hdr *hdr = (struct hgic_hdr *)data;

    if ((u16)(hg->rx_cookie + 1) != hdr->cookie) {
        hgic_err("cookie:%d-%d\r\n", hg->rx_cookie, hdr->cookie);
    }
    hg->rx_cookie = hdr->cookie;
    if(len < sizeof(struct hgic_hdr)) {
        hgic_err("Invaild agg rx len:%d\n",len);
        return;
    }

    data += sizeof(struct hgic_hdr);
    len  -= sizeof(struct hgic_hdr);
    while (len > sizeof(struct hgic_frm_hdr)) {
        hdr = (struct hgic_hdr *)data;
        hgic_hdr_format(hdr, 0);
        if (hdr->magic == HGIC_HDR_RX_MAGIC && hdr->type == HGIC_HDR_TYPE_FRM && len >= hdr->length) {
            hgics_rx_sigfrm(hg, data, hdr->length);
            data += hdr->length;
            len  -= hdr->length;
        } else {
            hgic_err("invalid agg data\r\n");
            break;
        }
    }
}

static void hgics_rx_bt_data(struct hgics_wdev *hg, u8 *data, int len)
{
#if 1//ndef __RTOS__
    struct hgic_ctrl_hdr *hdr = (struct hgic_ctrl_hdr *)data;

    if ((u16)(hg->rx_cookie + 1) != hdr->hdr.cookie) {
        hgic_err("cookie:%d-%d\r\n", hg->rx_cookie, hdr->hdr.cookie);
    }
    hg->rx_cookie = hdr->hdr.cookie;

#ifdef CONFIG_BT
    if (hg->hci) {
		data += sizeof(struct hgic_ctrl_hdr);
		len  -= sizeof(struct hgic_ctrl_hdr);
        hci_recv_fragment(hg->hci, hdr->hci.type, data, len);
    } else
#endif
    {
        hgic_fwctrl_rx(&hg->ctrl, data, len);
    }
#else
    hgic_dbg("Recv blenc data\n");
    hgic_fwctrl_rx(&hg->ctrl, data, len);
#endif
}

int hgics_rx_data(void *hgobj, u8 *data, int len)
{
    int i = 0;
    struct hgics_wdev *hg = hgobj;
    struct hgic_frm_hdr *hdr = NULL;
    struct hgic_dack_hdr *ackhdr = NULL;
    struct hgics_txstatus txstat;

    i = hgic_skip_padding(data);
    data += i; len -= i;
    hdr = (struct hgic_frm_hdr *)data;
    hgic_hdr_format((struct hgic_hdr *)hdr, 0);
    if (hdr->hdr.magic != HGIC_HDR_RX_MAGIC) {
        hgic_err("invalid rx magic: %x\r\n", hdr->hdr.magic);
        return -1;
    }

    if (hdr->hdr.type != HGIC_HDR_TYPE_BOOTDL && len < hdr->hdr.length) {
        hgic_err("invalid data length: %x/%x, type:%d\r\n", len, hdr->hdr.length, hdr->hdr.type);
        //hgics_print_hex(data, 16);
        return -1;
    }

    switch (hdr->hdr.type) {
        case HGIC_HDR_TYPE_ACK:
            hg->last_rx = jiffies;
            if (!test_bit(HGICS_STATE_START, &hg->state) || hg->if_test) {
                return -1;
            }
            if(len < sizeof(struct hgic_dack_hdr)) {
                hgic_err("invaild recv len:%d\n",len);
                return -1;
            }
            ackhdr = (struct hgic_dack_hdr *)data;
            for (i = 0; i < ackhdr->hdr.length && i < 2 * HGIC_BLOCK_ACK_CNT; i++) {
                memset(&txstat, 0, sizeof(txstat));
                txstat.cookie = le16_to_cpu(ackhdr->cookies[i]) & HGIC_TX_COOKIE_MASK;
                txstat.acked  = (le16_to_cpu(ackhdr->cookies[i]) & 0x8000) ? 1 : 0;
                hgics_rx_ack(hg, &txstat);
            }
            break;
        case HGIC_HDR_TYPE_FRM:
        case HGIC_HDR_TYPE_AGGFRM:
            hg->last_rx = jiffies;
            len = (len < hdr->hdr.length ? len : hdr->hdr.length);
            if (!test_bit(HGICS_STATE_START, &hg->state) || hg->if_test) {
                return -1;
            }
            if (hdr->hdr.type == HGIC_HDR_TYPE_AGGFRM) {
                hgics_rx_agg_frm(hg, data, len);
            } else {
                hgics_rx_sigfrm(hg, data, len);
            }
            break;
        case HGIC_HDR_TYPE_CMD:
        case HGIC_HDR_TYPE_CMD2:
        case HGIC_HDR_TYPE_EVENT:
        case HGIC_HDR_TYPE_EVENT2:
        case HGIC_HDR_TYPE_BOOTDL:
        case HGIC_HDR_TYPE_OTA:
            len = (len < hdr->hdr.length ? len : hdr->hdr.length);
            hgic_fwctrl_rx(&hg->ctrl, data, len);
            break;
        case HGIC_HDR_TYPE_TEST2:
            len = (len > 1600 ? len : hdr->hdr.length);
            hg->last_rx = jiffies;
            hg->test_rx_len += len;
            if (hg->if_test == 3) {
                i += sizeof(struct hgic_hdr);
                while(i < len){
                    if(data[i] == 0x1A && data[i+1] == 0x2B){
                        i += sizeof(struct hgic_hdr);
                    }else{
                        if (data[i] != 0xAA && data[i] != 0x55) {
                            printk("IF_TEST3: data verify fail\r\n");
                            break;
                        }
                        i++;
                    }
                }
            }
            break;
        case HGIC_HDR_TYPE_SOFTFC:
            hg->last_rx = jiffies;
            atomic_set(&hg->txwnd, hdr->hdr.cookie);
            complete(&hg->txwnd_cp);
            break;
        case HGIC_HDR_TYPE_BLUETOOTH:
            if (!test_bit(HGICS_STATE_INITED, &hg->state) || hg->if_test) {
                return -1;
            }
            len = (len < hdr->hdr.length ? len : hdr->hdr.length);
            hgics_rx_bt_data(hg, data, len);
            break;
        default:
            hgic_err("unknow packet type:%d, len:%d, cookie:%x\n", hdr->hdr.type, len, hdr->hdr.cookie);
            return -1;
    }
    return 0;
}

