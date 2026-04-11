/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <rtthread.h>
#include <rtdevice.h>

struct thread_pool {
    rt_mq_t task_mq;
    rt_thread_t *threads;
    rt_uint8_t thread_count;
};
typedef struct thread_pool thread_pool_t;

thread_pool_t *thread_pool_create(int thread_count);
int thread_pool_add(thread_pool_t *pool, void (*func)(void *), void *arg);
void thread_pool_destroy(thread_pool_t *pool);

#endif
