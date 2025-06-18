/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <stdio.h>
#define THREAD_PRIORITY 25
#define THREAD_STACK_SIZE 1024
#define THREAD_TIMESLICE 5
static rt_thread_t tid1 = RT_NULL;
static rt_uint32_t thread_count = 0;

rt_uint32_t get_thread_count(void)
{
    printf("thread_count:%d\n", thread_count);
    return thread_count;
}


/* 线程1的入口函数 */
static void thread_entry(void *parameter)
{
    // rt_uint32_t count = 0;
    while (1)
    {
        thread_count++;
        rt_thread_mdelay(1000);
    }
}

/* 线程示例 */
int test_thread(void)
{
    /* 创建线程1，名称是thread1，入口是thread1_entry*/
    tid1 = rt_thread_create("thread1",
                            thread_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);

    /* 如果获得线程控制块，启动这个线程 */
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);

    return 0;
}