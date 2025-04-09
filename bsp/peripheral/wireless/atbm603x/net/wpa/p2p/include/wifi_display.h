/*
 * wpa_supplicant - Wi-Fi Display
 * Copyright (c) 2011, Atheros Communications, Inc.
 * Copyright (c) 2011-2012, Qualcomm Atheros, Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef WIFI_DISPLAY_H
#define WIFI_DISPLAY_H

int wifi_display_init(struct atbmwifi_vif *priv);
void wifi_display_deinit(struct atbmwifi_vif *priv);
void wifi_display_enable(struct atbmwifi_vif *priv, int enabled);
struct wpabuf *wifi_display_get_wfd_ie(struct atbmwifi_vif *priv);
int wifi_display_subelem_set(struct atbmwifi_vif *priv, int subelem, char *val);
int wifi_display_subelem_set_from_ies(struct atbmwifi_vif *priv,
				      struct wpabuf *ie);
int wifi_display_subelem_get(struct atbmwifi_vif *priv, char *cmd,
			     char *buf, atbm_size_t buflen);
char * wifi_display_subelem_hex(const struct wpabuf *wfd_subelems, atbm_uint8 id);

#endif /* WIFI_DISPLAY_H */
