#include "asr_rtos_api.h"
#include "tasks_info.h"
#include "asr_dbg.h"

asr_task_config_t task_cfg[ASR_TASK_CONFIG_MAX] = {
    {ASR_TASK_CONFIG_UWIFI,             UWIFI_TASK_PRIORITY, UWIFI_TASK_STACK_SIZE},
    {ASR_TASK_CONFIG_UWIFI_SDIO,        UWIFI_SDIO_TASK_PRIORITY, UWIFI_SDIO_TASK_STACK_SIZE},
    {ASR_TASK_CONFIG_UWIFI_RX_TO_OS,    UWIFI_RX_TO_OS_TASK_PRIORITY, UWIFI_RX_TO_OS_TASK_STACK_SIZE},
    {ASR_TASK_CONFIG_UWIFI_AT,          UWIFI_AT_TASK_PRIORITY, UWIFI_AT_TASK_STACK_SIZE},
    {ASR_TASK_CONFIG_IPERF,             IPERF_TASK_PRIORITY, IPERF_TASK_STACK_SIZE},
    {ASR_TASK_CONFIG_PING,              PING_TASK_PRIORITY, PING_TASK_STACK_SIZE},
};

OSStatus asr_rtos_create_thread( asr_thread_hand_t* thread, uint8_t priority, const char* name, asr_thread_function_t function, uint32_t stack_size, asr_thread_arg_t arg )
{
    //int ret;
    rt_thread_t p_aosthread = NULL;
    if (thread == NULL || name == NULL || function == NULL) {
        return kGeneralErr;
    }

    if(priority < 1) priority = 1;
    if(priority > (RT_THREAD_PRIORITY_MAX - 1)) priority = RT_THREAD_PRIORITY_MAX - 1;

    p_aosthread = rt_thread_create(name, function, arg, stack_size, priority, 2);
    if(!p_aosthread){
        return kGeneralErr;
    }

    thread->thread = (asr_thread_t)p_aosthread;

    thread->stack_ptr = p_aosthread->stack_addr;

    if (rt_thread_startup(p_aosthread) != RT_EOK)
        return kGeneralErr;
    return kNoErr;
}

OSStatus asr_rtos_delete_thread( asr_thread_hand_t* thread )
{
    int ret;

    if(thread == NULL || thread->thread == NULL) {
        return kGeneralErr;
    }

    //waiting for task finish
    ret = rt_thread_delete(thread->thread);
    if (ret != 0) {
        return kGeneralErr;
    }

    thread->thread = NULL;
    thread->stack_ptr = NULL;

    return kNoErr;
}

void asr_rtos_suspend_thread(asr_thread_t* thread)
{
    LOG_E("not support yet!");
}

OSBool asr_rtos_is_current_thread( asr_thread_t* thread )
{
    LOG_E("not support yet!");
    return kTRUE;
}

asr_thread_t *asr_rtos_get_current_thread( void )
{
    LOG_E("not support yet!");
    return NULL;
}

OSBool asr_rtos_is_thread_valid(asr_thread_hand_t* thread)
{
    if (thread->thread != NULL) {
        return kTRUE;
    } else {
        return kFALSE;
    }
}

OSStatus asr_rtos_delay_milliseconds( uint32_t num_ms )
{
    rt_thread_mdelay(num_ms);
    return kNoErr;
}

OSStatus asr_rtos_print_thread_status( char* buffer, int length )
{
    LOG_E("not support yet!");
    return kNoErr;
}

asr_thread_t asr_rtos_get_thread_by_name(char* name)
{
    asr_thread_t aos_thread;
    aos_thread = (asr_thread_t)rt_thread_find(name);
    return aos_thread;
}

OSStatus asr_rtos_task_cfg_get(uint32_t index, asr_task_config_t *cfg )
{
    uint8_t i;

    if (!cfg || (index >= ASR_TASK_CONFIG_MAX)) {
        return kGeneralErr;
    }
    for (i = 0; i < ASR_TASK_CONFIG_MAX; i++) {
        if (task_cfg[i].index == index) {
            cfg -> task_priority = task_cfg[i].task_priority;
            cfg -> stack_size = task_cfg[i].stack_size;
            return kNoErr;
        }
    }

    return kGeneralErr;
}

// OSStatus asr_rtos_create_worker_thread( asr_worker_thread_t* worker_thread, uint8_t priority, uint32_t stack_size, uint32_t event_queue_size )
// {

// }

// OSStatus asr_rtos_delete_worker_thread( asr_worker_thread_t* worker_thread )
// {

// }

// void asr_rtos_suspend_all_thread(void)
// {

// }

// long asr_rtos_resume_all_thread(void)
// {

// }

// OSStatus asr_rtos_thread_join( asr_thread_t* thread )
// {

// }

// OSStatus asr_rtos_thread_force_awake( asr_thread_t* thread )
// {

// }

// void asr_rtos_thread_sleep(uint32_t seconds)
// {

// }

// void asr_rtos_thread_msleep(uint32_t milliseconds)
// {

// }



// OSStatus asr_rtos_send_asynchronous_event( asr_worker_thread_t* worker_thread, event_handler_t function, void* arg )
// {

// }

// OSStatus asr_rtos_register_timed_event( asr_timed_event_t* event_object, asr_worker_thread_t* worker_thread, event_handler_t function, uint32_t time_ms, void* arg )
// {

// }

// OSStatus asr_rtos_deregister_timed_event( asr_timed_event_t* event_object )
// {

// }

// int asr_rtos_init_event_fd(asr_event_t event_handle)
// {

// }

// int asr_rtos_deinit_event_fd(int fd)
// {

// }
