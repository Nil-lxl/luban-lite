/**
 ****************************************************************************************
 *
 * @file uwifi_ap_api.c
 *
 * @brief add function api for ap
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#include "uwifi_common.h"
#include "asr_wlan_api.h"
#include "asr_dbg.h"
#include "hostapd.h"
#include "uwifi_msg_rx.h"

#include "asr_wlan_api.h"
#include "asr_rtos_api.h"
#include "uwifi_wlan_list.h"

#ifdef CFG_ADD_API
uap_config_scan_chan g_scan_results ;
uint8_t g_scan_results_flag;

void Set_Scan_Chan(uap_config_scan_chan _chan)
{
    memcpy(&g_scan_results,&_chan,sizeof(uap_config_scan_chan));
}

uap_config_scan_chan Get_Scan_Chan(void)
{
    return g_scan_results;
}

void Set_Scan_Flag(uint8_t _flag)
{
    g_scan_results_flag = _flag ;
}

uint8_t Get_Scan_Flag(void)
{
    return g_scan_results_flag ;
}

INT32 UAP_Stalist(sta_list *list)
{

    int i = 0,j=0;
    for(i = 0; i < AP_MAX_ASSOC_NUM; i++)
    {
        if(g_ap_user_info.sta_table[i].aid)
        {
            memcpy(list->info[j++].mac_address,g_ap_user_info.sta_table[i].mac_addr, MAC_ADDR_LEN);//g_wpa_sta_info
        }
    }
    list->sta_count = g_ap_user_info.connect_peer_num;

    //TBD
    //power_mfg_status;
    //rssi;

    // for debug test
    dbg(D_ERR, D_UWIFI_CTRL, "%s: sta_count %d\r\n", __func__,list->sta_count);

    for(i = 0; i < list->sta_count; i++)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:[%d] %02x %02x %02x %02x %02x %02x\r\n", __func__,i,list->info[i].mac_address[0],list->info[i].mac_address[1],
        list->info[i].mac_address[2],list->info[i].mac_address[3],list->info[i].mac_address[4],list->info[i].mac_address[5]);
    }

    return 0;
}
// UINT16 pkt_fwd_ctl;
// Bit 0: Packet forwarding handled by Host (0) or Firmware (1)
// Bit 1: Intra-BSS broadcast packets are allowed (0) or denied (1)
// Bit 2: Intra-BSS unicast packets are allowed (0) or denied (1)
// Bit 3: Inter-BSS unicast packets are allowed (0) or denied (1)


INT32 UAP_PktFwd_Ctl(uap_pkt_fwd_ctl *param)
{
    if(GET_ACTION != param->action && SET_ACTION != param->action)
        return -1;

    struct list_mac *cur,*tmp;
    struct asr_vif *asr_vif;

    struct asr_hw *asr_hw = uwifi_get_asr_hw();

    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL\r\n",__func__);
        return -1;
    }

    if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if (NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:no vif!\r\n",__func__);
        return -1;
    }

    if(GET_ACTION == param->action)
    {
        param->pkt_fwd_ctl = (asr_vif->ap.flags>>0)&1;
    }
    else if(SET_ACTION == param->action)
    {
        uint8_t bit0 = (param->pkt_fwd_ctl>>0)&1;
        if ((ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_AP) && ( bit0 > -1))
        {
            if (bit0)
                asr_vif->ap.flags |= ASR_AP_ISOLATE;
            else
                asr_vif->ap.flags &= ~ASR_AP_ISOLATE;
        }
    }
    else
        return -1;
    dbg(D_ERR, D_UWIFI_CTRL, "%s: action=%d,pkt_fwd_ctl=%d,ap.flags=%d \r\n", __func__, param->action,param->pkt_fwd_ctl,asr_vif->ap.flags);
    dbg(D_ERR, D_UWIFI_CTRL, "%s: done \r\n", __func__);
    return 0;
}

INT32 UAP_GetCurrentChannel(void)
{
    struct list_mac *cur,*tmp;
    struct asr_vif *asr_vif;

    struct asr_hw *asr_hw = uwifi_get_asr_hw();

    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL\r\n",__func__);
        return -1;
    }

    if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if (NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:no vif!\r\n",__func__);
        return -1;
    }

    dbg(D_ERR, D_UWIFI_CTRL, "%s: get channel %d \r\n", __func__,asr_vif->ap.chan_num);
    return asr_vif->ap.chan_num;
}

INT32 UAP_Scan_Channels_Config(uap_config_scan_chan *param)
{
    if(GET_ACTION != param->action && SET_ACTION != param->action)
        return -1;

    struct list_mac *cur,*tmp;
    struct asr_vif *asr_vif;

    struct asr_hw *asr_hw = uwifi_get_asr_hw();

    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL\r\n",__func__);
        return -1;
    }

    if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if (NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:no vif!\r\n",__func__);
        return -1;
    }

    if(asr_vif->iftype != NL80211_IFTYPE_AP)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s: must in ap mode!\r\n",__func__);
        return -1;
    }

    if(GET_ACTION == param->action)
    {
        param->band = asr_vif->ap.st_chan.band;
        param->total_chan = asr_vif->ap.chan_num; // in get mode,total_chan is the current channel
    }
    else if(SET_ACTION == param->action)
    {
        Set_Scan_Flag(0);
        asr_wlan_start_scan();// will block untill scan done
        Get_Scan_Flag();
        uap_config_scan_chan _scan_results = Get_Scan_Chan();

        param->band = asr_vif->ap.st_chan.band ;//2.4GHz
        param->total_chan = _scan_results.total_chan;
        int i = 0;
        uint8_t channel_toset,channel_num_min = _scan_results.chan_num[1];// channel is > 0
        dbg(D_ERR, D_UWIFI_CTRL, "%s: total_chan=%d\r\n", __func__,param->total_chan);
        for(i=1;i<MAX_CHANNELS;++i)
        {
            param->chan_num[i] = _scan_results.chan_num[i];
            if(param->chan_num[i])
                dbg(D_ERR, D_UWIFI_CTRL, "%s: [%d] chan_num=%d\r\n", __func__,i,param->chan_num[i]);

            if(channel_num_min >param->chan_num[i])
            {
                channel_num_min = param->chan_num[i];
                channel_toset = i;
            }
        }
        dbg(D_ERR, D_UWIFI_CTRL, "%s:channel is %d\r\n", __func__,channel_toset);

        uwifi_ap_channel_change(channel_toset);

        param->total_chan = channel_toset; //use var total_chan to store channel_toset
    }
    else
        return -1;

    dbg(D_ERR, D_UWIFI_CTRL, "%s: done band %d chan %d \r\n", __func__,param->band ,param->total_chan);
    return 0;
}


INT32 UAP_Sta_Deauth(uap_802_11_mac_addr mac)
{
    struct uwifi_ap_peer_info *cur,*tmp;
    struct asr_vif *asr_vif;
    uint8_t found = 0;

    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL",__func__);
        return -1;
    }

    if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if( NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:no vif!\r\n",__func__);
        return -1;
    }

    if(asr_vif->iftype != NL80211_IFTYPE_AP)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s: must in ap mode!\r\n",__func__);
        return -1;
    }

    list_for_each_entry_safe(cur, tmp, &asr_vif->ap.peer_sta_list, list)
    {
        if (!memcmp(cur->peer_addr, mac, MAC_ADDR_LEN))
        {
            found++ ;
            break;
        }
    }
    if(!found)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:UAP_Death no mac match \r\n", __func__);
        return -1;
    }

    uwifi_hostapd_handle_deauth_msg((uint32_t)cur);
    dbg(D_ERR, D_UWIFI_CTRL, "%s:UAP_Death ", __func__);
    return 0;
}


INT32 UAP_Black_List_Onoff( uint8_t onoff)
{
    struct list_mac *cur,*tmp;
    struct asr_vif *asr_vif;

    struct asr_hw *asr_hw = uwifi_get_asr_hw();

    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL\r\n",__func__);
        return -1;
    }

    if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if (NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:no vif!\r\n",__func__);
        return -1;
    }

    asr_vif->ap.black_state = onoff;
    dbg(D_CRT,D_UWIFI_CTRL,"%s: onoff=%d \r\n",__func__,asr_vif->ap.black_state);
    return 0;

}

 INT32 UAP_Black_List_Get(blacklist *black_l)
 {
    struct list_mac *cur,*tmp;
    struct asr_vif *asr_vif;
    uint8_t found = 0,i = 0 ;

    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL\r\n",__func__);
        return -1;
    }

    if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if (NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:no vif!\r\n",__func__);
        return -1;
    }

    list_for_each_entry_safe(cur, tmp, &asr_vif->ap.peer_black_list, list)
    {
        if(cur)
        {
            memcpy(black_l->peer_addr[found],cur->peer_addr,MAC_ADDR_LEN);

            found ++;
        }
    }
    black_l->onoff = asr_vif->ap.black_state;
    black_l->count = found;
    dbg(D_CRT,D_UWIFI_CTRL,"%s:the list len is %d  onoff=%d \r\n",__func__,black_l->count,black_l->onoff );

    if(!found)
    {
        dbg(D_CRT, D_UWIFI_CTRL, "%s:black list is not exist \r\n",__func__);
        return -1;
    }

    for(i=0;i<black_l->count;++i)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:the list[%d] mac %02x:%02x:%02x:%02x:%02x:%02x \r\n",__func__,i,
        black_l->peer_addr[i][0],black_l->peer_addr[i][1],black_l->peer_addr[i][2],
        black_l->peer_addr[i][3],black_l->peer_addr[i][4],black_l->peer_addr[i][5]);
    }

    return 0;

 }

INT32 UAP_Black_List_Add( uap_802_11_mac_addr blackmac)
{
    struct list_mac *cur,*tmp;
    struct asr_vif *asr_vif;
    uint8_t found = 0,list_len=0;

    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL\r\n",__func__);
        return -1;
    }

    if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if (NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:no vif!\r\n",__func__);
        return -1;
    }

    list_for_each_entry_safe(cur, tmp, &asr_vif->ap.peer_black_list, list)
    {
        if(cur)
        {
            list_len++;
            if (!memcmp(cur->peer_addr, blackmac, MAC_ADDR_LEN))
            {
                found ++;
                break;
            }
        }
    }
    if(found)
    {
        dbg(D_CRT, D_UWIFI_CTRL, "%s:black list have added the mac %02x:%02x:%02x:%02x:%02x:%02x\r\n",__func__,
        blackmac[0],blackmac[1],blackmac[2],blackmac[3],blackmac[4],blackmac[5]);
        return -1;
    }
    dbg(D_CRT, D_UWIFI_CTRL, "%s: count=%d\r\n",__func__,found);
    list_len = list_len%AP_MAX_BLACK_NUM;
    memcpy(asr_vif->ap.peer_black[list_len].peer_addr, blackmac, MAC_ADDR_LEN);
    list_add_tail(&(asr_vif->ap.peer_black[list_len].list), &asr_vif->ap.peer_black_list);

    return 0;

}

INT32 UAP_Black_List_Del( uap_802_11_mac_addr blackmac)
{
    struct list_mac *cur,*tmp;
    struct asr_vif *asr_vif;
    uint8_t found = 0;

    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL\r\n",__func__);
        return -1;
    }

    if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if (NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:no vif!\r\n",__func__);
        return -1;
    }

    list_for_each_entry_safe(cur, tmp, &asr_vif->ap.peer_black_list, list)
    {
        if (!memcmp(cur->peer_addr, blackmac, MAC_ADDR_LEN))
        {
            found ++;
            break;
        }
    }
    if(!found)
    {
        dbg(D_CRT, D_UWIFI_CTRL, "%s:black list the mac not exist %02x:%02x:%02x:%02x:%02x:%02x\r\n",__func__,
        blackmac[0],blackmac[1],blackmac[2],blackmac[3],blackmac[4],blackmac[5]);
        return -1;
    }
    dbg(D_CRT,D_UWIFI_CTRL,"%s: found=%d\r\n",__func__,found);

    list_del(&(asr_vif->ap.peer_black[found-1].list));
    memset(&(asr_vif->ap.peer_black[found-1]), 0, sizeof(struct list_mac));

    return 0;
}

INT32 UAP_Black_List_Clear(void)
{

    struct list_mac *cur,*tmp;
    struct asr_vif *asr_vif;
    uint8_t found = 0;

    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL\r\n",__func__);
        return -1;
    }

    if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if (NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:no vif!\r\n",__func__);
        return -1;
    }

    dbg(D_CRT,D_UWIFI_CTRL,"%s: found=%d\r\n",__func__,found);

    if(!list_empty(&asr_vif->ap.peer_black_list))
    {
        list_del_init(&asr_vif->ap.peer_black_list);
    }

    return 0;

}


INT32 UAP_White_List_Onoff(uint8_t onoff)
{
    struct list_mac *cur,*tmp;
    struct asr_vif *asr_vif;

    struct asr_hw *asr_hw = uwifi_get_asr_hw();

    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL\r\n",__func__);
        return -1;
    }

    if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if (NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:no vif!\r\n",__func__);
        return -1;
    }

    asr_vif->ap.white_state = onoff;
    dbg(D_CRT,D_UWIFI_CTRL,"%s: white_state=%d \r\n",__func__,asr_vif->ap.white_state);

    return 0;
}

 INT32 UAP_White_List_Get(whitelist *white_l)
 {
    struct list_mac *cur,*tmp;
    struct asr_vif *asr_vif;
    uint8_t found = 0,i = 0 ;

    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL\r\n",__func__);
        return -1;
    }

    if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if (NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:no vif!\r\n",__func__);
        return -1;
    }

    list_for_each_entry_safe(cur, tmp, &asr_vif->ap.peer_white_list, list)
    {
        if(cur)
        {
            memcpy(white_l->peer_addr[found],cur->peer_addr,MAC_ADDR_LEN);

            found ++;
        }
    }
    white_l->onoff = asr_vif->ap.white_state ;
    white_l->count = found;
    dbg(D_CRT,D_UWIFI_CTRL,"%s:the list len is %d onoff=%d \r\n",__func__,white_l->count,white_l->onoff );

    if(!found)
    {
        dbg(D_CRT, D_UWIFI_CTRL, "%s:white list is not exist \r\n",__func__);
        return -1;
    }

    for(i=0;i<white_l->count;++i)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:the list[%d] mac %02x:%02x:%02x:%02x:%02x:%02x \r\n",__func__,i,
        white_l->peer_addr[i][0],white_l->peer_addr[i][1],white_l->peer_addr[i][2],
        white_l->peer_addr[i][3],white_l->peer_addr[i][4],white_l->peer_addr[i][5]);
    }

    return 0;

 }

INT32 UAP_White_List_Add( uap_802_11_mac_addr whitemac)
{
    struct list_mac *cur,*tmp;
    struct asr_vif *asr_vif;
    uint8_t found = 0,list_len=0;

    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL\r\n",__func__);
        return -1;
    }

    if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if (NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:no vif!\r\n",__func__);
        return -1;
    }

    list_for_each_entry_safe(cur, tmp, &asr_vif->ap.peer_white_list, list)
    {
        if(cur)
        {
            list_len++;
            if (!memcmp(cur->peer_addr, whitemac, MAC_ADDR_LEN))
            {
                found ++;
                break;
            }
        }
    }
    if(found)
    {
        dbg(D_CRT, D_UWIFI_CTRL, "%s:black list have added the mac %02x:%02x:%02x:%02x:%02x:%02x\r\n",__func__,
        whitemac[0],whitemac[1],whitemac[2],whitemac[3],whitemac[4],whitemac[5]);
        return -1;
    }
    dbg(D_CRT, D_UWIFI_CTRL, "%s: count=%d\r\n",__func__,found);
    list_len = list_len%AP_MAX_BLACK_NUM;
    memcpy(asr_vif->ap.peer_white[list_len].peer_addr, whitemac, MAC_ADDR_LEN);
    list_add_tail(&(asr_vif->ap.peer_white[list_len].list), &asr_vif->ap.peer_white_list);

    return 0;

}

INT32 UAP_White_List_Del( uap_802_11_mac_addr whitemac)
{
    struct list_mac *cur,*tmp;
    struct asr_vif *asr_vif;
    uint8_t found = 0;

    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL\r\n",__func__);
        return -1;
    }

    if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if (NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:no vif!\r\n",__func__);
        return -1;
    }

    list_for_each_entry_safe(cur, tmp, &asr_vif->ap.peer_white_list, list)
    {
        if (!memcmp(cur->peer_addr, whitemac, MAC_ADDR_LEN))
        {
            found ++;
            break;
        }
    }
    if(!found)
    {
        dbg(D_CRT, D_UWIFI_CTRL, "%s:white list the mac not exist %02x:%02x:%02x:%02x:%02x:%02x\r\n",__func__,
        whitemac[0],whitemac[1],whitemac[2],whitemac[3],whitemac[4],whitemac[5]);
        return -1;
    }
    dbg(D_CRT,D_UWIFI_CTRL,"%s: found=%d\r\n",__func__,found);

    list_del(&(asr_vif->ap.peer_white[found-1].list));
    memset(&(asr_vif->ap.peer_white[found-1]), 0, sizeof(struct list_mac));

    return 0;
}

INT32 UAP_White_List_Clear(void)
{

    struct list_mac *cur,*tmp;
    struct asr_vif *asr_vif;
    uint8_t found = 0;

    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    if(NULL == asr_hw)
    {
        dbg(D_CRT,D_UWIFI_CTRL,"%s:asr_hw is NULL\r\n",__func__);
        return -1;
    }

    if(asr_hw->ap_vif_idx  != 0xff)
    {
        asr_vif = asr_hw->vif_table[asr_hw->ap_vif_idx];
    }

    if (NULL == asr_vif)
    {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:no vif!\r\n",__func__);
        return -1;
    }

    dbg(D_CRT,D_UWIFI_CTRL,"%s: found=%d\r\n",__func__,found);

    if(!list_empty(&asr_vif->ap.peer_white_list))
    {
        list_del_init(&asr_vif->ap.peer_white_list);
    }

    return 0;
}


INT32 UAP_Params_Config(uap_config_param *  uap_param)
{
    if(GET_ACTION == uap_param->action)
    {
        whitelist white_l;
        blacklist black_l;

        UAP_White_List_Get(&white_l);
        if(!white_l.onoff)
        {// blacklist mode
            UAP_Black_List_Get(&black_l);
            if(!black_l.onoff)
            {
                uap_param->filter.filter_mode = DISABLE_FILTER;
                // if disable then get nothing
                return -1;
            }
            uap_param->filter.filter_mode = BLACK_FILTER;
            uap_param->filter.mac_count = black_l.count;
            int i = 0 ;
            for(i=0;i<black_l.count;++i)
            {
                memcpy( uap_param->filter.mac_list[i],black_l.peer_addr,MAC_ADDR_LEN);
            }
        }
        uap_param->filter.filter_mode = WHITE_FILTER;
        uap_param->filter.mac_count = white_l.count;
        int i = 0 ;
        for(i=0;i<white_l.count;++i)
        {
            memcpy( uap_param->filter.mac_list[i],white_l.peer_addr,MAC_ADDR_LEN);
        }

    }
    else if(SET_ACTION == uap_param->action)
    {
        if(DISABLE_FILTER == uap_param->filter.filter_mode)
        {
            UAP_Black_List_Onoff(DOFF);
            UAP_White_List_Onoff(DOFF);

            //if disable then set nothing
            return -1;
        }
        else if(WHITE_FILTER == uap_param->filter.filter_mode)
        {
            int list_len = uap_param->filter.mac_count;
            if(AP_MAX_WHITE_NUM < list_len)
            {
                //max list is 16
                return -1;
            }
            UAP_Black_List_Onoff(DOFF);
            UAP_White_List_Onoff(DON);

            UAP_White_List_Clear();
            int i = 0 ;
            for(i=0;i<list_len;++i)
            {
                UAP_White_List_Add( uap_param->filter.mac_list[i]);
            }

        }
        else if(BLACK_FILTER == uap_param->filter.filter_mode)
        {
            int list_len = uap_param->filter.mac_count;
            if(AP_MAX_BLACK_NUM < list_len)
            {
                //max list is 16
                return -1;
            }

            UAP_Black_List_Onoff(DON);
            UAP_White_List_Onoff(DOFF);

            UAP_Black_List_Clear();
            int i = 0 ;
            for(i=0;i<list_len;++i)
            {
                UAP_Black_List_Add( uap_param->filter.mac_list[i]);
            }
        }
    }
    else
        return -1;

    return 0;
}

INT32 UAP_BSS_Config(INT32 start_stop)
{
    if(DON == start_stop)
    {//start :normal mode
        asr_send_set_ps_mode(PS_MODE_OFF);
    }
    else if(DOFF == start_stop)
    {//stop :in save mode but not poweroff
        asr_send_set_ps_mode(PS_MODE_ON_DYN);
    }
    else
    {
        return -1;
    }

    dbg(D_ERR, D_UWIFI_CTRL, "%s: done \r\n", __func__);
    return 0;

}
#endif
