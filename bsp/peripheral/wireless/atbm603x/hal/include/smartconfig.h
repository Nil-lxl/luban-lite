/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#ifndef __SMART_CONFIG_H__
#define  __SMART_CONFIG_H__
#include "atbm_type.h"

#define SMART_MAX_CHANNEL 14
#define D0_IS_MULTICAST(_a) \
	(*((atbm_uint8 *)(_a)) & ((atbm_uint8)0x01))

#define PHY_STATUS0_N_AGGREGATION (1 << 5)
#define PHY_STATUS0_N_20_40M ((1 << 12) | (1 << 13))
#define PHY_STATUS0_N_RXMODE_MASK (7 << 0)


typedef struct SMARTCONFIG_S
{
	atbm_uint8 Packet_number;
	atbm_uint8 rate;
	atbm_uint16 length;
	atbm_uint32 rx_status0_start;
	atbm_uint32 Word1;
	atbm_uint32 Word2;
	atbm_uint32 Word3;
	atbm_uint32 Word4;
	atbm_uint32 Word5;
	atbm_uint32 Word6;
	//atbm_uint32 rx_status0_end;
	//atbm_uint32 rx_status1_end;
	atbm_uint16 Status0Cf0;
	atbm_uint16 Status0Snr;
	atbm_int8 Status1Rssi;
	atbm_uint8 Status1EvmLsb;
	atbm_uint8 Status1EvmMsb;
	atbm_uint8 Status1Rcpi;
	atbm_uint32 error_code;
}SMARTCONFIG_T;

typedef atbm_int32 (*ht40_smartconfig_rx_process_func)(atbm_uint16 length, atbm_int32 channel, atbm_int8 rssi, atbm_uint32 rx_type, atbm_uint8 *mac_hdr);

/*
enum smartconfig_status{
	CONFIG_ST_IDLE = 0,
	CONFIG_ST_START = 1,
	CONFIG_ST_GET_MAGIC = 2,
	CONFIG_ST_SWITCH_PAYLOAD_CH = 3,
	CONFIG_ST_GET_PAYLOAD = 4,
	CONFIG_ST_DONE_SUCCESS = 5,
	CONFIG_ST_DONE_FAIL =6,
	CONFIG_ST_DUPLICATE_FAIL =7,
	CONFIG_ST_STARTCONFIG_ERR =8,
};*/
enum smartconfig_status{
	CONFIG_ST_IDLE = 0,
	CONFIG_ST_REIDLE = 8,
	CONFIG_ST_START = 1,
	CONFIG_ST_GET_MAGIC = 2,
	CONFIG_ST_PAYLOAD = 3,
	CONFIG_ST_RE_PAYLOAD = 4,
	CONFIG_ST_GET_TOTALLEN = 5,
	CONFIG_ST_SWITCH_PAYLOAD_CH = 6,
	CONFIG_ST_GET_PAYLOAD = 7,
	CONFIG_ST_GET_PAYLOAD_CSUM = 8,
	CONFIG_ST_DONE_SUCCESS = 9,
	CONFIG_ST_DONE_FAIL =10,
	CONFIG_ST_DUPLICATE_FAIL =11,
	CONFIG_ST_STARTCONFIG_ERR =12,
};
#define SMARTCONFIG_DATA_BUF_LEN (256*2)

enum smartconfig_type{
	CONFIG_TP_ATBM_SMART = 0,
	CONFIG_TP_AIRKISS = 1,
};

struct smartconfig_config{
	enum smartconfig_type type;
	/*cnt : 1 ~ 10*/
	int magic_cnt;
	/*ms : 20ms ~ 200ms*/
	int magic_time;
	/*ms : 500ms ~ 10000ms*/
	int payload_time;
};

int smartconfig_start(struct smartconfig_config *cfg,int if_id);
int smartconfig_stop(int if_id);
int smartconfig_status(int if_id);

/*cnt : 2 ~ 10*/
int smartconfig_magic_channel_cnt(int cnt);
/*ms : 20ms ~ 200ms*/
int smartconfig_magic_channel_timeout(int ms);
/*ms : 500ms ~ 10000ms*/
int smartconfig_payload_timeout(int ms);

atbm_void smartconfig_success_notify(struct atbmwifi_vif *priv);

// int fun_recv_magic(struct atbmwifi_common *hw_priv,short rxdata);
//int fun_recv_payload(short rxdata);
//atbm_int32 ht_smartconfig_start_rx_fifo(atbm_uint16 length,atbm_int32 channel,atbm_int32 rssi,atbm_uint32 rx_type,atbm_uint8 *mac_hdr );

#endif  /*__SMART_CONFIG_H__*/
