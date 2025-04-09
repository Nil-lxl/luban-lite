/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#ifndef __ATBM_SKBUF_H_
#define __ATBM_SKBUF_H_



#define ATBM_MEM_ALIGNMENT 32
#define ATBM_MEM_ALIGN(addr) ((atbm_void *)(((atbm_uint32)(addr) + ATBM_MEM_ALIGNMENT - 1) & ~(ATBM_MEM_ALIGNMENT - 1)))
#define ATBM_MEM_ALIGN_SIZE(size) (((size) + ATBM_MEM_ALIGNMENT - 1) & ~(ATBM_MEM_ALIGNMENT - 1))

#define ATBM_HWBUF_EXTERN_HEADROM_LEN 	64  
#define ATBM_HWBUF_EXTERN_TAILROM_LEN 	64  


struct atbm_net_device{
	ATBM_NETIF *nif;
	/***more atbm struct**/
	struct atbm_net_device_ops * netdev_ops;
	atbm_uint16 lwip_enable;
	atbm_uint16 lwip_queue_enable;
	atbm_os_wait_queue_head_t tx_enable;
	atbm_os_wait_queue_head_t rx_enable;
	atbm_uint8  drv_priv[ZEROSIZE]; /*struct atbmwifi_vif **/
};


struct atbm_buff {  
	/** next pbuf in singly linked pbuf chain */
  struct atbm_buff *next;  
  struct atbm_buff *prev;
  /** pointer to the memory in the buffer,start buf */
  atbm_uint8 *Head;

  
  /** pointer to the valid data in the buffer,start buf */
  atbm_uint8 *abuf;
  
  /** pointer to the current data tail in the buffer */
  atbm_uint8 *Tail;

  /** data length in the buffer */
  atbm_uint16 dlen;  
  
  /**Head buffer  len*/
  atbm_uint16 bufferLen;  
  
  /** length of atbm_buff ,not include struct atbm_buf,when is_os_buffer =1 reseved*/
  atbm_uint16 totalLen;  

  atbm_uint8  reseved[2];  
  atbm_uint8  if_id;
  
  atbm_uint8 Type;
  /*indicate atbm_buff is not malloc by atbm_wifi,just malloc by tcpip */
  atbm_uint8 ref;
  atbm_uint8 priority;  
  atbm_uint8	cb[64]; // tx /rx  frame descriptors   /rate   tx info
}atbm_packed;

struct atbm_buff_head {
	/* These two members must be first. */
	struct atbm_buff	*next;
	struct atbm_buff	*prev;
	int				qlen;
	atbm_spinlock_t		lock;
};

#define ATBM_OS_SKB_LEN(skb)   ((skb)->dlen)
#define ATBM_OS_SKB_DATA(skb)   (((skb)->abuf))
#define ATBM_OS_SKB_HEAD(skb)   (((skb)->Head))

/**
 *	__skb_queue_head_init - initialize non-spinlock portions of atbm_buff_head
 *	@list: queue to initialize
 *
 *	This initializes only the list and queue length aspects of
 *	an atbm_buff_head object.  This allows to initialize the list
 *	aspects of an atbm_buff_head without reinitializing things like
 *	the spinlock.  It can also be used for on-stack atbm_buff_head
 *	objects where the spinlock is known to not be used.
 */
static __INLINE atbm_void __atbm_skb_queue_head_init(struct atbm_buff_head *list)
{
	list->prev = list->next = (struct atbm_buff *)list;
	list->qlen = 0;
}

/*
 * This function creates a split out lock class for each invocation;
 * this is needed for now since a whole lot of users of the skb-queue
 * infrastructure in drivers have different locking usage (in hardirq)
 * than the networking core (in softirq only). In the long run either the
 * network layer or drivers should need annotation to consolidate the
 * main types of usage into 3 classes.
 */
static __INLINE atbm_void atbm_skb_queue_head_init(struct atbm_buff_head *list)
{
	atbm_spin_lock_init(&list->lock);
	__atbm_skb_queue_head_init(list);
}

/**
 *	atbm_skb_queue_len	- get queue length
 *	@list_: list to measure
 *
 *	Return the length of an &atbm_buff queue.
 */
static __INLINE atbm_uint32 atbm_skb_queue_len(const struct atbm_buff_head *list_)
{
	return list_->qlen;
}
/**
 *	skb_queue_empty - check if a queue is empty
 *	@list: queue head
 *
 *	Returns ATBM_TRUE if the queue is empty, ATBM_FALSE otherwise.
 */
static __INLINE atbm_int32 atbm_skb_queue_empty(const struct atbm_buff_head *list)
{
	return list->next == (struct atbm_buff *)list;
}



struct atbm_buff * atbm_dev_alloc_skb(atbm_int32 len);
atbm_int32 atbm_dev_kfree_skb(struct atbm_buff * skb);
atbm_uint8 *atbm_skb_put(struct atbm_buff *skb, atbm_uint32 len);
atbm_uint8 *atbm_skb_push(struct atbm_buff *skb, atbm_uint32 len);
atbm_uint8 * atbm_skb_pull(struct atbm_buff *skb, atbm_uint32 len);
atbm_uint32 atbm_skb_headroom(const struct atbm_buff *skb);
atbm_int32 atbm_skb_tailroom(const struct atbm_buff *skb);
atbm_int32 atbm_skb_reserve(struct atbm_buff *skb, atbm_int32 len);
atbm_void atbm_skb_set_tail_pointer(struct atbm_buff *skb, const atbm_int32 offset);
atbm_void atbm_skb_trim(struct atbm_buff *skb, atbm_uint32 len);
atbm_void atbm_skb_queue_tail(struct atbm_buff_head *list, struct atbm_buff *newsk);
struct atbm_buff *atbm_skb_dequeue(struct atbm_buff_head *list);
atbm_void atbm_skb_queue_purge(struct atbm_buff_head *list);



/// put into public file   commm

#define atbm_compare_ether_addr(a,b) (!((((atbm_uint8 *)(a))[0] == ((atbm_uint8 *)(b))[0]) && \
								(((atbm_uint8 *)(a))[1] == ((atbm_uint8 *)(b))[1]) && \
								(((atbm_uint8 *)(a))[2] == ((atbm_uint8 *)(b))[2]) && \
								(((atbm_uint8 *)(a))[3] == ((atbm_uint8 *)(b))[3]) && \
								(((atbm_uint8 *)(a))[4] == ((atbm_uint8 *)(b))[4]) && \
								(((atbm_uint8 *)(a))[5] == ((atbm_uint8 *)(b))[5])))

//#define compare_ether_addr(a,b) atbm_memcmp(a,b,6)
#define atbm_is_multicast_ether_addr(addr) ((((atbm_uint8 *)addr)[0])& 0x1)
/**
 * is_broadcast_ether_addr - Determine if the Ethernet address is broadcast
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return ATBM_TRUE if the address is the broadcast address.
 */
static __INLINE atbm_int32 atbm_is_broadcast_ether_addr(const atbm_uint8 *addr)
{
	return (addr[0] & addr[1] & addr[2] & addr[3] & addr[4] & addr[5]) == 0xff;
}
/**
 * is_zero_ether_addr - Determine if the Ethernet address is broadcast
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return ATBM_TRUE if the address is the zero address.
 */
static __INLINE atbm_int32 atbm_is_zero_ether_addr(const atbm_uint8 *addr)
{
	return (addr[0] | addr[1] |  addr[2] |  addr[3] |  addr[4] | addr[5]) == 0;
}

#define atbm_is_valid_ether_addr(_addr)		(!atbm_is_zero_ether_addr(_addr) && !atbm_is_broadcast_ether_addr(_addr))

#define atbm_broadcast_ether_addr (atbm_uint8 *) "\xff\xff\xff\xff\xff\xff"


#define atbm_netif_wake_subqueue(a,b)
#define atbm_netif_stop_subqueue(a,b)
#define synchronize_net()


#define  ATBM_IEEE80211_SKB_DRIV_CB(skb) ((struct atbm_hal_driver_resverd *)((skb)->cb))
#define  ATBM_IEEE80211_SKB_TXCB(skb) ((struct atbmwifi_ieee80211_tx_info *)((skb)->cb))
#define  ATBM_IEEE80211_SKB_RXCB(skb) ((struct atbmwifi_ieee80211_rx_status *)((skb)->cb))
#endif /*__ATBM_SKBUF_H_*/
