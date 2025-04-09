/**************************************************************************************************************
 * altobeam RTOS WSM host interface (HI) implementation
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#ifndef WPA_COMMON_H
#define WPA_COMMON_H

#include "wpa_main.h"

#define WPA_IE_VENDOR_TYPE 0x0050f201
#define WPS_IE_VENDOR_TYPE 0x0050f204
#define WPA_HOST_DEBUG
#define WMM_OUI_TYPE 2
#define WMM_OUI_SUBTYPE_INFORMATION_ELEMENT 0
#define WMM_OUI_SUBTYPE_PARAMETER_ELEMENT 1
#define WMM_OUI_SUBTYPE_TSPEC_ELEMENT 2
#define WMM_VERSION 1
#define WPAS_MAX_SCAN_SSIDS 1
#ifndef WLAN_FC_TYPE_MGMT
#define WLAN_FC_TYPE_MGMT	(0)
#endif
#ifndef WLAN_FC_TYPE_CTRL
#define WLAN_FC_TYPE_CTRL	(1)
#endif
#ifndef WLAN_FC_TYPE_DATA
#define WLAN_FC_TYPE_DATA	(2)
#endif
#ifndef WLAN_CIPHER_SUITE_GCMP
#define WLAN_CIPHER_SUITE_GCMP	(0x000FAC08)
#endif
/*
typedef enum _SECURITY_TYPE
{
	KEY_NONE = 0,
	KEY_WEP,
	KEY_WEP_SHARE,
	KEY_WPA,
	KEY_WPA2,
	KEY_MAX,
}SECURITY_TYPE;*/

#if CONFIG_IEEE80211W
#define ATBM_WPA_IGTK_LEN 16
#define ATBM_WPA_IGTK_MAX_LEN 32

#define ATBM_WPA_IGTK_KDE_PREFIX_LEN (2 + 6)
struct wpa_igtk_kde {
	atbm_uint8 keyid[2];
	atbm_uint8 pn[6];
	atbm_uint8 igtk[ATBM_WPA_IGTK_MAX_LEN];
} atbm_packed;
#endif /* CONFIG_IEEE80211W */


struct wpa_eapol_ie_parse {
	const atbm_uint8 *wpa_ie;
	atbm_size_t wpa_ie_len;
	const atbm_uint8 *rsn_ie;
	atbm_size_t rsn_ie_len;
	const atbm_uint8 *pmkid;
	const atbm_uint8 *gtk;
	atbm_size_t gtk_len;
	const atbm_uint8 *mac_addr;
	atbm_size_t mac_addr_len;
#if CONFIG_PEERKEY
	const atbm_uint8 *smk;
	atbm_size_t smk_len;
	const atbm_uint8 *nonce;
	atbm_size_t nonce_len;
	const atbm_uint8 *lifetime;
	atbm_size_t lifetime_len;
	const atbm_uint8 *error;
	atbm_size_t error_len;
#endif /* CONFIG_PEERKEY */
#if CONFIG_IEEE80211W
	const atbm_uint8 *igtk;
	atbm_size_t igtk_len;
#endif /* CONFIG_IEEE80211W */
#if CONFIG_IEEE80211R
	const atbm_uint8 *mdie;
	atbm_size_t mdie_len;
	const atbm_uint8 *ftie;
	atbm_size_t ftie_len;
	const atbm_uint8 *reassoc_deadline;
	const atbm_uint8 *key_lifetime;
#endif /* CONFIG_IEEE80211R */
};

extern int wpa_common_install_gtk(struct atbmwifi_vif *priv,atbm_uint8 *gtk,
				      atbm_uint32 pairwise_cipher,atbm_uint16 key_index);
extern int wpa_common_install_ptk(struct atbmwifi_vif *priv,struct atbmwifi_wpa_ptk *ptk,
				      atbm_uint32 pairwise_cipher,atbm_uint16 key_index);
extern int wpa_common_install_wepkey(struct atbmwifi_vif *priv,char *key,
				      atbm_uint32 pairwise_cipher,atbm_uint16 key_index,atbm_uint32 linkid);

extern int wpa_commom_key_len(int cipher);

extern int wpa_commom_cipher_to_alg(int cipher);
extern int atbmwifi_wpa_parse_wpa_ie_rsn(const atbm_uint8 *rsn_ie, atbm_size_t rsn_ie_len,
			 struct atbmwifi_wpa_ie_data *data);

extern atbm_uint32 wpa_cipher_to_suite(int proto, int cipher);
extern int wpa_parse_wpa_ie_wpa(const atbm_uint8 *wpa_ie, atbm_size_t wpa_ie_len,
				struct atbmwifi_wpa_ie_data *data);
extern int wpa_eapol_key_mic(const atbm_uint8 *key, int ver, const atbm_uint8 *buf, atbm_size_t len,
		      atbm_uint8 *mic);
extern int wpa_parse_generic(const atbm_uint8 *pos, const atbm_uint8 *end,
			     struct wpa_eapol_ie_parse *ie);

extern atbm_void wpa_pmk_to_ptk(const atbm_uint8 *pmk, atbm_size_t pmk_len, const char *label,
		    const atbm_uint8 *addr1, const atbm_uint8 *addr2,
		    const atbm_uint8 *nonce1, const atbm_uint8 *nonce2,
		    atbm_uint8 *ptk, atbm_size_t ptk_len, int use_sha256);
extern int wpa_compare_rsn_ie(int ft_initial_assoc,
		       const atbm_uint8 *ie1, atbm_size_t ie1len,
		       const atbm_uint8 *ie2, atbm_size_t ie2len);

int atbmwifi_rc4_skip(const atbm_uint8 *key, atbm_size_t keylen, atbm_size_t skip,
	     atbm_uint8 *data, atbm_size_t data_len);
atbm_void wpa_comm_init_extra_ie(struct atbmwifi_vif *priv);

extern int atbmwifi_aes_wrap(const atbm_uint8 *kek, int n, const atbm_uint8 *plain, atbm_uint8 *cipher);
extern int atbmwifi_aes_unwrap(const atbm_uint8 *kek, int n, const atbm_uint8 *cipher, atbm_uint8 *plain);
extern int eapol_input(struct atbmwifi_vif *priv,struct atbm_buff *skb);
extern int wpa_drv_send_eapol(struct atbmwifi_vif *priv,const atbm_uint8 *dest,
				    atbm_uint16 proto, const atbm_uint8 *buf, atbm_size_t len);

extern int atbmwifi_eloop_register_timeout(unsigned int secs, unsigned int msecs,
			   atbm_void (*handler)(atbm_void *eloop_ctx, atbm_void *timeout_ctx),
			   atbm_void *eloop_data, atbm_void *user_data);
extern int atbmwifi_eloop_cancel_timeout(atbm_void (*handler)(atbm_void *eloop_ctx, atbm_void *sock_ctx),
			 atbm_void *eloop_data, atbm_void *user_data);
//extern atbm_void * os_realloc_array(atbm_void *ptr, atbm_size_t nmemb, atbm_size_t size);
extern int atbmwifi_os_get_random(unsigned char *buf, atbm_size_t len);
extern int atbmwifi_os_random(atbm_void);
extern atbm_void atbmwifi_inc_byte_array(atbm_uint8 *counter, atbm_size_t len);
extern atbm_void wpa_timer_task(atbm_void);
extern atbm_void wpa_supplicant_eapol_notice_ack(struct atbmwifi_vif *priv);
atbm_void ieee802_1x_receive(struct atbmwifi_vif *priv,
							const atbm_uint8 *sa, const atbm_uint8 *buf,
							atbm_size_t len);

atbm_void atbmwifi_wpa_supplicant_rx_eapol(atbm_void *ctx, atbm_uint8 *src_addr,
			     atbm_uint8 *buf, atbm_size_t len);

extern int atbmwifi_set_key(struct atbmwifi_vif *priv,int pairwise,int linkid);
struct atbmwifi_vif * wpa_get_driver_priv(struct atbmwifi_vif *wpsdata);
int wpa_common_install_igtk(struct atbmwifi_vif *priv,const atbm_uint8 *gtk,
				  atbm_uint32 pairwise_cipher,atbm_uint16 key_index);

#endif /* EAP_COMMON_H */
