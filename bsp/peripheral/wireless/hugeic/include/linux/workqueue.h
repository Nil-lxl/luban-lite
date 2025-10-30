#ifndef _LINUX_WORKQUEUE_H_
#define _LINUX_WORKQUEUE_H_
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/semaphore.h>

#if defined(__MBED__)
#include "os/mbed/workqueue.h"
#elif defined(__WINDOWS__)
#include "os/win/workqueue.h"
#elif defined(__LINUX__)
#include "os/linux/workqueue.h"
#elif defined(__CSKY__)
#include "os/c-sky/workqueue.h"
#else
//#include "porting/workqueue.h"
#endif

void tasklet_entry(void *arg);
typedef void (*tasklet_func_t)(unsigned long data);

struct tasklet_struct {
    unsigned int task;
    char  *name;
    void  *stack;
    tasklet_func_t   func;
    unsigned long    args;
    struct semaphore  sem;
    char   enable: 1,
           stop  : 1,
           running: 1;
    unsigned char scheduled;
    struct semaphore  rcu_sem;
    struct rculock   *rcu_locks[8];
    int    rculock_id;
    unsigned int magic;
};

typedef void (*work_func_t)(struct work_struct *work);

struct work_struct {
    struct list_head list;
    work_func_t      func;
    u32              scheduled: 1;
};

struct workqueue_struct {
    struct mutex          lock;
    struct tasklet_struct tasklet;
    struct list_head      works;
    void *running_work;
};

struct delayed_work {
    struct workqueue_struct *wq;
    struct mutex       lock;
    struct timer_list  timer;
    struct work_struct work;
    u32    pending: 1;
};

#define DECLARE_WORK(work, fun) struct work_struct work = { .func = fun }
#define DECLARE_DELAYED_WORK(work, fun) struct delayed_work work

#define INIT_WORK(wk, fun) do{ \
        memset(wk, 0, sizeof(struct work_struct ));\
        (wk)->func = fun; \
    } while(0)

#define INIT_DELAYED_WORK(wk, fun) do{\
        memset(wk, 0, sizeof(struct delayed_work));\
        setup_timer(&(wk)->timer, delayed_work_timer_cb, (unsigned long)wk);\
        mutex_init(&((wk)->lock));\
        INIT_WORK(&((wk)->work), fun);\
    }while(0)

#define work_busy(work) (!!((work)->scheduled))

#define create_singlethread_workqueue(name) alloc_ordered_workqueue(name, 2048)

static inline struct delayed_work *to_delayed_work(struct work_struct *work)
{
	return container_of(work, struct delayed_work, work);
}

#define from_tasklet(var, callback_tasklet, tasklet_fieldname)	\
	container_of(callback_tasklet, typeof(*var), tasklet_fieldname)

#define tasklet_setup(task, func) tasklet_create((task), (tasklet_func_t)(func), (0), (4096))
#define cancel_delayed_work os_cancel_delayed_work
#define schedule_delayed_work os_schedule_delayed_work

void rtnl_init(void);
void rtnl_lock(void);
void rtnl_unlock(void);
u8 lockdep_rtnl_is_held();

void tasklet_init(struct tasklet_struct *t, tasklet_func_t func, unsigned long data);
void tasklet_create(struct tasklet_struct *t, tasklet_func_t func,
                     unsigned long data, int stack_size);
void tasklet_destory(struct tasklet_struct *t);
void tasklet_schedule(struct tasklet_struct *tasklet);
struct workqueue_struct *alloc_ordered_workqueue(char *name, int flags);
void queue_work(struct workqueue_struct *wq, struct work_struct *work);
void delayed_work_timer_cb(unsigned long data);
void queue_delayed_work(struct workqueue_struct *wq, struct delayed_work *work, u32 delay);
int  cancel_work_sync(struct work_struct *work);
void cancel_delayed_work_sync(struct delayed_work *work);
void cancel_delayed_work(struct delayed_work *work);
void schedule_work(struct work_struct *work);
void schedule_delayed_work(struct delayed_work *work, uint32_t delay);
void tasklet_disable(struct tasklet_struct *tasklet);
void tasklet_enable(struct tasklet_struct *tasklet);
void tasklet_kill(struct tasklet_struct *tasklet);
void flush_delayed_work(struct delayed_work *work);
void destroy_delayed_work(struct delayed_work *work);
void flush_workqueue(struct workqueue_struct *workq);
void destroy_workqueue(struct workqueue_struct *workq);
void flush_work(struct work_struct *work);
void tasklet_core_init(void);
void tasklet_core_exit(void);
void tasklet_entry(void *arg);
bool mod_delayed_work(struct workqueue_struct *wq, struct delayed_work *dwork, unsigned long delay);

#endif

