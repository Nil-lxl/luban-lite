#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/bitops.h>
#include <linux/list.h>
#include <linux/rcu.h>
#include <linux/rculist.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/hrtimer.h>

void tasklet_hrtimer_cb(unsigned long data)
{
    struct tasklet_hrtimer *timer = (struct tasklet_hrtimer *)data;
    tasklet_schedule(&timer->task);
}

void tasklet_hrtimer_cancel(struct tasklet_hrtimer *timer)
{
    stop_timer(&timer->timer);
}

void tasklet_hrtimer_start(struct tasklet_hrtimer *timer, int timeout)
{
    mod_timer(&timer->timer, timeout);
}

void tasklet_hrtimer_init(struct tasklet_hrtimer *timer,
                          void (*cb)(struct tasklet_hrtimer *timer), int c, int mode)
{
    timer->task.name = "hrtimer";
    tasklet_create(&timer->task, (tasklet_func_t)cb, (unsigned long)timer, 1024);
    setup_timer(&timer->timer, tasklet_hrtimer_cb, (unsigned long)timer);
}

void tasklet_hrtimer_deinit(struct tasklet_hrtimer *timer)
{
    stop_timer(&timer->timer);
    tasklet_kill(&timer->task);
    del_timer(&timer->timer);
    tasklet_destory(&timer->task);
}

