/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "SID14_ClearDiagnosticInformation.h"
#include "service_cfg.h"
#include "uds_def.h"

#define UDS_DTC_GROUP_ALL	 0xFFFFFF

/******************************************************************************
* 函数名称: bool_t service_14_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 14 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_14_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
{
	bool_t ret = FALSE;
	
	(void)msg_buf;
	if(4 == msg_dlc)
		ret = TRUE;

	return ret;
}


/******************************************************************************
* 函数名称: void service_14_ClearDiagnosticInformation(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 14 服务 - 清除诊断信息
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_14_ClearDiagnosticInformation(const uint8_t* msg_buf, uint16_t msg_dlc)
{
	uint8_t rsp_buf[8];
	uint32_t dtc_group = 0;

	dtc_group = 0;
	dtc_group |= ((uint32_t)msg_buf[1]) << 16;
	dtc_group |= ((uint32_t)msg_buf[2]) << 8;
	dtc_group |= ((uint32_t)msg_buf[3]) << 0;

	if (dtc_group == UDS_DTC_GROUP_ALL)
	{
		// clear_dtc_by_group (dtc_group);
		rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_14);
		uds_positive_rsp(rsp_buf, 1);
	}
	else
	{
		uds_negative_rsp(SID_14, NRC_REQUEST_OUT_OF_RANGE);
	}	
}


/****************EOF****************/
