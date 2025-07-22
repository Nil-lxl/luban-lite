/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "SID36_TransferData.h"
#include "service_cfg.h"
#include "uds_def.h"



/******************************************************************************
* 函数名称: bool_t service_36_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 36 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_36_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
{
	bool_t ret = FALSE;
	
	ret = TRUE;

	return ret;
}


/******************************************************************************
* 函数名称: void service_36_TransferData(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 36 服务 - 数据传输
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_36_TransferData(const uint8_t* msg_buf, uint16_t msg_dlc)
{
	uint8_t rsp_buf[8];
	uint8_t bn = 0;

	// 这里需要解析 36 服务报文
	bn = msg_buf[1];

	rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_36);
	rsp_buf[1] = bn;
	uds_positive_rsp(rsp_buf, 2);
}


/****************EOF****************/
