/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#ifndef ATBM_OS_DEBUG_H
#define ATBM_OS_DEBUG_H
#ifdef RT_USING_ULOG
#include <ulog.h>
#endif
#define iot_printf rt_kprintf //LOG_RAW

#define __ATBM_FUNCTION__ __FUNCTION__
#endif //ATBM_OS_DEBUG_H
