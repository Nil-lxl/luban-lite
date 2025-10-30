#include "atbm_os_timer.h"
#include "atbm_hal.h"


int  atbm_InitTimer(OS_TIMER *pTimer, TIMER_CALLBACK pCallback, void * CallRef)
{
    atbm_uint8 flag;

    flag = RT_TIMER_FLAG_DEACTIVATED;
    flag |= RT_TIMER_FLAG_ONE_SHOT;
    flag |=RT_TIMER_FLAG_SOFT_TIMER;
    pTimer->tm = rt_timer_create("AtbmTimer", pCallback, CallRef,10, flag);
    if(!pTimer->tm){
        ATBM_ERROR("Init timer err\n");
        return -1;
    }
    return 0;
}

int  atbm_StartTimer(OS_TIMER *pTimer, atbm_uint32 Interval)
{

    atbm_uint32 result;
    atbm_uint32 ticks= rt_tick_from_millisecond(Interval);
    //change time
    result=rt_timer_control(pTimer->tm, RT_TIMER_CTRL_SET_TIME, &ticks);
    if (result != RT_EOK){
        ATBM_ERROR("Set timer err\n");
        return -1;
    }
    //start time
    result = rt_timer_start(pTimer->tm);
    if (result != RT_EOK){
        ATBM_ERROR("Start timer err\n");
        return -1;
    }
    return 0;
}

void atbm_CancelTimer(OS_TIMER *pTimer)
{
    rt_timer_stop(pTimer->tm);
}

void atbm_FreeTimer(OS_TIMER *pTimer)
{
    rt_timer_delete(pTimer->tm);
}

static inline unsigned long os_ticks_to_msec(unsigned long ticks)
{
    return ((ticks) * 1000) / (RT_TICK_PER_SECOND);
}


atbm_uint64 atbm_GetOsTimeMs()
{
	return os_ticks_to_msec(rt_tick_get());
}

atbm_uint32 atbm_GetOsTime()
{
	return atbm_GetOsTimeMs();
}

ATBM_BOOL atbm_TimeAfter(atbm_uint32 tickMs, atbm_uint32 intvMs)
{
	atbm_uint32 current_time = atbm_GetOsTimeMs();
	if(current_time < tickMs){
		iot_printf("current_time < tickMs!!!\n");
	}
	return (((int)(tickMs + intvMs - current_time)) >= 0);
}

atbm_void atbm_wifi_ticks_timer_init(atbm_void)
{
}


atbm_void atbm_SleepMs(atbm_uint32 ms){
    rt_thread_mdelay(ms);
}
