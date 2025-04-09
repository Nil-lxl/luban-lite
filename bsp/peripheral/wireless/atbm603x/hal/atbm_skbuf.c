/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#include "atbm_hal.h"
int atbm_skb_cnt =0;
int atbm_skb_free =0;

static struct atbm_buff * __atbm_pbuf_alloc(atbm_uint16 length)
{
	struct atbm_buff *p;
	atbm_uint32 size;
	atbm_skb_cnt++;
	size=ATBM_MEM_ALIGN_SIZE(sizeof(struct atbm_buff)  + ATBM_HWBUF_EXTERN_HEADROM_LEN + ATBM_HWBUF_EXTERN_TAILROM_LEN+ length);
	/* If atbm_buff is to be allocated in RAM, allocate memory for it. */
	p = (struct atbm_buff*) atbm_kzalloc(size,GFP_KERNEL);
	if (p == ATBM_NULL) {
		ATBM_WARN("__atbm_pbuf_alloc fail \n");
		return ATBM_NULL;
	}
	/* Set up internal structure of the atbm_buff. */
	ATBM_OS_SKB_DATA(p) = ATBM_MEM_ALIGN((atbm_void *)((atbm_uint8 *)p + sizeof(struct atbm_buff) + ATBM_HWBUF_EXTERN_HEADROM_LEN));
	ATBM_OS_SKB_HEAD(p) = ATBM_MEM_ALIGN((atbm_uint8 *)(p+1));
	//p->dlen =length+ HWBUF_EXTERN_TAILROM_LEN;

	p->ref = 1;
	p->totalLen = ATBM_MEM_ALIGN_SIZE(ATBM_HWBUF_EXTERN_HEADROM_LEN + ATBM_HWBUF_EXTERN_TAILROM_LEN + length);
	p->bufferLen = ATBM_MEM_ALIGN_SIZE(ATBM_HWBUF_EXTERN_HEADROM_LEN + ATBM_HWBUF_EXTERN_TAILROM_LEN +length);
	//wifi_printk(WIFI_ALWAYS,"****************size*****************=%d:%d:%d\n",size,p->totalLen,p->bufferLen);

	/*add for wifi*/
	ATBM_OS_SKB_LEN(p) = 0;
	p->Tail = ATBM_OS_SKB_DATA(p);
  
  return p;
}

atbm_void  atbm_skb_reinit(struct atbm_buff *skb)
{	
	//clear skb struct to 0
	
	ATBM_OS_SKB_DATA(skb) = ATBM_MEM_ALIGN((atbm_void  *)((atbm_uint8 *)skb + sizeof(struct atbm_buff) + ATBM_HWBUF_EXTERN_HEADROM_LEN));
	ATBM_OS_SKB_HEAD(skb) = (atbm_uint8 *)(skb+1);
	skb->bufferLen = skb->totalLen;

	//skb->ref = 1;
	//skb->totalLen = (ATBM_MEM_ALIGN_SIZE(sizeof(struct atbm_buff)  + HWBUF_EXTERN_HEADROM_LEN + HWBUF_EXTERN_TAILROM_LEN) + ATBM_MEM_ALIGN_SIZE(length));
	

	/*add for wifi*/
	ATBM_OS_SKB_LEN(skb) = 0;
	skb->Tail = ATBM_OS_SKB_DATA(skb);
}
static atbm_void __atbm_pbuf_free(struct atbm_buff *p)
{
	atbm_skb_free++;
	ATBM_ASSERT(p->ref == 1);
	p->ref--;
	atbm_kfree(p);
}

struct atbm_buff * atbm_dev_alloc_skb(atbm_int32 len) 
{
	struct atbm_buff * skb;
	skb =  __atbm_pbuf_alloc(len);
	if(skb == ATBM_NULL){
		ATBM_BUG_ON(1);
		return  ATBM_NULL;
	}
	return skb;
}

struct atbm_buff * atbm_dev_alloc_skbhdr(void) 
{
	struct atbm_buff *p;

	/* If atbm_buff is to be allocated in RAM, allocate memory for it. */
	p = (struct atbm_buff*) __atbm_pbuf_alloc(0);
	if (p == ATBM_NULL) {
		ATBM_WARN("atbm_dev_alloc_skbhdr fail \n");
		return ATBM_NULL;
	}
  
    return p;
}


atbm_int32 atbm_dev_kfree_skb(struct atbm_buff * skb)
{
#if defined (WLAN_ZERO_COPY1)
	atbm_dev_free_os_skb(skb);
#endif //#if defined (WLAN_ZERO_COPY1)
	__atbm_pbuf_free(skb);
	return 1;
}


/*
 *	Add data to an atbm_buff
 */
atbm_uint8 *atbm_skb_put(struct atbm_buff *skb, atbm_uint32 len)
{
	atbm_uint8 *tmp = skb->Tail;
	skb->Tail += len;
	ATBM_OS_SKB_LEN(skb)  += len;
	return tmp;
}

atbm_uint8 *atbm_skb_push(struct atbm_buff *skb, atbm_uint32 len)
{
 	ATBM_OS_SKB_DATA(skb) -= len;
 	ATBM_OS_SKB_LEN(skb)  += len;
	return ATBM_OS_SKB_DATA(skb);
}

atbm_uint8 * atbm_skb_pull(struct atbm_buff *skb, atbm_uint32 len)
{
	ATBM_OS_SKB_LEN(skb) -= len;
	return ATBM_OS_SKB_DATA(skb) += len;
}



/**
 *	atbm_skb_headroom - bytes at buffer head
 *	@skb: buffer to check
 *
 *	Return the number of bytes of free space at the head of an &atbm_buff.
 */
atbm_uint32 atbm_skb_headroom(const struct atbm_buff *skb)
{
	return ATBM_OS_SKB_DATA(skb) - (atbm_uint8 *)ATBM_OS_SKB_HEAD(skb);
}

/**
 *	skb_tailroom - bytes at buffer end
 *	@skb: buffer to check
 *
 *	Return the number of bytes of free space at the tail of an atbm_buff
 */
atbm_int32 atbm_skb_tailroom(const struct atbm_buff *skb)
{
	return ((atbm_uint8 *)ATBM_OS_SKB_HEAD(skb)+ skb->bufferLen) - (atbm_uint8 *)skb->Tail;
}

/**
 *	skb_reserve - adjust headroom
 *	@skb: buffer to alter
 *	@len: bytes to move
 *
 *	Increase the headroom of an empty &atbm_buff by reducing the tail
 *	room. This is only allowed for an empty buffer.
 */
atbm_int32 atbm_skb_reserve(struct atbm_buff *skb, atbm_int32 len)
{
	//atbm_uint8 * buf = OS_SKB_DATA(skb);
	//ATBM_WARN_ON_FUNC(OS_SKB_LEN(skb) > 0);
	//if(skb->dlen < len){
	//	return -1;
	//}
	ATBM_OS_SKB_DATA(skb) += len;
	skb->Tail = ATBM_OS_SKB_DATA(skb);
	//skb->tot_len -= len;
	//skb->dlen -= len;	
	return 0;
}


atbm_void atbm_skb_set_tail_pointer(struct atbm_buff *skb, const atbm_int32 offset)
{
	skb->Tail = ATBM_OS_SKB_DATA(skb) + offset;
}

atbm_void atbm_skb_trim(struct atbm_buff *skb, atbm_uint32 len)
{

	ATBM_OS_SKB_LEN(skb) = len;
	atbm_skb_set_tail_pointer(skb, len);
}

static atbm_void __atbm_skb_insert(struct atbm_buff *newsk,
				struct atbm_buff *prev, struct atbm_buff *next,
				struct atbm_buff_head *list)
{
	newsk->next = next;
	newsk->prev = prev;
	next->prev  = prev->next = newsk;
	list->qlen++;
}

/**
 *	__skb_queue_after_ - queue a buffer at the list head
 *	@list: list to use
 *	@prev: place after this buffer
 *	@newsk: buffer to queue
 *
 *	Queue a buffer int the middle of a list. This function takes no locks
 *	and you must therefore hold required locks before calling it.
 *
 *	A buffer cannot be placed on two lists at the same time.
 */
 /*
static atbm_void __skb_queue_after_(struct atbm_buff_head *list,
				     struct atbm_buff *prev,
				     struct atbm_buff *newsk)
{
	__atbm_skb_insert(newsk, prev, prev->next, list);
}
*/
 static  atbm_void __skb_queue_before_(struct atbm_buff_head *list,
				      struct atbm_buff *next,
				      struct atbm_buff *newsk)
{
	__atbm_skb_insert(newsk, next->prev, next, list);
}
/*
static  atbm_void __skb_queue_head_(struct atbm_buff_head *list,
				    struct atbm_buff *newsk)
{
	__skb_queue_after_(list, (struct atbm_buff *)list, newsk);
}
*/
/**
 *	__skb_queue_tail_ - queue a buffer at the list tail
 *	@list: list to use
 *	@newsk: buffer to queue
 *
 *	Queue a buffer at the end of a list. This function takes no locks
 *	and you must therefore hold required locks before calling it.
 *
 *	A buffer cannot be placed on two lists at the same time.
 */
static  atbm_void __skb_queue_tail_(struct atbm_buff_head *list,
				   struct atbm_buff *newsk)
{
	__skb_queue_before_(list, (struct atbm_buff *)list, newsk);
}
static  atbm_void __skb_unlink_(struct atbm_buff *skb, struct atbm_buff_head *list)
{
	struct atbm_buff *next, *prev;

	list->qlen--;
	next	   = skb->next;
	prev	   = skb->prev;
	skb->next  = skb->prev = ATBM_NULL;
	next->prev = prev;
	prev->next = next;
}

/**
 *	atbm_skb_peek_tail - peek at the tail of an &atbm_buff_head
 *	@list_: list to peek at
 *
 *	Peek an &atbm_buff. Unlike most other operations you _MUST_
 *	be careful with this one. A peek leaves the buffer on the
 *	list and someone else may run off with it. You must hold
 *	the appropriate locks or have a private queue to do this.
 *
 *	Returns %NULL for an empty list or a pointer to the tail element.
 *	The reference count is not incremented and the reference is therefore
 *	volatile. Use with caution.
 */
 /*
static  struct atbm_buff *atbm_skb_peek_tail(struct atbm_buff_head *list_)
{
	struct atbm_buff *list = ((struct atbm_buff *)list_)->prev;
	if (list == (struct atbm_buff *)list_)
		list = ATBM_NULL;
	return list;
}
*/
/**
 *	atbm_skb_peek - peek at the head of an &atbm_buff_head
 *	@list_: list to peek at
 *
 *	Peek an &atbm_buff. Unlike most other operations you _MUST_
 *	be careful with this one. A peek leaves the buffer on the
 *	list and someone else may run off with it. You must hold
 *	the appropriate locks or have a private queue to do this.
 *
 *	Returns %NULL for an empty list or a pointer to the head element.
 *	The reference count is not incremented and the reference is therefore
 *	volatile. Use with caution.
 */
struct atbm_buff *atbm_skb_peek(struct atbm_buff_head *list_)
{
	struct atbm_buff *list = ((struct atbm_buff *)list_)->next;
	if (list == (struct atbm_buff *)list_)
		list = ATBM_NULL;
	return list;
}
/**
 *	__skb_dequeue_tail_ - remove from the tail of the queue
 *	@list: list to dequeue from
 *
 *	Remove the tail of the list. This function does not take any locks
 *	so must be used with appropriate locks held only. The tail item is
 *	returned or %NULL if the list is empty.
 */
 /*
static  struct atbm_buff *__skb_dequeue_tail_(struct atbm_buff_head *list)
{
	struct atbm_buff *skb = atbm_skb_peek_tail(list);
	if (skb)
		__skb_unlink_(skb, list);
	return skb;
}
*/
/**
 *	__skb_dequeue_ - remove from the head of the queue
 *	@list: list to dequeue from
 *
 *	Remove the head of the list. This function does not take any locks
 *	so must be used with appropriate locks held only. The head item is
 *	returned or %NULL if the list is empty.
 */
static  struct atbm_buff *__skb_dequeue_(struct atbm_buff_head *list)
{
	struct atbm_buff *skb = atbm_skb_peek(list);
	if (skb)
		__skb_unlink_(skb, list);
	return skb;
}

/**
 *	skb_dequeue - remove from the head of the queue
 *	@list: list to dequeue from
 *
 *	Remove the head of the list. The list lock is taken so the function
 *	may be used safely with other locking list functions. The head item is
 *	returned or %NULL if the list is empty.
 */
struct atbm_buff *atbm_skb_dequeue(struct atbm_buff_head *list)
{
	unsigned long flags;
	struct atbm_buff *result;

	atbm_spin_lock_irqsave(&list->lock, &flags);
	wifi_printk(WIFI_DBG_MSG,"atbm: atbm_skb_dequeue(), start.\n");
	result = __skb_dequeue_(list);
	wifi_printk(WIFI_DBG_MSG,"atbm: atbm_skb_dequeue(), end.\n");
	atbm_spin_unlock_irqrestore(&list->lock, flags);
	return result;
}

atbm_void atbm_skb_queue_tail(struct atbm_buff_head *list, struct atbm_buff *newsk)
{
	
	unsigned long flags;

	atbm_spin_lock_irqsave(&list->lock, &flags);
	wifi_printk(WIFI_DBG_MSG,"atbm: atbm_skb_queue_tail(), start.\n");
	__skb_queue_tail_(list, newsk);
	wifi_printk(WIFI_DBG_MSG,"atbm: atbm_skb_queue_tail(), end.\n");
	atbm_spin_unlock_irqrestore(&list->lock, flags);
}
/**
 *	skb_queue_purge - empty a list
 *	@list: list to empty
 *
 *	Delete all buffers on an &atbm_buff list. Each buffer is removed from
 *	the list and one reference dropped. This function takes the list
 *	lock and is atomic with respect to other list locking functions.
 */
 atbm_void atbm_skb_queue_purge(struct atbm_buff_head *list)
{
	struct atbm_buff *skb;
	while ((skb = atbm_skb_dequeue(list)) != ATBM_NULL)
		atbm_dev_kfree_skb(skb);
}

