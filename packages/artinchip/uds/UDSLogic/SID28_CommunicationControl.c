/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "SID28_CommunicationControl.h"
#include "service_cfg.h"
#include "uds_def.h"


/* uds Communication control type */
typedef enum __UDS_CC_TYPE__
{
	UDS_CC_TYPE_NONE = 0,
	UDS_CC_TYPE_NORMAL,
	UDS_CC_TYPE_NM,
	UDS_CC_TYPE_NM_NOR
}uds_cc_type;


bool_t dis_normal_xmit;		// 禁止发送标志
bool_t dis_normal_recv;		// 禁止接收标志


/******************************************************************************
* 函数名称: bool_t service_28_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 28 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_28_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
{
	bool_t ret = FALSE;
	
	(void)msg_buf;
	if(3 == msg_dlc)
		ret = TRUE;

	return ret;
}


/******************************************************************************
* 函数名称: void service_28_CommunicationControl(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 28 服务 - 通讯控制
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_28_CommunicationControl(const uint8_t* msg_buf, uint16_t msg_dlc)
{
    uint8_t subfunction;
	uint8_t rsp_buf[8];
	uint8_t cc_type;

	subfunction = UDS_GET_SUB_FUNCTION(msg_buf[1]);
	cc_type = msg_buf[2];


    switch (subfunction)
	{
		case UDS_CC_MODE_RX_TX:		// 使能接收和发送
		    if (cc_type == UDS_CC_TYPE_NORMAL || cc_type == UDS_CC_TYPE_NM || cc_type == UDS_CC_TYPE_NM_NOR)
			{
			    dis_normal_xmit = FALSE;
				dis_normal_recv = FALSE;
				rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_28);
				rsp_buf[1] = subfunction;
				uds_positive_rsp(rsp_buf, 2);
			}
			else
			{
			    uds_negative_rsp(SID_28, NRC_REQUEST_OUT_OF_RANGE);
			}
		    break;
		case UDS_CC_MODE_NO_NO:		// 禁止接收、禁止发送
		    if (cc_type == UDS_CC_TYPE_NORMAL || cc_type == UDS_CC_TYPE_NM || cc_type == UDS_CC_TYPE_NM_NOR)
			{
			    dis_normal_xmit = TRUE;
				dis_normal_recv = TRUE;
				rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_28);
				rsp_buf[1] = subfunction;
				uds_positive_rsp(rsp_buf, 2);
			}
			else
			{
			    uds_negative_rsp(SID_28, NRC_REQUEST_OUT_OF_RANGE);
			}
			break;
		default:
		    uds_negative_rsp(SID_28, NRC_SUBFUNCTION_NOT_SUPPORTED);
		    break;
	}

}


/****************EOF****************/
