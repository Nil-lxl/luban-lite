/**
 ****************************************************************************************
 *
 * @file uwifi_msg_tx.h
 *
 * @brief TX function declarations
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */

#ifndef _UWIFI_MSG_TX_H_
#define _UWIFI_MSG_TX_H_

#include <stdint.h>
#include <stdbool.h>

enum {
    WLAN_RX_BEACON,         /* receive beacon packet */
    WLAN_RX_PROBE_REQ,      /* receive probe request packet */
    WLAN_RX_PROBE_RES,      /* receive probe response packet */
    WLAN_RX_ACTION,         /* receive action packet */
    WLAN_RX_MANAGEMENT,     /* receive ALL management packet */
    WLAN_RX_DATA,           /* receive ALL data packet */
    WLAN_RX_MCAST_DATA,     /* receive ALL multicast and broadcast packet */
    WLAN_RX_MONITOR_DEFAULT,/* receive probe req/rsp/beacon, my unicast/bcast data packet */
    WLAN_RX_SMART_CONFIG, /* ready for smartconfig packet broadcast*/
    WLAN_RX_SMART_CONFIG_MC, /* ready for smartconfig packet*/
    WLAN_RX_ALL,            /* receive ALL 802.11 packet */
};
#ifdef CONFIG_TWT

typedef struct {
    uint8_t setup_cmd;
    uint8_t flow_type;
    uint8_t wake_interval_exponent;
    uint16_t wake_interval_mantissa;
    uint8_t monimal_min_wake_duration;
    bool twt_upload_wait_trigger;
} wifi_twt_config_t;

/// Structure containing the parameters of the @ref ME_ITWT_CONFIG_REQ message.
struct me_itwt_config_req {
    /// setup_cmd
    uint8_t setup_cmd;
    /// Is trigger-enabled TWT.
    uint8_t trigger;
    /// Is implicit TWT.
    uint8_t implicit;
    /// Is announced TWT.
    uint8_t flow_type;
    /// TWT channel
    uint8_t twt_channel;
    /// TWT protection
    uint8_t twt_prot;
    /// wake interval exponent
    uint8_t wake_interval_exponent;
    /// wake interval mantissa
    uint16_t wake_interval_mantissa;
    /// target wakeup time low(us)
    uint32_t target_wake_time_lo;
    /// minial wake duration after twt sp start time
    uint8_t monimal_min_wake_duration;
    //  indicate upload should enable after rx trigger
    bool twt_upload_wait_trigger;
};

struct me_itwt_del_req {
    uint8_t flow_id;
};
#endif//CONFIG_TWT
int asr_send_reset(struct asr_hw *asr_hw);
int asr_send_start(struct asr_hw *asr_hw);
int asr_send_set_ps_options(uint8_t listen_bc_mc, uint16_t listen_interval);
void uwifi_msg_at2u_set_ps_mode(uint8_t ps_on);
int asr_send_set_ps_mode(uint32_t ps_on);
int asr_send_version_req(struct asr_hw *asr_hw, struct mm_version_cfm *cfm);
int asr_send_mm_get_info(struct asr_hw *asr_hw, struct mm_get_info_cfm *cfm);
int asr_send_add_if(struct asr_hw *asr_hw, const unsigned char *mac,
                     enum nl80211_iftype iftype, bool p2p, struct mm_add_if_cfm *cfm);
int asr_send_remove_if(struct asr_hw *asr_hw, uint8_t vif_index);

int asr_send_key_add(struct asr_hw *asr_hw, uint8_t vif_idx, uint8_t sta_idx, bool pairwise,
                      uint8_t *key, uint8_t key_len, uint8_t key_idx, uint8_t cipher_suite,
                      struct mm_key_add_cfm *cfm);
int asr_send_key_del(struct asr_hw *asr_hw, uint8_t hw_key_idx);
int asr_send_bcn_change(struct asr_hw *asr_hw, uint8_t vif_idx, uint32_t bcn_addr,
                         uint16_t bcn_len, uint16_t tim_oft, uint16_t tim_len, uint16_t *csa_oft);
int asr_send_roc(struct asr_hw *asr_hw, struct asr_vif *vif,
                  struct ieee80211_channel *chan, unsigned int duration);
int asr_send_cancel_roc(struct asr_hw *asr_hw);
int asr_send_set_power(struct asr_hw *asr_hw,  uint8_t vif_idx, int8_t pwr,
                        struct mm_set_power_cfm *cfm);
int asr_send_set_edca(struct asr_hw *asr_hw, uint8_t hw_queue, uint32_t param,
                       bool uapsd, uint8_t inst_nbr);

int asr_send_me_config_req(struct asr_hw *asr_hw);
int asr_send_me_chan_config_req(struct asr_hw *asr_hw);
int asr_send_me_set_control_port_req(struct asr_hw *asr_hw, bool opened,
                                      uint8_t sta_idx);
int asr_send_me_sta_add(struct asr_hw *asr_hw, struct station_parameters *params,
                         const uint8_t *mac, uint8_t inst_nbr, struct me_sta_add_cfm *cfm);
int asr_send_me_sta_del(struct asr_hw *asr_hw, uint8_t sta_idx);
int asr_send_me_traffic_ind(struct asr_hw *asr_hw, uint8_t sta_idx, bool uapsd, uint8_t tx_status);
int asr_send_me_rc_stats(struct asr_hw *asr_hw, uint8_t sta_idx,
                          struct me_rc_stats_cfm *cfm);
int asr_send_me_rc_set_rate(struct asr_hw *asr_hw,
                             uint8_t sta_idx,
                             uint16_t rate_idx);

int asr_send_apm_start_req(struct asr_hw *asr_hw, struct asr_vif *vif,
                            struct cfg80211_ap_settings *settings,
                            struct apm_start_cfm *cfm, struct asr_dma_elem *elem);
int asr_send_apm_stop_req(struct asr_hw *asr_hw, struct asr_vif *vif);
int asr_send_scanu_req(struct asr_vif *asr_vif,
                        struct cfg80211_scan_request *param);
int asr_send_apm_start_cac_req(struct asr_hw *asr_hw, struct asr_vif *vif,
                                struct cfg80211_chan_def *chandef,
                                struct apm_start_cac_cfm *cfm);
int asr_send_apm_stop_cac_req(struct asr_hw *asr_hw, struct asr_vif *vif);

int asr_send_cfg_rssi_req(struct asr_hw *asr_hw, uint8_t vif_index, int rssi_thold, uint32_t rssi_hyst);
int asr_send_cfg_rssi_level_req(struct asr_hw *asr_hw, bool enable);

int asr_send_sm_connect_req(struct asr_vif *asr_vif,
                             struct cfg80211_connect_params *sme,
                             struct sm_connect_cfm *cfm);

int asr_send_sm_disconnect_req(struct asr_hw *asr_hw,
                                struct asr_vif *asr_vif,
                                uint16_t reason);
int asr_msg_push(struct ipc_host_env_tag *env, void *msg_buf, uint16_t len);
#ifdef CFG_SNIFFER_SUPPORT
int asr_send_set_channel(struct asr_hw *asr_hw, struct ieee80211_channel *pst_chan, enum nl80211_channel_type chan_type);
int asr_send_set_idle(struct asr_hw *asr_hw, int idle);
int asr_send_set_filter(struct asr_hw *asr_hw, uint32_t filter);
#endif
#ifdef CFG_DBG
int asr_send_mem_read_req(struct asr_hw *asr_hw, uint32_t mem_addr);
int asr_send_mem_write_req(struct asr_hw *asr_hw, uint32_t mem_addr, uint32_t mem_data);
int asr_send_mod_filter_req(struct asr_hw *asr_hw, uint32_t mod_filter);
int asr_send_sev_filter_req(struct asr_hw *asr_hw, uint32_t env_filter);
int asr_send_get_sys_stat_req(struct asr_hw *asr_hw);
int asr_send_set_host_log_req(struct asr_hw *asr_hw, dbg_host_log_switch_t log_switch);
#endif
int asr_send_fw_softversion_req(struct asr_hw *asr_hw, struct mm_fw_softversion_cfm *cfm);

#ifdef CONFIG_SME
int asr_send_sm_auth_req(struct asr_vif *asr_vif, struct cfg80211_auth_request *auth_req, struct sm_auth_cfm *cfm);
int asr_send_sm_assoc_req(struct asr_vif *asr_vif, struct cfg80211_assoc_request *assoc_req, struct sm_assoc_cfm *cfm);
#endif

int asr_send_set_tx_pwr_rate(struct asr_hw *asr_hw, struct mm_set_tx_pwr_rate_cfm *cfm,
                 struct mm_set_tx_pwr_rate_req *tx_pwr);

int asr_send_get_rssi_req(struct asr_hw *asr_hw, u8 staid, s8 *rssi);
int asr_send_upload_fram_req(struct asr_hw *asr_hw, u8 vif_idx, u16 fram_type, u8 enable);
int asr_send_fram_appie_req(struct asr_hw *asr_hw, u8 vif_idx, u16 fram_type, u8 *ie, u8 ie_len);

int asr_send_efuse_txpwr_req(struct asr_hw *asr_hw, uint8_t * txpwr, uint8_t * txevm, uint8_t *freq_err, bool iswrite, uint8_t *index);

int asr_send_hif_sdio_info_req(struct asr_hw *asr_hw);
int asr_send_me_host_dbg_cmd(struct asr_hw *asr_hw, unsigned int host_dbg_cmd);

#endif /* _UWIFI_MSG_TX_H_ */
