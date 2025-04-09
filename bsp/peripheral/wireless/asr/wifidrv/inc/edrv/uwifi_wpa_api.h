/**
 ****************************************************************************************
 *
 * @file uwifi_wpa_api.h
 *
 * @brief api of wpa api
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#ifndef _UWIFI_WPA_API_H_
#define _UWIFI_WPA_API_H_

int wpa_start_connect(struct wifi_conn_t * wifi_connect_param);
int wpa_start_connect_adv(struct config_info *pst_config_info);
int wpa_start_scan(struct wifi_scan_t* wifi_scan);
int wpa_disconnect(void);
int wpa_set_power(uint8_t tx_power, uint32_t iftype);
int cipher_suite_to_wpa_cipher(uint32_t cipher_suite);
void wpa_update_config_info(struct config_info *pst_config_info, struct scan_result *pst_scan_result);
struct config_info* uwifi_get_sta_config_info(void);
void uwifi_clear_autoconnect_status(struct config_info *pst_config_info);

int wpa_get_channels_from_country_code(struct asr_hw *asr_hw);

#endif //_UWIFI_WPA_API_H_

