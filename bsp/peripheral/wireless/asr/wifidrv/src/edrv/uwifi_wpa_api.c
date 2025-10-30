/**
 ****************************************************************************************
 *
 * @file uwifi_wpa_api.c
 *
 * @brief wpa_supplicant call uwifi driver cfg80211_ops
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#include "uwifi_include.h"
#include "uwifi_kernel.h"
#include "uwifi_ops_adapter.h"
#include "wpa_adapter.h"
#include "uwifi_wpa_api.h"
#include "uwifi_asr_api.h"
#include "uwifi_msg.h"

#include "asr_dbg.h"
#ifdef CONFIG_IEEE80211W
#include "wpa_common.h"
#endif
#include "hostapd.h"

extern struct cfg80211_ops uwifi_cfg80211_ops;
extern uint8_t scan_done_flag;
extern uint8_t wifi_ready;
//extern uint8_t wifi_connect_err;
extern struct sta_info g_connect_info;

#if NX_SAE_PMK_CACHE
extern struct pmkid_caching user_pmkid_caching;
#endif


static struct countrycode_list ccodelist[] = {
{"CN",13},
{"EU",13},
{"JP",14},
{"US",11},
};

int wpa_get_channels_from_country_code(struct asr_hw *asr_hw)
{
    int channel = 13;
    int i;

    for(i=0;i<ARRAY_SIZE(ccodelist);i++)
    {
        if((asr_hw->country[0]==ccodelist[i].ccode[0]) && (asr_hw->country[1]==ccodelist[i].ccode[1]))
        {
            channel = ccodelist[i].channelcount;
            break;
        }
    }

    if(i==ARRAY_SIZE(ccodelist))
        dbg(D_ERR,D_UWIFI_CTRL,"Not find right country code(%c:%c), use default CN",asr_hw->country[0],asr_hw->country[1]);

    return channel;
}

#ifdef CFG_STATION_SUPPORT
void wpa_set_scan_info(struct wifi_scan_t* wifi_scan,
                struct asr_hw *asr_hw, struct cfg80211_scan_request *request)
{
    uint16_t i;
    uint16_t scan_freq[14]={2412,2417,2422,2427,2432,2437,2442,
                            2447,2452,2457,2462,2467,2472,2484};

    /* 1 channels: only support 2.4G */
    request->n_channels = wpa_get_channels_from_country_code(asr_hw);
    if(wifi_scan->scanmode==SCAN_FREQ_SSID)
        request->n_channels = 1;

    /* 2 ssids:  all channel, or ssid_scan/ssid_freq_scan */
    request->n_ssids = 1;//(wifi_scan->scanmode==SCAN_FREQ_SSID)?1:wifi_scan->scanmode;

    /* 3 wps need ie,not support wps
       wpas_build_ext_capab WLAN_EID_EXT_CAPAB=127, not support ext_capab
    */
    request->ie = NULL;
    request->ie_len = 0;

    /* 4 scan ssid or scan_freq_ssid*/
    if((request->n_ssids) && (wifi_scan->ssid != NULL) && (wifi_scan->ssid_len<=IEEE80211_MAX_SSID_LEN))
    {
        request->ssids->ssid_len = wifi_scan->ssid_len;
        memcpy(request->ssids->ssid,wifi_scan->ssid,wifi_scan->ssid_len);
    }

    if(wifi_scan->scanmode==SCAN_ALL_CHANNEL)
        request->ssids->ssid_len = 0;

    /* 5 channel*/
    if(wifi_scan->scanmode == SCAN_FREQ_SSID)
    {
        request->channels[0]->center_freq = wifi_scan->freq;
        request->channels[0]->band = IEEE80211_BAND_2GHZ;
        request->channels[0]->flags = 0;         // ori flags =0,wait to sure
        request->channels[0]->max_reg_power = 18;     // (dBm) , 20+ if need
    }
    else
    {
        for(i=0;i < request->n_channels;i++)
        {
            request->channels[i]->center_freq = scan_freq[i];
            request->channels[i]->band = IEEE80211_BAND_2GHZ;
            request->channels[i]->flags = 0;         // ori flags =0,wait to sure
            request->channels[i]->max_reg_power = 18;     // (dBm) , 20+ if need
        }
    }
    memcpy(request->bssid, wifi_scan->bssid, MAC_ADDR_LEN);

    /*6 no_cck */
    request->no_cck = 0;
}

uint8_t softap_scan = 0;
//uint8_t scan_timeout = 0;
int wpa_start_scan(struct wifi_scan_t* wifi_scan)
{
    int ret;
    int i;
    int n_ssids, n_channels;
    struct cfg80211_scan_request *request;
    struct asr_vif *asr_vif;
    struct asr_hw *asr_hw = uwifi_get_asr_hw();

    if(NULL == asr_hw)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"start_scan:pls open wifi first!");
        return -1;
    }

    if((asr_hw->sta_vif_idx == 0xff) || ((asr_vif=asr_hw->vif_table[asr_hw->sta_vif_idx]) == NULL))
    {
        if((asr_hw->ap_vif_idx == 0xff) || ((asr_vif=asr_hw->vif_table[asr_hw->ap_vif_idx]) == NULL))
        {
            dbg(D_ERR,D_UWIFI_CTRL,"start_scan:pls open sta mode first!sta=%0x ap=%0x\n",asr_hw->sta_vif_idx,asr_hw->ap_vif_idx);

            asr_wlan_set_scan_status(SCAN_MODE_ERR);
            return -1;
        }
        else
        {
            dbg(D_ERR, D_UWIFI_CTRL, "softap scan start!");
            softap_scan = 1;
        }
    }

    dbg(D_ERR,D_UWIFI_CTRL,"%s:sta=%d,ap=%d,softap_scan=%d,state=%d,ssid=%s\r\n",
        __func__,asr_hw->sta_vif_idx,asr_hw->ap_vif_idx,softap_scan,asr_vif->sta.connInfo.wifiState,
        wifi_scan->ssid_len > 0 ? wifi_scan->ssid : NULL);

    if(softap_scan == 0)
    {
        if((asr_vif->sta.connInfo.wifiState == WIFI_SCANNING)
            ||(asr_vif->sta.connInfo.wifiState == LINKED_SCAN)
            ||(asr_vif->sta.connInfo.wifiState == WIFI_WAITING_SCAN_RES)
            ||(asr_vif->sta.connInfo.wifiState == LINKED_WAITING_SCAN_RES)
            ||(asr_vif->sta.connInfo.wifiState == WIFI_CONNECTING)
            ||(asr_vif->sta.connInfo.wifiState == WIFI_ASSOCIATED))
        {
            dbg(D_ERR,D_UWIFI_CTRL,"Skip scan %d\n",asr_vif->sta.connInfo.wifiState);

            if(SCAN_TIME_OUT == scan_done_flag)
            {
                asr_vif->sta.connInfo.wifiState = WIFI_SCAN_FAILED;
                dbg(D_ERR,D_UWIFI_CTRL,"TIMEOUT Skip scan %d\n",asr_vif->sta.connInfo.wifiState);
            }
            return -1;
        }
    }

    n_ssids = 1;//(wifi_scan->scanmode==SCAN_FREQ_SSID)?1:wifi_scan->scanmode;
    //first we get channels from country code
    n_channels = wpa_get_channels_from_country_code(asr_hw);
    if(wifi_scan->scanmode==SCAN_FREQ_SSID)
        n_channels =1;

    request = asr_rtos_malloc(sizeof(*request)
            + sizeof(*request->ssids) * n_ssids
            + sizeof(*request->channels) * n_channels);

    if (!request) {
        ret = -ENOMEM;
        dbg(D_ERR,D_UWIFI_CTRL,"%s:request is NULL",__func__);
        return ret;
    }
    if(n_ssids)
        request->ssids = (void *)&request->channels[n_channels];

    for(i=0;i<n_channels;i++)
    {
        request->channels[i] = asr_rtos_malloc(sizeof(struct ieee80211_channel));
        if(NULL==request->channels[i])
        {
            int j;
            for(j=0;j<i;j++)
            {
                asr_rtos_free(request->channels[j]);
                request->channels[j] = NULL;
            }
            asr_rtos_free(request);
            request = NULL;

            dbg(D_ERR,D_UWIFI_CTRL,"%s:request->channels[%d] is NULL",__func__,i);
            return -ENOMEM;
        }
    }

    if(softap_scan == 0)
    {
        if(asr_vif->sta.connInfo.wifiState >= WIFI_ASSOCIATED)
        {
            dbg(D_ERR,D_UWIFI_CTRL,"%s:wifiState=%d\r\n",__func__,asr_vif->sta.connInfo.wifiState);
            if(asr_vif->sta.connInfo.wifiState != LINKED_SCAN_FAILED)
                asr_vif->sta.connInfo.wifiState = LINKED_SCAN;
        }
        else
            asr_vif->sta.connInfo.wifiState = WIFI_SCANNING;
    }
    //clear scan_list and scan_count
    memset(asr_hw->scan_list, 0, sizeof(struct scan_result)*MAX_AP_NUM);
    asr_hw->scan_count = 0;
    asr_hw->phead_scan_result = NULL;

    wpa_set_scan_info(wifi_scan, asr_hw, request);

    ret = uwifi_cfg80211_ops.scan(asr_vif, request);
    if(ret != 0)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s:scan send firmware failed, ret is %d",__func__,ret);
        for(i=0;i<request->n_channels;i++)
        {
            asr_rtos_free(request->channels[i]);
            request->channels[i] = NULL;
        }
        asr_rtos_free(request);
        request = NULL;

        if(softap_scan == 0)
        {
            if(asr_vif->sta.connInfo.wifiState >= WIFI_ASSOCIATED)
                asr_vif->sta.connInfo.wifiState = LINKED_SCAN_FAILED;
            else
                asr_vif->sta.connInfo.wifiState = WIFI_SCAN_FAILED;
        }
    }
#if 0
    if(asr_hw->connInfo.wifiState >= WIFI_ASSOCIATED)
        asr_hw->connInfo.wifiState = LINKED_WAITING_SCAN_RES;
    else
        asr_hw->connInfo.wifiState = WIFI_WAITING_SCAN_RES;
#endif
    return ret;
}
#endif

static int hex2num(uint8_t c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static int hex2byte(uint8_t *hex)
{
    int a, b;
    a = hex2num(*hex++);
    if (a < 0)
        return -1;
    b = hex2num(*hex++);
    if (b < 0)
        return -1;
    return (a << 4) | b;
}

/**
 * hexstr2bin - Convert ASCII hex string into binary data
 * @hex: ASCII hex string (e.g., "01ab")
 * @buf: Buffer for the binary data
 * @len: Length of the text to convert in bytes (of buf); hex will be double
 * this size
 * Returns: 0 on success, -1 on failure (invalid hex string)
 */
int hexstr2bin(uint8_t *hex, uint8_t *buf, size_t len)
{
    size_t i;
    int a;
    uint8_t *ipos = hex;
    uint8_t *opos = buf;

    for (i = 0; i < len; i++) {
        a = hex2byte(ipos);
        if (a < 0)
            return -1;
        *opos++ = a;
        ipos += 2;
    }
    return 0;
}

bool wpa_build_connect_sme(struct asr_vif *asr_vif, struct wifi_conn_t * wifi_connect_param,
                struct scan_result *p_scan, struct cfg80211_connect_params *sme)
{
    struct key_params key_params;

    sme->crypto.cipher_group =        p_scan->crypto.cipher_group;
    sme->crypto.n_ciphers_pairwise =  p_scan->crypto.n_ciphers_pairwise;
    sme->crypto.ciphers_pairwise[0] = p_scan->crypto.ciphers_pairwise[0];
    sme->crypto.control_port =        p_scan->crypto.control_port;

    sme->crypto.control_port_no_encrypt = p_scan->crypto.control_port_no_encrypt;
    sme->mfp = NL80211_MFP_NO;
    sme->crypto.control_port_ethertype = wlan_htons(p_scan->crypto.control_port_ethertype);
#ifdef CFG_ENCRYPT_SUPPORT
#if (defined(CONFIG_IEEE80211W) || NX_SAE)
    sme->crypto.cipher_mgmt_group = p_scan->crypto.cipher_mgmt_group;
    sme->mfp = p_scan->crypto.mfp_option;
    sme->crypto.akm_suites[0] = p_scan->crypto.akm_suites[0];
#endif
#endif

    memcpy(sme->bssid, p_scan->bssid, ETH_ALEN);

    sme->channel->band = IEEE80211_BAND_2GHZ;
    sme->channel->center_freq = p_scan->channel.center_freq;
    sme->channel->flags = p_scan->channel.flags;

    sme->ssid_len = p_scan->ssid.ssid_len;
    memcpy(sme->ssid, p_scan->ssid.ssid, p_scan->ssid.ssid_len);

    if(p_scan->encryp_protocol == WIFI_ENCRYP_OPEN)
        sme->ie_len = 0;
    #ifdef CFG_ENCRYPT_SUPPORT
    else if(p_scan->encryp_protocol == WIFI_ENCRYP_WPA)
    {
        sme->ie_len = p_scan->ie_len;
        memcpy(sme->ie,p_scan->ie, p_scan->ie_len);
    }else if(p_scan->encryp_protocol == WIFI_ENCRYP_WPA2)
    {
        sme->ie_len = p_scan->ie_len;
        memcpy(sme->ie,p_scan->ie, p_scan->ie_len);
        //sme->ie[21] |=BIT(7); need? MFP bit
    }else if((p_scan->encryp_protocol == WIFI_ENCRYP_WEP))
    {
        sme->ie_len = 0;
        sme->key_idx = 0; //defalut 0

        if(wifi_connect_param->pwd_len==5 || wifi_connect_param->pwd_len==10)
        {
            if(wifi_connect_param->pwd_len==5)
                memcpy(sme->key,wifi_connect_param->password,5);
            else
                hexstr2bin(wifi_connect_param->password,sme->key,5);

            sme->key_len = 5;
            sme->crypto.cipher_group = WLAN_CIPHER_SUITE_WEP40;
            sme->crypto.ciphers_pairwise[0] = WLAN_CIPHER_SUITE_WEP40;
        }
        else if(wifi_connect_param->pwd_len==13 || wifi_connect_param->pwd_len==26)
        {
            if(wifi_connect_param->pwd_len==13)
                memcpy(sme->key,wifi_connect_param->password,13);
            else
                hexstr2bin(wifi_connect_param->password,sme->key,13);

            sme->key_len = 13;
            sme->crypto.cipher_group = WLAN_CIPHER_SUITE_WEP104;
            sme->crypto.ciphers_pairwise[0] = WLAN_CIPHER_SUITE_WEP104;
        }
        else
        {
            dbg(D_ERR,D_UWIFI_CTRL,"wep key length is not right");
            return false;
        }

        //build add_key key_params
        //key_params->seq,seq_len  don't need.
        key_params.seq = NULL;
        key_params.seq_len = 0;
        key_params.cipher = sme->crypto.cipher_group;
        key_params.key = sme->key;
        key_params.key_len = sme->key_len;

        uwifi_cfg80211_ops.add_key(asr_vif, sme->key_idx, false, NULL, &key_params);
    }
    #endif
    sme->auth_type = NL80211_AUTHTYPE_OPEN_SYSTEM;

    /*   WEP share_key, need  more !!!
     ** if()
     **   sme->auth_type = NL80211_AUTHTYPE_SHARED_KEY;
     **
     ** sme->key = ;
     ** sme->key_len = ;
     */
    #ifdef CFG_ENCRYPT_SUPPORT
    #if NX_SAE
    if (p_scan->crypto.akm_suites[0] == RSN_AUTH_KEY_MGMT_SAE)
    {
        sme->auth_type = NL80211_AUTHTYPE_SAE;

        #if NX_SAE_PMK_CACHE
        /*check if PMK cached exist*/
        struct sta_info *pstat = &g_connect_info;
        struct wpa_priv *p_wpa_priv =  g_pwpa_priv[COEX_MODE_STA];
        //struct pmkid_caching *pmkid_cache_ptr = &p_wpa_priv->wpa_global_info.pmkid_cache;
        struct pmkid_caching *pmkid_cache_ptr = &user_pmkid_caching;
        unsigned char     pmk_cache_idx;

        pstat->pmkid_caching_idx = search_by_mac_pmkid_cache(p_wpa_priv, sme->bssid);
        if(pmkid_cached(pstat)) {
            pmk_cache_idx = pstat->pmkid_caching_idx;
            dbg(D_ERR,D_UWIFI_CTRL,"pmk cache exist,use cached PMK/PMKID ,idx[%d]",pmk_cache_idx);

            //memcpy(pstat->wpa_sta_info.PMK, pmkid_cache_ptr->pmkid_caching_entry[pmk_cache_idx].pmk, PMK_LEN);

            memcpy(pstat->sae_pmk, pmkid_cache_ptr->pmkid_caching_entry[pmk_cache_idx].pmk, PMK_LEN);

            memcpy(pstat->sae_pmkid, pmkid_cache_ptr->pmkid_caching_entry[pmk_cache_idx].pmkid, PMKID_LEN);

            dbg(D_ERR, D_UWIFI_CTRL, "[build sme]cached PMK");
            dbg(D_ERR, D_UWIFI_CTRL, "[build sme]cached PMKID");

            /*go on normal auth */
            sme->auth_type = NL80211_AUTHTYPE_OPEN_SYSTEM;

            // add pmkid in  rsne IE elmet for (re)associate frame,update scan ie and sme ie
            {
                // rsn_ie_hdr + (group_cipher(4)) + [pair_cipher_count(2) + pair_cipher(4)] + [akm_count(2)+akm_suit(4)] + (RSN_CAP(2))
                uint16_t length_before_pmkid = (sizeof(struct rsn_ie_hdr) + RSN_SELECTOR_LEN + (2+RSN_SELECTOR_LEN)+(2+RSN_SELECTOR_LEN) +2 );
                uint8_t  ie_temp[MAX_IE_LEN]={0};
                uint16_t length_after_pmkid = p_scan->ie_len - length_before_pmkid;
                uint8_t *pos;
                uint8_t update_pmkid = 0;
                uint8_t pmkid_cnt_exist = 0;
                struct wpa_priv *priv = g_pwpa_priv[COEX_MODE_STA];

                pos = p_scan->ie + length_before_pmkid;

                // if group mgt cipher suit exits , pmkid_cnt will be set 0
                if (length_after_pmkid >= 6) {
                    pmkid_cnt_exist = 1;
                    memcpy(ie_temp, pos+2, (length_after_pmkid-2)); //skip pmk_id count
                } else {
                    pmkid_cnt_exist = 0;
                    memcpy(ie_temp, pos, length_after_pmkid);
                }

                //dbg(D_ERR,D_UWIFI_CTRL,"RSN IE, length_before_pmkid=%d, length_after_pmkid=%d",length_before_pmkid,length_after_pmkid);
                dbg(D_ERR, D_UWIFI_CTRL, "ie_temp ");

                if (length_after_pmkid >= 18) {  //check pmk id has been added before
                    u16 num_pmkid = WPA_GET_LE16(pos);
                    if ((num_pmkid != 1) || (memcmp(pos+2,pstat->sae_pmkid,PMKID_LEN)))
                       update_pmkid =1;
                } else
                {
                   update_pmkid = 1;
                }
                dbg(D_ERR,D_UWIFI_CTRL,"update_pmkid=(%d)",update_pmkid);

                if (update_pmkid){    // add pmkid
                    /* PMKID Count (2 octets, little endian) */
                    *pos++ = 1;
                    *pos++ = 0;
                    /* PMKID */
                    memcpy(pos, pstat->sae_pmkid, PMKID_LEN);
                    pos += PMKID_LEN;

                    if (pmkid_cnt_exist) {
                        memcpy(pos, ie_temp, (length_after_pmkid-2));
                        p_scan->ie_len += PMKID_LEN;
                        *(p_scan->ie + 1) += PMKID_LEN;  //update rsn_ie_hdr length
                    } else {
                        memcpy(pos, ie_temp, length_after_pmkid);
                        p_scan->ie_len += 2 + PMKID_LEN;
                        *(p_scan->ie + 1) += 2 + PMKID_LEN;
                    }

                   // update sme ie
                   sme->ie_len = p_scan->ie_len;
                   memcpy(sme->ie,p_scan->ie, p_scan->ie_len);

                   /* STA save wpa_ie for further EAPOL packets construction */
                   memcpy(priv->wpa_global_info.AuthInfoBuf, p_scan->ie, p_scan->ie_len);
                   priv->wpa_global_info.AuthInfoElement.Octet = priv->wpa_global_info.AuthInfoBuf;
                   priv->wpa_global_info.AuthInfoElement.Length = p_scan->ie_len;

               }
               dbg(D_ERR, D_UWIFI_CTRL, "scan list update RSNE IE");
             }
             dbg(D_ERR,D_UWIFI_CTRL,"pmkid cache,go on normal auth,ie_len=%d",sme->ie_len);

        }
        #endif //NX_SAE_PMK_CACHE

        if (NL80211_AUTHTYPE_SAE == sme->auth_type) {
            //need send pwd/pwd_len to lwifi for sae flow
            sme->pwd_len = wifi_connect_param->pwd_len;
            memcpy(sme->password,wifi_connect_param->password,wifi_connect_param->pwd_len);
        }
    }
    #endif //NX_SAE
    #endif
    return true;
}

int wpa_build_connect_sme_adv(struct asr_vif *asr_vif, struct config_info * pst_config_info,
                                    struct cfg80211_connect_params *sme)
{
    struct key_params key_params;
    memset(&key_params, 0, sizeof(struct key_params));

    sme->crypto.control_port            = pst_config_info->control_port;
    sme->crypto.control_port_ethertype  = wlan_htons(pst_config_info->control_port_ethertype);
    sme->crypto.cipher_group            = pst_config_info->cipher_group;
    sme->crypto.ciphers_pairwise[0]     = pst_config_info->cipher_pairwise;
#ifdef CFG_ENCRYPT_SUPPORT
#if (defined(CONFIG_IEEE80211W) || NX_SAE)
    sme->crypto.cipher_mgmt_group = pst_config_info->cipher_mgmt_group;
    sme->mfp = pst_config_info->mfp_option;
    sme->crypto.akm_suites[0] = pst_config_info->akm_suite;
#endif
#endif

    memcpy(sme->bssid, pst_config_info->connect_ap_info.bssid, ETH_ALEN);
    sme->channel->band = IEEE80211_BAND_2GHZ;
    sme->channel->center_freq = phy_channel_to_freq(IEEE80211_BAND_2GHZ, pst_config_info->connect_ap_info.channel);
    sme->channel->flags = passive_scan_flag(asr_vif->asr_hw->wiphy->bands[IEEE80211_BAND_2GHZ]->channels[pst_config_info->connect_ap_info.channel-1].flags);
    sme->ssid_len = pst_config_info->connect_ap_info.ssid_len;
    memcpy(sme->ssid, pst_config_info->connect_ap_info.ssid, pst_config_info->connect_ap_info.ssid_len);

    switch (pst_config_info->connect_ap_info.security)
    {
        case WIFI_ENCRYP_OPEN:
            sme->crypto.n_ciphers_pairwise  = 0;
            sme->ie_len                     = 0;
            break;
        #ifdef CFG_ENCRYPT_SUPPORT
        case WIFI_ENCRYP_WEP:
            sme->ie_len                     = 0;
            sme->crypto.n_ciphers_pairwise  = 1;
            if(pst_config_info->connect_ap_info.pwd_len==5 || pst_config_info->connect_ap_info.pwd_len==10)
            {
                if(pst_config_info->connect_ap_info.pwd_len==5)
                    memcpy(sme->key,pst_config_info->connect_ap_info.pwd,5);
                else
                    hexstr2bin((uint8_t *)pst_config_info->connect_ap_info.pwd,sme->key,5);

                sme->key_len = 5;
                sme->crypto.cipher_group = WLAN_CIPHER_SUITE_WEP40;
                sme->crypto.ciphers_pairwise[0] = WLAN_CIPHER_SUITE_WEP40;
            }
            else if(pst_config_info->connect_ap_info.pwd_len==13 || pst_config_info->connect_ap_info.pwd_len==26)
            {
                if(pst_config_info->connect_ap_info.pwd_len==13)
                    memcpy(sme->key,pst_config_info->connect_ap_info.pwd,13);
                else
                    hexstr2bin((uint8_t *)pst_config_info->connect_ap_info.pwd,sme->key,13);

                sme->key_len = 13;
                sme->crypto.cipher_group = WLAN_CIPHER_SUITE_WEP104;
                sme->crypto.ciphers_pairwise[0] = WLAN_CIPHER_SUITE_WEP104;
            }
            else
            {
                dbg(D_ERR,D_UWIFI_CTRL,"wep key length is not right");
                return -1;
            }

            //build add_key key_params
            //key_params->seq,seq_len  don't need.
            key_params.seq = NULL;
            key_params.seq_len = 0;
            key_params.cipher = sme->crypto.cipher_group;
            key_params.key = sme->key;
            key_params.key_len = sme->key_len;

            uwifi_cfg80211_ops.add_key(asr_vif, sme->key_idx, false, NULL, &key_params);
            break;
        case WIFI_ENCRYP_WPA:
            sme->crypto.n_ciphers_pairwise  = 1;
            sme->ie_len = pst_config_info->ie_len;
            memcpy(sme->ie,pst_config_info->ie, pst_config_info->ie_len);
            break;
        case WIFI_ENCRYP_WPA2:
            sme->crypto.n_ciphers_pairwise  = 1;
            sme->ie_len = pst_config_info->ie_len;
            memcpy(sme->ie,pst_config_info->ie, pst_config_info->ie_len);
            break;
        #endif
        default:
            dbg(D_ERR,D_UWIFI_CTRL,"wpa_build_connect_sme_adv wrong encrypt");
            break;
    }
    sme->auth_type = NL80211_AUTHTYPE_OPEN_SYSTEM;

#ifdef CFG_ENCRYPT_SUPPORT
#if NX_SAE
    if (pst_config_info->akm_suite == RSN_AUTH_KEY_MGMT_SAE)
    {
        sme->auth_type = NL80211_AUTHTYPE_SAE;

    #if NX_SAE_PMK_CACHE
        /*check if PMK cached exist*/
        struct sta_info *pstat = &g_connect_info;
        struct wpa_priv *p_wpa_priv =     g_pwpa_priv[COEX_MODE_STA];
        //struct pmkid_caching *pmkid_cache_ptr = &p_wpa_priv->wpa_global_info.pmkid_cache;
        struct pmkid_caching *pmkid_cache_ptr = &user_pmkid_caching;
        unsigned char     pmk_cache_idx;

        pstat->pmkid_caching_idx = search_by_mac_pmkid_cache(p_wpa_priv, sme->bssid);
        if(pmkid_cached(pstat)) {
           pmk_cache_idx = pstat->pmkid_caching_idx;
           dbg(D_ERR,D_UWIFI_CTRL,"pmk cache exist,use cached PMK/PMKID ,idx[%d]",pmk_cache_idx);

           //memcpy(pstat->wpa_sta_info.PMK, pmkid_cache_ptr->pmkid_caching_entry[pmk_cache_idx].pmk, PMK_LEN);

           memcpy(pstat->sae_pmk, pmkid_cache_ptr->pmkid_caching_entry[pmk_cache_idx].pmk, PMK_LEN);

           memcpy(pstat->sae_pmkid, pmkid_cache_ptr->pmkid_caching_entry[pmk_cache_idx].pmkid, PMKID_LEN);

           dbg(D_ERR, D_UWIFI_CTRL, "[build sme_adv]cached PMK");
           dbg(D_ERR, D_UWIFI_CTRL, "[build sme_adv]cached PMKID");

           /*go on normal auth */
           sme->auth_type = NL80211_AUTHTYPE_OPEN_SYSTEM;
           // add pmkid in    rsne IE elmet for (re)associate frame,update scan ie and sme ie
           {
               uint16_t length_before_pmkid = (sizeof(struct rsn_ie_hdr) + RSN_SELECTOR_LEN + (2+RSN_SELECTOR_LEN)+(2+RSN_SELECTOR_LEN) +2 );
               uint8_t    ie_temp[MAX_IE_LEN]={0};
               uint16_t length_after_pmkid = pst_config_info->ie_len - length_before_pmkid;
               uint8_t *pos;
               uint8_t update_pmkid = 0;
               uint8_t pmkid_cnt_exist = 0;
               struct wpa_priv *priv = g_pwpa_priv[COEX_MODE_STA];

               pos = pst_config_info->ie + length_before_pmkid;

               // if group mgt cipher suit exits , pmkid_cnt will be set 0
               if (length_after_pmkid >= 6) {
                                   if ((length_after_pmkid-2) > MAX_IE_LEN) {
                               dbg(D_ERR, D_UWIFI_CTRL, "ie temp oversize,%d > %d", length_after_pmkid , MAX_IE_LEN);
                       return -1;
                   }

                   pmkid_cnt_exist = 1;
                   memcpy(ie_temp, pos+2, (length_after_pmkid-2)); //skip pmk_id count
               } else {
                   pmkid_cnt_exist = 0;
                   memcpy(ie_temp, pos, length_after_pmkid);
               }

               if (length_after_pmkid >= 18) {    //check pmk id has been added before
                   u16 num_pmkid = WPA_GET_LE16(pos);
                   if ((num_pmkid != 1) || (memcmp(pos+2,pstat->sae_pmkid,PMKID_LEN)))
                      update_pmkid =1;
               } else
               {
                  update_pmkid = 1;
               }
               dbg(D_ERR,D_UWIFI_CTRL,"update_pmkid=(%d)",update_pmkid);

               if (update_pmkid){     // add pmkid
                   /* PMKID Count (2 octets, little endian) */
                   *pos++ = 1;
                   *pos++ = 0;
                   /* PMKID */
                   memcpy(pos, pstat->sae_pmkid, PMKID_LEN);
                   pos += PMKID_LEN;

                  if (pmkid_cnt_exist) {
                      memcpy(pos, ie_temp, (length_after_pmkid-2));
                      pst_config_info->ie_len += PMKID_LEN;
                      *(pst_config_info->ie + 1) += PMKID_LEN;    //update rsn_ie_hdr length
                  } else {
                      memcpy(pos, ie_temp, length_after_pmkid);
                      pst_config_info->ie_len += 2 + PMKID_LEN;
                      *(pst_config_info->ie + 1) += 2 + PMKID_LEN;    //update rsn_ie_hdr length
                  }

                  // update sme ie
                  sme->ie_len = pst_config_info->ie_len;
                  memcpy(sme->ie,pst_config_info->ie, pst_config_info->ie_len);

                  /* STA save wpa_ie for further EAPOL packets construction */
                  memcpy(priv->wpa_global_info.AuthInfoBuf, pst_config_info->ie, pst_config_info->ie_len);
                  priv->wpa_global_info.AuthInfoElement.Octet = priv->wpa_global_info.AuthInfoBuf;
                  priv->wpa_global_info.AuthInfoElement.Length = pst_config_info->ie_len;

              }
              dbg(D_ERR, D_UWIFI_CTRL, "config_info update RSNE IE");
            }
           dbg(D_ERR,D_UWIFI_CTRL,"pmkid cache,go on normal auth,ie_len=%d",sme->ie_len);

       }
   #endif //NX_SAE_PMK_CACHE

        if (NL80211_AUTHTYPE_SAE == sme->auth_type) {
            //need send pwd/pwd_len to lwifi for sae flow
            sme->pwd_len = pst_config_info->connect_ap_info.pwd_len;
            memcpy(sme->password,pst_config_info->connect_ap_info.pwd,pst_config_info->connect_ap_info.pwd_len);

#if NX_STA_AP_COEX
             extern struct uwifi_ap_cfg_param g_ap_cfg_info;
             extern uint32_t current_iftype;
             struct asr_hw * asr_hw = asr_vif->asr_hw;
             if((asr_hw->ap_vif_idx != 0xff)
                 &&(asr_hw->chanctx_table[asr_hw->vif_table[asr_hw->ap_vif_idx]->ch_index].chan_def.chan)
                 &&(sme->channel->center_freq != asr_hw->chanctx_table[asr_hw->vif_table[asr_hw->ap_vif_idx]->ch_index].chan_def.chan->center_freq))
             {
                 struct uwifi_ap_cfg_param *ap_cfg_param;

                 ap_cfg_param = &g_ap_cfg_info;

                 ap_cfg_param->re_enable = 1;

                 uwifi_close_wifi(NL80211_IFTYPE_AP);

                 current_iftype = STA_MODE;
             }
#endif


        }
    }
#endif //NX_SAE
#endif

     return 0;
}

int cipher_suite_to_wpa_cipher(uint32_t cipher_suite)
{
    int cipher;
    /* Retrieve the cipher suite selector */
    switch (cipher_suite) {
    case WLAN_CIPHER_SUITE_WEP40:
        cipher = WPA_CIPHER_WEP40;
        break;
    case WLAN_CIPHER_SUITE_WEP104:
        cipher = WPA_CIPHER_WEP104;
        break;
    case WLAN_CIPHER_SUITE_TKIP:
        cipher = WPA_CIPHER_TKIP;
        break;
    case WLAN_CIPHER_SUITE_CCMP:
        cipher = WPA_CIPHER_CCMP;
        break;
    default:
        cipher = WPA_CIPHER_NONE;
        break;
    }
    return cipher;
}

void save_wifi_config_to_flash(struct config_info *configInfo)
{
}

void clear_wifi_config_from_flash(void)
{
}

void wpa_update_config_info(struct config_info *pst_config_info, struct scan_result *pst_scan_result)
{
    pst_config_info->cipher_group           = pst_scan_result->crypto.cipher_group;
    pst_config_info->cipher_pairwise        = pst_scan_result->crypto.ciphers_pairwise[0];
    pst_config_info->control_port           = pst_scan_result->crypto.control_port;
    pst_config_info->control_port_ethertype = pst_scan_result->crypto.control_port_ethertype;
    pst_config_info->connect_ap_info.channel= phy_freq_to_channel(IEEE80211_BAND_2GHZ, pst_scan_result->channel.center_freq);
    pst_config_info->connect_ap_info.security= pst_scan_result->encryp_protocol;

    if (pst_config_info->en_autoconnect != AUTOCONNECT_DISABLED)
        pst_config_info->en_autoconnect         = AUTOCONNECT_DIRECT_CONNECT;    /*when autoconnect not disable,update to direct connect */

#ifdef CFG_ENCRYPT_SUPPORT
#if (defined(CONFIG_IEEE80211W) || NX_SAE)
    pst_config_info->cipher_mgmt_group = pst_scan_result->crypto.cipher_mgmt_group;
    pst_config_info->mfp_option = pst_scan_result->crypto.mfp_option;
    pst_config_info->akm_suite = pst_scan_result->crypto.akm_suites[0];

    if (pst_scan_result->rsnxe_len) {
        pst_config_info->rsnxe = pst_scan_result->rsnxe[2];
    } else {
        pst_config_info->rsnxe = 0;
    }
#endif
#endif

    pst_config_info->ie_len          = pst_scan_result->ie_len;
    memset(pst_config_info->ie, 0, MAX_IE_LEN);
    memcpy(pst_config_info->ie, pst_scan_result->ie, pst_scan_result->ie_len);
    memcpy(pst_config_info->connect_ap_info.bssid, pst_scan_result->bssid, ETH_ALEN);

    dbg(D_ERR, D_UWIFI_CTRL, "update ap channel:%d,en_autoconnect=%d",pst_config_info->connect_ap_info.channel,pst_config_info->en_autoconnect);
    /*
    if write flash need more time, we can create a task to write
    */
    save_wifi_config_to_flash(pst_config_info);
}

#ifdef CFG_STATION_SUPPORT
void wpa_save_config_info(struct config_info *pst_config_info, struct wifi_conn_t *pst_wifi_connect_param, struct scan_result *pst_scan_result)
{
    memset(pst_config_info, 0, sizeof(struct config_info));
    pst_config_info->cipher_group           = pst_scan_result->crypto.cipher_group;
    pst_config_info->cipher_pairwise        = pst_scan_result->crypto.ciphers_pairwise[0];
    pst_config_info->control_port           = pst_scan_result->crypto.control_port;
    pst_config_info->control_port_ethertype = pst_scan_result->crypto.control_port_ethertype;
    pst_config_info->connect_ap_info.security = pst_wifi_connect_param->encrypt;
    pst_config_info->connect_ap_info.channel  = phy_freq_to_channel(IEEE80211_BAND_2GHZ, pst_scan_result->channel.center_freq);
    pst_config_info->wifi_retry_interval    = AUTOCONNECT_INTERVAL_INIT;

    if (pst_config_info->en_autoconnect != AUTOCONNECT_DISABLED)
        pst_config_info->en_autoconnect         = AUTOCONNECT_SCAN_CONNECT;

#ifdef CFG_ENCRYPT_SUPPORT
#if (defined(CONFIG_IEEE80211W) || NX_SAE)
    pst_config_info->cipher_mgmt_group = pst_scan_result->crypto.cipher_mgmt_group;
    pst_config_info->mfp_option = pst_scan_result->crypto.mfp_option;
    pst_config_info->akm_suite = pst_scan_result->crypto.akm_suites[0];

    if (pst_scan_result->rsnxe_len) {
        pst_config_info->rsnxe = pst_scan_result->rsnxe[2];
    } else {
        pst_config_info->rsnxe = 0;
    }
#endif
#endif

    pst_config_info->connect_ap_info.ssid_len = pst_wifi_connect_param->ssid_len;
    memcpy(pst_config_info->connect_ap_info.ssid, pst_wifi_connect_param->ssid, pst_wifi_connect_param->ssid_len);

    pst_config_info->connect_ap_info.pwd_len = pst_wifi_connect_param->pwd_len;
    memcpy(pst_config_info->connect_ap_info.pwd,pst_wifi_connect_param->password,pst_wifi_connect_param->pwd_len);

    pst_config_info->ie_len          = pst_scan_result->ie_len;
    memcpy(pst_config_info->ie, pst_scan_result->ie, pst_scan_result->ie_len);
    memcpy(pst_config_info->connect_ap_info.bssid, pst_scan_result->bssid, ETH_ALEN);

    /*
    if write flash need more time, we can create a task to write
    */
    save_wifi_config_to_flash(pst_config_info);
}

int wpa_start_connect(struct wifi_conn_t * wifi_connect_param)
{
    int                             ret = 0;
    struct wpa_psk                  psk;
    struct cfg80211_connect_params  sme;
    struct asr_vif                *asr_vif;
    struct config_info             *pst_config_info;
    memset(&psk, 0, sizeof(struct wpa_psk));
    memset(&sme, 0, sizeof(struct cfg80211_connect_params));

    struct asr_hw                 *asr_hw = uwifi_get_asr_hw();
    if(NULL == asr_hw)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s:asr_hw is NULL",__func__);
        ret = -1;
        goto free;
    }

    asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];
    if(NULL == asr_vif)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s:asr_vif is NULL",__func__);
        ret = -1;
        goto free;
    }

    sme.channel = asr_rtos_malloc(sizeof(struct ieee80211_channel));
    if(sme.channel == NULL)
    {
        ret = -1;
        goto free;
    }
    memset(sme.channel, 0, sizeof(struct ieee80211_channel));

    asr_rtos_lock_mutex(&asr_hw->mutex);

    //if have many APs like roaming network, find best rssi AP
    struct scan_result* pscanResult = asr_hw->phead_scan_result;

    while(NULL != pscanResult)
    {
        if(!memcmp(wifi_connect_param->ssid, pscanResult->ssid.ssid, pscanResult->ssid.ssid_len) &&
            (pscanResult->ssid.ssid_len == wifi_connect_param->ssid_len) &&
            (pscanResult->encryp_protocol == wifi_connect_param->encrypt))
            break;
        else
            pscanResult = pscanResult->next_ptr;
    }
    if(NULL == pscanResult)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s:did not find want connect ap from scan results",__func__);
        asr_rtos_unlock_mutex(&asr_hw->mutex);
        ret = -1;
        goto free;
    }

    /*
    like Android,when connect, first save config
    */
    pst_config_info = &(asr_vif->sta.configInfo);
    wpa_save_config_info(pst_config_info, wifi_connect_param, pscanResult);

    if((wifi_connect_param->encrypt != WIFI_ENCRYP_OPEN) && (wifi_connect_param->encrypt != WIFI_ENCRYP_WEP))
    {
        //for wpa store info
        memcpy(psk.macaddr, asr_vif->wlan_mac_add.mac_addr, ETH_ALEN);
        psk.ssid_len = wifi_connect_param->ssid_len;
        memcpy(psk.ssid,wifi_connect_param->ssid,wifi_connect_param->ssid_len);
        psk.pwd_len = wifi_connect_param->pwd_len;
        memcpy(psk.password,wifi_connect_param->password,wifi_connect_param->pwd_len);
        psk.encrpy_protocol = wifi_connect_param->encrypt;
        psk.cipher_group = cipher_suite_to_wpa_cipher(pscanResult->crypto.cipher_group);
        psk.ciphers_pairwise = cipher_suite_to_wpa_cipher(pscanResult->crypto.ciphers_pairwise[0]);
#if (defined(CONFIG_IEEE80211W) || NX_SAE)
        psk.cipher_mgmt_group = pscanResult->crypto.cipher_mgmt_group;
        psk.key_mgmt = pscanResult->crypto.akm_suites[0];
#endif

        if(pscanResult->ie_len)
        {
            psk.wpa_ie_len = pscanResult->ie_len;
            memcpy(psk.wpa_ie, pscanResult->ie, pscanResult->ie_len);
        }

        //temp disable
        wpa_connect_init(&psk, WLAN_OPMODE_INFRA);
    }

    if(!wpa_build_connect_sme(asr_vif, wifi_connect_param, pscanResult, &sme))
    {
        dbg(D_ERR,D_UWIFI_CTRL,"wpa_build_connect_sme failed, please check connect sme parameter");
        asr_rtos_unlock_mutex(&asr_hw->mutex);
        ret = -1;
        goto free;
    }

#if NX_SAE
    asr_vif->sta.rsnxe = pst_config_info->rsnxe;
#endif

    asr_vif->sta.connInfo.wifiState = WIFI_CONNECTING;
    ret = uwifi_cfg80211_ops.connect(asr_vif, &sme);
    if(ret != 0)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s:connect send firmware failed, ret is %d",__func__,ret);
        asr_vif->sta.connInfo.wifiState = WIFI_CONNECT_FAILED;
    }

    asr_rtos_unlock_mutex(&asr_hw->mutex);

free:
    if (NULL != sme.channel)
    {
        asr_rtos_free(sme.channel);
        sme.channel = NULL;
    }

    return ret;
}

int wpa_start_connect_adv(struct config_info * pst_config_info)
{
    int ret = 0;
    struct wpa_psk psk;
    struct cfg80211_connect_params sme;
    struct asr_vif *asr_vif;
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    memset(&psk, 0, sizeof(struct wpa_psk));
    memset(&sme, 0, sizeof(struct cfg80211_connect_params));

    if (NULL == asr_hw)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:asr_hw is NULL", __func__);
        return -1;
    }

    asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];
    if (NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:asr_vif is NULL", __func__);
        return -1;
    }

    if (asr_vif->sta.connInfo.wifiState >= WIFI_ASSOCIATED)
    {
        return 0;
    }

    sme.channel = asr_rtos_malloc(sizeof(struct ieee80211_channel));
    if(sme.channel == NULL)
    {
        return -1;
    }
    memset(sme.channel, 0, sizeof(struct ieee80211_channel));

    asr_rtos_lock_mutex(&asr_hw->mutex);

    if ((pst_config_info->connect_ap_info.security != WIFI_ENCRYP_OPEN) && (pst_config_info->connect_ap_info.security != WIFI_ENCRYP_WEP))
    {
        //for wpa store info
        memcpy(psk.macaddr, asr_vif->wlan_mac_add.mac_addr, ETH_ALEN);
        psk.ssid_len = pst_config_info->connect_ap_info.ssid_len;
        memcpy(psk.ssid,pst_config_info->connect_ap_info.ssid,pst_config_info->connect_ap_info.ssid_len);
        psk.pwd_len = pst_config_info->connect_ap_info.pwd_len;
        memcpy(psk.password,pst_config_info->connect_ap_info.pwd,pst_config_info->connect_ap_info.pwd_len);
        psk.encrpy_protocol = pst_config_info->connect_ap_info.security;
        psk.cipher_group = cipher_suite_to_wpa_cipher(pst_config_info->cipher_group);
        psk.ciphers_pairwise = cipher_suite_to_wpa_cipher(pst_config_info->cipher_pairwise);
#if (defined(CONFIG_IEEE80211W) || NX_SAE)
        psk.cipher_mgmt_group = pst_config_info->cipher_mgmt_group;
        psk.key_mgmt = pst_config_info->akm_suite;
#endif

        if(pst_config_info->ie_len)
        {
            psk.wpa_ie_len = pst_config_info->ie_len;
            memcpy(psk.wpa_ie, pst_config_info->ie, pst_config_info->ie_len);
        }

        //temp disable
        dbg(D_ERR, D_UWIFI_CTRL, "wpa_connect_init\r\n");
        wpa_connect_init(&psk, WLAN_OPMODE_INFRA);
    }

#if NX_SAE
    asr_vif->sta.rsnxe = pst_config_info->rsnxe;
    dbg(D_INF,D_UWIFI_CTRL,"%s:rsnxe=0x%X\n",__func__, asr_vif->sta.rsnxe);
#endif

    if (0 != wpa_build_connect_sme_adv(asr_vif, pst_config_info, &sme))
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s sme param err",__func__);
        asr_rtos_unlock_mutex(&asr_hw->mutex);
        asr_rtos_free(sme.channel);
        sme.channel = NULL;
        return -1;
    }
    dbg(D_ERR, D_UWIFI_CTRL, "%s+++\n", __func__);
    asr_vif->sta.connInfo.wifiState = WIFI_CONNECTING;
    ret = uwifi_cfg80211_ops.connect(asr_vif, &sme);
    if (ret != 0)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s:connect cmd fail %d",__func__,ret);
        asr_vif->sta.connInfo.wifiState = WIFI_CONNECT_FAILED;
    } else {
        wifi_event_cb(WLAN_EVENT_AUTH, NULL);
    }
    dbg(D_ERR, D_UWIFI_CTRL, "%s---\n", __func__);
    asr_rtos_unlock_mutex(&asr_hw->mutex);

    asr_rtos_free(sme.channel);

    //dbg(D_ERR, D_UWIFI_CTRL, "%s over ret=%d wifi_connect_err=%d scan_done_flag=%d\r\n", __func__,ret,wifi_connect_err,scan_done_flag);

    asr_wlan_set_scan_status(SCAN_CONNECT_ERR);
    sme.channel = NULL;
    return ret;
}

int wpa_disconnect(void)
{
    int ret = 0;
    struct asr_vif *asr_vif;
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    dbg(D_ERR,D_UWIFI_CTRL,"%s <<<(1)",__func__);
    if(NULL == asr_hw)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s:asr_hw is NULL",__func__);
        return -1;
    }
    asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];
    if(NULL == asr_vif)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s:asr_vif is NULL",__func__);
        return -1;
    }

    asr_rtos_lock_mutex(&asr_hw->mutex);

    //other operation
    /*
    clear asr_hw->configInfo
    */
    clear_wifi_config_from_flash();

    if(asr_vif->sta.connInfo.wifiState < WIFI_ASSOCIATED)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s:bypass disconnect when no connection",__func__);
        asr_rtos_unlock_mutex(&asr_hw->mutex);
        return 0;
    }
    dbg(D_ERR,D_UWIFI_CTRL,"%s <<<(2)",__func__);
    asr_vif->sta.connInfo.wifiState = WIFI_DISCONNECTING;
    ret = uwifi_cfg80211_ops.disconnect(asr_vif,WLAN_REASON_DEAUTH_LEAVING);//need check reason
    dbg(D_ERR,D_UWIFI_CTRL,"%s:disconnect over, ret is %d",__func__,ret);
    if(ret != 0)
    {
        asr_vif->sta.connInfo.wifiState = WIFI_DISCONNECT_FAILED;
        dbg(D_ERR,D_UWIFI_CTRL,"%s:disconnect send firmware failed, ret is %d",__func__,ret);
    }
    dbg(D_ERR,D_UWIFI_CTRL,"%s <<<(3)",__func__);
    asr_rtos_unlock_mutex(&asr_hw->mutex);
    return ret;
}
#endif

int wpa_set_power(uint8_t tx_power, uint32_t iftype)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    struct asr_vif *asr_vif;
    if(NULL == asr_hw)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"%s:asr_hw is NULL",__func__);
        return -1;
    }

    if(iftype == NL80211_IFTYPE_STATION)
    {
        asr_vif = asr_hw->vif_table[asr_hw->sta_vif_idx];
    }
    else if(iftype == NL80211_IFTYPE_AP)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }
    else
    {
        dbg(D_ERR, D_UWIFI_CTRL, "iftype error:%d",(int)iftype);
        return -1;
    }

    return uwifi_cfg80211_ops.set_tx_power(asr_vif,(enum nl80211_tx_power_setting)NL80211_TX_POWER_LIMITED,tx_power);
}

