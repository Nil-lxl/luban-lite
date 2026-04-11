#ifndef _HGIC_PORTING_DEFS_H_
#define _HGIC_PORTING_DEFS_H_

#include "rtconfig.h"

#ifndef typeof
#define typeof __typeof__
#endif

#ifndef MALLOC
#define MALLOC(size)                    rt_malloc(size)
#endif

#ifndef FREE
#define FREE(ptr)                       rt_free(ptr)
#endif

#define ASSERT(arg)                     os_assert(arg,__FUNCTION__,__LINE__,__builtin_return_address(0))

#define HZ  (1000L)
#define OS_TICKS_PER_SECOND (1000L)
#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC	1000L
#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define USEC_PER_SEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define FSEC_PER_SEC	1000000000000000LL

#define jiffies ((unsigned long)rt_tick_get())
#define jiffies_to_msecs(j) sys_jiffies_to_msecs((j))
#define msecs_to_jiffies(m) rt_tick_from_millisecond((m))
//#define os_sleep_ms(ms) rt_thread_sleep((rt_tick_from_millisecond((ms))) + 1)
#define os_sleep_ms(ms) rt_thread_mdelay(ms)

extern unsigned long sys_disable_irq(void);
extern void sys_enable_irq(unsigned long f);

char *strdup(const char *s);
#define print_lock()
#define print_unlock()


#ifdef HUGEIC_TXW901_DEBUG_LOG
#define PRINTF rt_kprintf
#define printk rt_kprintf
#else
#define PRINTF(...)
#define printk(...)
#endif


#ifndef simple_strtol
#define simple_strtol   strtol
#endif

#endif
