/**
 ****************************************************************************************
 *
 * @file ipc_compat.h
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */

#ifndef _IPC_H_
#define _IPC_H_

#ifdef THREADX
#include "uart.h"
#else
// #include <kernel.h>
#endif

#define __INLINE static __attribute__((__always_inline__)) inline

#define __ALIGN4 __aligned(4)

#ifdef THREADX
#define ASSERT_ERR(condition)                                                           \
    do {                                                                                \
        if ((!(condition))) {                                                  \
            uart_printf("%s:%d:ASSERT_ERR(" #condition ")\n", __FILE__,  __LINE__);          \
            while(1); \
        }                                                                               \
    } while(0)

#else
#define ASSERT_ERR(condition)                                                           \
    do {                                                                                \
        if ((!(condition))) {                                                  \
            while(1); \
        }                                                                               \
    } while(0)


#endif

#endif /* _IPC_H_ */
