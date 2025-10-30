#ifndef _HGICS_AP_H_
#define _HGICS_AP_H_

#if !defined(__RTOS__) && LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
extern void hgics_mac80211_beacon(struct timer_list *t);
#else
extern void hgics_mac80211_beacon(unsigned long arg);
#endif
extern void hgics_ap_reset_beacon(struct ieee80211_hw *hw,        struct ieee80211_vif *vif, struct ieee80211_bss_conf *info);

#endif
