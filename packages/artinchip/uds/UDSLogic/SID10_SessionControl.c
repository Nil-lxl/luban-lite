/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "SID10_SessionControl.h"
#include "service_cfg.h"
#include "SID27_SecurityAccess.h"

// 收到服务请求到做出响应之间的最大时间，单位：ms
#define P2_SERVER               50

// 回复 NRC 0x78 后到真正的正响应之间的最大时间，单位：ms，接收方接收到这个时间后要 x10
#define P2X_SERVER              400

// 诊断会话状态
static uds_session_t uds_session = UDS_SESSION_STD;


/******************************************************************************
* 函数名称: void set_current_session(uds_session_t session)
* 功能说明: 设置当前诊断会话状态
* 输入参数: uds_session_t session       --会话状态
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void set_current_session(uds_session_t session)
{
    uds_session = session;
}


/******************************************************************************
* 函数名称: uds_session_t get_current_session(void)
* 功能说明: 获取当前诊断会话状态
* 输入参数: 无
* 输出参数: 无
* 函数返回: 当前会话状态
* 其它说明: 无
******************************************************************************/
uds_session_t get_current_session(void)
{
    return uds_session;
}

/******************************************************************************
* 函数名称: bool_t service_10_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 10 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_10_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
{
	bool_t ret = FALSE;
	
	(void)msg_buf;
	if(2 == msg_dlc)
		ret = TRUE;

	return ret;
}


/******************************************************************************
* 函数名称: void service_10_SessionControl(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 10 服务 - 诊断会话控制
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_10_SessionControl(const uint8_t* msg_buf, uint16_t msg_dlc)
{
    uint8_t subfunction = 0;
	uint8_t rsp_buf[8];

	subfunction = UDS_GET_SUB_FUNCTION(msg_buf[1]);

	rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_10);
	rsp_buf[1] = subfunction;
	rsp_buf[2] = (uint8_t)(P2_SERVER >> 8);
	rsp_buf[3] = (uint8_t)(P2_SERVER & 0x00ff);
	rsp_buf[4] = (uint8_t)(P2X_SERVER >> 8);
	rsp_buf[5] = (uint8_t)(P2X_SERVER & 0x00ff);
	
    switch (subfunction)
	{
		case UDS_SESSION_STD:		// 默认会话
            set_current_session((uds_session_t)subfunction);
			set_current_sa_lv(UDS_SA_NON);
			uds_positive_rsp(rsp_buf, 6);
			break;

		case UDS_SESSION_PROG:		// 编程会话
            set_current_session((uds_session_t)subfunction);
			set_current_sa_lv(UDS_SA_NON);
			uds_positive_rsp(rsp_buf, 6);
            uds_timer_start(UDS_TIMER_S3server);
			break;

        case UDS_SESSION_EXT:		// 扩展会话
            set_current_session((uds_session_t)subfunction);
			set_current_sa_lv(UDS_SA_NON);
			uds_positive_rsp(rsp_buf, 6);
            uds_timer_start(UDS_TIMER_S3server);
			break;

		default:
			uds_negative_rsp(SID_10, NRC_SUBFUNCTION_NOT_SUPPORTED);
			break;
	}
}


/****************EOF****************/
