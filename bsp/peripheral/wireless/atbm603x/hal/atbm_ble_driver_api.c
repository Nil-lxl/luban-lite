/**************************************************************************************************************
 * altobeam RTOS API
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#include "atbm_hal.h"

extern struct atbmwifi_common g_hw_prv;
static atbm_uint8 atbm_ble_data[2048];
#define HCI_ACL_SHARE_SIZE	512

int atbm_ble_dev_rx(struct atbmwifi_common* hw_priv, atbm_uint8* event_buffer, atbm_size_t event_len, enum ieee80211_ble_msg_type msg_type);

#if CONFIG_ATBM_BLE_ADV_COEXIST

int atbm_ble_adv_coexit_start(atbm_uint8* data)
{
	struct ioctl_ble_start* ble_start = (struct ioctl_ble_start*)data;
	struct wsm_ble_msg_coex_start ble_coex = { 0 };

	if ((ble_start->ble_adv == 0) && (ble_start->ble_scan == 0)) {
		atbm_printk_err("both adv and scan is close!\n");
		return -1;
	}

	if ((ble_start->ble_scan) && (ble_start->ble_scan_win == 0)) {
		atbm_printk_err("ble scan enable, but scan_win is 0!\n");
		return -1;
	}

	if ((ble_start->ble_adv_chan != 0) && (ble_start->ble_adv_chan >= 37)
		&& (ble_start->ble_adv_chan <= 39)) {
		ble_coex.chan_flag |= BIT(ble_start->ble_adv_chan - 37);
	}

	if ((ble_start->ble_scan_chan != 0) && (ble_start->ble_scan_chan >= 37)
		&& (ble_start->ble_scan_chan <= 39)) {
		ble_coex.chan_flag |= BIT(ble_start->ble_scan_chan - 37 + 3);
	}

	if (ble_start->ble_adv) {
		ble_coex.coex_flag |= BIT(0);
	}

	if (ble_start->ble_scan) {
		ble_coex.coex_flag |= BIT(1);
	}

	ble_coex.interval = ble_start->ble_interval;
	ble_coex.scan_win = ble_start->ble_scan_win;
	ble_coex.ble_id = BLE_MSG_COEXIST_START;
	atbm_printk_init("atbm_ioctl_ble_adv_coexit_start\n");
	return wsm_ble_msg_coexist_start(atbm_info.hw_priv, &ble_coex, 0);
}

int atbm_ble_adv_coexit_stop(atbm_uint8* data)
{
	struct wsm_ble_msg ble_coex = { 0 };
	ble_coex.ble_id = BLE_MSG_COEXIST_STOP;
	return wsm_ble_msg_coexist_stop(atbm_info.hw_priv, &ble_coex, 0);
}
int atbm_ble_set_adv_data(atbm_uint8* data)
{
	struct ioctl_ble_adv_data* adv_data = (struct ioctl_ble_adv_data*)data;
	struct wsm_ble_msg_adv_data ble_adv_data = { 0 };

	memcpy(&ble_adv_data.mac[0], adv_data, sizeof(struct ioctl_ble_adv_data));
	ble_adv_data.ble_id = BLE_MSG_SET_ADV_DATA;
	return wsm_ble_msg_set_adv_data(atbm_info.hw_priv, &ble_adv_data, 0);
}

int atbm_ble_adv_resp_start(atbm_uint8* data)
{
	struct ioctl_ble_adv_resp_start* ble_start = (struct ioctl_ble_adv_resp_start*)data;
	struct wsm_ble_msg_adv_resp_start ble_adv_resp_msg = { 0 };


	ble_adv_resp_msg.interval = ble_start->ble_interval;
	ble_adv_resp_msg.ble_id = BLE_MSG_ADV_RESP_MODE_START;
	return wsm_ble_msg_set_adv_data(atbm_info.hw_priv, (struct wsm_ble_msg_adv_data*)&ble_adv_resp_msg, 0);
}

int atbm_ble_set_resp_data(atbm_uint8* data)
{
	struct ioctl_ble_resp_data* resp_data = (struct ioctl_ble_resp_data*)data;
	struct wsm_ble_msg_resp_data ble_resp_data = { 0 };

	memcpy(&ble_resp_data.resp_data_len, resp_data, sizeof(struct ioctl_ble_resp_data));
	ble_resp_data.ble_id = BLE_MSG_SET_RESP_DATA;
	return wsm_ble_msg_set_adv_data(atbm_info.hw_priv, (struct wsm_ble_msg_adv_data*)&ble_resp_data, 0);
}
#endif//#ifdefCONFIG_ATBM_BLE_ADV_COEXISTstatic int atbm_ioctl_notify_add(atbm_uint8 type, atbm_uint8 driver_mode, atbm_uint8 *event_buffer, atbm_uint16 event_len)

#define USING_DEVICE_MODE
static int atbm_ble_notify_add(atbm_uint8 type, atbm_uint8 driver_mode, atbm_uint8 *event_buffer, atbm_uint16 event_len)
{
#ifdef USING_DEVICE_MODE
    atbm_ble_os_msg_notify_add(type,driver_mode,event_buffer,event_len);
#else
	ble_msg_func(event_buffer);
#endif
}

void atbm_ble_adv_rpt_async(atbm_uint8 *event_buffer, atbm_uint16 event_len)
{
	if (atbm_ble_notify_add(0, 0, event_buffer, event_len) > 0)
	{
		wifi_printk(WIFI_BLE, "%s failed!\n", __FUNCTION__);
	}
}

void atbm_ioctl_ble_conn_rpt_async(atbm_uint8 *event_buffer, atbm_uint16 event_len)
{
	if (atbm_ble_notify_add(1, 0, event_buffer, event_len) > 0)
	{
		wifi_printk(WIFI_BLE, "%s failed!\n", __FUNCTION__);
	}
}

int atbm_ble_start(atbm_uint8 *data)
{
#if CONFIG_WIFI_BT_COMB
	struct atbmwifi_common* hw_priv = &g_hw_prv;

	wifi_printk(WIFI_BLE,"atbm_ioctl_ble_start\n");
	atbm_memcpy(atbm_ble_data, data, 1024);
	//ble full stack COMB
	ieee80211_ble_commb_start(hw_priv);
	ieee80211_ble_commb_subscribe(hw_priv, atbm_ble_dev_rx);
	return 0;
#else
//ble adv/scan comb
	return atbm_ble_adv_coexit_start(data);
#endif
	}
	
int atbm_ble_stop(atbm_uint8* data)
{
#if CONFIG_WIFI_BT_COMB
	struct atbmwifi_common* hw_priv = &g_hw_prv;

	wifi_printk(WIFI_BLE,"atbm_ioctl_ble_stop\n");
	atbm_memcpy(atbm_ble_data, data, 1024);
//ble full stack COMB
	ieee80211_ble_commb_unsubscribe(hw_priv);
	ieee80211_ble_commb_stop(hw_priv);
	return 0;
#else
//ble adv/scan comb
	return atbm_ble_adv_coexit_stop(data);
#endif
	}
	
int atbm_ble_dev_rx(struct atbmwifi_common* hw_priv, atbm_uint8* event_buffer, atbm_size_t event_len, enum ieee80211_ble_msg_type msg_type)
{
	//printk("atbm_ble_dev_rx len %d\n", event_len);
	//frame_hexdump_wp("rx:", event_buffer, event_len);
	if (atbm_ble_notify_add(0, 0, event_buffer, event_len) > 0) {
		wifi_printk(WIFI_BLE, "%s failed!\n", __FUNCTION__);
	}
	return 0;
}

void atbm_ble_dev_tx(atbm_uint8* buf, int len)
{
#ifdef CONFIG_WIFI_BT_COMB
	//struct ble_hci_hif_pkt* tx_pkt;
	struct atbmwifi_common* hw_priv = &g_hw_prv;

	memcpy(atbm_ble_data, buf, len);
	//struct platform_device* pble_dev = &local->ble_dev;
	char* xmit_buff;
	atbm_uint8* tx_pkt = &buf[2];
	atbm_uint16 tx_len = *(atbm_uint16*)buf;
	xmit_buff = NULL;

	//printk("atbm_ble_dev_tx %d\n", tx_len);
	rcu_read_lock();

	xmit_buff = ieee80211_ble_commb_ble_alloc_xmit( HCI_ACL_SHARE_SIZE);

	if (xmit_buff == NULL) {
		goto pkt_free;
	}

	//printk("atbm_ble_dev_tx %p tx_pkt %p len %d\n", xmit_buff, tx_pkt, tx_len);
	memcpy(xmit_buff, tx_pkt, tx_len);
	ieee80211_ble_commb_xmit(hw_priv, xmit_buff, tx_len);

pkt_free:
	rcu_read_unlock();
	//ble_hci_trans_free_hif_pkt(tx_pkt);
	return;
#else
	wifi_printk(WIFI_BLE, "unsupport ble mode\n");
#endif //#ifdef CONFIG_WIFI_BT_COMB
}


