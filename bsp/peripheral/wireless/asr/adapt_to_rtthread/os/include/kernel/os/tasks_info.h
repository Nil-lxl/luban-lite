#ifndef TASKS_INFO_H
#define TASKS_INFO_H

typedef enum  {
    ASR_OS_PRIORITY_LOW             = 10,
    ASR_OS_PRIORITY_BELOW_NORMAL    = 9,
    ASR_OS_PRIORITY_NORMAL          = 8,
    ASR_OS_PRIORITY_ABOVE_NORMAL    = 7,
    ASR_OS_PRIORITY_HIGH            = 6,
    ASR_OS_PRIORITY_REAL_TIME       = 4,
} ASR_OS_Priority;

#define     UWIFI_AT_TASK_NAME                "AT_task"
#define     UWIFI_AT_TASK_PRIORITY            (ASR_OS_PRIORITY_LOW)                   //(ASR_OS_PRIORITY_LOW)  (244)
#define     UWIFI_AT_TASK_STACK_SIZE          (2048 - 256)

#define     UWIFI_TASK_NAME                   "UWIFI_TASK"
#define     UWIFI_TASK_PRIORITY               (ASR_OS_PRIORITY_NORMAL)                // ASR_OS_PRIORITY_HIGH  (63)
#define     UWIFI_TASK_STACK_SIZE             (6656 - 512 - 1024 + 1024*12)           //(8192 - 768 - 512)

#define     UWIFI_SDIO_TASK_NAME              "UWIFI_SDIO_TASK"
#define     UWIFI_SDIO_TASK_PRIORITY          (ASR_OS_PRIORITY_ABOVE_NORMAL)          // ASR_OS_PRIORITY_ABOVE_NORMAL (64)
#define     UWIFI_SDIO_TASK_STACK_SIZE        (5888 - 1024 - 512 - 768 + 1024*12)

#define     UWIFI_RX_TO_OS_TASK_NAME          "UWIFI_RX_TO_OS_TASK"
#define     UWIFI_RX_TO_OS_TASK_PRIORITY      (ASR_OS_PRIORITY_HIGH)                  // ASR_OS_PRIORITY_NORMAL (65)
#define     UWIFI_RX_TO_OS_TASK_STACK_SIZE    (2560 - 512 + 1024*4)                   //(2048*2 - 768 - 512)

#define     IPERF_TASK_NAME                   "ASR_IPERF"
#define     IPERF_TASK_PRIORITY               (ASR_OS_PRIORITY_HIGH)
#define     IPERF_TASK_STACK_SIZE             (4 * 1024)

#define     PING_TASK_NAME                    "ASR_PING"
#define     PING_TASK_PRIORITY                (ASR_OS_PRIORITY_LOW)
#define     PING_TASK_STACK_SIZE              (2 * 1024)

#endif
