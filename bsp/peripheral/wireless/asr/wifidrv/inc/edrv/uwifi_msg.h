/**
 ****************************************************************************************
 *
 * @file uwifi_msg.h
 *
 * @brief uwifi msg function definition
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */

#ifndef _UWIFI_MSG_H_
#define _UWIFI_MSG_H_
#include <stdint.h>
#include "asr_rtos.h"
#include "uwifi_common.h"
#include "uwifi_kernel.h"
#include "uwifi_ops_adapter.h"
#include "uwifi_interface.h"

typedef struct
{
    uint8_t open_result;
    uint8_t open_monitor_result;
    uint8_t connect_result;
    uint8_t connect_adv_result;
    uint8_t disconnect_result;
    asr_semaphore_t open_semaphore;
    asr_semaphore_t open_monitor_semaphore;
    asr_semaphore_t connect_semaphore;
    asr_semaphore_t connect_adv_semaphore;
    asr_semaphore_t disconnect_semaphore;
} asr_api_ctrl_param_t;
extern asr_api_ctrl_param_t asr_api_ctrl_param;
extern struct asr_hw *sp_asr_hw;


extern bool txlogen;
extern bool rxlogen;
/******************************************************
 *           Constants msg_id definition
 ******************************************************/
enum driver_cmd
{
    F2D_MSG_RXDESC=0x01,
    F2D_MSG_ACK,
#ifdef APM_AGING_SUPPORT
    F2D_MSG_DEAUTH,
#endif
    F2D_MSG_TBTT_PRIM,
    I2D_DMA_TX_CFM=0x11,
    I2D_DMA_MSG,
#ifdef CFG_MIMO_UF
    I2D_RX_UF,
#endif
    LWIP2D_DATA_TX_SKB=0x21,
    LWIP2U_IP_GOT,
    LWIP2U_STA_IP_UPDATE,
    TCPIP2U_DATA,
    AT2D_OPEN_MODE=0x31,
    AT2D_OPEN_MONITOR_MODE,
    AT2D_SET_PS_MODE,
    AT2D_CLOSE,
    AT2D_SCAN_REQ,
    AT2D_CONNECT_REQ,
    AT2D_CONNECT_ADV_REQ,
    AT2D_DISCONNECT,
    AT2D_TX_POWER,
#ifdef CFG_SNIFFER_SUPPORT
    AT2D_SET_FILTER_TYPE,
    AT2D_SNIFFER_REGISTER_CB,
#endif
#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
    AT2D_CUS_FRAME,
#endif
    AOS2U_STA_START,
    RXU2U_ASSOCRSP_TX_CFM,
    RXU2U_DEAUTH_TX_CFM,
    RXU2U_RX_DEAUTH,
    RXU2U_DISCONNECT_STA,
    RXU2U_EAPOL_PACKET_PROCESS,
    RXU2U_WPA_INFO_CLEAR,
    RXU2U_HANDSHAKE_START,
    TIMER2U_EAPOL_RESEND,
    TIMER2U_WPA_COMPLETE_TO,
    TIMER2U_AUTOCONNECT_STA,
    SDIO_RX_REQ,
    SDIO_TX_REQ,
    SDIO_MSG_REQ,
    RXU2U_STA_HANDSHAKE_DONE,
    TIMER2U_SOFTAP_AUTH_TO,
    TIMER2U_SOFTAP_ASSOC_TO,
    TIMER2U_STA_SA_QUERY_TO,
    RXU2U_SME_PACKET_PROCESS,
};

/******************************************************
 *           Constants reason codes definition
 ******************************************************/
enum reasoncode
{
    DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT,
    DISCONNECT_REASON_DHCP_TIMEOUT,
    DISCONNECT_REASON_USER_GENERATE,
    DISCONNECT_REASON_INTERFACE_INVALID,
    DISCONNECT_REASON_STA_KRACK,
    DISCONNECT_REASON_STA_4TH_EAPOL_TX_FAIL,
    DISCONNECT_REASON_STA_SA_QUERY_FAIL,
};

/**************************************************
uwifi msg queue top defination
**************************************************/
void uwifi_msg_handle(void);
OSStatus uwifi_msg_queue_init(void);
OSStatus uwifi_msg_queue_deinit(void);

/**************************************************
uwifi rx msg queue top defination
**************************************************/
#ifdef APM_AGING_SUPPORT
void uwifi_msg_l2u_msg_deauth(uint8_t *buf);
#endif

/********************************************
uwifi send to uwifi
*********************************************/
void uwifi_msg_rxu2u_assocrsp_tx_cfm(uint32_t is_success, uint32_t param);
void uwifi_msg_rxu2u_deauth_tx_cfm(uint32_t is_success, uint32_t param);
void uwifi_msg_rxu2u_rx_deauth(uint32_t param);
void uwifi_msg_timer2u_autoconnect_sta(uint32_t param);
void uwifi_msg_rxu2u_eapol_packet_process(uint8_t *data, uint32_t data_len, uint32_t mode);
void uwifi_msg_rxu2u_handshake_start(uint32_t mode);

void uwifi_main_process_msg(struct uwifi_task_msg_t *msg);
void uwifi_rx_task_process(struct uwifi_task_msg_t *msg,asr_thread_arg_t arg);
int uwifi_dev_transmit_skb(struct sk_buff* skb, struct asr_vif *asr_vif);

void uwifi_msg_timer2u_eapol_resend(uint32_t arg);
void uwifi_msg_at2u_disconnect(enum reasoncode reason);
void uwifi_msg_rxu2u_wpa_info_clear(void* asr_vif, uint32_t security);

extern void uwifi_msg_at2u_open_mode(enum open_mode openmode,struct softap_info_t* softap_info);
extern void uwifi_msg_aos2u_sta_start(void* p_config);
extern void uwifi_msg_at2u_connect_adv_req(struct wifi_conn_t *wifi_con);
extern void uwifi_sniffer_handle_mgmt_cb_register(uint32_t fn);
extern int uwifi_sniffer_handle_cb_register(uint32_t fn);
extern void uwifi_msg_at2u_open_monitor_mode(void);
extern void uwifi_sniffer_handle_channel_change(uint8_t chan_num);
extern void uwifi_msg_at2u_close(uint32_t iftype);
extern void uwifi_msg_at2u_scan_req(struct wifi_scan_t* wifi_scan);
int asr_tx_task(struct asr_hw *asr_hw);
void uwifi_sdio_main(asr_thread_arg_t arg);
void uwifi_rx_to_os_main(asr_thread_arg_t arg);
uint8_t get_sdio_reg(uint8_t idx);
void uwifi_msg_rx_to_os_queue(void);


OSStatus uwifi_rx_to_os_msg_queue_init(void);
OSStatus uwifi_rx_to_os_msg_queue_deinit(void);

OSStatus uwifi_sdio_event_init(void);
OSStatus uwifi_sdio_event_deinit(void);
OSStatus uwifi_sdio_event_set(uint32_t mask);
OSStatus uwifi_sdio_event_clear(uint32_t mask);
OSStatus uwifi_sdio_event_wait(uint32_t mask, uint32_t *rcv_flags, uint32_t timeout);


void uwifi_msg_lwip2u_ip_got(uint32_t ipstat);
void uwifi_msg_lwip2u_tx_skb_data(struct sk_buff *skb);


int uwifi_check_legal_channel(int channel);

void dev_restart_work_func(struct asr_vif *asr_vif);

struct scan_result* asr_wlan_get_scan_ap(char* ssid,char ssid_len, char ap_security);
struct scan_result* asr_wlan_get_scan_ap_by_bssid(char* bssid, char ap_security);

void dump_sdio_info(struct asr_hw *asr_hw, const char *func, u32 line);
void uwifi_msg_at2u_softap_auth_to(uint32_t arg);
void uwifi_msg_at2u_softap_assoc_to(uint32_t arg);
void uwifi_msg_at2u_sta_handshake_done(void);
void uwifi_msg_timer2u_wpa_complete_to(uint32_t arg);
void uwifi_msg_at2u_sta_sa_query_to(uint32_t arg);

#endif // _UWIFI_MSG_H_
