/**
 ******************************************************************************
 *
 * @file uwifi_ops_adapter.c
 *
 * @brief cfg80211_ops,net_device_ops related implement
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */

#include "uwifi_include.h"
#include "uwifi_tx.h"
#include "uwifi_ops_adapter.h"
#include "uwifi_msg_tx.h"
#include "hostapd.h"
#include "uwifi_platform.h"
#include "uwifi_rx.h"
#include "uwifi_notify.h"

#include "ethernetif_wifi.h"
//#include "dhcpd.h"
#ifdef LWIP
#include "netif.h"
//#include "lwip_comm_wifi.h"
#endif
#include "wpa_adapter.h"

extern struct ethernetif g_ethernetif[COEX_MODE_MAX];
//extern uint8_t wifi_connect_err;
extern uint8_t wifi_conect_fail_code;

#if 0
static uint32_t cipher_suites[] = {
    WLAN_CIPHER_SUITE_WEP40,
    WLAN_CIPHER_SUITE_WEP104,
    WLAN_CIPHER_SUITE_TKIP,
    WLAN_CIPHER_SUITE_CCMP,
    0, // reserved entries to enable AES-CMAC and/or SMS4
    0,
};
#endif

static const int asr_ac2hwq[1][NL80211_NUM_ACS] = {
    {
        [NL80211_TXQ_Q_VO] = ASR_HWQ_VO,
        [NL80211_TXQ_Q_VI] = ASR_HWQ_VI,
        [NL80211_TXQ_Q_BE] = ASR_HWQ_BE,
        [NL80211_TXQ_Q_BK] = ASR_HWQ_BK
    }
};

const int asr_tid2hwq[IEEE80211_NUM_TIDS] = {
    ASR_HWQ_BE,
    ASR_HWQ_BK,
    ASR_HWQ_BK,
    ASR_HWQ_BE,
    ASR_HWQ_VI,
    ASR_HWQ_VI,
    ASR_HWQ_VO,
    ASR_HWQ_VO,
    /* TID_8 is used for management frames */
    ASR_HWQ_VO,
    /* At the moment, all others TID are mapped to BE */
    ASR_HWQ_BE,
    ASR_HWQ_BE,
    ASR_HWQ_BE,
    ASR_HWQ_BE,
    ASR_HWQ_BE,
    ASR_HWQ_BE,
    ASR_HWQ_BE,
};

static const int asr_hwq2uapsd[NL80211_NUM_ACS] = {
    [ASR_HWQ_VO] = IEEE80211_WMM_IE_STA_QOSINFO_AC_VO,
    [ASR_HWQ_VI] = IEEE80211_WMM_IE_STA_QOSINFO_AC_VI,
    [ASR_HWQ_BE] = IEEE80211_WMM_IE_STA_QOSINFO_AC_BE,
    [ASR_HWQ_BK] = IEEE80211_WMM_IE_STA_QOSINFO_AC_BK,
};

struct asr_vif g_asr_vif[COEX_MODE_MAX];


/*********************************************************************
 * helper
 *********************************************************************/

struct asr_sta *asr_get_sta(struct asr_hw *asr_hw, const uint8_t *mac_addr)
{
    int i;

    for (i = 0; i < asr_hw->sta_max_num; i++) {
        struct asr_sta *sta = &asr_hw->sta_table[i];
        if (sta->valid && (memcmp(mac_addr, sta->mac_addr, 6) == 0))
            return sta;
    }

    return NULL;
}

uint8_t *asr_build_bcn(struct asr_bcn *bcn, struct cfg80211_beacon_data *new)
{
    uint8_t *buf, *pos;

    if (new->head) {
        uint8_t *head = asr_rtos_malloc(new->head_len);

        if (!head)
            return NULL;

        if (bcn->head)
        {
            asr_rtos_free(bcn->head);
            bcn->head = NULL;
        }

        bcn->head = head;
        bcn->head_len = new->head_len;
        memcpy(bcn->head, new->head, new->head_len);
    }
    if (new->tail) {
        uint8_t *tail = asr_rtos_malloc(new->tail_len);

        if (!tail)
            return NULL;

        if (bcn->tail)
        {
            asr_rtos_free(bcn->tail);
            bcn->tail = NULL;
        }

        bcn->tail = tail;
        bcn->tail_len = new->tail_len;
        memcpy(bcn->tail, new->tail, new->tail_len);
    }

    if (!bcn->head)
        return NULL;

    bcn->tim_len = 6;
    bcn->len = bcn->head_len + bcn->tail_len + bcn->ies_len + bcn->tim_len;
    buf = asr_rtos_malloc(bcn->len);
    if (!buf)
        return NULL;

    // Build the beacon buffer
    pos = buf;
    memcpy(pos, bcn->head, bcn->head_len);
    pos += bcn->head_len;
    *pos++ = WLAN_EID_TIM;
    *pos++ = 4;
    *pos++ = 0;
    *pos++ = bcn->dtim;
    *pos++ = 0;
    *pos++ = 0;
    if (bcn->tail) {
        memcpy(pos, bcn->tail, bcn->tail_len);
        pos += bcn->tail_len;
    }
    if (bcn->ies) {
        memcpy(pos, bcn->ies, bcn->ies_len);
    }

    return buf;
}


static void asr_del_bcn(struct asr_bcn *bcn)
{
    if (bcn->head) {
        asr_rtos_free(bcn->head);
        bcn->head = NULL;
    }
    bcn->head_len = 0;

    if (bcn->tail) {
        asr_rtos_free(bcn->tail);
        bcn->tail = NULL;
    }
    bcn->tail_len = 0;

    if (bcn->ies) {
        asr_rtos_free(bcn->ies);
        bcn->ies = NULL;
    }
    bcn->ies_len = 0;
    bcn->tim_len = 0;
    bcn->dtim = 0;
    bcn->len = 0;
}

/**
 * Link channel ctxt to a vif and thus increments count for this context.
 */
void asr_chanctx_link(struct asr_vif *vif, uint8_t ch_idx, struct cfg80211_chan_def *chandef)
{
    struct asr_chanctx *ctxt;

    if (ch_idx >= NX_CHAN_CTXT_CNT) {
        dbg(D_ERR,D_UWIFI_CTRL,"Invalid channel ctxt id %d\n", ch_idx);
        return;
    }

    vif->ch_index = ch_idx;
    ctxt = &vif->asr_hw->chanctx_table[ch_idx];
    ctxt->count++;

    // For now chandef is NULL for STATION interface
    if (chandef) {
        if (!ctxt->chan_def.chan)
            ctxt->chan_def = *chandef;
        else {
            // TODO. check that chandef is the same as the one already
            // set for this ctxt
        }
    }
}

/**
 * Unlink channel ctxt from a vif and thus decrements count for this context
 */
void asr_chanctx_unlink(struct asr_vif *vif)
{
    struct asr_chanctx *ctxt;

    if (vif->ch_index == ASR_CH_NOT_SET)
        return;

    ctxt = &vif->asr_hw->chanctx_table[vif->ch_index];

    if (ctxt->count == 0) {
        dbg(D_ERR,D_UWIFI_CTRL,"Chan ctxt ref count is already 0");
    } else {
        ctxt->count--;
    }

    if (ctxt->count == 0) {
        if (vif->ch_index == vif->asr_hw->cur_chanctx) {
            /* If current chan ctxt is no longer linked to a vif
               disable radar detection (no need to check if it was activated) */
        }
        /* set chan to null, so that if this ctxt is relinked to a vif that
           don't have channel information, don't use wrong information */
        ctxt->chan_def.chan = NULL;
    }
    vif->ch_index = ASR_CH_NOT_SET;
}

int asr_chanctx_valid(struct asr_hw *asr_hw, uint8_t ch_idx)
{
    if (ch_idx >= NX_CHAN_CTXT_CNT ||
        asr_hw->chanctx_table[ch_idx].chan_def.chan == NULL) {
        return 0;
    }

    return 1;
}

static void asr_del_csa(struct asr_vif *vif)
{
    struct asr_csa *csa = vif->ap.csa;

    if (!csa)
        return;

    if (csa->dma.buf) {
        asr_rtos_free(csa->dma.buf);
        csa->dma.buf = NULL;
    }
    asr_del_bcn(&csa->bcn);
    asr_rtos_free(csa);
    csa = NULL;
    vif->ap.csa = NULL;
}

/*********************************************************************
 * netdev callbacks
 ********************************************************************/
/**
 * int (*ndo_open)(struct net_device *dev);
 *     This function is called when network device transistions to the up
 *     state.
 *
 * - Start FW if this is the first interface opened
 * - Add interface at fw level
 */
static int asr_open(struct asr_hw *asr_hw, struct asr_vif *asr_vif)
{
    struct mm_add_if_cfm add_if_cfm;
    int error = 0;

    // Check if it is the first opened VIF
    if (asr_hw->vif_started == 0){
        /*Device is now started*/
        asr_set_bit(ASR_DEV_STARTED,&asr_hw->phy_flags);
    }

    /* Forward the information to the LMAC,
     *     p2p value not used in FMAC configuration, iftype is sufficient */
    if ((error = asr_send_add_if(asr_hw, asr_vif->wlan_mac_add.mac_addr,
                                  ASR_VIF_TYPE(asr_vif), false, &add_if_cfm)))
        return error;
    if (add_if_cfm.status != 0) {
        return -EIO;
    }

    /* Save the index retrieved from LMAC */
    asr_vif->vif_index = add_if_cfm.inst_nbr;
    asr_vif->up = true;
    asr_hw->vif_started++;
    asr_hw->vif_table[add_if_cfm.inst_nbr] = asr_vif;

#ifdef CFG_STATION_SUPPORT
    if(asr_vif->iftype == NL80211_IFTYPE_STATION) //sta
    {
        asr_hw->sta_vif_idx = asr_vif->vif_index;
    } else
#endif
#ifdef CFG_SOFTAP_SUPPORT
    if(asr_vif->iftype == NL80211_IFTYPE_AP) //ap
    {
        asr_hw->ap_vif_idx = asr_vif->vif_index;
    } else
#endif
#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
    if(asr_vif->iftype == NL80211_IFTYPE_MONITOR)
    {
        asr_hw->monitor_vif_idx = asr_vif->vif_index;
    } else
#endif
    {
        dbg(D_ERR, D_UWIFI_CTRL, "error if type:%d", (int)asr_vif->iftype);
        error = -1;
    }

    dbg(D_INF,D_UWIFI_CTRL,"iftype=%d,vif_index=%d,inst_nbr=%d\n",
        asr_vif->iftype,asr_vif->vif_index,add_if_cfm.inst_nbr);

    return error;
}

/**
 * int (*ndo_stop)(struct net_device *dev);
 *     This function is called when network device transistions to the down
 *     state.
 *
 * - Remove interface at fw level
 * - Reset FW if this is the last interface opened
 */
static int asr_close(struct asr_hw *asr_hw, struct asr_vif *asr_vif)
{
    dbg(D_DBG,D_UWIFI_CTRL,"%s: asr_send_remove_if before iftype=%d,vif_index=%d vif_started=%d\r\n",__func__,asr_vif->iftype,asr_vif->vif_index,asr_hw->vif_started);
    asr_send_remove_if(asr_hw, asr_vif->vif_index);
    dbg(D_DBG,D_UWIFI_CTRL,"%s: asr_send_remove_if after iftype=%d,vif_index=%d vif_started=%d\r\n",__func__,asr_vif->iftype,asr_vif->vif_index,asr_hw->vif_started);

    asr_hw->vif_table[asr_vif->vif_index] = NULL;

    asr_hw->avail_idx_map |= BIT(asr_vif->drv_vif_index);

    asr_vif->up = false;


#ifdef CFG_STATION_SUPPORT
    if(asr_vif->iftype == NL80211_IFTYPE_STATION)
    {
        //FIXME
        //wifi_conect_fail_code = WLAN_STA_MODE_USER_CLOSE;
        //wifi_connect_err = 0;
        //g_user_close_wifi = 0;
        //wifi_event_cb(WLAN_EVENT_DISCONNECTED, NULL);
        //netif_remove(&lwip_netif[COEX_MODE_STA]);
        asr_hw->sta_vif_idx = 0xFF;
    } else
#endif
#ifdef CFG_SOFTAP_SUPPORT
    if(asr_vif->iftype == NL80211_IFTYPE_AP)
    {
        #ifdef CFG_ENCRYPT_SUPPORT
        wpas_priv_free(WLAN_OPMODE_MASTER);
        #endif
        //dhcpd_stop();
        //netif_remove(&lwip_netif[COEX_MODE_AP]);
        asr_hw->ap_vif_idx = 0xFF;
    } else
#endif
    {}

    list_del(&asr_vif->list);

    #ifdef LWIP
    //lwip_ifconfig_value(lwip_get_netif_wifi_ifname(),"ifstatus",LWIP_NETIF_IF_STATUS_DISABLE);
    #else
    // netif already down in DHCP_CLIENT_DOWN.
    #endif

    dbg(D_INF,D_UWIFI_CTRL,"%s: lwip_ifconfig_value after asr_vif->iftype=%d,asr_vif->vif_index=%d asr_hw->vif_started=%d \r\n",__func__,
                                                                                asr_vif->iftype,asr_vif->vif_index,asr_hw->vif_started);
    asr_vif = NULL;

    asr_hw->vif_started--;

    if (asr_hw->vif_started == 0) {

        dbg(D_ERR,D_UWIFI_CTRL,"vif_started become 0");
        /* This also lets both ipc sides remain in sync before resetting */
        //asr_ipc_tx_drain(asr_hw);

        asr_send_reset(asr_hw);

        // Set parameters to firmware
        asr_send_me_config_req(asr_hw);

        // Set channel parameters to firmware
        asr_send_me_chan_config_req(asr_hw);
        // Start the FW
        asr_send_start(asr_hw);
        /* Device not exist */
        asr_clear_bit(ASR_DEV_STARTED, &asr_hw->phy_flags);
    }

    return 0;
}

/**
 * struct net_device_stats* (*ndo_get_stats)(struct net_device *dev);
 *    Called when a user wants to get the network device usage
 *    statistics. Drivers must do one of the following:
 *    1. Define @ndo_get_stats64 to fill in a zero-initialised
 *       rtnl_link_stats64 structure passed by the caller.
 *    2. Define @ndo_get_stats to update a net_device_stats structure
 *       (which should normally be dev->stats) and return a pointer to
 *       it. The structure may be changed asynchronously only if each
 *       field is written atomically.
 *    3. Update dev->stats asynchronously and atomically, and define
 *       neither operation.
 */
//static struct net_device_stats *asr_get_stats(struct asr_vif *asr_vif)
//{
//    return &asr_vif->net_stats;
//}

struct net_device_ops uwifi_netdev_ops = {
    .ndo_open               = asr_open,
    .ndo_stop               = asr_close,
    .ndo_start_xmit         = asr_start_xmit,
    //.ndo_get_stats          = asr_get_stats,
    .ndo_select_queue       = asr_select_queue,
};
/*********************************************************************
 * Cfg80211 callbacks (and helper)
 *********************************************************************/
#ifndef ETHARP_HWADDR_LEN
#define ETHARP_HWADDR_LEN 6
#endif
int asr_interface_add(struct asr_hw *asr_hw, enum nl80211_iftype type, struct vif_params *params,
                                                                                             struct asr_vif **asr_vif)
{
    int min_idx, max_idx;
    int vif_idx = -1;
    int i;
    struct asr_vif *vif;
    struct ethernetif *ethernet_if;
    //struct netif *p_netif;
    //char mac_addr[ETHARP_HWADDR_LEN*3 - 1] = {0};
    char mac_addr[ETHARP_HWADDR_LEN*3] = {0};
    uint8_t perm_addr[ETH_ALEN] = {0};

    min_idx = 0;
    max_idx = NX_VIRT_DEV_MAX-1;

    for (i = min_idx; i <= max_idx; i++)
    {
        if ((asr_hw->avail_idx_map) & BIT(i))
        {
            vif_idx = i;
            break;
        }
    }
    if (vif_idx < 0)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"invalid vif_idx");
        return -1;
    }

    if(type == NL80211_IFTYPE_AP)
    {
        vif = &g_asr_vif[COEX_MODE_AP];
    }
    else //STA MONITOR IDLE
    {
        vif = &g_asr_vif[COEX_MODE_STA];
    }
    memset(vif, 0, sizeof(struct asr_vif));

    vif->drv_vif_index = vif_idx;
    vif->iftype = type;
    vif->up = false;
    vif->ch_index = ASR_CH_NOT_SET;
    //memset(&vif->net_stats, 0, sizeof(vif->net_stats));
    vif->asr_hw = asr_hw;
    asr_hw->iftype = type;
    // mrole add for tx flow ctrl.
    vif->txring_bytes = 0;
    vif->tx_skb_cnt = 0;

    //set vif mac address
    //different interface,different mac address
    memcpy(perm_addr, asr_hw->mac_addr, ETH_ALEN);
    perm_addr[5] ^= vif_idx;
    memcpy(vif->wlan_mac_add.mac_addr, perm_addr, ETH_ALEN);

    //show current mac address we used
    dbg(D_ERR,D_UWIFI_CTRL,"mac address is %02X:%02X:%02X:%02X:%02X:%02X (%d , %d)",perm_addr[0],perm_addr[1],perm_addr[2],perm_addr[3],perm_addr[4],perm_addr[5],
                                                                      asr_hw->sta_vif_idx,asr_hw->ap_vif_idx);


    if((type == NL80211_IFTYPE_STATION) || (type == NL80211_IFTYPE_AP))
    {
        if(type == NL80211_IFTYPE_STATION)
        {
            ethernet_if = &g_ethernetif[COEX_MODE_STA];
        }
        else
        {
            ethernet_if = &g_ethernetif[COEX_MODE_AP];
        }
        //ethernet_if = (struct ethernetif *)p_netif->state;

        //lwip structure init   COEX_TODO
        //ethernet_if->tcpip_wifi_xmit = uwifi_xmit_entry;
        ethernet_if->ethaddr = &(vif->wlan_mac_add);
        //FIXME or get netif->hwaddr_len by asr3601 interface;
        //get netif->hwaddr by asr3601 interface
        sprintf(mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X", vif->wlan_mac_add.mac_addr[0], vif->wlan_mac_add.mac_addr[1], vif->wlan_mac_add.mac_addr[2],
                                                            vif->wlan_mac_add.mac_addr[3],vif->wlan_mac_add.mac_addr[4], vif->wlan_mac_add.mac_addr[5]);
        #if defined LWIP && !defined JXC_SDK
        lwip_ifconfig(lwip_get_netif_wifi_ifname(), "mac", mac_addr);
        #else

        if (type == NL80211_IFTYPE_STATION)
            asr_set_netif_mac_addr(mac_addr,STA);
        else if (type == NL80211_IFTYPE_AP)
            asr_set_netif_mac_addr(mac_addr,SOFTAP);
        #endif

        //lwip_ifconfig(lwip_get_netif_wifi_ifname(), "mac", "50:9A:1C:4B:32:F9");
        //memcpy(p_netif->hwaddr, vif->wlan_mac_add.mac_addr, ETHARP_HWADDR_LEN);
        vif->net_if = (void *)ethernet_if;
    }

    switch (type) {
    case NL80211_IFTYPE_STATION:
        vif->sta.ap = NULL;
        uwifi_get_saved_config(&(vif->sta.configInfo));
        break;
    case NL80211_IFTYPE_AP:
        INIT_LIST_HEAD(&vif->ap.sta_list);
        memset(&vif->ap.bcn, 0, sizeof(vif->ap.bcn));
        break;
#ifdef CFG_SNIFFER_SUPPORT
    case NL80211_IFTYPE_MONITOR:
        vif->sniffer.rx_filter          = 0;
        vif->sniffer.chan_num            = 0;
        break;
#endif
    default:
        break;
    }

    if (params) {
        vif->use_4addr = params->use_4addr;
    } else
        vif->use_4addr = false;

    list_add_tail(&vif->list, &asr_hw->vifs);
    asr_hw->avail_idx_map &= ~BIT(vif_idx);
    *asr_vif = vif;

    asr_set_bit(ASR_DEV_INITED, &asr_hw->phy_flags);
    return 0;
}


/*
 * @brief Retrieve the asr_sta object allocated for a given MAC address
 * and a given role.
 */
static struct asr_sta *asr_retrieve_sta(struct asr_hw *asr_hw,
                                          struct asr_vif *asr_vif, uint8_t *addr,
                                          uint16_t fc, bool ap)
{
    if (ap) {
        /* only deauth, disassoc and action are bufferable MMPDUs */
        bool bufferable = ieee80211_is_deauth(fc) ||
                          ieee80211_is_disassoc(fc) ||
                          ieee80211_is_action(fc);

        /* Check if the packet is bufferable or not */
        if (bufferable)
        {
            /* Check if address is a broadcast or a multicast address */
            if (is_broadcast_ether_addr(addr) || is_multicast_ether_addr(addr)) {
                /* Returned STA pointer */
                struct asr_sta *asr_sta = &asr_hw->sta_table[asr_vif->ap.bcmc_index];

                if (asr_sta->valid)
                    return asr_sta;
            } else {
                /* Returned STA pointer */
                struct asr_sta *asr_sta;

                /* Go through list of STAs linked with the provided VIF */
                list_for_each_entry(asr_sta, &asr_vif->ap.sta_list, list) {
                    if (asr_sta->valid &&
                        ether_addr_equal(asr_sta->mac_addr, addr)) {
                        /* Return the found STA */
                        return asr_sta;
                    }
                }
            }
        }
    } else {
        return asr_vif->sta.ap;
    }

    return NULL;
}

/**
 * @add_virtual_intf: create a new virtual interface with the given name,
 *    must set the struct wireless_dev's iftype. Beware: You must create
 *    the new netdev in the wiphy's network namespace! Returns the struct
 *    wireless_dev, or an ERR_PTR. For P2P device wdevs, the driver must
 *    also set the address member in the wdev.
 */
static int asr_cfg80211_add_iface(struct asr_hw *asr_hw, enum nl80211_iftype type,
                                                                  struct vif_params *params, struct asr_vif **asr_vif)
{
    return asr_interface_add(asr_hw, type, params, asr_vif);
}

/**
 * @del_virtual_intf: remove the virtual interface
 */
static int asr_cfg80211_del_iface(struct asr_vif *asr_vif)
{
    list_del(&asr_vif->list);
    return 0;
}

/**
 * @change_virtual_intf: change type/configuration of virtual interface,
 *    keep the struct wireless_dev's iftype updated.
 */
static int asr_cfg80211_change_iface(struct asr_vif *asr_vif,
                   enum nl80211_iftype type, struct vif_params *params)
{
    if (asr_vif->up)
        return (-EBUSY);

    switch (type) {
    case NL80211_IFTYPE_STATION:
        asr_vif->sta.ap = NULL;
        break;
    case NL80211_IFTYPE_AP:
        INIT_LIST_HEAD(&asr_vif->ap.sta_list);
        memset(&asr_vif->ap.bcn, 0, sizeof(asr_vif->ap.bcn));
        break;
    default:
        break;
    }

    if (params->use_4addr != -1)
        asr_vif->use_4addr = params->use_4addr;

    return 0;
}

/**
 * @scan: Request to do a scan. If returning zero, the scan request is given
 *    the driver, and will be valid until passed to cfg80211_scan_done().
 *    For scan results, call cfg80211_inform_bss(); you can call this outside
 *    the scan/scan_done bracket too.
 */
static int asr_cfg80211_scan(struct asr_vif *asr_vif,
                                        struct cfg80211_scan_request *request)
{
    int error;
    asr_vif->asr_hw->scan_request = request;

    if ((error = asr_send_scanu_req(asr_vif, request)))
    {
        asr_vif->asr_hw->scan_request = NULL;
        return error;
    }

    return 0;
}

/**
 * @add_key: add a key with the given parameters. @mac_addr will be %NULL
 *    when adding a group key.
 */
static int asr_cfg80211_add_key(struct asr_vif *asr_vif,
                                 uint8_t key_index, bool pairwise, const uint8_t *mac_addr,
                                 struct key_params *params)
{
    int i, error = 0;
    struct mm_key_add_cfm key_add_cfm;
    uint8_t cipher = 0;
    struct asr_sta *sta = NULL;
    struct asr_key *asr_key;
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    if (mac_addr) {
        sta = asr_get_sta(asr_hw, mac_addr);
        if (!sta)
            return -EINVAL;
        asr_key = &sta->key;
    }
    else
        asr_key = &asr_vif->key[key_index];

    /* Retrieve the cipher suite selector */
    switch (params->cipher) {
    case WLAN_CIPHER_SUITE_WEP40:
        cipher = MAC_RSNIE_CIPHER_WEP40;
        break;
    case WLAN_CIPHER_SUITE_WEP104:
        cipher = MAC_RSNIE_CIPHER_WEP104;
        break;
    case WLAN_CIPHER_SUITE_TKIP:
        cipher = MAC_RSNIE_CIPHER_TKIP;
        break;
    case WLAN_CIPHER_SUITE_CCMP:
        cipher = MAC_RSNIE_CIPHER_CCMP;
        break;
    case WLAN_CIPHER_SUITE_AES_CMAC:
        cipher = MAC_RSNIE_CIPHER_AES_CMAC;
        break;
    case WLAN_CIPHER_SUITE_SMS4:
    {
        // Need to reverse key order
        uint8_t tmp, *key = (uint8_t *)params->key;
        cipher = MAC_RSNIE_CIPHER_SMS4;
        for (i = 0; i < WPI_SUBKEY_LEN/2; i++) {
            tmp = key[i];
            key[i] = key[WPI_SUBKEY_LEN - 1 - i];
            key[WPI_SUBKEY_LEN - 1 - i] = tmp;
        }
        for (i = 0; i < WPI_SUBKEY_LEN/2; i++) {
            tmp = key[i + WPI_SUBKEY_LEN];
            key[i + WPI_SUBKEY_LEN] = key[WPI_KEY_LEN - 1 - i];
            key[WPI_KEY_LEN - 1 - i] = tmp;
        }
        break;
    }
    default:
        return -EINVAL;
    }

    if ((error = asr_send_key_add(asr_hw, asr_vif->vif_index,
                                   (sta ? sta->sta_idx : 0xFF), pairwise,
                                   (uint8_t *)params->key, params->key_len,
                                   key_index, cipher, &key_add_cfm)))
        return error;

    if (key_add_cfm.status != 0)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"error status: %d\n",key_add_cfm.status);
        return -EIO;
    }

    /* Save the index retrieved from LMAC */
    asr_key->hw_idx = key_add_cfm.hw_key_idx;

    return 0;
}

/**
 * @del_key: remove a key given the @mac_addr (%NULL for a group key)
 *    and @key_index, return -ENOENT if the key doesn't exist.
 */
static int asr_cfg80211_del_key(struct asr_vif *asr_vif,
                                 uint8_t key_index, bool pairwise, const uint8_t *mac_addr)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    int error;
    struct asr_sta *sta = NULL;
    struct asr_key *asr_key;

    if (mac_addr) {
        sta = asr_get_sta(asr_hw, mac_addr);
        if (!sta)
            return -EINVAL;
        asr_key = &sta->key;
    }
    else
        asr_key = &asr_vif->key[key_index];

    error = asr_send_key_del(asr_hw, asr_key->hw_idx);

    return error;
}

#ifdef CONFIG_SME
int asr_cfg80211_auth(struct asr_vif *asr_vif,struct cfg80211_auth_request *req)
{
    struct sm_auth_cfm sm_auth_cfm;
    int error = 0;
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    #if 1
    if (asr_test_bit(ASR_DEV_CLOSEING, &asr_vif->dev_flags)
        || !asr_test_bit(ASR_DEV_STARTED, &asr_hw->phy_flags)
        || asr_test_bit(ASR_DEV_STA_DISCONNECTING, &asr_vif->dev_flags)
        || asr_test_bit(ASR_DEV_SCANING, &asr_vif->dev_flags)) {
        return -EBUSY;
    }
    #endif

    asr_set_bit(ASR_DEV_STA_CONNECTING, &asr_vif->dev_flags);

    /* For SHARED-KEY authentication, must install key first */
    if (req->auth_type == NL80211_AUTHTYPE_SHARED_KEY && req->key_len)
    {
        struct key_params key_params;
        key_params.key = req->key;
        key_params.seq = NULL;
        key_params.key_len = req->key_len;
        key_params.seq_len = 0;
        key_params.cipher = req->crypto.cipher_group;
        asr_cfg80211_add_key(asr_vif, req->key_idx, false, NULL, &key_params);
    }

    /* Forward the information to the LMAC */
    if ((error = asr_send_sm_auth_req(asr_vif, req, &sm_auth_cfm))) {
        asr_clear_bit(ASR_DEV_STA_CONNECTING, &asr_vif->dev_flags);
        return error;
    }

    if (sm_auth_cfm.status != CO_OK) {
        asr_clear_bit(ASR_DEV_STA_CONNECTING, &asr_vif->dev_flags);
    }

    // Check the status
    switch (sm_auth_cfm.status) {
    case CO_OK:
        error = 0;
        break;
    case CO_BUSY:
        error = -EINPROGRESS;
        break;
    case CO_OP_IN_PROGRESS:
        error = -EALREADY;
        break;
    default:
        error = -EIO;
        break;
    }

    return error;
}

int asr_cfg80211_assoc(struct asr_vif *asr_vif, struct cfg80211_assoc_request *req)
{
    struct sm_assoc_cfm sm_assoc_cfm;
    int error = 0;
    //struct asr_hw *asr_hw = asr_vif->asr_hw;

    /* Forward the information to the LMAC */
    if ((error = asr_send_sm_assoc_req(asr_vif, req, &sm_assoc_cfm)))
        return error;

    // Check the status
    switch (sm_assoc_cfm.status) {
    case CO_OK:
        error = 0;
        break;
    case CO_BUSY:
        error = -EINPROGRESS;
        break;
    case CO_OP_IN_PROGRESS:
        error = -EALREADY;
        break;
    default:
        error = -EIO;
        break;
    }

    return error;
}

int asr_cfg80211_deauth(struct asr_vif *asr_vif, struct cfg80211_deauth_request *req)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;
    dbg(D_INF,D_UWIFI_CTRL,"%s: rx reason_code=%d from AP\n", __func__, req->reason_code);
    req->reason_code = WLAN_REASON_DEAUTH_LEAVING;
    return (asr_send_sm_disconnect_req(asr_hw, asr_vif, req->reason_code));
}

int asr_cfg80211_disassoc(struct asr_vif *asr_vif,  uint16_t reason_code)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;
    dbg(D_INF,D_UWIFI_CTRL,"%s: rx reason_code=%d from AP\n", __func__, reason_code);
    return (asr_send_sm_disconnect_req(asr_hw, asr_vif, WLAN_REASON_DEAUTH_LEAVING));
}

#endif

/**
 * @connect: Connect to the ESS with the specified parameters. When connected,
 *    call cfg80211_connect_result() with status code %WLAN_STATUS_SUCCESS.
 *    If the connection fails for some reason, call cfg80211_connect_result()
 *    with the status from the AP.
 *    (invoked with the wireless_dev mutex held)
 */
struct cfg80211_assoc_request  g_assoc_req  = {0};
struct cfg80211_auth_request   g_auth_req   = {0};
struct cfg80211_deauth_request g_deauth_req = {0};


static void asr_build_auth_request(struct cfg80211_connect_params *sme)
{
    memset(&g_auth_req, 0, sizeof(struct cfg80211_auth_request));

    // conver sme to auth.
    memcpy(&g_auth_req.channel,sme->channel,sizeof(struct ieee80211_channel));
    memcpy(g_auth_req.bssid,sme->bssid,ETH_ALEN);
    memcpy(g_auth_req.ssid,sme->ssid,sme->ssid_len);
    g_auth_req.ssid_len = sme->ssid_len;
    g_auth_req.auth_type = sme->auth_type;

    memcpy(g_auth_req.ie, sme->ie, sme->ie_len);
    g_auth_req.ie_len = sme->ie_len;

    g_auth_req.mfp = sme->mfp;
    g_auth_req.crypto = sme->crypto;

    memcpy(g_auth_req.key, sme->key,MAX_KEY_LEN);
    g_auth_req.key_len = sme->key_len;
    g_auth_req.key_idx = sme->key_idx;

    #if NX_SAE
    memcpy(g_auth_req.password, sme->password , sme->pwd_len);
    g_auth_req.pwd_len = sme->pwd_len;
    #endif

    //sae update later.
}


static void asr_build_assoc_request(struct cfg80211_connect_params *sme)
{
    memset(&g_assoc_req, 0, sizeof(struct cfg80211_assoc_request));

    memcpy(g_assoc_req.bssid,sme->bssid,ETH_ALEN);
    memcpy(g_assoc_req.ie,sme->ie,sme->ie_len);
    g_assoc_req.ie_len = sme->ie_len;
    memcpy(&g_assoc_req.crypto,&sme->crypto,sizeof(struct cfg80211_crypto_settings));
    g_assoc_req.use_mfp = (sme->mfp != NL80211_MFP_NO);
}

static int asr_cfg80211_connect(struct asr_vif *asr_vif,
                                 struct cfg80211_connect_params *sme)
{
    #ifdef CONFIG_SME
    asr_build_auth_request(sme);

    // save assoc .
    asr_build_assoc_request(sme);

    #if NX_SAE
    if ((sme->auth_type == NL80211_AUTHTYPE_SAE) && (sme->pwd_len)) {
        sme_auth_start(asr_vif,&g_auth_req);
        return CO_OK;
    } else
    #endif
       return asr_cfg80211_auth(asr_vif,&g_auth_req);

    #else
    struct sm_connect_cfm sm_connect_cfm;
    int error = CO_FAIL;

    /* For SHARED-KEY authentication, must install key first */
    if (sme->auth_type == NL80211_AUTHTYPE_SHARED_KEY && sme->key)
    {
        struct key_params key_params;
        key_params.key = sme->key;
        key_params.seq = NULL;
        key_params.key_len = sme->key_len;
        key_params.seq_len = 0;
        key_params.cipher = sme->crypto.cipher_group;
        asr_cfg80211_add_key(asr_vif, sme->key_idx, false, NULL, &key_params);
    }

    /* Forward the information to the LMAC */
    if ((error = asr_send_sm_connect_req(asr_vif, sme, &sm_connect_cfm)))
        return error;

    // Check the status
    switch (sm_connect_cfm.status)
    {
        case CO_OK:
            error = 0;
            break;
        case CO_BUSY:
            error = -EINPROGRESS;
            break;
        case CO_OP_IN_PROGRESS:
            error = -EALREADY;
            break;
        default:
            error = -EIO;
            break;
    }

    return error;
    #endif
}

/**
 * @disconnect: Disconnect from the BSS/ESS.
 *    (invoked with the wireless_dev mutex held)
 */
static int asr_cfg80211_disconnect(struct asr_vif *asr_vif,
                                    uint16_t reason_code)
{

    dbg(D_CRT, D_UWIFI_CTRL, "%s asr_send_sm_disconnect_req start\r\n",__func__);
    int ret = asr_send_sm_disconnect_req(asr_vif->asr_hw, asr_vif, reason_code);
    dbg(D_CRT, D_UWIFI_CTRL, "%s asr_send_sm_disconnect_req end ret=%d\r\n",__func__,ret);

    return ret;
}

/**
 * @add_station: Add a new station.
 */
static int asr_cfg80211_add_station(struct asr_vif *asr_vif,
                                     const uint8_t *mac, struct station_parameters *params)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    struct me_sta_add_cfm me_sta_add_cfm;
    int error = 0;

    /* Indicate we are in a STA addition process - This will allow handling
     * potential PS mode change indications correctly
     */
    asr_hw->adding_sta = true;

    /* Forward the information to the LMAC */
    if ((error = asr_send_me_sta_add(asr_hw, params, mac, asr_vif->vif_index,
                                      &me_sta_add_cfm)))
        return error;

    // Check the status
    switch (me_sta_add_cfm.status)
    {
        case CO_OK:
        {
            struct asr_sta *sta = &asr_hw->sta_table[me_sta_add_cfm.sta_idx];
            int tid;
            sta->aid = params->aid;

            sta->sta_idx = me_sta_add_cfm.sta_idx;
            sta->ch_idx = asr_vif->ch_index;
            sta->vif_idx = asr_vif->vif_index;
            sta->vlan_idx = sta->vif_idx;
            sta->qos = (params->sta_flags_set & BIT(NL80211_STA_FLAG_WME)) != 0;
            sta->ht = params->ht_capa ? 1 : 0;
            sta->acm = 0;
            for (tid = 0; tid < NX_NB_TXQ_PER_STA; tid++) {
                int uapsd_bit = asr_hwq2uapsd[asr_tid2hwq[tid]];
                if (params->uapsd_queues & uapsd_bit)
                    sta->uapsd_tids |= 1 << tid;
                else
                    sta->uapsd_tids &= ~(1 << tid);
            }
            memcpy(sta->mac_addr, mac, ETH_ALEN);

            /* Ensure that we won't process PS change or channel switch ind*/
            asr_rtos_lock_mutex(&asr_hw->cmd_mgr.lock);
            asr_txq_sta_init(asr_hw, sta, asr_txq_vif_get_status(asr_vif));
            list_add_tail(&sta->list, &asr_vif->ap.sta_list);
            sta->valid = true;
            asr_ps_bh_enable(asr_hw, sta, sta->ps.active || me_sta_add_cfm.pm_state);
            asr_rtos_unlock_mutex(&asr_hw->cmd_mgr.lock);

            dbg(D_CRT, D_UWIFI_CTRL, "%s: sta_idx=%d,ps active=%d\r\n",__func__,sta->sta_idx,sta->ps.active);

            error = 0;

            break;
        }
        default:
            error = -EBUSY;
            break;
    }

    asr_hw->adding_sta = false;

    #if 0
    if (ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_AP) {
        if (error == 0) {
            wifi_event_cb(WLAN_EVENT_AP_ASSOCIATED, (void*)mac);
        } else {
            wifi_event_cb(WLAN_EVENT_AP_DISASSOCIATED, (void*)mac);
        }
    }
    #endif

    return error;
}

/**
 * @del_station: Remove a station
 */
static int asr_cfg80211_del_station(struct asr_vif *asr_vif,
                              struct station_del_parameters *params)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    struct asr_sta *cur, *tmp;
    int error = 0, found = 0;
    const uint8_t *mac = params->mac;

    list_for_each_entry_safe(cur, tmp, &asr_vif->ap.sta_list, list) {
        if ((!mac) || (!memcmp(cur->mac_addr, mac, ETH_ALEN))) {
            /* Ensure that we won't process PS change ind */
            asr_rtos_lock_mutex(&asr_hw->cmd_mgr.lock);
            cur->ps.active = false;
            cur->valid = false;
            asr_rtos_unlock_mutex(&asr_hw->cmd_mgr.lock);

            asr_txq_sta_deinit(asr_hw, cur);
            error = asr_send_me_sta_del(asr_hw, cur->sta_idx);
            if ((error != 0) && (error != -EPIPE))
                return error;

            list_del(&cur->list);
            found ++;

            //wifi_event_cb(WLAN_EVENT_AP_DISASSOCIATED, cur->mac_addr);
            break;
        }
    }

    if (!found)
        return -ENOENT;
    else
        return 0;
}

/**
 * @change_station: Modify a given station. Note that flags changes are not much
 *    validated in cfg80211, in particular the auth/assoc/authorized flags
 *    might come to the driver in invalid combinations -- make sure to check
 *    them, also against the existing state! Drivers must call
 *    cfg80211_check_station_change() to validate the information.
 */
static int asr_cfg80211_change_station(struct asr_hw *asr_hw,
                                        const uint8_t *mac, struct station_parameters *params)
{
    struct asr_sta *sta;

    sta = asr_get_sta(asr_hw, mac);
    if (!sta)
    {
        return -EINVAL;
    }

    if (params->sta_flags_mask & BIT(NL80211_STA_FLAG_AUTHORIZED))
        asr_send_me_set_control_port_req(asr_hw,
                (params->sta_flags_set & BIT(NL80211_STA_FLAG_AUTHORIZED)) != 0,
                sta->sta_idx);

    dbg(D_INF, D_UWIFI_CTRL, "%s: %X:%X:%X:%X:%X:%X,aid=%d,sta=%d,flag=0x%X\n",
        __func__, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], params->aid, sta->sta_idx, params->sta_flags_set);

    return 0;
}

/**
 * @start_ap: Start acting in AP mode defined by the parameters.
 */
static int asr_cfg80211_start_ap(struct asr_vif *asr_vif,
                                  struct cfg80211_ap_settings *settings)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    struct apm_start_cfm apm_start_cfm;
    struct asr_dma_elem elem;
    struct asr_sta *sta;
    int error = 0;
    dbg(D_DBG, D_UWIFI_CTRL, "%s enter...", __func__);
    /* Forward the information to the LMAC */
    if ((error = asr_send_apm_start_req(asr_hw, asr_vif, settings, &apm_start_cfm, &elem))) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s asr_send_apm_start_req failed", __func__);
        return error;
    }
    dbg(D_INF, D_UWIFI_CTRL, "%s status:0x%x bcmc:0x%x vif:0x%x ch:0x%x", __func__, apm_start_cfm.status, apm_start_cfm.bcmc_idx, apm_start_cfm.vif_idx, apm_start_cfm.ch_idx);
    // Check the status
    switch (apm_start_cfm.status)
    {
        case CO_OK:
        {
            uint8_t txq_status = 0;
            asr_vif->ap.bcmc_index = apm_start_cfm.bcmc_idx;
            //asr_vif->ap.flags = 0;//fixed it later
            sta = &asr_hw->sta_table[apm_start_cfm.bcmc_idx];
            sta->valid = true;
            sta->aid = 0;
            sta->sta_idx = apm_start_cfm.bcmc_idx;
            sta->ch_idx = apm_start_cfm.ch_idx;
            sta->vif_idx = asr_vif->vif_index;
            sta->qos = false;
            sta->acm = 0;
            sta->ps.active = false;
            asr_rtos_lock_mutex(&asr_hw->cmd_mgr.lock);
            asr_chanctx_link(asr_vif, apm_start_cfm.ch_idx,
                              &settings->chandef);
            if (asr_hw->cur_chanctx != apm_start_cfm.ch_idx) {
                txq_status = ASR_TXQ_STOP_CHAN;
            }
            asr_txq_vif_init(asr_hw, asr_vif, txq_status);
            asr_rtos_unlock_mutex(&asr_hw->cmd_mgr.lock);

            error = 0;
            /* If the AP channel is already the active, we probably skip radar
               activation on MM_CHANNEL_SWITCH_IND (unless another vif use this
               ctxt). In anycase retest if radar detection must be activated
             */
            break;
        }
        case CO_BUSY:
            error = -EINPROGRESS;
            break;
        case CO_OP_IN_PROGRESS:
            error = -EALREADY;
            break;
        default:
            error = -EIO;
            break;
    }

    if (error) {
        dbg(D_ERR,D_UWIFI_CTRL,"Failed to start AP (%d)", error);
    } else {
        dbg(D_INF,D_UWIFI_CTRL,"AP started: ch=%d, bcmc_idx=%d",
                    asr_vif->ch_index, asr_vif->ap.bcmc_index);
    }

    // Free the buffer used to build the beacon
    asr_rtos_free(elem.buf);
    elem.buf = NULL;
    dbg(D_DBG, D_UWIFI_CTRL, "%s exit", __func__);
    return error;
}


/**
 * @change_beacon: Change the beacon parameters for an access point mode
 *    interface. This should reject the call when AP mode wasn't started.
 */
static int asr_cfg80211_change_beacon(struct asr_vif *asr_vif,
                                       struct cfg80211_beacon_data *info)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    struct asr_bcn *bcn = &asr_vif->ap.bcn;
    struct asr_dma_elem elem;
    uint8_t *buf;
    int error = 0;



    // Build the beacon
    buf = asr_build_bcn(bcn, info);
    if (!buf)
        return -ENOMEM;

    // Fill in the DMA structure
    elem.buf = buf;
    elem.len = bcn->len;
    elem.dma_addr = (uint32_t)buf;

    // Forward the information to the LMAC



    error = asr_send_bcn_change(asr_hw, asr_vif->vif_index, elem.dma_addr,
                                 bcn->len, bcn->head_len, bcn->tim_len, NULL);
    asr_rtos_free(elem.buf);
    elem.buf = NULL;

    return error;
}

/**
 * * @stop_ap: Stop being an AP, including stopping beaconing.
 */
static int asr_cfg80211_stop_ap(struct asr_vif *asr_vif)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    struct station_del_parameters params;
    params.mac = NULL;

    asr_send_apm_stop_req(asr_hw, asr_vif);
    asr_chanctx_unlink(asr_vif);

    /* delete any remaining STA*/
    while (!list_empty(&asr_vif->ap.sta_list)) {
        asr_cfg80211_del_station(asr_vif, &params);
    }

    asr_txq_vif_deinit(asr_hw, asr_vif);
    asr_del_bcn(&asr_vif->ap.bcn);
    asr_del_csa(asr_vif);

    //uwifi_platform_deinit(asr_hw, asr_vif);
    //list_del(&asr_vif->list);
    uwifi_netdev_ops.ndo_stop(asr_hw, asr_vif);

    return 0;
}

/**
 * @set_tx_power: set the transmit power according to the parameters,
 *    the power passed is in mBm, to get dBm use MBM_TO_DBM(). The
 *    wdev may be %NULL if power was set for the wiphy, and will
 *    always be %NULL unless the driver supports per-vif TX power
 *    (as advertised by the nl80211 feature flag.)
 */
static int asr_cfg80211_set_tx_power(struct asr_vif *asr_vif,
                                      enum nl80211_tx_power_setting type, int dbm)
{
    int8_t pwr;
    int res = 0;

    if (type == NL80211_TX_POWER_AUTOMATIC) {
        pwr = 0x7f;
    } else {
        pwr = dbm;
    }

    res = asr_send_set_power(asr_vif->asr_hw, asr_vif->vif_index, pwr, NULL);

    return res;
}

#define RC_FIXED_RATE_NOT_SET    (0xFFFF)    /// Disabled fixed rate configuration
int asr_cfg80211_set_bitrate(u32 legacy ,u8 mcsrate_2G,u16 he_mcsrate_2G)
{
    struct asr_hw *asr_hw = uwifi_get_asr_hw();
    struct asr_vif *asr_vif = NULL;

    u16 rate_cfg = RC_FIXED_RATE_NOT_SET;
    u32 ratemask = 0xfffff;
    u8 mcs, ht_format, short_gi, nss, mcs_index, legacy_index,bg_rate,bw;
    struct asr_sta *sta = NULL;
    int ret = -1;
    u8 gi_2G = 0;

    u8 he_gi_2G = 0;
    u8 he_ltf_2G = 0;
    u8 he_format = 0;

    #if 0
    u8 mcsrate_2G = 0xff;
    u16 he_mcsrate_2G = 0x3ff;
    u32 legacy = 0xfff;
    #endif

    if(NULL == asr_hw)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"set_bitrate:pls open wifi first!");
        return -1;
    }

    if((asr_hw->sta_vif_idx == 0xff) || ((asr_vif=asr_hw->vif_table[asr_hw->sta_vif_idx]) == NULL)) {

        dbg(D_ERR,D_UWIFI_CTRL,"set_bitrate:pls connect ap first!");
        return -1;
    }


    mcs = ht_format = short_gi = nss = bw = bg_rate = mcs_index = legacy_index = 0;
    /*
       set fix rate cmd: iw dev wlan0 set bitrates [legacy-2.4 <legacy rate in Mbps>] [ht-mcs-2.4 <MCS index>]
     */

    /* only check 2.4 bands, skip the rest */
    /* copy legacy rate mask */
    #if 0
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 1)
    legacy = mask->control[NL80211_BAND_2GHZ].legacy;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
    legacy = mask->control[IEEE80211_BAND_2GHZ].legacy;
#else
    legacy = mask->control[IEEE80211_BAND_2GHZ].legacy;
#endif
    #endif
    ratemask = legacy;

    /* copy mcs rate mask , mcs[nss-1],for nss=1, use mcs[0]*/
    #if 0
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 1)
    mcsrate_2G = mask->control[NL80211_BAND_2GHZ].ht_mcs[0];
#elif(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
    mcsrate_2G = mask->control[IEEE80211_BAND_2GHZ].ht_mcs[0];
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(3, 10, 0))
    mcsrate_2G = mask->control[IEEE80211_BAND_2GHZ].mcs[0];
#endif
    #endif

    ratemask |= mcsrate_2G << 12;

#if 0 //LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 1)
        #ifdef CONFIG_ASR595X
        gi_2G = mask->control[NL80211_BAND_2GHZ].gi;
        he_mcsrate_2G = mask->control[NL80211_BAND_2GHZ].he_mcs[0];
        he_gi_2G  = mask->control[NL80211_BAND_2GHZ].he_gi;
        he_ltf_2G = mask->control[NL80211_BAND_2GHZ].he_ltf;
        #endif
#endif

    //set fix rate configuration (modulation,pre_type, short_gi, bw, nss, mcs)
    if (asr_hw->mod_params->sgi && gi_2G) {
        short_gi = 1;
    } else
        short_gi = 0;

    if (asr_hw->mod_params->use_2040) {
        bw = 1;
    } else
        bw = 0;

    /*
       rate control
                                       13~11                 10                9         8~7         6~0
       self_cts | no_protection | modulation_mask(3) | premble_type(1) | short_GT(1) | bw(2) |    mcs index(7)
       nss(4~3)   mcs(2~0)
     */

    if (ratemask == 0xfffff && (he_mcsrate_2G == 0x3ff))
        rate_cfg = RC_FIXED_RATE_NOT_SET;
    else {

        if ((mcsrate_2G == 0xff)
            ) {
            if (he_mcsrate_2G == 0x3ff)
            {
                // bg fixed rate set.
                //bg_rate = ratemask & 0xfff;
                while (legacy_index < 12) {
                    if (legacy & BIT(legacy_index))
                        bg_rate = legacy_index;

                    legacy_index++;
                }

                ht_format = 0;
                if (short_gi)
                  rate_cfg = ((ht_format & 0x7) << 11) | (bg_rate & 0x7f);
                else
                  rate_cfg = ((ht_format & 0x7) << 11) | (0x1 << 10) | (bg_rate & 0x7f);
            } else {

                he_format = 5;   //HE fixed rate config.
                mcs = 0;
                while (mcs_index < 10) {
                    if (he_mcsrate_2G & BIT(mcs_index))
                        mcs = mcs_index;

                    mcs_index++;
                }

                if (asr_hw->mod_params->nss)
                    nss = asr_hw->mod_params->nss - 1;

                rate_cfg =
                    ((he_format & 0x7) << 11) | ((he_gi_2G & 0x3) << 9) | ((bw & 0x3) << 7) | ((nss & 0x7) << 4) | (mcs & 0xf);
            }

        } else if (mcsrate_2G != 0xff) {    // ofdm rate
            ht_format = 2;   //HT-MF
            mcs = 0;
            while (mcs_index < 8) {
                if (mcsrate_2G & BIT(mcs_index))
                    mcs = mcs_index;

                mcs_index++;
            }

            if (asr_hw->mod_params->nss)
                nss = asr_hw->mod_params->nss - 1;

            rate_cfg =
                ((ht_format & 0x7) << 11) | (mcs & 0x7f) | ((bw & 0x3) << 7) | ((short_gi & 0x1) << 9);
        }
    }

    dbg(D_ERR,D_UWIFI_CTRL,"[%s]he_mcsrate_2G (0x%x) legacy(0x%x) mcsrate_2G(0x%x) bg_rate(0x%x) (%x:%x:%x:%x) ratemask(0x%x) rate_cfg(0x%x) ,he_ltf_2G=(0x%x) \n",
         __func__, he_mcsrate_2G,  legacy,      mcsrate_2G,      bg_rate,     ht_format, mcs, nss, short_gi, ratemask, rate_cfg,he_ltf_2G);

    if (asr_vif->iftype == NL80211_IFTYPE_AP) {
        #if 0
        struct asr_sta *asr_sta = NULL;
        list_for_each_entry(asr_sta, &asr_vif->ap.sta_list, list) {
            if (asr_sta && asr_sta->valid && ((addr == NULL)
                           || ether_addr_equal(asr_sta->mac_addr, addr))) {
                sta = asr_sta;
                ret = asr_send_me_rc_set_rate(asr_hw, sta->sta_idx, rate_cfg);
                dbg(D_ERR,D_UWIFI_CTRL,"set station idx:[%d] %d\n", sta->sta_idx, ret);
            }
        }
        #endif

        dbg(D_ERR,D_UWIFI_CTRL,"ap mode not support,skip \r\n");
        ret = 0;
    } else {
        sta = asr_vif->sta.ap;
        if (sta && sta->valid)
                ret = asr_send_me_rc_set_rate(asr_hw, sta->sta_idx, rate_cfg);
            else {
                 dbg(D_ERR,D_UWIFI_CTRL,"sta is NULL ,skip \r\n");
                 ret = 0;
            }
    }
    return ret;
}


static int asr_cfg80211_set_txq_params(struct asr_vif *asr_vif,
                                        struct ieee80211_txq_params *params)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    uint8_t hw_queue, aifs, cwmin, cwmax;
    uint32_t param;



    hw_queue = asr_ac2hwq[0][params->ac];

    aifs  = params->aifs;
    cwmin = fls(params->cwmin);
    cwmax = fls(params->cwmax);

    /* Store queue information in general structure */
    param  = (uint32_t) (aifs << 0);
    param |= (uint32_t) (cwmin << 4);
    param |= (uint32_t) (cwmax << 8);
    param |= (uint32_t) (params->txop) << 12;

    /* Send the MM_SET_EDCA_REQ message to the FW */
    return asr_send_set_edca(asr_hw, hw_queue, param, false, asr_vif->vif_index);
}


/**
 * @remain_on_channel: Request the driver to remain awake on the specified
 *    channel for the specified duration to complete an off-channel
 *    operation (e.g., public action frame exchange). When the driver is
 *    ready on the requested channel, it must indicate this with an event
 *    notification by calling cfg80211_ready_on_channel().
 */
static int
asr_cfg80211_remain_on_channel(struct asr_vif *asr_vif,
                                struct ieee80211_channel *chan,
                                unsigned int duration, uint64_t *cookie)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    struct asr_roc_elem *roc_elem;
    int error;



    /* Check that no other RoC procedure has been launched */
    if (asr_hw->roc_elem) {
        return -EBUSY;
    }

    /* Allocate a temporary RoC element */
    roc_elem = asr_rtos_malloc(sizeof(struct asr_roc_elem));

    /* Verify that element has well been allocated */
    if (!roc_elem) {
        return -ENOMEM;
    }

    /* Initialize the RoC information element */
    roc_elem->chan = chan;
    roc_elem->duration = duration;
    roc_elem->mgmt_roc = false;
    roc_elem->on_chan = false;
    roc_elem->asr_vif = asr_vif;

    /* Forward the information to the FMAC */
    error = asr_send_roc(asr_hw, asr_vif, chan, duration);

    /* If no error, keep all the information for handling of end of procedure */
    if (error == 0) {
        asr_hw->roc_elem = roc_elem;

        /* Set the cookie value */
        *cookie = (uint64_t)(asr_hw->roc_cookie_cnt);

        /* Initialize the OFFCHAN TX queue to allow off-channel transmissions */
        asr_txq_offchan_init(asr_vif);
    } else {
        /* Free the allocated element */
        asr_rtos_free(roc_elem);
        roc_elem = NULL;
    }

    return error;
}

/**
 * @cancel_remain_on_channel: Cancel an on-going remain-on-channel operation.
 *    This allows the operation to be terminated prior to timeout based on
 *    the duration value.
 */
static int asr_cfg80211_cancel_remain_on_channel( struct asr_hw *asr_hw,
                                                  uint64_t cookie)
{


    /* Check if a RoC procedure is pending */
    if (!asr_hw->roc_elem) {
        return 0;
    }

    /* Forward the information to the FMAC */
    return asr_send_cancel_roc(asr_hw);
}

/**
 * @get_channel: Get the current operating channel for the virtual interface.
 *    For monitor interfaces, it should return %NULL unless there's a single
 *    current monitoring channel.
 */
static int asr_cfg80211_get_channel(struct asr_vif *asr_vif,
                                     struct cfg80211_chan_def *chandef)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    struct asr_chanctx *ctxt;

    if (!asr_vif->up ||
        !asr_chanctx_valid(asr_hw, asr_vif->ch_index)) {
        return -ENODATA;
    }

    ctxt = &asr_hw->chanctx_table[asr_vif->ch_index];
    *chandef = ctxt->chan_def;

    return 0;
}

/**
 * @mgmt_tx: Transmit a management frame.
 */
static int asr_cfg80211_mgmt_tx(struct asr_vif *asr_vif,
                                 struct cfg80211_mgmt_tx_params *params,
                                 uint64_t *cookie)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    struct asr_sta *asr_sta;
    struct ieee80211_mgmt *mgmt = (void *)params->buf;
    int error = 0;
    bool ap = false;
    bool offchan = false;

    do
    {
        /* Check if provided VIF is an AP or a STA one */
        switch (ASR_VIF_TYPE(asr_vif)) {
        case NL80211_IFTYPE_AP:
            ap = true;
            break;
#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
        case NL80211_IFTYPE_MONITOR:
            return asr_start_mgmt_xmit(asr_vif, NULL, params, offchan, cookie);;
#endif
        case NL80211_IFTYPE_STATION:
        default:
            break;
        }

        /* Get STA on which management frame has to be sent */
        asr_sta = asr_retrieve_sta(asr_hw, asr_vif, mgmt->da,
                                     mgmt->frame_control, ap);

        /* Push the management frame on the TX path */
        error = asr_start_mgmt_xmit(asr_vif, asr_sta, params, offchan, cookie);
    } while (0);

    return error;
}

/**
 * @set_cqm_rssi_config: Configure connection quality monitor RSSI threshold.
 */
static
int asr_cfg80211_set_cqm_rssi_config(struct asr_vif *asr_vif,
                                  int32_t rssi_thold, uint32_t rssi_hyst)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    return asr_send_cfg_rssi_req(asr_hw, asr_vif->vif_index, rssi_thold, rssi_hyst);
}

/**
 * @set_cqm_rssi_level_config: Configure connection quality monitor RSSI level.
 */
static
int asr_cfg80211_set_cqm_rssi_level_config(struct asr_vif *asr_vif,
                                  bool enable)
{
    struct asr_hw *asr_hw = asr_vif->asr_hw;

    return asr_send_cfg_rssi_level_req(asr_hw, enable);

}

void asr_csa_finish(struct asr_csa *csa)
{
    struct asr_vif *vif = csa->vif;
    struct asr_hw *asr_hw = vif->asr_hw;
    int error = csa->status;

    if (!error)
        error = asr_send_bcn_change(asr_hw, vif->vif_index, csa->dma.dma_addr,
                                     csa->bcn.len, csa->bcn.head_len,
                                     csa->bcn.tim_len, NULL);

    if (error)
    {
        //cfg80211_stop_iface(asr_hw->wiphy, &vif->wdev, GFP_KERNEL);
        dbg(D_ERR,D_UWIFI_CTRL,"need disconnect");
        asr_cfg80211_disconnect(vif,WLAN_REASON_DEAUTH_LEAVING);
    }
    else
    {
        asr_chanctx_unlink(vif);
        asr_chanctx_link(vif, csa->ch_idx, &csa->chandef);
        asr_rtos_lock_mutex(&asr_hw->cmd_mgr.lock);
        if (asr_hw->cur_chanctx == csa->ch_idx) {
            //asr_radar_detection_enable_on_cur_channel(asr_hw);
            asr_txq_vif_start(vif, ASR_TXQ_STOP_CHAN, asr_hw);
        } else
            asr_txq_vif_stop(vif, ASR_TXQ_STOP_CHAN, asr_hw);
        asr_rtos_unlock_mutex(&asr_hw->cmd_mgr.lock);
        //cfg80211_ch_switch_notify(vif->ndev, &csa->chandef);
    }
    asr_del_csa(vif);
}

/**
 *
 * @channel_switch: initiate channel-switch procedure (with CSA). Driver is
 *    responsible for veryfing if the switch is possible. Since this is
 *    inherently tricky driver may decide to disconnect an interface later
 *    with cfg80211_stop_iface(). This doesn't mean driver can accept
 *    everything. It should do it's best to verify requests and reject them
 *    as soon as possible.
 */
int asr_cfg80211_channel_switch(struct asr_vif *asr_vif,
                                 struct cfg80211_csa_settings *params)
{
    struct asr_dma_elem elem;
    struct asr_bcn *bcn, *bcn_after;
    struct asr_csa *csa;
    uint16_t csa_oft[BCN_MAX_CSA_CPT];
    uint8_t *buf;
    int i, error = 0;

    if (asr_vif->ap.csa)
        return -EBUSY;

    if (params->n_counter_offsets_beacon > BCN_MAX_CSA_CPT)
        return -EINVAL;

    /* Build the new beacon with CSA IE */
    bcn = &asr_vif->ap.bcn;
    buf = asr_build_bcn(bcn, &params->beacon_csa);
    if (!buf)
        return -ENOMEM;

    memset(csa_oft, 0, sizeof(csa_oft));
    for (i = 0; i < params->n_counter_offsets_beacon; i++)
    {
        csa_oft[i] = params->counter_offsets_beacon[i] + bcn->head_len +
            bcn->tim_len;
    }

    /* If count is set to 0 (i.e anytime after this beacon) force it to 2 */
    if (params->count == 0) {
        params->count = 2;
        for (i = 0; i < params->n_counter_offsets_beacon; i++)
        {
            buf[csa_oft[i]] = 2;
        }
    }

    elem.buf = buf;
    elem.len = bcn->len;
    elem.dma_addr = (uint32_t)buf;

    /* Build the beacon to use after CSA. It will only be sent to fw once
       CSA is over. */
    csa = asr_rtos_malloc(sizeof(struct asr_csa));
    if (!csa) {
        error = -ENOMEM;
        goto end;
    }

    bcn_after = &csa->bcn;
    buf = asr_build_bcn(bcn_after, &params->beacon_after);
    if (!buf) {
        error = -ENOMEM;
        asr_del_csa(asr_vif);
        goto end;
    }

    asr_vif->ap.csa = csa;
    csa->vif = asr_vif;
    csa->chandef = params->chandef;
    csa->dma.buf = buf;
    csa->dma.len = bcn_after->len;
    csa->dma.dma_addr = (uint32_t)buf;

    /* Send new Beacon. FW will extract channel and count from the beacon */
    error = asr_send_bcn_change(asr_vif->asr_hw, asr_vif->vif_index, elem.dma_addr,
                                 bcn->len, bcn->head_len, bcn->tim_len, csa_oft);

    if (error) {
        asr_del_csa(asr_vif);
        goto end;
    } else {
        //INIT_WORK(&csa->work, asr_csa_finish);
        asr_csa_finish(csa);
    }

  end:
    asr_rtos_free(elem.buf);
    elem.buf = NULL;

    return error;
}


/**
 * @change_bss: Modify parameters for a given BSS (mainly for AP mode).
 */
int asr_cfg80211_change_bss(struct asr_vif *asr_vif,
                             struct bss_parameters *params)
{
    int res =  -EOPNOTSUPP;

    if ((ASR_VIF_TYPE(asr_vif) == NL80211_IFTYPE_AP) &&
        (params->ap_isolate > -1)) {

        if (params->ap_isolate)
            asr_vif->ap.flags |= ASR_AP_ISOLATE;
        else
            asr_vif->ap.flags &= ~ASR_AP_ISOLATE;

        res = 0;
    }

    return res;
}
#define GetFrameType(pbuf)       ((*(uint16_t *)(pbuf)) & 0x000C)
extern asr_semaphore_t g_asr_mgmt_sem;
void cfg80211_rx_mgmt(struct asr_vif *asr_vif, uint8_t *pframe, uint32_t len)
{
    uint8_t bc_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    if (memcmp(pframe+4, asr_vif->wlan_mac_add.mac_addr, MAC_ADDR_LEN)
        && memcmp(pframe+4, bc_addr, MAC_ADDR_LEN))
    {
        dbg(D_CRT, D_UWIFI_CTRL, "uwifi_handle_rx_mgmt ra_addr error");
        return;
    }

#ifdef CFG_STATION_SUPPORT
#ifdef CONFIG_SME
    if(asr_vif->iftype == NL80211_IFTYPE_STATION)
    {
        if (GetFrameType(pframe) == IEEE80211_FTYPE_MGMT)
        {
            asr_rtos_get_semaphore(&g_asr_mgmt_sem,ASR_WAIT_FOREVER);
            uwifi_wpa_handle_rx_mgmt(asr_vif, pframe, len);
            asr_rtos_set_semaphore(&g_asr_mgmt_sem);
        }
    }
    else
#endif
#endif
#ifdef CFG_SOFTAP_SUPPORT
    if(asr_vif->iftype == NL80211_IFTYPE_AP)
    {
        if (GetFrameType(pframe) == IEEE80211_FTYPE_DATA)
        {
            //uwifi_hostapd_handle_rx_data(asr_vif, pframe, len);
        }
        else
        {
            asr_rtos_get_semaphore(&g_asr_mgmt_sem,ASR_WAIT_FOREVER);
            uwifi_hostapd_handle_rx_mgmt(asr_vif, pframe, len);
            asr_rtos_set_semaphore(&g_asr_mgmt_sem);
        }
    }
    else
#endif
    {
        dbg(D_CRT, D_UWIFI_CTRL, "uwifi_handle_rx_mgmt donot support");
    }
}

/**
 * cfg80211_mgmt_tx_comp_status - notification of TX status for management frame
 * @buf: Management frame (header + body)
 * @len: length of the frame data
 * @ack: Whether frame was acknowledged
 *
 * This function is called whenever a management frame was requested to be
 * transmitted with cfg80211_ops::mgmt_tx() to report the TX status of the
 * transmission attempt.
 */
void cfg80211_mgmt_tx_comp_status(struct asr_vif *asr_vif, const uint8_t *buf, const uint32_t len,
                                                                                                        const bool ack)
{
#ifdef CFG_SOFTAP_SUPPORT
    if(asr_vif->iftype == NL80211_IFTYPE_AP)
    {
        uwifi_hostapd_mgmt_tx_comp_status(asr_vif, buf, len, ack);
    }
    else
#endif
    {
#if (defined CFG_SNIFFER_SUPPORT || defined CFG_CUS_FRAME)
        dbg(D_DBG, D_UWIFI_CTRL, "cfg80211_mgmt_tx_comp_status tx_comp_status:%d. \n",ack);
#else
        dbg(D_CRT, D_UWIFI_CTRL, "cfg80211_mgmt_tx_comp_status not support");
#endif
    }
}

struct cfg80211_ops uwifi_cfg80211_ops = {
    .add_virtual_intf = asr_cfg80211_add_iface,
    .del_virtual_intf = asr_cfg80211_del_iface,
    .change_virtual_intf = asr_cfg80211_change_iface,
#ifdef CFG_STATION_SUPPORT
    .scan = asr_cfg80211_scan,
    .connect = asr_cfg80211_connect,
    .disconnect = asr_cfg80211_disconnect,
#endif
#ifdef CFG_ENCRYPT_SUPPORT
    .add_key = asr_cfg80211_add_key,
    .del_key = asr_cfg80211_del_key,
#endif
    .add_station = asr_cfg80211_add_station,
    .del_station = asr_cfg80211_del_station,
    .change_station = asr_cfg80211_change_station,
    .mgmt_tx = asr_cfg80211_mgmt_tx,
#ifdef CFG_SOFTAP_SUPPORT
    .start_ap = asr_cfg80211_start_ap,
    .stop_ap = asr_cfg80211_stop_ap,
    .change_beacon = asr_cfg80211_change_beacon,
#endif
    .set_txq_params = asr_cfg80211_set_txq_params,
    .set_tx_power = asr_cfg80211_set_tx_power,
    .remain_on_channel = asr_cfg80211_remain_on_channel,
    .cancel_remain_on_channel = asr_cfg80211_cancel_remain_on_channel,
    .get_channel = asr_cfg80211_get_channel,
    .set_cqm_rssi_config = asr_cfg80211_set_cqm_rssi_config,
    .set_cqm_rssi_level_config = asr_cfg80211_set_cqm_rssi_level_config,
    .channel_switch = asr_cfg80211_channel_switch,
    .change_bss = asr_cfg80211_change_bss,
};

