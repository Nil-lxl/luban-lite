/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: mpp thread interface
 */

#include "mpp_thread.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include "rtconfig.h"

#if defined(KERNEL_RTTHREAD) || \
    defined(KERNEL_FREERTOS) || \
    defined(KERNEL_BAREMETAL) || \
    defined(KERNEL_RHINO) || \
    defined(KERNEL_UCOS_II)
#define MPP_USING_AIC_OSAL
#endif


#if defined(MPP_USING_AIC_OSAL)
#include "aic_osal.h"
struct mpp_sem_handle {
    aicos_sem_t sem;
};
#else
#include <pthread.h>
struct mpp_sem_handle {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_condattr_t condattr;
    int need_signal;
};
#endif

mpp_sem_t mpp_sem_create()
{
    struct mpp_sem_handle *h_sem =
        (struct mpp_sem_handle *)mpp_alloc(sizeof(struct mpp_sem_handle));
    if (h_sem == NULL)
        return NULL;
    memset(h_sem, 0, sizeof(struct mpp_sem_handle));

#if defined(MPP_USING_AIC_OSAL)
    h_sem->sem = aicos_sem_create(0);
    if (!h_sem->sem) {
        loge("aicos_sem_create err!!!\n");
        return NULL;
    }
#else
    h_sem->need_signal = 0;
    if (pthread_mutex_init(&h_sem->mutex, NULL) != 0) {
        loge("pthread_mutex_init err!!!\n");
        return NULL;
    }

    pthread_condattr_init(&h_sem->condattr);
    pthread_condattr_setclock(&h_sem->condattr, CLOCK_REALTIME);
    pthread_cond_init(&h_sem->cond, &h_sem->condattr);
#endif
    return (mpp_sem_t)h_sem;
}

void mpp_sem_delete(mpp_sem_t sem)
{
    if (sem) {
        struct mpp_sem_handle *h_sem = (struct mpp_sem_handle *)sem;
#if defined(MPP_USING_AIC_OSAL)
        aicos_sem_delete(h_sem->sem);
        h_sem->sem = NULL;
#else
        pthread_condattr_destroy(&h_sem->condattr);
        pthread_cond_destroy(&h_sem->cond);
        pthread_mutex_destroy(&h_sem->mutex);
#endif
        mpp_free(sem);
    }
}

int mpp_sem_signal(mpp_sem_t sem)
{
    if (!sem)
        return -1;
    struct mpp_sem_handle *h_sem = (struct mpp_sem_handle *)sem;
#if defined(MPP_USING_AIC_OSAL)
    aicos_sem_give(h_sem->sem);
#else
    pthread_mutex_lock(&h_sem->mutex);
    if (h_sem->need_signal) {
        pthread_cond_signal(&h_sem->cond);
    }
    pthread_mutex_unlock(&h_sem->mutex);
#endif
    return 0;
}

int mpp_sem_wait(mpp_sem_t sem, int64_t us)
{
    if (!sem)
        return -1;

    struct mpp_sem_handle *h_sem = (struct mpp_sem_handle *)sem;

#if defined(MPP_USING_AIC_OSAL)
    if (us == MPP_WAIT_FOREVER)
        aicos_sem_take(h_sem->sem, AICOS_WAIT_FOREVER);
    else
        aicos_sem_take(h_sem->sem, (unsigned int)(us * 1000));
    return 0;
#else
    int ret;
    pthread_mutex_lock(&h_sem->mutex);
    if (us == MPP_WAIT_FOREVER) {
        h_sem->need_signal = 1;
        ret = pthread_cond_wait(&h_sem->cond, &h_sem->mutex);
        h_sem->need_signal = 0;
    } else {
        struct timespec out_time;
        uint64_t tmp;
        ret = clock_gettime(CLOCK_REALTIME, &out_time);
        out_time.tv_sec += us / (1 * 1000 * 1000);
        tmp = out_time.tv_nsec / 1000 + us % (1 * 1000 * 1000);
        out_time.tv_sec += tmp / (1 * 1000 * 1000);
        tmp = tmp % (1 * 1000 * 1000);
        out_time.tv_nsec = tmp * 1000;
        h_sem->need_signal = 1;
        ret = pthread_cond_timedwait(&h_sem->cond, &h_sem->mutex, &out_time);
        h_sem->need_signal = 0;
    }

    pthread_mutex_unlock(&h_sem->mutex);
    return ret;
#endif
}
