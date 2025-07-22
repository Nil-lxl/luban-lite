/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "uds_def.h"
static rt_timer_t response_timer;

void response_timeout(void *param)
{
    rt_kprintf("\n[UDS] ERROR: Response timeout!\n");
}

void init_uds_timers()
{
    response_timer = rt_timer_create("uds_tout", response_timeout, RT_NULL, 2000,
                                     RT_TIMER_FLAG_ONE_SHOT);
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
void uds_recv_frame(uint32_t id, uint8_t* frame_buf, uint8_t frame_dlc)
{
    if(UDS_REQUEST_ID == id)
        uds_tp_recv_frame(0, frame_buf, frame_dlc);
    else if(UDS_FUNCTION_ID == id)
        uds_tp_recv_frame(1, frame_buf, frame_dlc);
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
    rt_err_t ret = 0;
    rt_size_t  size;
    struct rt_can_msg msg = {
        .id = id,
        .ide = (id > 0x7FF) ? 1 : 0,
        .rtr = 0,
        .len = len
    };
    memcpy(msg.data, data, len);

    rt_kprintf("[UDS] REQUEST: ID=0x%03X LEN=%d DATA: ", id, len);
    for(int i=0; i<len; i++) 
        rt_kprintf("%02X ", data[i]);
    rt_kprintf("\n");

    rt_device_t can_tx_dev = rt_device_find(CAN_TX_DEV_NAME);
    if (can_tx_dev) {
        ret = rt_device_open(can_tx_dev,
                         RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
        if (ret)
        {
            rt_kprintf("%s open failed!\n", CAN_TX_DEV_NAME);
            return;
        }

        ret = rt_device_control(can_tx_dev, RT_CAN_CMD_SET_BAUD, (void *)CAN1MBaud);
        if (ret)
        {
            rt_kprintf("%s set baudrate failed!\n", CAN_TX_DEV_NAME);
            return;
        }

        rt_device_control(can_tx_dev, RT_DEVICE_CTRL_SET_INT, NULL);
        size = rt_device_write(can_tx_dev, 0, &msg, sizeof(msg));
        if (size != sizeof(msg))
        {
            rt_kprintf("can dev write data failed!\n");
            return;
        }
    }

    if(expect_response) {
        rt_kprintf("[UDS] Waiting for response... (timeout=2000ms)\n");
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
