/**************************************************************************************************************
* altobeam IOT Wi-Fi
*
* Copyright (c) 2018, altobeam.inc   All rights reserved.
*
* The source code contains proprietary information of AltoBeam, and shall not be distributed, 
* copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#ifndef WPS_STA_H
#define WPS_STA_H



 int wpas_init_wps(struct wpa_supplicant *wpa_s);
 atbm_void wpas_deinit_wps(struct wpa_supplicant *wpa_s);
 atbm_void wpas_clear_wps(struct wpa_supplicant *wpa_s);
 int wpas_cancel_wps(struct wpa_supplicant *wpa_s);
 atbm_void wpas_wps_timeout(void *eloop_ctx, atbm_void *timeout_ctx);
 int wpas_wps_start_pbc(struct wpa_supplicant *wpa_s, const atbm_uint8 *bssid, int p2p_group);
 int wpas_wps_start_pin(struct wpa_supplicant *wpa_s, const atbm_uint8 *bssid,
			   const char *pin, int p2p_group, atbm_uint16 dev_pw_id);

#endif
