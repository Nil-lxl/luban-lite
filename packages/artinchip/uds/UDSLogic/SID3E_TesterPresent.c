/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "SID3E_TesterPresent.h"
#include "service_cfg.h"
#include "uds_def.h"

#define ZERO_SUBFUNCTION                  (0x00)


/******************************************************************************
* 函数名称: bool_t service_3E_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 3E 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_3E_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
{
	bool_t ret = FALSE;
	
	(void)msg_buf;
	if(2 == msg_dlc)
		ret = TRUE;

	return ret;
}


/******************************************************************************
* 函数名称: void service_3E_TesterPresent(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 3E 服务 - 待机握手
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_3E_TesterPresent(const uint8_t* msg_buf, uint16_t msg_dlc)
{
    uint8_t subfunction;
	uint8_t rsp_buf[8];

	subfunction = UDS_GET_SUB_FUNCTION (msg_buf[1]);
	if (subfunction == ZERO_SUBFUNCTION) 
	{
		rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_3E);
		rsp_buf[1] = subfunction;
		uds_positive_rsp (rsp_buf,2);
	}
	else
	{
		uds_negative_rsp(SID_3E, NRC_SUBFUNCTION_NOT_SUPPORTED);
	}
}


/****************EOF****************/
