#ifndef OS_THREAD_H_
#define OS_THREAD_H_

#include <rtthread.h>
#include <zephyr/sys/dlist.h>

//struct k_sem {
//    rt_sem_t *sem;
//    int count;
//    int limit;
//    sys_dlist_t poll_events;
//};

typedef void (*TaskFunction_t)( void * );
typedef TaskFunction_t RT_ThreadEntry_t;

typedef struct RT_Thread {
    rt_thread_t handle;
} RT_Thread_t;

int RT_ThreadCreate(RT_Thread_t *thread, const char *name,
                    RT_ThreadEntry_t entry, void *arg,
                    uint8_t priority, uint32_t stackSize);
#endif
