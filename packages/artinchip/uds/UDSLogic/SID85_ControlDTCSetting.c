/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "SID85_ControlDTCSetting.h"
#include "service_cfg.h"
#include "uds_def.h"


bool_t  dtc_setting = UDS_DTC_SETTING_ON;




/******************************************************************************
* 函数名称: bool_t service_85_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 85 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_85_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
{
	bool_t ret = FALSE;
	
	(void)msg_buf;
	if(2 == msg_dlc)
		ret = TRUE;

	return ret;
}


/******************************************************************************
* 函数名称: void service_85_ControlDTCSetting(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 85 服务 - 控制 DTC 设置
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_85_ControlDTCSetting(const uint8_t* msg_buf, uint16_t msg_dlc)
{
    uint8_t subfunction;
	uint8_t rsp_buf[8];
	subfunction = UDS_GET_SUB_FUNCTION (msg_buf[1]);

	switch (subfunction)
	{
		case UDS_DTC_SETTING_ON:
			dtc_setting = UDS_DTC_SETTING_ON;
		    rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_85);
		    rsp_buf[1] = subfunction;
		    uds_positive_rsp (rsp_buf,2);
		    break;
		case UDS_DTC_SETTING_OFF:
			dtc_setting = UDS_DTC_SETTING_OFF;
		    rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_85);
		    rsp_buf[1] = subfunction;
		    uds_positive_rsp (rsp_buf,2);
		    break;
		default:
		    uds_negative_rsp (SID_85,NRC_SUBFUNCTION_NOT_SUPPORTED);
		    break;
	}
}


/****************EOF****************/
