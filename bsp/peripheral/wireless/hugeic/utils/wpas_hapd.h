#ifndef _HGIC_WPAS_HAPD_H_
#define _HGIC_WPAS_HAPD_H_

void hgics_strcpy(char *dest, char *src, int cnt);
char *hgics_strchr(char *str, char s, int index);
int  hgics_str_cutdown(char *str,char s,char *output,unsigned int buff_len);
void hgics_wpacli_disable_network(char *ifname);
void hgics_wpacli_remove_network(char *ifname);
void hgics_wpacli_enable_network(char *ifname);
void hgics_wpacli_scan(char *ifname,char *param);
void hgics_wpacli_cancel_scan(char *ifname);
int  hgics_wpacli_get_scan_result(char *ifname,char *buff,unsigned int buff_len);
void hgics_wpacli_set_country(char *ifname,char *country_code);
void hgics_wpacli_get_country(char *ifname,char *country_code,unsigned int buff_len);
void hgics_wpacli_set_ssid(char *ifname,char *ssid);
void hgics_wpacli_set_psk(char *ifname,char *psk);
void hgics_wpacli_set_keymgmt(char *ifname,char *keymgmt);
void hgics_wpacli_get_status(char *ifname,char *buff,unsigned int buff_len);
int  hgics_wpacli_get_rssi(char *ifname);
void hgics_wpacli_disconnect(char *ifname);
void hgics_wpacli_set_scan_interval(char *ifname,unsigned int interval);
void hgics_hapdcli_get_status(char *ifname,char *buff,unsigned int buff_len);
void hgics_hapdcli_deauth(char *ifname);
void hgics_hapdcli_stop_ap(char *ifname);
int  hgics_hapdcli_update_config(char *ifname,char *reply_buff,int reply_buff_len);

#endif

