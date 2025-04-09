/*
 * hostapd / WPS integration
 * Copyright (c) 2008-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef WPS_HOSTAPD_H
#define WPS_HOSTAPD_H

#if CONFIG_WPS

int hostapd_init_wps(struct atbmwifi_vif *priv);
int hostapd_init_wps_complete(struct hostapd_data *hapd);
void hostapd_deinit_wps(struct hostapd_data *hapd);
int hostapd_cancel_wps(struct hostapd_data *hapd);
void hostapd_wps_timeout(void *eloop_ctx, atbm_void *timeout_ctx);
int hostapd_wps_button_pushed(struct hostapd_data *hapd, atbm_void* ctx);
void hostapd_wps_ap_pin_disable(struct hostapd_data *hapd);
int hostapd_wps_config_ap(struct hostapd_data *hapd, char *ssid,const char *auth, const char *encr,  char *key);
struct wps_registrar *wps_registrar_init(struct wps_context *wps,struct wps_registrar_config *cfg);

#endif /* CONFIG_WPS */
#endif /* WPS_HOSTAPD_H */
