#ifndef UWIFI_ASR_API_H_
#define UWIFI_ASR_API_H_


void clear_all_flag(void);
void init_all_flag(void);

void asr_wlan_set_scan_status(asr_scan_status_e _status);
void set_wifi_ready_status(asr_wifi_ready_status_e _status);
void asr_wlan_set_disconnect_status(asr_wlan_err_status_e _status);

void asr_wlan_set_wifi_ready_status(asr_wifi_ready_status_e _status);

int asr_wlan_start_scan_detail(char *ssid, int channel, char *bssid);

int asr_wlan_get_wifi_mode(void);
int asr_wlan_reset();

void asr_api_adapter_lock(void);
void asr_api_adapter_try_unlock(asr_api_rsp_status_t status);
void asr_api_adapter_unlock();
void asr_api_adapter_lock_init();

#endif