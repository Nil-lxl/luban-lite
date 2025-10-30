#ifndef _LINUX_MUTEX_H_
#define _LINUX_MUTEX_H_

#if defined(__MBED__)
#include "os/mbed/mutex.h"
#elif defined(__WINDOWS__)
#include "os/win/mutex.h"
#elif defined(__LINUX__)
#include "os/linux/mutex.h"
#elif defined(__CSKY__)
#include "os/c-sky/mutex.h"
#else
//#include "porting/mutex.h"
#endif

struct mutex {
    unsigned int mutex;  
    int locked;
    unsigned int vaild;
};

#define mutex_init os_mutex_init
#define mutex_lock os_mutex_lock
#define mutex_trylock os_mutex_trylock
#define mutex_unlock  os_mutex_unlock
#define mutex_destroy os_mutex_destroy

#define DEFINE_MUTEX(m) struct mutex m
#define mutex_lock_nested(lock, subclass) mutex_lock(lock)
#define lockdep_is_held(m) ASSERT((m)->locked)

void os_mutex_init(struct mutex *lock);
void os_mutex_lock(struct mutex *lock);
/**
 * Try to acquire the mutex atomically. Returns 1 if the mutex
 * has been acquired successfully, and 0 on contention.
 */
int  os_mutex_trylock(struct mutex *lock);
void os_mutex_unlock(struct mutex *lock);
void os_mutex_destroy(struct mutex *lock);

//int os_spinlock_init(unsigned int *spinlock_handle);
//void os_spinlock_delete(unsigned int spinlock_handle);
//void os_spinlock_lock(unsigned int spinlock_handle);
//void os_spinlock_unlock(unsigned int spinlock_handle);
#endif

