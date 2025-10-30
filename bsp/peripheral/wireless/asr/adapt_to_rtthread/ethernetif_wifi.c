#include "ethernetif_wifi.h"
#include "asr_dbg.h"
#include "uwifi_asr_api.h"
#include "uwifi_wpa_api.h"
#include "uwifi_msg.h"
#include "asr_wlan_api_aos.h"
#include "netif/etharp.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/sockets.h"
#include <netif/ethernetif.h>
#include <netdev.h>
#include <wlan.h>

#include "dhcp_server.h"
extern struct netif *netif_list;

/* Define those to better describe your network interface. */
#define IFNAME0 'w'
#define IFNAME1 'n'

#define ETH_RX_BUF_SIZE_WIFI 1500

#define NET_IPV4_ADDR_LEN INET_ADDRSTRLEN

struct ethernetif g_ethernetif[COEX_MODE_MAX];
struct netif lwip_netif[COEX_MODE_MAX];

uint8_t wifi_ready = WIFI_NOT_READY;
//uint8_t wifi_connect_err = 0;           /* WLAN error status*/
uint8_t wifi_conect_fail_code = 0;
uint8_t wifi_softap_fail_code = 0;
//uint8_t wifi_psk_is_ture = 0;           /*set after 4way handshake*/
uint8_t client_macaddr[6]={0};
// uint32_t g_netif_name[2];
extern struct rt_wlan_device * s_wlan_dev;
extern struct rt_wlan_device * s_ap_dev;

void ethernetif_input(struct ethernetif *eth_if,  RX_PACKET_INFO_T *rx_packet)
{
    struct rt_wlan_device *dev = (struct rt_wlan_device *)eth_if->netif_name;
    if(rt_wlan_dev_report_data(dev, rx_packet->data_ptr, rx_packet->data_len)){
        rt_kprintf("input pkt fail\n");
    }
}

uint8_t lwip_comm_init(enum open_mode openmode)
{
    struct ethernetif *ethernetif;
    struct rt_wlan_device *dev;

    if(openmode == STA_MODE) {
        ethernetif = &g_ethernetif[COEX_MODE_STA];
        dev = s_wlan_dev;
    } else if (openmode == SAP_MODE) {
        ethernetif = &g_ethernetif[COEX_MODE_AP];
        dev = s_ap_dev;
    } else {
        dbg(D_ERR, D_UWIFI_CTRL, "%s <<< unsupported mode", __func__);
        return 0;
    }

    memset(ethernetif, 0, sizeof(struct ethernetif));
    ethernetif->netif_name = (void *)dev;

    //does need copy the netif name?
    dbg(D_ERR, D_UWIFI_CTRL, "netif name:%s", dev->device.parent.name);
    ethernetif->eth_if_input = ethernetif_input;

    return 0;
}


void asr_set_netif_mac_addr(char *mac, int mode)
{

}

void asrnet_comm_dhcp_delete(void)
{

}

void asr_dhcps_start(void)
{
    dhcpd_start(g_ethernetif[COEX_MODE_AP].netif_name);
}

void asr_dhcps_stop(void)
{
    dhcpd_stop(g_ethernetif[COEX_MODE_AP].netif_name);
}

void asrnet_got_ip_action(void)
{
    struct config_info *pst_config_info = uwifi_get_sta_config_info();
    struct wifi_conn_info *pst_conn_info = NULL;

    if (NULL == pst_config_info)
        return;
    else
        pst_conn_info = (struct wifi_conn_info *)((void *)pst_config_info + sizeof(struct config_info));

    if (pst_conn_info->wifiState < WIFI_ASSOCIATED){
        dbg(D_ERR, D_LWIP, "unexpected gotip evt(%d)",pst_conn_info->wifiState);
        return;
    }

    #if 0
    if(WLAN_DHCP_CLIENT == pst_config_info->wlan_ip_stat.dhcp)
    {
        if (dhcpIPLostTimer.handle != NULL && asr_rtos_is_timer_running(&dhcpIPLostTimer))
        {
            asr_rtos_stop_timer(&dhcpIPLostTimer);
        }
    }
    #endif

    asr_wlan_get_mac_address_inchar(pst_config_info->wlan_ip_stat.macaddr);

    //DNS TBD
    pst_config_info->is_connected = 1;

    //if(WIFI_IS_READY != wifi_ready)
    //{
        //set_wifi_ready_status(WIFI_IS_READY);
        asr_wlan_set_wifi_ready_status(WIFI_IS_READY);
    //}

    //asr_wifi_enable_rssi_level_indication();

    uwifi_msg_lwip2u_ip_got((uint32_t)(&(pst_config_info->wlan_ip_stat)));

    uwifi_clear_autoconnect_status(pst_config_info);
}

void asrnet_got_ip_fail_action(void)
{
    struct config_info *pst_config_info = uwifi_get_sta_config_info();

    //if(WIFI_NOT_READY != wifi_ready)
    //{
    //    set_wifi_ready_status(WIFI_NOT_READY);
    //}

    if(WLAN_DHCP_CLIENT == pst_config_info->wlan_ip_stat.dhcp)
    {
#if 0
        // dhcp ip renew fail
        if (dhcpIPLostTimer.handle != NULL && asr_rtos_is_timer_running(&dhcpIPLostTimer))
        {
            asr_rtos_stop_timer(&dhcpIPLostTimer);
        }
        asr_rtos_start_timer(&dhcpIPLostTimer);
#endif
    }
}

void lwip_comm_dhcp_event_handler(enum lwip_dhcp_event evt)
{
    dbg(D_ERR, D_LWIP, "lwip_comm_dhcp_event_handler evt: %d", evt);
}
