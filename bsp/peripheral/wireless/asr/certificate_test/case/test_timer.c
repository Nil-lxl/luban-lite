#include "test_main.h"

static asr_semaphore_t    g_sem;
static asr_timer_t  g_timer;
static unsigned int g_var = 0;

#if (TEST_CONFIG_TIMER_ENABLED > 0)
static void timer_handler(void *arg)
{
    TEST_PRINT("timer handler");
    if(++g_var == 10) {
        asr_rtos_set_semaphore(&g_sem);
    }
}

CASE(asrosal_test_timer, asrosal_1_021)
{
    OSStatus ret = -1;

    g_var = 0;
    asr_rtos_init_semaphore(&g_sem, 0);
    ASSERT_NE(g_sem, NULL);

    ret = asr_rtos_init_timer(&g_timer, 200, timer_handler, NULL);
    ASSERT_EQ(ret, kNoErr);
    asr_rtos_start_timer(&g_timer);

    asr_rtos_get_semaphore(&g_sem, -1);
    asr_rtos_deinit_semaphore(&g_sem);

    ret = asr_rtos_deinit_timer(&g_timer);
    ASSERT_EQ(ret, kNoErr);
}

CASE(asrosal_test_timer, asrosal_1_022)
{
    OSStatus ret = kNoErr;

    g_var = 0;
    ret = asr_rtos_init_semaphore(&g_sem, 0);
    ASSERT_EQ(ret, kNoErr);
    ASSERT_NE(g_sem, NULL);

    ret = asr_rtos_init_timer(&g_timer, 200, timer_handler, NULL);
    ASSERT_EQ(ret, kNoErr);
    asr_rtos_start_timer(&g_timer);   // TODO: ?

    asr_rtos_delay_milliseconds(1000);

    asr_rtos_stop_timer(&g_timer);
    TEST_PRINT("timer stopped!");
    asr_rtos_delay_milliseconds(1000);
    asr_rtos_update_timer(&g_timer, 1000);
    TEST_PRINT("timer changed!");
    asr_rtos_start_timer(&g_timer);
    TEST_PRINT("timer started!");

    asr_rtos_get_semaphore(&g_sem, -1);
    asr_rtos_deinit_semaphore(&g_sem);

    // stop and free
    asr_rtos_stop_timer(&g_timer);
    ret = asr_rtos_deinit_timer(&g_timer);
    ASSERT_EQ(ret, kNoErr);
}
#endif /* TEST_CONFIG_TIMER_ENABLED */

/* timer test suite */
SUITE(asrosal_test_timer) = {
#if (TEST_CONFIG_TIMER_ENABLED > 0)
    ADD_CASE(asrosal_test_timer, asrosal_1_021),
    ADD_CASE(asrosal_test_timer, asrosal_1_022),
#endif
    ADD_CASE_NULL
};