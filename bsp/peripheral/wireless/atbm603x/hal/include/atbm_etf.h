/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#ifndef __ATBM_ETF_H__
#define __ATBM_ETF_H__


//keep it off under normal conditions
#define CONFIG_ATBM_PRODUCT_TEST_USE_GOLDEN_LED (1)//atbm test not use uart

#define SIGMASTAR_PRODUCT_TEST_USE_FEATURE_ID		(1)


enum ATBM_TEST_TYPE{
	TXRX_TEST_REQ,
	TXRX_TEST_RSP,
	TXRX_TEST_RESULT,
};

enum ATBM_TEST_RESULT{
	TXRX_TEST_NONE,
	TXRX_TEST_PASS,
	TXRX_TEST_FAIL,
};


struct ATBM_TEST_IE {
	atbm_uint8 ie_id;//D11_WIFI_ELT_ID
	atbm_uint8 len;
	atbm_uint8 oui[3]; //ATBM_OUI
	atbm_uint8 oui_type; //WIFI_ATBM_IE_OUI_TYPE
	atbm_uint8 test_type;
	atbm_uint8 resverd;	
	atbm_uint32 featureid;
	int result[16];
}__attribute__ ((packed));

struct test_threshold{
	int freq_ppm;
	int txevm;//txevm filter
	int rxevm;//send to lmac,rxevm filter
	int txevmthreshold;//test threshold
	int rxevmthreshold;//test threshold
	int txpwrmax;
	int txpwrmin;
	int rxpwrmax;
	int rxpwrmin;
	atbm_uint32 featureid;
	int rssifilter;
	int cableloss;
};

/* OUI's */
#define WIFI_OUI			{0x00, 0x50, 0xF2}
#define WFA_OUI			 {0x50, 0x6F, 0x9A}
#define ATBM_OUI			{0x00, 0xAA, 0xBB}
#define WIFI_WPA_OUI_TYPE	       0x01
#define WIFI_WME_OUI_TYPE	       0x02
#define WIFI_WPS_OUI_TYPE	       0x04
#define WIFI_P2P_IE_OUI_TYPE	    0x09
#define WIFI_ATBM_IE_OUI_TYPE	    0x0a

#define D11_WIFI_ELT_ID		0xDD 

//DCXO register address
#define DCXO_TRIM_REG 0x1610100c //bit 5:0



/*
switch (rate_value)
case 10: rate = WSM_TRANSMIT_RATE_1;
break;
case 20: rate = WSM_TRANSMIT_RATE_2;
break;
case 55: rate = WSM_TRANSMIT_RATE_5;
break;
case 110: rate = WSM_TRANSMIT_RATE_11;
break;
case 60: rate = WSM_TRANSMIT_RATE_6;
break;
case 90: rate = WSM_TRANSMIT_RATE_9;
break;
case 120: rate = WSM_TRANSMIT_RATE_12;
break;
case 180: rate = WSM_TRANSMIT_RATE_18;
break;
case 240: rate = WSM_TRANSMIT_RATE_24;
break;
case 360: rate = WSM_TRANSMIT_RATE_36;
break;
case 480: rate = WSM_TRANSMIT_RATE_48;
break;
case 540: rate = WSM_TRANSMIT_RATE_54;
break;
case 65: rate = WSM_TRANSMIT_RATE_HT_6;
break;
case 130: rate = WSM_TRANSMIT_RATE_HT_13;
break;
case 195: rate = WSM_TRANSMIT_RATE_HT_19;
break;
case 260: rate = WSM_TRANSMIT_RATE_HT_26;
break;
case 390: rate = WSM_TRANSMIT_RATE_HT_39;
break;
case 520: rate = WSM_TRANSMIT_RATE_HT_52;
break;
case 585: rate = WSM_TRANSMIT_RATE_HT_58;
break;
case 650: rate = WSM_TRANSMIT_RATE_HT_65;
*/
int atbm_etf_start_tx(int channel,int rate_value,int is_40M, int greedfiled);
int atbm_etf_stop_tx(void);
int atbm_etf_start_rx(int channel ,int is_40M);
int atbm_etf_stop_rx(void);
int atbm_etf_test_is_start(void);
int atbm_etf_start_tx_single_tone(int channel,int rate_value,int is_40M, int greedfiled);
atbm_void etf_v2_scan_end(struct atbmwifi_common *hw_priv, struct atbmwifi_vif *vif );

#endif  //__ATBM_ETF_H__
