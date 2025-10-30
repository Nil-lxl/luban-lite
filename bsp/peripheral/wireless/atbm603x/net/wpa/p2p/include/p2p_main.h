
#ifndef __P2P_MAIN_H__
#define __P2P_MAIN_H__


enum p2p_event {
	JOIN_SCAN_END,
	GO_NEG_PEER_WAIT,
	GO_NEG_COMPLETED,
	GO_DISC_RESP,
	P2P_RX_ACTION,
	P2P_ACTION_ACK,
	NOTHING
};

struct p2p_action{
	atbm_uint8 sa[6];
	atbm_uint8 da[6];
	atbm_uint8 bssid[6];
	atbm_uint8 category;
	atbm_uint16 freq;
	atbm_uint16 len;
	atbm_uint8 *data;
};

struct p2p_action_ack{
	atbm_uint8 sa[6];
	atbm_uint8 da[6];
	atbm_uint8 bssid[6];
	int res;
};

struct p2p_task_msg{
	enum p2p_event event;
	void *param;
};


#define P2P_QUEUE_NUM	16

#define P2P_LISTEN_REG_CLASS			81
#define P2P_LISTEN_CHANNEL				11
#define P2P_DEFULT_GO_INTENT			10  //0~15
#define P2P_MAX_PEERS					10


#define P2P_TASK_STK_SIZE 1024*8
//#define P2P_TASK_PRIORITY CONFIG_TCPAPP_THREAD_PRIORITY
#define P2P_TASK_PRIORITY 4


atbm_uint8 *atbm_p2p_add_scan_ie(struct atbmwifi_vif *priv, atbm_uint8 * pos);
atbm_uint8 *atbm_p2p_add_ap_beacon_ie(struct atbmwifi_vif *priv, atbm_uint8 * pos);
atbm_uint8 *atbm_p2p_add_ap_pbresp_ie(struct atbmwifi_vif *priv, atbm_uint8 * pos);
atbm_uint8 *atbm_p2p_add_ap_assoc_resp_ie(struct atbmwifi_vif *priv, atbm_uint8 * pos);
int atbm_p2p_prash_ie(struct atbmwifi_vif *priv, atbm_uint8 *bssid, 
				atbm_uint16 freq, atbm_int8 rssi, atbm_uint8 *data, atbm_uint16 len);
int atbm_p2p_probe_req_rx(struct atbmwifi_vif *priv, const atbm_uint8 *addr,
			  const atbm_uint8 *dst, const atbm_uint8 *bssid,
			  const atbm_uint8 *ie, atbm_size_t ie_len,
			  unsigned int rx_freq, int ssi_signal);
void atbm_p2p_rx_action(struct atbmwifi_vif *priv, const atbm_uint8 *da,
			const atbm_uint8 *sa, const atbm_uint8 *bssid,
			atbm_uint8 category, const atbm_uint8 *data, atbm_size_t len, atbm_uint16 freq);
void atbm_p2p_tx_action_ack(struct atbmwifi_vif *priv, struct atbmwifi_ieee80211_mgmt *mgmt, int status);
int atbm_p2p_init(struct atbmwifi_vif *priv);
void atbm_p2p_stop_find(struct atbmwifi_vif *priv);
int atbm_p2p_auto_connect(struct atbmwifi_vif *priv, atbm_uint8 *mac, int go_intent);
int atbm_p2p_find_wait_connect(struct atbmwifi_vif *priv, int go_intent);
int atbm_p2p_find_only(struct atbmwifi_vif *priv, int timeout);
int atbm_p2p_start(struct atbmwifi_vif *priv);
int atbm_p2p_get_peers(struct atbmwifi_vif *priv);
int atbm_p2p_deinit(struct atbmwifi_vif *priv);
void atbm_p2p_go_stop(struct atbmwifi_vif *priv);
int atbm_p2p_go_start(struct atbmwifi_vif *priv);
int atbm_p2p_restart(struct atbmwifi_vif *priv);

#endif

