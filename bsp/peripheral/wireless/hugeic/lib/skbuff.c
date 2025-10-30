#include <linux/types.h>
#include <linux/string.h>
#include <linux/skbuff.h>

////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef SKB_TRACE
//#define SKB_TRACE_BUFF_SIZE 128
#ifdef SKB_TRACE_BUFF_SIZE 
struct __mtrace {
    void *mem; // memory address
    void *fun; // return function address
    int size;
};
static struct __mtrace g_malloc_trace[SKB_TRACE_BUFF_SIZE];
static unsigned int g_mt_enable = 0;
#define MALLOC_TRACE(p,f,l) do{\
        if(g_mt_enable){\
            int i=0;\
            for(i=0;i<SKB_TRACE_BUFF_SIZE;i++){\
                if(0==g_malloc_trace[i].mem){\
                    g_malloc_trace[i].fun=f;\
                    g_malloc_trace[i].mem=p;\
                    g_malloc_trace[i].size=l;\                    
                    break;\
                }\
            }\
        }\
    }while(0);

#define MALLOC_RELEASE(p) do{\
        if(g_mt_enable){\
            int i=0;\
            for(i=0;i<SKB_TRACE_BUFF_SIZE;i++){\
                if(p==g_malloc_trace[i].mem){\
                    g_malloc_trace[i].mem=0;\
                    g_malloc_trace[i].fun=0;\
                    g_malloc_trace[i].size=0;\
                    break;\
                }\
            }\
        }\
    }while(0);

void malloc_trace(int enable)
{
#ifdef SKB_TRACE_BUFF_SIZE
    g_mt_enable = enable;
#endif
}

void malloc_trace_print()
{
#ifdef SKB_TRACE_BUFF_SIZE
    int i = 0;
    PRINTF("----------------------------------\nMalloc Trace:\n");
    PRINTF("Memory          Function\n");
    for (i = 0; i < SKB_TRACE_BUFF_SIZE; i++) {
        if (g_malloc_trace[i].mem > 0) {
            PRINTF("p:0x%8x      fn:0x%x size:%d\n", 
                g_malloc_trace[i].mem, g_malloc_trace[i].fun,g_malloc_trace[i].line);
        }
    }
    PRINTF("----------------------------------\n");
#endif
}
#else
#define MALLOC_TRACE(p)
#define MALLOC_RELEASE(p)
#define malloc_trace(e)
#define malloc_trace_print()
#endif
#endif


static void free_heapskb(void *priv, struct sk_buff *skb)
{
    if (skb) {
        if(skb->clone) {            
            kfree_skb(skb->clone);                        
        }
        if (skb->head) {
            //FREE(skb->head);
            free_skb_buff(skb->head,skb->free_priv);
        }
        FREE(skb);
    }
}

#ifdef SKB_TRACE
static struct sk_buff *alloc_heapskb(unsigned int size, char *func, unsigned int line)
#else
static struct sk_buff *alloc_heapskb(unsigned int size)
#endif
{
    u8 *data;
    int skb_size = ALIGN(sizeof(struct sk_buff), 32);
    int alloc_size = ALIGN(size + 8, 32);

    struct sk_buff *skb = (struct sk_buff *)MALLOC(skb_size);
    if (!skb) {
        return NULL;
    }
    memset(skb, 0, skb_size);
    
    //data = MALLOC(alloc_size + 32 + 32);
    data = malloc_skb_buff(alloc_size + 32 + 32,&skb->free_priv);
    if (!data || !skb->free_priv) {
        FREE(skb);
        return NULL;
    }

    skb->head = data;
    skb->data = data;
    skb->tail = skb->data;
    skb->end  = skb->data + alloc_size + 32 + 32;
    skb_reset_tail_pointer(skb);
    skb_reset_network_header(skb);
    skb_reset_transport_header(skb);
    skb->free = free_heapskb;
#ifdef SKB_TRACE
    skb->alloc_func = func;
    skb->alloc_len  = size;
    skb->line       = line;
    skb->copyed     = 0;
    skb->copy_from  = NULL;
    //MALLOC_TRACE(skb,func,size);
#endif
    atomic_set(&skb->users, 1);    
    return skb;
}

void _kfree_skb(struct sk_buff *skb)
{
    if (skb) {
        if (likely(!atomic_dec_and_test(&skb->users))) { 
            return;
        }
#ifdef SKB_POOL_ENABLE
        skb->queue_mapping = 0;
        skb->pkt_type = 0;
        skb->data_len = 0;
        skb->priority = 0;
        skb_reset_tail_pointer(skb);
        skb_reset_network_header(skb);
        skb_reset_transport_header(skb);
        memset(skb->cb, 0, sizeof(skb->cb));
#endif
#ifdef SKB_TRACE
        skb->alloc_func = NULL;
        skb->alloc_len  = 0;
        skb->line       = 0;
        skb->copyed     = 0;
        skb->copy_from  = NULL;
        //MALLOC_RELEASE(skb);
#endif
        skb->free(skb->free_priv, skb);
    }    
}

struct sk_buff *_alloc_tx_skb(uint32 size, char *func, unsigned int line)
{
#ifdef SKB_POOL_ENABLE
    return skb_txpool_alloc(size);
#else
    return alloc_heapskb(size, func, line);
#endif
}

struct sk_buff *_alloc_skb(unsigned int size, char *func, unsigned int line)
{
#ifdef SKB_POOL_ENABLE
    return skb_rxpool_alloc(size);
#else
    return alloc_heapskb(size, func, line);
#endif
}

int _skb_free_count(void)
{
#ifdef SKB_POOL_ENABLE
    return skbpool_free_count();
#else
    return 128;
#endif
}

void kfree_skb_list(struct sk_buff *segs)
{
    while (segs) {
        struct sk_buff *next = segs->next;

        kfree_skb(segs);
        segs = next;
    }
}

