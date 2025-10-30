#ifndef _LINUX_NETDEVICE_H
#define _LINUX_NETDEVICE_H

#include <linux/types.h>
//#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/if_ether.h>
#include <linux/ethtool.h>
//#include <net/cfg80211.h>
#include "hgic.h"
#ifndef MAC2STR
#define MAC2STR(a) (a)[0]&0xff, (a)[1]&0xff, (a)[2]&0xff, (a)[3]&0xff, (a)[4]&0xff, (a)[5]&0xff
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define COMPACT_MACSTR "%02x%02x%02x%02x%02x%02x"
#endif

#define MAX_ADDR_LEN    32      /* Largest hardware address length */
typedef u64 netdev_features_t;
enum {
    NETIF_F_SG_BIT,         /* Scatter/gather IO. */
    NETIF_F_IP_CSUM_BIT,        /* Can checksum TCP/UDP over IPv4. */
    __UNUSED_NETIF_F_1,
    NETIF_F_HW_CSUM_BIT,        /* Can checksum all the packets. */
    NETIF_F_IPV6_CSUM_BIT,      /* Can checksum TCP/UDP over IPV6 */
    NETIF_F_HIGHDMA_BIT,        /* Can DMA to high memory. */
    NETIF_F_FRAGLIST_BIT,       /* Scatter/gather IO. */
    NETIF_F_HW_VLAN_CTAG_TX_BIT,    /* Transmit VLAN CTAG HW acceleration */
    NETIF_F_HW_VLAN_CTAG_RX_BIT,    /* Receive VLAN CTAG HW acceleration */
    NETIF_F_HW_VLAN_CTAG_FILTER_BIT,/* Receive filtering on VLAN CTAGs */
    NETIF_F_VLAN_CHALLENGED_BIT,    /* Device cannot handle VLAN packets */
    NETIF_F_GSO_BIT,        /* Enable software GSO. */
    NETIF_F_LLTX_BIT,       /* LockLess TX - deprecated. Please */
    /* do not use LLTX in new drivers */
    NETIF_F_NETNS_LOCAL_BIT,    /* Does not change network namespaces */
    NETIF_F_GRO_BIT,        /* Generic receive offload */
    NETIF_F_LRO_BIT,        /* large receive offload */

    /**/NETIF_F_GSO_SHIFT,      /* keep the order of SKB_GSO_* bits */
    NETIF_F_TSO_BIT         /* ... TCPv4 segmentation */
    = NETIF_F_GSO_SHIFT,
    NETIF_F_UFO_BIT,        /* ... UDPv4 fragmentation */
    NETIF_F_GSO_ROBUST_BIT,     /* ... ->SKB_GSO_DODGY */
    NETIF_F_TSO_ECN_BIT,        /* ... TCP ECN support */
    NETIF_F_TSO6_BIT,       /* ... TCPv6 segmentation */
    NETIF_F_FSO_BIT,        /* ... FCoE segmentation */
    NETIF_F_GSO_GRE_BIT,        /* ... GRE with TSO */
    NETIF_F_GSO_UDP_TUNNEL_BIT, /* ... UDP TUNNEL with TSO */
    /**/NETIF_F_GSO_LAST =      /* last bit, see GSO_MASK */
        NETIF_F_GSO_UDP_TUNNEL_BIT,

    NETIF_F_FCOE_CRC_BIT,       /* FCoE CRC32 */
    NETIF_F_SCTP_CSUM_BIT,      /* SCTP checksum offload */
    NETIF_F_FCOE_MTU_BIT,       /* Supports max FCoE MTU, 2158 bytes*/
    NETIF_F_NTUPLE_BIT,     /* N-tuple filters supported */
    NETIF_F_RXHASH_BIT,     /* Receive hashing offload */
    NETIF_F_RXCSUM_BIT,     /* Receive checksumming offload */
    NETIF_F_NOCACHE_COPY_BIT,   /* Use no-cache copyfromuser */
    NETIF_F_LOOPBACK_BIT,       /* Enable loopback */
    NETIF_F_RXFCS_BIT,      /* Append FCS to skb pkt data */
    NETIF_F_RXALL_BIT,      /* Receive errored frames too */
    NETIF_F_HW_VLAN_STAG_TX_BIT,    /* Transmit VLAN STAG HW acceleration */
    NETIF_F_HW_VLAN_STAG_RX_BIT,    /* Receive VLAN STAG HW acceleration */
    NETIF_F_HW_VLAN_STAG_FILTER_BIT,/* Receive filtering on VLAN STAGs */

    /*
     * Add your fresh new feature above and remember to update
     * netdev_features_strings[] in net/core/ethtool.c and maybe
     * some feature mask #defines below. Please also describe it
     * in Documentation/networking/netdev-features.txt.
     */

    /**/NETDEV_FEATURE_COUNT
};

/* copy'n'paste compression ;) */
#define __NETIF_F_BIT(bit)  ((netdev_features_t)1 << (bit))
#define __NETIF_F(name)     __NETIF_F_BIT(NETIF_F_##name##_BIT)

#define NETIF_F_FCOE_CRC    __NETIF_F(FCOE_CRC)
#define NETIF_F_FCOE_MTU    __NETIF_F(FCOE_MTU)
#define NETIF_F_FRAGLIST    __NETIF_F(FRAGLIST)
#define NETIF_F_FSO     __NETIF_F(FSO)
#define NETIF_F_GRO     __NETIF_F(GRO)
#define NETIF_F_GSO     __NETIF_F(GSO)
#define NETIF_F_GSO_ROBUST  __NETIF_F(GSO_ROBUST)
#define NETIF_F_HIGHDMA     __NETIF_F(HIGHDMA)
#define NETIF_F_HW_CSUM     __NETIF_F(HW_CSUM)
#define NETIF_F_HW_VLAN_CTAG_FILTER __NETIF_F(HW_VLAN_CTAG_FILTER)
#define NETIF_F_HW_VLAN_CTAG_RX __NETIF_F(HW_VLAN_CTAG_RX)
#define NETIF_F_HW_VLAN_CTAG_TX __NETIF_F(HW_VLAN_CTAG_TX)
#define NETIF_F_IP_CSUM     __NETIF_F(IP_CSUM)
#define NETIF_F_IPV6_CSUM   __NETIF_F(IPV6_CSUM)
#define NETIF_F_LLTX        __NETIF_F(LLTX)
#define NETIF_F_LOOPBACK    __NETIF_F(LOOPBACK)
#define NETIF_F_LRO     __NETIF_F(LRO)
#define NETIF_F_NETNS_LOCAL __NETIF_F(NETNS_LOCAL)
#define NETIF_F_NOCACHE_COPY    __NETIF_F(NOCACHE_COPY)
#define NETIF_F_NTUPLE      __NETIF_F(NTUPLE)
#define NETIF_F_RXCSUM      __NETIF_F(RXCSUM)
#define NETIF_F_RXHASH      __NETIF_F(RXHASH)
#define NETIF_F_SCTP_CSUM   __NETIF_F(SCTP_CSUM)
#define NETIF_F_SG      __NETIF_F(SG)
#define NETIF_F_TSO6        __NETIF_F(TSO6)
#define NETIF_F_TSO_ECN     __NETIF_F(TSO_ECN)
#define NETIF_F_TSO     __NETIF_F(TSO)
#define NETIF_F_UFO     __NETIF_F(UFO)
#define NETIF_F_VLAN_CHALLENGED __NETIF_F(VLAN_CHALLENGED)
#define NETIF_F_RXFCS       __NETIF_F(RXFCS)
#define NETIF_F_RXALL       __NETIF_F(RXALL)
#define NETIF_F_GSO_GRE     __NETIF_F(GSO_GRE)
#define NETIF_F_GSO_GRE_CSUM    __NETIF_F(GSO_GRE_CSUM)
#define NETIF_F_GSO_IPIP    __NETIF_F(GSO_IPIP)
#define NETIF_F_GSO_SIT     __NETIF_F(GSO_SIT)
#define NETIF_F_GSO_UDP_TUNNEL  __NETIF_F(GSO_UDP_TUNNEL)
#define NETIF_F_GSO_UDP_TUNNEL_CSUM __NETIF_F(GSO_UDP_TUNNEL_CSUM)
#define NETIF_F_GSO_TUNNEL_REMCSUM __NETIF_F(GSO_TUNNEL_REMCSUM)
#define NETIF_F_HW_VLAN_STAG_FILTER __NETIF_F(HW_VLAN_STAG_FILTER)
#define NETIF_F_HW_VLAN_STAG_RX __NETIF_F(HW_VLAN_STAG_RX)
#define NETIF_F_HW_VLAN_STAG_TX __NETIF_F(HW_VLAN_STAG_TX)
#define NETIF_F_HW_L2FW_DOFFLOAD    __NETIF_F(HW_L2FW_DOFFLOAD)
#define NETIF_F_BUSY_POLL   __NETIF_F(BUSY_POLL)
#define NETIF_F_HW_SWITCH_OFFLOAD   __NETIF_F(HW_SWITCH_OFFLOAD)
#define NETIF_F_GSO_SOFTWARE	0

//typedef void (*netdev_event_hdl)(char *ifname, int evt_id, char *args, int len);
typedef void (*netdev_xmit_hdl)(struct sk_buff *skb);
typedef void (*netdev_recv_hdl)(struct sk_buff *skb);

struct net_device_stats {
    unsigned long   rx_packets;
    unsigned long   tx_packets;
    unsigned long   rx_bytes;
    unsigned long   tx_bytes;
    unsigned long   rx_errors;
    unsigned long   tx_errors;
    unsigned long   rx_dropped;
    unsigned long   tx_dropped;
    unsigned long   multicast;
    unsigned long   collisions;
    unsigned long   rx_length_errors;
    unsigned long   rx_over_errors;
    unsigned long   rx_crc_errors;
    unsigned long   rx_frame_errors;
    unsigned long   rx_fifo_errors;
    unsigned long   rx_missed_errors;
    unsigned long   tx_aborted_errors;
    unsigned long   tx_carrier_errors;
    unsigned long   tx_fifo_errors;
    unsigned long   tx_heartbeat_errors;
    unsigned long   tx_window_errors;
    unsigned long   rx_compressed;
    unsigned long   tx_compressed;
};

struct net_device;

struct net_device_ops {
    void (*ndo_uninit)(struct net_device *dev);
    int (*ndo_open)(struct net_device *dev);
    int (*ndo_stop)(struct net_device *dev);
    netdev_tx_t (*ndo_start_xmit)(struct sk_buff *skb, struct net_device *dev);
    u16(*ndo_select_queue)(struct net_device *dev, struct sk_buff *skb);
    int (*ndo_set_mac_address)(struct net_device *dev, void *addr);
    int (*ndo_change_mtu)(struct net_device *dev, int new_mtu);
    void (*ndo_set_rx_mode)(struct net_device *dev);
    int (*ndo_rx)(struct sk_buff *skb, struct net_device *dev);
    int (*ndo_do_ioctl)(struct net_device *dev, u32 cmd, u32 param1, u32 param2);
    struct net_device_stats* (*ndo_get_stats)(struct net_device *dev);
};


struct net_device {
    struct list_head list;
    char name[IFNAMSIZ];
    u8   ifindex;
    u8   type;
    u32  flags;
    u32  trans_start;
    const struct net_device_ops *netdev_ops;
    struct net_device_stats stats;
    unsigned char        dev_addr[ETH_ALEN];
    unsigned char        perm_addr[MAX_ADDR_LEN]; /* permanent hw address */
    struct wireless_dev  *ieee80211_ptr;
    struct ieee80211_vif *vif;
    unsigned short       needed_headroom;
    unsigned short       needed_tailroom;
    u32 assoc_state;

    u8    fwifidx;
    void *ctrl; //hgic_fwctrl
    void *priv;
    void *wpas;
    void *hostapd;
    hgic_event_cb event;
    unsigned int magic_num;
};

struct netdev_scatter_data {
    void   *addr;
    u32  size;
};

#define NET_DEVICE_MAGIC_NUM (0x6F6F6F6F)

/* netdevice notifier chain. Please remember to update the rtnetlink
 * notification exclusion list in rtnetlink_event() when adding new
 * types.
 */
#define NETDEV_UP   0x0001  /* For now you can't veto a device up/down */
#define NETDEV_DOWN 0x0002
#define NETDEV_REBOOT   0x0003  /* Tell a protocol stack a network interface
                   detected a hardware crash and restarted
                   - we can use this eg to kick tcp sessions
                   once done */
#define NETDEV_CHANGE   0x0004  /* Notify device state change */
#define NETDEV_REGISTER 0x0005
#define NETDEV_UNREGISTER   0x0006
#define NETDEV_CHANGEMTU    0x0007
#define NETDEV_CHANGEADDR   0x0008
#define NETDEV_GOING_DOWN   0x0009
#define NETDEV_CHANGENAME   0x000A
#define NETDEV_FEAT_CHANGE  0x000B
#define NETDEV_BONDING_FAILOVER 0x000C
#define NETDEV_PRE_UP       0x000D
#define NETDEV_PRE_TYPE_CHANGE  0x000E
#define NETDEV_POST_TYPE_CHANGE 0x000F
#define NETDEV_POST_INIT    0x0010
#define NETDEV_UNREGISTER_FINAL 0x0011
#define NETDEV_RELEASE      0x0012
#define NETDEV_NOTIFY_PEERS 0x0013
#define NETDEV_JOIN     0x0014

struct notifier_block;
typedef int (*notifier_fn_t)(struct notifier_block *nb, unsigned long action, void *data);
struct notifier_block {
    notifier_fn_t notifier_call;
};

static inline void *netdev_priv(const struct net_device *dev)
{
    return (char *)dev + ALIGN(sizeof(struct net_device), NETDEV_ALIGN);
}

#define free_netdev kfree
#define dev_net_set(...)
#define SET_NETDEV_DEV(net, pdev)
#define dev_close(dev) net_device_close(dev)
#define netif_tx_start_all_queues(...)
#define netif_start_subqueue(...)
#define netif_tx_stop_all_queues(...)
#define __hw_addr_unsync(...)
#define __hw_addr_sync(...)
#define unregister_netdevice_queue(...)
#define unregister_netdevice_many(...)
#define dev_name(dev) "mac80211"
#define netif_rx_ni netif_receive_skb
#define netif_rx netif_receive_skb
#define net_eq(a,b) (1)
#define netdev_err(dev, f...) PRINTF(f)
#define dev_set_name(...)
#define dev_get_by_index(net, id) net_device_get_by_index(id)
#define __dev_get_by_index(net, id) net_device_get_by_index(id)

void dev_queue_xmit(struct sk_buff *skb);
int netif_receive_skb(struct sk_buff *skb);
void netif_receive_skb_list(struct list_head *head);
int netif_ether_send(struct net_device *ndev, const u8 *dest, int proto, const u8 *data, int len);
int eth_mac_addr(struct net_device *dev, char *addr);
void net_device_init(void);
void net_device_exit(void);
struct net_device *alloc_netdev_mqs(int sizeof_priv, const char *name,
                                     void (*setup)(struct net_device *dev), int txqs, int rxqs);
int net_device_close(struct net_device *dev);
int net_device_open(struct net_device *dev);
int net_device_set_mac(const char *ifname, char *addr);
int register_netdev(struct net_device *dev);
void unregister_netdev(struct net_device *dev);
int register_netdevice_notifier(const struct notifier_block *nb);
int unregister_netdevice_notifier(const struct notifier_block *nb);
int linux_get_ifhwaddr(int sock, const char *ifname, u8 *addr);
int linux_set_ifhwaddr(int sock, const char *ifname, const u8 *addr);
struct net_device *net_device_get_by_index(int id);
struct net_device *net_device_get_by_name(const char *name);
int if_nametoindex(const char *ifname);
int net_device_up(const char *ifname);
int net_device_down(const char *ifname);
int netdev_deliver_wpa_ctrlmsg(struct net_device *dev, struct sk_buff *skb);
void net_device_event(struct net_device *dev, int evt_id, char *args, int len);
int dev_alloc_name(struct net_device *dev, char *name);
int linux_set_iface_flags(int sock, const char *ifname, int up);
int linux_iface_up(int sock, char *ifname);

#define dev_put(...)
#define dev_hold(...)
#define netif_running(dev) (dev->flags & IFF_RUNNING)
#define netif_wake_subqueue(...)
#define netif_stop_subqueue(...)
#define net_info_ratelimited printk
#define trace_wake_queue(...)
#define trace_stop_queue(...)
struct napi_struct {};

/* interface name assignment types (sysfs name_assign_type attribute) */
#define NET_NAME_UNKNOWN	0	/* unknown origin (not exposed to userspace) */
#define NET_NAME_ENUM		1	/* enumerated by kernel */
#define NET_NAME_PREDICTABLE	2	/* predictably named by the kernel */
#define NET_NAME_USER		3	/* provided by user-space */
#define NET_NAME_RENAMED	4	/* renamed by user-space */

#endif  /* _LINUX_NETDEVICE_H */

