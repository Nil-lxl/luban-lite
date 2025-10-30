
#ifndef _LINUX_HRTIMER_H_
#define _LINUX_HRTIMER_H_
#include <linux/timer.h>
#include <linux/workqueue.h>
#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME (0)
#endif
#define HRTIMER_MODE_ABS (0)

struct tasklet_hrtimer {
    struct timer_list     timer;
    struct tasklet_struct task;
};

void tasklet_hrtimer_init(struct tasklet_hrtimer *timer,
                          void (*cb)(struct tasklet_hrtimer *timer), int c, int mode);
void tasklet_hrtimer_deinit(struct tasklet_hrtimer *timer);
void tasklet_hrtimer_cancel(struct tasklet_hrtimer *timer);
void tasklet_hrtimer_start(struct tasklet_hrtimer *timer, int timeout);
#ifdef __cplusplus
}
#endif
#endif

