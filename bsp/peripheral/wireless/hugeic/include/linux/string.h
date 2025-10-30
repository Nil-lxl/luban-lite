#ifndef _LINUX_STRING_H_
#define _LINUX_STRING_H_
#include <linux/jiffies.h>
#include <linux/defs.h>

//void *malloc(size_t size);
//void free(void *ptr);
//void *zalloc(size_t size);
//void *realloc(void *ptr, size_t size);

void *_os_malloc(int size);
void _os_free(void *ptr);
void *_os_zalloc(int size);
void *_os_realloc(void *ptr, int size);
void *_os_malloc_t(int size, const char *func, int line);
void _os_free_t(void *ptr);
void *_os_zalloc_t(int size, const char *func, int line);
void *_os_realloc_t(void *ptr, int size, const char *func, int line);
void _os_alloc_status(void);

#if defined(__MBED__)
#include "os/mbed/string.h"
#elif defined(__WINDOWS__)
#include "os/win/string.h"
#elif defined(__LINUX__)
#include "os/linux/string.h"
#elif defined(__CSKY__)
#include "os/c-sky/string.h"
#else
//#include "porting/string.h"
#endif

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/rcu.h>
#if UMAC_CORE_DEBUG
#define printk PRINTF
#else
#define printk(...)
#endif

static inline void kfree(unsigned char *p) 
{
    if(p) {
        FREE(p);
    }
}

//#define kfree(p)        FREE((void *)p)
#define kzfree          kfree

#define __is_kfree_rcu_offset(offset) ((offset) < 4096)

static inline void kfree_call_rcu(struct rcu_head *head,
                  void (*func)(struct rcu_head *rcu))
{
    call_rcu(head, func);
}

#define __kfree_rcu(head, offset) \
        do { \
            kfree_call_rcu(head, (void (*)(struct rcu_head *))(unsigned long)(offset)); \
        } while (0)

#define kfree_rcu(ptr, rcu_head)					\
	__kfree_rcu(&((ptr)->rcu_head), offsetof(typeof(*(ptr)), rcu_head))

#endif
