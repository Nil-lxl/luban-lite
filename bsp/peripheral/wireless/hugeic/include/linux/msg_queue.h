
#ifndef _LINUX_MSGQ_H_
#define _LINUX_MSGQ_H_

#if defined(__MBED__)
#include "os/mbed/msg_queue.h"
#elif defined(__WINDOWS__)
#include "os/win/msg_queue.h"
#elif defined(__LINUX__)
#include "os/linux/msg_queue.h"
#elif defined(__CSKY__)
#include "os/c-sky/msg_queue.h"
#else
//#include "porting/msg_queue.h"
#endif

#define MSG_Q_WAIT_FOROVER 0xFFFFFFFF

struct msg_queue {
    unsigned int q;
    unsigned int vaild;
};

int msg_queue_init(struct msg_queue *msgq, int size);
int msg_queue_get(struct msg_queue *msgq, int millisec);
int msg_queue_put(struct msg_queue *msgq, int data, int millisec);
int msg_queue_del(struct msg_queue *msgq);
int msg_queue_count(struct msg_queue *msgq);

#endif

