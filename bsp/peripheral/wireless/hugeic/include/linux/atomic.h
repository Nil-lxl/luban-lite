#ifndef _LINUX_ATOMIC_H_
#define _LINUX_ATOMIC_H_

#if defined(__MBED__)
#include "os/mbed/atomic.h"
#elif defined(__WINDOWS__)
#include "os/win/atomic.h"
#elif defined(__LINUX__)
#include "os/linux/atomic.h"
#elif defined(__CSKY__)
#include "os/c-sky/atomic.h"
#else
//#include "porting/atomic.h"
#endif

typedef struct {
    unsigned int counter;
} atomic_t;

typedef struct {
    unsigned long long counter;
} atomic64_t;

#define atomic64_read(v)        ((v)->counter)
#define atomic_read(v)          ((v)->counter)
#define atomic_set(v,i)        ({ \
        unsigned int __mask__ = sys_disable_irq();\
        ((v)->counter = i); \
        sys_enable_irq(__mask__);\
    })
#define atomic_inc(v)           ({ \
        unsigned int __mask__ = sys_disable_irq();\
        ((v)->counter++); \
        sys_enable_irq(__mask__);\
    })
#define atomic_dec(v)           ({ \
        unsigned int __mask__ = sys_disable_irq();\
        ((v)->counter--); \
        sys_enable_irq(__mask__);\
    })
#define atomic_inc_return(v)    ({\
        unsigned int __mask__ = sys_disable_irq();\
        unsigned int __val__ = (++(v)->counter);\
        sys_enable_irq(__mask__);\
        __val__;\
    })
#define atomic_dec_return(v)    ({\
        unsigned int __mask__ = sys_disable_irq();\
        unsigned int __val__ = (--(v)->counter);\
        sys_enable_irq(__mask__);\
        __val__;\
    })
#define atomic_dec_and_test(v)    ({\
        unsigned int __mask__ = sys_disable_irq();\
        unsigned int __val__ = (--(v)->counter == 0);\
        sys_enable_irq(__mask__);\
        __val__;\
    })
#define atomic_cmpxchg(v,o,n)    ({\
        unsigned int __mask__ = sys_disable_irq();\
        unsigned int __val = (v)->counter;\
        if(__val == o) (v)->counter = n;\
        sys_enable_irq(__mask__);\
        __val;\
    })
#define atomic_add(i,v)        ({ \
        unsigned int __mask__ = sys_disable_irq();\
        ((v)->counter += (i)); \
        sys_enable_irq(__mask__);\
    })
#define atomic_sub_return(i,v)    ({\
        unsigned int __mask__ = sys_disable_irq();\
        unsigned int __val__ = ((v)->counter-=(i));\
        sys_enable_irq(__mask__);\
        __val__;\
    })

static inline int atomic_add_unless(atomic_t *v, int a, int u)
{
    int c, old;
    c = atomic_read(v);
    while (c != u && (old = atomic_cmpxchg((v), c, c + a)) != c) {
        c = old;
    }
    return c != u;
}


#endif
