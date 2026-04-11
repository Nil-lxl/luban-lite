#ifndef _HGIC_OS_PORTING_H_
#define _HGIC_OS_PORTING_H_

#include <rtthread.h>
#include "rtconfig.h"
#include "os_defs.h"

extern long rt_hw_interrupt_disable(void);
extern void rt_hw_interrupt_enable(long level);

#ifdef TXW901_MANUAL_PROV_MODE
#define BLENC_STA           0
#else
#define BLENC_STA           1
#define BLENC_DATA_SIZE     1024
#ifdef TXW901_BLE_BROADCAST_PROV_MODE
#define HGIC_BLE_TEST_MODE  1
#elif defined(TXW901_BLE_CONNECT_PROV_MODE)
#define HGIC_BLE_TEST_MODE  3
#endif
#endif

#define osWaitForever                   (0xffffffffu)
#define HIGH_TASK_PROI     8
#define NORMAL_TASK_PROI   12

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef sys_memcpy
extern void wifi_memory_copy(void *dst, void *src, unsigned long len);
static inline void sys_memcpy(void *dest, void *src, int len)
{
    wifi_memory_copy(dest, src, len);
}
#endif

static inline unsigned int os_disable_irq(void)
{
    return rt_hw_interrupt_disable();
}

static inline void os_enable_irq(unsigned int f)
{
    rt_hw_interrupt_enable(f);
}

static void hgic_assert(unsigned int arg, char *func, unsigned int line)
{
    if (!arg) {
        rt_kprintf("%s,%d:alios assert!ret:addr:%p\n", func, line, __builtin_return_address(0));
        while (1);
    }
}

#ifndef hgic_dbg
#define hgic_dbg(fmt, ...) PRINTF("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#ifndef hgic_err
#define hgic_err(fmt, ...) printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#ifndef ASSERT
#define ASSERT(arg)                     RT_ASSERT(arg)//hgic_assert(arg,__FUNCTION__,__LINE__)
#endif

#ifndef MEMCPY
#define MEMCPY(dst,src,len)             rt_memcpy(dst,src,len)
#endif

#ifndef MEMSET
#define MEMSET(s,c,n)                   rt_memset(s, c, n)
#endif

#ifndef MALLOC
#define MALLOC(size)                    rt_malloc(size)
#endif

#ifndef FREE
#define FREE(ptr)                       rt_free(ptr)
#endif

#ifndef ZALLOC
#define ZALLOC(size)                    hgic_zalloc(size)
#endif

#ifndef STRDUP
#define STRDUP(s)                       hgic_strdup(s)
#endif

#ifndef REALLOC
#define REALLOC(p,s)                    hgic_realloc(p,s)
#endif

typedef struct {
    unsigned int counter;
} os_atomic_t;

typedef struct {
    unsigned long long counter;
} os_atomic64_t;

#define os_atomic64_read(v)        ((v)->counter)
#define os_atomic_read(v)          ((v)->counter)

#define os_atomic_set(v,i)        ({ \
        int __mask__ = os_disable_irq();\
        ((v)->counter = i); \
        os_enable_irq(__mask__);\
    })
#define os_atomic_inc(v)           ({ \
        int __mask__ = os_disable_irq();\
        ((v)->counter++); \
        os_enable_irq(__mask__);\
    })
#define os_atomic_dec(v)           ({ \
        int __mask__ = os_disable_irq();\
        ((v)->counter--); \
        os_enable_irq(__mask__);\
    })
#endif
