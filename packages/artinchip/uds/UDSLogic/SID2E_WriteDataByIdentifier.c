/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include "SID2E_WriteDataByIdentifier.h"
#include "service_cfg.h"
#include "uds_def.h"

/******************************************************************************
* 函数名称: bool_t service_2E_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 2E 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_2E_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
{
	bool_t ret = FALSE;
	
	(void)msg_buf;
	if(msg_dlc > 4)
		ret = TRUE;

	return ret;
}

/******************************************************************************
* 函数名称: void service_2E_WriteDataByIdentifier(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 2E 服务 - 根据标识符写入数据
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_2E_WriteDataByIdentifier(const uint8_t* msg_buf, uint16_t msg_dlc)
{
    uint8_t rsp_buf[8];
	uint16_t did;
	bool_t write_ret = 0;

	did = ((uint16_t)msg_buf[1]) << 8;
	did |= msg_buf[2];

	switch(did)
	{
		case 0x1234:
			// write_ret = eeprom_write(xxx, xxx);
			if(write_ret)
			{
				rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_2E);
				rsp_buf[1] = msg_buf[1];
				rsp_buf[2] = msg_buf[2];
				uds_positive_rsp(rsp_buf, 3);
			}
			else
			{
				uds_negative_rsp(SID_2E, NRC_GENERAL_PROGRAMMING_FAILURE);
			}
			break;
		
		default:
			uds_negative_rsp(SID_2E, NRC_REQUEST_OUT_OF_RANGE);
			break;
	}
}
/****************EOF****************/
