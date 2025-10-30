#ifndef _LINUX_DEF_H_
#define _LINUX_DEF_H_

#if defined(__MBED__)
#include "os/mbed/defs.h"
#elif defined(__WINDOWS__)
#include "os/win/defs.h"
#elif defined(__LINUX__)
#include "os/linux/defs.h"
#elif defined(__CSKY__)
#include "os/c-sky/defs.h"
#else
#include "porting/defs.h"
#endif

int scnprintf(char *buf, unsigned long size, const char *fmt, ...);

struct dentry {};

#define __net_exit

#define unlikely(x) (x)
#define likely(x)   (x)

#ifdef __init
#undef __init
#endif
#define __init

#ifdef __exit
#undef __exit
#endif
#define __exit

#define false 0
#define true  1
#define VERIFY_READ 1
#define debug_noinline
#define __cacheline_aligned
#define __must_check
#define __rcu
#define __force
#define __attribute_const__
#define __user
#define __maybe_unused
#define __bitwise__
#define __bitwise
#define __user
#define __kernel
#define __safe
#define __force
#define __nocast
#define __iomem
#define __must_hold(x)
#define __acquires(x)
#define __releases(x)
#ifdef __pure
#undef __pure
#endif
#define __pure
#define __percpu
#define __rcu
#define might_sleep()
#define ASSERT_RTNL()

#define DEFINE_PER_CPU(t,n) t n
#define __builtin_constant_p(a) (a)
#define __chk_user_ptr(x)  (void)0
#define __chk_io_ptr(x)    (void)0
#define __builtin_warning(x, y...) (1)
#define __acquire(x)       (void)0
#define __release(x)       (void)0
#define __cond_lock(x,c)   (c)
#define softirq_count()    (1)
#define printk_ratelimit() (0)
#define access_ok(a,b,c)   (1)
#define IS_ENABLED(...)    (0)
#define ACCESS_ONCE(x)     (*(volatile typeof(x) *)&(x))
#define __ALIGN_KERNEL_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define __ALIGN_MASK(x, mask)   __ALIGN_KERNEL_MASK((x), (mask))
#define __get_user(s,d)     (s=*d,0)
#define kobject_uevent(...) (0)
#define pr_err  PRINTF
#define pr_info PRINTF
#define pr_debug PRINTF
#define pr_warn PRINTF
#define MAX_JIFFY_OFFSET ((LONG_MAX >> 1)-1)
#define rcu_dereference_rtnl(m) (m)
#define crypto_memneq memcmp
#ifndef offsetof
#define offsetof(type, member) ((long) &((type *) 0)->member)
#endif
#ifndef sizeof_field
#define sizeof_field(TYPE, MEMBER) sizeof((((TYPE *)0)->MEMBER))
#endif
#ifndef offsetofend
#define offsetofend(TYPE, MEMBER)  (offsetof(TYPE, MEMBER) + sizeof_field(TYPE, MEMBER))
#endif

#define kmalloc(s,f)   MALLOC(s)
#define kcalloc(s,c,f) MALLOC(s*c)
#define kzalloc(s,f) ({\
        void *__ptr = MALLOC(s);\
        if(__ptr) \
            memset(__ptr, 0, s);\
        __ptr;\
    })
#define kmemdup(p,s,f) ({\
        void *_n_ptr = MALLOC(s);\
        if(_n_ptr) \
            memcpy(_n_ptr, p, s);\
        _n_ptr;\
    })

#define kmalloc_array(n, s, f) kmalloc((s)*(n), f)
#define kvcalloc  kmalloc_array
#define kvzalloc  kzalloc
#define kvfree    kfree
#define free_percpu    kfree
#define alloc_percpu_gfp(s,f)  kmalloc(sizeof(s), f)

#define BUG_ON(e)       ASSERT(!(e))
#define BUILD_BUG_ON(e) ASSERT(!(e))
#define this_cpu_ptr(p) (p)
#define per_cpu_ptr(p,c) (p)

#define PAGE_SHIFT 12

#define panic  PRINTF
#define pr_err_ratelimited(msg)  PRINTF("%s\r\n", msg)
#define BUG()  PRINTF("BUG at: %s, line:%d\r\n", __func__, __LINE__)

#define WARN(condition, format...) ({                       \
        int __ret_warn_on = !!(condition);              \
        if (unlikely(__ret_warn_on))                    \
            printk(format);                  \
        unlikely(__ret_warn_on);                    \
    })

#define WARN_ON(x) ( { \
        int __r = !!(x);\
        if(__r) PRINTF("WARNING: %s:%d "#x"\r\n", __FUNCTION__, __LINE__);\
        __r;\
    })

#define WARN_ONCE    WARN
#define WARN_ON_ONCE WARN_ON
#define EXPORT_SYMBOL_GPL(f)
#define in_interrupt() (0)

extern void get_random_bytes(unsigned char *data, int len);

#define struct_size(p, member, count)	(sizeof(*(p))+(count)*sizeof(*(p)->member))

#define fips_enabled 1
#define for_each_possible_cpu(p)
#define ____cacheline_aligned_in_smp

#define BIT(n)          (0x01u << (n))

#endif

