/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#ifndef ATBMWIFI__QUEUE_H_INCLUDED
#define ATBMWIFI__QUEUE_H_INCLUDED

#include "atbm_hal.h"

struct atbmwifi_txpriv {
	atbm_uint8 link_id;
	atbm_uint8 raw_link_id;
	atbm_uint8 tid;
	atbm_uint8 rate_id;
	atbm_uint8 offset;
	atbm_uint8 if_id;
	atbm_uint8 reserve[2];
};
struct atbmwifi_queue_stats {
	atbm_spinlock_t		lock;
	int			*link_map_cache[2];
	atbm_uint8			num_queued[2];
	atbm_uint8			map_capacity;
	atbm_uint8			reserved;
	struct atbmwifi_common *hw_priv;
};
struct atbmwifi_queue_item
{
	struct atbm_list_head	head;
	struct atbm_buff		*skb;
	atbm_uint32			packetID;
	struct atbmwifi_txpriv	txpriv;
	atbm_uint8			generation;
	unsigned long queue_timestamp;
};

struct atbmwifi_queue {
	struct atbmwifi_queue_stats *stats;
	atbm_uint8				capacity;
	atbm_uint8				num_queued;
	atbm_uint8				num_queued_vif[2];

	atbm_uint8				num_pending;
	atbm_uint8				num_pending_vif[2];
	atbm_uint8				tx_locked_cnt;

	ATBM_BOOL			overfull;
	atbm_uint8			queue_id;
	atbm_uint8			generation;
	atbm_uint8			reserved;
	atbm_spinlock_t		lock;
	struct atbmwifi_queue_item *pool;
	struct atbm_list_head	queue;
	struct atbm_list_head	free_pool;
	struct atbm_list_head	pending;
	OS_TIMER  timeout;
	ATBM_BOOL 			queuedFlag;
	atbm_uint32 				ttl;
	int			*link_map_cache[2];
};

typedef atbm_void (*atbmwifi_queue_skb_dtor_t)(struct atbmwifi_common *priv,
	struct atbm_buff *skb,
	const struct atbmwifi_txpriv *txpriv);

 int atbmwifi_queue_stats_init(struct atbmwifi_queue_stats *stats,
			    atbm_uint32 map_capacity,
			    struct atbmwifi_common *hw_priv);
int atbmwifi_queue_init(struct atbmwifi_queue *queue,
		      struct atbmwifi_queue_stats *stats,
		      atbm_uint8 queue_id,
		      atbm_size_t capacity);
int atbmwifi_queue_clear(struct atbmwifi_queue *queue, int if_id);
atbm_void atbmwifi_queue_deinit(struct atbmwifi_queue *queue);

atbm_size_t atbmwifi_queue_get_num_queued(struct atbmwifi_vif *priv,
				   struct atbmwifi_queue *queue,
				   atbm_uint32 link_id_map);
int atbmwifi_queue_put(struct atbmwifi_queue *queue,
		     struct atbm_buff *skb,
		     struct atbmwifi_txpriv *txpriv);
int atbmwifi_queue_get(struct atbmwifi_queue *queue,
			int if_id,
		     atbm_uint32 link_id_map,
		     struct wsm_tx **tx,
		     struct atbmwifi_txpriv **txpriv);

int atbmwifi_queue_requeue(struct atbmwifi_queue *queue, atbm_uint32 packetID);
int atbmwifi_queue_requeue_all(struct atbmwifi_queue *queue);

int atbmwifi_queue_remove(struct atbmwifi_queue *queue,
			atbm_uint32 packetID);
int atbmwifi_queue_get_skb(struct atbmwifi_queue *queue, atbm_uint32 packetID,
			 struct atbm_buff **skb,
			 const struct atbmwifi_txpriv **txpriv);
atbm_void atbmwifi_queue_lock(struct atbmwifi_queue *queue,struct atbmwifi_vif *priv);
atbm_void atbmwifi_queue_unlock(struct atbmwifi_queue *queue,struct atbmwifi_vif *priv);
ATBM_BOOL atbmwifi_queue_get_xmit_timestamp(struct atbmwifi_queue *queue,
				     unsigned long *timestamp, int if_id,
				     atbm_uint32 pending_frameID);


ATBM_BOOL atbmwifi_queue_stats_is_empty(struct atbmwifi_queue_stats *stats,
				 atbm_uint32 link_id_map, int if_id);

static __INLINE atbm_uint8 atbmwifi_queue_get_queue_id(atbm_uint32 packetID)
{
	return (packetID >> 16) & 0xF;
}

static __INLINE atbm_uint8 atbmwifi_queue_get_if_id(atbm_uint32 packetID)
{
	return (packetID >> 20) & 0xF;
}

static __INLINE atbm_uint8 atbmwifi_queue_get_link_id(atbm_uint32 packetID)
{
	return (packetID >> 24) & 0xF;
}

static __INLINE atbm_uint8 atbmwifi_queue_get_generation(atbm_uint32 packetID)
{
	return (packetID >>  8) & 0xFF;
}

#endif /* ATBMWIFI__QUEUE_H_INCLUDED */
