/**
 ******************************************************************************
 *
 * @file uwifi_notify.h
 *
 * @brief uwifi_notify header
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */
#ifndef _UWIFI_NOTIFY_H_
#define _UWIFI_NOTIFY_H_

#include "asr_wlan_api_aos.h"
#ifdef ALIOS_SUPPORT
#ifdef ALIOS3_SUPPORT
#include <aos/hal/wifi.h>
#else
#include <hal/wifi.h>
#endif
#endif
#include "asr_rtos.h"

//#define UWIFI_NOFITY_MXCHIP

enum wifi_open_close_state
{
    WIFI_OPEN_SUCCESS,
    WIFI_OPEN_FAILED,
    WIFI_CLOSE_SUCCESS,
    WIFI_CLOSE_FAILED,
};

enum wifi_reboot_error
{
    WIFI_ERROR_INIT = 1,       //WIFI init failed
    WIFI_ERROR_NOGW,           //ARP can not communication with gateway
    WIFI_ERROR_NOBUS_CREDIT,   //wifi low layer has some problem, can not communication
};

// FIXME: comment this for compile
//#ifndef ALIOS_SUPPORT
typedef struct {
    void (*ip_got)(asr_wlan_ip_stat_t *pnet);
    void (*stat_chg)(asr_wifi_event_e stat);
    void (*scan_compeleted)(asr_wlan_scan_result_t *result);
    void (*associated_ap)(asr_wlan_ap_info_adv_t *ap_info);
    void (*rssi_chg)(uint8_t rssi_level);
    void (*error_status)(asr_wlan_err_status_e err_status);
    void (*ap_add_dev)(char *mac);
    void (*ap_del_dev)(char *mac);
} asr_wlan_event_cb_t;
extern asr_wlan_event_cb_t asr_wlan_event_cb_handle;
//#endif

void IOpenCloseStateCallback(enum wifi_open_close_state state);
#endif //_UWIFI_NOTIFY_H_