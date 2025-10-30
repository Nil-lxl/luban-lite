/**************************************************************************************************************
 * altobeam RTOS WSM host interface (HI) implementation
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#include "atbm_hal.h"
#include "atbm_sha1.h"
#include "wpa_common.h"

int wpa_common_install_wepkey(struct atbmwifi_vif *priv,char *key,
				      atbm_uint32 pairwise_cipher,atbm_uint16 key_index,atbm_uint32 linkid)
{
	struct atbm_cfg80211_connect_params *connect = &priv->connect;
//	wifi_printk(WIFI_DBG_MSG, "WPA:install wepkey\n\r");
	switch (pairwise_cipher) {
	case ATBM_WPA_CIPHER_WEP40:
		connect->crypto_pairwise= ATBM_WLAN_CIPHER_SUITE_WEP40;
		connect->crypto_group = ATBM_WLAN_CIPHER_SUITE_WEP40;
		atbm_memcpy(connect->key,key,5);
		connect->key_len = 5;
		break;
	case ATBM_WPA_CIPHER_WEP104:
		connect->crypto_pairwise= ATBM_WLAN_CIPHER_SUITE_WEP104;
		connect->crypto_group = ATBM_WLAN_CIPHER_SUITE_WEP104;
		atbm_memcpy(connect->key,key,13);
		connect->key_len = 13;
		break;
	case ATBM_WPA_CIPHER_NONE:
		return 0;
	default:
		
		return -1;
	}
	
 	connect->key_idx = key_index;

	atbmwifi_set_key(priv,1,linkid);

	return 0;
}
static char key_zero[64];

int wpa_common_install_gtk(struct atbmwifi_vif *priv,atbm_uint8 *gtk,
				      atbm_uint32 pairwise_cipher,atbm_uint16 key_index)
{
	struct atbm_cfg80211_connect_params *connect = &priv->connect;

	//key can't be all zero
	atbm_memset(key_zero,0,32 );
	if(atbm_memcmp(gtk,key_zero,16)==0){
		wifi_printk(WIFI_DBG_ERROR, "wpa_common_install_ptk all zero key drop\n\r");
		//dump_mem(connect->key,connect->key_len);
		return -1;
	}

	switch (pairwise_cipher) {
	case ATBM_WPA_CIPHER_CCMP:
		connect->crypto_group= ATBM_WLAN_CIPHER_SUITE_CCMP;
#if CONFIG_WPA2_REINSTALL_CERTIFICATION
		if(atbmwifi_is_sta_mode(priv->iftype)){
			if(memcmp(connect->gtk,gtk,16)==0){			
				wifi_printk(WIFI_DBG_ERROR, "wpa_common_install_ptk reintall drop\n\r");
				return -1;
			}
			atbm_memcpy(connect->gtk,gtk,16);
            atbm_memset(priv->connect.gtk_pn,0,8);
			priv->connect.gtk_pn_init = 1;
		}
#endif  //#if CONFIG_WPA2_REINSTALL_CERTIFICATION
        atbm_memcpy(connect->key,gtk,16);
		connect->key_len = 16;
		break;
	case ATBM_WPA_CIPHER_TKIP:
		connect->crypto_group = ATBM_WLAN_CIPHER_SUITE_TKIP;
#if CONFIG_WPA2_REINSTALL_CERTIFICATION
		if(atbmwifi_is_sta_mode(priv->iftype)){
			if(memcmp(connect->gtk,gtk,32)==0){			
				wifi_printk(WIFI_DBG_ERROR, "wpa_common_install_ptk reintall drop\n\r");
				return -1;
			}
			atbm_memcpy(connect->gtk,gtk,32);
			atbm_memset(priv->connect.gtk_pn,0,8);
			priv->connect.gtk_pn_init = 1;
		}
#endif  //#if CONFIG_WPA2_REINSTALL_CERTIFICATION
		atbm_memcpy(connect->key,gtk,32);
		connect->key_len = 32;
		break;
	case ATBM_WPA_CIPHER_NONE:
		return 0;
	default:
		
		return -1;
	}
	
 	connect->key_idx = key_index;

	atbmwifi_set_key(priv,1,1);

	return 0;
}
static char key_zero[64];
/*key_index = keyid | (linkid <<8)*/
int wpa_common_install_ptk(struct atbmwifi_vif *priv,struct atbmwifi_wpa_ptk *ptk,
				      atbm_uint32 pairwise_cipher,atbm_uint16 key_index)
{
	struct atbm_cfg80211_connect_params *connect = &priv->connect;
#if CONFIG_WPA2_REINSTALL_CERTIFICATION
	int  i =0;
#endif
	//key can't be all zero
	atbm_memset(key_zero,0,32 );
	if(atbm_memcmp(ptk->tk1,key_zero,16)==0){
		wifi_printk(WIFI_DBG_ERROR, "wpa_common_install_ptk all zero key drop\n\r");
		//dump_mem(connect->key,connect->key_len);
		return -1;
	}

	switch (pairwise_cipher) {
	case ATBM_WPA_CIPHER_CCMP:
		connect->crypto_pairwise= ATBM_WLAN_CIPHER_SUITE_CCMP;
#if CONFIG_WPA2_REINSTALL_CERTIFICATION
		if(atbmwifi_is_sta_mode(priv->iftype)){
			if(memcmp(connect->ptk,ptk->tk1,16)==0){			
				wifi_printk(WIFI_DBG_ERROR, "wpa_common_install_ptk reintall drop\n\r");
    			return 0;
			}
			atbm_memcpy(connect->ptk,ptk->tk1,16);
			for(i=0;i<8;i++){
				atbm_memset(priv->connect.ptk_pn[i],0,8);
				priv->connect.ptk_pn_init[i] =1;
			}	
			atbm_memset(priv->connect.ptk_noqos_pn,0,8);
			priv->connect.ptk_noqos_pn_init  = 1;
	 	}
#endif  //#if CONFIG_WPA2_REINSTALL_CERTIFICATION
		atbm_memcpy(connect->key,ptk->tk1,16);
		connect->key_len = 16;
		break;
	case ATBM_WPA_CIPHER_TKIP:		
		connect->crypto_pairwise = ATBM_WLAN_CIPHER_SUITE_TKIP;
#if CONFIG_WPA2_REINSTALL_CERTIFICATION
		if(atbmwifi_is_sta_mode(priv->iftype)){
			if(memcmp(connect->ptk,ptk->tk1,32)==0){			
				wifi_printk(WIFI_DBG_ERROR, "wpa_common_install_ptk reintall drop\n\r");
    			return 0;
			}
			atbm_memcpy(connect->ptk,ptk->tk1,32);
			for(i=0;i<8;i++){
				atbm_memset(priv->connect.ptk_pn[i],0,8);
				priv->connect.ptk_pn_init[i] =1;
			}	
			atbm_memset(priv->connect.ptk_noqos_pn,0,8);
			priv->connect.ptk_noqos_pn_init  = 1;
	 	}
#endif  //#if CONFIG_WPA2_REINSTALL_CERTIFICATION
		
		atbm_memcpy(connect->key,ptk->tk1,32);
		connect->key_len = 32;
		break;
	case ATBM_WPA_CIPHER_NONE:
		
		return 0;
	default:
		
		return -1;
	}
	

	/*key_index = keyid | (linkid <<8)*/
 	connect->key_idx = key_index&0xff;

	atbmwifi_set_key(priv,0,key_index>>8);

	return 0;
}

#if CONFIG_IEEE80211W
int wpa_common_install_igtk(struct atbmwifi_vif *priv,const atbm_uint8 *gtk,
				  atbm_uint32 pairwise_cipher,atbm_uint16 key_index)
{
	struct atbm_cfg80211_connect_params *connect = &priv->connect;

	if(pairwise_cipher != ATBM_WPA_CIPHER_AES_128_CMAC)
		return -1;
	connect->crypto_igtkgroup = ATBM_WLAN_CIPHER_SUITE_AES_CMAC;
	atbm_memcpy(connect->key, gtk, 16);
	connect->key_len = 16;

	connect->key_idx_igtk = key_index;

	atbmwifi_set_key(priv,2,1);

	return 0;
}
#endif

int wpa_commom_key_len(int cipher)
{
	switch (cipher) {
	case ATBM_WPA_CIPHER_CCMP:
	case ATBM_WPA_CIPHER_GCMP:
#if CONFIG_IEEE80211W
	case ATBM_WPA_CIPHER_AES_128_CMAC:
#endif
		return 16;
	case ATBM_WPA_CIPHER_TKIP:
		return 32;
	case ATBM_WPA_CIPHER_WEP104:
		return 13;
	case ATBM_WPA_CIPHER_WEP40:
		return 5;
	}

	return 0;
}
struct atbmwifi_vif * wpa_get_driver_priv(struct atbmwifi_vif* priv){
	return priv;
}
int wpa_commom_cipher_to_alg(int cipher)
{
	switch (cipher) {
		case ATBM_WPA_CIPHER_CCMP:
			return ATBM_WPA_ALG_CCMP;
#if 0
		case ATBM_WPA_CIPHER_GCMP:
			return ATBM_WPA_ALG_GCMP;
#endif
		case ATBM_WPA_CIPHER_TKIP:
			return ATBM_WPA_ALG_TKIP;
		case ATBM_WPA_CIPHER_WEP104:
		case ATBM_WPA_CIPHER_WEP40:
			return ATBM_WPA_ALG_WEP;
	}
	return ATBM_WPA_ALG_NONE;
}
static int rsn_selector_to_bitfield(const atbm_uint8 *s)
{
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_CIPHER_SUITE_NONE)
		return ATBM_WPA_CIPHER_NONE;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_CIPHER_SUITE_WEP40)
		return ATBM_WPA_CIPHER_WEP40;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_CIPHER_SUITE_TKIP)
		return ATBM_WPA_CIPHER_TKIP;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_CIPHER_SUITE_CCMP)
		return ATBM_WPA_CIPHER_CCMP;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_CIPHER_SUITE_WEP104)
		return ATBM_WPA_CIPHER_WEP104;
#if CONFIG_IEEE80211W
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_CIPHER_SUITE_AES_128_CMAC)
		return ATBM_WPA_CIPHER_AES_128_CMAC;
#endif /* CONFIG_IEEE80211W */
#if CONFIG_SAE
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_AUTH_KEY_MGMT_SAE)
		return ATBM_WPA_KEY_MGMT_SAE;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_AUTH_KEY_MGMT_FT_SAE)
		return ATBM_WPA_KEY_MGMT_FT_SAE;
#endif /* CONFIG_SAE */

	return 0;
}
static int rsn_key_mgmt_to_bitfield(const atbm_uint8 *s)
{
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_AUTH_KEY_MGMT_UNSPEC_802_1X)
		return ATBM_WPA_KEY_MGMT_IEEE8021X;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X)
		return ATBM_WPA_KEY_MGMT_PSK;
#if CONFIG_IEEE80211R
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_AUTH_KEY_MGMT_FT_802_1X)
		return ATBM_WPA_KEY_MGMT_FT_IEEE8021X;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_AUTH_KEY_MGMT_FT_PSK)
		return ATBM_WPA_KEY_MGMT_FT_PSK;
#endif /* CONFIG_IEEE80211R */
#if CONFIG_IEEE80211W
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_AUTH_KEY_MGMT_802_1X_SHA256)
		return ATBM_WPA_KEY_MGMT_IEEE8021X_SHA256;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_AUTH_KEY_MGMT_PSK_SHA256)
		return ATBM_WPA_KEY_MGMT_PSK_SHA256;
#endif /* CONFIG_IEEE80211W */
#if CONFIG_SAE
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_AUTH_KEY_MGMT_SAE)
		return ATBM_WPA_KEY_MGMT_SAE;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_RSN_AUTH_KEY_MGMT_FT_SAE)
		return ATBM_WPA_KEY_MGMT_FT_SAE;
#endif /* CONFIG_SAE */

	return 0;
}

int atbmwifi_wpa_parse_wpa_ie_rsn(const atbm_uint8 *rsn_ie, atbm_size_t rsn_ie_len,
			 struct atbmwifi_wpa_ie_data *data)
{
	const struct atbmwifi_rsn_ie_hdr *hdr;
	const atbm_uint8 *pos;
	int left;
	int i, count;

	atbm_memset(data, 0, sizeof(*data));
	data->proto = ATBM_WPA_PROTO_RSN;
	data->pairwise_cipher = ATBM_WPA_CIPHER_CCMP;
	data->group_cipher = ATBM_WPA_CIPHER_CCMP;
	data->key_mgmt = ATBM_WPA_KEY_MGMT_IEEE8021X;
	data->capabilities = 0;
	data->pmkid = ATBM_NULL;
	data->num_pmkid = 0;
#if CONFIG_IEEE80211W
	data->mgmt_group_cipher = ATBM_WPA_CIPHER_AES_128_CMAC;
#else /* CONFIG_IEEE80211W */
	data->mgmt_group_cipher = 0;
#endif /* CONFIG_IEEE80211W */

	if (rsn_ie_len == 0) {
		/* No RSN IE - fail silently */
		return -1;
	}

	if (rsn_ie_len < sizeof(struct atbmwifi_rsn_ie_hdr)) {
		return -1;
	}

	hdr = (const struct atbmwifi_rsn_ie_hdr *) rsn_ie;

	if (hdr->elem_id != ATBM_WLAN_EID_RSN ||
	    hdr->len != rsn_ie_len - 2 ||
	    ATBM_WPA_GET_LE16(hdr->version) != ATBM_RSN_VERSION) {
		return -2;
	}

	pos = (const atbm_uint8 *) (hdr + 1);
	left = rsn_ie_len - sizeof(*hdr);

	if (left >= ATBM_RSN_SELECTOR_LEN) {
		data->group_cipher = rsn_selector_to_bitfield(pos);
#if CONFIG_IEEE80211W
		if (data->group_cipher == ATBM_WPA_CIPHER_AES_128_CMAC) {
			wpa_printf(MSG_DEBUG, "%s: AES-128-CMAC used as group "
				   "cipher", "");
			return -1;
		}
#endif /* CONFIG_IEEE80211W */
		pos += ATBM_RSN_SELECTOR_LEN;
		left -= ATBM_RSN_SELECTOR_LEN;
	} else if (left > 0) {
		return -3;
	}

	if (left >= 2) {
		data->pairwise_cipher = 0;
		count = ATBM_WPA_GET_LE16(pos);
		pos += 2;
		left -= 2;
		if (count == 0 || left < count * ATBM_RSN_SELECTOR_LEN) {

			return -4;
		}
		for (i = 0; i < count; i++) {
			data->pairwise_cipher |= rsn_selector_to_bitfield(pos);
			pos += ATBM_RSN_SELECTOR_LEN;
			left -= ATBM_RSN_SELECTOR_LEN;
		}
#if CONFIG_IEEE80211W
		if (data->pairwise_cipher & ATBM_WPA_CIPHER_AES_128_CMAC) {
			wpa_printf(MSG_DEBUG, "%s: AES-128-CMAC used as "
				   "pairwise cipher", "");
			return -1;
		}
#endif /* CONFIG_IEEE80211W */
	} else if (left == 1) {

		return -5;
	}

	if (left >= 2) {
		data->key_mgmt = 0;
		count = ATBM_WPA_GET_LE16(pos);
		pos += 2;
		left -= 2;
		if (count == 0 || left < count * ATBM_RSN_SELECTOR_LEN) {

			return -6;
		}
		for (i = 0; i < count; i++) {
			data->key_mgmt |= rsn_key_mgmt_to_bitfield(pos);
			pos += ATBM_RSN_SELECTOR_LEN;
			left -= ATBM_RSN_SELECTOR_LEN;
		}
	} else if (left == 1) {

		return -7;
	}

	if (left >= 2) {
		data->capabilities = ATBM_WPA_GET_LE16(pos);
		pos += 2;
		left -= 2;
	}

	if (left >= 2) {
		data->num_pmkid = ATBM_WPA_GET_LE16(pos);
		pos += 2;
		left -= 2;
		if (left < (int) data->num_pmkid * ATBM_PMKID_LEN) {

			data->num_pmkid = 0;
			return -9;
		} else {
			data->pmkid = pos;
			pos += data->num_pmkid * ATBM_PMKID_LEN;
			left -= data->num_pmkid * ATBM_PMKID_LEN;
		}
	}

#if CONFIG_IEEE80211W
	if (left >= 4) {
		data->mgmt_group_cipher = rsn_selector_to_bitfield(pos);
		if (data->mgmt_group_cipher != ATBM_WPA_CIPHER_AES_128_CMAC) {
			wpa_printf(MSG_DEBUG, "%s: Unsupported management "
				   "group cipher 0x%x", "",
				   data->mgmt_group_cipher);
			return -10;
		}
		pos += ATBM_RSN_SELECTOR_LEN;
		left -= ATBM_RSN_SELECTOR_LEN;
	}
#endif /* CONFIG_IEEE80211W */

	return 0;
}

atbm_uint32 wpa_cipher_to_suite(int proto, int cipher)
{
	if (cipher & ATBM_WPA_CIPHER_CCMP)
		return (proto == ATBM_WPA_PROTO_RSN ?
			ATBM_RSN_CIPHER_SUITE_CCMP : ATBM_WPA_CIPHER_SUITE_CCMP);
#if 0
	if (cipher & WPA_CIPHER_GCMP)
		return RSN_CIPHER_SUITE_GCMP;
#endif
	if (cipher & ATBM_WPA_CIPHER_TKIP)
		return (proto == ATBM_WPA_PROTO_RSN ?
			ATBM_RSN_CIPHER_SUITE_TKIP : ATBM_WPA_CIPHER_SUITE_TKIP);
	if (cipher & ATBM_WPA_CIPHER_WEP104)
		return (proto == ATBM_WPA_PROTO_RSN ?
			ATBM_RSN_CIPHER_SUITE_WEP104 : ATBM_WPA_CIPHER_SUITE_WEP104);
	if (cipher & ATBM_WPA_CIPHER_WEP40)
		return (proto == ATBM_WPA_PROTO_RSN ?
			ATBM_RSN_CIPHER_SUITE_WEP40 : ATBM_WPA_CIPHER_SUITE_WEP40);
	if (cipher & ATBM_WPA_CIPHER_NONE)
		return (proto == ATBM_WPA_PROTO_RSN ?
			ATBM_RSN_CIPHER_SUITE_NONE : ATBM_WPA_CIPHER_SUITE_NONE);
	return 0;
}
static int wpa_key_mgmt_to_bitfield(const atbm_uint8 *s)
{
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_WPA_AUTH_KEY_MGMT_UNSPEC_802_1X)
		return ATBM_WPA_KEY_MGMT_IEEE8021X;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_WPA_AUTH_KEY_MGMT_PSK_OVER_802_1X)
		return ATBM_WPA_KEY_MGMT_PSK;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_WPA_AUTH_KEY_MGMT_NONE)
		return ATBM_WPA_KEY_MGMT_WPA_NONE;
	return 0;
}

static int wpa_selector_to_bitfield(const atbm_uint8 *s)
{
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_WPA_CIPHER_SUITE_NONE)
		return ATBM_WPA_CIPHER_NONE;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_WPA_CIPHER_SUITE_WEP40)
		return ATBM_WPA_CIPHER_WEP40;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_WPA_CIPHER_SUITE_TKIP)
		return ATBM_WPA_CIPHER_TKIP;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_WPA_CIPHER_SUITE_CCMP)
		return ATBM_WPA_CIPHER_CCMP;
	if (ATBM_RSN_SELECTOR_GET(s) == ATBM_WPA_CIPHER_SUITE_WEP104)
		return ATBM_WPA_CIPHER_WEP104;
	return 0;
}

int wpa_parse_wpa_ie_wpa(const atbm_uint8 *wpa_ie, atbm_size_t wpa_ie_len,
				struct atbmwifi_wpa_ie_data *data)
{
	const struct atbmwifi_wpa_ie_hdr *hdr;
	const atbm_uint8 *pos;
	int left;
	int i, count;

	atbm_memset(data, 0, sizeof(*data));
	data->proto = ATBM_WPA_PROTO_WPA;
	data->pairwise_cipher = ATBM_WPA_CIPHER_TKIP;
	data->group_cipher = ATBM_WPA_CIPHER_TKIP;
	data->key_mgmt = ATBM_WPA_KEY_MGMT_IEEE8021X;
	data->capabilities = 0;
	data->pmkid = ATBM_NULL;
	data->num_pmkid = 0;
	data->mgmt_group_cipher = 0;

	if (wpa_ie_len == 0) {
		/* No WPA IE - fail silently */
		return -1;
	}

	if (wpa_ie_len < sizeof(struct atbmwifi_wpa_ie_hdr)) {

		return -1;
	}

	hdr = (const struct atbmwifi_wpa_ie_hdr *) wpa_ie;

	if (hdr->elem_id != ATBM_WLAN_EID_VENDOR_SPECIFIC ||
	    hdr->len != wpa_ie_len - 2 ||
	    ATBM_RSN_SELECTOR_GET(hdr->oui) != ATBM_WPA_OUI_TYPE ||
	    ATBM_WPA_GET_LE16(hdr->version) != ATBM_WPA_VERSION) {

		return -1;
	}

	pos = (const atbm_uint8 *) (hdr + 1);
	left = wpa_ie_len - sizeof(*hdr);

	if (left >= ATBM_WPA_SELECTOR_LEN) {
		data->group_cipher = wpa_selector_to_bitfield(pos);
		pos += ATBM_WPA_SELECTOR_LEN;
		left -= ATBM_WPA_SELECTOR_LEN;
	} else if (left > 0) {

		return -1;
	}

	if (left >= 2) {
		data->pairwise_cipher = 0;
		count = ATBM_WPA_GET_LE16(pos);
		pos += 2;
		left -= 2;
		if (count == 0 || left < count * ATBM_WPA_SELECTOR_LEN) {

			return -1;
		}
		for (i = 0; i < count; i++) {
			data->pairwise_cipher |= wpa_selector_to_bitfield(pos);
			pos += ATBM_WPA_SELECTOR_LEN;
			left -= ATBM_WPA_SELECTOR_LEN;
		}
	} else if (left == 1) {

		return -1;
	}

	if (left >= 2) {
		data->key_mgmt = 0;
		count = ATBM_WPA_GET_LE16(pos);
		pos += 2;
		left -= 2;
		if (count == 0 || left < count * ATBM_WPA_SELECTOR_LEN) {

			return -1;
		}
		for (i = 0; i < count; i++) {
			data->key_mgmt |= wpa_key_mgmt_to_bitfield(pos);
			pos += ATBM_WPA_SELECTOR_LEN;
			left -= ATBM_WPA_SELECTOR_LEN;
		}
	} else if (left == 1) {

		return -1;
	}

	if (left >= 2) {
		data->capabilities = ATBM_WPA_GET_LE16(pos);
		pos += 2;
		left -= 2;
	}

	return 0;
}
int rsn_cipher_put_suites(atbm_uint8 *pos, int ciphers)
{
	int num_suites = 0;

	if (ciphers & ATBM_WPA_CIPHER_CCMP) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_CIPHER_SUITE_CCMP);
		pos += ATBM_RSN_SELECTOR_LEN;
		num_suites++;
	}
#if 0
	if (ciphers & WPA_CIPHER_GCMP) {
		ATBM_RSN_SELECTOR_PUT(pos, RSN_CIPHER_SUITE_GCMP);
		pos += ATBM_RSN_SELECTOR_LEN;
		num_suites++;
	}
#endif
	if (ciphers & ATBM_WPA_CIPHER_TKIP) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_CIPHER_SUITE_TKIP);
		pos += ATBM_RSN_SELECTOR_LEN;
		num_suites++;
	}
	if (ciphers & ATBM_WPA_CIPHER_NONE) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_CIPHER_SUITE_NONE);
		pos += ATBM_RSN_SELECTOR_LEN;
		num_suites++;
	}

	return num_suites;
}
int wpa_cipher_put_suites(atbm_uint8 *pos, int ciphers)
{
	int num_suites = 0;

	if (ciphers & ATBM_WPA_CIPHER_CCMP) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_CIPHER_SUITE_CCMP);
		pos += ATBM_WPA_SELECTOR_LEN;
		num_suites++;
	}
	if (ciphers & ATBM_WPA_CIPHER_TKIP) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_CIPHER_SUITE_TKIP);
		pos += ATBM_WPA_SELECTOR_LEN;
		num_suites++;
	}
	if (ciphers & ATBM_WPA_CIPHER_NONE) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_CIPHER_SUITE_NONE);
		pos += ATBM_WPA_SELECTOR_LEN;
		num_suites++;
	}

	return num_suites;
}

int wpa_write_rsn_ie(struct atbmwifi_cfg *conf, atbm_uint8 *buf, atbm_size_t len,
		     const atbm_uint8 *pmkid)
{
	struct atbmwifi_rsn_ie_hdr *hdr;
	int num_suites, res;
	atbm_uint8 *pos, *count;
	atbm_uint16 capab;
	atbm_uint32 suite;

	hdr = (struct atbmwifi_rsn_ie_hdr *) buf;
	hdr->elem_id = ATBM_WLAN_EID_RSN;
	ATBM_WPA_PUT_LE16(hdr->version, ATBM_RSN_VERSION);
	pos = (atbm_uint8 *) (hdr + 1);

	suite = wpa_cipher_to_suite(ATBM_WPA_PROTO_RSN, conf->group_cipher);
	if (suite == 0) {

		return -1;
	}
	ATBM_RSN_SELECTOR_PUT(pos, suite);
	pos += ATBM_RSN_SELECTOR_LEN;

	num_suites = 0;
	count = pos;
	pos += 2;



	res = rsn_cipher_put_suites(pos, conf->pairwise_cipher);
	num_suites += res;
	pos += res * ATBM_RSN_SELECTOR_LEN;

	if (num_suites == 0) {

		return -1;
	}
	ATBM_WPA_PUT_LE16(count, num_suites);

	num_suites = 0;
	count = pos;
	pos += 2;

	if (conf->key_mgmt & ATBM_WPA_KEY_MGMT_IEEE8021X) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_UNSPEC_802_1X);
		pos += ATBM_RSN_SELECTOR_LEN;
		num_suites++;
	}
	if (conf->key_mgmt & ATBM_WPA_KEY_MGMT_PSK) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X);
		pos += ATBM_RSN_SELECTOR_LEN;
		num_suites++;
	}
#if CONFIG_IEEE80211R
	if (conf->key_mgmt & ATBM_WPA_KEY_MGMT_FT_IEEE8021X) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_FT_802_1X);
		pos += ATBM_RSN_SELECTOR_LEN;
		num_suites++;
	}
	if (conf->key_mgmt & ATBM_WPA_KEY_MGMT_FT_PSK) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_FT_PSK);
		pos += ATBM_RSN_SELECTOR_LEN;
		num_suites++;
	}
#endif /* CONFIG_IEEE80211R */
#if CONFIG_IEEE80211W
	if (conf->key_mgmt & ATBM_WPA_KEY_MGMT_IEEE8021X_SHA256) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_802_1X_SHA256);
		pos += ATBM_RSN_SELECTOR_LEN;
		num_suites++;
	}
	if (conf->key_mgmt & ATBM_WPA_KEY_MGMT_PSK_SHA256) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_PSK_SHA256);
		pos += ATBM_RSN_SELECTOR_LEN;
		num_suites++;
	}
#endif /* CONFIG_IEEE80211W */
#ifdef CONFIG_SAE
	if (conf->key_mgmt & ATBM_WPA_KEY_MGMT_SAE) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_SAE);
		pos += ATBM_RSN_SELECTOR_LEN;
		num_suites++;
	}
	if (conf->key_mgmt & ATBM_WPA_KEY_MGMT_FT_SAE) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_FT_SAE);
		pos += ATBM_RSN_SELECTOR_LEN;
		num_suites++;
	}
#endif /* CONFIG_SAE */

	if (num_suites == 0) {
		return -1;
	}
	ATBM_WPA_PUT_LE16(count, num_suites);

	/* RSN Capabilities */
	capab = 0;
#if 0
	if (conf->rsn_preauth)
		capab |= WPA_CAPABILITY_PREAUTH;
	if (conf->peerkey)
		capab |= WPA_CAPABILITY_PEERKEY_ENABLED;
	if (conf->wmm_enabled) {
		/* 4 PTKSA replay counters when using WMM */
		capab |= (RSN_NUM_REPLAY_COUNTERS_16 << 2);
	}
#endif
#if CONFIG_IEEE80211W
	if (conf->ieee80211w != ATBM_NO_MGMT_FRAME_PROTECTION) {
		capab |= ATBM_WPA_CAPABILITY_MFPC;
		if (conf->ieee80211w == ATBM_MGMT_FRAME_PROTECTION_REQUIRED)
			capab |= ATBM_WPA_CAPABILITY_MFPR;
	}
#endif /* CONFIG_IEEE80211W */

	ATBM_WPA_PUT_LE16(pos, capab);
	pos += 2;

	if (pmkid) {
		if (pos + 2 + ATBM_PMKID_LEN > buf + len)
			return -1;
		/* PMKID Count */
		ATBM_WPA_PUT_LE16(pos, 1);
		pos += 2;
		atbm_memcpy(pos, pmkid, ATBM_PMKID_LEN);
		pos += ATBM_PMKID_LEN;
	}

#if CONFIG_IEEE80211W
	if (conf->ieee80211w != ATBM_NO_MGMT_FRAME_PROTECTION) {
		if (pos + 2 + 4 > buf + len)
			return -1;
		if (pmkid == ATBM_NULL) {
			/* PMKID Count */
			ATBM_WPA_PUT_LE16(pos, 0);
			pos += 2;
		}

		/* Management Group Cipher Suite */
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_CIPHER_SUITE_AES_128_CMAC);
		pos += ATBM_RSN_SELECTOR_LEN;
	}
#endif /* CONFIG_IEEE80211W */

	hdr->len = (pos - buf) - 2;

	return pos - buf;
}

int wpa_write_wpa_ie(struct atbmwifi_cfg *conf, atbm_uint8 *buf, atbm_size_t len)
{
	struct atbmwifi_wpa_ie_hdr *hdr;
	int num_suites;
	atbm_uint8 *pos, *count;
	atbm_uint32 suite;

	hdr = (struct atbmwifi_wpa_ie_hdr *) buf;
	hdr->elem_id = ATBM_WLAN_EID_VENDOR_SPECIFIC;
	ATBM_RSN_SELECTOR_PUT(hdr->oui, ATBM_WPA_OUI_TYPE);
	ATBM_WPA_PUT_LE16(hdr->version, ATBM_WPA_VERSION);
	pos = (atbm_uint8 *) (hdr + 1);

	suite = wpa_cipher_to_suite(ATBM_WPA_PROTO_WPA, conf->group_cipher);
	if (suite == 0) {

	}
	ATBM_RSN_SELECTOR_PUT(pos, suite);
	pos += ATBM_WPA_SELECTOR_LEN;

	count = pos;
	pos += 2;

	num_suites = wpa_cipher_put_suites(pos, conf->pairwise_cipher);
	if (num_suites == 0) {

		return -1;
	}
	pos += num_suites * ATBM_WPA_SELECTOR_LEN;
	ATBM_WPA_PUT_LE16(count, num_suites);

	num_suites = 0;
	count = pos;
	pos += 2;

	if (conf->key_mgmt& ATBM_WPA_KEY_MGMT_IEEE8021X) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_AUTH_KEY_MGMT_UNSPEC_802_1X);
		pos += ATBM_WPA_SELECTOR_LEN;
		num_suites++;
	}
	if (conf->key_mgmt & ATBM_WPA_KEY_MGMT_PSK) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_AUTH_KEY_MGMT_PSK_OVER_802_1X);
		pos += ATBM_WPA_SELECTOR_LEN;
		num_suites++;
	}

	if (num_suites == 0) {

		return -1;
	}
	ATBM_WPA_PUT_LE16(count, num_suites);

	/* WPA Capabilities; use defaults, so no need to include it */

	hdr->len = (pos - buf) - 2;

	return pos - buf;
}

atbm_void wpa_comm_init_extra_ie(struct atbmwifi_vif *priv)
{
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);
	struct atbmwifi_cfg80211_bss  *bss = &priv->bss;
	atbm_uint8 *wpa_ie = ATBM_NULL;
	int wpa_ie_len = 0;
	atbm_uint8 *pos;

	if(!config->wpa)
	{
		goto __exit0;
	}
	
	wpa_ie = (atbm_uint8 *)atbm_kmalloc(500,GFP_KERNEL);
	pos = wpa_ie;
	
	if(config->wpa & ATBM_WPA_PROTO_RSN)
	{
		wpa_ie_len = wpa_write_rsn_ie(config,
			       pos, wpa_ie +500 - pos, ATBM_NULL);

		if(wpa_ie_len<0)
		{
			goto __exit0 ;
		}

		pos += wpa_ie_len;
	}

	if(config->wpa & ATBM_WPA_PROTO_WPA)
	{
		wpa_ie_len =  wpa_write_wpa_ie(config,
			       pos, wpa_ie + sizeof(wpa_ie) - pos);

		if(wpa_ie_len <0 )
		{
			goto __exit0;
		}

		pos += wpa_ie_len;
	}

#if CONFIG_WPS
//	TO DO ADD WPS IE
#endif 

#if CONFIG_P2P
//  TO DO ADD WPS IE
#endif

	if((wpa_ie != ATBM_NULL)&&( wpa_ie_len != 0))
	{
		if(priv->extra_ie){
			atbm_kfree(priv->extra_ie);
			//priv->extra_ie = NULL;
		}
		priv->extra_ie = wpa_ie;
		priv->extra_ie_len = pos - wpa_ie;
		if(priv->bss.information_elements &&
			(priv->bss.len_information_elements < priv->extra_ie_len)){
			atbm_kfree(priv->bss.information_elements);
			priv->bss.information_elements = ATBM_NULL;
		}
		
		if(priv->bss.information_elements == ATBM_NULL){
			priv->bss.information_elements = (atbm_uint8 *)atbm_kmalloc(priv->extra_ie_len,GFP_KERNEL);
			if(priv->bss.information_elements == ATBM_NULL)
			{	
				priv->extra_ie = ATBM_NULL;
				priv->extra_ie_len = 0;
				goto __exit0 ;
			}
		}
		atbm_memcpy(bss->information_elements,wpa_ie,priv->extra_ie_len);
		bss->len_information_elements = priv->extra_ie_len;
		//wpa_ie have been saved in priv->extra_ie,so not free it 
		wpa_ie = ATBM_NULL;
	}
	wifi_printk(WIFI_DBG_INIT,"wpa_comm_init_extra_ie len (%d),proto(%s)\n",priv->extra_ie_len,config->wpa & ATBM_WPA_PROTO_WPA ? "ATBM_WPA_PROTO_WPA":"ATBM_WPA_PROTO_RSN");

__exit0:	
	atbm_kfree(wpa_ie);
	return ;
}


atbm_void atbmwifi_inc_byte_array(atbm_uint8 *counter, atbm_size_t len)
{
	int pos = len - 1;
	while (pos >= 0) {
		counter[pos]++;
		if (counter[pos] != 0)
			break;
		pos--;
	}
}

int atbmwifi_hmac_md5_vector(const atbm_uint8 *key, atbm_size_t key_len, atbm_size_t num_elem,
		    const atbm_uint8 *addr[], const atbm_size_t *len, atbm_uint8 *mac)
{
	atbm_uint8 k_pad[64]; /* padding - key XORd with ipad/opad */
	atbm_uint8 tk[16];
	const atbm_uint8 *_addr[6];
	atbm_size_t i, _len[6];

	if (num_elem > 5) {
		/*
		 * Fixed limit on the number of fragments to avoid having to
		 * allocate memory (which could fail).
		 */
		return -1;
	}

        /* if key is longer than 64 bytes reset it to key = MD5(key) */
        if (key_len > 64) {
		if (atbmwifi_md5_vector(1, &key, &key_len, tk))
			return -1;
		key = tk;
		key_len = 16;
        }

	/* the HMAC_MD5 transform looks like:
	 *
	 * MD5(K XOR opad, MD5(K XOR ipad, text))
	 *
	 * where K is an n byte key
	 * ipad is the byte 0x36 repeated 64 times
	 * opad is the byte 0x5c repeated 64 times
	 * and text is the data being protected */

	/* start out by storing key in ipad */
	atbm_memset(k_pad, 0, sizeof(k_pad));
	atbm_memcpy(k_pad, key, key_len);

	/* XOR key with ipad values */
	for (i = 0; i < 64; i++)
		k_pad[i] ^= 0x36;

	/* perform inner MD5 */
	_addr[0] = k_pad;
	_len[0] = 64;
	for (i = 0; i < num_elem; i++) {
		_addr[i + 1] = addr[i];
		_len[i + 1] = len[i];
	}
	if (atbmwifi_md5_vector(1 + num_elem, _addr, _len, mac))
		return -1;

	atbm_memset(k_pad, 0, sizeof(k_pad));
	atbm_memcpy(k_pad, key, key_len);
	/* XOR key with opad values */
	for (i = 0; i < 64; i++)
		k_pad[i] ^= 0x5c;

	/* perform outer MD5 */
	_addr[0] = k_pad;
	_len[0] = 64;
	_addr[1] = mac;
	_len[1] = MD5_MAC_LEN;
	return atbmwifi_md5_vector(2, _addr, _len, mac);
}

int atbmwifi_hmac_md5(const atbm_uint8 *key, atbm_size_t key_len, const atbm_uint8 *data, atbm_size_t data_len,
	      atbm_uint8 *mac)
{
	return atbmwifi_hmac_md5_vector(key, key_len, 1, &data, &data_len, mac);
}

int wpa_eapol_key_mic(const atbm_uint8 *key, int ver, const atbm_uint8 *buf, atbm_size_t len,
		      atbm_uint8 *mic)
{
	atbm_uint8 hash[SHA1_MAC_LEN];

	switch (ver) {
	case ATBM_WPA_KEY_INFO_TYPE_HMAC_MD5_RC4:
		return atbmwifi_hmac_md5(key, 16, buf, len, mic);
	case ATBM_WPA_KEY_INFO_TYPE_HMAC_SHA1_AES:
		if (atbm_hmac_sha1(key, 16, buf, len, hash))
			return -1;
		atbm_memcpy(mic, hash, MD5_MAC_LEN);
		break;
#if ((CONFIG_IEEE80211R==1) || (CONFIG_IEEE80211W==1))
	case ATBM_WPA_KEY_INFO_TYPE_AES_128_CMAC:
		return atbmwifi_omac1_aes_128(key, buf, len, mic);
#endif /* CONFIG_IEEE80211R || CONFIG_IEEE80211W */
#if CONFIG_SAE
	case ATBM_WPA_KEY_INFO_TYPE_AKM_DEFINED:
		wpa_printf(MSG_DEBUG,
				   "WPA: EAPOL-Key MIC using AES-CMAC (AKM-defined - SAE)");
		return atbmwifi_omac1_aes_128(key, buf, len, mic);
#endif

	default:
		return -1;
	}

	return 0;
}


int wpa_parse_generic(const atbm_uint8 *pos, const atbm_uint8 *end,
			     struct wpa_eapol_ie_parse *ie)
{
	if (pos[1] == 0)
		return 1;

	if (pos[1] >= 6 &&
	    ATBM_RSN_SELECTOR_GET(pos + 2) == ATBM_WPA_OUI_TYPE &&
	    pos[2 + ATBM_WPA_SELECTOR_LEN] == 1 &&
	    pos[2 + ATBM_WPA_SELECTOR_LEN + 1] == 0) {
		ie->wpa_ie = pos;
		ie->wpa_ie_len = pos[1] + 2;

		return 0;
	}

	if (pos + 1 + ATBM_RSN_SELECTOR_LEN < end &&
	    pos[1] >= ATBM_RSN_SELECTOR_LEN + ATBM_PMKID_LEN &&
	    ATBM_RSN_SELECTOR_GET(pos + 2) == ATBM_RSN_KEY_DATA_PMKID) {
		ie->pmkid = pos + 2 + ATBM_RSN_SELECTOR_LEN;

		return 0;
	}

	if (pos[1] > ATBM_RSN_SELECTOR_LEN + 2 &&
	    ATBM_RSN_SELECTOR_GET(pos + 2) == ATBM_RSN_KEY_DATA_GROUPKEY) {
		ie->gtk = pos + 2 + ATBM_RSN_SELECTOR_LEN;
		ie->gtk_len = pos[1] - ATBM_RSN_SELECTOR_LEN;

		return 0;
	}

	if (pos[1] > ATBM_RSN_SELECTOR_LEN + 2 &&
	    ATBM_RSN_SELECTOR_GET(pos + 2) == ATBM_RSN_KEY_DATA_MAC_ADDR) {
		ie->mac_addr = pos + 2 + ATBM_RSN_SELECTOR_LEN;
		ie->mac_addr_len = pos[1] - ATBM_RSN_SELECTOR_LEN;

		return 0;
	}

#if CONFIG_PEERKEY
	if (pos[1] > ATBM_RSN_SELECTOR_LEN + 2 &&
	    ATBM_RSN_SELECTOR_GET(pos + 2) == ATBM_RSN_KEY_DATA_SMK) {
		ie->smk = pos + 2 + ATBM_RSN_SELECTOR_LEN;
		ie->smk_len = pos[1] - ATBM_RSN_SELECTOR_LEN;
		wpa_hexdump_key(MSG_DEBUG, "WPA: SMK in EAPOL-Key",
				(const atbm_uint8*)pos, pos[1] + 2);
		return 0;
	}

	if (pos[1] > ATBM_RSN_SELECTOR_LEN + 2 &&
	    ATBM_RSN_SELECTOR_GET(pos + 2) == ATBM_RSN_KEY_DATA_NONCE) {
		ie->nonce = pos + 2 + ATBM_RSN_SELECTOR_LEN;
		ie->nonce_len = pos[1] - ATBM_RSN_SELECTOR_LEN;
		wpa_hexdump(MSG_DEBUG, "WPA: Nonce in EAPOL-Key",
			    (const atbm_uint8*)pos, pos[1] + 2);
		return 0;
	}

	if (pos[1] > ATBM_RSN_SELECTOR_LEN + 2 &&
	    ATBM_RSN_SELECTOR_GET(pos + 2) == ATBM_RSN_KEY_DATA_LIFETIME) {
		ie->lifetime = pos + 2 + ATBM_RSN_SELECTOR_LEN;
		ie->lifetime_len = pos[1] - ATBM_RSN_SELECTOR_LEN;

		return 0;
	}

	if (pos[1] > ATBM_RSN_SELECTOR_LEN + 2 &&
	    ATBM_RSN_SELECTOR_GET(pos + 2) == ATBM_RSN_KEY_DATA_ERROR) {
		ie->error = pos + 2 + ATBM_RSN_SELECTOR_LEN;
		ie->error_len = pos[1] - ATBM_RSN_SELECTOR_LEN;

		return 0;
	}
#endif /* CONFIG_PEERKEY */

#if CONFIG_IEEE80211W
	if (pos[1] > ATBM_RSN_SELECTOR_LEN + 2 &&
	    ATBM_RSN_SELECTOR_GET(pos + 2) == ATBM_RSN_KEY_DATA_IGTK) {
		ie->igtk = pos + 2 + ATBM_RSN_SELECTOR_LEN;
		ie->igtk_len = pos[1] - ATBM_RSN_SELECTOR_LEN;
		wpa_hexdump(MSG_DEBUG, "WPA: IGTK in EAPOL-Key",
				(const atbm_uint8*)pos, pos[1] + 2);
		return 0;
	}
#endif /* CONFIG_IEEE80211W */

	return 0;
}


/**
 * wpa_pmk_to_ptk - Calculate PTK from PMK, addresses, and nonces
 * @pmk: Pairwise master key
 * @ATBM_PMK_LEN: Length of PMK
 * @label: Label to use in derivation
 * @addr1: AA or SA
 * @addr2: SA or AA
 * @nonce1: ANonce or SNonce
 * @nonce2: SNonce or ANonce
 * @ptk: Buffer for pairwise transient key
 * @ptk_len: Length of PTK
 * @use_sha256: Whether to use SHA256-based KDF
 *
 * IEEE Std 802.11i-2004 - 8.5.1.2 Pairwise key hierarchy
 * PTK = PRF-X(PMK, "Pairwise key expansion",
 *             Min(AA, SA) || Max(AA, SA) ||
 *             Min(ANonce, SNonce) || Max(ANonce, SNonce))
 *
 * STK = PRF-X(SMK, "Peer key expansion",
 *             Min(MAC_I, MAC_P) || Max(MAC_I, MAC_P) ||
 *             Min(INonce, PNonce) || Max(INonce, PNonce))
 */
atbm_void wpa_pmk_to_ptk(const atbm_uint8 *pmk, atbm_size_t pmk_len, const char *label,
		    const atbm_uint8 *addr1, const atbm_uint8 *addr2,
		    const atbm_uint8 *nonce1, const atbm_uint8 *nonce2,
		    atbm_uint8 *ptk, atbm_size_t ptk_len, int use_sha256)
{
	atbm_uint8 data[2 * ATBM_ETH_ALEN + 2 * ATBM_WPA_NONCE_LEN];

	if (atbm_memcmp(addr1, addr2, ATBM_ETH_ALEN) < 0) {
		atbm_memcpy(data, addr1, ATBM_ETH_ALEN);
		atbm_memcpy(data + ATBM_ETH_ALEN, addr2, ATBM_ETH_ALEN);
	} else {
		atbm_memcpy(data, addr2, ATBM_ETH_ALEN);
		atbm_memcpy(data + ATBM_ETH_ALEN, addr1, ATBM_ETH_ALEN);
	}

	if (atbm_memcmp(nonce1, nonce2, ATBM_WPA_NONCE_LEN) < 0) {
		atbm_memcpy(data + 2 * ATBM_ETH_ALEN, nonce1, ATBM_WPA_NONCE_LEN);
		atbm_memcpy(data + 2 * ATBM_ETH_ALEN + ATBM_WPA_NONCE_LEN, nonce2,
			  ATBM_WPA_NONCE_LEN);
	} else {
		atbm_memcpy(data + 2 * ATBM_ETH_ALEN, nonce2, ATBM_WPA_NONCE_LEN);
		atbm_memcpy(data + 2 * ATBM_ETH_ALEN + ATBM_WPA_NONCE_LEN, nonce1,
			  ATBM_WPA_NONCE_LEN);
	}

#if CONFIG_IEEE80211W || CONFIG_SAE
	if (use_sha256)
		atbmwifi_sha256_prf(pmk, pmk_len, label, data, sizeof(data),
			   ptk, ptk_len);
	else
#endif /* CONFIG_IEEE80211W */
		atbm_sha1_prf(pmk, pmk_len, label, data, sizeof(data), ptk,
			 ptk_len);
}

int wpa_compare_rsn_ie(int ft_initial_assoc,
		       const atbm_uint8 *ie1, atbm_size_t ie1len,
		       const atbm_uint8 *ie2, atbm_size_t ie2len)
{
	if (ie1 == ATBM_NULL || ie2 == ATBM_NULL)
		return -1;

	if (ie1len == ie2len && atbm_memcmp(ie1, ie2, ie1len) == 0)
		return 0; /* identical IEs */

#if CONFIG_IEEE80211R
	if (ft_initial_assoc) {
		struct atbmwifi_wpa_ie_data ie1d, ie2d;
		/*
		 * The PMKID-List in RSN IE is different between Beacon/Probe
		 * Response/(Re)Association Request frames and EAPOL-Key
		 * messages in FT initial mobility domain association. Allow
		 * for this, but verify that other parts of the RSN IEs are
		 * identical.
		 */
		if (atbmwifi_wpa_parse_wpa_ie_rsn(ie1, ie1len, &ie1d) < 0 ||
		    atbmwifi_wpa_parse_wpa_ie_rsn(ie2, ie2len, &ie2d) < 0)
			return -1;
		if (ie1d.proto == ie2d.proto &&
		    ie1d.pairwise_cipher == ie2d.pairwise_cipher &&
		    ie1d.group_cipher == ie2d.group_cipher &&
		    ie1d.key_mgmt == ie2d.key_mgmt &&
		    ie1d.capabilities == ie2d.capabilities &&
		    ie1d.mgmt_group_cipher == ie2d.mgmt_group_cipher)
			return 0;
	}
#endif /* CONFIG_IEEE80211R */

	return -1;
}
#define S_SWAP(a,b) do { atbm_uint8 t = S[a]; S[a] = S[b]; S[b] = t; } while(0)


int atbmwifi_rc4_skip(const atbm_uint8 *key, atbm_size_t keylen, atbm_size_t skip,
	     atbm_uint8 *data, atbm_size_t data_len)
{
	atbm_uint32 i, j, k;
	atbm_uint8 *S, *pos;
	atbm_size_t kpos;

//	p_dbg("enter atbmwifi_rc4_skip\n");
	//release ok
	S = (atbm_uint8 *)atbm_kmalloc(256,GFP_KERNEL);
	if(S == 0)
		return -1;

	/* Setup RC4 state */
	for (i = 0; i < 256; i++)
		S[i] = i;
	j = 0;
	kpos = 0;
	for (i = 0; i < 256; i++) {
		j = (j + S[i] + key[kpos]) & 0xff;
		kpos++;
		if (kpos >= keylen)
			kpos = 0;
		S_SWAP(i, j);
	}

	/* Skip the start of the stream */
	i = j = 0;
	for (k = 0; k < skip; k++) {
		i = (i + 1) & 0xff;
		j = (j + S[i]) & 0xff;
		S_SWAP(i, j);
	}

	/* Apply RC4 to data */
	pos = data;
	for (k = 0; k < data_len; k++) {
		i = (i + 1) & 0xff;
		j = (j + S[i]) & 0xff;
		S_SWAP(i, j);
		*pos++ ^= S[(S[i] + S[j]) & 0xff];
	}
	atbm_kfree(S);
	return 0;
}
int atbmwifi_aes_unwrap(const atbm_uint8 *kek, int n, const atbm_uint8 *cipher, atbm_uint8 *plain)
{
	atbm_uint8 a[8], *r, b[16];
	int i, j;
	atbm_void *ctx;

	/* 1) Initialize variables. */
	atbm_memcpy(a, cipher, 8);
	r = plain;
	atbm_memcpy(r, cipher + 8, 8 * n);

	ctx = atbmwifi_aes_decrypt_init(kek, 16);
	if (ctx == ATBM_NULL)
		return -1;

	/* 2) Compute intermediate values.
	 * For j = 5 to 0
	 *     For i = n to 1
	 *         B = AES-1(K, (A ^ t) | R[i]) where t = n*j+i
	 *         A = MSB(64, B)
	 *         R[i] = LSB(64, B)
	 */
	for (j = 5; j >= 0; j--) {
		r = plain + (n - 1) * 8;
		for (i = n; i >= 1; i--) {
			atbm_memcpy(b, a, 8);
			b[7] ^= n * j + i;

			atbm_memcpy(b + 8, r, 8);
			atbmwifi_aes_decrypt(ctx, b, b);
			atbm_memcpy(a, b, 8);
			atbm_memcpy(r, b + 8, 8);
			r -= 8;
		}
	}
	atbmwifi_aes_decrypt_deinit(ctx);

	/* 3) Output results.
	 *
	 * These are already in @plain due to the location of temporary
	 * variables. Just verify that the IV matches with the expected value.
	 */
	for (i = 0; i < 8; i++) {
		if (a[i] != 0xa6)
			return -1;
	}

	return 0;
}

/**
 * atbmwifi_aes_wrap - Wrap keys with AES Key Wrap Algorithm (128-bit KEK) (RFC3394)
 * @kek: 16-octet Key encryption key (KEK)
 * @n: Length of the plaintext key in 64-bit units; e.g., 2 = 128-bit = 16
 * bytes
 * @plain: Plaintext key to be wrapped, n * 64 bits
 * @cipher: Wrapped key, (n + 1) * 64 bits
 * Returns: 0 on success, -1 on failure
 */
int atbmwifi_aes_wrap(const atbm_uint8 *kek, int n, const atbm_uint8 *plain, atbm_uint8 *cipher)
{
	atbm_uint8 *a, *r, b[16];
	int i, j;
	atbm_void *ctx;

	a = cipher;
	r = cipher + 8;

	/* 1) Initialize variables. */
	atbm_memset(a, 0xa6, 8);
	atbm_memcpy(r, plain, 8 * n);

	ctx = atbmwifi_aes_encrypt_init(kek, 16);
	if (ctx == ATBM_NULL)
		return -1;

	/* 2) Calculate intermediate values.
	 * For j = 0 to 5
	 *     For i=1 to n
	 *         B = AES(K, A | R[i])
	 *         A = MSB(64, B) ^ t where t = (n*j)+i
	 *         R[i] = LSB(64, B)
	 */
	for (j = 0; j <= 5; j++) {
		r = cipher + 8;
		for (i = 1; i <= n; i++) {
			atbm_memcpy(b, a, 8);
			atbm_memcpy(b + 8, r, 8);
			atbmwifi_aes_encrypt(ctx, b, b);
			atbm_memcpy(a, b, 8);
			a[7] ^= n * j + i;
			atbm_memcpy(r, b + 8, 8);
			r += 8;
		}
	}
	atbmwifi_aes_encrypt_deinit(ctx);

	/* 3) Output the results.
	 *
	 * These are already in @cipher due to the location of temporary
	 * variables.
	 */

	return 0;
}
int eapol_input(struct atbmwifi_vif *priv,struct atbm_buff *skb)
{
	struct atbmwifi_ieee8023_hdr *hdr;

	if(ATBM_NULL== skb)
		goto __error;
	hdr = (struct atbmwifi_ieee8023_hdr *)ATBM_OS_SKB_DATA(skb);

	if(hdr->h_proto != atbm_htons(ATBM_ETH_P_EAPOL))
		goto __error;

	/*Allow EAPOL frames to us, and always disallow all other destination addresses for them.*/
	if(atbm_compare_ether_addr(priv->mac_addr, hdr->h_dest))
		goto drop;

	//wifi_printk(WIFI_CONNECT,"eapol_input len(%d)\n",ATBM_OS_SKB_LEN(skb));
	if(ATBM_OS_SKB_LEN(skb) < 14)
		goto drop;

	atbmwifi_wpa_event_queue((atbm_void*)priv,(atbm_void*)skb,ATBM_NULL,WPA_EVENT__EAP_RX,ATBM_WPA_EVENT_NOACK);
	return 1;
drop:
	atbm_dev_kfree_skb(skb);
	return 1;
	
__error:
	return 0;
}
int wpa_drv_send_eapol(struct atbmwifi_vif *priv,const atbm_uint8 *dest,
				    atbm_uint16 proto, const atbm_uint8 *buf, atbm_size_t len)

{

	struct atbmwifi_ieee8023_hdr *ethhdr;
	struct atbm_buff *skb;
	struct atbmwifi_ieee80211_tx_info *tx_info;

	skb = atbm_dev_alloc_skb(len + sizeof(struct atbmwifi_ieee8023_hdr));	
	if(skb == ATBM_NULL)	{
		return -1;
	}


	buf -= sizeof(struct atbmwifi_ieee8023_hdr);
  	len += sizeof(struct atbmwifi_ieee8023_hdr);
	
	
	atbm_memcpy(atbm_skb_put(skb,len), (char*)buf, len);

	ethhdr = (struct atbmwifi_ieee8023_hdr *)skb->abuf;

	
	atbm_memcpy((atbm_void*)&ethhdr->h_dest,dest,6);
	atbm_memcpy((atbm_void*)&ethhdr->h_source,priv->mac_addr,6);
	
    ethhdr->h_proto = atbm_htons(proto);
	//b_eapol
	tx_info = ATBM_IEEE80211_SKB_TXCB(skb);
	tx_info->b_eapol = 1;
	tx_info->flags = ATBM_IEEE80211_TX_CTL_USE_MINRATE;
//	wifi_printk(WIFI_DBG_MSG,"send_eapol:len(%d)\n",OS_SKB_LEN(skb));
	atbmwifi_tx_start(skb,priv);

	return 0;
	
}

