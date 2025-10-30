
#ifndef _HGIC_SMAC_H_
#define _HGIC_SMAC_H_

#ifndef __RTOS__
#include <linux/version.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/wireless.h>
#endif
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/netdevice.h>

#if !defined(__RTOS__) && defined(CONFIG_HGIC_AH)
#include "umac/ieee80211.h"
#include "umac/mac80211.h"
#include "umac/cfg80211.h"
#else
#include <linux/ieee80211.h>
#include <net/mac80211.h>
#include <net/cfg80211.h>
#endif

#ifdef CONFIG_BT
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#endif

#include "../hgic_def.h"
#include "../utils/utils.h"
#include "../utils/fwdl.h"
#include "../utils/fwctrl.h"
#include "../utils/ota.h"
#include "../utils/iwpriv.h"

struct hgics_wdev;

#include "hw.h"
#include "11ah.h"
#include "procfs.h"

#define SKB_LIFETIME(skb) (*(ulong *)(&((skb)->cb[48-sizeof(ulong)])))

enum hgics_state {
    HGICS_STATE_INITED,
    HGICS_STATE_START,
    HGICS_STATE_REMOVE,
};

enum hgics_vif_state {
    HGICS_VIF_STATE_ADD,
    HGICS_VIF_STATE_BEACON,
};

struct hgics_status {
    u64 tx_fail;
    u64 tx_drop;    
    u64 tx_ok;
    u32 detect_tmr;
};

struct hgics_config {
#ifdef CONFIG_HGIC_AH
    struct hgics_11ah_config ahcfg;
#endif
};

struct hgics_wdev {
    u8 magic[4];
    void *dev;
    spinlock_t lock;
    u16 data_cookie;
    u16 rx_cookie;
    int id, dev_id;
    unsigned long state;
    u8 fw_state;
    struct hgic_fw_info fwinfo;
    struct ieee80211_hw *hw;
    struct hgic_bus *bus;
    struct mac_address macaddr[4];
    struct ieee80211_supported_band sbands[IEEE80211_NUM_BANDS];
    struct hgics_vif *ap;
    struct hgics_vif *sta;
    struct hgics_vif *p2p;
    struct timer_list detect_tmr;
    struct timer_list beacon_timer;
    unsigned long last_rx;
    char *conf_file;
    const struct hgics_hw *hghw;

    int      txq_size;
    struct sk_buff *txpd_skb;
    struct sk_buff_head data_txq[IEEE80211_NUM_ACS];
    struct sk_buff_head trans_q;    /*transferring queue*/
    struct sk_buff_head ack_q;      /*waitting queue for ack*/
    struct work_struct tx_work;
    struct work_struct delay_init;
    struct work_struct  detect_work;
    struct workqueue_struct *tx_wq;

    u32 beacon_int;
    u64 full_beacon_int;
    int power_level;
    u32 fw_freq;

    struct hgics_config *conf;
#ifdef CONFIG_HGIC_AH
    struct hgics_11ah_wdev ahdev;
#endif
    struct hgic_fwctrl  ctrl;
    struct hgic_bootdl  bootdl;
    struct hgics_procfs proc;
    struct hgics_status status;
    struct hgic_ota     ota;

    /*if test*/
    struct work_struct test_work;
    u32 test_rx_len, test_tx_len;
    u32 test_jiff, if_test;

    /* throughput statistic */
    ulong throughput_jiff;
    ulong tx_bytes, rx_bytes;

    /*soft fc*/
    int soft_fc;
    atomic_t txwnd;
    struct completion txwnd_cp;
    u8 radio_off;
    u8 cfg_changed;
    u8 bt_en, icmp_mntr;

#ifndef __RTOS__
    struct sk_buff_head evt_list;
    struct semaphore    evt_sema;
#endif

#ifdef CONFIG_BT
    struct hci_dev *hci;
#endif
};


struct hgics_vif {
    enum nl80211_iftype type;
    struct ieee80211_vif *vif;
    unsigned long      state;
    struct net_device *ndev;
    struct hgics_wdev *hg;
    u8 idx;
};

struct hgics_chanctx_priv {
};

struct hgics_txstatus {
    u16 cookie;
    u16 seq;
    u8 phy_stat;        /* PHY TX status */
    u8 frame_count;     /* Frame transmit count */
    u8 rts_count;       /* RTS transmit count */
    u8 supp_reason;     /* Suppression reason */
    u8 pm_indicated;    /* PM mode indicated to AP */
    u8 intermediate;    /* Intermediate status notification (not final) */
    u8 for_ampdu;       /* Status is for an AMPDU (afterburner) */
    u8 acked;           /* Wireless ACK received */
};

int hgics_rx_data(void *hgobj, u8 *data, int len);

#ifdef CONFIG_BT
int hgic_hcidev_init(struct hgic_fwctrl *ctrl, struct hci_dev *hci);
#endif

#endif

