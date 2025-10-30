
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/skb_list.h>

static inline struct sk_buff *__skb_list_peek(const struct skb_list *list)
{
    struct sk_buff *skb = list->next;
    if (skb == (struct sk_buff *)list) {
        skb = NULL;
    }
    return skb;
}

static inline struct sk_buff *__skb_list_tail(const struct skb_list *list)
{
    struct sk_buff *skb = list->prev;
    if (skb == (struct sk_buff *)list) {
        skb = NULL;
    }
    return skb;
}

static inline void __skb_list_insert(struct sk_buff *newsk,
                                     struct sk_buff *prev, struct sk_buff *next,
                                     struct skb_list *list)
{
    newsk->next = next;
    newsk->prev = prev;
    next->prev  = prev->next = newsk;
    list->count++;
    if(newsk == newsk->next){
        ASSERT(newsk != newsk->next);
    }
}

static inline void __skb_list_queue_after(struct skb_list *list,
        struct sk_buff *prev,
        struct sk_buff *newsk)
{
    __skb_list_insert(newsk, prev, prev->next, list);
}

static inline void __skb_list_queue_before(struct skb_list *list,
        struct sk_buff *next,
        struct sk_buff *newsk)
{
    __skb_list_insert(newsk, next->prev, next, list);
}

static inline void __skb_list_queue_head(struct skb_list *list, struct sk_buff *newsk)
{
    __skb_list_queue_after(list, (struct sk_buff *)list, newsk);
}

static inline void __skb_list_queue_tail(struct skb_list *list, struct sk_buff *newsk)
{
    list->prev->next = newsk;
    newsk->prev = list->prev;
    newsk->next = (struct sk_buff *)list;
    list->prev  = newsk;
    list->count++;
}

static inline void __skb_list_unlink(struct sk_buff *skb, struct skb_list *list)
{
    list->count--;
    skb->next->prev = skb->prev;
    skb->prev->next = skb->next;
    skb->next = skb->prev = NULL;
    //PRINTF("%s:skb:%p\n",__FUNCTION__,skb);
}

int32 skb_list_init(struct skb_list *list)
{
    list->prev = list->next = (struct sk_buff *)list;
    list->count = 0;
    return RET_OK;
}

int32 skb_list_destroy(struct skb_list *list)
{
    return RET_OK;
}

int32 skb_list_queue(struct skb_list *list, struct sk_buff *skb)
{
    uint32 mask = 0;

    //gpio_set_val(0, 1);
    mask = sys_disable_irq();
    __skb_list_queue_tail(list, skb);
    sys_enable_irq(mask);
    //gpio_set_val(0, 0);
    return RET_OK;
}

struct sk_buff *skb_list_dequeue(struct skb_list *list) //5us
{
    uint32 mask = 0;
    struct sk_buff *skb = NULL;

    //gpio_set_val(0, 1);
    mask = sys_disable_irq();
    skb = __skb_list_peek(list);
    if (skb) {
        __skb_list_unlink(skb, list);
    }
    sys_enable_irq(mask);
    //gpio_set_val(0, 0);
    return skb;
}

struct sk_buff *skb_list_first(struct skb_list *list)
{
    uint32 mask = 0;
    struct sk_buff *skb = NULL;

    //gpio_set_val(0, 1);
    mask = sys_disable_irq();
    skb = __skb_list_peek(list);
    sys_enable_irq(mask);
    //gpio_set_val(0, 0);
    return skb;
}

struct sk_buff *skb_list_last(struct skb_list *list)
{
    uint32 mask = 0;
    struct sk_buff *skb = NULL;

    //gpio_set_val(0, 1);
    mask = sys_disable_irq();
    skb = __skb_list_tail(list);
    sys_enable_irq(mask);
    //gpio_set_val(0, 0);
    return skb;
}

uint32 skb_list_count(struct skb_list *list)
{
    return list->count;
}

int32 skb_list_unlink(struct sk_buff *skb, struct skb_list *list)
{
    uint32 mask = 0;

    mask = sys_disable_irq();
    __skb_list_unlink(skb, list);
    sys_enable_irq(mask);
    return RET_OK;
}

int32 skb_list_queue_before(struct skb_list *list,
        struct sk_buff *next,
        struct sk_buff *newsk)
{
    uint32 mask = 0;

    mask = sys_disable_irq();
    __skb_list_queue_before(list, next, newsk);
    sys_enable_irq(mask);
    return RET_OK;
}

int32 skb_list_queue_after(struct skb_list *list,
        struct sk_buff *prev,
        struct sk_buff *newsk)
{
    uint32 mask = 0;

    mask = sys_disable_irq();
    __skb_list_queue_after(list, prev, newsk);
    sys_enable_irq(mask);
    return RET_OK;
}



