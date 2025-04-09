#ifndef __ETHERNETIF_WIFI_H__
#define __ETHERNETIF_WIFI_H__

#include "uwifi_common.h"
#include "uwifi_interface.h"

#define DHCP_IP_LOST_TIMEOUT_MS 10000

uint8_t lwip_comm_init(enum open_mode openmode);

//void ethernetif_input(struct ethernetif *eth_if, RX_PACKET_INFO_T *rx_packet);


void asrnet_got_ip_action(void);


void asr_set_netif_mac_addr(char *mac, int mode);

void asr_dhcps_start(void);

void asr_dhcps_stop(void);

#endif
