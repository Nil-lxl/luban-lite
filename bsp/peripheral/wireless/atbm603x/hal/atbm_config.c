/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#include "atbm_hal.h"
extern struct atbmwifi_common g_hw_prv;
static atbm_int32 Rssi_last = 0;
static atbm_int32 Rssi_aver = 0;

 atbm_int32 atbmwifi_get_rssi_avg(atbm_void)
{
	return -(Rssi_aver >> 10);
}

atbm_int32 atbmwifi_get_rssi(atbm_void)
 {
	 return Rssi_last;
 }

atbm_void atbmwifi_set_rssi(atbm_int8 rssi)
{
	Rssi_last = rssi;
	Rssi_aver = Rssi_aver ? ((Rssi_aver << 3) -  Rssi_aver + ((-rssi) << 10)) >> 3 : (-rssi) << 10;
}
/****************************************************
Function Name: atbmwifi_set_tx_time
Parameters:
		time_period: transmit period
		time_transmit: transmit time in the period
Return: Success(0), Error(<0)
******************************************************/
 atbm_int32 atbmwifi_set_tx_time(atbm_uint32 time_period, atbm_uint32 time_transmit)
{
	atbm_int32 ret = 0;
	char cmd[32];
	atbm_uint8 if_id;	
	struct atbmwifi_vif *priv=ATBM_NULL;
	struct atbmwifi_common *hw_priv=ATBM_NULL;
	atbm_for_each_vif(&g_hw_prv,priv,if_id){
		if(priv== NULL){
			continue;
		}
		if(time_transmit > time_period){
			wifi_printk(WIFI_DBG_ERROR, "atbmwifi_set_tx_time(), error, trans = %d, period = %d\n", time_transmit, time_period);
			return -ATBM_EINVAL;
		}
		hw_priv	= _atbmwifi_vifpriv_to_hwpriv(priv);

		atbm_memset(cmd, 0, sizeof(cmd));
		sprintf(cmd,"set_tx_time %d,%d ", time_period, time_transmit);
		ret = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,cmd, strlen(cmd), priv->if_id);
	}
	return ret;
}
/****************************************************
Function Name: atbmwifi_set_retry
Parameters:
		retry_num: max retry transmit number
		retry_time_ms: max retry transnit time
Return: Success(0), Error(<0)
******************************************************/
 atbm_int32 atbmwifi_set_retry(atbm_uint32 retry_num, atbm_uint32 retry_time_ms)
{
	atbm_int32 ret = 0;
	char cmd[32];
	atbm_uint8 if_id;	
	struct atbmwifi_vif *priv=ATBM_NULL;
	struct atbmwifi_common *hw_priv=ATBM_NULL;
	atbm_for_each_vif(&g_hw_prv,priv,if_id){
		if(priv== NULL){
			continue;
		}
		if(retry_num >= 200){
			wifi_printk(WIFI_DBG_ERROR, "atbmwifi_set_retry(), error, retry_num %d\n", retry_num);
		}

		if(retry_time_ms > 1000){
			wifi_printk(WIFI_DBG_ERROR, "atbmwifi_set_retry(), error, retry_time_ms %d\n", retry_time_ms);
		}
		hw_priv	= _atbmwifi_vifpriv_to_hwpriv(priv);
		atbm_memset(cmd, 0, sizeof(cmd));
		sprintf(cmd,"set_retry %d,%d ", retry_num, retry_time_ms);
		ret = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,cmd, strlen(cmd), priv->if_id);
	}
	return ret;
}

/****************************************************
Function Name: atbmwifi_set_txpower
Parameters:
		txpower_idx: [0:12]  -> [-3:0.5:3]dB -> [-30:5:30]/10 dB
Return: Success(0), Error(<0)
******************************************************/
 atbm_int32 atbmwifi_set_txpower(atbm_uint32 txpower_idx)
{
	atbm_int32 ret = 0;
	char cmd[32];
	struct atbmwifi_vif *priv=ATBM_NULL;
	struct atbmwifi_common *hw_priv=ATBM_NULL;
	atbm_uint8 if_id;
	atbm_for_each_vif(&g_hw_prv,priv,if_id){	
		if(priv== NULL){
			continue;
		}
		if(txpower_idx > 12){
			wifi_printk(WIFI_DBG_ERROR, "atbmwifi_set_txpower(), error, txpower_idx %d\n", txpower_idx);
		}
		hw_priv	= _atbmwifi_vifpriv_to_hwpriv(priv);
		atbm_memset(cmd, 0, sizeof(cmd));
		sprintf(cmd,"set_txpower %d ", txpower_idx);
		ret = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,cmd, strlen(cmd), priv->if_id);
	}
	return ret;
}

/****************************************************
Function Name: atbm_dev_set_adaptive
Parameters:		
0: disable
1: enable
Return: Success(0), Error(<0)
******************************************************/
atbm_int32 atbmwifi_dev_set_adaptive(atbm_uint32 adaptive)
{
    int ret = 0;
    char cmd[32];
	struct atbmwifi_vif *priv=ATBM_NULL;
	struct atbmwifi_common *hw_priv=ATBM_NULL;
	atbm_uint8 if_id;
    /*
    0: disable
    1: enable
    */	
	atbm_for_each_vif(&g_hw_prv,priv,if_id){	
	    if(priv== NULL){
			continue;
		}
		hw_priv	= _atbmwifi_vifpriv_to_hwpriv(priv);
	    atbm_memset(cmd, 0, sizeof(cmd));
	    sprintf(cmd, "set_adaptive %d ", adaptive);
	    
	    ret = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD, cmd, strlen(cmd), priv->if_id);
	}
    return ret;

}

/****************************************************
Function Name: atbmwifi_tx_rate_set
Parameters:
		rate: (11b)->10,20,55,110
			  (11g)->60,90,120,180,240,360,480,540
			  (11n)->65,130,195,260,390,520,585,650
			  Reset->0
Return: Success(0), Error(<0)
******************************************************/
atbm_int32 globle_rate = 0;
 atbm_int32 atbmwifi_set_tx_rate(atbm_int32 rate)
{
	atbm_int32 ret = 0;
	char cmd[32];
	atbm_uint8 if_id;
	struct atbmwifi_vif *priv=ATBM_NULL;
	struct atbmwifi_common *hw_priv=ATBM_NULL;

	atbm_memset(cmd, 0, sizeof(cmd));
	switch(rate){
		//11b
		case 10:
		case 20:
		case 55:
		case 110:
			globle_rate = RATE_INDEX_B_11M;
			break;
		//11g
		case 60:
		case 90:
		case 120:
		case 180:
		case 240:
		case 360:
		case 480:
		case 540:
			globle_rate = RATE_INDEX_A_54M;
			break;

		//11n
		case 65:
		case 130:
		case 195:
		case 260:
		case 390:
		case 520:
		case 585:
		case 650:
			globle_rate = RATE_INDEX_N_65M;
			break;

		//not force rate
		case 0:
			globle_rate = 0;
			break;

		default:
			wifi_printk(WIFI_DBG_ERROR, "atbm: invalid rate(%d)\n", rate);
			wifi_printk(WIFI_DBG_ERROR, "please enter: 10,20,55,110(11b)\n");
			wifi_printk(WIFI_DBG_ERROR, "              60,90,120,180,240,360,480,540(11g)\n");
			wifi_printk(WIFI_DBG_ERROR, "              65,130,195,260,390,520,585,650(11n)\n");
			return -ATBM_EINVAL;
	}
	
	atbm_for_each_vif(&g_hw_prv,priv,if_id){	
		if(priv== NULL){
			continue;
		}
		hw_priv	= _atbmwifi_vifpriv_to_hwpriv(priv);
		sprintf(cmd,"lmac_rate %d ", rate);
		ret = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,cmd, strlen(cmd), priv->if_id);
	}
	return ret;
}

/****************************************************
Function Name: atbmwifi_set_sgi
Parameters:
		sgi: short GI
Return: Success(0), Error(<0)
******************************************************/
 atbm_int32 atbmwifi_set_sgi(atbm_uint32 sgi)
{
	atbm_int32 ret = 0;
	char cmd[32];
	atbm_uint8 if_id;
	struct atbmwifi_vif *priv=ATBM_NULL;
	struct atbmwifi_common *hw_priv=ATBM_NULL;

	atbm_memset(cmd, 0, sizeof(cmd));
	atbm_for_each_vif(&g_hw_prv,priv,if_id){
		if(priv== NULL){
			continue;
		}
		hw_priv	= _atbmwifi_vifpriv_to_hwpriv(priv);
		sprintf(cmd,"lmac_sgi %d ", sgi);
		ret = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,cmd, strlen(cmd), priv->if_id);
	}
	return ret;
}

/****************************************************
Function Name: atbmwifi_set_rate_txpower_mode
Parameters:
		txpower_idx: [-16:16] -> [-8:0.5:8]dB -> [-80:5:80]/10 dB
Return: Success(0), Error(<0)
******************************************************/
 atbm_int32 atbmwifi_set_rate_txpower_mode(atbm_int32 txpower_idx)
{
	atbm_int32 ret = 0;
	char cmd[32];
	atbm_uint8 if_id;
	struct atbmwifi_vif *priv=ATBM_NULL;
	struct atbmwifi_common *hw_priv=ATBM_NULL;
	atbm_memset(cmd, 0, sizeof(cmd));
	atbm_for_each_vif(&g_hw_prv,priv,if_id){
		if(priv== NULL){
			continue;
		}
		hw_priv	= _atbmwifi_vifpriv_to_hwpriv(priv);
		sprintf(cmd,"set_rate_txpower_mode %d ", txpower_idx);
		ret = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,cmd, strlen(cmd), priv->if_id);
	}	
	return ret;
}

atbm_int32 atbmwifi_get_lmacLog_to_host(atbm_uint32 value){
	
	atbm_uint8 ucDbgPrintOpenFlag = 0;
	atbm_uint8 ret = 0;
	atbm_uint8 if_id;
	struct atbmwifi_common *hw_priv=ATBM_NULL;
	struct atbmwifi_vif *priv=ATBM_NULL;
	ucDbgPrintOpenFlag =value;
	atbm_for_each_vif(&g_hw_prv,priv,if_id){
		if(priv== NULL){
			continue;
		}
		hw_priv	= _atbmwifi_vifpriv_to_hwpriv(priv);
		ret=wsm_write_mib(hw_priv, WSM_MIB_ID_DBG_PRINT_TO_HOST,
			&ucDbgPrintOpenFlag, sizeof(ucDbgPrintOpenFlag), priv->if_id);
	}
	return ret;
}
/* TODO:COMBO:UAPSD will be supported only on one interface */
 int atbmwifi_set_uapsd_param(struct atbmwifi_vif *priv,
				const struct wsm_edca_params *arg)
{
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	struct wsm_uapsd_info		uapsd_info={0};
	int ret;
	atbm_uint16 uapsdFlags = 0;

	/* Here's the mapping AC [queue, bit]
	VO [0,3], VI [1, 2], BE [2, 1], BK [3, 0]*/

	if (arg->params[0].uapsdEnable)
		uapsdFlags |= 1 << 3;

	if (arg->params[1].uapsdEnable)
		uapsdFlags |= 1 << 2;

	if (arg->params[2].uapsdEnable)
		uapsdFlags |= 1 << 1;

	if (arg->params[3].uapsdEnable)
		uapsdFlags |= 1;

	/* Currently pseudo U-APSD operation is not supported, so setting
	* MinAutoTriggerInterval, MaxAutoTriggerInterval and
	* AutoTriggerStep to 0 */

	uapsd_info.uapsdFlags = atbm_cpu_to_le16(uapsdFlags);
	uapsd_info.minAutoTriggerInterval = 0;
	uapsd_info.maxAutoTriggerInterval = 0;
	uapsd_info.autoTriggerStep = 0;
	ret = wsm_set_uapsd_info(hw_priv, &uapsd_info,
				 priv->if_id);
	
	priv->uapsd_info.uapsdFlags=uapsd_info.uapsdFlags;
	return ret;
}
 static  atbm_void __atbmwifi_bf_configure(struct atbmwifi_vif *priv,struct wsm_beacon_filter_table *bf_table)
{
	bf_table->numOfIEs = atbm_cpu_to_le32(3);
	bf_table->entry[0].ieId = ATBM_WLAN_EID_VENDOR_SPECIFIC;
	bf_table->entry[0].actionFlags = WSM_BEACON_FILTER_IE_HAS_CHANGED |
					WSM_BEACON_FILTER_IE_NO_LONGER_PRESENT |
					WSM_BEACON_FILTER_IE_HAS_APPEARED;
	bf_table->entry[0].oui[0] = 0x50;
	bf_table->entry[0].oui[1] = 0x6F;
	bf_table->entry[0].oui[2] = 0x9A;

	bf_table->entry[1].ieId = ATBM_WLAN_EID_ERP_INFO;
	bf_table->entry[1].actionFlags = WSM_BEACON_FILTER_IE_HAS_CHANGED |
					WSM_BEACON_FILTER_IE_NO_LONGER_PRESENT |
					WSM_BEACON_FILTER_IE_HAS_APPEARED;

	bf_table->entry[2].ieId = ATBM_WLAN_EID_HT_OPERATION;
	bf_table->entry[2].actionFlags = WSM_BEACON_FILTER_IE_HAS_CHANGED |
					WSM_BEACON_FILTER_IE_NO_LONGER_PRESENT |
					WSM_BEACON_FILTER_IE_HAS_APPEARED;

}
 int atbmwifi_setup_mac(struct atbmwifi_common *hw_priv)
{
	int ret = 0;
	struct wsm_configuration cfg={0};
	cfg.dot11MaxTransmitMsduLifeTime = 512;
	cfg.dot11MaxReceiveLifeTime =512;
	cfg.dot11RtsThreshold =1000;
	cfg.dot11StationId = (atbm_uint8*)&hw_priv->addresses[0];
	/* Set low-power mode. */
	ret |= (wsm_configuration(hw_priv, &cfg, 0));
	return 0;
}

 int atbmwifi_vif_setup(struct atbmwifi_vif *priv)
{
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	struct wsm_edca_params		edca;
	struct config_edca_params	*wmm_param;
	int ret = 0;
	/* default EDCA */
	//VO
	wmm_param= &priv->wmm_params[ATBM_D11_ACI_AC_VO];
	WSM_EDCA_SET(&edca, ATBM_IEEE80211_AC_VO /*0*/, wmm_param->aifns, wmm_param->cwMin, wmm_param->cwMax,
			wmm_param->txOpLimit, 0xc8, wmm_param->uapsdEnable); 
	//VI
	wmm_param= &priv->wmm_params[ATBM_D11_ACI_AC_VI];
	WSM_EDCA_SET(&edca, ATBM_IEEE80211_AC_VI /*1*/, wmm_param->aifns, wmm_param->cwMin, wmm_param->cwMax,
			wmm_param->txOpLimit, 0xc8, wmm_param->uapsdEnable);
	//BE
	wmm_param= &priv->wmm_params[ATBM_D11_ACI_AC_BE];
	WSM_EDCA_SET(&edca, ATBM_IEEE80211_AC_BE /*2*/, wmm_param->aifns, wmm_param->cwMin, wmm_param->cwMax,
			wmm_param->txOpLimit, 0xc8,  wmm_param->uapsdEnable);
	//BK
	wmm_param= &priv->wmm_params[ATBM_D11_ACI_AC_BK];
	WSM_EDCA_SET(&edca, ATBM_IEEE80211_AC_BK /*3*/, wmm_param->aifns, wmm_param->cwMin, wmm_param->cwMax,
			wmm_param->txOpLimit, 0xc8,  wmm_param->uapsdEnable);

	ret = wsm_set_edca_params(hw_priv, &edca, priv->if_id);
	if (ATBM_WARN_ON(ret))
		goto out;

	ret = atbmwifi_set_uapsd_param(priv, &edca);
	if (ATBM_WARN_ON(ret))
		goto out;
	priv->wep_default_key_id = -1;
	priv->bf_control.enabled = WSM_BEACON_FILTER_ENABLE;
out:
	return ret;
}

 int atbmwifi_setup_mac_pvif(struct atbmwifi_vif *priv)
{
	int ret = 0;
	/* NOTE: There is a bug in FW: it reports signal
	* as RSSI if RSSI subscription is enabled.
	* It's not enough to set WSM_RCPI_RSSI_USE_RSSI. */
	/* NOTE2: RSSI based reports have been switched to RCPI, since
	* FW has a bug and RSSI reported values are not stable,
	* what can leads to signal level oscilations in user-end applications */
	struct wsm_rcpi_rssi_threshold threshold={0};
	threshold.rssiRcpiMode = WSM_RCPI_RSSI_THRESHOLD_ENABLE | WSM_RCPI_RSSI_DONT_USE_UPPER |WSM_RCPI_RSSI_DONT_USE_LOWER;
	threshold.rollingAverageCount = 16;
	/* Remember the decission here to make sure, we will handle
	 * the RCPI/RSSI value correctly on WSM_EVENT_RCPI_RSS */
	if (threshold.rssiRcpiMode & WSM_RCPI_RSSI_USE_RSSI)
		priv->cqm_use_rssi = ATBM_TRUE;


	/* Configure RSSI/SCPI reporting as RSSI. */
#ifdef P2P_MULTIVIF
	ret = wsm_set_rcpi_rssi_threshold(priv->hw_priv, &threshold,
					priv->if_id ? 1 : 0);
#else
	ret = wsm_set_rcpi_rssi_threshold(priv->hw_priv, &threshold,
					priv->if_id);
#endif
	return ret;
}


 atbm_void atbmwifi_update_filtering(struct atbmwifi_vif *priv)
{
	int ret;
	ATBM_BOOL ap_mode = 0;
	ATBM_BOOL bssid_filtering = !priv->rx_filter.bssid;
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	struct wsm_beacon_filter_control bf_disabled={0};
	struct wsm_beacon_filter_table bf_table_auto={0};
	struct wsm_beacon_filter_control bf_auto={0};
	bf_disabled.enabled = 0;
	bf_disabled.bcn_count = 1;
	bf_table_auto.numOfIEs = atbm_cpu_to_le32(2);
	bf_table_auto.entry[0].ieId = ATBM_WLAN_EID_VENDOR_SPECIFIC;
	bf_table_auto.entry[0].actionFlags = WSM_BEACON_FILTER_IE_HAS_CHANGED |
				WSM_BEACON_FILTER_IE_NO_LONGER_PRESENT |
				WSM_BEACON_FILTER_IE_HAS_APPEARED;
	bf_table_auto.entry[0].oui[0] = 0x50;
	bf_table_auto.entry[0].oui[1] = 0x6F;
	bf_table_auto.entry[0].oui[2] = 0x9A;

	bf_table_auto.entry[1].ieId = ATBM_WLAN_EID_HT_OPERATION;
	bf_table_auto.entry[1].actionFlags = WSM_BEACON_FILTER_IE_HAS_CHANGED |
				WSM_BEACON_FILTER_IE_NO_LONGER_PRESENT |
				WSM_BEACON_FILTER_IE_HAS_APPEARED;
	bf_auto.enabled = WSM_BEACON_FILTER_ENABLE |
		WSM_BEACON_FILTER_AUTO_ERP;
	bf_auto.bcn_count = 1;
	bf_auto.bcn_count = priv->bf_control.bcn_count;

	if (priv->join_status == ATBMWIFI__JOIN_STATUS_PASSIVE)
		return;
	else if (priv->join_status == ATBMWIFI__JOIN_STATUS_MONITOR)
		bssid_filtering = ATBM_FALSE;

	if (atbmwifi_is_ap_mode(priv->iftype))
		ap_mode = ATBM_TRUE;
	/*
	* When acting as p2p client being connected to p2p GO, in order to
	* receive frames from a different p2p device, turn off bssid filter.
	*
	* WARNING: FW dependency!
	* This can only be used with FW WSM371 and its successors.
	* In that FW version even with bssid filter turned off,
	* device will block most of the unwanted frames.
	*/
	//if (priv->vif && priv->vif->p2p)
		//bssid_filtering = ATBM_FALSE;

	ret = wsm_set_rx_filter(hw_priv, &priv->rx_filter, priv->if_id);
	if (!ret && !ap_mode) {
		if (ATBM_NL80211_IFTYPE_STATION != priv->iftype){
			__atbmwifi_bf_configure(priv,&bf_table_auto);
		}
		ret = wsm_set_beacon_filter_table(hw_priv, &bf_table_auto,
							priv->if_id);
		
	}
	if (!ret && !ap_mode) {
		if (priv->disable_beacon_filter)
			ret = wsm_beacon_filter_control(hw_priv,
					&bf_disabled, priv->if_id);
		else {
			if (ATBM_NL80211_IFTYPE_STATION != priv->iftype)
				ret = wsm_beacon_filter_control(hw_priv,
					&priv->bf_control, priv->if_id);
			else
				ret = wsm_beacon_filter_control(hw_priv,
					&bf_auto, priv->if_id);
		}
	}

	if (!ret)
		ret = wsm_set_bssid_filtering(hw_priv, bssid_filtering,
					priv->if_id);
	if (ret)
		wifi_printk(WIFI_DBG_ERROR,"%s:fail=%d.\n",
				__FUNCTION__, ret);
	return;
}

