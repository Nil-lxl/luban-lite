/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#include "atbm_hal.h"

void atbm_os_init_waitevent(atbm_os_wait_queue_head_t *pulSem)
{	 
    pulSem->sem = rt_sem_create("waitQueue", 0, RT_IPC_FLAG_PRIO);
    if (pulSem->sem == NULL){
        ATBM_ERROR("init_waitevent err\n");
    }
}


void atbm_os_delete_waitevent(atbm_os_wait_queue_head_t* pulSem)
{
    rt_sem_delete(pulSem->sem);
}


atbm_uint8 atbm_os_wait_event_timeout(atbm_os_wait_queue_head_t * pSem, atbm_uint32 ulTimeout)
{
	int ret;
	ret = rt_sem_take(pSem->sem, ulTimeout);
	if(ret == RT_EOK)//OS_SUCCESS;
		return 1;
	else if (-RT_ETIMEOUT == ret)
		return 0;
	else //fail
		return -1;
}

void atbm_os_wakeup_event(atbm_os_wait_queue_head_t * pSem)
{
	if(rt_sem_release(pSem->sem)!=0){
		ATBM_ERROR("wakeup_event fail\n");
	}
}


atbm_uint32 atbm_os_mutexLockInit(atbm_mutex * pmutex)
{
    pmutex->mutex= rt_mutex_create("AtbMutex", RT_IPC_FLAG_PRIO);

	if(pmutex->mutex){
		return 0;
	}
    ATBM_ERROR("mutex init fail\n");
    return -1;
}

atbm_uint8 atbm_os_DeleteMutex(atbm_mutex * pmutex)
{
	return rt_mutex_delete(pmutex->mutex);
}

atbm_uint8 atbm_os_mutexLock(atbm_mutex * pmutex,atbm_uint32 ulTimeout)
{
    return rt_mutex_take(pmutex->mutex, RT_WAITING_FOREVER);
}

atbm_uint8 atbm_os_mutexUnLock(atbm_mutex * pmutex_id)
{
    return rt_mutex_release(pmutex_id->mutex);
}

