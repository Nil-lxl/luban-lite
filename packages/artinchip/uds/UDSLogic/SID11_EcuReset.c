/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "SID11_EcuReset.h"
#include "service_cfg.h"
#include "uds_def.h"

typedef enum __UDS_RESET_T_
{
	UDS_RESET_NONE = 0,
	UDS_RESET_HARD,
	UDS_RESET_KEYOFFON,
	UDS_RESET_SOFT
}uds_reset_t;


/******************************************************************************
* 函数名称: bool_t service_11_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 11 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_11_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
{
	bool_t ret = FALSE;
	
	(void)msg_buf;
	if(2 == msg_dlc)
		ret = TRUE;

	return ret;
}

/******************************************************************************
* 函数名称: void service_11_EcuReset(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 11 服务 - ECU 重启
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_11_EcuReset(const uint8_t* msg_buf, uint16_t msg_dlc)
{
    uint8_t subfunction;
	uint8_t rsp_buf[8];

	subfunction = UDS_GET_SUB_FUNCTION(msg_buf[1]);

	switch (subfunction)
	{
		case UDS_RESET_HARD:
			rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_11);
			rsp_buf[1] = UDS_RESET_HARD;
			uds_positive_rsp(rsp_buf, 2);
			
			// Wait a moment for hard reset
			// Add hard reset here
			break;

		case UDS_RESET_SOFT:
			rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_11);
			rsp_buf[1] = UDS_RESET_SOFT;
			uds_positive_rsp(rsp_buf, 2);

			// Wait a moment for hard reset
			// Add soft reset here
			break;
		
		default:
			uds_negative_rsp(SID_11, NRC_SUBFUNCTION_NOT_SUPPORTED);
			break;
	}
}


/****************EOF****************/
