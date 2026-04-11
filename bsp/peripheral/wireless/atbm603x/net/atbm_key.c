/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#include "atbm_hal.h"


 atbm_void atbmwifi_free_key(struct atbmwifi_common *hw_priv, int idx)
{
	//ATBM_BUG_ON(!(hw_priv->key_map & BIT(idx)));
	atbm_memset(&hw_priv->keys[idx], 0, sizeof(hw_priv->keys[idx]));
	hw_priv->key_map &= ~BIT(idx);
}
 static atbm_uint8 atbmwifi_crypto_2_type(atbm_uint32 crypto,atbm_uint8 frame_type)    
{
	atbm_uint8 type = 0xff;

	
	switch(crypto)
	{
		case ATBM_WLAN_CIPHER_SUITE_WEP40:
		case ATBM_WLAN_CIPHER_SUITE_WEP104:
		//if(pairwise)
		//	type = WSM_KEY_TYPE_WEP_PAIRWISE;
	//	else
			type = WSM_KEY_TYPE_WEP_DEFAULT;
		break;

		case ATBM_WLAN_CIPHER_SUITE_TKIP:
			if(frame_type == 0)
				type = WSM_KEY_TYPE_TKIP_PAIRWISE;
			else
				type = WSM_KEY_TYPE_TKIP_GROUP;

			break;
		case ATBM_WLAN_CIPHER_SUITE_CCMP:
			if(frame_type == 0)
				type = WSM_KEY_TYPE_AES_PAIRWISE;
			else
				type = WSM_KEY_TYPE_AES_GROUP;
			break;
		case ATBM_WLAN_CIPHER_SUITE_AES_CMAC:
				type = WSM_KEY_TYPE_IGTK_GROUP;
			break;
		default:
			break;
			
	}

	return type;
}
int atbm_get_crypto(struct atbmwifi_vif *priv,int frame_type)
{
	if(frame_type == 0){
		return priv->connect.crypto_pairwise;
	}
	else if(frame_type == 1){
		return priv->connect.crypto_group;
#if CONFIG_IEEE80211W
	}else if(frame_type == 2){
		return priv->connect.crypto_igtkgroup;
#endif
	}else{
		return -1;
	}	
}
int atbm_get_key(struct atbmwifi_vif *priv,int frame_type,int linkid)
{
	struct atbmwifi_key_t *keybuff = ATBM_NULL;
	atbm_uint8 key_type = 0; 
	atbm_uint8	empty = ATBM_INVALID_KEY;
	int  index = 0;
	key_type = atbmwifi_crypto_2_type(atbm_get_crypto(priv,frame_type),frame_type);
	//wifi_printk(WIFI_CONNECT,"----->key_type %x\n",key_type);
	if(key_type == 0xff){
		return ATBM_INVALID_KEY;
	}
	if(key_type == WSM_KEY_TYPE_WEP_DEFAULT){
		frame_type = 1;
	}
	while(index<WSM_KEY_MAX_INDEX)
	{		
		keybuff = &priv->hw_priv->keys[index];
		if(keybuff->valid==1){
			if(frame_type == 0){
				if((priv->if_id == keybuff->if_id)
					&&(linkid == keybuff->linkid)
					&&(keybuff->key.type == key_type)){
					empty = index;
					break;
				}
			}
			else {
				if((priv->if_id == keybuff->if_id)
					&&(keybuff->key.type == key_type)){
					empty = index;
					break;
				}
			}
		}
		index++;
	}
	return empty;

}
//#ifdef CONFIG_SUPPORT_KEY 
 struct wsm_add_key *atbmwifi_get_key_buff(struct atbmwifi_vif *priv,int frame_type,int linkid)
{
	struct atbmwifi_key_t *keybuff = ATBM_NULL;
	
	atbm_uint8 key_type = 0; 
	int  index = 0;
	atbm_uint8  empty = ATBM_INVALID_KEY;
	
	key_type = atbmwifi_crypto_2_type(atbm_get_crypto(priv,frame_type),frame_type);
	empty=atbm_get_key(priv,frame_type, linkid);
	if(empty != ATBM_INVALID_KEY){
		keybuff = &priv->hw_priv->keys[empty];
		wifi_printk(WIFI_ALWAYS,"<WARNING>atbmwifi_get_key_buff updata key!!!!\n");
		return &keybuff->key;
	}
		
	while(index<WSM_KEY_MAX_INDEX)
	{		
		keybuff = &priv->hw_priv->keys[index];
		if(keybuff->valid==0){
			empty = index;
			break;
		}
		index++;
	}

	if(empty != ATBM_INVALID_KEY)
	{
		keybuff = &priv->hw_priv->keys[empty];
		keybuff->key.type = key_type;
		keybuff->key.entryIndex = empty;
		priv->hw_priv->key_map |= BIT(empty);
		keybuff->valid= 1;
		keybuff->if_id= priv->if_id;
		keybuff->linkid= linkid;
	}
	else {
		keybuff=ATBM_NULL;
		wifi_printk(WIFI_ALWAYS,"<ERROR>atbmwifi_get_key_buff  key FULL!!!!\n");
		return ATBM_NULL;
	}
	
	return &keybuff->key;
}

 int atbmwifi_set_key(struct atbmwifi_vif *priv,int frame_type,int linkid)
{
	int ret = -ATBM_EOPNOTSUPP;
	struct atbmwifi_common *hw_priv = priv->hw_priv;
	atbm_uint8 *peer_addr = ATBM_NULL;	
	
	wifi_printk(WIFI_DBG_MSG,"wsm_key linkid  (%d) (%d)\n",frame_type,linkid);
	if(atbmwifi_is_sta_mode(priv->iftype)){
		linkid = 0;
	}	
	else if(atbmwifi_is_ap_mode(priv->iftype)){
		if((linkid == 0) || (linkid> ATBMWIFI__MAX_STA_IN_AP_MODE))
		{
			wifi_printk(WIFI_DBG_MSG,"wsm_key linkid err (%d)\n",linkid);
			return ret;
		}
	}
	
	{		
 		struct wsm_add_key *wsm_key = atbmwifi_get_key_buff(priv,frame_type,linkid); //= &hw_priv->keys[idx];
		struct atbmwifi_sta_priv * sta_priv = atbmwifi_sta_find_form_hard_linkid(priv,linkid);

		if(wsm_key == ATBM_NULL)
		{
			wifi_printk(WIFI_DBG_MSG,"wsm_key get err\n");
			return ret;
		}
		if(atbmwifi_is_sta_mode(priv->iftype))
			peer_addr=priv->daddr;
		else 			
			peer_addr=sta_priv->mac;

		wifi_printk(WIFI_DBG_MSG,"wsm_key->entryIndex(%d)\n",wsm_key->entryIndex);
		switch (atbm_get_crypto(priv,frame_type)) {
			case ATBM_WLAN_CIPHER_SUITE_WEP40:
			case ATBM_WLAN_CIPHER_SUITE_WEP104:
 				if (priv->connect.key_len> 16) {
					atbmwifi_free_key(hw_priv, wsm_key->entryIndex);
					ret = -ATBM_EINVAL;
						return ret;
				}
				if (frame_type == 0) {
 					wsm_key->type = WSM_KEY_TYPE_WEP_PAIRWISE;
					atbm_memcpy(wsm_key->wepPairwiseKey.peerAddress,peer_addr, ATBM_ETH_ALEN);
					atbm_memcpy(wsm_key->wepPairwiseKey.keyData,
					&priv->connect.key[0], priv->connect.key_len);
					wsm_key->wepPairwiseKey.keyLength = priv->connect.key_len;
				} else {
					wsm_key->type = WSM_KEY_TYPE_WEP_DEFAULT;
					atbm_memcpy(wsm_key->wepGroupKey.keyData,
					&priv->connect.key[0], priv->connect.key_len);
					wsm_key->wepGroupKey.keyLength = priv->connect.key_len;
					wsm_key->wepGroupKey.keyId = priv->connect.key_idx;
				}
				break;
			case ATBM_WLAN_CIPHER_SUITE_TKIP:
				if (frame_type == 0) {
					wifi_printk(WIFI_DBG_MSG,"atbmwifi_set_key:p tkip\n");
					wsm_key->type = WSM_KEY_TYPE_TKIP_PAIRWISE;
					atbm_memcpy(wsm_key->tkipPairwiseKey.peerAddress,
						peer_addr, ATBM_ETH_ALEN);
					atbm_memcpy(wsm_key->tkipPairwiseKey.tkipKeyData,
						&priv->connect.key[0],  16);
					atbm_memcpy(wsm_key->tkipPairwiseKey.txMicKey,
						&priv->connect.key[16],	8);
					atbm_memcpy(wsm_key->tkipPairwiseKey.rxMicKey,
						&priv->connect.key[24],	8);
				} else {
					atbm_uint32 mic_offset =16;
						//((priv->iftype == ATBM_NL80211_IFTYPE_AP) ?
						//16 : 24);
					wsm_key->type = WSM_KEY_TYPE_TKIP_GROUP;
					atbm_memcpy(wsm_key->tkipGroupKey.tkipKeyData,
						&priv->connect.key[0],  16);
					atbm_memcpy(wsm_key->tkipGroupKey.rxMicKey,
						&priv->connect.key[mic_offset],	8);
			
					/* TODO: Where can I find TKIP SEQ? */
					atbm_memset(wsm_key->tkipGroupKey.rxSeqCounter,
						0,		8);
					wsm_key->tkipGroupKey.keyId = priv->connect.key_idx;
/*
					if(priv->iftype == ATBM_NL80211_IFTYPE_AP)
					wifi_printk(WIFI_DBG_MSG, 
					"gtk[16](0x%x),gtk[17](0x%x),gtk[18](0x%x),gtk[19](0x%x),\n"
					"gtk[20](0x%x),gtk[21](0x%x),gtk[22](0x%x),gtk[23](0x%x)\n",
					priv->connect.key[16],priv->connect.key[17],priv->connect.key[18],priv->connect.key[19],
					priv->connect.key[20],priv->connect.key[21],priv->connect.key[22],priv->connect.key[23]);
					else
					wifi_printk(WIFI_DBG_MSG, 
					"gtk[24](0x%x),gtk[25](0x%x),gtk[26](0x%x),gtk[27](0x%x),\n"
					"gtk[28](0x%x),gtk[29](0x%x),gtk[30](0x%x),gtk[31](0x%x)\n",
					priv->connect.key[24],priv->connect.key[25],priv->connect.key[26],priv->connect.key[27],
					priv->connect.key[28],priv->connect.key[29],priv->connect.key[30],priv->connect.key[31]);
*/
				}
				break;
			case ATBM_WLAN_CIPHER_SUITE_CCMP:
				if (frame_type == 0) {
//					wifi_printk(WIFI_DBG_MSG,"atbmwifi_set_key:p ccmp\n");
					wsm_key->type = WSM_KEY_TYPE_AES_PAIRWISE;
					atbm_memcpy(wsm_key->aesPairwiseKey.peerAddress,
						peer_addr, ATBM_ETH_ALEN);
					atbm_memcpy(wsm_key->aesPairwiseKey.aesKeyData,
						&priv->connect.key[0],  16);
				} else {
					wifi_printk(WIFI_DBG_MSG,"atbmwifi_set_key:g ccmp\n");
					wsm_key->type = WSM_KEY_TYPE_AES_GROUP;
					atbm_memcpy(wsm_key->aesGroupKey.aesKeyData,
						&priv->connect.key[0],  16);
					/* TODO: Where can I find AES SEQ? */
					atbm_memset(wsm_key->aesGroupKey.rxSeqCounter,
						0,				8);
					wsm_key->aesGroupKey.keyId = priv->connect.key_idx;
/*
					wifi_printk(WIFI_DBG_MSG, "gtk[0](0x%x),gtk[5](0x%x),gtk[10](0x%x),gtk[15](0x%x),gtk[30](0x%x)\n",
					priv->connect.key[0],priv->connect.key[5],priv->connect.key[10],priv->connect.key[15],
					priv->connect.key[30]);
*/
				}
				break;
#if CONFIG_IEEE80211W
			case ATBM_WLAN_CIPHER_SUITE_AES_CMAC: /*add 11W support*/
				{
					struct wsm_protected_mgmt_policy mgmt_policy;
					mgmt_policy.protectedMgmtEnable = 1;
					mgmt_policy.unprotectedMgmtFramesAllowed = 1;
					mgmt_policy.encryptionForAuthFrame = 1;
					wsm_set_protected_mgmt_policy(hw_priv, &mgmt_policy,
							  priv->if_id);
					wifi_printk(WIFI_ALWAYS, "WLAN_CIPHER_SUITE_AES_CMAC,index(%d)\n", priv->connect.key_idx);
					wsm_key->type = WSM_KEY_TYPE_IGTK_GROUP;
					atbm_memcpy(wsm_key->igtkGroupKey.igtKeyData, &priv->connect.key[0], 16);
					atbm_memset(wsm_key->igtkGroupKey.ipn, 0, 8);
					wsm_key->igtkGroupKey.keyId = priv->connect.key_idx_igtk;
				}
				break;
#endif
			default:
				ATBM_WARN_ON_FUNC(1);
				atbmwifi_free_key(hw_priv, wsm_key->entryIndex);
				ret = -ATBM_EOPNOTSUPP;
				return ret;
		}
		ret = wsm_add_key(hw_priv, wsm_key, priv->if_id);
		if (!ret)
		{
			wifi_printk(WIFI_DBG_MSG,"atbmwifi_set_key ok\n");
		}
		else
		{
			wifi_printk(WIFI_DBG_MSG,"atbmwifi_set_key err\n");
		}
	}
	return ret;
}

atbm_void atbmwifi_del_key(struct atbmwifi_vif *priv,int frame_type,int linkid)
{	
	int entryIndex = 0;
	
	if(atbmwifi_is_sta_mode(priv->iftype)){
		linkid = 0;
	}	
	else if(atbmwifi_is_ap_mode(priv->iftype)){
		if((linkid == 0) && (linkid> ATBMWIFI__MAX_STA_IN_AP_MODE))
		{
			wifi_printk(WIFI_DBG_ERROR,"atbmwifi_del_key linkid err (%d)\n",linkid);
			return ;
		}
	}

	///////////
	entryIndex = atbm_get_key(priv, frame_type,linkid);
	if(entryIndex == ATBM_INVALID_KEY){
		wifi_printk(WIFI_DBG_ERROR,"atbmwifi_del_key  err (%d %d)\n",linkid,frame_type);
		return;
	}
	wsm_remove_key(priv->hw_priv, &priv->hw_priv->keys[entryIndex].key, priv->if_id);	

	atbmwifi_free_key(priv->hw_priv, entryIndex);

}
//#endif

 atbm_void atbmwifi_wep_key_work(struct atbmwifi_vif *priv)
{
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	atbm_uint8 wep_default_key_id = priv->connect.key_idx;
	wifi_printk(WIFI_TX,"[STA] Setting default WEP key: %d\n",
		wep_default_key_id);
	wsm_write_mib(hw_priv, WSM_MIB_ID_DOT11_WEP_DEFAULT_KEY_ID,
		&wep_default_key_id, sizeof(wep_default_key_id), priv->if_id);
}

