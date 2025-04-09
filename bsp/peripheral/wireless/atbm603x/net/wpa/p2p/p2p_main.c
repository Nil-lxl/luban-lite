#include "p2p.h"
#include "p2p_i.h"
#include "p2p_debug.h"
#include "p2p_common.h"
#include "p2p_defs.h"
#include "p2p_list.h"

#include "atbm_hal.h"
#include "p2p_main.h"
#include "wps_main.h"

#if CONFIG_P2P

extern struct wifi_configure *hmac_cfg;


struct p2p_bss{
	struct dl_list list;
	atbm_uint8 bssid[6];
	int rssi;
	atbm_uint16 freq;
	atbm_uint8 *res;
	atbm_uint16 res_len;
};

//hal_queue_t  p2p_task_queue ;
struct dl_list p2p_bss_list;
struct p2p_data *p2p_global;


static void wpas_p2p_join_scan_req(struct atbmwifi_vif *priv, int freq,
				   const atbm_uint8 *ssid, atbm_size_t ssid_len);

static int wpas_send_action(void *ctx, unsigned int freq, const atbm_uint8 *dst,
			    const atbm_uint8 *src, const atbm_uint8 *bssid, const atbm_uint8 *buf,
			    atbm_size_t len, unsigned int wait_time, int *scheduled);

int atbm_p2p_connect(struct atbmwifi_vif *priv, const atbm_uint8 *peer_addr,
		     const char *pin, enum p2p_wps_method wps_method,
		     int persistent_group, int auto_join, int join, int auth,
		     int go_intent, int freq,
			 const atbm_uint8 *group_ssid, atbm_size_t group_ssid_len);


static void wpas_p2p_debug_print(void *ctx, int level, const char *msg)
{
	p2p_printf(level, "P2P: %s", msg);
}

extern atbm_uint8 test_p2p_mac[6];

static int wpas_p2p_connect_check(struct p2p_data *p2p, const atbm_uint8 *peer_addr)
{
	struct p2p_device *dev;
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)p2p->ctx;
	atbm_uint8 channel;
	int ret;
	
	dev = p2p_get_device(p2p, peer_addr);
	if (dev == NULL || (dev->flags & P2P_DEV_PROBE_REQ_ONLY)) {
		p2p_dbg(p2p, "Cannot connect to unknown P2P Device " MACSTR,
			MAC2STR(peer_addr));
		return -1;
	}

	ret = ieee80211_freq_to_chan(dev->listen_freq, &channel);
	if(ret != ATBM_HOSTAPD_MODE_IEEE80211G){
		return -1;
	}
	
	//p2p->cfg->channel = channel;				//change listen channel
	atbm_enable_listening(priv, (atbm_uint16)channel);
	
	return 0;
}

void p2p_scan_result(void)
{
	struct p2p_bss *bss;
	struct p2p_bss *bss_next;
	struct p2p_data	*p2pdata = p2p_global;
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)p2p_global->ctx;
	struct p2p_task_msg msg;

	if(priv->p2p_scan == 0){
		return;
	}

	dl_list_for_each_safe(bss, bss_next, &p2p_bss_list, struct p2p_bss, list){
		p2p_scan_res_handler(p2pdata, bss->bssid,
					 (int)bss->freq, 0, bss->rssi, bss->res, bss->res_len);
		dl_list_del(&bss->list);
		atbm_kfree(bss->res);
		atbm_kfree(bss);
	}
	if(priv->p2p_join){
		if(wpas_p2p_connect_check(p2pdata, priv->join_iface_addr)){
			if(priv->p2p_join_scan_count++ < 20){
				p2p_printf(MSG_INFO, "p2p join scan no found, continue scan");
				priv->scan_no_connect = 0;
				return;
			}else{
				p2p_printf(MSG_INFO, "p2p join scan max count, stop");
			}
		}
		
		priv->p2p_join_scan_count = 0;
		priv->p2p_scan = 0;
		priv->p2p_join = 0;
		priv->scan_expire = 0;
		priv->scan_no_connect = 1;	
		
		msg.event = JOIN_SCAN_END;
		msg.param = NULL;

		atbm_os_MsgQ_Send(&priv->p2p_task_msg, &msg, sizeof(struct p2p_task_msg), ATBM_TX_WAIT_FOREVER);

		return;
	}
	
end:
	/* Start join operation immediately */
	priv->p2p_scan = 0;
	priv->p2p_join = 0;
	priv->scan_expire = 0;
	priv->scan_no_connect = 1;	
	p2p_scan_res_handled(p2pdata);

	return;

	//扫描结束，应该进入监听模式
}

extern atbm_void sta_scan_start_timer_func(atbm_void *data1,atbm_void *data2);
static int wpas_p2p_scan(void *ctx, enum p2p_scan_type type, int freq,
			 unsigned int num_req_dev_types,
			 const atbm_uint8 *req_dev_types, const atbm_uint8 *dev_id, atbm_uint16 pw_id)
{
	struct atbmwifi_vif *priv = ctx;
	int ret;
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);	
	struct wsm_template_frame frame = {
		.frame_type = WSM_FRAME_TYPE_PROBE_REQUEST,
	};

	if(p2pdata == NULL){
		return -1;
	}	

	priv->p2p_scan = 1;
	priv->p2p_join = 0;
	priv->scan_expire = 0;
	priv->scan_no_connect = 1;
	priv->scan_expire = 2;

	priv->ssid_length = P2P_WILDCARD_SSID_LEN;
	atbm_memcpy(priv->ssid, P2P_WILDCARD_SSID, P2P_WILDCARD_SSID_LEN);
	priv->ssid[P2P_WILDCARD_SSID_LEN] = '\0';

	frame.skb = atbmwifi_ieee80211_send_probe_req(priv,NULL,priv->extra_ie,priv->extra_ie_len,1);

	if (!frame.skb){
		ret = -1;
		goto END;
	}
	ret = wsm_set_template_frame(hw_priv, &frame,
			priv->if_id);

	dl_list_init(&p2p_bss_list);
	atbmwifi_eloop_register_timeout(0,priv->scan_expire*1000,sta_scan_start_timer_func,(atbm_void *)priv,ATBM_NULL);
	p2p_notify_scan_trigger_status(p2pdata, 0);

	atbm_dev_kfree_skb(frame.skb);

	atbmwifi_sta_scan(priv);
END:
	return ret;
}

static void wpas_p2p_join_scan_req(struct atbmwifi_vif *priv, int freq,
				   const atbm_uint8 *ssid, atbm_size_t ssid_len)
{
	int ret;
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);	
	struct wsm_template_frame frame = {
		.frame_type = WSM_FRAME_TYPE_PROBE_REQUEST,
	};

	if(p2pdata == NULL){
		return;
	}	

	if (ssid && ssid_len) {
		priv->ssid_length = ssid_len;
		atbm_memcpy(priv->ssid, ssid, ssid_len);
		priv->ssid[ssid_len] = '\0';
	} else {
		priv->ssid_length = P2P_WILDCARD_SSID_LEN;
		atbm_memcpy(priv->ssid, P2P_WILDCARD_SSID, P2P_WILDCARD_SSID_LEN);
		priv->ssid[P2P_WILDCARD_SSID_LEN] = '\0';
	}
	//////////WPS_IE(在wps里面修改)

	priv->p2p_scan = 1;
	priv->p2p_join = 1;
	priv->scan_expire = 0;
	priv->scan_no_connect = 1;	

	frame.skb = atbmwifi_ieee80211_send_probe_req(priv,NULL,priv->extra_ie,priv->extra_ie_len,1);

	if (!frame.skb){
		ret = -1;
		return;
		//goto END;
	}
	ret = wsm_set_template_frame(hw_priv, &frame,
			priv->if_id);

//	atbmwifi_RegisterEventHandler(WIFI_SCAN_COMPLETE_EVENT, p2p_scan_result);
	dl_list_init(&p2p_bss_list);	
	atbmwifi_eloop_register_timeout(0, priv->scan_expire * 1000, sta_scan_start_timer_func,(atbm_void *)priv,ATBM_NULL);

	atbm_dev_kfree_skb(frame.skb);
}

static int wpas_send_action(void *ctx, unsigned int freq, const atbm_uint8 *dst,
			    const atbm_uint8 *src, const atbm_uint8 *bssid, const atbm_uint8 *buf,
			    atbm_size_t len, unsigned int wait_time, int *scheduled)
{
	struct atbmwifi_vif *priv = ctx;
	struct atbmwifi_common * hw_priv = priv->hw_priv;
	struct atbm_buff *skb;
	struct atbmwifi_ieee80211_mgmt *mgmt;

	if(len > 512){
		p2p_printf(MSG_ERROR, "<ERROR> wpas_send_action too long");
		return -1;
	}
	
	skb = atbm_dev_alloc_skb(sizeof(*mgmt) + len + 100);
	if (!skb)
	{
		p2p_printf(MSG_ERROR, "<ERROR> wpas_send_action alloc skb");
		return -1;
	}
	
	mgmt = (struct atbmwifi_ieee80211_mgmt *)atbm_skb_put(skb, 24);

	atbm_memset(mgmt, 0, 24);
	
	mgmt->frame_control = atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT | ATBM_IEEE80211_STYPE_ACTION);
	atbm_memcpy(mgmt->da, dst, ATBM_ETH_ALEN);
	atbm_memcpy(mgmt->sa, src, ATBM_ETH_ALEN);
	atbm_memcpy(mgmt->bssid, bssid, ATBM_ETH_ALEN);

	atbm_memcpy(atbm_skb_put(skb, len), buf, len);
	
	ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_INTFL_DONT_ENCRYPT | ATBM_IEEE80211_TX_CTL_NO_CCK_RATE;

	ATBM_IEEE80211_SKB_TXCB(skb)->b_net = 0;
	
	//printf("send_action, wait time:%d\n", wait_time);
	
	atbmwifi_tx(hw_priv, skb, priv);
	if(wait_time > 10){
		wait_time = 10;
	}
	atbm_SleepMs(wait_time);

//	atbmwifi_eloop_cancel_timeout(wpas_send_action_timeout, ctx, NULL);
//	atbmwifi_eloop_register_timeout(0, wait_time, wpas_send_action_timeout, priv, NULL);

	return 0;
}

static void wpas_send_action_done(void *ctx)
{
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)ctx;
	
	p2p_printf(MSG_DEBUG, "wpas_send_action_done");
//	atbmwifi_eloop_cancel_timeout(wpas_send_action_timeout, ctx, NULL);
}

static void wpas_go_neg_completed(void *ctx, struct p2p_go_neg_results *res)
{
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)ctx;
	void *param_res;
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	struct p2p_task_msg msg;
	
	priv->p2p_go_neg_process = 0;		

	if(res->status){		//P2P状态错误 协商失败
		p2p_printf(MSG_ERROR,"P2P GO COMPLET ERR:%d", res->status);
		p2p_set_state(p2pdata, P2P_IDLE);
		return;
	}
	printf("wpas_go_neg_completed\n");

	param_res = (void *)atbm_kmalloc(sizeof(struct p2p_go_neg_results), GFP_KERNEL);
	if(param_res == NULL){
		p2p_printf(MSG_ERROR,"param_res malloc err");
		return;
	}
	
	atbm_memset(param_res, 0, sizeof(struct p2p_go_neg_results));
	atbm_memcpy(param_res, res, sizeof(struct p2p_go_neg_results));
	
	msg.event = GO_NEG_COMPLETED;
	msg.param = param_res;

	atbm_os_MsgQ_Send(&priv->p2p_task_msg, &msg, sizeof(struct p2p_task_msg), ATBM_TX_WAIT_FOREVER);
}

//对方主动请求连接，最终会进入此回掉，可以通过此回掉去主动连接对方
static void wpas_go_neg_req_rx(void *ctx, const atbm_uint8 *src, atbm_uint16 dev_passwd_id,
			       atbm_uint8 go_intent)
{
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)ctx;
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;

	if(!priv->p2p_auto_go_req){
		return;
	}

	if(priv->p2p_ap){		//已经是GO的时候不处理GO请求
		p2p_printf(MSG_INFO,"P2P GO is active, ignore go req\n");
		return;				
	}	

	p2p_clear_timeout(p2pdata);
}

static int wpas_peer_waiting_cb(void *ctx, const atbm_uint8 *src)
{
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)ctx;
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	struct p2p_task_msg msg;

	printf("wpas_peer_waiting_cb\n");

	if(!priv->p2p_auto_go_req){
		return -1;
	}

	if(priv->p2p_ap){		//已经是GO的时候不处理GO请求
		p2p_printf(MSG_INFO,"P2P GO is active, ignore go req\n");
		return -1;				
	}	
	
	p2p_stop_find(p2pdata);

	if(wpas_p2p_connect_check(p2pdata, src)){
		printf("p2p channel set err\n");
		return -1;
	}

	atbm_memcpy(priv->join_iface_addr, src, ATBM_ETH_ALEN);

	msg.event = GO_NEG_PEER_WAIT;
	msg.param = NULL;

	atbm_os_MsgQ_Send(&priv->p2p_task_msg, &msg, sizeof(struct p2p_task_msg), ATBM_TX_WAIT_FOREVER);
	return 0;
}


static void wpas_dev_found(void *ctx, const atbm_uint8 *addr,
			   const struct p2p_peer_info *info,
			   int new_device)
{
#ifndef CONFIG_NO_STDOUT_DEBUG
	struct atbmwifi_vif *priv = ctx;
	char devtype[WPS_DEV_TYPE_BUFSIZE];
	char *wfd_dev_info_hex = NULL;
	
	if (info->p2ps_instance) {
		char str[256];
		const atbm_uint8 *buf = wpabuf_head(info->p2ps_instance);
		atbm_size_t len = wpabuf_len(info->p2ps_instance);

		while (len) {
			atbm_uint32 id;
			atbm_uint16 methods;
			atbm_uint8 str_len;

			if (len < 4 + 2 + 1)
				break;
			id = ATBM_WPA_GET_LE32(buf);
			buf += sizeof(atbm_uint32);
			methods = ATBM_WPA_GET_BE16(buf);
			buf += sizeof(atbm_uint16);
			str_len = *buf++;
			if (str_len > len - 4 - 2 - 1)
				break;
			atbm_memcpy(str, buf, str_len);
			str[str_len] = '\0';
			buf += str_len;
			len -= str_len + sizeof(atbm_uint32) + sizeof(atbm_uint16) + sizeof(atbm_uint8);

			p2p_printf(MSG_INFO,
				       P2P_EVENT_DEVICE_FOUND MACSTR
				       " p2p_dev_addr=" MACSTR
				       " pri_dev_type=%s name='%s'"
				       " config_methods=0x%x"
				       " dev_capab=0x%x"
				       " group_capab=0x%x"
				       " adv_id=%x asp_svc=%s%s",
				       MAC2STR(addr),
				       MAC2STR(info->p2p_device_addr),
				       wps_dev_type_bin2str(
					       info->pri_dev_type,
					       devtype, sizeof(devtype)),
				       info->device_name, methods,
				       info->dev_capab, info->group_capab,
				       id, str,
				       info->vendor_elems ?
				       " vendor_elems=1" : "");
		}
		goto done;
	}

	p2p_printf(MSG_INFO, P2P_EVENT_DEVICE_FOUND MACSTR
		       " p2p_dev_addr=" MACSTR
		       " pri_dev_type=%s name='%s' config_methods=0x%x "
		       "dev_capab=0x%x group_capab=0x%x%s%s%s new=%d",
		       MAC2STR(addr), MAC2STR(info->p2p_device_addr),
		       wps_dev_type_bin2str(info->pri_dev_type, devtype,
					    sizeof(devtype)),
		       info->device_name, info->config_methods,
		       info->dev_capab, info->group_capab,
		       wfd_dev_info_hex ? " wfd_dev_info=0x" : "",
		       wfd_dev_info_hex ? wfd_dev_info_hex : "",
		       info->vendor_elems ? " vendor_elems=1" : "",
		       new_device);

done:
	atbm_kfree(wfd_dev_info_hex);	

#endif	
}

static void wpas_dev_lost(void *ctx, const atbm_uint8 *dev_addr)
{
	p2p_printf(MSG_INFO, P2P_EVENT_DEVICE_LOST
		       "p2p_dev_addr=" MACSTR, MAC2STR(dev_addr));	
}

static void wpas_find_stopped(void *ctx)
{
	p2p_printf(MSG_INFO, P2P_EVENT_FIND_STOPPED);
	
}

static int wpas_start_listen(void *ctx, unsigned int freq,
			     unsigned int duration,
			     const struct wpabuf *probe_resp_ie)
{
	//设置信道，启动监听，回复PB REQ(P2P IE)
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)ctx;
	atbm_uint8 channel;
	int ret;
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	
	ret = ieee80211_freq_to_chan(freq, &channel);
	if(ret != ATBM_HOSTAPD_MODE_IEEE80211G){
		p2p_printf(MSG_ERROR, "unspported freq");
		return -1;
	}
//	wdt_pit_feed(1);
	atbm_enable_listening(priv, (atbm_uint16)channel);	
	p2p_listen_cb(p2pdata, freq, duration);
	priv->p2p_listen = 1;

	return 0;
}

static void wpas_stop_listen(void *ctx)
{
	//停止监听功能
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)ctx;
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;

	if(priv->p2p_listen){
		priv->p2p_listen = 0;
		if(priv->p2p_go_neg_process == 0){
			atbm_disable_listening(priv);
		}
		p2p_listen_end(p2pdata, 0);
	}
}

static int wpas_send_probe_resp(void *ctx, const struct wpabuf *buf,
				unsigned int freq)
{
	//GO 模式 监听回复PB REQ
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)ctx;
	struct atbmwifi_common * hw_priv = priv->hw_priv;
	struct atbm_buff *skb;
	int len = wpabuf_len(buf);

	skb = atbm_dev_alloc_skb(len + 100);
	if (!skb)
	{
		p2p_printf(MSG_ERROR, "<ERROR> wpas_send_probe_resp alloc skb");
		return -1;
	}

	atbm_memcpy(atbm_skb_put(skb, len), wpabuf_head(buf), len);
	
	ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_INTFL_DONT_ENCRYPT | ATBM_IEEE80211_TX_CTL_NO_CCK_RATE;

	ATBM_IEEE80211_SKB_TXCB(skb)->b_net = 0;
	
	atbmwifi_tx(hw_priv, skb, priv);

	return 0;
}

void wpas_sd_request(void *ctx, int freq, const atbm_uint8 *sa, atbm_uint8 dialog_token,
		     atbm_uint16 update_indic, const atbm_uint8 *tlvs, atbm_size_t tlvs_len)
{
	//广告服务，不支持
}
void wpas_sd_response(void *ctx, const atbm_uint8 *sa, atbm_uint16 update_indic,
		      const atbm_uint8 *tlvs, atbm_size_t tlvs_len)
{
	//广告服务，不支持
}
static void wpas_prov_disc_req(void *ctx, const atbm_uint8 *peer, atbm_uint16 config_methods,
			       const atbm_uint8 *dev_addr, const atbm_uint8 *pri_dev_type,
			       const char *dev_name, atbm_uint16 supp_config_methods,
			       atbm_uint8 dev_capab, atbm_uint8 group_capab, const atbm_uint8 *group_id,
			       atbm_size_t group_id_len)
{
	//回掉
}

static void wpas_prov_disc_resp(void *ctx, const atbm_uint8 *peer, atbm_uint16 config_methods)
{
	//回掉 处理p2p组回复启动WPS加网
}
static void wpas_prov_disc_fail(void *ctx, const atbm_uint8 *peer,
				enum p2p_prov_disc_status status,
				atbm_uint32 adv_id, const atbm_uint8 *adv_mac,
				const char *deferred_session_resp)
{
	//回掉
}
static atbm_uint8 wpas_invitation_process(void *ctx, const atbm_uint8 *sa, const atbm_uint8 *bssid,
				  const atbm_uint8 *go_dev_addr, const atbm_uint8 *ssid,
				  atbm_size_t ssid_len, int *go, atbm_uint8 *group_bssid,
				  int *force_freq, int persistent_group,
				  const struct p2p_channels *channels,
				  int dev_pw_id)
{
	//邀请服务，暂不支持
	return 0;
}
static void wpas_invitation_received(void *ctx, const atbm_uint8 *sa, const atbm_uint8 *bssid,
				     const atbm_uint8 *ssid, atbm_size_t ssid_len,
				     const atbm_uint8 *go_dev_addr, atbm_uint8 status,
				     int op_freq)
{
	//邀请服务，暂不支持
}

static void wpas_invitation_result(void *ctx, int status, const atbm_uint8 *bssid,
				   const struct p2p_channels *channels,
				   const atbm_uint8 *peer, int neg_freq,
				   int peer_oper_freq)
{
	//邀请服务，暂不支持
}

static int wpas_get_noa(void *ctx, const atbm_uint8 *interface_addr, atbm_uint8 *buf,
			atbm_size_t buf_len)
{
	return 0;
}

static int wpas_go_connected(void *ctx, const atbm_uint8 *dev_addr)
{
	//go 连接状态
	return 0;
}
static void wpas_presence_resp(void *ctx, const atbm_uint8 *src, atbm_uint8 status,
			       const atbm_uint8 *noa, atbm_size_t noa_len)
{

}
static int wpas_is_concurrent_session_active(void *ctx)
{
	//链接状态
	return 0;
}
static int wpas_p2p_in_progress(void *ctx)
{
	return 0;//正在处理状态
}
static int wpas_get_persistent_group(void *ctx, const atbm_uint8 *addr, const atbm_uint8 *ssid,
				     atbm_size_t ssid_len, atbm_uint8 *go_dev_addr,
				     atbm_uint8 *ret_ssid, atbm_size_t *ret_ssid_len,
				     atbm_uint8 *intended_iface_addr)
{
	return 0;
}
static int wpas_get_go_info(void *ctx, atbm_uint8 *intended_addr,
			    atbm_uint8 *ssid, atbm_size_t *ssid_len, int *group_iface,
			    unsigned int *freq)
{
	//获取GO信息
	return 0;
}
static int wpas_remove_stale_groups(void *ctx, const atbm_uint8 *peer, const atbm_uint8 *go,
				    const atbm_uint8 *ssid, atbm_size_t ssid_len)
{
	//
	return 0;
}
static void wpas_p2ps_prov_complete(void *ctx, atbm_uint8 status, const atbm_uint8 *dev,
				    const atbm_uint8 *adv_mac, const atbm_uint8 *ses_mac,
				    const atbm_uint8 *grp_mac, atbm_uint32 adv_id, atbm_uint32 ses_id,
				    atbm_uint8 conncap, int passwd_id,
				    const atbm_uint8 *persist_ssid,
				    atbm_size_t persist_ssid_size, int response_done,
				    int prov_start, const char *session_info,
				    const atbm_uint8 *feat_cap, atbm_size_t feat_cap_len,
				    unsigned int freq,
				    const atbm_uint8 *group_ssid, atbm_size_t group_ssid_len)
{

}

// P2P GO DISC resp cb
static int wpas_prov_disc_resp_cb(void *ctx)
{
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)ctx;
	struct p2p_task_msg msg;
	
	printf("wpas_prov_disc_resp_cb\n");
	
	if(priv->p2p_ap){
		msg.event = GO_DISC_RESP;
		msg.param = NULL;
		atbm_os_MsgQ_Send(&priv->p2p_task_msg, &msg, sizeof(struct p2p_task_msg), ATBM_TX_WAIT_FOREVER);
	}
	return 0;
}


static atbm_uint8 p2ps_group_capability(void *ctx, atbm_uint8 incoming, atbm_uint8 role,
				unsigned int *force_freq,
				unsigned int *pref_freq)
{
	return 0;
}

static int wpas_p2p_get_pref_freq_list(void *ctx, int go,
				       unsigned int *len,
				       unsigned int *freq_list)
{
	return 0;
}

static void wpas_p2p_add_chan(struct p2p_reg_class *reg, atbm_uint8 chan)
{
	reg->channel[reg->channels] = chan;
	reg->channels++;
}

static int wpas_p2p_setup_channels(struct p2p_channels *chan,
				   struct p2p_channels *cli_chan)
{
	int cla = 0;
	
	atbm_memset(cli_chan, 0, sizeof(*cli_chan));
	/* Operating class 81 - 2.4 GHz band channels 1..13 */
	chan->reg_class[cla].reg_class = 81;
	chan->reg_class[cla].channels = 0;
	
	wpas_p2p_add_chan(&chan->reg_class[cla], 1);
	wpas_p2p_add_chan(&chan->reg_class[cla], 6);
	wpas_p2p_add_chan(&chan->reg_class[cla], 11);
	
	if (chan->reg_class[cla].channels)
		cla++;	
	chan->reg_classes = cla;
	return 0;
}

static int wpas_p2p_join(struct atbmwifi_vif *priv, const atbm_uint8 *iface_addr,
			 const atbm_uint8 *dev_addr, enum p2p_wps_method wps_method,
			 int auto_join, int op_freq,
			 const atbm_uint8 *ssid, atbm_size_t ssid_len)
{
	p2p_printf(MSG_DEBUG, "P2P: Request to join existing group (iface "
		   MACSTR " dev " MACSTR " op_freq=%d)%s",
		   MAC2STR(iface_addr), MAC2STR(dev_addr), op_freq,
		   auto_join ? " (auto_join)" : "");
	if (ssid && ssid_len) {
		p2p_printf(MSG_DEBUG, "P2P: Group SSID specified: %s",
			   wpa_ssid_txt(ssid, ssid_len));
	}
	
	priv->p2p_auto_join = !!auto_join;
	atbm_memcpy(priv->join_iface_addr, iface_addr, ATBM_ETH_ALEN);
	atbm_memcpy(priv->join_dev_addr, dev_addr, ATBM_ETH_ALEN);
	priv->p2p_wps_method = wps_method;

	priv->p2p_join_scan_count = 0;
	wpas_p2p_join_scan_req(priv, op_freq, ssid, ssid_len);
	return 0;
}

static int wpas_p2p_start_go_neg(struct atbmwifi_vif *priv,
				 const atbm_uint8 *peer_addr,
				 enum p2p_wps_method wps_method,
				 int go_intent, const atbm_uint8 *own_interface_addr,
				 unsigned int force_freq, int persistent_group,
				 struct atbm_wpa_ssid *ssid, unsigned int pref_freq)
{

	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;

	if (persistent_group)
		persistent_group = 2;

	/*
	 * Increase GO config timeout if HT40 is used since it takes some time
	 * to scan channels for coex purposes before the BSS can be started.
	 */
	p2p_set_config_timeout(p2pdata, 255, 20);

	return p2p_connect(p2pdata, peer_addr, wps_method,
			   go_intent, own_interface_addr, force_freq,
			   persistent_group, ssid ? ssid->ssid : ATBM_NULL,
			   ssid ? ssid->ssid_len : 0, 0, pref_freq, 0);
}


static void wpas_p2p_assoc_req_ie(struct atbmwifi_vif *priv, const atbm_uint8 *bssid, int p2p_group)
{
	struct p2p_data	*p2pdata = (struct p2p_data	*)priv->p2pdata;
	int ret;

	if(priv->p2p_assoc_req_ie){
		atbm_kfree(priv->p2p_assoc_req_ie);
		priv->p2p_assoc_req_ie = NULL;
	}
	
	priv->p2p_assoc_req_ie = (atbm_uint8 *)atbm_kmalloc(100, GFP_KERNEL);
	ret = p2p_assoc_req_ie(p2pdata, bssid, priv->p2p_assoc_req_ie
								, 100, p2p_group, NULL);
	if(ret < 0){
		printf("P2P assoc req ie creat err\n");
		priv->p2p_assoc_req_ie_len = 0;
		return;
	}
	priv->p2p_assoc_req_ie_len = ret;
}

static void wpas_p2p_ie_update(void *ctx, struct wpabuf *beacon_ies,
			       struct wpabuf *proberesp_ies)
{
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)ctx;
	if(priv->p2p_ap){
		if(beacon_ies){
			wpabuf_free(priv->p2p_go_beacon_ie);
			priv->p2p_go_beacon_ie = beacon_ies;
		}
		wpabuf_free(priv->p2p_go_probe_resp_ie);
		priv->p2p_go_probe_resp_ie = proberesp_ies;
	}else{
		wpabuf_free(beacon_ies);
		wpabuf_free(proberesp_ies);
	}

	wpa_comm_init_extra_ie(priv);
	atbmwifi_ap_start_beacon(priv);
	atbm_kfree(priv->extra_ie);
	priv->extra_ie = NULL;
	priv->extra_ie_len = 0;
}

static void wpas_p2p_idle_update(void *ctx, int idle)
{

}

struct p2p_group *wpas_p2p_group_init(struct atbmwifi_vif *priv, struct p2p_go_neg_results *res)
{
	struct p2p_group *group;
	struct p2p_group_config *cfg;
	struct p2p_data	*p2pdata = (struct p2p_data	*)priv->p2pdata;
	
	if(p2pdata == NULL){
		return NULL;
	}

	cfg = (struct p2p_group_config *)atbm_kzalloc(sizeof(*cfg), GFP_KERNEL);
	if (cfg == NULL){
		return NULL;
	}

	cfg->persistent_group = 2;
	atbm_memcpy(cfg->interface_addr, priv->mac_addr, ATBM_ETH_ALEN);
	cfg->max_clients = 4;
	atbm_memcpy(cfg->ssid, res->ssid, res->ssid_len);
	cfg->ssid_len = res->ssid_len;
	cfg->freq = res->freq;
	cfg->cb_ctx = priv;
	cfg->ie_update = wpas_p2p_ie_update;
	cfg->idle_update = wpas_p2p_idle_update;
	cfg->ip_addr_alloc = 0;  //不支持4次握手获取IP地址
	group = p2p_group_init(p2pdata, cfg);
	if (group == NULL){
		atbm_kfree(cfg);
	}
	priv->p2pgroup = group;
	return group;	
}

void wpas_p2p_group_deinit(struct atbmwifi_vif *priv){
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;

	if(priv->p2p_ap){
		p2p_group_deinit(priv->p2pgroup);
		priv->p2pgroup = ATBM_NULL;
		priv->p2p_ap = 0;
	}
}
static void p2p_join_scan_process(struct atbmwifi_vif *priv)
{

	struct p2p_data	*p2pdata = (struct p2p_data	*)priv->p2pdata;
	int freq;
	struct p2p_device *dev;

	if(priv->p2p_auto_join){
		dev = p2p_get_device(p2pdata, priv->join_iface_addr);
		if (dev == NULL || (dev->flags & P2P_DEV_PROBE_REQ_ONLY)) {
			p2p_dbg(p2pdata, "Cannot connect to unknown P2P Device " MACSTR,
				MAC2STR(priv->join_iface_addr));
			return;
		}
			
		atbm_p2p_connect(priv, priv->join_iface_addr, NULL, WPS_PBC,
				 0, 0, 0, 0, priv->p2p_go_intent, dev->listen_freq,
				 NULL, 0);
		return;
	}

	if(priv->p2p_join){
		freq = p2p_get_oper_freq(p2pdata, priv->join_iface_addr);

		if(freq > 0){
			atbm_uint16 method;		
			switch (priv->p2p_wps_method) {
			case WPS_PIN_DISPLAY:
				method = WPS_CONFIG_KEYPAD;
				break;
			case WPS_PIN_KEYPAD:
				method = WPS_CONFIG_DISPLAY;
				break;
			case WPS_PBC:
				method = WPS_CONFIG_PUSHBUTTON;
				break;
			case WPS_P2PS:
				method = WPS_CONFIG_P2PS;
				break;
			default:
				method = 0;
				break;
			}
			if (p2p_prov_disc_req(p2pdata,priv->join_dev_addr,NULL, method, 1,
								  freq, priv->user_initiated_pd) < 0) {
						p2p_printf(MSG_DEBUG, "P2P: Failed to send Provision "
							   "Discovery Request before joining an "
							   "existing group");
						return;
			}
			return;
		}else{
			printf("p2p join scan no found, continue scan\n");
			priv->scan_no_connect = 0;
			return;
		}
	}
}

static void p2p_go_peer_wait_process(struct atbmwifi_vif *priv)
{
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	struct p2p_device *dev;
	int ret;
	atbm_uint8 channel;


	dev = p2p_get_device(p2pdata, priv->join_iface_addr);
	if (dev == NULL || (dev->flags & P2P_DEV_PROBE_REQ_ONLY)) {
		p2p_dbg(p2pdata, "Cannot connect to unknown P2P Device " MACSTR,
			MAC2STR(priv->join_iface_addr));
		return;
	}
	
	if(0 == (dev->flags & P2P_DEV_PEER_WAITING_RESPONSE)){
		return;
	}

	atbm_p2p_connect(priv, priv->join_iface_addr, NULL, WPS_PBC,
				 0, 0, 0, 0, priv->p2p_go_intent, dev->listen_freq, NULL, 0);
}

static void p2p_go_complete_process(struct atbmwifi_vif *priv, void *param)
{
	struct p2p_go_neg_results *res = (struct p2p_go_neg_results *)param;
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	atbm_uint8 op_class, op_channel;
	int len;

	if(res == NULL){
		return;
	}

	p2p_freq_to_channel(res->freq, &op_class, &op_channel);

	if (res->role_go) {
		atbm_disable_listening(priv);
		atbmwifi_stop_iftype(priv, priv->iftype);
		len = strlen(res->passphrase);
//		atbm_wifi_set_config(res->ssid, res->ssid_len, 
//					res->passphrase, len, ATBM_KEY_WPA2 , 0);	
		
//		hmac_cfg->channel_no = p2pdata->cfg->channel;
		priv->p2p_ap = 1;

		printf("wps ap ssid:%s,pwd:%s, channel:%d\n", 
			res->ssid, res->passphrase, op_channel);
		atbmwifi_start_iftype(priv, ATBM_NL80211_IFTYPE_P2P_GO);
//		atbmwifi_start_iftype(priv, ATBM_NL80211_IFTYPE_AP);
//		wifi_StartAP(res->ssid, res->ssid_len, (atbm_uint8 *)res->passphrase, len, p2pdata->cfg->channel,ATBM_KEY_WPA2, 0);
		wifi_SetAPConfig(priv,(unsigned char *)res->ssid, res->ssid_len, (atbm_uint8 *)res->passphrase, len, op_channel, ATBM_KEY_WPA2, 0);
		atbmwifi_wpa_event_start_ap(priv);

		wpas_p2p_group_init(priv, res);

		if(priv->appdata == NULL){
			printf("hostapd wps NULL\n");
			return;
		}
		
//		hmac_cfg->dev_password_len = 0; 
		priv->pbc = 1;
		priv->pin = 0;
		hostapd_wps_button_pushed(priv->appdata, res->peer_interface_addr); 		
		
	} else {
		wpa_printf(MSG_INFO, "res->wps_method:%d channel:%d\n", res->wps_method, op_channel);
		if(res->wps_method == WPS_PBC){
			priv->ssid_length = priv->config.ssid_len = res->ssid_len;
			atbm_memset(priv->ssid, 0, ATBM_IEEE80211_MAX_SSID_LEN);
			atbm_memset(priv->config.ssid, 0, ATBM_IEEE80211_MAX_SSID_LEN);
			atbm_memcpy(priv->ssid, res->ssid, res->ssid_len);
			atbm_memcpy(priv->config.ssid, res->ssid, res->ssid_len);
			priv->p2p_scan = 0;
			priv->p2p_join = 1;
			priv->iftype = ATBM_NL80211_IFTYPE_P2P_CLIENT;
			atbm_disable_listening(priv);
			atbm_SleepMs(10);
			wpas_p2p_assoc_req_ie(priv, res->peer_interface_addr, 1);
#if FAST_CONNECT_MODE
			atbm_wifi_set_fast_connect_mode(1, op_channel, ATBM_NULL);
#endif
			wpas_wps_p2p_start_pbc(priv->appdata, res->peer_interface_addr, 1);
		}
	}	
}

static void p2p_go_disc_resp_process(struct atbmwifi_vif *priv)
{	
//	hmac_cfg->dev_password_len = 0; 
	priv->pbc = 1;
	priv->pin = 0;
	hostapd_wps_button_pushed(priv->appdata, NULL); 		
}

static void p2p_rx_action_process(struct atbmwifi_vif *priv, void *param)
{	
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	struct p2p_action *action = (struct p2p_action *)param;

	if((action == NULL) || (p2pdata == NULL)){
		return;
	}

	p2p_rx_action(p2pdata, action->da, action->sa, action->bssid, 
					action->category, action->data, action->len, action->freq);
	atbm_kfree(action->data);
}


static void p2p_action_ack_process(struct atbmwifi_vif *priv, void *param)
{	
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	struct p2p_action_ack *action_ack = (struct p2p_action_ack *)param;

	if((action_ack == NULL) || (p2pdata == NULL)){
		return;
	}

	//printf("action_ack->res:%d\n", action_ack->res);
	
	p2p_send_action_cb(p2pdata, 0, action_ack->da, action_ack->sa, 
						action_ack->bssid, action_ack->res);

}

static void p2p_main_thread(void *arg)
{
	int ret;
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)arg;
	struct p2p_task_msg msg;
	int size = sizeof(struct p2p_task_msg);

	atbm_p2p_init(priv);

	while(1){
		atbm_memset(&msg, 0, sizeof(struct p2p_task_msg));
		ret = atbm_os_MsgQ_Recv(&priv->p2p_task_msg, &msg, sizeof(struct p2p_task_msg), ATBM_TX_WAIT_FOREVER);
		if(ret){
			continue;
		}
		
		switch(msg.event){
			case JOIN_SCAN_END:
				p2p_join_scan_process(priv);
				break;
			case GO_NEG_PEER_WAIT:
				p2p_go_peer_wait_process(priv);
				break;
			case GO_NEG_COMPLETED:
				p2p_go_complete_process(priv, msg.param);
				break;
			case GO_DISC_RESP:
				p2p_go_disc_resp_process(priv);
				break;
			case P2P_RX_ACTION:
				p2p_rx_action_process(priv, msg.param);
				break;
			case P2P_ACTION_ACK:
				p2p_action_ack_process(priv, msg.param);
				break;
			default:
				break;
		}
		if(msg.param){
			atbm_kfree(msg.param);
			msg.param = NULL;
		}
	}
}

static int p2p_main_task_creat(struct atbmwifi_vif *priv)
{
	pAtbm_thread_t p2p_thread;

	priv->p2p_task_msg_stack = (atbm_void *)atbm_kmalloc(P2P_QUEUE_NUM * sizeof(struct p2p_task_msg), GFP_KERNEL);
	if(!priv->p2p_task_msg_stack){
		return -1;
	}
	atbm_os_MsgQ_Create(&priv->p2p_task_msg, priv->p2p_task_msg_stack, sizeof(struct p2p_task_msg), P2P_QUEUE_NUM);

	p2p_thread = atbm_createThread(p2p_main_thread, (atbm_void*)priv, P2P_TASK_PRIORITY);
	priv->p2p_task = p2p_thread;

	return 0;
}

static int p2p_main_task_release(struct atbmwifi_vif *priv)
{
	if(priv->p2p_task){
		atbm_stopThread(priv->p2p_task);
		priv->p2p_task = NULL;
	}

	atbm_os_MsgQ_Delete(&priv->p2p_task_msg);

	if(priv->p2p_task_msg_stack){
		atbm_kfree(priv->p2p_task_msg_stack);
		priv->p2p_task_msg_stack = ATBM_NULL;
	}

	return 0;
}

atbm_uint8 * atbm_p2p_add_scan_ie(struct atbmwifi_vif *priv, atbm_uint8 * pos)
{
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	struct wpabuf *wpabuf = (struct wpabuf *)priv->p2p_scan_ie;

	if(p2pdata == NULL || wpabuf == NULL){
		return pos;
	}

	atbm_memcpy(pos, wpabuf->buf, wpabuf->used);
	
	pos += wpabuf->used;

	return pos;
}

atbm_uint8 *atbm_p2p_add_ap_beacon_ie(struct atbmwifi_vif *priv, atbm_uint8 * pos)
{
	struct wpabuf *wpabuf = (struct wpabuf *)priv->p2p_go_beacon_ie;
	
	if(priv->p2p_go_beacon_ie == NULL){
		return pos;
	}
	atbm_memcpy(pos, wpabuf->buf, wpabuf->used);
	
	pos += wpabuf->used;

	return pos;	
}

atbm_uint8 *atbm_p2p_add_ap_pbresp_ie(struct atbmwifi_vif *priv, atbm_uint8 * pos)
{
	struct wpabuf *wpabuf = (struct wpabuf *)priv->p2p_go_probe_resp_ie;
	
	if(priv->p2p_go_probe_resp_ie == NULL){
		return pos;
	}
	atbm_memcpy(pos, wpabuf->buf, wpabuf->used);
	
	pos += wpabuf->used;

	return pos;	
}

atbm_uint8 *atbm_p2p_add_ap_assoc_resp_ie(struct atbmwifi_vif *priv, atbm_uint8 * pos)
{
	struct wpabuf *wpabuf;

	wpabuf = p2p_group_assoc_resp_ie(priv->p2pgroup, 0);
	
	if(wpabuf == NULL){
		return pos;
	}
	
	atbm_memcpy(pos, wpabuf->buf, wpabuf->used);
	
	pos += wpabuf->used;
	
	wpabuf_free(wpabuf);

	return pos;	
}

void atbm_p2p_wps_sucess(struct atbmwifi_vif *priv, int registrar)
{
#if 0
	if(registrar){
		p2p_group_notif_formation_done(priv->p2pgroup);
	}else{
		
	}
#endif
}

int atbm_p2p_prash_ie(struct atbmwifi_vif *priv, atbm_uint8 *bssid, 
				atbm_uint16 freq, atbm_int8 rssi, atbm_uint8 *data, atbm_uint16 len){

	struct p2p_bss *bss;
	struct atbmwifi_ieee802_11_elems elems;
	int i;
	
	atbm_ieee802_11_parse_elems(data,len,&elems);
	
#if 0
	printf("prash ssid:");
	for(i=0; i<elems.ssid_len; i++){
		printf("%c", elems.ssid[i]);
	}
	printf("\n");
#endif
#if 0	//部分P2P设备的SSID非 "DIRECT-"
	if((elems.ssid_len != priv->ssid_length) 
		|| atbm_memcmp(elems.ssid, priv->ssid, priv->ssid_length)){
		return -1;
	}
#endif
	if((elems.p2p_ie == NULL) || (elems.wps_ie == NULL)){
		return -1;
	}	
	
	bss = (struct p2p_bss *)atbm_kmalloc(sizeof(struct p2p_bss), GFP_KERNEL);
	if(bss == NULL){
		return -1;
	}
	bss->res = (atbm_uint8 *)atbm_kmalloc(len, GFP_KERNEL);
	if(bss->res == NULL){
		atbm_kfree(bss);
		return -1;
	}
	atbm_memcpy(bss->bssid, bssid, 6);
	bss->rssi = (int)rssi; 
	bss->freq = freq;
	atbm_memcpy(bss->res, data, len);
	bss->res_len = len;
	dl_list_add(&p2p_bss_list, &bss->list);
	return 0;
}

int atbm_p2p_probe_req_rx(struct atbmwifi_vif *priv, const atbm_uint8 *addr,
			  const atbm_uint8 *dst, const atbm_uint8 *bssid,
			  const atbm_uint8 *ie, atbm_size_t ie_len,
			  unsigned int rx_freq, int ssi_signal)
{
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	
	if (p2pdata == NULL){
		return -1;
	}	
	//printf("rx pb_req from:"MACSTR"\n", MAC2STR(addr));
	return p2p_probe_req_rx(p2pdata, addr, dst, bssid,
				 		ie, ie_len, rx_freq, priv->p2p_join);
}

void atbm_p2p_rx_action(struct atbmwifi_vif *priv, const atbm_uint8 *da,
			const atbm_uint8 *sa, const atbm_uint8 *bssid,
			atbm_uint8 category, const atbm_uint8 *data, atbm_size_t len, atbm_uint16 freq)
{
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	struct p2p_action *action = NULL;
	struct p2p_task_msg msg;

	if (p2pdata == NULL){
		return;
	}
	if(category != WLAN_ACTION_PUBLIC){
		return;
	}
	
	action = (struct p2p_action *)atbm_kmalloc(sizeof(struct p2p_action), GFP_KERNEL);
	if(action == NULL){
		return;
	}
	action->data = (atbm_uint8 *)atbm_kmalloc(len, GFP_KERNEL);
	if(action->data == NULL){
		atbm_kfree(action);
		return;
	}
	atbm_memcpy(action->bssid, bssid, 6);
	atbm_memcpy(action->sa, sa, 6);
	atbm_memcpy(action->da, da, 6);
	atbm_memcpy(action->data, data, len);
	action->category = category;
	action->freq = freq;
	action->len = len;
	
	msg.event = P2P_RX_ACTION;
	msg.param = action;

	atbm_os_MsgQ_Send(&priv->p2p_task_msg, &msg, sizeof(struct p2p_task_msg), ATBM_TX_WAIT_FOREVER);
//	p2p_rx_action(p2pdata, da, sa, bssid, category, data, len, freq);
}


void atbm_p2p_tx_action_ack(struct atbmwifi_vif *priv, struct atbmwifi_ieee80211_mgmt *mgmt, int status)
{
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	struct p2p_action_ack *action_ack = NULL;
	struct p2p_task_msg msg;
	
	if(p2pdata == NULL){
		return;
	} 
	if(mgmt->u.action.category != WLAN_ACTION_PUBLIC){
		return;
	}
	
	action_ack = (struct p2p_action_ack *)atbm_kmalloc(sizeof(struct p2p_action_ack), GFP_KERNEL);
	if(action_ack == NULL){
		return;
	}

	atbm_memcpy(action_ack->bssid, mgmt->bssid, 6);
	atbm_memcpy(action_ack->da, mgmt->da, 6);
	atbm_memcpy(action_ack->sa, mgmt->sa, 6);

	//printf("action ack status:%X\n", status);
	if(status){
		action_ack->res = P2P_SEND_ACTION_NO_ACK;
	}else{
		action_ack->res = P2P_SEND_ACTION_SUCCESS;
	}
	
	msg.event = P2P_ACTION_ACK;
	msg.param = action_ack;

	atbm_os_MsgQ_Send(&priv->p2p_task_msg, &msg, sizeof(struct p2p_task_msg), ATBM_TX_WAIT_FOREVER);
}

void atbm_p2p_dev_info_set(struct atbmwifi_vif *priv, struct wps_context *wps_ctx)
{
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;

	if(p2pdata == NULL){
		return;
	}
	atbm_memcpy(wps_ctx->dev.mac_addr, p2pdata->cfg->dev_addr, ATBM_ETH_ALEN);
	wps_ctx->dev.device_name = p2pdata->cfg->dev_name;
	wps_ctx->dev.manufacturer = p2pdata->cfg->manufacturer;
	wps_ctx->dev.model_name = p2pdata->cfg->model_name;
	wps_ctx->dev.model_number = p2pdata->cfg->model_number;
	wps_ctx->dev.serial_number = p2pdata->cfg->serial_number;
	wps_ctx->config_methods = p2pdata->cfg->config_methods;		
}

int atbm_p2p_init(struct atbmwifi_vif *priv)
{
	struct p2p_config p2p;
	atbm_size_t ielen;
	struct wpabuf *ies;
	struct p2p_data *p2pdata;
	struct wps_context *wps_ctx;
	struct wpa_supplicant *wpa_s;

	if(priv->iftype != ATBM_NUM_NL80211_IFTYPES){
		atbmwifi_stop_iftype(priv, priv->iftype);
	}
	atbmwifi_start_iftype(priv, ATBM_NL80211_IFTYPE_P2P_CLIENT);
//	atbmwifi_start_iftype(priv, ATBM_NL80211_IFTYPE_STATION);
	wpa_s = (struct wpa_supplicant *)priv->appdata;

	priv->iftype = ATBM_NL80211_IFTYPE_P2P_CLIENT;
	priv->p2p_ap = 0;
	priv->p2p_join = 0;
	priv->p2p_scan = 0;

	if(priv->p2p_wps_privkey == ATBM_NULL){
		if(dh5_init((struct wpabuf **)&priv->p2p_wps_privkey, (struct wpabuf **)&priv->p2p_wps_pubkey) == NULL)
			return -1;
	}	

	if(wpas_wps_p2p_init(wpa_s)){
		return -1;
	}

	atbm_memset(&p2p, 0, sizeof(p2p));
	p2p.cb_ctx = priv;
	p2p.debug_print = wpas_p2p_debug_print;
	p2p.p2p_scan = wpas_p2p_scan;
	p2p.send_action = wpas_send_action;
	p2p.send_action_done = wpas_send_action_done;
	p2p.go_neg_completed = wpas_go_neg_completed;
	p2p.go_neg_req_rx = wpas_go_neg_req_rx;
	p2p.peer_waiting_cb = wpas_peer_waiting_cb;
	p2p.dev_found = wpas_dev_found;
	p2p.dev_lost = wpas_dev_lost;
	p2p.find_stopped = wpas_find_stopped;
	p2p.start_listen = wpas_start_listen;
	p2p.stop_listen = wpas_stop_listen;
	p2p.send_probe_resp = wpas_send_probe_resp;
//	p2p.sd_request = wpas_sd_request;
//	p2p.sd_response = wpas_sd_response;
	p2p.prov_disc_req = wpas_prov_disc_req;
	p2p.prov_disc_resp = wpas_prov_disc_resp;
	p2p.prov_disc_fail = wpas_prov_disc_fail;
//	p2p.invitation_process = wpas_invitation_process;
//	p2p.invitation_received = wpas_invitation_received;
//	p2p.invitation_result = wpas_invitation_result;
//	p2p.get_noa = wpas_get_noa;
	p2p.go_connected = wpas_go_connected;
//	p2p.presence_resp = wpas_presence_resp;
	p2p.is_concurrent_session_active = wpas_is_concurrent_session_active;
	p2p.is_p2p_in_progress = wpas_p2p_in_progress;
//	p2p.get_persistent_group = wpas_get_persistent_group;
	p2p.get_go_info = wpas_get_go_info;
//	p2p.remove_stale_groups = wpas_remove_stale_groups;
//	p2p.p2ps_prov_complete = wpas_p2ps_prov_complete;
	p2p.prov_disc_resp_cb = wpas_prov_disc_resp_cb;
	p2p.p2ps_group_capability = p2ps_group_capability;
//	p2p.get_pref_freq_list = wpas_p2p_get_pref_freq_list;

	wps_ctx = wpa_s->wsc_data->wps_ctx;

	atbm_memcpy(p2p.dev_addr, priv->mac_addr, ATBM_ETH_ALEN);	
	
	p2p.dev_name = wps_ctx->dev.device_name;
	p2p.manufacturer = wps_ctx->dev.manufacturer;
	p2p.model_name = wps_ctx->dev.model_name;
	p2p.model_number = wps_ctx->dev.model_number;
	p2p.serial_number = wps_ctx->dev.serial_number;	

	atbm_memcpy(p2p.uuid, wps_ctx->uuid, 16);
	p2p.config_methods = wps_ctx->config_methods;
	
	atbm_memcpy(p2p.pri_dev_type, wps_ctx->dev.pri_dev_type,
			  WPS_DEV_TYPE_LEN);
	p2p.num_sec_dev_types = 0;
	//atbm_memcpy(p2p.sec_dev_type, wpa_s->conf->sec_device_type,
	//	  		p2p.num_sec_dev_types * WPS_DEV_TYPE_LEN);


	if (wpas_p2p_setup_channels(&p2p.channels, &p2p.cli_channels)) {
		p2p_printf(MSG_ERROR,
			   "P2P: Failed to configure supported channel list");
		return -1;
	}	
	
	p2p.reg_class = P2P_LISTEN_REG_CLASS;
	p2p.channel = P2P_LISTEN_CHANNEL;
	p2p.channel_forced = 1;	

	p2p_printf(MSG_DEBUG, "P2P: Own listen channel: %d:%d",
		   p2p.reg_class, p2p.channel);	

	p2p.op_reg_class = P2P_LISTEN_REG_CLASS;
	p2p.op_channel = P2P_LISTEN_CHANNEL;
	p2p.cfg_op_channel = 1;
	p2p_printf(MSG_DEBUG, "P2P: Configured operating channel: "
		   "%d:%d", p2p.op_reg_class, p2p.op_channel);

	atbm_memcpy(p2p.country, "XX\x04", 3);

	p2p.concurrent_operations = 0;
	p2p.max_peers = P2P_MAX_PEERS; 
	p2p.p2p_intra_bss = 0;
	p2p.max_listen = 1000;
	p2p.passphrase_len = 8;	

	p2pdata = p2p_init(&p2p);
	priv->p2pdata = p2pdata;
	if(priv->p2pdata == NULL){
		p2p_printf(MSG_ERROR, "P2P: init error"); 
		return -1;
	}
	p2p_global = priv->p2pdata;
	p2p_global->ctx = priv;

	ielen = p2p_scan_ie_buf_len(p2pdata);	
	ies = wpabuf_alloc(ielen);
	if (ies == NULL) {
		p2p_printf(MSG_ERROR, "P2P: P2P_IE malloc error");
		return -1;
	}

	p2p_scan_ie(p2pdata, ies, NULL, BAND_2_4_GHZ);
	priv->p2p_scan_ie = ies;		//PB组包需要添加P2P_IER

//	MIB_Flags |= MIB_F_P2P_STARTED;
#ifdef CONFIG_WIFI_DISPLAY
	wifi_display_subelem_set(priv, 0, "000600111c440032");
	wifi_display_enable(priv, 1);
#endif

	return 0;
}

int atbm_p2p_find(struct atbmwifi_vif *priv, unsigned int timeout,
		  enum p2p_discovery_type type,
		  unsigned int num_req_dev_types, const atbm_uint8 *req_dev_types,
		  const atbm_uint8 *dev_id, unsigned int search_delay,
		  atbm_uint8 seek_cnt, const char **seek_string, int freq, int autoconn)
{
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	struct wpa_supplicant *wpa_s;

	if (p2pdata == NULL){
		return -1;
	}

	if(priv->p2p_ap){
		priv->p2p_ap = 0;
	}

	if(priv->iftype != ATBM_NUM_NL80211_IFTYPES){
		atbmwifi_stop_iftype(priv, priv->iftype);
	}

	atbmwifi_start_iftype(priv, ATBM_NL80211_IFTYPE_P2P_CLIENT);

	wpa_s = (struct wpa_supplicant *)priv->appdata;
	if(!wpa_s->wsc_data && wpas_wps_p2p_init(wpa_s)){
		return -1;
	}

	p2p_flush(p2pdata);

	priv->p2p_auto_go_req = autoconn;

	return p2p_find(p2pdata, timeout, type,
				num_req_dev_types, req_dev_types, dev_id,
				search_delay, seek_cnt, seek_string, freq);
}

void atbm_p2p_stop_find(struct atbmwifi_vif *priv)
{
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;

	if (p2pdata == NULL){
		return;
	}
	
	p2p_stop_find(p2pdata);
}

int atbm_p2p_connect(struct atbmwifi_vif *priv, const atbm_uint8 *peer_addr,
		     const char *pin, enum p2p_wps_method wps_method,
		     int persistent_group, int auto_join, int join, int auth,
		     int go_intent, int freq, const atbm_uint8 *group_ssid,
		     atbm_size_t group_ssid_len)
{
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)priv->appdata;
	
	if(p2pdata == NULL){
		return -1;
	}

	if(priv->p2p_go_neg_process){
		return -1;
	}

	priv->p2p_ap = 0;

	if (go_intent < 0 || go_intent > 15){
		go_intent = P2P_DEFULT_GO_INTENT;
	}
	priv->p2p_go_intent = go_intent;
	priv->p2p_wps_method = wps_method;
	
	if (pin){
		os_strlcpy(priv->p2p_pin, pin, sizeof(priv->p2p_pin));
	}else if(wps_method == WPS_PIN_DISPLAY){
		
	}else if(wps_method == WPS_P2PS){
		os_strlcpy(priv->p2p_pin, "12345670", sizeof(priv->p2p_pin));
	}else{
		priv->p2p_pin[0]='\0';
	}
		
	if(join || auto_join){
		atbm_uint8 iface_addr[ATBM_ETH_ALEN], dev_addr[ATBM_ETH_ALEN];
		atbm_memcpy(dev_addr, peer_addr, ATBM_ETH_ALEN);
		
		if (p2p_get_interface_addr(p2pdata, peer_addr,iface_addr) < 0) {
			atbm_memcpy(iface_addr, peer_addr, ATBM_ETH_ALEN);
			p2p_get_dev_addr(p2pdata, peer_addr,dev_addr);
		}
		if (auto_join){

		}
		priv->user_initiated_pd = 1;

		if(priv->iftype != ATBM_NL80211_IFTYPE_P2P_CLIENT && priv->iftype != ATBM_NUM_NL80211_IFTYPES){
			atbmwifi_stop_iftype(priv, priv->iftype);
		}
		
		atbmwifi_start_iftype(priv, ATBM_NL80211_IFTYPE_P2P_CLIENT);
		
		wpa_s = (struct wpa_supplicant *)priv->appdata;
		if(!wpa_s->wsc_data && wpas_wps_p2p_init(wpa_s)){
			return -1;
		}

		if (wpas_p2p_join(priv, iface_addr, dev_addr, wps_method,
				  auto_join, freq, group_ssid, group_ssid_len) < 0){
			return -1;
		}
		return 0;
	}

	if (wpas_p2p_start_go_neg(priv, peer_addr, wps_method,
				  go_intent, priv->mac_addr, freq,
				  persistent_group, NULL, 0) < 0) {
		return -1;
	}
	priv->p2p_go_neg_process = 1;

	return 0;
}

int atbm_p2p_listen(struct atbmwifi_vif *priv, unsigned int timeout,
	int autoconn, int channel)
{
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	struct wpa_supplicant *wpa_s;

	if (p2pdata == NULL){
		return -1;
	}

	if(priv->p2p_ap){
		priv->p2p_ap = 0;
	}

	if(priv->iftype != ATBM_NUM_NL80211_IFTYPES){
	 atbmwifi_stop_iftype(priv, priv->iftype);
	}

	atbmwifi_start_iftype(priv, ATBM_NL80211_IFTYPE_P2P_CLIENT);

	wpa_s = (struct wpa_supplicant *)priv->appdata;
	if(!wpa_s->wsc_data && wpas_wps_p2p_init(wpa_s)){
	 return -1;
	}

	p2p_flush(p2pdata);

	priv->p2p_auto_go_req = autoconn;
	p2pdata->cfg->channel = channel;
	p2pdata->cfg->op_channel = channel;

	return p2p_listen(p2pdata, timeout);
}

int atbm_p2p_go_start(struct atbmwifi_vif *priv)
{
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)priv->appdata;
	struct p2p_go_neg_results *res = NULL;
	int len;
	
	if(p2pdata == NULL){
		return -1;
	}

	atbm_disable_listening(priv);
	atbmwps_cancel(priv);

	if(priv->iftype != ATBM_NUM_NL80211_IFTYPES)
		atbmwifi_stop_iftype(priv, priv->iftype);

	priv->p2p_ap = 1;

	res = (struct p2p_go_neg_results *)atbm_kmalloc(sizeof(struct p2p_go_neg_results), GFP_KERNEL);
	if(res == NULL){
		return -1;
	}

	atbm_memset(res, 0, sizeof(struct p2p_go_neg_results));
	p2p_build_ssid(p2pdata, res->ssid, &res->ssid_len);
	p2p_random(res->passphrase, p2pdata->cfg->passphrase_len);

	len = strlen(res->passphrase);

	wpa_printf(MSG_INFO, "wps ap ssid:%s,pwd:%s, channel:%d\n", 
		res->ssid, res->passphrase, p2pdata->cfg->channel);

	atbmwifi_start_iftype(priv, ATBM_NL80211_IFTYPE_P2P_GO);

	wpas_p2p_group_init(priv, res);

	wifi_SetAPConfig(priv,(unsigned char *)res->ssid, res->ssid_len, (atbm_uint8 *)res->passphrase, len, p2pdata->cfg->channel, ATBM_KEY_WPA2, 0);
	atbmwifi_wpa_event_start_ap(priv);

	atbm_kfree(res);

	return 0;
}

void atbm_p2p_go_stop(struct atbmwifi_vif *priv){
	atbm_disable_listening(priv);
	atbmwps_deinit(priv);
	atbmwifi_stop_iftype(priv, priv->iftype);
	wpas_p2p_group_deinit(priv);

	priv->p2p_scan = 0;
	priv->p2p_join = 0;
	priv->p2p_auto_join = 0;

	if(priv->p2p_go_beacon_ie){
		wpabuf_free(priv->p2p_go_beacon_ie);
		priv->p2p_go_beacon_ie = ATBM_NULL;
	}
	if(priv->p2p_go_probe_resp_ie){
		wpabuf_free(priv->p2p_go_probe_resp_ie);
		priv->p2p_go_probe_resp_ie = ATBM_NULL;
	}
}

int atbm_p2p_auto_connect(struct atbmwifi_vif *priv, atbm_uint8 *mac, int go_intent)
{
	return atbm_p2p_connect(priv, mac, NULL, WPS_PBC,
		     	0, 1, 1, 0, go_intent, 0, NULL, 0);
}

int atbm_p2p_find_wait_connect(struct atbmwifi_vif *priv, int go_intent)
{
	priv->p2p_go_intent = go_intent;
	return atbm_p2p_find(priv, 0, 0, 0, NULL, NULL, 0, 0, NULL, 0, 1);
}

int atbm_p2p_find_only(struct atbmwifi_vif *priv, int timeout)
{
	return atbm_p2p_find(priv, timeout, 0, 0, NULL, NULL, 0, 0, NULL, 0, 0);
}

int atbm_p2p_listen_wait_connect(struct atbmwifi_vif *priv, int go_intent, int channel){
	wifi_printk(WIFI_ALWAYS, "priv:%x go_intent:%d channel:%d\n", priv, go_intent, channel);
	priv->p2p_go_intent = go_intent;
	return atbm_p2p_listen(priv, 0, 1, channel);
}

int atbm_p2p_start(struct atbmwifi_vif *priv)
{
	if(priv->p2pdata != NULL){
		iot_printf("p2p is started\n");
		return -1;
	}
	p2p_main_task_creat(priv);
	return 0;
}

int atbm_p2p_get_peers(struct atbmwifi_vif *priv)
{	
	struct p2p_data *p2pdata = (struct p2p_data *)priv->p2pdata;;
	struct p2p_device *dev = NULL;
	char buf[100];
	
	if (p2pdata == NULL){
		return -1;
	}

	dl_list_for_each(dev, &p2pdata->devices, struct p2p_device, list) {
		atbm_memset(buf, 0, 100);
		sprintf(buf,"dev mac:%02X%02X%02X%02X%02X%02X,name:",
			dev->info.p2p_device_addr[0],dev->info.p2p_device_addr[1],
			dev->info.p2p_device_addr[2],dev->info.p2p_device_addr[3],
			dev->info.p2p_device_addr[4],dev->info.p2p_device_addr[5]);
		iot_printf(buf);
		iot_printf(dev->info.device_name);
		iot_printf("\n");
	}
	return 0;	
}

int atbm_p2p_deinit(struct atbmwifi_vif *priv)
{
	if(priv->p2pdata == NULL){
		iot_printf("p2p is stopped\n");
		return -1;
	}

	if(priv->iftype != ATBM_NUM_NL80211_IFTYPES){
		atbmwifi_stop_iftype(priv, priv->iftype);
	}

	atbm_disable_listening(priv);
	atbmwps_cancel(priv);
	atbmwifi_stop();

	if(priv->p2p_ap){
		p2p_group_deinit(priv->p2pgroup);
		priv->p2pgroup = NULL;
		priv->p2p_ap = 0;
	}
	priv->p2p_scan = 0;
	priv->p2p_join = 0;
	priv->p2p_auto_join = 0;
	priv->p2p_go_neg_process = 0;
	priv->p2p_auto_go_req = 0;

	p2p_main_task_release(priv);

	p2p_deinit(priv->p2pdata);
	priv->p2pdata = NULL;

	if(priv->p2p_scan_ie){
		wpabuf_free(priv->p2p_scan_ie);
		priv->p2p_scan_ie = NULL;
	}

	if(priv->p2p_go_beacon_ie){
		wpabuf_free(priv->p2p_go_beacon_ie);
		priv->p2p_go_beacon_ie = NULL;		
	}
	
	if(priv->p2p_go_probe_resp_ie){
		wpabuf_free(priv->p2p_go_probe_resp_ie);
		priv->p2p_go_probe_resp_ie = NULL;		
	}
	
	if(priv->p2p_assoc_req_ie){
		atbm_kfree(priv->p2p_assoc_req_ie);
		priv->p2p_assoc_req_ie = NULL;		
	}
	
	if(priv->p2p_wps_privkey){
		wpabuf_free(priv->p2p_wps_privkey);
		priv->p2p_wps_privkey = NULL;		
	}	
	
	if(priv->p2p_wps_pubkey){
		wpabuf_free(priv->p2p_wps_pubkey);
		priv->p2p_wps_pubkey = NULL;		
	}

//	MIB_Flags &= ~MIB_F_P2P_STARTED;

//	atbmwifi_sta_start(priv);
	
	return 0;
}

int atbm_p2p_restart(struct atbmwifi_vif *priv){
	void *p2p_wps_pubkey = priv->p2p_wps_pubkey;
	void *p2p_wps_privkey = priv->p2p_wps_privkey;

	priv->p2p_wps_pubkey = ATBM_NULL;
	priv->p2p_wps_privkey = ATBM_NULL;
	atbm_p2p_deinit(priv);

	atbm_SleepMs(10);
	priv->p2p_wps_pubkey = p2p_wps_pubkey;
	priv->p2p_wps_privkey = p2p_wps_privkey;
	atbm_p2p_start(priv);
	return 0;
}
#endif


