#include <linux/types.h>
#include <linux/defs.h>
#include <linux/kernel.h>
#include <linux/bitops.h>
#include <linux/atomic.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
//#include "string.h"

#define TASKLET_DEF_STACK_SIZE (2048)

struct workqueue_mgr {
    struct mutex rtnl_mutex;
    struct mutex bh_lock;
    struct workqueue_struct *main_wq;
}wkq_mgr;

void tasklet_entry(void *arg)
{
    int ret = 0;
    struct tasklet_struct *task = (struct tasklet_struct *)arg;

    task->running = 1;
    while (!task->stop) {
        sema_down(&task->sem);
        //PRINTF("%s:Enter,task func:%p,task_stop:%d\n",__FUNCTION__,task->func,task->stop);
        if (task->scheduled > 0) {
            task->scheduled--;
        }
        if (task->enable) {
            task->func(task->args ? task->args : (unsigned long)task);
        }
        //PRINTF("%s:Leave,task func:%p,task_stop:%d\n",__FUNCTION__,task->func,task->stop);
    }
    task->running = 0;
}

static void workqueue_task(struct workqueue_struct *wq)
{
    struct work_struct *work = NULL;
    struct work_struct *next = NULL;
    unsigned int irq = 0;
    //mutex_lock(&wq->lock);
    irq = sys_disable_irq();
    list_for_each_entry_safe(work, next, &wq->works, list) {
        list_del(&work->list);
        work->scheduled = 0;
        wq->running_work = work->func;
        //mutex_unlock(&wq->lock);
        sys_enable_irq(irq);
        //PRINTF("%p enter\n",work->func);
        work->func(work);
        //PRINTF("%p leave\n",work->func);        
        //mutex_lock(&wq->lock);        
        irq = sys_disable_irq();
        wq->running_work = NULL;
    }
    //mutex_unlock(&wq->lock);
    sys_enable_irq(irq);
}

void tasklet_schedule(struct tasklet_struct *tasklet)
{
    if (tasklet->scheduled < 4096) { //avoid to release too match semaphore
        tasklet->scheduled++;
        sema_up(&tasklet->sem);
    } else {
        PRINTF("too much scheduled:%d,task_name:%s\n",tasklet->scheduled,tasklet->name);
    }
}

struct workqueue_struct *alloc_ordered_workqueue(char *name, int flags)
{
    struct workqueue_struct *wq = MALLOC(sizeof(struct workqueue_struct));
    if (wq) {
        memset(wq, 0, sizeof(struct workqueue_struct));
        wq->tasklet.name = name;
        INIT_LIST_HEAD(&wq->works);
        mutex_init(&wq->lock);
        tasklet_create(&wq->tasklet, (tasklet_func_t)workqueue_task, 
                (unsigned long)wq, flags > 4096 ? flags : 4096);
    }
    return wq;
}

void queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
    unsigned int irq = 0;
    if(wq == NULL || work == NULL) {
        PRINTF("%s:Invaild param:%p,%p\n",__FUNCTION__,wq,work);
        return;
    }
    irq = sys_disable_irq();
    //PRINTF("%s:Enter,work func:%p\n",__FUNCTION__,work->func);
    //mutex_lock(&wq->lock);
    if (!work->scheduled) {
        list_add_tail(&work->list, &wq->works);
        work->scheduled = 1;
    } 
    sys_enable_irq(irq);
    //mutex_unlock(&wq->lock);
    tasklet_schedule(&wq->tasklet);
    //PRINTF("%s:Leave,work func:%p\n",__FUNCTION__,work->func);
}

void delayed_work_timer_cb(unsigned long data)
{
    struct delayed_work *work = (struct delayed_work *)data;
    mutex_lock(&work->lock);
    if (work->pending) {
        work->pending = 0;
        queue_work(work->wq, &work->work);
    }
    mutex_unlock(&work->lock);
}

void queue_delayed_work(struct workqueue_struct *wq, struct delayed_work *work, u32 delay)
{
    mutex_lock(&work->lock);
    work->wq = wq;
    if (delay > 0) {
        work->pending = 1;
        mod_timer(&work->timer, delay + jiffies);
    } else {
        work->pending = 0;
        queue_work(work->wq, &work->work);
    }
    mutex_unlock(&work->lock);
}

int cancel_work_sync(struct work_struct *work)
{
    while (work->scheduled) {
        PRINTF("%s:Wait for cancel delay work,func:%p\n",work->func);
        sleep_ms(5);
    }
    return 0;
}

void cancel_delayed_work(struct delayed_work *work)
{
    mutex_lock(&work->lock);
    stop_timer(&work->timer);
    work->pending = 0;
    mutex_unlock(&work->lock);
}

void cancel_delayed_work_sync(struct delayed_work *work)
{
    cancel_delayed_work(work);
    while (work->work.scheduled) {
        sleep_ms(5);
    }
}

void schedule_work(struct work_struct *work)
{
    queue_work(wkq_mgr.main_wq, work);
}

void schedule_delayed_work(struct delayed_work *work, uint32_t delay)
{
    queue_delayed_work(wkq_mgr.main_wq, work, delay);
}

bool mod_delayed_work(struct workqueue_struct *wq, struct delayed_work *dwork, unsigned long delay)
{
    cancel_delayed_work_sync(dwork);
    queue_delayed_work(wq, dwork, delay);
	return true;
}

void flush_work(struct work_struct *work)
{
    while (work->scheduled) {
        sleep_ms(5);
    }
}

void flush_delayed_work(struct delayed_work *work)
{
    cancel_delayed_work_sync(work);
}

void flush_workqueue(struct workqueue_struct *workq)
{
    while (!list_empty(&workq->works)) {
        sleep_ms(5);
    }
}

void destroy_workqueue(struct workqueue_struct *workq)
{
    tasklet_kill(&workq->tasklet);
    tasklet_destory(&workq->tasklet);
    mutex_destroy(&workq->lock);
    FREE(workq);
}

void destroy_delayed_work(struct delayed_work *work)
{
    while (work->work.scheduled || work->pending) {
        sleep_ms(5);
    }
    del_timer(&(work->timer));
    mutex_destroy(&(work->lock));
}

void tasklet_disable(struct tasklet_struct *tasklet)
{
    tasklet->enable = 0;
}

void tasklet_enable(struct tasklet_struct *tasklet)
{
    tasklet->enable = 1;
}

void tasklet_kill(struct tasklet_struct *tasklet)
{
    tasklet->stop = 1;
    tasklet->enable = 0;
    sema_up(&tasklet->sem);
    while (tasklet->running) {
        sleep_ms(5);
    }
}

void tasklet_core_init(void)
{
    MEMSET(&wkq_mgr, 0, sizeof(struct workqueue_mgr));
    mutex_init(&wkq_mgr.rtnl_mutex);
    mutex_init(&wkq_mgr.bh_lock);
    wkq_mgr.main_wq = alloc_ordered_workqueue("main_wq", 4096);
    ASSERT(wkq_mgr.main_wq);
    
}

void tasklet_core_exit(void)
{
    destroy_workqueue(wkq_mgr.main_wq);
    mutex_destroy(&wkq_mgr.rtnl_mutex);
}

void rtnl_lock(void)
{
    mutex_lock(&wkq_mgr.rtnl_mutex);
}

void rtnl_unlock(void)
{
    mutex_unlock(&wkq_mgr.rtnl_mutex);
}

void local_bh_disable(void)
{
    mutex_lock(&wkq_mgr.bh_lock);
}

void local_bh_enable(void)
{
    mutex_unlock(&wkq_mgr.bh_lock);
}

u8 lockdep_rtnl_is_held()
{
    return wkq_mgr.rtnl_mutex.locked;
}

void tasklet_init(struct tasklet_struct *t, tasklet_func_t func, unsigned long data)
{
    tasklet_create(t, func, data, TASKLET_DEF_STACK_SIZE);
}

