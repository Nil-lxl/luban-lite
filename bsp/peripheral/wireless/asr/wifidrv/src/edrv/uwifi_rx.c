/**
 ****************************************************************************************
 *
 * @file uwifi_rx.c
 *
 * @brief rx related function
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#include "uwifi_rx.h"
#include "uwifi_include.h"
#include "wpa_adapter.h"
#include "uwifi_kernel.h"
#include "uwifi_platform.h"
#include "uwifi_msg.h"
#include "asr_types.h"
#include "ethernetif_wifi.h"
#include "uwifi_msg_tx.h"
#include "hostapd.h"

#if !defined ALIOS_SUPPORT && !defined JL_SDK
//#include <net/net_if.h>
#endif

#ifdef LWIP
#include "arch.h"
#endif
#include "asr_dbg.h"
#define ETH_HEADER_SIZE 14

#ifdef CFG_SNIFFER_SUPPORT
monitor_cb_t sniffer_rx_cb;
monitor_cb_t sniffer_rx_mgmt_cb;
#endif
#define UDP_OFFSET_INIPHD   9
#define ETH_HEADER_SIZE     14
#define UDP_PROTCOL_INIPHD  17
#define DHCP_PORT_SERVER    67
#define DHCP_PORT_CLIENT    68



/* See IEEE 802.1H for LLC/SNAP encapsulation/decapsulation */
/* Ethernet-II snap header (RFC1042 for most EtherTypes) */
const unsigned char rfc1042_header[]  __attribute((aligned(2))) =
    { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };

/* Bridge-Tunnel header (for EtherTypes ETH_P_AARP and ETH_P_IPX) */
const unsigned char bridge_tunnel_header[]  __attribute((aligned(2))) =
    { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0xf8 };

const uint8_t legrates_lut[] = {
    0,                          /* 0 */
    1,                          /* 1 */
    2,                          /* 2 */
    3,                          /* 3 */
    -1,                         /* 4 */
    -1,                         /* 5 */
    -1,                         /* 6 */
    -1,                         /* 7 */
    10,                         /* 8 */
    8,                          /* 9 */
    6,                          /* 10 */
    4,                          /* 11 */
    11,                         /* 12 */
    9,                          /* 13 */
    7,                          /* 14 */
    5                           /* 15 */
};


/**
 * is_eapol_pkt - check whether the rx pkt is a eapol pkt or not
 *
 * @rx_pkt: received rx pkt mac802.11 pkt, start from 802.11 mac header.
 * @return: true(1) if it is eapol pkt; false(0) if it is not.
 */
bool is_eapol_pkt(uint8_t* rx_pkt)
{
    uint8_t* ptr = rx_pkt;
    bool ret = false;

    ptr += (ETH_HEADER_SIZE-2);  //ptr point to protocol filed
    if((*ptr==0x88)&&(*(ptr+1)==0x8E)) //0x888e is 802.1x type
    {
        ret = true;
    }
    return ret;
}

bool is_ipv6_pkt(uint8_t* rx_pkt)
{
    uint8_t* ptr = rx_pkt;
    bool ret = false;

    ptr += (ETH_HEADER_SIZE-2);  //ptr point to protocol filed
    if((*ptr==0x86)&&(*(ptr+1)==0xdd)) //0x86DD is IPV6 type
    {
        ret = true;
    }
    return ret;
}

bool is_arp_pkt(uint8_t* rx_pkt)
{
    uint8_t* ptr = rx_pkt;
    bool ret = false;

    ptr += (ETH_HEADER_SIZE-2);  //ptr point to protocol filed
    if((*ptr==0x08)&&(*(ptr+1)==0x06)) //0x0806 is arp type
    {
        ret = true;
    }
    return ret;
}

#if 0
bool is_dhcp_pkt(uint8_t* rx_pkt)
{
    uint8_t* ptr = rx_pkt;
    bool ret = false;
    uint16_t sport=0,dport = 0;
    uint8_t* data = rx_pkt + sizeof(struct ethhdr);
    uint8_t direct_ps =0;

    ptr += (ETH_HEADER_SIZE-2);  //ptr point to protocol filed
    if((*ptr==0x08)&&(*(ptr+1)==0x00)) //0x0800 is ip type
    {
        if ((*data == 0x45) && (*(data+9) == 0x11))
    {
            sport = wlan_ntohs(*(uint16_t *)(data + 20));
            dport = wlan_ntohs(*(uint16_t *)(data + 22));

            if (sport == 67 && dport == 68) {
            direct_ps =2;
        }else if (sport == 68 && dport == 67) {
                    direct_ps =1;
        }

        if (direct_ps != 0) {
                ret = true;
        }
    }
    }
    return ret;
}
#endif

bool check_is_dhcp(struct asr_vif *asr_vif,bool is_tx, uint16_t eth_proto, uint8_t* type, uint8_t** d_mac, uint8_t* data,
    uint32_t data_len)
{
    uint16_t sport = 0, dport = 0;
    int index = 0;
    uint8_t direct_ps = 0;

    if (!data || data_len <= 270) {
        return false;
    }

    if (eth_proto == 0x800 && data[0] == 0x45 && data[9] == 0x11) {
        sport = wlan_ntohs(*(u16 *) (data + 20));
        dport = wlan_ntohs(*(u16 *) (data + 22));

        if (sport == 67 && dport == 68) {
            direct_ps = 2;
        } else if (sport == 68 && dport == 67) {
            direct_ps = 1;
        }

        if (direct_ps != 0) {
            index = 44;
            if (data[44] == 0 && data_len > 285 && data[270] == 3) {
                index = 282;
            }

            if (type) {
                *type = data[270];
            }

            if (d_mac) {
                *d_mac = data + 56;
            }

            if((ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_STATION) && ether_addr_equal(asr_vif->wlan_mac_add.mac_addr,(uint8_t*)(data + 56))) {
                #if 1
                dbg(D_ERR, D_UWIFI_DATA, "DHCP:%d,%d,%d,%d,%d,%d:%d:%d:%d,%02X:%02X:%02X:%02X:%02X:%02X\n",
                     is_tx, data[28], data[270], sport, dport, data[index], data[index + 1],
                     data[index + 2], data[index + 3], data[56], data[57], data[58], data[59], data[60],
                     data[61]);
                #endif
            }

            return true;
        }
    }

    return false;
}

#define ASR_IPH_CHKSUM_SET(hdr, chksum) (hdr)->_chksum = (chksum)
#define ASR_FOLD_U32T(u)          ((uint32_t)(((u) >> 16) + ((u) & 0x0000ffffUL)))
#define ASR_SWAP_BYTES_IN_WORD(w) (((w) & 0xff) << 8) | (((w) & 0xff00) >> 8)
#define ASR_IP_HLEN 20
#define ASR_IP_PROTO_UDP     17
uint16_t asr_standard_chksum(const void *dataptr, int len)
{
  const uint8_t *pb = (const uint8_t *)dataptr;
  const uint16_t *ps;
  uint16_t t = 0;
  uint32_t sum = 0;
  int odd = ((uintptr_t)pb & 1);

  /* Get aligned to uint16_t */
  if (odd && len > 0) {
    ((uint8_t *)&t)[1] = *pb++;
    len--;
  }

  /* Add the bulk of the data */
  ps = (const uint16_t *)(const void *)pb;
  while (len > 1) {
    sum += *ps++;
    len -= 2;
  }

  /* Consume left-over byte, if any */
  if (len > 0) {
    ((uint8_t *)&t)[0] = *(const uint8_t *)ps;
  }

  /* Add end bytes */
  sum += t;

  /* Fold 32-bit sum to 16 bits
     calling this twice is probably faster than if statements... */
  sum = ASR_FOLD_U32T(sum);
  sum = ASR_FOLD_U32T(sum);

  /* Swap if alignment was odd */
  if (odd) {
    sum = ASR_SWAP_BYTES_IN_WORD(sum);
  }

  return (uint16_t)sum;
}


uint16_t asr_inet_chksum(const void *dataptr, uint16_t len)
{
  return (uint16_t)~(unsigned int)asr_standard_chksum(dataptr, len);
}

void asr_set_ipchecksum(struct asr_ip_hdr *iphdr)
{
    uint16_t ip_hlen = ASR_IP_HLEN;

    if (!iphdr) {
        return;
    }

    ASR_IPH_CHKSUM_SET(iphdr, 0);
    ASR_IPH_CHKSUM_SET(iphdr, asr_inet_chksum(iphdr, ip_hlen));

    return;
}

static uint16_t asr_inet_cksum_pseudo_base(uint8_t *data, uint8_t proto, uint16_t proto_len, uint32_t acc)
{
    uint8_t swapped = 0;


    acc += asr_standard_chksum(data, proto_len);
    /*LWIP_DEBUGF(INET_DEBUG, ("inet_chksum_pseudo(): unwrapped lwip_chksum()=%"X32_F" \n", acc));*/
    /* just executing this next line is probably faster that the if statement needed
       to check whether we really need to execute it, and does no harm */
    acc = ASR_FOLD_U32T(acc);
    if (proto_len % 2 != 0) {
      swapped = 1 - swapped;
      acc = ASR_SWAP_BYTES_IN_WORD(acc);
    }


    if (swapped) {
        acc = ASR_SWAP_BYTES_IN_WORD(acc);
    }

    acc += (uint32_t)wlan_htons((uint16_t)proto);
    acc += (uint32_t)wlan_htons(proto_len);

    /* Fold 32-bit sum to 16 bits
     calling this twice is probably faster than if statements... */
    acc = ASR_FOLD_U32T(acc);
    acc = ASR_FOLD_U32T(acc);

    return (uint16_t)~(acc & 0xffffUL);
}

uint16_t asr_inet_chksum_pseudo(uint8_t *data, uint8_t proto, uint16_t proto_len,
       uint32_t src, uint32_t dest)
{
    uint32_t acc;

    acc = (src & 0xffffUL);
    acc += ((src >> 16) & 0xffffUL);

    acc += (dest & 0xffffUL);
    acc += ((dest >> 16) & 0xffffUL);
    /* fold down to 16 bits */
    acc = ASR_FOLD_U32T(acc);
    acc = ASR_FOLD_U32T(acc);

    return asr_inet_cksum_pseudo_base(data, proto, proto_len, acc);
}

void asr_set_udpchecksum(uint8_t *data, uint32_t data_len, uint32_t src_ip, uint32_t dst_ip)
{
    uint16_t udpchksum;
    struct asr_udp_hdr *udphdr = (struct asr_udp_hdr*)data;
    //uint16_t chksum_old = 0;

    //chksum_old = udphdr->chksum;
    udphdr->chksum = 0;

    udpchksum = asr_inet_chksum_pseudo(data, ASR_IP_PROTO_UDP, data_len,
        src_ip, dst_ip);

    /* chksum zero must become 0xffff, as zero means 'no checksum' */
    if (udpchksum == 0x0000) {
      udpchksum = 0xffff;
    }
    udphdr->chksum = udpchksum;

    //printf("%s:data_len=%d,src_ip=0x%X,dst_ip=0x%X,udpchksum=0x%X,chksum_old=0x%X\n",
    //    __func__, data_len, src_ip, dst_ip, udpchksum, chksum_old);

    return;
}

extern bool txlogen;
void uwifi_dispatch_data(struct asr_vif *asr_vif,struct sk_buff *rx_skb)
{
    struct recv_frame rec_frame;

    rec_frame.u.hdr.len = rx_skb->len;
    rec_frame.u.hdr.rx_data = rx_skb->data;

    //dbg(D_ERR, D_UWIFI_DATA, "%s: len=%d\n", __func__, rx_skb->len);
#ifdef CFG_ENCRYPT_SUPPORT
    if (is_eapol_pkt(rec_frame.u.hdr.rx_data)) {
        enum wlan_op_mode mode = WLAN_OPMODE_INFRA;
        if (rec_frame.u.hdr.rx_data[15] == 3) //eapol key filed value is 3
        {

            uint8_t *eapol_data = asr_rtos_malloc(rx_skb->len);
            if(!eapol_data)
            {
                dbg(D_ERR, D_UWIFI_DATA, "uwifi_dispatch_data():  alloc eapol_data failed\n");
                return;
            }
            memcpy(eapol_data,rx_skb->data,rx_skb->len);
            if(asr_vif->iftype == NL80211_IFTYPE_STATION)
            {
                mode = WLAN_OPMODE_INFRA;
                /* when there was EAPOL in RX queue, in reconnect process, the EAPOL should not be handled */
                if ((asr_vif->sta.connInfo.wifiState != WIFI_ASSOCIATED) && (asr_vif->sta.connInfo.wifiState != WIFI_CONNECTED))
                {
                    asr_rtos_free(eapol_data);
                    eapol_data = NULL;
                    return;
                }
            }
            else if(asr_vif->iftype == NL80211_IFTYPE_AP)
            {
                mode = WLAN_OPMODE_MASTER;
            }
            else
            {
                 dbg(D_ERR, D_UWIFI_CTRL, "mode err");
            }

            if (txlogen) {

                struct ethhdr *eth = (struct ethhdr *)(rec_frame.u.hdr.rx_data);

                dbg(D_ERR, D_UWIFI_CTRL, "%s rx eapol data: dst: %x:%x:%x:%x:%x:%x, src: %x:%x:%x:%x:%x:%x, 0x%x,len=%d,data=%p", __func__,
                    eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5],
                    eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5],
                    eth->h_proto,
                rx_skb->len,rx_skb->data);
        }

            uwifi_msg_rxu2u_eapol_packet_process(eapol_data,rx_skb->len,(uint32_t)mode);
        }
    } else
#endif
#ifndef  LWIP_DUALSTACK
    if(is_ipv6_pkt(rec_frame.u.hdr.rx_data)) {
        return;
    } else
#endif
    {
        struct ethernetif *ethernet_if = (struct ethernetif *)asr_vif->net_if;
        RX_PACKET_INFO_T rx_packet;
        struct ethhdr *eth = (struct ethhdr *)(rec_frame.u.hdr.rx_data);
        uint8_t dhcp_type = 0;
        uint8_t *d_mac = NULL;

        if (is_arp_pkt(rec_frame.u.hdr.rx_data) && txlogen) {

            dbg(D_ERR, D_UWIFI_CTRL, "%s rx arp: dst: %x:%x:%x:%x:%x:%x, src: %x:%x:%x:%x:%x:%x, 0x%x", __func__,
                eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5],
                eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5],
                eth->h_proto);
        } else if (check_is_dhcp(asr_vif, false, wlan_ntohs(eth->h_proto), &dhcp_type, &d_mac,
            rx_skb->data + sizeof(*eth), (u16) rx_skb->len - sizeof(*eth)) ) {

            if (ether_addr_equal(eth->h_dest, asr_vif->wlan_mac_add.mac_addr)) {    

                dbg(D_ERR, D_UWIFI_CTRL, "%s rx dhcp: dst: %x:%x:%x:%x:%x:%x, src: %x:%x:%x:%x:%x:%x, 0x%x", __func__,
                    eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5],
                    eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5],
                    eth->h_proto);

                #ifdef CFG_STATION_SUPPORT
                if (dhcp_type == 0x5) {
                    asr_set_bit(ASR_DEV_STA_DHCPEND, &asr_vif->dev_flags);

                    #ifndef LWIP
                    asrnet_got_ip_action();
                    #endif
                }
                #endif
            }
        } else if (txlogen) {

            dbg(D_ERR, D_UWIFI_CTRL, "%s rx other data: dst: %x:%x:%x:%x:%x:%x, src: %x:%x:%x:%x:%x:%x, 0x%x,len=%d,data=%p", __func__,
                eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5],
                eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5],
                eth->h_proto, rx_skb->len,rx_skb->data);
        }

        rx_packet.data_len = rx_skb->len;
        rx_packet.data_ptr = rx_skb->data;
        if (ethernet_if && ethernet_if->eth_if_input)
            ethernet_if->eth_if_input(ethernet_if, &rx_packet);
    }

    //dev_kfree_skb_rx(rx_skb);

}
int g_need_free_frame =0;
void asr_amsdu_to_8023s(struct sk_buff *skb, struct sk_buff_head *list)
{
    struct sk_buff *frame = NULL;
    uint16_t ethertype;
    uint8_t *payload;
    const struct ethhdr *eth;
    int32_t remaining;
    uint8_t dst[ETH_ALEN], src[ETH_ALEN];
    eth = (struct ethhdr *) skb->data;

    while (skb != frame)
    {
        uint8_t padding;
        uint16_t len = eth->h_proto;
        //uint32_t subframe_len = sizeof(struct ethhdr) + wlan_htons(len);
        uint32_t subframe_len = sizeof(struct ethhdr) + wlan_ntohs(len);

        remaining = skb->len;
        memcpy(dst, eth->h_dest, ETH_ALEN);
        memcpy(src, eth->h_source, ETH_ALEN);

        padding = (4 - subframe_len) & 0x3;
        /* the last MSDU has no padding */
        if (subframe_len > remaining)
            {
             dbg(D_ERR, D_UWIFI_DATA, "AMSDU:(%d %d)\n",subframe_len,remaining);
            goto purge;
            }

        skb_pull(skb, sizeof(struct ethhdr));
        /* reuse skb for the last subframe */
        if (remaining <= subframe_len + padding)
            frame = skb;
        else
        {
            /*
            * Allocate and reserve two bytes more for payload
            * alignment since sizeof(struct ethhdr) is 14.
            */
            if (subframe_len + 2 > WLAN_AMSDU_RX_LEN) {
                dbg(D_ERR, D_UWIFI_DATA, "%s: invalid len=%d\n", __func__, subframe_len + 2);
                goto purge;
            }
            frame = dev_alloc_skb_rx(subframe_len + 2);
            //if (!frame || !frame->data ||!frame->tail)
            if (!frame)
            {
                dbg(D_ERR, D_UWIFI_DATA, "AMSDU:frame null\n");
                goto purge;
            }
            //dbg(D_ERR, D_UWIFI_DATA, "AMSDU:alloc buffer len=%d\n", subframe_len + 2);
            skb_reserve(frame, sizeof(struct ethhdr) + 2);
            memcpy(skb_put(frame, wlan_ntohs(len)), skb->data, wlan_ntohs(len));
            eth = (struct ethhdr *)skb_pull(skb, wlan_ntohs(len) + padding);
            if (!eth)
            {
                dev_kfree_skb_rx(frame);
                goto purge;
            }else
             g_need_free_frame = 1;
        }

        frame->priority = skb->priority;

        payload = frame->data;
        ethertype = (payload[6] << 8) | payload[7];
        if ((ether_addr_equal(payload, rfc1042_header) &&
            ethertype != ETH_P_AARP && ethertype != ETH_P_IPX) ||
            ether_addr_equal(payload, bridge_tunnel_header))
        {
            /* remove RFC1042 or Bridge-Tunnel
            * encapsulation and replace EtherType */
            //dbg(D_ERR, D_UWIFI_DATA, "AMSDU:(xxx)(5)\n");
            skb_pull(frame, 6);
            memcpy(skb_push(frame, ETH_ALEN), src, ETH_ALEN);
            memcpy(skb_push(frame, ETH_ALEN), dst, ETH_ALEN);
        }
        else
        {
           //dbg(D_ERR, D_UWIFI_DATA, "AMSDU:(xxx)(6)\n");
            memcpy(skb_push(frame, sizeof(uint16_t)), &len, sizeof(uint16_t));
            memcpy(skb_push(frame, ETH_ALEN), src, ETH_ALEN);
            memcpy(skb_push(frame, ETH_ALEN), dst, ETH_ALEN);
        }

        __skb_queue_tail(list, frame);

    }

    return;

    purge:
        dbg(D_ERR, D_UWIFI_DATA, "AMSDU: ..1\n");
        if(list)
            __skb_queue_purge(list);
        dbg(D_ERR, D_UWIFI_DATA, "AMSDU: ..2\n");
        if(skb)
            {
            dbg(D_ERR, D_UWIFI_DATA, "AMSDU: ..2..1..(%x %x)\n",skb->len,skb->priority);
            dev_kfree_skb_rx(skb);
            }
        dbg(D_ERR, D_UWIFI_DATA, "AMSDU: ..3\n");
}
/**
 * asr_rx_data_skb - Process one data frame
 *
 * @asr_hw: main driver data
 * @asr_vif: vif that received the buffer
 * @skb: skb received
 * @rxhdr: HW rx descriptor
 * @return: true if buffer has been forwarded to upper layer
 *
 * If buffer is amsdu , it is first split into a list of skb.
 * Then each skb may be:
 * - forwarded to upper layer
 * - resent on wireless interface
 *
 * When vif is a STA interface, every skb is only forwarded to upper layer.
 * When vif is an AP interface, multicast skb are forwarded and resent, whereas
 * skb for other BSS's STA are only resent.
 */
bool arp_debug_log = 0;
int g_amsdu = 0;
int g_amsdu_cnt = 0;
extern int lalalaen;
int asr_rxdataind_run_cnt;
extern struct net_device_ops uwifi_netdev_ops;
extern bool asr_xmit_opt;
static bool asr_rx_data_skb(struct asr_hw *asr_hw, struct asr_vif *asr_vif,
                             struct sk_buff *skb,  struct hw_rxhdr *rxhdr)
{
    struct sk_buff_head list;
    struct sk_buff *rx_skb;
    bool amsdu = rxhdr->flags_is_amsdu;
    bool resend = false, forward = true;
    struct ethernetif *p_ethernetif = NULL;

    __skb_queue_head_init(&list);

    // save for rx amsdu
    if (amsdu)
    {
        //dbg(D_ERR, D_UWIFI_CTRL, "%s %d %d", __func__, __LINE__, g_amsdu_cnt++);

        #ifndef CFG_AMSDU_TEST

        return false;
        #endif
        if(skb)
            asr_amsdu_to_8023s(skb, &list);

    } else
    {
        __skb_queue_head(&list, skb);
    }


    if ((ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_AP) && !(asr_vif->ap.flags & ASR_AP_ISOLATE))
    {
        const struct ethhdr *eth;
        rx_skb = skb_peek(&list);
        eth = eth_hdr(rx_skb);

        if (is_multicast_ether_addr(eth->h_dest)) {
            /* broadcast pkt need to be forwared to upper layer and resent
               on wireless interface */
            resend = true;
            if(eth->h_proto == wlan_htons(ETH_P_IP))
            {
                uint8_t *ptr;
                uint8_t udp_ofs_iniphd = 0;
                uint8_t dhcp_port_ofs = 0;
                ptr = rx_skb->data;
                udp_ofs_iniphd = ETH_HEADER_SIZE + UDP_OFFSET_INIPHD;
                dhcp_port_ofs = ETH_HEADER_SIZE + ((*(ptr + ETH_HEADER_SIZE)&0xF) * 4) + 1;
                if(*(ptr + udp_ofs_iniphd) == UDP_PROTCOL_INIPHD)
                {
                    if((*(ptr + dhcp_port_ofs) == DHCP_PORT_CLIENT) && (*(ptr + dhcp_port_ofs + 2) == DHCP_PORT_SERVER))
                    {
                        //DHCP-1/3 don't need resend.
                        resend = false;
                    }
                }
            }
        } else
        {
            /* unicast pkt for STA inside the BSS, no need to forward to upper
               layer simply resend on wireless interface */
            if (rxhdr->flags_dst_idx != 0xFF)
            {
                struct asr_sta *sta = &asr_hw->sta_table[rxhdr->flags_dst_idx];
                if (sta->valid && (sta->vlan_idx == asr_vif->vif_index))
                {
                    forward = false;
                    resend = true;
                }
            }
        }
    }
    while (!skb_queue_empty(&list))
    {

        rx_skb = __skb_dequeue(&list);
        if (skb_queue_empty(&list)) {
            g_need_free_frame = 0;
        }

        /* forward pkt to upper layer */
        if (forward)
        {
            /* resend will push the skb data pointer, so forward first, then resend */

            uwifi_dispatch_data(asr_vif,rx_skb);
            if(g_need_free_frame)
            {
                dev_kfree_skb_rx(rx_skb);
            }
            /* Update statistics */
            //asr_vif->net_stats.rx_packets++;
            //asr_vif->net_stats.rx_bytes += rx_skb->len;
        }

#if 1
        /* resend pkt on wireless interface */
        if (resend)//do realy copy here
        {
            int res;
            asr_vif->is_resending = true;

            p_ethernetif = (struct ethernetif *)asr_vif->net_if;
            if (asr_xmit_opt) {
                   struct sk_buff *skb_copy = NULL;
                   if (!skb_queue_empty(&asr_hw->tx_sk_free_list)) {

                       skb_copy = skb_dequeue(&asr_hw->tx_sk_free_list);

                       skb_reserve(skb_copy, PKT_BUF_RESERVE_HEAD);

                       memcpy(skb_copy->data, rx_skb->data, rx_skb->len);
                       //fill in pkt.
                       skb_put(skb_copy,rx_skb->len);

                       if (skb_copy->len > TX_AGG_BUF_UNIT_SIZE)
                           dbg(D_ERR,D_UWIFI_DATA,"%s: skb oversize %d", __func__,skb_copy->len);

                       skb_copy->private[0] = 0xDEADBEAF;
                       skb_copy->private[1] = (uint32_t)((void *)skb_copy->data);
                       skb_copy->private[2] = skb_copy->len;

                       res = uwifi_netdev_ops.ndo_start_xmit(asr_vif,(struct sk_buff*)skb_copy);

                   }else {

                       dbg(D_ERR,D_UWIFI_DATA,"%s: tx_sk_free_list run out", __func__);
                       res = ASR_ERR_BUF;
                   }
            } else {
                #ifdef CFG_STA_AP_COEX
                res = uwifi_xmit_buf(rx_skb->data, rx_skb->len,p_ethernetif);
                #else
                res = uwifi_xmit_buf(rx_skb->data, rx_skb->len);
                #endif
            }

            asr_vif->is_resending = false;

            /* note: buffer is always consummed by dev_queue_xmit */
            if (res == NET_XMIT_DROP)
            {
                //asr_vif->net_stats.rx_dropped++;
                //asr_vif->net_stats.tx_dropped++;
            }
            else if (res != NET_XMIT_SUCCESS)
            {
                dbg(D_ERR,D_UWIFI_DATA,"Failed to re-send buffer to driver (res=%d)",res);
                //asr_vif->net_stats.tx_errors++;
            }
        }
        else
        {
            /* In resend case, buffer freed by TX_CFM. */
        }
#endif

        if(amsdu && forward)
        {
            //dev_kfree_skb_rx(rx_skb);
        }
    }

    return forward;
}

/**
 * asr_rx_mgmt - Process one 802.11 management frame
 *
 * @asr_hw: main driver data
 * @asr_vif: vif that received the buffer
 * @skb: skb received
 * @rxhdr: HW rx descriptor
 *
 * Process the management frame and free the corresponding skb
 */
extern uint32_t current_iftype;
static void asr_rx_mgmt(struct asr_hw *asr_hw, struct asr_vif *asr_vif,
                         struct sk_buff *skb,  struct hw_rxhdr *hw_rxhdr)
{
    struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)skb->data;

    dbg(D_ERR, D_UWIFI_CTRL, "[asr_rx_mgmt]current_iftype=%d, vif_type=%d, fc=0x%X len=%d \n", current_iftype,ASR_VIF_TYPE(asr_vif),mgmt->frame_control, skb->len);

    if(ieee80211_is_beacon(mgmt->frame_control))
    {
        #if (defined CFG_STATION_SUPPORT && defined CFG_SNIFFER_SUPPORT)
        if ((ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_STATION) && sniffer_rx_mgmt_cb) {
            sniffer_rx_mgmt_cb(skb->data, skb->len, phy_freq_to_channel(IEEE80211_BAND_2GHZ, hw_rxhdr->phy_prim20_freq));
        }
        #endif

        /*cfg80211_report_obss_beacon(asr_hw->wiphy, skb->data, skb->len,
                                        hw_rxhdr->phy_prim20_freq,
                                        hw_rxhdr->hwvect.rssi1);*/
    }
#ifdef CFG_STATION_SUPPORT
#ifdef CONFIG_SME
    // when fw enable sme upload, should handle auth/(re)assoc/deauth/deassoc/sa query frame here.
    else if (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_STATION
       && (ieee80211_is_deauth(mgmt->frame_control) || ieee80211_is_disassoc(mgmt->frame_control))) {
        if ( !asr_test_bit(ASR_DEV_STA_CONNECTED, &asr_vif->dev_flags) || !ether_addr_equal(asr_vif->sta.ap->mac_addr, mgmt->bssid)) {
            dbg(D_ERR, D_UWIFI_CTRL,
                 "[asr_rx_mgmt] sta disconnected, ignore rx deauth (%02X:%02X:%02X:%02X:%02X:%02X),drv_flags=0x%X,reason=%d \n",
                 mgmt->bssid[0], mgmt->bssid[1], mgmt->bssid[2], mgmt->bssid[3], mgmt->bssid[4], mgmt->bssid[5],
                 asr_vif->dev_flags, mgmt->u.deauth.reason_code);
        }
        else {

            dbg(D_ERR, D_UWIFI_CTRL,
                 "[asr_rx_mgmt] sae %s protected=%d reason=%d,auth_type=%d,drv_flags=0x%X \n",
                 ieee80211_is_deauth(mgmt->frame_control) ? "deauth" : "disassoc",
                 ieee80211_has_protected(mgmt->frame_control), mgmt->u.deauth.reason_code,
                 asr_vif->sta.auth_type, asr_vif->dev_flags);

            if (asr_vif->sta.auth_type == NL80211_AUTHTYPE_SAE && !asr_test_bit(ASR_DEV_STA_DHCPEND, &asr_vif->dev_flags)) {
                asr_clear_bit(ASR_DEV_STA_CONNECTED, &asr_vif->dev_flags);
                asr_clear_bit(ASR_DEV_STA_DHCPEND, &asr_vif->dev_flags);
                asr_send_sm_disconnect_req(asr_hw, asr_vif, mgmt->u.deauth.reason_code);
            } else {
                cfg80211_rx_mgmt(asr_vif, skb->data, skb->len);
            }
        }
    }
    else if ((ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_STATION) && ieee80211_is_auth(mgmt->frame_control)) {

        if (asr_test_bit(ASR_DEV_STA_AUTH, &asr_vif->dev_flags)) {
            dbg(D_ERR, D_UWIFI_CTRL,"[asr_rx_mgmt]rx auth,type=%d,tran=%d \n"
                , asr_vif->sta.auth_type, mgmt->u.auth.auth_transaction);

            if (mgmt->u.auth.auth_transaction == 2) {
                asr_clear_bit(ASR_DEV_STA_AUTH, &asr_vif->dev_flags);
            }

            cfg80211_rx_mgmt(asr_vif, skb->data, skb->len);
        } else {
            dbg(D_ERR, D_UWIFI_CTRL,"[asr_rx_mgmt]drop auth,drv_flags=0x%X\n", asr_vif->dev_flags);
        }
    }
    else if ((ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_STATION) &&
              ieee80211_is_assoc_resp(mgmt->frame_control)) {
            dbg(D_ERR, D_UWIFI_CTRL,
                "[asr_rx_mgmt]rx assoc_resp ,search bss, bssid(%02X:%02X:%02X:%02X:%02X:%02X)\n",
                mgmt->bssid[0], mgmt->bssid[1], mgmt->bssid[2],
                mgmt->bssid[3], mgmt->bssid[4], mgmt->bssid[5]);

            cfg80211_rx_mgmt(asr_vif, skb->data, skb->len);
    }
    else if ((ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_STATION) &&
             (ieee80211_is_action(mgmt->frame_control) && (mgmt->u.action.category == WLAN_CATEGORY_SA_QUERY))) {
            dbg(D_ERR, D_UWIFI_CTRL,"[asr_rx_mgmt]rx sa query \n");
            cfg80211_rx_mgmt(asr_vif, skb->data, skb->len);
    }
#endif
    else if ((ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_STATION) &&
               (ieee80211_is_action(mgmt->frame_control) && (mgmt->u.action.category == WLAN_CATEGORY_FAST_BSS)))
    {
        struct cfg80211_ft_event_params ft_event;
        ft_event.target_ap = (uint8_t *)&mgmt->u.action + ETH_ALEN + 2;
        ft_event.ies = (uint8_t *)&mgmt->u.action + ETH_ALEN*2 + 2;
        ft_event.ies_len = hw_rxhdr->hwvect.len - (ft_event.ies - (uint8_t *)mgmt);
        ft_event.ric_ies = NULL;
        ft_event.ric_ies_len = 0;

    dbg(D_ERR, D_UWIFI_CTRL,"[asr_rx_mgmt]rx fast bss \n");

        //cfg80211_ft_event(asr_vif->ndev, &ft_event);
    } else if((ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_STATION) &&
               (ieee80211_is_mgmt(mgmt->frame_control)))
    {
        #ifdef CFG_SNIFFER_SUPPORT
        //dbg(D_INF, D_UWIFI_CTRL,"[asr_rx_mgmt]rx sniffer check \n");

        if (NULL != sniffer_rx_mgmt_cb) {
            sniffer_rx_mgmt_cb(skb->data, skb->len, phy_freq_to_channel(IEEE80211_BAND_2GHZ, hw_rxhdr->phy_prim20_freq));
        }
        #endif
    }
#endif
#ifdef CFG_SOFTAP_SUPPORT
    else if(ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_AP)
    {

        dbg(D_ERR, D_UWIFI_CTRL,"[asr_rx_mgmt]ap rx mgmt flags_vif_idx =%d , \n" , hw_rxhdr->flags_vif_idx);
        //dbg(D_INF, D_UWIFI_CTRL,"freq:%u,rssi:%d,skb_len:%u",(unsigned int)hw_rxhdr->phy_prim20_freq,
        //                                                hw_rxhdr->hwvect.rx_vect1.rssi1,(unsigned int)skb->len);


#if NX_STA_AP_COEX
        /* fix #7436 issue. */
        if (current_iftype == STA_AP_MODE ) {
            if (hw_rxhdr->flags_vif_idx == 0xFF && (GetFrameSubType(skb->data) == IEEE80211_STYPE_PROBE_REQ)) {
                if (NULL != sniffer_rx_mgmt_cb)
                    sniffer_rx_mgmt_cb(skb->data, skb->len, phy_freq_to_channel(IEEE80211_BAND_2GHZ, hw_rxhdr->phy_prim20_freq));
            }
        }
 #endif

        if(hw_rxhdr->flags_vif_idx == 0xFF)
        {
            list_for_each_entry(asr_vif, &asr_hw->vifs, list){
                if (asr_vif->up && asr_vif->iftype == NL80211_IFTYPE_AP)
                    cfg80211_rx_mgmt(asr_vif, skb->data, skb->len);
            }
        }
        else
        {
            cfg80211_rx_mgmt(asr_vif, skb->data, skb->len);
        }
    }
#endif

}

/**
 * asr_rx_get_vif - Return pointer to the destination vif
 *
 * @asr_hw: main driver data
 * @vif_idx: vif index present in rx descriptor
 *
 * Select the vif that should receive this frame returns NULL if the destination
 * vif is not active.
 * If no vif is specified, this is probably a mgmt broadcast frame, so select
 * the first active interface
 */
static
struct asr_vif *asr_rx_get_vif(struct asr_hw * asr_hw, int vif_idx)
{
    struct asr_vif *asr_vif = NULL;

    if (vif_idx == 0xFF) {
        list_for_each_entry(asr_vif, &asr_hw->vifs, list) {
            if (asr_vif->up)
                return asr_vif;
        }
        return NULL;
    } else if (vif_idx < NX_VIRT_DEV_MAX) {
        asr_vif = asr_hw->vif_table[vif_idx];
        if (!asr_vif || !asr_vif->up)
            return NULL;
    }

    return asr_vif;
}
#ifdef CFG_DBG
void asr_rx_handle_dbg(struct asr_hw *asr_hw, struct ipc_dbg_msg *msg)
{
    dbg(D_ERR, D_UWIFI_CTRL, "FW log:%s", msg->string);
}

uint8_t asr_dbgind(void *pthis, void *hostid)
{
    struct asr_hw *asr_hw = (struct asr_hw *)pthis;
    struct sk_buff *skb = (struct sk_buff*)hostid;
    struct ipc_dbg_msg *msg;
    uint8_t ret = 0;

    /* Retrieve the message structure */
    msg = (struct ipc_dbg_msg *)skb->data;
    asr_rx_handle_dbg(asr_hw, msg);
    skb_queue_tail(&asr_hw->rx_data_sk_list, skb);
    return ret;
}
#else
uint8_t asr_dbgind(void *pthis, void *hostid)
{
    return 0;
}

#endif

#ifdef CFG_AMSDU_TEST
static int rx_split_skb_realloc(struct asr_hw *asr_hw)
{
    struct sk_buff *skb = NULL;
    int malloc_retry = 0;

    while (malloc_retry < 3) {
        if (asr_rxbuff_alloc(asr_hw, asr_hw->ipc_env->rx_bufsz_split, &skb)) {
            malloc_retry++;
            dbg(D_ERR, D_UWIFI_CTRL,"%s:%d: MEM ALLOC FAILED, malloc_retry=%d\n", __func__, __LINE__, malloc_retry);
            //msleep(1);
        } else {
            // Add the sk buffer structure in the table of rx buffer
            skb_queue_tail(&asr_hw->rx_sk_split_list, skb);

            while (skb_queue_len(&asr_hw->rx_sk_split_list) < asr_hw->ipc_env->rx_bufnb_split) {
                if (asr_rxbuff_alloc(asr_hw, asr_hw->ipc_env->rx_bufsz_split, &skb)) {
                    dbg(D_ERR, D_UWIFI_CTRL,"%s:%d: MEM ALLOC FAILED, rx_sk_split_list len=%d\n", __func__, __LINE__, skb_queue_len(&asr_hw->rx_sk_split_list));
                    break;
                } else {
                    // Add the sk buffer structure in the table of rx buffer
                    skb_queue_tail(&asr_hw->rx_sk_split_list, skb);
                }
            }

            dbg(D_ERR, D_UWIFI_CTRL,"%s: rx_sk_split_list realloc success len=%d\n", __func__, skb_queue_len(&asr_hw->rx_sk_split_list));

            malloc_retry = 0;

            return 0;
        }
    }

    return -1;
}
#endif

// unlike no amsdu , not reuse pskb and need free skb and realloc.
static int rx_data_dispatch(struct asr_hw *asr_hw, struct hw_rxhdr *hw_rxhdr, struct sk_buff *skb,uint16_t rx_status)
{
    struct asr_vif *asr_vif = NULL;

    //dbg(D_ERR, D_UWIFI_CTRL,"%s: len=%d\n", __func__, skb->len);

    /* Check if we need to forward the buffer */
    asr_vif = asr_rx_get_vif(asr_hw, hw_rxhdr->flags_vif_idx);

    if (!asr_vif) {
        dbg(D_ERR, D_UWIFI_CTRL,
            "Frame received but no active vif (%d)",
            hw_rxhdr->flags_vif_idx);
        //dev_kfree_skb_rx(skb);
        return 0;
    }

    /*
        for normal mode:   go  asr_rx_mgmt / asr_rx_data_skb
        for sniffer mode:  call sniffer callback.
    */
    #ifdef CFG_SNIFFER_SUPPORT
    if ((ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_MONITOR)) {
        /* sniffer mode callback */
        if ((NULL != sniffer_rx_cb) && (rx_status & RX_STAT_MONITOR)) {
            /* check if there is callback function to forward the buffer */
            sniffer_rx_cb(skb->data, skb->len, phy_freq_to_channel(IEEE80211_BAND_2GHZ, hw_rxhdr->phy_prim20_freq));
        } else {

            // FIXME: may adapt to wireshark later.
            dbg(D_ERR, D_UWIFI_CTRL,"sniffer mode but no cb (0x%x)", rx_status);
        }

        //dev_kfree_skb_rx(skb);

    } else
    #endif
    if (hw_rxhdr->flags_is_80211_mpdu) {
        //dbg(D_ERR, D_UWIFI_CTRL,"rxmgmt\n");
        asr_rx_mgmt(asr_hw, asr_vif, skb, hw_rxhdr);

        //if(skb->head)
        //    dev_kfree_skb_rx(skb);

    } else {
        if (hw_rxhdr->flags_sta_idx != 0xff) {
            struct asr_sta *sta = &asr_hw->sta_table[hw_rxhdr->flags_sta_idx];

            //asr_rx_statistic(asr_hw, hw_rxhdr,sta);

            if (sta->vlan_idx != asr_vif->vif_index) {
                asr_vif = asr_hw->vif_table[sta->vlan_idx];
                if (!asr_vif) {

                    dbg(D_ERR, D_UWIFI_CTRL,"%s: drop data, asr_vif is NULL, %d,%d \n"
                        , __func__, hw_rxhdr->flags_sta_idx, sta->vlan_idx);

                    //dev_kfree_skb_rx(skb);
                    return 0;
                }
            }

            if (hw_rxhdr->flags_is_4addr
                && !asr_vif->use_4addr) {
                //cfg80211_rx_unexpected_4addr_frame(asr_vif->ndev,sta->mac_addr, GFP_ATOMIC);
            }
        }

        skb->priority = 256 + hw_rxhdr->flags_user_prio;
        if (!asr_rx_data_skb(asr_hw, asr_vif, skb, hw_rxhdr)) {
            dbg(D_ERR, D_UWIFI_CTRL,"%s: drop data, forward is false\n", __func__);
            //dev_kfree_skb_rx(skb);
        }

        //if(skb->head)
        //    dev_kfree_skb_rx(skb);
    }

    return 0;
}

/**
 * asr_rxdataind - Process rx buffer
 *
 * @pthis: Pointer to the object attached to the IPC structure
 *         (points to struct asr_hw is this case)
 * @hostid: Address of the RX descriptor
 *
 * This function is called for each buffer received by the fw
 *
 */
int asr_rxdataind_fn_cnt;

// new rxdataind and deaggr skb free and alloc.
u8 asr_rxdataind(void *pthis, void *hostid)
{
    struct asr_hw *asr_hw = NULL;
    struct sk_buff *skb_head = NULL;
    struct sk_buff *skb = NULL;
    struct host_rx_desc *pre_host_rx_desc = NULL;
    struct host_rx_desc *host_rx_desc = NULL;
    struct hw_rxhdr *hw_rxhdr = NULL;
    int msdu_offset = 0;
    int frame_len = 0;
    int num = 0;
    u16 totol_frmlen = 0;
    u16 seq_num = 0;
    u16 fn_num = 0;
    u16 remain_len = 0;
    static volatile bool amsdu_flag = false;
    static volatile u16 pre_msdu_offset = 0;
    uint16_t  rx_status = 0;

    if (!pthis || !hostid) {
        return -EFAULT;
    }

    asr_hw = (struct asr_hw *)pthis;
    skb_head = (struct sk_buff *)hostid;

    if (!skb_head->data) {
        return -EFAULT;
    }

    host_rx_desc = (struct host_rx_desc *)skb_head->data;
    hw_rxhdr     = (struct hw_rxhdr *)&host_rx_desc->frmlen;
    msdu_offset  = host_rx_desc->pld_offset;
    frame_len    = host_rx_desc->frmlen;
    totol_frmlen = host_rx_desc->totol_frmlen;
    seq_num      = host_rx_desc->seq_num;
    fn_num       = host_rx_desc->fn_num;
    num          = host_rx_desc->num;
    remain_len   = totol_frmlen;

    asr_rxdataind_run_cnt++;
    if (fn_num)
        asr_rxdataind_fn_cnt++;

#ifndef SDIO_DEAGGR
    while (num--) {
#endif

#ifdef CFG_AMSDU_TEST
rx_sk_tryagain:
        if (amsdu_flag) {
            amsdu_flag = false;

            skb = skb_dequeue(&asr_hw->rx_sk_split_list);
            if (!skb){
                dbg(D_ERR, D_UWIFI_CTRL,"%s: rx_sk_split_list empty\n", __func__);
                goto check_next;
            }
            pre_host_rx_desc = (struct host_rx_desc *)(skb->data - pre_msdu_offset);

            // if current fn_num is 0, last saved amsdu should drop, current skb continue rx.
            if (fn_num == 0) {
                hw_rxhdr = (struct hw_rxhdr *)&host_rx_desc->frmlen;

                dbg(D_ERR, D_UWIFI_CTRL,"amsdu drop %d: %d %d %d %d %d(%d %d %d %d %d)\n",
                    __LINE__, msdu_offset,frame_len, totol_frmlen,seq_num, fn_num,
                    pre_host_rx_desc->totol_frmlen,pre_host_rx_desc->pld_offset,
                    pre_host_rx_desc->seq_num, pre_host_rx_desc->fn_num, skb->len);

                // free saved amsdu skb.
                skb_reinit(skb);
                skb_queue_tail(&asr_hw->rx_sk_split_list, skb);

                goto rx_sk_tryagain;
            }

            hw_rxhdr = (struct hw_rxhdr *)&pre_host_rx_desc->frmlen;
            rx_status = pre_host_rx_desc->rx_status;

            if (totol_frmlen == pre_host_rx_desc->totol_frmlen
                && seq_num == pre_host_rx_desc->seq_num &&
                frame_len > 0 && remain_len >= frame_len &&
                (skb->len + frame_len + pre_host_rx_desc->pld_offset < WLAN_AMSDU_RX_LEN)) {
                remain_len = totol_frmlen - skb->len;
            } else {
                dbg(D_ERR, D_UWIFI_CTRL,"amsdu drop2 %d: %d %d %d %d %d(%d %d %d %d %d)\n",__LINE__,msdu_offset,frame_len,totol_frmlen,seq_num,fn_num,
                    pre_host_rx_desc->totol_frmlen,pre_host_rx_desc->pld_offset,pre_host_rx_desc->seq_num,pre_host_rx_desc->fn_num,skb->len);

                skb_reinit(skb);
                skb_queue_tail(&asr_hw->rx_sk_split_list, skb);
                goto check_next;
            }

            memcpy(skb->data + skb->len, (uint8_t *) host_rx_desc + msdu_offset, frame_len);
            skb_put(skb, frame_len);
            remain_len -= frame_len;

            if (remain_len) {
                skb_queue_head(&asr_hw->rx_sk_split_list, skb);
                amsdu_flag = true;
                goto check_next;
            }

            rx_data_dispatch(asr_hw, hw_rxhdr, skb,rx_status);

            //dev_kfree_skb_rx(skb);
            skb_reinit(skb);
            skb_queue_tail(&asr_hw->rx_sk_split_list, skb);

        }else if (totol_frmlen > frame_len) {
            // first part of amsdu .
            if (frame_len <= 0 || totol_frmlen + msdu_offset >= WLAN_AMSDU_RX_LEN ||
                frame_len + msdu_offset >= WLAN_AMSDU_RX_LEN || fn_num != 0) {

                dbg(D_ERR, D_UWIFI_CTRL, "amsdu drop %d: %d %d %d %d %d\n",
                    __LINE__, msdu_offset, frame_len, totol_frmlen, seq_num, fn_num);

                num = 0;
                goto check_next;
            }

            skb = skb_dequeue(&asr_hw->rx_sk_split_list);
            if (!skb){
                dbg(D_ERR, D_UWIFI_CTRL,"%s: rx_sk_split_list empty\n", __func__);
                goto check_next;
            }

            memcpy(skb->data, host_rx_desc, msdu_offset + frame_len);
            skb_reserve(skb, msdu_offset);
            skb_put(skb, frame_len);
            pre_msdu_offset = msdu_offset;
            pre_host_rx_desc = host_rx_desc;
            remain_len = totol_frmlen - frame_len;

            //dbg(D_ERR, D_UWIFI_CTRL,"amsdu1: %d %d %d %d %d\n",msdu_offset,frame_len,totol_frmlen,seq_num,fn_num);

            skb_queue_head(&asr_hw->rx_sk_split_list, skb);
            amsdu_flag = true;
        }
        else
#endif  // CFG_AMSDU_TEST
        {
            // no-amsdu case.
            if (frame_len <= 0 || frame_len >= WLAN_AMSDU_RX_LEN || totol_frmlen >= WLAN_AMSDU_RX_LEN
                || msdu_offset + frame_len >= WLAN_AMSDU_RX_LEN || (totol_frmlen > frame_len) ) {
                dbg(D_ERR, D_UWIFI_CTRL, "amsdu drop %d, (%p,%d,%d,%d,%d,%d)\n",
                    __LINE__, host_rx_desc, msdu_offset, frame_len, totol_frmlen, seq_num, fn_num);

                num = 0;
                goto check_next;
            }

            skb = asr_hw->pskb;
            memset(skb, 0, sizeof(struct sk_buff));
            skb->tail = (uint8_t *)host_rx_desc;
            skb->head = (uint8_t *)host_rx_desc;
            skb->end = skb->head + host_rx_desc->sdio_rx_len;
            skb->data = (uint8_t *)host_rx_desc;

            rx_status = host_rx_desc->rx_status;

            skb_reserve(skb, msdu_offset);
            skb_put(skb, frame_len);

            pre_msdu_offset = 0;

            // here skb is ethier single msdu skb or amsdu skb.
            rx_data_dispatch(asr_hw, hw_rxhdr, skb,rx_status);
        }

check_next:

#ifdef SDIO_DEAGGR
        skb_reinit(skb_head);
        skb_queue_tail(&asr_hw->rx_sk_sdio_deaggr_list, skb_head);
#else

        if (num) {
            // next rx pkt
            host_rx_desc = (struct host_rx_desc *)((uint8_t *) host_rx_desc + host_rx_desc->sdio_rx_len);

            if ((unsigned int)host_rx_desc - (unsigned int)skb_head->data + msdu_offset> IPC_RXBUF_SIZE) {
                dbg(D_ERR, D_UWIFI_CTRL,"%s,%d: rx data over flow,%08X,%08X\n"
                    , __func__, __LINE__, (unsigned int)skb_head->data, (unsigned int)host_rx_desc);
                break;
            }

            hw_rxhdr = (struct hw_rxhdr *)&host_rx_desc->frmlen;
            msdu_offset = host_rx_desc->pld_offset;
            frame_len = host_rx_desc->frmlen;
            totol_frmlen = host_rx_desc->totol_frmlen;
            seq_num = host_rx_desc->seq_num;
            fn_num = host_rx_desc->fn_num;

            if ((unsigned int)host_rx_desc - (unsigned int)skb_head->data + msdu_offset + frame_len > IPC_RXBUF_SIZE) {
                dbg(D_ERR, D_UWIFI_CTRL,"%s,%d: rx data over flow,%08X,%08X,%d,%d,%d,%d\n"
                    , __func__, __LINE__, (unsigned int)skb_head->data, (unsigned int)host_rx_desc, msdu_offset, frame_len
                    ,((struct host_rx_desc *)skb_head->data)->num,num);
                break;
            }
        }

    }

    // Add the sk buffer structure in the table of rx buffer
    skb_queue_tail(&asr_hw->rx_data_sk_list, skb_head);
#endif

    return 0;
}


#if 0


uint8_t asr_rxdataind(void *pthis, void *hostid)
{
    struct asr_hw *asr_hw = (struct asr_hw *)pthis;
    struct sk_buff *skb_head = (struct sk_buff *)hostid;
    struct sk_buff *skb = NULL;
    struct host_rx_desc *pre_host_rx_desc = NULL;
    struct host_rx_desc *host_rx_desc = (struct host_rx_desc *)skb_head->data;
    struct hw_rxhdr *hw_rxhdr = (struct hw_rxhdr *)&host_rx_desc->frmlen;
    int msdu_offset = host_rx_desc->pld_offset;
    int frame_len = host_rx_desc->frmlen;
    int num = host_rx_desc->num;
    uint16_t totol_frmlen = host_rx_desc->totol_frmlen;
    uint16_t seq_num = host_rx_desc->seq_num;
    uint16_t fn_num = host_rx_desc->fn_num;
    uint16_t remain_len = totol_frmlen;
    static volatile bool amsdu_flag = false;
    static volatile uint16_t pre_msdu_offset = 0;
    uint16_t  rx_status = 0;

    asr_rxdataind_run_cnt++;

    while (num--) {

rx_sk_tryagain:
#ifdef CFG_AMSDU_TEST
        if (amsdu_flag) {
            skb = skb_dequeue(&asr_hw->rx_sk_split_list);
            if (!skb){
                dbg(D_ERR, D_UWIFI_CTRL,"%s: rx_sk_split_list empty\n", __func__);
                break;
            }

            amsdu_flag = false;
            pre_host_rx_desc = (struct host_rx_desc *)(skb->data - pre_msdu_offset);

            if (fn_num == 0) {
                hw_rxhdr = (struct hw_rxhdr *)&host_rx_desc->frmlen;

                dbg(D_ERR, D_UWIFI_CTRL,"amsdu drop1 %d: %d %d %d %d %d(%d %d %d %d %d)\n"
                    ,__LINE__,msdu_offset,frame_len,totol_frmlen,seq_num,fn_num,
                    pre_host_rx_desc->totol_frmlen,pre_host_rx_desc->pld_offset
                    ,pre_host_rx_desc->seq_num,pre_host_rx_desc->fn_num,skb->len);

                //dev_kfree_skb_rx(skb);
                skb_reinit(skb);
                skb_queue_tail(&asr_hw->rx_sk_split_list, skb);
                goto rx_sk_tryagain;
            }

            hw_rxhdr = (struct hw_rxhdr *)&pre_host_rx_desc->frmlen;

            rx_status = pre_host_rx_desc->rx_status;

            //dbg(D_ERR, D_UWIFI_CTRL,"amsdu3: %d %d %d %d %d(%d %d %d %d)\n",msdu_offset,frame_len,totol_frmlen,seq_num,fn_num,
            //    pre_host_rx_desc->totol_frmlen,pre_host_rx_desc->seq_num,pre_host_rx_desc->fn_num,skb->len);

            if (totol_frmlen == pre_host_rx_desc->totol_frmlen
                && seq_num == pre_host_rx_desc->seq_num) {
                remain_len = totol_frmlen - skb->len;
            } else {
                dbg(D_ERR, D_UWIFI_CTRL,"amsdu drop2 %d: %d %d %d %d %d(%d %d %d %d %d)\n",__LINE__,msdu_offset,frame_len,totol_frmlen,seq_num,fn_num,
                    pre_host_rx_desc->totol_frmlen,pre_host_rx_desc->pld_offset,pre_host_rx_desc->seq_num,pre_host_rx_desc->fn_num,skb->len);
                //dev_kfree_skb_rx(skb);

                skb_reinit(skb);
                skb_queue_tail(&asr_hw->rx_sk_split_list, skb);
                goto check_next;
            }

            memcpy(skb->data + skb->len, (uint8_t *) host_rx_desc + msdu_offset, frame_len);
            skb_put(skb, frame_len);
            remain_len -= frame_len;

            if (remain_len) {
                skb_queue_head(&asr_hw->rx_sk_split_list, skb);
                amsdu_flag = true;
                goto check_next;
            }

            rx_data_dispatch(asr_hw, hw_rxhdr, skb,rx_status);

            //dev_kfree_skb_rx(skb);
            skb_reinit(skb);
            skb_queue_tail(&asr_hw->rx_sk_split_list, skb);

        } else if (totol_frmlen > frame_len) {
            if (frame_len <= 0 || totol_frmlen + msdu_offset >= WLAN_AMSDU_RX_LEN || frame_len + msdu_offset >= WLAN_AMSDU_RX_LEN
                || fn_num != 0) {

                dbg(D_ERR, D_UWIFI_CTRL,"amsdu drop5 %d: %d %d %d %d %d\n",__LINE__,msdu_offset,frame_len,totol_frmlen,seq_num,fn_num);
                //dev_kfree_skb_rx(skb);
                skb_reinit(skb);
                skb_queue_tail(&asr_hw->rx_sk_split_list, skb);
                goto check_next;
            }

            skb = skb_dequeue(&asr_hw->rx_sk_split_list);
            if (!skb){
                dbg(D_ERR, D_UWIFI_CTRL,"%s: rx_sk_split_list empty\n", __func__);
                break;
            }

            memcpy(skb->data, host_rx_desc, msdu_offset + frame_len);
            skb_reserve(skb, msdu_offset);
            skb_put(skb, frame_len);
            pre_msdu_offset = msdu_offset;
            remain_len = totol_frmlen;
            remain_len -= frame_len;
            pre_host_rx_desc = host_rx_desc;

            //dbg(D_ERR, D_UWIFI_CTRL,"amsdu1: %d %d %d %d %d\n",msdu_offset,frame_len,totol_frmlen,seq_num,fn_num);

            skb_queue_head(&asr_hw->rx_sk_split_list, skb);
            amsdu_flag = true;
            //goto check_next;

        } else
#endif
        {
            if (frame_len <= 0 || frame_len >= WLAN_AMSDU_RX_LEN || totol_frmlen >= WLAN_AMSDU_RX_LEN
                || msdu_offset + frame_len >= WLAN_AMSDU_RX_LEN) {
                dbg(D_ERR, D_UWIFI_CTRL,"amsdu drop8 %d, (%p,%d,%d,%d,%d,%d,%d)\n",
                    __LINE__, host_rx_desc, msdu_offset, frame_len, num, totol_frmlen, seq_num, fn_num);

                goto check_next;
            }

            skb = asr_hw->pskb;
            memset(skb, 0, sizeof(struct sk_buff));
            skb->tail = (uint8_t *)host_rx_desc;
            skb->head = (uint8_t *)host_rx_desc;
            skb->end = skb->head + host_rx_desc->sdio_rx_len;
            skb->data = (uint8_t *)host_rx_desc;

            rx_status = host_rx_desc->rx_status;

            skb_reserve(skb, msdu_offset);
            skb_put(skb, frame_len);

            pre_msdu_offset = 0;

            rx_data_dispatch(asr_hw, hw_rxhdr, skb,rx_status);
            //goto check_next;
        }

check_next:
        if (num) {
            // next rx pkt
            host_rx_desc = (struct host_rx_desc *)((uint8_t *) host_rx_desc + host_rx_desc->sdio_rx_len);

            if ((unsigned int)host_rx_desc - (unsigned int)skb_head->data + msdu_offset> IPC_RXBUF_SIZE) {
                dbg(D_ERR, D_UWIFI_CTRL,"%s,%d: rx data over flow,%08X,%08X\n"
                    , __func__, __LINE__, (unsigned int)skb_head->data, (unsigned int)host_rx_desc);
                break;
            }

            hw_rxhdr = (struct hw_rxhdr *)&host_rx_desc->frmlen;
            msdu_offset = host_rx_desc->pld_offset;
            frame_len = host_rx_desc->frmlen;
            totol_frmlen = host_rx_desc->totol_frmlen;
            seq_num = host_rx_desc->seq_num;
            fn_num = host_rx_desc->fn_num;

            if ((unsigned int)host_rx_desc - (unsigned int)skb_head->data + msdu_offset + frame_len > IPC_RXBUF_SIZE) {
                dbg(D_ERR, D_UWIFI_CTRL,"%s,%d: rx data over flow,%08X,%08X,%d,%d\n"
                    , __func__, __LINE__, (unsigned int)skb_head->data, (unsigned int)host_rx_desc, msdu_offset, frame_len);
                break;
            }
        }

    }

    // Add the sk buffer structure in the table of rx buffer
    skb_queue_tail(&asr_hw->rx_data_sk_list, skb_head);

    return 0;
}


#endif   // SDIO_DEAGGR
