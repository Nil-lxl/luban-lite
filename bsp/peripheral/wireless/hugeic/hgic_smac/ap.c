#ifdef __RTOS__
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/etherdevice.h>
#include <linux/hrtimer.h>
#include <net/cfg80211.h>
#include <net/mac80211.h>
#include "umac_config.h"
#else
#include <linux/version.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/net.h>
#include <linux/slab.h>
#include <linux/etherdevice.h>
#endif

#include "hgics.h"
#include "ap.h"
#include "util.h"

static void hgics_beacon_queue(struct hgics_wdev *hg, struct sk_buff *skb, u8 beacon)
{
    struct hgic_frm_hdr *frmhdr = NULL;
    struct ieee80211_tx_info *info;
    u8 pad = ((u32)skb->data & 0x3) ? (4 - ((u32)skb->data & 0x3)) : 0;

    info   = IEEE80211_SKB_CB(skb);
    frmhdr = (struct hgic_frm_hdr *)skb_push(skb, sizeof(struct hgic_frm_hdr) + pad);
    memset(frmhdr, 0, sizeof(struct hgic_frm_hdr) + pad);
    frmhdr->hdr.magic  = cpu_to_le16(HGIC_HDR_TX_MAGIC);
    frmhdr->hdr.length = cpu_to_le16(skb->len);
    frmhdr->hdr.type   = (beacon ? HGIC_HDR_TYPE_BEACON : HGIC_HDR_TYPE_FRM);
    frmhdr->hdr.ifidx  = 0;
    frmhdr->hdr.flags  = pad;
    frmhdr->tx_info.band   = info->band;
    frmhdr->tx_info.tx_mcs = 0xff;
    frmhdr->tx_info.tx_bw  = 0xff;
    frmhdr->tx_info.tx_flags  = info->flags;
    frmhdr->tx_info.tx_flags2 = 0;
    if (!beacon) {
        frmhdr->tx_info.tx_flags2 |= HGIC_HDR_FLAGS2_AFT_BEACON;
    }
    skb_queue_tail(&hg->data_txq[0], skb);
    queue_work(hg->tx_wq, &hg->tx_work);
}

static void hgics_mac80211_beacon_tx(void *arg, u8 *mac, struct ieee80211_vif *vif)
{
    struct sk_buff *skb;
    struct hgics_wdev   *hg = (struct hgics_wdev *)arg;
    struct ieee80211_hw *hw = hg->hw;
    struct hgics_vif  *hgvif = (struct hgics_vif *)vif->drv_priv;

    if (vif->type != NL80211_IFTYPE_AP || 
        test_bit(HGICS_STATE_REMOVE, &hg->state) ||
        !test_bit(HGICS_VIF_STATE_BEACON, &hgvif->state)) {
        return;
    }

    skb = ieee80211_beacon_get(hw, vif);
    if (skb == NULL) {
        hgic_err("get beacon fail\r\n");
        return;
    }

    hgics_beacon_queue(hg, skb, 1);
    skb = ieee80211_get_buffered_bc(hw, vif);
    while (skb) {
        hgics_beacon_queue(hg, skb, 0);
        skb = ieee80211_get_buffered_bc(hw, vif);
    }
}

#if !defined(__RTOS__) && LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
void hgics_mac80211_beacon(struct timer_list *t)
{
    struct hgics_wdev *hg = from_timer(hg, t, beacon_timer);
#else
void hgics_mac80211_beacon(unsigned long arg)
{
    struct hgics_wdev *hg = (struct hgics_wdev *)arg;
#endif
    struct ieee80211_hw *hw = hg->hw;

    if (test_bit(HGICS_STATE_REMOVE, &hg->state)) {
        return;
    }
    IEEE80211_ITERATE_ACTIVE_INTERFACES_ATOMIC(hw, IEEE80211_IFACE_ITER_NORMAL, hgics_mac80211_beacon_tx, hg);
    mod_timer(&hg->beacon_timer, jiffies + msecs_to_jiffies(hg->beacon_int));
}

void hgics_ap_reset_beacon(struct ieee80211_hw *hw,        struct ieee80211_vif *vif, struct ieee80211_bss_conf *info)
{
    struct hgics_wdev *hg = (struct hgics_wdev *)hw->priv;
    struct hgics_vif  *hgvif = (struct hgics_vif *)vif->drv_priv;

    if (vif->type == NL80211_IFTYPE_AP || vif->type == NL80211_IFTYPE_AP_VLAN) {
        set_bit(HGICS_VIF_STATE_BEACON, &hgvif->state);
        del_timer(&hg->beacon_timer);
        if (info->enable_beacon) {
            mod_timer(&hg->beacon_timer, jiffies + msecs_to_jiffies(hg->beacon_int));
        }
    }
}

