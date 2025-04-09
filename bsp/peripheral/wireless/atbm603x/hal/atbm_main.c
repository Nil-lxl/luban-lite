/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#ifndef LINUX_OS
#include <string.h>
#include <stdio.h>
#endif
#include "atbm_hal.h"
#include "svn_version.h"
#if ATBM_SUPPORT_BRIDGE
#include "atbm_bridge.h"
#endif

 atbm_uint8 default_macaddr[6] = {0x00,0x00,0x11,0x32,0x43,0x69};
extern struct tcpip_opt lwip_tcp_opt;
struct atbmwifi_common g_hw_prv;
#if ATBM_SDIO_BUS
#define SET_SDIO_DOWNLOAD_BLOCKSIZE(hw_priv) do { \
			hw_priv->sbus_ops->lock(hw_priv->sbus_priv);\
			ATBM_WARN_ON_FUNC(hw_priv->sbus_ops->set_block_size(hw_priv->sbus_priv,DOWNLOAD_BLOCK_SIZE));\
			hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);  \
		}while(0)
#else
#define SET_SDIO_DOWNLOAD_BLOCKSIZE(hw_priv) do{ }while(0)
#endif

#define SET_SDIO_TRANSMIT_BLOCKSIZE(hw_priv) do{ \
			hw_priv->sbus_ops->lock(hw_priv->sbus_priv);\
			ATBM_WARN_ON_FUNC(hw_priv->sbus_ops->set_block_size(hw_priv->sbus_priv,ATBM_SDIO_BLOCK_SIZE));\
			hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);\
	}while(0)

extern atbm_void atbm_get_efuse_data(struct atbmwifi_common *hw_priv);
extern atbm_void atbm_wifi_ticks_timer_init(atbm_void);
extern int atbm_wifi_add_interfaces(struct atbmwifi_common *hw_priv,enum atbm_nl80211_iftype iftype, char *if_name);
extern int atbm_wifi_remove_interfaces(struct atbmwifi_vif *priv);
extern atbm_int32 atbmwifi_enable_lmaclog(atbm_uint32 value );
extern atbm_void atbm_free_netdev(struct atbm_net_device * netdev);
extern 	int atbm_create_timerTask(struct atbmwifi_common *hw_priv);
struct atbmwifi_common *_atbmwifi_vifpriv_to_hwpriv(struct atbmwifi_vif *priv)
{
	return priv->hw_priv;
}
struct atbmwifi_vif *_atbmwifi_hwpriv_to_vifpriv(struct atbmwifi_common *hw_priv,int if_id)
{
	ATBM_WARN_ON_FUNC(if_id < 0);
	if ((if_id < 0) || (if_id >= ATBM_WIFI_MAX_VIFS)){
		wifi_printk((WIFI_TX|WIFI_WARN_CODE), "_atbmwifi_hwpriv_to_vifpriv if_id=%d \n",if_id);
		return ATBM_NULL;
	}

	return hw_priv->vif_list[if_id];
}

static  int _atbmwifi_get_nr_hw_ifaces(struct atbmwifi_common *hw_priv)
{
	return 1;
}

/* TODO: use rates and channels from the device */
#define RATETAB_ENT(_rate, _rateid, _flags)		{_rate, _rateid, _flags}

/*
struct atbmwifi_ieee80211_rate {
atbm_uint16 bitrate;
atbm_uint8 hw_value;
atbm_uint8 rate_flag;
};

.bitrate	= (_rate),		\
.hw_value	= (_rateid),		\
.rate_flag	= (_flags),	\
*/

struct atbmwifi_ieee80211_rate atbmwifi_rates[ATBM_WIFI_RATE_SIZE] = {
	RATETAB_ENT(2,  0,   ATBM_IEEE80211_RT_BASIC|ATBM_IEEE80211_RT_11B),
	RATETAB_ENT(4,  1,   ATBM_IEEE80211_RT_BASIC|ATBM_IEEE80211_RT_11B),
	RATETAB_ENT(11,  2,   ATBM_IEEE80211_RT_BASIC|ATBM_IEEE80211_RT_11B),
	RATETAB_ENT(22, 3,   ATBM_IEEE80211_RT_BASIC|ATBM_IEEE80211_RT_11B),
	RATETAB_ENT(12,  6,   ATBM_IEEE80211_RT_11G),
	RATETAB_ENT(18,  7,  ATBM_IEEE80211_RT_11G),
	RATETAB_ENT(24, 8,  ATBM_IEEE80211_RT_11G),
	RATETAB_ENT(36, 9,  ATBM_IEEE80211_RT_11G),
	RATETAB_ENT(48, 10, ATBM_IEEE80211_RT_11G),
	RATETAB_ENT(72, 11, ATBM_IEEE80211_RT_11G),
	RATETAB_ENT(96, 12, ATBM_IEEE80211_RT_11G),
	RATETAB_ENT(108, 13, ATBM_IEEE80211_RT_11G),
};

#if CONFIG_5G_SUPPORT
struct atbmwifi_ieee80211_rate atbmwifi_rates_5g[ATBM_WIFI_RATE_SIZE] = {	
	RATETAB_ENT(12,  6,   ATBM_IEEE80211_RT_BASIC|ATBM_IEEE80211_RT_11G),
	RATETAB_ENT(18,  7,  ATBM_IEEE80211_RT_11G),
	RATETAB_ENT(24, 8,  ATBM_IEEE80211_RT_BASIC|ATBM_IEEE80211_RT_11G),
	RATETAB_ENT(36, 9,  ATBM_IEEE80211_RT_11G),
	RATETAB_ENT(48, 10, ATBM_IEEE80211_RT_BASIC|ATBM_IEEE80211_RT_11G),
	RATETAB_ENT(72, 11, ATBM_IEEE80211_RT_11G),
	RATETAB_ENT(96, 12, ATBM_IEEE80211_RT_11G),
	RATETAB_ENT(108, 13, ATBM_IEEE80211_RT_11G),
};
#endif

//500k  unit
struct atbmwifi_ieee80211_rate atbm_mcs_rates[] = {
	RATETAB_ENT(13,  14, ATBM_IEEE80211_TX_RC_MCS),
	RATETAB_ENT(26, 15, ATBM_IEEE80211_TX_RC_MCS),
	RATETAB_ENT(39, 16, ATBM_IEEE80211_TX_RC_MCS),
	RATETAB_ENT(52, 17, ATBM_IEEE80211_TX_RC_MCS),
	RATETAB_ENT(78, 18, ATBM_IEEE80211_TX_RC_MCS),
	RATETAB_ENT(104, 19, ATBM_IEEE80211_TX_RC_MCS),
	RATETAB_ENT(117, 20, ATBM_IEEE80211_TX_RC_MCS),
	RATETAB_ENT(130, 21, ATBM_IEEE80211_TX_RC_MCS),
	RATETAB_ENT(12 , 22, ATBM_IEEE80211_TX_RC_MCS),
};
#define CHAN2G(_channel, _freq, _flags) {30, _channel, _flags}
#define CHAN5G(_channel, _flags) {				\
	.hw_value		= (_channel),			\
	.flags			= (_flags),			\
	.max_power		= 30,				\
}

const static struct atbmwifi_ieee80211_channel atbmwifi_2ghz_chantable_const[] = {
	CHAN2G(1, 2412, 0),
	CHAN2G(2, 2417, 0),
	CHAN2G(3, 2422, 0),
	CHAN2G(4, 2427, 0),
	CHAN2G(5, 2432, 0),
	CHAN2G(6, 2437, 0),
	CHAN2G(7, 2442, 0),
	CHAN2G(8, 2447, 0),
	CHAN2G(9, 2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	CHAN2G(14, 2484, 0),
};

static struct atbmwifi_ieee80211_channel atbmwifi_2ghz_chantable[14] = {
	CHAN2G(1, 2412, 0),
	CHAN2G(2, 2417, 0),
	CHAN2G(3, 2422, 0),
	CHAN2G(4, 2427, 0),
	CHAN2G(5, 2432, 0),
	CHAN2G(6, 2437, 0),
	CHAN2G(7, 2442, 0),
	CHAN2G(8, 2447, 0),
	CHAN2G(9, 2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	CHAN2G(14, 2484, 0),
};

#if CONFIG_5G_SUPPORT
const static struct atbmwifi_ieee80211_channel atbm_5ghz_chantable[] = {
	CHAN5G(34, 0),		CHAN5G(36, 0),
	CHAN5G(38, 0),		CHAN5G(40, 0),
	CHAN5G(42, 0),		CHAN5G(44, 0),
	CHAN5G(46, 0),		CHAN5G(48, 0),
	CHAN5G(52, 0),		CHAN5G(56, 0),
	CHAN5G(60, 0),		CHAN5G(64, 0),
	CHAN5G(100, 0),		CHAN5G(104, 0),
	CHAN5G(108, 0),		CHAN5G(112, 0),
	CHAN5G(116, 0),		CHAN5G(120, 0),
	CHAN5G(124, 0),		CHAN5G(128, 0),
	CHAN5G(132, 0),		CHAN5G(136, 0),
	CHAN5G(140, 0),		CHAN5G(149, 0),
	CHAN5G(153, 0),		CHAN5G(157, 0),
	CHAN5G(161, 0),		CHAN5G(165, 0),
#if 0
	CHAN5G(184, 0),		CHAN5G(188, 0),
	CHAN5G(192, 0),		CHAN5G(196, 0),
	CHAN5G(200, 0),		CHAN5G(204, 0),
	CHAN5G(208, 0),		CHAN5G(212, 0),
	CHAN5G(216, 0),
#endif
};
#endif

struct atbmwifi_ieee80211_supported_band atbmwifi_band_2ghz;

atbm_void atbmwifi_band_2ghz_init(atbm_void)
{
	atbm_memset(&atbmwifi_band_2ghz, 0, sizeof(atbmwifi_band_2ghz));
	
	atbmwifi_band_2ghz.channels = atbmwifi_2ghz_chantable;
	atbmwifi_band_2ghz.n_channels = ATBM_ARRAY_SIZE(atbmwifi_2ghz_chantable);
	atbmwifi_band_2ghz.bitrates =atbmwifi_g_rates;
	atbmwifi_band_2ghz.n_bitrates = atbmwifi_g_rates_size;

#if BW_40M_SUPPORT
	atbmwifi_band_2ghz.ht_cap.cap = ATBM_IEEE80211_HT_CAP_GRN_FLD| 
									ATBM_IEEE80211_HT_CAP_SGI_20|
									(1 << ATBM_IEEE80211_HT_CAP_RX_STBC_SHIFT)|
									ATBM_IEEE80211_HT_CAP_SUP_WIDTH_20_40|
									ATBM_IEEE80211_HT_CAP_DSSSCCK40|
									ATBM_IEEE80211_HT_CAP_SGI_40;
#else
	atbmwifi_band_2ghz.ht_cap.cap = ATBM_IEEE80211_HT_CAP_SGI_20|
									(1 << ATBM_IEEE80211_HT_CAP_RX_STBC_SHIFT);
#endif  //BW_40M_SUPPORT
	
	atbmwifi_band_2ghz.ht_cap.ht_supported = 1;
	atbmwifi_band_2ghz.ht_cap.ampdu_factor = ATBM_IEEE80211_HT_MAX_AMPDU_32K;
	atbmwifi_band_2ghz.ht_cap.ampdu_density = ATBM_IEEE80211_HT_MPDU_DENSITY_NONE;
	//atbmwifi_band_2ghz.ht_cap.
	atbmwifi_band_2ghz.ht_cap.mcs.rx_mask[0] = 0xFF;
	atbmwifi_band_2ghz.ht_cap.mcs.rx_highest = 0;
	atbmwifi_band_2ghz.ht_cap.mcs.tx_params |= ATBM_IEEE80211_HT_MCS_TX_DEFINED;
}

#if CONFIG_5G_SUPPORT
static struct atbmwifi_ieee80211_supported_band atbmwifi_band_5ghz = {
	.channels = atbm_5ghz_chantable,
	.n_channels = ATBM_ARRAY_SIZE(atbm_5ghz_chantable),
	.bitrates = atbmwifi_a_rates,
	.n_bitrates = atbmwifi_a_rates_size,
	.ht_cap = {
		.cap = ATBM_IEEE80211_HT_CAP_GRN_FLD |
#if BW_40M_SUPPORT
			ATBM_IEEE80211_HT_CAP_SUP_WIDTH_20_40|
			ATBM_IEEE80211_HT_CAP_DSSSCCK40|
#endif
			ATBM_IEEE80211_HT_CAP_SGI_20 |
			ATBM_IEEE80211_HT_CAP_SGI_40|
			(1 << ATBM_IEEE80211_HT_CAP_RX_STBC_SHIFT),
		.ht_supported = 1,
		.ampdu_factor = ATBM_IEEE80211_HT_MAX_AMPDU_32K,
		.ampdu_density = ATBM_IEEE80211_HT_MPDU_DENSITY_NONE,
		.mcs = {
			.rx_mask[0] = 0xFF,
			.rx_highest = __atbm_cpu_to_le16(0),
			.tx_params = ATBM_IEEE80211_HT_MCS_TX_DEFINED,
		},
	},
};
#endif
void  atbm_efuse_read_byte(struct atbmwifi_common *priv,unsigned int byteIndex, unsigned int *value)
{
    atbm_direct_write_reg_32(priv,0x16b00000, (byteIndex<<8));
    atbm_direct_read_reg_32(priv,0x16b00004,value);
}

unsigned int atbm_efuse_read_bit(struct atbmwifi_common *priv,unsigned int bitIndex)
{
    unsigned int    efuseBitIndex = bitIndex;
    unsigned int byteIndex;
    unsigned int value = 0;

    {
        byteIndex = efuseBitIndex / 8;
        atbm_efuse_read_byte(priv,byteIndex, &value);
    }
    value = value >> (efuseBitIndex % 8);
    value &= 0x1;
    return value;
}

int atbm_check_efuse(struct atbmwifi_common *priv)
{
    if ((atbm_efuse_read_bit(priv,152) == 1) && (atbm_efuse_read_bit(priv,154) == 1))
    {
        return 1;
    }
    return 0;
}

void  atbm_HwGetChipType(struct atbmwifi_common *priv)
{
    unsigned int chipver = 0;

    atbm_direct_read_reg_32(priv,0x0acc017c,&chipver);
    chipver&=0xff;
    wifi_printk(WIFI_ALWAYS,"chipID:0x%x\n",chipver);
    switch(chipver)
    {
        case 0x49:
            if(atbm_check_efuse(priv)){
                iot_printf("chipver: ARESM \r\n");
            }else{
                iot_printf("chipver: ARESB \r\n");
            }
            break;
        case 0x4a:
            iot_printf("chipver: ASMLITE \r\n");
            break;
        case 0xa3:
            iot_printf("chipver: MERCURIUS \r\n");
            break;
        default:
            wifi_printk(WIFI_ALWAYS,"%s, <ERROR> cannot read chip id\n",__func__ );
        break;
    }
}
/****************************************************************************/
 int Atbmwifi_halEntry(struct sbus_priv *sbus)
{ 
	int Status;
	int if_id;
	int i;
	int ret =0;

	struct atbmwifi_common *hw_priv;
	struct wsm_operational_mode mode;

	atbm_memset(&mode, 0, sizeof(struct wsm_operational_mode));
#if SUPPORT_LIGHT_SLEEP
	mode.power_mode = wsm_power_mode_quiescent;
#else
	mode.power_mode = wsm_power_mode_active;
#endif
	mode.disableMoreFlagUsage = ATBM_TRUE;
	wifi_printk(WIFI_ALWAYS,"atbm: Atbmwifi_halEntry() <===\n");	
	/*Init static and global struct */
	atbmwifi_band_2ghz_init();
	net_device_ops_init();

	hw_priv = &g_hw_prv;
	atbm_memset(&g_hw_prv,0 ,sizeof(struct atbmwifi_common));
#if ATBM_USB_BUS
	hw_priv->sbus_ops = &atbm_usb_sbus_ops;
#else	
	hw_priv->sbus_ops = &atbm_sdio_sbus_ops;
#endif
	hw_priv->sbus_priv = sbus;
	sbus->core = hw_priv;
	///////////////////////hw_priv;////////
	/*Atbm work queue Task*/
	atbm_create_workqueue(hw_priv);
	/*Atbm timer Task*/
	atbm_create_timerTask(hw_priv);
	atbm_os_mutexLockInit(&hw_priv->wsm_cmd_mux);
	ATBM_INIT_LIST_HEAD(&hw_priv->event_queue); 
	ATBM_INIT_LIST_HEAD(&hw_priv->tx_urb_cmp); 												\
	atbm_spin_lock_init(&hw_priv->event_queue_lock); 	
	atbm_spin_lock_init(&hw_priv->tx_com_lock); 	
	atbm_spin_lock_init(&hw_priv->rx_com_lock); 	
	atbm_skb_queue_head_init(&hw_priv->rx_frame_queue);
	atbm_skb_queue_head_init(&hw_priv->rx_frame_free);
	atbm_skb_queue_head_init(&hw_priv->tx_frame_queue);
	atbm_skb_queue_head_init(&hw_priv->tx_frame_free);
#if ATBM_SDIO_BUS
	atbm_os_init_waitevent(&hw_priv->wsm_synchanl_done);
	hw_priv->wsm_sync_channl=atbm_init_work(hw_priv, wsm_sync_channl_reset,hw_priv);
#endif

#if BW_40M_SUPPORT
	hw_priv->channel_type = ATBM_NL80211_CHAN_HT40PLUS;
#else
	hw_priv->channel_type = ATBM_NL80211_CHAN_HT20;
#endif
	hw_priv->bands[ATBM_IEEE80211_BAND_2GHZ] = &atbmwifi_band_2ghz;
#if CONFIG_5G_SUPPORT
	hw_priv->bands[ATBM_IEEE80211_BAND_5GHZ] = &atbmwifi_band_5ghz;
#endif

	hw_priv->short_frame_max_tx_count  = TEST_SHORT_RETRY_NUM;
	hw_priv->long_frame_max_tx_count  = TEST_LONG_RETRY_NUM;

#if ATBM_PKG_REORDER
	hw_priv->ba_tid_tx_mask = ATBMWIFI__TX_BLOCK_ACK_ENABLED_FOR_ALL_TID;
#if CONFIG_WPA2_REINSTALL_CERTIFICATION
	//	#if wpa2 reinstall certification need close ampdu,because rx PN reorder
	hw_priv->ba_tid_rx_mask = ATBMWIFI__RX_BLOCK_ACK_DISABLED_FOR_ALL_TID;
#else //CONFIG_WPA2_REINSTALL_CERTIFICATION
	hw_priv->ba_tid_rx_mask = ATBMWIFI__TX_BLOCK_ACK_ENABLED_FOR_ALL_TID;
#endif //CONFIG_WPA2_REINSTALL_CERTIFICATION
#else //ATBM_PKG_REORDER
	hw_priv->ba_tid_tx_mask = ATBMWIFI__RX_BLOCK_ACK_DISABLED_FOR_ALL_TID;
	hw_priv->ba_tid_rx_mask = ATBMWIFI__RX_BLOCK_ACK_DISABLED_FOR_ALL_TID;
#endif //ATBM_PKG_REORDER
	hw_priv->max_rx_aggregation_subframes = 64;//IEEE80211_MAX_AMPDU_BUF;
	hw_priv->max_tx_aggregation_subframes = 64;//IEEE80211_MAX_AMPDU_BUF;
	atbm_spin_lock_init(&hw_priv->aggr_lock);

	hw_priv->wsm_cbc.scan_complete = atbmwifi_scan_complete_cb;
	hw_priv->wsm_cbc.suspend_resume = atbm_suspend_resume;
	/*Queue init*/
	wsm_buf_init(&hw_priv->wsm_cmd_buf);
	atbm_spin_lock_init(&hw_priv->wsm_cmd.lock);
	atbm_os_init_waitevent(&hw_priv->wsm_startup_done);
	/*Register bh*/
	ret=atbm_register_bh(hw_priv);
	if (ret!=0){
		goto AtbmMain_ERR;
	}
    /* check chip type */
    atbm_HwGetChipType(hw_priv);

	/*Set DownLoad firmware Blocksize*/
	SET_SDIO_DOWNLOAD_BLOCKSIZE(hw_priv);
	/*Start download fw*/
	Status=atbm_load_firmware(hw_priv);
	if (Status){
		wifi_printk(WIFI_ALWAYS,"DownLoad FwErr,Pls check\n");
		goto AtbmMain_ERR;
	}
#if ATBM_SDIO_BUS
    SET_SDIO_TRANSMIT_BLOCKSIZE(hw_priv);
#endif
__wait_start_up:
	if (atbm_os_wait_event_timeout(&hw_priv->wsm_startup_done,2*HZ) < 0) {
		if(!hw_priv->wsm_caps.firmwareReady){
			wifi_printk(WIFI_OS,"wait_event_interruptible_timeout wsm_startup_done timeout ERROR !!\n");
			goto AtbmMain_ERR;
		}
	}
	else {
		if(!hw_priv->wsm_caps.firmwareReady){
			wifi_printk(WIFI_DBG_MSG,"atbm: Atbmwifi_halEntry(), FW is not ready(%dms).\n", atbm_GetOsTimeMs());
			goto __wait_start_up;
		}
	}
	atbm_firmware_init_check(hw_priv);
	/*Timer Init*/
	atbm_timer_init(hw_priv);

#if (PROJ_TYPE == ASMLITE)
	atbm_phy_init(hw_priv);
#endif

	/*Queue stats init*/
	if (atbm_unlikely(atbmwifi_queue_stats_init(&hw_priv->tx_queue_stats,
			WLAN_LINK_ID_MAX,
			hw_priv))) {
		ret = -2;
		goto AtbmMain_ERR;
	}

	/*Queue init*/
	hw_priv->vif0_throttle = ATBM_WIFI_MAX_QUEUE_SZ;
	for (i = 0; i < 4; ++i) {
		if (atbm_unlikely(atbmwifi_queue_init(&hw_priv->tx_queue[i],
				&hw_priv->tx_queue_stats, i, ATBM_WIFI_MAX_QUEUE_SZ))) { 
			ret = -3;
			goto AtbmMain_ERR;
		}
	}

	for (if_id = 0; if_id < _atbmwifi_get_nr_hw_ifaces(hw_priv); if_id++) { 
		wsm_set_operational_mode(hw_priv, &mode, if_id);
		/* Enable multi-TX confirmation */
		wsm_use_multi_tx_conf(hw_priv, ATBM_TRUE, if_id);
	}

	/*get mac addr from efuse*/
	atbm_get_mac_address(hw_priv);
	/*Get efuse other Information*/
	atbm_get_efuse_data(hw_priv);

	/*Timer list initial*/
	atbm_wifi_ticks_timer_init();
	/*Intial Support coutry */
	atbmwifi_ieee80211_channel_country(hw_priv,country_chinese);
	/*mac80211 stack control & initial*/
	atbmwifi_netstack_init(hw_priv);
	
	/*Init the two virtul interface,Here need intial two netDev*/
	/*1 Add Station interface */
	atbm_wifi_add_interfaces(hw_priv,ATBM_NL80211_IFTYPE_STATION,"wlan0");
	/*2 Add P2p Interface*/
	atbm_wifi_add_interfaces(hw_priv,ATBM_NL80211_IFTYPE_AP,"p2p0");
	/*MacAddr Set only one times*/
	atbmwifi_setup_mac(hw_priv);

	/* Intial End Open Lmac log*/
	atbmwifi_enable_lmaclog(0);

#if ATBM_SUPPORT_BRIDGE
	atbm_brpool_init(hw_priv);
#endif
#if CONFIG_WIFI_BT_COMB
	ieee80211_ble_dev_int(hw_priv);
#endif
	return ret;
AtbmMain_ERR:
	wifi_printk(WIFI_ALWAYS,"atbm: Atbmwifi_halEntry() <===error\n");
	return ret;
}
atbm_void atbm_core_release(struct atbmwifi_common *hw_priv)
{
	struct atbmwifi_vif *priv;
	int queue_id;
	int i;

	atbm_for_each_vif(hw_priv,priv,i){
		if(priv== NULL){
			continue;
		}

		//tcp_opt->net_disable(priv->ndev);
		/*Flush the queue */
		//__atbm_flush(hw_priv, ATBM_TRUE, priv->if_id, 0xff);
		if(priv->enabled){
			/*Free wpa supplicant & hostapd mem*/
			if(atbmwifi_is_sta_mode(priv->iftype))
				atbmwifi_stop_sta(priv);
			if(atbmwifi_is_ap_mode(priv->iftype))
				atbmwifi_stop_ap(priv);
		}
		/*Free netdev*/
		atbm_wifi_remove_interfaces(priv);
	}

	/*Destory the atbm_bh|*/
	atbm_unregister_bh(hw_priv);
	/*Destory the atbm_workQueue*/
	atbm_destory_task_work(hw_priv);
	/*Destory the atbm_wpa_event*/
	atbmwifi_wpa_event_destory();
	/*Free atbm timer*/
	atbm_timer_free(hw_priv);

	/*Free TxQueue Timer*/
	for (queue_id = 0; queue_id < 4; ++queue_id){
		atbmwifi_queue_deinit(&hw_priv->tx_queue[queue_id]);
	}

	for (i = 0; i < ATBM_WIFI_MAX_VIFS; i++) {
		if (hw_priv->tx_queue_stats.link_map_cache[i]) {
			atbm_kfree(hw_priv->tx_queue_stats.link_map_cache[i]);
			hw_priv->tx_queue_stats.link_map_cache[i] = ATBM_NULL;
		}
	}
	wsm_buf_deinit(&hw_priv->wsm_cmd_buf);
	atbm_os_delete_waitevent(&hw_priv->wsm_synchanl_done);
}
