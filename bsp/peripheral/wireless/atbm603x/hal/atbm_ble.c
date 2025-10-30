#include "atbm_hal.h"

#if CONFIG_WIFI_BT_COMB
#define IEEE80211_BLE_SKB_HEADNEED	128
static void  ieee80211_ble_dump(const char *string,atbm_uint8 *mem,atbm_size_t len)
{
#if 0
	atbm_size_t i = 0;
	atbm_printk_err("[%s]:\n",string);

	for(i = 0; i< len ; i++){
		if(!(i % 16)){
			atbm_printk_err("\n");
		}
		atbm_printk_err("[%x]",mem[i]);
	}
#endif	
}
static int ieee80211_ble_thread_wakeup(struct ieee80211_ble_thread *thread)
{
	atbm_os_wakeup_event(&thread->wq);
	return 0;
}

static int ieee80211_ble_thread_deinit(struct ieee80211_ble_thread *thread)
{
	void *bh;
	struct atbmwifi_common	*hw_priv = thread->hw_priv;
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;
	
	atbm_spin_lock_bh(&ble_local->ble_spin_lock);
	bh = thread->thread;
	thread->thread = ATBM_NULL;
	atbm_spin_unlock_bh(&ble_local->ble_spin_lock);
	if (bh){
		atbm_stopThreadInternal(bh);
	}
	atbm_os_delete_waitevent(&thread->wq);
	return 0;
}

static int ieee80211_ble_thread_init(struct ieee80211_ble_thread *thread)
{
	atbm_os_init_waitevent(&thread->wq);

	thread->thread = atbm_createThreadInternal(thread->name,thread->thread_fn, thread, thread->prio);
	
	if (!thread->thread){
		wifi_printk(WIFI_DBG_ERROR, "sdio %s err\n",thread->name);
		return -1;
	}
	return  0;
}
static int ieee80211_ble_xmit_thread(struct ieee80211_ble_thread *thread)
{
	struct atbm_buff *skb;
	struct atbmwifi_common	*hw_priv= thread->hw_priv;
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;

	while(1){
		atbm_os_wait_event_timeout(&thread->wq, 10*HZ);
		while((skb  = atbm_skb_dequeue(&ble_local->ble_xmit_queue))){
			
			/*
			*start tx
			*/
			//printk("[ble xmit]:len [%d]\n",skb->len);
			ieee80211_ble_dump(__FUNCTION__,skb->abuf,skb->dlen);
			atbm_ble_do_ble_xmit(hw_priv,skb->abuf,skb->dlen);
			/*
			*free skb
			*/
			atbm_dev_kfree_skb(skb);
		}
	}
	return 0;
}
static int ieee80211_ble_xmit_init(struct atbmwifi_common	*hw_priv)
{
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;
	struct ieee80211_ble_thread *thread = &ble_local->xmit_thread;
	
	atbm_skb_queue_head_init(&ble_local->ble_xmit_queue);

	thread->flags = 0;
	thread->name  = "ble_xmit";
	thread->period_handle = NULL;
	thread->thread_fn = ieee80211_ble_xmit_thread;
	thread->hw_priv = hw_priv;
	thread->prio = BLE_XMIT_PRIO;

	if(ieee80211_ble_thread_init(thread)){
		wifi_printk(WIFI_DBG_ERROR, "ble_xmit thread err\n");
		return -1;
	}
	
	return 0;
}
void  ieee80211_ble_recv(struct atbmwifi_common	*hw_priv, struct atbm_buff *skb)
{
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;

	atbm_spin_lock_bh(&ble_local->ble_spin_lock);
	
	if(ble_local->ble_started == ATBM_TRUE){
		atbm_skb_queue_tail(&ble_local->ble_recv_queue,skb);
		ieee80211_ble_thread_wakeup(&ble_local->recv_thread);
	}else {
		atbm_dev_kfree_skb(skb);
	}
	
	atbm_spin_unlock_bh(&ble_local->ble_spin_lock);
}
static int ieee80211_ble_recv_thread(struct ieee80211_ble_thread *thread)
{
	struct atbm_buff *skb;
	struct atbmwifi_common	*hw_priv = thread->hw_priv;
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;
	int  (*ble_cb)(struct atbmwifi_common	*hw_priv, atbm_uint8 * buff, atbm_size_t buff_size, enum ieee80211_ble_msg_type msg_type);

	while(1){
		atbm_os_wait_event_timeout(&thread->wq, 10*HZ);
		atbm_os_mutexLock(&ble_local->ble_mutex_lock, 0);
		ble_cb = ble_local->ble_recv_callback;
		
		while((skb  =  atbm_skb_dequeue(&ble_local->ble_recv_queue))){
			struct ieee80211_ble_status *status = (struct ieee80211_ble_status *)&skb->cb[0];
			wifi_printk(WIFI_DBG_MSG, "%s:ble(%d)(%d)\n",__FUNCTION__,skb->dlen,status->msg_type);
			if(ble_cb) ble_cb(hw_priv,skb->abuf,skb->dlen,status->msg_type);

			atbm_dev_kfree_skb(skb);
		}
		
		atbm_os_mutexUnLock(&ble_local->ble_mutex_lock);
	}

	return 0;
}
static int ieee80211_ble_recv_init(struct atbmwifi_common	*hw_priv)
{
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;
	struct ieee80211_ble_thread *thread = &ble_local->recv_thread;
	
	atbm_skb_queue_head_init(&ble_local->ble_recv_queue);

	thread->flags = 0;
	thread->name  = "ble_recv";
	thread->period_handle = NULL;
	thread->thread_fn = ieee80211_ble_recv_thread;
	thread->hw_priv = hw_priv;
	thread->prio = BLE_RECV_PRIO;

	if(ieee80211_ble_thread_init(thread)){
		wifi_printk(WIFI_DBG_ERROR, "ble_recv thread err\n");
		return -1;
	}
	
	return 0;
}
static int ieee80211_ble_xmit_exit(struct atbmwifi_common	*hw_priv)
{
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;
	struct ieee80211_ble_thread *thread = &ble_local->xmit_thread;
	
	ieee80211_ble_thread_deinit(thread);

	atbm_skb_queue_purge(&ble_local->ble_xmit_queue);
	return  0;
}

static int ieee80211_ble_recv_exit(struct atbmwifi_common	*hw_priv)
{
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;
	struct ieee80211_ble_thread *thread = &ble_local->recv_thread;
	
	ieee80211_ble_thread_deinit(thread);

	atbm_skb_queue_purge(&ble_local->ble_recv_queue);
	return  0;
}


int ieee80211_ble_commb_start(struct atbmwifi_common	*hw_priv)
{
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;

	wifi_printk(WIFI_DBG_INIT, "ble start\n");
	if(ieee80211_ble_recv_init(hw_priv)){
		goto fail_recv;
	}

	if(ieee80211_ble_xmit_init(hw_priv)){
		goto fail_xmit;
	}
	/*
	*start sucess
	*/

	atbm_spin_lock_bh(&ble_local->ble_spin_lock);
	ble_local->ble_started = ATBM_TRUE;
	atbm_spin_unlock_bh(&ble_local->ble_spin_lock);
	return 0;
fail_xmit:
	ieee80211_ble_recv_exit(hw_priv);
fail_recv:
	wifi_printk(WIFI_DBG_INIT,"ble start err\n");
	return -1;

}

int ieee80211_ble_commb_stop(struct atbmwifi_common	*hw_priv)
{
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;

	ble_local->ble_started = ATBM_FALSE;

	ieee80211_ble_xmit_exit(hw_priv);
	ieee80211_ble_recv_exit(hw_priv);
	wifi_printk(WIFI_DBG_INIT, "ble stop\n");
	return 0;
}

int ieee80211_ble_commb_xmit(struct atbmwifi_common* hw_priv, atbm_uint8* xmit, atbm_size_t xmit_len)
{
	//struct ieee80211_local *local = ble_to_local(pble_dev);
	struct ieee80211_ble_buff *ble_buff;
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;
	
	struct atbm_buff *skb;
	ieee80211_ble_dump(__FUNCTION__,xmit,xmit_len);
	ble_buff = atbm_container_of((void *)xmit, struct ieee80211_ble_buff, mem);

	skb = ble_buff->skb;

	ATBM_BUG_ON((skb->abuf + IEEE80211_BLE_SKB_HEADNEED) != (atbm_uint8*)ble_buff);
	
	atbm_skb_reserve(skb, IEEE80211_BLE_SKB_HEADNEED+sizeof(struct ieee80211_ble_buff));
	atbm_skb_put(skb,xmit_len);
	
	atbm_spin_lock_bh(&ble_local->ble_spin_lock);
	
	if(ble_local->ble_started == ATBM_TRUE){
		//printk("[%s]:len [%d]\n",__func__,skb->len);
		atbm_skb_queue_tail(&ble_local->ble_xmit_queue,skb);
		ieee80211_ble_thread_wakeup(&ble_local->xmit_thread);
	}else {
		atbm_dev_kfree_skb(skb);
	}
	
	atbm_spin_unlock_bh(&ble_local->ble_spin_lock);
	return 0;
}

int ieee80211_ble_commb_subscribe(struct atbmwifi_common* hw_priv,
	int (*recv)(struct atbmwifi_common* hw_priv, atbm_uint8* recv, atbm_size_t recv_len, enum ieee80211_ble_msg_type msg_type))
{
	//struct ieee80211_local *local = ble_to_local(pble_dev);
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;

	wifi_printk(WIFI_DBG_INIT,"ble subscribe\n");
	atbm_os_mutexLock(&ble_local->ble_mutex_lock, 0);
	ble_local->ble_recv_callback = recv;
	atbm_os_mutexUnLock(&ble_local->ble_mutex_lock);
	
	return 0;
}
int ieee80211_ble_commb_unsubscribe(struct atbmwifi_common* hw_priv)
{
	//struct ieee80211_local *local = ble_to_local(pble_dev);
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;
	wifi_printk(WIFI_DBG_INIT,"ble unsubscribe\n");
	atbm_os_mutexLock(&ble_local->ble_mutex_lock, 0);
	ble_local->ble_recv_callback = NULL;
	atbm_os_mutexUnLock(&ble_local->ble_mutex_lock);

	synchronize_rcu();
	return 0;
}
char *ieee80211_ble_commb_ble_alloc_xmit(atbm_size_t len)
{
	struct atbm_buff *skb;
	struct  ieee80211_ble_buff *ble_buff;
	
	skb = atbm_dev_alloc_skb(len +  IEEE80211_BLE_SKB_HEADNEED + sizeof(struct  ieee80211_ble_buff));

	if(skb == NULL){
		return  NULL;
	}

	ble_buff = (struct  ieee80211_ble_buff *)(skb->abuf + IEEE80211_BLE_SKB_HEADNEED);
	ble_buff->skb = skb;

	return (char *)ble_buff->mem;
}


int ieee80211_ble_dev_int(struct atbmwifi_common* hw_priv)
{
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;
	
	ble_local->ble_recv_callback = 0;
	atbm_spin_lock_init(&ble_local->ble_spin_lock);
	atbm_os_mutexLockInit(&ble_local->ble_mutex_lock);
	return 0;
	
}
int ieee80211_ble_dev_register(struct atbmwifi_common* hw_priv)
{
	return 0;
}
void ieee80211_ble_dev_deregister(struct atbmwifi_common* hw_priv)
{
	struct ieee80211_ble_local *ble_local = &hw_priv->ble_local;
	
	//platform_device_unregister(&local->ble_dev);
	atbm_os_DeleteMutex(&ble_local->ble_mutex_lock);
}
#endif
