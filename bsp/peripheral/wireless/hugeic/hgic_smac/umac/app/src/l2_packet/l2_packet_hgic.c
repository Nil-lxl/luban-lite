/*
 * WPA Supplicant - Layer2 packet handling with Linux packet sockets
 * Copyright (c) 2003-2015, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/list.h>
#include <linux/rcu.h>
#include <linux/rculist.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/msg_queue.h>
#include <linux/handlers.h>
#include <net/netlink.h>

#include "utils/includes.h"
#include "utils/common.h"
#include "utils/eloop.h"
#include "l2_packet.h"
#include "utils/os.h"

struct l2_packet_data {
    int fd;
    char ifname[IFNAMSIZ + 1];
    u8 own_addr[ETH_ALEN];
    void (*rx_callback)(void *ctx, const u8 *src_addr, const u8 *buf, size_t len);
    void *rx_callback_ctx;
};

int l2_packet_get_own_addr(struct l2_packet_data *l2, u8 *addr)
{
    memcpy(addr, l2->own_addr, ETH_ALEN);
    return 0;
}


int l2_packet_send(struct l2_packet_data *l2, const u8 *dst_addr, u16 proto,
                   const u8 *buf, size_t len)
{
    struct net_device *ndev = net_device_get_by_name(l2->ifname);
    return netif_ether_send(ndev, dst_addr, proto, buf, len);
}


static void l2_packet_receive(int sock, void *eloop_ctx, void *sock_ctx)
{
    struct l2_packet_data *l2 = eloop_ctx;
    struct nl_msg *skb = NULL;
    struct ethhdr  *hdr = NULL;

    ASSERT(l2 && l2->rx_callback);
    wpa_printf(MSG_MSGDUMP, "l2_packet_receive msg");
    skb = nl_socket_recv(l2->fd, 100);
    if (skb == NULL) {
        wpa_printf(MSG_ERROR, "rx eapol failed!\r\n");
        return;
    }

    wpa_printf(MSG_DEBUG, "rx eapol data, len=%d\r\n", skb->len);
    hdr = (struct ethhdr *)skb->data;
    skb_pull(skb, sizeof(struct ethhdr));
    l2->rx_callback(l2->rx_callback_ctx, hdr->h_source, skb->data, skb->len);
    nlmsg_free(skb);
}

struct l2_packet_data *l2_packet_init(
    const char *ifname, const u8 *own_addr, unsigned short protocol,
    void (*rx_callback)(void *ctx, const u8 *src_addr,
                        const u8 *buf, size_t len),
    void *rx_callback_ctx, int l2_hdr)
{
    struct l2_packet_data *l2;
    int ret = 0;

    l2 = (struct l2_packet_data *)os_zalloc(sizeof(struct l2_packet_data));
    if (l2 == NULL) {
        return NULL;
    }

    l2->fd = (int)nl80211_handle_alloc((char *)ifname);
    if (l2->fd == 0) {
        wpa_printf(MSG_ERROR, "l2_packet_init failed: %s", "error");
        FREE(l2);
        return NULL;
    }

    ret = nl_get_multicast_id("eapol");
    if (ret >= 0) {
        if (l2->fd == 0 || l2->fd == -1) {
            PRINTF("%s,%d: invaild handle:%d\r\n", __FUNCTION__, __LINE__, l2->fd);
            FREE(l2);
            return NULL;
        }
        ret = nl_socket_add_membership((struct nl_sock *)l2->fd, ret);
    }
    if (ret < 0) {
        wpa_printf(MSG_DEBUG, "register EAPOL socket failed!\r\n");
        nl_socket_free((struct nl_sock *)l2->fd);
        FREE(l2);
        return NULL;
    }

    l2->rx_callback = rx_callback;
    l2->rx_callback_ctx = rx_callback_ctx;
    os_strlcpy(l2->ifname, ifname, sizeof(l2->ifname));
    linux_get_ifhwaddr(0, ifname, l2->own_addr);
    eloop_register_read_sock(l2->fd, l2_packet_receive, l2, NULL);
    return l2;
}

void l2_packet_deinit(struct l2_packet_data *l2)
{
    if (l2 == NULL)
    { 
        return;
    }

    PRINTF("%s:Deinit l2 socket:%p\n",__FUNCTION__,l2->fd);

    if (l2->fd != 0 && l2->fd != -1) {
        eloop_unregister_read_sock(l2->fd);
        nl_socket_free((struct nl_sock *)l2->fd);
    }
    os_free(l2);
}


int l2_packet_get_ip_addr(struct l2_packet_data *l2, char *buf, size_t len)
{
    return 0;
}

void l2_packet_notify_auth_start(struct l2_packet_data *l2)
{
}

int l2_packet_set_packet_filter(struct l2_packet_data *l2,
                                enum l2_packet_filter_type type)
{
    return 0;
}
