#ifndef _CPU_LOAD_H_
#define _CPU_LOAD_H_

#include <rtthread.h>
#include <stdio.h>
#include <string.h>
#include <rtdevice.h>

#define CPU_LOAD_MONITOR

typedef struct
{
    struct rt_thread *thread;
    uint64_t count;
    uint64_t base;
}task_monitor_t;

extern uint64_t aic_get_time_us64(void);
void thread_init_hook(rt_thread_t thread);
void sched_hook(struct rt_thread* from, struct rt_thread* to);

#endif