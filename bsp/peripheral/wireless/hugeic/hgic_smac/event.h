
#ifndef _HGICS_EVENT_H_
#define _HGICS_EVENT_H_

void hgics_rx_fw_event(struct hgic_fwctrl *ctrl, struct sk_buff *skb);
void hgics_event(struct hgics_wdev *hg, char *ifname, int event, int param1, int param2);

#endif

