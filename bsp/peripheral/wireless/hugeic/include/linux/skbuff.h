#ifndef _LINUX_SKB_H_
#define _LINUX_SKB_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <byteorder/little_endian.h>
#include <linux/atomic.h>
#include <linux/defs.h>

#define SKB_TRACE

#if BITS_PER_LONG > 32
#define NET_SKBUFF_DATA_USES_OFFSET 1
#endif

#ifdef NET_SKBUFF_DATA_USES_OFFSET
typedef unsigned int sk_buff_data_t;
#else
typedef unsigned char *sk_buff_data_t;
#endif

#define SKB_DATA_ALIGN(s)   ALIGN(s, 32)

/* Don't change this without changing skb_csum_unnecessary! */
#define CHECKSUM_NONE       0
#define CHECKSUM_UNNECESSARY    1
#define CHECKSUM_COMPLETE   2
#define CHECKSUM_PARTIAL    3

#define PACKET_OTHERHOST    3       /* To someone else  */


struct sk_buff {
    struct sk_buff      *next;
    struct sk_buff      *prev;
    struct sk_buff      *clone;
    struct list_head	list;
    char  cb[48] __attribute__((aligned(8)));
    struct net_device   *dev;
    unsigned int        len, data_len;
    __u16               priority;
    __be16              protocol;
    __u16               queue_mapping;

    unsigned char      *transport_header;
    unsigned char      *network_header;
    unsigned char      *tail;
    unsigned char      *end;
    unsigned char      *head, *data;

    /* skb flags */
    __u16               pkt_type: 3,
                        ip_summed: 2,
                        cloned: 1,
                        acked : 1,
                        lmaced:1;
    atomic_t            users;
    void               *txinfo;
    void               *sta;
    u8                  aid;
    ////////////////////////////////////////////////
    u32 lifetime;
    void (*free)(void *free_priv, struct sk_buff *skb);
    void *free_priv;
#ifdef SKB_TRACE
    char *alloc_func;
    unsigned int line;
    unsigned int alloc_len;
    unsigned int copyed;
    char *copy_from;
#endif
};

struct sk_buff_head {
    struct sk_buff  *next;
    struct sk_buff  *prev;
    __u32            qlen;
    spinlock_t       lock;
};

#define skb_set_mac_header(...)
#define skb_release_head_state(s)
#define skb_reset_mac_header(...)
#define qdisc_all_tx_empty( ... )   (0)
#define sock_net(...) (0)

#define netdev_alloc_skb(d, s)    alloc_skb(s)
#define dev_alloc_skb(s)          alloc_skb(s)
#define dev_kfree_skb(a)          kfree_skb(a)
#define dev_kfree_skb_any(a)      kfree_skb(a)
#if defined(__MBED__) || defined(__CSKY__)
extern struct sk_buff *alloc_skb(unsigned int size);
extern struct sk_buff *alloc_tx_skb(uint32 size);
extern void kfree_skb(struct sk_buff *skb);
extern int skb_free_count(void);
#else
#ifndef SKB_TRACE
#define alloc_skb(s) _alloc_skb((s))
#define alloc_tx_skb(s) _alloc_tx_skb((s))
#define kfree_skb(s) _kfree_skb((s))
#define skb_free_count() _skb_free_count()
#else
#define alloc_skb(s) _alloc_skb((s),__FUNCTION__,__LINE__)
#define alloc_tx_skb(s) _alloc_tx_skb((s),__FUNCTION__,__LINE__)
#define kfree_skb(s) _kfree_skb((s))
#endif
#endif

#ifdef SKB_TRACE
extern struct sk_buff *_alloc_skb(unsigned int size,char *func,unsigned int line);
extern struct sk_buff *_alloc_tx_skb(uint32 size,char *func,unsigned int line);
#else
extern struct sk_buff *_alloc_skb(unsigned int size);
extern struct sk_buff *_alloc_tx_skb(uint32 size);
#endif
extern void _kfree_skb(struct sk_buff *skb);
extern int _skb_free_count(void)    ;
#ifdef SKB_POOL_ENABLE
extern int skb_rxpool_init(uint8 *buff, uint32 size);
extern int skb_txpool_init(uint8 *buff, uint32 size);
struct sk_buff *skb_rxpool_alloc(uint32 size);
struct sk_buff *skb_txpool_alloc(uint32 size);
int32 skbpool_free_count(void);
#endif

extern struct sk_buff *skb_clone(struct sk_buff *skb, gfp_t priority);
//extern struct sk_buff *skb_copy(const struct sk_buff *skb, gfp_t priority);
#define skb_copy(skb,mask)  _skb_copy((skb), (mask),__FUNCTION__,__LINE__)

extern int  pskb_expand_head(struct sk_buff *skb, int nhead, int ntail, gfp_t gfp_mask);
extern struct sk_buff *skb_copy_expand(const struct sk_buff *skb,
                                       int newheadroom, int newtailroom, gfp_t priority);

#ifdef NET_SKBUFF_DATA_USES_OFFSET
static inline unsigned char *skb_end_pointer(const struct sk_buff *skb)
{
    return skb->head + skb->end;
}
static inline unsigned int skb_end_offset(const struct sk_buff *skb)
{
    return skb->end;
}
#else
static inline unsigned char *skb_end_pointer(const struct sk_buff *skb)
{
    return skb->end;
}
static inline unsigned int skb_end_offset(const struct sk_buff *skb)
{
    return skb->end - skb->head;
}
#endif

static inline int skb_shared(const struct sk_buff *skb)
{
	return atomic_read(&skb->users) != 1;
}

static inline int skb_queue_empty(const struct sk_buff_head *list)
{
    return list->next == (struct sk_buff *)list;
}

static inline bool skb_queue_is_last(const struct sk_buff_head *list,
                                     const struct sk_buff *skb)
{
    return skb->next == (struct sk_buff *)list;
}

static inline bool skb_queue_is_first(const struct sk_buff_head *list,
                                      const struct sk_buff *skb)
{
    return skb->prev == (struct sk_buff *)list;
}

static inline struct sk_buff *skb_queue_next(const struct sk_buff_head *list,
        const struct sk_buff *skb)
{
    BUG_ON(skb_queue_is_last(list, skb));
    return skb->next;
}

static inline struct sk_buff *skb_queue_prev(const struct sk_buff_head *list,
        const struct sk_buff *skb)
{
    /* This BUG_ON may seem severe, but if we just return then we
     * are going to dereference garbage.
     */
    BUG_ON(skb_queue_is_first(list, skb));
    return skb->prev;
}

static inline int skb_cloned(const struct sk_buff *skb)
{
    return skb->cloned;
}

/**
 *  skb_get - reference buffer
 *  @skb: buffer to reference
 *
 *  Makes another reference to a socket buffer and returns a pointer
 *  to the buffer.
 */
static inline struct sk_buff *skb_get(struct sk_buff *skb)
{
    atomic_inc(&skb->users);
    return skb;
}

static inline struct sk_buff *skb_peek(const struct sk_buff_head *list_)
{
    struct sk_buff *skb = list_->next;

    if (skb == (struct sk_buff *)list_) {
        skb = NULL;
    }
    return skb;
}

static inline struct sk_buff *skb_peek_tail(const struct sk_buff_head *list_)
{
    struct sk_buff *skb = list_->prev;

    if (skb == (struct sk_buff *)list_) {
        skb = NULL;
    }
    return skb;
}

static inline __u32 skb_queue_len(const struct sk_buff_head *list_)
{
    return list_->qlen;
}

static inline void __skb_queue_head_init(struct sk_buff_head *list)
{
    list->prev = list->next = (struct sk_buff *)list;
    list->qlen = 0;
}

static inline void skb_queue_head_init(struct sk_buff_head *list)
{
    spin_lock_init(&list->lock);
    __skb_queue_head_init(list);
}

static inline void skb_queue_head_deinit(struct sk_buff_head *list)
{
    BUG_ON(!skb_queue_empty(list));
    spin_lock_deinit(&list->lock);
}

static inline void __skb_insert(struct sk_buff *newsk,
                                struct sk_buff *prev, struct sk_buff *next,
                                struct sk_buff_head *list)
{
    newsk->next = next;
    newsk->prev = prev;
    next->prev  = prev->next = newsk;
    list->qlen++;
    ASSERT(newsk != newsk->next);
}

static inline void __skb_queue_splice(const struct sk_buff_head *list,
                                      struct sk_buff *prev,
                                      struct sk_buff *next)
{
    struct sk_buff *first = list->next;
    struct sk_buff *last = list->prev;

    first->prev = prev;
    prev->next = first;

    last->next = next;
    next->prev = last;
}

static inline void skb_queue_splice(const struct sk_buff_head *list,
                                    struct sk_buff_head *head)
{
    if (!skb_queue_empty(list)) {
        __skb_queue_splice(list, (struct sk_buff *) head, head->next);
        head->qlen += list->qlen;
    }
}

static inline void skb_queue_splice_init(struct sk_buff_head *list,
        struct sk_buff_head *head)
{
    if (!skb_queue_empty(list)) {
        __skb_queue_splice(list, (struct sk_buff *) head, head->next);
        head->qlen += list->qlen;
        __skb_queue_head_init(list);
    }
}

static inline void skb_queue_splice_tail(const struct sk_buff_head *list,
        struct sk_buff_head *head)
{
    if (!skb_queue_empty(list)) {
        __skb_queue_splice(list, head->prev, (struct sk_buff *) head);
        head->qlen += list->qlen;
    }
}

static inline void skb_queue_splice_tail_init(struct sk_buff_head *list,
        struct sk_buff_head *head)
{
    if (!skb_queue_empty(list)) {
        __skb_queue_splice(list, head->prev, (struct sk_buff *) head);
        head->qlen += list->qlen;
        __skb_queue_head_init(list);
    }
}

static inline void __skb_queue_after(struct sk_buff_head *list,
                                     struct sk_buff *prev,
                                     struct sk_buff *newsk)
{
    __skb_insert(newsk, prev, prev->next, list);
}

extern void skb_append(struct sk_buff *old, struct sk_buff *newsk,
                       struct sk_buff_head *list);

static inline void __skb_queue_before(struct sk_buff_head *list,
                                      struct sk_buff *next,
                                      struct sk_buff *newsk)
{
    __skb_insert(newsk, next->prev, next, list);
}

static inline void __skb_queue_head(struct sk_buff_head *list,
                                    struct sk_buff *newsk)
{
    __skb_queue_after(list, (struct sk_buff *)list, newsk);
}

/**
 *  __skb_queue_tail - queue a buffer at the list tail
 *  @list: list to use
 *  @newsk: buffer to queue
 *
 *  Queue a buffer at the end of a list. This function takes no locks
 *  and you must therefore hold required locks before calling it.
 *
 *  A buffer cannot be placed on two lists at the same time.
 */
extern void skb_queue_tail(struct sk_buff_head *list, struct sk_buff *newsk);
static inline void __skb_queue_tail(struct sk_buff_head *list,
                                    struct sk_buff *newsk)
{
    __skb_queue_before(list, (struct sk_buff *)list, newsk);
}

extern void	   skb_unlink(struct sk_buff *skb, struct sk_buff_head *list);
static inline void __skb_unlink(struct sk_buff *skb, struct sk_buff_head *list)
{
    struct sk_buff *next, *prev;

    list->qlen--;
    next       = skb->next;
    prev       = skb->prev;
    skb->next  = skb->prev = NULL;
    next->prev = prev;
    prev->next = next;
    ASSERT(next != skb);
}

/**
 *  __skb_dequeue - remove from the head of the queue
 *  @list: list to dequeue from
 *
 *  Remove the head of the list. This function does not take any locks
 *  so must be used with appropriate locks held only. The head item is
 *  returned or %NULL if the list is empty.
 */
extern struct sk_buff *skb_dequeue(struct sk_buff_head *list);
static inline struct sk_buff *__skb_dequeue(struct sk_buff_head *list)
{
    struct sk_buff *skb = skb_peek(list);
    if (skb) {
        __skb_unlink(skb, list);
    }
    return skb;
}

/**
 *  __skb_dequeue_tail - remove from the tail of the queue
 *  @list: list to dequeue from
 *
 *  Remove the tail of the list. This function does not take any locks
 *  so must be used with appropriate locks held only. The tail item is
 *  returned or %NULL if the list is empty.
 */
extern struct sk_buff *skb_dequeue_tail(struct sk_buff_head *list);
static inline struct sk_buff *__skb_dequeue_tail(struct sk_buff_head *list)
{
    struct sk_buff *skb = skb_peek_tail(list);
    if (skb) {
        __skb_unlink(skb, list);
    }
    return skb;
}


static inline bool skb_is_nonlinear(const struct sk_buff *skb)
{
    return (skb->data_len>0);
}

static inline unsigned int skb_headlen(const struct sk_buff *skb)
{
    return skb->len - skb->data_len;
}


#ifdef NET_SKBUFF_DATA_USES_OFFSET
static inline unsigned char *skb_tail_pointer(const struct sk_buff *skb)
{
    return skb->head + skb->tail;
}
static inline void skb_reset_tail_pointer(struct sk_buff *skb)
{
    skb->tail = skb->data - skb->head;
}
static inline void skb_set_tail_pointer(struct sk_buff *skb, const int offset)
{
    skb_reset_tail_pointer(skb);
    skb->tail += offset;
}
#else /* NET_SKBUFF_DATA_USES_OFFSET */
static inline unsigned char *skb_tail_pointer(const struct sk_buff *skb)
{
    return skb->tail;
}
static inline void skb_reset_tail_pointer(struct sk_buff *skb)
{
    skb->tail = skb->data;
}
static inline void skb_set_tail_pointer(struct sk_buff *skb, const int offset)
{
    skb->tail = skb->data + offset;
}
#endif /* NET_SKBUFF_DATA_USES_OFFSET */

/*
 *  Add data to an sk_buff
 */
static inline unsigned char *skb_put(struct sk_buff *skb, unsigned int len)
{
    unsigned char *tmp = skb_tail_pointer(skb);
    skb->tail += len;
    skb->len  += len;
    ASSERT(skb->tail <= skb->end);
    return tmp;
}

static inline void *skb_put_zero(struct sk_buff *skb, unsigned int len)
{
	void *tmp = skb_put(skb, len);

	memset(tmp, 0, len);

	return tmp;
}

static inline void *skb_put_data(struct sk_buff *skb, const void *data,
				 unsigned int len)
{
	void *tmp = skb_put(skb, len);

	memcpy(tmp, data, len);

	return tmp;
}

static inline void skb_put_u8(struct sk_buff *skb, u8 val)
{
	*(u8 *)skb_put(skb, 1) = val;
}

static inline struct sk_buff *__skb_peek(const struct sk_buff_head *list_)
{
	return list_->next;
}

static inline unsigned char *skb_push(struct sk_buff *skb, unsigned int len)
{
    skb->data -= len;
    skb->len  += len;
    //ASSERT(skb->data >= skb->head);
    if(skb->data < skb->head) {
        while(1) {
            PRINTF("%s:Error!assert:data:%p,hdr:%p,alloc:%s,line:%d\n",
                __FUNCTION__,skb->data,skb->head,skb->alloc_func,skb->line);
            if(skb->copyed) {
                PRINTF("%s:Copyed skb,copy_from:%s\n",__FUNCTION__,skb->copy_from);
            }
            os_sleep_ms(1000);
        }
    }
    return skb->data;
}

static inline unsigned char *__skb_pull(struct sk_buff *skb, unsigned int len)
{
    skb->len -= len;
    BUG_ON(skb->len < skb->data_len);
    return skb->data += len;
}

static inline unsigned char *skb_pull_inline(struct sk_buff *skb, unsigned int len)
{
    return unlikely(len > skb->len) ? NULL : __skb_pull(skb, len);
}

static inline unsigned char *skb_pull(struct sk_buff *skb, unsigned int len)
{
    return skb_pull_inline(skb, len);
}

extern unsigned char *__pskb_pull_tail(struct sk_buff *skb, int delta);

static inline unsigned char *__pskb_pull(struct sk_buff *skb, unsigned int len)
{
    if (len > skb_headlen(skb) &&
        !__pskb_pull_tail(skb, len - skb_headlen(skb))) {
        return NULL;
    }
    skb->len -= len;
    return skb->data += len;
}

static inline unsigned char *pskb_pull(struct sk_buff *skb, unsigned int len)
{
    return unlikely(len > skb->len) ? NULL : __pskb_pull(skb, len);
}

static inline int pskb_may_pull(struct sk_buff *skb, unsigned int len)
{
    if (likely(len <= skb_headlen(skb))) {
        return 1;
    }
    if (unlikely(len > skb->len)) {
        return 0;
    }
    return __pskb_pull_tail(skb, len - skb_headlen(skb)) != NULL;
}

static inline unsigned int skb_headroom(const struct sk_buff *skb)
{
    return skb->data - skb->head;
}

static inline int skb_tailroom(const struct sk_buff *skb)
{
    return skb->end - skb->tail;
}

static inline int skb_dataroom(const struct sk_buff *skb)
{
    return (skb->end - skb->data);
}

static inline void skb_reserve(struct sk_buff *skb, int len)
{
    skb->data += len;
    skb->tail += len;
    ASSERT(skb->data < skb->end);
}

#ifdef NET_SKBUFF_DATA_USES_OFFSET
static inline bool skb_transport_header_was_set(const struct sk_buff *skb)
{
    return skb->transport_header != ~0U;
}

static inline unsigned char *skb_transport_header(const struct sk_buff *skb)
{
    return skb->head + skb->transport_header;
}

static inline void skb_reset_transport_header(struct sk_buff *skb)
{
    skb->transport_header = skb->data - skb->head;
}

static inline void skb_set_transport_header(struct sk_buff *skb,
        const int offset)
{
    skb_reset_transport_header(skb);
    skb->transport_header += offset;
}

static inline unsigned char *skb_network_header(const struct sk_buff *skb)
{
    return skb->head + skb->network_header;
}

static inline void skb_reset_network_header(struct sk_buff *skb)
{
    skb->network_header = skb->data - skb->head;
}

static inline void skb_set_network_header(struct sk_buff *skb, const int offset)
{
    skb_reset_network_header(skb);
    skb->network_header += offset;
}

#else /* NET_SKBUFF_DATA_USES_OFFSET */
static inline bool skb_transport_header_was_set(const struct sk_buff *skb)
{
    return skb->transport_header != NULL;
}

static inline unsigned char *skb_transport_header(const struct sk_buff *skb)
{
    return skb->transport_header;
}

static inline void skb_reset_transport_header(struct sk_buff *skb)
{
    skb->transport_header = skb->data;
}

static inline void skb_set_transport_header(struct sk_buff *skb,
        const int offset)
{
    skb->transport_header = skb->data + offset;
}

static inline unsigned char *skb_network_header(const struct sk_buff *skb)
{
    return skb->network_header;
}

static inline void skb_reset_network_header(struct sk_buff *skb)
{
    skb->network_header = skb->data;
}

static inline void skb_set_network_header(struct sk_buff *skb, const int offset)
{
    skb->network_header = skb->data + offset;
}

#endif /* NET_SKBUFF_DATA_USES_OFFSET */

extern int ___pskb_trim(struct sk_buff *skb, unsigned int len);

static inline void __skb_trim(struct sk_buff *skb, unsigned int len)
{
    if (unlikely(skb_is_nonlinear(skb))) {
        WARN_ON(1);
        return;
    }
    skb->len = len;
    skb_set_tail_pointer(skb, len);
}

extern void skb_trim(struct sk_buff *skb, unsigned int len);

static inline int __pskb_trim(struct sk_buff *skb, unsigned int len)
{
    __skb_trim(skb, len);
    return 0;
}

static inline int pskb_trim(struct sk_buff *skb, unsigned int len)
{
    return (len < skb->len) ? __pskb_trim(skb, len) : 0;
}

/**
 *  __skb_queue_purge - empty a list
 *  @list: list to empty
 *
 *  Delete all buffers on an &sk_buff list. Each buffer is removed from
 *  the list and one reference dropped. This function does not take the
 *  list lock and the caller must hold the relevant locks to use it.
 */
extern void skb_queue_purge(struct sk_buff_head *list);
static inline void __skb_queue_purge(struct sk_buff_head *list)
{
    struct sk_buff *skb;
    while ((skb = __skb_dequeue(list)) != NULL) {
        kfree_skb(skb);
    }
}

static inline int __skb_linearize(struct sk_buff *skb)
{
    return __pskb_pull_tail(skb, skb->data_len) ? 0 : -ENOMEM;
}

static inline void skb_orphan(struct sk_buff *skb)
{
}


/**
 *  skb_linearize - convert paged skb to linear one
 *  @skb: buffer to linarize
 *
 *  If there is no free memory -ENOMEM is returned, otherwise zero
 *  is returned and the old skb data released.
 */
static inline int skb_linearize(struct sk_buff *skb)
{
    return 0;//skb_is_nonlinear(skb) ? __skb_linearize(skb) : 0;
}

#define skb_queue_walk(queue, skb) \
    for (skb = (queue)->next;                   \
         skb != (struct sk_buff *)(queue);              \
         skb = skb->next)

#define skb_queue_walk_safe(queue, skb, tmp)                    \
    for (skb = (queue)->next, tmp = skb->next;          \
         skb != (struct sk_buff *)(queue);              \
         skb = tmp, tmp = skb->next)

#define skb_queue_walk_from(queue, skb)                     \
    for (; skb != (struct sk_buff *)(queue);            \
         skb = skb->next)

#define skb_queue_walk_from_safe(queue, skb, tmp)               \
    for (tmp = skb->next;                       \
         skb != (struct sk_buff *)(queue);              \
         skb = tmp, tmp = skb->next)

#define skb_queue_reverse_walk(queue, skb) \
    for (skb = (queue)->prev;                   \
         skb != (struct sk_buff *)(queue);              \
         skb = skb->prev)

#define skb_queue_reverse_walk_safe(queue, skb, tmp)                \
    for (skb = (queue)->prev, tmp = skb->prev;          \
         skb != (struct sk_buff *)(queue);              \
         skb = tmp, tmp = skb->prev)

#define skb_queue_reverse_walk_from_safe(queue, skb, tmp)           \
    for (tmp = skb->prev;                       \
         skb != (struct sk_buff *)(queue);              \
         skb = tmp, tmp = skb->prev)

/* Iterate through singly-linked GSO fragments of an skb. */
#define skb_list_walk_safe(first, skb, next_skb)                               \
	for ((skb) = (first), (next_skb) = (skb) ? (skb)->next : NULL; (skb);  \
	     (skb) = (next_skb), (next_skb) = (skb) ? (skb)->next : NULL)

extern int skb_copy_bits(const struct sk_buff *skb, int offset,
                         unsigned char *to, int len);

void kfree_skb_list(struct sk_buff *segs);

static inline void skb_mark_not_on_list(struct sk_buff *skb)
{
	skb->next = NULL;
}


static inline void skb_copy_from_linear_data_offset(const struct sk_buff *skb,
        const int offset, unsigned char *to,
        const unsigned int len)
{
    memcpy((void *)to, skb->data + offset, len);
}

static inline void skb_set_queue_mapping(struct sk_buff *skb, u16 queue_mapping)
{
    skb->queue_mapping = queue_mapping;
}

static inline u16 skb_get_queue_mapping(const struct sk_buff *skb)
{
    return skb->queue_mapping;
}

static inline void skb_copy_queue_mapping(struct sk_buff *to, const struct sk_buff *from)
{
    to->queue_mapping = from->queue_mapping;
}

static inline int skb_network_offset(const struct sk_buff *skb)
{
	return skb_network_header(skb) - skb->data;
}


#endif

