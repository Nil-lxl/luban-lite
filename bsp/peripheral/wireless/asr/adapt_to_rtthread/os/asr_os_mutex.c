#include "asr_rtos_api.h"
#include "asr_dbg.h"

OSStatus asr_rtos_init_semaphore( asr_semaphore_t* semaphore, int value )
{
    rt_sem_t p_aossem = NULL;

    p_aossem = rt_sem_create("asr_sem", value, RT_IPC_FLAG_PRIO);
    if(p_aossem == NULL){
        return kGeneralErr;
    }

    *semaphore = (asr_semaphore_t*)p_aossem;

    return kNoErr;
}

OSStatus asr_rtos_set_semaphore( asr_semaphore_t* semaphore )
{
    rt_sem_t p_aossem = (rt_sem_t)(*semaphore);
    if (p_aossem == NULL)
    {
        return kGeneralErr;
    }

    rt_sem_release(p_aossem);
    return kNoErr;
}

OSStatus asr_rtos_get_semaphore( asr_semaphore_t* semaphore, uint32_t timeout_ms )
{
    int ret = 0;

    rt_sem_t p_aossem = (rt_sem_t)(*semaphore);
    if (p_aossem == NULL)
    {
        return kGeneralErr;
    }

    ret = rt_sem_take(p_aossem, asr_os_msec_to_ticks(timeout_ms));
    if (ret != 0)
    {
        return kTimeoutErr;
    }

    return kNoErr;
}

OSStatus asr_rtos_deinit_semaphore( asr_semaphore_t* semaphore )
{
    if (semaphore == NULL || *semaphore == NULL)
    {
        return kGeneralErr;
    }

    rt_sem_t p_aossem = (rt_sem_t)(*semaphore);
    if (p_aossem == NULL)
    {
        return kGeneralErr;
    }

    rt_sem_delete(p_aossem);
    *semaphore = NULL;

    return kNoErr;
}

OSStatus asr_rtos_init_mutex( asr_mutex_t* mutex )
{
    rt_mutex_t p_aosmux = NULL;

    if (mutex == NULL)
    {
        return kGeneralErr;
    }

    p_aosmux = rt_mutex_create("asrmutex", RT_IPC_FLAG_PRIO);
    if(p_aosmux == NULL){
        LOG_E("%s fail\n", __func__);
        return kGeneralErr;
    }
    *mutex = p_aosmux;
    return kNoErr;
}

OSStatus asr_rtos_lock_mutex( asr_mutex_t* mutex )
{
    int ret = -1;

    if (mutex == NULL)
    {
        return kGeneralErr;
    }

    rt_mutex_t p_aosmux = (rt_mutex_t )(*mutex);
    if (NULL == p_aosmux)
    {
        return kGeneralErr;
    }

    ret = rt_mutex_take(p_aosmux, RT_WAITING_FOREVER);
    if (ret != 0)
    {
        return kGeneralErr;
    }

    return kNoErr;
}

OSStatus asr_rtos_unlock_mutex( asr_mutex_t* mutex )
{
    int ret = -1;

    if (mutex == NULL)
    {
        return kGeneralErr;
    }

    rt_mutex_t p_aosmux = (rt_mutex_t )(*mutex);
    if (NULL == p_aosmux)
    {
        return kGeneralErr;
    }

    ret = rt_mutex_release(p_aosmux);
    if (ret != 0)
    {
        return kGeneralErr;
    }

    return kNoErr;
}

OSStatus asr_rtos_deinit_mutex( asr_mutex_t* mutex )
{
    if (mutex == NULL || *mutex == NULL)
    {
        return kGeneralErr;
    }

    rt_mutex_t p_aosmux = (rt_mutex_t )(*mutex);

    rt_mutex_delete(p_aosmux);
    *mutex = NULL;

    return kNoErr;
}
