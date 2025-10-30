#include "atbm_hal.h"
#include "atbm_os_skbuf.h"
#include "atbm_os_api.h"
#include "lwip/netif.h"
#include "netif/etharp.h"
atbm_void atbm_skbbuffer_init()
{
}
struct atbm_net_device * atbm_alloc_netdev(atbm_int32 size)
{
    struct atbm_net_device *  netdev = atbm_kmalloc(size + sizeof(struct atbm_net_device),GFP_KERNEL);

    ATBM_ASSERT((netdev != ATBM_NULL));
    if(netdev)
        atbm_memset(netdev,0,(size + sizeof(struct atbm_net_device)));

    netdev->nif = atbm_kmalloc(sizeof(struct netif),GFP_KERNEL);
    ATBM_ASSERT(netdev->nif != ATBM_NULL);
    if(netdev->nif)
        atbm_memset(netdev->nif,0,sizeof(struct netif));
    atbm_os_init_waitevent(&netdev->tx_enable);
    atbm_os_init_waitevent(&netdev->rx_enable);
    wifi_printk(WIFI_ALWAYS,"atbm_alloc_netdev,netdev(%p),nif(%p),priv(%p)\n",netdev,netdev->nif,&netdev->drv_priv[0]);
    return  netdev;
}
atbm_void * netdev_drv_priv(struct atbm_net_device *ndev)
{
    return &ndev->drv_priv[0];

}
atbm_void atbm_free_netdev(struct atbm_net_device * netdev)
{
    if(netdev != ATBM_NULL)
        atbm_kfree(netdev);
}

atbm_void atbm_lwip_init(struct atbm_net_device *dev)
{
}

atbm_void atbm_lwip_enable(struct atbm_net_device *dev)
{
    int queue_id;
    struct atbmwifi_vif *priv = netdev_drv_priv(dev);
    struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);

    if(!dev->lwip_enable){
        dev->lwip_enable=1;
        dev->lwip_queue_enable=1;
        atbm_os_wakeup_event(&dev->tx_enable);

        for(queue_id = ATBM_IEEE80211_AC_VO; queue_id <= ATBM_IEEE80211_AC_BK; queue_id++){
            if(hw_priv->tx_queue[queue_id].overfull == ATBM_TRUE){
                dev->lwip_queue_enable=0;
            }
        }
    }
}

atbm_void atbm_lwip_disable(struct atbm_net_device *dev)
{
    if(dev->lwip_enable){
        dev->lwip_enable=0;
        dev->lwip_queue_enable=0;
    }
}

atbm_void atbm_lwip_txdone(struct atbm_net_device *dev)
{
}

atbm_void atbm_lwip_wake_queue(struct atbm_net_device *dev,int num)
{
    if(!dev->lwip_queue_enable && dev->lwip_enable){
        dev->lwip_queue_enable = 1;
        atbm_os_wakeup_event(&dev->tx_enable);
    }
}

atbm_void atbm_lwip_stop_queue(struct atbm_net_device *dev,int num)
{
    if(dev->lwip_queue_enable && dev->lwip_enable){
        dev->lwip_queue_enable = 0;
    }
}

atbm_void atbm_lwip_task_event(struct atbm_net_device *dev)
{
}
extern struct atbmwifi_common g_hw_prv;

int atbm_wifi_tx_pkt(atbm_int8 *buff, atbm_uint32 len, atbm_int8 if_id)
{
    struct atbmwifi_vif *priv = _atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,if_id);
    struct atbm_buff *AtbmBuf = ATBM_NULL;
    if(!priv || !priv->ndev->lwip_enable)
    {
        return -1;
    }
    int retry=0;
	while(!priv->ndev->lwip_queue_enable){
		atbm_os_wait_event_timeout(&priv->ndev->tx_enable, 50); 
		if(++retry >= 3){
			wifi_printk(WIFI_TX,"[%s] wait queue timeout \n",priv->if_id ? "AP":"STA");
			break;
		}
	}
    AtbmBuf = atbm_dev_alloc_skb(len);
    if (!AtbmBuf)
    {
        wifi_printk(WIFI_TX,"<ERROR> tx_pkt alloc skb \n");
        ATBM_BUG_ON(1);
        return -1;
    }

    //donot use for cyc, because use pbuf_copy_partial
    uint8_t *tmp = atbm_skb_put(AtbmBuf, len);
    memcpy(tmp, buff, len);

    if(priv->ndev && priv->ndev->netdev_ops){
        priv->ndev->netdev_ops->ndo_start_xmit(priv, AtbmBuf);
    } else {
        ATBM_BUG_ON(1);
        atbm_dev_kfree_skb(AtbmBuf);
    }
    return 0;
}
extern void atbm6031_wifi_input(void *buf,int len,int if_id);
static atbm_void __atbm_wifi_rx_pkt(struct atbm_net_device *dev, struct atbm_buff *atbm_skb) 
{
    atbm_uint16 len = 0;
    atbm_uint8 *data=ATBM_NULL;
    struct atbmwifi_vif *priv=NULL;
    ATBM_NETIF *netif = dev->nif;
    data = ATBM_OS_SKB_DATA(atbm_skb);
    /* Obtain the size of the packet and put it into the "len" variable. */
    len = ATBM_OS_SKB_LEN(atbm_skb);
    if(netif==NULL || 0 == len){
        goto RcvErr;
    }
    priv=(struct atbmwifi_vif *)dev->drv_priv;
    atbm6031_wifi_input(data, len, ATBM_NL80211_IFTYPE_AP == priv->iftype? 1:0);
    atbm_dev_kfree_skb(atbm_skb);
    return;
RcvErr:
    atbm_dev_kfree_skb(atbm_skb);
    atbm_skb=ATBM_NULL;
}

//not required here ,   lwip_tcp_opt.net_rx = ethernetif_input.
atbm_uint32 atbm_wifi_rx_pkt(struct atbm_net_device *dev, struct atbm_buff *at_skb)
{
    struct eth_hdr *ethhdr;
    ethhdr = (struct eth_hdr *)ATBM_OS_SKB_DATA(at_skb);
    //printf("ethhdr type:%x\n",htons(ethhdr->type));
    switch (htons(ethhdr->type)) {
      /* IP or ARP packet? */
      case ETHTYPE_IP:
      case ETHTYPE_ARP:
      case 0x888E:
#if PPPOE_SUPPORT
      /* PPPoE packet? */
      case ETHTYPE_PPPOEDISC:
      case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
        /* full packet send to tcpip_thread to process */
        __atbm_wifi_rx_pkt(dev,at_skb);
        break;
      default:
        atbm_dev_kfree_skb(at_skb);
        break;
    }
    return 0;
}

struct tcpip_opt lwip_tcp_opt ={
    .net_init = atbm_lwip_init,
    .net_enable = atbm_lwip_enable,//
    .net_disable = atbm_lwip_disable,//
    .net_rx = atbm_wifi_rx_pkt,
    .net_tx_done =  atbm_lwip_txdone,
    .net_start_queue =  atbm_lwip_wake_queue,
    .net_stop_queue =   atbm_lwip_stop_queue,
    .net_task_event =   atbm_lwip_task_event,//
};
