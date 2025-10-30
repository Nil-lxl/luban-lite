
#ifdef __RTOS__
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
#include <linux/hrtimer.h>
#else
#include <linux/version.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/net.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/wireless.h>
#endif

#include "hgics.h"
#include "event.h"

#define HGIC_EVENT_MAX (16)

void hgics_rx_fw_event(struct hgic_fwctrl *ctrl, struct sk_buff *skb)
{
    char drop  = 0;
    char *data = NULL;
    u32 data_len;
    u32 evt_id = 0;
    struct hgic_ctrl_hdr *evt = NULL;
    struct hgics_wdev *hg = container_of(ctrl, struct hgics_wdev, ctrl);

    evt = (struct hgic_ctrl_hdr *)skb->data;
    if (evt->hdr.type == HGIC_HDR_TYPE_EVENT || evt->hdr.type == HGIC_HDR_TYPE_EVENT2) {
        data = (char *)(evt + 1);
        data_len = skb->len - sizeof(struct hgic_ctrl_hdr);
        evt_id = HDR_EVTID(evt);
        //hgic_dbg("rx event %d\r\n", evt_id);
        switch (evt_id) {
            case HGIC_EVENT_FWDBG_INFO:
                drop = 1;
                printk("%s",data);
                break;
            default:
                break;
        }
    }

#ifdef __RTOS__
    if (evt->hdr.type == HGIC_HDR_TYPE_EVENT || evt->hdr.type == HGIC_HDR_TYPE_EVENT2) {        
        hgics_event(hg, hgic_param_ifname(NULL), evt_id, data, data_len);
    } else {        
        hgics_event(hg, hgic_param_ifname(NULL), HGIC_EVENT_HGIC_DATA, skb->data, skb->len);
    }
    dev_kfree_skb(skb);
#else
    if (!drop) {
        if (skb_queue_len(&hg->evt_list) > HGIC_EVENT_MAX) {
            kfree_skb(skb_dequeue(&hg->evt_list));
            hgic_err("event list is full (max %d), new event:%d\r\n", HGIC_EVENT_MAX, evt_id);
        }
        skb_queue_tail(&hg->evt_list, skb);
        up(&hg->evt_sema);
    } else {
        dev_kfree_skb(skb);
    }
#endif
}

