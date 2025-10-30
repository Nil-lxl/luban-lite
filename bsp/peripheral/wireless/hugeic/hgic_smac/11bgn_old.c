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
#include "11bgn.h"

#define CHAN_2G(channel, freqency, chflags)  { \
        .band = NL80211_BAND_2GHZ, \
        .center_freq = (freqency), \
        .hw_value = (channel), \
        .flags = chflags, \
        .max_antenna_gain = 0, \
        .max_power = 19, \
    }

#define CHAN_5G(channel, chflags)  { \
        .band = NL80211_BAND_5GHZ, \
        .center_freq = 5000 + 5*(channel), \
        .hw_value = (channel), \
        .flags = chflags, \
        .max_antenna_gain = 0, \
        .max_power = 21, \
    }

#define RATE(rate100m, _flags) { \
        .bitrate = (rate100m), \
        .flags = (_flags), \
        .hw_value = (rate100m / 5), \
    }


static struct ieee80211_channel hgics_ch2g[] = {
    CHAN_2G(1, 2412, IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_2G(2, 2417, IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_2G(3, 2422, IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_2G(4, 2427, IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_2G(5, 2432, 0),
    CHAN_2G(6, 2437, 0),
    CHAN_2G(7, 2442, 0),
    CHAN_2G(8, 2447, IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_2G(9, 2452, IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_2G(10, 2457, IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_2G(11, 2462, IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_2G(12, 2467, IEEE80211_CHAN_NO_IR |  IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_2G(13, 2472, IEEE80211_CHAN_NO_IR |  IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_2G(14, 2484, IEEE80211_CHAN_NO_IR | 
                      IEEE80211_CHAN_NO_HT40PLUS | IEEE80211_CHAN_NO_HT40MINUS | IEEE80211_CHAN_NO_OFDM)
};

static struct ieee80211_channel hgics_ch5g[] = {
    /* UNII-1 */
    CHAN_5G(36, IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_5G(40, IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_5G(44, IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_5G(48, IEEE80211_CHAN_NO_HT40PLUS),
    /* UNII-2 */
    CHAN_5G(52, IEEE80211_CHAN_RADAR | 
                IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_5G(56, IEEE80211_CHAN_RADAR | 
                IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_5G(60, IEEE80211_CHAN_RADAR | 
                IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_5G(64, IEEE80211_CHAN_RADAR | 
                IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40PLUS),
    /* MID */
    CHAN_5G(100, IEEE80211_CHAN_RADAR | 
                 IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_5G(104, IEEE80211_CHAN_RADAR | 
                 IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_5G(108, IEEE80211_CHAN_RADAR | 
                 IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_5G(112, IEEE80211_CHAN_RADAR | 
                 IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_5G(116, IEEE80211_CHAN_RADAR | 
                 IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_5G(120, IEEE80211_CHAN_RADAR | 
                 IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_5G(124, IEEE80211_CHAN_RADAR | 
                 IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_5G(128, IEEE80211_CHAN_RADAR | 
                 IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_5G(132, IEEE80211_CHAN_RADAR | 
                 IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_5G(136, IEEE80211_CHAN_RADAR | 
                 IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_5G(140, IEEE80211_CHAN_RADAR | 
                 IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_HT40PLUS |
                 IEEE80211_CHAN_NO_HT40MINUS),
    /* UNII-3 */
    CHAN_5G(149, IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_5G(153, IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_5G(157, IEEE80211_CHAN_NO_HT40MINUS),
    CHAN_5G(161, IEEE80211_CHAN_NO_HT40PLUS),
    CHAN_5G(165, IEEE80211_CHAN_NO_HT40PLUS | IEEE80211_CHAN_NO_HT40MINUS)
};

/*
 * The rate table is used for both 2.4G and 5G rates. The
 * latter being a subset as it does not support CCK rates.
 */
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
	.band = NL80211_BAND_2GHZ,
	.channels = hgics_ch2g,
	.n_channels = ARRAY_SIZE(hgics_ch2g),
	.bitrates = hgics_legacy_ratetable,
	.n_bitrates = ARRAY_SIZE(hgics_legacy_ratetable),
	.ht_cap = {
		.cap = IEEE80211_HT_CAP_GRN_FLD | IEEE80211_HT_CAP_SGI_20,
		.ht_supported  = true,
		.ampdu_factor  = IEEE80211_HT_MAX_AMPDU_8K,
		.ampdu_density = 6,
		.mcs = {
			.rx_mask	= {0xff, 0x00, 0, 0, 0, 0, 0, 0, 0, 0},
			.rx_highest = 0,//cpu_to_le16(500),
			.tx_params	= IEEE80211_HT_MCS_TX_DEFINED
		}
	}
};

static const struct ieee80211_supported_band hgics_band_5G = {
    .band = NL80211_BAND_5GHZ,
    .channels = hgics_ch5g,
    .n_channels = ARRAY_SIZE(hgics_ch5g),
    .bitrates   = hgics_legacy_ratetable,
    .n_bitrates = ARRAY_SIZE(hgics_legacy_ratetable),
    .ht_cap = {
        .cap = IEEE80211_HT_CAP_GRN_FLD | IEEE80211_HT_CAP_SGI_20 | IEEE80211_HT_CAP_SGI_40,
        .ht_supported = true,
        .ampdu_factor = IEEE80211_HT_MAX_AMPDU_64K,
        .ampdu_density = 6,
        .mcs = {
            .rx_mask    = {0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0},
            .rx_highest = cpu_to_le16(500),
            .tx_params  = IEEE80211_HT_MCS_TX_DEFINED
        }
    }
};

static const struct ieee80211_iface_limit hgics_iface_limits[] = {
	{
		.max = 4,
		.types = BIT(NL80211_IFTYPE_STATION) |
			 BIT(NL80211_IFTYPE_AP)
	},
	{
		.max = 4,
		.types = BIT(NL80211_IFTYPE_P2P_CLIENT) |
			 BIT(NL80211_IFTYPE_P2P_GO)
	},
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_P2P_DEVICE)
	} 
};

static const struct ieee80211_iface_combination hgics_iface_combos[] = {
	{
		 .max_interfaces = 8,
		 .num_different_channels = 1,
		 .n_limits = ARRAY_SIZE(hgics_iface_limits),
		 .limits = hgics_iface_limits
	}
};


void hgics_11bgn_hw_init(struct ieee80211_hw *hw)
{
    struct hgics_wdev *hg = hw->priv;
    struct ieee80211_supported_band *sband = &hg->sbands[NL80211_BAND_2GHZ];
    unsigned char i = 0;

    hw->queues = 5;
    hw->offchannel_tx_hw_queue = 4;
    /*
    hw->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) |
                                 BIT(NL80211_IFTYPE_AP);
                                 */

    hw->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) |
		BIT(NL80211_IFTYPE_P2P_CLIENT) |
		BIT(NL80211_IFTYPE_AP) |
		BIT(NL80211_IFTYPE_P2P_GO) |
		BIT(NL80211_IFTYPE_P2P_DEVICE) |
		BIT(NL80211_IFTYPE_MESH_POINT) |
		BIT(NL80211_IFTYPE_ADHOC);
    
    ieee80211_hw_set(hw, SIGNAL_DBM);
    ieee80211_hw_set(hw, AMPDU_AGGREGATION);
    ieee80211_hw_set(hw, TX_AMPDU_SETUP_IN_HW);
    ieee80211_hw_set(hw, QUEUE_CONTROL);
    ieee80211_hw_set(hw, HAS_RATE_CONTROL);
//	ieee80211_hw_set(hw, REPORTS_TX_ACK_STATUS);
//	ieee80211_hw_set(hw, HOST_BROADCAST_PS_BUFFERING);	
	ieee80211_hw_set(hw, MFP_CAPABLE);
//    ieee80211_hw_set(hw, REMAIN_ON_CHANNEL);

    memcpy(sband, &hgics_band_2G, sizeof(struct ieee80211_supported_band));
    hw->wiphy->bands[NL80211_BAND_2GHZ] = sband;
    
    hw->vif_data_size = sizeof(struct hgics_vif);
    hw->chanctx_data_size = sizeof(struct hgics_chanctx_priv);
    hw->max_rates = 4;
    hw->max_rate_tries = 11;
/*
    for (i = 0; i < 4; i++) {
        memcpy(hg->macaddr[i].addr, hg->fwinfo.mac, 6);
        hg->macaddr[i].addr[5] += i;
    }
    */
    memcpy(hg->macaddr[i].addr, hg->fwinfo.mac, 6);
    hw->wiphy->n_addresses = 1;

    hw->wiphy->iface_combinations = hgics_iface_combos;
	hw->wiphy->n_iface_combinations =
	ARRAY_SIZE(hgics_iface_combos);
}

void hgics_11bgn_free(struct hgics_wdev *hg)
{

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
