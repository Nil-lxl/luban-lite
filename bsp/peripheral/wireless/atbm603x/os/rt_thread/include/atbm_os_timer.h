#ifndef ATBM_OS_TIMER_H
#define ATBM_OS_TIMER_H
#include "atbm_type.h"
#include <rtdef.h>

typedef atbm_void (*TIMER_CALLBACK)(atbm_void * CallRef);

typedef struct {
    rt_timer_t tm;
}OS_TIMER;

int atbm_InitTimer(OS_TIMER *pTimer, TIMER_CALLBACK pCallback, atbm_void * CallRef);
int atbm_StartTimer(OS_TIMER *pTimer, atbm_uint32 Interval/*ms*/);
void atbm_CancelTimer(OS_TIMER *pTimer);
void atbm_FreeTimer(OS_TIMER *pTimer);
atbm_uint32  atbm_GetOsTime(atbm_void);
ATBM_BOOL atbm_TimeAfter(atbm_uint32 tickMs, atbm_uint32 intvMs);
atbm_uint64 atbm_GetOsTimeMs();
atbm_void atbm_SleepMs(atbm_uint32 ms);
#define atbm_mdelay atbm_SleepMs
#endif /* ATBM_OS_TIMER_H */
