#ifndef _WLAN_CFG_H_
#define _WLAN_CFG_H_

//#define CONFIG_DEBUG_SUPPORT
#ifdef CFG_ENCRYPT_SUPPORT
#define INCLUDE_WPA_WPA2_PSK
#endif
#define EAP_WPAS_AUTH_SUPPORT

#endif //_WLAN_CFG_H_