/**
 ****************************************************************************************
 *
 * @file uwifi_ipc_shared.h
 *
 * @brief Shared data between SHARERAM.
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */

#ifndef _UWIFI_IPC_SHARED_H_
#define _UWIFI_IPC_SHARED_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "mac.h"
#include "asr_types.h"
#include "ipc_shared.h"
//#include "dma.h"
//#include "la.h"
#include "dbg.h"
#include "uwifi_types.h"
struct rxdesc_tag
{
    /// Host Buffer Address
    uint32_t host_id;
    /// Length
    uint32_t frame_len;
    /// Status
    uint8_t status;
};

struct asr_e2amsg_elem {
    struct ipc_e2a_msg *msgbuf_ptr;
    dma_addr_t dma_addr;
};

// IPC message TYPE
enum
{
    IPC_MSG_NONE = 0,
    IPC_MSG_WRAP,
    IPC_MSG_KMSG,
    IPC_DBG_STRING,
};

#endif // _UWIFI_IPC_SHARED_H_
