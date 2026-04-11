/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#ifndef ATBMWIFI_KEY_H
#define ATBMWIFI_KEY_H
#define WEP_IV_LEN		4
#define WEP_ICV_LEN		4
#define ALG_CCMP_KEY_LEN	16
#define CCMP_HDR_LEN		8
#define CCMP_MIC_LEN		8
#define CCMP_TK_LEN		16
#define CCMP_PN_LEN		6
#define TKIP_IV_LEN		8
#define TKIP_ICV_LEN		4
#define CMAC_PN_LEN		6
#define WAPI_IV_LEN		18
#define WAPI_ICV_LEN		16


#define ATBM_INVALID_KEY 0xff
int atbm_get_key(struct atbmwifi_vif *priv,int pairwise,int linkid);
int atbm_get_crypto(struct atbmwifi_vif *priv,int pairwise);

#endif //ATBMWIFI_KEY_H