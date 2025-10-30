/**
 ****************************************************************************************
 *
 * @file uwifi_task.c
 *
 * @brief uwifi task init
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#include "uwifi_task.h"
#include "asr_rtos.h"
#include "asr_dbg.h"
#include "uwifi_msg.h"
#include "tasks_info.h"
#include "uwifi_platform.h"
//these define uwifi task

asr_thread_hand_t  uwifi_task_handle={0};

/**
 ****************************************************************************************
 * @brief main entry of UWIFI_TASK.
 ****************************************************************************************
 */
static void uwifi_main(asr_thread_arg_t arg)
{
    uwifi_msg_handle();
}


/**
 ****************************************************************************************
 * @brief api of init UWIFI_TASK.
 ****************************************************************************************
 */
OSStatus init_uwifi(void)
{
    OSStatus status = kNoErr;
    asr_task_config_t cfg;
    //asr_dump_poolsize();

    status = uwifi_msg_queue_init();
    if(status != kNoErr)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"OS Error:uwifi_msg_queue_init %x",status);
        return status;
    }
    if (kNoErr != asr_rtos_task_cfg_get(ASR_TASK_CONFIG_UWIFI, &cfg)) {
        dbg(D_ERR,D_UWIFI_CTRL,"get uwifi task information fail");
        return -1;
    }
    //asr_dump_poolsize();
    status = asr_rtos_create_thread(&uwifi_task_handle, cfg.task_priority,UWIFI_TASK_NAME, uwifi_main, cfg.stack_size, 0);
    if(status != kNoErr)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"OS Error:asr_rtos_create_thread");
        uwifi_msg_queue_deinit();
        return status;
    }

    dbg(D_CRT, D_UWIFI_CTRL,  "init_uwifi");
    return status;
}

/**
 ****************************************************************************************
 * @brief api of deinit UWIFI_TASK.
 ****************************************************************************************
 */
OSStatus deinit_uwifi(void)
{
    OSStatus status = kNoErr;


    status = asr_rtos_delete_thread(&uwifi_task_handle);
    if(status != kNoErr)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"[%s]OS Error:asr_rtos_delete_thread",__func__);
        return status;
    }
    status = uwifi_msg_queue_deinit();
    if(status != kNoErr)
    {
        dbg(D_ERR,D_UWIFI_CTRL,"OS Error:uwifi_msg_queue_deinit");
        return status;
    }

    dbg(D_CRT, D_UWIFI_CTRL,  "deinit_uwifi");

    return status;
}

