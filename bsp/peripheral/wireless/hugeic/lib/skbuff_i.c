#include <linux/types.h>
#include <linux/string.h>
#include <linux/skbuff.h>

/* Make sure a field is enclosed inside headers_start/headers_end section */
#define CHECK_SKB_FIELD(field) \
    BUILD_BUG_ON(offsetof(struct sk_buff, field) <      \
                 offsetof(struct sk_buff, headers_start));  \
    BUILD_BUG_ON(offsetof(struct sk_buff, field) >      \
                 offsetof(struct sk_buff, headers_end));    \

static void __copy_skb_header(struct sk_buff *n, const struct sk_buff *old)
{
    n->dev       = old->dev;
    n->transport_header  = old->transport_header;
    n->network_header    = old->network_header;
    memcpy(n->cb, old->cb, sizeof(old->cb));
    n->pkt_type      = old->pkt_type;
    n->ip_summed     = old->ip_summed;
    skb_copy_queue_mapping(n, old);
    n->priority      = old->priority;
    n->protocol      = old->protocol;
}

static void copy_skb_header(struct sk_buff *n, const struct sk_buff *old)
{
    __copy_skb_header(n, old);
}

#define skb_copy(skb,mask)  _skb_copy((skb), (mask),__FUNCTION__,__LINE__)
#if 0
struct sk_buff *skb_copy(const struct sk_buff *skb, gfp_t gfp_mask)
{
    int headerlen = skb_headroom(skb);
    unsigned int size = skb->end - skb->head;//skb_end_offset(skb) + skb->data_len;
    struct sk_buff *n = alloc_tx_skb(size);

    if (!n) {
        return NULL;
    }

    /* Set the data pointer */
    skb_reserve(n, headerlen);
    /* Set the tail pointer and length */
    skb_put(n, skb->len);

    memcpy(n->data, skb->data, skb->len);
    copy_skb_header(n, skb);
    return n;

}
#endif

struct sk_buff *_skb_copy(const struct sk_buff *skb,
                          gfp_t gfp_mask,
                          char *func, unsigned int line)
{
    int headerlen = skb_headroom(skb);
    unsigned int size = skb->end - skb->head;//skb_end_offset(skb) + skb->data_len;
    struct sk_buff *n = alloc_tx_skb(size);

    if (!n) {
        return NULL;
    }

    /* Set the data pointer */
    skb_reserve(n, headerlen);
    /* Set the tail pointer and length */
    skb_put(n, skb->len);

    memcpy(n->data, skb->data, skb->len);
    copy_skb_header(n, skb);
    n->copy_from  = skb->alloc_func;
    n->alloc_func = func;
    n->line       = line;
    n->alloc_len  = size;
    n->copyed     = 1;
    return n;

}


#ifdef __RTOS__
struct sk_buff *skb_clone(struct sk_buff *skb, gfp_t priority)
{
    struct sk_buff *nskb = alloc_skb(128 + 4); // 4 bytes for skb_reserve assert
    if (nskb) {
        skb_reserve(nskb, 128);
        nskb->data = skb->data;
        nskb->len  = skb->len;
        nskb->tail = skb->tail;
        nskb->end  = skb->end;
        nskb->cloned = 1;
        nskb->clone  = skb_get(skb);
    }
    return nskb;
}
#endif

void skb_trim(struct sk_buff *skb, unsigned int len)
{
    if (skb->len > len) {
        __skb_trim(skb, len);
    }
}

unsigned char *__pskb_pull_tail(struct sk_buff *skb, int delta)
{
    return NULL;
}

int skb_copy_bits(const struct sk_buff *skb, int offset, unsigned char *to, int len)
{
    int copy_len  = 0;

    if (!to || len < 0) {
        PRINTF("%s,%d:input param error!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    copy_len = skb->len - offset;
    if (copy_len < 0) {
        PRINTF("%s,%d:input param error!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if (copy_len > len) {
        memcpy(to, skb->data + offset, len);
    } else {
        memcpy(to, skb->data + offset, copy_len);
    }
    return 0;
}

struct sk_buff *skb_dequeue(struct sk_buff_head *list)
{
    unsigned long flags;
    struct sk_buff *result;
    unsigned int mask = 0;

    //spin_lock_irqsave(&list->lock, flags);
    mask = sys_disable_irq();
    result = __skb_dequeue(list);
    sys_enable_irq(mask);
    //spin_unlock_irqrestore(&list->lock, flags);
    return result;
}

void skb_queue_purge(struct sk_buff_head *list)
{
    struct sk_buff *skb;
    while ((skb = skb_dequeue(list)) != NULL) {
        kfree_skb(skb);
    }
}

void skb_queue_tail(struct sk_buff_head *list, struct sk_buff *newsk)
{
    unsigned long flags;
    unsigned int mask = 0;

	mask = sys_disable_irq();

    //spin_lock_irqsave(&list->lock, flags);
    
    __skb_queue_tail(list, newsk);
    sys_enable_irq(mask);
    //spin_unlock_irqrestore(&list->lock, flags);
}

void skb_queue_head(struct sk_buff_head *list, struct sk_buff *newsk)
{
	unsigned long flags;
    unsigned int mask = 0;

    mask = sys_disable_irq();
	//spin_lock_irqsave(&list->lock, flags);
	__skb_queue_head(list, newsk);    
    sys_enable_irq(mask);
	//spin_unlock_irqrestore(&list->lock, flags);
}

int  pskb_expand_head(struct sk_buff *skb, int nhead, int ntail, gfp_t gfp_mask)
{
    int size = nhead + (skb->end - skb->head) + ntail;
    unsigned char *data = NULL;
    long off   = 0;
    void *buff_priv = NULL;

    if (atomic_read(&skb->users) != 1 || nhead < 0) {
        return -EFAULT;
    }

    size = ALIGN(size, 32); 
    data = malloc_skb_buff(size, &buff_priv);
    if(!data) {
        return -ENOMEM;
    }
    memset(data,0,size);
    memcpy(data + nhead, skb->head, skb->end - skb->head);

    if(skb->head) {
        free_skb_buff(skb->head, skb->free_priv);
    }

    off = (data + nhead) - skb->head;
    skb->head     = data;
    skb->data    += off;
    skb->end      = skb->head + size;
    skb->tail    += off;
    skb->transport_header += off;
	skb->network_header   += off;
#ifdef SKB_TRACE
    skb->alloc_func = __FUNCTION__;
    skb->line       = __LINE__;
#endif
    skb->cloned    = 0;
    skb->free_priv = buff_priv;
    return 0;
}


struct sk_buff *skb_copy_expand(const struct sk_buff *skb,
                                int newheadroom, int newtailroom, gfp_t priority)
{
    struct sk_buff *nskb = NULL;
    int size = newheadroom + (skb->end - skb->head) + newtailroom;
    unsigned char *data = NULL;
    long off   = skb->data - skb->head;
    void *buff_priv = NULL;

    size = ALIGN(size, 32); 

    nskb = alloc_skb(size);
    if(!nskb) {
        return NULL; 
    }
    skb_reserve(nskb, off + newheadroom);
    memcpy(nskb->data,skb->data,skb->len);
    skb_put(nskb, skb->len);
    
    nskb->protocol         = skb->protocol; 
    nskb->dev              = skb->dev;           
    nskb->pkt_type         = skb->pkt_type; 
    nskb->priority         = skb->priority; 
    nskb->transport_header = skb->transport_header;
    nskb->network_header   = skb->network_header;
    nskb->sta              = skb->data;
    nskb->txinfo           = skb->txinfo;
    nskb->aid              = skb->aid;
    memcpy(nskb->cb, skb->cb, sizeof(nskb->cb)); 
    return nskb;
}

void skb_unlink(struct sk_buff *skb, struct sk_buff_head *list)
{
    unsigned long flags;
    unsigned int mask = 0;

	mask = sys_disable_irq();

    //spin_lock_irqsave(&list->lock, flags);
    __skb_unlink(skb, list);
    sys_enable_irq(mask);
    //spin_unlock_irqrestore(&list->lock, flags);
}

