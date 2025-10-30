#ifdef HGIC_RAW
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/list.h>
#include <linux/rcu.h>
#include <linux/rculist.h>
#include <linux/timer.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/ieee80211.h>
#include <net/genetlink.h>
#include <net/cfg80211.h>
#include <net/mac80211.h>

#include "hgic_def.h"

#define HTONS(x) ((u16)((((x) & (u16)0x00ffU) << 8) | (((x) & (u16)0xff00U) >> 8)))
#define NTOHS(x) HTONS(x)
#define HTONL(x) ((((x) & (u32)0x000000ffUL) << 24) | \
                  (((x) & (u32)0x0000ff00UL) <<  8) | \
                  (((x) & (u32)0x00ff0000UL) >>  8) | \
                  (((x) & (u32)0xff000000UL) >> 24))
#define NTOHL(x) HTONL(x)

#define HGIC_RAW_ETHTYPE (0x66AA)
#define RXQ_MAX_SIZE     (16)

struct hgic_wifi {
    char   bssid[6];
    struct net_device   *dev;
    struct semaphore     sem;
    struct sk_buff_head  rxq;
} hgic_raw;

static void wifi_netdev_recv(struct sk_buff *skb)
{
    struct ethhdr *hdr = (struct ethhdr *)skb->data;
    if (NTOHS(hdr->h_proto) == HGIC_RAW_ETHTYPE) {
        while (hgic_raw.rxq.qlen > RXQ_MAX_SIZE) {
            dev_kfree_skb_any(skb_dequeue(&hgic_raw.rxq));
        }
        skb_queue_tail(&hgic_raw.rxq, skb);
        sema_up(&hgic_raw.sem);
    } else {
        kfree_skb(skb);
    }
}

int32 sys_register_netdevice(struct net_device *dev)
{
    hgic_raw.dev = dev;
    dev->priv   = &hgic_raw;
    dev->recv   = wifi_netdev_recv;
    return 0;
}

int32 sys_unregister_netdevice(struct net_device *dev)
{
    hgic_clear_queue(&hgic_raw.rxq);
    hgic_raw.dev = NULL;
    return 0;
}

void hgic_raw_init(void)
{
    memset(&hgic_raw, 0, sizeof(hgic_raw));
    sema_init(&hgic_raw.sem, 0);
    skb_queue_head_init(&hgic_raw.rxq);
}

int hgic_raw_send(char *dest, char *data, int len)
{
    struct sk_buff *skb = NULL;
    struct ethhdr  *hdr = NULL;

    if (hgic_raw.dev == NULL) {
        return -ENXIO;
    }
    if (dest == NULL || data == NULL || len == 0) {
        return -EINVAL;
    }

    skb = alloc_skb(len + sizeof(struct ethhdr) + hgic_raw.dev->needed_headroom);
    if (skb == NULL) {
        return -ENOMEM;
    }
    hdr = skb_put(skb, sizeof(struct ethhdr));
    memcpy(hdr->h_dest, dest, 6);
    memcpy(hdr->h_source, hgic_raw.dev->dev_addr, 6);
    hdr->h_proto = HTONS(HGIC_RAW_ETHTYPE);
    memcpy(skb_put(skb, len), data, len);
    skb->dev = hgic_raw.dev;
    dev_queue_xmit(skb);
    return 0;
}

int hgic_raw_rcev(char *buf, int size, char *src)
{
    int len = 0;
    struct ethhdr  *hdr = NULL;
    struct sk_buff *skb = NULL;

    sema_down(&hgic_raw.sem);
    skb = skb_dequeue(&hgic_raw.rxq);
    if (skb) {
        hdr = (struct ethhdr *)skb->data;
        skb_pull(skb, sizeof(struct ethhdr));
        len = skb->len < size ? skb->len : size;
        memcpy(buf, skb->data, len);
        if (src) {
            memcpy(src, hdr->h_source, 6);
        }
        kfree_skb(skb);
    }
    return len;
}
#endif

