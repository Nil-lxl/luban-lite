
#ifdef __RTOS__
#include <linux/types.h>
#include <linux/unaligned.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/hrtimer.h>
#include <linux/netdevice.h>
#else
#include <linux/version.h>
//#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>

#define MAC2STR(a) (a)[0]&0xff, (a)[1]&0xff, (a)[2]&0xff, (a)[3]&0xff, (a)[4]&0xff, (a)[5]&0xff
#define MACSTR     "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#include "hgics.h"
#include "hw.h"
#include "ap.h"
#include "util.h"
#include "event.h"
#include "stabr.h"


#define TXWND_INIT_VAL (3)

static int txq_size    = 1024;
static int if_test     = 0;
static int hgics_dack  = 0;
static int no_bootdl   = 0;
static char *conf_file = "/etc/hgics.conf";
static char *fw_file   = "hgics.bin";
static char *ifname    = NULL;
static int if_agg      = 0;
static int bt_en       = 0;
static const u8 default_mac[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
extern struct hgic_fw_info hgic_fwinfo;

#ifdef __RTOS__
static int iface_cnt = 1;
static hgic_init_cb  init_cb = NULL;
static hgic_event_cb event_cb = NULL;
extern struct hgics_config *hgics_configs_get();
extern void hgics_configs_put(struct hgics_config *conf);
#endif

static const int priority_to_ac[8] = {
    IEEE80211_AC_BE,
    IEEE80211_AC_BK,
    IEEE80211_AC_BK,
    IEEE80211_AC_BE,
    IEEE80211_AC_VI,
    IEEE80211_AC_VI,
    IEEE80211_AC_VO,
    IEEE80211_AC_VO
};

extern void malloc_trace(int enable);
extern void malloc_trace_print();

static u16 hgics_data_cookie(void *priv)
{
    struct hgics_wdev *hg = priv;
    unsigned long flags;
    uint16_t cookie = 0;

    spin_lock_irqsave(&hg->lock, flags);
    cookie = hg->data_cookie++;
    hg->data_cookie &= HGIC_TX_COOKIE_MASK;
    spin_unlock_irqrestore(&hg->lock, flags);
    return cookie;
}

static int hgics_ops_start(struct ieee80211_hw *hw)
{
    int err = 0;
    struct hgics_wdev *hg = (struct hgics_wdev *)hw->priv;

    hgic_dbg("Enter ...\n");
    hg->hghw->ops->start(hw);
    hg->radio_off = 0;
    hgic_fwctrl_open_dev(&hg->ctrl, 1);
    set_bit(HGICS_STATE_START, &hg->state);
    ieee80211_wake_queues(hw);
    hgic_dbg("Leave, err:%d\r\n", err);
    return err;
}

static void hgics_ops_stop(struct ieee80211_hw *hw)
{
    struct hgics_wdev *hg = (struct hgics_wdev *)hw->priv;

    hgic_enter();
    hg->hghw->ops->stop(hw);
    ieee80211_stop_queues(hw);
    hgic_clear_queue(&hg->data_txq[0]);
    hgic_clear_queue(&hg->data_txq[1]);
    hgic_clear_queue(&hg->data_txq[2]);
    hgic_clear_queue(&hg->data_txq[3]);
    hgic_clear_queue(&hg->trans_q);
    hgic_clear_queue(&hg->ack_q);
#ifndef __RTOS__
    hgic_clear_queue(&hg->evt_list);
#endif
    hgic_fwctrl_close_dev(&hg->ctrl, 1);
    clear_bit(HGICS_STATE_START, &hg->state);
    hg->radio_off = 1;
    hgic_leave();
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
static int hgics_ops_start_ap(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
    int err = 0;
    struct hgics_wdev *hg    = (struct hgics_wdev *)hw->priv;
    struct hgics_vif  *hgvif = (struct hgics_vif *)vif->drv_priv;

    hgic_dbg("Enter ...\n");
    hg->hghw->ops->start_ap(hw, vif);
    hgic_fwctrl_set_beacon_start(&hg->ctrl, hgvif->idx, 1);
    hgic_dbg("Leave, err:%d\r\n", err);
    return err;
}

static void hgics_ops_stop_ap(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
    struct hgics_wdev *hg    = (struct hgics_wdev *)hw->priv;
    struct hgics_vif  *hgvif = (struct hgics_vif *)vif->drv_priv;
    hgic_enter();
    hg->hghw->ops->stop_ap(hw, vif);
    del_timer(&hg->beacon_timer);
    hgic_fwctrl_set_beacon_start(&hg->ctrl, hgvif->idx, 0);
    clear_bit(HGICS_VIF_STATE_BEACON, &hgvif->state);
    hgic_leave();
}
#endif

static int hgics_ops_add_interface(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
    int err = 0;
    struct hgics_wdev *hg    = (struct hgics_wdev *)hw->priv;
    struct hgics_vif  *hgvif = (struct hgics_vif *)vif->drv_priv;

    hgic_dbg("add interface, type=%d\r\n", vif->type);
    switch (vif->type) {
        case NL80211_IFTYPE_STATION:
            hg->sta = hgvif;
            hgvif->idx = HGIC_WDEV_ID_STA;
            break;
        case NL80211_IFTYPE_AP:
            hg->ap = hgvif;
            hgvif->idx = HGIC_WDEV_ID_AP;
            break;
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
        case NL80211_IFTYPE_P2P_DEVICE:
            hg->p2p = hgvif;
            hgvif->idx = HGIC_WDEV_ID_AP;
            break;
#endif
        default:
            hgic_err("unknow interface type:%d!!\n", vif->type);
            break;
    }

    hgvif->vif  = vif;
    hgvif->hg   = hg;
    hgvif->type = vif->type;
    set_bit(HGICS_VIF_STATE_ADD, &hgvif->state);
    clear_bit(HGICS_VIF_STATE_BEACON, &hgvif->state);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
    vif->cab_queue = 0;
    vif->hw_queue[IEEE80211_AC_VO] = 0;
    vif->hw_queue[IEEE80211_AC_VI] = 1;
    vif->hw_queue[IEEE80211_AC_BE] = 2;
    vif->hw_queue[IEEE80211_AC_BK] = 3;
#endif
#ifdef __RTOS__
    if(vif->dev) {
        vif->dev->event = event_cb;
        vif->dev->ctrl  = &hg->ctrl;
        vif->dev->fwifidx = hgvif->idx;
    }
#endif

    hgic_dbg("if%d set mac %pM\r\n", hgvif->idx, vif->addr);
    hg->hghw->ops->add_interface(hw, vif);
    hgic_fwctrl_set_mac(&hg->ctrl, hgvif->idx, vif->addr);
    hgic_leave();
    return err;
}

static void hgics_ops_remove_interface(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
    struct hgics_wdev *hg    = (struct hgics_wdev *)hw->priv;
    struct hgics_vif  *hgvif = (struct hgics_vif *)vif->drv_priv;

    hgic_dbg("remove interface, type=%d\r\n", vif->type);
    if (test_bit(HGICS_VIF_STATE_ADD, &hgvif->state)) {
        switch (vif->type) {
            case NL80211_IFTYPE_STATION:
                hg->sta = NULL;
                break;
            case NL80211_IFTYPE_AP:
                hg->ap = NULL;
                hgic_fwctrl_set_beacon_start(&hg->ctrl, hgvif->idx, 0);
                break;
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
            case NL80211_IFTYPE_P2P_DEVICE:
                hg->p2p = NULL;
                break;
#endif
            default:
                break;
        }
        clear_bit(HGICS_VIF_STATE_ADD, &hgvif->state);
        clear_bit(HGICS_VIF_STATE_BEACON, &hgvif->state);
    }

    hg->hghw->ops->remove_interface(hw, vif);
    hgic_leave();
}

static int hgics_ops_change_interface(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
                                      enum nl80211_iftype newtype, bool newp2p)
{
    struct hgics_wdev *hg    = hw->priv;
    struct hgics_vif  *hgvif = (struct hgics_vif *)vif->drv_priv;

    hgic_dbg("change interface: type: %d -> %d\r\n", vif->type, newtype);
    switch (vif->type) {
        case NL80211_IFTYPE_STATION:
            hg->sta = NULL;
            break;
        case NL80211_IFTYPE_AP:
            hgic_fwctrl_set_beacon_start(&hg->ctrl,  hgvif->idx, 0);
            hg->ap = NULL;
            break;
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
        case NL80211_IFTYPE_P2P_DEVICE:
            hg->p2p = NULL;
            break;
#endif
        default:
            break;
    }

    switch (newtype) {
        case NL80211_IFTYPE_STATION:
            hg->sta = hgvif;
            break;
        case NL80211_IFTYPE_AP:
            hg->ap = hgvif;
			hgic_fwctrl_set_beacon_start(&hg->ctrl,  hgvif->idx, 1);
            break;
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
        case NL80211_IFTYPE_P2P_DEVICE:
            hg->p2p = hgvif;
            break;
#endif
        default:
            break;
    }

    hgvif->type = newtype;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
    vif->cab_queue = 0;
#endif
    return 0;
}

static int hgics_ops_config(struct ieee80211_hw *hw, u32 changed)
{
    int err = 0;
    u32 hw_value = 0;
    struct hgics_wdev     *hg   = (struct hgics_wdev *)hw->priv;
    struct ieee80211_conf *conf = &hw->conf;

    if (changed & IEEE80211_CONF_CHANGE_MONITOR) {
        hgic_dbg("set monitor mode is %s\r\n", (conf->flags & IEEE80211_CONF_MONITOR) ? "enable" : "disable");
        hgic_fwctrl_set_promisc(&hg->ctrl, 1, (conf->flags & IEEE80211_CONF_MONITOR) ? 1 : 0);
    }

    if (changed & IEEE80211_CONF_CHANGE_LISTEN_INTERVAL) {
        hgic_fwctrl_set_listen_interval(&hg->ctrl, 1, conf->listen_interval);
    }

    if (changed & IEEE80211_CONF_CHANGE_POWER) {
        hgic_fwctrl_set_txpower(&hg->ctrl, 1, conf->power_level);
        hg->power_level = conf->power_level;
    }

    if (changed & IEEE80211_CONF_CHANGE_CHANNEL) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
        hw_value = conf->chandef.chan->center_freq;
#else
        hw_value = conf->channel->center_freq;
#endif
        hgic_fwctrl_set_center_freq(&hg->ctrl, 1, hw_value);
        hg->fw_freq = hw_value;
    }

    if (changed & IEEE80211_CONF_CHANGE_RETRY_LIMITS) {
        hgic_fwctrl_set_tx_count(&hg->ctrl, 1,
                                 conf->short_frame_max_tx_count,
                                 conf->long_frame_max_tx_count);
    }

    return err;
}

static void hgics_ops_bss_info_changed(struct ieee80211_hw *hw,
                                       struct ieee80211_vif *vif,
                                       struct ieee80211_bss_conf *info, u32 changed)
{
    struct hgics_wdev *hg = (struct hgics_wdev *)hw->priv;
    struct hgics_vif  *hgvif = (struct hgics_vif *)vif->drv_priv;

    if (changed & BSS_CHANGED_BEACON_INT) {
        hg->beacon_int = info->beacon_int;
        //hgic_dbg("  BCNINT: %d\n", hg->beacon_int);
    }

    if (changed & BSS_CHANGED_BEACON_ENABLED) {
        //hgic_dbg("  BEACON CHANGED: %d, vif type:%d\n", info->enable_beacon, vif->type);
        hgics_ap_reset_beacon(hw, vif, info);
    }

    if (changed & BSS_CHANGED_ERP_CTS_PROT) {
        //hgic_dbg("  ERP_CTS_PROT: %d\n", info->use_cts_prot);
    }

    if (changed & BSS_CHANGED_ERP_PREAMBLE) {
        //hgic_dbg("  ERP_PREAMBLE: %d\n", info->use_short_preamble);
    }

    if (changed & BSS_CHANGED_ERP_SLOT) {
        //hgic_dbg("  ERP_SLOT: %d\n", info->use_short_slot);
    }

    if (changed & BSS_CHANGED_HT) {
        //hgic_dbg("  HT: op_mode=0x%x\n", info->ht_operation_mode);
    }

    if (changed & BSS_CHANGED_BASIC_RATES) {
        //hgic_dbg("  BASIC_RATES: 0x%llx\n", (unsigned long long) info->basic_rates);
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
    if (changed & BSS_CHANGED_TXPOWER) {
        //hgic_dbg("  TX Power: %d dBm\n", info->txpower);
        hgic_fwctrl_set_txpower(&hg->ctrl, hgvif->idx, info->txpower);
    }
#endif

    if (changed & BSS_CHANGED_ASSOC) {
        //hgic_dbg("  assoc succces, aid:%d\n", info->aid);
        hgic_fwctrl_set_aid(&hg->ctrl, hgvif->idx, info->aid);
    }

#if 0
    if (changed & BSS_CHANGED_BSSID) {
    }
    if (changed & BSS_CHANGED_SSID) {
    }
    if (changed & BSS_CHANGED_BEACON) {
    }
    if (changed & BSS_CHANGED_AP_PROBE_RESP) {
    }
    if (changed & BSS_CHANGED_CQM) {
    }
    if (changed & BSS_CHANGED_IBSS) {
    }
    if (changed & BSS_CHANGED_ARP_FILTER) {
    }
    if (changed & BSS_CHANGED_QOS) {
    }
#endif
}

static void hgics_ops_configure_filter(struct ieee80211_hw *hw, unsigned int changed_flags,
                                       unsigned int *total_flags, u64 multicast)
{
    *total_flags = 0;
}

static int hgics_ops_sta_add(struct ieee80211_hw *hw, struct ieee80211_vif *vif, struct ieee80211_sta *sta)
{
    struct hgics_wdev *hg = (struct hgics_wdev *)hw->priv;
    struct hgics_vif  *hgvif = (struct hgics_vif *)vif->drv_priv;
    hgic_dbg("ifidx %d add sta %pM, aid:%d\r\n", hgvif->idx, sta->addr, sta->aid);
    return hgic_fwctrl_add_sta(&hg->ctrl, hgvif->idx, sta->aid, sta->addr);
}

static int hgics_ops_sta_remove(struct ieee80211_hw *hw, struct ieee80211_vif *vif, struct ieee80211_sta *sta)
{
    struct hgics_wdev *hg = (struct hgics_wdev *)hw->priv;
    struct hgics_vif  *hgvif = (struct hgics_vif *)vif->drv_priv;
    hgic_dbg("del sta %pM, aid:%d\r\n", sta->addr, sta->aid);
    return hgic_fwctrl_del_sta(&hg->ctrl, hgvif->idx, sta->addr);
}

static void hgics_ops_sta_notify(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
                                 enum sta_notify_cmd cmd, struct ieee80211_sta *sta)
{
    //struct hgics_wdev *hg = (struct hgics_wdev *)hw->priv;
    switch (cmd) {
        case STA_NOTIFY_SLEEP:
            //hgic_dbg("sta (aid=%d, addr=%pM), sleep ...\r\n", sta->aid, sta->addr);
            break;
        case STA_NOTIFY_AWAKE:
            //hgic_dbg("sta (aid=%d, addr=%pM), awake!\r\n", sta->aid, sta->addr);
            break;
        default:
            break;
    }
}

static int hgics_ops_set_rts_threshold(struct ieee80211_hw *hw, u32 value)
{
    struct hgics_wdev *hg = (struct hgics_wdev *)hw->priv;
    hgic_dbg("set rts threshold=%d\r\n", value);
    return hgic_fwctrl_set_rts_threshold(&hg->ctrl, 1, value);
}

static int hgics_ops_set_key(struct ieee80211_hw *hw, enum set_key_cmd cmd,
                             struct ieee80211_vif *vif, struct ieee80211_sta *sta,
                             struct ieee80211_key_conf *key)
{
    int ret = -1;
    struct hgics_wdev    *hg    = (struct hgics_wdev *)hw->priv;
    struct hgic_key_conf *hgkey = kzalloc(sizeof(struct hgic_key_conf) + key->keylen, GFP_KERNEL);
    struct hgics_vif  *hgvif = (struct hgics_vif *)vif->drv_priv;
    u8 *addr = (u8 *)(sta ? sta->addr : vif->bss_conf.bssid);

    if (hgkey == NULL) {
        return -ENOMEM;
    }

    if (vif->type == NL80211_IFTYPE_AP && sta == NULL) {
        addr = vif->addr;
    }

    hgkey->cipher = cpu_to_le32(key->cipher);
    hgkey->flags  = cpu_to_le32(key->flags);
    hgkey->keyidx = key->keyidx;
    hgkey->keylen = key->keylen;
    memcpy(hgkey->key, key->key, key->keylen);
    ret = hgic_fwctrl_set_key(&hg->ctrl, hgvif->idx, cmd, addr,
                              (u8 *)hgkey, sizeof(struct hgic_key_conf) + key->keylen);
    kfree(hgkey);
    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
static int hgics_ops_conf_tx(struct ieee80211_hw *hw, struct ieee80211_vif *vif, u16 ac, const struct ieee80211_tx_queue_params *params)
{
    struct hgics_vif  *hgvif = (struct hgics_vif *)vif->drv_priv;
#else
static int hgics_ops_conf_tx(struct ieee80211_hw *hw, u16 ac, const struct ieee80211_tx_queue_params *params)
{
    struct hgics_vif  *hgvif = NULL;
#endif
    struct hgics_wdev *hg = (struct hgics_wdev *)hw->priv;
    u8 hw_ac[IEEE80211_NUM_ACS] = {3, 2, 0, 1};
    struct hgic_txq_param txq;

    if (hg->sta && hg->ap && hgvif == hg->ap) {
        u8 maxusage[2] = {50, 50};
        hgic_fwctrl_set_pool_maxusage(&hg->ctrl, 1, maxusage);
        hgic_dbg("relay mode, ignore ap edca param\r\n");
        return 0;
    } else {
        u8 maxusage[2] = {80, 80};
        hgic_dbg("config txq%d params: aifsn=%d, cw_min=%d, cw_max=%d, txop=%d\r\n",
                 ac, params->aifs, params->cw_min, params->cw_max, params->txop);
        hgic_fwctrl_set_pool_maxusage(&hg->ctrl, 1, maxusage);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
        txq.acm    = params->acm;
#else
        txq.acm    = 0;
#endif
        txq.aifs   = params->aifs;
        txq.txop   = params->txop;
        txq.cw_max = params->cw_max;
        txq.cw_min = params->cw_min;
        return hgic_fwctrl_set_txq_param(&hg->ctrl, hgvif?hgvif->idx:1, hw_ac[ac], &txq);
    }
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
static void hgics_ops_tx(struct ieee80211_hw *hw, struct ieee80211_tx_control *control, struct sk_buff *skb)
#else
static void hgics_ops_tx(struct ieee80211_hw *hw, struct sk_buff *skb)
#endif
{
    struct ieee80211_tx_info *info;
    struct hgic_frm_hdr *frmhdr = NULL;
    struct hgics_wdev   *hg  = (struct hgics_wdev *)hw->priv;
    u8 pad = ((u32)skb->data & 0x3); //push offset | <--|data .....|
    int ac_num = (skb->protocol == cpu_to_be16(ETH_P_PAE)) ? IEEE80211_AC_VO : priority_to_ac[skb->queue_mapping & 7];
    struct sk_buff_head *txq = &hg->data_txq[ac_num];

    if (!test_bit(HGICS_STATE_START, &hg->state) || skb->len < 10) {
        ieee80211_free_txskb(hw, skb);
        hg->status.tx_drop++;
        return;
    }

    if (txq->qlen > hg->txq_size) {
        ieee80211_free_txskb(hw, skb_dequeue(txq));
        hg->status.tx_drop++;
    }
#ifndef __RTOS__
    hgics_icmp_monitor(hg->ctrl.icmp_mntr, skb, 1);
#endif
    info = IEEE80211_SKB_CB(skb);
    frmhdr = (struct hgic_frm_hdr *)skb_push(skb, sizeof(struct hgic_frm_hdr) + pad);
    memset(frmhdr, 0, sizeof(struct hgic_frm_hdr) + pad);
    frmhdr->hdr.magic  = HGIC_HDR_TX_MAGIC;
    frmhdr->hdr.length = skb->len;
    frmhdr->hdr.type   = HGIC_HDR_TYPE_FRM;
    frmhdr->hdr.ifidx  = 0;
    frmhdr->hdr.flags  = pad;
    frmhdr->tx_info.band   = info->band;
    frmhdr->tx_info.tx_mcs = 0xff;
    frmhdr->tx_info.tx_bw  = 0xff;
    frmhdr->tx_info.tx_flags = cpu_to_le32(info->flags);
    frmhdr->tx_info.tx_flags2 = 0;
    skb_queue_tail(txq, skb);
    queue_work(hg->tx_wq, &hg->tx_work);
    return;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0)
static int hgics_ops_ampdu_action(struct ieee80211_hw *hw,
                                  struct ieee80211_vif *vif,
                                  struct ieee80211_ampdu_params *params)
{
    return 0;
}
#else
static int hgics_ops_ampdu_action(struct ieee80211_hw *hw,
                                  struct ieee80211_vif *vif,
                                  enum ieee80211_ampdu_mlme_action action,
                                  struct ieee80211_sta *sta, u16 tid, u16 *ssn,
                                  u8 buf_size)
{
    return 0;
}
#endif

static struct ieee80211_ops hgics_ops = {
    .tx = hgics_ops_tx,
    .start = hgics_ops_start,
    .stop = hgics_ops_stop,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
    .start_ap = hgics_ops_start_ap,
    .stop_ap = hgics_ops_stop_ap,
#endif
    .add_interface = hgics_ops_add_interface,
    .remove_interface = hgics_ops_remove_interface,
    .change_interface = hgics_ops_change_interface,
    .config = hgics_ops_config,
    .bss_info_changed = hgics_ops_bss_info_changed,
    .configure_filter = hgics_ops_configure_filter,
    .sta_add = hgics_ops_sta_add,
    .sta_remove = hgics_ops_sta_remove,
    .sta_notify = hgics_ops_sta_notify,
    .set_rts_threshold = hgics_ops_set_rts_threshold,
    .set_key = hgics_ops_set_key,
    .conf_tx = hgics_ops_conf_tx,
    .ampdu_action = hgics_ops_ampdu_action,
};

static int hgics_request_txwnd(struct hgic_bus *bus)
{
    struct hgic_hdr *hdr = NULL;
    struct sk_buff *skb = dev_alloc_skb(sizeof(struct hgic_hdr) + 2);

    if (skb == NULL) {
        return -ENOMEM;
    }

    hdr = (struct hgic_hdr *)skb->data;
    hdr->magic  = HGIC_HDR_TX_MAGIC;
    hdr->length = sizeof(struct hgic_hdr) + 2;
    hdr->type   = HGIC_HDR_TYPE_SOFTFC;
    hdr->ifidx  = 0;
    hdr->flags  = 0;
    hdr->cookie = 0;
    skb_put(skb, sizeof(struct hgic_hdr) + 2);
    hgic_hdr_format(hdr, 1);
    return bus->tx_packet(bus, skb);
}

static int hgics_check_txwnd(struct hgics_wdev *hg, int min)
{
    int err  = 0;

    if (!hg->soft_fc || test_bit(HGIC_BUS_FLAGS_INBOOT, &hg->bus->flags)) {

        return 0;
    }

    while (hg->soft_fc && atomic_read(&hg->txwnd) < min && test_bit(HGICS_STATE_START, &hg->state) &&
           !test_bit(HGIC_BUS_FLAGS_INBOOT, &hg->bus->flags)) {

        err = hgics_request_txwnd(hg->bus);
        if (!err) {
            wait_for_completion_timeout(&hg->txwnd_cp, msecs_to_jiffies(10));
        }

        if (atomic_read(&hg->txwnd) < min) {
            wait_for_completion_timeout(&hg->txwnd_cp, msecs_to_jiffies(10));
        }
    }

    if (!hg->soft_fc || test_bit(HGIC_BUS_FLAGS_INBOOT, &hg->bus->flags)) {
        return 0;
    }

    if (atomic_read(&hg->txwnd) < min) {
        //hgic_err("Error:No tx wnd:%d\n",atomic_read(&hg->txwnd));
        return -1;
    } else {
        atomic_dec(&hg->txwnd);
    }
    return 0;
}

static void hgics_test_work(struct work_struct *work)
{
    int ret    = 0;
    u32 diff_jiff = 0;
    struct hgics_wdev *hg  = NULL;
    struct sk_buff    *skb = NULL;
    struct hgic_frm_hdr *frmhdr = NULL;

    hgic_dbg("start if test ...\r\n");
    hg = container_of(work, struct hgics_wdev, test_work);
    hg->test_jiff = jiffies;
    set_bit(HGICS_STATE_START, &hg->state);

    while (!test_bit(HGICS_STATE_REMOVE, &hg->state)) {
        if (time_after(jiffies, hg->test_jiff + msecs_to_jiffies(5000))) {
            diff_jiff  = jiffies_to_msecs(jiffies - hg->test_jiff);
            diff_jiff /= 1000;
            if (diff_jiff == 0) {
                diff_jiff = 0xffff;
            }

            hgic_dbg("HGIC IF TEST: tx:%d KB/s, rx:%d KB/s (%d %d %d)\r\n",
                   (hg->test_tx_len / 1024) / diff_jiff,
                   (hg->test_rx_len / 1024) / diff_jiff,
                   hg->test_tx_len, hg->test_rx_len, diff_jiff);
            hg->test_rx_len = 0;
            hg->test_tx_len = 0;
            hg->test_jiff = jiffies;
        }
        skb = dev_alloc_skb(1536 + hg->bus->drv_tx_headroom);
        if (skb) {
            skb_reserve(skb, hg->bus->drv_tx_headroom);
            frmhdr = (struct hgic_frm_hdr *)skb->data;
            frmhdr->hdr.magic  = cpu_to_le16(HGIC_HDR_TX_MAGIC);
            frmhdr->hdr.length = cpu_to_le16(1536);
            frmhdr->hdr.cookie = cpu_to_le16(hgics_data_cookie(hg));
            frmhdr->hdr.type   = (hg->if_test == 1 ? HGIC_HDR_TYPE_TEST : HGIC_HDR_TYPE_TEST2);
            frmhdr->hdr.ifidx  = 0;
            frmhdr->hdr.flags  = 0;
            if (hg->if_test == 3) {
                int i = 0;
                for (i = 8; i < 1536; i++) {
                    skb->data[i] = (i & 1) ? 0x55 : 0xAA;
                }
            }
            skb_put(skb, 1536);
            while ((!test_bit(HGICS_STATE_REMOVE, &hg->state)) && hgics_check_txwnd(hg, TXWND_INIT_VAL)) {
                msleep(10);
            }
            ret = hg->bus->tx_packet(hg->bus, skb);
            if (ret) {
                msleep(10);
            }
        } else {
            msleep(10);
        }
    }
    hgic_dbg("if test stop!\r\n");
}

static void hgics_tx_complete(void *hgobj, struct sk_buff *skb, int success)
{
    struct hgics_wdev   *hg       = (struct hgics_wdev *)hgobj;
    struct hgic_frm_hdr *frmhdr   = (struct hgic_frm_hdr *)skb->data;
    struct ieee80211_tx_info *txi = IEEE80211_SKB_CB(skb);

    if (hg->if_test) {
        hg->test_tx_len += skb->len;
        dev_kfree_skb_any(skb);
        return;
    }

    hgic_hdr_format((struct hgic_hdr *)frmhdr, 0);
    if (success) {
        hg->status.tx_ok++;
        clear_bit(HGIC_BUS_FLAGS_ERROR, &hg->bus->flags);
    }else{
        hg->status.tx_fail++;
        set_bit(HGIC_BUS_FLAGS_ERROR, &hg->bus->flags);
        printk("tx fail: %d\r\n", frmhdr->hdr.cookie);
        if (!test_bit(HGICS_STATE_REMOVE, &hg->state)) {
            schedule_work(&hg->detect_work);
        }
    }

    if (frmhdr->hdr.magic == HGIC_HDR_TX_MAGIC && frmhdr->hdr.type == HGIC_HDR_TYPE_FRM) {
        if (hgics_dack) {
            skb_unlink(skb, &hg->trans_q);
            if (success && !(txi->flags & IEEE80211_TX_CTL_NO_ACK)) {
                skb_queue_tail(&hg->ack_q, skb);
            } else {
                skb_pull(skb, sizeof(struct hgic_frm_hdr) + frmhdr->hdr.flags);
                ieee80211_tx_status_irqsafe(hg->hw, skb);
            }
        } else {
            txi->flags |= IEEE80211_TX_STAT_ACK;
            skb_pull(skb, sizeof(struct hgic_frm_hdr) + frmhdr->hdr.flags);
            ieee80211_tx_status_irqsafe(hg->hw, skb);
        }
    } else {
        dev_kfree_skb_any(skb);
    }
}

static int hgics_tx_data_skb(struct hgics_wdev *hg, struct sk_buff *skb, int tx_len)
{
    int ret = 0;

    hg->txpd_skb = NULL;
    hgic_hdr_format((struct hgic_hdr *)skb->data, 1);
    ret = hg->bus->tx_packet(hg->bus, skb);
    if (ret == -EBUSY) {
        hg->txpd_skb = skb;
        return -1;
    } else {
        hg->tx_bytes += tx_len;
    }
    return 0;
}

static int hgics_tx_single_frm(struct hgics_wdev *hg, struct sk_buff *skb)
{
    struct hgic_frm_hdr *hdr = (struct hgic_frm_hdr *)skb->data;
    if (hgics_dack) {
        skb_queue_tail(&hg->trans_q, skb);
    }
    hdr->hdr.cookie = hgics_data_cookie(hg);
    return hgics_tx_data_skb(hg, skb, skb->len - sizeof(struct hgic_frm_hdr));
}

static int hgics_tx_agg_frm(struct hgics_wdev *hg, int txq, struct sk_buff *skb)
{
    struct sk_buff *agg_skb;
    struct hgic_frm_hdr *hdr = NULL;
    struct hgic_hdr *agghdr = NULL;
    int cpylen = 0;
    int datalen = 0;

    agg_skb = dev_alloc_skb(hg->bus->drv_tx_headroom + if_agg);
    if (agg_skb) {
        skb_reserve(agg_skb, hg->bus->drv_tx_headroom);
        agghdr = (struct hgic_hdr *)agg_skb->data;
        memset(agghdr, 0, sizeof(struct hgic_hdr));
        agghdr->magic  = HGIC_HDR_TX_MAGIC;
        agghdr->type   = HGIC_HDR_TYPE_AGGFRM;
        agghdr->length = sizeof(struct hgic_hdr);
        agghdr->cookie = hgics_data_cookie(hg);

        do {
            hdr = (struct hgic_frm_hdr *)skb->data;
            cpylen = hdr->hdr.length;
            datalen += cpylen - sizeof(struct hgic_frm_hdr);
            if (agghdr->length + ALIGN(cpylen, 4) > if_agg) {
                skb_queue_head(&hg->data_txq[txq], skb);
                break;
            }

            hdr->hdr.cookie = hgics_data_cookie(hg);
            hdr->hdr.length = ALIGN(cpylen, 4);
            agghdr->length += hdr->hdr.length;
            hgic_hdr_format((struct hgic_hdr *)hdr, 1);
            memcpy(agg_skb->data + agghdr->length, skb->data, cpylen);
            hgics_tx_complete(hg, skb, 1);

            skb = skb_dequeue(&hg->data_txq[txq]);
        } while (skb);

        if (agghdr->length > sizeof(struct hgic_frm_hdr)) {
            skb_put(agg_skb, agghdr->length);
            return hgics_tx_data_skb(hg, agg_skb, datalen);
        } else {
            hgic_err("invalid agg frm, cookie:%d\r\n", agghdr->cookie);
            dev_kfree_skb(agg_skb);
            return 0;
        }
    } else {
        return hgics_tx_single_frm(hg, skb);
    }
}

static void hgics_throughput_print(struct hgics_wdev *hg)
{
    ulong tx_Kbytes, rx_Kbytes;
    u32 diff_jiff = jiffies_to_msecs(jiffies - hg->throughput_jiff);
    if (hg->ctrl.throughput && diff_jiff >= 1000 * hg->ctrl.throughput) {
        diff_jiff /= 1000;
        tx_Kbytes  = hg->tx_bytes / 1024;
        rx_Kbytes  = hg->rx_bytes / 1024;
        hg->tx_bytes = 0;
        hg->rx_bytes = 0;
        hg->throughput_jiff = jiffies;
        printk("hgics throughput: TX: %luKB/s, RX: %luKB/s [%d,%d,%d,%d]\r\n",
               tx_Kbytes / diff_jiff, rx_Kbytes / diff_jiff,
               hg->data_txq[0].qlen, hg->data_txq[1].qlen,
               hg->data_txq[2].qlen, hg->data_txq[3].qlen);
    }
}

//static int hgics_tx_work_dump = 0;
static void hgics_tx_work(struct work_struct *work)
{
    int i      = 0;
    int exit   = 0;
    int tx_cnt = 0;
    struct sk_buff    *skb = NULL;
    struct hgic_hdr   *hdr = NULL;
    struct hgics_wdev *hg  = container_of(work, struct hgics_wdev, tx_work);

    //hgic_enter();
    //if(hgics_tx_work_dump == 0) { dump_stack(); hgics_tx_work_dump=1; }

    if (test_bit(HGICS_STATE_REMOVE, &hg->state)) {
        return;
    }

    while (!exit) {
        exit = 1;
        for (i = 0; i < IEEE80211_NUM_ACS && test_bit(HGICS_STATE_START, &hg->state); i++) {
            tx_cnt = 0;
            while (!skb_queue_empty(&hg->data_txq[i]) && test_bit(HGICS_STATE_START, &hg->state)) {
                exit = 0;
                if (++tx_cnt > 4 - i) {
                    break;
                }

                if (hgics_check_txwnd(hg, TXWND_INIT_VAL + 1)) {
                    return;
                }

                if (!test_bit(HGICS_STATE_START, &hg->state)  || hg->fw_state != STATE_FW ||
                    test_bit(HGIC_BUS_FLAGS_INBOOT, &hg->bus->flags)) {
                    hgic_clear_queue(&hg->data_txq[i]);
                    continue;
                }

                skb = skb_dequeue(&hg->data_txq[i]);
                if (skb) {
                    if (!skb_queue_empty(&hg->data_txq[i]) && if_agg) {
                        hgics_tx_agg_frm(hg, i, skb);
                    } else {
                        hgics_tx_single_frm(hg, skb);
                    }
                }
            }
        }
    }

    hgics_throughput_print(hg);

    while (skb_queue_len(&hg->ack_q) > 128) {
        skb = skb_dequeue(&hg->ack_q);
        if (skb) {
            hdr = (struct hgic_hdr *)skb->data;
            skb_pull(skb, sizeof(struct hgic_frm_hdr) + hdr->flags);
            ieee80211_tx_status_irqsafe(hg->hw, skb);
        }
    }


    //hgic_leave();
}

static int hgics_core_init(struct hgics_wdev *hg)
{
    int  err = 0;
    struct ieee80211_hw *hw = hg->hw;

    hgic_dbg("Enter\n");
    err = hg->hghw->ops->init(hg);
    if (err) {
        hgic_err("hgic hwinit faile, ret=%d\r\n", err);
        return -1;
    }

    err = ieee80211_register_hw(hw);
    if (err) {
        hgic_err("ieee80211_register_hw failed, status:%d\n", err);
        return err;
    }

#ifdef CONFIG_BT
    if (hg->hci && hgic_hcidev_init(&hg->ctrl, hg->hci) < 0) {
        hci_free_dev(hg->hci);
        hg->hci = NULL;
        hgic_err("HCI dev init fail\r\n");
    }
#endif

    hgics_create_procfs(hg);
    set_bit(HGICS_STATE_INITED, &hg->state);
    hgic_dbg("ok\n");
    return 0;
}

static int hgics_bootdl_start(struct hgics_wdev *hg, int fw_state)
{
    int err    = -1;
    int retry  = 10;

    set_bit(HGIC_BUS_FLAGS_INBOOT, &hg->bus->flags);
    while (fw_state != STATE_FW && retry-- > 0 && err) {
        if(hg->bootdl.fw_info.version) {//fwinfo->version:1:both,0:need
            msleep(100);
        }
        fw_state = hgic_bootdl_cmd_enter(&hg->bootdl);
        if (fw_state == STATE_BOOT) {
            err = hgic_bootdl_download(&hg->bootdl, fw_file);
        }
        if (fw_state < 0 || err) {
            msleep(10);
        }
    }
    clear_bit(HGIC_BUS_FLAGS_INBOOT, &hg->bus->flags);
    return fw_state;
}

static int hgics_bootdl_check_reboot(struct hgics_wdev *hg, int fw_state)
{
    int retry = 10;
    hgic_dbg("Bootdownload %s\n", hg->bootdl.fw_info.version ? "No Reset" : "Reset");
    if (fw_state != STATE_FW && hg->bootdl.fw_info.version != 0) { //fwinfo->version:1:both,1:need
        set_bit(HGIC_BUS_FLAGS_INBOOT, &hg->bus->flags);
        while (fw_state != STATE_FW && retry-- > 0 && !test_bit(HGICS_STATE_REMOVE, &hg->state)) {
            if(hg->bootdl.fw_info.version) {//fwinfo->version:1:both,0:need
                msleep(1000);
            }
            hgic_dbg("Enter!\n");
            fw_state = hgic_bootdl_cmd_enter(&hg->bootdl);
        }
        clear_bit(HGIC_BUS_FLAGS_INBOOT, &hg->bus->flags);
    }
    return fw_state;
}

static int hgics_bootdl_check_reinit(struct hgics_wdev *hg, int fw_state)
{
    int retry = 10;

    if (fw_state != STATE_FW && hg->bus->reinit && test_bit(HGIC_BUS_FLAGS_NOPOLL, &hg->bus->flags)) {
        while (retry-- > 0 && !test_bit(HGICS_STATE_REMOVE, &hg->state)) {
            msleep(100);
            if (!hg->bus->reinit(hg->bus)) {
                hgic_dbg("Enter!\n");
                fw_state = hgic_bootdl_cmd_enter(&hg->bootdl);
                break;
            }
        }
    }

    mod_timer(&hg->detect_tmr, jiffies + msecs_to_jiffies(HGIC_DETECT_TIMER));
    return fw_state;
}

static int hgics_download_fw(struct hgics_wdev *hg)
{
    int status  = no_bootdl ? STATE_FW : -1;
    status = hgics_bootdl_start(hg, status);
    status = hgics_bootdl_check_reboot(hg, status);
    status = hgics_bootdl_check_reinit(hg, status);
    hg->fw_state = status;
    hgic_dbg("Leave, status:%d\n", status);
    return (status == STATE_FW);
}

static void hgics_refresh_mac(struct hgics_wdev *hg)
{
    if(hg->sta){
        hgic_fwctrl_set_mac(&hg->ctrl, hg->sta->idx, hg->sta->vif->addr);
    }
    if(hg->ap){
        hgic_fwctrl_set_mac(&hg->ctrl, hg->ap->idx, hg->ap->vif->addr);
    }
    if(hg->p2p){
        hgic_fwctrl_set_mac(&hg->ctrl, hg->p2p->idx, hg->p2p->vif->addr);
    }
}

static void hgics_delay_init(struct work_struct *work)
{
    int ret = 0;
    struct hgics_wdev *hg = container_of(work, struct hgics_wdev, delay_init);

    //dump_stack();
    if (test_bit(HGICS_STATE_REMOVE, &hg->state)) {
        return;
    }

    ret = hgics_download_fw(hg);
    if (ret) {

        hgic_fwctrl_get_fwinfo(&hg->ctrl, HGIC_WDEV_ID_STA, &hg->fwinfo);
        hgic_dbg("hgic fw info:%d.%d.%d.%d, svn version:%d, "MACSTR"\r\n",
               (hg->fwinfo.version >> 24) & 0xff, (hg->fwinfo.version >> 16) & 0xff,
               (hg->fwinfo.version >> 8) & 0xff, (hg->fwinfo.version & 0xff),
               hg->fwinfo.svn_version, MAC2STR(hg->fwinfo.mac));

        memcpy(&hgic_fwinfo,&hg->fwinfo,sizeof(struct hgic_fw_info));

        hg->soft_fc = (hg->fwinfo.version < 0x2000000);
        hg->ctrl.fwinfo = &hg->fwinfo;
        if(hg->soft_fc){
            set_bit(HGIC_BUS_FLAGS_SOFTFC, &hg->bus->flags);
        }else{
            clear_bit(HGIC_BUS_FLAGS_SOFTFC, &hg->bus->flags);
        }
        if (test_bit(HGICS_STATE_INITED, &hg->state)) {
            hgics_refresh_mac(hg);
            if(hg->radio_off == 0){
                hgics_ops_start(hg->hw);
            }
        } else {
            if (is_zero_ether_addr(hg->fwinfo.mac)) {
                memcpy(hg->fwinfo.mac, default_mac, 6);
                hgic_err("fwinfo mac is Zero, use default mac address\r\n");
            }
            //memcpy(hg->hw->wiphy->perm_addr, hg->fwinfo.mac, 6);
            ret = hgics_core_init(hg);
            if (!ret) {
                hgic_dbg("hgics core init done!\r\n");
            }

            if (hg->if_test) {
                queue_work(hg->tx_wq, &hg->test_work);
            }
        }
#ifdef __RTOS__
        if (init_cb) init_cb(0);
#endif
    }
    hgic_dbg("Leave, %s:ret=%d, soft_fc=%d\n", ret? "FAILED" : "SUCCESS",
            ret, hg->soft_fc);
}

//static int hgics_detect_work_dump = 0;
static void hgics_detect_work(struct work_struct *work)
{
    int status = -1;
    int retry = 4;
    struct hgics_wdev *hg  = container_of(work, struct hgics_wdev, detect_work);

    //if(hgics_detect_work_dump == 0) { dump_stack(); hgics_detect_work_dump=1; }
    hg->status.detect_tmr++;
    if (!test_bit(HGIC_BUS_FLAGS_SLEEP, &hg->bus->flags) && !test_bit(HGICS_STATE_REMOVE, &hg->state)) {
        if (test_bit(HGIC_BUS_FLAGS_ERROR, &hg->bus->flags) ||
            time_after(jiffies, hg->last_rx + msecs_to_jiffies(HGIC_DETECT_TIMER))) {
            while (retry-- > 0 && status != STATE_FW) {
                status = hgic_bootdl_cmd_enter(&hg->bootdl);
            }

            //hgic_dbg("status=%d, fw_state=%d, REMOVE:%d\r\n", status, hg->fw_state, test_bit(HGICS_STATE_REMOVE, &hg->state));
            if (status != STATE_FW || hg->fw_state != STATE_FW) {
                hgic_err("need reload firmware ...\r\n");
                clear_bit(HGICS_STATE_START, &hg->state);
                hgic_clear_queue(&hg->ctrl.txq);
                hgic_clear_queue(&hg->data_txq[0]);
                hgic_clear_queue(&hg->data_txq[1]);
                hgic_clear_queue(&hg->data_txq[2]);
                hgic_clear_queue(&hg->data_txq[3]);
                hgic_clear_queue(&hg->trans_q);
                hgic_clear_queue(&hg->ack_q);
                if (hg->bus->reinit) {
                    hg->bus->reinit(hg->bus);
                }
                if (!test_bit(HGICS_STATE_REMOVE, &hg->state)) {
                    hg->soft_fc = 0;
                    hg->fw_state = -1;
                    schedule_work(&hg->delay_init);
                }
            }
        }
    }

    if (!test_bit(HGICS_STATE_REMOVE, &hg->state)) {
        mod_timer(&hg->detect_tmr, jiffies + msecs_to_jiffies(HGIC_DETECT_TIMER));
    }
    //hgic_dbg("Leave detect work\n");
}

#if !defined(__RTOS__) && LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
static void hgics_detect_timer(struct timer_list *t)
{
    struct hgics_wdev *hg = from_timer(hg, t, detect_tmr);
    if (!test_bit(HGICS_STATE_REMOVE, &hg->state)) {
        schedule_work(&hg->detect_work);
    }
}
#else
static void hgics_detect_timer(unsigned long arg)
{
    struct hgics_wdev *hg = (struct hgics_wdev *) arg;
    if (!hg) {
        hgic_err("Input param error!\n");
        return;
    }
    if (!test_bit(HGICS_STATE_REMOVE, &hg->state)) {
        schedule_work(&hg->detect_work);
    } else {
        hgic_err("Error state!\n");
    }
}
#endif

static void hgics_probe_post(void *priv)
{
    struct hgics_wdev *hg  = (struct hgics_wdev *)priv;
    schedule_work(&hg->delay_init);
}

static void hgics_core_remove(void *arg)
{
    struct hgics_wdev *hg = (struct hgics_wdev *)arg;
    if (hg) {
        hgic_dbg("Enter\r\n");
        hg->ctrl.remove = 1;
        set_bit(HGICS_STATE_REMOVE, &hg->state);
        clear_bit(HGICS_STATE_START, &hg->state);

        hgic_dbg(" trace ...\r\n");
        cancel_work_sync(&hg->delay_init);
        hgic_dbg(" trace ...\r\n");
        cancel_work_sync(&hg->detect_work);
        hgic_dbg(" trace ...\r\n");
        cancel_work_sync(&hg->tx_work);
        hgic_dbg(" trace ...\r\n");
        cancel_work_sync(&hg->test_work);

        hgic_dbg(" trace ...\r\n");
        del_timer_sync(&hg->detect_tmr);
        hgic_dbg(" trace ...\r\n");
        del_timer_sync(&hg->beacon_timer);

        hgic_dbg(" trace ...\r\n");
        cancel_work_sync(&hg->delay_init);
        hgic_dbg(" trace ...\r\n");
        cancel_work_sync(&hg->detect_work);
        hgic_dbg(" trace ...\r\n");
        cancel_work_sync(&hg->tx_work);
        hgic_dbg(" trace ...\r\n");
        cancel_work_sync(&hg->test_work);
        hgic_dbg(" trace ...\r\n");
        if (test_bit(HGICS_STATE_INITED, &hg->state)) {
            ieee80211_unregister_hw(hg->hw);
        }

        hgic_dbg(" trace ...\r\n");
        hgic_fwctrl_release(&hg->ctrl);
        hgic_dbg(" trace ...\r\n");
        hgic_ota_release(&hg->ota);
        hgic_dbg(" trace ...\r\n");
        hgic_bootdl_release(&hg->bootdl, 0);
        hgic_dbg(" trace ...\r\n");
        hgics_delete_procfs(hg);
        hgic_dbg(" trace ...\r\n");
        if(hg->hghw){
            hg->hghw->ops->free(hg);
        }
        hgic_dbg("trace ...\r\n");
        if (hg->tx_wq) {
            flush_workqueue(hg->tx_wq);
            destroy_workqueue(hg->tx_wq);
        }
        hgic_dbg(" trace ...\r\n");
        hgic_clear_queue(&hg->data_txq[0]);
        hgic_clear_queue(&hg->data_txq[1]);
        hgic_clear_queue(&hg->data_txq[2]);
        hgic_clear_queue(&hg->data_txq[3]);
        hgic_clear_queue(&hg->trans_q);
        hgic_clear_queue(&hg->ack_q);
        hgic_dbg("trace ...\r\n");

#ifdef CONFIG_BT
        if (hg->hci) {
            hci_free_dev(hg->hci);
        }
#endif
        hgic_dbg(" trace ...\r\n");

#ifdef __RTOS__
        skb_queue_head_deinit(&hg->data_txq[0]);
        skb_queue_head_deinit(&hg->data_txq[1]);
        skb_queue_head_deinit(&hg->data_txq[2]);
        skb_queue_head_deinit(&hg->data_txq[3]);
        skb_queue_head_deinit(&hg->trans_q);
        skb_queue_head_deinit(&hg->ack_q);
        deinit_completion(&hg->txwnd_cp);
        spin_lock_deinit(&hg->lock);
        if (hg->conf) {
            hgics_configs_put(hg->conf);
        }
#else
        hgic_clear_queue(&hg->evt_list);
        if (hg->conf) {
            kfree(hg->conf);
        }
#endif

        if (hg->txpd_skb) {
            kfree_skb(hg->txpd_skb);
        }
        hgics_flag_del(hg->hw->wiphy);
        ieee80211_free_hw(hg->hw);
        hgic_dbg("Leave\n");
    }
}

static int hgics_core_probe(void *dev, struct hgic_bus *bus)
{
    int i = 0;
    struct hgics_wdev   *hg = NULL;
    struct ieee80211_hw *hw = NULL;

    BUG_ON(bus->tx_ctrl == NULL || bus->tx_packet == NULL);
    hw = ieee80211_alloc_hw(sizeof(struct hgics_wdev), &hgics_ops);
    if (!hw) {
        hgic_err("ieee80211_alloc_hw failed!!\n");
        return -1;
    }

    set_wiphy_dev(hw->wiphy, dev);
    memset(hw->priv, 0, sizeof(struct hgics_wdev));
    hg = (struct hgics_wdev *)hw->priv;
    memset(hg, 0, sizeof(struct hgics_wdev));
    put_unaligned_le32(0xD8833253, hg->magic);
    hg->dev       = dev;
    hg->hw        = hw;
    hg->bus       = bus;
    hg->txq_size  = txq_size;
    hg->dev_id    = bus->dev_id;
    hg->conf_file = conf_file;
    hg->bt_en     = bt_en;
    hg->if_test   = if_test;
    hg->fw_state  = -1;
    hg->hghw      = hgics_hw_match(hg->dev_id);
    if (hg->hghw == NULL) {
        goto __probe_failed;
    }

    hgic_fwctrl_init(&hg->ctrl, dev, bus);
    hg->ctrl.priv = hg;
    hg->ctrl.rx_event = hgics_rx_fw_event;
    hg->ctrl.data_cookie = hgics_data_cookie;
    hgic_ota_init(&hg->ota, &hg->ctrl, &hg->fwinfo);
    spin_lock_init(&hg->lock);
    skb_queue_head_init(&hg->trans_q);
    skb_queue_head_init(&hg->ack_q);
    init_completion(&hg->txwnd_cp);
    atomic_set(&hg->txwnd, TXWND_INIT_VAL);
    for (i = 0; i < IEEE80211_NUM_ACS; i++) {
        skb_queue_head_init(&hg->data_txq[i]);
    }

    INIT_WORK(&hg->tx_work, hgics_tx_work);
    INIT_WORK(&hg->delay_init, hgics_delay_init);
    INIT_WORK(&hg->detect_work, hgics_detect_work);
    INIT_WORK(&hg->test_work, hgics_test_work);
    init_timer(&hg->detect_tmr);
    init_timer(&hg->beacon_timer);
    setup_timer(&hg->detect_tmr, hgics_detect_timer, (unsigned long)hg);
    setup_timer(&hg->beacon_timer, hgics_mac80211_beacon, (unsigned long)hg);
    hgic_bootdl_init(&hg->bootdl, hg->bus, &hg->ctrl);

    hw->extra_tx_headroom  = sizeof(struct hgic_frm_hdr) + hg->bus->drv_tx_headroom + 4;
    hw->wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;
    hw->wiphy->addresses   = hg->macaddr;
#ifdef __RTOS__
    hg->tx_wq = ALLOC_ORDERED_WORKQUEUE("hgics_wkq", 4096);
#else
    hg->tx_wq = ALLOC_ORDERED_WORKQUEUE("hgics_wkq", 0);
#endif
    if (!hg->tx_wq) {
        goto __probe_failed;
    }

#ifdef __RTOS__
    hg->conf = hgics_configs_get();
#else
    skb_queue_head_init(&hg->evt_list);
    sema_init(&hg->evt_sema, 0);
    hg->conf = kzalloc(sizeof(struct hgics_config), GFP_KERNEL);
    if (hg->conf == NULL) {
        goto __probe_failed;
    }
#endif

    bus->tx_complete = hgics_tx_complete;
    bus->rx_packet   = hgics_rx_data;
    bus->probe_post  = hgics_probe_post;
    bus->remove      = hgics_core_remove;
    bus->suspend     = NULL;
    bus->resume      = NULL;
    bus->bus_priv    = hg;
    hgics_flag_new(hw->wiphy);

	hgic_dbg("txq_size=%d, if_agg=%d,hg:%p\r\n", hg->txq_size, if_agg,hg);
    return 0;

__probe_failed:
    hgic_err("hgics probe failed!\n");
    hgics_core_remove(hg);
    return -1;
}

#ifdef __linux__
static int hgics_netdev_notifier_call(struct notifier_block *nb, unsigned long state, void *ndev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,18,21)
    struct net_device *dev = netdev_notifier_info_to_dev(ndev);
#else
    struct net_device *dev = (struct net_device *)ndev;
#endif

    if (dev && dev->ieee80211_ptr && hgics_flag_check(dev->ieee80211_ptr->wiphy)) {
        switch (state) {
            case NETDEV_REGISTER:
#if defined(CONFIG_HGIC_STABR)
                hgic_stabr_attach(dev);
#endif

                //if(ifname){
                //    dev_change_name(dev, ifname);
                //}
                break;
            default:
                break;
        }
    }
    return NOTIFY_DONE;
}

static struct notifier_block hgics_netdev_notifier = {
    .notifier_call = hgics_netdev_notifier_call,
};
#endif

int __init hgics_init(void)
{
    VERSOIN_SHOW("smac");
    hgic_dbg("Enter,Hugeic wifi build time,%s,%s\n",__DATE__,__TIME__);

#if defined(__RTOS__) || defined(CONFIG_HGIC_AH)
#ifdef __RTOS__
    os_timer_mgr_init();
    rcu_init();
    tasklet_core_init();
    net_device_init();
#endif
    cfg80211_init();
    ieee80211_init();
#ifdef __RTOS__
    hglink_init();
    hgloop_init();
    hgics_xmit_init();
#endif
#endif

#ifdef CONFIG_HGIC_USB
    hgic_usb_init(hgics_core_probe, if_agg);
#endif
#ifdef CONFIG_HGIC_SDIO
    hgic_sdio_init(hgics_core_probe, if_agg);
#endif

#ifdef __linux__
    register_netdevice_notifier(&hgics_netdev_notifier);
#endif

#ifdef CONFIG_HGIC_STABR
    hgic_stabr_init();
#endif

    hgic_dbg("Leave\n");
    return 0;
}

void __exit hgics_exit(void)
{
    hgic_dbg("Enter\n");

#ifdef CONFIG_HGIC_STABR
    hgic_stabr_release();
#endif
#ifdef __linux__
    unregister_netdevice_notifier(&hgics_netdev_notifier);
#endif

#ifdef CONFIG_HGIC_USB
    hgic_usb_exit();
#endif

#ifdef CONFIG_HGIC_SDIO
    hgic_sdio_exit();
#endif

#if defined(__RTOS__) || defined(CONFIG_HGIC_AH)
#ifdef __RTOS__
    hgics_xmit_deinit();
    hgloop_exit();
    hglink_exit();
#endif
    ieee80211_exit();
    cfg80211_exit();
#ifdef __RTOS__
    net_device_exit();
    tasklet_core_exit();
    os_timer_mgr_delete();
#endif
#endif

    hgic_dbg("Leave\n");
}
/*
void hgics_umac_stop(void)
{
    hgloop_exit();
    hglink_exit();
}

void hgics_umac_init(void)
{
    hglink_init();
    hgloop_init();
}
*/
#ifdef __RTOS__
int hgic_wakeup_detect_timer(void *arg)
{
    if(!arg) {
        hgic_err("Input param error!\n");
        return -1;
    }
    hgics_detect_timer(arg);
    return 0;
}

void hgic_state_remove(void *arg)
{
    struct hgics_wdev *hg = (struct hgics_wdev *) arg;
    if (hg) {
        set_bit(HGICS_STATE_REMOVE, &hg->state);
        clear_bit(HGICS_STATE_START, &hg->state);
    }
}

void hgic_state_wakeup(void *arg)
{
    struct hgics_wdev *hg = (struct hgics_wdev *) arg;
    //hgic_dbg("Enter!\n");
    if (!hg) {
        hgic_err("Input param error!\n");
        return;
    } else {
        clear_bit(HGICS_STATE_REMOVE, &hg->state);
        set_bit(HGICS_STATE_START, &hg->state);
    }
    hgic_dbg("Leave!\n");
}

int hgics_cmd(char *ifname, unsigned int cmd, unsigned int param1, unsigned int param2)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev == NULL) {
        hgic_err("Can not get ndev!\n");
        return -ENODEV;
    }
    if(ndev->ctrl == NULL) {
        hgic_err("Can not get ndev ctrl!\n");
        return -ENODEV;
    }
    return hgic_ioctl(ndev->ctrl, ndev->fwifidx, cmd, param1, param2);
}

const char *hgic_param_ifname(const char *name)
{
    if (name) ifname = (char *)name;
    return (const char *)ifname;
}
void hgic_param_iftest(int iftest)
{
    if_test = iftest;
}
char *hgic_param_fwfile(const char *fw)
{
    if (fw) fw_file = fw;
    return fw_file;
}
void hgic_param_initcb(hgic_init_cb cb)
{
    init_cb = cb;
}
void hgic_param_eventcb(hgic_event_cb cb)
{
    event_cb = cb;
}
void hgics_event(struct hgics_wdev *hg, char *ifname, int event, int param1, int param2)
{
    if (event_cb) {
        event_cb(ifname, event, param1, param2);
    }
}
#endif

module_init(hgics_init);
module_exit(hgics_exit);

module_param(txq_size, int, 0);
module_param(conf_file, charp, 0644);
module_param(fw_file, charp, 0644);
module_param(if_test, int, 0);
module_param(hgics_dack, int, 0);
module_param(no_bootdl, int, 0);
module_param(if_agg, int, 0);
module_param(bt_en, int, 0);
module_param(icmp_mntr, int, 1);

MODULE_DESCRIPTION("HUGE-IC SoftMAC WLAN Driver");
MODULE_AUTHOR("Dongyun");
MODULE_LICENSE("GPL");

//identical_mac_addr_allowed
//cfg80211_can_use_iftype_chan

