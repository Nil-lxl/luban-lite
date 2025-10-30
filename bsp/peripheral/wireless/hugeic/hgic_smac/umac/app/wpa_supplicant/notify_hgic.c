/*
 * wpa_supplicant - Event notifications
 * Copyright (c) 2009-2010, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "utils/includes.h"

#include "utils/common.h"
#include "common/wpa_ctrl.h"
#include "config.h"
#include "wpa_supplicant_i.h"
#include "wps_supplicant.h"
#ifndef WPA_SUPPLICANT_RTOS
#include "binder/binder.h"
#include "dbus/dbus_common.h"
#include "dbus/dbus_old.h"
#include "dbus/dbus_new.h"
#endif
#include "rsn_supp/wpa.h"
#include "fst/fst.h"
#include "driver_i.h"
#include "bss.h"
#include "scan.h"
#include "p2p_supplicant.h"
#include "sme.h"
#include "notify.h"
#include <linux/netdevice.h>
#if defined(__MBED__) || defined(__CSKY__)
#include "lib/umac2/umac.h"
#else
#include "hgic_def.h"
#endif

int wpas_notify_supplicant_initialized(struct wpa_global *global)
{
    return 0;
}


void wpas_notify_supplicant_deinitialized(struct wpa_global *global)
{
}


int wpas_notify_iface_added(struct wpa_supplicant *wpa_s)
{
    return 0;
}


void wpas_notify_iface_removed(struct wpa_supplicant *wpa_s)
{
}


void wpas_notify_state_changed(struct wpa_supplicant *wpa_s,
                               enum wpa_states new_state,
                               enum wpa_states old_state)
{
    int  len = 1;
    char state[8];
    struct net_device *dev = net_device_get_by_name(wpa_s->ifname);

    if(dev == NULL) {
        PRINTF("%s:%d:Error,can not find netdevice %s\n",__FUNCTION__,__LINE__,wpa_s->ifname);
        return;
    }

    state[0] = new_state;
    switch (new_state) {
        case WPA_COMPLETED:
            memcpy(state + 1, wpa_s->current_bss->bssid, ETH_ALEN);
            len += ETH_ALEN;
            PRINTF("connect success!\r\n");
            break;
        case WPA_SCANNING:
        case WPA_INTERFACE_DISABLED:
        case WPA_INACTIVE:
            break;
        case WPA_DISCONNECTED:
            if(old_state == WPA_SCANNING) 
                return;
            if(wpa_s->current_bss) {
                memcpy(state + 1, wpa_s->current_bss->bssid, ETH_ALEN);
                len += ETH_ALEN;
                PRINTF("disconnect with "MACSTR"!\r\n",MAC2STR(wpa_s->current_bss->bssid));                
            }
            break;
        default:
            break;
    }
    net_device_event(dev, HGIC_EVENT_STATE_CHG, state, len);
}

void wpas_notify_disconnect_reason(struct wpa_supplicant *wpa_s)
{
    struct net_device *dev = net_device_get_by_name(wpa_s->ifname);
    net_device_event(dev, HGIC_EVENT_DISCONNECT_REASON, (char *)&wpa_s->disconnect_reason, sizeof(int));
}

void wpas_notify_auth_status_code(struct wpa_supplicant *wpa_s)
{
}

void wpas_notify_assoc_status_code(struct wpa_supplicant *wpa_s)
{
    struct net_device *dev = net_device_get_by_name(wpa_s->ifname);
    dev->assoc_state = wpa_s->assoc_status_code;
    net_device_event(dev, HGIC_EVENT_ASSOC_STATUS, (char *)&wpa_s->assoc_status_code, sizeof(int));
}

void wpas_notify_roam_time(struct wpa_supplicant *wpa_s)
{
}


void wpas_notify_roam_complete(struct wpa_supplicant *wpa_s)
{
}


void wpas_notify_session_length(struct wpa_supplicant *wpa_s)
{
}

void wpas_notify_bss_tm_status(struct wpa_supplicant *wpa_s)
{
}

void wpas_notify_network_changed(struct wpa_supplicant *wpa_s)
{
}


void wpas_notify_ap_scan_changed(struct wpa_supplicant *wpa_s)
{
}


void wpas_notify_bssid_changed(struct wpa_supplicant *wpa_s)
{
}


void wpas_notify_auth_changed(struct wpa_supplicant *wpa_s)
{
}


void wpas_notify_network_enabled_changed(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
}


void wpas_notify_network_selected(struct wpa_supplicant *wpa_s,  struct wpa_ssid *ssid)
{
}


void wpas_notify_network_request(struct wpa_supplicant *wpa_s,
                                 struct wpa_ssid *ssid,
                                 enum wpa_ctrl_req_type rtype,
                                 const char *default_txt)
{
}


void wpas_notify_scanning(struct wpa_supplicant *wpa_s)
{
    //struct net_device *dev = net_device_get_by_name(wpa_s->ifname);
    //net_device_event(dev, HGIC_EVENT_SCANNING, 0, 0);

    struct net_device *dev = net_device_get_by_name(hgic_param_ifname(NULL));
    if(!dev) {
        PRINTF("%s:Device %s not found!\n",__FUNCTION__,wpa_s->ifname);
        return;
    }    
    PRINTF("%s,%d:if %s notify:Scanning!\n",__FUNCTION__,__LINE__,wpa_s->ifname);
    net_device_event(dev, HGIC_EVENT_SCANNING, 0, 0);    
}


void wpas_notify_scan_done(struct wpa_supplicant *wpa_s, int success)
{
    //struct net_device *dev = net_device_get_by_name(wpa_s->ifname);
    //net_device_event(dev, HGIC_EVENT_SCAN_DONE, 0, 0);
    
    struct net_device *dev = net_device_get_by_name(hgic_param_ifname(NULL));
    if(!dev) {
        PRINTF("%s:Device %s not found!\n",__FUNCTION__,wpa_s->ifname);
        return;
    }    
    PRINTF("%s,%d:%s notify:Scan done!\n",__FUNCTION__,__LINE__,wpa_s->ifname);
    net_device_event(dev, HGIC_EVENT_SCAN_DONE, 0, 0);
}


void wpas_notify_scan_results(struct wpa_supplicant *wpa_s)
{
}


void wpas_notify_wps_credential(struct wpa_supplicant *wpa_s,
                                const struct wps_credential *cred)
{
}


void wpas_notify_wps_event_m2d(struct wpa_supplicant *wpa_s,
                               struct wps_event_m2d *m2d)
{
}


void wpas_notify_wps_event_fail(struct wpa_supplicant *wpa_s,
                                struct wps_event_fail *fail)
{
}


void wpas_notify_wps_event_success(struct wpa_supplicant *wpa_s)
{
}

void wpas_notify_wps_event_pbc_overlap(struct wpa_supplicant *wpa_s)
{
}


void wpas_notify_network_added(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
}


void wpas_notify_persistent_group_added(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
}


void wpas_notify_persistent_group_removed(struct wpa_supplicant *wpa_s,  struct wpa_ssid *ssid)
{
}


void wpas_notify_network_removed(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
}


void wpas_notify_bss_added(struct wpa_supplicant *wpa_s, u8 bssid[], unsigned int id)
{
}


void wpas_notify_bss_removed(struct wpa_supplicant *wpa_s, u8 bssid[], unsigned int id)
{
}


void wpas_notify_bss_freq_changed(struct wpa_supplicant *wpa_s, unsigned int id)
{
}


void wpas_notify_bss_signal_changed(struct wpa_supplicant *wpa_s, unsigned int id)
{
}


void wpas_notify_bss_privacy_changed(struct wpa_supplicant *wpa_s, unsigned int id)
{
}


void wpas_notify_bss_mode_changed(struct wpa_supplicant *wpa_s, unsigned int id)
{
}


void wpas_notify_bss_wpaie_changed(struct wpa_supplicant *wpa_s, unsigned int id)
{
}


void wpas_notify_bss_rsnie_changed(struct wpa_supplicant *wpa_s, unsigned int id)
{
}


void wpas_notify_bss_wps_changed(struct wpa_supplicant *wpa_s, unsigned int id)
{
}


void wpas_notify_bss_ies_changed(struct wpa_supplicant *wpa_s, unsigned int id)
{
}


void wpas_notify_bss_rates_changed(struct wpa_supplicant *wpa_s, unsigned int id)
{
}


void wpas_notify_bss_seen(struct wpa_supplicant *wpa_s, unsigned int id)
{
}


void wpas_notify_blob_added(struct wpa_supplicant *wpa_s, const char *name)
{
}


void wpas_notify_blob_removed(struct wpa_supplicant *wpa_s, const char *name)
{
}


void wpas_notify_debug_level_changed(struct wpa_global *global)
{
}


void wpas_notify_debug_timestamp_changed(struct wpa_global *global)
{
}


void wpas_notify_debug_show_keys_changed(struct wpa_global *global)
{
}


void wpas_notify_suspend(struct wpa_global *global)
{
}


void wpas_notify_resume(struct wpa_global *global)
{
}


#ifdef CONFIG_P2P
void wpas_notify_p2p_find_stopped(struct wpa_supplicant *wpa_s)
{
    struct net_device *dev = net_device_get_by_name(hgic_param_ifname(NULL));
    if(!dev) {
        PRINTF("%s:Device %s not found!\n",__FUNCTION__,wpa_s->ifname);
        return;
    }
    net_device_event(dev, HGIC_EVENT_P2P_DEV_FIND_STOP, "p2p_dev_find_stop", 0);
}


void wpas_notify_p2p_device_found(struct wpa_supplicant *wpa_s,
                                  const u8 *dev_addr, int new_device)
{
    struct net_device *dev = net_device_get_by_name(hgic_param_ifname(NULL));
    if(!dev) {
        PRINTF("%s:Device %s not found!\n",__FUNCTION__,wpa_s->ifname);
        return;
    }
    net_device_event(dev, HGIC_EVENT_P2P_DEV_FIND, dev_addr, 6);
}


void wpas_notify_p2p_device_lost(struct wpa_supplicant *wpa_s,
                                 const u8 *dev_addr)
{
    struct net_device *dev = net_device_get_by_name(hgic_param_ifname(NULL));
    if(!dev) {
        PRINTF("%s:Device %s not found!\n",__FUNCTION__,wpa_s->ifname);
        return;
    }
    PRINTF("%s,%d:"MACSTR" lost\n",__FUNCTION__,__LINE__,MAC2STR(dev_addr));
    net_device_event(dev, HGIC_EVENT_P2P_DEV_LOST, dev_addr, 6);
}


void wpas_notify_p2p_group_removed(struct wpa_supplicant *wpa_s,
                                   const struct wpa_ssid *ssid,
                                   const char *role)
{
    struct net_device *dev = net_device_get_by_name(hgic_param_ifname(NULL));
    if(!dev) {
        PRINTF("%s:Device %s not found!\n",__FUNCTION__,wpa_s->ifname);
        return;
    }
    PRINTF("%s,%d:"MACSTR" lost\n",__FUNCTION__,__LINE__,MAC2STR(ssid->bssid));
    net_device_event(dev, HGIC_EVENT_P2P_GROUP_REMOVE, ssid->bssid, 6);
}


void wpas_notify_p2p_go_neg_req(struct wpa_supplicant *wpa_s,
                                const u8 *src, u16 dev_passwd_id, u8 go_intent)
{
    struct net_device *dev = net_device_get_by_name(hgic_param_ifname(NULL));
    if(!dev) {
        PRINTF("%s:Device %s not found!\n",__FUNCTION__,wpa_s->ifname);
        return;
    }
    net_device_event(dev, HGIC_EVENT_P2P_GO_NEG_REQ, src, 6);
}


void wpas_notify_p2p_go_neg_completed(struct wpa_supplicant *wpa_s,
                                      struct p2p_go_neg_results *res)
{
}


void wpas_notify_p2p_invitation_result(struct wpa_supplicant *wpa_s,
                                       int status, const u8 *bssid)
{
    struct net_device *dev = net_device_get_by_name(hgic_param_ifname(NULL));
    if(!dev) {
        PRINTF("%s:Device %s not found!\n",__FUNCTION__,wpa_s->ifname);
        return;
    }
    PRINTF("%s,%d:Invitation result:%d\n",__FUNCTION__,__LINE__,status);
    net_device_event(dev, HGIC_EVENT_P2P_INV_RESULT, status, sizeof(int));
}


void wpas_notify_p2p_sd_request(struct wpa_supplicant *wpa_s,
                                int freq, const u8 *sa, u8 dialog_token,
                                u16 update_indic, const u8 *tlvs,
                                size_t tlvs_len)
{
}


void wpas_notify_p2p_sd_response(struct wpa_supplicant *wpa_s,
                                 const u8 *sa, u16 update_indic,
                                 const u8 *tlvs, size_t tlvs_len)
{
}

void wpas_notify_p2p_provision_discovery(struct wpa_supplicant *wpa_s,
        const u8 *dev_addr, int request,
        enum p2p_prov_disc_status status,
        u16 config_methods,
        unsigned int generated_pin)
{
}


void wpas_notify_p2p_group_started(struct wpa_supplicant *wpa_s,
				   struct wpa_ssid *ssid, int persistent,
				   int client, const u8 *ip)

{
}

void wpas_notify_p2p_group_formation_failure(struct wpa_supplicant *wpa_s,
        const char *reason)
{
    struct net_device *dev = net_device_get_by_name(hgic_param_ifname(NULL));
    if(!dev) {
        PRINTF("%s:Device %s not found!\n",__FUNCTION__,wpa_s->ifname);
        return;
    }
    PRINTF("%s,%d:group formation failure,reason:%s\n",__FUNCTION__,__LINE__,reason);
    net_device_event(dev, HGIC_EVENT_P2P_GROUP_FAIL, reason, strlen(reason));
}


void wpas_notify_p2p_wps_failed(struct wpa_supplicant *wpa_s,
                                struct wps_event_fail *fail)
{
}


void wpas_notify_p2p_invitation_received(struct wpa_supplicant *wpa_s,
        const u8 *sa, const u8 *go_dev_addr,
        const u8 *bssid, int id, int op_freq)
{
    struct net_device *dev = net_device_get_by_name(hgic_param_ifname(NULL));
    if(!dev) {
        PRINTF("%s:Device %s not found!\n",__FUNCTION__,wpa_s->ifname);
        return;
    }
    PRINTF("%s:sa:"MACSTR",go:"MACSTR",freq:%d\n",__FUNCTION__,
        MAC2STR(sa),MAC2STR(go_dev_addr),op_freq);   
    net_device_event(dev, HGIC_EVENT_P2P_INV_RECV, sa, 6);
}

#endif /* CONFIG_P2P */

#if 0//def CONFIG_P2P

void wpas_notify_p2p_find_stopped(struct wpa_supplicant *wpa_s)
{
	/* Notify P2P find has stopped */
	wpas_dbus_signal_p2p_find_stopped(wpa_s);
}


void wpas_notify_p2p_device_found(struct wpa_supplicant *wpa_s,
				  const u8 *dev_addr, int new_device)
{
	if (new_device) {
		/* Create the new peer object */
		wpas_dbus_register_peer(wpa_s, dev_addr);
	}

	/* Notify a new peer has been detected*/
	wpas_dbus_signal_peer_device_found(wpa_s, dev_addr);
}


void wpas_notify_p2p_device_lost(struct wpa_supplicant *wpa_s,
				 const u8 *dev_addr)
{
	wpas_dbus_unregister_peer(wpa_s, dev_addr);

	/* Create signal on interface object*/
	wpas_dbus_signal_peer_device_lost(wpa_s, dev_addr);
}


void wpas_notify_p2p_group_removed(struct wpa_supplicant *wpa_s,
				   const struct wpa_ssid *ssid,
				   const char *role)
{
	wpas_dbus_signal_p2p_group_removed(wpa_s, role);

	wpas_dbus_unregister_p2p_group(wpa_s, ssid);
}


void wpas_notify_p2p_go_neg_req(struct wpa_supplicant *wpa_s,
				const u8 *src, u16 dev_passwd_id, u8 go_intent)
{
	wpas_dbus_signal_p2p_go_neg_req(wpa_s, src, dev_passwd_id, go_intent);
}


void wpas_notify_p2p_go_neg_completed(struct wpa_supplicant *wpa_s,
				      struct p2p_go_neg_results *res)
{
	wpas_dbus_signal_p2p_go_neg_resp(wpa_s, res);
}


void wpas_notify_p2p_invitation_result(struct wpa_supplicant *wpa_s,
				       int status, const u8 *bssid)
{
	wpas_dbus_signal_p2p_invitation_result(wpa_s, status, bssid);
}


void wpas_notify_p2p_sd_request(struct wpa_supplicant *wpa_s,
				int freq, const u8 *sa, u8 dialog_token,
				u16 update_indic, const u8 *tlvs,
				size_t tlvs_len)
{
	wpas_dbus_signal_p2p_sd_request(wpa_s, freq, sa, dialog_token,
					update_indic, tlvs, tlvs_len);
}


void wpas_notify_p2p_sd_response(struct wpa_supplicant *wpa_s,
				 const u8 *sa, u16 update_indic,
				 const u8 *tlvs, size_t tlvs_len)
{
	wpas_dbus_signal_p2p_sd_response(wpa_s, sa, update_indic,
					 tlvs, tlvs_len);
}


/**
 * wpas_notify_p2p_provision_discovery - Notification of provision discovery
 * @dev_addr: Who sent the request or responded to our request.
 * @request: Will be 1 if request, 0 for response.
 * @status: Valid only in case of response (0 in case of success)
 * @config_methods: WPS config methods
 * @generated_pin: PIN to be displayed in case of WPS_CONFIG_DISPLAY method
 *
 * This can be used to notify:
 * - Requests or responses
 * - Various config methods
 * - Failure condition in case of response
 */
void wpas_notify_p2p_provision_discovery(struct wpa_supplicant *wpa_s,
					 const u8 *dev_addr, int request,
					 enum p2p_prov_disc_status status,
					 u16 config_methods,
					 unsigned int generated_pin)
{
	wpas_dbus_signal_p2p_provision_discovery(wpa_s, dev_addr, request,
						 status, config_methods,
						 generated_pin);
}


void wpas_notify_p2p_group_started(struct wpa_supplicant *wpa_s,
				   struct wpa_ssid *ssid, int persistent,
				   int client, const u8 *ip)
{
	/* Notify a group has been started */
	wpas_dbus_register_p2p_group(wpa_s, ssid);

	wpas_dbus_signal_p2p_group_started(wpa_s, client, persistent, ip);
}


void wpas_notify_p2p_group_formation_failure(struct wpa_supplicant *wpa_s,
					     const char *reason)
{
	/* Notify a group formation failed */
	wpas_dbus_signal_p2p_group_formation_failure(wpa_s, reason);
}


void wpas_notify_p2p_wps_failed(struct wpa_supplicant *wpa_s,
				struct wps_event_fail *fail)
{
	wpas_dbus_signal_p2p_wps_failed(wpa_s, fail);
}


void wpas_notify_p2p_invitation_received(struct wpa_supplicant *wpa_s,
					 const u8 *sa, const u8 *go_dev_addr,
					 const u8 *bssid, int id, int op_freq)
{
	/* Notify a P2P Invitation Request */
	wpas_dbus_signal_p2p_invitation_received(wpa_s, sa, go_dev_addr, bssid,
						 id, op_freq);
}

#endif /* CONFIG_P2P */

void wpas_notify_eap_error(struct wpa_supplicant *wpa_s, int error_code)
{
	wpa_msg(wpa_s, MSG_ERROR, WPA_EVENT_EAP_ERROR_CODE "%d", error_code);
}

static void wpas_notify_ap_sta_authorized(struct wpa_supplicant *wpa_s,
        const u8 *sta,
        const u8 *p2p_dev_addr)
{
}


static void wpas_notify_ap_sta_deauthorized(struct wpa_supplicant *wpa_s,
        const u8 *sta,
        const u8 *p2p_dev_addr)
{
}


void wpas_notify_sta_authorized(struct wpa_supplicant *wpa_s,
                                const u8 *mac_addr, int authorized,
                                const u8 *p2p_dev_addr)
{
}


void wpas_notify_certification(struct wpa_supplicant *wpa_s,
			       struct tls_cert_data *cert,
			       const char *cert_hash)
{

}


void wpas_notify_preq(struct wpa_supplicant *wpa_s,
                      const u8 *addr, const u8 *dst, const u8 *bssid,
                      const u8 *ie, size_t ie_len, u32 ssi_signal)
{
}


void wpas_notify_eap_status(struct wpa_supplicant *wpa_s, const char *status,
                            const char *parameter)
{
}


void wpas_notify_network_bssid_set_changed(struct wpa_supplicant *wpa_s,
        struct wpa_ssid *ssid)
{
}


void wpas_notify_network_type_changed(struct wpa_supplicant *wpa_s,
                                      struct wpa_ssid *ssid)
{
}

