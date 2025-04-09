/**
 ****************************************************************************************
 *
 * @file uwifi_msg_rx.c
 *
 * @brief RX function definitions
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#include "uwifi_common.h"

#include "uwifi_include.h"
#include "uwifi_msg_rx.h"
#include "uwifi_ops_adapter.h"
#include "uwifi_wpa_api.h"
#include "uwifi_asr_api.h"
#include "uwifi_tx.h"
#include "uwifi_rx.h"
#include "asr_wlan_api.h"
#include "asr_wlan_api_aos.h"
#include "uwifi_notify.h"
#include "uwifi_interface.h"
#include "uwifi_msg.h"
#include "wpas_psk.h"
#include "wpa_adapter.h"
#include "asr_rtos_api.h"
#if (defined(CONFIG_IEEE80211W) || NX_SAE)
#include "wpa_common.h"
#include "defs.h"
#endif
#include "hostapd.h"

#if NX_SAE
#define MAC_PRIV_RS_BEACON_LOSS          WLAN_REASON_UNSPECIFIED  // fw define.
#else
#define MAC_PRIV_RS_BEACON_LOSS                  0x100
#endif

//#define MAC_PRIV_RS_AUTH_FAIL             0x200
//#define MAC_PRIV_RS_SAE_AUTH_CONFIRM_FAIL      0x201
//#define MAC_PRIV_RS_ASSOC_FAIL         0x300
//#define MAC_PRIV_RS_DEAUTH_CFM                 0x400

extern int scan_done_flag;
extern uint8_t wifi_ready;
//extern uint8_t wifi_psk_is_ture;
extern uint32_t current_iftype;

#ifdef CFG_ADD_API
#include "asr_wlan_api.h"
#endif
//extern uint8_t wifi_connect_err ;
extern uint8_t wifi_conect_fail_code;
extern const int nx_txdesc_cnt_msk[];

extern struct sta_info g_connect_info;
#if NX_SAE
#if NX_SAE_PMK_CACHE
extern struct pmkid_caching user_pmkid_caching;
#endif
#endif

static int asr_freq_to_idx(struct asr_hw *asr_hw, int freq)
{
    struct ieee80211_supported_band *sband;
    int band, ch, idx = 0;

    for (band = IEEE80211_BAND_2GHZ; band < IEEE80211_NUM_BANDS; band++) {
        sband = asr_hw->wiphy->bands[band];
        if (!sband) {
            continue;
        }

        for (ch = 0; ch < sband->n_channels; ch++, idx++) {
            if (sband->channels[ch].center_freq == freq) {
                goto exit;
            }
        }
    }
exit:
    // Channel has been found, return the index
    return idx;
}

uint8_t* uwifi_mac_ie_find(uint8_t *addr, uint32_t buflen, uint32_t ie_id)
{
    if (!addr)
        return NULL;

    uint32_t end = (uint32_t)addr + buflen;
    //loop as long as we do not go beyond the frame size
    while ((uint32_t)addr < end)
    {
        //check if the current IE is the one looked for
        if (ie_id == (*addr))
        {
            //the IE id matches and it is not OUI IE, return the pointer to this IE
            return addr;
        }
        //move on to the next IE
        addr += addr[MAC_INFOELT_LEN_OFT] + MAC_INFOELT_INFO_OFT;
    }

    return NULL;
}

uint8_t* uwifi_mac_ext_ie_find(uint8_t *addr, uint32_t buflen, uint32_t ie_id, uint8_t ext_ie_id)
{
    if (!addr)
        return NULL;

    addr = uwifi_mac_ie_find(addr,buflen,ie_id);

    if(!addr)
        return NULL;

    if (ext_ie_id == *(uint8_t *)((uint32_t)(addr + MAC_INFOELT_EXT_ID_OFT)))
    {
        // the extension field matches, return the pointer to this IE
        return addr;
    }

    return NULL;
}

uint8_t * wpa_get_ie(uint8_t *addr, uint16_t buflen,
                                               uint32_t vendor_type)
{
    uint32_t end = (uint32_t)addr + buflen;
    //end = pos + bss->ie_len;

    while ((uint32_t)addr + 1 < end) {
        if ((uint32_t)addr + 2 + addr[1] > end)
            break;
        if (addr[0] == vendor_type && addr[1] >= 4 &&
                        WPA_OUI == WPA_GET_BE32(&addr[2]))
            return addr;
        addr += 2 + addr[1];
    }

    return NULL;
}

uint8_t * rsn_get_ie(uint8_t *addr, uint16_t buflen,
                                               uint32_t vendor_type)
{
    uint32_t end = (uint32_t)addr + buflen;
    //end = pos + bss->ie_len;

    while ((uint32_t)addr + 1 < end) {
        if ((uint32_t)addr + 2 + addr[1] > end)
            break;
        if (addr[0] == vendor_type && addr[1] >= 4 &&
                        RSN_OUI == (WPA_GET_BE24(&addr[4])))
                        //RSN_OUI == (0x00FFFFFF & (WPA_GET_BE32(&addr[4])>>8)))
            return addr;
        addr += 2 + addr[1];
    }

    return NULL;
}

#ifdef CFG_STATION_SUPPORT
void sorting_scan_result_list(struct asr_hw *asr_hw)
{
    int i;
    asr_hw->phead_scan_result = asr_hw->scan_list;
    struct scan_result* current;
    struct scan_result* next;

    for(i=1;i<asr_hw->ap_num;i++)
    {
        current = asr_hw->phead_scan_result;
        next = current->next_ptr;

        if(asr_hw->scan_list[i].rssi > current->rssi)
        {
            asr_hw->scan_list[i].next_ptr = current;
            asr_hw->phead_scan_result = &asr_hw->scan_list[i];
            continue;
        }
        else
        {
            while(NULL != next)
            {
                if(asr_hw->scan_list[i].rssi > next->rssi)
                {
                    asr_hw->scan_list[i].next_ptr = next;
                    break;
                }
                else
                {
                    current = next;
                    next = next->next_ptr;
                }
            }
            current->next_ptr = &asr_hw->scan_list[i];
        }
    }
}
#endif


static inline int asr_rx_p2p_vif_ps_change_ind(struct asr_hw *asr_hw,
                                                struct asr_cmd *cmd,
                                                struct ipc_e2a_msg *msg)
{
    int vif_idx  = ((struct mm_p2p_vif_ps_change_ind *)msg->param)->vif_index;
    int ps_state = ((struct mm_p2p_vif_ps_change_ind *)msg->param)->ps_state;
    struct asr_vif *vif_entry;

    vif_entry = asr_hw->vif_table[vif_idx];

    if (vif_entry) {
        goto found_vif;
    }

    goto exit;

found_vif:


    if (ps_state == MM_PS_MODE_OFF) {
        // Start TX queues for provided VIF
        asr_txq_vif_start(vif_entry, ASR_TXQ_STOP_VIF_PS, asr_hw);
    }
    else {
        // Stop TX queues for provided VIF
        asr_txq_vif_stop(vif_entry, ASR_TXQ_STOP_VIF_PS, asr_hw);
    }

exit:
    return 0;
}


int asr_rx_channel_survey_ind(struct asr_hw *asr_hw,
                                             struct asr_cmd *cmd,
                                             struct ipc_e2a_msg *msg)
{
    struct mm_channel_survey_ind *ind = (struct mm_channel_survey_ind *)msg->param;
    // Get the channel index
    int idx = asr_freq_to_idx(asr_hw, ind->freq);
    //dbg(D_DBG,D_UWIFI_CTRL,"%s find channel idx=%d \r\n", __func__,idx);
    // Get the survey
    struct asr_survey_info *asr_survey = &asr_hw->survey[idx];
    // Store the received parameters
    asr_survey->chan_time_ms = ind->chan_time_ms;
    asr_survey->chan_time_busy_ms = ind->chan_time_busy_ms;
    asr_survey->noise_dbm = ind->noise_dbm;
    asr_survey->filled = (SURVEY_INFO_CHANNEL_TIME |
                           SURVEY_INFO_CHANNEL_TIME_BUSY);

    if (ind->noise_dbm != 0) {
        asr_survey->filled |= SURVEY_INFO_NOISE_DBM;
    }
    return 0;
}

extern uint8_t softap_scan;
extern asr_wlan_cb_scan_compeleted asr_wlan_vendor_scan_comp_callback;
extern asr_semaphore_t asr_wlan_vendor_scan_semaphore;
extern uint8_t is_scan_adv;
//extern int only_scan_done_flag;
//extern asr_mutex_t g_only_scan_mutex;
#ifdef CFG_STATION_SUPPORT
/***************************************************************************
 * Messages from SCANU task
 **************************************************************************/
int asr_rx_scanu_start_cfm(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    struct asr_vif *asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];
    asr_wlan_scan_result_t scan_results;
    int i = 0;

    asr_clear_bit(ASR_DEV_SCANING, &asr_vif->dev_flags);
    memset(&scan_results, 0, sizeof(scan_results));

    asr_rtos_stop_timer(&(asr_hw->scan_cmd_timer));

    asr_hw->ap_num = asr_hw->scan_count;
    dbg(D_CRT,D_UWIFI_CTRL,"%s ap_num:%d", __func__, asr_hw->ap_num);
    if (asr_hw->preq_ie.buf) {
        asr_rtos_free(asr_hw->preq_ie.buf);
        asr_hw->preq_ie.buf = NULL;
    }

    if (asr_hw->scan_request)
    {
        sorting_scan_result_list(asr_hw);
        int j;
        for(j=0;j<asr_hw->scan_request->n_channels;j++)
        {
            asr_rtos_free(asr_hw->scan_request->channels[j]);
            asr_hw->scan_request->channels[j] = NULL;
        }
        asr_rtos_free(asr_hw->scan_request);
        asr_hw->scan_request = NULL;
    }

    if(softap_scan == 0)
    {
        if(asr_vif->sta.connInfo.wifiState >= WIFI_ASSOCIATED)
            asr_vif->sta.connInfo.wifiState = LINKED_SCAN_DONE;
        else
            asr_vif->sta.connInfo.wifiState = WIFI_SCAN_DONE;
    }


    scan_results.is_scan_adv = is_scan_adv;
    scan_results.ap_num = asr_hw->ap_num;
    if (scan_results.ap_num > 0) {
        scan_results.ap_list = asr_rtos_malloc(sizeof(*scan_results.ap_list)*scan_results.ap_num);
        if(NULL == scan_results.ap_list)
        {
            dbg(D_ERR,D_UWIFI_CTRL,"scancfm aplist NULL");
            goto scanu_cfm;
        }
    }

    struct scan_result* current = asr_hw->phead_scan_result;

    #ifdef CFG_ADD_API
    uap_config_scan_chan _scan_results;
    memset(&_scan_results,0,sizeof(_scan_results));
    _scan_results.action = 0;//get function
    if(current->channel.band == IEEE80211_BAND_2GHZ)
        _scan_results.band = 1;
    _scan_results.total_chan = scan_results.ap_num ; // total channel
    #endif

    while((i < scan_results.ap_num)&&(NULL != current))
    {
        if(SCAN_DEINIT == scan_done_flag)
        {
            if (scan_results.ap_num > 0)
            {
                asr_rtos_free(scan_results.ap_list);
                scan_results.ap_list = NULL;
            }
            return 0;
        }

        #ifdef CFG_ADD_API
        _scan_results.chan_num[current->chn]++;
        #endif

        if(i==0)
            dbg(D_CRT,D_UWIFI_CTRL,"[ap:%d] name=%s | rssi=%d | encry=%d | channel=%d | sae_h2e=%d | bssid=%02x:%02x:%02x:%02x:%02x:%02x \r\n",i,current->ssid.ssid,current->rssi,
            current->encryp_protocol,current->chn,current->sae_h2e,
            current->bssid[0],current->bssid[1],current->bssid[2],current->bssid[3],current->bssid[4],current->bssid[5]);
        else if(i==1)
            dbg(D_CRT,D_UWIFI_CTRL,"scan result is %d",scan_results.ap_num);

        memcpy(scan_results.ap_list[i].ssid,current->ssid.ssid,current->ssid.ssid_len);
        scan_results.ap_list[i].ap_power = current->rssi;
        memcpy(scan_results.ap_list[i].bssid,current->bssid,ETH_ALEN);
        scan_results.ap_list[i].channel = current->chn;
        scan_results.ap_list[i].security = current->encryp_protocol;
        i++;
        current = current->next_ptr;
    }

    #ifdef CFG_ADD_API
    if(!Get_Scan_Flag())
    {
        Set_Scan_Chan(_scan_results);
        Set_Scan_Flag(1);
    }
    #endif

    //this is for vendor scan such as midea specific scan
    if(asr_wlan_vendor_scan_comp_callback)
    {
        asr_wlan_vendor_scan_comp_callback(&scan_results);
        asr_wlan_vendor_scan_comp_callback = 0;
        if(asr_wlan_vendor_scan_semaphore)
        {
            dbg(D_ERR, D_UWIFI_CTRL, "set sem asr_wlan_vendor_scan_semaphore");
            asr_rtos_set_semaphore(&asr_wlan_vendor_scan_semaphore);
        }
    }

    wifi_event_cb(WLAN_EVENT_SCAN_COMPLETED, (void *)&scan_results);

    //asr_rtos_lock_mutex(&g_only_scan_mutex);
    //only_scan_done_flag = 1;
    //asr_rtos_unlock_mutex(&g_only_scan_mutex);

    if (scan_results.ap_num > 0) {
        asr_rtos_free(scan_results.ap_list);
        scan_results.ap_list = NULL;
    }

    /*like Android, when scan completed, if there have saved AP, compare and try to auto connect.
      use phead_scan_result list, had sorted, when have multi same ssid and same encryption, will connect to best rssi AP.
      impletion as function, if not need, can easily mark

      if connInfo.wifiState is WIFI_CONNECTING or >=WIFI_ASSOCIATED, no need auto connect
    */
    if(softap_scan == 0)
    {
        if((asr_vif->sta.connInfo.wifiState == WIFI_CONNECTING) || (asr_vif->sta.connInfo.wifiState >= WIFI_ASSOCIATED))
        {
            dbg(D_ERR, D_UWIFI_CTRL, "scancfm r");
            dbg(D_ERR, D_UWIFI_CTRL, "%s softap scan end! wifiState=%d \r\n",__func__,asr_vif->sta.connInfo.wifiState);

            if(asr_vif->sta.connInfo.wifiState == LINKED_SCAN_DONE)
                goto scanu_cfm;
            asr_wlan_set_scan_status(SCAN_OTHER_ERR);
            return 0;
        }
    }
    else
    {
        dbg(D_ERR, D_UWIFI_CTRL, "softap scan end!");
        softap_scan = 0;
    }
scanu_cfm:
    asr_wlan_set_scan_status(SCAN_FINISH_DOWN);
    return 0;
}

#ifdef CFG_ENCRYPT_SUPPORT
int wpa_scan_info_update_scan_list(struct scan_result *pscan_result, struct wpa_ie_data *ie_data)
{
    uint16_t wpa_ie_len = MAX_IE_LEN;
    enum nl80211_mfp mfp = NL80211_MFP_NO;
    struct pmkid_caching_entry *pmkid_cache_entry_ptr;
#if NX_SAE_PMK_CACHE
    unsigned char pmkid_cache_idx = NUM_PMKID_CACHE;
#endif

    pscan_result->crypto.cipher_group = RSN_CIPHER_SUITE_CCMP;
    if (ie_data->has_group)
    {
        pscan_result->crypto.cipher_group = wpa_cipher_to_suite(WPA_PROTO_RSN, ie_data->group_cipher);
    }

    pscan_result->crypto.ciphers_pairwise[0] = RSN_CIPHER_SUITE_CCMP;
    if (ie_data->has_pairwise)
    {
        pscan_result->crypto.ciphers_pairwise[0] = wpa_cipher_to_suite(WPA_PROTO_RSN, ie_data->pairwise_cipher);
    }

    pscan_result->crypto.akm_suites[0] = RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X;
    if (ie_data->key_mgmt)
    {
        pscan_result->crypto.akm_suites[0] = wpa_akm_to_suite(ie_data->key_mgmt);
    }

#if (defined(CONFIG_IEEE80211W) || NX_SAE)
    if (ie_data->capabilities & WPA_CAPABILITY_MFPC)
    {
        pscan_result->mfp = NL80211_MFP_CAPABLE;
        pscan_result->crypto.mfp_option = NL80211_MFP_CAPABLE;
    }
    if (ie_data->capabilities & WPA_CAPABILITY_MFPR)
    {
        pscan_result->mfp = NL80211_MFP_REQUIRED;
        pscan_result->crypto.mfp_option = NL80211_MFP_REQUIRED;
    }
    mfp = pscan_result->mfp;

    pscan_result->crypto.cipher_mgmt_group = RSN_CIPHER_SUITE_AES_128_CMAC;
    if (ie_data->mgmt_group_cipher)
    {
        pscan_result->crypto.cipher_mgmt_group = wpa_cipher_to_suite(WPA_PROTO_RSN, ie_data->mgmt_group_cipher);
    } else
        pscan_result->crypto.cipher_mgmt_group = 0;
#endif

    // ie used in assoc-request ,format
    // 1.use PMKSA and has pmkid&pmk store in station,
    //     element ID(1) + length(1) + version(2) + group data(4) + pairwise count + pairwise list + akm count + akm list + rsnc + pkmid + pkmid List +group M
    // all count=1, max length=44
#if NX_SAE_PMK_CACHE
    struct wpa_priv *p_wpa_priv =  g_pwpa_priv[COEX_MODE_STA];

    if (pscan_result->crypto.akm_suites[0] == RSN_AUTH_KEY_MGMT_SAE)
        pmkid_cache_idx = search_by_mac_pmkid_cache(p_wpa_priv, pscan_result->bssid);

    if (pmkid_cache_idx < NUM_PMKID_CACHE){
        //pmkid_cache_entry_ptr = &p_wpa_priv->wpa_global_info.pmkid_cache.pmkid_caching_entry[pmkid_cache_idx];
        pmkid_cache_entry_ptr = &user_pmkid_caching.pmkid_caching_entry[pmkid_cache_idx];
    }else
        pmkid_cache_entry_ptr = NULL;
#else
    pmkid_cache_entry_ptr = NULL;
#endif

    wpa_ie_len = wpa_gen_wpa_ie_rsn(pscan_result->ie, wpa_ie_len, ie_data->pairwise_cipher, ie_data->group_cipher,
        ie_data->key_mgmt, ie_data->mgmt_group_cipher, pmkid_cache_entry_ptr, mfp);

    if (wpa_ie_len > MAX_IE_LEN)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:rsn ie_len[%u] is more than MAX[%u]", __func__, wpa_ie_len, MAX_IE_LEN);
        return -1;
    }
    pscan_result->ie_len = wpa_ie_len;

    //dbg(D_INF, D_UWIFI_CTRL, "[%s]scan list group suit:(%x), pair suit:(%x), akm_suit:(%x), pmkidx:(%d)", __func__,
    //    (unsigned int)(pscan_result->crypto.cipher_group), (unsigned int)(pscan_result->crypto.ciphers_pairwise[0]),
    //    (unsigned int)(pscan_result->crypto.akm_suites[0]),pmkid_cache_idx);

    return 0;
}
#endif

int wpa_get_scan_info(struct asr_hw *asr_hw,struct scanu_result_ind *ind)
{
    uint8_t ssid_len;
    uint16_t frm_len;
    uint16_t scan_chan;
    uint8_t *frm_addr;
    uint8_t *elem_addr;
    int8_t min_rssi;
    bool sameap = false;
    int position = -1;
    struct ProbeRsp *frm = NULL;
    uint16_t unicast_chiper_count;

    int j = 0;
#ifdef CFG_ENCRYPT_SUPPORT
    struct wpa_ie_data data = {0};
#endif

    /*if small than MAX_AP_NUM
          will first compare whether same AP, if same, use latest ap replace old ap, signal use max rssi
          if not, add to array
      if large than MAX_AP_NUM
          find whether same ap, if same, replace old ap, signal use max rssi
          if not, find minimum rssi ap and replace
          if not find, discard
    */
    frm = (struct ProbeRsp *)ind->payload;

    /* temp rssi */
    min_rssi = ind->rssi;

    /* whether same ap*/
    for(j=0;j < asr_hw->scan_count;j++)
    {
         if(MAC_ADDR_CMP_6(frm->h.addr3.array, asr_hw->scan_list[j].bssid))
         {

             sameap = true;
             position = j;
             break;
         }

         if(asr_hw->scan_list[j].rssi<min_rssi)
         {
             min_rssi = asr_hw->scan_list[j].rssi;
             position = j;
         }
    }
    if ((!sameap) && (j == MAX_AP_NUM) && (position == -1))
    {
        dbg(D_ERR,D_UWIFI_CTRL,"more than MAX_AP_NUM AP, and rssi is minimum, discard");
        return 0;
    }
    else if ((!sameap) && (j < MAX_AP_NUM))
        position = j;
    /* 1 bssid */
    memcpy(asr_hw->scan_list[position].bssid,(void*)frm->h.addr3.array,ETH_ALEN);
    /* 2 rssi */
    if(sameap)
        asr_hw->scan_list[position].rssi= (int8_t)max_t(int8_t,ind->rssi,asr_hw->scan_list[position].rssi);
    else
        asr_hw->scan_list[position].rssi= ind->rssi;
    /* 3 ssid  refer to fw scanu_frame_handler */
    frm_addr = (frm->variable);
    frm_len = ind->length - PROBE_RSP_SSID_OFT;
    //dbg(D_ERR, D_UWIFI_CTRL, "frm_addr:0x%x frm_len:%d\r\n", frm_addr, frm_len);
    elem_addr = uwifi_mac_ie_find(frm_addr, frm_len, SSID_ELEM_ID);
    if (elem_addr != NULL)
    {
        ssid_len = *(elem_addr + SSID_LEN_OFT_ELEM);
        if (ssid_len > MAX_SSID_LEN)
            ssid_len = MAX_SSID_LEN;
        asr_hw->scan_list[position].ssid.ssid_len = ssid_len;
        memcpy(asr_hw->scan_list[position].ssid.ssid, elem_addr +SSID_OFT_ELEM ,ssid_len);
        asr_hw->scan_list[position].ssid.ssid[ssid_len] = 0;
    }
    else
    {
        // SSID is not broadcasted
        asr_hw->scan_list[position].ssid.ssid_len = 0;
    }
    /* 4 channel*/
    // retrieve the channel
    elem_addr = uwifi_mac_ie_find(frm_addr, frm_len, MAC_EID_DS);
    if (elem_addr != 0)
    {
        scan_chan = elem_addr[MAC_EID_DS_CHANNEL_OFT];
    }
    else
    {
        scan_chan = phy_freq_to_channel(ind->band, ind->center_freq);
    }

    if ((scan_chan == 0) || (scan_chan > 14))
    {
        dbg(D_ERR,D_UWIFI_CTRL,"wpa_get_scan_info: scan chan(%d) not support, not add in scan list", scan_chan);
        memset(&asr_hw->scan_list[position], 0, sizeof(struct scan_result));
        return -1;
    }
    asr_hw->scan_list[position].chn = scan_chan;
    asr_hw->scan_list[position].channel.band = ind->band;
    asr_hw->scan_list[position].channel.center_freq = phy_channel_to_freq(ind->band,scan_chan);
    asr_hw->scan_list[position].channel.flags =
                        passive_scan_flag(asr_hw->wiphy->bands[IEEE80211_BAND_2GHZ]->channels[scan_chan-1].flags);
    /* 5 encrpy: none  /wpa / rsn /wep  */
    if(((frm->capa) & BIT(4)) == 0)
    {
        asr_hw->scan_list[position].encryp_protocol = WIFI_ENCRYP_OPEN;
        asr_hw->scan_list[position].crypto.cipher_group = WPA_CIPHER_NONE;
        asr_hw->scan_list[position].crypto.ciphers_pairwise[0] = WPA_CIPHER_NONE;
    }
#ifdef CFG_ENCRYPT_SUPPORT
    else
    {
        elem_addr = rsn_get_ie(frm_addr, frm_len, RSN_ELEM_ID);
        if (elem_addr != NULL)
        {
            asr_hw->scan_list[position].encryp_protocol = WIFI_ENCRYP_WPA2;

            uint16_t  wpa_ie_len = elem_addr[1]+2;
            if (wpa_parse_wpa_ie_rsn(elem_addr, wpa_ie_len, &data) == 0)
            {
                if (data.key_mgmt & WPA_KEY_MGMT_SAE)
                    dbg(D_ERR, D_UWIFI_CTRL, "(%s):rsn ied data[%x (%x %x) (%x %x) (%x) (%x) (%x)][(%d)(%d)]",
                    asr_hw->scan_list[position].ssid.ssid,data.proto, data.pairwise_cipher, data.has_pairwise, data.group_cipher, data.has_group,
                    data.key_mgmt, data.capabilities, data.mgmt_group_cipher,asr_hw->scan_list[position].chn,asr_hw->scan_list[position].channel.center_freq);

                // station select pairwise_cipher
                if (data.pairwise_cipher & WPA_CIPHER_CCMP){
                    data.pairwise_cipher = WPA_CIPHER_CCMP;
                } else if (data.pairwise_cipher & WPA_CIPHER_TKIP){
                    data.pairwise_cipher = WPA_CIPHER_TKIP;
                } else if (data.pairwise_cipher & WPA_CIPHER_WEP104) {
                    data.pairwise_cipher = WPA_CIPHER_WEP104;
                } else if (data.pairwise_cipher & WPA_CIPHER_WEP40) {
                    data.pairwise_cipher = WPA_CIPHER_WEP40;
                } else {
                    dbg(D_ERR, D_UWIFI_CTRL, "pairwise_cipher not support!!!(%x)",data.pairwise_cipher);
                }

                // station select akm suit and update mgmt_group_cipher
                #if NX_SAE
                if (data.key_mgmt & WPA_KEY_MGMT_SAE){
                    data.key_mgmt = WPA_KEY_MGMT_SAE;
                } else
                #endif
                if (data.key_mgmt & WPA_KEY_MGMT_PSK_SHA256){
                    data.key_mgmt = WPA_KEY_MGMT_PSK_SHA256;
                } else if (data.key_mgmt & WPA_KEY_MGMT_IEEE8021X_SHA256){
                    data.key_mgmt = WPA_KEY_MGMT_IEEE8021X_SHA256;
                } else if (data.key_mgmt & WPA_KEY_MGMT_PSK){
                    data.key_mgmt = WPA_KEY_MGMT_PSK;
                    //data.mgmt_group_cipher = 0;
                } else if (data.key_mgmt & WPA_KEY_MGMT_IEEE8021X) {
                    data.key_mgmt = WPA_KEY_MGMT_IEEE8021X;
                    //data.mgmt_group_cipher = 0;
                } else {
                    data.mgmt_group_cipher = 0;
                    dbg(D_ERR, D_UWIFI_CTRL, "key_mgmt not support!!!(%x)",data.key_mgmt);
                }

                // update scan_list here
                if (wpa_scan_info_update_scan_list(&asr_hw->scan_list[position], &data) < 0)
                {
                    return -1;
                }
            }
            else
            {

                 dbg(D_ERR, D_UWIFI_CTRL, "wpa parse wpa rsn ie failed, discard!!!");
                 return -1;
            }

            asr_hw->scan_list[position].crypto.control_port = true;       //ibss or separate auth/asso=1
            asr_hw->scan_list[position].crypto.control_port_ethertype = ETH_P_PAE;//0x888E 802.1X

        }
        else
        {
            elem_addr = wpa_get_ie(frm_addr, frm_len, WPA_ELEM_ID);
            if (elem_addr != NULL)
            {
                asr_hw->scan_list[position].encryp_protocol = WIFI_ENCRYP_WPA;
                if(WPA_CCMP_OUI == WPA_GET_BE32(&(elem_addr[WPA_MULTI_CIPHER_OUI_OFT])))
                {
                    asr_hw->scan_list[position].crypto.cipher_group = WLAN_CIPHER_SUITE_CCMP;
                }
                else if(WPA_TKIP_OUI == WPA_GET_BE32(&(elem_addr[WPA_MULTI_CIPHER_OUI_OFT])))
                {
                    asr_hw->scan_list[position].crypto.cipher_group = WLAN_CIPHER_SUITE_TKIP;
                }

                //wpa ciphers_pairwise
                if(WPA_CCMP_OUI == WPA_GET_BE32(&(elem_addr[WPA_UNICAST_CIPHER_OUI_OFT])))
                {
                    asr_hw->scan_list[position].crypto.ciphers_pairwise[0] = WLAN_CIPHER_SUITE_CCMP;
                }
                else if(WPA_TKIP_OUI == WPA_GET_BE32(&(elem_addr[WPA_UNICAST_CIPHER_OUI_OFT])))
                {
                    asr_hw->scan_list[position].crypto.ciphers_pairwise[0] = WLAN_CIPHER_SUITE_TKIP;
                }

                asr_hw->scan_list[position].ie_len = elem_addr[1]+2;//total IE length = typelen+2(1 byte type+1 byte len field);
                if (asr_hw->scan_list[position].ie_len > MAX_IE_LEN)
                {
                    dbg(D_ERR,D_UWIFI_CTRL,"wpa_get_scan_info::wpa ie_len[%d] is more than MAX[%d]", asr_hw->scan_list[position].ie_len, MAX_IE_LEN);
                    return -1;
                }
                memcpy(asr_hw->scan_list[position].ie,&elem_addr[0],elem_addr[1]+2);

                //unicast chiper count>1
                //unicast_chiper_count = *((uint16_t*)(&elem_addr[WPA_UNICAST_CIPHER_COUNT_OFT]));
                unicast_chiper_count = elem_addr[WPA_UNICAST_CIPHER_COUNT_OFT] | (elem_addr[WPA_UNICAST_CIPHER_COUNT_OFT+1] << 8);
                dbg(D_ERR,D_UWIFI_CTRL,"chiper_count:%d", unicast_chiper_count);
                if(unicast_chiper_count > 1)
                {
                    asr_hw->scan_list[position].ie[1] = elem_addr[1]-4*(unicast_chiper_count-1);
                    asr_hw->scan_list[position].ie_len = elem_addr[1]+2-4*(unicast_chiper_count-1);
                    asr_hw->scan_list[position].ie[WPA_UNICAST_CIPHER_COUNT_OFT] = 0x1;
                    asr_hw->scan_list[position].ie[WPA_UNICAST_CIPHER_COUNT_OFT+1] = 0x0;
                    dbg(D_ERR,D_UWIFI_CTRL,"memcpy cnt:%d",elem_addr[1]+2-(WPA_UNICAST_CIPHER_OUI_OFT+unicast_chiper_count*4));
                    memcpy(&asr_hw->scan_list[position].ie[WPA_AFTER_CIPHER_OUI_OFT],&elem_addr[WPA_UNICAST_CIPHER_OUI_OFT+unicast_chiper_count*4],
                        elem_addr[1]+2-(WPA_UNICAST_CIPHER_OUI_OFT+unicast_chiper_count*4));
                }
                asr_hw->scan_list[position].crypto.control_port = true;      //ibss or separate auth/asso=1
                asr_hw->scan_list[position].crypto.control_port_ethertype = ETH_P_PAE;//0x888E 802.1X
            }
            else
            {
                asr_hw->scan_list[position].encryp_protocol = WIFI_ENCRYP_WEP;
            }
        }
    }
#endif
    if(asr_hw->scan_list[position].encryp_protocol == WIFI_ENCRYP_OPEN)
        asr_hw->scan_list[position].crypto.n_ciphers_pairwise = 0;
    else
        asr_hw->scan_list[position].crypto.n_ciphers_pairwise = 1;

#ifdef CFG_ENCRYPT_SUPPORT
    uint8_t rsnxe_capa;

    elem_addr = uwifi_mac_ie_find(frm_addr, frm_len, RSNXE_ELEM_ID);
    if (elem_addr != NULL)
    {
        dbg(D_INF,D_UWIFI_CTRL, "%s:len=%d,rsnxe=0x%X\n", __func__, elem_addr[1], elem_addr[2]);

        asr_hw->scan_list[position].rsnxe_len = elem_addr[1] + 2;
        if (asr_hw->scan_list[position].rsnxe_len > MAX_RSNXE_LEN) {
            asr_hw->scan_list[position].rsnxe_len = MAX_RSNXE_LEN;
        }
        memcpy(asr_hw->scan_list[position].rsnxe, elem_addr, asr_hw->scan_list[position].rsnxe_len);

        //append rsnxe to wpa ie for epol tx.
        #if 0
        memcpy(asr_hw->scan_list[position].ie + asr_hw->scan_list[position].ie_len,elem_addr,elem_addr[1]+2);
        asr_hw->scan_list[position].ie_len += elem_addr[1]+2;
        #else
        uint8_t temp_rsnxe[18];
        uint8_t temp_rsnxe_len = (elem_addr[2] & 0x0F) + 1 + 2;
        memcpy(temp_rsnxe, asr_hw->scan_list[position].rsnxe, temp_rsnxe_len);
        temp_rsnxe[1] = (elem_addr[2] & 0x0F) + 1;
        memcpy(asr_hw->scan_list[position].ie + asr_hw->scan_list[position].ie_len, temp_rsnxe, temp_rsnxe_len);
        asr_hw->scan_list[position].ie_len += temp_rsnxe_len;

        rsnxe_capa = elem_addr[2];

        asr_hw->scan_list[position].sae_h2e = !!(rsnxe_capa & BIT(5));

        dbg(D_ERR, D_UWIFI_CTRL, "(%s): RSNXE support, sae_h2e=%d",asr_hw->scan_list[position].ssid.ssid,asr_hw->scan_list[position].sae_h2e);

        #endif
    } else {
        dbg(D_INF,D_UWIFI_CTRL, "%s:rsnxe is null\n", __func__);
        asr_hw->scan_list[position].rsnxe_len = 0;
        asr_hw->scan_list[position].sae_h2e = 0;
    }
#endif

    /* not defined
    ** asr_hw->scan_list[position].mfp  = ;
    ** asr_hw->scan_list[position].crypto.control_port_ethertype = ;
    ** asr_hw->scan_list[position].crypto.control_port_no_encrypt = ;// ap mode set
    */

    if ((!sameap) && (j < MAX_AP_NUM))
        asr_hw->scan_count++;

    return 0;
}

int asr_rx_scanu_result_ind(struct asr_hw *asr_hw,
                                           struct asr_cmd *cmd,
                                           struct ipc_e2a_msg *msg)
{
    struct scanu_result_ind *ind = (struct scanu_result_ind *)msg->param;

    wpa_get_scan_info(asr_hw,ind);

    return 0;
}
#endif

int asr_fw_softversion_cfm(struct asr_hw *asr_hw,
                                           struct asr_cmd *cmd,
                                           struct ipc_e2a_msg *msg)
{
    struct mm_fw_softversion_cfm *version = (struct mm_fw_softversion_cfm *)msg->param;
    dbg(D_DBG,D_UWIFI_CTRL,"%s: FW:version :%s\r\n",__func__,version->fw_softversion);

    return 0;
}

#ifdef CFG_SOFTAP_SUPPORT
static int asr_local_rx_deauth(struct asr_hw *asr_hw, struct asr_vif *asr_vif, struct asr_sta *sta, u16 reason_code)
{
    struct ieee80211_mgmt deauth_frame;

    if (asr_hw == NULL || asr_vif == NULL || sta == NULL) {
        return -1;
    }

    memset((char *)&deauth_frame, 0, sizeof(struct ieee80211_mgmt));
    deauth_frame.frame_control = 0xC0;
    deauth_frame.duration = 0;

    memcpy(deauth_frame.sa, sta->mac_addr, ETH_ALEN);

    if (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_STATION) {
        memcpy(deauth_frame.bssid, deauth_frame.sa, ETH_ALEN);
    } else if (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_AP ) {
        memcpy(deauth_frame.bssid, asr_vif->wlan_mac_add.mac_addr, ETH_ALEN);
    } else {
        //todo
        memcpy(deauth_frame.bssid, deauth_frame.sa, ETH_ALEN);
    }
    memcpy(deauth_frame.da, asr_vif->wlan_mac_add.mac_addr, ETH_ALEN);
    deauth_frame.u.deauth.reason_code = reason_code;

    dbg(D_ERR,D_UWIFI_CTRL, "%s: sta=%d mac=%02X:%02X:%02X:%02X:%02X:%02X deauth reason=%d \n",
        __func__, sta->sta_idx, sta->mac_addr[0],sta->mac_addr[1],sta->mac_addr[2],sta->mac_addr[3]
        ,sta->mac_addr[4],sta->mac_addr[5],deauth_frame.u.deauth.reason_code);


    if (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_AP) {

        cfg80211_rx_mgmt(asr_vif, (u8 *)&deauth_frame, sizeof(struct ieee80211_mgmt));

    } else if (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_STATION) {

    }

    return 0;
}

static int asr_sta_link_loss_ind(struct asr_hw *asr_hw, struct asr_cmd *cmd, struct ipc_e2a_msg *msg)
{
    struct mm_sta_link_loss_ind *ind = (struct mm_sta_link_loss_ind *)msg->param;
    struct asr_vif *asr_vif = NULL;


    dbg(D_ERR,D_UWIFI_CTRL, "%s: %d,%d.\n", __func__, ind->vif_idx, ind->staid);

    if (ind->vif_idx >= NX_VIRT_DEV_MAX || ind->staid >= NX_REMOTE_STA_MAX) {
        return 0;
    }

    asr_vif = asr_hw->vif_table[ind->vif_idx];
    if (!asr_vif) {
        return 0;
    }

    if (asr_vif->up && asr_hw->sta_table[ind->staid].valid) {

        asr_local_rx_deauth(asr_hw, asr_vif, &asr_hw->sta_table[ind->staid], 1);
    }

    return 0;
}
#endif

#if NX_SAE
extern uint8_t   g_sae_pmk[SAE_PMK_LEN];
extern uint8_t   g_pmkid[SAE_PMKID_LEN];
#endif

#ifdef CFG_STATION_SUPPORT
int asr_rx_sm_connect_ind(struct asr_hw *asr_hw,
                                         struct asr_cmd *cmd,
                                         struct ipc_e2a_msg *msg)
{
    struct sm_connect_ind *ind = (struct sm_connect_ind *)msg->param;
    struct asr_vif *asr_vif = asr_hw->vif_table[ind->vif_idx];
    struct config_info *pst_config_info = NULL;
    //const uint8_t *req_ie;

    /* Retrieve IE addresses and lengths */
    //req_ie = (const uint8_t *)ind->assoc_ie_buf;
    asr_clear_bit(ASR_DEV_STA_CONNECTING, &asr_vif->dev_flags);

    dbg(D_ERR,D_UWIFI_CTRL,"%s: status_code=%d,vif_idx=%d,roamed=%d,ap_idx=%d,wifi_ready=%d\r\n",
        __func__,ind->status_code, ind->vif_idx, ind->roamed, ind->ap_idx,wifi_ready);

    // Fill-in the AP information
    if (ind->status_code == 0)
    {
        struct asr_sta *sta = &asr_hw->sta_table[ind->ap_idx];
        uint8_t txq_status;
        sta->valid = true;
        sta->sta_idx = ind->ap_idx;
        sta->ch_idx = ind->ch_idx;
        sta->vif_idx = ind->vif_idx;
        sta->vlan_idx = sta->vif_idx;
        sta->qos = ind->qos;
        sta->acm = ind->acm;
        sta->ps.active = false;
        sta->aid = ind->aid;

#ifdef CONFIG_ASR595X
        sta->band = ind->chan.band;
        //sta->width = ind->chan.width;
        sta->center_freq = ind->chan.prim20_freq;
        sta->center_freq1 = ind->chan.center1_freq;
        sta->center_freq2 = ind->chan.center2_freq;
#else
        sta->band = ind->band;
        sta->width = ind->width;
        sta->center_freq = ind->center_freq;
        sta->center_freq1 = ind->center_freq1;
        sta->center_freq2 = ind->center_freq2;
#endif
        asr_vif->sta.ap = sta;
        // TODO: Get chan def in this case (add params in cfm ??)
        asr_chanctx_link(asr_vif, ind->ch_idx, NULL);
        memcpy(sta->mac_addr, ind->bssid.array, ETH_ALEN);
        if (ind->ch_idx == asr_hw->cur_chanctx) {
            txq_status = 0;
        } else {
            txq_status = ASR_TXQ_STOP_CHAN;
        }
        memcpy(sta->ac_param, ind->ac_param, sizeof(sta->ac_param));
        asr_txq_sta_init(asr_hw, sta, txq_status);
    }

    if (!ind->roamed)
    {

        pst_config_info = uwifi_get_sta_config_info();
        dbg(D_ERR,D_UWIFI_CTRL,"%s: asr_hw->ap_vif_idx=%d security=%d wifiState=%d,en_autoconnect=%d,drv_flag=0x%x\r\n",__func__
                                             ,asr_hw->ap_vif_idx,pst_config_info->connect_ap_info.security,
                         asr_vif->sta.connInfo.wifiState,pst_config_info->en_autoconnect,(unsigned int)asr_vif->dev_flags);

        if(ind->status_code == 0)
        {
            asr_set_bit(ASR_DEV_STA_CONNECTED, &asr_vif->dev_flags);
            asr_vif->sta.connInfo.wifiState = WIFI_ASSOCIATED;
            /*open and wep encrypt, wpa state change to completed
              for wpa/wpa2 encrypt, should finish four eapol handshake

              like Android or like RK, dhcp should start by AT/APP, call netifapi_dhcp_start and set timeout
              so uwifi should notify AT/APP using EVENT_STATION_UP when supplicant state change to completed
            */

            if(NULL != pst_config_info)
            {
                if((pst_config_info->connect_ap_info.security==WIFI_ENCRYP_OPEN) || (pst_config_info->connect_ap_info.security==WIFI_ENCRYP_WEP))
                {
                    asr_vif->sta.connInfo.wifiState = WIFI_CONNECTED;
                    wifi_event_cb(WLAN_EVENT_CONNECTED, NULL);
                    lwip_comm_dhcp_event_handler(DHCP_CLIENT_UP);
                }
#ifdef CFG_ENCRYPT_SUPPORT
                else if((pst_config_info->connect_ap_info.security == WIFI_ENCRYP_WPA) || (pst_config_info->connect_ap_info.security == WIFI_ENCRYP_WPA2))
                {
                    enum wlan_op_mode mode = WLAN_OPMODE_INFRA;

                    if(asr_vif->iftype == NL80211_IFTYPE_STATION)
                    {
                        mode = WLAN_OPMODE_INFRA;

                        dbg(D_ERR,D_UWIFI_CTRL,"[%s] save peer's PMK,akm suit=(%u) ",__func__,(unsigned int)pst_config_info->akm_suite);
                        struct sta_info *pstat = &g_connect_info;

                        #if NX_SAE
                         //set sae pmk from ind here
                        if (pst_config_info->akm_suite == RSN_AUTH_KEY_MGMT_SAE)
                        {
                            pstat->is_sae_sta = 1;

                            #if NX_SAE_PMK_CACHE
                            if(pmkid_cached(pstat)){
                                dbg(D_ERR,D_UWIFI_CTRL,"[%s] use cached pmk, not update cache",__func__);
                            }else
                            #endif
                            {
                                memcpy(pstat->sae_pmk, g_sae_pmk, PMK_LEN);
                                wpa_hexdump(MSG_DEBUG, "save pmk from ind ",pstat->sae_pmk, SAE_PMK_LEN);
                                #ifdef LIB_DEBUG
                                printf("SAE PMK:\r\n");
                                for(int kk=0;kk<SAE_PMK_LEN;kk++)
                                    printf("%02X", pstat->sae_pmk[kk]);
                                #endif

                                memcpy(pstat->sae_pmkid, g_pmkid, SAE_PMKID_LEN);
                                wpa_hexdump(MSG_DEBUG, "save pmkid from ind",pstat->sae_pmkid, SAE_PMKID_LEN);

                                #if NX_SAE_PMK_CACHE
                                //struct wpa_priv *p_wpa_priv =  g_pwpa_priv[COEX_MODE_STA];
                                pstat->pmkid_caching_idx = add_pmkid_cache(&user_pmkid_caching,g_pmkid,g_sae_pmk,(unsigned char *)ind->bssid.array);
                                #endif
                            }

                            // sae need record h2e.
                            struct scan_result *pst_dst_scan_info = NULL;
                            if (pst_config_info->connect_ap_info.ssid_len)
                                pst_dst_scan_info = asr_wlan_get_scan_ap(pst_config_info->connect_ap_info.ssid, pst_config_info->connect_ap_info.ssid_len, pst_config_info->connect_ap_info.security);
                            if (NULL == pst_dst_scan_info)
                            {
                                if(!is_zero_mac_addr((const uint8_t *)pst_config_info->connect_ap_info.bssid))
                                    pst_dst_scan_info = asr_wlan_get_scan_ap_by_bssid(pst_config_info->connect_ap_info.bssid, pst_config_info->connect_ap_info.security);
                                if (NULL == pst_dst_scan_info)
                                {
                                    dbg(D_ERR, D_UWIFI_CTRL, "search connected ap fail");
                                }
                            }

                            if (pst_dst_scan_info){
                                pstat->ap_rsnxe_len = pst_dst_scan_info->rsnxe_len;
                                if (pstat->ap_rsnxe_len)
                                    pstat->ap_rsnxe = pst_dst_scan_info->rsnxe;
                                else
                                    pstat->ap_rsnxe = NULL;

                                dbg(D_ERR, D_UWIFI_CTRL, "record sae h2e,len=%d,rsnxe=0x%X",pstat->ap_rsnxe_len,pstat->ap_rsnxe);
                            }

                            // free sae data.
                            wpa_free_sae();
                        }else
                        #endif
                        {
                            pstat->is_sae_sta = 0;
                        }
                    }
                    else if(asr_vif->iftype == NL80211_IFTYPE_AP)
                    {
                        mode = WLAN_OPMODE_MASTER;
                    }
                    else
                    {
                         dbg(D_ERR, D_UWIFI_CTRL, "mode err");
                    }

                    uwifi_msg_rxu2u_handshake_start(mode);
                }
#endif

                wifi_event_cb(WLAN_EVENT_ASSOCIATED, NULL);
            }

            //when softap start first and chan is different from sta's connected channel, close softaap. not in ap channel, need refine later.
            if((asr_hw->ap_vif_idx != 0xff)
                &&(asr_hw->vif_table[asr_hw->sta_vif_idx]->sta.ap)
                &&(asr_hw->chanctx_table[asr_hw->vif_table[asr_hw->ap_vif_idx]->ch_index].chan_def.chan)
                &&(asr_hw->vif_table[asr_hw->sta_vif_idx]->sta.ap->center_freq !=
                   asr_hw->chanctx_table[asr_hw->vif_table[asr_hw->ap_vif_idx]->ch_index].chan_def.chan->center_freq))
            {
                struct uwifi_ap_cfg_param *ap_cfg_param;

                ap_cfg_param = &g_ap_cfg_info;

                ap_cfg_param->re_enable = 1;

                uwifi_close_wifi(NL80211_IFTYPE_AP);

                current_iftype = STA_MODE;

            }
        }
        else
        {
            asr_clear_bit(ASR_DEV_STA_CONNECTED, &asr_vif->dev_flags);
            dbg(D_ERR, D_UWIFI_CTRL, "connect fail:status code:%d,drv_flag=0x%x", ind->status_code,(unsigned int)asr_vif->dev_flags);

            // free sae data.
            #if NX_SAE
            if (pst_config_info && pst_config_info->akm_suite == RSN_AUTH_KEY_MGMT_SAE) {
                wpa_free_sae();
            }

            #if NX_SAE_PMK_CACHE
            if ((ind->status_code == WLAN_STATUS_INVALID_PMKID))
            {
               /*invalid pmkid or assoc fail, dut has cache, peer has no cache, del dut cache idx */
               struct sta_info *pstat = &g_connect_info;
               if(pmkid_cached(pstat)) {
                   pmkid_cache_del(&user_pmkid_caching, pstat->pmkid_caching_idx);
                   pstat->pmkid_caching_idx = NUM_PMKID_CACHE;
               }
            }
            #endif
            #endif /* NX_SAE */

            asr_vif->sta.connInfo.wifiState = WIFI_CONNECT_FAILED;
            /* decide if we need an auto reconnect */
            uwifi_schedule_reconnect(&(asr_vif->sta.configInfo));

            if (AUTOCONNECT_DISABLED == asr_vif->sta.configInfo.en_autoconnect) {
               dbg(D_ERR, D_UWIFI_CTRL,"%s: disable autoconnect, set retry max err here!\n",__func__);
               asr_wlan_set_disconnect_status(WLAN_STA_MODE_CONN_RETRY_MAX);
            }
        }
    }

    return 0;
}

extern uint8_t is_scan_adv;
extern bool asr_xmit_opt;
int asr_rx_sm_disconnect_ind(struct asr_hw *asr_hw,
                                            struct asr_cmd *cmd,
                                            struct ipc_e2a_msg *msg)
{

    struct sm_disconnect_ind *ind = (struct sm_disconnect_ind *)msg->param;
    struct asr_vif *asr_vif = asr_hw->vif_table[ind->vif_idx];
    struct config_info *config_info = &(asr_vif->sta.configInfo);
    struct asr_vif *asr_vif_tmp = NULL;

    asr_txq_sta_deinit(asr_hw, asr_vif->sta.ap);
    asr_vif->sta.ap->valid = false;
    asr_vif->sta.ap = NULL;
    asr_chanctx_unlink(asr_vif);
    asr_clear_bit(ASR_DEV_STA_DISCONNECTING, &asr_vif->dev_flags);
    asr_clear_bit(ASR_DEV_STA_CONNECTING, &asr_vif->dev_flags);
    asr_clear_bit(ASR_DEV_STA_AUTH, &asr_vif->dev_flags);
    asr_clear_bit(ASR_DEV_STA_DHCPEND, &asr_vif->dev_flags);
    dbg(D_ERR, D_UWIFI_SUPP, "%s time=@0x%x,dev_flags=0x%X", __func__, asr_rtos_get_time(),(unsigned int)asr_vif->dev_flags);

    /* if vif is not up, asr_close has already been called */
    if (asr_vif->up) {
        if (!ind->ft_over_ds) {
            asr_clear_bit(ASR_DEV_STA_CONNECTED, &asr_vif->dev_flags);
            dbg(D_ERR, D_UWIFI_SUPP, "%s is_connected=%d wifi_ready=%d en_autoconnect=%d ,disconnect reason_code = %d \r\n", __func__,
                                        config_info->is_connected,wifi_ready,config_info->en_autoconnect,ind->reason_code);

            config_info->is_connected = 0;

            if(WIFI_NOT_READY != wifi_ready)
            {
                set_wifi_ready_status(WIFI_NOT_READY);
            }


            /* In recieve deauth or internel reason disconnect, delete key here */
            if ((AUTOCONNECT_DISABLED != config_info->en_autoconnect)
                &&(config_info->connect_ap_info.security != WIFI_ENCRYP_OPEN))
                uwifi_msg_rxu2u_wpa_info_clear((void*)asr_vif, (uint32_t)config_info->connect_ap_info.security);

            lwip_comm_dhcp_event_handler(DHCP_CLIENT_DOWN);
            wifi_event_cb(WLAN_EVENT_DISCONNECTED, NULL);

            memset(&asr_vif->sta.connInfo,0,sizeof(asr_vif->sta.connInfo));//clear connInfo
            asr_vif->sta.connInfo.wifiState = WIFI_DISCONNECTED;

            #if NX_SAE_PMK_CACHE
            if((ind->reason_code != MAC_PRIV_RS_BEACON_LOSS))
            {
               /*temp add beacon loss bypass case for not del pmk cache when disconnect, add more in the future   */
               /*peer send deauth frame and del pmk cache, dut should also del pmk cache idx */
               struct sta_info *pstat = &g_connect_info;
               if(pmkid_cached(pstat) && (pstat->is_sae_sta)) {
                   dbg(D_ERR,D_UWIFI_CTRL,"disconnect process,del pmk idx (%d)",pstat->pmkid_caching_idx);
                   pmkid_cache_del(&user_pmkid_caching, pstat->pmkid_caching_idx);
                   pstat->pmkid_caching_idx = NUM_PMKID_CACHE;
               }
            }
            #endif

            dbg(D_ERR, D_UWIFI_SUPP, "%s is_connected=%d wifi_ready=%d en_autoconnect=%d interval=%d\r\n", __func__,
                                         config_info->is_connected,wifi_ready,config_info->en_autoconnect,config_info->wifi_retry_interval);

            if(ind->reason_code == MAC_PRIV_RS_BEACON_LOSS)
            {
                asr_wlan_set_disconnect_status(WLAN_STA_MODE_BEACON_LOSS);
            }
            else if((ind->reason_code == WLAN_REASON_PREV_AUTH_NOT_VALID)
                    ||(ind->reason_code == WLAN_REASON_MIC_FAILURE)
                    ||(ind->reason_code == WLAN_REASON_4WAY_HANDSHAKE_TIMEOUT))
            {

                asr_wlan_set_disconnect_status(WLAN_STA_MODE_PASSWORD_ERR);
            }

            //clear the tx buffer, the buffered data will be dropped. there will no send data in tx direction

            if (asr_xmit_opt) {

                asr_rtos_lock_mutex(&asr_hw->tx_lock);
                asr_drop_tx_vif_skb(asr_hw,asr_vif);
                asr_vif->tx_skb_cnt = 0;
                asr_rtos_unlock_mutex(&asr_hw->tx_lock);

            } else {

                asr_rtos_lock_mutex(&asr_hw->tx_agg_env_lock);

                if (asr_hw->vif_started > 1) {
                    asr_tx_agg_buf_mask_vif(asr_hw,asr_vif);
                } else {
                    asr_tx_agg_buf_reset(asr_hw);
                    list_for_each_entry(asr_vif_tmp,&asr_hw->vifs,list){
                        asr_vif_tmp->txring_bytes = 0;
                    }
                }

                asr_rtos_unlock_mutex(&asr_hw->tx_agg_env_lock);
            }

            //other code for del key
        }
    }

    /* decide if we need an auto reconnect */
    is_scan_adv = RECONNECT_SCAN;
    uwifi_schedule_reconnect(config_info);

    return 0;
}
#endif

/***************************************************************************
 * Messages from MM task
 **************************************************************************/
static  int asr_rx_chan_pre_switch_ind(struct asr_hw *asr_hw,
                                              struct asr_cmd *cmd,
                                              struct ipc_e2a_msg *msg)
{
    struct asr_vif *asr_vif;
    struct mm_channel_pre_switch_ind *mm_channel_ind = (struct mm_channel_pre_switch_ind *)(msg->param);
    int chan_idx = mm_channel_ind->chan_index;

    list_for_each_entry(asr_vif, &asr_hw->vifs, list) {
        if (asr_vif->up && asr_vif->ch_index == chan_idx) {
            asr_txq_vif_stop(asr_vif, ASR_TXQ_STOP_CHAN, asr_hw);
        }
    }


    return 0;
}

static  int asr_rx_chan_switch_ind(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    struct asr_vif *asr_vif;
    struct mm_channel_switch_ind *mm_channel_ind = (struct mm_channel_switch_ind *)(msg->param);
    int chan_idx = mm_channel_ind->chan_index;
    bool roc     = mm_channel_ind->roc;
    bool roc_tdls = mm_channel_ind->roc_tdls;

    if (roc_tdls) {
        uint8_t vif_index = mm_channel_ind->vif_index;
        list_for_each_entry(asr_vif, &asr_hw->vifs, list) {
            if (asr_vif->vif_index == vif_index) {
            }
        }
    } else if (!roc) {
        list_for_each_entry(asr_vif, &asr_hw->vifs, list) {
            if (asr_vif->up && asr_vif->ch_index == chan_idx) {
                asr_txq_vif_start(asr_vif, ASR_TXQ_STOP_CHAN, asr_hw);
            }
        }
    } else {
        /* Retrieve the allocated RoC element */
        struct asr_roc_elem *roc_elem = asr_hw->roc_elem;

        /* For debug purpose (use ftrace kernel option) */

        /* If mgmt_roc is true, remain on channel has been started by ourself */
        if (!roc_elem->mgmt_roc) {
            /* Inform the host that we have switch on the indicated off-channel */
            //cfg80211_ready_on_channel(roc_elem->wdev, (uint64_t)(asr_hw->roc_cookie_cnt),
            //                          roc_elem->chan, roc_elem->duration, GFP_KERNEL);
        }

        /* Keep in mind that we have switched on the channel */
        roc_elem->on_chan = true;

        // Enable traffic on OFF channel queue
        asr_txq_offchan_start(asr_hw);
    }

    asr_hw->cur_chanctx = chan_idx;
    return 0;
}

static  int asr_rx_remain_on_channel_exp_ind(struct asr_hw *asr_hw,
                                                    struct asr_cmd *cmd,
                                                    struct ipc_e2a_msg *msg)
{
    /* Retrieve the allocated RoC element */
    struct asr_roc_elem *roc_elem = asr_hw->roc_elem;
    /* Get VIF on which RoC has been started */
    struct asr_vif *asr_vif = roc_elem->asr_vif;

    /* For debug purpose (use ftrace kernel option) */

    /* If mgmt_roc is true, remain on channel has been started by ourself */
    /* If RoC has been cancelled before we switched on channel, do not call cfg80211 */
    if (!roc_elem->mgmt_roc && roc_elem->on_chan) {
        /* Inform the host that off-channel period has expired */
        //cfg80211_remain_on_channel_expired(roc_elem->wdev, (uint64_t)(asr_hw->roc_cookie_cnt),
        //                                   roc_elem->chan, GFP_KERNEL);
    }

    /* De-init offchannel TX queue */
    asr_txq_offchan_deinit(asr_vif);

    /* Increase the cookie counter cannot be zero */
    asr_hw->roc_cookie_cnt++;

    if (asr_hw->roc_cookie_cnt == 0) {
        asr_hw->roc_cookie_cnt = 1;
    }

    /* Free the allocated RoC element */
    asr_rtos_free(roc_elem);
    roc_elem = NULL;
    asr_hw->roc_elem = NULL;

    return 0;
}

#if 0
static  int asr_rx_p2p_vif_ps_change_ind(struct asr_hw *asr_hw,
                                                struct asr_cmd *cmd,
                                                struct ipc_e2a_msg *msg)
{
    struct mm_p2p_vif_ps_change_ind *mm_p2p_change_ind = (struct mm_p2p_vif_ps_change_ind *)msg->param;
    int vif_idx  = mm_p2p_change_ind->vif_index;
    int ps_state = mm_p2p_change_ind->ps_state;
    struct asr_vif *vif_entry;



    vif_entry = asr_hw->vif_table[vif_idx];

    if (vif_entry) {
        goto found_vif;
    }

    goto exit;

found_vif:


    if (ps_state == MM_PS_MODE_OFF) {
        // Start TX queues for provided VIF
        asr_txq_vif_start(vif_entry, ASR_TXQ_STOP_VIF_PS, asr_hw);
    }
    else {
        // Stop TX queues for provided VIF
        asr_txq_vif_stop(vif_entry, ASR_TXQ_STOP_VIF_PS, asr_hw);
    }

exit:
    return 0;
}


static  int asr_rx_p2p_noa_upd_ind(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    return 0;
}
#endif

static inline int asr_rx_hif_sdio_info_ind(struct asr_hw *asr_hw, struct asr_cmd *cmd, struct ipc_e2a_msg *msg)
{
    struct mm_hif_sdio_info_ind *ind = (struct mm_hif_sdio_info_ind *)msg->param;


    asr_hw->hif_sdio_info_ind = *ind;

    dbg(D_ERR,D_UWIFI_CTRL, "%s: %d %d %d.\n", __func__, ind->flag, ind->host_tx_data_cur_idx, ind->rx_data_cur_idx);

    return 0;
}

static inline int asr_rx_p2p_noa_upd_ind(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    return 0;
}

static  int asr_rx_rssi_status_ind(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    struct mm_rssi_status_ind *ind = (struct mm_rssi_status_ind *)msg->param;
    int vif_idx  = ind->vif_index;

    struct asr_vif *vif_entry;




    vif_entry = asr_hw->vif_table[vif_idx];
    if (vif_entry) {
        wifi_event_cb(WLAN_EVENT_RSSI, ind);
        /*cfg80211_cqm_rssi_notify(vif_entry->ndev,
                                 rssi_status ? NL80211_CQM_RSSI_THRESHOLD_EVENT_LOW :
                                               NL80211_CQM_RSSI_THRESHOLD_EVENT_HIGH,
                                 GFP_KERNEL);*/
    }

    return 0;
}
static  int asr_rx_rssi_level_ind(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    struct mm_rssi_level_ind *ind = (struct mm_rssi_level_ind *)msg->param;
    int vif_idx  = ind->vif_index;

    struct asr_vif *vif_entry;


    vif_entry = asr_hw->vif_table[vif_idx];
    if (vif_entry) {
        //dbg(D_ERR, D_UWIFI_CTRL, "%s vif_idx=%d rssi_level=%d\r\n",__func__,vif_idx,ind->rssi_level);
        wifi_event_cb(WLAN_EVENT_RSSI_LEVEL, &(ind->rssi_level));
        /*cfg80211_cqm_rssi_notify(vif_entry->ndev,
                                 rssi_status ? NL80211_CQM_RSSI_THRESHOLD_EVENT_LOW :
                                               NL80211_CQM_RSSI_THRESHOLD_EVENT_HIGH,
                                 GFP_KERNEL);*/
    }

    return 0;
}

static inline int asr_rx_pktloss_notify_ind(struct asr_hw *asr_hw,
                                             struct asr_cmd *cmd,
                                             struct ipc_e2a_msg *msg)
{
    struct mm_pktloss_ind *ind = (struct mm_pktloss_ind *)msg->param;
    struct asr_vif *vif_entry;
    int vif_idx  = ind->vif_index;

    dbg(D_ERR,D_UWIFI_CTRL,"asr_rx_pktloss_notify_ind , vif_idx=%d",vif_idx);

    vif_entry = asr_hw->vif_table[vif_idx];
    if (vif_entry) {
        //cfg80211_cqm_pktloss_notify(vif_entry->ndev, (const u8 *)ind->mac_addr.array,
        //                            ind->num_packets, GFP_ATOMIC);
    }

    return 0;
}

bool g_disable_staps_block_tx = false;
static inline int asr_rx_ps_change_ind(struct asr_hw *asr_hw,
                                        struct asr_cmd *cmd,
                                        struct ipc_e2a_msg *msg)
{
    struct mm_ps_change_ind *ind = (struct mm_ps_change_ind *)msg->param;
    struct asr_sta *sta = &asr_hw->sta_table[ind->sta_idx];

    if (ind->sta_idx >= (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX)) {
        dbg(D_ERR,D_UWIFI_CTRL,"Invalid sta index reported by fw %d\n",
                  ind->sta_idx);
        return 1;
    }

    //dbg(D_INF,D_UWIFI_CTRL, "Sta %d, change PS mode to %s", sta->sta_idx, ind->ps_state ? "ON" : "OFF");

    if (g_disable_staps_block_tx || (asr_xmit_opt == false))
        return 0;

    if (sta->valid) {
        asr_ps_bh_enable(asr_hw, sta, ind->ps_state);
    } else if (asr_hw->adding_sta) {
        sta->ps.active = ind->ps_state ? true : false;
    } else {
        dbg(D_ERR,D_UWIFI_CTRL, "Ignore PS mode change on invalid sta %d\n",ind->sta_idx);
    }

    return 0;
}

static inline int asr_rx_traffic_req_ind(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    struct mm_traffic_req_ind *ind = (struct mm_traffic_req_ind *)msg->param;
    struct asr_sta *sta = &asr_hw->sta_table[ind->sta_idx];

    dbg(D_ERR,D_UWIFI_CTRL, "Sta %d, asked for %d pkt, uapsd=%d", sta->sta_idx, ind->pkt_cnt,ind->uapsd);

    asr_ps_bh_traffic_req(asr_hw, sta, ind->pkt_cnt,
                           ind->uapsd ? UAPSD_ID : LEGACY_PS_ID);

    return 0;
}

static  int asr_rx_csa_counter_ind(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    struct mm_csa_counter_ind *ind = (struct mm_csa_counter_ind *)msg->param;
    struct asr_vif *vif;
    bool found = false;



    // Look for VIF entry
    list_for_each_entry(vif, &asr_hw->vifs, list) {
        if (vif->vif_index == ind->vif_index) {
            found=true;
            break;
        }
    }

    if (found) {
        if (vif->ap.csa)
            vif->ap.csa->count = ind->csa_count;
        else
            dbg(D_ERR,D_UWIFI_CTRL,"CSA counter update but no active CSA");
    }

    return 0;
}


static  int asr_rx_csa_finish_ind(struct asr_hw *asr_hw,
                                         struct asr_cmd *cmd,
                                         struct ipc_e2a_msg *msg)
{
    struct mm_csa_finish_ind *ind = (struct mm_csa_finish_ind *)msg->param;
    struct asr_vif *vif;
    bool found = false;


    // Look for VIF entry
    list_for_each_entry(vif, &asr_hw->vifs, list) {
        if (vif->vif_index == ind->vif_index) {
            found=true;
            break;
        }
    }

    if (found) {
        if (ASR_VIF_TYPE(vif) == NL80211_IFTYPE_AP) {
            if (vif->ap.csa) {
                vif->ap.csa->status = ind->status;
                vif->ap.csa->ch_idx = ind->chan_idx;
                //schedule_work(&vif->ap.csa->work);
                asr_csa_finish(vif->ap.csa);
            } else
                dbg(D_ERR,D_UWIFI_CTRL,"CSA finish indication but no active CSA");
        } else {
            if (ind->status == 0) {
                asr_chanctx_unlink(vif);
                asr_chanctx_link(vif, ind->chan_idx, NULL);
                if (asr_hw->cur_chanctx == ind->chan_idx) {
                    //asr_radar_detection_enable_on_cur_channel(asr_hw);
                    asr_txq_vif_start(vif, ASR_TXQ_STOP_CHAN, asr_hw);
                } else
                    asr_txq_vif_stop(vif, ASR_TXQ_STOP_CHAN, asr_hw);
            }
        }
    }

    return 0;
}

static  int asr_rx_csa_traffic_ind(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    struct mm_csa_traffic_ind *ind = (struct mm_csa_traffic_ind *)msg->param;
    struct asr_vif *vif;
    bool found = false;



    // Look for VIF entry
    list_for_each_entry(vif, &asr_hw->vifs, list) {
        if (vif->vif_index == ind->vif_index) {
            found=true;
            break;
        }
    }

    if (found) {
        if (ind->enable)
            asr_txq_vif_start(vif, ASR_TXQ_STOP_CSA, asr_hw);
        else
            asr_txq_vif_stop(vif, ASR_TXQ_STOP_CSA, asr_hw);
    }

    return 0;
}


#ifdef CONFIG_TWT
static int asr_rx_twt_traffic_ind(struct asr_hw *asr_hw, struct asr_cmd *cmd, struct ipc_e2a_msg *msg)
{
    struct mm_twt_traffic_ind *ind = (struct mm_twt_traffic_ind *)msg->param;
    //struct asr_sta *sta = &asr_hw->sta_table[ind->sta_idx];

    struct asr_vif *vif;
    bool found = false;

    //pr_info("sta-%d:vif-%d rx_twt_traffic_ind (%d)\n", ind->sta_idx, ind->vif_index, ind->enable);

    // Look for VIF entry
    list_for_each_entry(vif, &asr_hw->vifs, list) {
        if (vif->vif_index == ind->vif_index) {
            found = true;
            break;
        }
    }

    if (found) {
        if (ind->enable)
            asr_txq_vif_start(vif, ASR_TXQ_STOP_TWT, asr_hw);
        else
            asr_txq_vif_stop(vif, ASR_TXQ_STOP_TWT, asr_hw);
    }

    return 0;

}
#endif



/***************************************************************************
 * Messages from ME task
 **************************************************************************/
static  int asr_rx_me_tkip_mic_failure_ind(struct asr_hw *asr_hw,
                                                  struct asr_cmd *cmd,
                                                  struct ipc_e2a_msg *msg)
{
    //struct me_tkip_mic_failure_ind *ind = (struct me_tkip_mic_failure_ind *)msg->param;



    /*cfg80211_michael_mic_failure(dev, (uint8_t *)&ind->addr, (ind->ga?NL80211_KEYTYPE_GROUP:
                                 NL80211_KEYTYPE_PAIRWISE), ind->keyid,
                                 (uint8_t *)&ind->tsc, GFP_ATOMIC);*/

    return 0;
}

static  int asr_rx_me_tx_credits_update_ind(struct asr_hw *asr_hw,
                                                   struct asr_cmd *cmd,
                                                   struct ipc_e2a_msg *msg)
{
    struct me_tx_credits_update_ind *ind = (struct me_tx_credits_update_ind *)msg->param;



    asr_txq_credit_update(asr_hw, ind->sta_idx, ind->tid, ind->credits);

    return 0;
}

#if CFG_DBG
static int asr_dbg_mem_read_cfm(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    //struct dbg_mem_read_cfm *cfm = (struct dbg_mem_read_cfm *)msg->param;


    return 0;
}

static int asr_dbg_mem_write_cfm(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    //struct dbg_mem_write_cfm *cfm = (struct dbg_mem_write_cfm *)msg->param;

    return 0;
}

static int asr_dbg_mod_filter_cfm(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{

    return 0;

}

static int asr_dbg_sev_filter_cfm(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{

    return 0;
}

static int asr_dbg_sys_stat_cfm(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    //struct dbg_get_sys_stat_cfm *cfm = (struct dbg_get_sys_stat_cfm *)msg->param;

    return 0;
}

static int asr_dbg_set_host_log_cfm(struct asr_hw *asr_hw,
                                          struct asr_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    struct dbg_set_host_log_cfm *cfm = (struct dbg_set_host_log_cfm *)msg->param;

    return cfm->status;
}
#endif

/***************************************************************************
 * Messages from APM task
 **************************************************************************/
static const msg_cb_fct mm_hdlrs[MSG_I(MM_MAX)] = {
    [MSG_I(MM_CHANNEL_SWITCH_IND)]     = asr_rx_chan_switch_ind,
    [MSG_I(MM_CHANNEL_PRE_SWITCH_IND)] = asr_rx_chan_pre_switch_ind,
    [MSG_I(MM_REMAIN_ON_CHANNEL_EXP_IND)] = asr_rx_remain_on_channel_exp_ind,
    [MSG_I(MM_PS_CHANGE_IND)]           = asr_rx_ps_change_ind,
    [MSG_I(MM_TRAFFIC_REQ_IND)]        = asr_rx_traffic_req_ind,
    [MSG_I(MM_P2P_VIF_PS_CHANGE_IND)]  = asr_rx_p2p_vif_ps_change_ind,
    [MSG_I(MM_CSA_COUNTER_IND)]        = asr_rx_csa_counter_ind,
    [MSG_I(MM_CSA_FINISH_IND)]         = asr_rx_csa_finish_ind,
    [MSG_I(MM_CSA_TRAFFIC_IND)]        = asr_rx_csa_traffic_ind,
    [MSG_I(MM_CHANNEL_SURVEY_IND)]     = asr_rx_channel_survey_ind,
    [MSG_I(MM_P2P_NOA_UPD_IND)]        = asr_rx_p2p_noa_upd_ind,
    [MSG_I(MM_RSSI_STATUS_IND)]        = asr_rx_rssi_status_ind,
#ifdef CONFIG_ASR595X
    [MSG_I(MM_PKTLOSS_IND)]            = asr_rx_pktloss_notify_ind,
#endif
    [MSG_I(MM_RSSI_LEVEL_IND)]         = asr_rx_rssi_level_ind,
    [MSG_I(MM_HIF_SDIO_INFO_IND)]      = asr_rx_hif_sdio_info_ind,
    [MSG_I(MM_FW_SOFTVERSION_CFM)]     = asr_fw_softversion_cfm,
#ifdef CONFIG_TWT
    [MSG_I(MM_TWT_TRAFFIC_IND)]        = asr_rx_twt_traffic_ind,
#endif
    #ifdef CFG_SOFTAP_SUPPORT
    [MSG_I(MM_STA_LINK_LOSS_IND)]     = asr_sta_link_loss_ind,
    #endif
};

#ifdef CFG_STATION_SUPPORT
static const msg_cb_fct scan_hdlrs[MSG_I(SCANU_MAX)] = {
    [MSG_I(SCANU_START_CFM)]           = asr_rx_scanu_start_cfm,
    [MSG_I(SCANU_RESULT_IND)]          = asr_rx_scanu_result_ind,
};
#endif

static const msg_cb_fct me_hdlrs[MSG_I(ME_MAX)] = {
    [MSG_I(ME_TKIP_MIC_FAILURE_IND)] = asr_rx_me_tkip_mic_failure_ind,
    [MSG_I(ME_TX_CREDITS_UPDATE_IND)] = asr_rx_me_tx_credits_update_ind,
};

#ifdef CFG_STATION_SUPPORT
static const msg_cb_fct sm_hdlrs[MSG_I(SM_MAX)] = {
    [MSG_I(SM_CONNECT_IND)]    = asr_rx_sm_connect_ind,
    [MSG_I(SM_DISCONNECT_IND)] = asr_rx_sm_disconnect_ind,
};
#endif

static const msg_cb_fct apm_hdlrs[MSG_I(APM_MAX)] = {};
#if CFG_DBG
static const msg_cb_fct dbg_hdlrs[MSG_I(DBG_MAX)] = {
    [MSG_I(DBG_MEM_READ_CFM)]   = asr_dbg_mem_read_cfm,
    [MSG_I(DBG_MEM_WRITE_CFM)]   = asr_dbg_mem_write_cfm,
    [MSG_I(DBG_SET_MOD_FILTER_CFM)]   = asr_dbg_mod_filter_cfm,
    [MSG_I(DBG_SET_SEV_FILTER_CFM)]   = asr_dbg_sev_filter_cfm,
    [MSG_I(DBG_GET_SYS_STAT_CFM)]   = asr_dbg_sys_stat_cfm,
    [MSG_I(DBG_SET_HOST_LOG_CFM)]   = asr_dbg_set_host_log_cfm,
};
#endif
static const msg_cb_fct *msg_hdlrs[] = {
    [TASK_MM]    = mm_hdlrs,
#if CFG_DBG
    [TASK_DBG]   = dbg_hdlrs,
#endif
#ifdef CFG_STATION_SUPPORT
    [TASK_SCANU] = scan_hdlrs,
#endif
    [TASK_ME]    = me_hdlrs,
#ifdef CFG_STATION_SUPPORT
    [TASK_SM]    = sm_hdlrs,
#endif
#ifdef CFG_SOFTAP_SUPPORT
    [TASK_APM]   = apm_hdlrs,
#endif
};

bool is_log_ignore_msg(struct ipc_e2a_msg *msg)
{
    if (((MSG_T(msg->id) == TASK_MM) && ((MSG_I(msg->id) == MM_PS_CHANGE_IND)
                                         ||  (MSG_I(msg->id) == MM_P2P_VIF_PS_CHANGE_IND)
                                         ||  (MSG_I(msg->id) == MM_CHANNEL_PRE_SWITCH_IND)
                                         ||  (MSG_I(msg->id) == MM_CHANNEL_SWITCH_IND)
                                         ||  (MSG_I(msg->id) == MM_CHANNEL_SURVEY_IND)
                                         ||  (MSG_I(msg->id) == MM_P2P_NOA_UPD_IND)
                                         #ifdef CONFIG_ASR595X
                                         ||  (MSG_I(msg->id) == MM_TWT_TRAFFIC_IND)
                                         #endif
                                         ||  (MSG_I(msg->id) == MM_REMAIN_ON_CHANNEL_EXP_IND)
                                         ||  (MSG_I(msg->id) == MM_REMAIN_ON_CHANNEL_CFM)
                                         ||  (MSG_I(msg->id) == MM_TRAFFIC_REQ_IND)))
         || ((MSG_T(msg->id) == TASK_ME) && (MSG_I(msg->id) == MSG_I(ME_TRAFFIC_IND_CFM)))) {
        return true;
    } else
        return false;
}
/**
 *
 */
void asr_rx_handle_msg(struct asr_hw *asr_hw, struct ipc_e2a_msg *msg)
{
    if (!is_log_ignore_msg(msg)) {
        dbg(D_ERR, D_UWIFI_CTRL, "dRX [%d %d]", MSG_T(msg->id), MSG_I(msg->id));
    }

    asr_hw->cmd_mgr.msgind(&asr_hw->cmd_mgr, msg,
                            msg_hdlrs[MSG_T(msg->id)][MSG_I(msg->id)]);
}
