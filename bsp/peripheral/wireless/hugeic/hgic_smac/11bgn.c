#ifdef __RTOS__
#include <linux/types.h>
#include <net/cfg80211.h>
#include <net/mac80211.h>
#include <linux/hrtimer.h>
#else
#include <linux/version.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/net.h>
#include <linux/slab.h>
#include <linux/etherdevice.h>
#endif

#include "hgics.h"

#define CONFIG_HGIC_P2P 

#define CHAN_2G(channel, freqency, chflags)  { \
        .band = IEEE80211_BAND_2GHZ, \
        .center_freq = (freqency), \
        .hw_value = (channel), \
        .flags = chflags, \
        .max_antenna_gain = 0, \
        .max_power = 19, \
    }

#define RATE(rate100m, _flags) { \
        .bitrate = (rate100m), \
        .flags = (_flags), \
        .hw_value = (rate100m / 5), \
    }

static struct ieee80211_channel hgics_ch2g[] = {
    CHAN_2G(1, 2412, 0),
    CHAN_2G(2, 2417, 0),
    CHAN_2G(3, 2422, 0),
    CHAN_2G(4, 2427, 0),
    CHAN_2G(5, 2432, 0),
    CHAN_2G(6, 2437, 0),
    CHAN_2G(7, 2442, 0),
    CHAN_2G(8, 2447, 0),
    CHAN_2G(9, 2452, 0),
    CHAN_2G(10, 2457, 0),
    CHAN_2G(11, 2462, 0),
    CHAN_2G(12, 2467, 0),
    CHAN_2G(13, 2472, 0),
    CHAN_2G(14, 2484, 0)
};

static struct ieee80211_rate hgics_legacy_ratetable[] = {
    RATE(10, 0),
    RATE(20, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(55, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(110, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(60, 0),
    RATE(90, 0),
    RATE(120, 0),
    RATE(180, 0),
    RATE(240, 0),
    RATE(360, 0),
    RATE(480, 0),
    RATE(540, 0),
};

static const struct ieee80211_supported_band hgics_band_2G = {
    .band = IEEE80211_BAND_2GHZ,
    .channels = hgics_ch2g,
    .n_channels = ARRAY_SIZE(hgics_ch2g),
    .bitrates = hgics_legacy_ratetable,
    .n_bitrates = ARRAY_SIZE(hgics_legacy_ratetable),
    .ht_cap = {
        .cap = IEEE80211_HT_CAP_GRN_FLD | IEEE80211_HT_CAP_SGI_20 /* | IEEE80211_HT_CAP_SGI_40 */,
        .ht_supported  = true,
        .ampdu_factor  = IEEE80211_HT_MAX_AMPDU_8K,
        .ampdu_density = 6,
        .mcs = {
            .rx_mask    = {0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            .rx_highest = cpu_to_le16(0),
            .tx_params  = IEEE80211_HT_MCS_TX_DEFINED
        }
    }
};

static const struct ieee80211_iface_limit hgics_iface_limits[] = {
	{
#ifdef CONFIG_HGIC_P2P
		.max = 4,
#else
		.max = 2,
#endif
		.types = BIT(NL80211_IFTYPE_STATION) |
			     BIT(NL80211_IFTYPE_AP)
	},
#ifdef CONFIG_HGIC_P2P	
	{
		.max = 4,
		.types = BIT(NL80211_IFTYPE_P2P_CLIENT) |
			     BIT(NL80211_IFTYPE_P2P_GO)
	},
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_P2P_DEVICE)
	}
#endif
};

static const struct ieee80211_iface_combination hgics_iface_combos[] = {
	{
#ifdef CONFIG_HGIC_P2P	
		 .max_interfaces = 8,
#else
		 .max_interfaces = 2,
#endif
		 .num_different_channels = 1,
		 .n_limits = ARRAY_SIZE(hgics_iface_limits),
		 .limits = hgics_iface_limits
	}
};

static int hgics_bgnops_init(struct hgics_wdev *hg)
{
    int i = 0;
    struct ieee80211_hw *hw = hg->hw;
    struct ieee80211_supported_band *sband = &hg->sbands[IEEE80211_BAND_2GHZ];

    hgic_dbg("Enter\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,14,0)
    hw->channel_change_time = 1;
#endif
    hw->queues = 5;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
    hw->offchannel_tx_hw_queue = 4;
#endif

    hw->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) 
        | BIT(NL80211_IFTYPE_AP)
#ifdef CONFIG_HGIC_P2P
		| BIT(NL80211_IFTYPE_P2P_CLIENT)
		| BIT(NL80211_IFTYPE_P2P_GO)
		| BIT(NL80211_IFTYPE_P2P_DEVICE)
		| BIT(NL80211_IFTYPE_MESH_POINT)
		| BIT(NL80211_IFTYPE_ADHOC)
#endif
		;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,2,0)
    ieee80211_hw_set(hw, SIGNAL_DBM);
    ieee80211_hw_set(hw, AMPDU_AGGREGATION);
    ieee80211_hw_set(hw, TX_AMPDU_SETUP_IN_HW);
    ieee80211_hw_set(hw, QUEUE_CONTROL);
    ieee80211_hw_set(hw, HAS_RATE_CONTROL);
    ieee80211_hw_set(hw, MFP_CAPABLE);
#else
    hw->flags = IEEE80211_HW_SIGNAL_DBM |
                IEEE80211_HW_AMPDU_AGGREGATION | 
                IEEE80211_HW_MFP_CAPABLE |
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
                IEEE80211_HW_TX_AMPDU_SETUP_IN_HW |
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
#endif
                IEEE80211_HW_HAS_RATE_CONTROL;
#endif

    memcpy(sband, &hgics_band_2G, sizeof(struct ieee80211_supported_band));
    hw->wiphy->bands[IEEE80211_BAND_2GHZ] = sband;
    hw->wiphy->iface_combinations = hgics_iface_combos;
    hw->wiphy->n_iface_combinations = ARRAY_SIZE(hgics_iface_combos);
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
    hw->wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
#endif
    hw->vif_data_size = sizeof(struct hgics_vif);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
    hw->chanctx_data_size = sizeof(struct hgics_chanctx_priv);
#endif
    hw->max_rates = 4;
    hw->max_rate_tries = 11;
    hw->max_rx_aggregation_subframes = 8;
    hw->max_tx_aggregation_subframes = 64;

    for (i = 0; i < 2; i++) {
        memcpy(hg->macaddr[i].addr, hg->fwinfo.mac, 6);
        hg->macaddr[i].addr[5] += i;
    }
    hw->wiphy->n_addresses = 2;

#ifdef CONFIG_BT
    if (hg->bt_en && (hg->dev_id == HGIC_WLAN_8400 || hg->dev_id == HGIC_WLAN_8410 
        || hg->dev_id == HGIC_WLAN_8000)) {
        hg->hci = hci_alloc_dev();
        hgic_dbg("bluetooth enabled\r\n");
    }
#endif

    hgic_dbg("Leave\n");
    return 0;
}

static int hgics_bgnops_free(struct hgics_wdev *hg)
{
    return 0;
}

static int hgics_bgnops_create_procfs(struct hgics_wdev *hg)
{
    return 0;
}

static int hgics_bgnops_delete_procfs(struct hgics_wdev *hg)
{
    return 0;
}

static int hgics_bgnops_start(struct ieee80211_hw *hw)
{
    return 0;
}
static int hgics_bgnops_stop(struct ieee80211_hw *hw)
{
    return 0;
}
static int hgics_bgnops_start_ap(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
    return 0;
}
static int hgics_bgnops_stop_ap(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
    return 0;
}
static int hgics_bgnops_add_interface(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
    return 0;
}
static int hgics_bgnops_remove_interface(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
    return 0;
}

struct hgics_hw_ops hgics_hw_bgnops = {
    .init = hgics_bgnops_init,
    .free = hgics_bgnops_free,
    .start = hgics_bgnops_start,
    .stop = hgics_bgnops_stop,
    .start_ap = hgics_bgnops_start_ap,
    .stop_ap = hgics_bgnops_stop_ap,
    .add_interface = hgics_bgnops_add_interface,
    .remove_interface = hgics_bgnops_remove_interface,
    .create_procfs = hgics_bgnops_create_procfs,
    .delete_procfs = hgics_bgnops_delete_procfs,
};

