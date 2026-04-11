/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#include "atbm_hal.h"

#ifndef LINUX_OS
#define num_present_cpus() 1
#endif

 static  atbm_void __atbmwifi_queue_lock(struct atbmwifi_queue *queue,struct atbmwifi_vif *priv)
{
	atbm_uint8 if_id;
	if (!priv){
		return ;
	}
	if (queue->tx_locked_cnt++ == 0) {
		wifi_printk(WIFI_QUEUE,"[TX] Queue[%d] lock\n",queue->queue_id);
		atbm_for_each_vif(priv->hw_priv, priv, if_id){
			if(!priv && priv->enabled){
				continue;
			}
			tcp_opt->net_stop_queue(priv->ndev,queue->queue_id);
		}
	}
}

static  atbm_void __atbmwifi_queue_unlock(struct atbmwifi_queue *queue,struct atbmwifi_vif *priv )
{
	atbm_uint8 if_id;
	if ((!priv) || (queue->tx_locked_cnt==0)){
		return ;
	}
	//ATBM_BUG_ON(!queue->tx_locked_cnt);
	if (--queue->tx_locked_cnt == 0) {
		wifi_printk(WIFI_QUEUE, "[TX] Queue[%d] unlock\n",queue->queue_id);
		atbm_for_each_vif(priv->hw_priv, priv, if_id){
			if(!priv){
				continue;
			}
			tcp_opt->net_start_queue(priv->ndev,queue->queue_id);
		}
	}
}

 static  atbm_void atbmwifi_queue_parse_id(atbm_uint32 packetID, atbm_uint8 *queue_generation,
						atbm_uint8 *queue_id,
						atbm_uint8 *item_generation,
						atbm_uint8 *item_id,
						atbm_uint8 *if_id,
						atbm_uint8 *link_id)
{
	*item_id		= (packetID >>  0) & 0xFF;
	*item_generation	= (packetID >>  8) & 0xFF;
	*queue_id		= (packetID >> 16) & 0xF;
	*if_id			= (packetID >> 20) & 0xF;
	*link_id		= (packetID >> 24) & 0xF;
	*queue_generation	= (packetID >> 28) & 0xF;
}

 static  atbm_uint32 atbmwifi_queue_make_packet_id(atbm_uint8 queue_generation, atbm_uint8 queue_id,
						atbm_uint8 item_generation, atbm_uint8 item_id,
						atbm_uint8 if_id, atbm_uint8 link_id)
{
	/*TODO:COMBO: Add interfaceID to the packetID */
	return ((atbm_uint32)item_id << 0) |
		((atbm_uint32)item_generation << 8) |
		((atbm_uint32)queue_id << 16) |
		((atbm_uint32)if_id << 20) |
		((atbm_uint32)link_id << 24) |
		((atbm_uint32)queue_generation << 28);
}
 int atbmwifi_queue_stats_init(struct atbmwifi_queue_stats *stats,
			    atbm_uint32 map_capacity,
			    struct atbmwifi_common *hw_priv)
{
	int i;

	atbm_memset(stats, 0, sizeof(*stats));
	stats->map_capacity = map_capacity;
	stats->hw_priv = hw_priv;
	atbm_spin_lock_init(&stats->lock);
	//init_waitqueue_head(&stats->wait_link_id_empty);
	for (i = 0; i < ATBM_WIFI_MAX_VIFS; i++) { 
		stats->link_map_cache[i] = (int *)atbm_kzalloc(sizeof(int[WLAN_LINK_ID_MAX]),GFP_KERNEL);
		if (!stats->link_map_cache[i]) {
			for (; i >= 0; i--)
				atbm_kfree(stats->link_map_cache[i]);
			return -ATBM_ENOMEM;
		}
	}
	return 0;
}
atbm_void atbmwifi_queued_timeout(atbm_void *data1,atbm_void *data2)
{
	struct atbmwifi_queue *queue =(struct atbmwifi_queue *)data1;
	unsigned long flags;
	struct atbmwifi_queue_stats *stats = queue->stats;
	struct atbmwifi_queue_item *item = ATBM_NULL;
	struct atbmwifi_vif *priv=ATBM_NULL;
	struct atbm_buff *skb=ATBM_NULL;
	int if_id;	
	queue->queuedFlag=ATBM_FALSE;
	atbm_spin_lock_irqsave(&queue->lock, &flags);
	while (!atbm_list_empty(&queue->queue)) {
		struct atbmwifi_txpriv *txpriv;
		item = atbm_list_first_entry(
			&queue->queue, struct atbmwifi_queue_item, head);
		if (atbm_TimeAfter(item->queue_timestamp, queue->ttl)){
			wifi_printk(WIFI_QUEUE,"queued_to not remove..%d..%d\n",atbm_GetOsTimeMs(), (int)item->queue_timestamp);
			break;
		}
		txpriv = &item->txpriv;
		skb=item->skb;
		item->skb=ATBM_NULL;
		if_id = txpriv->if_id;
		--queue->num_queued;
		--queue->num_queued_vif[if_id];
		--queue->link_map_cache[if_id][txpriv->link_id];
		atbm_spin_lock(&stats->lock);
		--stats->num_queued[if_id];
		--stats->link_map_cache[if_id][txpriv->link_id];
		atbm_spin_unlock(&stats->lock);
		priv = _atbmwifi_hwpriv_to_vifpriv(stats->hw_priv, if_id);
		if (!priv) {
			wifi_printk(WIFI_QUEUE,"Priv Null....\n");
		}
		atbm_list_move_tail(&item->head, &queue->free_pool);
		atbmwifi_skb_dtor(priv->hw_priv, skb,txpriv);
	}
	if (queue->overfull) {
		//wifi_printk(WIFI_ALWAYS,"atbmwifi_removed_queued_timeout\n");
		if (queue->num_queued <= (queue->capacity/2)) {
			queue->overfull = ATBM_FALSE;
			__atbmwifi_queue_unlock(queue,priv);
		} 
	}else{
		queue->queuedFlag=ATBM_FALSE;
	}
	
	if (!atbm_list_empty(&queue->queue)) {
		atbmwifi_eloop_register_timeout(0,5*HZ,atbmwifi_queued_timeout,(atbm_void *)queue,ATBM_NULL);
		queue->queuedFlag=ATBM_TRUE;
	}
	atbm_spin_unlock_irqrestore(&queue->lock,flags);
}
 int atbmwifi_queue_init(struct atbmwifi_queue *queue,
		      struct atbmwifi_queue_stats *stats,
		      atbm_uint8 queue_id,
		      atbm_size_t capacity)
{
	int i;

	atbm_memset(queue, 0, sizeof(*queue));
	queue->stats = stats;
	queue->capacity = capacity;
	queue->queue_id = queue_id;
	queue->queuedFlag=ATBM_FALSE;
	queue->ttl = 1*HZ;
	ATBM_INIT_LIST_HEAD(&queue->queue);
	ATBM_INIT_LIST_HEAD(&queue->pending);
	ATBM_INIT_LIST_HEAD(&queue->free_pool);
	atbm_spin_lock_init(&queue->lock);
	//atbm_InitTimer(&queue->timeout,atbmwifi_queued_timeout,(atbm_void*)queue);
	queue->pool = (struct atbmwifi_queue_item *)atbm_kzalloc(sizeof(struct atbmwifi_queue_item) * capacity /*pool*/
						+ (sizeof(int[WLAN_LINK_ID_MAX])* ATBM_WIFI_MAX_VIFS)/*link_map_cache*/,GFP_KERNEL);
	
	if (!queue->pool)
		return -ATBM_ENOMEM;

	for (i = 0; i < ATBM_WIFI_MAX_VIFS; i++) {
		queue->link_map_cache[i] =(atbm_void *) (((atbm_uint8 *)queue->pool)
									+sizeof(struct atbmwifi_queue_item) * capacity
									+(sizeof(int[WLAN_LINK_ID_MAX])*i));
	}
	for (i = 0; i < capacity; ++i)
		atbm_list_add_tail(&queue->pool[i].head, &queue->free_pool);

	
	wifi_printk(WIFI_CONNECT,"atbmwifi_queue_init,cap(%d)\n",queue->capacity);
	return 0;
}

/* TODO:COMBO: Flush only a particular interface specific parts */
 int atbmwifi_queue_clear(struct atbmwifi_queue *queue, int if_id)
{
	int i, cnt, iter;
	struct atbmwifi_queue_stats *stats = queue->stats;
	unsigned long flags;
	
	//atbm_LIST_HEAD(gc_list);

	cnt = 0;
	atbm_spin_lock_irqsave(&queue->lock, &flags);
	queue->generation++;
	queue->generation &= 0xf;
	atbm_list_splice_tail_init(&queue->queue, &queue->pending);
	while (!atbm_list_empty(&queue->pending)) {
		struct atbmwifi_queue_item *item = atbm_list_first_entry(
			&queue->pending, struct atbmwifi_queue_item, head);
		ATBM_WARN_ON_FUNC(!item->skb);
		if (ATBM_WIFI_ALL_IFS == if_id || item->txpriv.if_id == if_id) {
			//atbmwifi_queue_register_post_gc(&gc_list, item);
			atbmwifi_skb_dtor(stats->hw_priv, item->skb, &item->txpriv);
			item->skb = ATBM_NULL;
			atbm_list_move_tail(&item->head, &queue->free_pool);
			cnt++;
		}
	}
	queue->num_queued -= cnt;
	queue->num_pending -= cnt;
	if (ATBM_WIFI_ALL_IFS != if_id) {
		queue->num_queued_vif[if_id] = 0;
		queue->num_pending_vif[if_id] = 0;
	} else {
		for (iter = 0; iter < ATBM_WIFI_MAX_VIFS; iter++) {
			queue->num_queued_vif[iter] = 0;
			queue->num_pending_vif[iter] = 0;
		}
	}
	atbm_spin_lock(&stats->lock);
	if (ATBM_WIFI_ALL_IFS != if_id) {
		for (i = 0; i < stats->map_capacity; ++i) {
			stats->num_queued[if_id] -=
				queue->link_map_cache[if_id][i];
			stats->link_map_cache[if_id][i] -=
				queue->link_map_cache[if_id][i];
			queue->link_map_cache[if_id][i] = 0;
		}
	} else {
		for (iter = 0; iter < ATBM_WIFI_MAX_VIFS; iter++) {
			for (i = 0; i < stats->map_capacity; ++i) {
				stats->num_queued[iter] -=
					queue->link_map_cache[iter][i];
				stats->link_map_cache[iter][i] -=
					queue->link_map_cache[iter][i];
				queue->link_map_cache[iter][i] = 0;
			}
		}
	}
	atbm_spin_unlock(&stats->lock);
	if (atbm_unlikely(queue->overfull)) {
		queue->overfull = ATBM_FALSE;
		__atbmwifi_queue_unlock(queue,_atbmwifi_hwpriv_to_vifpriv(stats->hw_priv,if_id));
	}
	atbm_spin_unlock_irqrestore(&queue->lock,flags);
	//wake_up(stats->wait_link_id_empty);
	//atbmwifi_queue_post_gc(stats, &gc_list);

	return 0;
}



 atbm_void atbmwifi_queue_deinit(struct atbmwifi_queue *queue)
{
	int i;
	
	atbmwifi_queue_clear(queue, ATBM_WIFI_ALL_IFS);
	ATBM_INIT_LIST_HEAD(&queue->free_pool);
	atbm_kfree(queue->pool);
	for (i = 0; i < ATBM_WIFI_MAX_VIFS; i++) {
		queue->link_map_cache[i] = ATBM_NULL;
	}
	queue->pool = ATBM_NULL;
	queue->capacity = 0;
}

 atbm_size_t atbmwifi_queue_get_num_queued(struct atbmwifi_vif *priv,
				   struct atbmwifi_queue *queue,
				   atbm_uint32 link_id_map)
{
	atbm_size_t ret;
	int i, bit;
	atbm_size_t map_capacity = queue->stats->map_capacity;
	unsigned long flags;

	if (!link_id_map)
		return 0;

	atbm_spin_lock_irqsave(&queue->lock, &flags);
	if (atbm_likely(link_id_map == (atbm_uint32) -1)) {
		ret = queue->num_queued_vif[priv->if_id] -
			queue->num_pending_vif[priv->if_id];

	} else {
		ret = 0;
		for (i = 0, bit = 1; i < map_capacity; ++i, bit <<= 1) {
			if (link_id_map & bit)
				ret +=
				queue->link_map_cache[priv->if_id][i];
		}
	}
	atbm_spin_unlock_irqrestore(&queue->lock,flags);
	return ret;
}

 int atbmwifi_queue_put(struct atbmwifi_queue *queue,
		     struct atbm_buff *skb,
		     struct atbmwifi_txpriv *txpriv)
{
	int ret = 0;
	//atbm_LIST_HEAD(gc_list);
	struct atbmwifi_queue_stats *stats = queue->stats;
	unsigned long flags;
	/* TODO:COMBO: Add interface ID info to queue item */

	if (txpriv->link_id >= queue->stats->map_capacity)
		return -ATBM_EINVAL;

	atbm_spin_lock_irqsave(&queue->lock, &flags);
	if (!(atbm_list_empty(&queue->free_pool))) {
		struct atbmwifi_queue_item *item = atbm_list_first_entry(
			&queue->free_pool, struct atbmwifi_queue_item, head);
		ATBM_BUG_ON(item->skb);
		atbm_list_move_tail(&item->head, &queue->queue);
		item->skb = skb;
		item->txpriv = *txpriv;
		item->generation = 0;
		item->packetID = atbmwifi_queue_make_packet_id(
			queue->generation, queue->queue_id,
			item->generation, item - queue->pool,
			txpriv->if_id, txpriv->raw_link_id);
		item->queue_timestamp = atbm_GetOsTimeMs();

		++queue->num_queued;
		++queue->num_queued_vif[txpriv->if_id];
		++queue->link_map_cache[txpriv->if_id][txpriv->link_id];

		atbm_spin_lock(&stats->lock);
		++stats->num_queued[txpriv->if_id];
		++stats->link_map_cache[txpriv->if_id][txpriv->link_id];
		atbm_spin_unlock(&stats->lock);

		/*
		 * TX may happen in parallel sometimes.
		 * Leave extra queue slots so we don't overflow.
		 */
		if ((queue->overfull == ATBM_FALSE) &&
				(queue->num_queued >=(stats->hw_priv->vif0_throttle)-num_present_cpus())) {
			queue->overfull = ATBM_TRUE;
			__atbmwifi_queue_lock(queue,_atbmwifi_hwpriv_to_vifpriv(stats->hw_priv,txpriv->if_id));
			//wifi_printk(WIFI_CONNECT,"queue->overfull %d,num_queued %d------%d>>>\n",queue->overfull,queue->num_queued,queue->queuedFlag);
		}
		if(queue->queuedFlag==ATBM_FALSE){
			atbmwifi_eloop_register_timeout(0,5*HZ,atbmwifi_queued_timeout,(atbm_void *)queue,ATBM_NULL);
			queue->queuedFlag = ATBM_TRUE;
		}
	} else {
		ret = -ATBM_ENOENT;
	}
	atbm_spin_unlock_irqrestore(&queue->lock,flags);
	return ret;
}
/*
get frame from the queue->queue, then move it to queue->pending
*/
 int atbmwifi_queue_get(struct atbmwifi_queue *queue,
			int if_id,
		     atbm_uint32 link_id_map,
		     struct wsm_tx **tx,
		     struct atbmwifi_txpriv **txpriv)
{
	int ret = -ATBM_ENOENT;
	struct atbmwifi_queue_item *item;
	struct atbmwifi_queue_stats *stats = queue->stats;
	ATBM_BOOL wakeup_stats = ATBM_FALSE;
	unsigned long flags;

	atbm_spin_lock_irqsave(&queue->lock, &flags);
	for(item = atbm_list_entry((&queue->queue)->next, struct atbmwifi_queue_item, head);
			&(item)->head != (&queue->queue); 	
	     item = atbm_list_entry((item)->head.next, struct atbmwifi_queue_item, head)){
		if ((item->txpriv.if_id == if_id) &&
			(link_id_map & BIT(item->txpriv.link_id))) {
			ret = 0;
			break;
		}else{
			ret = -1;
		}
	}
	if (!/*ATBM_WARN_ON*/(ret)) {
		*tx = (struct wsm_tx *)ATBM_OS_SKB_DATA(item->skb);
		*txpriv = &item->txpriv;
		(*tx)->packetID = atbm_cpu_to_le32(item->packetID);
		atbm_list_move_tail(&item->head, &queue->pending);
		++queue->num_pending;
		++queue->num_pending_vif[item->txpriv.if_id];
		--queue->link_map_cache[item->txpriv.if_id]
				[item->txpriv.link_id];
		//item->xmit_timestamp = atbm_GetOsTimeMs;

		atbm_spin_lock(&stats->lock);
		--stats->num_queued[item->txpriv.if_id];
		if (!--stats->link_map_cache[item->txpriv.if_id]
					[item->txpriv.link_id])
			wakeup_stats = ATBM_TRUE;

		atbm_spin_unlock(&stats->lock);
	}
	atbm_spin_unlock_irqrestore(&queue->lock,flags);
	//if (wakeup_stats)
	//	wake_up(stats->wait_link_id_empty);
	return ret;
}


 int atbmwifi_queue_requeue(struct atbmwifi_queue *queue, atbm_uint32 packetID)
{
	int ret = 0;
	atbm_uint8 queue_generation, queue_id, item_generation, item_id, if_id, link_id;
	struct atbmwifi_queue_item *item;
	struct atbmwifi_queue_stats *stats = queue->stats;
	unsigned long flags;
	atbm_spin_lock_irqsave(&queue->lock, &flags);

	atbmwifi_queue_parse_id(packetID, &queue_generation, &queue_id,
				&item_generation, &item_id, &if_id, &link_id);

	item = &queue->pool[item_id];


	/*if_id = item->txpriv.if_id;*/

	ATBM_BUG_ON(queue_id != queue->queue_id);
	if (atbm_unlikely(queue_generation != queue->generation)) {
		ret = -ATBM_ENOENT;
	} else if (atbm_unlikely(item_id >= (unsigned) queue->capacity)) {
		ATBM_WARN_ON_FUNC(1);
		ret = -ATBM_EINVAL;
	} else if (atbm_unlikely(item->generation != item_generation)) {
		ATBM_WARN_ON_FUNC(1);
		ret = -ATBM_ENOENT;
	} else {
		--queue->num_pending;
		--queue->num_pending_vif[if_id];
		++queue->link_map_cache[if_id][item->txpriv.link_id];

		atbm_spin_lock(&stats->lock);
		++stats->num_queued[item->txpriv.if_id];
		++stats->link_map_cache[if_id][item->txpriv.link_id];
		atbm_spin_unlock(&stats->lock);

		item->generation = ++item_generation;
		item->packetID = atbmwifi_queue_make_packet_id(
			queue_generation, queue_id, item_generation, item_id,
			if_id, link_id);
		atbm_list_move(&item->head, &queue->queue);
	}
	atbm_spin_unlock_irqrestore(&queue->lock,flags);
	return ret;
}

 int atbmwifi_queue_requeue_all(struct atbmwifi_queue *queue)
{
	struct atbmwifi_queue_stats *stats = queue->stats;
	unsigned long flags;
	
	atbm_spin_lock_irqsave(&queue->lock, &flags);
	while (!atbm_list_empty(&queue->pending)) {
		struct atbmwifi_queue_item *item = atbm_list_entry(
			queue->pending.prev, struct atbmwifi_queue_item, head);

		--queue->num_pending;
		--queue->num_pending_vif[item->txpriv.if_id];
		++queue->link_map_cache[item->txpriv.if_id]
				[item->txpriv.link_id];

		atbm_spin_lock(&stats->lock);
		++stats->num_queued[item->txpriv.if_id];
		++stats->link_map_cache[item->txpriv.if_id]
				[item->txpriv.link_id];
		atbm_spin_unlock(&stats->lock);

		++item->generation;
		item->packetID = atbmwifi_queue_make_packet_id(
			queue->generation, queue->queue_id,
			item->generation, item - queue->pool,
			item->txpriv.if_id, item->txpriv.raw_link_id);
		atbm_list_move(&item->head, &queue->queue);
	}
	atbm_spin_unlock_irqrestore(&queue->lock,flags);

	return 0;
}

 int atbmwifi_queue_remove(struct atbmwifi_queue *queue, atbm_uint32 packetID)
{
	int ret = 0;
	atbm_uint8 queue_generation, queue_id, item_generation, item_id, if_id, link_id;
	struct atbmwifi_queue_item *item;
	struct atbmwifi_queue_stats *stats = queue->stats;
	struct atbm_buff *gc_skb = ATBM_NULL;
	struct atbmwifi_txpriv gc_txpriv;
	unsigned long flags;
	atbm_spin_lock_irqsave(&queue->lock, &flags);

	atbmwifi_queue_parse_id(packetID, &queue_generation, &queue_id,
				&item_generation, &item_id, &if_id, &link_id);
	

	item = &queue->pool[item_id];
	if(item==ATBM_NULL){
		atbm_spin_unlock_irqrestore(&queue->lock,flags);
		return 0;
	}
	ATBM_BUG_ON(queue_id != queue->queue_id);
	/*TODO:COMBO:Add check for interface ID also */
	if (atbm_unlikely(queue_generation != queue->generation)) {
		ATBM_WARN_ON_FUNC(1);
		ret = -ATBM_ENOENT;
	} else if (atbm_unlikely(item_id >= (unsigned) queue->capacity)) {
		ATBM_WARN_ON_FUNC(1);
		ret = -ATBM_EINVAL;
	} else if (atbm_unlikely(item->generation != item_generation)) {
		ATBM_WARN_ON_FUNC(1);
		ret = -ATBM_ENOENT;
	} else {
		gc_txpriv = item->txpriv;
		gc_skb = item->skb;
		item->skb = ATBM_NULL;
		--queue->num_pending;
		--queue->num_pending_vif[if_id];
		--queue->num_queued;
		--queue->num_queued_vif[if_id];
		//++queue->num_sent;
		++item->generation;
		/* Do not use atbm_list_move_tail here, but atbm_list_move:
		 * try to utilize cache row.
		 */
		atbm_list_move(&item->head, &queue->free_pool);
		//wifi_printk(WIFI_ALWAYS,"[TX] queue_rmove %d %d\n",queue->num_queued,queue->overfull);

		if (atbm_unlikely(queue->overfull) &&
		    (queue->num_queued <= (stats->hw_priv->vif0_throttle / 2))) {
			queue->overfull = ATBM_FALSE;
			__atbmwifi_queue_unlock(queue,_atbmwifi_hwpriv_to_vifpriv(stats->hw_priv,if_id));
		}
	}

	atbm_spin_unlock_irqrestore(&queue->lock,flags);

	if (gc_skb){
	//	wifi_printk(WIFI_ALWAYS,"atbmwifi_queue_remove \n");
		atbmwifi_skb_dtor(stats->hw_priv, gc_skb, &gc_txpriv);
	}

	return ret;
}

 int atbmwifi_queue_get_skb(struct atbmwifi_queue *queue, atbm_uint32 packetID,
			 struct atbm_buff **skb,
			 const struct atbmwifi_txpriv **txpriv)
{
	int ret = 0;
	atbm_uint8 queue_generation, queue_id, item_generation, item_id, if_id, link_id;
	struct atbmwifi_queue_item *item;
	unsigned long flags;
	
	atbm_spin_lock_irqsave(&queue->lock, &flags);

	atbmwifi_queue_parse_id(packetID, &queue_generation, &queue_id,
				&item_generation, &item_id, &if_id, &link_id);

	item = &queue->pool[item_id];

	ATBM_BUG_ON(queue_id != queue->queue_id);
	/* TODO:COMBO: Add check for interface ID here */
	if (atbm_unlikely(queue_generation != queue->generation)) {
		ret = -ATBM_ENOENT;
	} else if (atbm_unlikely(item_id >= (unsigned) queue->capacity)) {
		ATBM_WARN_ON_FUNC(1);
		ret = -ATBM_EINVAL;
	} else if (atbm_unlikely(item->generation != item_generation)) {
		ATBM_WARN_ON_FUNC(1);
		ret = -ATBM_ENOENT;
	} else {
		*skb = item->skb;
		*txpriv = &item->txpriv;
	}
	atbm_spin_unlock_irqrestore(&queue->lock,flags);
	return ret;
}

 atbm_void atbmwifi_queue_lock(struct atbmwifi_queue *queue,struct atbmwifi_vif *priv)
{
	unsigned long flags;
	atbm_spin_lock_irqsave(&queue->lock, &flags);
	__atbmwifi_queue_lock(queue,priv);
	atbm_spin_unlock_irqrestore(&queue->lock,flags);
}

 atbm_void atbmwifi_queue_unlock(struct atbmwifi_queue *queue,struct atbmwifi_vif *priv)
{
	unsigned long flags;
	atbm_spin_lock_irqsave(&queue->lock, &flags);
	__atbmwifi_queue_unlock(queue,priv);
	atbm_spin_unlock_irqrestore(&queue->lock,flags);
}



 ATBM_BOOL atbmwifi_queue_stats_is_empty(struct atbmwifi_queue_stats *stats,
				 atbm_uint32 link_id_map, int if_id)
{
	ATBM_BOOL empty = ATBM_TRUE;

	atbm_spin_lock(&stats->lock);
	if (link_id_map == (atbm_uint32)-1)
		empty = stats->num_queued[if_id] == 0;
	else {
		int i, if_id;
		for (if_id = 0; if_id < ATBM_WIFI_MAX_VIFS; if_id++) {
			for (i = 0; i < stats->map_capacity; ++i) {
				if (link_id_map & BIT(i)) {
					if (stats->link_map_cache[if_id][i]) {
						empty = ATBM_FALSE;
						break;
					}
				}
			}
		}
	}
	atbm_spin_unlock(&stats->lock);

	return empty;
}


