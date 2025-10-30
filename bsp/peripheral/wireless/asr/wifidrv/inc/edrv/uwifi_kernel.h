/*********************************************************************
 *
 * @file uwifi_kernel.h
 *
 * @brief Kernel API definitions.
 *
 * Copyright (C) ASR 2017
 *

**********************************************************************
 */

#ifndef __UWIFI_KERNEL_H__
#define __UWIFI_KERNEL_H__

#include <stdint.h>
#include <stdbool.h>
#include "errno.h"
#include "uwifi_types.h"
#include "asr_rtos.h"
#include "uwifi_ieee80211.h"
#include "uwifi_wlan_list.h"
#include "asr_types.h"

/// Flag bits
#ifndef SCAN_PASSIVE_BIT
#define SCAN_PASSIVE_BIT BIT(0)
#define SCAN_DISABLED_BIT BIT(1)
#endif

/* when open RX BA, RX consumes 3(IPC_RXBUF_CNT)+4(WINDOW_SIZE) buffers, so 8 buffers at least needed */
#define STATIC_SKB_TX_NUM   3
#define DYNAMIC_SKB_TX_NUM   15
#define TOTAL_SKB_NUM        (IPC_RXBUF_CNT+CFG_REORD_BUF+STATIC_SKB_TX_NUM)     //5    //10
#define MAX_SKB_BUF_SIZE     IPC_RXBUF_SIZE

#define min_t(type, x, y)  ((x) < (y) ? (x) : (y))
#define max_t(type, x, y)  ((x) > (y) ? (x) : (y))

#define __round_mask(x, y) ((__typeof__(x))((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#endif



enum ieee80211_channel_flags {
    IEEE80211_CHAN_DISABLED        = 1<<0,
    IEEE80211_CHAN_NO_IR        = 1<<1,
    /* hole at 1<<2 */
    IEEE80211_CHAN_RADAR        = 1<<3,
    IEEE80211_CHAN_NO_HT40PLUS    = 1<<4,
    IEEE80211_CHAN_NO_HT40MINUS    = 1<<5,
    IEEE80211_CHAN_NO_OFDM        = 1<<6,
    IEEE80211_CHAN_NO_80MHZ        = 1<<7,
    IEEE80211_CHAN_NO_160MHZ    = 1<<8,
    IEEE80211_CHAN_INDOOR_ONLY    = 1<<9,
    IEEE80211_CHAN_GO_CONCURRENT    = 1<<10,
    IEEE80211_CHAN_NO_20MHZ        = 1<<11,
    IEEE80211_CHAN_NO_10MHZ        = 1<<12,
};

//add
typedef struct  list_head   _list;
typedef struct    __queue    {
    struct    list_head    queue;
    asr_mutex_t        lock;
} _queue;
/*
 *    This is an Ethernet frame header.
 */
struct ethhdr {
    unsigned char    h_dest[ETH_ALEN];    /* destination eth addr    */
    unsigned char    h_source[ETH_ALEN];    /* source ether addr    */
    uint16_t        h_proto;        /* packet type ID field    */
} __attribute__((packed));

struct iphdrs {
    uint8_t ihl:4;
    uint8_t    version:4;
    uint8_t    tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t    ttl;
    uint8_t    protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
    /*The options start here. */
};

struct asr_pbuf {
    struct asr_pbuf* next;
    void *payload;
    uint16_t tot_len;
    uint16_t len;
};

struct sk_buff {
    /* These two members must be first. */
    struct sk_buff        *next;
    struct sk_buff        *prev;
    struct  list_head list;
    uint32_t len;

    uint8_t *end;//fixed can not change after alloc
    uint8_t *head;//fixed can not change after alloc
    uint8_t *data;//data start
    uint8_t *tail; //data tail
    uint32_t priority;
    /*
    * This is the control buffer. It is free to use for every
    * layer. Please put your private variables there. If you
    * want to keep them across layers you have to do a skb_clone()
    * first. This is owned by whoever has the skb queued ATM.
    */
    uint32_t private[3];
    char            cb[48] __attribute__((aligned(8)));
};
#define SK_BUFF_T_SIZE (sizeof(struct sk_buff))

struct sk_buff_head {
    /* These two members must be first. */
    struct sk_buff    *next;
    struct sk_buff    *prev;

    uint32_t        qlen;
    asr_mutex_t    lock;
};

struct skb_priv {
    uint32_t free_skb_cnt;
    _queue free_skb_queue;
    uint8_t * allocated_skb_addr;
    uint8_t * skb_addr;
    uint8_t tx_skb_cnt;
    uint8_t tx_skb_static_cnt;
};

uint8_t *skb_pull(struct sk_buff *skb, uint32_t len);
unsigned char *skb_put(struct sk_buff *skb, unsigned int len);
uint8_t *skb_push(struct sk_buff *skb, uint32_t len);
void skb_queue_tail(struct sk_buff_head *list, struct sk_buff *newsk);
void skb_append(struct sk_buff *old, struct sk_buff *newsk, struct sk_buff_head *list);
void skb_queue_head(struct sk_buff_head *list, struct sk_buff *newsk);

uint8_t *skb_reinit(struct sk_buff *skb);

int32_t wifi_free_pktbuf(struct sk_buff *skb, uint8_t is_tx);
struct sk_buff *wifi_alloc_pktbuf(uint32_t size, uint8_t is_tx);
int32_t wlan_init_skb_priv(void);
void wlan_free_skb_priv(void);
#define dev_alloc_skb_tx(a) wifi_alloc_pktbuf(a, 1)
#define dev_alloc_skb_rx(a) wifi_alloc_pktbuf(a, 0)
#define dev_kfree_skb_tx(a) wifi_free_pktbuf(a, 1)
#define dev_kfree_skb_rx(a) wifi_free_pktbuf(a, 0)

#define skb_queue_walk_safe(queue, skb, tmp)                    \
        for (skb = (queue)->next, tmp = skb->next;            \
             skb != (struct sk_buff *)(queue);                \
             skb = tmp, tmp = skb->next)

inline static _list *os_list_get_next(_list   *list)
{
    return list->next;
}

inline static _list   *os_get_queue_head(_queue   *queue)
{
    return (&(queue->queue));
}

/**
 *    skb_queue_len    - get queue length
 *    @list_: list to measure
 *
 *    Return the length of an &sk_buff queue.
 */
static inline uint32_t skb_queue_len(const struct sk_buff_head *list_)
{
    return list_->qlen;
}

/**
 *    __skb_queue_head_init - initialize non-spinlock portions of sk_buff_head
 *    @list: queue to initialize
 *
 *    This initializes only the list and queue length aspects of
 *    an sk_buff_head object.  This allows to initialize the list
 *    aspects of an sk_buff_head without reinitializing things like
 *    the spinlock.  It can also be used for on-stack sk_buff_head
 *    objects where the spinlock is known to not be used.
 */
static inline void __skb_queue_head_init(struct sk_buff_head *list)
{
    list->prev = list->next = (struct sk_buff *)list;
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
static inline void skb_queue_head_init(struct sk_buff_head *list)
{
    asr_rtos_init_mutex(&list->lock);  //ldw init spinloc, will adapt to semp or mutex
    __skb_queue_head_init(list);
}

/**
 *    skb_queue_empty - check if a queue is empty
 *    @list: queue head
 *
 *    Returns true if the queue is empty, false otherwise.
 */
static inline int skb_queue_empty(const struct sk_buff_head *list)
{
    return list->next == (const struct sk_buff *) list;
}

/*
 *    Insert an sk_buff on a list.
 *
 *    The "__skb_xxxx()" functions are the non-atomic ones that
 *    can only be called with interrupts disabled.
 */
static inline void __skb_insert(struct sk_buff *newsk,
                struct sk_buff *prev, struct sk_buff *next,
                struct sk_buff_head *list)
{
    newsk->next = next;
    newsk->prev = prev;
    next->prev  = prev->next = newsk;
    list->qlen++;
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

/**
 *    skb_queue_splice - join two skb lists, this is designed for stacks
 *    @list: the new list to add
 *    @head: the place to add it in the first list
 */
static inline void skb_queue_splice(const struct sk_buff_head *list,
                    struct sk_buff_head *head)
{
    if (!skb_queue_empty(list)) {
        __skb_queue_splice(list, (struct sk_buff *) head, head->next);
        head->qlen += list->qlen;
    }
}

/**
 *    skb_queue_splice_init - join two skb lists and reinitialise the emptied list
 *    @list: the new list to add
 *    @head: the place to add it in the first list
 *
 *    The list at @list is reinitialised
 */
static inline void skb_queue_splice_init(struct sk_buff_head *list,
                     struct sk_buff_head *head)
{
    if (!skb_queue_empty(list)) {
        __skb_queue_splice(list, (struct sk_buff *) head, head->next);
        head->qlen += list->qlen;
        __skb_queue_head_init(list);
    }
}

/**
 *    skb_queue_splice_tail - join two skb lists, each list being a queue
 *    @list: the new list to add
 *    @head: the place to add it in the first list
 */
static inline void skb_queue_splice_tail(const struct sk_buff_head *list,
                     struct sk_buff_head *head)
{
    if (!skb_queue_empty(list)) {
        __skb_queue_splice(list, head->prev, (struct sk_buff *) head);
        head->qlen += list->qlen;
    }
}

/**
 *    skb_queue_splice_tail_init - join two skb lists and reinitialise the emptied list
 *    @list: the new list to add
 *    @head: the place to add it in the first list
 *
 *    Each of the lists is a queue.
 *    The list at @list is reinitialised
 */
static inline void skb_queue_splice_tail_init(struct sk_buff_head *list,
                          struct sk_buff_head *head)
{
    if (!skb_queue_empty(list)) {
        __skb_queue_splice(list, head->prev, (struct sk_buff *) head);
        head->qlen += list->qlen;
        __skb_queue_head_init(list);
    }
}

/**
 *    skb_peek - peek at the head of an &sk_buff_head
 *    @list_: list to peek at
 *
 *    Peek an &sk_buff. Unlike most other operations you _MUST_
 *    be careful with this one. A peek leaves the buffer on the
 *    list and someone else may run off with it. You must hold
 *    the appropriate locks or have a private queue to do this.
 *
 *    Returns %NULL for an empty list or a pointer to the head element.
 *    The reference count is not incremented and the reference is therefore
 *    volatile. Use with caution.
 */
static inline struct sk_buff *skb_peek(const struct sk_buff_head *list_)
{
    struct sk_buff *skb = list_->next;

    if (skb == (struct sk_buff *)list_)
        skb = NULL;
    return skb;
}

static inline void __skb_queue_before(struct sk_buff_head *list,
                      struct sk_buff *next,
                      struct sk_buff *newsk)
{
    __skb_insert(newsk, next->prev, next, list);
}

static inline void __skb_queue_tail(struct sk_buff_head *list,
                   struct sk_buff *newsk)
{
    __skb_queue_before(list, (struct sk_buff *)list, newsk);
}

/*
 * remove sk_buff from list. _Must_ be called atomically, and with
 * the list known..
 */
void skb_unlink(struct sk_buff *skb, struct sk_buff_head *list);
static inline void __skb_unlink(struct sk_buff *skb, struct sk_buff_head *list)
{
    struct sk_buff *next, *prev;

    list->qlen--;
    next       = skb->next;
    prev       = skb->prev;
    skb->next  = skb->prev = NULL;
    next->prev = prev;
    prev->next = next;
}

/**
 *    __skb_dequeue - remove from the head of the queue
 *    @list: list to dequeue from
 *
 *    Remove the head of the list. This function does not take any locks
 *    so must be used with appropriate locks held only. The head item is
 *    returned or %NULL if the list is empty.
 */
static inline struct sk_buff *__skb_dequeue(struct sk_buff_head *list)
{
    struct sk_buff *skb = skb_peek(list);
    if (skb)
        __skb_unlink(skb, list);
    return skb;
}

/**
 *    __skb_queue_after - queue a buffer at the list head
 *    @list: list to use
 *    @prev: place after this buffer
 *    @newsk: buffer to queue
 *
 *    Queue a buffer int the middle of a list. This function takes no locks
 *    and you must therefore hold required locks before calling it.
 *
 *    A buffer cannot be placed on two lists at the same time.
 */
static inline void __skb_queue_after(struct sk_buff_head *list,
                     struct sk_buff *prev,
                     struct sk_buff *newsk)
{
    __skb_insert(newsk, prev, prev->next, list);
}

/**
 *    __skb_queue_head - queue a buffer at the list head
 *    @list: list to use
 *    @newsk: buffer to queue
 *
 *    Queue a buffer at the start of a list. This function takes no locks
 *    and you must therefore hold required locks before calling it.
 *
 *    A buffer cannot be placed on two lists at the same time.
 */
static inline void __skb_queue_head(struct sk_buff_head *list,
                    struct sk_buff *newsk)
{
    __skb_queue_after(list, (struct sk_buff *)list, newsk);
}

static inline void __skb_queue_purge(struct sk_buff_head *list)
{
    struct sk_buff *skb;
    while ((skb = __skb_dequeue(list)) != NULL)
        dev_kfree_skb_rx(skb);
}

static inline struct ethhdr *eth_hdr(const struct sk_buff *skb)
{
    return (struct ethhdr *)(skb->data);
}

static inline uint8_t ipv4_get_dsfield(const struct iphdrs *iph)
{
    return iph->tos;
}

/**
 * the dsfield is bit4-bit12 start from the ipv6 header
 */
static inline uint8_t ipv6_get_dsfield(const uint16_t *ipv6h)
{
    //return ntohs(*ipv6h) >> 4;
    return __be16_to_cpu(*ipv6h) >> 4;
}

/**
 * is_multicast_ether_addr - Determine if the Ethernet address is a multicast.
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is a multicast address.
 * By definition the broadcast address is also a multicast address.
 */
static inline bool is_multicast_ether_addr(const uint8_t *addr)
{
    return 0x01 & addr[0];
}

/**
 * is_broadcast_ether_addr - Determine if the Ethernet address is broadcast
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is the broadcast address.
 *
 * Please note: addr must be aligned to u16.
 */
static inline bool is_broadcast_ether_addr(const uint8_t *addr)
{
    return (*(const uint16_t *)(addr + 0) &
        *(const uint16_t *)(addr + 2) &
        *(const uint16_t *)(addr + 4)) == 0xffff;
}

/**
 * ether_addr_equal - Compare two Ethernet addresses
 * @addr1: Pointer to a six-byte array containing the Ethernet address
 * @addr2: Pointer other six-byte array containing the Ethernet address
 *
 * Compare two Ethernet addresses, returns true if equal
 *
 * Please note: addr1 & addr2 must both be aligned to u16.
 */
static inline bool ether_addr_equal(const uint8_t *addr1, const uint8_t *addr2)
{
    const uint16_t *a = (const uint16_t *)addr1;
    const uint16_t *b = (const uint16_t *)addr2;

    return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) == 0;
}

static  inline uint8_t passive_scan_flag(uint32_t flags) {
    if (flags & (IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_RADAR))
        return SCAN_PASSIVE_BIT;
    return 0;
}

#if !defined(ALIOS_SUPPORT) && !defined(AWORKS) && !defined(STM32H743xx)
/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */

static inline int fls(int x)
{
    int r = 32;

    if (!x)
        return 0;
    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}
#endif

/**
 *    skb_reserve - adjust headroom
 *    @skb: buffer to alter
 *    @len: bytes to move
 *
 *    Increase the headroom of an empty &sk_buff by reducing the tail
 *    room. This is only allowed for an empty buffer.
 */
static inline void skb_reserve(struct sk_buff *skb, int len)
{
    skb->data += len;
    skb->tail += len;
}

static inline bool skb_is_nonlinear(const struct sk_buff *skb)
{
    return skb->len;
}

/**
 *    skb_headroom - bytes at buffer head
 *    @skb: buffer to check
 *
 *    Return the number of bytes of free space at the head of an &sk_buff.
 */
static inline unsigned int skb_headroom(const struct sk_buff *skb)
{
    return skb->data - skb->head;
}

/**
 *    skb_tailroom - bytes at buffer end
 *    @skb: buffer to check
 *
 *    Return the number of bytes of free space at the tail of an sk_buff
 */
static inline int skb_tailroom(const struct sk_buff *skb)
{
    //return skb_is_nonlinear(skb) ? 0 : skb->end - skb->tail;
    return skb->end - skb->tail;
}


#endif //__UWIFI_KERNEL_H__
