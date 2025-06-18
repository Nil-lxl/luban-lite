/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "thread_pool.h"

struct thread_pool_task {
    void (*func)(void *);
    void *arg;
};
typedef struct thread_pool_task thread_pool_task_t;

static void thread_pool_worker(void *arg)
{
    thread_pool_t *pool = (thread_pool_t *)arg;
    thread_pool_task_t task;

    while (1) {
        if (rt_mq_recv(pool->task_mq, &task, sizeof(thread_pool_task_t), RT_WAITING_FOREVER) == RT_EOK) {
            if (task.func == NULL)
                break;
            task.func(task.arg);
        }
    }
}

thread_pool_t *thread_pool_create(int thread_count)
{
    thread_pool_t *pool = rt_malloc(sizeof(thread_pool_t));
    if (!pool) return NULL;

    pool->task_mq = rt_mq_create("tpool_mq",
                               2048,
                               sizeof(thread_pool_task_t),
                               RT_IPC_FLAG_FIFO);
    if (!pool->task_mq) {
        return NULL;
    }

    pool->threads = rt_malloc(sizeof(rt_thread_t) * thread_count);
    pool->thread_count = thread_count;
    char tname[64] = {0};
    for (int i = 0; i < thread_count; i++) {
        rt_snprintf(tname, sizeof(tname), "tpool%d", i);
        pool->threads[i] = rt_thread_create(tname,
                                          thread_pool_worker,
                                          pool,
                                          16384,
                                          20,
                                          5);
        if (pool->threads[i]) {
            rt_thread_startup(pool->threads[i]);
        }
    }

    return pool;
}

int thread_pool_add(thread_pool_t *pool, void (*func)(void *), void *arg)
{
    thread_pool_task_t task = {
        .func = func,
        .arg = arg
    };

    return rt_mq_send(pool->task_mq, &task, sizeof(thread_pool_task_t));
}

void thread_pool_destroy(thread_pool_t *pool)
{
    if (!pool) return;

    thread_pool_task_t exit_task = {0};
    for (int i = 0; i < pool->thread_count; i++) {
        rt_mq_send(pool->task_mq, &exit_task, sizeof(thread_pool_task_t));
    }

    for (int i = 0; i < pool->thread_count; i++) {
        rt_thread_delete(pool->threads[i]);
    }

    rt_mq_delete(pool->task_mq);
    rt_free(pool->threads);
    rt_free(pool);
}
