#ifndef _LINUX_SPINLOCK_H_
#define _LINUX_SPINLOCK_H_

#include "linux/mutex.h"
#include "linux/defs.h"

typedef struct {
    struct mutex m;
    int locked;
} spinlock_t;

void local_bh_disable(void);
void local_bh_enable(void);
/*
#define spin_lock_init(lock) do{ \
        memset(lock, 0, sizeof(spinlock_t));\
        mutex_init(&((lock)->m)); \
    } while(0)
*/
#define spin_lock_init(lock) do{ \
        mutex_init(&((lock)->m)); \
    } while(0)

//#define spin_lock_init(lock)           mutex_init(&((lock)->m))
#define spin_lock_deinit(lock)         mutex_destroy(&((lock)->m))
#define spin_lock(lock)                ({mutex_lock(&((lock)->m));(lock)->locked=1;})
#define spin_lock_lh(lock)             ({mutex_lock(&((lock)->m));(lock)->locked=1;})
#define spin_lock_bh(lock)             ({mutex_lock(&((lock)->m));(lock)->locked=1;})
#define spin_lock_irq(lock)            ({mutex_lock(&((lock)->m));(lock)->locked=1;})
#define spin_lock_irqsave(lock, flags) ({mutex_lock(&((lock)->m));(lock)->locked=1;})

#define spin_trylock(lock)             ({\
        int __ret_val__ = mutex_trylock(&((lock)->m));\
        if(__ret_val__) (lock)->locked=1;\
        __ret_val__;})
#define spin_trylock_bh(lock)          ({\
        int __ret_val__ = mutex_trylock(&((lock)->m));\
        if(__ret_val__) (lock)->locked=1;\
        __ret_val__;})
#define spin_trylock_irq(lock)         ({\
        int __ret_val__ = mutex_trylock(&((lock)->m));\
        if(__ret_val__) (lock)->locked=1;\
        __ret_val__;})
#define spin_unlock(lock)              ({(lock)->locked=0; mutex_unlock(&((lock)->m));})
#define spin_unlock_bh(lock)           ({(lock)->locked=0; mutex_unlock(&((lock)->m));})
#define spin_unlock_irq(lock)          ({(lock)->locked=0; mutex_unlock(&((lock)->m));})
#define spin_unlock_irqrestore(lock, flags)  ({(lock)->locked=0; mutex_unlock(&((lock)->m));})
#define read_unlock_bh(lock)           ({(lock)->locked=0; mutex_unlock(&((lock)->m));})
#define read_lock_bh(lock)             ({mutex_lock(&((lock)->m));(lock)->locked=1;})
#define write_lock_bh(lock)            ({mutex_lock(&((lock)->m));(lock)->locked=1;})
#define write_unlock_bh(lock)          ({(lock)->locked=0; mutex_unlock(&((lock)->m));})

#define lockdep_assert_held(p) ASSERT((p)->locked)

#define DEFINE_SPINLOCK(lock) spinlock_t lock
#define DEFINE_RWLOCK(lock)   spinlock_t lock

#endif
