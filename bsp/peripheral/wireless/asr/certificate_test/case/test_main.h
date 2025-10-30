#ifndef TEST_MAIN_H
#define TEST_MAIN_H

#include <stdio.h>
#include <stdlib.h>

#include "asr_rtos_api.h"
#include "cutest/cut.h"

#ifndef SYSINFO_ARCH
#define SYSINFO_ARCH        "ARMV7"
#endif
#ifndef SYSINFO_MCU
#define SYSINFO_MCU         "STM32H7"
#endif
#ifndef SYSINFO_DEVICE_NAME
#define SYSINFO_DEVICE_NAME "ASR"
#endif
#ifndef SYSINFO_APP_VERSION
#define SYSINFO_APP_VERSION "1.0"
#endif
#define SYSINFO_KERNEL      "ASR OSAL"

/* dynamic memory alloc test */
#define TEST_CONFIG_MM_ENABLED                  (1)
#define TEST_CONFIG_MALLOC_MAX_SIZE             (1024)
#define TEST_CONFIG_MALLOC_FREE_TIMES           (100000)

/* task test */
#define TEST_CONFIG_TASK_ENABLED                (1)
#if (TEST_CONFIG_TASK_ENABLED > 0)
#ifndef TEST_CONFIG_STACK_SIZE
#define TEST_CONFIG_STACK_SIZE                  (512)
#define TEST_CONFIG_TASK_PRIO                   (2)
#endif
#define TEST_CONFIG_MAX_TASK_COUNT              (10)
#define TEST_CONFIG_CREATE_TASK_TIMES           (10000)
#endif

/* task communication test */
#define TEST_CONFIG_TASK_COMM_ENABLED           (1)
#if (TEST_CONFIG_TASK_COMM_ENABLED > 0)
#ifndef TEST_CONFIG_STACK_SIZE
#define TEST_CONFIG_STACK_SIZE                  (512)
#endif
#define TEST_CONFIG_SYNC_TIMES                  (100000)
#define TEST_CONFIG_QUEUE_BUF_SIZE              (32)
#endif

/* timer test */
#define TEST_CONFIG_TIMER_ENABLED               (1)

/* kv test */
#define TEST_CONFIG_KV_ENABLED                  (0)
#if (TEST_CONFIG_KV_ENABLED > 0)
#define TEST_CONFIG_KV_TIMES                    (10000)
#endif

#define TEST_PRINT(fmt, args...)            printf(fmt"\r\n", ##args)

#endif //TEST_MAIN_H