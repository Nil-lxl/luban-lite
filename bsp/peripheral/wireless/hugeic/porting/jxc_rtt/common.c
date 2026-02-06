#include "os_porting.h"
#include "os_defs.h"

extern long rt_hw_interrupt_disable(void);
extern void rt_hw_interrupt_enable(long level);

void os_assert(unsigned int arg, char *func, unsigned int line, char *caller)
{
    if (!arg) {
        printf("%s,%d:assert,caller:%p\n", func, line,caller);
        while (1);
    }
}

void os_printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void wifi_memory_copy(void *dst, void *src, unsigned long len)
{
    rt_memcpy(dst, src, len);
}

unsigned long sys_disable_irq(void)
{
    return rt_hw_interrupt_disable();
}

void sys_enable_irq(unsigned long f)
{
   rt_hw_interrupt_enable(f);
}

#if 1
/*
unsigned int sys_msecs_to_jiffies(unsigned long ms)
{
    unsigned short padding;
    unsigned long  ticks;

    padding = 1000 / OS_TICKS_PER_SECOND;
    padding = (padding > 0) ? (padding - 1) : 0;

    ticks = ((ms + padding) * OS_TICKS_PER_SECOND) / 1000;

    return ticks;
}
*/
unsigned int sys_jiffies_to_msecs(unsigned int ticks)
{
/*
    unsigned int padding;
    unsigned int time;

    if(OS_TICKS_PER_SECOND > 1000) {
        padding = OS_TICKS_PER_SECOND / 1000;
        padding = (padding > 0) ? (padding - 1) : 0;
    } else {
        padding = 0;
    }

    time = ((ticks + padding) * 1000) / OS_TICKS_PER_SECOND;
    return time;
*/
    if (ticks == 0)
        return 1;

    return ticks;
}
#endif


