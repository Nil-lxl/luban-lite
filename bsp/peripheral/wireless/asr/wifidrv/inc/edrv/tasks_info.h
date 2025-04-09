#ifndef TASKS_INFO_H
#define TASKS_INFO_H

#ifndef THREADX
// zephyr os.
typedef enum  {
    #ifdef JL_SDK
    // freertos
    ASR_OS_PRIORITY_LOW             = 26,
    ASR_OS_PRIORITY_BELOW_NORMAL    = 27,
    ASR_OS_PRIORITY_NORMAL          = 28,
    ASR_OS_PRIORITY_ABOVE_NORMAL    = 29,
    ASR_OS_PRIORITY_HIGH            = 30,
    ASR_OS_PRIORITY_REAL_TIME       = 31,
    #else
    ASR_OS_PRIORITY_LOW             = 9,
    ASR_OS_PRIORITY_BELOW_NORMAL    = 8,
    ASR_OS_PRIORITY_NORMAL             = 7,
    ASR_OS_PRIORITY_ABOVE_NORMAL    = 6,
    ASR_OS_PRIORITY_HIGH            = 5,
    ASR_OS_PRIORITY_REAL_TIME       = 0,
    #endif
} ASR_OS_Priority;
#else
// theadx os.
typedef enum  {
    ASR_OS_PRIORITY_LOW             = 244,
    ASR_OS_PRIORITY_BELOW_NORMAL    = 66,
    ASR_OS_PRIORITY_NORMAL             = 65,
    ASR_OS_PRIORITY_ABOVE_NORMAL    = 64,
    ASR_OS_PRIORITY_HIGH            = 63,
    ASR_OS_PRIORITY_REAL_TIME       = 10,
} ASR_OS_Priority;
#endif

#ifdef ALIOS_SUPPORT
// alios
//#define     LWIP_DHCP_TASK_NAME                 "dhcp"
//#define     LWIP_DHCP_TASK_PRIORITY             29
//#define     LWIP_DHCP_TASK_STACK_SIZE           1536

//#define     UWIFI_RX_TASK_NAME                  "UWIFI_RX_TASK"
//#define     UWIFI_RX_TASK_PRIORITY              15
//#define     UWIFI_RX_TASK_STACK_SIZE            4096 //7168

//#define     UWIFI_TASK_NAME                     "UWIFI_TASK"
//#define     UWIFI_TASK_PRIORITY                 20
//#define     UWIFI_TASK_STACK_SIZE               7168 //5120

//#define     LWIFI_TASK_THREAD_NAME              "LWIFI_TASK"
//#define     LWIFI_TASK_PRIORITY                 17
//#define     LWIFI_TASK_STACK_SIZE               2048



// copy from threadx
#define     UWIFI_TASK_NAME                   "UWIFI_TASK"
#define     UWIFI_TASK_PRIORITY               (ASR_OS_PRIORITY_HIGH)                // ASR_OS_PRIORITY_HIGH  (63)
#define     UWIFI_TASK_STACK_SIZE             (6656 - 512 - 1024 + 1024*12)           //(8192 - 768 - 512)

#define     UWIFI_SDIO_TASK_NAME              "UWIFI_SDIO_TASK"
#define     UWIFI_SDIO_TASK_PRIORITY          (ASR_OS_PRIORITY_ABOVE_NORMAL)          // ASR_OS_PRIORITY_ABOVE_NORMAL (64)
#define     UWIFI_SDIO_TASK_STACK_SIZE        (5888 - 1024 - 512 - 768 + 1024*12)

#define     UWIFI_RX_TO_OS_TASK_NAME          "UWIFI_RX_TO_OS_TASK"
#define     UWIFI_RX_TO_OS_TASK_PRIORITY      (ASR_OS_PRIORITY_NORMAL)                  // ASR_OS_PRIORITY_NORMAL (65)
#define     UWIFI_RX_TO_OS_TASK_STACK_SIZE    (2560 - 512 + 1024*4)                   //(2048*2 - 768 - 512)


/******** MACRO for AT cli ************/
#define     UWIFI_AT_TASK_NAME                "AT_task"
#define     UWIFI_AT_TASK_PRIORITY            (30)
#define     UWIFI_AT_TASK_STACK_SIZE          (2048 - 256)

#define     UART_TASK_NAME                "UART_task"
#define     UART_TASK_PRIORITY            (19)
#define     UART_TASK_STACK_SIZE          (2048)

#else

// threadx or zephyr

#define     DOUB_85_SC_TASK_NAME              "doub_85_sc_task"
#define     DOUB_85_SC_TASK_PRIORITY          (ASR_OS_PRIORITY_LOW)
#define     DOUB_85_SC_TASK_STACK_SIZE        2048

#define     UWIFI_AT_TASK_NAME                "AT_task"
#define     UWIFI_AT_TASK_PRIORITY            (ASR_OS_PRIORITY_LOW)                   //(ASR_OS_PRIORITY_LOW)  (244)
#define     UWIFI_AT_TASK_STACK_SIZE          (2048 - 256)

#define     UWIFI_AT_UART_RCV_TASK_NAME         "AT_UART_RCV_task"
#define     UWIFI_AT_UART_RCV_TASK_STACK_SIZE   (4096 - 256)
#define     UWIFI_AT_UART_RCV_TASK_PRIORITY     (ASR_OS_PRIORITY_BELOW_NORMAL)        //(ASR_OS_PRIORITY_BELOW_NORMAL)  (244)

#ifdef THREADX
// threadx
#define     UWIFI_TASK_NAME                   "UWIFI_TASK"
#define     UWIFI_TASK_PRIORITY               (ASR_OS_PRIORITY_HIGH)                // ASR_OS_PRIORITY_HIGH  (63)
#define     UWIFI_TASK_STACK_SIZE             (6656 - 512 - 1024 + 1024*12)           //(8192 - 768 - 512)

#define     UWIFI_SDIO_TASK_NAME              "UWIFI_SDIO_TASK"
#define     UWIFI_SDIO_TASK_PRIORITY          (ASR_OS_PRIORITY_ABOVE_NORMAL)          // ASR_OS_PRIORITY_ABOVE_NORMAL (64)
#define     UWIFI_SDIO_TASK_STACK_SIZE        (5888 - 1024 - 512 - 768 + 1024*12)

#define     UWIFI_RX_TO_OS_TASK_NAME          "UWIFI_RX_TO_OS_TASK"
#define     UWIFI_RX_TO_OS_TASK_PRIORITY      (ASR_OS_PRIORITY_NORMAL)                  // ASR_OS_PRIORITY_NORMAL (65)
#define     UWIFI_RX_TO_OS_TASK_STACK_SIZE    (2560 - 512 + 1024*4)                   //(2048*2 - 768 - 512)

#else
// zephyr

#define     UWIFI_TASK_NAME                   "UWIFI_TASK"
#define     UWIFI_TASK_PRIORITY               (ASR_OS_PRIORITY_NORMAL)                // ASR_OS_PRIORITY_HIGH  (63)
#define     UWIFI_TASK_STACK_SIZE             (6656 - 512 - 1024 + 1024*12)           //(8192 - 768 - 512)

#define     UWIFI_SDIO_TASK_NAME              "UWIFI_SDIO_TASK"
#define     UWIFI_SDIO_TASK_PRIORITY          (ASR_OS_PRIORITY_ABOVE_NORMAL)          // ASR_OS_PRIORITY_ABOVE_NORMAL (64)
#define     UWIFI_SDIO_TASK_STACK_SIZE        (5888 - 1024 - 512 - 768 + 1024*12)

#define     UWIFI_RX_TO_OS_TASK_NAME          "UWIFI_RX_TO_OS_TASK"
#define     UWIFI_RX_TO_OS_TASK_PRIORITY      (ASR_OS_PRIORITY_HIGH)                  // ASR_OS_PRIORITY_NORMAL (65)
#define     UWIFI_RX_TO_OS_TASK_STACK_SIZE    (2560 - 512 + 1024*4)                   //(2048*2 - 768 - 512)
#endif

#endif
#endif
