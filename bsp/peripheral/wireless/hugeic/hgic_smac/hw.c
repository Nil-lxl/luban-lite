
#include "hgics.h"

extern struct hgics_hw_ops hgics_hw_ahops;
extern struct hgics_hw_ops hgics_hw_bgnops;

static const struct hgics_hw hgics_hw_list[] = {
#ifdef CONFIG_HGIC_AH
    {HGIC_WLAN_4002, &hgics_hw_ahops},
#endif
#ifdef CONFIG_HGIC_2G
    {HGIC_WLAN_8400, &hgics_hw_bgnops},    
    {HGIC_WLAN_8410, &hgics_hw_bgnops},
    {HGIC_WLAN_8000, &hgics_hw_bgnops},    
#endif
};

const struct hgics_hw *hgics_hw_match(u32 dev_id)
{
    int i = 0;

    for (i = 0; i < ARRAY_SIZE(hgics_hw_list); i++) {
        if (hgics_hw_list[i].chip_id == dev_id) {
            return &hgics_hw_list[i];
        }
    }

    hgic_err("not support chipid %x\r\n", dev_id);
    return NULL;
}


