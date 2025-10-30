
#include <asm/unaligned.h>
#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/if_vlan.h>
#include <linux/skbuff.h>
#include <linux/compiler.h>
#include <linux/gfp.h>
#include <net/mac80211.h>
#include "stabr.h"
#include "util.h"

#ifdef CONFIG_HGIC_STABR
#define ID_IDX(id)           ((id)&0x0f)

extern struct stabr_protocol stabr_arp;
extern struct stabr_protocol stabr_ip;
extern ulong ieee80211_rx_cb;

static struct stabr_protocol *g_stabr_protos[] = {
    &stabr_arp,
    &stabr_ip,
};

static struct stabr_table s_stabr_table;
static const struct net_device_ops *_nops_bak = NULL;
static struct net_device_ops  _nops_new;

static void hgic_stabr_add(struct stabr_table_entry *entry)
{
    struct list_head *head = &s_stabr_table.list[ID_IDX(entry->id)];

    if (entry->list.next) {
        list_del(&entry->list);
    }
    list_add(&entry->list, head);
}

static void hgic_stabr_table_check(void)
{
    struct stabr_table_entry *entry = NULL;
    u32 mtmo = STABR_ENTRY_LIFETIME;
    u32 tmo  = 0;
    u32 i    = 0;
    struct list_head *head;

    spin_lock(&s_stabr_table.lock);
    if (s_stabr_table.entry_cnt >= s_stabr_table.max_cnt) {
        mtmo = 1000;
    } else if (s_stabr_table.entry_cnt >= (s_stabr_table.max_cnt / 2)) {
        mtmo = 5000;
    }

    for (i = 0; i < 16; i++) {
        head = &s_stabr_table.list[i];
        while (!list_empty(head)) {
            entry = list_entry(head->prev, struct stabr_table_entry, list);
            tmo   = (entry->flag & stabr_entry_flag_ucast) ? STABR_ENTRY_LIFETIME : mtmo;
            if (time_after(jiffies, entry->lifetime + tmo)) {
                list_del(&entry->list);
                s_stabr_table.entry_cnt--;
                kfree(entry);
            } else {
                break;
            }
        }
    }
    spin_unlock(&s_stabr_table.lock);
}

struct stabr_table_entry *hgic_stabr_table_update(u32 id, char *mac, u16 flag)
{
    struct stabr_table_entry *entry = NULL;

    if (id == 0) {
        return entry;
    }

    entry = hgic_stabr_find_entry(id);
    if (entry) {
        memcpy(entry->mac, mac, ETH_ALEN);
        entry->lifetime = jiffies;
        entry->flag    |= flag;
        spin_lock(&s_stabr_table.lock);
        hgic_stabr_add(entry);
        spin_unlock(&s_stabr_table.lock);
    } else {
        entry = kmalloc(sizeof(struct stabr_table_entry), GFP_ATOMIC);
        if (entry) {
            memset(entry, 0, sizeof(struct stabr_table_entry));
            entry->id = id;
            entry->flag = flag;
            entry->lifetime = jiffies;
            memcpy(entry->mac, mac, ETH_ALEN);
            spin_lock(&s_stabr_table.lock);
            hgic_stabr_add(entry);
            s_stabr_table.entry_cnt++;
            spin_unlock(&s_stabr_table.lock);
            //STABR_PRINT("update: %pI4/%pM\r\n", &entry->id, mac);
        }
    }

    hgic_stabr_table_check();
    return entry;
}

struct stabr_table_entry *hgic_stabr_find_entry(u32 id)
{
    struct stabr_table_entry *entry = NULL;
    struct stabr_table_entry *find  = NULL;
    struct list_head *head = &s_stabr_table.list[ID_IDX(id)];

    if (id == 0) {
        return NULL;
    }

    spin_lock(&s_stabr_table.lock);
    if (!list_empty(head)) {
        list_for_each_entry(entry, head, list) {
            if (entry->id == id) {
                find = entry;
                break;
            }
        }
    }
    spin_unlock(&s_stabr_table.lock);
    return find;
}

static struct stabr_protocol *hgic_stabr_get_proto(struct sk_buff *skb)
{
    int i = 0;
    int protocol = 0;
    struct ethhdr *ehdr = NULL;

    ehdr = (struct ethhdr *)skb->data;
    if (ntohs(ehdr->h_proto) == ETH_P_8021Q) {
        protocol = ntohs(get_unaligned((u16 *)(skb->data + 16)));
    } else {
        protocol = ntohs(ehdr->h_proto);
    }

    for (i = 0; i < ARRAY_SIZE(g_stabr_protos); i++) {
        if (g_stabr_protos[i]->protocol == protocol) {
            return g_stabr_protos[i];
        }
    }

    return NULL;
}

static void hgic_stabr_rx(void *priv, struct sk_buff *skb)
{
    struct stabr_protocol *proto = NULL;
    if (get_unaligned_le32(priv) == 0xD8833253 && skb->dev) {
        if (skb->dev->netdev_ops == (const struct net_device_ops *)&_nops_new &&
            skb->dev->ieee80211_ptr->iftype == NL80211_IFTYPE_STATION && (skb->dev->priv_flags & IFF_BRIDGE_PORT)) {
            proto = hgic_stabr_get_proto(skb);
            if (proto) {
                proto->rx_handler(skb);
            }
        }
    }
}

static netdev_tx_t hgic_stabr_xmit(struct sk_buff *skb, struct net_device *dev)
{
    struct stabr_protocol *proto = NULL;
    struct sk_buff *nskb = skb;

    if (_nops_bak == NULL) {
        return NETDEV_TX_BUSY;
    }

    if (skb->dev->ieee80211_ptr->iftype == NL80211_IFTYPE_STATION && (skb->dev->priv_flags & IFF_BRIDGE_PORT)) {
        if (!ether_addr_equal(skb->dev->dev_addr, skb->data + ETH_ALEN)) {
            proto = hgic_stabr_get_proto(skb);
            if (proto) {
                nskb = proto->tx_handler(skb);
            } else {
                kfree_skb(skb);
                return NETDEV_TX_OK;
            }
        }
    }

    return _nops_bak->ndo_start_xmit(nskb, dev);
}

void hgic_stabr_attach(struct net_device *dev)
{
    if (_nops_bak == NULL) {
        memcpy(&_nops_new, dev->netdev_ops, sizeof(struct net_device_ops));
        _nops_new.ndo_start_xmit = hgic_stabr_xmit;
        _nops_bak = dev->netdev_ops;
    }
    dev->priv_flags &= ~IFF_DONT_BRIDGE;
    dev->netdev_ops = (const struct net_device_ops *)&_nops_new;
}

int hgic_stabr_init(void)
{
    int i = 0;

    memset(&s_stabr_table, 0, sizeof(s_stabr_table));
    for (i = 0; i < 16; i++) {
        INIT_LIST_HEAD(&s_stabr_table.list[i]);
    }
    spin_lock_init(&s_stabr_table.lock);
    s_stabr_table.max_cnt = 1024;
    ieee80211_rx_cb = (ulong)hgic_stabr_rx;
    return 0;
}

int hgic_stabr_release(void)
{
    int i = 0;
    struct stabr_table_entry *entry, *n;
    struct list_head *head = NULL;

    spin_lock(&s_stabr_table.lock);
    for (i = 0; i < 16; i++) {
        head = &s_stabr_table.list[i];
        if (!list_empty(head)) {
            list_for_each_entry_safe(entry, n, head, list) {
                list_del(&entry->list);
                kfree(entry);
            }
        }
    }
    spin_unlock(&s_stabr_table.lock);
    ieee80211_rx_cb = 0;
#ifdef __RTOS__
    spin_lock_deinit(&s_stabr_table.lock);
#endif
    return 0;
}

/*
u32 ieee80211_rx_cb = 0;
EXPORT_SYMBOL(ieee80211_rx_cb);
if(ieee80211_rx_cb && rx->local->hw.priv && get_unaligned_le32(rx->local->hw.priv) == 0xD8833253){
    ((void (*)(struct sk_buff *))ieee80211_rx_cb)(skb);
}
*/
#endif

