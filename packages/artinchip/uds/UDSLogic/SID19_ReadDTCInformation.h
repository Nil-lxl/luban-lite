/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _SID19_READ_DTC_INFORMATION_H_
#define _SID19_READ_DTC_INFORMATION_H_


#include <stdint.h>
#include "uds_def.h"


/******************************************************************************
* 函数名称: void service_19_ReadDTCInformation(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 19 服务 - 读取故障码信息
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_19_ReadDTCInformation(const uint8_t* msg_buf, uint16_t msg_dlc);


/******************************************************************************
* 函数名称: bool_t service_19_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 19 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_19_check_len(const uint8_t* msg_buf, uint16_t msg_dlc);


#endif


/****************EOF****************/
