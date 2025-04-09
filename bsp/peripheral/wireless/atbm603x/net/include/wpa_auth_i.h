/**************************************************************************************************************
 * altobeam RTOS WSM host interface (HI) implementation
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#ifndef WPA_AUTH_I_H
#define WPA_AUTH_I_H
#include "wpa_main.h"
/* max(dot11RSNAConfigGroupUpdateCount,dot11RSNAConfigPairwiseUpdateCount) */
#define RSNA_MAX_EAPOL_RETRIES 4

//struct atbmwifi_wpa_group;


struct atbmwifi_wpa_group {
	struct atbmwifi_wpa_group *next;

	Boolean GInit;
	int GKeyDoneStations;
	Boolean GTKReKey;
	int GTK_len;
	int GN, GM;
	Boolean GTKAuthenticator;
	atbm_uint8 Counter[ATBM_WPA_NONCE_LEN];

	enum {
		ATBM_WPA_GROUP_GTK_INIT = 0,
		ATBM_WPA_GROUP_SETKEYS, 
		ATBM_WPA_GROUP_SETKEYSDONE
	} wpa_group_state;

	atbm_uint8 GMK[ATBM_WPA_GMK_LEN];
	atbm_uint8 GTK[2][ATBM_WPA_GTK_MAX_LEN];
	atbm_uint8 GNonce[ATBM_WPA_NONCE_LEN];
	Boolean changed;
	Boolean first_sta_seen;
	Boolean reject_4way_hs_for_entropy;
#if CONFIG_IEEE80211W
	atbm_uint8 IGTK[2][ATBM_WPA_IGTK_LEN];
	int GN_igtk, GM_igtk;
#endif /* CONFIG_IEEE80211W */
};

typedef enum {
		ATBM_WPA_PTK_INITIALIZE, 
		ATBM_WPA_PTK_DISCONNECT, 
		ATBM_WPA_PTK_DISCONNECTED,
		ATBM_WPA_PTK_PTKSTART,
		ATBM_WPA_PTK_PTKCALCNEGOTIATING, 
		ATBM_WPA_PTK_PTKINITNEGOTIATING, 
		ATBM_WPA_PTK_PTKINITDONE,
		ATBM_WPA_PTK_INSTALL
	} wpa_ptk_state_e;

typedef	enum {
		ATBM_WPA_PTK_GROUP_IDLE = 0,
		ATBM_WPA_PTK_GROUP_REKEYNEGOTIATING,
		ATBM_WPA_PTK_GROUP_REKEYESTABLISHED,
		ATBM_WPA_PTK_GROUP_KEYINSTALLED,
		ATBM_WPA_PTK_GROUP_KEYERROR
	} wpa_ptk_group_state_e;

struct atbmwifi_wpa_state_machine {
//	struct wpa_authenticator *wpa_auth;
	struct atbmwifi_wpa_group *group;

	atbm_uint8 addr[ATBM_ETH_ALEN];
	wpa_ptk_state_e wpa_ptk_state;
	wpa_ptk_group_state_e wpa_ptk_group_state;

	unsigned int 	Init:1,
					DeauthenticationRequest:1,
					AuthenticationRequest:1,
					ReAuthenticationRequest:1,
					Disconnect:1,
				    TimeoutEvt:1,
				    EAPOLKeyReceived:1,
				    EAPOLKeyPairwise:1,
				    EAPOLKeyRequest:1,
				    MICVerified:1,
					GUpdateStationKeys:1,
					PTK_valid:1,
					pairwise_set:1,
					Pair:1,
					PInitAKeys:1, /* WPA only, not in IEEE 802.11i */
					PTKRequest:1, /* not in IEEE 802.11i state machine */
				 	has_GTK:1,
				    PtkGroupInit:1; /* init request for PTK Group state machine */

	int TimeoutCtr;
	int GTimeoutCtr;
	atbm_uint8 ANonce[ATBM_WPA_NONCE_LEN];
	atbm_uint8 SNonce[ATBM_WPA_NONCE_LEN];
	atbm_uint8 PMK[ATBM_PMK_LEN];
	struct atbmwifi_wpa_ptk PTK;
	int keycount;
	atbm_uint8 *last_rx_eapol_key; /* starting from IEEE 802.1X header */
	atbm_size_t last_rx_eapol_key_len;

	unsigned short 	changed:1,
					in_step_loop:1,
					pending_deinit:1,
					started:1,
					mgmt_frame_prot:1,
					rx_eapol_key_secure:1,
					update_snonce:1;
	atbm_uint8			  linkid;
	atbm_uint8			  reserve;//
	atbm_uint8 req_replay_counter[ATBM_WPA_REPLAY_COUNTER_LEN];
	int req_replay_counter_used;
	struct atbmwifi_wpa_key_replay_counter {
		atbm_uint8 counter[ATBM_WPA_REPLAY_COUNTER_LEN];
		Boolean valid;
	} key_replay[RSNA_MAX_EAPOL_RETRIES],
		prev_key_replay[RSNA_MAX_EAPOL_RETRIES];
	atbm_uint8 *wpa_ie;
	atbm_size_t wpa_ie_len;

	enum {
		ATBM_WPA_VERSION_NO_WPA = 0 /* WPA not used */,
		ATBM_WPA_VERSION_WPA = 1 /* WPA / IEEE 802.11i/D3.0 */,
		ATBM_WPA_VERSION_WPA2 = 2 /* WPA2 / IEEE 802.11i */
	} wpa;
	int pairwise; /* Pairwise cipher suite, WPA_CIPHER_* */
	int wpa_key_mgmt; /* the selected WPA_KEY_MGMT_* */
//	struct rsn_pmksa_cache_entry *pmksa; /* cache pmk, use for radius/wps , WPA/WPA2 not use this,WPA PSK is save in ssid->psk */

	atbm_uint32 dot11RSNAStatsTKIPLocalMICFailures;
	atbm_uint32 dot11RSNAStatsTKIPRemoteMICFailures;


	int pending_1_of_4_timeout;

	atbm_uint8 pmkid[ATBM_PMKID_LEN]; /* valid if pmkid_set == 1 */
	unsigned int pmkid_set;
	struct rsn_pmksa_cache_entry *pmksa; /* current PMKSA entry */
};

/* per group key state machine data */

#if 0

/* per authenticator data */
struct wpa_authenticator {
	struct atbmwifi_wpa_group *group;

	unsigned int dot11RSNAStatsTKIPRemoteMICFailures;
	atbm_uint32 dot11RSNAAuthenticationSuiteSelected;
	atbm_uint32 dot11RSNAPairwiseCipherSelected;
	atbm_uint32 dot11RSNAGroupCipherSelected;
	atbm_uint8 dot11RSNAPMKIDUsed[ATBM_PMKID_LEN];
	atbm_uint32 dot11RSNAAuthenticationSuiteRequested; /* FIX: update */
	atbm_uint32 dot11RSNAPairwiseCipherRequested; /* FIX: update */
	atbm_uint32 dot11RSNAGroupCipherRequested; /* FIX: update */
	unsigned int dot11RSNATKIPCounterMeasuresInvoked;
	unsigned int dot11RSNA4WayHandshakeFailures;

	//struct wpa_stsl_negotiation *stsl_negotiations;

	struct wpa_auth_config conf;
	struct wpa_auth_callbacks cb;

	atbm_uint8 *wpa_ie;
	atbm_size_t wpa_ie_len;

	atbm_uint8 addr[ATBM_ETH_ALEN];

	struct rsn_pmksa_cache *pmksa;
	//struct wpa_ft_pmk_cache *ft_pmk_cache;
};


int wpa_write_rsn_ie(struct wpa_auth_config *conf, atbm_uint8 *buf, atbm_size_t len,
		     const atbm_uint8 *pmkid);
atbm_void wpa_auth_logger(struct wpa_authenticator *wpa_auth, const atbm_uint8 *addr,
		     logger_level level, const char *txt);
atbm_void wpa_auth_vlogger(struct wpa_authenticator *wpa_auth, const atbm_uint8 *addr,
		      logger_level level, const char *fmt, ...);
atbm_void __wpa_send_eapol(struct wpa_authenticator *wpa_auth,
		      struct atbmwifi_wpa_state_machine *sm, int key_info,
		      const atbm_uint8 *key_rsc, const atbm_uint8 *nonce,
		      const atbm_uint8 *kde, atbm_size_t kde_len,
		      int keyidx, int encr, int force_version);
int wpa_auth_for_each_sta(struct wpa_authenticator *wpa_auth,
			  int (*cb)(struct atbmwifi_wpa_state_machine *sm, atbm_void *ctx),
			  atbm_void *cb_ctx);
int wpa_auth_for_each_auth(struct wpa_authenticator *wpa_auth,
			   int (*cb)(struct wpa_authenticator *a, atbm_void *ctx),
			   atbm_void *cb_ctx);

#if CONFIG_PEERKEY
int wpa_stsl_remove(struct wpa_authenticator *wpa_auth,
		    struct wpa_stsl_negotiation *neg);
atbm_void wpa_smk_error(struct wpa_authenticator *wpa_auth,
		   struct atbmwifi_wpa_state_machine *sm, struct atbmwifi_wpa_eapol_key *key);
atbm_void wpa_smk_m1(struct wpa_authenticator *wpa_auth,
		struct atbmwifi_wpa_state_machine *sm, struct atbmwifi_wpa_eapol_key *key);
atbm_void wpa_smk_m3(struct wpa_authenticator *wpa_auth,
		struct atbmwifi_wpa_state_machine *sm, struct atbmwifi_wpa_eapol_key *key);
#endif /* CONFIG_PEERKEY */

#if CONFIG_IEEE80211R
int wpa_write_mdie(struct wpa_auth_config *conf, atbm_uint8 *buf, atbm_size_t len);
int wpa_write_ftie(struct wpa_auth_config *conf, const atbm_uint8 *r0kh_id,
		   atbm_size_t r0kh_id_len,
		   const atbm_uint8 *anonce, const atbm_uint8 *snonce,
		   atbm_uint8 *buf, atbm_size_t len, const atbm_uint8 *subelem,
		   atbm_size_t subelem_len);
int wpa_auth_derive_ptk_ft(struct atbmwifi_wpa_state_machine *sm, const atbm_uint8 *pmk,
			   struct atbmwifi_wpa_ptk *ptk, atbm_size_t ptk_len);
struct wpa_ft_pmk_cache * wpa_ft_pmk_cache_init(atbm_void);
atbm_void wpa_ft_pmk_cache_deinit(struct wpa_ft_pmk_cache *cache);
atbm_void wpa_ft_install_ptk(struct atbmwifi_wpa_state_machine *sm);
#endif /* CONFIG_IEEE80211R */
#endif
#endif /* WPA_AUTH_I_H */
