#ifndef _ASR_WIFI_API_AOS_H_
#define _ASR_WIFI_API_AOS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "asr_wlan_api.h"

typedef enum
{
    WLAN_EVENT_SCAN_COMPLETED,
    WLAN_EVENT_ASSOCIATED,
    WLAN_EVENT_CONNECTED,
    WLAN_EVENT_IP_GOT,
    WLAN_EVENT_DISCONNECTED,
    WLAN_EVENT_AP_UP,
    WLAN_EVENT_AP_DOWN,
    WLAN_EVENT_RSSI,
    WLAN_EVENT_RSSI_LEVEL,
    WLAN_EVENT_STA_CLOSE,
    WLAN_EVENT_AUTH,
    WLAN_EVENT_4WAY_HANDSHAKE,
    WLAN_EVENT_4WAY_HANDSHAKE_DONE,
    WLAN_EVENT_AP_PEER_UP,
    WLAN_EVENT_AP_PEER_DOWN,
    WLAN_EVENT_MAX,
}asr_wlan_event_e;

/**
 *  @brief  Input network precise paras in asr_wlan_start_adv function.
 */
typedef struct
{
  asr_wlan_ap_info_adv_t ap_info;         /**< @ref apinfo_adv_t. */
  char  key[64];                /**< Security key or PMK of the wlan. */
  int   key_len;                /**< The length of the key. */
  char  local_ip_addr[16];      /**< Static IP configuration, Local IP address. */
  char  net_mask[16];           /**< Static IP configuration, Netmask. */
  char  gateway_ip_addr[16];    /**< Static IP configuration, Router IP address. */
  char  dns_server_ip_addr[16]; /**< Static IP configuration, DNS server IP address. */
  char  dhcp_mode;              /**< DHCP mode, @ref DHCP_Disable, @ref DHCP_Client and @ref DHCP_Server. */
  char  reserved[32];
  int   wifi_retry_interval;    /**< Retry interval if an error is occured when connecting an access point,
                                  time unit is millisecond. */
} asr_wlan_init_info_adv_st;

/** @brief  used in station and softap mode, get mac address(in char mode) of WIFI device
 *
 * @param mac_addr    : pointer to get the mac address
 *
 *  @return    0       : on success.
 *  @return    other   : error occurred
 */
int asr_wlan_get_mac_address_inchar(char *puc_mac);

int asr_wlan_suspend_sta(void);
int asr_wlan_suspend_ap(void);
int asr_wlan_suspend(void);
void asr_wlan_register_mgmt_monitor_cb(monitor_cb_t fn);

/*Wifi event callback interface
*
* @return    void
*/
extern void wifi_event_cb(asr_wlan_event_e evt, void* info);
#ifdef __cplusplus
    }
#endif

#endif //_ASR_WIFI_API_AOS_H_

