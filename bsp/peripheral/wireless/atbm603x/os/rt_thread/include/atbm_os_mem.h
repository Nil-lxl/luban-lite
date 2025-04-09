/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#ifndef ATBM_OS_MEM_H
#define ATBM_OS_MEM_H
#include <rtthread.h>

extern void *atbm_wifi_zalloc(unsigned int size);
extern void *atbm_realloc(void *ptr, atbm_size_t size);

#define atbm_kmalloc(x,y)   rt_malloc(x)
#define atbm_kzalloc(x,y)   atbm_wifi_zalloc(x)
#define atbm_kfree          rt_free
#define atbm_memcpy         rt_memcpy
#define atbm_memset         rt_memset
#define atbm_memmove  		rt_memmove
#define atbm_memcmp 		rt_memcmp
#define atbm_calloc			rt_calloc

static inline void * atbm_realloc_array(void *ptr, size_t nmemb, size_t size)
{
	if (size && nmemb > (~(size_t) 0) / size)
		return NULL;
	return atbm_realloc(ptr, nmemb * size);
}


#endif /* ATBM_OS_MEM_H */
