/*
 *  Header files of wps_protocol_handler
 *
 */

#ifndef _WPS_PROTOCOL_HANDLER_H_
#define _WPS_PROTOCOL_HANDLER_H_

#define KEY_NONCE_LEN			32
#define KEY_RC_LEN				8
#define KEY_IV_LEN				16
#define KEY_RSC_LEN				8
#define KEY_ID_LEN				8
#define KEY_MIC_LEN				16
#define KEY_MATERIAL_LEN		2

#define ETHER_HDRLEN			14
#define LIB1X_EAPOL_HDRLEN		4
#define LIB1X_EAPOL_MESSAGE_HDRLEN		5

#define LIB1X_ETHER_EAPOL_TYPE	0x888E

#define LIB1X_EAPOL_VER				1	//0000 0001B
#define LIB1X_EAPOL_EAPPKT			0       //0000 0000B
#define LIB1X_EAPOL_START			1	//0000 0001B
#define LIB1X_EAPOL_LOGOFF			2       //0000 0010B
#define LIB1X_EAPOL_KEY				3	//0000 0011B
#define LIB1X_EAPOL_ENCASFALERT			4	//0000 0100B

#define LIB1X_EAP_CODE_REQUEST			(1)
#define LIB1X_EAP_CODE_RESPONSE			(2)
#define LIB1X_EAP_CODE_FAIL			(4)

#define LIB1X_EAP_TYPE_IDENTITY			(1)
#define LIB1X_EAP_TYPE_NOTIFICATION		(2)
#define LIB1X_EAP_TYPE_NAK			(3)
#define LIB1X_EAP_TYPE_EXPANDED			(254)	/* Wi-Fi Simple Configuation(WSC) */

#define LIB1X_EAP_OPCODE_WSC_START		(0x01)
#define LIB1X_EAP_OPCODE_WSC_ACK		(0x02)
#define LIB1X_EAP_OPCODE_WSC_NACK		(0x03)
#define LIB1X_EAP_OPCODE_WSC_MSG		(0x04)
#define LIB1X_EAP_OPCODE_WSC_DONE		(0x05)
#define LIB1X_EAP_OPCODE_WSC_FRAG_ACK		(0x06)

#define WPS_START_DELAY_TIME    1000
#define WPS_START_RESEND_TIME   10000

struct rtw_wps_context
{
  	/* WPS State Machine */
        unsigned char clientWpsDone; 
  	unsigned char clientWpsProcessing;
	
	/* Avoid handle duplicated message */
	unsigned char record_last_message_type;

	/* For handling fragmented messages */
	unsigned char last_message_has_more_frag;
		
	unsigned char pad_aligned;
	unsigned char identifier;

	/* AP passphrase */
	char passphrase[65];

	/* Keep wps process info */
	struct eap_wsc_data *wsc_data; 
	struct wps_context * wps;

	/* timer to delay send eapol start */
	_timer wps_start_timer;
};

struct _LIB1X_EAPOL_WSC 
{
	unsigned char 		Code;
	unsigned char 		Identifier;
	unsigned short 		Length;
	
	unsigned char 		Type;
	unsigned char 		Vendor_Id[3];
	
	unsigned int 		Vendor_Type;
	
	unsigned char 		OpCode;
	unsigned char 		Flags;
	unsigned short 		Message_Length;
	
	unsigned char 		*Message_Data;
};

struct lib1x_eapol_message_hdr 
{
	unsigned char 		Code;
	unsigned char 		Identifier;
	unsigned short 		Length;
	unsigned char 		Type;
};

typedef struct _LIB1X_EAPOL_KEY
{
 	unsigned char		key_desc_ver;
 	unsigned char		key_info[2];
 	unsigned char		key_len[2];
 	unsigned char		key_replay_counter[KEY_RC_LEN];
 	unsigned char		key_nounce[KEY_NONCE_LEN];
 	unsigned char		key_iv[KEY_IV_LEN];
 	unsigned char		key_rsc[KEY_RSC_LEN];
 	unsigned char		key_id[KEY_ID_LEN];
 	unsigned char		key_mic[KEY_MIC_LEN];
 	unsigned char		key_data_len[KEY_MATERIAL_LEN];
 	unsigned char		*key_data;
} lib1x_eapol_key;

struct lib1x_eapol
{
 	unsigned char		protocol_version;
 	unsigned char		packet_type;			// This makes it odd in number !
 	unsigned short		packet_body_length;
};

struct wlan_ethhdr_t
{
	unsigned char		daddr[ETH_ALEN];
	unsigned char		saddr[ETH_ALEN];
	unsigned short		type;
};

extern struct rtw_wps_context g_wps_context; 

extern xqueue_handle_t queue_for_credential;

int wpas_wps_dev_config(u8 *dev_addr, u8 bregistrar);
int wpas_wps_init(const char* ifname);
void wpas_wps_deinit(void);
int wpas_wps_registrar_button_pushed(void);
int wpas_wps_registrar_check_done(void);
int wpas_wps_registrar_add_pin(const u8 *pin, size_t pin_len);
int wpas_wps_registrar_wps_cancel(void);
int wpas_wps_enrollee_init_assoc_ie(void);
int wpas_wps_enrollee_init_probe_ie(u16 config_methods);
void wpas_wsc_sta_wps_start_hdl(char *buf, int buf_len, int flags, void *userdata);
void wpas_wsc_wps_finish_hdl(char *buf, int buf_len, int flags, void *userdata);
void wpas_wsc_eapol_recvd_hdl(char *buf, int buf_len, int flags, void *userdata);

unsigned int wps_pin_valid(unsigned int pin);
unsigned int wps_generate_pin(void);
//#endif //CONFIG_WPS
#endif //_WPS_PROTOCOL_HANDLER_H_
