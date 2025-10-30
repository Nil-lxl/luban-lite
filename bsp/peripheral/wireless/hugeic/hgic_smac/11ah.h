#ifndef _HGICS_11AH_H_
#define _HGICS_11AH_H_

#ifdef CONFIG_HGIC_AH
#define UMAC_CHAN_CNT (32)

struct hgics_11ah_wdev {
    struct ieee80211_channel channels[UMAC_CHAN_CNT];
    struct ieee80211_regdomain *regd;
};

struct hgics_11ah_config {
    unsigned short freq_start, freq_end;
    unsigned short chan_list[UMAC_CHAN_CNT];
    unsigned char  tx_bw, tx_mcs, acs, acs_tm, primary_chan, bgrssi, bss_bw, chan_cnt;
};

#endif
#endif

