
#ifndef HGICS_HW_H_
#define HGICS_HW_H_

struct hgics_hw_ops {
    int (*init)(struct hgics_wdev *hg);
    int (*free)(struct hgics_wdev *hg);
    int (*start)(struct ieee80211_hw *hw);
    int (*stop)(struct ieee80211_hw *hw);
    int (*start_ap)(struct ieee80211_hw *hw, struct ieee80211_vif *vif);
    int (*stop_ap)(struct ieee80211_hw *hw, struct ieee80211_vif *vif);
    int (*add_interface)(struct ieee80211_hw *hw, struct ieee80211_vif *vif);
    int (*remove_interface)(struct ieee80211_hw *hw, struct ieee80211_vif *vif);
    int (*create_procfs)(struct hgics_wdev *hg);
    int (*delete_procfs)(struct hgics_wdev *hg);
};

struct hgics_hw {
    u32 chip_id;
    struct hgics_hw_ops *ops;
};

const struct hgics_hw *hgics_hw_match(u32 dev_id);

#endif


