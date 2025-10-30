#include <linux/types.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/rcu.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>

#if 0
#define RCULOCK_COUNT      (16)

struct rculock {
    struct mutex mutex;
    u32 lr_val;
    u32 locked;
    u32 wait_task;
};

struct rculock_mgr {
    struct mutex mutex;
    struct rculock locks[RCULOCK_COUNT];//rculock for all tasks
};

struct rculock_task {
    unsigned int task_id;
    int rculock_id; //array index of rculock_task->rcu_lock
    struct rculock *rcu_locks[RCULOCK_COUNT];//
};

static struct rculock_task _rcu_tasks[RCULOCK_COUNT];
static struct rculock_mgr _rculock_mgr;

static struct rculock *__request_rculock()
{
    int i = 0;
    struct rculock *lock = NULL;

    for (i = 0; i < RCULOCK_COUNT; i++) {
        if (_rculock_mgr.locks[i].lr_val == 0) {
            lock = &_rculock_mgr.locks[i];
            break;
        }
    }
    return lock;
}

static void __release_rculock(struct rculock *lock)
{
    ASSERT(lock);
    lock->lr_val = 0;
    lock->locked = 0;
}

static struct rculock_task * __rcu_tasklock_request(void)
{
    int i = 0;
    struct rculock_task *task = NULL; 

    for (i = 0; i < RCULOCK_COUNT; i++) {
        if (_rcu_tasks[i].task_id == 0) {
            _rcu_tasks[i].rculock_id = -1;
            task = &_rcu_tasks[i];
        }
    }
    return task;
}

static int __rcu_tasklock_release(struct rculock_task *prcu)
{
    ASSERT(prcu);
    prcu->task_id = 0;
    prcu->rculock_id = -1;
}

static struct rculock *lr_to_rculock(u32 code_addr)
{
    int i = 0;
    struct rculock *lock = NULL;

    for (i = 0; i < RCULOCK_COUNT; i++) {
        if (_rculock_mgr.locks[i].lr_val == code_addr) {
            lock = &_rculock_mgr.locks[i];
            break;
        }
    }
    return lock;
}

static struct rculock_task *taskid_to_taskrcu(u32 taskid)
{
    int i = 0;
    struct rculock_task *prcu = NULL;

    for (i = 0; i < RCULOCK_COUNT; i++) {
        if (_rcu_tasks[i].task_id == taskid) {
            prcu = &_rcu_tasks[i];
            break;
        }
    }
    return prcu;
}

void _rcu_read_lock()
{
#if 0
    //struct tasklet_struct *cur_task = get_current();
    unsigned int task_id = sys_get_current_taskid();
    unsigned int lrReg = RETURN_ADDR();
    struct rculock *lock = NULL;
    struct rculock_task *prcu = NULL;
    int    wait = 0;

    ASSERT(lrReg);
    lock = lr_to_rculock(lrReg);

__WAIT:
    if (wait) {
        mutex_lock(&lock->mutex);
    }
    wait = 0;
    mutex_lock(&_rculock_mgr.mutex);

    if (lock) {
        wait = lock->locked;
    } else {
        lock = __request_rculock();
        ASSERT(lock);
    }

    lock->wait_task ++;
    if (wait) {
        //lock->wait_task ++;
    } else {
        prcu = taskid_to_taskrcu(task_id);
        if (!prcu) {
            prcu = __rcu_tasklock_request();
            ASSERT(prcu);
            prcu->task_id  = task_id;
        }
        ASSERT(prcu->rculock_id < RCULOCK_COUNT);
        ASSERT(prcu->rculock_id >= -1);
        lock->locked = 1;
        lock->lr_val = lrReg;
        prcu->rculock_id++;
        prcu->rcu_locks[prcu->rculock_id] = lock;
    }
    mutex_unlock(&_rculock_mgr.mutex);

    if (wait) {
        goto __WAIT;
    }
#endif
}

void _rcu_read_unlock()
{
#if 0
    unsigned int task_id = sys_get_current_taskid();
    struct rculock *lock = NULL;
    struct rculock_task *prcu = NULL;

    prcu = taskid_to_taskrcu(task_id);
    ASSERT(prcu);
    ASSERT(prcu->rculock_id > -1);

    mutex_lock(&_rculock_mgr.mutex);

    lock = prcu->rcu_locks[prcu->rculock_id];
    prcu->rcu_locks[prcu->rculock_id] = NULL;
    prcu->rculock_id--;
    ASSERT(lock->wait_task);
    lock->locked = 0;
    lock->wait_task --;
    mutex_unlock(&lock->mutex);
    
    if (prcu->rculock_id == -1) {
        //__release_rculock(lock);
        __rcu_tasklock_release(prcu);
    }
    if (lock->wait_task == 0) {
        __release_rculock(lock);
    }
    mutex_unlock(&_rculock_mgr.mutex);

#endif
}

void _rcu_init(void)
{
    int i = 0;
    memset(&_rculock_mgr, 0, sizeof(_rculock_mgr));
    memset(&_rcu_tasks, 0, sizeof(_rcu_tasks));
    mutex_init(&_rculock_mgr.mutex);
    for(i = 0;i < RCULOCK_COUNT; i++) {
        mutex_init(&_rculock_mgr.locks[i].mutex);
    }
}

void _rcu_exit(void)
{
    int i = 0;
    
    mutex_destroy(&_rculock_mgr.mutex);
    for(i = 0;i < RCULOCK_COUNT; i++) {
        mutex_destroy(&_rculock_mgr.locks[i].mutex);
    }

}



#else
#include <linux/atomic.h>

struct rcu_mgr {
    struct mutex lock;
    atomic_t readers;
    struct rcu_head *head;
} __rcu_mgr;

static void __rcu_reclaim(void)
{
	unsigned long offset;
    struct rcu_head *head;

    os_mutex_lock(&__rcu_mgr.lock);
    while (__rcu_mgr.head) {
        head = __rcu_mgr.head;
        __rcu_mgr.head = head->next;
    
        offset = (unsigned long)head->func;
        if (__is_kfree_rcu_offset(offset)) {
            kfree((void *)head - offset);
        } else {
            head->func(head);
        }
    }
    os_mutex_unlock(&__rcu_mgr.lock);
}

void _rcu_init(void)
{
    memset(&__rcu_mgr,0,sizeof(__rcu_mgr));
    os_mutex_init(&__rcu_mgr.lock);
    atomic_set(&__rcu_mgr.readers, 0);
    __rcu_mgr.head = NULL;
}

void rcu_exit(void)
{
    synchronize_rcu();
    __rcu_reclaim();
}

void rcu_read_lock()
{
    atomic_inc(&__rcu_mgr.readers);
}

void rcu_read_unlock()
{
    if (atomic_dec_and_test(&__rcu_mgr.readers)) {
        __rcu_reclaim();
    }
}

void _call_rcu(struct rcu_head *head, void (*func)(struct rcu_head *rcu))
{
    os_mutex_lock(&__rcu_mgr.lock);
    head->func = func;
    head->next = NULL;
    if (__rcu_mgr.head == NULL) {
        __rcu_mgr.head = head;
    } else {
        head->next = __rcu_mgr.head;
        __rcu_mgr.head = head;
    }
    os_mutex_unlock(&__rcu_mgr.lock);
}

void synchronize_rcu(void)
{
	while( atomic_read(&__rcu_mgr.readers)){
        os_sleep_ms(10);
    }
}

#endif

