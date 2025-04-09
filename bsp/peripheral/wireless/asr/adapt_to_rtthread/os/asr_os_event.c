#include "asr_rtos_api.h"


OSStatus asr_rtos_create_event(asr_event_t *evt)
{
    rt_event_t p_event;

    p_event = rt_event_create("asr_evt", RT_IPC_FLAG_PRIO);
    if(p_event == NULL) {
        return kGeneralErr;
    }
    *evt = (asr_event_t)p_event;

    return kNoErr;
}

OSStatus asr_rtos_deinit_event(asr_event_t *evt)
{
    rt_event_t p_event = (rt_event_t )(*evt);

    if (p_event == NULL)
        return kGeneralErr;

    rt_event_delete(p_event);
    *evt = NULL;

    return kNoErr;
}

OSStatus asr_rtos_wait_for_event(asr_event_t *evt, uint32_t mask, uint32_t *rcv_flags, uint32_t timeout)
{
    int ret;
    rt_event_t p_event = (rt_event_t )(*evt);

    if (p_event == NULL)
        return kGeneralErr;

    ret = rt_event_recv(p_event, mask, RT_EVENT_FLAG_OR|RT_EVENT_FLAG_CLEAR, timeout, rcv_flags);
    if(ret == RT_EOK) {
        return kNoErr;
    }
    else if(ret == -RT_ETIMEOUT) {
        return kTimeoutErr;
    }
    else {
        return kGeneralErr;
    }
}

OSStatus asr_rtos_set_event(asr_event_t *evt, uint32_t mask)
{
    int ret;
    rt_event_t p_event = (rt_event_t )(*evt);

    if (p_event == NULL)
        return kGeneralErr;

    ret = rt_event_send(p_event, mask);
    if (ret != RT_EOK) {
        return kGeneralErr;
    }

    return kNoErr;
}

OSStatus asr_rtos_clear_event(asr_event_t *evt, uint32_t mask)
{
    // uint32_t rcv_flags;
    // rt_event_t p_event = (rt_event_t )(*evt);

    // if (p_event == NULL)
    //     return kGeneralErr;

    // (void)rt_event_recv(p_event, mask, RT_EVENT_FLAG_CLEAR, 0, rcv_flags);

    return kNoErr;
}
