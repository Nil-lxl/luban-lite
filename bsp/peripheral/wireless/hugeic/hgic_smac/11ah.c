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

#ifdef CONFIG_HGIC_AH
#include "hgics.h"

static const struct ieee80211_rate hgics_s1g_1M_rates[] = {
    /*11ah MCS 0 ~ 9 with 1MHZ bw Nss = 1*/
    { .bitrate = 300, .flags = IEEE80211_RATE_SUPPORTS_S1G_1MHZ },
    { .bitrate = 600, .flags = IEEE80211_RATE_SUPPORTS_S1G_1MHZ },
    { .bitrate = 900, .flags = IEEE80211_RATE_SUPPORTS_S1G_1MHZ },
    { .bitrate = 1200, .flags = IEEE80211_RATE_SUPPORTS_S1G_1MHZ },
    { .bitrate = 1800, .flags = IEEE80211_RATE_SUPPORTS_S1G_1MHZ },
    { .bitrate = 2400, .flags = IEEE80211_RATE_SUPPORTS_S1G_1MHZ },
    { .bitrate = 2700, .flags = IEEE80211_RATE_SUPPORTS_S1G_1MHZ },
    { .bitrate = 3000, .flags = IEEE80211_RATE_SUPPORTS_S1G_1MHZ },
    { .bitrate = 3600, .flags = IEEE80211_RATE_SUPPORTS_S1G_1MHZ },
    { .bitrate = 4000, .flags = IEEE80211_RATE_SUPPORTS_S1G_1MHZ },
    { .bitrate = 150, .flags = IEEE80211_RATE_SUPPORTS_S1G_MCS10 },
};

static const struct ieee80211_rate hgics_s1g_2M_rates[] = {
    /*11ah MCS 0 ~ 9 with 2MHZ bw Nss = 1*/
    { .bitrate = 650, .flags = IEEE80211_RATE_SUPPORTS_S1G_2MHZ },
    { .bitrate = 1300, .flags = IEEE80211_RATE_SUPPORTS_S1G_2MHZ },
    { .bitrate = 1950, .flags = IEEE80211_RATE_SUPPORTS_S1G_2MHZ },
    { .bitrate = 2600, .flags = IEEE80211_RATE_SUPPORTS_S1G_2MHZ },
    { .bitrate = 3900, .flags = IEEE80211_RATE_SUPPORTS_S1G_2MHZ },
    { .bitrate = 5200, .flags = IEEE80211_RATE_SUPPORTS_S1G_2MHZ },
    { .bitrate = 5850, .flags = IEEE80211_RATE_SUPPORTS_S1G_2MHZ },
    { .bitrate = 6500, .flags = IEEE80211_RATE_SUPPORTS_S1G_2MHZ },
    { .bitrate = 7800, .flags = IEEE80211_RATE_SUPPORTS_S1G_2MHZ },
};

static const struct ieee80211_rate hgics_s1g_4M_rates[] = {
    /*11ah MCS 0 ~ 9 with 4MHZ bw Nss = 1*/
    { .bitrate = 1350, .flags = IEEE80211_RATE_SUPPORTS_S1G_4MHZ },
    { .bitrate = 2700, .flags = IEEE80211_RATE_SUPPORTS_S1G_4MHZ },
    { .bitrate = 4050, .flags = IEEE80211_RATE_SUPPORTS_S1G_4MHZ },
    { .bitrate = 5400, .flags = IEEE80211_RATE_SUPPORTS_S1G_4MHZ },
    { .bitrate = 8100, .flags = IEEE80211_RATE_SUPPORTS_S1G_4MHZ },
    { .bitrate = 10800, .flags = IEEE80211_RATE_SUPPORTS_S1G_4MHZ },
    { .bitrate = 12150, .flags = IEEE80211_RATE_SUPPORTS_S1G_4MHZ },
    { .bitrate = 13500, .flags = IEEE80211_RATE_SUPPORTS_S1G_4MHZ },
    { .bitrate = 16200, .flags = IEEE80211_RATE_SUPPORTS_S1G_4MHZ },
    { .bitrate = 18000, .flags = IEEE80211_RATE_SUPPORTS_S1G_4MHZ },
};

static const struct ieee80211_rate hgics_s1g_8M_rates[] = {
    /*11ah MCS 0 ~ 9 with 8MHZ bw Nss = 1, unit is 1k*/
    { .bitrate = 2925, .flags = IEEE80211_RATE_SUPPORTS_S1G_8MHZ },
    { .bitrate = 5850, .flags = IEEE80211_RATE_SUPPORTS_S1G_8MHZ },
    { .bitrate = 8775, .flags = IEEE80211_RATE_SUPPORTS_S1G_8MHZ },
    { .bitrate = 11700, .flags = IEEE80211_RATE_SUPPORTS_S1G_8MHZ },
    { .bitrate = 17550, .flags = IEEE80211_RATE_SUPPORTS_S1G_8MHZ },
    { .bitrate = 23400, .flags = IEEE80211_RATE_SUPPORTS_S1G_8MHZ },
    { .bitrate = 26325, .flags = IEEE80211_RATE_SUPPORTS_S1G_8MHZ },
    { .bitrate = 29250, .flags = IEEE80211_RATE_SUPPORTS_S1G_8MHZ },
    { .bitrate = 35100, .flags = IEEE80211_RATE_SUPPORTS_S1G_8MHZ },
    { .bitrate = 39000, .flags = IEEE80211_RATE_SUPPORTS_S1G_8MHZ },
};

static struct ieee80211_rate *hgics_11ah_rates(int bw, int *count)
{
    *count = 0;
    switch (bw) {
        case 1:
            *count = ARRAY_SIZE(hgics_s1g_1M_rates);
            return (struct ieee80211_rate *)hgics_s1g_1M_rates;
        case 2:
            *count = ARRAY_SIZE(hgics_s1g_2M_rates);
            return (struct ieee80211_rate *)hgics_s1g_2M_rates;
        case 4:
            *count = ARRAY_SIZE(hgics_s1g_4M_rates);
            return (struct ieee80211_rate *)hgics_s1g_4M_rates;
        case 8:
            *count = ARRAY_SIZE(hgics_s1g_8M_rates);
            return (struct ieee80211_rate *)hgics_s1g_8M_rates;
        case 16:
        default:
            printk("not support channel bandwidth:%d\r\n", bw);
            break;
    }
    return NULL;
}

static void hgics_11ah_check_config(struct hgics_wdev *hg)
{
    int i = 0;

    if (hg->conf->ahcfg.bss_bw < 1 || hg->conf->ahcfg.bss_bw > 8) {
        hg->conf->ahcfg.bss_bw = 8;
    }
    if (hg->conf->ahcfg.tx_bw < 1 || hg->conf->ahcfg.tx_bw > hg->conf->ahcfg.bss_bw) {
        hg->conf->ahcfg.tx_bw = hg->conf->ahcfg.bss_bw;
    }
    if (hg->conf->ahcfg.tx_mcs > 7) {
        hg->conf->ahcfg.tx_mcs = 0xFF; //auto
    }
    if (hg->conf->ahcfg.freq_start < 7300 || hg->conf->ahcfg.freq_start > 9300) {
        hg->conf->ahcfg.freq_start = 7300;
    }
    if (hg->conf->ahcfg.freq_end < hg->conf->ahcfg.freq_start || hg->conf->ahcfg.freq_end > 9300) {
        hg->conf->ahcfg.freq_end = 9300;
    }
    if (hg->conf->ahcfg.primary_chan > 7) {
        hg->conf->ahcfg.primary_chan = 0;
    }
    if (hg->conf->ahcfg.bgrssi > 64) {
        hg->conf->ahcfg.bgrssi = 0;
    }
    if (hg->conf->ahcfg.chan_cnt == 0) {
        hg->conf->ahcfg.chan_cnt = ((hg->conf->ahcfg.freq_end - hg->conf->ahcfg.freq_start) / (hg->conf->ahcfg.bss_bw * 10)) + 1;
        if (hg->conf->ahcfg.chan_cnt > UMAC_CHAN_CNT) {
            hg->conf->ahcfg.chan_cnt = UMAC_CHAN_CNT;
        }
        for (i = 0; i < hg->conf->ahcfg.chan_cnt; i++) {
            hg->conf->ahcfg.chan_list[i] = hg->conf->ahcfg.freq_start + (i * hg->conf->ahcfg.bss_bw * 10);
        }
    }
}

static int hgics_11ah_create_channles(struct hgics_wdev *hg)
{
    int i   = 0;

    printk("HUGE-IC 11AH device support %d channels:\r\n", hg->conf->ahcfg.chan_cnt);
    for (i = 0; i < hg->conf->ahcfg.chan_cnt; i++) {
        hg->ahdev.channels[i].band = IEEE80211_BAND_1GHZ;
        hg->ahdev.channels[i].center_freq = hg->conf->ahcfg.chan_list[i];
        hg->ahdev.channels[i].hw_value    = hg->conf->ahcfg.chan_list[i];
        hg->ahdev.channels[i].max_power = 30;
        hg->ahdev.channels[i].orig_mpwr = 30;
        hg->ahdev.channels[i].orig_mag = INT_MAX;
        printk("  channel[%d] = %d.%dMHz\r\n", i,
               hg->ahdev.channels[i].center_freq / 10,
               hg->ahdev.channels[i].center_freq % 10);
    }
    hg->sbands[IEEE80211_BAND_1GHZ].band = IEEE80211_BAND_1GHZ;
    hg->sbands[IEEE80211_BAND_1GHZ].channels = hg->ahdev.channels;
    hg->sbands[IEEE80211_BAND_1GHZ].n_channels = hg->conf->ahcfg.chan_cnt;
    return hg->conf->ahcfg.chan_cnt;
}

static int hgics_11ah_create_regulatory(struct hgics_wdev *hg)
{
    u16 freq_start, freq_end;

    if (hg->ahdev.regd == NULL)
        hg->ahdev.regd = kzalloc(sizeof(struct ieee80211_regdomain) +
                                 sizeof(struct ieee80211_reg_rule), GFP_KERNEL);

    if (hg->ahdev.regd == NULL) {
        return -ENOMEM;
    }

    freq_start = hg->conf->ahcfg.chan_list[0];
    freq_end   = hg->conf->ahcfg.chan_list[hg->conf->ahcfg.chan_cnt - 1];
    hg->ahdev.regd->alpha2[0] = '9';
    hg->ahdev.regd->alpha2[1] = '9';
    hg->ahdev.regd->n_reg_rules = 1;
    hg->ahdev.regd->reg_rules[0].freq_range.start_freq_khz = MHZ_TO_KHZ(freq_start);
    hg->ahdev.regd->reg_rules[0].freq_range.end_freq_khz = MHZ_TO_KHZ(freq_end);
    hg->ahdev.regd->reg_rules[0].freq_range.max_bandwidth_khz = MHZ_TO_KHZ(hg->conf->ahcfg.bss_bw);
    hg->ahdev.regd->reg_rules[0].power_rule.max_antenna_gain = DBI_TO_MBI(6);
    hg->ahdev.regd->reg_rules[0].power_rule.max_eirp = DBM_TO_MBM(20);
    hg->ahdev.regd->reg_rules[0].flags = 0;
    hg->hw->wiphy->flags |= WIPHY_FLAG_CUSTOM_REGULATORY | WIPHY_FLAG_STRICT_REGULATORY;
    wiphy_apply_custom_regulatory(hg->hw->wiphy, hg->ahdev.regd);
    return 0;
}

static void hgics_11ah_load_config(struct hgics_wdev *hg)
{
#ifndef __RTOS__
    ssize_t ret = 0;
    struct file *fp = NULL;
    char *buf = kzalloc(1024, GFP_KERNEL);

    fp = filp_open(hg->conf_file, O_RDONLY, 0);
    if (!IS_ERR(fp) && buf) {
        ret = _KERNEL_READ(fp, buf, 1024);
        hg->conf->ahcfg.freq_start = hgic_config_read_int(buf, "freq_start");
        hg->conf->ahcfg.freq_end   = hgic_config_read_int(buf, "freq_end");
        hg->conf->ahcfg.bss_bw     = hgic_config_read_int(buf, "bss_bw");
        hg->conf->ahcfg.tx_bw      = hgic_config_read_int(buf, "tx_bw");
        hg->conf->ahcfg.tx_mcs     = hgic_config_read_int(buf, "tx_mcs");
        hg->conf->ahcfg.acs        = hgic_config_read_int(buf, "acs");
        hg->conf->ahcfg.acs_tm     = hgic_config_read_int(buf, "acs_tm");
        hg->conf->ahcfg.primary_chan = hgic_config_read_int(buf, "primary_chan");
        hg->conf->ahcfg.bgrssi = hgic_config_read_int(buf, "bg_rssi");
        hg->conf->ahcfg.chan_cnt = hgic_config_read_u16_array(buf, "chan_list", hg->conf->ahcfg.chan_list, UMAC_CHAN_CNT);
    }

    hg->cfg_changed = 1;
    printk("hgics config:\r\n");
    printk("    freq_start=%d\r\n", hg->conf->ahcfg.freq_start);
    printk("    freq_end  =%d\r\n", hg->conf->ahcfg.freq_end);
    printk("    bss_bw    =%d\r\n", hg->conf->ahcfg.bss_bw);
    printk("    tx_bw     =%d\r\n", hg->conf->ahcfg.tx_bw);
    printk("    tx_mcs    =%d\r\n", hg->conf->ahcfg.tx_mcs);
    printk("    bgrssi    =%d\r\n", hg->conf->ahcfg.bgrssi);
    printk("    acs       =%d\r\n", hg->conf->ahcfg.acs);
    printk("    acs_tm    =%d\r\n", hg->conf->ahcfg.acs_tm);
    printk("    primary_chan =%d\r\n", hg->conf->ahcfg.primary_chan);

    if (!IS_ERR(fp)) { filp_close(fp, NULL); }
    if (buf) { kfree(buf); }
#endif
}

static int hgics_ahops_init(struct hgics_wdev *hg)
{
    struct ieee80211_hw *hw = hg->hw;
    struct ieee80211_supported_band *sband = &hg->sbands[IEEE80211_BAND_1GHZ];

    hgic_dbg("Enter\n");
    hgics_11ah_load_config(hg);
    hgics_11ah_check_config(hg);

    hw->channel_change_time = 1;
    hw->queues = 5;
    hw->offchannel_tx_hw_queue = 4;
    hw->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) |
                                 BIT(NL80211_IFTYPE_AP);

    hw->flags = IEEE80211_HW_SIGNAL_DBM |
                IEEE80211_HW_AMPDU_AGGREGATION |
                IEEE80211_HW_TX_AMPDU_SETUP_IN_HW |
                IEEE80211_HW_QUEUE_CONTROL |
                IEEE80211_HW_SUPPORTS_S1G_RATES |
                IEEE80211_HW_HAS_RATE_CONTROL;

    cfg80211_set_s1g_chan_list(hg->conf->ahcfg.chan_list, hg->conf->ahcfg.chan_cnt);
    hgics_11ah_create_channles(hg);
    sband->bitrates = hgics_11ah_rates(hg->conf->ahcfg.bss_bw, &sband->n_bitrates);

    memset(&(sband->s1g_cap.cap), 0, sizeof(struct s1g_capabilities_info));
    sband->s1g_cap.s1g_supported = true;
    sband->s1g_cap.cap.s1g_long = 1;
    if (hg->conf->ahcfg.bss_bw == 1) {
        sband->s1g_cap.cap.s1g_short_GI_1M = 1;
    } else if (hg->conf->ahcfg.bss_bw == 2) {
        sband->s1g_cap.cap.s1g_short_GI_2M = 1;
    } else if (hg->conf->ahcfg.bss_bw == 4) {
        sband->s1g_cap.cap.s1g_short_GI_4M = 1;
    } else if (hg->conf->ahcfg.bss_bw == 8) {
        sband->s1g_cap.cap.s1g_short_GI_8M = 1;
    } else {
        printk("not support bandwidth:%dM\r\n", hg->conf->ahcfg.bss_bw);
    }

    sband->s1g_cap.cap.Supported_BW = 0;
    sband->s1g_cap.cap.PV1_frame_support = 1;
    memset(&(sband->s1g_cap.s1g_mcs), 0, sizeof(struct ieee80211_s1g_mcs_info));
    sband->s1g_cap.s1g_mcs.Rx_Max_S1G_MCS_1SS = 2; //support MCS0 ~ 9
    sband->s1g_cap.s1g_mcs.Tx_Max_S1G_MCS_1SS = 2; //support MCS0 ~ 9
    hw->wiphy->bands[NL80211_BAND_1GHZ] = sband;
    hw->vif_data_size = sizeof(struct hgics_vif);
    hw->chanctx_data_size = sizeof(struct hgics_chanctx_priv);
    hw->max_rates = 4;
    hw->max_rate_tries = 11;
    memcpy(hg->macaddr[0].addr, hg->fwinfo.mac, 6);
    hw->wiphy->n_addresses = 1;
    hgics_11ah_create_regulatory(hg);
    hgic_dbg("Leave\n");
    return 0;
}

static int hgics_ahops_free(struct hgics_wdev *hg)
{
    if (hg->ahdev.regd) {
        kfree(hg->ahdev.regd);
    }
    return 0;
}

static int hgics_ahops_create_procfs(struct hgics_wdev *hg)
{
    return 0;
}

static int hgics_ahops_delete_procfs(struct hgics_wdev *hg)
{
    return 0;
}

static int hgics_ahops_start(struct ieee80211_hw *hw)
{
    struct hgics_wdev *hg = (struct hgics_wdev *)hw->priv;
    if (hg->cfg_changed) {
        hgics_ahops_init(hw); //re-init
        ieee80211_init_operchandef(hw);
        hgic_fwctrl_close_dev(&hg->ctrl, 1);
        if (hg->conf->ahcfg.chan_cnt > 0) {
            hgic_fwctrl_set_chan_list(&hg->ctrl, 1, hg->conf->ahcfg.chan_list, hg->conf->ahcfg.chan_cnt);
        } else {
            hgic_fwctrl_set_freq_range(&hg->ctrl, 1, hg->conf->ahcfg.freq_start, hg->conf->ahcfg.freq_end, hg->conf->ahcfg.bss_bw);
        }
        hgic_fwctrl_set_bss_bw(&hg->ctrl, 1, hg->conf->ahcfg.bss_bw);
        hgic_fwctrl_set_tx_mcs(&hg->ctrl, 1, hg->conf->ahcfg.tx_mcs);
        hgic_fwctrl_set_primary_chan(&hg->ctrl, 1, hg->conf->ahcfg.primary_chan);
        hgic_fwctrl_set_bgrssi(&hg->ctrl, 1, hg->conf->ahcfg.bgrssi);
        hg->cfg_changed = 0;
    }
    return 0;
}
static int hgics_ahops_stop(struct ieee80211_hw *hw)
{
    return 0;
}
static int hgics_ahops_start_ap(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
    struct hgics_wdev *hg = hw->priv;
    if (hg->conf->ahcfg.acs) {
        hgic_dbg("acs:%d, time:%d\r\n", hg->conf->ahcfg.acs, hg->conf->ahcfg.acs_tm);
        hgic_fwctrl_set_acs(&hg->ctrl, 1, hg->conf->ahcfg.acs, hg->conf->ahcfg.acs_tm);
    }
    return 0;
}
static int hgics_ahops_stop_ap(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
    return 0;
}
static int hgics_ahops_add_interface(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
    struct hgics_vif *hgvif = (struct hgics_vif *)vif->drv_priv;
    hgvif->ndev = vif->dev;
    return 0;
}
static int hgics_ahops_remove_interface(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
    struct hgics_vif *hgvif = (struct hgics_vif *)vif->drv_priv;
    hgvif->ndev = NULL;
    return 0;
}

struct hgics_hw_ops hgics_hw_ahops = {
    .init = hgics_ahops_init,
    .free = hgics_ahops_free,
    .start = hgics_ahops_start,
    .stop = hgics_ahops_stop,
    .start_ap = hgics_ahops_start_ap,
    .stop_ap = hgics_ahops_stop_ap,
    .add_interface = hgics_ahops_add_interface,
    .remove_interface = hgics_ahops_remove_interface,
    .create_procfs = hgics_ahops_create_procfs,
    .delete_procfs = hgics_ahops_delete_procfs,
};

#endif

