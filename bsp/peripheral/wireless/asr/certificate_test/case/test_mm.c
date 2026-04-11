#include "test_main.h"

#if (TEST_CONFIG_MM_ENABLED > 0)
CASE(asrosal_test_mm, asrosal_1_001)
{
    unsigned int size = 512;
    unsigned char *ptr = NULL;

    ptr = asr_rtos_malloc(size);
    ASSERT_NOT_NULL(ptr);

    asr_rtos_free(ptr);
}

CASE(asrosal_test_mm, asrosal_1_002)
{
    unsigned int size = 512;
    unsigned char *ptr = NULL;
    int i = 0;

    ptr = asr_rtos_malloc(size);
    ASSERT_NOT_NULL(ptr);

    for (; i<size; i++) {
        if (*(ptr+i) != 0) {
            asr_rtos_free(ptr);
            ASSERT_FAIL();
        }
    }
    asr_rtos_free(ptr);
}

CASE(asrosal_test_mm, asrosal_1_003)
{
    unsigned int size1 = 512;
    unsigned int size2 = 1024;
    unsigned char *ptr = NULL;
    int i = 0;

    ptr = asr_rtos_malloc(size1);
    ASSERT_NOT_NULL(ptr);
    memset(ptr, 0x5A, size1);

    ptr = asr_rtos_realloc(ptr, size2);
    ASSERT_NOT_NULL(ptr);
    memset(ptr+size1, 0xA5, size2-size1);

    for(i=0; i<size1; i++) {
        if(*(ptr+i) != 0x5A) {
            asr_rtos_free(ptr);
            ASSERT_FAIL();
        }
    }
    for(; i<size2; i++) {
        if(*(ptr+i) != 0xA5) {
            asr_rtos_free(ptr);
            ASSERT_FAIL();
        }
    }
    asr_rtos_free(ptr);
}

CASE(asrosal_test_mm, asrosal_1_004)
{
    unsigned int size1 = 512;
    unsigned int size2 = 256;
    unsigned char *ptr = NULL;
    int i = 0;

    ptr = asr_rtos_malloc(size1);
    ASSERT_NOT_NULL(ptr);
    memset(ptr, 0x5A, size1);

    ptr = asr_rtos_realloc(ptr, size2);
    ASSERT_NOT_NULL(ptr);

    for(i=0; i<size2; i++) {
        if(*(ptr+i) != 0x5A) {
            asr_rtos_free(ptr);
            ASSERT_FAIL();
        }
    }
    asr_rtos_free(ptr);
}

CASE(asrosal_test_mm, asrosal_1_005)
{
    unsigned int size = 1024;
    unsigned char *ptr = NULL;
    int i = TEST_CONFIG_MALLOC_FREE_TIMES;

    while(i--) {
        ptr = asr_rtos_malloc(size);
        ASSERT_NOT_NULL(ptr);
        memset(ptr, 0x5A, size);
        asr_rtos_free(ptr);
    }
}
#endif /* TEST_CONFIG_MM_ENABLED */

/* memory manager test suite */
SUITE(asrosal_test_mm) = {
#if (TEST_CONFIG_MM_ENABLED > 0)
    ADD_CASE(asrosal_test_mm, asrosal_1_001),
    ADD_CASE(asrosal_test_mm, asrosal_1_002),
    ADD_CASE(asrosal_test_mm, asrosal_1_003),
    ADD_CASE(asrosal_test_mm, asrosal_1_004),
    ADD_CASE(asrosal_test_mm, asrosal_1_005),
#endif
    ADD_CASE_NULL
};