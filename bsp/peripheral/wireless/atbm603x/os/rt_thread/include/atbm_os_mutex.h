#ifndef ATBM_OS_MUTEX_H
#define ATBM_OS_MUTEX_H
#include "atbm_type.h"
#include "rtthread.h"

typedef struct{
    rt_mutex_t mutex;
}atbm_mutex;

typedef struct{
    rt_sem_t sem;
}atbm_os_wait_queue_head_t;

/*sem*/
atbm_uint8 atbm_os_wait_event_timeout(atbm_os_wait_queue_head_t * pulSemId, atbm_uint32 ulTimeout);
void atbm_os_wakeup_event(atbm_os_wait_queue_head_t * pulSemId);
void atbm_os_init_waitevent(atbm_os_wait_queue_head_t* pulSemId);
void atbm_os_DeleteSem(atbm_os_wait_queue_head_t* pulSem);
void atbm_os_delete_waitevent(atbm_os_wait_queue_head_t* pulSem);

/*mutex*/
atbm_uint32 atbm_os_mutexLockInit(atbm_mutex * pmutex_id);
atbm_uint8 atbm_os_DeleteMutex(atbm_mutex * pmutex_id);
atbm_uint8 atbm_os_mutexLock(atbm_mutex * pmutex_id,atbm_uint32 ulTimeout);
atbm_uint8 atbm_os_mutexUnLock(atbm_mutex * pmutex_id);
#endif /* ATBM_OS_MUTEX_H */

