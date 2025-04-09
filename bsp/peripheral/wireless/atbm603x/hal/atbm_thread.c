/**************************************************************************************************************
 * altobeam RTOS WSM host interface (HI) implementation
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#include "atbm_hal.h"

static int atbm_ThreadCallback(void *arg){
	atbm_thread_internal_t *thread = (atbm_thread_internal_t *)arg;

	while(!thread->stop){
		if(thread->cb){
			thread->cb(thread->p_arg);
		}
	}
	atbm_os_wakeup_event(&thread->stopEv);
}

atbm_thread_internal_t *atbm_createThreadInternal(char *name, atbm_void(*task)(atbm_void *p_arg),atbm_void *p_arg,int prio){
	atbm_thread_internal_t *thread = atbm_kmalloc(sizeof(atbm_thread_internal_t), GFP_KERNEL);
	if(!thread)
		return ATBM_NULL;
	
	thread->cb = task;
	thread->p_arg = p_arg;
	thread->stop = 0;

	atbm_os_init_waitevent(&thread->stopEv);	

	thread->thread = atbm_createThread(name,atbm_ThreadCallback,(atbm_void*)thread,prio);
	if(!thread->thread){
		atbm_kfree(thread);
		return ATBM_NULL;
	}
	return thread;
}

int atbm_stopThreadInternal(atbm_thread_internal_t *thread){
	thread->stop = 1;
	atbm_os_wait_event_timeout(&thread->stopEv, 10*HZ);

	atbm_stopThread(thread->thread);
	atbm_kfree(thread);
};



