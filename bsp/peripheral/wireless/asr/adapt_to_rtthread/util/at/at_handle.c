#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "at_handle.h"
#include "asr_at_api.h"
#include "asr_rtos.h"
#include "asr_wlan_api.h"
#include "uwifi_asr_api.h"
#include "mac.h"
#include "asr_dbg.h"
#include "tasks_info.h"
#include "asr_gpio.h"
#include "asr_sdio.h"


#if defined(LWIP) || defined(ALIOS_SUPPORT)
#include "sockets.h"
#include "ip_addr.h"
#include "lwipopts.h"
#endif


#ifdef LWIP_APP_PING
#include "lwip_app_ping.h"
#include "ping.h"
#endif
#ifdef LWIP_APP_IPERF
#include "lwip_app_iperf.h"
#endif
#include "uwifi_cmds.h"
#include "uwifi_include.h"

#include "uwifi_msg_tx.h"

#define bzero(a,b)          memset(a, 0, b)

#define ARGCMAXLEN 20     // 7
#define AT_CMD_TIMEOUT  3000

#define AT_USER_DEBUG

#ifdef AT_USER_DEBUG
char exit_cmd_mode=0;
#ifdef LWIP_APP_PING
#ifdef LWIP_2_1_2
static ip_addr_t at_ping;
#else
static struct ip_addr at_ping;
#endif
#endif
#define  VLIST_DEBUG_SIZE           1024
 char    v_list[VLIST_DEBUG_SIZE] = {0};
 char    uart_buf[UART_CMD_NB][UART_RXBUF_LEN];
 uint8_t start_idx        = 0;
 uint8_t cur_idx          = 0;
 uint8_t uart_buf_len[UART_CMD_NB] = {0};
 uint8_t uart_idx         = 0;

 int g_asr_gpio_inited = 0;

_at_user_info at_user_info = {0};

int asr_at_wlan_get_err_code(int argc, char **argv);


#ifdef LWIP_APP_IPERF
extern void asr_wifi_iperf(int argc, char **argv);
#endif
int asr_at_iperf(int argc, char **argv);


#ifdef LWIP_APP_PING
extern void api_lwip_do_ping(char *hostname, int ipv4v6, printCallback callback);
extern void api_lwip_stop_ping(void);
#endif



extern int scan_done_flag;
extern uint32_t current_iftype;

static int cli_getchar(u8 *ch)
{
    return 0;
}

int sdio_wifi_open(void)
{
    return 0;
}

void sdio_wifi_close(void)
{
    return;
}

static uint8_t mac_addr_asc_to_hex(char param)
{
    uint8_t val;

    if((param >= 48) && (param <= 57))
        val = param-48;
    else if((param >= 65) && (param <= 70))
        val = param-55;
    else if((param >= 97) && (param <= 102))
        val = param-87;
    else
    {
        LOG_I("wifi_set_mac_addr:error param\r\n");
        val = 0xA;
    }
    return val;
}

static void asr_mac_addr_string_to_array(char *input_str, uint8_t *mac_addr)
{
    int i;
    for(i = 0; i < (MAC_ADDR_LEN<<1); i++)
    {
        if(i%2 || i==1)
            mac_addr[i>>1] = mac_addr_asc_to_hex(input_str[i]) | (mac_addr[i>>1]&0xF0);
        else
            mac_addr[i>>1] = (mac_addr_asc_to_hex(input_str[i])<<4) | (mac_addr[i>>1]&0xF);
    }
}

/*
 * command table struct
 *******************************************************
 */

extern const char *asr_get_wifi_version(void);

static int asr_at_show_version(int argc, char **argv)
{
    LOG_I("wifi version: %s\r\n",asr_get_wifi_version());
    return CONFIG_OK;
}
/*
    do wifi test works
        1, open sta/close sta
        2, open ap/close ap
        3, open sta/close sta or open ap/close ap
*/
#if 0
static int asr_at_wifi_test(int argc, char **argv)
{

    int i;
    asr_wlan_init_type_t conf = {0};
    asr_wlan_ap_init_t init_info = {0};
    conf.wifi_mode = STA;

    dbg(D_ERR, D_UWIFI_CTRL, "0, open mode ( init )");
    asr_wlan_open(&conf);
    // do a scan works
    dbg(D_ERR, D_UWIFI_CTRL, "0, scan mode");
    asr_wlan_start_scan();
    // assign the parameters setting
    strcpy(conf.wifi_ssid, "AP2345");
    strcpy(conf.wifi_key, "12345678");
    // initialize the ap parameter setting
    memset(&init_info, 0, sizeof(asr_wlan_ap_init_t));
    memcpy(init_info.ssid, "asr_test_123", 32);
    init_info.channel = 11;
    memcpy(init_info.pwd, "12345678", 64);
    init_info.interval = 0;
    init_info.hide = 0;
    // 1, sta + ap
    // sta connect to AP
    dbg(D_ERR, D_UWIFI_CTRL, "1, sta + ap mode [w open/close]");
    asr_wlan_open(&conf);
    // close the sta
    asr_wlan_close();
    //
    // open the ap
    asr_wlan_ap_open(&init_info);
    //asr_rtos_delay_milliseconds(2000+rand()%1000);
    // close the ap
    asr_wlan_close();
    dbg(D_ERR, D_UWIFI_CTRL, "2, sta + sta mode [w open/close]");
    // 2, sta + sta
    // sta connect to AP
    asr_wlan_open(&conf);
    // close the sta
    asr_wlan_close();
    //
    // sta connect to AP
    asr_wlan_open(&conf);
    // close the sta
    asr_wlan_close();
    dbg(D_ERR, D_UWIFI_CTRL, "3, ap + ap mode [w open/close]");
    // 3, ap + ap
    // open the ap
    asr_wlan_ap_open(&init_info);
    //asr_rtos_delay_milliseconds(2000+rand()%1000);
    //close the ap
    asr_wlan_close();
    // open the ap
    asr_wlan_ap_open(&init_info);
    //asr_rtos_delay_milliseconds(2000+rand()%1000);
    //close the ap
    asr_wlan_close();
    dbg(D_ERR, D_UWIFI_CTRL, "4, ap + sta mode [w open/close]");
    // 4, ap + sta
    // open the ap
    asr_wlan_ap_open(&init_info);
    //asr_rtos_delay_milliseconds(2000+rand()%1000);
    //close the ap
    asr_wlan_close();
    // sta connect to AP
    asr_wlan_open(&conf);
    // close the sta
    asr_wlan_close();
    // 5, sta + sta without close
    dbg(D_ERR, D_UWIFI_CTRL, "5, sta + sta [w/o open/close]");
    // sta connect to AP
    asr_wlan_open(&conf);
    strcpy(conf.wifi_ssid, "AP23456");
    strcpy(conf.wifi_key, "12345678");
    // sta connect to AP
    asr_wlan_open(&conf);
    // close the sta
    asr_wlan_close();


    return CONFIG_OK;
}
#endif

#define UWIFI_AUTOCONNECT_DEFAULT_TRIES 5
#define UWIFI_AUTOCONNECT_MAX_TRIES 255
static int asr_at_wifi_open(int argc, char **argv)
{
    asr_wlan_init_type_t conf = {0};
    conf.wifi_mode = STA;

    switch(argc)
    {
        case 1:
            break;
        case 2:
            if ((strcmp(argv[1],"sta") == 0))
            {
                if((current_iftype != NON_MODE_) && (current_iftype != STA_MODE))
                {
                    LOG_I("cur mode is %d, close it first!",current_iftype);
                    return CONFIG_FAIL;
                }
                else
                {
                    asr_wlan_open(&conf);
                    return CONFIG_OK;
                }
                break;
            }
            else
            {
                LOG_I("asr_wifi_open:error param\r\n");
                return PARAM_RANGE;
            }
            break;
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            if ((strcmp(argv[1],"sta") == 0))
            {
                if((current_iftype != NON_MODE_) && (current_iftype != STA_MODE))
                {
                    LOG_I("cur mode is %d, close it first!",current_iftype);
                    return CONFIG_FAIL;
                }

                conf.dhcp_mode = WLAN_DHCP_CLIENT;

                if(strlen((char *)argv[2]) > 32)
                {
                    return PARAM_RANGE;
                }

                if((strcmp(argv[0],"wifi_open_bssid") == 0))
                    asr_mac_addr_string_to_array(argv[2], (uint8_t*)conf.mac_addr);
                else
                    strcpy((char*)conf.wifi_ssid, (char*)argv[2]);

                if (argc >= 4)
                {
                    if((strlen(argv[3]) > 4)&&(strlen(argv[3]) < 65))
                        strcpy((char*)conf.wifi_key, (char*)argv[3]);
                    else if((strlen(argv[3]) == 1) && (convert_str_to_int(argv[3]) == 0))
                        memset(conf.wifi_key, 0, 64);
                    else
                    {
                        LOG_I("param4 error, input 0 or pwd(length >= 5)\n");
                        return PARAM_RANGE;
                    }
                }

                if (argc >= 5)
                {
                    conf.channel = convert_str_to_int(argv[4]);
            if (conf.channel == 0)
                        LOG_I("channel 0 indicate full channel scan");
            else if((conf.channel > 14) || (conf.channel < 1))
                    {
                        LOG_I("param5 error,channel range:1~14");
                        return PARAM_RANGE;
                    }
                }

                conf.wifi_retry_times = UWIFI_AUTOCONNECT_DEFAULT_TRIES;
                if(argc >= 6)
                {
                    conf.wifi_retry_times = convert_str_to_int(argv[5]);
                    if (conf.wifi_retry_times == ASR_STR_TO_INT_ERR)
                    {
                        LOG_I("retry times error, should less than 255");
                        return PARAM_RANGE;
                    }
                    if (conf.wifi_retry_times > UWIFI_AUTOCONNECT_MAX_TRIES)
                    {
                        LOG_I("wifi retry time greater than %d, set it to be %d\n",UWIFI_AUTOCONNECT_MAX_TRIES,UWIFI_AUTOCONNECT_MAX_TRIES);
                        conf.wifi_retry_times = UWIFI_AUTOCONNECT_MAX_TRIES;
                    }
                }

                conf.en_autoconnect = AUTOCONNECT_SCAN_CONNECT;   // default enable auto-connect.
                if(argc >= 7)
                {
                    conf.en_autoconnect = convert_str_to_int(argv[6]);
                    if (conf.en_autoconnect >= AUTOCONNECT_TYPE_MAX) {
                        LOG_I("en_autoconnect error, should be 0~2");
                        return PARAM_RANGE;
                    }
                }

                if((at_user_info.flag_sta == 1) && (at_user_info.dis_dhcp == 1))
                {
                    memcpy(conf.local_ip_addr, at_user_info.staip, 16);
                    memcpy(conf.gateway_ip_addr, at_user_info.stagw, 16);
                    memcpy(conf.net_mask, at_user_info.stamask, 16);
                    if(strlen((const char*)conf.local_ip_addr))
                        conf.dhcp_mode = WLAN_DHCP_DISABLE;
                }

            }
            else if ((strcmp(argv[1],"ap") == 0))
            {
                if(current_iftype != NON_MODE_)
                {
                    LOG_I("cur mode is %d, close it first!",current_iftype);
                    return CONFIG_FAIL;
                }
                conf.wifi_mode = SOFTAP;

                if(strlen((char *)argv[2]) > 32)
                {
                    return PARAM_RANGE;
                }
                strcpy((char*)conf.wifi_ssid, (char*)argv[2]);

                if(argc >= 4)
                {
                    if((strlen(argv[3]) > 7)&&(strlen(argv[3]) < 65))
                        strcpy((char*)conf.wifi_key, (char*)argv[3]);
                    else if((strlen(argv[3]) == 1) && (convert_str_to_int(argv[3]) == 0))
                        memset(conf.wifi_key, 0, 64);
                    else
                    {
                        LOG_I("param4 error, input 0 or pwd(length >= 8)\n");
                        return PARAM_RANGE;
                    }
                }

                if(argc >= 5)
                {
                    conf.channel = convert_str_to_int(argv[4]);
                    if((conf.channel > 14) || (conf.channel < 1))
                    {
                        LOG_I("param5 error,channel range:1~14");
                        return PARAM_RANGE;
                    }
                }
                if(argc >= 6)
                {
                    conf.hide = convert_str_to_int(argv[5]);
                    if(conf.hide == ASR_STR_TO_INT_ERR)
                    {
                        LOG_I("param6 error, hide err");
                        return PARAM_RANGE;
                    }
                }

                if (argc >= 7)
                {
                    conf.interval = convert_str_to_int(argv[6]);
                    if(conf.interval == ASR_STR_TO_INT_ERR)
                    {
                        LOG_I("param7 error, interval err");
                        return PARAM_RANGE;
                    }
                }

                //if(at_user_info.flag_sap == 1)
                {
                    memcpy(conf.local_ip_addr, at_user_info.sapip, 16);
                    memcpy(conf.gateway_ip_addr, at_user_info.sapgw, 16);
                    memcpy(conf.net_mask, at_user_info.sapmask, 16);
                }
            }
            else
            {
                LOG_I("asr_wifi_open:error param\r\n");
                return PARAM_RANGE;
            }
            break;
        default:
            LOG_I("asr_wifi_open:error param\r\n");
            return PARAM_RANGE;
    }

    LOG_I("+++mode:%d ssid:%s key:%s+++\r\n", conf.wifi_mode, conf.wifi_ssid, conf.wifi_key);
    asr_wlan_clear_pmk();
    asr_wlan_open(&conf);

    return RSP_NULL;
}
MSH_CMD_EXPORT_ALIAS(asr_at_wifi_open, at_wifi_open, ASR wifi Open);

static int asr_at_wifi_close(int argc, char **argv)
{
    uint8_t echo_flag = 0;
    uint32_t iftype_tmp = current_iftype;
    if(iftype_tmp == NON_MODE_)
    {
        LOG_I("no wlan interface open, don't need close!");

        return CONFIG_FAIL;

    }
    else if(iftype_tmp == SNIFFER_MODE)
    {
        LOG_I("cur mode is sniffer,use wifi_sniffer_stop close!");
        return CONFIG_FAIL;
    }

    echo_flag = at_user_info.uart_echo;
    memset(&at_user_info, 0, sizeof(at_user_info));
    at_user_info.uart_echo = echo_flag;

    asr_wlan_close();

    return RSP_NULL;
}
MSH_CMD_EXPORT_ALIAS(asr_at_wifi_close, at_wifi_close, ASR wifi Close);

static int asr_at_wifi_deinit(int argc, char **argv)
{
    asr_wlan_deinit();
    return CONFIG_OK;
}
static int asr_at_wifi_init(int argc, char **argv)
{
    asr_wlan_init();
    return CONFIG_OK;
}

int convert_str_to_int(char *str)
{
    int result = 0;
    char *stop;
    result = strtol(str, &stop, 0);
    if(*stop != '\0')
    {
        return ASR_STR_TO_INT_ERR;
    }
    return result;
}

extern bool user_scan_flag; /*scan flag which indicate call by asr scan api*/
static int asr_at_wifi_scan(int argc, char **argv)
{
    char* ssid = 0;
    int channel = 0;
    char  bssid[6] = {0};
    char* invalidBssid = "000000000000";

    if((current_iftype == STA_MODE) || (current_iftype == SAP_MODE))
    {
        if(argc > 1) //channel
        {
            channel = convert_str_to_int(argv[1]);
            if(channel == ASR_STR_TO_INT_ERR)
            {
                LOG_I("channel error");
                return PARAM_RANGE;
            }

            if(argc > 2) //channel bssid
            {
                if(strcmp(argv[2], invalidBssid) != 0)
                {
                    if(strlen(argv[2]) == (6<<1))
                    {
                        asr_mac_addr_string_to_array(argv[2], (uint8_t*)bssid);
                        LOG_I("scan bssid %02x:%02x:%02x:%02x:%02x:%02x",
                            bssid[0],bssid[1],bssid[2],bssid[3],bssid[4],bssid[5]);
                    }
                    else
                    {
                        LOG_I("scan bssid len error");
                    }
                }

                if(argc > 3) //channel bssid ssid
                {
                    ssid = argv[3];
                }
            }
        }

        asr_wlan_start_scan_detail(ssid, channel, bssid);
    user_scan_flag = true;
        return RSP_NULL;
    }
    else
    {
        return CONFIG_FAIL;
    }
}
MSH_CMD_EXPORT_ALIAS(asr_at_wifi_scan, at_wifi_scan, ASR wifi SCAN);

#ifdef CFG_ADD_API
static void asr_at_uap_stalist(int argc, char **argv)
{
    sta_list list={0} ;
    UAP_Stalist(&list);
}

static void asr_at_uap_scan_channel(int argc, char **argv)
{
    uap_config_scan_chan param={0};
    if(argc == 2)
    {
        param.action = atoi(argv[1]);
    }
    else
    {
        LOG_I("asr_at_uap_scan_channel:error param\r\n");
        param.action = 0 ;
    }
    UAP_Scan_Channels_Config(&param);
}

static void asr_at_uap_deauth(int argc, char **argv)
{
    uap_802_11_mac_addr param={0} ;

    if(argc == 2)
    {
        if(strlen(argv[1]) == (MAC_ADDR_LEN<<1))
        {
            asr_mac_addr_string_to_array(argv[1], param);
            LOG_I("mac addr seted 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n",param[0],param[1],param[2],param[3],param[4],param[5]);
            UAP_Sta_Deauth(param);
        }
        else
        {
            LOG_I("asr_at_uap_deauth:mac addr len error\r\n");
        }
    }
    else
    {
        LOG_I("asr_at_uap_deauth:error param\r\n");
    }
}



static void asr_at_uap_blacklist_onoff(int argc, char **argv)
{
    uint8_t onoff ;
    if(argc == 2)
    {
        LOG_I("asr_at_uap_blacklist_argv[1]:%d %s \r\n",argv[1],argv[1]);
        onoff = atoi(argv[1]);
        onoff = onoff?1:0;
        LOG_I("asr_at_uap_blacklist_onoff:%d \r\n",onoff);
        UAP_Black_List_Onoff(onoff);
    }
    else
    {
        LOG_I("asr_at_uap_blacklist_onoff:error param\r\n");
    }
}

static void asr_at_uap_blacklist_get(int argc, char **argv)
{
    blacklist black_l;
    UAP_Black_List_Get(&black_l);
}

static void asr_at_uap_blacklist_add(int argc, char **argv)
{
    uint8_t param[MAC_ADDR_LEN] ;

    if(argc == 2)
    {
        if(strlen(argv[1]) == (MAC_ADDR_LEN<<1))
        {
            asr_mac_addr_string_to_array(argv[1], param);
            LOG_I("mac addr seted 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n",param[0],param[1],param[2],param[3],param[4],param[5]);
            UAP_Black_List_Add(param);
        }
        else
        {
            LOG_I("asr_at_uap_blacklist_add:mac addr len error\r\n");
        }
    }
    else
    {
        LOG_I("asr_at_uap_blacklist_add:error param\r\n");
    }
}

static void asr_at_uap_blacklist_del(int argc, char **argv)
{
    uint8_t param[MAC_ADDR_LEN] ;

    if(argc == 2)
    {
        if(strlen(argv[1]) == (MAC_ADDR_LEN<<1))
        {
            asr_mac_addr_string_to_array(argv[1], param);
            LOG_I("mac addr seted 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n",param[0],param[1],param[2],param[3],param[4],param[5]);
            UAP_Black_List_Del(param);
        }
        else
        {
            LOG_I("asr_at_uap_blacklist_del:mac addr len error\r\n");
        }
    }
    else
    {
        LOG_I("asr_at_uap_blacklist_del:error param\r\n");
    }
}

static void asr_at_uap_blacklist_clear(int argc, char **argv)
{
    UAP_Black_List_Clear();
}

static void asr_at_uap_whitelist_onoff(int argc, char **argv)
{
    uint8_t onoff ;
    if(argc == 2)
    {
        onoff = atoi(argv[1]);
        onoff = onoff?1:0;
        UAP_White_List_Onoff(onoff);
    }
    else
    {
        LOG_I("asr_at_uap_whitelist_onoff:error param\r\n");
    }
}

static void asr_at_uap_whitelist_get(int argc, char **argv)
{
    whitelist white_l;
    UAP_White_List_Get(&white_l);
}

static void asr_at_uap_whitelist_add(int argc, char **argv)
{
    uint8_t param[MAC_ADDR_LEN] ;

    if(argc == 2)
    {
        if(strlen(argv[1]) == (MAC_ADDR_LEN<<1))
        {
            asr_mac_addr_string_to_array(argv[1], param);
            LOG_I("mac addr seted 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n",param[0],param[1],param[2],param[3],param[4],param[5]);
            UAP_White_List_Add(param);
        }
        else
        {
            LOG_I("asr_at_uap_whitelist_add:mac addr len error\r\n");
        }
    }
    else
    {
        LOG_I("asr_at_uap_whitelist_add:error param\r\n");
    }
}

static void asr_at_uap_whitelist_del(int argc, char **argv)
{
    uint8_t param[MAC_ADDR_LEN] ;

    if(argc == 2)
    {
        if(strlen(argv[1]) == (MAC_ADDR_LEN<<1))
        {
            asr_mac_addr_string_to_array(argv[1], param);
            LOG_I("mac addr seted 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n",param[0],param[1],param[2],param[3],param[4],param[5]);
            UAP_White_List_Del(param);
        }
        else
        {
            LOG_I("asr_at_uap_whitelist_del:mac addr len error\r\n");
        }
    }
    else
    {
        LOG_I("asr_at_uap_whitelist_del:error param\r\n");
    }
}

static void asr_at_uap_whitelist_clear(int argc, char **argv)
{
    UAP_White_List_Clear();
}


static void asr_at_uap_filter(int argc, char **argv)
{
    uap_config_param uap_param={0};
    // 0  0 //disable get
    // 0  1  //display whitelist
    // 0  2  ///display blacklist

    // 1  0 // disable set
    // 1  1  mac //set whitelist
    // 1  2  mac //set blacklist

    // t_u16 filter_mode;
    // t_u16 mac_count;
    // uap_802_11_mac_addr mac_list[16];

    switch(argc)
    {
        case 3:
        case 4:
            uap_param.action = atoi(argv[1]);
            uap_param.filter.filter_mode = atoi(argv[2]);
            if(argc == 4)
            {
                if(strlen(argv[3]) == (MAC_ADDR_LEN<<1))
                {
                    uap_param.filter.mac_count = 1;
                    asr_mac_addr_string_to_array(argv[3], uap_param.filter.mac_list[0]);
                    LOG_I("mac addr seted 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n",uap_param.filter.mac_list[0][0],uap_param.filter.mac_list[0][1]
                    ,uap_param.filter.mac_list[0][2],uap_param.filter.mac_list[0][3],uap_param.filter.mac_list[0][4],uap_param.filter.mac_list[0][5]);
                    UAP_Params_Config(&uap_param);
                }
                else
                {
                    LOG_I("asr_at_uap_whitelist_del:mac addr len error\r\n");
                    return ;
                }
            }
            break;
    }
    UAP_Params_Config(&uap_param);

}


static void asr_at_pkg_forward(int argc, char **argv)
{
    uap_pkt_fwd_ctl param={0};
    if(argc<2 )
    {
        LOG_I("uap_pkt_fwd_ctl:error param\r\n");
        param.action = 0 ;
    }
    else
    {
        param.action = atoi(argv[1]);
        if(param.action==1)
        {
            if(argc<3) return -1;
            param.pkt_fwd_ctl = atoi(argv[2]);
        }
    }
    UAP_PktFwd_Ctl(&param);
}

static void asr_at_get_channel(int argc, char **argv)
{
    UAP_GetCurrentChannel();
}


static void asr_at_bss_config(int argc, char **argv)
{
    INT32 ps_on = 0;
    if(argc == 2)
    {
        ps_on = atoi(argv[1]);
    }
    else
    {
        ps_on = 0 ;
        dbg(D_ERR, D_UWIFI_CTRL, "%s: arg error use default value %d \r\n", __func__,ps_on);
    }
    UAP_BSS_Config(ps_on);
}

#endif

static int asr_at_wifi_get_ip_status(int argc, char **argv)
{
    asr_wlan_ip_stat_t *param = asr_wlan_get_ip_status();
    LOG_I("Got ip:%s, gw:%s, mask:%s, mac:%s\r\n", param->ip, param->gate, param->mask, param->macaddr);
    return CONFIG_OK;
}

static int asr_at_wifi_get_link_status (int argc, char **argv)
{
    asr_wlan_link_stat_t param;
    asr_wlan_get_link_status(&param);
    LOG_I("connect:%d ssid:%s\r\n", param.is_connected, param.ssid);
    return CONFIG_OK;
}

static int asr_at_wifi_set_dhcp(int argc, char **argv)
{
    int dhcp;

    if(argc < 2)
    {
        return PARAM_MISS;
    }

    dhcp = atoi(argv[1]);

    if(dhcp == 1)
    {
        at_user_info.dis_dhcp = 0;
        return CONFIG_OK;
    }
    else if(dhcp == 0)
    {
        at_user_info.dis_dhcp = 1;
        return CONFIG_OK;
    }
    else
    {
        return PARAM_RANGE;
    }
}

static int asr_at_wifi_set_sta_ip(int argc, char **argv)
{
    struct in_addr  ip;
    struct in_addr  gw;
    struct in_addr  mask;


    if(argc < 4)
    {
        return PARAM_MISS;
    }

    if((strlen(argv[1])>16) || (strlen(argv[2])>16) || (strlen(argv[3])>16))
    {
        LOG_I("ip gate mask length error!");
        return PARAM_RANGE;
    }

    #ifdef ALIOS_SUPPORT
    inet_pton(AF_INET, argv[1], &ip);
    inet_pton(AF_INET, argv[2], &gw);
    inet_pton(AF_INET, argv[3], &mask);
    #else
    //net_addr_pton(AF_INET, argv[1], &ip);
    //net_addr_pton(AF_INET, argv[2], &gw);
    //net_addr_pton(AF_INET, argv[3], &mask);
    ipaddr_aton(argv[1], &ip);
    ipaddr_aton(argv[2], &gw);
    ipaddr_aton(argv[3], &mask);
    #endif


    if((gw.s_addr & mask.s_addr) != (ip.s_addr & mask.s_addr))
    {
        LOG_I("ip&mask must equal gateway&mask");
        return PARAM_RANGE;
    }

    if((strcmp(argv[0],"wifi_set_sta_ip") == 0))
    {
        memcpy(at_user_info.staip, argv[1], 16);
        memcpy(at_user_info.stagw, argv[2], 16);
        memcpy(at_user_info.stamask, argv[3], 16);
        at_user_info.flag_sta = 1;
    }

    return CONFIG_OK;
}

static int asr_at_wifi_get_macaddr(int argc, char **argv)
{
    uint8_t macaddr[6];
    int ret;

    ret = asr_wlan_get_mac_address(macaddr);

    if(ret == 0)
    {
        LOG_I("mac addr 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
        return CONFIG_OK;
    }else
        return CONFIG_FAIL;
}

extern void asr_wlan_set_mac_address(uint8_t *mac_addr);
extern int write_wifimac_to_mrd(const char *mac);
static int asr_at_wifi_set_macaddr(int argc, char **argv)
{
    uint8_t macaddr[6];

    if(argc == 2)
    {
        if(strlen(argv[1]) == (MAC_ADDR_LEN<<1))
        {
            asr_mac_addr_string_to_array(argv[1], macaddr);
            #ifdef THREADX
            write_wifimac_to_mrd(macaddr);
            #else
            //asr_wlan_set_mac_address(macaddr);
            #endif

            LOG_I("mac addr setted 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
            return CONFIG_OK;
        }
        else
        {
            LOG_I("wifi_set_mac_addr:mac addr len error\r\n");
            return PARAM_RANGE;
        }
    }
    else
    {
        LOG_I("wifi_set_mac_addr:error param\r\n");
        return PARAM_RANGE;
    }
}

static int asr_at_wifi_enable_log(int argc, char **argv)
{
    asr_wlan_start_debug_mode();
    return CONFIG_OK;
}

static int asr_at_wifi_disable_log(int argc, char **argv)
{
    asr_wlan_stop_debug_mode();
    return CONFIG_OK;
}

extern bool txlogen;
extern int wpa_debug_level;
static int asr_at_wifi_enable_txlog(int argc, char **argv)
{
    if (argc < 2)
    {
        return PARAM_MISS;
    }

    txlogen = atoi(argv[1]);

    if (argc > 2)
      wpa_debug_level = atoi(argv[2]);

    return CONFIG_OK;
}

static int asr_at_wifi_disable_txlog(int argc, char **argv)
{
    txlogen = 0;
    wpa_debug_level = 5;
    return CONFIG_OK;
}
static int asr_at_wifi_enable_ps(int argc, char **argv)
{
    uint8_t ps_on = 0;
    if (argc <= 1)
    {
        struct asr_hw *asr_hw = uwifi_get_asr_hw();
        if(asr_hw)
            dbg(D_ERR, D_AT, "get ps mode is %d\n",asr_hw->ps_on);
        return PARAM_RANGE;
    }

    ps_on = atoi(argv[1]);

    asr_wlan_set_ps_mode(ps_on);

    return CONFIG_OK;
}

#if 0
static void asr_at_reset(int argc, char **argv)
{
    //NVIC_SystemReset();
}
#endif

/*
 *******************************************************
 * @softap
 *
 *******************************************************
 */


#ifdef CFG_SNIFFER_SUPPORT
/*
 *******************************************************
 * @sniffer
 *
 *******************************************************
 */
static int asr_at_wifi_sniffer_set_type(int argc, char **argv)
{
    uint32_t type = 0;
    if (argc <= 1)
    {
        dbg(D_ERR, D_AT, "sniffer set channel parameter error");
        return PARAM_RANGE;
    }

    type = atoi(argv[1]);

    uwifi_set_filter_type(type);
    return CONFIG_OK;
}

static int asr_at_wifi_sniffer_set_channel(int argc, char **argv)
{
    uint8_t channel = 0;

    if (argc <= 1)
    {
        dbg(D_ERR, D_AT, "sniffer set channel parameter error");
        return PARAM_RANGE;
    }

    channel = atoi(argv[1]);

    if ((channel <= 0) || (channel > 13))
    {
        dbg(D_ERR, D_AT, "sniffer set channel wrong channel:%d", channel);
        return PARAM_RANGE;
    }

    asr_wlan_monitor_set_channel(channel);
    return CONFIG_OK;
}

void asr_at_monitor_rx_cb(uint8_t*data, int len)
{
    /* defined for debug */
    dbg(D_ERR, D_AT, "monitor_rx_cb:0x%x data_len:%d", *data, len);
}

static int asr_at_wifi_sniffer_start(int argc, char **argv)
{
    int ret;
    int mode = asr_wlan_get_wifi_mode();
    if(mode != NON_MODE_)
    {
        dbg(D_ERR, D_AT, "current mode is %d,close it first!",mode);
        return CONFIG_FAIL;
    }

    asr_wlan_register_monitor_cb(asr_at_monitor_rx_cb);

    ret = asr_wlan_start_monitor();
    if(ret == 0)
        return CONFIG_OK;
    else
        return CONFIG_FAIL;
}

static int asr_at_wifi_sniffer_stop(int argc, char **argv)
{
    int ret = 0;
    int mode = asr_wlan_get_wifi_mode();
    if(mode != SNIFFER)
    {
        dbg(D_ERR, D_AT, "current mode is %d,use wifi_close!",mode);
        return CONFIG_FAIL;
    }
    asr_wlan_register_monitor_cb(NULL);

    ret = asr_wlan_stop_monitor();
    if(ret)
        return CONFIG_FAIL;
    else
        return CONFIG_OK;
}
#endif

/*
 *******************************************************
 * @custom mgmt
 *
 *******************************************************
 */
#ifdef CFG_CUS_FRAME
static int asr_at_wifi_send_custom_mgmt(int argc, char **argv)
{
    uint8_t ret = 0;
    uint32_t cus_len;
    uint8_t  *cus_frame, *pframe;
    uint8_t  cus_frame_add1[MAC_ADDR_LEN] = {0xAA,0x00,0xCC,0xDD,0xEE,0xFF};
    uint8_t  cus_frame_add2[MAC_ADDR_LEN] = {0x8C,0xF2,0x28,0x04,0x6C,0x51};

    dbg(D_ERR, D_UWIFI_CTRL, "asr_wifi_send_custom_mgmt start");

    // build tx mgmt frame
    cus_frame = asr_rtos_malloc(CUS_MGMT_FRAME);
    if(cus_frame == NULL)
    {
        dbg(D_CRT, D_UWIFI_CTRL, "cus_frame alloc error, return");
        return PARAM_RANGE;
    }

    dbg(D_CRT, D_UWIFI_CTRL,"build [auth rsp] frame \n");
    pframe = uwifi_init_mgmtframe_header((struct ieee80211_hdr*)cus_frame,
                  cus_frame_add2, cus_frame_add1, cus_frame_add2, IEEE80211_STYPE_AUTH);
    //auth_type:
    *(pframe++) = 0;
    *(pframe++) = 0;
    //auth seq num
    *(pframe++) = 2;
    *(pframe++) = 0;
    //auth status
    *(pframe++) = WLAN_STATUS_SUCCESS & 0xff;
    *(pframe++) = 0;

    cus_len = pframe - cus_frame;

    // send tx mgmt frame
    ret = asr_wlan_send_raw_frame(cus_frame, cus_len);
    if(ret)
    {
        dbg(D_CRT, D_UWIFI_CTRL, "asr_wifi_send_custom_mgmt error");
    }
    asr_rtos_free(cus_frame);
    return CONFIG_OK;
}
#endif


static int asr_at_wifi_reset(int argc, char **argv)
{
    dbg(D_ERR, D_UWIFI_CTRL, "asr_at_wifi_reset start");
    asr_wlan_reset();
    return CONFIG_OK;
}

#ifdef THREADX

#ifndef iotNPINGCnf
typedef struct _iotNPINGCnf
{
    UINT32 result;
    UINT8 retrynum;
    UINT8 remote_addr[128];
    UINT16 ttl;
    UINT16 rtt;
}iotNPINGCnf;
#endif

void ping_cb_t(char* output_str)
{
    iotNPINGCnf pingcnf={0};
    UINT32 bytes=0;
    if(strstr(output_str,"fail")){
        dbg(D_INF, D_AT, "%s ping  failed \r\n", __func__);
    }else{
        sscanf(output_str,"%*s%*s%*s %[^:]",
            pingcnf.remote_addr);

        dbg(D_INF, D_AT, "%s: remote_addr %s\r\n", __func__, pingcnf.remote_addr);

        output_str =  strstr(output_str,"bytes");

        sscanf(output_str,"bytes=%d time=%dms TTL=%d\r\n",
            &bytes, &pingcnf.rtt, &pingcnf.ttl);

        dbg(D_INF, D_AT, "%s: bytes %d, rtt %d, ttl %d\r\n", __func__,
        bytes,pingcnf.rtt,pingcnf.ttl);
        //pingcnf.retrynum = 1;
        //pingcnf.result = IOTRC_SUCCESS;
        dbg(D_INF, D_AT, "%s ping ok \r\n", __func__);
    }

}
static int asr_at_wifi_ping(int argc, char **argv)//argc number of param
{
    if(argc!=2)
        return PARAM_RANGE;
    dbg(D_INF, D_AT, "%s enter...", __func__);
    char ip_string[128];
    strcpy((char *)ip_string, argv[1]);
    api_lwip_do_ping(ip_string, 4, ping_cb_t);
    return CONFIG_OK;
}

int asr_at_wifi_ping_stop(int argc, char **argv)
{
    api_lwip_stop_ping();
    return CONFIG_OK;
}
#else

// adapt to zephyr net
int asr_at_wifi_ping(int argc, char **argv)
{
    dbg(D_INF, D_AT, "call asr_wifi_ping_start, not implement yet!");
    //asr_wifi_ping_start(argc, argv);
    return CONFIG_OK;
}

int asr_at_wifi_ping_stop(int argc, char **argv)
{
    dbg(D_INF, D_AT, "call asr_wifi_ping_stop, not implement yet!");
    // asr_wifi_ping_stop();
    return CONFIG_OK;
}
#endif // !THREADX

extern void thread_analyzer_print(void);
//extern void sys_heap_print_info(struct sys_heap *heap, bool dump_chunks);
// extern struct k_heap _system_heap;

static int asr_at_wifi_vlist(int argc, char **argv)//argc number of param
{
    //vTaskList(v_list);
    //LOG_I(v_list);
    //LOG_I("left free heap %d\r\n",xPortGetFreeHeapSize());
    //memset(v_list,0,VLIST_DEBUG_SIZE);
    // struct k_heap *sys_heap = (struct k_heap *)(&_system_heap);

    // sys_heap_print_info(&sys_heap->heap,0);
    // LOG_I("\r\n ######################################################## \r\n");

    // thread_analyzer_print();
    return CONFIG_OK;
}

#if ASR_DEBUG_MALLOC_TRACE
/*********************************************************************
*Function:asr_show_malloc
*Description:Show all malloc not free memory in FreeRTOS,
* Each memory block, show address,length,malloc function and line in source code
*Output: output to uart log
*
********************************************************************/
static void asr_at_show_malloc(int argc, char **argv)
{
    asr_rtos_dump_malloc();
}
#endif
#if CFG_DBG
/*********************************************************************
*Function:asr_at_set_host_log
*Description:Set Host Log switch
*
********************************************************************/
static int  asr_at_set_host_log(int argc, char **argv)
{
    if (argc < 2)
        return -1;
    return asr_wlan_host_log_set(atoi(argv[1]));
}
#endif

uint32_t asr_crc32(const uint8_t *buf, uint32_t len)
{
    uint32_t i;
    uint32_t crc = 0xffffffff;

    while (len--) {
        crc ^= *buf++;
        for (i = 0; i < 8; ++i) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc = (crc >> 1);
        }
    }
    return ~crc;
}

static int asr_at_set_power(int argc, char **argv)
{
    int power = 0;

    if (argc != 2) {
        dbg(D_INF, D_AT, "%s: wifi_setpower [power 0/1]\n", __func__);
        return -1;
    }

    if (!g_asr_gpio_inited) {
        asr_gpio_init();
        g_asr_gpio_inited = 1;
    }

    power = atoi(argv[1]);

    dbg(D_INF, D_AT, "%s: %d\n", __func__, power);

    asr_wlan_gpio_power(power);

    return 0;
}

static int asr_at_set_reset(int argc, char **argv)
{
    int reset = 0;

    if (argc != 2) {
        dbg(D_INF, D_AT, "%s: wifi_setreset [reset 0/1]\n", __func__);
        return -1;
    }

    if (!g_asr_gpio_inited) {
        asr_gpio_init();
        g_asr_gpio_inited = 1;
    }

    reset = atoi(argv[1]);

    dbg(D_INF, D_AT, "%s: %d\n", __func__, reset);

    asr_wlan_set_gpio_reset_pin(!reset);

    return 0;
}

static int asr_at_detect_wifi(int argc, char **argv)
{
    int ret = 0;

    if (argc != 1) {
        dbg(D_INF, D_AT, "%s: wifi_detect\n", __func__);
        return -1;
    }

    if (!g_asr_gpio_inited) {
        asr_gpio_init();
        g_asr_gpio_inited = 1;
    }


    dbg(D_INF, D_AT, "%s: \n", __func__);

    asr_sdio_release_irq();

#if 1
    ret = asr_wlan_gpio_reset();
    if (ret < 0) {
        return ret;
    }

    ret = sdio_bus_probe();
    if (ret < 0) {
        dbg(D_CRT, D_UWIFI_CTRL, "%s: sdio_bus_probe failed ret=%d!", __func__, ret);
        //return ret;
    }
#endif

    return 0;
}


/*
 * COMMAND TABLE ARRAY
 ********************************************************************
 */
static int asr_at_show_command(int argc, char **argv);

cmd_entry cmd_table[] = {
    {"?",                            asr_at_show_command,             0},
    {"quit",                            asr_at_deinit,             0},
    {"wifi_version",                 asr_at_show_version,             0},
    {"wifi_open",                    asr_at_wifi_open,                0},
    {"wifi_close",                   asr_at_wifi_close,               0},
    #if 1
    {"wifi_set_dhcp",                asr_at_wifi_set_dhcp,           0},
    {"wifi_set_sta_ip",             asr_at_wifi_set_sta_ip,           0},
    #endif
    {"wifi_get_mac_addr",            asr_at_wifi_get_macaddr,         0},
    {"wifi_set_mac_addr",            asr_at_wifi_set_macaddr,         0},
    {"wifi_scan",                    asr_at_wifi_scan,                0},
    {"wifi_enable_log",              asr_at_wifi_enable_log,          0},
    {"wifi_disable_log",             asr_at_wifi_disable_log,         0},
    {"wifi_enable_txlog",            asr_at_wifi_enable_txlog,        0},
    {"wifi_disable_txlog",           asr_at_wifi_disable_txlog,       0},
    {"wifi_enable_ps",               asr_at_wifi_enable_ps,           0},
    {"wifi_init",                    asr_at_wifi_init,                0},
    {"wifi_deinit",                  asr_at_wifi_deinit,              0},
#ifdef CFG_SNIFFER_SUPPORT
    {"wifi_sniffer_start",           asr_at_wifi_sniffer_start,       0},
    {"wifi_sniffer_stop",            asr_at_wifi_sniffer_stop,        0},
    {"wifi_sniffer_set_channel",     asr_at_wifi_sniffer_set_channel, 0},
    {"asr_wifi_sniffer_set_type",    asr_at_wifi_sniffer_set_type,    0},
#endif
    {"wifi_ping",                    asr_at_wifi_ping,                0},
    {"wifi_ping_stop",               asr_at_wifi_ping_stop,           0},
    {"wifi_iperf",                 asr_at_iperf,                    0},
    {"asr_wifi_get_ip_status",       asr_at_wifi_get_ip_status,       0},
    {"asr_wifi_get_link_status",     asr_at_wifi_get_link_status,     0},
    {"wifi_error",                   asr_at_wlan_get_err_code,        0},
    {"wifi_reset",                   asr_at_wifi_reset,               0},
   // {"wifi_test",                    asr_at_wifi_test,                0},
#ifdef CFG_CUS_FRAME
    {"asr_wifi_send_custom_mgmt",   asr_at_wifi_send_custom_mgmt,    0},
#endif
    {"asr_wifi_vlist",              asr_at_wifi_vlist,               0},
#if ASR_DEBUG_MALLOC_TRACE
    {"asr_show_malloc",             asr_at_show_malloc,              0},
#endif
#if CFG_DBG
    {"asr_set_host_log",            asr_at_set_host_log,              0},
#endif
#if CFG_ADD_API
    {"UAP_Stalist",            asr_at_uap_stalist,            0},
    {"UAP_Scan_Channel",       asr_at_uap_scan_channel,       0},
    {"UAP_Deauth",             asr_at_uap_deauth,             0},

    {"UAP_Black_List_Onoff",   asr_at_uap_blacklist_onoff,     0},
    {"UAP_Black_List_Get",     asr_at_uap_blacklist_get,       0},
    {"UAP_Black_List_Add",     asr_at_uap_blacklist_add,       0},
    {"UAP_Black_List_Del",     asr_at_uap_blacklist_del,       0},
    {"UAP_Black_List_Clear",   asr_at_uap_blacklist_clear,     0},

    {"UAP_White_List_Onoff",   asr_at_uap_whitelist_onoff,     0},
    {"UAP_White_List_Get",     asr_at_uap_whitelist_get,       0},
    {"UAP_White_List_Add",     asr_at_uap_whitelist_add,       0},
    {"UAP_White_List_Del",     asr_at_uap_whitelist_del,       0},
    {"UAP_White_List_Clear",   asr_at_uap_whitelist_clear,     0},

    {"UAP_GetCurrentChannel",   asr_at_get_channel,     0},
    {"UAP_BSS_Config",          asr_at_bss_config,      0},
    {"UAP_Params_Config",       asr_at_uap_filter,             0},
    {"UAP_PktFwd_Ctl",          asr_at_pkg_forward,            0},
#endif
    {"wifi_setpower",           asr_at_set_power,                0},
    {"wifi_setreset",           asr_at_set_reset,                0},
    {"wifi_detect",           asr_at_detect_wifi,              0},
};

void asr_at_cmd_helper_init(void)
{
    int i = 0;

    for(i = 0; i < sizeof(cmd_table) / sizeof(cmd_table[0]); i++)
    {
        if(strcmp("?", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "AT command as below, use \"? command\" for more details :)\r\n";
        }
        else if(strcmp("wifi_version", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_version\r\n\
cmd function: show wifi release version\r\n";
        }
        else if(strcmp("wifi_open", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_open\r\n\
cmd function: open wifi station mode\r\n\
cmd format:   wifi_open sta SSID\r\n\
cmd function: open wifi station mode and connect unencrypted AP\r\n\
cmd format:   wifi_open sta SSID PASSWORD CHANNEL retry_times en_autoconnect\r\n\
cmd function: open wifi station mode and connect encrypted AP\r\n\
cmd format:   wifi_open ap SSID CHANNEL\r\n\
cmd function: open wifi unencrypted softap mode on given channel\r\n\
cmd format:   wifi_open ap SSID CHANNEL PASSWORD\r\n\
cmd function: open wifi encrypted softap mode on given channel\r\n\
cmd format:   wifi_open ap SSID CHANNEL PASSWORD HIDDEN_SSID BEACONINTERVAL\r\n\
cmd function: open wifi encrypted softap mode on given channel\r\n";
        }
        else if(strcmp("wifi_close", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_close\r\n\
cmd function: close wifi station mode or softap mode\r\n";
        }
        else if(strcmp("wifi_get_mac_addr", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_get_mac_addr\r\n\
cmd function: get mac address of device, call after wifi opened\r\n";
        }
        else if(strcmp("wifi_set_mac_addr", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_set_mac_addr MAC_ADDR(e.g. AA00CCEFCDAB)\r\n\
cmd function: set default mac address of device, call before wifi open\r\n";
        }
        else if(strcmp("wifi_scan", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_scan\r\n\
cmd function: scan AP on all channel, used in station mode\r\n";
        }
        else if(strcmp("wifi_enable_log", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_enable_log\r\n\
cmd function: enable wifi stack log\r\n";
        }
        else if(strcmp("wifi_disable_log", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_disable_log\r\n\
cmd function: disable wifi stack log\r\n";
        }
        #ifdef CFG_SNIFFER_SUPPORT
        else if(strcmp("wifi_sniffer_start", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_sniffer_start\r\n\
cmd function: open wifi sniffer mode\r\n";
        }
        else if(strcmp("wifi_sniffer_stop", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_sniffer_stop\r\n\
cmd function: close wifi sniffer mode\r\n";
        }
        else if(strcmp("wifi_sniffer_set_channel", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_sniffer_set_channel CHANNEL(e.g. 1)\r\n\
cmd function: set channel of sniffer mode\r\n";
        }
        #endif
        else if(strcmp("wifi_ping", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_ping IP_ADDR(e.g. 192.168.1.1)\r\n\
cmd function: ping given ip address\r\n";
        }
        else if(strcmp("wifi_iperf", cmd_table[i].command) == 0)
        {
            cmd_table[i].help =  "cmd format:   wifi_iperf -c host [-p port] [-u] [-t]\r\n\
cmd format:   wifi_iperf -s [-p port] [-u] [-t]\r\n\
cmd function:\r\n\
              -c host     :run as iperf client connect to host\r\n\
              -s          :run as iperf server\r\n\
              -p port     :client connect to/server port default 5001\r\n\
              -u          :use udp do iperf client/server\r\n\
                           If -u not enable, use tcp default\r\n\
              -t          :terminate iperf client/server/all\r\n";
        }
        #ifdef ASR_RFTEST
        else if(strcmp("wifi_setchn", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_setchn n\r\n\
cmd function: switch channel for 20M bandwith in rftest mode\r\n\
cmd format:   wifi_setchn n ht40+/ht40-\r\n\
cmd function: switch channel for 40M bandwith in rftest mode\r\n";
        }
        else if(strcmp("wifi_test", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_test rx n\r\n\
cmd function: rx sensitivity test.statistic per between n seconds in rftest mode\r\n\
cmd format:   wifi_test tx 11b    1/2/5.5/11                      [ppdu_len]\r\n\
cmd function: rf tx 11b power/evm test  in rftest mode\r\n\
cmd format:   wifi_test tx 11g    6/9/12/18/24/36/48/54           [ppdu_len]\r\n\
cmd function: rf tx 11g power/evm test  in rftest mode\r\n\
cmd format:   wifi_test tx 11n    0~7                             [ppdu_len]\r\n\
cmd function: rf tx 11n power/evm test  in rftest mode\r\n\
cmd format:   wifi_test tx 11n40  non/dup/mf/gf 1~54/1~54/0~7/0~7 [ppdu_len]\r\n\
cmd function: rf tx 11n40 power/evm test  in rftest mode\r\n";
        }
        else if(strcmp("wifi_setpow", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_setpow n\r\n\
cmd function: add tx power base on current power in rftest mode\r\n";
        }
        else if(strcmp("wifi_setduty", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_setduty n\r\n\
cmd function: set tx frame space for diff rate in rftest mode\r\n";
        }
        else if(strcmp("wifi_getv", cmd_table[i].command) == 0)
        {
            cmd_table[i].help = "cmd format:   wifi_getv n\r\n\
cmd function: get value of tmmt and txpwr in rftest mode\r\n";
        }
        #endif
    }
}

/*used for user register AT*/
asr_at_cmd_entry *user_cmd_table = 0;
uint32_t user_cmd_num = 0;
int asr_at_cmd_register(asr_at_cmd_entry *cmd_entry, int num)
{
    user_cmd_table = cmd_entry;
    user_cmd_num = num;

    return 0;
}

/*********************************************************************
*Function:asr_show_command
*Description:Show all command at_handle support
*Output: output to uart log
*
********************************************************************/
static int asr_at_show_command(int argc, char **argv)
{
    int i;

    if(argc ==  1)
    {
        LOG_I("%s",cmd_table[0].help);

        for(i = 1; i < sizeof(cmd_table) / sizeof(cmd_table[0]); i++)
        {
            if(memcmp(cmd_table[i].command, "asr", 4) != 0) //asr_ cmd is internal cmd, will not show to customer
            {
                LOG_I("%s",cmd_table[i].command);
            }
        }
    }
    else if(argc == 2)
    {
        for(i = 1; i < sizeof(cmd_table) / sizeof(cmd_table[0]); i++)
        {
            if(strcmp(argv[1],cmd_table[i].command) == 0)
            {
                if(cmd_table[i].help)
                {
                    LOG_I("%s",cmd_table[i].help);
                }

                break;
            }
        }
    }
    else
    {
        LOG_I("cmd param error\r\n");
        return CONFIG_FAIL;
    }
    return CONFIG_OK;
}

/*
 *********************************************************************
 * @brief make argv point to uart_buf address,return number of command
 *
 * @param[in] buf point to uart_buf
 * @param[in] argv point to char *argv[ARGCMAXLEN];
 *
 * @return number of command param
 *********************************************************************
 */
static int parse_cmd(char *buf, char **argv)
{
    int argc = 0;
    //buf point to uart_buf
    while((argc < ARGCMAXLEN) && (*buf != '\0'))
    {
        argv[argc] = buf;
        argc++;
        buf++;
        //space and NULL character
        while((*buf != ' ') && (*buf != '\0'))
            buf++;
        //one or more space characters
        while(*buf == ' ')
        {
            *buf = '\0';
            buf ++;
         }
    }
    return argc;
}

/*
 *********************************************************************
 * @brief entry function of at module
 *
 *********************************************************************
 */

asr_thread_hand_t AT_Task_Handler = {0};
//asr_thread_t UART_Task_Handler = 0;
asr_queue_stuct_t  at_task_msg_queue;
#define AT_QUEUE_SIZE UART_CMD_NB


#ifdef THREADX
void uart_at_msg_send(char *cmd_str)
{
    struct uart_at_msg uart_at_msg={NULL};
    uart_at_msg.str = (uint32_t)cmd_str;
    //dbg(D_ERR, D_UWIFI_CTRL, "%s <<< 1 0x%x", __func__, cmd_str);
    asr_rtos_push_to_queue(&at_task_msg_queue, &uart_at_msg, ASR_NEVER_TIMEOUT);
    //dbg(D_ERR, D_UWIFI_CTRL, "%s <<< 2", __func__);
}

void at_command_process(char *cmd_str)
{
    int i, argc;
    char *argv[ARGCMAXLEN];
    int ret;
    if((argc = parse_cmd(cmd_str, argv)) > 0)
    {
        for(i = 0; i < sizeof(cmd_table) / sizeof(cmd_table[0]); i++)
        {
            //LOG_I("command:%s - argv:%s\r\n", cmd_table[i].command, argv[0]);
            if(strcmp(argv[0], cmd_table[i].command) == 0)
            {
                asr_api_adapter_lock();
                ret = cmd_table[i].function(argc, argv);
                asr_api_adapter_try_unlock(ret);
                break;
            }
        }

        for(i = 0; i < user_cmd_num; i++)
        {
            if(strcmp(argv[0], user_cmd_table[i].command) == 0)
            {
                asr_api_adapter_lock();
                ret = user_cmd_table[i].function(argc, argv);
                asr_api_adapter_try_unlock(ret);
                break;
            }
        }
    }
}

#else
void at_handle_uartirq(char ch)
{
    uint32_t msg_queue_elmt;

    if(AT_Task_Handler.thread && !(((start_idx + 1)%UART_CMD_NB) == cur_idx))
    {
        if(ch =='\r')//in windows platform '\r\n' added to end
        {
            uart_buf[start_idx][uart_idx]='\0';
            uart_buf_len[start_idx]=uart_idx;
            uart_idx=0;
            if(start_idx == UART_CMD_NB -1)
                start_idx = 0;
            else
                start_idx++;
            asr_rtos_push_to_queue(&at_task_msg_queue, &msg_queue_elmt, ASR_NO_WAIT);
        }
        else if(ch =='\b')
        {
            if(uart_idx>0)
                uart_buf[start_idx][--uart_idx] = '\0';
        }
        else if(ch > (MIN_USEFUL_DEC - 1) && ch<(MAX_USEFUL_DEC + 1))
        {
            uart_buf[start_idx][uart_idx++] = ch;
            if(uart_idx > UART_RXBUF_LEN - 1 ) {
                LOG_I("error:uart char_str length must <= 127,return\n");
                uart_idx=0;
            }
        }
    }
}

void at_command_process(char *cmd_str)
{
    int i, argc;
    char *argv[ARGCMAXLEN];
    int ret;
    if((argc = parse_cmd(uart_buf[cur_idx], argv)) > 0)
    {
        for(i = 0; i < sizeof(cmd_table) / sizeof(cmd_table[0]); i++)
        {
            //LOG_I("command:%s - argv:%s\r\n", cmd_table[i].command, argv[0]);
            if(strcmp(argv[0], cmd_table[i].command) == 0)
            {
                asr_api_adapter_lock();
                ret = cmd_table[i].function(argc, argv);
                asr_api_adapter_try_unlock(ret);
                break;
            }
        }

        for(i = 0; i < user_cmd_num; i++)
        {
            if(strcmp(argv[0], user_cmd_table[i].command) == 0)
            {
                asr_api_adapter_lock();
                ret = user_cmd_table[i].function(argc, argv);
                asr_api_adapter_try_unlock(ret);
                break;
            }
        }
    }

    memset(uart_buf[cur_idx],0,UART_RXBUF_LEN);

    if(cur_idx == UART_CMD_NB - 1)
    {
        cur_idx = 0;
    }else
        cur_idx ++;

}
#endif    // !THREADX

void AT_task(asr_thread_arg_t arg)
{
    uart_at_msg_t at_cmd_msg;

    //asr_rtos_set_semaphore(&at_cmd_protect);
    asr_api_adapter_unlock();

    while(SCAN_DEINIT == scan_done_flag)
        asr_rtos_delay_milliseconds(5);

    while(SCAN_DEINIT != scan_done_flag)
    {
        LOG_I(":");
        asr_rtos_pop_from_queue(&at_task_msg_queue, &at_cmd_msg, ASR_WAIT_FOREVER);
        //LOG_I("%s 0x%x\r\n", __func__, (void *)(at_cmd_msg.str));
        at_command_process((char *)(at_cmd_msg.str));
    }
}

int asr_at_init(void)
{
    OSStatus ret;
    asr_task_config_t cfg;

    if(!AT_Task_Handler.thread)
    {

        // common for asr api adapter(AT cmd)sync.
        asr_api_adapter_lock_init();
        asr_wifi_event_cb_register();

        asr_at_cmd_helper_init();

        ret = asr_rtos_init_queue(&at_task_msg_queue, "AT_TASK_QUEUE", sizeof(uart_at_msg_t), AT_QUEUE_SIZE);
        if (ret == kGeneralErr) {
            LOG_I("init queue failed\r\n");
            return -1;
        }
        if (kNoErr != asr_rtos_task_cfg_get(ASR_TASK_CONFIG_UWIFI_AT, &cfg)) {
            return -1;
        }
        ret = asr_rtos_create_thread(&AT_Task_Handler, cfg.task_priority,UWIFI_AT_TASK_NAME, AT_task, cfg.stack_size, 0);
        if (ret == kGeneralErr) {
            LOG_I("create thread failed\r\n");
            return -1;
        }
        dbg(D_ERR, D_UWIFI_CTRL, "%s <<<", __func__);

        char c = 0;
        while (1 == cli_getchar(&c))
        {
            if (c == 3)  // ^C: exit
                break;
            printf("%c", c);
            at_handle_uartirq(c);
        }

        asr_at_deinit(0,0);

    }
    return 0;
}

int asr_at_deinit(int argc, char **argv)
{
    if(AT_Task_Handler.thread)
    {
        asr_rtos_delete_thread(&AT_Task_Handler);
        AT_Task_Handler.thread = 0;
        asr_rtos_deinit_queue(&at_task_msg_queue);
    }
    return 0;
}

int asr_at_wlan_get_err_code(int argc, char **argv)
{
    uint8_t mdoe = 0;
    if(argc == 2)
    {
        mdoe = atoi(argv[1]);
    }
    else
    {
        LOG_I("%s:error param exit\r\n",__func__);
        return PARAM_RANGE;
    }
    asr_wlan_get_err_code(mdoe);
    return CONFIG_OK;
}

int asr_at_get_fm_version(int argc, char **argv)
{
    int ret = -1 ;
    struct asr_hw  *asr_hw    = uwifi_get_asr_hw();
    if(asr_hw==NULL)
    {
        LOG_I("asr_hw is null\n");
        return CONFIG_FAIL;
    }

    if ((ret = asr_send_fw_softversion_req(asr_hw, &asr_hw->fw_softversion_cfm)))
    {
        LOG_I("fw req is err ret:%d\n",ret);
        return CONFIG_FAIL;
    }

    LOG_I("%s fw version is%s\r\n",__func__,asr_hw->fw_softversion_cfm.fw_softversion);

    return CONFIG_OK;
}

int asr_at_iperf(int argc, char **argv)
{
//    dbg(D_INF, D_AT, "call asr_wifi_iperf, not implement yet!");

    asr_wifi_iperf(argc, argv);
    return CONFIG_OK;
}

MSH_CMD_EXPORT_ALIAS(asr_at_iperf, at_wifi_iperf, ASR wifi iperf);


void AosWrap_asr_at_init(int argc, char **argv)
{
    int ret = 0;

    ret = asr_wlan_init();
    if (ret) {
        dbg(D_CRT, D_AT, "call asr_wlan_init failed!");
    }

    ret = asr_at_init();
    if (ret) {
        dbg(D_CRT, D_AT, "call asr_at_init failed!");
    }
}


MSH_CMD_EXPORT_ALIAS(AosWrap_asr_at_init, asr_cli, ASR wifi cli console);


#endif  // AT_USER_DEBUG
