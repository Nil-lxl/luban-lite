/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#ifndef ATBMWIFI_THREAD_H
#define ATBMWIFI_THREAD_H

typedef void (*thread_callback)(struct atbm_sdio_func *func);

typedef struct{
	thread_callback cb;
	atbm_void *p_arg;
	pAtbm_thread_t thread;
	ATBM_BOOL stop;
	atbm_os_wait_queue_head_t stopEv;
}atbm_thread_internal_t;

atbm_thread_internal_t *atbm_createThreadInternal(char *name, atbm_void(*task)(atbm_void *p_arg),atbm_void *p_arg,int prio);
int atbm_stopThreadInternal(atbm_thread_internal_t *thread);
#endif //ATBMWIFI_THREAD_H
