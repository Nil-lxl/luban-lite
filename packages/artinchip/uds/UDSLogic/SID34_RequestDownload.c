/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "SID34_RequestDownload.h"
#include "service_cfg.h"
#include "uds_def.h"


// 36 服务数据传输报文总大小
#define TOTAL_LEN_36	(512 + 2)


/******************************************************************************
* 函数名称: bool_t service_34_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 34 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_34_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
{
	bool_t ret = FALSE;
	
	ret = TRUE;

	return ret;
}


/******************************************************************************
* 函数名称: void service_34_RequestDownload(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 34 服务 - 请求下载
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_34_RequestDownload(const uint8_t* msg_buf, uint16_t msg_dlc)
{
	uint8_t rsp_buf[8];

	// 这里需要解析 34 服务报文

	rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_34);
	rsp_buf[1] = 0x20;
	rsp_buf[2] = (uint8_t)(TOTAL_LEN_36 >> 8);
	rsp_buf[3] = (uint8_t)(TOTAL_LEN_36 >> 0);
	uds_positive_rsp(rsp_buf, 4);
}


/****************EOF****************/
