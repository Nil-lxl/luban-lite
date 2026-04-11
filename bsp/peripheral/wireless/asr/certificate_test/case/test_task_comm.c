#include "test_main.h"

static asr_queue_stuct_t  g_queue1;
static asr_queue_stuct_t  g_queue2;
static asr_queue_stuct_t  g_queue3;
static asr_semaphore_t    g_sem;
static asr_queue_stuct_t  g_queue;
static asr_timer_t  g_timer;
static asr_mutex_t  g_mutex;
static asr_semaphore_t    g_sem_taskexit_sync;
static unsigned int g_var = 0;

/* task: decrease g_var with mutex*/
static void asrosal_task4(void *arg)
{
    int i = 0;

    TEST_PRINT("task name task4: decrease");
    for(i=0; i<TEST_CONFIG_SYNC_TIMES; i++) {
        asr_rtos_lock_mutex(&g_mutex);
        g_var--;
        asr_rtos_unlock_mutex(&g_mutex);
    }
    asr_rtos_set_semaphore(&g_sem_taskexit_sync);
    while(1)
    {
        asr_rtos_delay_milliseconds(100);
    }
}

/* task: increase g_var with mutex*/
static void asrosal_task5(void *arg)
{
    int i = 0;

    TEST_PRINT("task name task5: increase");
    for(i=0; i<TEST_CONFIG_SYNC_TIMES; i++) {
        asr_rtos_lock_mutex(&g_mutex);
        g_var++;
        asr_rtos_unlock_mutex(&g_mutex);
    }
    asr_rtos_set_semaphore(&g_sem_taskexit_sync);
    while(1)
    {
        asr_rtos_delay_milliseconds(100);
    }
}

/* task: decrease g_var with sem */
static void asrosal_task6(void *arg)
{
    int i = 0;

    TEST_PRINT("task name task6: decrease");
    for(i=0; i<TEST_CONFIG_SYNC_TIMES; i++) {
        asr_rtos_get_semaphore(&g_sem, -1);
        g_var--;
        asr_rtos_set_semaphore(&g_sem);
    }
    asr_rtos_set_semaphore(&g_sem_taskexit_sync);
    while(1)
    {
        asr_rtos_delay_milliseconds(100);
    }
}

/* task: increase g_var with sem */
static void asrosal_task7(void *arg)
{
    int i = 0;

    TEST_PRINT("task name task7: increase");
    for(i=0; i<TEST_CONFIG_SYNC_TIMES; i++) {
        asr_rtos_get_semaphore(&g_sem, -1);
        g_var++;
        asr_rtos_set_semaphore(&g_sem);
    }
    asr_rtos_set_semaphore(&g_sem_taskexit_sync);
    while(1)
    {
        asr_rtos_delay_milliseconds(100);
    }
}

/* task: g_queue1 -> g_queue2 */
static void asrosal_task8(void *arg)
{
    int msg = -1;
    unsigned int size = sizeof (int);

    while(1) {
        asr_rtos_pop_from_queue(&g_queue1, &msg, -1);
        asr_rtos_push_to_queue(&g_queue2, &msg, 10);
        if(msg == TEST_CONFIG_SYNC_TIMES) {
            break;
        }
    }
    while(1)
    {
        asr_rtos_delay_milliseconds(100);
    }
}

/* task: g_queue2 -> g_queue3 */
static void asrosal_task9(void *arg)
{
    int msg = -1;

    while(1) {
        asr_rtos_pop_from_queue(&g_queue2, &msg, -1);
        asr_rtos_push_to_queue(&g_queue3, &msg, 100);
        if(msg == TEST_CONFIG_SYNC_TIMES) {
            break;
        }
    }
    while(1)
    {
        asr_rtos_delay_milliseconds(100);
    }
}

#if (TEST_CONFIG_TASK_COMM_ENABLED > 0)
CASE(asrosal_test_task_comm, asrosal_1_014)
{
    OSStatus ret = -1;

    ret = asr_rtos_init_mutex(&g_mutex);
    ASSERT_NE(g_mutex, NULL);
    ASSERT_EQ(ret, kNoErr);

    // TODO:
    // ret = aos_mutex_is_valid((aos_mutex_t*)&g_mutex->mutex);
    // ASSERT_EQ(ret, 1);

    cut_printf("lock\n");
    ret = asr_rtos_lock_mutex(&g_mutex);
    ASSERT_EQ(ret, kNoErr);

    cut_printf("unlock\n");
    ret = asr_rtos_unlock_mutex(&g_mutex);
    ASSERT_EQ(ret, kNoErr);

    asr_rtos_deinit_mutex(&g_mutex);
}

CASE(asrosal_test_task_comm, asrosal_1_015)
{
    unsigned int stack_size = TEST_CONFIG_STACK_SIZE;
    char task_name[10] = {0};
    const unsigned int task_count = 4;
    int ret = -1;
    int i = 0;
    asr_thread_hand_t  task[4] = {{0}};

    ASSERT_TRUE(task_count%2 == 0);

    asr_rtos_init_semaphore(&g_sem_taskexit_sync, 0);
    asr_rtos_init_mutex(&g_mutex);
    g_var = 0;

    for(i=0; i<task_count; i++) {
        snprintf(task_name, 10, "task%d", i+1);
        if(i < task_count/2) {
            ret = asr_rtos_create_thread(&task[i], TEST_CONFIG_TASK_PRIO, task_name, asrosal_task4, stack_size, NULL);
            ASSERT_EQ(ret, kNoErr);
        }
        else {
            ret = asr_rtos_create_thread(&task[i], TEST_CONFIG_TASK_PRIO, task_name, asrosal_task5, stack_size, NULL);
            ASSERT_EQ(ret, kNoErr);
        }
        asr_rtos_delay_milliseconds(10);
    }
    for(i=0; i<task_count; i++) {
        asr_rtos_get_semaphore(&g_sem_taskexit_sync, -1);
    }
    TEST_PRINT("g_var = %d", g_var);
    ASSERT_EQ(g_var, 0);

    asr_rtos_deinit_semaphore(&g_sem_taskexit_sync);
    asr_rtos_deinit_mutex(&g_mutex);

    for (i = 0; i < task_count; i++)
        asr_rtos_delete_thread(&task[i]);
}

CASE(asrosal_test_task_comm, asrosal_1_016)
{
    int ret = -1;

    asr_rtos_init_semaphore(&g_sem, 0);
    ASSERT_NE(g_sem, NULL);

    // TODO:
    // ret = aos_sem_is_valid((aos_sem_t *)g_sem->semaphore);
    // ASSERT_EQ(ret, 1);

    ret = asr_rtos_get_semaphore(&g_sem, 100);
    ASSERT_NE(ret, 0);

    asr_rtos_set_semaphore(&g_sem);

    ret = asr_rtos_get_semaphore(&g_sem, 100);
    ASSERT_EQ(ret, 0);

    asr_rtos_deinit_semaphore(&g_sem);
}

CASE(asrosal_test_task_comm, asrosal_1_017)
{
    unsigned int stack_size = TEST_CONFIG_STACK_SIZE;
    char task_name[10] = {0};
    const unsigned int task_count = 4;
    int ret = -1;
    int i = 0;
    asr_thread_hand_t  task[4] = {{0}};

    ASSERT_TRUE(task_count%2 == 0);

    asr_rtos_init_semaphore(&g_sem_taskexit_sync, 0);
    asr_rtos_init_semaphore(&g_sem, 1);
    g_var = 0;

    for(i=0; i<task_count; i++) {
        snprintf(task_name, 10, "task%d", i+1);
        if(i < 2) {
            ret = asr_rtos_create_thread(&task[i], TEST_CONFIG_TASK_PRIO, task_name, asrosal_task6, stack_size, NULL);
            ASSERT_EQ(ret, kNoErr);
        }
        else {
            ret = asr_rtos_create_thread(&task[i], TEST_CONFIG_TASK_PRIO, task_name, asrosal_task7, stack_size, NULL);
            ASSERT_EQ(ret, kNoErr);
        }
        asr_rtos_delay_milliseconds(10);
    }
    for(i=0; i<task_count; i++) {
        asr_rtos_get_semaphore(&g_sem_taskexit_sync, -1);
    }
    TEST_PRINT("g_var = %d", g_var);
    ASSERT_EQ(g_var, 0);

    asr_rtos_deinit_semaphore(&g_sem_taskexit_sync);
    asr_rtos_deinit_semaphore(&g_sem);
    for (i = 0; i < task_count; i++)
        asr_rtos_delete_thread(&task[i]);
}

CASE(asrosal_test_task_comm, asrosal_1_018)
{
    OSStatus ret = -1;
    char msg_send[TEST_CONFIG_QUEUE_BUF_SIZE] = {0};
    char msg_recv[TEST_CONFIG_QUEUE_BUF_SIZE] = {0};

    ret = asr_rtos_init_queue(&g_queue, "g_queue", TEST_CONFIG_QUEUE_BUF_SIZE, TEST_CONFIG_QUEUE_BUF_SIZE);
    ASSERT_EQ(ret, kNoErr);

    ASSERT_EQ(asr_rtos_is_queue_valid(&g_queue), kTRUE);

    //TODO:
    // ret = aos_queue_is_valid((aos_queue_t *)((drv_mq_t)g_queue.queue)->mq);
    // ASSERT_EQ(ret, 1);

    ret = asr_rtos_pop_from_queue(&g_queue, msg_recv, 10);
    ASSERT_NE(ret, kNoErr);

    snprintf(msg_send, TEST_CONFIG_QUEUE_BUF_SIZE, "hello,queue!");
    ret = asr_rtos_push_to_queue(&g_queue, msg_send, 10);
    ASSERT_EQ(ret, kNoErr);

    memset(msg_recv, 0, TEST_CONFIG_QUEUE_BUF_SIZE);
    ret = asr_rtos_pop_from_queue(&g_queue, msg_recv, 10);
    ASSERT_EQ(ret, kNoErr);
    ASSERT_STR_EQ(msg_recv, msg_send);

    asr_rtos_deinit_queue(&g_queue);
    ASSERT_EQ(asr_rtos_is_queue_valid(&g_queue), kFALSE);
}

CASE(asrosal_test_task_comm, asrosal_1_019)
{
    OSStatus          ret = -1;
    unsigned int stack_size = 512;//TEST_CONFIG_STACK_SIZE;
    unsigned int msg_send = 0;
    unsigned int msg_recv = 0;
    unsigned int msg_size = sizeof(int);
    asr_thread_hand_t  task[2] = {{0}};
    int i = 0;

    ret = asr_rtos_init_queue(&g_queue1, "g_queue1", msg_size, TEST_CONFIG_QUEUE_BUF_SIZE);
    ASSERT_EQ(ret, kNoErr);
    ret = asr_rtos_init_queue(&g_queue2, "g_queue2", msg_size, TEST_CONFIG_QUEUE_BUF_SIZE);
    ASSERT_EQ(ret, kNoErr);
    ret = asr_rtos_init_queue(&g_queue3, "g_queue3", msg_size, TEST_CONFIG_QUEUE_BUF_SIZE);
    ASSERT_EQ(ret, kNoErr);

    ret = asr_rtos_create_thread(&task[0], TEST_CONFIG_TASK_PRIO, "asrosal_task8", asrosal_task8, stack_size, NULL);
    ASSERT_EQ(ret, kNoErr);
    ret = asr_rtos_create_thread(&task[1], TEST_CONFIG_TASK_PRIO, "asrosal_task9", asrosal_task9, stack_size, NULL);
    ASSERT_EQ(ret, kNoErr);

    for(i=1; i<=TEST_CONFIG_SYNC_TIMES; i++) {
        msg_send = i;
        ret = asr_rtos_push_to_queue(&g_queue1, &msg_send, 100);
        ASSERT_EQ(ret, kNoErr);

        ret = asr_rtos_pop_from_queue(&g_queue3, &msg_recv, -1);
        ASSERT_EQ(ret, kNoErr);
        ASSERT_EQ(msg_send, msg_recv);
        if(i%(TEST_CONFIG_SYNC_TIMES/10) == 0) {
            TEST_PRINT("%d/%d", i, TEST_CONFIG_SYNC_TIMES);
        }
    }
    ASSERT_EQ(msg_recv, TEST_CONFIG_SYNC_TIMES);
    asr_rtos_deinit_queue(&g_queue1);
    asr_rtos_deinit_queue(&g_queue2);
    asr_rtos_deinit_queue(&g_queue3);

    asr_rtos_delete_thread(&task[0]);
    asr_rtos_delete_thread(&task[1]);
}
#endif /* TEST_CONFIG_TASK_COMM_ENABLED */

#if ((TEST_CONFIG_TIMER_ENABLED > 0) && (TEST_CONFIG_TASK_COMM_ENABLED > 0))
static void task_comm_timer_handler(void *arg)
{
    unsigned int msg_send = 0x1a2a3a4a;

    TEST_PRINT("timer handler");
    asr_rtos_push_to_queue(&g_queue, &msg_send, ASR_NO_WAIT);
}

CASE(asrosal_test_task_comm, asrosal_1_020)
{
    OSStatus ret = -1;
    unsigned int msg_recv = 0;

    ret = asr_rtos_init_queue(&g_queue, "g_queue", 4, 4);
    ASSERT_EQ(ret, kNoErr);

    asr_rtos_init_timer(&g_timer, 200, task_comm_timer_handler, NULL);
    asr_rtos_start_timer(&g_timer);

    ret = asr_rtos_pop_from_queue(&g_queue, &msg_recv, ASR_WAIT_FOREVER);
    ASSERT_EQ(ret, kNoErr);

    ASSERT_EQ(0x1a2a3a4a, msg_recv);

    asr_rtos_deinit_queue(&g_queue);

    asr_rtos_deinit_timer(&g_timer);
}
#endif

/* task commication */
SUITE(asrosal_test_task_comm) = {
#if (TEST_CONFIG_TASK_COMM_ENABLED > 0)
    ADD_CASE(asrosal_test_task_comm, asrosal_1_014),
    ADD_CASE(asrosal_test_task_comm, asrosal_1_015),
    ADD_CASE(asrosal_test_task_comm, asrosal_1_016),
    ADD_CASE(asrosal_test_task_comm, asrosal_1_017),
    ADD_CASE(asrosal_test_task_comm, asrosal_1_018),
    ADD_CASE(asrosal_test_task_comm, asrosal_1_019),
#endif
#if ((TEST_CONFIG_TIMER_ENABLED > 0) && (TEST_CONFIG_TASK_COMM_ENABLED > 0))
    ADD_CASE(asrosal_test_task_comm, asrosal_1_020),
#endif
    ADD_CASE_NULL
};