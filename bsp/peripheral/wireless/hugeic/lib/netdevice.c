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

#if defined(__MBED__) || defined(__CSKY__)
#include "lib/umac2/umac.h"
#else
#include "../hgic_def.h"
#endif

#define NOTIFY_BLOCK_CNT 4
struct netdev_mgr {
    u8     if_index; //only support two interface.
    struct mutex netdev_mutex;
    struct list_head netdev_list;
    struct notifier_block *net_notifier[NOTIFY_BLOCK_CNT];
} _netdev_mgr;

extern void sys_memcpy(void *dest, void *src, int len);
extern void sys_netif_recv(void *priv, char *data, int len,void *buff_priv);
extern void *sys_netif_register(void *dev, char *name);
extern int sys_netif_unregister(void *priv);

static void call_netdevice_notifiers(unsigned long action, void *data)
{
    int i = 0;
    for (i = 0; i < NOTIFY_BLOCK_CNT; i++) {
        if (_netdev_mgr.net_notifier[i]) {
            _netdev_mgr.net_notifier[i]->notifier_call(0, action, data);
        }
    }
}

int register_netdevice_notifier(const struct notifier_block *nb)
{
    int i = 0;
    int f = -1;
    for (i = 0; i < NOTIFY_BLOCK_CNT; i++) {
        if (_netdev_mgr.net_notifier[i] == NULL && f == -1) {
            f = i;
        } else if (_netdev_mgr.net_notifier[i] == (struct notifier_block *)nb) {
            return 0;
        }
    }
    if (f != -1) {
        _netdev_mgr.net_notifier[f] = (struct notifier_block *)nb;
    }
    return (f == -1 ? -EOVERFLOW : 0);
}

int unregister_netdevice_notifier(const struct notifier_block *nb)
{
    int i = 0;
    for (i = 0; i < NOTIFY_BLOCK_CNT; i++) {
        if (_netdev_mgr.net_notifier[i] == nb) {
            _netdev_mgr.net_notifier[i] = NULL;
            break;
        }
    }
    return 0;
}

int net_device_xmit(struct net_device *dev, char *data, int len)
{
    struct net_device *ndev = dev;
    struct sk_buff    *skb  = alloc_skb(len + ndev->needed_headroom + 64 + ndev->needed_tailroom);
    if (skb) {
        skb_reserve(skb, ndev->needed_headroom);
        memcpy(skb->data, data, len);
        skb_put(skb, len);
        skb->dev = ndev;
        //PRINTF("xmit %p\n",skb);
        dev_queue_xmit(skb);
        //PRINTF("xmit %p done\n",skb);
        return len;
    } else {
        PRINTF("%s:Error!No memory!\n",__FUNCTION__);
        return -ENOMEM;
    }
}

int net_device_xmit_scatter(void *dev, void *scat_info,int count, int total_len)
{
    struct net_device *ndev = dev;
    struct netdev_scatter_data *scat_data = NULL;
    struct sk_buff    *skb  = NULL;
    unsigned char *pdata    = NULL;
    int len = 0;
    int i = 0;

    if(!scat_info) {
        PRINTF("%s,%d:Input param error!\n",__FUNCTION__,__LINE__);
        return -EINVAL;
    }
    scat_data = (struct netdev_scatter_data *)scat_info;

    skb = alloc_skb(total_len + ndev->needed_headroom + 64 + ndev->needed_tailroom);
    if (skb) {
        skb_reserve(skb, ndev->needed_headroom);
        pdata = skb->data;
        for (i = 0; i < count; i++) {
            memcpy(pdata,(unsigned char*)scat_data[i].addr,scat_data[i].size);
            pdata += scat_data[i].size;
            len += scat_data[i].size;
        }
        ASSERT(len == total_len);
        skb_put(skb, len);
        skb->dev = ndev;
        dev_queue_xmit(skb);
        return len;
    } else {
        PRINTF("%s:Error!No memory!\n",__FUNCTION__);
        return -ENOMEM;
    }
}

int net_device_get_addr(struct net_device *dev, char *addr)
{
    if(dev == NULL || addr == NULL) {
        PRINTF("%s,%d:Input param error!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    memcpy(addr, dev->dev_addr, 6);
    return 0;
}

int net_device_set_addr(struct net_device *dev, char *addr)
{
    dev->netdev_ops->ndo_set_mac_address(dev, addr);
    return 0;
}

char **net_device_buff_alloc(struct net_device *dev, int len)
{
    struct sk_buff *skb  = alloc_skb(len + dev->needed_headroom + dev->needed_tailroom);
    if (skb) {
        skb_reserve(skb, dev->needed_headroom);
        return (char **)&skb->data;
    }
    return 0;
}

int net_device_buff_free(void *dev, char **buff)
{
    struct sk_buff *skb  = container_of(buff, struct sk_buff, data);
    kfree_skb(skb);
    return 0;
}

int net_device_buff_send(struct net_device *dev, char **buff, int len)
{
    struct sk_buff *skb  = container_of(buff, struct sk_buff, data);
    if (skb) {
        skb_put(skb, len);
        skb->dev = dev;
        dev_queue_xmit(skb);
        return 0;
    }
    return -ENOMEM;
}

static void netdev_recv_def(struct sk_buff *skb)
{
    if (skb->dev && skb->dev->priv) {
        if (skb->dev->magic_num != NET_DEVICE_MAGIC_NUM) {
            while (1) {
                hgic_err("Invaild dev magic_num:%x\n", skb->dev->magic_num);
            }
        }
        sys_netif_recv(skb->dev->priv, skb->data, skb->len,skb->free_priv);
    }
    //skb->head = NULL;
    kfree_skb(skb);
}

int register_netdev(struct net_device *dev)
{
    call_netdevice_notifiers(NETDEV_POST_INIT, dev);
    mutex_lock(&_netdev_mgr.netdev_mutex);
    list_add(&dev->list, &_netdev_mgr.netdev_list);
    mutex_unlock(&_netdev_mgr.netdev_mutex);
    call_netdevice_notifiers(NETDEV_REGISTER, dev);
    //dev->recv = netdev_recv_def;
    dev->magic_num = NET_DEVICE_MAGIC_NUM;
    dev->priv = sys_netif_register(dev, dev->name);
    return (dev->priv ? 0 : -EPERM);
}

void unregister_netdev(struct net_device *dev)
{
    if (dev) {
        net_device_close(dev);
        call_netdevice_notifiers(NETDEV_UNREGISTER, dev);
        mutex_lock(&_netdev_mgr.netdev_mutex);
        list_del(&dev->list);
        mutex_unlock(&_netdev_mgr.netdev_mutex);
        sys_netif_unregister(dev->priv);
        kfree(dev);
    }
}

int dev_alloc_name(struct net_device *dev, char *name)
{
    mutex_lock(&_netdev_mgr.netdev_mutex);
    dev->ifindex = (_netdev_mgr.if_index & 0x1) ? 1 : 0;
    _netdev_mgr.if_index |= (1 << _netdev_mgr.if_index);
    sprintf(dev->name, name, dev->ifindex);
    hgic_param_ifname(dev->name);
    mutex_unlock(&_netdev_mgr.netdev_mutex);
    return 0;
}

struct net_device *alloc_netdev_mqs(int sizeof_priv, const char *name,
                                    void (*setup)(struct net_device *dev), int txqs, int rxqs)
{
    size_t alloc_size = 0;
    struct net_device *dev = NULL;
    struct net_device *p   = NULL;

    alloc_size = sizeof(struct net_device);
    if (sizeof_priv) {
        /* ensure 32-byte alignment of private area */
        alloc_size = ALIGN(alloc_size, NETDEV_ALIGN);
        alloc_size += sizeof_priv;
    }
    /* ensure 32-byte alignment of whole construct */
    alloc_size += NETDEV_ALIGN - 1;

    dev = kzalloc(alloc_size, GFP_KERNEL);
    if (dev) {
        memset(dev, 0, sizeof_priv + sizeof(struct net_device));
        setup(dev);
        dev_alloc_name(dev, (char *)name);
    }
    return dev;
}

int netif_ether_send(struct net_device *ndev, const u8 *dest, int proto, const u8 *data, int len)
{
    struct ethhdr *ehdr = NULL;
    struct sk_buff *skb = NULL;

    skb = dev_alloc_skb(ndev->needed_headroom + ndev->needed_tailroom +
                        sizeof(struct hgic_hdr) + sizeof(struct ethhdr) + len);
    if (skb) {
        skb_reserve(skb, ndev->needed_headroom + sizeof(struct hgic_hdr));
        ehdr = (struct ethhdr *)skb->data;
        ehdr->h_proto = htons(proto);
        memcpy(ehdr->h_dest, dest, ETH_ALEN);
        memcpy(ehdr->h_source, ndev->dev_addr, ETH_ALEN);
        memcpy(ehdr + 1, data, len);
        skb->protocol = htons(proto);
        skb->dev = ndev;
        skb_put(skb, sizeof(struct ethhdr) + len);
        dev_queue_xmit(skb);
    }
    return 0;
}

int netif_receive_skb(struct sk_buff *skb)
{
    //PRINTF("%s,%d:recv data packet,len:%d\n,", __FUNCTION__,__LINE__, skb->len);
    if (skb->dev && skb->dev->priv) {
        if (skb->dev->magic_num != NET_DEVICE_MAGIC_NUM) {
            while (1) {
                hgic_err("Invaild dev magic_num:%x\n", skb->dev->magic_num);
            }
        }
        sys_netif_recv(skb->dev->priv, skb->data, skb->len,skb->free_priv);
    }
    //skb->head = NULL;
    kfree_skb(skb);
}

void netif_receive_skb_list(struct list_head *head)
{
    struct sk_buff *skb, *next;

    if (list_empty(head)){
        return;
    }
    list_for_each_entry_safe(skb, next, head, list) {
        list_del(&skb->list);
        netif_receive_skb(skb);
    }
}

int eth_mac_addr(struct net_device *dev, char *addr)
{
    if(!dev || !addr) {
        hgic_err("Input param error,dev:%p,addr:%p\n",dev,addr);
        return -EINVAL;
    }
    if (!is_valid_ether_addr((const u8 *)addr)) {
        return -EADDRNOTAVAIL;
    }
    //PRINTF("set addr:"MACSTR"\r\n", MAC2STR(addr));
    memcpy(dev->dev_addr, addr, ETH_ALEN);
    return 0;
}

int if_nametoindex(const char *ifname)
{
    struct net_device *dev = net_device_get_by_name(ifname);
    return (dev) ? (dev->ifindex) : 0;
}

int linux_get_ifhwaddr(int sock, const char *ifname, u8 *addr)
{
    struct net_device *dev = net_device_get_by_name(ifname);
    if (dev) {
        memcpy(addr, dev->dev_addr, ETH_ALEN);
        //PRINTF("get addr:"MACSTR"\r\n", MAC2STR(addr));
        return 0;
    }
    return -1;
}

int linux_set_ifhwaddr(int sock, const char *ifname, const u8 *addr)
{
    return net_device_set_mac(ifname, (char *)addr);
}

struct net_device *net_device_get_by_index(int id)
{
    struct net_device *pos, *dev = NULL;

    //mutex_lock(&netdev_mutex);
    list_for_each_entry(pos, &_netdev_mgr.netdev_list, list) {
        if (pos->ifindex == id) {
            dev = pos;
            break;
        }
    }
    //mutex_unlock(&netdev_mutex);
    return dev;
}

struct net_device *net_device_get_by_addr(char *addr)
{
    struct net_device *pos, *dev = NULL;

    if (addr == NULL) { return dev; }
    //mutex_lock(&netdev_mutex);
    list_for_each_entry(pos, &_netdev_mgr.netdev_list, list) {
        if (memcmp(pos->dev_addr, addr, ETH_ALEN)  == 0) {
            dev = pos;
            break;
        }
    }
    //mutex_unlock(&netdev_mutex);
    return dev;
}

struct net_device *net_device_get_by_name(const char *name)
{
    struct net_device *pos, *dev = NULL;

    if (name == NULL) {
        return dev;
    }
    //mutex_lock(&netdev_mutex);
    list_for_each_entry(pos, &_netdev_mgr.netdev_list, list) {
        //PRINTF("%s:netdev %s in nedev_list, input name:%s\n",__FUNCTION__,pos->name, name);
        if (strcmp(pos->name, name)  == 0) {
            dev = pos;
            break;
        }
    }
    //mutex_unlock(&netdev_mutex);
    if (!dev) {
        //PRINTF("%s:dev %s not find!\n", __FUNCTION__, name);
    }
    return dev;
}

int net_device_set_mac(const char *ifname, char *addr)
{
    struct net_device *dev = net_device_get_by_name(ifname);
    if (dev) {
        return dev->netdev_ops->ndo_set_mac_address(dev, (void *)addr);
    }
    return -1;
}

int net_device_open(struct net_device *dev)
{
    int err = -1;

    if (dev) {
        if (netif_running(dev)) {
            return 0;
        }
        call_netdevice_notifiers(NETDEV_PRE_UP, dev);
        err = dev->netdev_ops->ndo_open(dev);
        if (!err) {
            dev->flags |= IFF_RUNNING;
            call_netdevice_notifiers(NETDEV_UP, dev);
        }
        PRINTF("open net device:%s (err:%d)\r\n", dev->name, err);
    } else {
        PRINTF("Error,dev is NULL!\n");
    }
    return err;
}

int net_device_close(struct net_device *dev)
{
    int err = -1;

    if (dev) {
        if (!netif_running(dev)) {
            return 0;
        }
        call_netdevice_notifiers(NETDEV_GOING_DOWN, dev);
        err = dev->netdev_ops->ndo_stop(dev);
        dev->flags &= ~IFF_RUNNING;
        call_netdevice_notifiers(NETDEV_DOWN, dev);
        PRINTF("close net device:%s (err:%d)\r\n", dev->name, err);
    }
    return err;
}

void net_device_init(void)
{
    INIT_LIST_HEAD(&_netdev_mgr.netdev_list);
    mutex_init(&_netdev_mgr.netdev_mutex);
}

void net_device_exit(void)
{
    mutex_destroy(&_netdev_mgr.netdev_mutex);
}

static void net_device_state_change(struct net_device *dev, int evt_id, char *args, int len)
{
    unsigned char *pstate = (unsigned char *)args;
    int event_id = 0;

    if(pstate == NULL || dev == NULL) {
        hgic_err("Input param error!\n");
        return;
    }
    if(evt_id != HGIC_EVENT_STATE_CHG) {
        dev->event(dev->name, evt_id, args, len);
        return;
    }

    switch(*pstate) {
        case 9: //WPA_COMPLETED
            event_id = HGIC_EVENT_CONECTED;
            hgic_dbg("Connected,MAC:"MACSTR"\n",MAC2STR(pstate+1));
            //memcpy(evt->mac_addr,(char *)(pstate + 1),6);
            dev->event(dev->name, event_id, args + 1, len - 1);
            break;
       case 0: //WPA_DISCONNECTED
            event_id = HGIC_EVENT_DISCONECTED;
            hgic_dbg("Disonnected,MAC:"MACSTR"\n",MAC2STR(pstate+1));
            dev->event(dev->name, event_id, args + 1, len - 1);
            break;
       case 5: //WPA_ASSOCIATING
            event_id = HGIC_EVENT_CONECT_START;
            dev->event(dev->name, event_id, NULL, 0);
            break;
       default:
            event_id = *pstate;
            dev->event(dev->name, event_id, NULL, 0);
            break;
    }
}


void net_device_event(struct net_device *dev, int evt_id, char *args, int len)
{
    int event = 0;
    if (dev && dev->event) {
        if(evt_id == HGIC_EVENT_STATE_CHG) {
            net_device_state_change(dev,evt_id,args,len);
        } else {
            dev->event(dev->name, evt_id, args, len);
        }
    }
}

int linux_set_iface_flags(int sock, const char *ifname, int up)
{
    return up ? net_device_up(ifname) : net_device_down(ifname);
}

int linux_iface_up(int sock, char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    return ndev && netif_running(ndev);
}

int net_device_up(const char *ifname)
{
    return net_device_open(net_device_get_by_name(ifname));
}

int net_device_down(const char *ifname)
{
    return net_device_close(net_device_get_by_name(ifname));
}

