
#include <asm/unaligned.h>
#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/compiler.h>
#include <linux/if_vlan.h>
#include <linux/gfp.h>
#include <linux/if_arp.h>
#include "stabr.h"

#ifdef CONFIG_HGIC_STABR

static void hgic_stabr_arp_rx_handle(struct sk_buff *skb)
{
    struct stabr_table_entry *entry = NULL;
    struct ethhdr *ehdr = NULL;
    struct arphdr *ahdr = NULL;
    u8 *target_mac, *target_ip;

    ehdr = (struct ethhdr *)skb->data;
    if (ntohs(ehdr->h_proto) == ETH_P_8021Q) {
        ahdr = (struct arphdr *)(skb->data + VLAN_ETH_HLEN);
    } else {
        ahdr = (struct arphdr *)(skb->data + ETH_HLEN);
    }
    target_mac = (u8 *)(ahdr + 1) + 10;
    target_ip  = target_mac + ETH_ALEN;
    if (is_unicast_ether_addr(ehdr->h_dest) && get_unaligned((u32 *) target_ip)) {
        entry = hgic_stabr_find_entry(get_unaligned((u32 *) target_ip));
        if (entry) {
            //STABR_PRINT("RX DEST:  %pI4/%pM -> %pM\r\n", target_ip, target_mac, entry->fmac);
            memcpy(target_mac, entry->mac, ETH_ALEN);
            memcpy(ehdr->h_dest, entry->mac, ETH_ALEN);
            //} else {
            //    STABR_PRINT("no entry for %pI4/%pM\r\n", target_ip, target_mac);
            //    hgic_stabr_status();
        }
    }
}

static struct sk_buff *hgic_stabr_arp_tx_handle(struct sk_buff *skb)
{
    struct ethhdr *ehdr = NULL;
    struct arphdr *ahdr = NULL;
    struct sk_buff *ret_skb = skb;
    struct net_device *dev = skb->dev;
    u8  *src_mac, *src_ip;
    u16 flag = 0;

    if (skb_cloned(ret_skb)) {
        ret_skb = skb_copy(ret_skb, GFP_KERNEL);
        if (ret_skb) {
            dev_kfree_skb_any(skb);
        } else {
            ret_skb = skb;
        }
    }

    ehdr = (struct ethhdr *)ret_skb->data;
    if (ntohs(ehdr->h_proto) == ETH_P_8021Q) {
        ahdr = (struct arphdr *)(ret_skb->data + VLAN_ETH_HLEN);
    } else {
        ahdr = (struct arphdr *)(ret_skb->data + ETH_HLEN);
    }

    if (is_unicast_ether_addr(ehdr->h_dest)){ 
        flag |= stabr_entry_flag_ucast; 
    }

    src_mac = (char *)(ahdr + 1);
    src_ip  = src_mac + ETH_ALEN;
    if (is_unicast_ether_addr(ehdr->h_source)) {
        if (get_unaligned((u32 *) src_ip)) {
            hgic_stabr_table_update(get_unaligned((u32 *) src_ip), src_mac, flag);
        }
        //STABR_PRINT("TX  SRC: %pI4/%pM -> %pM\r\n", (u32 *)src_ip, src_mac, dev->dev_addr);
        memcpy(src_mac, dev->dev_addr, ETH_ALEN);
        memcpy(ehdr->h_source, dev->dev_addr, ETH_ALEN);
    }

    return ret_skb;
}

struct stabr_protocol stabr_arp = {
    .protocol = ETH_P_ARP,
    .rx_handler = hgic_stabr_arp_rx_handle,
    .tx_handler = hgic_stabr_arp_tx_handle
};

#endif

