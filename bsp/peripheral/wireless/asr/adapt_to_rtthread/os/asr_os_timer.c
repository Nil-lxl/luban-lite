#include "asr_rtos_api.h"
#include "asr_dbg.h"
//#include "rtconfig"

#define os_ticks_to_msec(x) asr_os_ticks_to_msec(x)
#define os_msec_to_ticks(x) asr_os_msec_to_ticks(x)
#define portTICK_PERIOD_MS          ( 1000 / get_rt_tick_per_second() )
#define portMAX_DELAY               (uint32_t)0xffffffff


uint32_t get_rt_tick_per_second(void)
{
    return RT_TICK_PER_SECOND;
}

uint32_t asr_rtos_get_time(void)
{
	unsigned int tick,time;
	tick= rt_tick_get();
	time=os_ticks_to_msec(tick);
	return time;
}

uint32_t asr_rtos_get_ticks(void)
{
    unsigned int time = rt_tick_get();
    return time;
}

uint32_t asr_rtos_msec_to_ticks(uint32_t msecs)
{
    uint32_t msRet;

    if (msecs == ASR_WAIT_FOREVER)
        msRet = portMAX_DELAY;
    else
        msRet = msecs / portTICK_PERIOD_MS;
    return msRet;
}

uint32_t asr_rtos_ticks_to_msec(uint32_t ticks)
{
    uint32_t msRet;
    msRet = ticks * portTICK_PERIOD_MS;
    return msRet;
}

static void asr_timer_function(void *timer_arg)
{
    asr_timer_t* asr_timer = (asr_timer_t*)timer_arg;

    // dbg(D_ERR, D_UWIFI_CTRL, "%s:timer=0x%X,asr_timer=0x%X,function=0x%X,arg=0x%X",
    //    __func__, timer, asr_timer, asr_timer->function, asr_timer->arg);

    if (asr_timer && asr_timer->function) {
        asr_timer->function(asr_timer->arg);
    }
}

OSStatus asr_rtos_init_timer( asr_timer_t* timer, uint32_t time_ms, timer_handler_t function, void* arg )
{
    rt_timer_t handle;
    int flag;
    flag  = RT_TIMER_FLAG_DEACTIVATED;
    flag |= RT_TIMER_CTRL_SET_PERIODIC;
	flag |= RT_TIMER_FLAG_SOFT_TIMER;

    if (timer == NULL || function == NULL || time_ms == 0 || timer->handle != NULL) {
        return kGeneralErr;
    }


    memset(timer, 0, sizeof(asr_timer_t));

    timer->arg = arg;
    timer->function = function;
    timer->inittime = time_ms;
    timer->repeat = 1;

    handle = rt_timer_create("asr_timer", asr_timer_function, timer,
                             os_msec_to_ticks(time_ms), flag);
    if(handle == NULL) {
        return kGeneralErr;
    }

    timer->handle = handle;

    //dbg(D_ERR, D_UWIFI_CTRL, "%s:timer=0x%X,asr_timer=0x%X,function=0x%X,arg=0x%X",
    //    __func__, *(aos_timer_t*)(timer->handle), timer, timer->function, timer->arg);

    return kNoErr;
}

OSStatus asr_rtos_start_timer( asr_timer_t* timer )
{
    int ret;

    if (timer == NULL || timer->handle == NULL) {
        return kGeneralErr;
    }

    ret = rt_timer_start(timer->handle);
    if(ret != RT_EOK) {
        return kGeneralErr;
    }

    return kNoErr;
}

OSStatus asr_rtos_stop_timer( asr_timer_t* timer )
{
    int ret;

    if (timer == NULL || timer->handle == NULL) {
        return kGeneralErr;
    }

    ret = rt_timer_stop(timer->handle);
    if(ret != RT_EOK) {
        return kGeneralErr;
    }

    return kNoErr;
}

OSStatus asr_rtos_reload_timer( asr_timer_t* timer )
{
    int ret;

    if (timer == NULL || timer->handle == NULL) {
        return kGeneralErr;
    }

    ret = rt_timer_start(timer->handle);
    if(ret != RT_EOK) {
        return kGeneralErr;
    }

    return kNoErr;
}

OSStatus asr_rtos_deinit_timer( asr_timer_t* timer )
{
    if (timer == NULL || timer->handle == NULL) {
        return kGeneralErr;
    }

    rt_timer_stop(timer->handle);
    rt_timer_delete(timer->handle);

    timer->handle = NULL;
    return kNoErr;
}

OSBool asr_rtos_is_timer_running( asr_timer_t* timer )
{
    if (timer == NULL || timer->handle == NULL) {
        return kFALSE;
    }

    int sta = 0;
    rt_timer_control(timer->handle, RT_TIMER_CTRL_GET_STATE, &sta);

    return (sta == RT_TIMER_FLAG_ACTIVATED);
}

OSStatus asr_rtos_update_timer( asr_timer_t* timer, uint32_t time_ms )
{
    int ret;
    int tick = os_msec_to_ticks(time_ms);

    if (timer == NULL || timer->handle == NULL) {
        return kGeneralErr;
    }

    ret = rt_timer_stop(timer->handle);
    if(ret != RT_EOK) {
        return kGeneralErr;
    }

    ret = rt_timer_control(timer->handle, RT_TIMER_CTRL_SET_TIME, &tick);
    if(ret != RT_EOK) {
        return kGeneralErr;
    }

    return kNoErr;
}

void asr_msleep(uint32_t time_ms)
{
    rt_thread_mdelay(time_ms);
}
