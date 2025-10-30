
#ifndef _LINUX_TIMER_H_
#define _LINUX_TIMER_H_
#include <linux/jiffies.h>

#define TIMER_CPUMASK		0x0003FFFF
#define TIMER_MIGRATING		0x00040000
#define TIMER_BASEMASK		(TIMER_CPUMASK | TIMER_MIGRATING)
#define TIMER_DEFERRABLE	0x00080000
#define TIMER_PINNED		0x00100000
#define TIMER_IRQSAFE		0x00200000
#define TIMER_INIT_FLAGS	(TIMER_DEFERRABLE | TIMER_PINNED | TIMER_IRQSAFE)
#define TIMER_ARRAYSHIFT	22
#define TIMER_ARRAYMASK		0xFFC00000

//typedef void (*timer_func_t)(unsigned long data);

#if defined(__MBED__)
#include "os/mbed/timer.h"
#elif defined(__WINDOWS__)
#include "os/win/timer.h"
#elif defined(__LINUX__)
#include "os/linux/timer.h"
#elif defined(__CSKY__)
#include "os/c-sky/timer.h"
#else
//#include "porting/timer.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct hgic_timer {
    unsigned int timer;
    unsigned int index;
    unsigned int used;
    unsigned int tmr_magic;
    struct timer_list *timer_priv;
    struct mutex tmr_lock;
    void *timer_mgr;
};

struct timer_list {
    struct hgic_timer *tmr;
    void (*function)(unsigned long data);
    unsigned long data;
    unsigned long expires; //tick
    unsigned char pending;
    unsigned int  modifed_expires;
    unsigned int  vaild;
    unsigned int  enable;
    void *priv;
};

typedef void (*timer_func_t)(unsigned long data);

int os_timer_mgr_init(void);
void os_timer_mgr_delete(void);


#define timer_init os_timer_init
#define init_timer os_init_timer
#define setup_timer os_setup_timer
#define timer_setup os_timer_setup
#define add_timer os_add_timer
#define mod_timer os_mod_timer
#define stop_timer os_stop_timer
#define del_timer os_del_timer
#define del_timer_sync os_del_timer_sync

void os_timer_init(void);
int os_init_timer(struct timer_list *timer);
int os_setup_timer(struct timer_list *timer, void (*func)(unsigned long data), unsigned long data);
int os_timer_setup(struct timer_list *timer, void (*func)(struct timer_list *timer), unsigned int flags);
int os_add_timer(struct timer_list *timer);
int os_mod_timer(struct timer_list *timer, unsigned long expires);
int os_stop_timer(struct timer_list *timer);
int os_del_timer(struct timer_list *timer);
int os_del_timer_sync(struct timer_list *timer);

#define timer_pending(timer) (0)
#define init_timer_deferrable(timer) init_timer(timer)
#define do_posix_clock_monotonic_gettime(ts) get_sys_time(ts)
#define from_timer(var, callback_timer, timer_fieldname) \
        container_of(callback_timer, typeof(*var), timer_fieldname)
#define ktime_get_boottime_ns() (jiffies)

static inline u32 ktime_get_seconds()
{
    struct timespec tm;
    get_sys_time(&tm);
    return tm.tv_sec;
}

#ifdef __cplusplus
}
#endif
#endif

