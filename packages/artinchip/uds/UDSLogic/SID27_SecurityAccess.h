/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _SID27_SECURITY_ACCESS_H_
#define _SID27_SECURITY_ACCESS_H_


#include <stdint.h>
#include "uds_def.h"
#include "uds_def.h"

/******************************************************************************
* 函数名称: void service_27_SecurityAccess(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 27 服务 - 安全访问
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_27_SecurityAccess(const uint8_t* msg_buf, uint16_t msg_dlc);


/******************************************************************************
* 函数名称: bool_t service_27_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 27 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_27_check_len(const uint8_t* msg_buf, uint16_t msg_dlc);


/******************************************************************************
* 函数名称: void set_current_sa_lv(uds_sa_lv level)
* 功能说明: 设置当前安全访问等级
* 输入参数: uds_sa_lv level        	--安全访问等级
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void set_current_sa_lv(uds_sa_lv level);


/******************************************************************************
* 函数名称: uds_session_t get_current_sa_lv(void)
* 功能说明: 获取当前安全访问等级
* 输入参数: 无
* 输出参数: 无
* 函数返回: 当前安全访问等级
* 其它说明: 无
******************************************************************************/
uds_sa_lv get_current_sa_lv(void);



#endif


/****************EOF****************/
