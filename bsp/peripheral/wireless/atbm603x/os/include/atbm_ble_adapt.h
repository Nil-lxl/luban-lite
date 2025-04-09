/**************************************************************************************************************
 * altobeam RTOS
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#ifndef _ATBM_BLE_ADAPT_H_
#define _ATBM_BLE_ADAPT_H_

typedef signed char s8;

typedef unsigned char u8;

typedef signed short s16;

typedef unsigned short u16;

typedef signed int s32;

typedef unsigned int u32;

typedef signed long long s64;

typedef unsigned long long u64;

typedef signed char int8_t;

typedef unsigned char uint8_t;

typedef signed short int16_t;

typedef unsigned short uint16_t;

typedef signed long int32_t;

typedef unsigned long uint32_t;

typedef signed long long int64_t;

typedef unsigned long long uint64_t;

#ifndef bool
typedef int bool;
#define true 1
#define false 0
#endif

#define INT_MAX		((int)(~0U>>1))
#define INT_MIN		(-INT_MAX - 1)
#define UINT_MAX	(~0U)
#define LONG_MAX	((long)(~0UL>>1))
#define LONG_MIN	(-LONG_MAX - 1)

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif


enum BLE_THREAD_PRIO
{
	BLE_TASK_PRIO = 5,
	BLE_AT_PRIO,
	BLE_APP_PRIO,
};

#include "atbm_type.h"
#include "atbm_sysops.h"

#endif /*_ATBM_BLE_ADAPT_H_*/
