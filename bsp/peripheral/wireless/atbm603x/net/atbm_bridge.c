#include "atbm_hal.h"

#define ATBM_BRIDGE_POOL_MAX_SIZE (ATBMWIFI__MAX_STA_IN_AP_MODE+1)
#define STA_BRIDGE_POOL_NUM ATBMWIFI__MAX_STA_IN_AP_MODE

#define ATBM_IPPROTO_UDP 17
#define ATBM_DHCP_MAGIC	0x63825363
#define ATBM_BROADCAST_FLAG	0x8000

#define IS_BOOTP_PORT(src_port,des_port) ((((src_port) == 67)&&((des_port) == 68)) || \
										   (((src_port) == 68)&&((des_port) == 67)))

#define atbm_const_htons(n) ((n & 0xff) << 8) | ((n & 0xff00) >> 8)

struct atbmwifi_vif *ap_priv;


struct arphdr {
	atbm_uint16		ar_hrd;		/* format of hardware address	*/
	atbm_uint16		ar_pro;		/* format of protocol address	*/
	atbm_uint8	ar_hln;		/* length of hardware address	*/
	atbm_uint8	ar_pln;		/* length of protocol address	*/
	atbm_uint16		ar_op;		/* ARP opcode (command)		*/
};


struct atbm_udp_hdr {
	atbm_uint16	source;
	atbm_uint16	dest;
	atbm_uint16	len;
	atbm_uint16	check;
};

struct dhcpMessage {
	atbm_uint8 op;
	atbm_uint8 htype;
	atbm_uint8 hlen;
	atbm_uint8 hops;
	atbm_uint32 xid;
	atbm_uint16 secs;
	atbm_uint16 flags;
	atbm_uint32 ciaddr;
	atbm_uint32 yiaddr;
	atbm_uint32 siaddr;
	atbm_uint32 giaddr;
	atbm_uint8 chaddr[16];
	atbm_uint8 sname[64];
	atbm_uint8 file[128];
	atbm_uint32 cookie;
	atbm_uint8 options[308]; /* 312 - cookie */
};

struct atbm_bridge_pool{
	int active;
	atbm_uint8 mac[ATBM_ETH_ALEN];
	atbm_uint32 ip;
};

struct atbm_bridge_pool br_pool[ATBM_BRIDGE_POOL_MAX_SIZE];

atbm_spinlock_t pool_lock;

atbm_uint32 get_staif_ip(){
	atbm_uint32 ip = 0;
	atbm_spin_lock(&pool_lock);
	if(br_pool[ATBM_BRIDGE_POOL_MAX_SIZE-1].active){
		ip = br_pool[ATBM_BRIDGE_POOL_MAX_SIZE-1].ip;
	}
	atbm_spin_unlock(&pool_lock);
	return ip;
}

atbm_void set_staif_ip(atbm_uint32 ip){
	atbm_spin_lock(&pool_lock);
	br_pool[ATBM_BRIDGE_POOL_MAX_SIZE-1].active = 1;
	br_pool[ATBM_BRIDGE_POOL_MAX_SIZE-1].ip = ip;
	atbm_spin_unlock(&pool_lock);
}

atbm_uint8 *get_staif_mac(){
	atbm_uint8 *mac = ATBM_NULL;
	atbm_spin_lock(&pool_lock);
	if(br_pool[ATBM_BRIDGE_POOL_MAX_SIZE-1].active){
		mac = br_pool[ATBM_BRIDGE_POOL_MAX_SIZE-1].mac;
	}
	atbm_spin_unlock(&pool_lock);
	return mac;
}

atbm_void set_staif_mac(atbm_uint8 *mac){
	atbm_spin_lock(&pool_lock);
	br_pool[ATBM_BRIDGE_POOL_MAX_SIZE-1].active = 1;
	atbm_memcpy(br_pool[ATBM_BRIDGE_POOL_MAX_SIZE-1].mac, mac, ATBM_ETH_ALEN);
	atbm_spin_unlock(&pool_lock);
}

int insert_item_to_brpool(atbm_uint8 *mac, atbm_uint32 ip){
	int i, found = -1;
	int id;

	if(!ap_priv)
		return -1;

	id = atbmwifi_find_link_id(ap_priv, mac);
	if(id <= 0 || id > ATBMWIFI__MAX_STA_IN_AP_MODE)
		return -1;

	atbm_spin_lock(&pool_lock);
	br_pool[id-1].active = 1;
	br_pool[id-1].ip = ip;
	atbm_memcpy(br_pool[id-1].mac, mac, ATBM_ETH_ALEN);
	atbm_spin_unlock(&pool_lock);
	return -1;
}

int remove_item_from_brpool(int id){
	if(id <= 0 || id > ATBMWIFI__MAX_STA_IN_AP_MODE){
		return -1;
	}
	atbm_spin_lock(&pool_lock);
	br_pool[id].active = 0;
	atbm_spin_unlock(&pool_lock);
}

atbm_uint8 *get_mac_byip_from_brpool(atbm_uint32 ip){
	int i;
	atbm_spin_lock(&pool_lock);
	for(i = 0; i < ATBMWIFI__MAX_STA_IN_AP_MODE; i++){
		if(br_pool[i].active){
			if(br_pool[i].ip == ip){
				atbm_spin_unlock(&pool_lock);
				return br_pool[i].mac;
			}
		}
	}
	atbm_spin_unlock(&pool_lock);
	return ATBM_NULL;
}

atbm_uint32 get_ip_bymac_from_brpool(atbm_uint8 *mac){
	int i;
	atbm_spin_lock(&pool_lock);
	for(i = 0; i < ATBM_BRIDGE_POOL_MAX_SIZE; i++){
		if(br_pool[i].active){
			if(!atbm_memcmp(br_pool[i].mac, mac, ATBM_ETH_ALEN)){
				atbm_spin_unlock(&pool_lock);
				return br_pool[i].ip;
			}
		}
	}
	atbm_spin_unlock(&pool_lock);
	return 0;
}

int ieee80211_is_dhcp_frame(struct atbm_buff *skb)
{
	const struct atbm_ip_hdr *ip; 
	struct atbmwifi_ieee8023_hdr *ehdr = (struct atbmwifi_ieee8023_hdr *) ATBM_OS_SKB_DATA(skb);

	ip =(struct atbm_ip_hdr *)((atbm_uint8*)ehdr +sizeof(struct atbmwifi_ieee8023_hdr));
	//wifi_printk(WIFI_ALWAYS, "h_proto:%x ATBM_ETH_P_IP:%x _proto:%d ATBM_IPPROTO_UDP:%d\n", ehdr->h_proto, atbm_htons(ATBM_ETH_P_IP), ip->_proto, ATBM_IPPROTO_UDP);
	
	if (ehdr->h_proto == atbm_htons(ATBM_ETH_P_IP))	{
		if (ATBM_IPPROTO_UDP==ip->_proto) {
			//dump_mem(ehdr, sizeof(struct atbmwifi_ieee8023_hdr) + sizeof(struct atbm_ip_hdr) + sizeof(struct atbm_udp_hdr));
			//wifi_printk(WIFI_ALWAYS, "_v_hl:%d\n", ip->_v_hl);
			struct atbm_udp_hdr *udph=(struct atbm_udp_hdr *)((atbm_uint8*)ip+((ip->_v_hl & 0xf)<<2));
			//wifi_printk(WIFI_ALWAYS, "src:%d dst:%d\n", atbm_ntohs(udph->source), atbm_ntohs(udph->dest));
			if(IS_BOOTP_PORT(atbm_ntohs(udph->source),atbm_ntohs(udph->dest)))	{
				return 1;
			}
		}
	}
	return 0;
}


void ieee80211_tx_set_dhcp_bcast_flag(struct atbm_buff *skb)
{
	//struct ethhdr *ehdr = (struct ethhdr *)skb->data;
	if(skb == NULL)
		return;

	if(ieee80211_is_dhcp_frame(skb)) // DHCP request
	{
		struct atbm_ip_hdr* iph = (struct atbm_ip_hdr *)(ATBM_OS_SKB_DATA(skb) + ATBM_ETH_HLEN);
		struct atbm_udp_hdr *udph = (struct atbm_udp_hdr *)((atbm_uint8 *)iph + ((iph->_v_hl & 0xf) << 2));
		struct dhcpMessage *dhcph =	(struct dhcpMessage *)((atbm_uint8 *)udph + sizeof(struct atbm_udp_hdr));

		//wifi_printk(WIFI_ALWAYS, "deliver dhcp!!! dhcph->cookie:%x dhcph->flags:%x\n", dhcph->cookie, dhcph->flags);
		if(dhcph->cookie == atbm_htonl(ATBM_DHCP_MAGIC)) // match magic word
		{
			if(!(dhcph->flags & atbm_htons(ATBM_BROADCAST_FLAG))) // if not broadcast
			{
				register int sum = 0;
				wifi_printk(WIFI_ALWAYS, "set dhcp broadcast flag!!\n");
				dhcph->flags |= atbm_htons(ATBM_BROADCAST_FLAG);
				// recalculate checksum
				sum = ~(udph->check) & 0xffff;
				sum += dhcph->flags;
				while(sum >> 16)
					sum = (sum & 0xffff) + (sum >> 16);
				udph->check = ~sum;
			}
		}
	}
}

int ieee80211_brigde_network_find_and_replace(struct atbm_buff *skb, atbm_uint32 ip){
	atbm_uint8 *mac = get_mac_byip_from_brpool(ip);
	//wifi_printk(WIFI_ALWAYS, "get ip:%d.%d.%d.%d\n", (ip & 0xff), (ip & 0xff00) >> 8, (ip & 0xff0000) >> 16, (ip & 0xff000000) >> 24);
	if(mac){
		//wifi_printk(WIFI_ALWAYS, "get mac from ip ok!!\n");
		atbm_memcpy(ATBM_OS_SKB_DATA(skb), mac, ATBM_ETH_ALEN);
		return 1;
	}
	return 0;
}

struct atbm_buff *get_sta_deliver_skb(struct atbm_buff *skb){
	struct atbmwifi_ieee8023_hdr *ehdr = (struct atbmwifi_ieee8023_hdr *) ATBM_OS_SKB_DATA(skb);
	int need_insert = 1;
	struct atbm_buff *brskb = ATBM_NULL;
	struct atbmwifi_ieee8023_hdr *brhdr;

	//wifi_printk(WIFI_ALWAYS, "ap->sta ehdr->h_proto:%x atbm_const_htons(ATBM_ETH_P_IP):%x\n", ehdr->h_proto, atbm_const_htons(ATBM_ETH_P_IP));
	switch(ehdr->h_proto){
		case atbm_const_htons(ATBM_ETH_P_IP):
		{
			struct atbm_ip_hdr *iph;
			atbm_uint32 ip;

			brskb = atbm_dev_alloc_skb(ATBM_OS_SKB_LEN(skb));
			atbm_memcpy(ATBM_OS_SKB_DATA(brskb), ATBM_OS_SKB_DATA(skb), ATBM_OS_SKB_LEN(skb));
			atbm_skb_put(brskb,ATBM_OS_SKB_LEN(skb));
			brhdr = ATBM_OS_SKB_DATA(brskb);
			//wifi_printk(WIFI_ALWAYS, "mac:"MACSTR"\n", get_staif_mac());
			atbm_memcpy(brhdr->h_source, get_staif_mac(), ATBM_ETH_ALEN);

			iph = (struct atbm_ip_hdr *)((atbm_uint8*)brhdr +sizeof(struct atbmwifi_ieee8023_hdr));
			ip = get_ip_bymac_from_brpool(brhdr->h_source);
			if(ip == iph->src.addr)
				need_insert = 0;

			if(iph->src.addr == 0)
				need_insert = 0;

			ieee80211_tx_set_dhcp_bcast_flag(brskb);
			if(need_insert){
				insert_item_to_brpool(brhdr->h_source, iph->src.addr);
			}
		}
		break;
		case atbm_const_htons(ATBM_ETH_P_ARP):
		{
			struct arphdr *arp = (struct arphdr *)(ehdr +1);
			atbm_uint32 src_ipaddr, tgt_ipaddr;
			char *src_devaddr, *tgt_devaddr;
			char *arpptr;

			if(arp->ar_pro != atbm_htons(ATBM_ETH_P_IP))
			{
				return ATBM_NULL;
			}

			brskb = atbm_dev_alloc_skb(ATBM_OS_SKB_LEN(skb));
			atbm_memcpy(ATBM_OS_SKB_DATA(brskb), ATBM_OS_SKB_DATA(skb), ATBM_OS_SKB_LEN(skb));
			atbm_skb_put(brskb,ATBM_OS_SKB_LEN(skb));
			brhdr = ATBM_OS_SKB_DATA(brskb);
			//wifi_printk(WIFI_ALWAYS, "mac:"MACSTR"\n", get_staif_mac());
			atbm_memcpy(brhdr->h_source, get_staif_mac(), ATBM_ETH_ALEN);

			arp = (struct arphdr *)(brhdr +1);
			arpptr = (char *)(arp + 1);
			src_devaddr = arpptr;
			arpptr += ATBM_ETH_ALEN;
			atbm_memcpy(&src_ipaddr, arpptr, sizeof(atbm_uint32));
			arpptr += sizeof(atbm_uint32);
			tgt_devaddr = arpptr;
			arpptr += ATBM_ETH_ALEN;
			atbm_memcpy(&tgt_ipaddr, arpptr, sizeof(atbm_uint32));
	
			insert_item_to_brpool(ehdr->h_source, src_ipaddr);
			atbm_memcpy(src_devaddr, get_staif_mac(), ATBM_ETH_ALEN);
		}
		break;
		default:
		return ATBM_NULL;
	}

	//dump_mem(ATBM_OS_SKB_DATA(skb), ATBM_OS_SKB_LEN(skb));
	return brskb;
}

struct atbm_buff *get_ap_deliver_skb(struct atbm_buff *skb){
	struct atbmwifi_ieee8023_hdr *ehdr = (struct atbmwifi_ieee8023_hdr *) ATBM_OS_SKB_DATA(skb);
	int need_insert = 1;
	struct atbm_buff *brskb = ATBM_NULL;
	struct atbmwifi_ieee8023_hdr *brhdr;

	//wifi_printk(WIFI_ALWAYS, "sta->ap ehdr->h_proto:%x atbm_const_htons(ATBM_ETH_P_IP):%x\n", ehdr->h_proto, atbm_const_htons(ATBM_ETH_P_IP));
	switch(ehdr->h_proto){
		case atbm_const_htons(ATBM_ETH_P_IP):
		{
			struct atbm_ip_hdr* iph = (struct atbm_ip_hdr *)(ehdr +1);

			if (!ieee80211_brigde_network_find_and_replace(skb, iph->dest.addr)) {
				if (*((unsigned char *)&iph->dest + 3) == 0xff) {
					// L2 is unicast but L3 is broadcast, make L2 bacome broadcast
					atbm_memset(ATBM_OS_SKB_DATA(skb), 0xff, ATBM_ETH_ALEN);
				}
				if(!atbm_is_multicast_ether_addr(ATBM_OS_SKB_DATA(skb))){
					return ATBM_NULL;
				}
			}
		}
		break;
		case atbm_const_htons(ATBM_ETH_P_ARP):
		{
			struct arphdr *arp = (struct arphdr *)(ehdr +1);
			atbm_uint32 src_ipaddr, tgt_ipaddr;
			char *sip,*tip;
			char *src_devaddr, *tgt_devaddr;
			const char *arpptr = (char *)(arp + 1);

			src_devaddr = arpptr;
			arpptr += ATBM_ETH_ALEN;
            sip = arpptr;
			atbm_memcpy(&src_ipaddr, arpptr, sizeof(atbm_uint32));
			arpptr += sizeof(atbm_uint32);
			tgt_devaddr = arpptr;
			arpptr += ATBM_ETH_ALEN;
            tip = arpptr;
			atbm_memcpy(&tgt_ipaddr, arpptr, sizeof(atbm_uint32));

			if(!ieee80211_brigde_network_find_and_replace(skb, tgt_ipaddr)){
				if(!atbm_is_multicast_ether_addr(ATBM_OS_SKB_DATA(skb))){
					return ATBM_NULL;
				}
			}
			//if(memcmp(tgt_devaddr, skb->data, ETH_ALEN)){
			//record sourc
			//frame_hexdump("\nrx beforce replace ARP:", ((char *)(arp + 1))-2,22);
			// change to ARP target mac address to Lookup result
			atbm_memcpy(tgt_devaddr, ATBM_OS_SKB_DATA(skb), ATBM_ETH_ALEN);
		}
		break;
		default:
		return ATBM_NULL;
	}

	brskb = atbm_dev_alloc_skb(ATBM_OS_SKB_LEN(skb));
	atbm_memcpy(ATBM_OS_SKB_DATA(brskb), ATBM_OS_SKB_DATA(skb), ATBM_OS_SKB_LEN(skb));
	atbm_skb_put(brskb,ATBM_OS_SKB_LEN(skb));
	//brhdr = ATBM_OS_SKB_DATA(brskb);
	//wifi_printk(WIFI_ALWAYS, "mac:"MACSTR"\n", get_staif_mac());
	//dump_mem(ATBM_OS_SKB_DATA(skb), ATBM_OS_SKB_LEN(skb));
	return brskb;
}

atbm_void atbm_brpool_init(struct atbmwifi_common *hw_priv){
	struct atbmwifi_vif *sta_priv = _atbmwifi_hwpriv_to_vifpriv(hw_priv,0);
	atbm_memset(br_pool, 0, sizeof(br_pool));
	atbm_spin_lock_init(&pool_lock);
	set_staif_mac(sta_priv->mac_addr);
	ap_priv = _atbmwifi_hwpriv_to_vifpriv(hw_priv, 1);
}

atbm_void atbm_brpool_deinit(){
}


