/*
 * Copyright (C) 2018-2024 AICSemi Ltd.
 *
 * All Rights Reserved
 */
#ifndef _AIC__PLAT_LOG_H_
#define _AIC__PLAT_LOG_H_

#include "aic8800_config.h"

#ifdef PLATFORM_ASR_THREADX
#include "common_print.h"
#define AIC_PLAT_PRINTF_API         CPUartLogPrintf
#elif defined(CONFIG_RTTHEAD_PLAT)
#include <rtdbg.h>
#define AIC_PLAT_PRINTF_API         rt_kprintf
#endif

#ifdef AIC_LOG_DEBUG_ON
#define AIC_LOG_PRINTF(fmt, ...)    {AIC_PLAT_PRINTF_API("[AIC_I]"fmt, ##__VA_ARGS__);}
#define AIC_LOG_TRACE(fmt, ...)     {AIC_PLAT_PRINTF_API("[AIC_T]"fmt, ##__VA_ARGS__);}
#define AIC_LOG_ERROR(fmt, ...)     {AIC_PLAT_PRINTF_API("[AIC_E]"fmt, ##__VA_ARGS__);}
//#define AIC_LOG_INFO(fmt, ...)      {AIC_PLAT_PRINTF_API(fmt, ##__VA_ARGS__);}

#else
#define AIC_LOG_PRINTF(...)         do { } while(0)
#define AIC_LOG_TRACE(...)          do { } while(0)
#define AIC_LOG_ERROR(...)          do { } while(0)
#define AIC_LOG_INFO(...)           do { } while(0)
#endif /* AIC_LOG_DEBUG_ON */

#define aic_dbg                     AIC_LOG_PRINTF


#endif /* _AIC_LOG_H_ */

