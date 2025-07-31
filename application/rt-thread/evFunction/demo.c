#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"
#include "aic_core.h"

static rt_sem_t semaphore;
static rt_thread_t send_thread;
static rt_thread_t get_thread;

static int num1 = 0;
static int num2 = 0;

void send_thread_entry(void *param) {
    while (1) {
        rt_sem_take(semaphore, RT_WAITING_FOREVER);
        num1 += 2;
        LOG_I("num1:%d", num1);
        
        rt_thread_mdelay(1000);

        rt_sem_release(semaphore);

    }
}
void get_thread_entry(void *param) {
    while (1) {
        rt_sem_take(semaphore, RT_WAITING_FOREVER);
        // num2+=1;
        if (num1 == 6) {
            LOG_W("get num1:%d", num1);

        }

        rt_sem_release(semaphore);
        rt_thread_mdelay(1000);

    }
}

void demo_startup() {
    semaphore = rt_sem_create("semaphore", 1, RT_IPC_FLAG_FIFO);
    if (semaphore == RT_NULL) {
        LOG_E("semaphore create failed!");
        abort();
    }

    send_thread = rt_thread_create("demo_send", send_thread_entry, RT_NULL, 1024, 18, 10);
    if (send_thread != RT_NULL)
        rt_thread_startup(send_thread);

    get_thread = rt_thread_create("demo_get", get_thread_entry, RT_NULL, 1024, 18, 10);
    if (get_thread != RT_NULL)
        rt_thread_startup(get_thread);

}
// INIT_APP_EXPORT(demo_startup);
