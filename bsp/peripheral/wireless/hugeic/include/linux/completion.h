#ifndef __LINUX_COMPLETION_H
#define __LINUX_COMPLETION_H
#include <linux/semaphore.h>

struct completion {
    struct semaphore sema;
};

#define init_completion(x) sema_init(&((x)->sema), 0)
#define wait_for_completion_timeout(x, tmo) !(down_timeout(&((x)->sema), jiffies_to_msecs(tmo)))
#define complete(x) sema_up(&((x)->sema))
#define deinit_completion(x) sema_destroy(&((x)->sema))


#endif
