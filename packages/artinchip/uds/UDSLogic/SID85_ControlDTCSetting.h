/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _SID85_CONTROL_DTC_SETTING_H_
#define _SID85_CONTROL_DTC_SETTING_H_


#include <stdint.h>
#include "uds_def.h"

/* uds DTC setting */
typedef enum __UDS_DTC_SETTING__
{
	UDS_DTC_SETTING_NONE = 0,
	UDS_DTC_SETTING_ON,         // 使能故障码设置
	UDS_DTC_SETTING_OFF         // 禁止故障码设置
}uds_dtc_setting;


/******************************************************************************
* 函数名称: void service_85_ControlDTCSetting(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 85 服务 - 控制 DTC 设置
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_85_ControlDTCSetting(const uint8_t* msg_buf, uint16_t msg_dlc);


/******************************************************************************
* 函数名称: bool_t service_85_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 85 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_85_check_len(const uint8_t* msg_buf, uint16_t msg_dlc);


#endif

/****************EOF****************/
