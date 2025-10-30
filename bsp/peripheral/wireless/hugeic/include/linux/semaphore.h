#ifndef __LINUX_SEMAPHORE_H
#define __LINUX_SEMAPHORE_H

#if defined(__MBED__)
#include "os/mbed/semaphore.h"
#elif defined(__WINDOWS__)
#include "os/win/semaphore.h"
#elif defined(__LINUX__)
#include "os/linux/semaphore.h"
#elif defined(__CSKY__)
#include "os/c-sky/semaphore.h"
#else
//#include "porting/semaphore.h"
#endif

struct semaphore {
    unsigned int sem;
    unsigned int vaild;
};

#define sema_init os_sema_init
#define sema_destroy os_sema_destroy
#define sema_down os_sema_down
#define down_interruptible os_down_interruptible
#define down_killable os_down_killable
#define down_trylock os_down_trylock
#define down_timeout os_down_timeout
#define sema_up os_sema_up

void os_sema_init(struct semaphore *sem, int val);
void os_sema_destroy(struct semaphore *sem);
int os_sema_down(struct semaphore *sem);
int os_down_interruptible(struct semaphore *sem);
int os_down_killable(struct semaphore *sem);
int os_down_trylock(struct semaphore *sem);
int os_down_timeout(struct semaphore *sem, long jif);
void os_sema_up(struct semaphore *sem);


#endif
