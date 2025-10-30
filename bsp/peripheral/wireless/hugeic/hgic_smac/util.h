
#ifndef _HGICS_UTILS_H_
#define _HGICS_UTILS_H_

extern void hgics_flag_new(void *v);
extern void hgics_flag_del(void *v);
extern int  hgics_flag_check(void *v);
extern u64 hgics_get_tsf(struct ieee80211_hw *hw, struct ieee80211_vif *vif);
extern void hgic_print_hex(char *buf, int len);
u32 hgics_get_icmp_seq(struct sk_buff *skb);
void hgics_icmp_monitor(u8 en, struct sk_buff *skb, u8 tx);

#endif


