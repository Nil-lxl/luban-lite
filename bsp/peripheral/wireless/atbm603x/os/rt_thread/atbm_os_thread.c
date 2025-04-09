#include "atbm_hal.h"
#include "atbm_os_thread.h"
#include <string.h>

#define WIFI_STACK_SIZE 2048
#define WIFI_STACK_TICK 5

pAtbm_thread_t atbm_createThread(char* name,atbm_void(*task)(atbm_void *p_arg),atbm_void *p_arg,int prio)
{
    pAtbm_thread_t th;
    th = rt_thread_create(name, task,p_arg, WIFI_STACK_SIZE, prio, WIFI_STACK_TICK);
    if (!th){
        iot_printf("thread init fail!!!\n");
        ATBM_BUG_ON(1);
    }
    rt_thread_startup(th);
    return th;
}

int atbm_stopThread(pAtbm_thread_t thread_id)
{	
	if(rt_thread_delete(thread_id)!=RT_EOK){
	    return -1;
	}
	return 0;
}

int atbm_ThreadStopEvent(pAtbm_thread_t thread_id)
{
    return 0;
}

