#ifndef ATBM_OS_THREAD_H
#define ATBM_OS_THREAD_H
#include "atbm_type.h"
#include <rtthread.h>

#define WIFI_THREAD_PRIO_START 10
#define BH_TASK_PRIO (WIFI_THREAD_PRIO_START)	 /** High **/
#define HIF_TASK_PRIO (WIFI_THREAD_PRIO_START + 1)
#define WORK_TASK_PRIO (WIFI_THREAD_PRIO_START + 2)
#define ELOOP_TASK_PRIO (WIFI_THREAD_PRIO_START + 3)
#define TEST_TASK_PRIO (WIFI_THREAD_PRIO_START + 4)

//#define TXURB_TASK_PRIO (WIFI_THREAD_PRIO_START + 1)
//#define RXURB_TASK_PRIO (WIFI_THREAD_PRIO_START + 2)
#define TX_BH_TASK_PRIO (WIFI_THREAD_PRIO_START)
#define RX_BH_TASK_PRIO (WIFI_THREAD_PRIO_START)

#define BLE_XMIT_PRIO (WIFI_THREAD_PRIO_START + 4)
#define BLE_RECV_PRIO (WIFI_THREAD_PRIO_START + 4)

typedef rt_thread_t pAtbm_thread_t;

pAtbm_thread_t atbm_createThread(char *name,atbm_void(*task)(atbm_void *p_arg),atbm_void *p_arg,int prio);
int atbm_stopThread(pAtbm_thread_t thread_id);
int atbm_ThreadStopEvent(pAtbm_thread_t thread_id);


#endif /* ATBM_OS_THREAD_H */
