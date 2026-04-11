/**
 ****************************************************************************************
 *
 * @file uwifi_common.h
 *
 * @brief api of common define
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#ifndef _UWIFI_COMMON_H_
#define _UWIFI_COMMON_H_

#include <stdint.h>
#include "uwifi_types.h"
#include "asr_config.h"

#define MAX_PBUF_NUM_A_PACKET    6
/**
 ****************************************************************************************
 * Compare two MAC addresses.
 * @param[in] addr1_ptr Pointer to the first MAC address.
 * @param[in] addr2_ptr Pointer to the second MAC address.
 * @return True if equal, false if not.
 ****************************************************************************************
 */
#define MAC_ADDR_CMP_6(addr1_ptr, addr2_ptr)                                            \
    ((*(((uint8_t*)(addr1_ptr)) + 0) == *(((uint8_t*)(addr2_ptr)) + 0)) &&          \
     (*(((uint8_t*)(addr1_ptr)) + 1) == *(((uint8_t*)(addr2_ptr)) + 1)) &&           \
     (*(((uint8_t*)(addr1_ptr)) + 2) == *(((uint8_t*)(addr2_ptr)) + 2)) &&           \
     (*(((uint8_t*)(addr1_ptr)) + 3) == *(((uint8_t*)(addr2_ptr)) + 3)) &&           \
     (*(((uint8_t*)(addr1_ptr)) + 4) == *(((uint8_t*)(addr2_ptr)) + 4)) &&           \
     (*(((uint8_t*)(addr1_ptr)) + 5) == *(((uint8_t*)(addr2_ptr)) + 5)))

//struct eth_addr
//{
//    uint8_t addr[6];
//};

typedef enum
{
    COEX_MODE_STA = 0,
#if NX_STA_AP_COEX
    COEX_MODE_AP = 1,
#else
    COEX_MODE_AP = 0,
#endif
    COEX_MODE_MAX,
}COEX_MODE_E;

struct wpa_psk
{
    uint8_t macaddr[ETH_ALEN];
    uint8_t ssid[MAX_SSID_LEN];
    uint8_t ssid_len;
    uint8_t password[MAX_PASSWORD_LEN];
    uint8_t pwd_len;
    uint8_t encrpy_protocol;
    uint32_t cipher_group;
    uint32_t ciphers_pairwise;
#if (defined(CONFIG_IEEE80211W) || NX_SAE)
    uint32_t key_mgmt;
    uint32_t cipher_mgmt_group;
#endif    
    uint8_t wpa_ie[MAX_IE_LEN];
    uint8_t wpa_ie_len;
};

/******************************************************
 *           Structures msg definition
 ******************************************************/
struct uwifi_task_msg_t
{
    uint32_t id;
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
}__attribute__ ((__packed__));

// TCPIP packet info struct
typedef struct _packet_info_tag
{
    uint8_t*          data_ptr[MAX_PBUF_NUM_A_PACKET];   /* data pointer */
    uint16_t          data_len[MAX_PBUF_NUM_A_PACKET];   /* data length */
    uint16_t          pbuf_num;
    uint16_t          packet_len;
} TCPIP_PACKET_INFO_T;

typedef struct _rx_packet_info_tag
{
    uint8_t*    data_ptr;   /* data pointer */
    uint32_t    data_len;   /* data length - full packet encapsulation length */
} RX_PACKET_INFO_T;

struct ethernetif;

typedef int (*tcpip_wifi_xmit_fn)(TCPIP_PACKET_INFO_T    *packet, struct ethernetif *ethernetif);
typedef void (*ethernet_if_input_fn)(struct ethernetif *eth_if,  RX_PACKET_INFO_T *rx_packet);

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
struct ethernetif {
    //struct netif *netif;
    char * netif_name;
    struct wlan_mac *ethaddr;

    ethernet_if_input_fn eth_if_input;
};

//struct asr_hw* uwifi_get_asr_hw(void);

void uwifi_wpa_handshake_start(uint32_t mode);
#endif //_UWIFI_COMMON_H_
