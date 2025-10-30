#ifdef __RTOS__
#include <linux/types.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/skbuff.h>
#include "../hgic_def.h"
#include "utils.h"
#include "porting/usb.h"
#else
#include <linux/version.h>
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/usb.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,0,0)
#include <linux/atomic.h>
#else
#include <asm/atomic.h>
#endif
#include <linux/slab.h>
#include <linux/netdevice.h>

#include "../hgic_def.h"
#include "utils.h"

#endif

#define HGIC_PKT_MAX_LEN (20*1024)
#define HGIC_TX_URB_CNT  8
#define HGIC_RX_URB_CNT  32

#define HGIC_USB_STATUS_STOP    BIT(0)
#define HGIC_USB_STATUS_ERR     BIT(1)

#define HGIC_USB_BUF_FLAG_USED  BIT(0)
#define HGIC_USB_BUF_FLAG_RX    BIT(1)

#define USB_TX_HEADROM 4

struct hgic_usb_buf {
    struct list_head  list;
    struct hgic_usb *usbdev;
    struct urb       *urb;
    void             *data;
    int    flag;

};

struct hgic_usb {
    usb_device_t *udev;
    uint   ep_in, ep_out;
    uint   ep_out_size, ep_in_size;
    uint   status;
    struct list_head  tx_freeq;
    struct list_head  rx_freeq;
    struct list_head  used;
    struct semaphore  tx_sema;
    spinlock_t qlock;
    struct hgic_bus bus;
};

static int txq_cnt = HGIC_TX_URB_CNT;
static int rxq_cnt = HGIC_RX_URB_CNT;
static hgic_probe probe_hdl = NULL;
static void hgic_usb_receive(urb_t *urb);
static u32 max_pkt_len = HGIC_PKT_MAX_LEN;
#ifndef __RTOS__
static const struct usb_device_id hgic_usb_wdev_ids[] = {
    { USB_DEVICE(HGIC_VENDOR_ID, HGIC_WLAN_4002) },
    { USB_DEVICE(HGIC_VENDOR_ID, HGIC_WLAN_4104) },
    { USB_DEVICE(HGIC_VENDOR_ID, HGIC_WLAN_8400)},
    { USB_DEVICE(HGIC_VENDOR_ID, HGIC_WLAN_8410)},    
    { USB_DEVICE(HGIC_VENDOR_ID, HGIC_WLAN_8000)},
    { /* end: all zeroes */             },
};

MODULE_DEVICE_TABLE(usb, hgic_usb_wdev_ids);

void hgic_usb_init_endpoint(struct hgic_usb *usbdev, struct usb_interface *intf)
{

    struct usb_host_interface *iface_desc = NULL;
    struct usb_endpoint_descriptor *endpoint = NULL;
    int i = 0;

    iface_desc = intf->cur_altsetting;
    for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
        endpoint = &iface_desc->endpoint[i].desc;
        if (usb_endpoint_is_bulk_in(endpoint)) {
            usbdev->ep_in_size = le16_to_cpu(endpoint->wMaxPacketSize);
            usbdev->ep_in = usb_endpoint_num(endpoint);
            hgic_dbg("IN  BULK: ep_in_size:%x, ep_in:%x\n", usbdev->ep_in_size, usbdev->ep_in);
        } else if (usb_endpoint_is_bulk_out(endpoint)) {
            usbdev->ep_out_size = le16_to_cpu(endpoint->wMaxPacketSize);
            usbdev->ep_out = usb_endpoint_num(endpoint);
            hgic_dbg("OUT BULK: ep_out_size:%x, ep_out:%x\n", usbdev->ep_out_size, usbdev->ep_out);
        }
    }
}

#define urb_get_context(urb)    ((urb)->context)
#define urb_get_actual_length(urb)  ((urb)->actual_length)
#define urb_get_status(urb) ((urb)->status)
#define urb_set_xfer_flags(urb, flags) (((urb)->transfer_flags) |= (flags))
#define usb_get_product(id) ((id)->idProduct)

unsigned char *hgic_usb_alloc_dma_rxbuff(unsigned int len,unsigned int flags)
{
    kmalloc(len, flags);    
}

void hgic_usb_free_dma_rxbuff(unsigned char *ptr)
{
    kfree(ptr);
}

#else
extern unsigned char *hgic_usb_alloc_dma_rxbuff(unsigned int len,unsigned int flags);
extern void hgic_usb_free_dma_rxbuff(unsigned char *ptr);
#endif

static void hgic_usb_free(struct hgic_usb *usbdev)
{
    unsigned long flags;
    struct hgic_usb_buf *buf, *n;

    spin_lock_irqsave(&usbdev->qlock, flags);
    list_for_each_entry_safe(buf, n, &usbdev->tx_freeq, list) {
        list_del(&buf->list);
        usb_free_urb(buf->urb);
        if(buf->data){
            hgic_err("skb is not free??\r\n");
            kfree_skb(buf->data);
            buf->data = NULL;
        }
        kfree(buf);
    }
    list_for_each_entry_safe(buf, n, &usbdev->rx_freeq, list) {
        list_del(&buf->list);
        //kfree(buf->data);
        hgic_usb_free_dma_rxbuff(buf->data);
        usb_free_urb(buf->urb);
        kfree(buf);
    }
    list_for_each_entry_safe(buf, n, &usbdev->used, list) {
        usb_kill_urb(buf->urb);
        list_del(&buf->list);
        if (buf->flag & HGIC_USB_BUF_FLAG_RX) {
            //kfree(buf->data);            
            hgic_usb_free_dma_rxbuff(buf->data);
        }        
        usb_free_urb(buf->urb);
        kfree(buf);
    }
    spin_unlock_irqrestore(&usbdev->qlock, flags);
}


static void hgic_usb_cancle(struct hgic_usb *usbdev)
{
    unsigned long flags;
    struct hgic_usb_buf *buf, *n;
    unsigned int mask = 0;

    //mask = sys_disable_irq();

    //spin_lock_irqsave(&usbdev->qlock, flags);
    mask = sys_disable_irq();
    list_for_each_entry_safe(buf, n, &usbdev->used, list) {
        usb_unlink_urb(buf->urb);
    }    
    sys_enable_irq(mask);
    //spin_unlock_irqrestore(&usbdev->qlock, flags);
}

static int hgic_usb_qinit(struct hgic_usb *usb, struct list_head *q, int qsize, u8 rx)
{
    int i = 0;
    struct hgic_usb_buf *buf = NULL;

    for (i = 0; i < qsize; i++) {
        buf = kzalloc(sizeof(struct hgic_usb_buf), GFP_ATOMIC);
        if (buf == NULL) {
            hgic_err("alloc fail, i=%d\r\n", i);
            break;
        }

        buf->usbdev = usb;
        buf->urb = usb_alloc_urb(0, GFP_KERNEL);
        if (buf->urb) {
            if (rx) {
                //buf->data = kmalloc(max_pkt_len, GFP_KERNEL);
                //buf->data = &usb_rx_buff;
                buf->data = hgic_usb_alloc_dma_rxbuff(max_pkt_len, GFP_KERNEL);
                if (buf->data == NULL) {
                    usb_free_urb(buf->urb);
                    kfree(buf);
                    hgic_err("alloc fail, len=%d\r\n", max_pkt_len);
                    break;
                }
                buf->flag |= HGIC_USB_BUF_FLAG_RX;
            }
            list_add_tail(&buf->list, q);
        } else {
            kfree(buf);
            break;
        }
    }
    return i;
}

static struct hgic_usb_buf *hgic_usb_deq(struct hgic_usb *usbdev, struct list_head *q)
{
    struct hgic_usb_buf *buf = NULL;
    unsigned long flags;    
    unsigned int mask = 0;

    //spin_lock_irqsave(&usbdev->qlock, flags);
    mask = sys_disable_irq();
    if (!list_empty(q)) {
        buf = list_first_entry(q, struct hgic_usb_buf, list);
        buf->flag |= HGIC_USB_BUF_FLAG_USED;
        list_del(&buf->list);
        list_add_tail(&buf->list, &usbdev->used);
    }
    //spin_unlock_irqrestore(&usbdev->qlock, flags);    
    sys_enable_irq(mask);
    return buf;
}

static void hgic_usb_enq(struct hgic_usb_buf *buf, struct list_head *q)
{
    unsigned long flags;
    unsigned int mask = 0;

    if(buf == NULL || buf->usbdev == NULL || q == NULL) {
        hgic_err("%s:Input praram error!\n",__FUNCTION__,__LINE__);
        return;
    }
    //spin_lock_irqsave(&buf->usbdev->qlock, flags);
    mask = sys_disable_irq();
    buf->flag &= ~HGIC_USB_BUF_FLAG_USED;
    list_del(&buf->list);
    list_add_tail(&buf->list, q);
    sys_enable_irq(mask);
    //spin_unlock_irqrestore(&buf->usbdev->qlock, flags);
}

static int hgic_usb_submit_rx_urb(struct hgic_usb_buf *buf)
{
    int ret = -1;

    if (buf->usbdev->status & (HGIC_USB_STATUS_STOP | HGIC_USB_STATUS_ERR)) {        
        hgic_err("Usbdev not running!\n");
        hgic_usb_enq(buf, &buf->usbdev->rx_freeq);
        return -1;
    }

    usb_fill_bulk_urb(buf->urb, buf->usbdev->udev,
                      usb_rcvbulkpipe(buf->usbdev->udev, buf->usbdev->ep_in),
                      buf->data, max_pkt_len, hgic_usb_receive, buf);
    ret = usb_submit_urb(buf->urb, GFP_ATOMIC);
    if (ret) {
        hgic_err("submit rx urb %p failed: %d\n", buf->urb, ret);
        hgic_usb_enq(buf, &buf->usbdev->rx_freeq);
        buf->usbdev->status |= HGIC_USB_STATUS_ERR;
        hgic_usb_cancle(buf->usbdev);
        return -1;
    }
    return 0;
}

int hgic_usb_submit_rx_urbs(struct hgic_usb *usbdev)
{
    int ret = 0;
    struct hgic_usb_buf *buf = NULL;

    while ((buf = hgic_usb_deq(usbdev, &usbdev->rx_freeq))) {
        ret = hgic_usb_submit_rx_urb(buf);
        if (ret) {
            break;
        }
    }
    return ret;
}

static void hgic_usb_receive(urb_t *urb)
{
    struct hgic_usb_buf *buf = NULL;
    unsigned int use_tick = 0;
    unsigned int jiff = 0;

    if(!urb) {        
        hgic_err("Invaild input param\r\n");
        return;
    }
    buf = (struct hgic_usb_buf *)urb_get_context(urb);
    if(!buf) {        
        hgic_err("Invaild urb context\r\n");
        return;
    }
    //hgic_enter();
    if(urb_get_status(urb)) {
        hgic_err("status error:%d\n",urb_get_status(urb));
        return;
    }
    
    if (buf->usbdev->status & HGIC_USB_STATUS_STOP) {        
        hgic_err("STOP, drop data, status=%x,urb:%p\r\n", buf->usbdev->status,urb);
        hgic_usb_enq(buf, &buf->usbdev->rx_freeq);
        return;
    }

    if (urb_get_actual_length(urb) > 0) {
        buf->usbdev->bus.rx_packet(buf->usbdev->bus.bus_priv, buf->data, urb_get_actual_length(urb));
    }
    hgic_usb_submit_rx_urb(buf);
    //hgic_leave();
}

static void hgic_usb_tx_complete(urb_t *urb)
{
    struct hgic_usb_buf *buf = (struct hgic_usb_buf *)urb_get_context(urb);
    struct hgic_usb     *usbdev = NULL;//buf->usbdev;

    if(!buf) {        
        hgic_err("Invaild urb context\r\n");
        return;
    }
    usbdev = buf->usbdev;

    //hgic_dbg("tx complete!\n");
    usbdev->bus.tx_complete(usbdev->bus.bus_priv, (struct sk_buff *)buf->data, !urb_get_status(urb));
    buf->data = NULL;
    hgic_usb_enq(buf, &usbdev->tx_freeq);
    up(&usbdev->tx_sema);
}

static int hgic_usb_tx_packet(void *bus, struct sk_buff *skb)
{
    int ret = 0;
    int len = ALIGN(skb->len, 4);
    struct hgic_usb_buf *buf    = NULL;
    struct hgic_usb     *usbdev = container_of(bus, struct hgic_usb, bus);

#ifdef CONFIG_USB_ZERO_PACKET
    if (IS_ALIGNED(len, usbdev->ep_out_size)) len += 4;
#endif

    if (usbdev->status & (HGIC_USB_STATUS_STOP | HGIC_USB_STATUS_ERR)) {
        hgic_err("Error in status!\n");
        ret = -EIO;
        goto __fail;
    }

    ret = down_timeout(&usbdev->tx_sema, 1000);
    if (ret) {
        hgic_err("Error in tx timeout!\n");
        ret = -EIO;
        goto __fail;
    }

    if (usbdev->status & (HGIC_USB_STATUS_STOP | HGIC_USB_STATUS_ERR)) {
        hgic_err("Error in status!\n");
        ret = -EIO;
        goto __fail;
    }

    buf = hgic_usb_deq(usbdev, &usbdev->tx_freeq);
    if (buf == NULL) {
        ret = -ENOMEM;
        hgic_err("Get usb tx buff failed!\n");
        goto __fail;
    }
    buf->data = skb;
    //hgic_dbg("data:%p,len:%d,context:%p\n",skb->data,len,buf);
    usb_fill_bulk_urb(buf->urb, usbdev->udev, usb_sndbulkpipe(usbdev->udev, usbdev->ep_out),
                      skb->data, len, hgic_usb_tx_complete, buf);
    //buf->urb->transfer_flags |= URB_ZERO_PACKET;
    urb_set_xfer_flags(buf->urb, URB_ZERO_PACKET);
    //PRINTF("%s:Send skb:%p,data:%p\n",__FUNCTION__,skb,skb->data);
    ret = usb_submit_urb(buf->urb, GFP_ATOMIC);
    if (ret) {
        hgic_err("usb_submit_urb failed, ret:%d\n", ret);
        goto __fail;
    }

    return ret;

__fail:
    if (buf) {
        buf->data = NULL;
        hgic_usb_enq(buf, &usbdev->tx_freeq);
    }
    usbdev->bus.tx_complete(usbdev->bus.bus_priv, skb, 0);
    return ret;
}


/*
static void hgic_usb_free(struct hgic_usb *usbdev)
{
    unsigned long flags;
    struct hgic_usb_buf *buf, *n;

    spin_lock_irqsave(&usbdev->qlock, flags);
    list_for_each_entry_safe(buf, n, &usbdev->tx_freeq, list) {
        list_del(&buf->list);
        usb_free_urb(buf->urb);
        if(buf->skb){
            hgic_err("skb is not free??\r\n");
            kfree_skb(buf->skb);
            buf->skb = NULL;
        }
        kfree(buf);
    }
    list_for_each_entry_safe(buf, n, &usbdev->rx_freeq, list) {
        list_del(&buf->list);
        if(buf->skb){
            hgic_err("skb is not free??\r\n");
            kfree_skb(buf->skb);
            buf->skb = NULL;
        }        
        usb_free_urb(buf->urb);
        kfree(buf);
    }
    list_for_each_entry_safe(buf, n, &usbdev->used, list) {
        usb_kill_urb(buf->urb);
        list_del(&buf->list);  
        if(buf->skb){
            //hgic_err("skb is not free??\r\n");
            kfree_skb(buf->skb);
            buf->skb = NULL;
        }      
        usb_free_urb(buf->urb);
        kfree(buf);
    }
    spin_unlock_irqrestore(&usbdev->qlock, flags);
}
*/
#ifndef __RTOS__
int hgic_usb_probe(usb_interface_t *intf, const struct usb_device_id *id)
#else 
typedef int (*reinit_callback)(void *bus);
int hgic_usb_probe(usb_interface_t *intf, const struct usb_device_id *id,reinit_callback cb)
#endif
{
    int i   = 0;
    int ret = 0;
    usb_device_t *udev = NULL;
    struct hgic_usb   *usbdev = NULL;

    hgic_dbg("new usb card: vendor:%x, id:%x\n", usb_get_vendor(id), usb_get_product(id));
    udev = interface_to_usbdev(intf);
    usbdev = kzalloc(sizeof(struct hgic_usb), GFP_KERNEL);
    if (!usbdev) {
        return -ENOMEM;
    }

    hgic_dbg("Enter\n");
    memset(usbdev, 0, sizeof(struct hgic_usb));
    usbdev->udev = udev;
    usbdev->bus.type = HGIC_BUS_USB;
    usbdev->bus.dev_id = usb_get_product(id);
    usbdev->bus.drv_tx_headroom = USB_TX_HEADROM;
    usbdev->bus.tx_packet = hgic_usb_tx_packet;
    usbdev->bus.tx_ctrl = hgic_usb_tx_packet;
    usbdev->bus.bootdl_pktlen = 2048 - 8;
    usbdev->bus.bootdl_cksum = HGIC_BUS_BOOTDL_CHECK_0XFD;
    usbdev->bus.probe = probe_hdl;
    //usbdev->status = HGIC_USB_STATUS_STOP;

#ifdef __RTOS__
    usbdev->bus.reinit = cb;
    set_bit(HGIC_BUS_FLAGS_NOPOLL, &usbdev->bus.flags);
#endif
    hgic_usb_init_endpoint(usbdev, intf);
    spin_lock_init(&usbdev->qlock);
    INIT_LIST_HEAD(&usbdev->tx_freeq);
    INIT_LIST_HEAD(&usbdev->rx_freeq);
    INIT_LIST_HEAD(&usbdev->used);
    ret = hgic_usb_qinit(usbdev, &usbdev->tx_freeq, txq_cnt, 0);
    sema_init(&usbdev->tx_sema, ret);
    hgic_dbg("usb txq:%d\r\n", ret);
    ret = hgic_usb_qinit(usbdev, &usbdev->rx_freeq, rxq_cnt, 1);
    hgic_dbg("usb rxq:%d\r\n", ret);

    usbdev->bus.probe(usbdev_to_dev(udev), &usbdev->bus);
    if (!usbdev->bus.bus_priv) {
        ret = -ENOMEM;
        goto __failed;
    }

    usb_get_dev(udev);
    usb_set_intfdata(intf, usbdev);
    //usbdev->status &= ~HGIC_USB_STATUS_STOP;

    ret = hgic_usb_submit_rx_urbs(usbdev);
    if (ret) {
        hgic_err("submit urb error,ret:%d\n",ret);
        goto __failed;
    }

    if (usbdev->bus.probe_post) {
        usbdev->bus.probe_post(usbdev->bus.bus_priv);
    }
    return ret;

__failed:
    hgic_err("USB probe failed,ret:%d\n",ret);
    if (usbdev->bus.remove) {
        usbdev->bus.remove(usbdev->bus.bus_priv);
    }
    hgic_usb_free(usbdev);
    kfree(usbdev);
    return -1;
}


#ifndef __RTOS__

static void hgic_usb_disconnect(struct usb_interface *intf)
{
    struct hgic_usb *usbdev = usb_get_intfdata(intf);
    hgic_dbg("Enter\n");
    if (usbdev) {
        usbdev->status |= HGIC_USB_STATUS_STOP;
        up(&usbdev->tx_sema);
        if (usbdev->bus.remove) {
            usbdev->bus.remove(usbdev->bus.bus_priv);
        }
        hgic_usb_free(usbdev);
        kfree(usbdev);
        usb_set_intfdata(intf, NULL);
    }
    hgic_dbg("Leave\n");
}

static struct usb_driver hgic_usb_driver = {
    .name = KBUILD_MODNAME,
    .probe = hgic_usb_probe,
    .disconnect = hgic_usb_disconnect,
    .id_table = hgic_usb_wdev_ids,
    .suspend = NULL,
    .resume = NULL,
    .supports_autosuspend = 1,
};

int __init hgic_usb_init(hgic_probe probe, u32 max_pkt)
{
    int ret = 0;

    hgic_dbg("Enter, max_pkt_len = %d\n", max_pkt_len);
    probe_hdl = probe;
    if (max_pkt > HGIC_PKT_MAX_LEN) {
        max_pkt_len = 5 * max_pkt;
    }
    ret = usb_register(&hgic_usb_driver);
    if (ret) {
        hgic_err("driver register failed: %d\n", ret);
    }
    hgic_dbg("Leave\n");
    return ret;
}

void __exit hgic_usb_exit(void)
{
    hgic_dbg("Enter\n");
    usb_deregister(&hgic_usb_driver);
    hgic_dbg("Leave\n");
}

#else 
int __init hgic_usb_init(hgic_probe probe,u32 max_pkt)
{
    int ret = 0;

    hgic_dbg("Enter, max_pkt_len = %d\n", max_pkt_len);
    probe_hdl = probe;
    if(max_pkt > HGIC_PKT_MAX_LEN){
        max_pkt_len = max_pkt;
    }
    
    ret = sys_usb_init();
    if(ret) {
        hgic_err("sys_usb_init failed: %d\n", ret);
        return ret;
    }
    hgic_dbg("Leave\n");
    return ret;
}

void __exit hgic_usb_exit(void)
{
    hgic_dbg("Enter\n");
    //usb_deregister(&hgic_usb_driver);
    sys_usb_exit();
    hgic_dbg("Leave\n");
}

int hgic_usb_stop(usb_interface_t *intf) 
{
    struct hgic_usb *usbdev = (struct hgic_usb *)usb_get_intfdata(intf);
    if(usbdev == NULL) {
        return -1;
    }
    usbdev->status |= HGIC_USB_STATUS_STOP;
    return 0;
}

int hgic_usb_err(usb_interface_t *intf) 
{
    struct hgic_usb *usbdev = (struct hgic_usb *)usb_get_intfdata(intf);
    if(usbdev == NULL) {
        return -1;
    }
    usbdev->status |= HGIC_USB_STATUS_ERR;
    return 0;
}

void hgic_usb_disconnect(usb_interface_t *intf)
{
    struct hgic_usb *usbdev = (struct hgic_usb *)usb_get_intfdata(intf);
    hgic_dbg("hgic_usb_disconnect>>>>\r\n");
    if (usbdev) {
        usbdev->status |= HGIC_USB_STATUS_STOP;
        if (usbdev->bus.remove) {
            //usbdev->bus.remove(usbdev->bus.bus_priv);
        }
        hgic_usb_free(usbdev);
        //kfree(usbdev);
        //usb_set_intfdata(intf, NULL);
    }
    hgic_dbg("Leave\n");
}

void hgic_usb_queue_init(usb_interface_t *intf)
{
    struct hgic_usb *usbdev = (struct hgic_usb *)usb_get_intfdata(intf);
    int ret = 0;

    PRINTF("%s:Enter,usbdev:%p\n",__FUNCTION__,usbdev);
    //hgic_usb_free(usbdev);
    hgic_usb_init_endpoint(usbdev, intf);
    //spin_lock_init(&usbdev->qlock);
    INIT_LIST_HEAD(&usbdev->tx_freeq);
    INIT_LIST_HEAD(&usbdev->rx_freeq);
    INIT_LIST_HEAD(&usbdev->used);
    ret = hgic_usb_qinit(usbdev, &usbdev->tx_freeq, txq_cnt, 0);
    //sema_init(&usbdev->tx_sema, ret);
    hgic_dbg("usb txq:%d,queue:%p\r\n", ret, &usbdev->tx_freeq);
    ret = hgic_usb_qinit(usbdev, &usbdev->rx_freeq, rxq_cnt, 1);
    hgic_dbg("usb rxq:%d,queue:%p\r\n", ret, &usbdev->rx_freeq);
}

void *hgic_usb_get_bus_priv(usb_interface_t *intf)
{
    struct hgic_usb *usbdev = NULL;
    if(!intf) {
        hgic_err("Input param error!\n");
        return NULL;
    }
    usbdev = (struct hgic_usb *)usb_get_intfdata(intf);
    if(!usbdev) {        
        hgic_err("Input param error");
        return NULL;
    } else {        
        return usbdev->bus.bus_priv;
    }
}

#endif

