/**
 ****************************************************************************************
 *
 * @file uwifi_ops_adapter.h
 *
 * @brief uwifi cfg/netdevice related header
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */

#ifndef _UWIFI_OPS_ADAPTER_H_
#define _UWIFI_OPS_ADAPTER_H_

#include <stdint.h>
#include <stdbool.h>
#include "uwifi_types.h"
#include "uwifi_kernel.h"
#include "uwifi_cmds.h"

//#include "uwifi_tx.h"
#include "uwifi_wlan_list.h"
#include "mac.h"
#include "asr_rtos.h"
#include "uwifi_common.h"
#include "asr_wlan_api_aos.h"

#include "uwifi_defs.h"

#define WIFI_SSID_LEN          32

struct asr_csa;
struct asr_vif;
struct vif_params;
enum nl80211_iftype;
struct config_info;
enum lwip_dhcp_event;
#ifdef CONFIG_SME
struct cfg80211_auth_request;
struct cfg80211_assoc_request;
struct cfg80211_deauth_request;
#endif

void asr_csa_finish(struct asr_csa *csa);
void cfg80211_mgmt_tx_comp_status(struct asr_vif *asr_vif, const uint8_t *buf, const uint32_t len, const bool ack);
void cfg80211_rx_mgmt(struct asr_vif *asr_vif, uint8_t *pframe, uint32_t len);
int asr_sniffer_start(struct asr_hw *asr_hw);
int asr_interface_add(struct asr_hw *asr_hw, enum nl80211_iftype type, struct vif_params *params,
                                                                                            struct asr_vif **asr_vif);
int uwifi_xmit_entry(TCPIP_PACKET_INFO_T    *packet, struct ethernetif *ethernetif);
#ifdef CFG_STA_AP_COEX
int uwifi_xmit_buf(void *buf, uint32_t len, void *ethernetif);
int uwifi_xmit_pbuf(struct asr_pbuf *pbuf, void *ethernetif);
#else
int uwifi_xmit_buf(void *buf, uint32_t len);
int uwifi_xmit_pbuf(struct asr_pbuf *pbuf);
#endif

void uwifi_get_mac(uint8_t *buf);
void uwifi_get_device_mac(uint8_t *buf);
void uwifi_get_saved_config(struct config_info *configInfo);
struct asr_hw* uwifi_platform_init(void);
void uwifi_platform_deinit(struct asr_hw *asr_hw, struct asr_vif *asr_vif);
void uwifi_platform_deinit_api(void);
void lwip_comm_dhcp_event_handler(enum lwip_dhcp_event evt);

#ifdef CONFIG_SME
int asr_cfg80211_auth(struct asr_vif *asr_vif,struct cfg80211_auth_request *req);
int asr_cfg80211_assoc(struct asr_vif *asr_vif, struct cfg80211_assoc_request *req);
int asr_cfg80211_deauth(struct asr_vif *asr_vif, struct cfg80211_deauth_request *req);
#endif

void asr_dhcps_start(void);
void asr_dhcps_stop(void);

#endif //_UWIFI_OPS_ADAPTER_H_
