/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "uds_def.h"
//Negative Response
#define IS_NEGATIVE_RESPONSE(frame_buf) (frame_buf[0] == 0x7F)
//Positive Response
#define IS_POSITIVE_RESPONSE(frame_buf, service_id) (frame_buf[1] == (0x40 + (service_id)))
//eg：0x50 03
#define IS_RESPONSE_WITH_SUBFUNC(frame_buf, service_id, subfunc) \
    (frame_buf[1] == (0x40 + (service_id)) && frame_buf[2] == (subfunc))

//Check if it is a negative response for a specific service
#define IS_NEGATIVE_RESPONSE_FOR_SERVICE(frame_buf, service_id) \
    (IS_NEGATIVE_RESPONSE(frame_buf) && frame_buf[1] == (service_id))

#define UDS_SERVICE_DIAG_SESSION_CTRL     0x10
#define UDS_SERVICE_ECU_RESET             0x11
#define UDS_SERVICE_READ_DATA             0x22
#define UDS_SERVICE_SECURITY_ACCESS       0x27
#define UDS_SERVICE_COMMUNICATION_CTRL    0x28
#define UDS_SERVICE_ROUTINE_CTRL          0x31
#define UDS_SERVICE_REQUEST_DOWNLOAD      0x34
#define UDS_SERVICE_REQUEST_UPLOAD        0x35
#define UDS_SERVICE_TRANSFER_DATA         0x36
#define UDS_SERVICE_REQUEST_TRANSFER_EXIT 0x37
#define UDS_SERVICE_WRITE_DATA            0x3E
#define UDS_SERVICE_CTRL_DTC              0x85

static rt_timer_t response_timer;

void response_timeout(void *param)
{
    rt_kprintf("\n[UDS] ERROR: Response timeout!\n");
}

void init_uds_timers()
{
    response_timer =
        rt_timer_create("uds_tout", response_timeout, RT_NULL, 2000, RT_TIMER_FLAG_ONE_SHOT);
}

static uint8_t g_key[4] = { 0 };
typedef struct {
    uint8_t data[8];
    uint8_t len;
    uint8_t next_state;
} UdsRequest;

static const UdsRequest uds_requests[] = {
    [UDS_STATE_ENTER_EXT_SESSION] = {
        .data = {0x03, 0x22, 0xF1, 0x86},
        .len = 4,
        .next_state = UDS_STATE_READ_ECU_INFO,
    },
    [UDS_STATE_READ_ECU_INFO] = {
        .data = {0x02, 0x10, 0x02},
        .len = 3,
        .next_state = UDS_STATE_ENTER_PROG_SESSION,
    },
    [UDS_STATE_ENTER_PROG_SESSION] = {
        .data = {0x02, 0x27, 0x01},
        .len = 3,
        .next_state = UDS_STATE_SAFE_ACCESS,
    },
    [UDS_STATE_REQUEST_UPLOAD_EXIT] = {
        .data = {0x01, 0x37},
        .len = 2,
        .next_state = UDS_STATE_APP_CHECK,
    }
};

#define UDS_STATE_COUNT (UDS_STATE_ENTER_DEFAULT_SESSION + 1)
void send_uds_service(void)
{
    uint8_t uds_request[8] = { 0 };
    uint8_t uds_request_len = 0;

    if (g_current_state >= UDS_STATE_COUNT) {
        rt_kprintf("[UDS] Invalid state: %d\n", g_current_state);
        return;
    }

    const UdsRequest *req = &uds_requests[g_current_state];

    if (g_current_state == UDS_STATE_SAFE_ACCESS) {
        uds_request[0] = 0x06;
        uds_request[1] = 0x27;
        uds_request[2] = 0x02;
        if (g_key[0] || g_key[1] || g_key[2] || g_key[3]) {
            uds_request[3] = g_key[0];
            uds_request[4] = g_key[1];
            uds_request[5] = g_key[2];
            uds_request[6] = g_key[3];
            uds_request_len = 7;
            g_current_state = UDS_STATE_REQUEST_UPLOAD_WAIT;
        } else {
            rt_kprintf("[UDS] Error: Key not initialized\n");
            return;
        }
    } else {
        if (req->len > 0 && req->len <= 8) {
            memcpy(uds_request, req->data, req->len);
            uds_request_len = req->len;
            g_current_state = req->next_state;
        } else {
            rt_kprintf("[UDS] Invalid request length: %d\n", req->len);
            return;
        }
    }

    if (uds_request_len > 0) {
        uds_send_frame(UDS_REQUEST_ID, uds_request, uds_request_len, 1);
    } else {
        rt_kprintf("[UDS] Error: Attempting to send empty frame\n");
    }
}

int uds_calculate_key_direct(const uint8_t *frame_buf, uint8_t *key_buf)
{
    if (frame_buf == NULL || key_buf == NULL) {
        return -1;
    }
    uint8_t frame_len = frame_buf[0];
    if (frame_len < 6) {
        return -1;
    }
    if (frame_buf[1] != 0x67 || frame_buf[2] != 0x01) {
        return -1;
    }

    uint32_t seed = (uint32_t)frame_buf[3] << 24 | (uint32_t)frame_buf[4] << 16 |
                    (uint32_t)frame_buf[5] << 8 | (uint32_t)frame_buf[6];
    uint32_t calculated_key = ~seed;
    key_buf[0] = (uint8_t)(calculated_key >> 24);
    key_buf[1] = (uint8_t)(calculated_key >> 16);
    key_buf[2] = (uint8_t)(calculated_key >> 8);
    key_buf[3] = (uint8_t)calculated_key;

    return 0;
}

/******************************************************************************
* 函数名称: static void handle_uds_state_machine(uint32_t id, uint8_t* frame_buf, uint8_t frame_dlc)
* 功能说明: UDS状态机处理
* 输入参数: uint32_t    id              --消息帧 ID
    　　　　uint8_t*    frame_buf       --接收报文帧数据首地址
    　　　　uint8_t     frame_dlc       --接收报文帧数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: ecu响应，处理状态机

******************************************************************************/
static void handle_uds_state_machine(uint32_t id, uint8_t *frame_buf, uint8_t frame_dlc)
{
    if (frame_dlc < 2) {
        return;
    }
    rt_mutex_take(g_uds_state_mutex, RT_WAITING_FOREVER);
    switch (g_current_state) {
        case UDS_STATE_ENTER_EXT_SESSION:
            if (IS_RESPONSE_WITH_SUBFUNC(frame_buf, UDS_SERVICE_DIAG_SESSION_CTRL, 0x03)) {
                send_uds_service();
            }
            break;
        case UDS_STATE_READ_ECU_INFO:
            if (IS_POSITIVE_RESPONSE(frame_buf, UDS_SERVICE_READ_DATA)) {
                send_uds_service();
            }
            break;
        case UDS_STATE_ENTER_PROG_SESSION:
            if (IS_RESPONSE_WITH_SUBFUNC(frame_buf, UDS_SERVICE_DIAG_SESSION_CTRL, 0x02)) {
                send_uds_service();
            }
            break;
        case UDS_STATE_SAFE_ACCESS:
            if (IS_RESPONSE_WITH_SUBFUNC(frame_buf, UDS_SERVICE_SECURITY_ACCESS, 0x01)) {
                // Calculate the key
                uds_calculate_key_direct(frame_buf, g_key);
                send_uds_service();
            }
            break;
        case UDS_STATE_SAFE_ACCESS_SEND_VERIFY:
            if (IS_RESPONSE_WITH_SUBFUNC(frame_buf, UDS_SERVICE_SECURITY_ACCESS, 0x02)) {
                send_uds_service();
            }
            break;
        case UDS_STATE_ERASE_CODE:
            if (IS_RESPONSE_WITH_SUBFUNC(frame_buf, UDS_SERVICE_ROUTINE_CTRL, 0x01)) {
                send_uds_service();
            }
            break;
        case UDS_STATE_REQUEST_UPLOAD:
            break;
        case UDS_STATE_SEND_UPLOAD:
            break;
        case UDS_STATE_REQUEST_UPLOAD_EXIT:
            break;
        case UDS_STATE_APP_CHECK:
            break;
        case UDS_STATE_CODE_CHECK:
            break;
        case UDS_STATE_REQUEST_UPLOAD_WAIT:
            g_current_state = UDS_STATE_REQUEST_UPLOAD;
            break;
        case UDS_STATE_WAIT_DOWNLOAD_RESPONSE:
            if (frame_dlc >= 5 && IS_POSITIVE_RESPONSE(frame_buf, UDS_SERVICE_REQUEST_DOWNLOAD)) {
                g_max_block_size = (frame_buf[3] << 8) | frame_buf[4];
                g_bytes_per_block = (g_max_block_size > 0) ? g_max_block_size : 32;
                g_current_state = UDS_STATE_SEND_UPLOAD;
            } else if (IS_NEGATIVE_RESPONSE_FOR_SERVICE(frame_buf, UDS_SERVICE_REQUEST_DOWNLOAD)) {
                g_current_state = UDS_STATE_ERROR;
            } else {
                uds_tp_recv_frame(0, frame_buf, frame_dlc);
            }
            break;
        case UDS_STATE_WAIT_TRANSFER_RESPONSE:
            if (IS_POSITIVE_RESPONSE(frame_buf, UDS_SERVICE_TRANSFER_DATA)) {
                rt_sem_release(&g_flow_control_sem);

            } else if (IS_NEGATIVE_RESPONSE_FOR_SERVICE(frame_buf, UDS_SERVICE_TRANSFER_DATA)) {
                g_current_state = UDS_STATE_ERROR;
            } else {
                uds_tp_recv_frame(0, frame_buf, frame_dlc);
            }
            break;
        default:
            break;
    }
    rt_mutex_release(g_uds_state_mutex);
}

/******************************************************************************
* 函数名称: void uds_recv_frame(uint32_t id, uint8_t* frame_buf, uint8_t frame_dlc)
* 功能说明: 接收到一帧报文
* 输入参数: uint32_t    id              --消息帧 ID
    　　　　uint8_t*    frame_buf       --接收报文帧数据首地址
    　　　　uint8_t     frame_dlc       --接收报文帧数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: frame_dlc 长度必须等于 FRAME_SIZE，否则会被判断为无效帧
******************************************************************************/
void uds_recv_frame(uint32_t id, uint8_t *frame_buf, uint8_t frame_dlc)
{
    if (UDS_REQUEST_ID == id)
        uds_tp_recv_frame(0, frame_buf, frame_dlc);
    else if (UDS_FUNCTION_ID == id)
        uds_tp_recv_frame(1, frame_buf, frame_dlc);
    else if (UDS_RESPONSE_ID == id)
        handle_uds_state_machine(id, frame_buf, frame_dlc);
}

/******************************************************************************
* 函数名称: void uds_send_frame(uint32_t id, uint8_t* frame_buf, uint8_t frame_dlc)
* 功能说明: 发送一帧报文
* 输入参数: uint8_t     response_id     --应答 ID
    　　　　uint8_t*    frame_buf       --发送报文帧数据首地址
    　　　　uint8_t     frame_dlc       --发送报文帧数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: frame_dlc 长度应当等于 FRAME_SIZE
******************************************************************************/
void uds_send_frame(uint32_t id, uint8_t *data, uint8_t len, uint8_t expect_response)
{
    rt_size_t size;
    struct rt_can_msg msg = { 0 };
    msg.id = id;
    msg.ide = (id > 0x7FF) ? 1 : 0;
    msg.rtr = 0;
    msg.len = len;

    memcpy(msg.data, data, len);
    size = rt_device_write(g_can_tx_dev, 0, &msg, sizeof(msg));
    if (size != sizeof(msg)) {
        rt_kprintf("[UDS] Error: Failed to send frame\n");
        return;
    }
    if (expect_response) {
        // rt_kprintf("[UDS] Waiting for response... (timeout=2000ms)\n");
    }
}

/******************************************************************************
* 函数名称: void uds_init(void)
* 功能说明: UDS 初始化
* 输入参数: 无
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void uds_init(void)
{
    service_init();
}

/******************************************************************************
* 函数名称: void uds_1ms_task(void)
* 功能说明: UDS 周期任务
* 输入参数: 无
* 输出参数: 无
* 函数返回: 无
* 其它说明: 该函数需要被 1ms 周期调用
******************************************************************************/
void uds_1ms_task(void)
{
    network_task();
    service_task();
}
