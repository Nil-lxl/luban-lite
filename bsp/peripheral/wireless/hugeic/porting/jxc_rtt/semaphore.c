#include "os_porting.h"
#include <rtthread.h> 

int os_sem_new(uintptr_t *sem_hdl,int val)
{
    rt_sem_t sem = NULL;
    static unsigned char index = 0;
    char sem_name[16] = {0};
    
    snprintf(sem_name, sizeof(sem_name), "hgs_%d", index);
    index ++;
    sem = rt_sem_create(sem_name, 0, RT_IPC_FLAG_FIFO);
    if(sem == NULL) {
        hgic_err("Create semaphore error!\n");
        return -1;
    } else {
        if(sem_hdl) {
            *sem_hdl = (uintptr_t)sem;
        }
        return 0;
    }
}

void os_sem_free(uintptr_t sem_hdl)
{
    rt_sem_t sem = (rt_sem_t)sem_hdl;
    if(sem == NULL) {
        hgic_err("Input param error!\n");
        return;
    }
    rt_sem_delete(sem);
}

int os_sem_take(uintptr_t sem_hdl,unsigned int timeout)
{
    rt_sem_t sem = (rt_sem_t)sem_hdl;
    int ret = 0;
    
    if(sem == NULL) {
        hgic_err("Input param error!\n");
        return -1;
    }
    if(timeout == osWaitForever) {        
        return rt_sem_take(sem, RT_WAITING_FOREVER);
    } else {
        return rt_sem_take(sem, rt_tick_from_millisecond(timeout));
    }
}

void os_sem_release(uintptr_t sem_hdl)
{
    rt_sem_t sem = (rt_sem_t)sem_hdl;
    int ret = 0;
    
    if(sem == NULL) {
        hgic_err("Input param error!\n");
        return;
    }    
    rt_sem_release(sem);
}

