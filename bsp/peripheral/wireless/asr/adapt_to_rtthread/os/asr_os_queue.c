#include "asr_rtos_api.h"
#include "asr_dbg.h"
//#include "asm/atomic.h"

OSStatus asr_rtos_init_queue( asr_queue_stuct_t* queue_struct, const char* name, uint32_t message_size, uint32_t number_of_messages )
{
    rt_mq_t p_aosqueue = NULL;
    if (queue_struct == NULL || name == NULL || message_size == 0 || number_of_messages == 0) {
        return kGeneralErr;
    }

    queue_struct->max_size = number_of_messages;
    queue_struct->cur_size = 0;
    queue_struct->msg_size = message_size;

    if (kNoErr != asr_rtos_init_mutex(&queue_struct->mutex))
    {
        LOG_E("queue %s mutex init failed", name);
        return kGeneralErr;
    }

    p_aosqueue = rt_mq_create(name, message_size, number_of_messages, RT_IPC_FLAG_FIFO);
    if(p_aosqueue == NULL)
    {
        asr_rtos_deinit_mutex(&queue_struct->mutex);
        LOG_E("queue %s create failed, size: %d*%d", name, message_size, number_of_messages);

        return kGeneralErr;
    }

    queue_struct->queue = (asr_queue_t)p_aosqueue;

    return kNoErr;
}

OSStatus asr_rtos_push_to_queue_front( asr_queue_stuct_t* queue_struct, void* message, uint32_t timeout_ms )
{
    return kNoErr;
}

OSStatus asr_rtos_push_to_queue( asr_queue_stuct_t* queue_struct, void* message, uint32_t timeout_ms )
{
    int ret = -1;
    if (queue_struct == NULL || queue_struct->queue == NULL || message == NULL)
    {
        return kGeneralErr;
    }

    // non-blocking
    if (queue_struct->cur_size >= queue_struct->max_size && timeout_ms == 0) {
        LOG_E("%s fail, queue is full,cur_size=%d,max_size=%d.\n"
            , __func__, queue_struct->cur_size, queue_struct->max_size);
        return kGeneralErr;
    }

    rt_mq_t p_aosqueue = (rt_mq_t)(queue_struct->queue);
    if (NULL == p_aosqueue)
    {
        return kGeneralErr;
    }

    ret = rt_mq_send_wait(p_aosqueue, message, queue_struct->msg_size, asr_os_msec_to_ticks(timeout_ms));
    if (ret) {
        return kGeneralErr;
    }

    asr_rtos_lock_mutex(&queue_struct->mutex);
    queue_struct->cur_size++;
    asr_rtos_unlock_mutex(&queue_struct->mutex);

    return kNoErr;
}

OSStatus asr_rtos_pop_from_queue( asr_queue_stuct_t* queue_struct, void* message, uint32_t timeout_ms )
{
    int ret;

    // non-blocking
    if (queue_struct->cur_size == 0 && timeout_ms == 0) {
        LOG_E("%s fail, queue is empty.\n", __func__);
        return kGeneralErr;
    }

    rt_mq_t p_aosqueue = (rt_mq_t)(queue_struct->queue);
    if (NULL == p_aosqueue)
    {
        return kGeneralErr;
    }

    ret = rt_mq_recv(p_aosqueue, message, queue_struct->msg_size, asr_os_msec_to_ticks(timeout_ms));
    if (ret != RT_EOK)
    {
        return kGeneralErr;
    }

    asr_rtos_lock_mutex(&queue_struct->mutex);
    if (queue_struct->cur_size) {
        queue_struct->cur_size--;
    }
    asr_rtos_unlock_mutex(&queue_struct->mutex);
    return kNoErr;
}

OSStatus asr_rtos_deinit_queue( asr_queue_stuct_t* queue_struct )
{
    int ret = 0;

    rt_mq_t p_aosqueue = (rt_mq_t )(queue_struct->queue);
    if (NULL == p_aosqueue)
    {
        LOG_E("%s(%d), ERROR!\n", __func__, __LINE__);
        return kGeneralErr;
    }

    rt_mq_delete(p_aosqueue);
    queue_struct->queue = NULL;
    ret = asr_rtos_deinit_mutex(&queue_struct->mutex);
    queue_struct->max_size = 0;
    queue_struct->cur_size = 0;
    queue_struct->msg_size = 0;
    return ret;
}

OSBool asr_rtos_is_queue_empty( asr_queue_stuct_t* queue_struct )
{
    if (queue_struct->cur_size == 0) {
        return kTRUE;
    }
    return kFALSE;
}

OSBool asr_rtos_is_queue_full( asr_queue_stuct_t* queue_struct )
{
    if (queue_struct->cur_size >= queue_struct->max_size) {
        return kTRUE;
    }
    return kFALSE;
}

OSBool asr_rtos_is_queue_valid( asr_queue_stuct_t* queue_struct )
{
    if (queue_struct == NULL) {
        return kFALSE;
    }
    if (queue_struct -> queue == NULL) {
        return kFALSE;
    }
    return kTRUE;
}
