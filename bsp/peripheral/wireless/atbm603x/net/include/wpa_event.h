/**************************************************************************************************************
 * altobeam RTOS WSM host interface (HI) implementation
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#ifndef _WPA_TIMER_H
#define _WPA_TIMER_H
#include "atbm_type.h"
#define ATBM_WPA_EVENT_ACK		(1)
#define ATBM_WPA_EVENT_NOACK	(0)
enum atbm_wpa_event_id {
	WPA_EVENT__INIT,
	WPA_EVENT__DEINIT,
	WPA_EVENT__SUPPLICANT_SCAN,
	WPA_EVENT__SUPPLICANT_SCAN_END,
	WPA_EVENT__SUPPLICANT_SCAN_EXPIRE,
	WPA_EVENT__SUPPLICANT_START_CONNECT,
	WPA_EVENT__SUPPLICANT_AUTHEN,
	WPA_EVENT__SUPPLICANT_ASSOCIAT,
	WPA_EVENT__SUPPLICANT_ASSOCIATED,
	WPA_EVENT__SUPPLICANT_CONNECTED,
	WPA_EVENT__SUPPLICANT_DEAUTHEN,
	WPA_EVENT__SUPPLICANT_CONNECT_FAIL,
	WPA_EVENT__SUPPLICANT_DISCONN,
	WPA_EVENT__CONNECT_TIMEOUT,
	WPA_EVENT__HOSTAPD_START,
	WPA_EVENT__HOSTAPD_STA_ASSOCIATED,
	WPA_EVENT__HOSTAPD_STA_HANDSHAKE_START,
	WPA_EVENT__HOSTAPD_STA_HANDSHAKE,
	WPA_EVENT__HOSTAPD_STA_DEAUTHENED,
	WPA_EVENT__RX_PKG,
	WPA_EVENT__EAP_TX_RESP,
	WPA_EVENT__EAP_RX,
	WPA_EVENT__EAP_TX_STATUS,
	WPA_EVENT__SMARTCONFIG_SUCCESS,
	WPA_EVENT_TX_AGGR_QUEUE_PACKET,
	WPA_EVENT__MAX,
};
int wpa_event_init(void);
atbm_int32 atbmwifi_wpa_event_queue(void *user_data1,void *user_data2,void *user_data3,
	enum atbm_wpa_event_id event_id,atbm_uint8 wait_ack);
atbm_uint32 atbmwifi_wpa_event_destory(atbm_void);

#endif //_WPA_TIMER_H
