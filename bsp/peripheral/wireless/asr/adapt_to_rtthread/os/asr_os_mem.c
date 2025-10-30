#include "asr_rtos_api.h"
#include "asr_dbg.h"

int32_t asr_malloc_cnt = 0;

void *lega_rtos_malloc( uint32_t size)
{
	return asr_rtos_malloc(size);
}

void lega_rtos_free(void *pv)
{
	return asr_rtos_free(pv);
}

void *asr_rtos_malloc( uint32_t xWantedSize )
{
    void *ptmp = NULL;
    /* expect zero init area */
    // ptmp = pvPortMalloc(size);
    ptmp = rt_malloc(xWantedSize);
    if (ptmp) {
        memset(ptmp, 0, xWantedSize);
        asr_malloc_cnt++;
    }

    return ptmp;
}

void asr_rtos_free( void *pv )
{
    rt_free(pv);
    asr_malloc_cnt--;
}

void *asr_rtos_calloc(size_t nmemb, size_t size)
{
    void *ptmp = NULL;

    ptmp = rt_calloc(nmemb, size);
    if (ptmp) {
        asr_malloc_cnt++;
    }

    return ptmp;
}

void *asr_rtos_realloc(void *ptr, uint32_t size)
{
    void *ptmp;
    ptmp = rt_realloc(ptr, size);
    if (ptmp) {
        asr_malloc_cnt++;
    }

    return ptmp;
}

int asr_rtos_GetError(void)
{
    return 0;
}

void asr_rtos_SetError(int err)
{

}
