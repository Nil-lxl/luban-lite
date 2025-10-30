#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/bitops.h>
#include <linux/list.h>
#include <linux/rcu.h>
#include <linux/rculist.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/ieee80211.h>
#include <net/genetlink.h>
#include <net/cfg80211.h>
#include <net/mac80211.h>

#include "hgics.h"
#include "util.h"

struct {
    struct sk_buff_head   txq;
    struct tasklet_struct task;
} hgxmit;

static int hgics_xmit_netdev_notify(struct notifier_block *nb, unsigned long state, void *ndev)
{
    int ret = 0;
    struct net_device *dev = (struct net_device *)ndev;
    struct sk_buff *skb = NULL;
    struct sk_buff *tmp = NULL;

    switch (state) {
        case NETDEV_DOWN:
        case NETDEV_UNREGISTER:
            spin_lock(&hgxmit.txq.lock);
            if (!skb_queue_empty(&hgxmit.txq)) {
                skb_queue_walk_safe(&hgxmit.txq, skb, tmp) {
                    if (skb->dev == ndev) {
                        __skb_unlink(skb, &hgxmit.txq);
                        kfree_skb(skb);
                    }
                }
            }
            spin_unlock(&hgxmit.txq.lock);
            break;
        default:
            break;
    }
    return ret;
}

static const struct notifier_block hgics_xmit_netdev_notifier = {
    .notifier_call = hgics_xmit_netdev_notify,
};

static void hgics_xmit_task(void *data)
{
    struct sk_buff *skb = NULL;

    while (1) {
        skb = skb_dequeue(&hgxmit.txq);
        if (skb == NULL) {
            return;
        }

        if (skb->dev == NULL) {
            kfree_skb(skb);
            continue;
        }

        if(skb->dev->netdev_ops == NULL) {
            while(1) {
                hgic_err("Invaild netdev:%p\n",skb->dev);                
            }
            kfree_skb(skb);
            continue;
        }

        if(skb->dev->magic_num != NET_DEVICE_MAGIC_NUM) {
            while(1) {
                hgic_err("Invaild dev magic_num:%x\n",skb->dev->magic_num);
            }
        }
        skb->protocol = eth_type_trans(skb, skb->dev);
        skb_set_queue_mapping(skb, skb->dev->netdev_ops->ndo_select_queue(skb->dev, skb));
        //hgic_dbg("xmit %p\n",skb);
        skb->dev->netdev_ops->ndo_start_xmit(skb, skb->dev);
        //hgic_dbg("xmit %p done\n",skb);
    }
}

void hgics_xmit_init(void)
{
    register_netdevice_notifier(&hgics_xmit_netdev_notifier);
    skb_queue_head_init(&hgxmit.txq);
    hgxmit.task.name = "umac_tx";
    tasklet_create(&hgxmit.task, (tasklet_func_t)hgics_xmit_task,
                   (unsigned long)&hgxmit, 4096);
}

void hgics_xmit_deinit(void)
{
    tasklet_kill(&hgxmit.task);
    tasklet_destory(&hgxmit.task);
    hgic_clear_queue(&hgxmit.txq);
    skb_queue_head_deinit(&hgxmit.txq);
    unregister_netdevice_notifier(&hgics_xmit_netdev_notifier);
}

void dev_queue_xmit(struct sk_buff *skb)
{       
    skb_queue_tail(&hgxmit.txq, skb);
    tasklet_schedule(&hgxmit.task);
}

