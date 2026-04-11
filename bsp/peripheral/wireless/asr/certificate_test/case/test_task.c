
#include "test_main.h"

static asr_thread_hand_t  g_task = {0};
static asr_semaphore_t    g_sem_taskexit_sync;
static int should_not_reach=0;

static void task0(void *arg)
{
    asr_rtos_set_semaphore(&g_sem_taskexit_sync);

    while(1)
    {
        asr_rtos_delay_milliseconds(1000);
    }
}

/* task: print task name */
static void task1(void *arg)
{
    asr_rtos_set_semaphore(&g_sem_taskexit_sync);

    while(1)
    {
        asr_rtos_delay_milliseconds(1000);
    }
}

/* task: create task key */
static void task2(void *arg)
{
    asr_rtos_set_semaphore(&g_sem_taskexit_sync);

    while(1)
    {
        asr_rtos_delay_milliseconds(1000);
    }
}

/* task: set/get task-specific value */
static void task3(void *arg)
{
    asr_rtos_set_semaphore(&g_sem_taskexit_sync);

    while(1)
    {
        asr_rtos_delay_milliseconds(1000);
    }
}

static void task4(void *arg)
{
    OSStatus ret = 0;

	TEST_PRINT("enter task name task4");

    if(g_task.thread != NULL) {
        printf("task4 exit!\n");
        ret = asr_rtos_delete_thread(&g_task);
        if (ret == kNoErr) {
            g_task.thread = NULL;
        }
    }

    while(1)
    {
        should_not_reach++;
    }
}

#if (TEST_CONFIG_TASK_ENABLED > 0)
CASE(asrosal_test_task, asrosal_1_006)
{
    unsigned int stack_size = TEST_CONFIG_STACK_SIZE;
    OSStatus ret = 0;
    asr_thread_hand_t  task = {0};

    asr_rtos_init_semaphore(&g_sem_taskexit_sync, 0);

    ret = asr_rtos_create_thread(&task, TEST_CONFIG_TASK_PRIO, "task1", task1, stack_size, NULL);
    ASSERT_EQ(ret, kNoErr);

    asr_rtos_get_semaphore(&g_sem_taskexit_sync, -1);
    TEST_PRINT("task1 exit!");

    asr_rtos_deinit_semaphore(&g_sem_taskexit_sync);

    asr_rtos_delete_thread(&task);
}

CASE(asrosal_test_task, asrosal_1_007)
{
    unsigned int stack_size = TEST_CONFIG_STACK_SIZE;
    asr_thread_hand_t  task = {0};
    int ret = -1;

    asr_rtos_init_semaphore(&g_sem_taskexit_sync, 0);

    ret = asr_rtos_create_thread(&task, TEST_CONFIG_TASK_PRIO, "task1", task1, stack_size, NULL);
    ASSERT_EQ(ret, kNoErr);

    asr_rtos_get_semaphore(&g_sem_taskexit_sync, -1);
    TEST_PRINT("task1 exit!");

    asr_rtos_deinit_semaphore(&g_sem_taskexit_sync);
    asr_rtos_delete_thread(&task);
}

CASE(asrosal_test_task, asrosal_1_008)
{
    unsigned int stack_size = TEST_CONFIG_STACK_SIZE;
    int ret = -1;
    asr_thread_hand_t  task = {0};

    asr_rtos_init_semaphore(&g_sem_taskexit_sync, 0);

    ret = asr_rtos_create_thread(&task, TEST_CONFIG_TASK_PRIO, "asrosal_1_008_task2", task2, stack_size, NULL);
    ASSERT_EQ(ret, kNoErr);

    asr_rtos_get_semaphore(&g_sem_taskexit_sync, -1);
    TEST_PRINT("asrosal_1_008_task2 exit!");

    asr_rtos_deinit_semaphore(&g_sem_taskexit_sync);
    asr_rtos_delete_thread(&task);
}

CASE(asrosal_test_task, asrosal_1_009)
{
    unsigned int stack_size = TEST_CONFIG_STACK_SIZE;
    int ret = -1;
    asr_thread_hand_t  task = {0};

    asr_rtos_init_semaphore(&g_sem_taskexit_sync, 0);

    ret = asr_rtos_create_thread(&task, TEST_CONFIG_TASK_PRIO, "asrosal_1_009_task2", task2, stack_size, NULL);
    ASSERT_EQ(ret, kNoErr);

    asr_rtos_get_semaphore(&g_sem_taskexit_sync, -1);
    TEST_PRINT("task2 exit!");

    asr_rtos_deinit_semaphore(&g_sem_taskexit_sync);
    asr_rtos_delete_thread(&task);
}

CASE(asrosal_test_task, asrosal_1_010)
{
    unsigned int stack_size = TEST_CONFIG_STACK_SIZE;
    int ret = -1;
    asr_thread_hand_t  task = {0};

    asr_rtos_init_semaphore(&g_sem_taskexit_sync, 0);

    ret = asr_rtos_create_thread(&task, TEST_CONFIG_TASK_PRIO, "asrosal_1_010_task3", task3, stack_size, NULL);
    ASSERT_EQ(ret, kNoErr);

    asr_rtos_get_semaphore(&g_sem_taskexit_sync, -1);
    TEST_PRINT("task3 exit!");

    asr_rtos_deinit_semaphore(&g_sem_taskexit_sync);
    asr_rtos_delete_thread(&task);
}

CASE(asrosal_test_task, asrosal_1_011)
{
    unsigned int stack_size = TEST_CONFIG_STACK_SIZE;
    char task_name[32] = {0};
    int ret = -1;
    int i = 0;
    asr_thread_hand_t  task[TEST_CONFIG_MAX_TASK_COUNT] = {{0}};

    asr_rtos_init_semaphore(&g_sem_taskexit_sync, 0);
     
    for(i=0; i<TEST_CONFIG_MAX_TASK_COUNT; i++) {
        memset(task_name, 0, sizeof(task_name));
        snprintf(task_name, 10, "asrosal_1_011_task%02d", i);
        ret = asr_rtos_create_thread(&task[i], TEST_CONFIG_TASK_PRIO, task_name, task1, stack_size, NULL);
        ASSERT_EQ(ret, kNoErr);
        asr_rtos_delay_milliseconds(1);
    }

    for(i=0; i<TEST_CONFIG_MAX_TASK_COUNT; i++) {
        asr_rtos_get_semaphore(&g_sem_taskexit_sync, -1);
    }
    TEST_PRINT("%d tasks exit!", TEST_CONFIG_MAX_TASK_COUNT);

    asr_rtos_deinit_semaphore(&g_sem_taskexit_sync);
    for (i = 0; i < TEST_CONFIG_MAX_TASK_COUNT; i++)
        asr_rtos_delete_thread(&task[i]);
}

CASE(asrosal_test_task, asrosal_1_012)
{
    unsigned int stack_size = TEST_CONFIG_STACK_SIZE;
    char task_name[32] = {0};
    int ret = -1;
    int i = 0;
    asr_thread_hand_t  task = {0};


    for(i=0; i<TEST_CONFIG_CREATE_TASK_TIMES; i++) {
        memset(task_name, 0, sizeof(task_name));
        snprintf(task_name, 32, "asr_1_012_task%02d", i);
        if(i % 500 == 0) {
            TEST_PRINT("create task: %d/%d", i, TEST_CONFIG_CREATE_TASK_TIMES);
        }

        asr_rtos_init_semaphore(&g_sem_taskexit_sync, 0);
        ret = asr_rtos_create_thread(&task, TEST_CONFIG_TASK_PRIO, task_name, task0, stack_size, NULL);
        ASSERT_EQ(ret, kNoErr);

        asr_rtos_get_semaphore(&g_sem_taskexit_sync, -1);
        asr_rtos_deinit_semaphore(&g_sem_taskexit_sync);

        asr_rtos_delete_thread(&task);
        asr_rtos_delay_milliseconds(1);
    }
}

CASE(asrosal_test_task, asrosal_1_013)
{
    unsigned int stack_size = TEST_CONFIG_STACK_SIZE;
    OSStatus ret = 0;

    ret = asr_rtos_create_thread(&g_task, TEST_CONFIG_TASK_PRIO, "task4", task4, stack_size, NULL);
    ASSERT_EQ(ret, kNoErr);

    asr_rtos_delay_milliseconds(200);
    //ASSERT_EQ(g_task.thread, NULL);
    ASSERT_EQ(should_not_reach, 0);
}
#endif /* TEST_CONFIG_TASK_ENABLED */

/* task test suite */
SUITE(asrosal_test_task) = {
#if (TEST_CONFIG_TASK_ENABLED > 0)
    ADD_CASE(asrosal_test_task, asrosal_1_006),
    ADD_CASE(asrosal_test_task, asrosal_1_007),
    ADD_CASE(asrosal_test_task, asrosal_1_008),
    ADD_CASE(asrosal_test_task, asrosal_1_009),
    ADD_CASE(asrosal_test_task, asrosal_1_010),
    ADD_CASE(asrosal_test_task, asrosal_1_011),
    ADD_CASE(asrosal_test_task, asrosal_1_012),
    ADD_CASE(asrosal_test_task, asrosal_1_013),
#endif
    ADD_CASE_NULL
};