/*
 * Wi-Fi Protected Setup - Strict protocol validation routines
 * Copyright (c) 2010, Atheros Communications, Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */
#ifdef CONFIG_WPS_STRICT
#include "atbm_hal.h"
#include "wpa_debug.h"


#ifndef WPS_STRICT_ALL
#define WPS_STRICT_WPS2
#endif /* WPS_STRICT_ALL */


 static int wps_validate_version(const atbm_uint8 *version, int mandatory)
{
	if (version == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Version attribute "
				   "missing");
			return -1;
		}
		return 0;
	}
	if (*version != 0x10) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Version attribute "
			   "value 0x%x", *version);
		return -1;
	}
	return 0;
}


 static int wps_validate_version2(const atbm_uint8 *version2, int mandatory)
{
	if (version2 == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Version2 attribute "
				   "missing");
			return -1;
		}
		return 0;
	}
	if (*version2 < 0x20) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Version2 attribute "
			   "value 0x%x", *version2);
		return -1;
	}
	return 0;
}


 static int wps_validate_request_type(const atbm_uint8 *request_type, int mandatory)
{
	if (request_type == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Request Type "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (*request_type > 0x03) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Request Type "
			   "attribute value 0x%x", *request_type);
		return -1;
	}
	return 0;
}


 static int wps_validate_response_type(const atbm_uint8 *response_type, int mandatory)
{
	if (response_type == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Response Type "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (*response_type > 0x03) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Response Type "
			   "attribute value 0x%x", *response_type);
		return -1;
	}
	return 0;
}


 static int valid_config_methods(atbm_uint16 val, int wps2)
{
	if (wps2) {
		if ((val & 0x6000) && !(val & WPS_CONFIG_DISPLAY)) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Physical/Virtual "
				   "Display flag without old Display flag "
				   "set");
			return 0;
		}
		if (!(val & 0x6000) && (val & WPS_CONFIG_DISPLAY)) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Display flag "
				   "without Physical/Virtual Display flag");
			return 0;
		}
		if ((val & 0x0600) && !(val & WPS_CONFIG_PUSHBUTTON)) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Physical/Virtual "
				   "PushButton flag without old PushButton "
				   "flag set");
			return 0;
		}
		if (!(val & 0x0600) && (val & WPS_CONFIG_PUSHBUTTON)) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: PushButton flag "
				   "without Physical/Virtual PushButton flag");
			return 0;
		}
	}

	return 1;
}


 static int wps_validate_config_methods(const atbm_uint8 *config_methods, int wps2,
				       int mandatory)
{
	atbm_uint16 val;

	if (config_methods == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Configuration "
				   "Methods attribute missing");
			return -1;
		}
		return 0;
	}

	val = ATBM_WPA_GET_BE16(config_methods);
	if (!valid_config_methods(val, wps2)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Configuration "
			   "Methods attribute value 0x%04x", val);
		return -1;
	}
	return 0;
}


 static int wps_validate_ap_config_methods(const atbm_uint8 *config_methods, int wps2,
					  int mandatory)
{
	atbm_uint16 val;

	if (wps_validate_config_methods(config_methods, wps2, mandatory) < 0)
		return -1;
	if (config_methods == NULL)
		return 0;
	val = WPA_GET_BE16(config_methods);
	if (val & WPS_CONFIG_PUSHBUTTON) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Configuration "
			   "Methods attribute value 0x%04x in AP info "
			   "(PushButton not allowed for registering new ER)",
			   val);
		return -1;
	}
	return 0;
}


 static int wps_validate_uuid_e(const atbm_uint8 *uuid_e, int mandatory)
{
	if (uuid_e == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: UUID-E "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_uuid_r(const atbm_uint8 *uuid_r, int mandatory)
{
	if (uuid_r == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: UUID-R "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_primary_dev_type(const atbm_uint8 *primary_dev_type,
					 int mandatory)
{
	if (primary_dev_type == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Primary Device Type "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_rf_bands(const atbm_uint8 *rf_bands, int mandatory)
{
	if (rf_bands == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: RF Bands "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (*rf_bands != WPS_RF_24GHZ && *rf_bands != WPS_RF_50GHZ &&
	    *rf_bands != (WPS_RF_24GHZ | WPS_RF_50GHZ)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Rf Bands "
			   "attribute value 0x%x", *rf_bands);
		return -1;
	}
	return 0;
}


 static int wps_validate_assoc_state(const atbm_uint8 *assoc_state, int mandatory)
{
	atbm_uint16 val;
	if (assoc_state == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Association State "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	val = ATBM_WPA_GET_BE16(assoc_state);
	if (val > 4) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Association State "
			   "attribute value 0x%04x", val);
		return -1;
	}
	return 0;
}


 static int wps_validate_config_error(const atbm_uint8 *config_error, int mandatory)
{
	atbm_uint16 val;

	if (config_error == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Configuration Error "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	val = ATBM_WPA_GET_BE16(config_error);
	if (val > 18) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Configuration Error "
			   "attribute value 0x%04x", val);
		return -1;
	}
	return 0;
}


 static int wps_validate_dev_password_id(const atbm_uint8 *dev_password_id,
					int mandatory)
{
	atbm_uint16 val;

	if (dev_password_id == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Device Password ID "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	val = ATBM_WPA_GET_BE16(dev_password_id);
	if (val >= 0x0006 && val <= 0x000f) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Device Password ID "
			   "attribute value 0x%04x", val);
		return -1;
	}
	return 0;
}


 static int wps_validate_manufacturer(const atbm_uint8 *manufacturer, atbm_size_t len,
				     int mandatory)
{
	if (manufacturer == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Manufacturer "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (len > 0 && manufacturer[len - 1] == 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Manufacturer "
			   "attribute value %s %d\n", manufacturer, len);
		return -1;
	}
	return 0;
}


 static int wps_validate_model_name(const atbm_uint8 *model_name, atbm_size_t len,
				   int mandatory)
{
	if (model_name == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Model Name "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (len > 0 && model_name[len - 1] == 0) {
		//wpa_hexdump_ascii(MSG_ERROR, "WPS-STRICT: Invalid Model Name "
		//	   "attribute value", model_name, len);
		return -1;
	}
	return 0;
}


 static int wps_validate_model_number(const atbm_uint8 *model_number, atbm_size_t len,
				     int mandatory)
{
	if (model_number == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Model Number "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (len > 0 && model_number[len - 1] == 0) {
		//wpa_hexdump_ascii(MSG_ERROR, "WPS-STRICT: Invalid Model Number "
		//	   "attribute value", model_number, len);
		return -1;
	}
	return 0;
}


 static int wps_validate_serial_number(const atbm_uint8 *serial_number, atbm_size_t len,
				      int mandatory)
{
	if (serial_number == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Serial Number "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (len > 0 && serial_number[len - 1] == 0) {
		//wpa_hexdump_ascii(MSG_ERROR, "WPS-STRICT: Invalid Serial "
		//		  "Number attribute value",
		//		  serial_number, len);
		return -1;
	}
	return 0;
}


 static int wps_validate_dev_name(const atbm_uint8 *dev_name, atbm_size_t len,
				 int mandatory)
{
	if (dev_name == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Device Name "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (len > 0 && dev_name[len - 1] == 0) {
		//wpa_hexdump_ascii(MSG_ERROR, "WPS-STRICT: Invalid Device Name "
		//	   "attribute value", dev_name, len);
		return -1;
	}
	return 0;
}


 static int wps_validate_request_to_enroll(const atbm_uint8 *request_to_enroll,
					  int mandatory)
{
	if (request_to_enroll == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Request to Enroll "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (*request_to_enroll > 0x01) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Request to Enroll "
			   "attribute value 0x%x", *request_to_enroll);
		return -1;
	}
	return 0;
}


 static int wps_validate_req_dev_type(const atbm_uint8 *req_dev_type[], atbm_size_t num,
				     int mandatory)
{
	if (num == 0) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Requested Device "
				   "Type attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_wps_state(const atbm_uint8 *wps_state, int mandatory)
{
	if (wps_state == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Wi-Fi Protected "
				   "Setup State attribute missing");
			return -1;
		}
		return 0;
	}
	if (*wps_state != WPS_STATE_NOT_CONFIGURED &&
	    *wps_state != WPS_STATE_CONFIGURED) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Wi-Fi Protected "
			   "Setup State attribute value 0x%x", *wps_state);
		return -1;
	}
	return 0;
}


 static int wps_validate_ap_setup_locked(const atbm_uint8 *ap_setup_locked,
					int mandatory)
{
	if (ap_setup_locked == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: AP Setup Locked "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (*ap_setup_locked > 1) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid AP Setup Locked "
			   "attribute value 0x%x", *ap_setup_locked);
		return -1;
	}
	return 0;
}


 static int wps_validate_selected_registrar(const atbm_uint8 *selected_registrar,
					   int mandatory)
{
	if (selected_registrar == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Selected Registrar "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (*selected_registrar > 1) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Selected Registrar "
			   "attribute value 0x%x", *selected_registrar);
		return -1;
	}
	return 0;
}


 static int wps_validate_sel_reg_config_methods(const atbm_uint8 *config_methods,
					       int wps2, int mandatory)
{
	atbm_uint16 val;

	if (config_methods == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Selected Registrar "
				   "Configuration Methods attribute missing");
			return -1;
		}
		return 0;
	}

	val = ATBM_WPA_GET_BE16(config_methods);
	if (!valid_config_methods(val, wps2)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Selected Registrar "
			   "Configuration Methods attribute value 0x%04x",
			   val);
		return -1;
	}
	return 0;
}


 static int wps_validate_authorized_macs(const atbm_uint8 *authorized_macs, atbm_size_t len,
					int mandatory)
{
	if (authorized_macs == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Authorized MACs "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (len > 30 && (len % ETH_ALEN) != 0) {
		wpa_hexdump(MSG_ERROR, "WPS-STRICT: Invalid Authorized "
			    "MACs attribute value", authorized_macs, len);
		return -1;
	}
	return 0;
}


 static int wps_validate_msg_type(const atbm_uint8 *msg_type, int mandatory)
{
	if (msg_type == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Message Type "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (*msg_type < WPS_Beacon || *msg_type > WPS_WSC_DONE) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Message Type "
			   "attribute value 0x%x", *msg_type);
		return -1;
	}
	return 0;
}


 static int wps_validate_mac_addr(const atbm_uint8 *mac_addr, int mandatory)
{
	if (mac_addr == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: MAC Address "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (mac_addr[0] & 0x01) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid MAC Address "
			   "attribute value " MACSTR, MAC2STR(mac_addr));
		return -1;
	}
	return 0;
}


 static int wps_validate_enrollee_nonce(const atbm_uint8 *enrollee_nonce, int mandatory)
{
	if (enrollee_nonce == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Enrollee Nonce "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_registrar_nonce(const atbm_uint8 *registrar_nonce,
					int mandatory)
{
	if (registrar_nonce == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Registrar Nonce "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_public_key(const atbm_uint8 *public_key, atbm_size_t len,
				   int mandatory)
{
	if (public_key == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Public Key "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (len != 192) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Public Key "
			   "attribute length %d", (int) len);
		return -1;
	}
	return 0;
}


 static int num_bits_set(atbm_uint16 val)
{
	int c;
	for (c = 0; val; c++)
		val &= val - 1;
	return c;
}


 static int wps_validate_auth_type_flags(const atbm_uint8 *flags, int mandatory)
{
	atbm_uint16 val;

	if (flags == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Authentication Type "
				   "Flags attribute missing");
			return -1;
		}
		return 0;
	}
	val = ATBM_WPA_GET_BE16(flags);
	if ((val & ~WPS_AUTH_TYPES) || !(val & WPS_AUTH_WPA2PSK)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Authentication Type "
			   "Flags attribute value 0x%04x", val);
		return -1;
	}
	return 0;
}


 static int wps_validate_auth_type(const atbm_uint8 *type, int mandatory)
{
	atbm_uint16 val;

	if (type == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Authentication Type "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	val = ATBM_WPA_GET_BE16(type);
	if ((val & ~WPS_AUTH_TYPES) || val == 0 ||
	    (num_bits_set(val) > 1 &&
	     val != (WPS_AUTH_WPAPSK | WPS_AUTH_WPA2PSK))) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Authentication Type "
			   "attribute value 0x%04x", val);
		return -1;
	}
	return 0;
}


 static int wps_validate_encr_type_flags(const atbm_uint8 *flags, int mandatory)
{
	atbm_uint16 val;

	if (flags == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Encryption Type "
				   "Flags attribute missing");
			return -1;
		}
		return 0;
	}
	val = ATBM_WPA_GET_BE16(flags);
	if ((val & ~WPS_ENCR_TYPES) || !(val & WPS_ENCR_AES)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Encryption Type "
			   "Flags attribute value 0x%04x", val);
		return -1;
	}
	return 0;
}


 static int wps_validate_encr_type(const atbm_uint8 *type, int mandatory)
{
	atbm_uint16 val;

	if (type == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Encryption Type "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	val = ATBM_WPA_GET_BE16(type);
	if ((val & ~WPS_ENCR_TYPES) || val == 0 ||
	    (num_bits_set(val) > 1 && val != (WPS_ENCR_TKIP | WPS_ENCR_AES))) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Encryption Type "
			   "attribute value 0x%04x", val);
		return -1;
	}
	return 0;
}


 static int wps_validate_conn_type_flags(const atbm_uint8 *flags, int mandatory)
{
	if (flags == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Connection Type "
				   "Flags attribute missing");
			return -1;
		}
		return 0;
	}
	if ((*flags & ~(WPS_CONN_ESS | WPS_CONN_IBSS)) ||
	    !(*flags & WPS_CONN_ESS)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Connection Type "
			   "Flags attribute value 0x%02x", *flags);
		return -1;
	}
	return 0;
}


 static int wps_validate_os_version(const atbm_uint8 *os_version, int mandatory)
{
	if (os_version == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: OS Version "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_authenticator(const atbm_uint8 *authenticator, int mandatory)
{
	if (authenticator == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Authenticator "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_e_hash1(const atbm_uint8 *hash, int mandatory)
{
	if (hash == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: E-Hash1 "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_e_hash2(const atbm_uint8 *hash, int mandatory)
{
	if (hash == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: E-Hash2 "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_r_hash1(const atbm_uint8 *hash, int mandatory)
{
	if (hash == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: R-Hash1 "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_r_hash2(const atbm_uint8 *hash, int mandatory)
{
	if (hash == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: R-Hash2 "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_encr_settings(const atbm_uint8 *encr_settings, atbm_size_t len,
				   int mandatory)
{
	if (encr_settings == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Encrypted Settings "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (len < 16) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Encrypted Settings "
			   "attribute length %d", (int) len);
		return -1;
	}
	return 0;
}


 static int wps_validate_settings_delay_time(const atbm_uint8 *delay, int mandatory)
{
	if (delay == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Settings Delay Time "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_r_snonce1(const atbm_uint8 *nonce, int mandatory)
{
	if (nonce == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: R-SNonce1 "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_r_snonce2(const atbm_uint8 *nonce, int mandatory)
{
	if (nonce == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: R-SNonce2 "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_e_snonce1(const atbm_uint8 *nonce, int mandatory)
{
	if (nonce == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: E-SNonce1 "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_e_snonce2(const atbm_uint8 *nonce, int mandatory)
{
	if (nonce == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: E-SNonce2 "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_key_wrap_auth(const atbm_uint8 *auth, int mandatory)
{
	if (auth == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Key Wrap "
				   "Authenticator attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_ssid(const atbm_uint8 *ssid, atbm_size_t ssid_len, int mandatory)
{
	if (ssid == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: SSID "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (ssid_len == 0 || ssid[ssid_len - 1] == 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid SSID "
				  "attribute value %s %d\n", ssid, ssid_len);
		return -1;
	}
	return 0;
}


 static int wps_validate_network_key_index(const atbm_uint8 *idx, int mandatory)
{
	if (idx == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Network Key Index "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_network_idx(const atbm_uint8 *idx, int mandatory)
{
	if (idx == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Network Index "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	return 0;
}


 static int wps_validate_network_key(const atbm_uint8 *key, atbm_size_t key_len,
				    const atbm_uint8 *encr_type, int mandatory)
{
	if (key == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Network Key "
				   "attribute missing");
			return -1;
		}
		return 0;
	}
	if (((encr_type == NULL || ATBM_WPA_GET_BE16(encr_type) != WPS_ENCR_WEP) &&
	     key_len > 8 && key_len < 64 && key[key_len - 1] == 0) ||
	    key_len > 64) {
		//wpa_hexdump_ascii_key(MSG_ERROR, "WPS-STRICT: Invalid Network "
		//		      "Key attribute value", key, key_len);
		return -1;
	}
	return 0;
}


 static int wps_validate_network_key_shareable(const atbm_uint8 *val, int mandatory)
{
	if (val == NULL) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Network Key "
				   "Shareable attribute missing");
			return -1;
		}
		return 0;
	}
	if (*val > 1) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Network Key "
			   "Shareable attribute value 0x%x", *val);
		return -1;
	}
	return 0;
}


 static int wps_validate_cred(const atbm_uint8 *cred, atbm_size_t len)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	struct wpabuf buf;

	if (cred == NULL)
		return -1;

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: cred alloc failed");
		ret = -1;
		goto __error__;
	}
	
	wpabuf_set(&buf, cred, len);
	if (wps_parse_msg(&buf, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse Credential");
		ret = -1;
		goto __error__;
	}

	if (wps_validate_network_idx(attr->network_idx, 1) ||
	    wps_validate_ssid(attr->ssid, attr->ssid_len, 1) ||
	    wps_validate_auth_type(attr->auth_type, 1) ||
	    wps_validate_encr_type(attr->encr_type, 1) ||
	    wps_validate_network_key_index(attr->network_key_idx, 0) ||
	    wps_validate_network_key(attr->network_key, attr->network_key_len,
				     attr->encr_type, 1) ||
	    wps_validate_mac_addr(attr->mac_addr, 1) ||
	    wps_validate_network_key_shareable(attr->network_key_shareable, 0))
	{
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Credential");
		ret = -1;
		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);

	return ret;
}


 static int wps_validate_credential(const atbm_uint8 *cred[], atbm_size_t len[], atbm_size_t num,
				   int mandatory)
{
	atbm_size_t i;

	if (num == 0) {
		if (mandatory) {
			wpa_printf(MSG_ERROR, "WPS-STRICT: Credential "
				   "attribute missing");
			return -1;
		}
		return 0;
	}

	for (i = 0; i < num; i++) {
		if (wps_validate_cred(cred[i], len[i]) < 0)
			return -1;
	}

	return 0;
}


 int wps_validate_beacon(const struct wpabuf *wps_ie)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2, sel_reg;

	if (wps_ie == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No WPS IE in Beacon frame");
		ret = -1;
		goto __error__;
	}
	
	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: beacon alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(wps_ie, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse WPS IE in "
			   "Beacon frame");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	sel_reg = attr->selected_registrar != NULL &&
		*attr->selected_registrar != 0;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_wps_state(attr->wps_state, 1) ||
	    wps_validate_ap_setup_locked(attr->ap_setup_locked, 0) ||
	    wps_validate_selected_registrar(attr->selected_registrar, 0) ||
	    wps_validate_dev_password_id(attr->dev_password_id, sel_reg) ||
	    wps_validate_sel_reg_config_methods(attr->sel_reg_config_methods,
						wps2, sel_reg) ||
	    wps_validate_uuid_e(attr->uuid_e, 0) ||
	    wps_validate_rf_bands(attr->rf_bands, 0) ||
	    wps_validate_version2(attr->version2, wps2) ||
	    wps_validate_authorized_macs(attr->authorized_macs,
					 attr->authorized_macs_len, 0)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Beacon frame");
		ret = -1;
		goto __error__;
	}
	
__error__:
	if(attr)
		atbm_kfree(attr);

	return ret;
}


 int wps_validate_beacon_probe_resp(const struct wpabuf *wps_ie, int probe,
				   const atbm_uint8 *addr)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2, sel_reg;

	if (wps_ie == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No WPS IE in "
			   "%sProbe Response frame", probe ? "" : "Beacon/");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: probe resp alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(wps_ie, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse WPS IE in "
			   "%sProbe Response frame", probe ? "" : "Beacon/");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	sel_reg = attr->selected_registrar != NULL &&
		*attr->selected_registrar != 0;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_wps_state(attr->wps_state, 1) ||
	    wps_validate_ap_setup_locked(attr->ap_setup_locked, 0) ||
	    wps_validate_selected_registrar(attr->selected_registrar, 0) ||
	    wps_validate_dev_password_id(attr->dev_password_id, sel_reg) ||
	    wps_validate_sel_reg_config_methods(attr->sel_reg_config_methods,
						wps2, sel_reg) ||
	    wps_validate_response_type(attr->response_type, probe) ||
	    wps_validate_uuid_e(attr->uuid_e, probe) ||
	    wps_validate_manufacturer(attr->manufacturer, attr->manufacturer_len,
				      probe) ||
	    wps_validate_model_name(attr->model_name, attr->model_name_len,
				    probe) ||
	    wps_validate_model_number(attr->model_number, attr->model_number_len,
				      probe) ||
	    wps_validate_serial_number(attr->serial_number,
				       attr->serial_number_len, probe) ||
	    wps_validate_primary_dev_type(attr->primary_dev_type, probe) ||
	    wps_validate_dev_name(attr->dev_name, attr->dev_name_len, probe) ||
	    wps_validate_ap_config_methods(attr->config_methods, wps2, probe) ||
	    wps_validate_rf_bands(attr->rf_bands, 0) ||
	    wps_validate_version2(attr->version2, wps2) ||
	    wps_validate_authorized_macs(attr->authorized_macs,
					 attr->authorized_macs_len, 0)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid %sProbe Response "
			   "frame from " MACSTR, probe ? "" : "Beacon/",
			   MAC2STR(addr));
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */
		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);
	
	return ret;
}


 int wps_validate_probe_req(const struct wpabuf *wps_ie, const atbm_uint8 *addr)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (wps_ie == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No WPS IE in "
			   "Probe Request frame");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: probe req alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(wps_ie, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse WPS IE in "
			   "Probe Request frame");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_request_type(attr->request_type, 1) ||
	    wps_validate_config_methods(attr->config_methods, wps2, 1) ||
	    wps_validate_uuid_e(attr->uuid_e, attr->uuid_r == NULL) ||
	    wps_validate_uuid_r(attr->uuid_r, attr->uuid_e == NULL) ||
	    wps_validate_primary_dev_type(attr->primary_dev_type, 1) ||
	    wps_validate_rf_bands(attr->rf_bands, 1) ||
	    wps_validate_assoc_state(attr->assoc_state, 1) ||
	    wps_validate_config_error(attr->config_error, 1) ||
	    wps_validate_dev_password_id(attr->dev_password_id, 1) ||
	    wps_validate_version2(attr->version2, wps2) ||
	    wps_validate_manufacturer(attr->manufacturer, attr->manufacturer_len,
				      wps2) ||
	    wps_validate_model_name(attr->model_name, attr->model_name_len,
				    wps2) ||
	    wps_validate_model_number(attr->model_number, attr->model_number_len,
				      wps2) ||
	    wps_validate_dev_name(attr->dev_name, attr->dev_name_len, wps2) ||
	    wps_validate_request_to_enroll(attr->request_to_enroll, 0) ||
	    wps_validate_req_dev_type(attr->req_dev_type, attr->num_req_dev_type,
				      0)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid Probe Request "
			   "frame from " MACSTR, MAC2STR(addr));
		ret = -1;
		goto __error__;
	}
	
__error__:
	if(attr)
		atbm_kfree(attr);

	return ret;
}


 int wps_validate_assoc_req(const struct wpabuf *wps_ie)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (wps_ie == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No WPS IE in "
			   "(Re)Association Request frame");
		return -1;
	}
	
	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: assoc req alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(wps_ie, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse WPS IE in "
			   "(Re)Association Request frame");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_request_type(attr->request_type, 1) ||
	    wps_validate_version2(attr->version2, wps2)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid (Re)Association "
			   "Request frame");
		ret = -1;
		goto __error__;
	}
	
__error__:
	if(attr)
		atbm_kfree(attr);

	return ret;
}


 int wps_validate_assoc_resp(const struct wpabuf *wps_ie)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (wps_ie == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No WPS IE in "
			   "(Re)Association Response frame");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: assoc resp alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(wps_ie, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse WPS IE in "
			   "(Re)Association Response frame");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_response_type(attr->response_type, 1) ||
	    wps_validate_version2(attr->version2, wps2)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid (Re)Association "
			   "Response frame");
		ret = -1;
		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);

	return ret;
}


 int wps_validate_m1(const struct wpabuf *tlvs)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in M1");
		return -1;
	}
	
	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: m1 alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in M1");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_msg_type(attr->msg_type, 1) ||
	    wps_validate_uuid_e(attr->uuid_e, 1) ||
	    wps_validate_mac_addr(attr->mac_addr, 1) ||
	    wps_validate_enrollee_nonce(attr->enrollee_nonce, 1) ||
	    wps_validate_public_key(attr->public_key, attr->public_key_len, 1) ||
	    wps_validate_auth_type_flags(attr->auth_type_flags, 1) ||
	    wps_validate_encr_type_flags(attr->encr_type_flags, 1) ||
	    wps_validate_conn_type_flags(attr->conn_type_flags, 1) ||
	    wps_validate_config_methods(attr->config_methods, wps2, 1) ||
	    wps_validate_wps_state(attr->wps_state, 1) ||
	    wps_validate_manufacturer(attr->manufacturer, attr->manufacturer_len,
				      1) ||
	    wps_validate_model_name(attr->model_name, attr->model_name_len, 1) ||
	    wps_validate_model_number(attr->model_number, attr->model_number_len,
				      1) ||
	    wps_validate_serial_number(attr->serial_number,
				       attr->serial_number_len, 1) ||
	    wps_validate_primary_dev_type(attr->primary_dev_type, 1) ||
	    wps_validate_dev_name(attr->dev_name, attr->dev_name_len, 1) ||
	    wps_validate_rf_bands(attr->rf_bands, 1) ||
	    wps_validate_assoc_state(attr->assoc_state, 1) ||
	    wps_validate_dev_password_id(attr->dev_password_id, 1) ||
	    wps_validate_config_error(attr->config_error, 1) ||
	    wps_validate_os_version(attr->os_version, 1) ||
	    wps_validate_version2(attr->version2, wps2) ||
	    wps_validate_request_to_enroll(attr->request_to_enroll, 0)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid M1");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */
		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);

	return ret;
}


 int wps_validate_m2(const struct wpabuf *tlvs)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in M2");
		return -1;
	}
	
	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: m2 alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in M2");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_msg_type(attr->msg_type, 1) ||
	    wps_validate_enrollee_nonce(attr->enrollee_nonce, 1) ||
	    wps_validate_registrar_nonce(attr->registrar_nonce, 1) ||
	    wps_validate_uuid_r(attr->uuid_r, 1) ||
	    wps_validate_public_key(attr->public_key, attr->public_key_len, 1) ||
	    wps_validate_auth_type_flags(attr->auth_type_flags, 1) ||
	    wps_validate_encr_type_flags(attr->encr_type_flags, 1) ||
	    wps_validate_conn_type_flags(attr->conn_type_flags, 1) ||
	    wps_validate_config_methods(attr->config_methods, wps2, 1) ||
	    wps_validate_manufacturer(attr->manufacturer, attr->manufacturer_len,
				      1) ||
	    wps_validate_model_name(attr->model_name, attr->model_name_len, 1) ||
	    wps_validate_model_number(attr->model_number, attr->model_number_len,
				      1) ||
	    wps_validate_serial_number(attr->serial_number,
				       attr->serial_number_len, 1) ||
	    wps_validate_primary_dev_type(attr->primary_dev_type, 1) ||
	    wps_validate_dev_name(attr->dev_name, attr->dev_name_len, 1) ||
	    wps_validate_rf_bands(attr->rf_bands, 1) ||
	    wps_validate_assoc_state(attr->assoc_state, 1) ||
	    wps_validate_config_error(attr->config_error, 1) ||
	    wps_validate_dev_password_id(attr->dev_password_id, 1) ||
	    wps_validate_os_version(attr->os_version, 1) ||
	    wps_validate_version2(attr->version2, wps2) ||
	    wps_validate_authenticator(attr->authenticator, 1)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid M2");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */

		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);

	return ret;

}


 int wps_validate_m2d(const struct wpabuf *tlvs)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in M2D");
		return -1;
	}
	
	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: m2d alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in M2D");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_msg_type(attr->msg_type, 1) ||
	    wps_validate_enrollee_nonce(attr->enrollee_nonce, 1) ||
	    wps_validate_registrar_nonce(attr->registrar_nonce, 1) ||
	    wps_validate_uuid_r(attr->uuid_r, 1) ||
	    wps_validate_auth_type_flags(attr->auth_type_flags, 1) ||
	    wps_validate_encr_type_flags(attr->encr_type_flags, 1) ||
	    wps_validate_conn_type_flags(attr->conn_type_flags, 1) ||
	    wps_validate_config_methods(attr->config_methods, wps2, 1) ||
	    wps_validate_manufacturer(attr->manufacturer, attr->manufacturer_len,
				      1) ||
	    wps_validate_model_name(attr->model_name, attr->model_name_len, 1) ||
	    wps_validate_model_number(attr->model_number, attr->model_number_len,
				      1) ||
	    wps_validate_serial_number(attr->serial_number,
				       attr->serial_number_len, 1) ||
	    wps_validate_primary_dev_type(attr->primary_dev_type, 1) ||
	    wps_validate_dev_name(attr->dev_name, attr->dev_name_len, 1) ||
	    wps_validate_rf_bands(attr->rf_bands, 1) ||
	    wps_validate_assoc_state(attr->assoc_state, 1) ||
	    wps_validate_config_error(attr->config_error, 1) ||
	    wps_validate_os_version(attr->os_version, 1) ||
	    wps_validate_version2(attr->version2, wps2)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid M2D");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */
		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);

	return ret;

}


 int wps_validate_m3(const struct wpabuf *tlvs)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in M3");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: m3 alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in M3");
		return -1;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_msg_type(attr->msg_type, 1) ||
	    wps_validate_registrar_nonce(attr->registrar_nonce, 1) ||
	    wps_validate_e_hash1(attr->e_hash1, 1) ||
	    wps_validate_e_hash2(attr->e_hash2, 1) ||
	    wps_validate_version2(attr->version2, wps2) ||
	    wps_validate_authenticator(attr->authenticator, 1)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid M3");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */

		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);

	return 0;
}


 int wps_validate_m4(const struct wpabuf *tlvs)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in M4");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: m4 alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in M4");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_msg_type(attr->msg_type, 1) ||
	    wps_validate_enrollee_nonce(attr->enrollee_nonce, 1) ||
	    wps_validate_r_hash1(attr->r_hash1, 1) ||
	    wps_validate_r_hash2(attr->r_hash2, 1) ||
	    wps_validate_encr_settings(attr->encr_settings,
				       attr->encr_settings_len, 1) ||
	    wps_validate_version2(attr->version2, wps2) ||
	    wps_validate_authenticator(attr->authenticator, 1)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid M4");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */
		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);

	return 0;
}


 int wps_validate_m4_encr(const struct wpabuf *tlvs, int wps2)
{
	int ret = 0;
	struct wps_parse_attr *attr;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in M4 encrypted "
			   "settings");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: m4 encr alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in M4 encrypted settings");
		ret = -1;
		goto __error__;
	}

	if (wps_validate_r_snonce1(attr->r_snonce1, 1) ||
	    wps_validate_key_wrap_auth(attr->key_wrap_auth, 1)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid M4 encrypted "
			   "settings");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */
		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);

	return ret;
}


 int wps_validate_m5(const struct wpabuf *tlvs)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in M5");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: m5 alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in M5");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_msg_type(attr->msg_type, 1) ||
	    wps_validate_registrar_nonce(attr->registrar_nonce, 1) ||
	    wps_validate_encr_settings(attr->encr_settings,
				       attr->encr_settings_len, 1) ||
	    wps_validate_version2(attr->version2, wps2) ||
	    wps_validate_authenticator(attr->authenticator, 1)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid M5");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */
		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);

	return ret;
}


 int wps_validate_m5_encr(const struct wpabuf *tlvs, int wps2)
{
	int ret = 0;
	struct wps_parse_attr *attr;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in M5 encrypted "
			   "settings");
		return -1;
	}
	
	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: m5 encr alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in M5 encrypted settings");
		ret = -1;
		goto __error__;
	}

	if (wps_validate_e_snonce1(attr->e_snonce1, 1) ||
	    wps_validate_key_wrap_auth(attr->key_wrap_auth, 1)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid M5 encrypted "
			   "settings");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */

		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);

	return ret;
}


 int wps_validate_m6(const struct wpabuf *tlvs)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in M6");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: m6 alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in M6");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_msg_type(attr->msg_type, 1) ||
	    wps_validate_enrollee_nonce(attr->enrollee_nonce, 1) ||
	    wps_validate_encr_settings(attr->encr_settings,
				       attr->encr_settings_len, 1) ||
	    wps_validate_version2(attr->version2, wps2) ||
	    wps_validate_authenticator(attr->authenticator, 1)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid M6");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */

		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);
	
	return ret;
}


 int wps_validate_m6_encr(const struct wpabuf *tlvs, int wps2)
{
	int ret = 0;
	struct wps_parse_attr *attr;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in M6 encrypted "
			   "settings");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: m6 encr alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in M6 encrypted settings");
		return -1;
	}

	if (wps_validate_r_snonce2(attr->r_snonce2, 1) ||
	    wps_validate_key_wrap_auth(attr->key_wrap_auth, 1)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid M6 encrypted "
			   "settings");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */

		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);
	
	return ret;
}


 int wps_validate_m7(const struct wpabuf *tlvs)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in M7");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: m7 alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in M7");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_msg_type(attr->msg_type, 1) ||
	    wps_validate_registrar_nonce(attr->registrar_nonce, 1) ||
	    wps_validate_encr_settings(attr->encr_settings,
				       attr->encr_settings_len, 1) ||
	    wps_validate_settings_delay_time(attr->settings_delay_time, 0) ||
	    wps_validate_version2(attr->version2, wps2) ||
	    wps_validate_authenticator(attr->authenticator, 1)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid M7");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */
		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);
	
	return ret;
}


 int wps_validate_m7_encr(const struct wpabuf *tlvs, int ap, int wps2)
{
	int ret = 0;
	struct wps_parse_attr *attr;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in M7 encrypted "
			   "settings");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: m7 encr alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in M7 encrypted settings");
		ret = -1;
		goto __error__;
	}

	if (wps_validate_e_snonce2(attr->e_snonce2, 1) ||
	    wps_validate_ssid(attr->ssid, attr->ssid_len, !ap) ||
	    wps_validate_mac_addr(attr->mac_addr, !ap) ||
	    wps_validate_auth_type(attr->auth_type, !ap) ||
	    wps_validate_encr_type(attr->encr_type, !ap) ||
	    wps_validate_network_key_index(attr->network_key_idx, 0) ||
	    wps_validate_network_key(attr->network_key, attr->network_key_len,
				     attr->encr_type, !ap) ||
	    wps_validate_key_wrap_auth(attr->key_wrap_auth, 1)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid M7 encrypted "
			   "settings");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */
		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);
	
	return ret;
}


 int wps_validate_m8(const struct wpabuf *tlvs)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in M8");
		return -1;
	}
	
	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: m8 alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in M8");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_msg_type(attr->msg_type, 1) ||
	    wps_validate_enrollee_nonce(attr->enrollee_nonce, 1) ||
	    wps_validate_encr_settings(attr->encr_settings,
				       attr->encr_settings_len, 1) ||
	    wps_validate_version2(attr->version2, wps2) ||
	    wps_validate_authenticator(attr->authenticator, 1)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid M8");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */

		goto __error__;
	}
	
__error__:
	if(attr)
		atbm_kfree(attr);

	return 0;
}


 int wps_validate_m8_encr(const struct wpabuf *tlvs, int ap, int wps2)
{
	int ret = 0;
	struct wps_parse_attr *attr;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in M8 encrypted "
			   "settings");
		return -1;
	}
	
	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: m8 encr alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in M8 encrypted settings");
		return -1;
	}

	if (wps_validate_ssid(attr->ssid, attr->ssid_len, ap) ||
	    wps_validate_auth_type(attr->auth_type, ap) ||
	    wps_validate_encr_type(attr->encr_type, ap) ||
	    wps_validate_network_key_index(attr->network_key_idx, 0) ||
	    wps_validate_mac_addr(attr->mac_addr, ap) ||
	    wps_validate_credential(attr->cred, attr->cred_len, attr->num_cred,
				    !ap) ||
	    wps_validate_key_wrap_auth(attr->key_wrap_auth, 1)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid M8 encrypted "
			   "settings");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */
		goto __error__;

	}
	
__error__:
	if(attr)
		atbm_kfree(attr);

	return 0;
}


 int wps_validate_wsc_ack(const struct wpabuf *tlvs)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in WSC_ACK");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: ack alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in WSC_ACK");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_msg_type(attr->msg_type, 1) ||
	    wps_validate_enrollee_nonce(attr->enrollee_nonce, 1) ||
	    wps_validate_registrar_nonce(attr->registrar_nonce, 1) ||
	    wps_validate_version2(attr->version2, wps2)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid WSC_ACK");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */
		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);

	return 0;
}


 int wps_validate_wsc_nack(const struct wpabuf *tlvs)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in WSC_NACK");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: nack alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in WSC_NACK");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_msg_type(attr->msg_type, 1) ||
	    wps_validate_enrollee_nonce(attr->enrollee_nonce, 1) ||
	    wps_validate_registrar_nonce(attr->registrar_nonce, 1) ||
	    wps_validate_config_error(attr->config_error, 1) ||
	    wps_validate_version2(attr->version2, wps2)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid WSC_NACK");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */
		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);

	return 0;
}


 int wps_validate_wsc_done(const struct wpabuf *tlvs)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in WSC_Done");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: done alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in WSC_Done");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_msg_type(attr->msg_type, 1) ||
	    wps_validate_enrollee_nonce(attr->enrollee_nonce, 1) ||
	    wps_validate_registrar_nonce(attr->registrar_nonce, 1) ||
	    wps_validate_version2(attr->version2, wps2)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid WSC_Done");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */
		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);

	return 0;
}


 int wps_validate_upnp_set_selected_registrar(const struct wpabuf *tlvs)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	int wps2;
	int sel_reg;

	if (tlvs == NULL) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: No TLVs in "
			   "SetSelectedRegistrar");
		return -1;
	}

	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: upnp alloc failed");
		ret = -1;
		goto __error__;
	}

	if (wps_parse_msg(tlvs, attr) < 0) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Failed to parse attributes "
			   "in SetSelectedRegistrar");
		ret = -1;
		goto __error__;
	}

	wps2 = attr->version2 != NULL;
	sel_reg = attr->selected_registrar != NULL &&
		*attr->selected_registrar != 0;
	if (wps_validate_version(attr->version, 1) ||
	    wps_validate_dev_password_id(attr->dev_password_id, sel_reg) ||
	    wps_validate_sel_reg_config_methods(attr->sel_reg_config_methods,
						wps2, sel_reg) ||
	    wps_validate_version2(attr->version2, wps2) ||
	    wps_validate_authorized_macs(attr->authorized_macs,
					 attr->authorized_macs_len, wps2) ||
	    wps_validate_uuid_r(attr->uuid_r, wps2)) {
		wpa_printf(MSG_ERROR, "WPS-STRICT: Invalid "
			   "SetSelectedRegistrar");
#ifdef WPS_STRICT_WPS2
		if (wps2)
			ret = -1;
#else /* WPS_STRICT_WPS2 */
		ret = -1;
#endif /* WPS_STRICT_WPS2 */
		goto __error__;
	}

__error__:
	if(attr)
		atbm_kfree(attr);

	return 0;
}
#endif
