#include <linux/types.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/msg_queue.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/defs.h>
#include "os.h"

#define IPC_MUTEX_VALID_MAGIC_NUM (0x1A1A1A1A)
#define IPC_SEMA_VALID_MAGIC_NUM (0x2B2B2B1B)
#define IPC_MSGQ_VALID_MAGIC_NUM (0x3C3C3C3C)
#define IPC_TIMER_VALID_MAGIC_NUM (0x4D4D4D4D)
#define IPC_TASK_VALID_MAGIC_NUM (0x5E5E5E5E)
#define IPC_INVALID_MAGIC_NUM (0x4D3C2B1A)

#define osWaitForever                   (0xffffffffu)
#define hgic_dbg(fmt, ...) PRINTF("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define hgic_err(fmt, ...) printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

typedef void (*task_pfunc)(void *);

extern int   os_msgqueue_init(unsigned int *queue_hdl,int elem_num);
extern void *os_msgqueue_recv(unsigned int queue_hdl,int millisec);
extern int   os_msgqueue_send(unsigned int queue_hdl,int data,int millisec);
extern void  os_msgqueue_del(unsigned int queue_hdl);
extern int   os_msgqueue_get_count(unsigned int queue_hdl);
extern int os_mutex_new(unsigned int *mutex_handle);
extern int os_mutex_take(unsigned int mutex_handle);
extern void os_mutex_release(unsigned int mutex_handle);
extern void os_mutex_delete(unsigned int mutex_handle);
extern int  os_sem_new(unsigned int *sem_hdl,int val);
extern void os_sem_free(unsigned int sem_hdl);
extern int  os_sem_take(unsigned int sem_hdl,unsigned int timeout);
extern void os_sem_release(unsigned int sem_hdl);
extern void os_timer_stop(unsigned int timer);
extern void os_timer_start(unsigned int timer);
extern int  os_timer_create(unsigned int *tmr,void *param, timer_func_t func,int ms);
extern int  os_timer_change(unsigned int timer,unsigned int ms);
extern void os_timer_free(unsigned int timer);
extern int  os_thread_create(unsigned int *taskhdl,task_pfunc func,
                            char *name,unsigned int param,
                            unsigned int stack_size,unsigned int task_prio);
extern void os_thread_delete(unsigned int task);
//-------------------Memory---------------------------//
void *osal_zalloc(unsigned long size)
{
    void *ptr = MALLOC(size);
    if (ptr)
    { MEMSET(ptr, 0, size); }
    return ptr;
}

char *osal_strdup(const char *s)
{
    unsigned long len;
    char *d;

    if(s == NULL) return NULL;
    len = strlen(s);
    d = MALLOC(len + 1);
    if (d == NULL) {
        return NULL;
    }
    memcpy(d, s, len);
    d[len] = '\0';
    return d;
}

void *osal_realloc(void *sptr, unsigned long size)
{
    void *ptr = NULL;

    if (sptr == NULL)
    { return osal_zalloc(size); }

    ptr = MALLOC(size);
    if (ptr) {
        MEMSET(ptr, 0, size);
        MEMCPY(ptr, sptr, size);
        FREE(sptr);
    }
    return ptr;
}

//-------------------Task-----------------------------//
void tasklet_create(struct tasklet_struct *t, tasklet_func_t func,
                    unsigned long data, int stack_size)
{
    char *name = t->name;
    unsigned int task_prio = 1;//Normal
    int ret = 0;

    memset(t, 0, sizeof(struct tasklet_struct));
    os_sema_init(&t->sem, 0);
    os_sema_init(&t->rcu_sem, 0);
    t->name = name;
    t->func = func;
    t->args = data;
    t->enable = 1;
    t->stop   = 0;
    t->running = 0;
    t->rculock_id = -1;

    ret = os_thread_create(&t->task, tasklet_entry, name, t, stack_size, task_prio);
    ASSERT(!ret);
    t->magic = IPC_TASK_VALID_MAGIC_NUM;
    hgic_dbg("Task %s create success,task_hdl:%p,func:%p\n", name,
             t->task, func);
}

void tasklet_destory(struct tasklet_struct *t)
{
    int idx = 0;
    if (t == NULL) {
        hgic_err("Input param error!\n");
        return;
    }
    if (t->magic != IPC_TASK_VALID_MAGIC_NUM) {
        hgic_err("Invaild tasklet_struct!\n");
        return;
    }

    hgic_dbg("Destroy task,task handle:%p\n", t->task);
    os_task_delete(t->task);
    t->magic = IPC_INVALID_MAGIC_NUM;
}

//-------------------Msgqueue-------------------------//
int msg_queue_init(struct msg_queue *msgq, int size)
{
    int ret = 0;
    if (msgq->vaild != IPC_MSGQ_VALID_MAGIC_NUM) {
        ret = os_msgqueue_init(&msgq->q, size);
        ASSERT(!ret);
        msgq->vaild = IPC_MSGQ_VALID_MAGIC_NUM;
    }
    return 0;
}

int msg_queue_get(struct msg_queue *msgq, int millisec)
{
    int ret = 0;

    ASSERT(msgq);
    if (msgq->vaild != IPC_MSGQ_VALID_MAGIC_NUM) {
        hgic_err("Invaild msgq!ret_addr:%p\n", RETURN_ADDR());
        ASSERT(0);
    }
    return (int)os_msgqueue_recv(msgq->q, millisec);
}

int msg_queue_put(struct msg_queue *msgq, int data, int millisec)
{
    int ret = 0;
    ASSERT(msgq);
    if (msgq->vaild != IPC_MSGQ_VALID_MAGIC_NUM) {
       hgic_err("Invaild msgq!ret_addr:%p\n", RETURN_ADDR());
       return -1;
    }

    return os_msgqueue_send(msgq->q, data, millisec);
}

int msg_queue_del(struct msg_queue *msgq)
{
    int ret = 0;
    if (msgq) {
        if (msgq->vaild != IPC_MSGQ_VALID_MAGIC_NUM) {
            hgic_err("Invaild msgq!ret_addr:%p\n", RETURN_ADDR());
            ASSERT(0);
        }
        os_msgqueue_del(msgq->q);
        msgq->vaild = IPC_INVALID_MAGIC_NUM;
        return ret;
    }
    return 0;
}

int msg_queue_count(struct msg_queue *msgq)
{
    if (msgq->vaild != IPC_MSGQ_VALID_MAGIC_NUM) {
        hgic_err("Invaild msgq!ret_addr:%p\n", RETURN_ADDR());
        ASSERT(0);
    }
    return os_msgqueue_get_count(msgq->q);
}

//------------------------Mutex----------------------------//
void os_mutex_init(struct mutex *lock)
{
    int ret = 0;
    ASSERT(lock);
    if (lock->vaild != IPC_MUTEX_VALID_MAGIC_NUM) {
        ret = os_mutex_new(&lock->mutex);
        ASSERT(!ret);
        lock->vaild = IPC_MUTEX_VALID_MAGIC_NUM;
    } else {
        hgic_err("Already inited!ret_addr:%p\n", RETURN_ADDR());
    }
}

void os_mutex_lock(struct mutex *lock)
{
    int ret = 0;
    ASSERT(lock);
    if (lock->vaild != IPC_MUTEX_VALID_MAGIC_NUM) {
        hgic_err("Invaild mutex!ret_addr:%p,vaild:0x%x\n", RETURN_ADDR(),lock->vaild);
        while(1);
    }
    ret = os_mutex_take(lock->mutex);
    if (ret == 0) {
        lock->locked ++;

    } else {
        hgic_err("mutex lock failed,ret_addr:%p\n",RETURN_ADDR());
    }
}

/*Returns 1 if the mutex has been acquired successfully*/
int os_mutex_trylock(struct mutex *lock)
{
    int ret = 0;

    ASSERT(lock);
    if (lock->vaild != IPC_MUTEX_VALID_MAGIC_NUM) {
        hgic_err("Invaild mutex!ret_addr:%p\n", RETURN_ADDR());
        ASSERT(0);
    }
    if (lock->locked) {
        return 0;
    } else {
        ret = os_mutex_take(lock->mutex);
        if (ret == 0) {
            lock->locked ++;
            return 1;
        } else {
            return 0;
        }
    }
}

void os_mutex_unlock(struct mutex *lock)
{
    ASSERT(lock);
    if (lock->vaild != IPC_MUTEX_VALID_MAGIC_NUM) {
        hgic_err("Invaild mutex!ret_addr:%p,%x\n", RETURN_ADDR(),lock->vaild);
        while(1);
    }
    lock->locked --;
    os_mutex_release(lock->mutex);
}

void os_mutex_destroy(struct mutex *lock)
{
    ASSERT(lock);
    if (lock->vaild != IPC_MUTEX_VALID_MAGIC_NUM) {
        hgic_err("Invaild mutex!ret_addr:%p\n", RETURN_ADDR());
        ASSERT(0);
    }
    os_mutex_delete(lock->mutex);
    lock->vaild = IPC_INVALID_MAGIC_NUM;
}

//------------------------Semaphore------------------------//
void os_sema_init(struct semaphore *sem, int val)
{
    int ret = 0;
    ASSERT(sem);
    if(sem->vaild != IPC_SEMA_VALID_MAGIC_NUM) {
        ret = os_sem_new(&sem->sem, val);
        if(ret) {
            hgic_err("Error:%d\n",ret);
            return;
        }
        sem->vaild = IPC_SEMA_VALID_MAGIC_NUM;
    }
}

void os_sema_destroy(struct semaphore *sem)
{
    ASSERT(sem);
    if(sem->vaild != IPC_SEMA_VALID_MAGIC_NUM) {
        hgic_err("Invaild sem!ret_addr:%p\n",RETURN_ADDR());
        ASSERT(0);
    }
    os_sem_free(sem->sem);
    sem->vaild = IPC_INVALID_MAGIC_NUM;
}

int os_sema_down(struct semaphore *sem)
{
    int ret = 0;
    ASSERT(sem);
    if(sem->vaild != IPC_SEMA_VALID_MAGIC_NUM) {
        hgic_err("Invaild sem!ret_addr:%p\n",RETURN_ADDR());
        ASSERT(0);
    }
    ret = os_sem_take(sem->sem, osWaitForever);
    return ret;
}

/*If the semaphore is successfully acquired, returns 0.*/
int os_down_interruptible(struct semaphore *sem)
{
    return os_sema_down(sem);
}

/*If the semaphore is successfully acquired, returns 0.*/
int os_down_killable(struct semaphore *sem)
{
    return os_sema_down(sem);
}

/*If the semaphore is successfully acquired, returns 0.*/
int os_down_timeout(struct semaphore *sem, long millisec)
{
    ASSERT(sem);
    if(sem->vaild != IPC_SEMA_VALID_MAGIC_NUM) {
        hgic_err("Invaild sem!ret_addr:%p\n",RETURN_ADDR());
        ASSERT(0);
    }
    return os_sem_take(sem->sem, millisec);
}

void os_sema_up(struct semaphore *sem)
{
    ASSERT(sem);
    if(sem->vaild != IPC_SEMA_VALID_MAGIC_NUM) {
        hgic_err("Invaild sem!ret_addr:%p\n",RETURN_ADDR());
        ASSERT(0);
    }
    os_sem_release(sem->sem);
}


//------------------------Timer Management----------------------------//

#define PRE_ALLOC_TIMER_NUM 32
#define HGIC_TIMER_MAGIC    (0x6F6F6F6F)
#define TIMER_TASK_STACK_SIZE     4096

struct hgic_timer_mgmt {
    struct mutex timer_mgmt_lock;
    struct hgic_timer timer_array[PRE_ALLOC_TIMER_NUM];
    struct msg_queue timer_queue;
    int timer_task_hdl;
};

static struct hgic_timer_mgmt g_timer_mgmt;
static void timer_cb(void *args);

static int hgic_timer_vaild(struct hgic_timer *tmr)
{
    if (!tmr) {
        hgic_err("Invaild timer!\n");
        return -1;
    }
    if (tmr->tmr_magic != HGIC_TIMER_MAGIC) {
        hgic_err("Invaild timer magic:0x%x\n", tmr->tmr_magic);
        return -2;
    }
    if (tmr->used == 0) {
        hgic_err("Unused timer!\n");
        return -4;
    }
    if (tmr->timer_priv == NULL) {
        hgic_err("Invaild timer priv data!\n");
        return -5;
    }
    if (tmr->timer_priv->enable == 0) {
        hgic_err("Timer %p disable!\n",tmr->timer_priv);
        return -6;
    }
    return 0;
}

void hgic_timer_disable(struct timer_list *timer)
{
    if(timer == NULL || timer->tmr == NULL) {
        hgic_err("Input param error!\n");
    }
    os_timer_stop(timer->tmr->timer);
    timer->enable = 0;
}

void hgic_timer_enable(struct timer_list *timer)
{
    if(timer == NULL || timer->tmr == NULL) {
        hgic_err("Input param error!\n");
    }
    os_timer_start(timer->tmr->timer);
    timer->enable = 1;
}

static void hgic_timer_cb(void *arg)
{
    struct hgic_timer *tmr = (struct hgic_timer *)arg;
    struct hgic_timer_mgmt *timer_mgmt = NULL;
    int ret = 0;

    ASSERT(tmr);
    ASSERT(tmr->tmr_magic == HGIC_TIMER_MAGIC);
    //os_mutex_lock(&tmr->tmr_lock);
    //ASSERT(tmr->timer_priv);
    if(!tmr->timer_priv) {
        hgic_err("invaild tmr:%p,timer_priv:%p\n",tmr,tmr->timer_priv);
        //os_mutex_unlock(&tmr->tmr_lock);
        return;
    }
    ASSERT(tmr->timer_mgr);
    if(!tmr->used) {
        hgic_err("Unused timer,function:%p\n",tmr->timer_priv->function);
        //os_mutex_unlock(&tmr->tmr_lock);
        return;
    }

    timer_mgmt = (struct hgic_timer_mgmt *)tmr->timer_mgr;
    ret = msg_queue_put(&timer_mgmt->timer_queue, (int)tmr, 0);
    if(ret) {
        hgic_err("Timer msgqueue send failed,ret:%d\n",ret);
    }
    //os_mutex_unlock(&tmr->tmr_lock);
}

static void timer_cb(void *args)
{
    struct timer_list *timer = (struct timer_list *)args;
    ASSERT(timer);
    ASSERT(timer->tmr);
    if (timer->vaild == IPC_TIMER_VALID_MAGIC_NUM) {
        timer->pending = 1;
        timer->function((timer->data != 0xFFFFFFFF ? timer->data : (unsigned long)timer));
        timer->pending = 0;
    } else {
        hgic_err("Invaild timer!\n");
        ASSERT(0);
    }
}

static void hgic_timer_task(void *arg)
{
    struct hgic_timer_mgmt *timer_mgmt = (struct hgic_timer_mgmt *)arg;
    struct hgic_timer *tmr = NULL;
    int ret = 0;

    while (1) {
        tmr = NULL;
        tmr = (struct hgic_timer *)msg_queue_get(&timer_mgmt->timer_queue, osWaitForever);

        //os_mutex_lock(&tmr->tmr_lock);
        ret = hgic_timer_vaild(tmr);
        if (ret) {
            hgic_err("Invaild timer,ret:%d\n", ret);
            //os_mutex_unlock(&tmr->tmr_lock);
            continue;
        }
        timer_cb(tmr->timer_priv);
        //os_mutex_unlock(&tmr->tmr_lock);
    }
}

int os_timer_mgr_init()
{
    struct hgic_timer_mgmt *timer_mgmt = &g_timer_mgmt;
    struct hgic_timer *tmr = NULL;
    int i = 0;
    int ret = 0;
    unsigned int task_prio = 0;//HIGH

    memset(timer_mgmt, 0, sizeof(struct hgic_timer_mgmt));
    os_mutex_init(&timer_mgmt->timer_mgmt_lock);
    ret = msg_queue_init(&timer_mgmt->timer_queue, 32);
    ASSERT(!ret);

    for (i = 0; i < PRE_ALLOC_TIMER_NUM; i++) {
        tmr = &timer_mgmt->timer_array[i];
        memset(tmr, 0, sizeof(struct hgic_timer));
        tmr->index = i;
        os_mutex_init(&tmr->tmr_lock);
        ret = os_timer_create(&tmr->timer,tmr,hgic_timer_cb,300);
        ASSERT(!ret);
        tmr->tmr_magic = HGIC_TIMER_MAGIC;
        tmr->timer_mgr = timer_mgmt;
    }
    ret = os_thread_create(&timer_mgmt->timer_task_hdl,
        hgic_timer_task, "hgtmr_task", timer_mgmt, TIMER_TASK_STACK_SIZE, task_prio);
    ASSERT(!ret);
    return 0;
}

static struct hgic_timer *hgic_get_free_timer(void)
{
    struct hgic_timer *tmr = NULL;
    struct hgic_timer_mgmt *timer_mgmt = &g_timer_mgmt;
    int i = 0;
    int find = 0;
    unsigned int intr = 0;

    os_mutex_lock(&timer_mgmt->timer_mgmt_lock);
    for (i = 0; i < PRE_ALLOC_TIMER_NUM; i++) {
        tmr = &timer_mgmt->timer_array[i];
        if (tmr->used == 0) {
            find = 1;
            break;
        }
    }

    if (find == 0) {
        os_mutex_unlock(&timer_mgmt->timer_mgmt_lock);
        hgic_err("no free timer!\n");
        for (i = 0; i < PRE_ALLOC_TIMER_NUM; i++) {
            tmr = &timer_mgmt->timer_array[i];
            hgic_err("timer index:%d,callback:%p,magic:0x%x\n",
                i,tmr->timer_priv->function,tmr->timer_priv->vaild);
        }
        return NULL;
    } else {
        tmr->used = 1;
        os_mutex_unlock(&timer_mgmt->timer_mgmt_lock);
        return tmr;
    }
}

static void hgic_timer_del(struct hgic_timer *tmr)
{
    unsigned int intr = 0;
    struct hgic_timer_mgmt *timer_mgmt = NULL;
    ASSERT(tmr->tmr_magic == HGIC_TIMER_MAGIC);
    ASSERT(tmr->timer_mgr);

    timer_mgmt = (struct hgic_timer_mgmt *)tmr->timer_mgr;
    os_mutex_lock(&timer_mgmt->timer_mgmt_lock);
    tmr->timer_priv = NULL;
    tmr->used = 0;
    os_mutex_unlock(&timer_mgmt->timer_mgmt_lock);
}

//-------------------------Timer---------------------//
int os_init_timer(struct timer_list *timer)
{
    ASSERT(timer);
    if (timer->vaild != IPC_TIMER_VALID_MAGIC_NUM) {
        timer->pending = 0;
        timer->tmr  = hgic_get_free_timer();
        if (timer->tmr == NULL) {
            hgic_err("Failed!\n");
            return -1;
        }
        ASSERT(timer->tmr->tmr_magic == HGIC_TIMER_MAGIC);
        os_mutex_lock(&timer->tmr->tmr_lock);
        hgic_timer_disable(timer);
        timer->tmr->timer_priv = timer;
        timer->vaild = IPC_TIMER_VALID_MAGIC_NUM;
        os_mutex_unlock(&timer->tmr->tmr_lock);
        return 0;
    } else {
        return 0;
    }
}

int os_setup_timer(struct timer_list *timer, void (*func)(unsigned long data), unsigned long data)
{
    int ret = 0;
    ret = os_init_timer(timer); //must auto create
    if (ret) {
        hgic_err("Failed!\n");
        return ret;
    }
    ASSERT(timer->tmr->tmr_magic == HGIC_TIMER_MAGIC);
    os_mutex_lock(&timer->tmr->tmr_lock);
    timer->function = func;
    timer->data     = data;
    os_mutex_unlock(&timer->tmr->tmr_lock);
    return 0;
}

int os_timer_setup(struct timer_list *timer, void (*func)(struct timer_list *timer), unsigned int flags)
{
    int ret = 0;
    ret = os_init_timer(timer); //must auto create
    if (ret) {
        hgic_err("Failed!\n");
        return ret;
    }
    ASSERT(timer->tmr->tmr_magic == HGIC_TIMER_MAGIC);
    os_mutex_lock(&timer->tmr->tmr_lock);
    timer->function = (timer_func_t)func;
    timer->data     = 0xFFFFFFFF;
    os_mutex_unlock(&timer->tmr->tmr_lock);

    return 0;
}

int os_add_timer(struct timer_list *timer)
{
    int ret = 0;
    int ms  = 0;

    ret = os_init_timer(timer); //must auto create
    if (ret) {
        hgic_dbg("Failed!\n");
        return ret;
    }
    ASSERT(timer->tmr->tmr_magic == HGIC_TIMER_MAGIC);
    ms = jiffies_to_msecs(timer->expires - jiffies);
    os_mutex_lock(&timer->tmr->tmr_lock);
    hgic_timer_disable(timer);
    os_timer_change(timer->tmr->timer, ms < 10 ? 10 : ms);
    hgic_timer_enable(timer);
    os_mutex_unlock(&timer->tmr->tmr_lock);
    return 0;
}

int os_mod_timer(struct timer_list *timer, unsigned long expires)
{
    //ASSERT(timer);
    //ASSERT(timer->tmr);
    //ASSERT(timer->tmr->tmr_magic == HGIC_TIMER_MAGIC);
    //os_mutex_lock(&timer->tmr->tmr_lock);
    timer->expires = expires;
    //os_mutex_unlock(&timer->tmr->tmr_lock);
    return os_add_timer(timer);
}

int os_stop_timer(struct timer_list *timer)
{
    int ret = 0;
    ASSERT(timer);
    if (timer->vaild != IPC_TIMER_VALID_MAGIC_NUM) {
        hgic_err("Invaild timer!ret_addr:%p\n", RETURN_ADDR());
        ASSERT(0);
    }
    ASSERT(timer->tmr);
    ASSERT(timer->tmr->tmr_magic == HGIC_TIMER_MAGIC);
    os_mutex_lock(&timer->tmr->tmr_lock);
    hgic_timer_disable(timer);
    os_mutex_unlock(&timer->tmr->tmr_lock);
    return ret;
}

int os_del_timer(struct timer_list *timer)
{
    struct mutex *timer_lock = NULL;

    ASSERT(timer);
    if (timer->vaild != IPC_TIMER_VALID_MAGIC_NUM) {
        //hgic_err("Invaild timer!ret_addr:%p,magic:%x\n", RETURN_ADDR(), timer->vaild);
        return 0;
    }
    ASSERT(timer->tmr);
    ASSERT(timer->tmr->tmr_magic == HGIC_TIMER_MAGIC);

    timer_lock = &timer->tmr->tmr_lock;
    os_mutex_lock(timer_lock);
    hgic_timer_disable(timer);
    hgic_timer_del(timer->tmr);
    timer->tmr = NULL;
    timer->vaild = IPC_INVALID_MAGIC_NUM;
    os_mutex_unlock(timer_lock);
    return 0;
}

int os_del_timer_sync(struct timer_list *timer)
{
    struct mutex *timer_lock = NULL;

    ASSERT(timer);
    if (timer->vaild != IPC_TIMER_VALID_MAGIC_NUM) {
        //hgic_err("Invaild timer!ret_addr:%p,magic:%x\n", RETURN_ADDR(), timer->vaild);
        return 0;
    }
    ASSERT(timer->tmr);

    while (timer->pending == 1) {
        hgic_dbg("Wait for timer handle done...\n");
        sleep_ms(1000);
    }

    timer_lock = &timer->tmr->tmr_lock;
    os_mutex_lock(timer_lock);
    hgic_timer_disable(timer);
	hgic_timer_del(timer->tmr);
    timer->tmr = NULL;
    timer->vaild = IPC_INVALID_MAGIC_NUM;
    os_mutex_unlock(timer_lock);
    return 0;
}

void os_timer_mgr_delete()
{
    struct hgic_timer *tmr = NULL;
    struct hgic_timer_mgmt *timer_mgmt = &g_timer_mgmt;
    int i = 0;

    os_mutex_lock(&timer_mgmt->timer_mgmt_lock);
    for (i = 0; i < PRE_ALLOC_TIMER_NUM; i++) {
        tmr = &timer_mgmt->timer_array[i];
        hgic_timer_disable(tmr->timer_priv);
        os_timer_free(tmr->timer);
        os_mutex_destroy(&tmr->tmr_lock);
        tmr->index = 0;
        tmr->timer_priv = NULL;
        tmr->used = 0;
        tmr->tmr_magic = IPC_INVALID_MAGIC_NUM;
    }
    os_task_delete(timer_mgmt->timer_task_hdl);
    msg_queue_del(&timer_mgmt->timer_queue);
    os_mutex_unlock(&timer_mgmt->timer_mgmt_lock);
    os_mutex_destroy(&timer_mgmt->timer_mgmt_lock);
    memset(timer_mgmt, 0, sizeof(struct hgic_timer_mgmt));
}

