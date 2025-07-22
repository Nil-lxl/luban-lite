/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _SID2E_TEST_PRESENT_H_
#define _SID2E_TEST_PRESENT_H_

#include <stdint.h>
#include "uds_def.h"

/******************************************************************************
* 函数名称: void service_3E_TesterPresent(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 3E 服务 - 待机握手
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_3E_TesterPresent(const uint8_t* msg_buf, uint16_t msg_dlc);


/******************************************************************************
* 函数名称: bool_t service_3E_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 3E 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_3E_check_len(const uint8_t* msg_buf, uint16_t msg_dlc);


#endif
/****************EOF****************/
