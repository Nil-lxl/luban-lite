#include <asm/unaligned.h>
#include <asm/byteorder.h>
#include <linux/compiler.h>
#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/gfp.h>
#include "stabr.h"

#ifdef CONFIG_HGIC_STABR

static u32 hgic_stabr_ip_find_specID(struct iphdr  *iph)
{
    struct udphdr *udp = (struct udphdr *)(iph + 1);

    if (iph->protocol == 17) {
        if (ntohs(udp->source) == 67 || ntohs(udp->dest) == 67) {
            return get_unaligned((uint8_t *)(udp + 1) + 4);
        }
    }

    return 0;
}

static void hgic_stabr_ip_rx_handle(struct sk_buff *skb)
{
    struct ethhdr *ehdr = NULL;
    struct iphdr  *iph  = NULL;
    struct stabr_table_entry *entry = NULL;
    u32 id = 0;

    ehdr = (struct ethhdr *)skb->data;
    if (ntohs(ehdr->h_proto) == ETH_P_8021Q) {
        iph = (struct iphdr *)(skb->data + VLAN_ETH_HLEN);
    } else {
        iph = (struct iphdr *)(skb->data + ETH_HLEN);
    }

    id = hgic_stabr_ip_find_specID(iph);
    if (!id && get_unaligned(&iph->daddr)) {
        id = get_unaligned(&iph->daddr);
    }

    if (id && is_unicast_ether_addr(ehdr->h_dest)) {
        entry = hgic_stabr_find_entry(id);
        if (entry) {
            //STABR_PRINT("RX DEST: %pI4/%pM -> %pM\r\n", &iph->daddr, ehdr->h_dest, entry->fmac);
            memcpy(ehdr->h_dest, entry->mac, ETH_ALEN);
            //}else{
            //STABR_PRINT("no entry for %pI4/%pM\r\n", &id, ehdr->h_dest);
            //hgic_stabr_status();
        }
    }
}

static struct sk_buff *hgic_stabr_ip_tx_handle(struct sk_buff *skb)
{
    struct ethhdr  *ehdr = NULL;
    struct iphdr   *iph  = NULL;
    struct sk_buff *ret_skb = skb;
    struct net_device *dev = skb->dev;
    u32 id = 0;
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
        iph = (struct iphdr *)(ret_skb->data + VLAN_ETH_HLEN);
    } else {
        iph = (struct iphdr *)(ret_skb->data + ETH_HLEN);
    }

    if (is_unicast_ether_addr(ehdr->h_dest)){ 
        flag |= stabr_entry_flag_ucast; 
    }

    id = hgic_stabr_ip_find_specID(iph);
    if (!id && get_unaligned(&iph->saddr)) {
        id = get_unaligned(&iph->saddr);
    }

    if (id && is_unicast_ether_addr(ehdr->h_source)) {
        hgic_stabr_table_update(id, ehdr->h_source, flag);
    }

    //STABR_PRINT("TX  SRC: %pI4/%pM -> %pM\r\n", &iph->saddr, ehdr->h_source, dev->dev_addr);
    memcpy(ehdr->h_source, dev->dev_addr, ETH_ALEN);
    return ret_skb;
}

struct stabr_protocol stabr_ip = {
    .protocol = ETH_P_IP,
    .rx_handler = hgic_stabr_ip_rx_handle,
    .tx_handler = hgic_stabr_ip_tx_handle
};

#endif

