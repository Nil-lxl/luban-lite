/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_OSAL_FREERTOS_H_
#define _ARTINCHIP_AIC_OSAL_FREERTOS_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <string.h>
#include <ucos_ii.h>

#include <aic_errno.h>
#include <aic_tlsf.h>

//--------------------------------------------------------------------+
// Interrupt Define
//--------------------------------------------------------------------+

static inline int aicos_in_irq(void)
{
    return OSIntNesting;
}

//--------------------------------------------------------------------+
// Timeout Define
//--------------------------------------------------------------------+

#define AICOS_WAIT_FOREVER (-1)

//--------------------------------------------------------------------+
// Mem API
//--------------------------------------------------------------------+
static inline void *aicos_malloc(unsigned int mem_type, size_t size)
{
    return aic_tlsf_malloc(mem_type, size);
}

static inline void aicos_free(unsigned int mem_type, void *mem)
{
    aic_tlsf_free(mem_type, mem);
}

static inline void *aicos_malloc_align(uint32_t mem_type, size_t size, size_t align)
{
    return aic_tlsf_malloc_align(mem_type, size, align);
}

static inline void aicos_free_align(uint32_t mem_type, void *mem)
{
    aic_tlsf_free_align(mem_type, mem);
}

static inline void *aicos_memdup(unsigned int mem_type, void *src, size_t size)
{
    void *p;

    p = aic_tlsf_malloc(mem_type, size);
    if (p)
        memcpy(p, src, size);
    return p;
}

static inline void *aicos_memdup_align(unsigned int mem_type, void *src, size_t size, size_t align)
{
    void *p;

    p = aic_tlsf_malloc_align(mem_type, size, align);
    if (p)
        memcpy(p, src, size);
    return p;
}

//--------------------------------------------------------------------+
// Thread API
//--------------------------------------------------------------------+

static inline INT32U aicos_ucos_ms2tick(INT32U ms)
{
    return ms * OS_TICKS_PER_SEC / 1000;
}

extern aicos_thread_t ucos_thread_array[];

/* Notes:
 * if any thread is created by 'aicos_thread_create', it must be deleted by 'aicos_thread_delete'
 * if any thread is created by 'OSTaskCreateExt' or 'OSTaskCreate', it must be deleted by 'OSTaskDel'
 */
static inline aicos_thread_t aicos_thread_create(const char *name, uint32_t stack_size, uint32_t prio, aic_thread_entry_t entry, void *args)
{
    INT8U *stack_top_plus_regsize;
    OS_STK *stack;
    INT8U ret;

    stack_top_plus_regsize = aicos_malloc_align(MEM_DEFAULT, stack_size + sizeof(void *), sizeof(void *));
    if (stack_top_plus_regsize == NULL)
        return NULL;

    *stack_top_plus_regsize = prio;
    stack = (OS_STK *)(stack_top_plus_regsize + sizeof(void *));

    ret = OSTaskCreateExt(entry,
                          args,
                          (OS_STK *)(stack + stack_size),
                          (INT8U)prio,
                          (INT16U)prio,
                          stack,
                          stack_size / sizeof(OS_STK),
                          NULL,
                          OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
    if (ret != OS_ERR_NONE) {
        aicos_free(MEM_DEFAULT, stack_top_plus_regsize);
        return NULL;
    }

    ucos_thread_array[prio] = (aicos_thread_t)stack_top_plus_regsize;
    OSTaskNameSet(prio, (INT8U  *)name, &ret);

    return (aicos_thread_t)stack_top_plus_regsize;
}

static inline void aicos_thread_delete(aicos_thread_t thread)
{
    INT8U prio;
    INT8U *stack_top_plus_regsize;
#if OS_CRITICAL_METHOD == 3u                            /* Allocate storage for CPU status register    */
    OS_CPU_SR     cpu_sr = 0u;
#endif

    if (thread == NULL) {
        OS_ENTER_CRITICAL();
        prio = OSTCBCur->OSTCBPrio;
        OS_EXIT_CRITICAL();
        stack_top_plus_regsize = ucos_thread_array[prio];
    } else {
        stack_top_plus_regsize = (INT8U *)thread - sizeof(void *);
        prio = *stack_top_plus_regsize;
    }

    /* OSTaskDel is unsafe... */
    OSTaskDel(prio);
    ucos_thread_array[prio] = 0;
    aicos_free(MEM_DEFAULT, stack_top_plus_regsize);
}

static inline void aicos_thread_suspend(aicos_thread_t thread)
{
    INT8U *prio;

    if (thread == NULL)
        return;

    prio = (INT8U *)thread - sizeof(void *);

    OSTaskSuspend(*prio);
}

static inline void aicos_thread_resume(aicos_thread_t thread)
{
    INT8U *prio;

    if (thread == NULL)
        return;

    prio = (INT8U *)thread - sizeof(void *);

    OSTaskResume(*prio);
}

//--------------------------------------------------------------------+
// Counting Semaphore API
//--------------------------------------------------------------------+

static inline aicos_sem_t aicos_sem_create(uint32_t initial_count)
{
    return (aicos_sem_t)OSSemCreate(initial_count);
}

static inline void aicos_sem_delete(aicos_sem_t sem)
{
    INT8U ret;

    OSSemDel((OS_EVENT *)sem, OS_DEL_NO_PEND, &ret);
}

static inline int aicos_sem_take(aicos_sem_t sem, uint32_t msec)
{
    INT8U ret;
    INT32U tick;

    if (aicos_in_irq()) {
        return -EINVAL;
    }

    if (msec == AICOS_WAIT_FOREVER) {
        OSSemPend((OS_EVENT *)sem, 0, &ret);
    } else {
        tick = aicos_ucos_ms2tick(msec);
        if (tick == 0)
            return -EINVAL;

        OSSemPend((OS_EVENT *)sem, tick, &ret);
    }


    return (ret == OS_ERR_NONE) ? 0 : -ETIMEDOUT;
}

static inline int aicos_sem_give(aicos_sem_t sem)
{
    INT8U ret;

    ret = OSSemPost((OS_EVENT *)sem);

    return (ret == OS_ERR_NONE) ? 0 : -EINVAL;
}

//--------------------------------------------------------------------+
// Mutex API (priority inheritance)
//--------------------------------------------------------------------+

static inline aicos_mutex_t aicos_mutex_create(void)
{
    INT8U err;
    return (aicos_mutex_t)OSMutexCreate(10, &err);
}

static inline void aicos_mutex_delete(aicos_mutex_t mutex)
{
    INT8U err;
    OSMutexDel((OS_EVENT *)mutex, OS_DEL_NO_PEND, &err);
}

static inline int aicos_mutex_take(aicos_mutex_t mutex, uint32_t msec)
{
    INT8U ret;
    INT32U tick;

    if (aicos_in_irq()) {
        return -EINVAL;
    }

    if (msec == AICOS_WAIT_FOREVER) {
        OSMutexPend((OS_EVENT *)mutex, 0, &ret);
    } else {
        tick = aicos_ucos_ms2tick(msec);
        if (tick == 0)
            return -EINVAL;

        OSMutexPend((OS_EVENT *)mutex, tick, &ret);
    }


    return (ret == OS_ERR_NONE) ? 0 : -ETIMEDOUT;
}


static inline int aicos_mutex_give(aicos_mutex_t mutex)
{
    INT8U ret;

    ret = OSMutexPost((OS_EVENT *)mutex);

    return (ret == OS_ERR_NONE) ? 0 : -EINVAL;
}

//--------------------------------------------------------------------+
// Event API
//--------------------------------------------------------------------+

static inline aicos_event_t aicos_event_create(void)
{
    INT8U ret;
    return (aicos_event_t)OSFlagCreate(0, &ret);
}

static inline void aicos_event_delete(aicos_event_t event)
{
    INT8U ret;
    OSFlagDel((OS_FLAG_GRP *)event, OS_DEL_NO_PEND, &ret);
}

static inline int aicos_event_recv(aicos_event_t event, uint32_t set, uint32_t *recved, uint32_t msec)
{
    INT8U ret;
    INT32U tick;

    if (msec == AICOS_WAIT_FOREVER) {
        *recved = (uint32_t)OSFlagPend((OS_FLAG_GRP *)event, (OS_FLAGS)set, OS_FLAG_WAIT_SET_ANY, 0, &ret);
    } else {
        tick = aicos_ucos_ms2tick(msec);
        if (tick == 0)
            return -EINVAL;

        *recved = (uint32_t)OSFlagPend((OS_FLAG_GRP *)event, (OS_FLAGS)set, OS_FLAG_WAIT_SET_ANY, tick, &ret);
    }

    return (ret == OS_ERR_NONE) ? 0 : -EINVAL;
}

static inline int aicos_event_send(aicos_event_t event, uint32_t set)
{
    INT8U ret;

    OSFlagPost((OS_FLAG_GRP *)event, set, OS_FLAG_SET, &ret);

    return (ret == OS_ERR_NONE) ? 0 : -EINVAL;
}

//--------------------------------------------------------------------+
// Queue API
//--------------------------------------------------------------------+

typedef struct {
    void *mem;          /* payload memory */
    OS_MEM *mem_pool;   /* payload pool */
    OS_EVENT *qcb;      /* ucos queue control block */
    void **qcb_pool;    /* ucos qcb pool */
    uint32_t item_size; /* item size */
} ucos_qm_t;

static inline aicos_queue_t aicos_queue_create(uint32_t item_size, uint32_t queue_size)
{
    ucos_qm_t *qm;
    INT8U ret;

    // todo: is item_size nessery to be aligned?

    qm = aicos_malloc(MEM_DEFAULT, sizeof(ucos_qm_t));
    if (!qm)
        return NULL;

    qm->mem = aicos_malloc(MEM_DEFAULT, item_size * queue_size);
    if (!qm->mem) {
        aicos_free(MEM_DEFAULT, qm);
        return NULL;
    }
    qm->qcb_pool = aicos_malloc(MEM_DEFAULT, queue_size * sizeof(void *));
    if (!qm->qcb_pool) {
        aicos_free(MEM_DEFAULT, qm->mem);
        aicos_free(MEM_DEFAULT, qm);
        return NULL;
    }

    qm->mem_pool = OSMemCreate(qm->mem, queue_size, item_size, &ret);
    if (ret != OS_ERR_NONE) {
        aicos_free(MEM_DEFAULT, qm->qcb_pool);
        aicos_free(MEM_DEFAULT, qm->mem);
        aicos_free(MEM_DEFAULT, qm);
    }

    qm->qcb = OSQCreate(qm->qcb_pool, queue_size);
    if (!qm->qcb) {
        aicos_free(MEM_DEFAULT, qm->qcb_pool);
        aicos_free(MEM_DEFAULT, qm->mem);
        aicos_free(MEM_DEFAULT, qm);
    }

    qm->item_size = item_size;

    return (aicos_queue_t)qm;
}

static inline void aicos_queue_delete(aicos_queue_t queue)
{
    ucos_qm_t *qm = (ucos_qm_t *)queue;
    INT8U ret;

    if (queue == NULL)
        return;

    OSQDel(qm->qcb, OS_DEL_NO_PEND, &ret);
    aicos_free(MEM_DEFAULT, qm->qcb_pool);
    aicos_free(MEM_DEFAULT, qm->mem);
    aicos_free(MEM_DEFAULT, qm);
}

static inline int aicos_queue_send(aicos_queue_t queue, void const *buff)
{
    ucos_qm_t *qm = (ucos_qm_t *)queue;
    INT8U ret;
    void *mem_temp;

    if ((queue == NULL) || (buff == NULL))
        return -EINVAL;

    mem_temp = OSMemGet(qm->mem_pool, &ret);
    if (ret != OS_ERR_NONE || mem_temp == NULL) {
        return -ENOMEM;
    }

    memcpy(mem_temp, buff, qm->item_size);

    ret = OSQPost(qm->qcb, mem_temp);
    if (ret != OS_ERR_NONE) {
        OSMemPut(qm->mem_pool, mem_temp);
        return -EINVAL;
    }

    return 0;
}

static inline int aicos_queue_receive(aicos_queue_t queue, void *buff, uint32_t msec)
{
    ucos_qm_t *qm = (ucos_qm_t *)queue;
    INT8U ret;
    void *mem_temp;
    INT32U tick;

    if ((queue == NULL) || (buff == NULL))
        return -EINVAL;

    if (msec == AICOS_WAIT_FOREVER) {
        tick = 0;
    } else {
        tick = aicos_ucos_ms2tick(msec);
        if (tick == 0)
            return -EINVAL;
    }

    mem_temp = OSQPend(qm->qcb, tick, &ret);
    if ((ret != OS_ERR_NONE) || (mem_temp == NULL)) {
        return -ETIMEDOUT;
    }

    memcpy(buff, mem_temp, qm->item_size);
    OSMemPut(qm->mem_pool, mem_temp);

    return 0;
}

static inline int aicos_queue_empty(aicos_queue_t queue)
{
    OS_Q_DATA q_data;
    ucos_qm_t *qm = (ucos_qm_t *)queue;
    INT8U ret;

    if (queue == NULL)
        return -EINVAL;

    ret = OSQQuery(qm->qcb, &q_data);
    if (OS_ERR_NONE != ret) {
        return -EINVAL;
    }

    return (q_data.OSNMsgs == 0);
}

//--------------------------------------------------------------------+
// Wait Queue API
//--------------------------------------------------------------------+
typedef struct
{
  unsigned int set;
  aicos_event_t event;
}osal_wqueue_def_t;

typedef osal_wqueue_def_t* osal_wqueue_t;

static inline aicos_wqueue_t aicos_wqueue_create(void)
{
    osal_wqueue_t wqueue;

    wqueue = aicos_malloc(0, sizeof(osal_wqueue_t));
    if (NULL == wqueue)
        return NULL;

    wqueue->event = aicos_event_create();
    wqueue->set = 0;

    return (aicos_wqueue_t)wqueue;
}

static inline void aicos_wqueue_delete(aicos_wqueue_t wqueue)
{
    osal_wqueue_t wq = (osal_wqueue_t)wqueue;
    aicos_event_t event = (aicos_event_t)wq->event;

    aicos_event_delete(event);
    aicos_free(0, wq);
}

static inline void aicos_wqueue_wakeup(aicos_wqueue_t wqueue)
{
    osal_wqueue_t wq = (osal_wqueue_t)wqueue;
    aicos_event_t event = (aicos_event_t)wq->event;

    if (wq->set != 1)
        return;

    wq->set = 0;
    aicos_event_send(event, 0x1);
}

static inline int aicos_wqueue_wait(aicos_wqueue_t wqueue, uint32_t msec)
{
    osal_wqueue_t wq = (osal_wqueue_t)wqueue;
    aicos_event_t event = (aicos_event_t)wq->event;
    uint32_t recved;

    wq->set = 1;
    return aicos_event_recv(event, 0x1, &recved, 100);
}

//--------------------------------------------------------------------+
// Critical API
//--------------------------------------------------------------------+

static inline size_t aicos_enter_critical_section(void)
{
    return OS_CPU_SR_Save();
}

static inline void aicos_leave_critical_section(size_t flag)
{
    OS_CPU_SR_Restore(flag);
}

//--------------------------------------------------------------------+
// Sleep API
//--------------------------------------------------------------------+

static inline void aicos_msleep(uint32_t delay)
{
    OSTimeDly(aicos_ucos_ms2tick(delay));
}

#ifdef __cplusplus
 }
#endif
#endif
