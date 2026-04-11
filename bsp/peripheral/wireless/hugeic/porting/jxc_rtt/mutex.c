#include "os_porting.h"
#include <rtthread.h>

int os_mutex_new(uintptr_t *mutex_handle)
{
    rt_mutex_t mutex = NULL;
    int ret = 0;
        static unsigned char index = 0;
    char mutex_name[16] = {0};

    snprintf(mutex_name, sizeof(mutex_name), "hgm_%d", index);
    index ++;

    mutex = rt_mutex_create(mutex_name, RT_IPC_FLAG_FIFO);
    if (mutex == NULL) {
        hgic_err("Create mutex failed!\n");
        return -1;
    } else {
        if (mutex_handle) {
            *mutex_handle = (uintptr_t)mutex;
        }
        return 0;
    }
}

int os_mutex_take(uintptr_t mutex_handle)
{
    rt_mutex_t  mutex = (rt_mutex_t)mutex_handle;
    int ret = 0;

    if (mutex == NULL) {
        hgic_err("Input param error!\n");
        return -1;
    }
    return rt_mutex_take(mutex, RT_WAITING_FOREVER);
}

void os_mutex_release(uintptr_t mutex_handle)
{
    rt_mutex_t  mutex = (rt_mutex_t)mutex_handle;
    int ret = 0;

    if (mutex == NULL) {
        hgic_err("Input param error!\n");
        return;
    }
    rt_mutex_release(mutex);
}

void os_mutex_delete(uintptr_t mutex_handle)
{
    rt_mutex_t  mutex = (rt_mutex_t)mutex_handle;
    if (mutex == NULL) {
        hgic_err("Input param error!\n");
        return;
    }
    rt_mutex_delete(mutex);
}
