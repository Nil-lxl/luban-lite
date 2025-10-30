
#include "atbm_hal.h"
#include "atbm_rc80211_pid.h"
extern 	struct atbmwifi_ieee80211_supported_band atbmwifi_band_2ghz;
extern atbm_int32 globle_rate;

#define OS_TICKER_INTV 1000
struct rc_pid_info  hmac_pid_rc;
extern atbm_uint32 atbm_os_random(void);
  atbm_uint8  mcs_support(struct atbmwifi_cfg80211_rate  *sta, atbm_uint8 index)
{	
	struct atbmwifi_ieee80211_mcs_info *mcs = &sta->ht_cap.mcs;
	
	int  streams = (index+(MCS_GROUP_RATES-1))/MCS_GROUP_RATES;
		
	return mcs->rx_mask[streams - 1];

}

  int rate_supported(struct atbmwifi_cfg80211_rate  *sta,int index)
{
	return ((sta == ATBM_NULL) || (sta->support_rates & BIT(index)));
}
#if 0

 int  rate_control_pid_rate_support(struct rc_pid_sta_info *spinfo,struct atbmwifi_cfg80211_rate  *sta,
				 int index)
{
	if(spinfo->b_ht_sta){
		return mcs_support(sta,index);
	}
	else {		
		return rate_supported(sta,index);

	}
}

/* Adjust the rate while ensuring that we won't switch to a lower rate if it
 * exhibited a worse failed frames behaviour and we'll choose the highest rate
 * whose failed frames behaviour is not worse than the one of the original rate
 * target. While at it, check that the new rate is valid. */
 static atbm_void rate_control_pid_adjust_rate(struct rc_pid_info *pinfo,
					 struct atbmwifi_cfg80211_rate  *sta,
					 struct rc_pid_sta_info *spinfo, int adj,
					 struct rc_pid_rateinfo *rinfo)
{
	int tmp = spinfo->txrate_idx;
	int maxrateidx = pinfo->max_rate_index;

	/* Fit the rate found to the nearest supported rate. */
    while (tmp <= maxrateidx && tmp >= 0){
		if (adj < 0)
			tmp--;
		else
			tmp++;
		if(tmp > maxrateidx || tmp < 0)
			break;
		if(rate_control_pid_rate_support(spinfo,sta,rinfo[tmp].index)) {
			spinfo->txrate_idx = rinfo[tmp].index;
			break;
		};
		
	}

}



 static atbm_void rate_control_pid_sample(struct rc_pid_info *pinfo,
				    struct atbmwifi_cfg80211_rate  *sta,
				    struct rc_pid_sta_info *spinfo)
{

	struct rc_pid_rateinfo *rinfo = pinfo->rinfo;
	atbm_uint32 pf;
//	atbm_int32 err_avg;
//	atbm_uint32 err_prop;
//	atbm_uint32 err_int;
//	atbm_uint32 err_der;
	int adj=0;//, i, j, tmp;
//	unsigned long period;
	
	
	spinfo->last_sample = atbm_GetOsTimeMs();

	/* This should never happen, but in case, we assume the old sample is
	 * still a good measurement and copy it. */
	if (atbm_unlikely(spinfo->tx_num_xmit == 0)){
		//pf = spinfo->last_pf;
		return ;
	}
	else {
		pf = spinfo->tx_num_failed * 100 / spinfo->tx_num_xmit;
	}

	spinfo->tx_num_xmit = 0;
	spinfo->tx_num_failed = 0;

	/* If we just switched rate, update the rate behaviour info. */
	if (pf > 40) {
		adj=-1;
	}	
	else if (pf < 15) {
		adj=1;
	}


	/* Change rate. */
	if (adj)
		rate_control_pid_adjust_rate(pinfo,sta, spinfo, adj, rinfo);
}
#endif


static  const atbm_uint16   BitsPerSymbol[RATE_INDEX_MAX] =
     /*   6     9     12     18  24     36    48     54         6.5  13 19.5  26  39  52  58.5  65  */
       {  4,8,22,44,0,0, 24, 36, 48, 72, 96, 144, 192, 216, 26, 52, 78, 104, 156, 208,234, 260,0   };

 static atbm_void  rate_control_pid_tx_status(struct atbmwifi_cfg80211_rate  *sta, atbm_void  *priv_sta,struct txrate_status  * status)
{
	struct rc_pid_sta_info *spinfo = priv_sta;
	int rateidx;
	int pf;
	int i =0;
	int max_rate=0 ;
	int max_throughput=0 ;
	atbm_uint16 txthroughput[RATE_INDEX_MAX];
	if (!spinfo){
		return;
	}

	if(spinfo->b_ht_sta){
		rateidx = spinfo->txrate_idx + RATE_INDEX_N_6_5M;
	}
	else {
		rateidx = spinfo->txrate_idx;
	}

	for(i=0;i<RATE_INDEX_MAX;i++){
		if(status->txcnt[i] > 0){
			pf = (status->txcnt[i]-status->txfail[i]) *100/status->txcnt[i];
			if(spinfo->pf[i]){
				spinfo->pf[i] = (pf*50 + spinfo->pf[i]*50)/100;
			}
			else {
				spinfo->pf[i] = pf;
			}
			spinfo->pf_cnt[i] += status->txcnt[i];
		}
		txthroughput[i]=spinfo->pf[i] * BitsPerSymbol[i];
		if(spinfo->b_ht_sta){
			if(i>=RATE_INDEX_N_6_5M){
				if(txthroughput[i] > max_throughput){
					max_throughput = txthroughput[i];
					max_rate = i;
				}
			}
		}
		else {
			if(i<RATE_INDEX_N_6_5M){
				if(txthroughput[i] > max_throughput){
					max_throughput = txthroughput[i];
					max_rate = i;
				}
			}
		}
	}


	/* Change rate. */
	/*if(rateidx !=  spinfo->txrate_idx)*/{
		if(spinfo->b_ht_sta){
			if(max_rate >= RATE_INDEX_N_6_5M){
				spinfo->txrate_idx = max_rate - RATE_INDEX_N_6_5M;
			}
		}
		else {
				//maxrateidx = pinfo->max_rate_index;
				spinfo->txrate_idx = max_rate;
		}
	}
}


 ATBM_BOOL rate_control_send_low(struct atbmwifi_ieee80211_tx_info *txrc,struct rc_pid_sta_info *spinfo)
{

	if (txrc->flags & ATBM_IEEE80211_TX_CTL_USE_MINRATE) {		
 		txrc->hw_rate_id =  spinfo->lowest_txrate_idx;		
		txrc->control.rates[0].idx = txrc->hw_rate_id ;
		return ATBM_TRUE;
	}
	else if (txrc->flags & ATBM_IEEE80211_TX_CTL_NO_CCK_RATE) {
		
		txrc->hw_rate_id = RATE_INDEX_A_6M;
		txrc->control.rates[0].idx = txrc->hw_rate_id ;
		
		return ATBM_TRUE;
	}
	else if (txrc->flags & ATBM_IEEE80211_TX_CTL_USE_FIXRATE) {
		
		txrc->hw_rate_id = globle_rate;
		txrc->control.rates[0].idx = txrc->hw_rate_id ;
		
		return ATBM_TRUE;
	}

	
	return ATBM_FALSE;
}


 static atbm_void
rate_control_pid_get_rate(struct atbmwifi_cfg80211_rate  *sta,
			  atbm_void *priv_sta,
			 struct atbmwifi_ieee80211_tx_info *txrc,struct atbm_buff *skb)
{
	struct rc_pid_sta_info *spinfo = priv_sta;
	struct rc_pid_info * pinfo = &hmac_pid_rc;
	atbm_uint8 rateidx;	
	atbm_uint8 maxrateidx = pinfo->max_rate_index;	
	atbm_uint32 atbmRandom;
	atbm_uint32 random1;
	atbm_uint32 random2;


	/* Send management frames and NO_ACK data using lowest rate. */
	if (rate_control_send_low(txrc,spinfo))
		return;
	//
	rateidx = spinfo->txrate_idx;
	
	//check if need sample rate
	if (rateidx < maxrateidx){
		atbmRandom = atbm_os_random();
		random1 = atbmRandom%20;//5%
		random2 = atbmRandom%40;
		if(random1 == 9){
			if( spinfo->b_ht_sta){
				rateidx = spinfo->txrate_idx + RATE_INDEX_N_6_5M + 1;
				txrc->ht = 1;
				if((spinfo->pf_cnt[maxrateidx+RATE_INDEX_N_6_5M])==0){
					rateidx = maxrateidx+RATE_INDEX_N_6_5M;
					//txrc->ht = 1;
					//goto __just_set;
				}
			}
			else {
				rateidx = spinfo->txrate_idx + 1;
				txrc->ht = 0;
				if((spinfo->pf_cnt[maxrateidx])==0){					
					rateidx = maxrateidx;
					//txrc->ht = 0;
					//goto __just_set;
				}
			}
			goto __just_set;				
		}
		else if(random2 == 33){
			if( spinfo->b_ht_sta){
				rateidx = atbmRandom%maxrateidx + RATE_INDEX_N_6_5M;
				txrc->ht = 1;
			}
			else {
				rateidx = atbmRandom%maxrateidx;
				txrc->ht = 0;
			}
		}
	}

	// normal control rate
	if( spinfo->b_ht_sta){
		rateidx = spinfo->txrate_idx + RATE_INDEX_N_6_5M;
		txrc->ht = 1;
	}
	else {
		rateidx = spinfo->txrate_idx;
		txrc->ht = 0;
	}

__just_set:
	txrc->hw_rate_id = rateidx;

	txrc->control.rates[0].idx = rateidx;
	txrc->control.rates[0].flags = 0;
	txrc->control.rates[0].count = 1;
	//info->b_HT = rateidx;

}

static  atbm_int8 atbmwifi_rate_lowest_index(struct atbmwifi_ieee80211_supported_band *sband,
		  struct atbmwifi_cfg80211_rate   *sta)
{
	int i;

	for (i = 0; i < sband->n_bitrates; i++){
		if (rate_supported(sta, i))
			return i;
	}
	/* warn when we cannot find a rate. */
	ATBM_WARN_ON_FUNC(1);

	/* and return 0 (the lowest index) */
	return 0;
}

 static atbm_void
rate_control_pid_sta_rate_init(struct atbmwifi_cfg80211_rate *sta, atbm_void *priv_sta)
{
	struct rc_pid_sta_info *spinfo = priv_sta;
	struct rc_pid_info *pinfo = &hmac_pid_rc;
	struct rc_pid_rateinfo *rinfo = pinfo->no_ht_rinfo;	
	struct rc_pid_rateinfo *ht_rinfo = pinfo->ht_rinfo;	
	int i;

	wifi_printk(WIFI_ALWAYS,"rate_control_pid_sta_rate_init \n");

	spinfo->b_ht_sta  = sta->ht;
	spinfo->pinfo = pinfo;
 
	if(spinfo->b_ht_sta){
		pinfo->rinfo = pinfo->ht_rinfo;	
		pinfo->max_rate_index = pinfo->max_ht_rate_index;
		spinfo->txrate_idx = RATE_INDEX_N_19_5M-RATE_INDEX_N_6_5M;
	}
	else {		
		spinfo->txrate_idx = RATE_INDEX_A_24M;//atbmwifi_band_2ghz.bitrates[0].hw_value;
	}
	/* TODO: This routine should consider using RSSI from previous packets
	 * as we need to have IEEE 802.1X auth succeed immediately after assoc..
	 * Until that method is implemented, we will use the lowest supported
	 * rate as a workaround. */

	/* Sort the rates. This is optimized for the most common case (i.e.
	 * almost-sorted CCK+OFDM rates). Kind of bubble-sort with reversed
	 * mapping too. */
	for (i = 0; i <=pinfo->max_no_ht_rate_index; i++) {
		rinfo[i].index = i;
		rinfo[i].rev_index = i;
	}

	for (i = 0; i <= pinfo->max_ht_rate_index; i++) {
		ht_rinfo[i].index = i;
		ht_rinfo[i].rev_index = i;
	}


	spinfo->lowest_txrate_idx = atbmwifi_rate_lowest_index(&atbmwifi_band_2ghz,sta);
}

 static atbm_void rate_control_pid_alloc(void)
{
	struct rc_pid_info *pinfo;

   // max_rates = atbmwifi_band_2ghz.n_bitrates;
	atbm_memset(&hmac_pid_rc,0,sizeof(hmac_pid_rc));
	pinfo = &hmac_pid_rc;
	//initial no ht
	//rinfo = (struct rc_pid_rateinfo *)(pinfo +1);
	pinfo->max_no_ht_rate_index=RATE_INDEX_A_54M;
	//pinfo->no_ht_rinfo = rinfo;
	//initial  ht
	//rinfo = rinfo+max_rates;		
	pinfo->max_ht_rate_index = MCS_GROUP_RATES*CONFIG_HT_MCS_STREAM_MAX_STREAMS -1;
	//pinfo->ht_rinfo = rinfo;
	
	//initial rinfo
	pinfo->rinfo = pinfo->no_ht_rinfo;
	pinfo->max_rate_index = pinfo->max_no_ht_rate_index;
	pinfo->sampling_period = RC_PID_INTERVAL;

	pinfo->oldrate = 0;


	return ;
}

 static atbm_void rate_control_pid_free(void *addr)
{
	//struct rc_pid_info *pinfo = priv;
	
	//atbm_kfree(pinfo);
}

 static atbm_void *rate_control_pid_alloc_sta(void)
{
	struct rc_pid_sta_info *spinfo;
	int i=0;
	
	for(i=0;i<ATBMWIFI__MAX_STA_IN_AP_MODE;i++){
		spinfo = &hmac_pid_rc.spinfo[i];
		if(spinfo->b_valid == 0)
			goto __find;
	}

	return ATBM_NULL;
	
__find:
	spinfo->b_valid = ATBM_TRUE;
	spinfo->last_sample = atbm_GetOsTimeMs();
	//spinfo->pinfo
 
	return spinfo;
}

 static atbm_void rate_control_pid_free_sta(atbm_void *priv_sta)
{
	struct rc_pid_sta_info *spinfo = priv_sta;
	spinfo->b_valid = 0;
	
	//atbm_kfree(priv_sta);
}
struct rate_control_ops mac80211_ratectrl_pid;

 struct rate_control_ops * rate_control_ops_pid_init(atbm_void)
{		
	mac80211_ratectrl_pid.tx_status = rate_control_pid_tx_status;
	mac80211_ratectrl_pid.get_rate =  rate_control_pid_get_rate;
	mac80211_ratectrl_pid.sta_rate_init = rate_control_pid_sta_rate_init;
	mac80211_ratectrl_pid.alloc= 	 rate_control_pid_alloc;
	mac80211_ratectrl_pid.free =    rate_control_pid_free;
	mac80211_ratectrl_pid.alloc_sta = rate_control_pid_alloc_sta;
	mac80211_ratectrl_pid.free_sta =  rate_control_pid_free_sta;
	//
	mac80211_ratectrl_pid.alloc();

	return &mac80211_ratectrl_pid;
}

atbm_void atbmwifi_rate_control_get_rate(struct atbmwifi_vif *priv,struct atbmwifi_txinfo *t)
{
//	int i;
	struct atbmwifi_ieee80211_tx_info *tx_info = t->tx_info;

	tx_info->control.rates[0].idx = -1;
	tx_info->control.rates[0].flags = 0;
	tx_info->control.rates[0].count = 1;

	if((priv->iftype == ATBM_NL80211_IFTYPE_AP)||(priv->iftype ==ATBM_NL80211_IFTYPE_P2P_GO)){
		if((t->sta_priv)&&(t->sta_priv->sta_rc_priv))
			mac80211_ratectrl_pid.get_rate(&t->sta_priv->rate,t->sta_priv->sta_rc_priv,t->tx_info, t->skb);
		else{
			tx_info->flags |= ATBM_IEEE80211_TX_CTL_USE_MINRATE;
		}
	}
	else {
		if(priv->bss.rc_priv)
			mac80211_ratectrl_pid.get_rate(&priv->bss.sta_priv.rate,priv->bss.rc_priv,t->tx_info, t->skb);
		else {
			tx_info->flags |= ATBM_IEEE80211_TX_CTL_USE_MINRATE;		
		}
	}

}

int atbmwifi_ieee80211_tx_h_rate_ctrl(struct atbmwifi_vif *priv,struct atbmwifi_txinfo *t)
{

	/*
	 * If we're associated with the sta at this point we know we can at
	 * least send the frame at the lowest bit rate.
	 */
	atbmwifi_rate_control_get_rate(priv,t);

	
	return 0;
	
}
int atbm_get_tx_hwrate(struct atbmwifi_common *hw_priv,struct atbmwifi_ieee80211_tx_rate *rate)
{
	return rate->idx;
}

atbm_void atbm_tx_policy_put(struct atbmwifi_common *hw_priv, int idx)
{
}

 int atbmwifi_tx_policy_get(struct atbmwifi_common *hw_priv,
		  struct atbmwifi_ieee80211_tx_rate *rates,
		  int count, ATBM_BOOL *renew)
{
	return 3;
}
 int atbm_tx_policy_upload(struct atbmwifi_common *hw_priv)
{
	return 0;
}
 int atbm_policy_upload_work(struct atbm_work_struct *work)
{	
	return 0;
}
