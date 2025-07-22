/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _SID11_ECU_RESET_H_
#define _SID11_ECU_RESET_H_

#include <stdint.h>
#include "uds_def.h"


/******************************************************************************
* 函数名称: bool_t service_11_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 11 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_11_check_len(const uint8_t* msg_buf, uint16_t msg_dlc);


/******************************************************************************
* 函数名称: void service_11_EcuReset(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 11 服务 - ECU 重启
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_11_EcuReset(const uint8_t* msg_buf, uint16_t msg_dlc);


#endif

/****************EOF****************/
