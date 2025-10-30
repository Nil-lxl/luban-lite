/**************************************************************************************************************
* altobeam IOT Wi-Fi
*
* Copyright (c) 2018, altobeam.inc   All rights reserved.
*
* The source code contains proprietary information of AltoBeam, and shall not be distributed, 
* copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#ifndef WPS_MAIN_H
#define WPS_MAIN_H
#include "atbm_hal.h"
#include "wpa_main.h"
#include "wpa_supplicant_i.h"
#include "wps_defs.h"
#include "wpabuf.h"
#include "wps_attr_parse.h"

//#define os_realloc realloc
//#define os_strlen strlen
//#define os_strdup strdup
//#define os_snprintf snprintf

#define WSC_FRAGMENT_SIZE 1400

#define WPS_DEV_TYPE_LEN 8
#define WPS_DEV_TYPE_BUFSIZE 21
#define WPS_SEC_DEV_TYPE_MAX_LEN 128
/* maximum number of advertised WPS vendor extension attributes */
#define MAX_WPS_VENDOR_EXTENSIONS 10
/* maximum size of WPS Vendor extension attribute */
#define WPS_MAX_VENDOR_EXT_LEN 1024


#define EAP_VENDOR_TYPE_WSC 1

#define WSC_FLAGS_MF 0x01
#define WSC_FLAGS_LF 0x02

#define WSC_ID_REGISTRAR "WFA-SimpleConfig-Registrar-1-0"
#define WSC_ID_REGISTRAR_LEN 30
#define WSC_ID_ENROLLEE "WFA-SimpleConfig-Enrollee-1-0"
#define WSC_ID_ENROLLEE_LEN 29
/*EAP code: 1byte, Identifier: 1byte, Length: 2bytes, Type: 1byte, TypeData: 29*/
#define WSC_ID_ENROLLEE_IE_LEN (5+WSC_ID_ENROLLEE_LEN)

/**
*Primary Device Type
*/
enum primary_device_type{
	/*Sub Category:
	1 PC
	2 Server
	3 Media Center
	4 Ultra-mobile PC
	5 Notebook
	6 Desktop
	7 MID (Mobile Internet Device)
	8 Netbook
	9 Tablet
	10 Ultrabook
	*/
	Category_Computer = 1,
	
	/*Sub Category:
	1 Keyboard
	2 Mouse
	3 Joystick
	4 Trackball
	5 Gaming controller
	6 Remote
	7 Touchscreen
	8 Biometric reader
	9 Barcode reader
	*/
	Category_Input_Device,
	
	/*Sub Category:
	1 Printer or Print Server
	2 Scanner
	3 Fax
	4 Copier
	5 All-in-one (Printer, Scanner, Fax, Copier)
	*/
	Category_Printer_Scanner_Faxes_Copies,
	
	/*Sub Category:
	1 Digital Still Camera
	2 Video Camera
	3 Web Camera
	4 Security Camera
	*/
	Category_Camera,

	/*Sub Category:
	1 NAS
	*/
	Category_Storage,

	/*Sub Category:
	1 AP
	2 Router
	3 Switch
	4 Gateway
	5 Bridge
	*/
	Category_Network_Infrastructure = 6,

	/*Sub Category:
	1 Television
	2 Electronic Picture Frame
	3 Projector
	4 Monitor
	*/
	Category_Displays,

	/*Sub Category:
	1 DAR
	2 PVR
	3 MCX
	4 Set-top box
	5 Media Server/Media Adapter/Media Extender
	6 Portable Video Player
	*/
	Category_Multimedia_Device,

	/*Sub Category:
	1 Xbox
	2 Xbox360
	3 Playstation
	4 Game Console/Game Console Adapter
	5 Portable Gaming Device
	*/
	Category_Gaming_Devices,

	/*Sub Category:
	1 Windows Mobile
	2 Phone ：C single mode
	3 Phone ：C dual mode
	4 Smartphone ：C single mode
	5 Smartphone ：C dual mode
	*/
	Category_Telephone,

	/*Sub Category:
	1 Audio tuner/receiver
	2 Speakers
	3 Portable Music Player (PMP)
	4 Headset (headphones + microphone)
	5 Headphones
	6 Microphone
	7 Home Theater Systems
	*/
	Category_Audio_Device,

	/*
	1 Computer docking station
	2 Media kiosk
	*/
	Category_Docking_Device,
	
	Category_Others = 255
};
#if 0
/**
 * enum wps_process_res - WPS message processing result
 */
enum wps_process_res {
	/**
	 * WPS_DONE - Processing done
	 */
	WPS_DONE,

	/**
	 * WPS_CONTINUE - Processing continues
	 */
	WPS_CONTINUE,

	/**
	 * WPS_FAILURE - Processing failed
	 */
	WPS_FAILURE,

	/**
	 * WPS_PENDING - Processing continues, but waiting for an external
	 *	event (e.g., UPnP message from an external Registrar)
	 */
	WPS_PENDING
};
#endif
typedef enum
{
	WAIT_START = 0, 
	MESG, 
	FRAG_ACK, 
	WAIT_FRAG_ACK, 
	ATBM_WPS_DONE, 
	ATBM_WPS_FAIL 
} WPS_STATE;


#if 0
/**
 * struct wps_config - WPS configuration for a single registration protocol run
 */
struct wps_config {
	/**
	 * wps - Pointer to long term WPS context
	 */
	struct wps_context *wps;

	/**
	 * registrar - Whether this end is a Registrar
	 */
	int registrar;

	/**
	 * pin - Enrollee Device Password (%NULL for Registrar or PBC)
	 */
	const atbm_uint8 *pin;

	/**
	 * pin_len - Length on pin in octets
	 */
	atbm_size_t pin_len;

	/**
	 * pbc - Whether this is protocol run uses PBC
	 */
	int pbc;

	/**
	 * assoc_wps_ie: (Re)AssocReq WPS IE (in AP; %NULL if not AP)
	 */
	const struct wpabuf *assoc_wps_ie;

	/**
	 * new_ap_settings - New AP settings (%NULL if not used)
	 *
	 * This parameter provides new AP settings when using a wireless
	 * stations as a Registrar to configure the AP. %NULL means that AP
	 * will not be reconfigured, i.e., the station will only learn the
	 * current AP settings by using AP PIN.
	 */
	const struct wps_credential *new_ap_settings;

	/**
	 * peer_addr: MAC address of the peer in AP; %NULL if not AP
	 */
	const atbm_uint8 *peer_addr;

	/**
	 * use_psk_key - Use PSK format key in Credential
	 *
	 * Force PSK format to be used instead of ASCII passphrase when
	 * building Credential for an Enrollee. The PSK value is set in
	 * struct wpa_context::psk.
	 */
	int use_psk_key;

	/**
	 * dev_pw_id - Device Password ID for Enrollee when PIN is used
	 */
	atbm_uint16 dev_pw_id;

	/**
	 * p2p_dev_addr - P2P Device Address from (Re)Association Request
	 *
	 * On AP/GO, this is set to the P2P Device Address of the associating
	 * P2P client if a P2P IE is included in the (Re)Association Request
	 * frame and the P2P Device Address is included. Otherwise, this is set
	 * to %NULL to indicate the station does not have a P2P Device Address.
	 */
	const atbm_uint8 *p2p_dev_addr;

	/**
	 * pbc_in_m1 - Do not remove PushButton config method in M1 (AP)
	 *
	 * This can be used to enable a workaround to allow Windows 7 to use
	 * PBC with the AP.
	 */
	int pbc_in_m1;
};
#endif
struct eap_wsc_data
{
	WPS_STATE state;
	int registrar;
	struct wpabuf *in_buf;
	struct wpabuf *out_buf;
	enum wsc_op_code in_op_code, out_op_code;
	atbm_size_t out_used;
	atbm_size_t fragment_size;
	struct wps_data *wps;
	struct wps_context *wps_ctx;
};

#if 0
/**
 * union wps_event_data - WPS event data
 */
union wps_event_data {
	/**
	 * struct wps_event_m2d - M2D event data
	 */
	struct wps_event_m2d {
		atbm_uint16 config_methods;
		const atbm_uint8 *manufacturer;
		atbm_size_t manufacturer_len;
		const atbm_uint8 *model_name;
		atbm_size_t model_name_len;
		const atbm_uint8 *model_number;
		atbm_size_t model_number_len;
		const atbm_uint8 *serial_number;
		atbm_size_t serial_number_len;
		const atbm_uint8 *dev_name;
		atbm_size_t dev_name_len;
		const atbm_uint8 *primary_dev_type; /* 8 octets */
		atbm_uint16 config_error;
		atbm_uint16 dev_password_id;
	} m2d;

	/**
	 * struct wps_event_fail - Registration failure information
	 * @msg: enum wps_msg_type
	 */
	struct wps_event_fail {
		int msg;
		atbm_uint16 config_error;
		atbm_uint16 error_indication;
	} fail;

	struct wps_event_pwd_auth_fail {
		int enrollee;
		int part;
	} pwd_auth_fail;

	struct wps_event_er_ap {
		const atbm_uint8 *uuid;
		const atbm_uint8 *mac_addr;
		const char *friendly_name;
		const char *manufacturer;
		const char *manufacturer_url;
		const char *model_description;
		const char *model_name;
		const char *model_number;
		const char *model_url;
		const char *serial_number;
		const char *upc;
		const atbm_uint8 *pri_dev_type;
		atbm_uint8 wps_state;
	} ap;

	struct wps_event_er_enrollee {
		const atbm_uint8 *uuid;
		const atbm_uint8 *mac_addr;
		int m1_received;
		atbm_uint16 config_methods;
		atbm_uint16 dev_passwd_id;
		const atbm_uint8 *pri_dev_type;
		const char *dev_name;
		const char *manufacturer;
		const char *model_name;
		const char *model_number;
		const char *serial_number;
	} enrollee;

	struct wps_event_er_ap_settings {
		const atbm_uint8 *uuid;
		const struct wps_credential *cred;
	} ap_settings;

	struct wps_event_er_set_selected_registrar {
		const atbm_uint8 *uuid;
		int sel_reg;
		atbm_uint16 dev_passwd_id;
		atbm_uint16 sel_reg_config_methods;
		enum {
			WPS_ER_SET_SEL_REG_START,
			WPS_ER_SET_SEL_REG_DONE,
			WPS_ER_SET_SEL_REG_FAILED
		} state;
	} set_sel_reg;
};
#endif
//Enrollee Device Password PIN Code Length
#define PIN_CODE_LENGTH 8


 atbm_void eap_wsc_state(struct eap_wsc_data *data, int state);
 struct wpabuf * wpas_eap_wsc_process(struct wpa_supplicant *wpa_s, const struct wpabuf *reqData);
 int atbmwps_start_pbc(struct atbmwifi_vif *priv, atbm_uint8 *p2p_info);
 int atbmwps_start_pin(struct atbmwifi_vif *priv, const char *pin, atbm_uint8 *buf, atbm_uint32 info);
 int atbmwps_cancel(struct atbmwifi_vif *priv);
 int atbmwps_deinit(struct atbmwifi_vif *priv);
 int atbmwps_init(struct atbmwifi_vif *priv);

#endif
