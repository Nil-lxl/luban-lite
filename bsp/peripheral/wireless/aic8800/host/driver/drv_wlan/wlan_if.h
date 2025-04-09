#ifndef _WLAN_IF_H_
#define _WLAN_IF_H_

#include "mac.h"
#include "wifi_al.h"
#include "co_list.h"
#include "netif_port.h"

extern uint8_t is_fixed_ip;
extern uint32_t fixed_ip, fixed_gw, fixed_mask;

enum AIC_WIFI_AP_STATUS
{
    AIC_WIFI_AP_IDLE,
    AIC_WIFI_AP_STARTING,
    AIC_WIFI_AP_STARTED,
    AIC_WIFI_AP_STOPPING,
    AIC_WIFI_AP_STOPPED,
};

#define MAX_AP_COUNT 32
typedef struct wifi_ap_info {
    unsigned char   ssid[32];
    unsigned char   bssid[6];
    unsigned int    channel;
    unsigned int    rssi;
} wifi_ap_info_t;

typedef struct _wifi_ap_list
{
	unsigned short  ap_count;
	wifi_ap_info_t ap_info[MAX_AP_COUNT];
}wifi_ap_list_t;

typedef void (*aic_scan_cb_t)(struct mac_scan_result *);

enum wifi_mac_acl_mode {
	WIFI_MAC_ACL_DISABLED,
	WIFI_MAC_ACL_BLACKLIST,
	WIFI_MAC_ACL_WHITELIST
};
#define WIFI_MAC_ADDR_LEN   6
struct wifi_mac_node {
	struct co_list_hdr node;
	char mac[WIFI_MAC_ADDR_LEN];
};

/* timeout_ms: 0 -> 10000, -1 -> no timeout, (timeout_ms > 0 && bit0 is 1) -> wep
 * return value: -1:password < 8, -6: dhcp fail
 */
int wlan_start_sta(uint8_t *ssid, uint8_t *pw, int timeout_ms);

int wlan_sta_connect(uint8_t *ssid, uint8_t *pw, int timeout_ms);

int wlan_start_ap(struct aic_ap_cfg *user_cfg);

int wlan_start_p2p(struct aic_p2p_cfg *user_cfg);

int wlan_disconnect_sta(uint8_t idx);

int wlan_stop_ap(void);

int wlan_stop_p2p(void);

int wlan_start_wps(void);

int wlan_dhcp(net_if_t *net_if);

int wlan_get_connect_status(void);

int wlan_ap_switch_channel(uint8_t chan_num);

int wlan_ap_disassociate_sta(struct mac_addr *macaddr);

/* should be called before 'wlan_start_ap', default (192 | (168 << 8) | (88 << 16) | (1 << 24)) */
void set_ap_ip_addr(uint32_t new_ip_addr);
uint32_t get_ap_ip_addr(void);

/* Default: 255.255.255.0 -> 0x00FFFFFF */
void set_ap_subnet_mask(uint32_t new_mask);
uint32_t get_ap_subnet_mask(void);

/* Beacon interval, default: 100 */
void set_ap_bcn_interval(uint32_t bcn_interval_ms);

void set_ap_channel_num(uint8_t num);

void set_ap_hidden_ssid(uint8_t val);

void set_ap_enable_he_rate(uint8_t en);

void set_ap_allow_sta_inactivity_s(uint8_t s);

void set_sta_connect_chan_num(uint32_t chan_num);
uint32_t get_sta_connect_chan_freq(void);

void wlan_if_scan_open(void);
void wlan_if_scan(aic_scan_cb_t scan_cb);
void wlan_if_getscan(wifi_ap_list_t *ap_list);
void wlan_if_scan_close(void);
int wlan_ap_mac_acl_init(void);
int wlan_ap_mac_acl_deinit(void);
int wlan_ap_set_mac_acl_mode(char mode);
int wlan_ap_get_mac_acl_mode(void);
int wlan_ap_add_blacklist(struct mac_addr *macaddr);
int wlan_ap_delete_blacklist(struct mac_addr *macaddr);
int wlan_ap_add_whitelist(struct mac_addr *macaddr);
int wlan_ap_delete_whitelist(struct mac_addr *macaddr);
uint8_t wlan_ap_get_mac_acl_list_cnt(void);
void * wlan_ap_get_mac_acl_list(void);

/*
 * Get the number of associated sta, max: 32
*/
uint8_t wlan_ap_get_associated_sta_cnt(void);

/*
 * Get list of associated sta
*/
void *wlan_ap_get_associated_sta_list(void);

/*
 * Get rssi of associated sta
 * [in] mac addrress of sta
*/
int8_t wlan_ap_get_associated_sta_rssi(uint8_t *addr);

extern uint8_t disconnected_by_user;

void wlan_ap_each_staid_how(void(*how)(uint8_t id));
void wlan_reset_env(void);

#endif /* _WLAN_IF_H_ */
