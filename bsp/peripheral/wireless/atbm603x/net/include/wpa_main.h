/**************************************************************************************************************
 * altobeam RTOS WSM host interface (HI) implementation
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#ifndef  _WPA_H
#define  _WPA_H

#define EAPOL_VERSION 1
enum { 
	ATBM_IEEE802_1X_TYPE_EAP_PACKET = 0,
	ATBM_IEEE802_1X_TYPE_EAPOL_START = 1,
	ATBM_IEEE802_1X_TYPE_EAPOL_LOGOFF = 2,
	ATBM_IEEE802_1X_TYPE_EAPOL_KEY = 3,
	ATBM_IEEE802_1X_TYPE_EAPOL_ENCAPSULATED_ASF_ALERT = 4
};
enum { 
	ATBM_EAPOL_KEY_TYPE_RC4 = 1, 
	ATBM_EAPOL_KEY_TYPE_RSN = 2,
	ATBM_EAPOL_KEY_TYPE_WPA = 254 
};
struct atbmwifi_wpa_ie_data {
	int proto;
	int pairwise_cipher;
	int group_cipher;
	int key_mgmt;
	int capabilities;
	atbm_size_t num_pmkid;
	const atbm_uint8 *pmkid;
	int mgmt_group_cipher;
};
enum atbmwifi_wpa_sm_conf_params {
	ATBM_RSNA_PMK_LIFETIME /* dot11RSNAConfigPMKLifetime */,
	ATBM_RSNA_PMK_REAUTH_THRESHOLD /* dot11RSNAConfigPMKReauthThreshold */,
	ATBM_RSNA_SA_TIMEOUT /* dot11RSNAConfigSATimeout */,
	ATBM_WPA_PARAM_PROTO,
	ATBM_WPA_PARAM_PAIRWISE,
	ATBM_WPA_PARAM_GROUP,
	ATBM_WPA_PARAM_KEY_MGMT,
	ATBM_WPA_PARAM_MGMT_GROUP,
	ATBM_WPA_PARAM_RSN_ENABLED,
	ATBM_WPA_PARAM_MFP
};
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

#ifndef bswap_16
#define bswap_16(a) ((((atbm_uint16) (a) << 8) & 0xff00) | (((atbm_uint16) (a) >> 8) & 0xff))
#endif

#ifndef bswap_32
#define bswap_32(a) ((((atbm_uint32) (a) << 24) & 0xff000000) | \
		     (((atbm_uint32) (a) << 8) & 0xff0000) | \
     		     (((atbm_uint32) (a) >> 8) & 0xff00) | \
     		     (((atbm_uint32) (a) >> 24) & 0xff))
#endif

#define ATBM_WPA_PUT_LE8(a, val)	(a)[0] =  (val)

#define ATBM_WPA_GET_BE16(a) ((atbm_uint16) (((a)[0] << 8) | (a)[1]))
#define ATBM_WPA_PUT_BE16(a, val)			\
	do {					\
		(a)[0] = ((atbm_uint16) (val)) >> 8;	\
		(a)[1] = ((atbm_uint16) (val)) & 0xff;	\
	} while (0)

#define ATBM_WPA_GET_LE16(a) ((atbm_uint16) (((a)[1] << 8) | (a)[0]))
#define ATBM_WPA_PUT_LE16(a, val)			\
	do {					\
		(a)[1] = ((atbm_uint16) (val)) >> 8;	\
		(a)[0] = ((atbm_uint16) (val)) & 0xff;	\
	} while (0)

#define ATBM_WPA_GET_BE24(a) ((((atbm_uint32) (a)[0]) << 16) | (((atbm_uint32) (a)[1]) << 8) | \
			 ((atbm_uint32) (a)[2]))
#define ATBM_WPA_PUT_BE24(a, val)					\
	do {							\
		(a)[0] = (atbm_uint8) ((((atbm_uint32) (val)) >> 16) & 0xff);	\
		(a)[1] = (atbm_uint8) ((((atbm_uint32) (val)) >> 8) & 0xff);	\
		(a)[2] = (atbm_uint8) (((atbm_uint32) (val)) & 0xff);		\
	} while (0)

#define ATBM_WPA_GET_BE32(a) ((((atbm_uint32) (a)[0]) << 24) | (((atbm_uint32) (a)[1]) << 16) | \
			 (((atbm_uint32) (a)[2]) << 8) | ((atbm_uint32) (a)[3]))
#define ATBM_WPA_PUT_BE32(a, val)					\
	do {							\
		(a)[0] = (atbm_uint8) ((((atbm_uint32) (val)) >> 24) & 0xff);	\
		(a)[1] = (atbm_uint8) ((((atbm_uint32) (val)) >> 16) & 0xff);	\
		(a)[2] = (atbm_uint8) ((((atbm_uint32) (val)) >> 8) & 0xff);	\
		(a)[3] = (atbm_uint8) (((atbm_uint32) (val)) & 0xff);		\
	} while (0)

#define ATBM_WPA_GET_LE32(a) ((((atbm_uint32) (a)[3]) << 24) | (((atbm_uint32) (a)[2]) << 16) | \
			 (((atbm_uint32) (a)[1]) << 8) | ((atbm_uint32) (a)[0]))
#define ATBM_WPA_PUT_LE32(a, val)					\
	do {							\
		(a)[3] = (atbm_uint8) ((((atbm_uint32) (val)) >> 24) & 0xff);	\
		(a)[2] = (atbm_uint8) ((((atbm_uint32) (val)) >> 16) & 0xff);	\
		(a)[1] = (atbm_uint8) ((((atbm_uint32) (val)) >> 8) & 0xff);	\
		(a)[0] = (atbm_uint8) (((atbm_uint32) (val)) & 0xff);		\
	} while (0)

#define ATBM_WPA_GET_BE64(a) ((((atbm_uint64) (a)[0]) << 56) | (((atbm_uint64) (a)[1]) << 48) | \
			 (((atbm_uint64) (a)[2]) << 40) | (((atbm_uint64) (a)[3]) << 32) | \
			 (((atbm_uint64) (a)[4]) << 24) | (((atbm_uint64) (a)[5]) << 16) | \
			 (((atbm_uint64) (a)[6]) << 8) | ((atbm_uint64) (a)[7]))
#define ATBM_WPA_PUT_BE64(a, val)				\
	do {						\
		(a)[0] = (atbm_uint8) (((atbm_uint64) (val)) >> 56);	\
		(a)[1] = (atbm_uint8) (((atbm_uint64) (val)) >> 48);	\
		(a)[2] = (atbm_uint8) (((atbm_uint64) (val)) >> 40);	\
		(a)[3] = (atbm_uint8) (((atbm_uint64) (val)) >> 32);	\
		(a)[4] = (atbm_uint8) (((atbm_uint64) (val)) >> 24);	\
		(a)[5] = (atbm_uint8) (((atbm_uint64) (val)) >> 16);	\
		(a)[6] = (atbm_uint8) (((atbm_uint64) (val)) >> 8);	\
		(a)[7] = (atbm_uint8) (((atbm_uint64) (val)) & 0xff);	\
	} while (0)

#define ATBM_WPA_GET_LE64(a) ((((atbm_uint64) (a)[7]) << 56) | (((atbm_uint64) (a)[6]) << 48) | \
			 (((atbm_uint64) (a)[5]) << 40) | (((atbm_uint64) (a)[4]) << 32) | \
			 (((atbm_uint64) (a)[3]) << 24) | (((atbm_uint64) (a)[2]) << 16) | \
			 (((atbm_uint64) (a)[1]) << 8) | ((atbm_uint64) (a)[0]))
/*
typedef atbm_uint16 __bitwise __be16;
typedef atbm_uint16 __bitwise atbm_uint16;
typedef atbm_uint32 __bitwise be32;
typedef atbm_uint32 __bitwise atbm_uint32;
typedef u64 __bitwise be64;
typedef u64 __bitwise le64;
*/
#define __force 
#define atbm_le_to_host16(n) ((__force atbm_uint16) (atbm_uint16) (n))
#define atbm_host_to_le16(n) ((__force atbm_uint16) (atbm_uint16) (n))
#define atbm_be_to_host16(n) bswap_16((__force atbm_uint16) (atbm_uint16) (n))
#define atbm_host_to_be16(n) ((__force atbm_uint16) bswap_16((n)))
#define atbm_le_to_host32(n) ((__force atbm_uint32) (atbm_uint32) (n))
#define atbm_host_to_le32(n) ((__force atbm_uint32) (atbm_uint32) (n))
#define atbm_be_to_host32(n) bswap_32((__force atbm_uint32) (be32) (n))
#define atbm_host_to_be32(n) ((__force atbm_uint32) bswap_32((n)))
#define atbm_le_to_host64(n) ((__force atbm_uint64) (atbm_uint64) (n))
#define atbm_host_to_le64(n) ((__force atbm_uint64) (atbm_uint64) (n))
#define atbm_be_to_host64(n) bswap_64((__force atbm_uint64) (atbm_uint64) (n))
#define atbm_host_to_be64(n) ((__force atbm_uint64) bswap_64((n)))
#define ATBM_MAX_SSID_LEN 32

#define ATBM_ETH_P_EAPOL ATBM_ETH_P_PAE
/**
 * struct wpa_driver_capa - Driver capability information
 */
#define ATBM_WPA_DRIVER_CAPA_KEY_MGMT_WPA		0x00000001
#define ATBM_WPA_DRIVER_CAPA_KEY_MGMT_WPA2		0x00000002
#define ATBM_WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK	0x00000004
#define ATBM_WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK	0x00000008
#define ATBM_WPA_DRIVER_CAPA_KEY_MGMT_WPA_NONE	0x00000010
#define ATBM_WPA_DRIVER_CAPA_KEY_MGMT_FT		0x00000020
#define ATBM_WPA_DRIVER_CAPA_KEY_MGMT_FT_PSK		0x00000040
			
#define ATBM_WPA_DRIVER_CAPA_ENC_WEP40	0x00000001
#define ATBM_WPA_DRIVER_CAPA_ENC_WEP104	0x00000002
#define ATBM_WPA_DRIVER_CAPA_ENC_TKIP	0x00000004
#define ATBM_WPA_DRIVER_CAPA_ENC_CCMP	0x00000008
			
#define ATBM_WPA_DRIVER_AUTH_OPEN		0x00000001
#define ATBM_WPA_DRIVER_AUTH_SHARED		0x00000002
#define ATBM_WPA_DRIVER_AUTH_LEAP		0x00000004
	
	/* Driver generated WPA/RSN IE */
#define ATBM_WPA_DRIVER_FLAGS_DRIVER_IE	0x00000001
	/* Driver needs static WEP key setup after association command */
#define ATBM_WPA_DRIVER_FLAGS_SET_KEYS_AFTER_ASSOC 0x00000002
#define ATBM_WPA_DRIVER_FLAGS_USER_SPACE_MLME 0x00000004
	/* Driver takes care of RSN 4-way handshake internally; PMK is configured with
	 * struct wpa_driver_ops::set_key using alg = WPA_ALG_PMK */
#define ATBM_WPA_DRIVER_FLAGS_4WAY_HANDSHAKE 0x00000008
#define ATBM_WPA_DRIVER_FLAGS_WIRED		0x00000010
	/* Driver provides separate commands for authentication and association (SME in
	 * wpa_supplicant). */
#define ATBM_WPA_DRIVER_FLAGS_SME		0x00000020
	/* Driver supports AP mode */
#define ATBM_WPA_DRIVER_FLAGS_AP		0x00000040
	/* Driver needs static WEP key setup after association has been completed */
#define ATBM_WPA_DRIVER_FLAGS_SET_KEYS_AFTER_ASSOC_DONE	0x00000080
	
	
#define ATBM_IEEE80211_MODE_INFRA	0
#define ATBM_IEEE80211_MODE_IBSS	1
#define ATBM_IEEE80211_MODE_AP	2
			
#define ATBM_IEEE80211_CAP_ESS	0x0001
#define ATBM_IEEE80211_CAP_IBSS	0x0002
#define ATBM_IEEE80211_CAP_PRIVACY	0x0010
		 
#define ATBM_WPA_SCAN_QUAL_INVALID		BIT(0)
#define ATBM_WPA_SCAN_NOISE_INVALID		BIT(1)
#define ATBM_WPA_SCAN_LEVEL_INVALID		BIT(2)
#define ATBM_WPA_SCAN_LEVEL_DBM		BIT(3)
#define ATBM_WPA_SCAN_AUTHENTICATED		BIT(4)
#define ATBM_WPA_SCAN_ASSOCIATED		BIT(5)

#define ATBM_DEFAULT_EAP_WORKAROUND ((unsigned int) -1)
#define ATBM_DEFAULT_EAPOL_FLAGS (ATBM_EAPOL_FLAG_REQUIRE_KEY_UNICAST | \
			     ATBM_EAPOL_FLAG_REQUIRE_KEY_BROADCAST)
#define ATBM_DEFAULT_PROTO (ATBM_WPA_PROTO_WPA | ATBM_WPA_PROTO_RSN)
#define ATBM_DEFAULT_KEY_MGMT (ATBM_WPA_KEY_MGMT_PSK | ATBM_WPA_KEY_MGMT_IEEE8021X)
#define ATBM_DEFAULT_PAIRWISE (ATBM_WPA_CIPHER_CCMP | ATBM_WPA_CIPHER_TKIP)
#define ATBM_DEFAULT_GROUP (ATBM_WPA_CIPHER_CCMP | ATBM_WPA_CIPHER_TKIP | \
		       ATBM_WPA_CIPHER_WEP104 | ATBM_WPA_CIPHER_WEP40)
#define ATBM_DEFAULT_FRAGMENT_SIZE 1398


#define ATBM_WPA_MAX_SSID_LEN 32

/* IEEE 802.11i */
#define ATBM_PMKID_LEN 16
#define ATBM_PMK_LEN 32
#define ATBM_PMK_LEN_MAX 64
#define ATBM_WPA_REPLAY_COUNTER_LEN 8
#define ATBM_WPA_NONCE_LEN 32
#define ATBM_WPA_KEY_RSC_LEN 8
#define ATBM_WPA_GMK_LEN 32
#define ATBM_WPA_GTK_MAX_LEN 32  
#define ATBM_WPA_SELECTOR_LEN 4
#define ATBM_WPA_VERSION 1
#define ATBM_RSN_SELECTOR_LEN 4
#define ATBM_RSN_VERSION 1
#define ATBM_RSN_SELECTOR(a, b, c, d) \
	((((atbm_uint32) (a)) << 24) | (((atbm_uint32) (b)) << 16) | (((atbm_uint32) (c)) << 8) | \
	 (atbm_uint32) (d))

#define ATBM_WPA_AUTH_KEY_MGMT_NONE ATBM_RSN_SELECTOR(0x00, 0x50, 0xf2, 0)
#define ATBM_WPA_AUTH_KEY_MGMT_UNSPEC_802_1X ATBM_RSN_SELECTOR(0x00, 0x50, 0xf2, 1)
#define ATBM_WPA_AUTH_KEY_MGMT_PSK_OVER_802_1X ATBM_RSN_SELECTOR(0x00, 0x50, 0xf2, 2)
#define ATBM_WPA_CIPHER_SUITE_NONE ATBM_RSN_SELECTOR(0x00, 0x50, 0xf2, 0)
#define ATBM_WPA_CIPHER_SUITE_WEP40 ATBM_RSN_SELECTOR(0x00, 0x50, 0xf2, 1)
#define ATBM_WPA_CIPHER_SUITE_TKIP ATBM_RSN_SELECTOR(0x00, 0x50, 0xf2, 2)
#define ATBM_WPA_CIPHER_SUITE_CCMP ATBM_RSN_SELECTOR(0x00, 0x50, 0xf2, 4)
#define ATBM_WPA_CIPHER_SUITE_WEP104 ATBM_RSN_SELECTOR(0x00, 0x50, 0xf2, 5)


#define ATBM_RSN_AUTH_KEY_MGMT_UNSPEC_802_1X ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 1)
#define ATBM_RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 2)
#define ATBM_RSN_AUTH_KEY_MGMT_FT_802_1X ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 3)
#define ATBM_RSN_AUTH_KEY_MGMT_FT_PSK ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 4)
#define ATBM_RSN_AUTH_KEY_MGMT_802_1X_SHA256 ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 5)
#define ATBM_RSN_AUTH_KEY_MGMT_PSK_SHA256 ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 6)
#define ATBM_RSN_AUTH_KEY_MGMT_TPK_HANDSHAKE ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 7)
#define ATBM_RSN_AUTH_KEY_MGMT_SAE ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 8)
#define ATBM_RSN_AUTH_KEY_MGMT_FT_SAE ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 9)

#define ATBM_RSN_CIPHER_SUITE_NONE ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 0)
#define ATBM_RSN_CIPHER_SUITE_WEP40 ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 1)
#define ATBM_RSN_CIPHER_SUITE_TKIP ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 2)
#define ATBM_RSN_CIPHER_SUITE_CCMP ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 4)
#define ATBM_RSN_CIPHER_SUITE_WEP104 ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 5)
#if CONFIG_IEEE80211W
#define ATBM_RSN_CIPHER_SUITE_AES_128_CMAC ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 6)
#endif /* CONFIG_IEEE80211W */

/* EAPOL-Key Key Data Encapsulation
 * GroupKey and PeerKey require encryption, otherwise, encryption is optional.
 */
#define ATBM_RSN_KEY_DATA_GROUPKEY ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 1)
#define ATBM_RSN_KEY_DATA_MAC_ADDR ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 3)
#define ATBM_RSN_KEY_DATA_PMKID ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 4)
#if CONFIG_PEERKEY
#define ATBM_RSN_KEY_DATA_SMK ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 5)
#define ATBM_RSN_KEY_DATA_NONCE ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 6)
#define ATBM_RSN_KEY_DATA_LIFETIME ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 7)
#define ATBM_RSN_KEY_DATA_ERROR ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 8)
#endif /* CONFIG_PEERKEY */
#if CONFIG_IEEE80211W
#define ATBM_RSN_KEY_DATA_IGTK ATBM_RSN_SELECTOR(0x00, 0x0f, 0xac, 9)
#endif /* CONFIG_IEEE80211W */

#define ATBM_WPA_OUI_TYPE ATBM_RSN_SELECTOR(0x00, 0x50, 0xf2, 1)

#define ATBM_RSN_SELECTOR_PUT(a, val) ATBM_WPA_PUT_BE32((atbm_uint8 *) (a), (val))
#define ATBM_RSN_SELECTOR_GET(a) ATBM_WPA_GET_BE32((const atbm_uint8 *) (a))

#define ATBM_RSN_NUM_REPLAY_COUNTERS_1 0
#define ATBM_RSN_NUM_REPLAY_COUNTERS_2 1
#define ATBM_RSN_NUM_REPLAY_COUNTERS_4 2
#define ATBM_RSN_NUM_REPLAY_COUNTERS_16 3


#ifdef _MSC_VER
#pragma pack(push, 1)
#endif /* _MSC_VER */

#if CONFIG_IEEE80211W
#define ATBM_WPA_IGTK_LEN 16
#endif /* CONFIG_IEEE80211W */


/* IEEE 802.11, 7.3.2.25.3 RSN Capabilities */
#define ATBM_WPA_CAPABILITY_PREAUTH BIT(0)
#define ATBM_WPA_CAPABILITY_NO_PAIRWISE BIT(1)
/* B2-B3: PTKSA Replay Counter */
/* B4-B5: GTKSA Replay Counter */
#define ATBM_WPA_CAPABILITY_MFPR BIT(6)
#define ATBM_WPA_CAPABILITY_MFPC BIT(7)
#define ATBM_WPA_CAPABILITY_PEERKEY_ENABLED BIT(9)


/* IEEE 802.11r */
#define ATBM_MOBILITY_DOMAIN_ID_LEN 2
#define ATBM_FT_R0KH_ID_MAX_LEN 48
#define ATBM_FT_R1KH_ID_LEN 6
#define ATBM_WPA_PMK_NAME_LEN 16


/* IEEE 802.11, 8.5.2 EAPOL-Key frames */
#define ATBM_WPA_KEY_INFO_TYPE_MASK ((atbm_uint16) (BIT(0) | BIT(1) | BIT(2)))
#define ATBM_WPA_KEY_INFO_TYPE_AKM_DEFINED 0
#define ATBM_WPA_KEY_INFO_TYPE_HMAC_MD5_RC4 BIT(0)
#define ATBM_WPA_KEY_INFO_TYPE_HMAC_SHA1_AES BIT(1)
#define ATBM_WPA_KEY_INFO_TYPE_AES_128_CMAC 3
#define ATBM_WPA_KEY_INFO_KEY_TYPE BIT(3) /* 1 = Pairwise, 0 = Group key */
/* bit4..5 is used in WPA, but is reserved in IEEE 802.11i/RSN */
#define ATBM_WPA_KEY_INFO_KEY_INDEX_MASK (BIT(4) | BIT(5))
#define ATBM_WPA_KEY_INFO_KEY_INDEX_SHIFT 4
#define ATBM_WPA_KEY_INFO_INSTALL BIT(6) /* pairwise */
#define ATBM_WPA_KEY_INFO_TXRX BIT(6) /* group */
#define ATBM_WPA_KEY_INFO_ACK BIT(7)
#define ATBM_WPA_KEY_INFO_MIC BIT(8)
#define ATBM_WPA_KEY_INFO_SECURE BIT(9)
#define ATBM_WPA_KEY_INFO_ERROR BIT(10)
#define ATBM_WPA_KEY_INFO_REQUEST BIT(11)
#define ATBM_WPA_KEY_INFO_ENCR_KEY_DATA BIT(12) /* IEEE 802.11i/RSN only */
#define ATBM_WPA_KEY_INFO_SMK_MESSAGE BIT(13)
      
#define ATBM_AES_ENCRYPT	1
#define ATBM_AES_DECRYPT	0
        
#define ATBM_AES_MAXNR 14
#define ATBM_AES_BLOCK_SIZE 16

struct aes_key_st {
#ifdef AES_LONG
    unsigned long rd_key[4 *(ATBM_AES_MAXNR + 1)];
#else
    unsigned int rd_key[4 *(ATBM_AES_MAXNR + 1)];
#endif
    int rounds;
};
typedef struct aes_key_st AES_KEY;

# define GETU32(pt) (((atbm_uint32)(pt)[0] << 24) ^ ((atbm_uint32)(pt)[1] << 16) ^ ((atbm_uint32)(pt)[2] <<  8) ^ ((atbm_uint32)(pt)[3]))
# define PUTU32(ct, st) { (ct)[0] = (atbm_uint8)((st) >> 24); (ct)[1] = (atbm_uint8)((st) >> 16); (ct)[2] = (atbm_uint8)((st) >>  8); (ct)[3] = (atbm_uint8)(st); }


struct atbmwifi_wpa_gtk_data {
	enum atbm_wpa_alg alg;
	int tx, key_rsc_len, keyidx;
	atbm_uint8 gtk[32];
	int gtk_len;
};

struct atbmwifi_ieee802_1x_hdr {
	atbm_uint8 version;
	atbm_uint8 type;
	atbm_uint16 length;
	/* followed by length octets of data */
};


struct atbm_wpa_ssid {
	//struct wpa_ssid *next;
	//;struct wpa_ssid *pnext;
	//int id;
	//int priority;
	atbm_uint8 *ssid;
	atbm_size_t ssid_len;
	atbm_uint8 bssid[ATBM_ETH_ALEN];
	atbm_uint8 psk[ATBM_PMK_LEN];
	char *passphrase;
	int pairwise_cipher;
	int group_cipher;
	int key_mgmt;
	int proto;
	int auth_alg;
	//int scan_ssid;

	/**
	* peerkey -  Whether PeerKey handshake for direct links is allowed
	*
	* This is only used when both RSN/WPA2 and IEEE 802.11e (QoS) are
	* enabled.
	*
	* 0 = disabled (default)
	* 1 = enabled
	*/
	atbm_uint16 bssid_set:1,

		psk_set:1,
		temporary:1,
		disabled:1,
		peerkey:1,
		export_keys:1;

#define ATBM_EAPOL_FLAG_REQUIRE_KEY_UNICAST BIT(0)
#define ATBM_EAPOL_FLAG_REQUIRE_KEY_BROADCAST BIT(1)

	atbm_uint16 eapol_flags;

	/**
	 * eap - EAP peer configuration for this network
	 */
//	struct eap_peer_config eap;

//	struct eap_peer_config eap;
#define ATBM_NUM_WEP_KEYS 4
#define ATBM_MAX_WEP_KEY_LEN 16
	/**
	 * wep_key - WEP keys
	 */
	atbm_uint8 wep_key[ATBM_NUM_WEP_KEYS][ATBM_MAX_WEP_KEY_LEN];

	/**
	 * wep_key_len - WEP key lengths
	 */
	atbm_size_t wep_key_len[ATBM_NUM_WEP_KEYS];

	/**
	 * wep_tx_keyidx - Default key index for TX frames using WEP
	 */
	int wep_tx_keyidx;
	//int proactive_key_caching;
	//int mixed_cell;
	int leap;
	int non_leap;
	//unsigned int eap_workaround;	
	/**
	 * mode - IEEE 802.11 operation mode (Infrastucture/IBSS)
	 *
	 * 0 = infrastructure (Managed) mode, i.e., associate with an AP.
	 *
	 * 1 = IBSS (ad-hoc, peer-to-peer)
	 *
	 * 2 = AP (access point)
	 *
	 * 3 = P2P Group Owner (can be set in the configuration file)
	 *
	 * 4 = P2P Group Formation (used internally; not in configuration
	 * files)
	 *
	 * Note: IBSS can only be used with key_mgmt NONE (plaintext and
	 * static WEP) and key_mgmt=WPA-NONE (fixed group key TKIP/CCMP). In
	 * addition, ap_scan has to be set to 2 for IBSS. WPA-None requires
	 * following network block options: proto=WPA, key_mgmt=WPA-NONE,
	 * pairwise=NONE, group=TKIP (or CCMP, but not both), and psk must also
	 * be set (either directly or using ASCII passphrase).
	 */
	enum atbm_wpas_mode {
		ATBM_WPAS_MODE_INFRA = 0,
		ATBM_WPAS_MODE_IBSS = 1,
		ATBM_WPAS_MODE_AP = 2,
		ATBM_WPAS_MODE_P2P_GO = 3,
		ATBM_WPAS_MODE_P2P_GROUP_FORMATION = 4,
	} mode;
//	int disabled;	
     /**
	 * peerkey -  Whether PeerKey handshake for direct links is allowed
	 *
	 * This is only used when both RSN/WPA2 and IEEE 802.11e (QoS) are
	 * enabled.
	 *
	 * 0 = disabled (default)
	 * 1 = enabled
	 */
//	int peerkey;
	char *id_str;
	int frequency;
	//int wpa_ptk_rekey;
	//int *scan_freq;
	//char *bgscan;
	int *freq_list;
//	struct eap_peer_config  conf;
#if CONFIG_IEEE80211W
	/**
	 * ieee80211w - Whether management frame protection is enabled
	 *
	 * This value is used to configure policy for management frame
	 * protection (IEEE 802.11w). 0 = disabled, 1 = optional, 2 = required.
	 */
	//enum mfp_options ieee80211w;
#endif /* CONFIG_IEEE80211W */
#if CONFIG_P2P	
#ifndef P2P_MAX_STORED_CLIENTS
#define P2P_MAX_STORED_CLIENTS 3
#endif /* P2P_MAX_STORED_CLIENTS */
	
	/**
	 * p2p_group - Network generated as a P2P group (used internally)
	 */
	/**
	 * p2p_persistent_group - Whether this is a persistent group
	 */
	int p2p_group:1,
		p2p_persistent_group:1;

	/**
	 * p2p_mode - p2p actual mode
	 * P2P_UNKNOWN,
	 * P2P_CLIENT,
	 * P2P_GO
	 */
	//atbm_uint8 p2p_mode;

#endif /* CONFIG_P2P */
	unsigned int bg_scan_period;
	unsigned int eap_workaround;

};

struct atbmwifi_wpa_eapol_key {
	atbm_uint8 type;
	/* Note: key_info, key_length, and key_data_length are unaligned */
	atbm_uint8 key_info[2]; /* big endian */
	atbm_uint8 key_length[2]; /* big endian */
	atbm_uint8 replay_counter[ATBM_WPA_REPLAY_COUNTER_LEN];
	atbm_uint8 key_nonce[ATBM_WPA_NONCE_LEN];
	atbm_uint8 key_iv[16];
	atbm_uint8 key_rsc[ATBM_WPA_KEY_RSC_LEN];
	atbm_uint8 key_id[8]; /* Reserved in IEEE 802.11i/RSN */
	atbm_uint8 key_mic[16];
	atbm_uint8 key_data_length[2]; /* big endian */
	/* followed by key_data_length bytes of key_data */
};

struct atbmwifi_wpa_ptk {
	atbm_uint8 kck[16]; /* EAPOL-Key Key Confirmation Key (KCK) */
	atbm_uint8 kek[16]; /* EAPOL-Key Key Encryption Key (KEK) */
	atbm_uint8 tk1[16]; /* Temporal Key 1 (TK1) */
	union {
		atbm_uint8 tk2[16]; /* Temporal Key 2 (TK2) */
		struct {
			atbm_uint8 tx_mic_key[8];
			atbm_uint8 rx_mic_key[8];
		}auth;
	}u;
};
struct atbmwifi_wpa_ie_hdr {
	atbm_uint8 elem_id;
	atbm_uint8 len;
	atbm_uint8 oui[4]; /* 24-bit OUI followed by 8-bit OUI type */
	atbm_uint8 version[2]; /* little endian */
};

struct atbmwifi_rsn_ie_hdr {
	atbm_uint8 elem_id; /* ATBM_WLAN_EID_RSN */
	atbm_uint8 len;
	atbm_uint8 version[2]; /* little endian */
};
struct atbmwifi_rsn_error_kde {
	atbm_uint16 mui;
	atbm_uint16 error_type;
};
#define ATBM_PEERKEY_MAX_IE_LEN 80
struct atbm_wpa_peerkey {
//	struct wpa_peerkey *next;
	int initiator; /* whether this end was initator for SMK handshake */
	atbm_uint8 addr[ATBM_ETH_ALEN]; /* other end MAC address */
	atbm_uint8 inonce[ATBM_WPA_NONCE_LEN]; /* Initiator Nonce */
	atbm_uint8 pnonce[ATBM_WPA_NONCE_LEN]; /* Peer Nonce */
	atbm_uint8 rsnie_i[ATBM_PEERKEY_MAX_IE_LEN]; /* Initiator RSN IE */
	atbm_size_t rsnie_i_len;
	atbm_uint8 rsnie_p[ATBM_PEERKEY_MAX_IE_LEN]; /* Peer RSN IE */
	atbm_size_t rsnie_p_len;
	atbm_uint8 smk[ATBM_PMK_LEN];
	int smk_complete;
	atbm_uint8 smkid[ATBM_PMKID_LEN];
	atbm_uint32 lifetime;
//	os_time_t expiration;
	int cipher; /* Selected cipher (WPA_CIPHER_*) */
	atbm_uint8 replay_counter[ATBM_WPA_REPLAY_COUNTER_LEN];
	int replay_counter_set;
	int use_sha256; /* whether AKMP indicate SHA256-based derivations */

	struct atbmwifi_wpa_ptk stk, tstk;
	int stk_set, tstk_set;
};


#define ATBM_RSN_FT_CAPAB_FT_OVER_DS BIT(0)
#define ATBM_RSN_FT_CAPAB_FT_RESOURCE_REQ_SUPP BIT(1)

struct rsn_ftie {
	atbm_uint8 mic_control[2];
	atbm_uint8 mic[16];
	atbm_uint8 anonce[ATBM_WPA_NONCE_LEN];
	atbm_uint8 snonce[ATBM_WPA_NONCE_LEN];
	/* followed by optional parameters */
};

#define ATBM_FTIE_SUBELEM_R1KH_ID 1
#define ATBM_FTIE_SUBELEM_GTK 2
#define ATBM_FTIE_SUBELEM_R0KH_ID 3
#define ATBM_FTIE_SUBELEM_IGTK 4

struct atbm_rsn_rdie {
	atbm_uint8 id;
	atbm_uint8 descr_count;
	atbm_uint16 status_code;
};
struct atbm_wpa_bss {
	unsigned int id;
	unsigned int scan_miss_count;
	//unsigned int last_update_idx;
	unsigned int flags;
	atbm_uint8 bssid[ATBM_ETH_ALEN];
	atbm_uint8 ssid[32];
	atbm_size_t ssid_len;
	int freq;
	atbm_uint16 beacon_int;
	atbm_uint16 caps;
	int qual;
	int noise;
	int level;
	atbm_uint64 tsf;
	atbm_size_t ie_len;
};

struct atbmwifi_rsn_pmksa_cache_supplicant {
	atbm_uint8 pmkid[ATBM_PMKID_LEN];
	atbm_uint8 pmk[ATBM_PMK_LEN];
	atbm_size_t pmk_len;
	atbm_uint8 aa[ATBM_ETH_ALEN];
};
#if 0

struct wpa_sm_ctx {
	atbm_void *ctx; /* pointer to arbitrary upper level context */
	atbm_void *msg_ctx; /* upper level context for wpa_msg() calls */

	atbm_void (*set_state)(atbm_void *ctx, enum wpa_states state);
	enum wpa_states (*get_state)(atbm_void *ctx);
	//void (*deauthenticate)(void * ctx, int reason_code); 
	//void (*disassociate)(void *ctx, int reason_code);
	//int (*set_key)(void *ctx, enum wpa_alg alg,
	//	       const atbm_uint8 *addr, int key_idx, int set_tx,
	//	       const atbm_uint8 *seq, atbm_size_t seq_len,
	//	       const atbm_uint8 *key, atbm_size_t key_len);
	//void * (*get_network_ctx)(void *ctx);
	//int (*get_bssid)(void *ctx, atbm_uint8 *bssid);
	int (*ether_send)(atbm_void *ctx, const atbm_uint8 *dest, atbm_uint16 proto, const atbm_uint8 *buf,
			  atbm_size_t len);
	int (*get_beacon_ie)(atbm_void *ctx);
	atbm_void (*cancel_auth_timeout)(atbm_void *ctx);
	atbm_uint8 * (*alloc_eapol)(atbm_void *ctx, atbm_uint8 type, const atbm_void *data, atbm_uint16 data_len,
			    atbm_size_t *msg_len, atbm_void **data_pos);
	int (*add_pmkid)(atbm_void *ctx, const atbm_uint8 *bssid, const atbm_uint8 *pmkid);
	int (*remove_pmkid)(atbm_void *ctx, const atbm_uint8 *bssid, const atbm_uint8 *pmkid);
	atbm_void (*set_config_blob)(atbm_void *ctx, struct wpa_config_blob *blob);
	const struct wpa_config_blob * (*get_config_blob)(atbm_void *ctx,
							  const char *name);
	int (*mlme_setprotection)(atbm_void *ctx, const atbm_uint8 *addr,
				  int protection_type, int key_type);
	int (*update_ft_ies)(atbm_void *ctx, const atbm_uint8 *md, const atbm_uint8 *ies,
			     atbm_size_t ies_len);
	int (*send_ft_action)(atbm_void *ctx, atbm_uint8 action, const atbm_uint8 *target_ap,
			      const atbm_uint8 *ies, atbm_size_t ies_len);
	int (*mark_authenticated)(atbm_void *ctx, const atbm_uint8 *target_ap);
#ifdef CONFIG_TDLS
	int (*tdls_get_capa)(atbm_void *ctx, int *tdls_supported,
			     int *tdls_ext_setup);
	int (*send_tdls_mgmt)(atbm_void *ctx, const atbm_uint8 *dst,
			      atbm_uint8 action_code, atbm_uint8 dialog_token,
			      atbm_uint16 status_code, const atbm_uint8 *buf, atbm_size_t len);
	int (*tdls_oper)(atbm_void *ctx, int oper, const atbm_uint8 *peer);
	int (*tdls_peer_addset)(atbm_void *ctx, const atbm_uint8 *addr, int add,
				atbm_uint16 capability, const atbm_uint8 *supp_rates,
				atbm_size_t supp_rates_len);
#endif /* CONFIG_TDLS */
	atbm_void (*set_rekey_offload)(atbm_void *ctx, const atbm_uint8 *kek, const atbm_uint8 *kck,
				  const atbm_uint8 *replay_ctr);
};


#endif

#define ATBM_PMK_CACHE_NUM 2


struct atbmwifi_wpa_sm {
	struct wpa_supplicant *wpa_s;

	atbm_uint8 pmk[ATBM_PMK_LEN];
	atbm_size_t pmk_len;
	struct atbmwifi_wpa_ptk ptk, tptk;
	int ptk_set, tptk_set;
	atbm_uint8 snonce[ATBM_WPA_NONCE_LEN];
	atbm_uint8 anonce[ATBM_WPA_NONCE_LEN]; /* ANonce from the last 1/4 msg */
	int renew_snonce;
	atbm_uint8 rx_replay_counter[ATBM_WPA_REPLAY_COUNTER_LEN];
	int rx_replay_counter_set;
//	struct eapol_sm *eapol; /* EAPOL state machine from upper level code */
#if CONFIG_SAE
	struct rsn_pmksa_cache *pmksa; /* PMKSA cache */
	struct rsn_pmksa_cache_entry *cur_pmksa; /* current PMKSA entry */
#endif

	atbm_uint8 own_addr[ATBM_ETH_ALEN];
	atbm_uint8 bssid[ATBM_ETH_ALEN];

	unsigned int dot11RSNAConfigPMKLifetime;
	unsigned int dot11RSNAConfigPMKReauthThreshold;
	unsigned int dot11RSNAConfigSATimeout;

	/* Selected configuration (based on Beacon/ProbeResp WPA IE) */
	unsigned int proto;
	unsigned int pairwise_cipher;
	unsigned int group_cipher;
	unsigned int key_mgmt;
	unsigned int mgmt_group_cipher;

	int rsn_enabled; /* Whether RSN is enabled in configuration */
	int mfp; /* 0 = disabled, 1 = optional, 2 = mandatory */
	atbm_uint16 linkid;
	atbm_uint8 *assoc_wpa_ie; /* Own WPA/RSN IE from (Re)AssocReq */
	atbm_size_t assoc_wpa_ie_len;
	atbm_uint8 *ap_wpa_ie, *ap_rsn_ie;
	atbm_size_t ap_wpa_ie_len, ap_rsn_ie_len;

#if CONFIG_PEERKEY
	struct wpa_peerkey *peerkey;
#endif /* CONFIG_PEERKEY */

#if CONFIG_IEEE80211R
	atbm_uint8 xxkey[ATBM_PMK_LEN]; /* PSK or the second 256 bits of MSK */
	atbm_size_t xxkey_len;
	atbm_uint8 pmk_r0[ATBM_PMK_LEN];
	atbm_uint8 pmk_r0_name[ATBM_WPA_PMK_NAME_LEN];
	atbm_uint8 pmk_r1[ATBM_PMK_LEN];
	atbm_uint8 pmk_r1_name[ATBM_WPA_PMK_NAME_LEN];
	atbm_uint8 mobility_domain[ATBM_MOBILITY_DOMAIN_ID_LEN];
	atbm_uint8 r0kh_id[ATBM_FT_R0KH_ID_MAX_LEN];
	atbm_size_t r0kh_id_len;
	atbm_uint8 r1kh_id[ATBM_FT_R1KH_ID_LEN];
	int ft_completed;
	int over_the_ds_in_progress;
	atbm_uint8 target_ap[ATBM_ETH_ALEN]; /* over-the-DS target AP */
	int set_ptk_after_assoc;
	atbm_uint8 mdie_ft_capab; /* FT Capability and Policy from target AP MDIE */
	atbm_uint8 assoc_resp_ies[64]; /* MDIE and FTIE from (Re)Association Response */
	atbm_size_t assoc_resp_ies_len;
#endif /* CONFIG_IEEE80211R */
};

typedef enum {
	EAP_CODE_REQUEST = 1, 
	EAP_CODE_RESPONSE = 2, 
	EAP_CODE_SUCCESS = 3,
	EAP_CODE_FAILURE = 4, 
	EAP_CODE_INITIATE = 5,
	EAP_CODE_FINISH = 6
}EapCodeType;

/* EAP Request and Response data begins with one octet Type. Success and
 * Failure do not have additional data. */

/*
 * EAP Method Types as allocated by IANA:
 * http://www.iana.org/assignments/eap-numbers
 */
typedef enum {
	ATBM_EAP_TYPE_NONE = 0,
	ATBM_EAP_TYPE_IDENTITY = 1 /* RFC 3748 */,
	ATBM_EAP_TYPE_NOTIFICATION = 2 /* RFC 3748 */,
	ATBM_EAP_TYPE_NAK = 3 /* Response only, RFC 3748 */,
	ATBM_EAP_TYPE_MD5 = 4, /* RFC 3748 */
	ATBM_EAP_TYPE_OTP = 5 /* RFC 3748 */,
	ATBM_EAP_TYPE_GTC = 6, /* RFC 3748 */
	ATBM_EAP_TYPE_TLS = 13 /* RFC 2716 */,
	ATBM_EAP_TYPE_LEAP = 17 /* Cisco proprietary */,
	ATBM_EAP_TYPE_SIM = 18 /* RFC 4186 */,
	ATBM_EAP_TYPE_TTLS = 21 /* RFC 5281 */,
	ATBM_EAP_TYPE_AKA = 23 /* RFC 4187 */,
	ATBM_EAP_TYPE_PEAP = 25 /* draft-josefsson-pppext-eap-tls-eap-06.txt */,
	ATBM_EAP_TYPE_MSCHAPV2 = 26 /* draft-kamath-pppext-eap-mschapv2-00.txt */,
	ATBM_EAP_TYPE_TLV = 33 /* draft-josefsson-pppext-eap-tls-eap-07.txt */,
	ATBM_EAP_TYPE_TNC = 38 /* TNC IF-T v1.0-r3; note: tentative assignment;
			   * type 38 has previously been allocated for
			   * EAP-HTTP Digest, (funk.com) */,
	ATBM_EAP_TYPE_FAST = 43 /* RFC 4851 */,
	ATBM_EAP_TYPE_PAX = 46 /* RFC 4746 */,
	ATBM_EAP_TYPE_PSK = 47 /* RFC 4764 */,
	ATBM_EAP_TYPE_SAKE = 48 /* RFC 4763 */,
	ATBM_EAP_TYPE_IKEV2 = 49 /* RFC 5106 */,
	ATBM_EAP_TYPE_AKA_PRIME = 50 /* draft-arkko-eap-aka-kdf-10.txt */,
	ATBM_EAP_TYPE_GPSK = 51 /* RFC 5433 */,
	ATBM_EAP_TYPE_EXPANDED = 254 /* RFC 3748 */

} EapType;


/* SMI Network Management Private Enterprise Code for vendor specific types */
typedef enum {
	ATBM_EAP_VENDOR_IETF = 0,
	ATBM_EAP_VENDOR_MICROSOFT = 0x000137 /* Microsoft */,
	ATBM_EAP_VENDOR_WFA = 0x00372A /* Wi-Fi Alliance */
}EapVenType;

#define ATBM_EAP_MSK_LEN 64
#define ATBM_EAP_EMSK_LEN 64

typedef enum {
	DECISION_FAIL, DECISION_COND_SUCC, DECISION_UNCOND_SUCC
} EapDecision;

typedef enum {
	METHOD_NONE, METHOD_INIT, METHOD_CONT, METHOD_MAY_CONT, METHOD_DONE
} EapMethodState;


#define ATBM_IEEE8021X_REPLAY_COUNTER_LEN 8
#define ATBM_IEEE8021X_KEY_SIGN_LEN 16
#define ATBM_IEEE8021X_KEY_IV_LEN 16

#define ATBM_IEEE8021X_KEY_INDEX_FLAG 0x80
#define ATBM_IEEE8021X_KEY_INDEX_MASK 0x03

struct atbmwifi_ieee802_1x_eapol_key {
	atbm_uint8 type;
	/* Note: key_length is unaligned */
	atbm_uint8 key_length[2];
	/* does not repeat within the life of the keying material used to
	 * encrypt the Key field; 64-bit NTP timestamp MAY be used here */
	atbm_uint8 replay_counter[ATBM_IEEE8021X_REPLAY_COUNTER_LEN];
	atbm_uint8 key_iv[ATBM_IEEE8021X_KEY_IV_LEN]; /* cryptographically atbm_os_random number */
	atbm_uint8 key_index; /* key flag in the most significant bit:
		       * 0 = broadcast (default key),
		       * 1 = unicast (key mapping key); key index is in the
		       * 7 least significant bits */
	/* HMAC-MD5 message integrity check computed with MS-MPPE-Send-Key as
	 * the key */
	atbm_uint8 key_signature[ATBM_IEEE8021X_KEY_SIGN_LEN];

	/* followed by key: if packet body length = 44 + key length, then the
	 * key field (of key_length bytes) contains the key in encrypted form;
	 * if packet body length = 44, key field is absent and key_length
	 * represents the number of least significant octets from
	 * MS-MPPE-Send-Key attribute to be used as the keying material;
	 * RC4 key used in encryption = Key-IV + MS-MPPE-Recv-Key */
};

struct atbm_eap_hdr {
	atbm_uint8 code;
	atbm_uint8 identifier;
	atbm_uint16 length; /* including code and identifier; network byte order */
	/* followed by length-4 octets of data */
};


int atbmwifi_wpa_parse_wpa_ie(const atbm_uint8 *wpa_ie, atbm_size_t wpa_ie_len,
		     struct atbmwifi_wpa_ie_data *data);
atbm_void wpa_sm_set_pmk_from_pmksa(struct atbmwifi_wpa_sm *sm);
atbm_void wpa_sm_set_pmk(struct atbmwifi_wpa_sm *sm, const atbm_uint8 *pmk, atbm_size_t pmk_len,
			const atbm_uint8 *pmkid, const atbm_uint8 *bssid);
atbm_void wpa_deauthen(struct atbmwifi_vif *priv);
int sme_set_sae_group(struct wpa_supplicant *wpa_s);

struct eapol_sm ;
#endif
