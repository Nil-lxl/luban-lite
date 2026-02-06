#include "os_porting.h"
#include <rtthread.h>

int os_msgqueue_init(uintptr_t *queue_hdl, int elem_num)
{
    rt_mailbox_t q = NULL;
    static unsigned char index = 0;
    char queue_name[16] = {0};

    snprintf(queue_name, sizeof(queue_name), "hgq_%d", index);
    index ++;
    q = rt_mb_create(queue_name, elem_num, RT_IPC_FLAG_FIFO);
    if (q == NULL) {
        hgic_err("Create msgqueue failed\n");
        return -1;
    } else {
        if (queue_hdl) {
            *queue_hdl = (uintptr_t)q;
        }
        return 0;
    }
}

void *os_msgqueue_recv(uintptr_t queue_hdl, int millisec)
{
    rt_mailbox_t q = (rt_mailbox_t)queue_hdl;
    void *msg = NULL;
    int ret = 0;

    if (q == NULL) {
        hgic_err("Input param error!\n");
        return NULL;
    }
    if (millisec == osWaitForever) {
        ret = rt_mb_recv(q, &msg, RT_WAITING_FOREVER);
    } else {
        ret = rt_mb_recv(q, &msg, rt_tick_from_millisecond(millisec));
    }
    if (ret == RT_EOK) {
        return (void *)msg;
    } else {
        //hgic_err("Recv timeout!\n");
        return NULL;
    }
}

int os_msgqueue_send(uintptr_t queue_hdl, void *data, int millisec)
{
    rt_mailbox_t q = (rt_mailbox_t)queue_hdl;
    int ret = 0;

    if (q == NULL) {
        hgic_err("Input param error!\n");
        return -1;
    }
    return rt_mb_send(q, data);
}

void os_msgqueue_del(uintptr_t queue_hdl)
{
    rt_mailbox_t q = (rt_mailbox_t)queue_hdl;
    int ret = 0;

    if (q == NULL) {
        hgic_err("Input param error!\n");
        return;
    }
    rt_mb_delete(q);
}

int os_msgqueue_get_count(uintptr_t queue_hdl)
{
    rt_mailbox_t q = (rt_mailbox_t)queue_hdl;

    if (q == NULL) {
        hgic_err("Input param error!\n");
        return 0;
    }
    return q->entry;
}

