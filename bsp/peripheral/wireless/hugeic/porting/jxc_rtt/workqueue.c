#include "os_porting.h"
#include <rthw.h>
#include <rtdef.h>
#include <rtthread.h>

typedef void (*task_pfunc)(void *);

int os_thread_create(uintptr_t *taskhdl,task_pfunc func,
                            char *name,void *param,
                            unsigned int stack_size,unsigned int task_prio)
{
    unsigned char taskname[32] = {0};
    rt_thread_t task = NULL;
    int idx = 0;
    int ret = 0;
    unsigned int prio = task_prio == 0 ? HIGH_TASK_PROI : NORMAL_TASK_PROI;

    snprintf(taskname, sizeof(taskname), "hg_%s", name);

    //ret = xTaskCreate(func,name,stack_size,(void *)param,prio,&task);
    task = rt_thread_create(taskname, func, (void *)param, stack_size, prio, 10);
    if(task == NULL) {
        hgic_err("Create task %s failed!\n",name);
        return -1;
    } else {
        if(taskhdl) {
            *taskhdl = (uintptr_t)task;
        }
		rt_thread_startup(task);
        return 0;
    }
}

void os_thread_delete(uintptr_t taskhdl)
{
    rt_thread_t *task = (rt_thread_t *)taskhdl;
    if(task == NULL)
        return;
    rt_thread_delete(task);
}

