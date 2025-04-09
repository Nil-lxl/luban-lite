/**************************************************************************************************************
 * altobeam RTOS API
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#ifndef ATBM_WIFI_DRIVER_API_H
#define ATBM_WIFI_DRIVER_API_H
#include "atbm_type.h"
#ifdef __cplusplus
extern "C" {
#endif
	

//typedef int int32;
//typedef unsigned int uint32;
typedef atbm_void* wl_drv_hdl;

/////////////////////////////////////////////////////////////////////////////////

/////////////////////the following struct copy from wlan_ATBM.h, will delete after include wlan_ATBM.h

/////////////////////////////////////////////////////////////////////////////////

typedef enum __ATBM_WIFI_MODE
{
	ATBM_WIFI_STA_MODE,
	ATBM_WIFI_AP_MODE,
	ATBM_WIFI_ADHOC,
	ATBM_WIFI_MONITOR,
	ATBM_WIFI_P2P_CLIENT,
	ATBM_WIFI_P2P_GO
}ATBM_WIFI_MODE;
enum {
	WLAN_CHANNEL_1 = 1,
	WLAN_CHANNEL_2,
	WLAN_CHANNEL_3,
	WLAN_CHANNEL_4,
	WLAN_CHANNEL_5,
	WLAN_CHANNEL_6,
	WLAN_CHANNEL_7,
	WLAN_CHANNEL_8,
	WLAN_CHANNEL_9,
	WLAN_CHANNEL_10,
	WLAN_CHANNEL_11,
	WLAN_CHANNEL_12,
	WLAN_CHANNEL_13
};
typedef int WLAN_CHANNEL;

typedef enum {
	/* CCK rate. */	
	WLAN_RATE_1M	= 2,
	WLAN_RATE_2M	= 4,
	WLAN_RATE_5M5	= 11,
	WLAN_RATE_11M	= 22,

	/* OFDM Rate. */	   
	WLAN_RATE_6M	= 12,
	WLAN_RATE_9M	= 18,
	WLAN_RATE_12M   = 24,
	WLAN_RATE_18M	= 36,
	WLAN_RATE_24M	= 48,
	WLAN_RATE_36M	= 72,
	WLAN_RATE_48M	= 96,
	WLAN_RATE_54M	= 108,

	/* HT Rate. */	
	WLAN_MCS_RATE_0 = 128,			/*MCS0 128*/
	WLAN_MCS_RATE_1,
	WLAN_MCS_RATE_2,
	WLAN_MCS_RATE_3,
	WLAN_MCS_RATE_4,
	WLAN_MCS_RATE_5,
	WLAN_MCS_RATE_6,
	WLAN_MCS_RATE_7,				/*MCS7 135*/
	WLAN_MCS_RATE_8,
	WLAN_MCS_RATE_9,
	WLAN_MCS_RATE_10,
	WLAN_MCS_RATE_11,
	WLAN_MCS_RATE_12,
	WLAN_MCS_RATE_13,
	WLAN_MCS_RATE_14,
	WLAN_MCS_RATE_15,
	WLAN_MCS_RATE_16,
	WLAN_MCS_RATE_17,
	WLAN_MCS_RATE_18,
	WLAN_MCS_RATE_19,
	WLAN_MCS_RATE_20,
	WLAN_MCS_RATE_21,
	WLAN_MCS_RATE_22,
	WLAN_MCS_RATE_23,
	WLAN_MCS_RATE_24,
	WLAN_MCS_RATE_25,
	WLAN_MCS_RATE_26,
	WLAN_MCS_RATE_27,
	WLAN_MCS_RATE_28,
	WLAN_MCS_RATE_29,
	WLAN_MCS_RATE_30,
	WLAN_MCS_RATE_31				/*MCS31 159*/
}WLAN_RATE;


/* Supported authentication mode. */
/* Values are used to select the authentication mode used to join a network. */
enum {
	WLM_WPA_AUTH_DISABLED = 0x0000,	/* Legacy (i.e., non-WPA) */
	WLM_WPA_AUTH_NONE = 0x0001,		/* none (IBSS) */
	WLM_WPA_AUTH_PSK = 0x0004,		/* Pre-shared key */
	WLM_WPA2_AUTH_PSK = 0x0080		/* Pre-shared key */
};
typedef int WLM_AUTH_MODE;

/* WLAN Security Encryption. */
/* Values are used to select the type of encryption used for testing. */
enum {
	WLM_ENCRYPT_NONE = 0,    /* No encryption. */
	WLM_ENCRYPT_WEP = 1,     /* WEP encryption. */
	WLM_ENCRYPT_TKIP = 2,    /* TKIP encryption. */
	WLM_ENCRYPT_AES = 4,     /* AES encryption. */
	WLM_ENCRYPT_WSEC = 8,    /* Software WSEC encryption. */
	WLM_ENCRYPT_FIPS = 0x80  /* FIPS encryption. */
};
typedef int WLM_ENCRYPTION;


#define WLAN_MCSSET_LEN				16 
typedef struct _WLAN_BSS_INFO
{
    atbm_uint32      version;        /* version field */
    atbm_uint32      length;         /* byte length of data in this record,
                                 * starting at version and including IEs
                                 */
    atbm_uint8       BSSID[6];
    atbm_uint16      beacon_period;  /* units are Kusec */
    atbm_uint16      capability;     /* Capability information */
    atbm_uint8       SSID_len;
    atbm_uint8       SSID[32];

  //  struct {
  //      uint32  count;          /* # rates in this set */
  //      atbm_uint8   rates[16];      /* rates in 500kbps units w/hi bit set if basic */
  //  } rateset;                  /* supported rates */

    atbm_uint8/*CHANSPEC*/    chanspec;       /* chanspec for bss */
  //  uint16      atim_window;    /* units are Kusec */
    atbm_uint8       dtim_period;    /* DTIM period */
    atbm_int8       RSSI;           /* receive signal strength (in dBm) */
    atbm_int8        phy_noise;      /* noise (in dBm) */
    atbm_uint8       n_cap;          /* BSS is 802.11N Capable */
    atbm_uint32      nbss_cap;       /* 802.11N BSS Capabilities (based on HT_CAP_*) */
    atbm_uint8       ctl_ch;         /* 802.11N BSS control channel number */
 //   uint32      reserved32[1];  /* Reserved for expansion of BSS properties */
 //   atbm_uint8       flags;          /* flags */
  //  atbm_uint8       reserved[3];    /* Reserved for expansion of BSS properties */
   // atbm_uint8       basic_mcs[WLAN_MCSSET_LEN];  /* 802.11N BSS required MCS set */
  //  uint16      ie_offset;      /* offset at which IEs start, from beginning */
  //  uint32      ie_length;      /* byte length of Information Elements */
  //  int16       SNR;            /* average SNR of during frame reception */
    /* Add new fields here */
    /* variable length Information Elements */
	atbm_int8	security;	/*return type ATBM_SECURITY_TYPE, -1 not recognized*/
} WLAN_BSS_INFO;

typedef struct wl_connection_info
{
	atbm_uint8	Ssid_len;
	atbm_uint8	Ssid[32];
	int	 	Rssi;
	int		Phy_rate;
	int 	channel;
} wl_connection_info_t;

typedef struct _WLAN_CONNECTION_INFO
{
	atbm_uint8   Ssid_len;
	atbm_uint8   Ssid[32];
	atbm_int32   Rssi;
	atbm_int32   Phy_rate;
	atbm_int32   channel;
} WLAN_CONNECTION_INFO;

/* Supported authentication mode. */
/* Values are used to select the authentication mode used to join a network. */
enum {
	WLAN_WPA_AUTH_DISABLED = 0x0000,	/* Legacy (i.e., non-WPA) */
	WLAN_WPA_AUTH_NONE = 0x0001,		/* none (IBSS) */
	WLAN_WPA_AUTH_PSK = 0x0004,		/* Pre-shared key */
	WLAN_WPA2_AUTH_PSK = 0x0080,		/* Pre-shared key */
	WLAN_MIX_AUTH_PSK = 0x0100,	        /* Pre-shared key */
	WLAN_ENCRYPT_WEP_SHARED = 0x0200,
	WLAN_WPA_AUTH_SAE = 0x0400,	        /* SAE */
};
typedef int WLAN_AUTH_MODE;


/* WLAN Security Encryption. */
/* Values are used to select the type of encryption used for testing. */
enum {
	WLAN_ENCRYPT_NONE = 0,    /* No encryption. */
	WLAN_ENCRYPT_WEP = 1,     /* WEP encryption. */
	WLAN_ENCRYPT_TKIP = 2,    /* TKIP encryption. */
	WLAN_ENCRYPT_AES = 4,     /* AES encryption. */
	WLAN_ENCRYPT_WSEC = 8,    /* Software WSEC encryption. */
	WLAN_ENCRYPT_FIPS = 0x80  /* FIPS encryption. */
};
typedef int WLAN_ENCRYPTION;


typedef struct _WLAN_SCAN_RESULT {
	atbm_uint32 buflen;
	atbm_uint32 version;
	atbm_uint32 count;
	WLAN_BSS_INFO bss_info[1];
} WLAN_SCAN_RESULT;

typedef enum _ATBM_SECURITY_TYPE
{
	ATBM_KEY_NONE = 0,
	ATBM_KEY_WEP,
	ATBM_KEY_WEP_SHARE,
	ATBM_KEY_WPA,
	ATBM_KEY_WPA2,
	ATBM_KEY_MIX,
	ATBM_KEY_SAE,
	ATBM_KEY_SAE_COMPIT,
	ATBM_KEY_MAX,
}ATBM_SECURITY_TYPE;

typedef struct _ATBM_WLAN_CONNECTION_INFO
{
	atbm_uint8   Ssid_len;
	atbm_uint8   Ssid[32];
	atbm_int32   Rssi;
	atbm_int32   Phy_rate;
	atbm_int32   channel;
} ATBM_WLAN_CONNECTION_INFO;

typedef struct _WLAN_ETHER_ADDR
{
	atbm_uint8 mac[6];
}WLAN_ETHER_ADDR;

typedef struct _WLAN_MACLIST
{
	atbm_uint32 count;			   // number of MAC addresses 
	WLAN_ETHER_ADDR ea[ATBMWIFI__MAX_STA_IN_AP_MODE];	   // variable length array of MAC addresses 
}WLAN_MACLIST;

typedef struct _FAST_LINK_INFO{
	atbm_int32 enable;
	atbm_int32 ssid_offset;
	atbm_int32 psk_offset;
	atbm_uint8 config[200];
	atbm_uint8 bss[600];
	atbm_uint8 ie[350];
}FAST_LINK_INFO;

/////////////////////////////////////////////////////////////////////////////////

/////////////////////end  wlan_ATBM.h defines

/////////////////////////////////////////////////////////////////////////////////


/****************************************************************************
* Function:   	atbm_wifi_hw_init
*
* Purpose:   	This function is used to initialize and start atbm wifi  hardware.
			may be GPO, BUS PROBE, firmware init etc.
*
* Parameters: none
*
* Returns:	Returns driver handle, otherwise a NULL pointer.
****************************************************************************/
atbm_int32  atbm_wifi_hw_init(atbm_void);

/****************************************************************************
* Function:   	atbm_wifi_hw_deinit
*
* Purpose:   	This function is used to release and clean up the driver
*
* Parameters: none
*
* Returns:	Returns 0 if succeed, otherwise a negative error code.
****************************************************************************/
atbm_int32  atbm_wifi_hw_deinit(atbm_void);


/****************************************************************************
* Function:   	atbm_wifi_on
*
* Purpose:   	This function is used to initialize and start atbm wifi  module as AP mode or STA mode.                     
*
* Parameters: AP_sta_mode     0: Ap Mode, 1 STA mode
*
* Returns:	Returns 0 if succeed, otherwise a negative error code.
****************************************************************************/
atbm_void* atbm_wifi_on(ATBM_WIFI_MODE AP_sta_mode);
atbm_void* atbm_wifi_on_vif(ATBM_WIFI_MODE AP_sta_mode,atbm_uint8 if_id);


/****************************************************************************
* Function:   	atbm_wifi_off
*
* Purpose:   	This function is used to stop atbm wifi  module.
*
* Returns:	Returns none.
*****************************************************************************/
atbm_void  atbm_wifi_off(atbm_uint8 if_id);
atbm_void  atbm_wifi_off_vif(atbm_uint8 if_id);


/****************************************************************************
* Function:   	atbm_wifi_scan_network
*
* Purpose:   	This function is used to ask driver to perform channel scan and return scan result.
*
* Parameters: scan_buf		Buffer to store the information of the found APs
*			buf_size		Size of the buffer
*
* Returns:	Returns 0 if succeed, otherwise a negative error code.
******************************************************************************/
int atbm_wifi_scan_network(char* scan_buf, atbm_uint32 buf_size);



/****************************************************************************
* Function:   	atbm_wifi_get_mode
*
* Purpose:   	This function is used to get  wifi mode 
*
* Parameters: None
*
* Returns:	Returns 0 if in STA mode, 1 in SW AP mode.
******************************************************************************/
atbm_int32 atbm_wifi_get_current_mode(void);   //0 : sta, 1: SW AP
atbm_int32 atbm_wifi_get_current_mode_vif(atbm_uint8 if_id);   //0 : sta, 1: SW AP



/****************************************************************************
* Function:   	atbm_wifi_get_mac_address
*
* Purpose:   	This function is used to get wifi MAC addressi
*
* Parameters: point to buffer of MAC address
*
* Returns:	None.
******************************************************************************/
#ifndef ATBM_COMB_IF
atbm_void atbm_wifi_get_mac_address(unsigned char *addr);
#else
atbm_void atbm_wifi_get_mac_address(atbm_uint8 if_id,unsigned char *addr);
#endif

atbm_int32 wifi_ConnectAP_vif(atbm_uint8 if_id,atbm_uint8 * ssid,int ssidlen,atbm_uint8 * password,
		int passwdlen,ATBM_SECURITY_TYPE key_mgmt);
atbm_void wifi_StartAP_vif(atbm_uint8 if_id,atbm_uint8 * ssid,int ssidlen,atbm_uint8 * password,
	int passwdlen,int channel,ATBM_SECURITY_TYPE key_mgmt,ATBM_BOOL ssidBcst);
/****************************************************************************
* Function:   	atbm_wifi_tx_pkt
*
* Purpose:   	This function is used to send packet to wifi driver
*
* Parameters: point to buffer of packet
*
* Returns:	None.
******************************************************************************/
#ifndef ATBM_COMB_IF 
atbm_int32 atbm_wifi_tx_pkt(atbm_int8 *pbuf, atbm_uint32 pktlen, atbm_int8 if_id);
#else
atbm_int32 atbm_wifi_tx_pkt(atbm_int8 *pbuf, atbm_uint32 pktlen, atbm_int8 if_id);
#endif


//void atbm_wifi_rx_pkt(netif, total_len);   //not required here ,   lwip_tcp_opt.net_rx = ethernetif_input.



/****************************************************************************
* Function:   	atbm_wifi_get_avail_tx_queue_count
*
* Purpose:   	This function is used to return the available buffer count of driver TX queue
*
* Parameters: None
*
* Returns:	Returns number of available buffer count of driver TX queue.
******************************************************************************/
atbm_int32 atbm_wifi_get_avail_tx_queue_count(atbm_void);




/****************************************************************************
* Function:     atbm_wifi_sta_join_ap
*
* Purpose:      This function is used to ask driver to join a netwrok.
*
* Parameters: ssid          SSID of the AP used to join a network
*            authMode   authentication mode used to join a network
*            encryption encryption mode used to join a network
*            key            passphrase used to join a network
*
* Returns:  Returns 0 if succeed, otherwise a negative error code.
******************************************************************************/
atbm_int32 atbm_wifi_sta_join_ap(char *ssid, char *bssid, WLAN_AUTH_MODE authMode, WLAN_ENCRYPTION encryption, char *key);

/****************************************************************************
* Function:   	atbm_wifi_get_driver_version
*
* Purpose:   	This function is used to return the driver's release version
*
* Parameters: None
*
* Returns:	Returns the driver release version
*****************************************************************************
*/
signed char* atbm_wifi_get_driver_version(atbm_void);



/****************************************************************************
* Function:     wlan_get_connection_info
*
* Purpose:      This function is used to get the current connection information at STA mode
*
* Parameters: wlan connection information
*
* Returns:  Returns 0 if succeed, otherwise a negative error code.

typedef struct _WLAN_CONNECTION_INFO
{
    atbm_uint8   Ssid_len;
    atbm_uint8   Ssid[32];
    int32   Rssi;
    int32   Phy_rate;
    int32   channel;
} WLAN_CONNECTION_INFO;
*****************************************************************************
*/
atbm_int32 atbm_wifi_get_connected_info(ATBM_WLAN_CONNECTION_INFO *wlan_connection_info);
atbm_int32 atbm_wifi_get_connected_info_vif(atbm_uint8 if_id,ATBM_WLAN_CONNECTION_INFO *wlan_connection_info);
/****************************************************************************
* Function:   	wlan_get_connection_info
*
* Purpose:   	This function is used to get the current connection information at STA mode
*
* Parameters: wlan connection information
*
* Returns:	Returns 0 if succeed, otherwise a negative error code.
****************************************************************************/














/*************************************************************************************

**************                SW AP functions                                                                        *********

**************************************************************************************/

/****************************************************************************
* Function:     atbm_wifi_ap_create
*
* Purpose:      This function is used to create a SW AP network
*
* Parameters: ssid          SSID of the SW AP to be created
*            authMode   Authentication mode used for the SW AP
*            encryption Encryption mode used for the SW AP
*            key            Passphrase used for the SW AP
*            channel        Channle used for the SW AP
*            ssidBcst       0: to broadcast SSID, 1: to hide SSID
*
* Returns:  Returns 0 if succeed, otherwise a negative error code.
*****************************************************************************
*/
atbm_int32 atbm_wifi_ap_create(char* ssid, int authMode, int encryption, 
							char *key, int channel, ATBM_BOOL ssidBcst );
atbm_int32 atbm_wifi_ap_create_vif(atbm_uint8 if_id,char* ssid, int authMode, int encryption, 
							char *key, int channel, ATBM_BOOL ssidBcst );



/****************************************************************************
* Function:     wlan_get_assoc_list
*
* Purpose:      This function is used to the associated client list in SW AP mode
*
* Parameters: buf           The buffer to store the associated client list
*                    uiBufSize       size of the buffer
*
* Returns:  Returns 0 if succeed, otherwise a negative error code.

* For ioctls that take a list of MAC addresses *
typedef struct _WLAN_MACLIST
{
	uint32 count;              // number of MAC addresses 
	WLAN_ETHER_ADDR ea[1];     // variable length array of MAC addresses 
}WLAN_MACLIST;
******************************************************************************/
	
atbm_int32 atbm_wifi_get_associated_client_list(atbm_uint8 *pchBuf, atbm_uint32 uiBufSize);






atbm_int32 atbm_wifi_set_256BITSEFUSE(atbm_uint8 *data, atbm_uint32 length);
atbm_int32 atbm_wifi_get_256BITSEFUSE(atbm_uint8 *data, atbm_uint32 length);







/*************************************************************************************

**************                  Manufacturing test functions                                                     *********

**************************************************************************************/
	
atbm_void  atbm_wifi_mfg_start(void);


/**************************************************************************************
* Function:     atbm_wifi_mfg_set_pktTxBG
*
* Purpose:      This function is used to perform manufacturing 11b/g continuous TX test
*
* Parameters:   channel       Channel used for TX
*               	  rate          11b/g rate used for TX
*               	  powerValue    Output power index, -1 means default power
*
* Returns:      Returns 0 if succeed, otherwise a negative error code.
****************************************************************************************/
atbm_int32 atbm_wifi_mfg_set_pktTxBG(WLAN_CHANNEL channel, WLAN_RATE rate, atbm_int32 powerValue);


/***********************************************************************************
* Function:     atbm_wifi_mfg_set_PktTxN
*
* Purpose:      This function is used to perform manufacturing 11n continuous TX test
*
* Parameters:   channel       Channel used for TX
*              	   rate          11n rate used for TX
*               	  powerValue    Output power index, -1 means default power
*
* Returns:      Returns 0 if succeed, otherwise a negative error code.
************************************************************************************/
atbm_int32 atbm_wifi_mfg_set_PktTxN(WLAN_CHANNEL channel, WLAN_RATE rate, atbm_int32 powerValue);


/****************************************************************************
* Function:     atbm_wifi_mfg_CarrierTone
*
* Purpose:      This function is used to perform manufacturing non-modulation TX test
*
* Parameters:   channel       Channel used for test
*
* Returns:      Returns 0 if succeed, otherwise a negative error code.
******************************************************************************/
atbm_int32 atbm_wifi_mfg_CarrierTone(WLAN_CHANNEL channel);


/****************************************************************************
* Function:     atbm_wifi_mfg_set_PktRxMode
*
* Purpose:      This function  is used to perform manufacturing RX test
*
* Parameters:   channel     Channel used for RX
*
* Returns:      Returns 0 if succeed, otherwise a negative error code.
******************************************************************************/
atbm_int32 atbm_wifi_mfg_set_PktRxMode(WLAN_CHANNEL channel);


/****************************************************************************
* Function:     atbm_wifi_mfg_get_RxPkt
*
* Purpose:      This function is used to get received packet count
*
* Parameters:   uiCount     Received packet count
*
* Returns:      Returns 0 if succeed, otherwise a negative error code.
******************************************************************************/
atbm_int32 atbm_wifi_mfg_get_RxPkt(atbm_uint32* uiCount);



/****************************************************************************
* Function:     atbm_wifi_mfg_stop
*
* Purpose:      This function is used to stop manufacturing test
*
* Parameters:   None
*
* Returns:      Returns 0 if succeed, otherwise a negative error code.
******************************************************************************/
atbm_int32 atbm_wifi_mfg_stop(atbm_void);

int atbm_wifi_etf_start_tx(int channel,int rate_value,int is_40M, int greedfiled);
int atbm_wifi_etf_stop_tx(void);
int atbm_wifi_etf_start_rx(int channel ,int is_40M);
int atbm_wifi_etf_stop_rx(void);


int atbm_wifi_isconnected(atbm_uint8 if_id);
int atbm_wifi_sta_disjoin_ap(void);
int atbm_akwifi_setup_sdio(void);
ATBM_NETIF *atbm_priv_get_netif(struct atbm_net_device *dev);
atbm_uint8 atbm_get_wifimode(struct atbmwifi_vif *priv);
atbm_uint8 atbm_smartconfig_start(void);
atbm_uint8 atbm_smartconfig_stop(void);


atbm_int32 atbm_wifi_set_fast_connect_mode(atbm_uint8 enable, atbm_uint8 channel, atbm_uint8 *pmk);
atbm_int32 atbm_wifi_get_fast_connect_info(atbm_uint8 *channel, atbm_uint8 *pmk);

//First connect:config serve as ssid while bss as password
atbm_int32 atbm_wifi_fast_link_noscan(FAST_LINK_INFO * finfo);
atbm_void atbm_wifi_get_linkinfo_noscan(FAST_LINK_INFO * finfo);

atbm_uint8 atbm_wpspbc_start(ATBM_WIFI_MODE AP_sta_mode);
atbm_uint8 atbm_wpspin_start(ATBM_WIFI_MODE AP_sta_mode,const char *pin);
atbm_uint8 atbm_wpsmode_cancel(ATBM_WIFI_MODE AP_sta_mode);

int atbm_wifi_p2p_start(void);
int atbm_wifi_p2p_find(int timeout);
int atbm_wifi_p2p_find_accept(int go_intent);
int atbm_wifi_p2p_find_stop(int timeout);
int atbm_wifi_p2p_listen_accept(int go_intent, int channel);
int atbm_wifi_p2p_show_peers(void);
int atbm_wifi_p2p_go_start(void);
int atbm_wifi_p2p_connect(atbm_uint8 *mac, int go_intent);
int atbm_wifi_p2p_stop(void);

#ifdef __cplusplus
	}
#endif

#endif	/* ATBM_WIFI_H */

