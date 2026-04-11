/**************************************************************************************************************
* altobeam IOT Wi-Fi
*
* Copyright (c) 2018, altobeam.inc   All rights reserved.
*
* The source code contains proprietary information of AltoBeam, and shall not be distributed, 
* copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#include "atbm_hal.h"
#include "wps_dev_attr.h"
#include "wpa_debug.h"

#ifndef CONFIG_WPS_STRICT
#define WPS_WORKAROUNDS
#endif /* CONFIG_WPS_STRICT */
#define wps_free_nfc_pw_tokens(t, p) do { } while (0)

extern unsigned char * atbmwifi_base64_encode(unsigned char *src, atbm_size_t len,
			      atbm_size_t *out_len);
extern int atbmwifi_wpa_snprintf_hex(char *buf, atbm_size_t buf_size, const atbm_uint8 *data, atbm_size_t len);
extern int  atbmwifi_ap_start_beacon(struct atbmwifi_vif *priv);
struct wps_registrar_device {
	struct wps_registrar_device *next;
	struct wps_device_data dev;
	atbm_uint8 uuid[WPS_UUID_LEN];
};


struct wps_registrar {
	struct wps_context *wps;

	atbm_uint8 pbc;
	atbm_uint8 selected_registrar;
	atbm_uint8 skip_cred_build;
	atbm_uint8 disable_auto_conf;
	atbm_uint8 sel_reg_union;
	//atbm_uint8 sel_reg_union;
	atbm_uint8 sel_reg_dev_password_id_override;
	atbm_uint8 sel_reg_config_methods_override;
	atbm_uint8 static_wep_only;
	atbm_uint8 force_pbc_overlap;

	int (*new_psk_cb)(void *ctx, const atbm_uint8 *mac_addr, const atbm_uint8 *psk,
			  atbm_size_t psk_len);
	int (*set_ie_cb)(void *ctx, struct wpabuf *beacon_ie,
			 struct wpabuf *probe_resp_ie);
	void (*pin_needed_cb)(void *ctx, const atbm_uint8 *uuid_e,
			      const struct wps_device_data *dev);
	void (*reg_success_cb)(void *ctx, const atbm_uint8 *mac_addr,
			       const atbm_uint8 *uuid_e, const atbm_uint8 *dev_pw,
			       atbm_size_t dev_pw_len);
	void (*set_sel_reg_cb)(void *ctx, int sel_reg, atbm_uint16 dev_passwd_id,
			       atbm_uint16 sel_reg_config_methods);
	void (*enrollee_seen_cb)(void *ctx, const atbm_uint8 *addr, const atbm_uint8 *uuid_e,
				 const atbm_uint8 *pri_dev_type, atbm_uint16 config_methods,
				 atbm_uint16 dev_password_id, atbm_uint8 request_type,
				 const char *dev_name);
	void *cb_ctx;


	struct wpabuf *extra_cred;
//	int dualband;

	//struct wps_registrar_device *devices;


	//atbm_uint8 authorized_macs[WPS_MAX_AUTHORIZED_MACS][ATBM_ETH_ALEN];
	//atbm_uint8 authorized_macs_union[WPS_MAX_AUTHORIZED_MACS][ATBM_ETH_ALEN];

	atbm_uint8 p2p_dev_addr[ATBM_ETH_ALEN];
};


 static int wps_set_ie(struct wps_registrar *reg);
 static atbm_void wps_registrar_pbc_timeout(void *eloop_ctx, atbm_void *timeout_ctx);
 static atbm_void wps_registrar_set_selected_timeout(void *eloop_ctx,
					       atbm_void *timeout_ctx);




#if 0
 static atbm_void wps_free_devices(struct wps_registrar_device *dev)
{
	struct wps_registrar_device *prev;

	while (dev) {
		prev = dev;
		dev = dev->next;
		wps_device_data_free(&prev->dev);
		atbm_kfree(prev);
	}
}


 static struct wps_registrar_device * wps_device_get(struct wps_registrar *reg,
						    const atbm_uint8 *addr)
{
	struct wps_registrar_device *dev;

	for (dev = reg->devices; dev; dev = dev->next) {
		if (atbm_memcmp(dev->dev.mac_addr, addr, ATBM_ETH_ALEN) == 0)
			return dev;
	}
	return NULL;
}


 static atbm_void wps_device_clone_data(struct wps_device_data *dst,
				  struct wps_device_data *src)
{
	atbm_memcpy(dst->mac_addr, src->mac_addr, ATBM_ETH_ALEN);
	atbm_memcpy(dst->pri_dev_type, src->pri_dev_type, WPS_DEV_TYPE_LEN);

#define WPS_STRDUP(n) \
	atbm_kfree(dst->n); \
	dst->n = src->n ? strdup(src->n) : NULL

	WPS_STRDUP(device_name);
	WPS_STRDUP(manufacturer);
	WPS_STRDUP(model_name);
	WPS_STRDUP(model_number);
	WPS_STRDUP(serial_number);
#undef WPS_STRDUP
}


 int wps_device_store(struct wps_registrar *reg,
		     struct wps_device_data *dev, const atbm_uint8 *uuid)
{
	struct wps_registrar_device *d;

	d = wps_device_get(reg, dev->mac_addr);
	if (d == NULL) {
		d = (struct wps_registrar_device *)atbm_kzalloc(sizeof(*d), GFP_KERNEL);
		if (d == NULL)
			return -1;
		d->next = reg->devices;
		reg->devices = d;
	}

	wps_device_clone_data(&d->dev, dev);
	atbm_memcpy(d->uuid, uuid, WPS_UUID_LEN);

	return 0;
}
#endif //0



 int wps_registrar_pbc_overlap(struct wps_registrar *reg,
			      const atbm_uint8 *addr, const atbm_uint8 *uuid_e)
{
	return 0;
}


 static int wps_build_wps_state(struct wps_context *wps, struct wpabuf *msg)
{
	wpa_printf(MSG_DEBUG, "WPS:  * Wi-Fi Protected Setup State (%d)",
		   wps->wps_state);
	wpabuf_put_be16(msg, ATTR_WPS_STATE);
	wpabuf_put_be16(msg, 1);
	wpabuf_put_u8(msg, wps->wps_state);
	return 0;
}




 static int wps_build_ap_setup_locked(struct wps_context *wps,
				     struct wpabuf *msg)
{
	if (wps->ap_setup_locked && wps->ap_setup_locked != 2) {
		wpa_printf(MSG_DEBUG, "WPS:  * AP Setup Locked");
		wpabuf_put_be16(msg, ATTR_AP_SETUP_LOCKED);
		wpabuf_put_be16(msg, 1);
		wpabuf_put_u8(msg, 1);
	}
	return 0;
}


 static int wps_build_selected_registrar(struct wps_registrar *reg,
					struct wpabuf *msg)
{
	if (!reg->sel_reg_union)
		return 0;
	wpa_printf(MSG_DEBUG, "WPS:  * Selected Registrar");
	wpabuf_put_be16(msg, ATTR_SELECTED_REGISTRAR);
	wpabuf_put_be16(msg, 1);
	wpabuf_put_u8(msg, 1);
	return 0;
}


 static int wps_build_sel_reg_dev_password_id(struct wps_registrar *reg,
					     struct wpabuf *msg)
{
	atbm_uint16 id = reg->pbc ? DEV_PW_PUSHBUTTON : DEV_PW_DEFAULT;
	if (!reg->sel_reg_union)
		return 0;
	if (reg->sel_reg_dev_password_id_override >= 0)
		id = reg->sel_reg_dev_password_id_override;
	wpa_printf(MSG_DEBUG, "WPS:  * Device Password ID (%d)", id);
	wpabuf_put_be16(msg, ATTR_DEV_PASSWORD_ID);
	wpabuf_put_be16(msg, 2);
	wpabuf_put_be16(msg, id);
	return 0;
}

/*
 static int wps_build_sel_pbc_reg_uuid_e(struct wps_registrar *reg,
					struct wpabuf *msg)
{
	atbm_uint16 id = reg->pbc ? DEV_PW_PUSHBUTTON : DEV_PW_DEFAULT;
	if (!reg->sel_reg_union)
		return 0;
	if (reg->sel_reg_dev_password_id_override >= 0)
		id = reg->sel_reg_dev_password_id_override;
	if (id != DEV_PW_PUSHBUTTON || !reg->dualband)
		return 0;
	return wps_build_uuid_e(msg, reg->wps->uuid);
}
*/

 static atbm_void wps_set_pushbutton(atbm_uint16 *methods, atbm_uint16 conf_methods)
{
	*methods |= WPS_CONFIG_PUSHBUTTON;
#if CONFIG_WPS2
	if ((conf_methods & WPS_CONFIG_VIRT_PUSHBUTTON) ==
	    WPS_CONFIG_VIRT_PUSHBUTTON)
		*methods |= WPS_CONFIG_VIRT_PUSHBUTTON;
	if ((conf_methods & WPS_CONFIG_PHY_PUSHBUTTON) ==
	    WPS_CONFIG_PHY_PUSHBUTTON)
		*methods |= WPS_CONFIG_PHY_PUSHBUTTON;
	if ((*methods & WPS_CONFIG_VIRT_PUSHBUTTON) !=
	    WPS_CONFIG_VIRT_PUSHBUTTON &&
	    (*methods & WPS_CONFIG_PHY_PUSHBUTTON) !=
	    WPS_CONFIG_PHY_PUSHBUTTON) {
		/*
		 * Required to include virtual/physical flag, but we were not
		 * configured with push button type, so have to default to one
		 * of them.
		 */
		*methods |= WPS_CONFIG_PHY_PUSHBUTTON;
	}
#endif /* CONFIG_WPS2 */
}


 static int wps_build_sel_reg_config_methods(struct wps_registrar *reg,
					    struct wpabuf *msg)
{
	atbm_uint16 methods;
	if (!reg->sel_reg_union)
		return 0;
	methods = reg->wps->config_methods;
	methods &= ~WPS_CONFIG_PUSHBUTTON;
#if CONFIG_WPS2
	methods &= ~(WPS_CONFIG_VIRT_PUSHBUTTON |
		     WPS_CONFIG_PHY_PUSHBUTTON);
#endif /* CONFIG_WPS2 */
	if (reg->pbc)
		wps_set_pushbutton(&methods, reg->wps->config_methods);
	if (reg->sel_reg_config_methods_override >= 0)
		methods = reg->sel_reg_config_methods_override;
	wpa_printf(MSG_DEBUG, "WPS:  * Selected Registrar Config Methods (%x)",
		   methods);
	wpabuf_put_be16(msg, ATTR_SELECTED_REGISTRAR_CONFIG_METHODS);
	wpabuf_put_be16(msg, 2);
	wpabuf_put_be16(msg, methods);
	return 0;
}


 static int wps_build_probe_config_methods(struct wps_registrar *reg,
					  struct wpabuf *msg)
{
	atbm_uint16 methods;
	/*
	 * These are the methods that the AP supports as an Enrollee for adding
	 * external Registrars.
	 */
	/*rsharma for WFD*/
	methods = reg->wps->config_methods;
//	methods = reg->wps->config_methods & ~WPS_CONFIG_PUSHBUTTON;
//#if CONFIG_WPS2
///	methods &= ~(WPS_CONFIG_VIRT_PUSHBUTTON |
//		     WPS_CONFIG_PHY_PUSHBUTTON);
//#endif /* CONFIG_WPS2 */
	wpa_printf(MSG_DEBUG, "WPS:  * Config Methods (%x)", methods);
	wpabuf_put_be16(msg, ATTR_CONFIG_METHODS);
	wpabuf_put_be16(msg, 2);
	wpabuf_put_be16(msg, methods);
	return 0;
}


 static int wps_build_config_methods_r(struct wps_registrar *reg,
				      struct wpabuf *msg)
{
	return wps_build_config_methods(msg, reg->wps->config_methods);
}



/**
 * wps_registrar_init - Initialize WPS Registrar data
 * @wps: Pointer to longterm WPS context
 * @cfg: Registrar configuration
 * Returns: Pointer to allocated Registrar data or %NULL on failure
 *
 * This function is used to initialize WPS Registrar functionality. It can be
 * used for a single Registrar run (e.g., when run in a supplicant) or multiple
 * runs (e.g., when run as an internal Registrar in an AP). Caller is
 * responsible for freeing the returned data with wps_registrar_deinit() when
 * Registrar functionality is not needed anymore.
 */
 struct wps_registrar *
wps_registrar_init(struct wps_context *wps,struct wps_registrar_config *cfg)
{
	struct wps_registrar *reg = (struct wps_registrar *)atbm_kzalloc(sizeof(*reg), GFP_KERNEL);
	if (reg == NULL)
		return NULL;

	reg->wps = wps;
	reg->cb_ctx = cfg->cb_ctx;
	reg->skip_cred_build = cfg->skip_cred_build;
	if (cfg->extra_cred) {
		reg->extra_cred = wpabuf_alloc_copy(cfg->extra_cred,
						    cfg->extra_cred_len);
		if (reg->extra_cred == NULL) {
			atbm_kfree(reg);
			return NULL;
		}
	}
	reg->disable_auto_conf = cfg->disable_auto_conf;
	reg->sel_reg_dev_password_id_override = -1;
	reg->sel_reg_config_methods_override = -1;
	reg->static_wep_only = cfg->static_wep_only;
	//reg->dualband = cfg->dualband;

	return reg;
}


/**
 * wps_registrar_deinit - Deinitialize WPS Registrar data
 * @reg: Registrar data from wps_registrar_init()
 */
 atbm_void wps_registrar_deinit(struct wps_registrar *reg)
{
	if (reg == NULL)
		return;
	atbmwifi_eloop_cancel_timeout(wps_registrar_pbc_timeout, reg, NULL);
	atbmwifi_eloop_cancel_timeout(wps_registrar_set_selected_timeout, reg, NULL);
	wpabuf_free(reg->extra_cred);
	atbm_kfree(reg);
}


 int wps_registrar_add_pin(struct wps_registrar *reg,const atbm_uint8 *addr)
{
	//wpa_printf(MSG_DEBUG, "WPS: A new PIN configured ");
	//wpa_hexdump(MSG_DEBUG, "WPS: UUID", uuid, WPS_UUID_LEN);
	//wpa_hexdump_ascii_key(MSG_DEBUG, "WPS: PIN", pin, pin_len);
	reg->selected_registrar = 1;
	reg->pbc = 0;
	wps_registrar_selected_registrar_changed(reg, 0);
	atbmwifi_eloop_cancel_timeout(wps_registrar_set_selected_timeout, reg, NULL);
	atbmwifi_eloop_register_timeout(WPS_PBC_WALK_TIME, 0,
			       wps_registrar_set_selected_timeout,
			       reg, NULL);

	return 0;
}


/* static atbm_void wps_registrar_remove_pin(struct wps_registrar *reg)
{
//	atbm_uint8 bcast[ATBM_ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	wps_registrar_selected_registrar_changed(reg, 0);
}*/

 static atbm_void wps_registrar_stop_pbc(struct wps_registrar *reg)
{
	reg->selected_registrar = 0;
	reg->pbc = 0;
	atbm_memset(reg->p2p_dev_addr, 0, ATBM_ETH_ALEN);
	wps_registrar_selected_registrar_changed(reg, 0);
}


 static atbm_void wps_registrar_pbc_timeout(void *eloop_ctx, atbm_void *timeout_ctx)
{
	struct wps_registrar *reg = eloop_ctx;

	wpa_printf(MSG_DEBUG, "WPS: PBC timed out - disable PBC mode");
	wps_pbc_timeout_event(reg->wps);
	wps_registrar_stop_pbc(reg);
}


/**
 * wps_registrar_button_pushed - Notify Registrar that AP button was pushed
 * @reg: Registrar data from wps_registrar_init()
 * @p2p_dev_addr: Limit allowed PBC devices to the specified P2P device, %NULL
 *	indicates no such filtering
 * Returns: 0 on success, -1 on failure, -2 on session overlap
 *
 * This function is called on an AP when a push button is pushed to activate
 * PBC mode. The PBC mode will be stopped after walk time (2 minutes) timeout
 * or when a PBC registration is completed. If more than one Enrollee in active
 * PBC mode has been detected during the monitor time (previous 2 minutes), the
 * PBC mode is not activated and -2 is returned to indicate session overlap.
 * This is skipped if a specific Enrollee is selected.
 */
 int wps_registrar_button_pushed(struct wps_registrar *reg,
				const atbm_uint8 *p2p_dev_addr)
{
	if (p2p_dev_addr == NULL &&
	    wps_registrar_pbc_overlap(reg, NULL, NULL)) {
		wpa_printf(MSG_ERROR, "WPS: PBC overlap - do not start PBC "
			   "mode");
		wps_pbc_overlap_event(reg->wps);
		return -2;
	}
	wpa_printf(MSG_DEBUG, "WPS: Button pushed - PBC mode started");
	reg->force_pbc_overlap = 0;
	reg->selected_registrar = 1;
	reg->pbc = 1;
	if (p2p_dev_addr)
		atbm_memcpy(reg->p2p_dev_addr, p2p_dev_addr, ATBM_ETH_ALEN);
	else
		atbm_memset(reg->p2p_dev_addr, 0, ATBM_ETH_ALEN);
	//wps_registrar_add_authorized_mac(reg,
	//				 (atbm_uint8 *) "\xff\xff\xff\xff\xff\xff");
	wps_registrar_selected_registrar_changed(reg, 0);

	atbmwifi_eloop_cancel_timeout(wps_registrar_set_selected_timeout, reg, NULL);
	atbmwifi_eloop_cancel_timeout(wps_registrar_pbc_timeout, reg, NULL);
	atbmwifi_eloop_register_timeout(120, 0, wps_registrar_pbc_timeout,
			       reg, NULL);
	return 0;
}


 static atbm_void wps_registrar_pbc_completed(struct wps_registrar *reg)
{
	wpa_printf(MSG_DEBUG, "WPS: PBC completed - stopping PBC mode");
	atbmwifi_eloop_cancel_timeout(wps_registrar_pbc_timeout, reg, NULL);
	wps_registrar_stop_pbc(reg);
}


 static atbm_void wps_registrar_pin_completed(struct wps_registrar *reg)
{
	wpa_printf(MSG_ALWAYS, "WPS: PIN completed internal Registrar");
	atbmwifi_eloop_cancel_timeout(wps_registrar_set_selected_timeout, reg, NULL);
	reg->selected_registrar = 0;
	wps_registrar_selected_registrar_changed(reg, 0);
}


 atbm_void wps_registrar_complete(struct wps_registrar *registrar, const atbm_uint8 *uuid_e,
			    const atbm_uint8 *dev_pw, atbm_size_t dev_pw_len)
{
	if (registrar->pbc) {
		wps_registrar_pbc_completed(registrar);
	} else {
		wps_registrar_pin_completed(registrar);
	}
/*
	if (dev_pw &&
	    wps_registrar_invalidate_wildcard_pin(registrar, dev_pw,
						  dev_pw_len) == 0) {
		wpa_hexdump_key(MSG_DEBUG, "WPS: Invalidated wildcard PIN",
				dev_pw, dev_pw_len);
	}*/
}


 int wps_registrar_wps_cancel(struct wps_registrar *reg)
{
	if (reg->pbc) {
		wpa_printf(MSG_DEBUG, "WPS: PBC is set - cancelling it");
		wps_registrar_pbc_timeout(reg, NULL);
		atbmwifi_eloop_cancel_timeout(wps_registrar_pbc_timeout, reg, NULL);
		return 1;
	} else if (reg->selected_registrar) {
		/* PIN Method */
		wpa_printf(MSG_DEBUG, "WPS: PIN is set - cancelling it");
		wps_registrar_pin_completed(reg);
		//wps_registrar_invalidate_wildcard_pin(reg, NULL, 0);
		return 1;
	}
	return 0;
}




/* static atbm_void wps_cb_pin_needed(struct wps_registrar *reg, const atbm_uint8 *uuid_e,
			      const struct wps_device_data *dev)
{
	if (reg->pin_needed_cb == NULL)
		return;

	reg->pin_needed_cb(reg->cb_ctx, uuid_e, dev);
}*/


 static atbm_void wps_cb_reg_success(struct wps_registrar *reg, const atbm_uint8 *mac_addr,
			       const atbm_uint8 *uuid_e, const atbm_uint8 *dev_pw,
			       atbm_size_t dev_pw_len)
{
	if (reg->reg_success_cb == NULL)
		return;

	reg->reg_success_cb(reg->cb_ctx, mac_addr, uuid_e, dev_pw, dev_pw_len);
}



 static atbm_void wps_cb_set_sel_reg(struct wps_registrar *reg)
{
	atbm_uint16 methods = 0;
	if (reg->set_sel_reg_cb == NULL)
		return;

	if (reg->selected_registrar) {
		methods = reg->wps->config_methods & ~WPS_CONFIG_PUSHBUTTON;
#if CONFIG_WPS2
		methods &= ~(WPS_CONFIG_VIRT_PUSHBUTTON |
			     WPS_CONFIG_PHY_PUSHBUTTON);
#endif /* CONFIG_WPS2 */
		if (reg->pbc)
			wps_set_pushbutton(&methods, reg->wps->config_methods);
	}

	wpa_printf(MSG_ALWAYS, "WPS: wps_cb_set_sel_reg: sel_reg=%d "
		   "config_methods=0x%x pbc=%d methods=0x%x",
		   reg->selected_registrar, reg->wps->config_methods,
		   reg->pbc, methods);

	reg->set_sel_reg_cb(reg->cb_ctx, reg->selected_registrar,
			    reg->pbc ? DEV_PW_PUSHBUTTON : DEV_PW_DEFAULT,
			    methods);
}


 static int wps_set_ie(struct wps_registrar *reg)
{
	struct wpabuf *beacon;
	struct wpabuf *probe;
	//const atbm_uint8 *auth_macs;
	//atbm_size_t count;
	atbm_size_t vendor_len = 0;
	int i;
	struct hostapd_data *hapd = reg->cb_ctx;

//	if (reg->selected_registrar == 0)
//		goto out;

	for (i = 0; i < MAX_WPS_VENDOR_EXTENSIONS; i++) {
		if (reg->wps->dev.vendor_ext[i]) {
			vendor_len += 2 + 2;
			vendor_len += wpabuf_len(reg->wps->dev.vendor_ext[i]);
		}
	}

	beacon = wpabuf_alloc(400 + vendor_len);
	if (beacon == NULL)
		return -1;
	probe = wpabuf_alloc(500 + vendor_len);
	if (probe == NULL) {
		wpabuf_free(beacon);
		return -1;
	}

	//auth_macs = wps_authorized_macs(reg, &count);

	wpa_printf(MSG_DEBUG, "WPS: Build Beacon IEs");

	if (wps_build_version(beacon) ||
	    wps_build_wps_state(reg->wps, beacon) ||
	    wps_build_ap_setup_locked(reg->wps, beacon) ||
	    wps_build_selected_registrar(reg, beacon) ||
	    wps_build_sel_reg_dev_password_id(reg, beacon) ||
	    wps_build_sel_reg_config_methods(reg, beacon) ||
	  //  wps_build_sel_pbc_reg_uuid_e(reg, beacon) ||
	   // (reg->dualband && wps_build_rf_bands(&reg->wps->dev, beacon)) ||
	   // wps_build_wfa_ext(beacon, 0, auth_macs, count) ||
	    wps_build_vendor_ext(&reg->wps->dev, beacon)) {
		wpabuf_free(beacon);
		wpabuf_free(probe);
		return -1;
	}

#if CONFIG_P2P
	if (wps_build_dev_name(&reg->wps->dev, beacon) ||
	    wps_build_primary_dev_type(&reg->wps->dev, beacon)) {
		wpabuf_free(beacon);
		wpabuf_free(probe);
		return -1;
	}
#endif /* CONFIG_P2P */

	wpa_printf(MSG_DEBUG, "WPS: Build Probe Response IEs");

	if (wps_build_version(probe) ||
	    wps_build_wps_state(reg->wps, probe) ||
	    wps_build_ap_setup_locked(reg->wps, probe) ||
	    wps_build_selected_registrar(reg, probe) ||
	    wps_build_sel_reg_dev_password_id(reg, probe) ||
	    wps_build_sel_reg_config_methods(reg, probe) ||
	    wps_build_resp_type(probe, reg->wps->ap ? WPS_RESP_AP :
				WPS_RESP_REGISTRAR) ||
	    wps_build_uuid_e(probe, reg->wps->uuid) ||
	    wps_build_device_attrs(&reg->wps->dev, probe) ||
	    wps_build_probe_config_methods(reg, probe) ||
	    wps_build_rf_bands(&reg->wps->dev, probe) ||
	   // wps_build_wfa_ext(probe, 0, auth_macs, count) ||
	    wps_build_vendor_ext(&reg->wps->dev, probe)) {
		wpabuf_free(beacon);
		wpabuf_free(probe);
		return -1;
	}

	beacon = wps_ie_encapsulate(beacon);
	probe = wps_ie_encapsulate(probe);

	if (!beacon || !probe) {
		wpabuf_free(beacon);
		wpabuf_free(probe);
		return -1;
	}

	if (reg->static_wep_only) {
		/*
		 * Windows XP and Vista clients can get confused about
		 * EAP-Identity/Request when they probe the network with
		 * EAPOL-Start. In such a case, they may assume the network is
		 * using IEEE 802.1X and prompt user for a certificate while
		 * the correct (non-WPS) behavior would be to ask for the
		 * static WEP key. As a workaround, use Microsoft Provisioning
		 * IE to advertise that legacy 802.1X is not supported.
		 */
		const atbm_uint8 ms_wps[7] = {
			ATBM_WLAN_EID_VENDOR_SPECIFIC, 5,
			/* Microsoft Provisioning IE (00:50:f2:5) */
			0x00, 0x50, 0xf2, 5,
			0x00 /* no legacy 802.1X or MS WPS */
		};
		wpa_printf(MSG_DEBUG, "WPS: Add Microsoft Provisioning IE "
			   "into Beacon/Probe Response frames");
		wpabuf_put_data(beacon, ms_wps, sizeof(ms_wps));
		wpabuf_put_data(probe, ms_wps, sizeof(ms_wps));
	}

	
	wpabuf_free(hapd->wps_beacon_ie);
	hapd->wps_beacon_ie = beacon;
	hapd->priv->wps_beacon_ie = hapd->wps_beacon_ie->buf;
	hapd->priv->wps_beacon_ie_len = hapd->wps_beacon_ie->used;
	wpabuf_free(hapd->wps_probe_resp_ie);
	hapd->wps_probe_resp_ie = probe;
	hapd->priv->wps_probe_resp_ie = hapd->wps_probe_resp_ie->buf;
	hapd->priv->wps_probe_resp_ie_len = hapd->wps_probe_resp_ie->used;
	wpa_printf(MSG_DEBUG, "WPS: atbmwifi_ap_start_beacon\n");
	wpa_comm_init_extra_ie(hapd->priv);
	atbmwifi_ap_start_beacon(hapd->priv);
	return 0;
out:
		
	wpabuf_free(hapd->wps_beacon_ie);
	hapd->wps_beacon_ie = NULL;
	hapd->priv->wps_beacon_ie = NULL;
	hapd->priv->wps_beacon_ie_len = 0;
	wpabuf_free(hapd->wps_probe_resp_ie);
	hapd->wps_probe_resp_ie = NULL;
	hapd->priv->wps_probe_resp_ie = NULL;
	hapd->priv->wps_probe_resp_ie_len = 0;
	wpa_printf(MSG_DEBUG, "WPS: atbmwifi_ap_start_beacon, clear wps ie\n");

	hostapd_init_extra_ie(hapd->priv);
	wpa_printf(MSG_DEBUG,"WPS: atbmwifi_ap_start_beacon, set wpa ie\n");
	atbmwifi_ap_start_beacon(hapd->priv);
	atbm_kfree(hapd->priv->extra_ie);
	hapd->priv->extra_ie = NULL;
	hapd->priv->extra_ie_len = 0;

	return 0;
}

 static int wps_build_uuid_r(struct wps_data *wps, struct wpabuf *msg)
{
	wpa_printf(MSG_DEBUG, "WPS:  * UUID-R");
	wpabuf_put_be16(msg, ATTR_UUID_R);
	wpabuf_put_be16(msg, WPS_UUID_LEN);
	wpabuf_put_data(msg, wps->uuid_r, WPS_UUID_LEN);
	return 0;
}


 static int wps_build_r_hash(struct wps_data *wps, struct wpabuf *msg)
{
	atbm_uint8 *hash;
	const atbm_uint8 *addr[4];
	atbm_size_t len[4];

	if (atbmwifi_os_get_random(wps->snonce, 2 * WPS_SECRET_NONCE_LEN) < 0)
		return -1;
	wpa_hexdump(MSG_DEBUG, "WPS: R-S1", wps->snonce, WPS_SECRET_NONCE_LEN);
	wpa_hexdump(MSG_DEBUG, "WPS: R-S2",
		    wps->snonce + WPS_SECRET_NONCE_LEN, WPS_SECRET_NONCE_LEN);

	if (wps->dh_pubkey_e == NULL || wps->dh_pubkey_r == NULL) {
		wpa_printf(MSG_ERROR, "WPS: DH public keys not available for "
			   "R-Hash derivation");
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPS:  * R-Hash1");
	wpabuf_put_be16(msg, ATTR_R_HASH1);
	wpabuf_put_be16(msg, SHA256_MAC_LEN);
	hash = wpabuf_put(msg, SHA256_MAC_LEN);
	/* R-Hash1 = HMAC_AuthKey(R-S1 || PSK1 || PK_E || PK_R) */
	addr[0] = wps->snonce;
	len[0] = WPS_SECRET_NONCE_LEN;
	addr[1] = wps->psk1;
	len[1] = WPS_PSK_LEN;
	addr[2] = wpabuf_head(wps->dh_pubkey_e);
	len[2] = wpabuf_len(wps->dh_pubkey_e);
	addr[3] = wpabuf_head(wps->dh_pubkey_r);
	len[3] = wpabuf_len(wps->dh_pubkey_r);
	atbmwifi_hmac_sha256_vector(wps->authkey, WPS_AUTHKEY_LEN, 4, addr, len, hash);
	wpa_hexdump(MSG_DEBUG, "WPS: R-Hash1", hash, SHA256_MAC_LEN);

	wpa_printf(MSG_DEBUG, "WPS:  * R-Hash2");
	wpabuf_put_be16(msg, ATTR_R_HASH2);
	wpabuf_put_be16(msg, SHA256_MAC_LEN);
	hash = wpabuf_put(msg, SHA256_MAC_LEN);
	/* R-Hash2 = HMAC_AuthKey(R-S2 || PSK2 || PK_E || PK_R) */
	addr[0] = wps->snonce + WPS_SECRET_NONCE_LEN;
	addr[1] = wps->psk2;
	atbmwifi_hmac_sha256_vector(wps->authkey, WPS_AUTHKEY_LEN, 4, addr, len, hash);
	wpa_hexdump(MSG_DEBUG, "WPS: R-Hash2", hash, SHA256_MAC_LEN);

	return 0;
}


 static int wps_build_r_snonce1(struct wps_data *wps, struct wpabuf *msg)
{
	wpa_printf(MSG_DEBUG, "WPS:  * R-SNonce1");
	wpabuf_put_be16(msg, ATTR_R_SNONCE1);
	wpabuf_put_be16(msg, WPS_SECRET_NONCE_LEN);
	wpabuf_put_data(msg, wps->snonce, WPS_SECRET_NONCE_LEN);
	return 0;
}

 static int wps_build_cred_network_idx(struct wpabuf *msg,
				      const struct wps_credential *cred)
{
	wpa_printf(MSG_DEBUG, "WPS:  * Network Index (1)");
	wpabuf_put_be16(msg, ATTR_NETWORK_INDEX);
	wpabuf_put_be16(msg, 1);
	wpabuf_put_u8(msg, 1);
	return 0;
}
 static int wps_build_cred_ssid(struct wpabuf *msg,
			       const struct wps_credential *cred)
{
	wpa_printf(MSG_DEBUG, "WPS:  * SSID");
	wpa_hexdump_ascii(MSG_DEBUG, "WPS: SSID for Credential",
			  cred->ssid, cred->ssid_len);
	wpabuf_put_be16(msg, ATTR_SSID);
	wpabuf_put_be16(msg, cred->ssid_len);
	wpabuf_put_data(msg, cred->ssid, cred->ssid_len);
	return 0;
}


 static int wps_build_r_snonce2(struct wps_data *wps, struct wpabuf *msg)
{
	wpa_printf(MSG_DEBUG, "WPS:  * R-SNonce2");
	wpabuf_put_be16(msg, ATTR_R_SNONCE2);
	wpabuf_put_be16(msg, WPS_SECRET_NONCE_LEN);
	wpabuf_put_data(msg, wps->snonce + WPS_SECRET_NONCE_LEN,
			WPS_SECRET_NONCE_LEN);
	return 0;
}



 static int wps_build_cred_auth_type(struct wpabuf *msg,
				    const struct wps_credential *cred)
{
	wpa_printf(MSG_DEBUG, "WPS:  * Authentication Type (0x%x)",
		   cred->auth_type);
	wpabuf_put_be16(msg, ATTR_AUTH_TYPE);
	wpabuf_put_be16(msg, 2);
	wpabuf_put_be16(msg, cred->auth_type);
	return 0;
}


 static int wps_build_cred_encr_type(struct wpabuf *msg,
				    const struct wps_credential *cred)
{
	wpa_printf(MSG_DEBUG, "WPS:  * Encryption Type (0x%x)",
		   cred->encr_type);
	wpabuf_put_be16(msg, ATTR_ENCR_TYPE);
	wpabuf_put_be16(msg, 2);
	wpabuf_put_be16(msg, cred->encr_type);
	return 0;
}


 static int wps_build_cred_network_key(struct wpabuf *msg,
				      const struct wps_credential *cred)
{
	wpa_printf(MSG_DEBUG, "WPS:  * Network Key (len=%d)",
		   (int) cred->key_len);
	wpa_hexdump_key(MSG_DEBUG, "WPS: Network Key",
			cred->key, cred->key_len);
	wpabuf_put_be16(msg, ATTR_NETWORK_KEY);
	wpabuf_put_be16(msg, cred->key_len);
	wpabuf_put_data(msg, cred->key, cred->key_len);
	return 0;
}


 static int wps_build_cred_mac_addr(struct wpabuf *msg,
				   const struct wps_credential *cred)
{
	wpa_printf(MSG_DEBUG, "WPS:  * MAC Address (" MACSTR ")",
		   MAC2STR(cred->mac_addr));
	wpabuf_put_be16(msg, ATTR_MAC_ADDR);
	wpabuf_put_be16(msg, ATBM_ETH_ALEN);
	wpabuf_put_data(msg, cred->mac_addr, ATBM_ETH_ALEN);
	return 0;
}


 static int wps_build_credential(struct wpabuf *msg,
				const struct wps_credential *cred)
{
	if (wps_build_cred_network_idx(msg, cred) ||
	    wps_build_cred_ssid(msg, cred) ||
	    wps_build_cred_auth_type(msg, cred) ||
	    wps_build_cred_encr_type(msg, cred) ||
	    wps_build_cred_network_key(msg, cred) ||
	    wps_build_cred_mac_addr(msg, cred))
		return -1;
	return 0;
}



 int wps_build_cred(struct wps_data *wps, struct wpabuf *msg)
{
	struct wpabuf *cred;

	if (wps->wps->registrar->skip_cred_build)
		goto skip_cred_build;

	wpa_printf(MSG_DEBUG, "WPS:  * Credential");
	if (wps->use_cred) {
		atbm_memcpy(&wps->cred, wps->use_cred, sizeof(wps->cred));
		goto use_provided;
	}
	atbm_memset(&wps->cred, 0, sizeof(wps->cred));

	atbm_memcpy(wps->cred.ssid, wps->wps->ssid, wps->wps->ssid_len);
	wps->cred.ssid_len = wps->wps->ssid_len;

	/* Select the best authentication and encryption type */
	if (wps->auth_type & WPS_AUTH_WPA2PSK)
		wps->auth_type = WPS_AUTH_WPA2PSK;
	else if (wps->auth_type & WPS_AUTH_WPAPSK)
		wps->auth_type = WPS_AUTH_WPAPSK;
	else if (wps->auth_type & WPS_AUTH_OPEN)
		wps->auth_type = WPS_AUTH_OPEN;
	else if (wps->auth_type & WPS_AUTH_SHARED)
		wps->auth_type = WPS_AUTH_SHARED;
	else {
		wpa_printf(MSG_DEBUG, "WPS: Unsupported auth_type 0x%x",
			   wps->auth_type);
		return -1;
	}
	wps->cred.auth_type = wps->auth_type;

	if (wps->auth_type == WPS_AUTH_WPA2PSK ||
	    wps->auth_type == WPS_AUTH_WPAPSK) {
		if (wps->encr_type & WPS_ENCR_AES)
			wps->encr_type = WPS_ENCR_AES;
		else if (wps->encr_type & WPS_ENCR_TKIP)
			wps->encr_type = WPS_ENCR_TKIP;
		else {
			wpa_printf(MSG_DEBUG, "WPS: No suitable encryption "
				   "type for WPA/WPA2");
			return -1;
		}
	} else {
		if (wps->encr_type & WPS_ENCR_WEP)
			wps->encr_type = WPS_ENCR_WEP;
		else if (wps->encr_type & WPS_ENCR_NONE)
			wps->encr_type = WPS_ENCR_NONE;
		else {
			wpa_printf(MSG_DEBUG, "WPS: No suitable encryption "
				   "type for non-WPA/WPA2 mode");
			return -1;
		}
	}
	wps->cred.encr_type = wps->encr_type;
	/*
	 * Set MAC address in the Credential to be the Enrollee's MAC address
	 */
	atbm_memcpy(wps->cred.mac_addr, wps->mac_addr_e, ATBM_ETH_ALEN);

	if (wps->wps->wps_state == WPS_STATE_NOT_CONFIGURED && wps->wps->ap &&
	    !wps->wps->registrar->disable_auto_conf) {
		atbm_uint8 r[16];
		/* Generate a random passphrase */
		if (atbmwifi_os_get_random(r, sizeof(r)) < 0)
			return -1;
		atbm_kfree(wps->new_psk);
		wps->new_psk = atbmwifi_base64_encode(r, sizeof(r), &wps->new_psk_len);
		if (wps->new_psk == NULL)
			return -1;
		wps->new_psk_len--; /* remove newline */
		while (wps->new_psk_len &&
		       wps->new_psk[wps->new_psk_len - 1] == '=')
			wps->new_psk_len--;
		wpa_hexdump_ascii_key(MSG_DEBUG, "WPS: Generated passphrase",
				      wps->new_psk, wps->new_psk_len);
		atbm_memcpy(wps->cred.key, wps->new_psk, wps->new_psk_len);
		wps->cred.key_len = wps->new_psk_len;
	} else if (wps->use_psk_key && wps->wps->psk_set) {
		char hex[65];
		wpa_printf(MSG_DEBUG, "WPS: Use PSK format for Network Key");
		atbmwifi_wpa_snprintf_hex(hex, sizeof(hex), wps->wps->psk, 32);
		atbm_memcpy(wps->cred.key, hex, 32 * 2);
		wps->cred.key_len = 32 * 2;
	} else if (wps->wps->network_key) {
		atbm_memcpy(wps->cred.key, wps->wps->network_key,
			  wps->wps->network_key_len);
		wps->cred.key_len = wps->wps->network_key_len;
		
		wpa_printf(MSG_DEBUG, "WPS: Use cred.key for Network Key %s", wps->cred.key);
	} else if (wps->auth_type & (WPS_AUTH_WPAPSK | WPS_AUTH_WPA2PSK)) {
		char hex[65];
		/* Generate a random per-device PSK */
		atbm_kfree(wps->new_psk);
		wps->new_psk_len = 32;
		wps->new_psk = atbm_kmalloc(wps->new_psk_len, GFP_KERNEL);
		if (wps->new_psk == NULL)
			return -1;
		if (atbmwifi_os_get_random(wps->new_psk, wps->new_psk_len) < 0) {
			atbm_kfree(wps->new_psk);
			wps->new_psk = NULL;
			return -1;
		}
		wpa_hexdump_key(MSG_DEBUG, "WPS: Generated per-device PSK",
				wps->new_psk, wps->new_psk_len);
		atbmwifi_wpa_snprintf_hex(hex, sizeof(hex), wps->new_psk,
				 wps->new_psk_len);
		atbm_memcpy(wps->cred.key, hex, wps->new_psk_len * 2);
		wps->cred.key_len = wps->new_psk_len * 2;
	}

use_provided:
#ifdef CONFIG_WPS_TESTING
	if (wps_testing_dummy_cred)
		cred = wpabuf_alloc(200);
	else
		cred = NULL;
	if (cred) {
		struct wps_credential dummy;
		wpa_printf(MSG_DEBUG, "WPS: Add dummy credential");
		atbm_memset(&dummy, 0, sizeof(dummy));
		atbm_memcpy(dummy.ssid, "dummy", 5);
		dummy.ssid_len = 5;
		dummy.auth_type = WPS_AUTH_WPA2PSK;
		dummy.encr_type = WPS_ENCR_AES;
		atbm_memcpy(dummy.key, "dummy psk", 9);
		dummy.key_len = 9;
		atbm_memcpy(dummy.mac_addr, wps->mac_addr_e, ATBM_ETH_ALEN);
		wps_build_credential(cred, &dummy);
		wpa_hexdump_buf(MSG_DEBUG, "WPS: Dummy Credential", cred);

		wpabuf_put_be16(msg, ATTR_CRED);
		wpabuf_put_be16(msg, wpabuf_len(cred));
		wpabuf_put_buf(msg, cred);

		wpabuf_free(cred);
	}
#endif /* CONFIG_WPS_TESTING */

	cred = wpabuf_alloc(200);
	if (cred == NULL)
		return -1;

	if (wps_build_credential(cred, &wps->cred)) {
		wpabuf_free(cred);
		return -1;
	}

	wpabuf_put_be16(msg, ATTR_CRED);
	wpabuf_put_be16(msg, wpabuf_len(cred));
	wpabuf_put_buf(msg, cred);
	wpabuf_free(cred);

skip_cred_build:
	if (wps->wps->registrar->extra_cred) {
		wpa_printf(MSG_DEBUG, "WPS:  * Credential (pre-configured)");
		wpabuf_put_buf(msg, wps->wps->registrar->extra_cred);
	}

	return 0;
}


 static int wps_build_ap_settings(struct wps_data *wps, struct wpabuf *msg)
{
	wpa_printf(MSG_DEBUG, "WPS:  * AP Settings");

	if (wps_build_credential(msg, &wps->cred))
		return -1;

	return 0;
}


 static struct wpabuf * wps_build_ap_cred(struct wps_data *wps)
{
	struct wpabuf *msg, *plain;

	msg = wpabuf_alloc(1000);
	if (msg == NULL)
		return NULL;

	plain = wpabuf_alloc(200);
	if (plain == NULL) {
		wpabuf_free(msg);
		return NULL;
	}

	if (wps_build_ap_settings(wps, plain)) {
		wpabuf_free(plain);
		wpabuf_free(msg);
		return NULL;
	}

	wpabuf_put_be16(msg, ATTR_CRED);
	wpabuf_put_be16(msg, wpabuf_len(plain));
	wpabuf_put_buf(msg, plain);
	wpabuf_free(plain);

	return msg;
}


 static struct wpabuf * wps_build_m2(struct wps_data *wps)
{
	struct wpabuf *msg;

	if (atbmwifi_os_get_random(wps->nonce_r, WPS_NONCE_LEN) < 0)
		return NULL;
	//wpa_hexdump(MSG_DEBUG, "WPS: Registrar Nonce",
	//	    wps->nonce_r, WPS_NONCE_LEN);
	//wpa_hexdump(MSG_DEBUG, "WPS: UUID-R", wps->uuid_r, WPS_UUID_LEN);

	wpa_printf(MSG_ALWAYS, "WPS: Building Message M2");
	msg = wpabuf_alloc(1000);
	if (msg == NULL)
		return NULL;

	if (wps_build_version(msg) ||
	    wps_build_msg_type(msg, WPS_M2) ||
	    wps_build_enrollee_nonce(wps, msg) ||
	    wps_build_registrar_nonce(wps, msg) ||
	    wps_build_uuid_r(wps, msg) ||
	    wps_build_public_key(wps, msg) ||
	    wps_derive_keys(wps) ||
	    wps_build_auth_type_flags(wps, msg) ||
	    wps_build_encr_type_flags(wps, msg) ||
	    wps_build_conn_type_flags(wps, msg) ||
	    wps_build_config_methods_r(wps->wps->registrar, msg) ||
	    wps_build_device_attrs(&wps->wps->dev, msg) ||
	    wps_build_rf_bands(&wps->wps->dev, msg) ||
	    wps_build_assoc_state(wps, msg) ||
	    wps_build_config_error(msg, WPS_CFG_NO_ERROR) ||
	    wps_build_dev_password_id(msg, wps->dev_pw_id) ||
	    wps_build_os_version(&wps->wps->dev, msg) ||
	    wps_build_wfa_ext(msg, 0, NULL, 0) ||
	    wps_build_authenticator(wps, msg)) {
		wpabuf_free(msg);
		return NULL;
	}

	//wps->int_reg = 1;
	wps->state = RECV_M3;
	return msg;
}


 static struct wpabuf * wps_build_m2d(struct wps_data *wps)
{
	struct wpabuf *msg;
	atbm_uint16 err = wps->config_error;

	wpa_printf(MSG_ALWAYS, "WPS: Building Message M2D");
	msg = wpabuf_alloc(1000);
	if (msg == NULL)
		return NULL;

	if (wps->wps->ap && wps->wps->ap_setup_locked &&
	    err == WPS_CFG_NO_ERROR)
		err = WPS_CFG_SETUP_LOCKED;

	if (wps_build_version(msg) ||
	    wps_build_msg_type(msg, WPS_M2D) ||
	    wps_build_enrollee_nonce(wps, msg) ||
	    wps_build_registrar_nonce(wps, msg) ||
	    wps_build_uuid_r(wps, msg) ||
	    wps_build_auth_type_flags(wps, msg) ||
	    wps_build_encr_type_flags(wps, msg) ||
	    wps_build_conn_type_flags(wps, msg) ||
	    wps_build_config_methods_r(wps->wps->registrar, msg) ||
	    wps_build_device_attrs(&wps->wps->dev, msg) ||
	    wps_build_rf_bands(&wps->wps->dev, msg) ||
	    wps_build_assoc_state(wps, msg) ||
	    wps_build_config_error(msg, err) ||
	    wps_build_os_version(&wps->wps->dev, msg) ||
	    wps_build_wfa_ext(msg, 0, NULL, 0)) {
		wpabuf_free(msg);
		return NULL;
	}

	wps->state = RECV_M2D_ACK;
	return msg;
}


 static struct wpabuf * wps_build_m4(struct wps_data *wps)
{
	struct wpabuf *msg, *plain;

	wpa_printf(MSG_ALWAYS, "WPS: Building Message M4");

	wps_derive_psk(wps, wps->dev_password, wps->dev_password_len);

	plain = wpabuf_alloc(200);
	if (plain == NULL)
		return NULL;

	msg = wpabuf_alloc(1000);
	if (msg == NULL) {
		wpabuf_free(plain);
		return NULL;
	}

	if (wps_build_version(msg) ||
	    wps_build_msg_type(msg, WPS_M4) ||
	    wps_build_enrollee_nonce(wps, msg) ||
	    wps_build_r_hash(wps, msg) ||
	    wps_build_r_snonce1(wps, plain) ||
	    wps_build_key_wrap_auth(wps, plain) ||
	    wps_build_encr_settings(wps, msg, plain) ||
	    wps_build_wfa_ext(msg, 0, NULL, 0) ||
	    wps_build_authenticator(wps, msg)) {
	    
		wpa_printf(MSG_ALWAYS, "WPS: Building M4 ERROR");
		wpabuf_free(plain);
		wpabuf_free(msg);
		return NULL;
	}
	wpabuf_free(plain);

	wps->state = RECV_M5;
	return msg;
}


 static struct wpabuf * wps_build_m6(struct wps_data *wps)
{
	struct wpabuf *msg, *plain;

	wpa_printf(MSG_ALWAYS, "WPS: Building Message M6");

	plain = wpabuf_alloc(200);
	if (plain == NULL)
		return NULL;

	msg = wpabuf_alloc(1000);
	if (msg == NULL) {
		wpabuf_free(plain);
		return NULL;
	}

	if (wps_build_version(msg) ||
	    wps_build_msg_type(msg, WPS_M6) ||
	    wps_build_enrollee_nonce(wps, msg) ||
	    wps_build_r_snonce2(wps, plain) ||
	    wps_build_key_wrap_auth(wps, plain) ||
	    wps_build_encr_settings(wps, msg, plain) ||
	    wps_build_wfa_ext(msg, 0, NULL, 0) ||
	    wps_build_authenticator(wps, msg)) {
		wpabuf_free(plain);
		wpabuf_free(msg);
		return NULL;
	}
	wpabuf_free(plain);

	wps->wps_pin_revealed = 1;
	wps->state = RECV_M7;
	return msg;
}


 static struct wpabuf * wps_build_m8(struct wps_data *wps)
{
	struct wpabuf *msg, *plain;

	wpa_printf(MSG_ALWAYS, "WPS: Building Message M8");

	plain = wpabuf_alloc(500);
	if (plain == NULL)
		return NULL;

	msg = wpabuf_alloc(1000);
	if (msg == NULL) {
		wpabuf_free(plain);
		return NULL;
	}

	if (wps_build_version(msg) ||
	    wps_build_msg_type(msg, WPS_M8) ||
	    wps_build_enrollee_nonce(wps, msg) ||
	    ((wps->wps->ap || wps->er) && wps_build_cred(wps, plain)) ||
	    (!wps->wps->ap && !wps->er && wps_build_ap_settings(wps, plain)) ||
	    wps_build_key_wrap_auth(wps, plain) ||
	    wps_build_encr_settings(wps, msg, plain) ||
	    wps_build_wfa_ext(msg, 0, NULL, 0) ||
	    wps_build_authenticator(wps, msg)) {
		wpabuf_free(plain);
		wpabuf_free(msg);
		return NULL;
	}
	wpabuf_free(plain);

//	wps->state = RECV_DONE;
	return msg;
}


 struct wpabuf * wps_registrar_get_msg(struct wps_data *wps,
				      enum wsc_op_code *op_code)
{
	struct wpabuf *msg;

	switch (wps->state) {
	case SEND_M2:
		msg = wps_build_m2(wps);
		*op_code = WSC_MSG;
		break;
	case SEND_M2D:
		msg = wps_build_m2d(wps);
		*op_code = WSC_MSG;
		break;
	case SEND_M4:
		msg = wps_build_m4(wps);
		*op_code = WSC_MSG;
		break;
	case SEND_M6:
		msg = wps_build_m6(wps);
		*op_code = WSC_MSG;
		break;
	case SEND_M8:
		msg = wps_build_m8(wps);
		*op_code = WSC_MSG;
		break;
	case RECV_DONE:
		msg = wps_build_wsc_ack(wps);
		*op_code = WSC_ACK;
		break;
	case SEND_WSC_NACK:
		msg = wps_build_wsc_nack(wps);
		*op_code = WSC_NACK;
		break;
	default:
		wpa_printf(MSG_ERROR, "WPS: Unsupported state %d for building "
			   "a message", wps->state);
		msg = NULL;
		break;
	}

	if (*op_code == WSC_MSG && msg) {
		/* Save a copy of the last message for Authenticator derivation
		 */
		wpabuf_free(wps->last_msg);
		wps->last_msg = wpabuf_dup(msg);
	}

	return msg;
}


 static int wps_process_enrollee_nonce(struct wps_data *wps, const atbm_uint8 *e_nonce)
{
	if (e_nonce == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Enrollee Nonce received");
		return -1;
	}

	atbm_memcpy(wps->nonce_e, e_nonce, WPS_NONCE_LEN);
	//wpa_printf(MSG_DEBUG, "WPS: Enrollee Nonce");
		    //wps->nonce_e, WPS_NONCE_LEN);

	return 0;
}


 static int wps_process_registrar_nonce(struct wps_data *wps, const atbm_uint8 *r_nonce)
{
	if (r_nonce == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Registrar Nonce received");
		return -1;
	}

	if (atbm_memcmp(wps->nonce_r, r_nonce, WPS_NONCE_LEN) != 0) {
		wpa_printf(MSG_DEBUG, "WPS: Invalid Registrar Nonce received");
		return -1;
	}

	return 0;
}


 static int wps_process_uuid_e(struct wps_data *wps, const atbm_uint8 *uuid_e)
{
	if (uuid_e == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No UUID-E received");
		return -1;
	}

	atbm_memcpy(wps->uuid_e, uuid_e, WPS_UUID_LEN);
	wpa_printf(MSG_DEBUG, "WPS: uuid_e");
	//wpa_hexdump(MSG_DEBUG, "WPS: UUID-E", wps->uuid_e, WPS_UUID_LEN);

	return 0;
}


 static int wps_process_dev_password_id(struct wps_data *wps, const atbm_uint8 *pw_id)
{
	if (pw_id == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Device Password ID received");
		return -1;
	}

	wps->dev_pw_id = ATBM_WPA_GET_BE16(pw_id);
	//wpa_printf(MSG_DEBUG, "WPS: Device Password ID %d", wps->dev_pw_id);

	return 0;
}


 static int wps_process_e_hash1(struct wps_data *wps, const atbm_uint8 *e_hash1)
{
	if (e_hash1 == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No E-Hash1 received");
		return -1;
	}

	atbm_memcpy(wps->peer_hash1, e_hash1, WPS_HASH_LEN);
	wpa_hexdump(MSG_DEBUG, "WPS: E-Hash1", wps->peer_hash1, WPS_HASH_LEN);

	return 0;
}


 static int wps_process_e_hash2(struct wps_data *wps, const atbm_uint8 *e_hash2)
{
	if (e_hash2 == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No E-Hash2 received");
		return -1;
	}

	atbm_memcpy(wps->peer_hash2, e_hash2, WPS_HASH_LEN);
	wpa_hexdump(MSG_DEBUG, "WPS: E-Hash2", wps->peer_hash2, WPS_HASH_LEN);

	return 0;
}


 static int wps_process_e_snonce1(struct wps_data *wps, const atbm_uint8 *e_snonce1)
{
	atbm_uint8 hash[SHA256_MAC_LEN];
	const atbm_uint8 *addr[4];
	atbm_size_t len[4];

	if (e_snonce1 == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No E-SNonce1 received");
		return -1;
	}

	wpa_hexdump_key(MSG_DEBUG, "WPS: E-SNonce1", e_snonce1,
			WPS_SECRET_NONCE_LEN);

	/* E-Hash1 = HMAC_AuthKey(E-S1 || PSK1 || PK_E || PK_R) */
	addr[0] = e_snonce1;
	len[0] = WPS_SECRET_NONCE_LEN;
	addr[1] = wps->psk1;
	len[1] = WPS_PSK_LEN;
	addr[2] = wpabuf_head(wps->dh_pubkey_e);
	len[2] = wpabuf_len(wps->dh_pubkey_e);
	addr[3] = wpabuf_head(wps->dh_pubkey_r);
	len[3] = wpabuf_len(wps->dh_pubkey_r);
	atbmwifi_hmac_sha256_vector(wps->authkey, WPS_AUTHKEY_LEN, 4, addr, len, hash);

	if (atbm_memcmp(wps->peer_hash1, hash, WPS_HASH_LEN) != 0) {
		wpa_printf(MSG_ERROR, "WPS: E-Hash1 derived from E-S1 does "
			   "not match with the pre-committed value");
		wps->config_error = WPS_CFG_DEV_PASSWORD_AUTH_FAILURE;
		//wps_pwd_auth_fail_event(wps->wps, 0, 1);
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPS: Enrollee proved knowledge of the first "
		   "half of the device password");

	return 0;
}


 static int wps_process_e_snonce2(struct wps_data *wps, const atbm_uint8 *e_snonce2)
{
	atbm_uint8 hash[SHA256_MAC_LEN];
	const atbm_uint8 *addr[4];
	atbm_size_t len[4];

	if (e_snonce2 == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No E-SNonce2 received");
		return -1;
	}

	wpa_hexdump_key(MSG_DEBUG, "WPS: E-SNonce2", e_snonce2,
			WPS_SECRET_NONCE_LEN);

	/* E-Hash2 = HMAC_AuthKey(E-S2 || PSK2 || PK_E || PK_R) */
	addr[0] = e_snonce2;
	len[0] = WPS_SECRET_NONCE_LEN;
	addr[1] = wps->psk2;
	len[1] = WPS_PSK_LEN;
	addr[2] = wpabuf_head(wps->dh_pubkey_e);
	len[2] = wpabuf_len(wps->dh_pubkey_e);
	addr[3] = wpabuf_head(wps->dh_pubkey_r);
	len[3] = wpabuf_len(wps->dh_pubkey_r);
	atbmwifi_hmac_sha256_vector(wps->authkey, WPS_AUTHKEY_LEN, 4, addr, len, hash);

	if (atbm_memcmp(wps->peer_hash2, hash, WPS_HASH_LEN) != 0) {
		wpa_printf(MSG_ERROR, "WPS: E-Hash2 derived from E-S2 does "
			   "not match with the pre-committed value");
		//wps_registrar_invalidate_pin(wps->wps->registrar, wps->uuid_e);
		wps->config_error = WPS_CFG_DEV_PASSWORD_AUTH_FAILURE;
		//wps_pwd_auth_fail_event(wps->wps, 0, 2);
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPS: Enrollee proved knowledge of the second "
		   "half of the device password");
	wps->wps_pin_revealed = 0;
	//wps_registrar_unlock_pin(wps->wps->registrar, wps->uuid_e);

	/*
	 * In case wildcard PIN is used and WPS handshake succeeds in the first
	 * attempt, wps_registrar_unlock_pin() would not free the PIN, so make
	 * sure the PIN gets invalidated here.
	 */
	//wps_registrar_invalidate_pin(wps->wps->registrar, wps->uuid_e);

	return 0;
}


 static int wps_process_mac_addr(struct wps_data *wps, const atbm_uint8 *mac_addr)
{
	if (mac_addr == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No MAC Address received");
		return -1;
	}

	//wpa_printf(MSG_DEBUG, "WPS: Enrollee MAC Address ");// MACSTR,
		  // MAC2STR(mac_addr));
	atbm_memcpy(wps->mac_addr_e, mac_addr, ATBM_ETH_ALEN);
	atbm_memcpy(wps->peer_dev.mac_addr, mac_addr, ATBM_ETH_ALEN);

	return 0;
}


 static int wps_process_pubkey(struct wps_data *wps, const atbm_uint8 *pk,
			      atbm_size_t pk_len)
{
	if (pk == NULL || pk_len == 0) {
		wpa_printf(MSG_ERROR, "WPS: No Public Key received");
		return -1;
	}

#ifdef CONFIG_WPS_OOB
	if (wps->wps->oob_conf.pubkey_hash != NULL) {
		const atbm_uint8 *addr[1];
		atbm_uint8 hash[WPS_HASH_LEN];

		addr[0] = pk;
		atbmwifi_sha256_vector(1, addr, &pk_len, hash);
		if (atbm_memcmp(hash,
			      wpabuf_head(wps->wps->oob_conf.pubkey_hash),
			      WPS_OOB_PUBKEY_HASH_LEN) != 0) {
			wpa_printf(MSG_ERROR, "WPS: Public Key hash error");
			return -1;
		}
	}
#endif /* CONFIG_WPS_OOB */

	wpabuf_free(wps->dh_pubkey_e);
	wps->dh_pubkey_e = wpabuf_alloc_copy(pk, pk_len);
	if (wps->dh_pubkey_e == NULL)
		return -1;

	return 0;
}


 static int wps_process_auth_type_flags(struct wps_data *wps, const atbm_uint8 *auth)
{
	atbm_uint16 auth_types;

	if (auth == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Authentication Type flags "
			   "received");
		return -1;
	}

	auth_types = ATBM_WPA_GET_BE16(auth);

	//wpa_printf(MSG_DEBUG, "WPS: Enrollee Authentication Type flags 0x%x",
	//	   auth_types);
	wps->auth_type = wps->wps->auth_types & auth_types;
	if (wps->auth_type == 0) {
		wpa_printf(MSG_ERROR, "WPS: No match in supported "
			   "authentication types (own 0x%x Enrollee 0x%x)",
			   wps->wps->auth_types, auth_types);
#ifdef WPS_WORKAROUNDS
		/*
		 * Some deployed implementations seem to advertise incorrect
		 * information in this attribute. For example, Linksys WRT350N
		 * seems to have a byteorder bug that breaks this negotiation.
		 * In order to interoperate with existing implementations,
		 * assume that the Enrollee supports everything we do.
		 */
		wpa_printf(MSG_ERROR, "WPS: Workaround - assume Enrollee "
			   "does not advertise supported authentication types "
			   "correctly");
		wps->auth_type = wps->wps->auth_types;
#else /* WPS_WORKAROUNDS */
		return -1;
#endif /* WPS_WORKAROUNDS */
	}

	return 0;
}


 static int wps_process_encr_type_flags(struct wps_data *wps, const atbm_uint8 *encr)
{
	atbm_uint16 encr_types;

	if (encr == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Encryption Type flags "
			   "received");
		return -1;
	}

	encr_types = ATBM_WPA_GET_BE16(encr);

	//wpa_printf(MSG_DEBUG, "WPS: Enrollee Encryption Type flags 0x%x",
	//	   encr_types);
	wps->encr_type = wps->wps->encr_types & encr_types;
	if (wps->encr_type == 0) {
		wpa_printf(MSG_ERROR, "WPS: No match in supported "
			   "encryption types (own 0x%x Enrollee 0x%x)",
			   wps->wps->encr_types, encr_types);
#ifdef WPS_WORKAROUNDS
		/*
		 * Some deployed implementations seem to advertise incorrect
		 * information in this attribute. For example, Linksys WRT350N
		 * seems to have a byteorder bug that breaks this negotiation.
		 * In order to interoperate with existing implementations,
		 * assume that the Enrollee supports everything we do.
		 */
		wpa_printf(MSG_ERROR, "WPS: Workaround - assume Enrollee "
			   "does not advertise supported encryption types "
			   "correctly");
		wps->encr_type = wps->wps->encr_types;
#else /* WPS_WORKAROUNDS */
		return -1;
#endif /* WPS_WORKAROUNDS */
	}

	return 0;
}


 static int wps_process_conn_type_flags(struct wps_data *wps, const atbm_uint8 *conn)
{
	if (conn == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Connection Type flags "
			   "received");
		return -1;
	}

	//wpa_printf(MSG_DEBUG, "WPS: Enrollee Connection Type flags 0x%x",
	//	   *conn);

	return 0;
}


 static int wps_process_config_methods(struct wps_data *wps, const atbm_uint8 *methods)
{
	atbm_uint16 m;

	if (methods == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Config Methods received");
		return -1;
	}

	m = ATBM_WPA_GET_BE16(methods);

	/*wpa_printf(MSG_DEBUG, "WPS: Enrollee Config Methods 0x%x"
		   "%s%s%s%s%s%s%s%s%s", m,
		   m & WPS_CONFIG_USBA ? " [USBA]" : "",
		   m & WPS_CONFIG_ETHERNET ? " [Ethernet]" : "",
		   m & WPS_CONFIG_LABEL ? " [Label]" : "",
		   m & WPS_CONFIG_DISPLAY ? " [Display]" : "",
		   m & WPS_CONFIG_EXT_NFC_TOKEN ? " [Ext NFC Token]" : "",
		   m & WPS_CONFIG_INT_NFC_TOKEN ? " [Int NFC Token]" : "",
		   m & WPS_CONFIG_NFC_INTERFACE ? " [NFC]" : "",
		   m & WPS_CONFIG_PUSHBUTTON ? " [PBC]" : "",
		   m & WPS_CONFIG_KEYPAD ? " [Keypad]" : "");
*/
	if (!(m & WPS_CONFIG_DISPLAY) && !wps->use_psk_key) {
		/*
		 * The Enrollee does not have a display so it is unlikely to be
		 * able to show the passphrase to a user and as such, could
		 * benefit from receiving PSK to reduce key derivation time.
		 */
		wpa_printf(MSG_ERROR, "WPS: Prefer PSK format key due to "
			   "Enrollee not supporting display");
		wps->use_psk_key = 1;
	}

	return 0;
}


 static int wps_process_wps_state(struct wps_data *wps, const atbm_uint8 *state)
{
	if (state == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Wi-Fi Protected Setup State "
			   "received");
		return -1;
	}

	//wpa_printf(MSG_DEBUG, "WPS: Enrollee Wi-Fi Protected Setup State %d",
	//	   *state);

	return 0;
}


 static int wps_process_assoc_state(struct wps_data *wps, const atbm_uint8 *assoc)
{
	//atbm_uint16 a;

	if (assoc == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Association State received");
		return -1;
	}

	//a = ATBM_WPA_GET_BE16(assoc);
	//wpa_printf(MSG_DEBUG, "WPS: Enrollee Association State %d", a);

	return 0;
}


 static int wps_process_config_error(struct wps_data *wps, const atbm_uint8 *err)
{
	//atbm_uint16 e;

	if (err == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Configuration Error received");
		return -1;
	}

	//e = WPA_GET_BE16(err);
	//wpa_printf(MSG_DEBUG, "WPS: Enrollee Configuration Error %d", e);

	return 0;
}

#if 0
 static int wps_registrar_p2p_dev_addr_match(struct wps_data *wps)
{
#if CONFIG_P2P
	struct wps_registrar *reg = wps->wps->registrar;

	if (atbm_is_zero_ether_addr(reg->p2p_dev_addr))
		return 1; /* no filtering in use */

	if (atbm_memcmp(reg->p2p_dev_addr, wps->p2p_dev_addr, ATBM_ETH_ALEN) != 0) {
		wpa_printf(MSG_ERROR, "WPS: No match on P2P Device Address "
			   "filtering for PBC: expected " MACSTR " was "
			   MACSTR " - indicate PBC session overlap",
			   MAC2STR(reg->p2p_dev_addr),
			   MAC2STR(wps->p2p_dev_addr));
		return 0;
	}
#endif /* CONFIG_P2P */
	return 1;
}
#endif

 static int wps_registrar_skip_overlap(struct wps_data *wps)
{
#if CONFIG_P2P
	struct wps_registrar *reg = wps->wps->registrar;

	if (atbm_is_zero_ether_addr(reg->p2p_dev_addr))
		return 0; /* no specific Enrollee selected */

	if (atbm_memcmp(reg->p2p_dev_addr, wps->p2p_dev_addr, ATBM_ETH_ALEN) == 0) {
		wpa_printf(MSG_ERROR, "WPS: Skip PBC overlap due to selected "
			   "Enrollee match");
		return 1;
	}
#endif /* CONFIG_P2P */
	return 0;
}


 static enum wps_process_res wps_process_m1(struct wps_data *wps,
					   struct wps_parse_attr *attr)
{
	wpa_printf(MSG_ALWAYS, "WPS: Received M1");

	if (wps->state != RECV_M1) {
		wpa_printf(MSG_ERROR, "WPS: Unexpected state (%d) for "
			   "receiving M1\n", wps->state);
		return WPS_FAILURE;
	}

	if (wps_process_uuid_e(wps, attr->uuid_e) ||
	    wps_process_mac_addr(wps, attr->mac_addr) ||
	    wps_process_enrollee_nonce(wps, attr->enrollee_nonce) ||
	    wps_process_pubkey(wps, attr->public_key, attr->public_key_len) ||
	    wps_process_auth_type_flags(wps, attr->auth_type_flags) ||
	    wps_process_encr_type_flags(wps, attr->encr_type_flags) ||
	    wps_process_conn_type_flags(wps, attr->conn_type_flags) ||
	    wps_process_config_methods(wps, attr->config_methods) ||
	    wps_process_wps_state(wps, attr->wps_state) ||
	    wps_process_device_attrs(&wps->peer_dev, attr) ||
	    wps_process_rf_bands(&wps->peer_dev, attr->rf_bands) ||
	    wps_process_assoc_state(wps, attr->assoc_state) ||
	    wps_process_dev_password_id(wps, attr->dev_password_id) ||
	    wps_process_config_error(wps, attr->config_error) ||
	    wps_process_os_version(&wps->peer_dev, attr->os_version))
		return WPS_FAILURE;

	if (wps->dev_pw_id < 0x10 &&
	    wps->dev_pw_id != DEV_PW_DEFAULT &&
	    wps->dev_pw_id != DEV_PW_USER_SPECIFIED &&
	    wps->dev_pw_id != DEV_PW_MACHINE_SPECIFIED &&
	    wps->dev_pw_id != DEV_PW_REGISTRAR_SPECIFIED &&
	    (wps->dev_pw_id != DEV_PW_PUSHBUTTON ||
	     !wps->wps->registrar->pbc)) {
		wpa_printf(MSG_ERROR, "WPS: Unsupported Device Password ID %d",
			   wps->dev_pw_id);
		wps->state = SEND_M2D;
		return WPS_CONTINUE;
	}

	/*if (wps->dev_pw_id == DEV_PW_PUSHBUTTON) {
		if ((wps->wps->registrar->force_pbc_overlap ||
		     wps_registrar_pbc_overlap(wps->wps->registrar,
					       wps->mac_addr_e, wps->uuid_e) ||
		     !wps_registrar_p2p_dev_addr_match(wps)) &&
		    !wps_registrar_skip_overlap(wps)) {
			wpa_printf(MSG_DEBUG, "WPS: PBC overlap - deny PBC "
				   "negotiation");
			wps->state = SEND_M2D;
			wps->config_error = WPS_CFG_MULTIPLE_PBC_DETECTED;
			wps_pbc_overlap_event(wps->wps);
			wps_fail_event(wps->wps, WPS_M1,
				       WPS_CFG_MULTIPLE_PBC_DETECTED,
				       WPS_EI_NO_ERROR);
			wps->wps->registrar->force_pbc_overlap = 1;
			return WPS_CONTINUE;
		}
		wpa_printf(MSG_DEBUG, "WPS: add_pbc_session\n");
		wps_registrar_add_pbc_session(wps->wps->registrar,
					      wps->mac_addr_e, wps->uuid_e);
		wps->pbc = 1;
	}*/

#if 0//def WPS_WORKAROUNDS
	/*
	 * It looks like Mac OS X 10.6.3 and 10.6.4 do not like Network Key in
	 * passphrase format. To avoid interop issues, force PSK format to be
	 * used.
	 */
	if (!wps->use_psk_key &&
	    wps->peer_dev.manufacturer &&
	    strncmp(wps->peer_dev.manufacturer, "Apple ", 6) == 0 &&
	    wps->peer_dev.model_name &&
	    strcmp(wps->peer_dev.model_name, "AirPort") == 0) {
		wpa_printf(MSG_DEBUG, "WPS: Workaround - Force Network Key in "
			   "PSK format");
		wps->use_psk_key = 1;
	}
#endif /* WPS_WORKAROUNDS */
	wpa_printf(MSG_DEBUG, "WPS: SEND_M2");

	wps->state = SEND_M2;
	return WPS_CONTINUE;
}


 static enum wps_process_res wps_process_m3(struct wps_data *wps,
					   const struct wpabuf *msg,
					   struct wps_parse_attr *attr)
{
	wpa_printf(MSG_ALWAYS, "WPS: Received M3");

	if (wps->state != RECV_M3) {
		wpa_printf(MSG_ERROR, "WPS: Unexpected state (%d) for "
			   "receiving M3", wps->state);
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	if (wps->pbc && wps->wps->registrar->force_pbc_overlap &&
	    !wps_registrar_skip_overlap(wps)) {
		wpa_printf(MSG_ERROR, "WPS: Reject negotiation due to PBC "
			   "session overlap");
		wps->state = SEND_WSC_NACK;
		wps->config_error = WPS_CFG_MULTIPLE_PBC_DETECTED;
		return WPS_CONTINUE;
	}

	if (wps_process_registrar_nonce(wps, attr->registrar_nonce) ||
	    wps_process_authenticator(wps, attr->authenticator, msg) ||
	    wps_process_e_hash1(wps, attr->e_hash1) ||
	    wps_process_e_hash2(wps, attr->e_hash2)) {
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	wps->state = SEND_M4;
	return WPS_CONTINUE;
}


 static enum wps_process_res wps_process_m5(struct wps_data *wps,
					   const struct wpabuf *msg,
					   struct wps_parse_attr *attr)
{
	struct wpabuf *decrypted;
	struct wps_parse_attr *eattr;

	wpa_printf(MSG_ALWAYS, "WPS: Received M5");

	if (wps->state != RECV_M5) {
		wpa_printf(MSG_ERROR, "WPS: Unexpected state (%d) for "
			   "receiving M5", wps->state);
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	if (wps->pbc && wps->wps->registrar->force_pbc_overlap &&
	    !wps_registrar_skip_overlap(wps)) {
		wpa_printf(MSG_ERROR, "WPS: Reject negotiation due to PBC "
			   "session overlap");
		wps->state = SEND_WSC_NACK;
		wps->config_error = WPS_CFG_MULTIPLE_PBC_DETECTED;
		return WPS_CONTINUE;
	}

	if (wps_process_registrar_nonce(wps, attr->registrar_nonce) ||
	    wps_process_authenticator(wps, attr->authenticator, msg)) {
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	decrypted = wps_decrypt_encr_settings(wps, attr->encr_settings,
					      attr->encr_settings_len);
	if (decrypted == NULL) {
		wpa_printf(MSG_ERROR, "WPS: Failed to decrypted Encrypted "
			   "Settings attribute");
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	if (wps_validate_m5_encr(decrypted, attr->version2 != NULL) < 0) {
		wpabuf_free(decrypted);
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	wpa_printf(MSG_DEBUG, "WPS: Processing decrypted Encrypted Settings "
		   "attribute");
	
	eattr = atbm_kmalloc(sizeof(*eattr), GFP_KERNEL);
	if (eattr == NULL)
	{
		wpa_printf(MSG_ERROR, "WPS: msg M5 malloc failed");		
		wps->state = SEND_WSC_NACK;
		return WPS_FAILURE;
	}
	if (wps_parse_msg(decrypted, eattr) < 0 ||
	    wps_process_key_wrap_auth(wps, decrypted, eattr->key_wrap_auth) ||
	    wps_process_e_snonce1(wps, eattr->e_snonce1)) {
		wpabuf_free(decrypted);
		wps->state = SEND_WSC_NACK;
		atbm_kfree(eattr);
		return WPS_CONTINUE;
	}
	wpabuf_free(decrypted);
	atbm_kfree(eattr);

	wps->state = SEND_M6;
	return WPS_CONTINUE;
}


 static atbm_void wps_sta_cred_cb(struct wps_data *wps)
{
	/*
	 * Update credential to only include a single authentication and
	 * encryption type in case the AP configuration includes more than one
	 * option.
	 */
	if (wps->cred.auth_type & WPS_AUTH_WPA2PSK)
		wps->cred.auth_type = WPS_AUTH_WPA2PSK;
	else if (wps->cred.auth_type & WPS_AUTH_WPAPSK)
		wps->cred.auth_type = WPS_AUTH_WPAPSK;
	if (wps->cred.encr_type & WPS_ENCR_AES)
		wps->cred.encr_type = WPS_ENCR_AES;
	else if (wps->cred.encr_type & WPS_ENCR_TKIP)
		wps->cred.encr_type = WPS_ENCR_TKIP;
	wpa_printf(MSG_DEBUG, "WPS: Update local configuration based on the "
		   "AP configuration");
	if (wps->wps->cred_cb)
		wps->wps->cred_cb(wps->wps->cb_ctx, &wps->cred);
}


 static atbm_void wps_cred_update(struct wps_credential *dst,
			    struct wps_credential *src)
{
	atbm_memcpy(dst->ssid, src->ssid, sizeof(dst->ssid));
	dst->ssid_len = src->ssid_len;
	dst->auth_type = src->auth_type;
	dst->encr_type = src->encr_type;
	dst->key_idx = src->key_idx;
	atbm_memcpy(dst->key, src->key, sizeof(dst->key));
	dst->key_len = src->key_len;
}


 static int wps_process_ap_settings_r(struct wps_data *wps,
				     struct wps_parse_attr *attr)
{
	struct wpabuf *msg;

	if (wps->wps->ap || wps->er)
		return 0;

	/* AP Settings Attributes in M7 when Enrollee is an AP */
	if (wps_process_ap_settings(attr, &wps->cred) < 0)
		return -1;

	wpa_printf(MSG_INFO, "WPS: Received old AP configuration from AP");

	if (wps->new_ap_settings) {
		wpa_printf(MSG_INFO, "WPS: Update AP configuration based on "
			   "new settings");
		wps_cred_update(&wps->cred, wps->new_ap_settings);
		return 0;
	} else {
		/*
		 * Use the AP PIN only to receive the current AP settings, not
		 * to reconfigure the AP.
		 */

		/*
		 * Clear selected registrar here since we do not get to
		 * WSC_Done in this protocol run.
		 */
		wps_registrar_pin_completed(wps->wps->registrar);

		msg = wps_build_ap_cred(wps);
		if (msg == NULL)
			return -1;
		wps->cred.cred_attr = wpabuf_head(msg);
		wps->cred.cred_attr_len = wpabuf_len(msg);

		if (wps->ap_settings_cb) {
			wps->ap_settings_cb(wps->ap_settings_cb_ctx,
					    &wps->cred);
			wpabuf_free(msg);
			return 1;
		}
		wps_sta_cred_cb(wps);

		wps->cred.cred_attr = NULL;
		wps->cred.cred_attr_len = 0;
		wpabuf_free(msg);

		return 1;
	}
}


 static enum wps_process_res wps_process_m7(struct wps_data *wps,
					   const struct wpabuf *msg,
					   struct wps_parse_attr *attr)
{
	struct wpabuf *decrypted;
	struct wps_parse_attr *eattr;

	wpa_printf(MSG_ALWAYS, "WPS: Received M7");

	if (wps->state != RECV_M7) {
		wpa_printf(MSG_ERROR, "WPS: Unexpected state (%d) for "
			   "receiving M7", wps->state);
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	if (wps->pbc && wps->wps->registrar->force_pbc_overlap &&
	    !wps_registrar_skip_overlap(wps)) {
		wpa_printf(MSG_ERROR, "WPS: Reject negotiation due to PBC "
			   "session overlap");
		wps->state = SEND_WSC_NACK;
		wps->config_error = WPS_CFG_MULTIPLE_PBC_DETECTED;
		return WPS_CONTINUE;
	}

	if (wps_process_registrar_nonce(wps, attr->registrar_nonce) ||
	    wps_process_authenticator(wps, attr->authenticator, msg)) {
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	decrypted = wps_decrypt_encr_settings(wps, attr->encr_settings,
					      attr->encr_settings_len);
	if (decrypted == NULL) {
		wpa_printf(MSG_ERROR, "WPS: Failed to decrypt Encrypted "
			   "Settings attribute");
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	if (wps_validate_m7_encr(decrypted, wps->wps->ap || wps->er,
				 attr->version2 != NULL) < 0) {
		wpabuf_free(decrypted);
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	wpa_printf(MSG_DEBUG, "WPS: Processing decrypted Encrypted Settings "
		   "attribute");
	
	eattr = atbm_kmalloc(sizeof(*eattr), GFP_KERNEL);
	if (eattr == NULL)
	{
		wpa_printf(MSG_ERROR, "WPS: msg M7 malloc failed");		
		wps->state = SEND_WSC_NACK;
		return WPS_FAILURE;
	}
	if (wps_parse_msg(decrypted, eattr) < 0 ||
	    wps_process_key_wrap_auth(wps, decrypted, eattr->key_wrap_auth) ||
	    wps_process_e_snonce2(wps, eattr->e_snonce2) ||
	    wps_process_ap_settings_r(wps, eattr)) {
		wpabuf_free(decrypted);
		wps->state = SEND_WSC_NACK;
		atbm_kfree(eattr);
		return WPS_CONTINUE;
	}

	wpabuf_free(decrypted);
	atbm_kfree(eattr);

	wps->state = SEND_M8;
	return WPS_CONTINUE;
}

 inline static enum wps_process_res wps_process_wsc_msg(struct wps_data *wps,
						const struct wpabuf *msg)
{
	struct wps_parse_attr *attr;
	enum wps_process_res ret = WPS_CONTINUE;

	wpa_printf(MSG_ALWAYS, "WPS: Received WSC_MSG");
	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if (attr == NULL)
	{
		wpa_printf(MSG_ERROR, "WPS: msg malloc failed");
		return WPS_FAILURE;
	}
	atbm_memset(attr, 0, sizeof(*attr));
	
	if (wps_parse_msg(msg, attr) < 0)
	{
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (attr->msg_type == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Message Type attribute");
		wps->state = SEND_WSC_NACK;
		atbm_kfree(attr);
		return WPS_CONTINUE;
	}

	if (*attr->msg_type != WPS_M1 &&
	    (attr->registrar_nonce == NULL ||
	     atbm_memcmp(wps->nonce_r, attr->registrar_nonce,
		       WPS_NONCE_LEN) != 0)) {
		wpa_printf(MSG_ERROR, "WPS: Mismatch in registrar nonce");	
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	switch (*attr->msg_type) {
	case WPS_M1:
		if (wps_validate_m1(msg) < 0)
		{
			atbm_kfree(attr);
			return WPS_FAILURE;
		}
		ret = wps_process_m1(wps, attr);
		break;
	case WPS_M3:
		if (wps_validate_m3(msg) < 0)
		{
			atbm_kfree(attr);
			return WPS_FAILURE;
		}
		ret = wps_process_m3(wps, msg, attr);
		if (ret == WPS_FAILURE || wps->state == SEND_WSC_NACK)
			wps_fail_event(wps->wps, WPS_M3, wps->config_error,
				       wps->error_indication);
		break;
	case WPS_M5:
		if (wps_validate_m5(msg) < 0)
		{
			atbm_kfree(attr);
			return WPS_FAILURE;
		}
		ret = wps_process_m5(wps, msg, attr);
		if (ret == WPS_FAILURE || wps->state == SEND_WSC_NACK)
			wps_fail_event(wps->wps, WPS_M5, wps->config_error,
				       wps->error_indication);
		break;
	case WPS_M7:
		if (wps_validate_m7(msg) < 0)
		{
			atbm_kfree(attr);
			return WPS_FAILURE;
		}
		ret = wps_process_m7(wps, msg, attr);
		if (ret == WPS_FAILURE || wps->state == SEND_WSC_NACK)
			wps_fail_event(wps->wps, WPS_M7, wps->config_error,
				       wps->error_indication);
		break;
	default:
		wpa_printf(MSG_ERROR, "WPS: Unsupported Message Type %d",
			   *attr->msg_type);	
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (ret == WPS_CONTINUE) {
		/* Save a copy of the last message for Authenticator derivation
		 */
		wpabuf_free(wps->last_msg);
		wps->last_msg = wpabuf_dup(msg);
	}
	atbm_kfree(attr);

	return ret;
}


 static enum wps_process_res wps_process_wsc_ack(struct wps_data *wps,
						const struct wpabuf *msg)
{
	struct wps_parse_attr *attr;

	wpa_printf(MSG_ALWAYS, "WPS: Received WSC_ACK");
	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if (attr == NULL)
	{
		wpa_printf(MSG_ERROR, "WPS: msg wsc_done malloc failed");		
		return WPS_FAILURE;
	}

	if (wps_parse_msg(msg, attr) < 0)
	{
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (attr->msg_type == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Message Type attribute");
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (*attr->msg_type != WPS_WSC_ACK) {
		wpa_printf(MSG_ERROR, "WPS: Invalid Message Type %d",
			   *attr->msg_type);
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

#if CONFIG_WPS_UPNP
	if (wps->wps->wps_upnp && wps->ext_reg && wps->state == RECV_M2D_ACK &&
	    upnp_wps_subscribers(wps->wps->wps_upnp)) {
		if (wps->wps->upnp_msgs)
			return WPS_CONTINUE;
		wpa_printf(MSG_ERROR, "WPS: Wait for response from an "
			   "external Registrar");
		atbm_kfree(attr);
		return WPS_PENDING;
	}
#endif /* CONFIG_WPS_UPNP */

	if (attr->registrar_nonce == NULL ||
	    atbm_memcmp(wps->nonce_r, attr->registrar_nonce, WPS_NONCE_LEN) != 0)
	{
		wpa_printf(MSG_ERROR, "WPS: Mismatch in registrar nonce");
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (attr->enrollee_nonce == NULL ||
	    atbm_memcmp(wps->nonce_e, attr->enrollee_nonce, WPS_NONCE_LEN) != 0) {
		wpa_printf(MSG_ERROR, "WPS: Mismatch in enrollee nonce");
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (wps->state == RECV_M2D_ACK) {
#if CONFIG_WPS_UPNP
		if (wps->wps->wps_upnp &&
		    upnp_wps_subscribers(wps->wps->wps_upnp)) {
			if (wps->wps->upnp_msgs)
				return WPS_CONTINUE;
			if (wps->ext_reg == 0)
				wps->ext_reg = 1;
			wpa_printf(MSG_ERROR, "WPS: Wait for response from an "
				   "external Registrar");
			atbm_kfree(attr);
			return WPS_PENDING;
		}
#endif /* CONFIG_WPS_UPNP */

		wpa_printf(MSG_ERROR, "WPS: No more registrars available - "
			   "terminate negotiation");
	}
	atbm_kfree(attr);

	return WPS_FAILURE;
}


 static enum wps_process_res wps_process_wsc_nack(struct wps_data *wps,
						 const struct wpabuf *msg)
{
	struct wps_parse_attr *attr;
	int old_state;
	atbm_uint16 config_error;

	wpa_printf(MSG_DEBUG, "WPS: Received WSC_NACK");
	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if (attr == NULL)
	{
		wpa_printf(MSG_DEBUG, "WPS: msg wsc_done malloc failed");		
		return WPS_FAILURE;
	}

	old_state = wps->state;
	wps->state = SEND_WSC_NACK;

	if (wps_parse_msg(msg, attr) < 0)
	{
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (attr->msg_type == NULL) {
		wpa_printf(MSG_DEBUG, "WPS: No Message Type attribute");
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (*attr->msg_type != WPS_WSC_NACK) {
		wpa_printf(MSG_DEBUG, "WPS: Invalid Message Type %d",
			   *attr->msg_type);
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

#if CONFIG_WPS_UPNP
	if (wps->wps->wps_upnp && wps->ext_reg) {
		wpa_printf(MSG_DEBUG, "WPS: Negotiation using external "
			   "Registrar terminated by the Enrollee");
		atbm_kfree(attr);
		return WPS_FAILURE;
	}
#endif /* CONFIG_WPS_UPNP */

	if (attr->registrar_nonce == NULL ||
	    atbm_memcmp(wps->nonce_r, attr->registrar_nonce, WPS_NONCE_LEN) != 0)
	{
		wpa_printf(MSG_DEBUG, "WPS: Mismatch in registrar nonce");
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (attr->enrollee_nonce == NULL ||
	    atbm_memcmp(wps->nonce_e, attr->enrollee_nonce, WPS_NONCE_LEN) != 0) {
		wpa_printf(MSG_DEBUG, "WPS: Mismatch in enrollee nonce");
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (attr->config_error == NULL) {
		wpa_printf(MSG_DEBUG, "WPS: No Configuration Error attribute "
			   "in WSC_NACK");
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	config_error = ATBM_WPA_GET_BE16(attr->config_error);
	wpa_printf(MSG_DEBUG, "WPS: Enrollee terminated negotiation with "
		   "Configuration Error %d", config_error);

	switch (old_state) {
	case RECV_M3:
		//wps_fail_event(wps->wps, WPS_M2, config_error,
		//	       wps->error_indication);
		break;
	case RECV_M5:
		//wps_fail_event(wps->wps, WPS_M4, config_error,
		//	       wps->error_indication);
		break;
	case RECV_M7:
		//wps_fail_event(wps->wps, WPS_M6, config_error,
		//	       wps->error_indication);
		break;
	case RECV_DONE:
		//wps_fail_event(wps->wps, WPS_M8, config_error,
		//	       wps->error_indication);
		break;
	default:
		break;
	}
	atbm_kfree(attr);

	return WPS_FAILURE;
}


 static enum wps_process_res wps_process_wsc_done(struct wps_data *wps,
						 const struct wpabuf *msg)
{
	struct wps_parse_attr *attr;

	wpa_printf(MSG_ALWAYS, "WPS: Received WSC_Done");
	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if (attr == NULL)
	{
		wpa_printf(MSG_ERROR, "WPS: msg wsc_done malloc failed");		
		return WPS_FAILURE;
	}

	if (wps->state != SEND_M8
#if CONFIG_WPS_UPNP
		&& (!wps->wps->wps_upnp || !wps->ext_reg)
#endif
	) {
		wpa_printf(MSG_ERROR, "WPS: Unexpected state (%d) for "
			   "receiving WSC_Done", wps->state);
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (wps_parse_msg(msg, attr) < 0)
	{
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (attr->msg_type == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Message Type attribute");
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (*attr->msg_type != WPS_WSC_DONE) {
		wpa_printf(MSG_ERROR, "WPS: Invalid Message Type %d",
			   *attr->msg_type);
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (attr->registrar_nonce == NULL ||
	    atbm_memcmp(wps->nonce_r, attr->registrar_nonce, WPS_NONCE_LEN) != 0)
	{
		wpa_printf(MSG_ERROR, "WPS: Mismatch in registrar nonce");
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	if (attr->enrollee_nonce == NULL ||
	    atbm_memcmp(wps->nonce_e, attr->enrollee_nonce, WPS_NONCE_LEN) != 0) {
		wpa_printf(MSG_ERROR, "WPS: Mismatch in enrollee nonce");
		atbm_kfree(attr);
		return WPS_FAILURE;
	}

	wpa_printf(MSG_DEBUG, "WPS: Negotiation completed successfully");
	//wps_device_store(wps->wps->registrar, &wps->peer_dev,
	//		 wps->uuid_e);

	if (wps->wps->wps_state == WPS_STATE_NOT_CONFIGURED && wps->new_psk &&
	    wps->wps->ap && !wps->wps->registrar->disable_auto_conf) {
		struct wps_credential cred;

		wpa_printf(MSG_ERROR, "WPS: Moving to Configured state based "
			   "on first Enrollee connection");

		atbm_memset(&cred, 0, sizeof(cred));
		atbm_memcpy(cred.ssid, wps->wps->ssid, wps->wps->ssid_len);
		cred.ssid_len = wps->wps->ssid_len;
		cred.auth_type = WPS_AUTH_WPAPSK | WPS_AUTH_WPA2PSK;
		cred.encr_type = WPS_ENCR_TKIP | WPS_ENCR_AES;
		atbm_memcpy(cred.key, wps->new_psk, wps->new_psk_len);
		cred.key_len = wps->new_psk_len;

		wps->wps->wps_state = WPS_STATE_CONFIGURED;
		wpa_hexdump_ascii_key(MSG_DEBUG,
				      "WPS: Generated random passphrase",
				      wps->new_psk, wps->new_psk_len);
		if (wps->wps->cred_cb)
			wps->wps->cred_cb(wps->wps->cb_ctx, &cred);

		atbm_kfree(wps->new_psk);
		wps->new_psk = NULL;
	}

	if (!wps->wps->ap && !wps->er)
		wps_sta_cred_cb(wps);

	if (wps->new_psk) {
		//if (wps_cb_new_psk(wps->wps->registrar, wps->mac_addr_e,
		//		   wps->new_psk, wps->new_psk_len)) {
		//	wpa_printf(MSG_DEBUG, "WPS: Failed to configure the "
		//		   "new PSK");
		//}
		atbm_kfree(wps->new_psk);
		wps->new_psk = NULL;
	}

	wps->wps->wpa_success_deauth = 1;
	wps_cb_reg_success(wps->wps->registrar, wps->mac_addr_e, wps->uuid_e,
			   wps->dev_password, wps->dev_password_len);

	if (wps->pbc) {
		wps_registrar_pbc_completed(wps->wps->registrar);
	} else {
		wps_registrar_pin_completed(wps->wps->registrar);
	}
	/* TODO: maintain AuthorizedMACs somewhere separately for each ER and
	 * merge them into APs own list.. */

	wps->state = RECV_DONE;
	//wps_success_event(wps->wps);
	atbm_kfree(attr);

	return WPS_DONE;
}


 enum wps_process_res wps_registrar_process_msg(struct wps_data *wps,
					       enum wsc_op_code op_code,
					       const struct wpabuf *msg)
{
	enum wps_process_res ret;

	//wpa_printf(MSG_DEBUG, "WPS: Processing received message (len=%lu "
	//	   "op_code=%d)\n",
	//	   (unsigned long) wpabuf_len(msg), op_code);

	switch (op_code) {
	case WSC_MSG:
		return wps_process_wsc_msg(wps, msg);
	case WSC_ACK:
		if (wps_validate_wsc_ack(msg) < 0)
			return WPS_FAILURE;
		return wps_process_wsc_ack(wps, msg);
	case WSC_NACK:
		if (wps_validate_wsc_nack(msg) < 0)
			return WPS_FAILURE;
		return wps_process_wsc_nack(wps, msg);
	case WSC_Done:
		if (wps_validate_wsc_done(msg) < 0)
			return WPS_FAILURE;
		ret = wps_process_wsc_done(wps, msg);
		if (ret == WPS_FAILURE) {
			wps->state = SEND_WSC_NACK;
			wps_fail_event(wps->wps, WPS_WSC_DONE,
				       wps->config_error,
				       wps->error_indication);
		}
		return ret;
	default:
		wpa_printf(MSG_DEBUG, "WPS: Unsupported op_code %d", op_code);
		return WPS_FAILURE;
	}
}


 int wps_registrar_update_ie(struct wps_registrar *reg)
{
	return wps_set_ie(reg);
}


 static atbm_void wps_registrar_set_selected_timeout(void *eloop_ctx,
					       atbm_void *timeout_ctx)
{
	struct wps_registrar *reg = eloop_ctx;

	wpa_printf(MSG_DEBUG, "WPS: Selected Registrar timeout - "
		   "unselect internal Registrar");
	reg->selected_registrar = 0;
	reg->pbc = 0;
	wps_registrar_selected_registrar_changed(reg, 0);
}




 static atbm_void wps_registrar_sel_reg_union(struct wps_registrar *reg)
{

}


/**
 * wps_registrar_selected_registrar_changed - SetSelectedRegistrar change
 * @reg: Registrar data from wps_registrar_init()
 *
 * This function is called when selected registrar state changes, e.g., when an
 * AP receives a SetSelectedRegistrar UPnP message.
 */
 atbm_void wps_registrar_selected_registrar_changed(struct wps_registrar *reg, atbm_uint16 dev_pw_id)
{
	wpa_printf(MSG_ALWAYS, "WPS: Selected registrar information changed");

	reg->sel_reg_union = reg->selected_registrar;
	reg->sel_reg_dev_password_id_override = -1;
	reg->sel_reg_config_methods_override = -1;
	//atbm_memcpy(reg->authorized_macs_union, reg->authorized_macs,
	//	  WPS_MAX_AUTHORIZED_MACS * ATBM_ETH_ALEN);
	if (reg->selected_registrar) {
		atbm_uint16 methods;

		methods = reg->wps->config_methods & ~WPS_CONFIG_PUSHBUTTON;
#if CONFIG_WPS2
		methods &= ~(WPS_CONFIG_VIRT_PUSHBUTTON |
			     WPS_CONFIG_PHY_PUSHBUTTON);
#endif /* CONFIG_WPS2 */
		if (reg->pbc) {
			reg->sel_reg_dev_password_id_override =
				DEV_PW_PUSHBUTTON;
			wps_set_pushbutton(&methods, reg->wps->config_methods);
		}
		wpa_printf(MSG_ALWAYS, "WPS: Internal Registrar selected "
			   "(pbc=%d)", reg->pbc);
		reg->sel_reg_config_methods_override = methods;
	} 
	else {
		wpa_printf(MSG_DEBUG, "WPS: Internal Registrar not selected");
	}
	wps_registrar_sel_reg_union(reg);
	wpa_printf(MSG_DEBUG, "WPS: wps_set_ie\n");
	wps_set_ie(reg);
	wps_cb_set_sel_reg(reg);
}

#if 0
 int wps_registrar_get_info(struct wps_registrar *reg, const atbm_uint8 *addr,
			   char *buf, atbm_size_t buflen)
{
	struct wps_registrar_device *d;
	int len = 0, ret;
	char uuid[40];
	char devtype[WPS_DEV_TYPE_BUFSIZE];

	d = wps_device_get(reg, addr);
	if (d == NULL)
		return 0;
	if (uuid_bin2str(d->uuid, uuid, sizeof(uuid)))
		return 0;

	ret = sprintf(buf + len,
			  "wpsUuid=%s\n"
			  "wpsPrimaryDeviceType=%s\n"
			  "wpsDeviceName=%s\n"
			  "wpsManufacturer=%s\n"
			  "wpsModelName=%s\n"
			  "wpsModelNumber=%s\n"
			  "wpsSerialNumber=%s\n",
			  uuid,
			  wps_dev_type_bin2str(d->dev.pri_dev_type, devtype,
					       sizeof(devtype)),
			  d->dev.device_name ? d->dev.device_name : "",
			  d->dev.manufacturer ? d->dev.manufacturer : "",
			  d->dev.model_name ? d->dev.model_name : "",
			  d->dev.model_number ? d->dev.model_number : "",
			  d->dev.serial_number ? d->dev.serial_number : "");
	if (ret < 0 || (atbm_size_t) ret >= buflen - len)
		return len;
	len += ret;

	return len;
}
#endif

 int wps_registrar_config_ap(struct wps_registrar *reg,
			    struct wps_credential *cred)
{
#if CONFIG_WPS2
	wpa_printf(MSG_ALWAYS, "encr_type=0x%x\n", cred->encr_type);
	if (!(cred->encr_type & (WPS_ENCR_NONE | WPS_ENCR_TKIP |
				 WPS_ENCR_AES))) {
		if (cred->encr_type & WPS_ENCR_WEP) {
			wpa_printf(MSG_INFO, "WPS: Reject new AP settings "
				   "due to WEP configuration");
			return -1;
		}

		wpa_printf(MSG_INFO, "WPS: Reject new AP settings due to "
			   "invalid encr_type 0x%x", cred->encr_type);
		return -1;
	}

	if ((cred->encr_type & (WPS_ENCR_TKIP | WPS_ENCR_AES)) ==
	    WPS_ENCR_TKIP) {
		wpa_printf(MSG_DEBUG, "WPS: Upgrade encr_type TKIP -> "
			   "TKIP+AES");
		cred->encr_type |= WPS_ENCR_AES;
	}

	if ((cred->auth_type & (WPS_AUTH_WPAPSK | WPS_AUTH_WPA2PSK)) ==
	    WPS_AUTH_WPAPSK) {
		wpa_printf(MSG_DEBUG, "WPS: Upgrade auth_type WPAPSK -> "
			   "WPAPSK+WPA2PSK");
		cred->auth_type |= WPS_AUTH_WPA2PSK;
	}
#endif /* CONFIG_WPS2 */

	if (reg->wps->cred_cb)
		return reg->wps->cred_cb(reg->wps->cb_ctx, cred);

	return -1;
}



