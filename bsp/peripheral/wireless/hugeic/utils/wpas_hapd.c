//#include "os_porting.h"
#include <linux/defs.h>
#include <linux/types.h>
#include "hgic.h"

#define STR_CUTDOWN_BUFFER_LEN  (2048)

#ifndef MAC2STR
#define MAC2STR(a) (a)[0]&0xff, (a)[1]&0xff, (a)[2]&0xff, (a)[3]&0xff, (a)[4]&0xff, (a)[5]&0xff
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define COMPACT_MACSTR "%02x%02x%02x%02x%02x%02x"
#endif

#define hgic_dbg(fmt, ...) PRINTF("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define hgic_err(fmt, ...) printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

int hgics_str_cutdown(char *str,char s,char *output,unsigned int buff_len)
{
    char *buff = NULL;
    char *ptr  = NULL;

    if(str == NULL) {
        hgic_dbg("Input param error!\n");
        return -1;
    }
    if(strlen(str) == 0 || strlen(str) > STR_CUTDOWN_BUFFER_LEN) {
        hgic_dbg("String too long,plz small than %d\n",STR_CUTDOWN_BUFFER_LEN);
        return -1;
    }

    buff = MALLOC(STR_CUTDOWN_BUFFER_LEN);
    if(buff == NULL) {
        hgic_dbg("Input param error!\n");
        return -1;
    }
    memset(buff,0,STR_CUTDOWN_BUFFER_LEN);
    ptr = buff;
    while(*str) {
        if(*str == s) {
            break;
        }
        *ptr = *str;
        ptr++;
        str++;
    }

    if(output) {
        memset(output,0,buff_len);
        memcpy(output,buff,buff_len > strlen(buff) ? strlen(buff) : buff_len);
    }
    FREE(buff);
    return 0;
}

void hgics_wpacli_disable_network(char *ifname)
{
    char buff[128];
    memset(buff,0,sizeof(buff));
    wpas_cli(ifname, "DISABLE_NETWORK 0 ", buff, sizeof(buff));
}

void hgics_wpacli_remove_network(char *ifname)
{
    char buff[128];
    memset(buff,0,sizeof(buff));
    wpas_cli(ifname, "REMOVE_NETWORK 0 ", buff, sizeof(buff));
}

void hgics_wpacli_enable_network(char *ifname)
{
    char buff[128];
    memset(buff,0,sizeof(buff));
    wpas_cli(ifname, "ENABLE_NETWORK 0 ", buff, sizeof(buff));
}

void hgics_wpacli_scan(char *ifname,char *param)
{
    char buff[32];
    char hex_output[64];

    memset(buff,0,sizeof(buff));
    if(param == NULL) {
        wpas_cli(ifname, "SCAN", buff, sizeof(buff));
    }
    else {
        char cmd[256] = "SCAN ssid ";
        hgic_dbg("Check input scan ssid:%s\n",param);
        memset(hex_output,0,sizeof(hex_output));
        stringToHexArray(param, hex_output, sizeof(hex_output));
        hgic_print_hex(hex_output, 16);
        append_hex_to_string(cmd, sizeof(cmd), hex_output, strlen(param));
        hgic_dbg("\nSet scan cmd:%s\n",cmd);
        wpas_cli(ifname, cmd, buff, sizeof(buff));
    }
}

void hgics_wpacli_cancel_scan(char *ifname)
{
    char buff[32];
    memset(buff,0,sizeof(buff));
    wpas_cli(ifname, "ABORT_SCAN", buff, sizeof(buff));
}

int hgics_wpacli_get_scan_result(char *ifname,char *buff,unsigned int buff_len)
{
    memset(buff,0,buff_len);
    wpas_cli(ifname, "SCAN_RESULTS", buff, buff_len);
}

void hgics_wpacli_set_country(char *ifname,char *country_code)
{
    char cmd[128];
    char out[128];
    memset(cmd,0,sizeof(cmd));
    memset(out,0,sizeof(out));
    snprintf(cmd, 128,"SET COUNTRY=\"%s\"",country_code);
    hgic_dbg("Set cmd:%s\n",cmd);
    wpas_cli(ifname,cmd,out,sizeof(out));
}

void hgics_wpacli_get_country(char *ifname,char *country_code,unsigned int buff_len)
{
    char out[128];
    memset(out,0,sizeof(out));
    wpas_cli(ifname,"GET COUNTRY",out,sizeof(out));
    if(country_code) {
        memcpy(country_code,out,strlen(out) > buff_len ? buff_len : strlen(out));
    }
}

void hgics_wpacli_set_ssid(char *ifname,char *ssid)
{
    char cmd[128];
    char out[128];
    memset(cmd,0,sizeof(cmd));
    memset(out,0,sizeof(out));
    snprintf(cmd, 128,"SET_NETWORK 0 ssid \"%s\"",ssid);
    hgic_dbg("Set ssid cmd:%s\n",cmd);
    wpas_cli(ifname,cmd,out,sizeof(out));
}
void hgics_wpacli_set_bssid(char *ifname,char *bssid)
{
    char cmd[128];
    char out[128];
    memset(cmd,0,sizeof(cmd));
    memset(out,0,sizeof(out));
    snprintf(cmd, 128,"BSSID 0 %s",bssid);
    hgic_dbg("Set ssid cmd:%s\n",cmd);
    wpas_cli(ifname,cmd,out,sizeof(out));
}

void hgics_wpacli_set_psk(char *ifname,char *psk)
{
    char cmd[128];
    char out[128];
    memset(cmd,0,sizeof(cmd));
    memset(out,0,sizeof(out));
    snprintf(cmd, 128,"SET_NETWORK 0 psk \"%s\"",psk);
    hgic_dbg("Set psk cmd:%s\n",cmd);
    wpas_cli(ifname,cmd,out,sizeof(out));
}

void hgics_wpacli_set_keymgmt(char *ifname,char *keymgmt)
{
    char out[128];

    memset(out,0,sizeof(out));
    if(strcmp(keymgmt, "NONE") == 0) {
        wpas_cli(ifname,"SET_NETWORK 0 key_mgmt NONE",out,sizeof(out));
    } else {
        wpas_cli(ifname,"SET_NETWORK 0 key_mgmt WPA-PSK",out,sizeof(out));
    }
}

void hgics_wpacli_get_status(char *ifname,char *buff,unsigned int buff_len)
{
    if(ifname == NULL || buff == NULL) {
        hgic_err("Input param error!\n");
        return;
    }
    memset(buff,0,buff_len);
    wpas_cli(ifname,"STATUS",buff,buff_len);
}

int hgics_wpacli_get_rssi(char *ifname)
{
    char *buff = NULL;
    char *ptr = NULL;
    int rssi   = 0;

    buff = MALLOC(1024);
    if(!buff) {
        hgic_err("Error,no memory!\n");
        return -1;
    }
    wpas_cli(ifname,"SIGNAL_POLL",buff, 1024);
    ptr = strstr(buff, "AVG_RSSI=");
    if (ptr) {
        rssi = atoi(ptr + 9);
    }
    FREE(buff);
    return rssi;
}

void hgics_wpacli_remove_interface(char *ifname)
{
    char cmd[128];
    char out[128];
    memset(cmd,0,sizeof(cmd));
    memset(out,0,sizeof(out));
    snprintf(cmd, 128,"INTERFACE_REMOVE \"%s\"",ifname);
    hgic_dbg("Remove interface cmd:%s\n",cmd);
    wpas_cli(ifname,cmd,out,sizeof(out));
}

void hgics_wpacli_set_freq(char *ifname,unsigned int freq)
{
    char cmd[128];
    char out[128];
    memset(cmd,0,sizeof(cmd));
    memset(out,0,sizeof(out));
    snprintf(cmd, 128,"SET freq=%d\"",freq);
    hgic_dbg("set freq cmd:%s\n",cmd);
    wpas_cli(ifname,cmd,out,sizeof(out));
}

void hgics_hapdcli_get_status(char *ifname,char *buff,unsigned int buff_len)
{
    if(ifname == NULL || buff == NULL) {
        hgic_err("Input param error!\n");
        return;
    }
    memset(buff,0,buff_len);
    hapd_cli(ifname,"STATUS",buff,buff_len);
}

void hgics_hapdcli_deauth(char *ifname)
{
    char buff[32];
    memset(buff,0,sizeof(buff));
    hapd_cli(ifname, "DEAUTHENTICATE", buff, sizeof(buff));
}

void hgics_wpacli_disconnect(char *ifname)
{
    char buff[32];
    wpas_cli(ifname, "DISCONNECT", buff, sizeof(buff));
}

void hgics_wpacli_reconnect(char *ifname,char *param)
{
    char buff[32];
    wpas_cli(ifname, "RECONNECT", buff, sizeof(buff));
}

void hgics_wpacli_select_network(char *ifname,char *param)
{
    char cmd[128];
    char out[128];
    memset(cmd,0,sizeof(cmd));
    memset(out,0,sizeof(out));
    snprintf(cmd, 128,"SELECT_NETWORK \"%s\"",param);
    hgic_dbg("select network cmd:%s\n",cmd);
    wpas_cli(ifname,cmd,out,sizeof(out));
    wpas_cli(ifname,"SELECT_NETWORK 0",out,sizeof(out));
}

void hgics_wpacli_set_scan_interval(char *ifname,unsigned int interval)
{
    char cmd[128];
    char out[128];
    memset(cmd,0,sizeof(cmd));
    memset(out,0,sizeof(out));
    snprintf(cmd, 128,"SCAN_INTERVAL %d\"",interval);
    hgic_dbg("set scan interval cmd:%s\n",cmd);
    wpas_cli(ifname,cmd,out,sizeof(out));
}


void hgics_hapdcli_stop_ap(char *ifname)
{
    char buff[32];
    memset(buff,0,sizeof(buff));
    hapd_cli(ifname, "STOP_AP", buff, sizeof(buff));
}

int hgics_hapdcli_update_config(char *ifname,char *reply_buff,int reply_buff_len)
{
    return hapd_cli(ifname, "UPDATE_CONF", reply_buff, reply_buff_len);
}

int hgics_hapdcli_set_ssid(char *ifname,char *ssid)
{
    char cmd[128];
    char out[128];
    memset(cmd,0,sizeof(cmd));
    memset(out,0,sizeof(out));
    snprintf(cmd, 128,"SET ssid %s",ssid);
    hgic_dbg("set ssid cmd:%s\n",cmd);
    hapd_cli(ifname,cmd,out,sizeof(out));
}

void hgics_hapdcli_reload(char *ifname)
{
    char buff[32];
    hapd_cli(ifname, "RELOAD", buff, sizeof(buff));
}

void hgics_hapdcli_disable_ap(char *ifname)
{
    char buff[32];
    hapd_cli(ifname, "DISABLE", buff, sizeof(buff));
}

void hgics_hapdcli_enable_ap(char *ifname)
{
    char buff[32];
    hapd_cli(ifname, "ENABLE", buff, sizeof(buff));
}


