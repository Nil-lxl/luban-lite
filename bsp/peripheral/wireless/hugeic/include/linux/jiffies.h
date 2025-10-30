#ifndef _LINUX_JIFFIES_H_
#define _LINUX_JIFFIES_H_

#if defined(__MBED__)
#include "os/mbed/jiffies.h"
#elif defined(__WINDOWS__)
#include "os/win/jiffies.h"
#elif defined(__LINUX__)
#include "os/linux/jiffies.h"
#elif defined(__CSKY__)
#include "os/c-sky/jiffies.h"
#else
//#include "porting/jiffies.h"
#include <linux/defs.h>
#endif

#define MSEC_PER_SEC    1000L
#define USEC_PER_MSEC   1000L   
#define NSEC_PER_USEC   1000L
#define NSEC_PER_MSEC   1000000L
#define USEC_PER_SEC    1000000L
#define NSEC_PER_SEC    1000000000L
#define FSEC_PER_SEC    1000000000000000LL

#define time_after(a,b)  ((long)(b) - (long)(a) < 0)
#define time_before(a,b)    time_after((b),(a))
#define time_is_before_jiffies(a) time_after((jiffies), (a))
#define time_is_after_jiffies(a) time_before((jiffies), (a))
#define get_sys_time os_systime

#define time_after_eq(a,b)  ((long)(a) - (long)(b) >= 0)
#define time_before_eq(a,b) time_after_eq((b),(a))

#define ktime_get_real()   (jiffies)
#define ktime_to_us(k)     ((k)*(USEC_PER_SEC/HZ))
#define ktime_to_ms(k)     ((k)*(MSEC_PER_SEC/HZ))

#define os_sleep(s) os_sleep_ms(((s)*HZ))
#define sleep_ms(ms) os_sleep_ms((ms))
#define msleep(ms) os_sleep_ms((ms))
#define udelay(u) os_sleep_ms(1)
#define mdelay(ms) os_sleep_ms((ms))  

struct timespec {
	long	tv_sec;		/* seconds */
	long	tv_nsec;	/* and nanoseconds */
};

struct tm {
    int tm_sec;      // seconds after the minute - [0..61] 
                     //   (note 61 allows for two leap seconds)
    int tm_min;      // minutes after the hour - [0..59]
    int tm_hour;     // hours since midnight - [0..23]
    int tm_mday;     // days of the month - [1..31]
    int tm_mon;      // months since January - [0..11]
    int tm_year;     // years since 1900
    int tm_wday;     // days since Sunday - [0..6]
    int tm_yday;     // days since January 1 - [0..365]
    int tm_isdst;    // Daylight Saving Time flag - positive if DST is in
                     // effect, 0 if not in effect, and negative if the info
                     // is not available
};

static inline void os_systime(struct timespec *tm)
{
    unsigned int sec  = 0;
    unsigned int msec = 0;

    if (tm != 0) {
        sec  = jiffies / HZ;
        msec = jiffies % HZ;
        tm->tv_sec  = sec;
        tm->tv_nsec = msec * 1000 * 1000;
    }
}

static inline unsigned int usecs_to_jiffies(unsigned int usec)
{
    unsigned int jiff = (usec * HZ) / USEC_PER_SEC;
    return jiff ? jiff : 1;
}


unsigned long round_jiffies_up(unsigned long j);
unsigned long round_jiffies(unsigned long j);
unsigned long round_jiffies_relative(unsigned long j);
#endif
