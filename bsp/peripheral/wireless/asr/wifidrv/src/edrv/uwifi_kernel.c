/**
 ******************************************************************************
 *
 * @file uwifi_kernel.c
 *
 * @brief like linux kernel related function
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */
#include "uwifi_kernel.h"
#include "uwifi_include.h"
extern uint32_t current_iftype;

typedef struct
{
    uint8_t buf[MAX_SKB_BUF_SIZE];
}sk_buffer_t;

struct skb_priv gskb_priv={0};

//uint8_t g_sk_buf_t[SK_BUFF_T_SIZE * TOTAL_SKB_NUM + 4];
//sk_buffer_t g_sk_buf_pool[TOTAL_SKB_NUM];

/*
 *    the following is os independent
 */
void os_init_listhead(_list *list)
{
    INIT_LIST_HEAD(list);
}

uint32_t os_list_empty(_list *phead)
{
    return list_empty(phead);
}

void os_list_insert_head(_list *plist, _list *phead)
{
    list_add(plist, phead);
}

void os_list_insert_tail(_list *plist, _list *phead)
{
    list_add_tail(plist, phead);
}

void os_list_delete(_list *plist)
{
    list_del_init(plist);
}

void os_init_queue(_queue *pqueue)
{
    os_init_listhead(&(pqueue->queue));
    asr_rtos_init_mutex(&(pqueue->lock));
}

void os_deinit_queue(_queue *pqueue)
{
    asr_rtos_deinit_mutex(&(pqueue->lock));
}

uint32_t os_queue_empty(_queue *pqueue)
{
     return (os_list_empty(&(pqueue->queue)));
}

uint32_t os_end_of_queue(_list *head, _list *plist)
{
    if (head == plist)
        return true;
    else
        return false;
}

#if 0
//skb buffer management
struct sk_buff *_alloc_skb(struct skb_priv *pskb_priv)
{
    uint32_t i;
    struct sk_buff *skb = NULL;

    /*maybe we should aligmnet for datasize */
    pskb_priv->allocated_skb_addr = g_sk_buf_t;
    memset(pskb_priv->allocated_skb_addr, 0, sizeof(g_sk_buf_t[SK_BUFF_T_SIZE * TOTAL_SKB_NUM + 4]));

    pskb_priv->skb_addr = pskb_priv->allocated_skb_addr + 4 -
                            ((unsigned long) (pskb_priv->allocated_skb_addr) & 3);

    skb = (struct sk_buff *) pskb_priv->skb_addr;


    memset(g_sk_buf_pool, 0, sizeof(g_sk_buf_pool[TOTAL_SKB_NUM]));
    for (i = 0; i < TOTAL_SKB_NUM; i++)
    {
        skb->tail = skb->data = g_sk_buf_pool[i].buf;
        skb->head = skb->data;
        skb->end = skb->head + MAX_SKB_BUF_SIZE;
        //skb->len = MAX_SKB_BUF_SIZE;
        //os_memset(skb->data, 0, size);

        os_init_listhead(&(skb->list));
        os_list_insert_tail(&skb->list, &(pskb_priv->free_skb_queue.queue));
        pskb_priv->free_skb_cnt++;

        skb++;
    }

    return (struct sk_buff *) pskb_priv->skb_addr;
}

void _free_skb(struct skb_priv *pskb_priv)
{
    uint32_t i;
    struct sk_buff *skb = (struct sk_buff *)pskb_priv->skb_addr;;

    for (i=0; i<TOTAL_SKB_NUM; i++)
    {
        if (skb)
        {
            if (skb->head)
            {
                //asr_rtos_free(skb->head);
                skb->head = NULL;
                skb->data = NULL;
            }
        }
        else
        {
            break;
        }
        skb++;
    }

    if (pskb_priv->allocated_skb_addr)
    {
        //asr_rtos_free(pskb_priv->allocated_skb_addr);
        pskb_priv->allocated_skb_addr = NULL;
    }
}
#endif

int32_t wlan_init_skb_priv(void)
{
    struct skb_priv *pskb_priv = &gskb_priv;

    memset((uint8_t *)pskb_priv, 0, sizeof(struct skb_priv));

    os_init_queue(&pskb_priv->free_skb_queue);


    return _SUCCESS;
}

void wlan_free_skb_priv(void)
{
    struct skb_priv *pskb_priv = &gskb_priv;

    os_deinit_queue(&pskb_priv->free_skb_queue);
}
extern int g_amsdu;
int g_amsdu_free_cnt = 0;
int g_amsdu_malloc_cnt = 0;

struct sk_buff *wifi_alloc_pktbuf(uint32_t size, uint8_t is_tx)
{
    struct skb_priv *pskb_priv = &gskb_priv;
    struct sk_buff *pskb =  NULL;
    _queue *pfree_skb_queue = &(pskb_priv->free_skb_queue);

    if(pfree_skb_queue->lock == NULL)
        return NULL;

    asr_rtos_lock_mutex(&pfree_skb_queue->lock);
    {
        pskb = asr_rtos_malloc(sizeof(struct sk_buff));

        if (NULL == pskb)
        {
            dbg(D_ERR, D_UWIFI_DATA,"skb malloc from stack failed\n");
            asr_rtos_unlock_mutex(&pfree_skb_queue->lock);
            return NULL;
        }
        memset(pskb, 0, sizeof(struct sk_buff));

        pskb->data = asr_rtos_malloc(size);
        if (NULL == pskb->data)
        {
            dbg(D_ERR, D_UWIFI_DATA,"data malloc from stack failed\n");
            asr_rtos_free(pskb);
            asr_rtos_unlock_mutex(&pfree_skb_queue->lock);
            return NULL;
        }
        memset(pskb->data, 0, size);

        pskb->tail = pskb->data;
        pskb->head = pskb->data;
        pskb->end = pskb->head + size;
        pskb_priv->tx_skb_cnt++;
    }

    asr_rtos_unlock_mutex(&pfree_skb_queue->lock);
    if(is_tx==55)
        g_amsdu_malloc_cnt++;

    return pskb;
}


int32_t wifi_free_pktbuf(struct sk_buff *skb, uint8_t is_tx)
{
    struct skb_priv *pskb_priv = &gskb_priv;
    _queue *pfree_skb_queue = &pskb_priv->free_skb_queue;

    if (skb==NULL) {
        return _FAIL;
    }

    if(pfree_skb_queue->lock == NULL)
        return _FAIL;

    asr_rtos_lock_mutex(&pfree_skb_queue->lock);

    if(skb->head){

        asr_rtos_free(skb->head);

        skb->head = NULL;
        skb->data = NULL;
    }

    if(skb)
        asr_rtos_free(skb);

    skb = NULL;

    asr_rtos_unlock_mutex(&pfree_skb_queue->lock);

    if(is_tx == 55)
        g_amsdu_free_cnt++;

    return _SUCCESS;
}

uint8_t *skb_pull(struct sk_buff *skb, uint32_t len)
{
    //return skb->data += len;

    if (len > skb->len)
    {
        dbg(D_ERR, D_UWIFI_DATA, "skb_pull len is more than skb len, pull len:%u, skb len:%u\n", (unsigned int)len,
                                                                                                (unsigned int)skb->len);
        return NULL;
    }

    skb->len -= len;
    skb->data = (uint8_t *)(((uint32_t)skb->data) + len);

    return skb->data;
}

uint8_t *skb_push(struct sk_buff *skb, uint32_t len)
{
    if ((skb->data-len) < skb->head)
    {
        dbg(D_ERR, D_UWIFI_DATA, "skb_push len is more than skb head, skb data:%x, skb head:%x, len:%u\n",
                                                   (unsigned int)skb->data, (unsigned int)skb->head, (unsigned int)len);
        return NULL;
    }

    skb->len += len;
    skb->data = (uint8_t *)(((uint32_t)skb->data) - len);

    return skb->data;
}

/**
 *    skb_put - add data to a buffer
 *    @skb: buffer to use
 *    @len: amount of data to add
 *
 *    This function extends the used data area of the buffer.
 *    A pointer to the first byte of the extra data is returned.
 */
unsigned char *skb_put(struct sk_buff *skb, unsigned int len)
{
    unsigned char *tmp = skb->tail;

    skb->tail += len;
    skb->len  += len;
    if(skb->tail>skb->end)
    {
        dbg(D_ERR, D_UWIFI_DATA, "skb_put tail over the end, head:%x, data:%x, tail:%x, end:%x, len:%d\n",
                                                                                           (unsigned int)skb->head,
                                                                                           (unsigned int)skb->data,
                                                                                           (unsigned int)skb->tail,
                                                                                           (unsigned int)skb->end, len);
        return NULL;
    }
    return tmp;
}

uint8_t *skb_reinit(struct sk_buff *skb)
{
    uint8_t *phead = NULL,*pend = NULL;
    if (!skb)
    {
        return NULL;
    }

    phead = skb->head;
    pend = skb->end;

    memset(skb, 0, sizeof(struct sk_buff));
    memset(phead, 0, (uint32_t)pend - (uint32_t)phead);

    skb->data = phead;
    skb->tail = skb->data;
    skb->head = skb->data;
    skb->end = pend;

    return skb->data;
}

/**
 *    skb_queue_tail - queue a buffer at the list tail
 *    @list: list to use
 *    @newsk: buffer to queue
 *
 *    Queue a buffer at the tail of the list. This function takes the
 *    list lock and can be used safely with other locking &sk_buff functions
 *    safely.
 *
 *    A buffer cannot be placed on two lists at the same time.
 */
void skb_queue_tail(struct sk_buff_head *list, struct sk_buff *newsk)
{
    asr_rtos_lock_mutex(&list->lock);
    __skb_queue_tail(list, newsk);
    asr_rtos_unlock_mutex(&list->lock);
}

/**
 *    skb_append    -    append a buffer
 *    @old: buffer to insert after
 *    @newsk: buffer to insert
 *    @list: list to use
 *
 *    Place a packet after a given packet in a list. The list locks are taken
 *    and this function is atomic with respect to other list locked calls.
 *    A buffer cannot be placed on two lists at the same time.
 */
void skb_append(struct sk_buff *old, struct sk_buff *newsk, struct sk_buff_head *list)
{
    asr_rtos_lock_mutex(&list->lock);
    __skb_queue_after(list, old, newsk);
    asr_rtos_unlock_mutex(&list->lock);
}

/**
 *    skb_queue_head - queue a buffer at the list head
 *    @list: list to use
 *    @newsk: buffer to queue
 *
 *    Queue a buffer at the start of the list. This function takes the
 *    list lock and can be used safely with other locking &sk_buff functions
 *    safely.
 *
 *    A buffer cannot be placed on two lists at the same time.
 */
void skb_queue_head(struct sk_buff_head *list, struct sk_buff *newsk)
{
    asr_rtos_lock_mutex(&list->lock);
    __skb_queue_head(list, newsk);
    asr_rtos_unlock_mutex(&list->lock);
}


/**
 *    skb_unlink    -    remove a buffer from a list
 *    @skb: buffer to remove
 *    @list: list to use
 *
 *    Remove a packet from a list. The list locks are taken and this
 *    function is atomic with respect to other list locked calls
 *
 *    You must know what list the SKB is on.
 */
void skb_unlink(struct sk_buff *skb, struct sk_buff_head *list)
{
    asr_rtos_lock_mutex(&list->lock);
    __skb_unlink(skb, list);
    asr_rtos_unlock_mutex(&list->lock);
}


