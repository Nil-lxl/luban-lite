/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#include "atbm_hal.h"
#include "atbm_os_mem.h"
void *atbm_wifi_zalloc(unsigned int size)
{
	atbm_void *mem = ATBM_NULL;

	mem = (atbm_void*)atbm_kmalloc(size,GFP_KERNEL);

	if(mem != ATBM_NULL){
		atbm_memset(mem,0,size);
	}

	return mem;
}

void *atbm_realloc(void *ptr, atbm_size_t size)
{
	return rt_realloc(ptr,size);
}

