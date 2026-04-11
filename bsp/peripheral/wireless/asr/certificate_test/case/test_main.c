#include "test_main.h"

static int asrosal_dump_test_config(void)
{
#define _PARSE(x) #x
#define PARSE(x) _PARSE(x)
#define PRINT_CONFIG(x) TEST_PRINT("\33[0;35m%s=%s\33[0m", #x, PARSE(x))
    if (strlen(SYSINFO_ARCH)==0 || strlen(SYSINFO_MCU) == 0 || strlen(SYSINFO_DEVICE_NAME)==0) {
        TEST_PRINT("Please set your device info first!");
        return -1;
    }
    else {
        PRINT_CONFIG(SYSINFO_ARCH);
        PRINT_CONFIG(SYSINFO_MCU);
        PRINT_CONFIG(SYSINFO_DEVICE_NAME);
        PRINT_CONFIG(SYSINFO_KERNEL);
        PRINT_CONFIG(aos_version_get());
        PRINT_CONFIG(SYSINFO_APP_VERSION);
    }

    PRINT_CONFIG(TEST_CONFIG_MM_ENABLED);
#if (TEST_CONFIG_MM_ENABLED > 0)
    PRINT_CONFIG(TEST_CONFIG_MALLOC_MAX_SIZE);
    PRINT_CONFIG(TEST_CONFIG_MALLOC_FREE_TIMES);
#endif

    PRINT_CONFIG(TEST_CONFIG_TASK_ENABLED);
#if (TEST_CONFIG_TASK_ENABLED > 0)
    PRINT_CONFIG(TEST_CONFIG_MAX_TASK_COUNT);
    PRINT_CONFIG(TEST_CONFIG_CREATE_TASK_TIMES);
    PRINT_CONFIG(TEST_CONFIG_STACK_SIZE);
    PRINT_CONFIG(TEST_CONFIG_TASK_PRIO);
#endif

    PRINT_CONFIG(TEST_CONFIG_TASK_COMM_ENABLED);
#if (TEST_CONFIG_TASK_COMM_ENABLED > 0)
    PRINT_CONFIG(TEST_CONFIG_SYNC_TIMES);
#endif

    PRINT_CONFIG(TEST_CONFIG_TIMER_ENABLED);

    PRINT_CONFIG(TEST_CONFIG_KV_ENABLED);
#if (TEST_CONFIG_KV_ENABLED > 0)
    PRINT_CONFIG(TEST_CONFIG_KV_TIMES);
#endif

    return 0;
}

extern SUITE(asrosal_test_mm);
extern SUITE(asrosal_test_task);
extern SUITE(asrosal_test_task_comm);
extern SUITE(asrosal_test_timer);

void asrosal_test(void)
{
    RESET_ALLSUITE();

    if (0 == asrosal_dump_test_config()) {
        TEST_PRINT("asr osal test start!");
        ADD_SUITE(asrosal_test_mm);
        ADD_SUITE(asrosal_test_task);
        ADD_SUITE(asrosal_test_task_comm);
        ADD_SUITE(asrosal_test_timer);
        cut_main(0, NULL);
        TEST_PRINT("asr osal test finished!");
    }
    else {
        TEST_PRINT("asr osal test error!");
    }

    return ;
}