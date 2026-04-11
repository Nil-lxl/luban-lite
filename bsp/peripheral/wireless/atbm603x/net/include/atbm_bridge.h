#ifndef ATBM_BRIDGE_H

#define ATBM_BRIDGE_H
struct atbm_buff *get_sta_deliver_skb(struct atbm_buff *skb);

struct atbm_buff *get_ap_deliver_skb(struct atbm_buff *skb);

atbm_void atbm_brpool_init(struct atbmwifi_common *hw_priv);

atbm_void atbm_brpool_deinit(struct atbmwifi_common *hw_priv);

int remove_item_from_brpool(int id);

#endif
