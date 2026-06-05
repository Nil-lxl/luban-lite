#include <aic_core.h>
#include <errno.h>
#include <rtthread.h>
#include <zephyr/kernel.h>
#include <ipc/workqueue.h>
#include "os_thread.h"


static void k_wrap_thread(void * arg)
{
	struct k_thread *this_thread = (struct k_thread *)arg;

    //printf("%s, %d\n", __func__, __LINE__);
	this_thread->entry(this_thread->p1, this_thread->p2, this_thread->p3);

#if defined(BT_DEINIT)
	this_thread->entry = NULL;
	this_thread->p1 = NULL;
	this_thread->p2 = NULL;
	this_thread->p3 = NULL;
#endif

#if defined(CONFIG_BT_DEBUG_RESOURCES_USE)
	ble_thread_cnt--;
#endif

    //OS_ThreadDelete(&this_thread->task);
}

k_tid_t k_thread_create(struct k_thread *new_thread,
				  k_thread_stack_t *stack,
				  size_t stack_size,
				  k_thread_entry_t entry,
				  void *p1, void *p2, void *p3,
				  int prio, uint32_t options, k_timeout_t delay)
{
    int ret;
    new_thread->entry = entry;
    new_thread->p1 = p1;
    new_thread->p2 = p2;
    new_thread->p3 = p3;

	/*
	 * (4 * stack_size) is compatative to the old k_thread_create
     * by tw, this should be optimized
	 */
    ret = RT_ThreadCreate(&new_thread->task, "Zephyr",
                          k_wrap_thread, new_thread,
    					  prio, (8 * stack_size));
    if (ret) {
        pr_err("create ble task fail\n");
		return NULL;
    }

#if defined(CONFIG_BT_DEBUG_RESOURCES_USE)
    ble_thread_cnt++;
#endif

    return new_thread;
}

void k_thread_abort(k_tid_t thread)
{

}

int k_thread_name_set(k_tid_t thread, const char *str)
{
    return 0;
}




int k_sem_init(struct k_sem *sem, unsigned int initial_count,
			  unsigned int limit)
{
    //int ret = rt_sem_init(&sem->sem, "os_sem", initial_count, RT_IPC_FLAG_PRIO);
    if (sem->sem == NULL) {
        sem->sem = rt_sem_create("os_sem", initial_count, RT_IPC_FLAG_PRIO);
        //printf("new sem:%p, name:%s\n", sem->sem, "os_sem");
    }

    //printf("init sem:%p, name:%s\n", sem->sem, "os_sem");

    sys_dlist_init(&sem->poll_events);

    return 0;
}

int k_sem_take(struct k_sem *sem, k_timeout_t timeout)
{
    int ret = 0;
    unsigned int t = timeout;

    //if (timeout == K_FOREVER) {
    //    t = 0xFFFF;
    //} else if (timeout == K_NO_WAIT)
    //{
    //    t = 0;
    //}

    //printf("take sem:%p, name:%s\n", sem->sem, "os_sem");
    ret = rt_sem_take(sem->sem, t);
    if (ret != RT_EOK) {
        return -RT_ETIMEOUT;
    }

    return ret;
}

void k_sem_give(struct k_sem *sem)
{
    //int ret = 0;

    //ret = rt_sem_release(sem->sem);
    //if (ret != RT_EOK) {
    //    return -RT_ERROR;
    //}

    //return ret;
    //printf("give sem:%p, name:%s\n", sem->sem, "os_sem");
    rt_sem_release(sem->sem);
}

//int k_sem_detach(struct k_sem *sem)
//{
//    return rt_sem_detach(&sem->sem);
//}

unsigned int k_sem_count_get(struct k_sem *sem)
{
    return sem->sem->value;
}










//void k_queue_init(struct k_queue *queue)
//{
//    sys_slist_init(&queue->data_q);
//    sys_dlist_init(&queue->poll_events);
//
//    //k_sem_init(&queue->sem, 0, 10);
//}
//
//void *k_queue_get(struct k_queue *queue, k_timeout_t timeout)
//{
//    void *buf = NULL;
//    unsigned int key;
//    int ret;
//
//    //ret = k_sem_take(&queue->sem, timeout);
//    //if (ret < 0) {
//    //    return NULL;
//    //}
//
//    key = irq_lock();
//    buf = sys_slist_get(&queue->data_q);
//    irq_unlock(key);
//
//    return buf;
//}
//
//int k_queue_is_empty(struct k_queue *queue)
//{
//    return (int)sys_slist_is_empty(&queue->data_q);
//}
//
//void k_queue_prepend(struct k_queue *queue, void *data)
//{
//    unsigned int key;
//
//    key = irq_lock();
//    sys_slist_prepend(&queue->data_q, data);
//    irq_unlock(key);
//
//    //k_sem_give(&queue->sem);
//}
//
//void k_queue_append(struct k_queue *queue, void *data)
//{
//    unsigned int key;
//
//    key = irq_lock();
//    sys_slist_append(&queue->data_q, data);
//    irq_unlock(key);
//
//    //k_sem_give(&queue->sem);
//}
//
//void *k_queue_peek_head(struct k_queue *queue)
//{
//    unsigned int key;
//
//    key = irq_lock();
//    sys_slist_peek_head(&queue->data_q);
//    irq_unlock(key);
//
//    //k_sem_give(&queue->sem);
//}

void k_sched_lock(void)
{
    rt_enter_critical();
}

void k_sched_unlock(void)
{
    rt_exit_critical();
}


int k_mutex_init(struct k_mutex *mutex)
{
    if (mutex->mutex == NULL) {
        mutex->mutex = rt_mutex_create("os_mutex", RT_IPC_FLAG_PRIO);
        //printf("new mutex:%p, name:%s\n", mutex->mutex, "os_mutex");
    }
    //printf("init mutex:%p, name:%s\n", mutex->mutex, "os_mutex");

    return 0;
}

int k_mutex_lock(struct k_mutex *mutex, k_timeout_t timeout)
{
    return rt_mutex_take(mutex->mutex, timeout);
}

int k_mutex_unlock(struct k_mutex *mutex)
{
    return rt_mutex_release(mutex->mutex);
}
