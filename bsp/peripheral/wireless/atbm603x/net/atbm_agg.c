/*
 * HT handling
 *
 * Copyright 2003, Jouni Malinen <jkmaline@cc.hut.fi>
 * Copyright 2002-2005, Instant802 Networks, Inc.
 * Copyright 2005-2006, Devicescape Software, Inc.
 * Copyright 2006-2007	Jiri Benc <jbenc@suse.cz>
 * Copyright 2007, Michael Wu <flamingice@sourmilk.net>
 * Copyright 2007-2010, Intel Corporation
 *
 */

#include "atbm_hal.h"

#if ATBM_DRIVER_PROCESS_BA
/**
 * DOC: TX A-MPDU aggregation
 *
 * Aggregation on the TX side requires setting the hardware flag
 * %IEEE80211_HW_AMPDU_AGGREGATION. The driver will then be handed
 * packets with a flag indicating A-MPDU aggregation. The driver
 * or device is responsible for actually aggregating the frames,
 * as well as deciding how many and which to aggregate.
 *
 * When TX aggregation is started by some subsystem (usually the rate
 * control algorithm would be appropriate) by calling the
 * ieee80211_start_tx_ba_session() function, the driver will be
 * notified via its @ampdu_action function, with the
 * %IEEE80211_AMPDU_TX_START action.
 *
 * In response to that, the driver is later required to call the
 * ieee80211_start_tx_ba_cb_irqsafe() function, which will really
 * start the aggregation session after the peer has also responded.
 * If the peer responds negatively, the session will be stopped
 * again right away. Note that it is possible for the aggregation
 * session to be stopped before the driver has indicated that it
 * is done setting it up, in which case it must not indicate the
 * setup completion.
 *
 * Also note that, since we also need to wait for a response from
 * the peer, the driver is notified of the completion of the
 * handshake by the %IEEE80211_AMPDU_TX_OPERATIONAL action to the
 * @ampdu_action callback.
 *
 * Similarly, when the aggregation session is stopped by the peer
 * or something calling ieee80211_stop_tx_ba_session(), the driver's
 * @ampdu_action function will be called with the action
 * %IEEE80211_AMPDU_TX_STOP. In this case, the call must not fail,
 * and the driver must later call ieee80211_stop_tx_ba_cb_irqsafe().
 */

int atbm_ampdu_action(struct atbmwifi_common *hw_priv,
			struct atbmwifi_vif *priv,
			enum atbmwifi_ieee80211_ampdu_mlme_action action,
			struct atbmwifi_sta_priv *sta_priv, atbm_uint16 tid, atbm_uint16 *ssn,
			atbm_uint8 buf_size, atbm_uint8 hw_token)
{
	/* Aggregation is implemented fully in firmware,
	 * including block ack negotiation.
	 * In case of AMPDU aggregation in RX direction
	 * re-ordering of packets takes place on host. mac80211
	 * needs the ADDBA Request to setup reodering.mac80211 also
	 * sends ADDBA Response which is discarded in the driver as
	 * FW generates the ADDBA Response on its own.*/
	int ret;
	
	if(!hw_priv->driver_setup_ba){
		return 0;
	}
	
	wifi_printk(WIFI_ALWAYS, "AMPDU[%d]:action[%d],tid[%d],ssn[%d],buff_size[%d],token[%d],"MACSTR"\n",
			priv->if_id,action,tid,ssn ? *ssn:0,buf_size,hw_token,MAC2STR(sta_priv->mac));
	switch (action) {
	case ATBM_IEEE80211_AMPDU_RX_START:
	case ATBM_IEEE80211_AMPDU_RX_STOP:
	{
		struct wsm_rx_ba_session ba_session;

		ba_session.mode = action == ATBM_IEEE80211_AMPDU_RX_START ? WSM_RX_BA_SESSION_MODE__ENABLE : WSM_RX_BA_SESSION_MODE__DISABLE;
		ba_session.win_size = buf_size;
		ba_session.tid = tid;
		ba_session.rxtx = 1;
		atbm_memcpy(ba_session.TA,sta_priv->mac,ATBM_ETH_ALEN);
		ba_session.ssn = ssn ? *ssn:0;
		ba_session.timeout = 0;
		ba_session.hw_token = hw_token;
		ba_session.resv2 = 0;
		/* Just return OK to mac80211 */
		ret = wsm_req_rx_ba_session(hw_priv,&ba_session,priv->if_id);
		break;
	}
	case ATBM_IEEE80211_AMPDU_TX_OPERATIONAL:
	case ATBM_IEEE80211_AMPDU_TX_STOP:
	{
		struct wsm_rx_ba_session ba_session;

		ba_session.mode = action == ATBM_IEEE80211_AMPDU_TX_OPERATIONAL ? WSM_RX_BA_SESSION_MODE__ENABLE : WSM_RX_BA_SESSION_MODE__DISABLE;
		ba_session.win_size = buf_size;
		ba_session.tid = tid;
		ba_session.rxtx = 0;
		atbm_memcpy(ba_session.TA,sta_priv->mac,ATBM_ETH_ALEN);
		ba_session.ssn = ssn ? *ssn:0;
		ba_session.timeout = 0;
		ba_session.hw_token = hw_token;
		ba_session.resv2 = 0;
		/* Just return OK to mac80211 */
		ret = wsm_req_rx_ba_session(hw_priv,&ba_session,priv->if_id);
		break;
	}
	case ATBM_IEEE80211_AMPDU_TX_START:
		ret = 0;
		break;
	default:
		ret = -ATBM_ENOTSUPP;
	}
	return ret;
}


static int ieee80211_tx_ba_alloc_hw_token(struct atbmwifi_common *hw_priv)
{
	int token = -1;

	atbm_spin_lock_bh(&hw_priv->aggr_lock);

	if(!hw_priv->driver_setup_ba){
		token = 0;
		goto exit;
	}
	token = atbm_find_first_zero_bit(hw_priv->tx_aggr_map,hw_priv->max_hw_support_tx_aggs);

	if(token < 0 || token >= hw_priv->max_hw_support_tx_aggs){
		token = -1;
		goto exit;
	}

	atbm_set_bit(token,hw_priv->tx_aggr_map);
exit:
	atbm_spin_unlock_bh(&hw_priv->aggr_lock);
	return token;
}

static void ieee80211_tx_ba_free_hw_token(struct atbmwifi_common *hw_priv,int token)
{
	atbm_spin_lock_bh(&hw_priv->aggr_lock);

	if(!hw_priv->driver_setup_ba){
		goto exit;
	}
	
	if(token > hw_priv->max_hw_support_tx_aggs || token < 0){
		goto exit;
	}

	atbm_clear_bit(token,hw_priv->tx_aggr_map);
exit:
	atbm_spin_unlock_bh(&hw_priv->aggr_lock);
}

static int ieee80211_tx_ba_session_limit(struct atbmwifi_sta_priv *sta_priv, int tid)
{
	if(!atbm_TimeAfter(sta_priv->ampdu_mlme.timestamp[tid], 300*HZ)){
		sta_priv->ampdu_mlme.trylimit[tid] = 0;
		sta_priv->ampdu_mlme.timestamp[tid] = atbm_GetOsTimeMs();
	}
	return (++sta_priv->ampdu_mlme.trylimit[tid] <= HT_AGG_TX_BURST);
}
void ieee80211_assign_tid_tx(struct atbmwifi_sta_priv *sta_priv, int tid,
			     struct tid_ampdu_tx *tid_tx)
{
	sta_priv->ampdu_mlme.tid_tx[tid] = tid_tx;
}

int ___ieee80211_stop_tx_ba_session(struct atbmwifi_sta_priv *sta_priv, atbm_uint16 tid,
				    enum ieee80211_back_parties initiator,
				    ATBM_BOOL tx)
{
	struct atbmwifi_vif *priv = sta_priv->priv;
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	struct tid_ampdu_tx *tid_tx;
	int ret;

	atbm_spin_lock_bh(&sta_priv->lock);

	tid_tx = sta_priv->ampdu_mlme.tid_tx[tid];
	if (!tid_tx) {
		atbm_spin_unlock_bh(&sta_priv->lock);
		return -ATBM_ENOENT;
	}
	/* if we're already stopping ignore any new requests to stop */
	if (atbm_test_bit(HT_AGG_STATE_STOPPING, &tid_tx->state)) {
		atbm_spin_unlock_bh(&sta_priv->lock);
		return -ATBM_EALREADY;
	}

	if (atbm_test_bit(HT_AGG_STATE_WANT_START, &tid_tx->state)) {
		/* not even started yet! */
		ieee80211_tx_ba_free_hw_token(hw_priv,tid_tx->hw_token);
		ieee80211_assign_tid_tx(sta_priv, tid, NULL);
		atbm_spin_unlock_bh(&sta_priv->lock);
		atbm_kfree(tid_tx);
		return 0;
	}

	atbm_set_bit(HT_AGG_STATE_STOPPING, &tid_tx->state);
	atbm_spin_unlock_bh(&sta_priv->lock);
#ifdef CONFIG_MAC80211_ATBM_HT_DEBUG
	atbm_printk_agg("Tx BA session stop requested for %pM tid %u\n",
	       sta->sta.addr, tid);
#endif /* CONFIG_MAC80211_ATBM_HT_DEBUG */
	//wifi_printk(WIFI_ALWAYS, "%s:%d\n", __FUNCTION__,__LINE__);
	atbmwifi_eloop_cancel_timeout(sta_addba_resp_timer_expired, sta_priv, tid);

	/*
	 * After this packets are no longer handed right through
	 * to the driver but are put onto tid_tx->pending instead,
	 * with locking to ensure proper access.
	 */
	atbm_clear_bit(HT_AGG_STATE_OPERATIONAL, &tid_tx->state);

	/*
	 * There might be a few packets being processed right now (on
	 * another CPU) that have already gotten past the aggregation
	 * check when it was still OPERATIONAL and consequently have
	 * IEEE80211_TX_CTL_AMPDU set. In that case, this code might
	 * call into the driver at the same time or even before the
	 * TX paths calls into it, which could confuse the driver.
	 *
	 * Wait for all currently running TX paths to finish before
	 * telling the driver. New packets will not go through since
	 * the aggregation session is no longer OPERATIONAL.
	 */

	tid_tx->stop_initiator = initiator;
	tid_tx->tx_stop = tx;

	ret = atbm_ampdu_action(hw_priv, priv,
			       ATBM_IEEE80211_AMPDU_TX_STOP,
			       sta_priv, tid, NULL, 0, tid_tx->hw_token);

	/* HW shall not deny going back to legacy */
	if (ATBM_WARN_ON(ret)) {
		/*
		 * We may have pending packets get stuck in this case...
		 * Not bothering with a workaround for now.
		 */
	}
	
	ieee80211_tx_ba_free_hw_token(hw_priv,tid_tx->hw_token);

	if (tid_tx->stop_initiator == WLAN_BACK_INITIATOR && tid_tx->tx_stop && hw_priv->driver_setup_ba)
		ieee80211_send_delba(priv, sta_priv->mac, tid,
			WLAN_BACK_INITIATOR, ATBM_WLAN_REASON_QSTA_NOT_USE);
	/*
	 * When we get here, the TX path will not be lockless any more wrt.
	 * aggregation, since the OPERATIONAL bit has long been cleared.
	 * Thus it will block on getting the lock, if it occurs. So if we
	 * stop the queue now, we will not get any more packets, and any
	 * that might be being processed will wait for us here, thereby
	 * guaranteeing that no packets go to the tid_tx pending queue any
	 * more.
	 */

	atbm_spin_lock_bh(&sta_priv->lock);
	ieee80211_assign_tid_tx(sta_priv, tid, NULL);
	atbm_spin_unlock_bh(&sta_priv->lock);

	/* future packets must not find the tid_tx struct any more */
	atbmwifi_tx_aggr_queued_nolock(priv, sta_priv, tid_tx, tid);

	atbm_kfree(tid_tx);
	return ret;
}

/*
 * After sending add Block Ack request we activated a timer until
 * add Block Ack response will arrive from the recipient.
 * If this timer expires sta_addba_resp_timer_expired will be executed.
 */
void sta_addba_resp_timer_expired(atbm_void *data1,atbm_void *data2)
{
	/* not an elegant detour, but there is no choice as the timer passes
	 * only one argument, and both sta_info and TID are needed, so init
	 * flow in sta_info_create gives the TID as data, while the timer_to_id
	 * array gives the sta through container_of */
	int tid = (int)data2;
	struct atbmwifi_sta_priv *sta_priv = (struct atbmwifi_sta_priv *)data1;
	struct tid_ampdu_tx *tid_tx;


	/* check if the TID waits for addBA response */
	tid_tx = sta_priv->ampdu_mlme.tid_tx[tid];
	if (!tid_tx ||
	    atbm_test_bit(HT_AGG_STATE_RESPONSE_RECEIVED, &tid_tx->state)) {
		rcu_read_unlock();
		wifi_printk(WIFI_BASETUP, "timer expired on tid %d but we are not "
				"(or no longer) expecting addBA response there\n",
			tid);
		return;
	}

	wifi_printk(WIFI_BASETUP, "addBA response timer expired on tid %d\n", tid);

	ieee80211_stop_tx_ba_session(sta_priv, tid);
}

static inline int ieee80211_ac_from_tid(int tid)
{
	return ieee802_1d_to_ac[tid & 7];
}

void ieee80211_tx_ba_session_handle_start(struct atbmwifi_sta_priv *sta_priv, int tid)
{
	struct atbmwifi_vif *priv = sta_priv->priv;
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	struct tid_ampdu_tx *tid_tx;
	atbm_uint16 start_seq_num;
	int ret;

	tid_tx = sta_priv->ampdu_mlme.tid_tx[tid];
	//wifi_printk(WIFI_ALWAYS, "%s:%d\n", __FUNCTION__,__LINE__);
	/*
	 * Start queuing up packets for this aggregation session.
	 * We're going to release them once the driver is OK with
	 * that.
	 */
	atbm_clear_bit(HT_AGG_STATE_WANT_START, &tid_tx->state);

	/*
	 * Make sure no packets are being processed. This ensures that
	 * we have a valid starting sequence number and that in-flight
	 * packets have been flushed out and no packets for this TID
	 * will go into the driver during the ampdu_action call.
	 */
	//synchronize_net();

	start_seq_num = sta_priv->tid_seq[tid] >> 4;

	ret = atbm_ampdu_action(hw_priv, priv, ATBM_IEEE80211_AMPDU_TX_START,
			       sta_priv, tid, &start_seq_num, 0, tid_tx->hw_token);
	if (ret) {
		wifi_printk(WIFI_DBG_ERROR, "BA request denied - HW unavailable for"
					" tid %d\n", tid);
		atbmwifi_wpa_event_queue(sta_priv, tid, ATBM_NULL, WPA_EVENT_TX_AGGR_QUEUE_PACKET, ATBM_FALSE);
		atbm_kfree(tid_tx);
		return;
	}

	/* activate the timer for the recipient's addBA response */
	atbmwifi_eloop_register_timeout(0,ADDBA_RESP_INTERVAL,sta_addba_resp_timer_expired,sta_priv, tid);
	if (ATBM_WARN_ON(atbm_set_bit(HT_AGG_STATE_DRV_READY, &tid_tx->state))){
		wifi_printk(WIFI_DBG_ERROR, "BA request already send%d\n", tid);
		return;
	}

	wifi_printk(WIFI_BASETUP, "activated addBA response timer on tid %d\n", tid);

	atbm_spin_lock_bh(&sta_priv->lock);
	sta_priv->ampdu_mlme.addba_req_num[tid]++;
	atbm_spin_unlock_bh(&sta_priv->lock);

	/* send AddBA request */
	atbmwifi_ieee80211_send_addba_request(priv, sta_priv->mac, tid,
				     tid_tx->dialog_token, start_seq_num,
				     hw_priv->max_tx_aggregation_subframes,
				     tid_tx->timeout);
}

int ieee80211_start_tx_ba_session(struct atbmwifi_sta_priv *sta_priv, atbm_uint16 tid,
				  atbm_uint16 timeout)
{
	struct tid_ampdu_tx *tid_tx;
	struct atbmwifi_vif *priv = sta_priv->priv;
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	int ret = 0;
	int token = -1;

	if(tid >= STA_TID_NUM)
		return -ATBM_EINVAL;

	wifi_printk(WIFI_BASETUP, "Open BA session requested for "MACSTR" tid %u\n",
	       MAC2STR(sta_priv->mac), tid);
	/*
	 * The aggregation code is not prepared to handle
	 * anything but STA/AP due to the BSSID handling.
	 * IBSS could work in the code but isn't supported
	 * by drivers or the standard.
	 */
	if (priv->iftype != ATBM_NL80211_IFTYPE_STATION &&
	    priv->iftype != ATBM_NL80211_IFTYPE_AP)
		return -ATBM_EINVAL;

	if (sta_priv->flags & WLAN_STA_BLOCK_BA) {
		wifi_printk(WIFI_DBG_MSG, "BA sessions blocked. "
		       "Denying BA session request\n");
		return -ATBM_EINVAL;
	}

	token = ieee80211_tx_ba_alloc_hw_token(hw_priv);
	if(token == -1){
		wifi_printk(WIFI_DBG_ERROR, "BA sessions alloc HW token fail. \n");
		return -ATBM_EINVAL;
	}
	atbm_spin_lock_bh(&sta_priv->lock);

	/* we have tried too many times, receiver does not want A-MPDU */
	if (ieee80211_tx_ba_session_limit(sta_priv,tid) == 0) {
		wifi_printk(WIFI_DBG_ERROR, "BA sessions failure exceed limit. \n");
		ret = -ATBM_EBUSY;
		goto err_unlock_sta;
	}

	tid_tx = sta_priv->ampdu_mlme.tid_tx[tid];
	/* check if the TID is not in aggregation flow already */
	if (tid_tx || sta_priv->ampdu_mlme.tid_start_tx[tid]) {
		wifi_printk(WIFI_BASETUP,"BA request denied - session is not "
				 "idle on tid %u %p %p\n", tid,tid_tx,sta_priv->ampdu_mlme.tid_start_tx[tid]);
		ret = -ATBM_EAGAIN;
		goto err_unlock_sta;
	}

	/* prepare A-MPDU MLME for Tx aggregation */
	atbm_spin_unlock_bh(&sta_priv->lock); //aden add
	tid_tx = atbm_kzalloc(sizeof(struct tid_ampdu_tx), GFP_ATOMIC);
	atbm_spin_lock_bh(&sta_priv->lock);  //aden add
	if (!tid_tx) {
		ret = -ATBM_ENOMEM;
		goto err_unlock_sta;
	}

	atbm_skb_queue_head_init(&tid_tx->pending);
	atbm_set_bit(HT_AGG_STATE_WANT_START, &tid_tx->state);

	tid_tx->timeout = timeout;
	
	/* assign a dialog token */
	sta_priv->ampdu_mlme.dialog_token_allocator++;
	tid_tx->dialog_token = sta_priv->ampdu_mlme.dialog_token_allocator;
	tid_tx->hw_token = token;
	/*
	 * Finally, assign it to the start array; the work item will
	 * collect it and move it to the normal array.
	 */
	sta_priv->ampdu_mlme.tid_start_tx[tid] = tid_tx;

	atbm_queue_work(hw_priv, sta_priv->ampdu_mlme.work);
	/* this flow continues off the work */
 err_unlock_sta:
 	if(ret != 0){
		ieee80211_tx_ba_free_hw_token(hw_priv,token);
 	}
	atbm_spin_unlock_bh(&sta_priv->lock);
	return ret;
}
//EXPORT_SYMBOL(ieee80211_start_tx_ba_session);

static void ieee80211_agg_tx_operational(struct atbmwifi_common *hw_priv,
					 struct atbmwifi_sta_priv *sta_priv, atbm_uint16 tid)
{
	struct tid_ampdu_tx *tid_tx;

	tid_tx = sta_priv->ampdu_mlme.tid_tx[tid];

	wifi_printk(WIFI_BASETUP, "Aggregation is on for tid %d\n", tid);

	//wifi_printk(WIFI_ALWAYS, "%s:%d\n", __FUNCTION__,__LINE__);
	atbm_ampdu_action(hw_priv, sta_priv->priv,
			 ATBM_IEEE80211_AMPDU_TX_OPERATIONAL,
			 sta_priv, tid, ATBM_NULL, tid_tx->buf_size, tid_tx->hw_token);

	/*
	 * Now mark as operational. This will be visible
	 * in the TX path, and lets it go lock-free in
	 * the common case.
	 */
	atbm_set_bit(HT_AGG_STATE_OPERATIONAL, &tid_tx->state);

	/*
	 * synchronize with TX path, while splicing the TX path
	 * should block so it won't put more packets onto pending.
	 */

	atbmwifi_wpa_event_queue(sta_priv, tid, ATBM_NULL, WPA_EVENT_TX_AGGR_QUEUE_PACKET, ATBM_FALSE);	
}

int __ieee80211_stop_tx_ba_session(struct atbmwifi_sta_priv *sta_priv, atbm_uint16 tid,
				   enum ieee80211_back_parties initiator,
				   ATBM_BOOL tx)
{
	int ret;

	atbm_os_mutexLock(&sta_priv->ampdu_mlme.mtx, 0);
	ret = ___ieee80211_stop_tx_ba_session(sta_priv, tid, initiator, tx);
	atbm_os_mutexUnLock(&sta_priv->ampdu_mlme.mtx);

	return ret;
}


int ieee80211_stop_tx_ba_session(struct atbmwifi_sta_priv *sta_priv, atbm_uint16 tid)
{
	struct atbmwifi_vif *priv = sta_priv->priv;
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	struct tid_ampdu_tx *tid_tx;
	int ret = 0;


	if (tid >= STA_TID_NUM)
		return -ATBM_EINVAL;

	atbm_spin_lock_bh(&sta_priv->lock);
	tid_tx = sta_priv->ampdu_mlme.tid_tx[tid];

	if (!tid_tx) {
		ret = -ATBM_ENOENT;
		goto unlock;
	}

	if (atbm_test_bit(HT_AGG_STATE_STOPPING, &tid_tx->state)) {
		/* already in progress stopping it */
		ret = 0;
		goto unlock;
	}

	atbm_set_bit(HT_AGG_STATE_WANT_STOP, &tid_tx->state);
	atbm_queue_work(hw_priv, sta_priv->ampdu_mlme.work);

 unlock:
	atbm_spin_unlock_bh(&sta_priv->lock);
	return ret;
}

void ieee80211_process_addba_resp(struct atbmwifi_common *hw_priv,
				  struct atbmwifi_sta_priv *sta_priv,
				  struct atbmwifi_ieee80211_mgmt *mgmt,
				  atbm_size_t len)
{
	struct tid_ampdu_tx *tid_tx;
	atbm_uint16 capab, tid;
	atbm_uint8 buf_size;

	capab = atbm_le16_to_cpu(mgmt->u.action.u.addba_resp.capab);
	tid = (capab & ATBM_IEEE80211_ADDBA_PARAM_TID_MASK) >> 2;
	buf_size = (capab & ATBM_IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK) >> 6;

	atbm_os_mutexLock(&sta_priv->ampdu_mlme.mtx, 0);
	tid_tx = sta_priv->ampdu_mlme.tid_tx[tid];
	if (!tid_tx)
		goto out;

	if (mgmt->u.action.u.addba_resp.dialog_token != tid_tx->dialog_token) {
		wifi_printk(WIFI_DBG_ERROR, "wrong addBA response token, tid %d\n", tid);
		goto out;
	}

	atbmwifi_eloop_cancel_timeout(sta_addba_resp_timer_expired, sta_priv, tid);

	wifi_printk(WIFI_BASETUP, "switched off addBA timer for tid %d\n", tid);

	/*
	 * addba_resp_timer may have fired before we got here, and
	 * caused WANT_STOP to be set. If the stop then was already
	 * processed further, STOPPING might be set.
	 */
	if (atbm_test_bit(HT_AGG_STATE_WANT_STOP, &tid_tx->state) ||
	    atbm_test_bit(HT_AGG_STATE_STOPPING, &tid_tx->state)) {
		wifi_printk(WIFI_DBG_ERROR,
		       "got addBA resp for tid %d but we already gave up\n",
		       tid);
		goto out;
	}
	//wifi_printk(WIFI_ALWAYS, "%s:%d\n", __FUNCTION__,__LINE__);
	/*
	 * IEEE 802.11-2007 7.3.1.14:
	 * In an ADDBA Response frame, when the Status Code field
	 * is set to 0, the Buffer Size subfield is set to a value
	 * of at least 1.
	 */
	if (atbm_le16_to_cpu(mgmt->u.action.u.addba_resp.status)
			== ATBM_WLAN_STATUS_SUCCESS && buf_size) {
		if (atbm_set_bit(HT_AGG_STATE_RESPONSE_RECEIVED,
				     &tid_tx->state)) {
			/* ignore duplicate response */
			goto out;
		}

		tid_tx->buf_size = buf_size;

		if (atbm_test_bit(HT_AGG_STATE_DRV_READY, &tid_tx->state))
			ieee80211_agg_tx_operational(hw_priv, sta_priv, tid);

		sta_priv->ampdu_mlme.addba_req_num[tid] = 0;
	} else {
		___ieee80211_stop_tx_ba_session(sta_priv, tid, WLAN_BACK_INITIATOR,
						ATBM_TRUE);
	}
 out:
	atbm_os_mutexUnLock(&sta_priv->ampdu_mlme.mtx);
}

static int ieee80211_rx_ba_alloc_hw_token(struct atbmwifi_common *hw_priv)
{
	int token = -1;
	
	atbm_spin_lock_bh(&hw_priv->aggr_lock);

	if(!hw_priv->driver_setup_ba){
		token = 0;
		goto exit;
	}
	token = atbm_find_first_zero_bit(hw_priv->rx_aggr_map,hw_priv->max_hw_support_rx_aggs);

	if(token < 0 || token >= hw_priv->max_hw_support_rx_aggs){
		token = -1;
		goto exit;
	}

	atbm_set_bit(token,hw_priv->rx_aggr_map);
exit:
	atbm_spin_unlock_bh(&hw_priv->aggr_lock);
	return token;
}

static void ieee80211_rx_ba_free_hw_token(struct atbmwifi_common *hw_priv,int token)
{
	atbm_spin_lock_bh(&hw_priv->aggr_lock);
	
	if(!hw_priv->driver_setup_ba){
		goto exit;
	}
	if(token > hw_priv->max_hw_support_rx_aggs || token < 0){
		goto exit;
	}

	atbm_clear_bit(token,hw_priv->rx_aggr_map);
exit:
	atbm_spin_unlock_bh(&hw_priv->aggr_lock);
}

static void ieee80211_free_tid_rx(struct tid_ampdu_rx *tid_rx)
{
	atbm_kfree(tid_rx);
}

void ___ieee80211_stop_rx_ba_session(struct atbmwifi_sta_priv *sta_priv, atbm_uint16 tid,
				     atbm_uint16 initiator, atbm_uint16 reason, ATBM_BOOL tx)
{
	struct atbmwifi_vif *priv = sta_priv->priv;
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	struct atbm_ba_params ba_params;


	if(sta_priv->rx_token[tid] == -1){
		return;
	}

	ba_params.tid = tid;
	ba_params.action = ATBM_BA__ACTION_RX_DELBA;
	atbm_updata_ba_tid_params(priv,&ba_params);

	wifi_printk(WIFI_BASETUP,"Rx BA session stop requested for %pM tid %u\n",
	       sta_priv->mac, tid);

	if (atbm_ampdu_action(hw_priv, priv, ATBM_IEEE80211_AMPDU_RX_STOP,
			     sta_priv, tid, NULL, 0, (atbm_uint8)sta_priv->rx_token[tid]))
		wifi_printk(WIFI_DBG_ERROR, "HW problem - can not stop rx "
				"aggregation for tid %d\n", tid);

	/* check if this is a self generated aggregation halt */
	if (initiator == WLAN_BACK_RECIPIENT && tx && hw_priv->driver_setup_ba)
		ieee80211_send_delba(priv, sta_priv->mac,
				     tid, 0, reason);

	atbmwifi_eloop_cancel_timeout(sta_rx_agg_session_timer_expired, &sta_priv->timer_to_tid[tid], ATBM_NULL);

	ieee80211_rx_ba_free_hw_token(hw_priv,sta_priv->rx_token[tid]);
	sta_priv->rx_token[tid] = -1;
}

void __ieee80211_stop_rx_ba_session(struct atbmwifi_sta_priv *sta_priv, atbm_uint16 tid,
				    atbm_uint16 initiator, atbm_uint16 reason, ATBM_BOOL tx)
{
	atbm_os_mutexLock(&sta_priv->ampdu_mlme.mtx, 0);
	___ieee80211_stop_rx_ba_session(sta_priv, tid, initiator, reason, tx);
	atbm_os_mutexUnLock(&sta_priv->ampdu_mlme.mtx);
}

/*
 * After accepting the AddBA Request we activated a timer,
 * resetting it after each frame that arrives from the originator.
 */
void sta_rx_agg_session_timer_expired(atbm_void *data1,atbm_void *data2)
{
	/* not an elegant detour, but there is no choice as the timer passes
	 * only one argument, and various sta_info are needed here, so init
	 * flow in sta_info_create gives the TID as data, while the timer_to_id
	 * array gives the sta through container_of */
	atbm_uint8 *ptid = (atbm_uint8 *)data1;
	atbm_uint8 *timer_to_id = ptid - *ptid;
	struct atbmwifi_sta_priv *sta_priv = atbm_container_of(timer_to_id, struct atbmwifi_sta_priv,
					 timer_to_tid[0]);
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(sta_priv->priv);

	wifi_printk(WIFI_BASETUP, "rx session timer expired on tid %d\n", (atbm_uint16)*ptid);
	atbm_set_bit(*ptid, sta_priv->ampdu_mlme.tid_rx_timer_expired);
	atbm_queue_work(hw_priv, sta_priv->ampdu_mlme.work);
}

void ieee80211_process_addba_request(struct atbmwifi_common *hw_priv,
				     struct atbmwifi_sta_priv *sta_priv,
				     struct atbmwifi_ieee80211_mgmt *mgmt,
				     int len)
{
	struct tid_ampdu_rx *tid_agg_rx;
	atbm_uint16 capab, tid, timeout, ba_policy, buf_size, start_seq_num;
	atbm_uint16 status;
	int tx = hw_priv->driver_setup_ba;
	struct atbm_ba_params ba_params;

	atbm_uint8 dialog_token;
	int ret = -ATBM_EOPNOTSUPP;
	int hw_token = -1;

	/* extract session parameters from addba request frame */
	dialog_token = mgmt->u.action.u.addba_req.dialog_token;
	timeout = atbm_le16_to_cpu(mgmt->u.action.u.addba_req.timeout);
	start_seq_num =
		atbm_le16_to_cpu(mgmt->u.action.u.addba_req.start_seq_num) >> 4;

	capab = atbm_le16_to_cpu(mgmt->u.action.u.addba_req.capab);
	ba_policy = (capab & ATBM_IEEE80211_ADDBA_PARAM_POLICY_MASK) >> 1;
	tid = (capab & ATBM_IEEE80211_ADDBA_PARAM_TID_MASK) >> 2;
	buf_size = (capab & ATBM_IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK) >> 6;
	status = ATBM_WLAN_STATUS_REQUEST_DECLINED;
	if (sta_priv->flags & WLAN_STA_BLOCK_BA) {
		wifi_printk(WIFI_BASETUP, "Suspend in progress. "
		       "Denying ADDBA request\n");
		goto end_no_lock;
	}

	/* sanity check for incoming parameters:
	 * check if configuration can support the BA policy
	 * and if buffer size does not exceeds max value */
	/* XXX: check own ht delayed BA capability?? */
	if ((ba_policy != 1) &&
	     (!(sta_priv->rate.ht_cap.cap & ATBM_IEEE80211_HT_CAP_DELAY_BA))) {
		status = ATBM_WLAN_STATUS_INVALID_QOS_PARAM;
		wifi_printk(WIFI_DBG_ERROR, "AddBA Req with bad params from "
				"%pM on tid %u. policy %d, buffer size %d\n",
				mgmt->sa, tid, ba_policy,
				buf_size);
		goto end_no_lock;
	}
	/* determine default buffer size */
	if (buf_size == 0)
		buf_size = 64;//IEEE80211_MAX_AMPDU_BUF;

	/* make sure the size doesn't exceed the maximum supported by the hw */
	if (buf_size > hw_priv->max_rx_aggregation_subframes)
		buf_size = hw_priv->max_rx_aggregation_subframes;

	/* examine state machine */
	atbm_os_mutexLock(&sta_priv->ampdu_mlme.mtx, 0);

	if (sta_priv->rx_token[tid] != -1) {
		wifi_printk(WIFI_BASETUP, "unexpected AddBA Req from "
				"%pM on tid %u\n",
				mgmt->sa, tid);

		/* delete existing Rx BA session on the same tid */
		___ieee80211_stop_rx_ba_session(sta_priv, tid, WLAN_BACK_RECIPIENT,
						ATBM_WLAN_STATUS_UNSPECIFIED_QOS,
						ATBM_FALSE);
	}

	/* prepare A-MPDU MLME for Rx aggregation */
	hw_token = ieee80211_rx_ba_alloc_hw_token(hw_priv);

	if (hw_token == -1){
		wifi_printk(WIFI_DBG_ERROR, "aggr token not enough\n");
		goto end;
	}

	ret = atbm_ampdu_action(hw_priv, sta_priv->priv, ATBM_IEEE80211_AMPDU_RX_START,
			       sta_priv, tid, &start_seq_num, 0, hw_token);
	wifi_printk(WIFI_BASETUP, "Rx A-MPDU request on tid %d result %d\n", tid, ret);

	if (ret) {
		ieee80211_rx_ba_free_hw_token(hw_priv, hw_token);
		goto end;
	}

	ba_params.link_id = sta_priv->link_id;
	ba_params.action = ATBM_BA__ACTION_RX_ADDBR;
	ba_params.timeout = timeout;
	capab = atbm_le16_to_cpu(mgmt->u.action.u.addba_req.capab);
	ba_params.tid =  (capab & ATBM_IEEE80211_ADDBA_PARAM_TID_MASK) >> 2;
//	ba_params.win_size =  (capab & IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK) >> 6;
	ba_params.win_size = BUFF_STORED_LEN;
	ba_params.ssn = start_seq_num;
	ba_params.sta_priv = sta_priv;
	atbm_updata_ba_tid_params(sta_priv->priv,&ba_params);

	sta_priv->rx_token[tid] = hw_token;

	status = ATBM_WLAN_STATUS_SUCCESS;

	/* activate it for RX */
	if (timeout)
		atbmwifi_eloop_register_timeout(0, timeout, sta_rx_agg_session_timer_expired, &sta_priv->timer_to_tid[tid], ATBM_NULL);
end:
	atbm_os_mutexUnLock(&sta_priv->ampdu_mlme.mtx);

end_no_lock:
	if(tx)
		ieee80211_send_addba_resp(sta_priv->priv, sta_priv->mac, tid,
				  dialog_token, status, 1, buf_size, timeout);
	return;
}

void ieee80211_process_delba(struct atbmwifi_common *hw_priv,
				     struct atbmwifi_sta_priv *sta_priv,
				     struct atbmwifi_ieee80211_mgmt *mgmt,
				     int len)
{
	atbm_uint16 tid, params;
	atbm_uint16 initiator;

	params = atbm_le16_to_cpu(mgmt->u.action.u.delba.params);
	tid = (params & ATBM_IEEE80211_DELBA_PARAM_TID_MASK) >> 12;
	initiator = (params & ATBM_IEEE80211_DELBA_PARAM_INITIATOR_MASK) >> 11;


	wifi_printk(WIFI_BASETUP, "delba from %pM (%s) tid %d reason code %d\n",
			mgmt->sa, initiator ? "initiator" : "recipient", tid,
			atbm_le16_to_cpu(mgmt->u.action.u.delba.reason_code));

	if (initiator == WLAN_BACK_INITIATOR)
		__ieee80211_stop_rx_ba_session(sta_priv, tid, WLAN_BACK_INITIATOR, 0,
					       ATBM_TRUE);
	__ieee80211_stop_tx_ba_session(sta_priv, tid, WLAN_BACK_RECIPIENT,
				       ATBM_TRUE);
}

void ieee80211_ba_session_work(struct atbm_work_struct *work)
{
	struct atbmwifi_sta_priv *sta_priv = (struct atbmwifi_sta_priv *)work;
#if ATBM_DRIVER_PROCESS_BA
	struct tid_ampdu_tx *tid_tx;
#endif
	int tid;

	/*
	 * When this flag is set, new sessions should be
	 * blocked, and existing sessions will be torn
	 * down by the code that set the flag, so this
	 * need not run.
	 */

	if (sta_priv->flags & WLAN_STA_BLOCK_BA)
		return;

	atbm_os_mutexLock(&sta_priv->ampdu_mlme.mtx, 0);
	for (tid = 0; tid < STA_TID_NUM; tid++) {
		if (atbm_clear_bit(tid, sta_priv->ampdu_mlme.tid_rx_timer_expired))
			___ieee80211_stop_rx_ba_session(
				sta_priv, tid, WLAN_BACK_RECIPIENT,
				ATBM_WLAN_REASON_QSTA_TIMEOUT, ATBM_TRUE);

		if (atbm_clear_bit(tid, sta_priv->ampdu_mlme.tid_rx_stop_requested))
			___ieee80211_stop_rx_ba_session(
				sta_priv, tid, WLAN_BACK_RECIPIENT,
				ATBM_WLAN_REASON_UNSPECIFIED, ATBM_TRUE);

		tid_tx = sta_priv->ampdu_mlme.tid_start_tx[tid];
		if (tid_tx) {
			/*
			 * Assign it over to the normal tid_tx array
			 * where it "goes live".
			 */
			atbm_spin_lock_bh(&sta_priv->lock);

			sta_priv->ampdu_mlme.tid_start_tx[tid] = NULL;
			/* could there be a race? */
			if (sta_priv->ampdu_mlme.tid_tx[tid])
				atbm_kfree(tid_tx);
			else
				ieee80211_assign_tid_tx(sta_priv, tid, tid_tx);
			atbm_spin_unlock_bh(&sta_priv->lock);
			ieee80211_tx_ba_session_handle_start(sta_priv, tid);
			continue;
		}

		tid_tx = sta_priv->ampdu_mlme.tid_tx[tid];
		if (tid_tx && atbm_clear_bit(HT_AGG_STATE_WANT_STOP,
						 &tid_tx->state)){
			___ieee80211_stop_tx_ba_session(sta_priv, tid,
							WLAN_BACK_INITIATOR,
							ATBM_TRUE);
		}

	}
	atbm_os_mutexUnLock(&sta_priv->ampdu_mlme.mtx);
}

void ieee80211_sta_tear_down_BA_sessions(struct atbmwifi_sta_priv *sta_priv, ATBM_BOOL tx)
{
	int i;
	struct atbmwifi_common *hw_priv = sta_priv->priv->hw_priv;

	atbm_cancel_work(hw_priv, sta_priv->ampdu_mlme.work);

	for (i = 0; i <  STA_TID_NUM; i++) {
		__ieee80211_stop_tx_ba_session(sta_priv, i, WLAN_BACK_INITIATOR, tx);
		__ieee80211_stop_rx_ba_session(sta_priv, i, WLAN_BACK_RECIPIENT,
					       ATBM_WLAN_REASON_QSTA_LEAVE_QBSS, tx);
	}
}

ATBM_BOOL ieee80211_tx_prep_agg(struct atbmwifi_sta_priv *sta_priv,
			   struct atbm_buff *skb,
			   struct atbmwifi_ieee80211_tx_info * tx_info,
			   struct tid_ampdu_tx *tid_tx,
			   int tid)
{
	ATBM_BOOL queued = ATBM_FALSE;

	if (atbm_test_bit(HT_AGG_STATE_OPERATIONAL, &tid_tx->state)) {
		tx_info->flags |= ATBM_IEEE80211_TX_CTL_AMPDU;
	} else if (atbm_test_bit(HT_AGG_STATE_WANT_START, &tid_tx->state)) {
		/*
		* nothing -- this aggregation session is being started
		* but that might still fail with the driver
		*/
	} else {
		atbm_spin_lock(&sta_priv->lock);
		/*
		* Need to re-check now, because we may get here
		*
		*  1) in the window during which the setup is actually
		* 	already done, but not marked yet because not all
		* 	packets are spliced over to the driver pending
		* 	queue yet -- if this happened we acquire the lock
		* 	either before or after the splice happens, but
		* 	need to recheck which of these cases happened.
		*
		*  2) during session teardown, if the OPERATIONAL bit
		* 	was cleared due to the teardown but the pointer
		* 	hasn't been assigned NULL yet (or we loaded it
		* 	before it was assigned) -- in this case it may
		* 	now be NULL which means we should just let the
		* 	packet pass through because splicing the frames
		* 	back is already done.
		*/
		tid_tx = sta_priv->ampdu_mlme.tid_tx[tid];
		if (!tid_tx) {
			/* do nothing, let packet pass through */
		} else if (atbm_test_bit(HT_AGG_STATE_OPERATIONAL, &tid_tx->state)) {
			tx_info->flags |= ATBM_IEEE80211_TX_CTL_AMPDU;
		} else {
			queued = ATBM_TRUE;
			atbm_skb_queue_tail(&tid_tx->pending, skb);
		}
		atbm_spin_unlock(&sta_priv->lock);
	}

	return queued;
}

atbm_void atbmwifi_tx_aggr_queued(struct atbmwifi_vif *priv, struct atbmwifi_sta_priv *sta_priv, int tid){
	struct atbmwifi_common * hw_priv = priv->hw_priv;
	struct atbm_buff *skb;
	struct tid_ampdu_tx *tid_tx;
	struct atbmwifi_ieee80211_tx_info * tx_info;

	tid_tx = sta_priv->ampdu_mlme.tid_tx[tid];
	atbm_os_mutexLock(&sta_priv->ampdu_mlme.mtx, 0);
	while ((skb = atbm_skb_dequeue(&tid_tx->pending)) != ATBM_NULL){
		tx_info = ATBM_IEEE80211_SKB_TXCB(skb);
		if(ieee80211_tx_prep_agg(sta_priv, skb, tx_info, tid_tx, tid))
			break;
		ieee80211_tx_h_sequence(sta_priv, skb, tid);
		atbmwifi_tx(hw_priv,skb,priv);
	}
	atbm_os_mutexUnLock(&sta_priv->ampdu_mlme.mtx);
}

atbm_void atbmwifi_tx_aggr_queued_nolock(struct atbmwifi_vif *priv, struct atbmwifi_sta_priv *sta_priv,
		struct tid_ampdu_tx *tid_tx, int tid){
	struct atbmwifi_common * hw_priv = priv->hw_priv;
	struct atbm_buff *skb;

	sta_priv->flags |= WLAN_STA_BLOCK_BA;
	if(tid_tx){
		while ((skb = atbm_skb_dequeue(&tid_tx->pending)) != ATBM_NULL){
			ieee80211_tx_h_sequence(sta_priv, skb, tid);
			atbmwifi_tx(hw_priv,skb,priv);
		}
	}
	sta_priv->flags &= ~WLAN_STA_BLOCK_BA;
}

#endif
