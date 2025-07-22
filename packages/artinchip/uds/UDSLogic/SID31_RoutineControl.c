/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "SID31_RoutineControl.h"
#include "service_cfg.h"
#include "uds_def.h"


/*
 * 31 01 ff 00 擦除内存
 * 31 01 02 02 检查传输数据完整性
 * 31 01 02 03 更新条件查询
 * 31 01 ff 01 软硬件一致性检查
 * 31 01 df e0 从节点模式选择
 * 31 01 02 04 数字签名
*/ 


typedef enum __UDS_ROUTINE_CTRL_TYPE__
{
	UDS_ROUTINE_CTRL_NONE = 0,
	UDS_ROUTINE_CTRL_START = 0x01,
	UDS_ROUTINE_CTRL_STOP = 0x02,
	UDS_ROUTINE_CTRL_REQUEST_RESULT = 0x03
}uds_routine_ctrl_type;


/******************************************************************************
* 函数名称: bool_t service_31_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 31 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_31_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
{
	bool_t ret = FALSE;
	
	(void)msg_buf;
	if(msg_dlc > 4)
		ret = TRUE;

	return ret;
}


/******************************************************************************
* 函数名称: void service_31_RoutineControl(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 31 服务 - 例程控制
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_31_RoutineControl(const uint8_t* msg_buf, uint16_t msg_dlc)
{
    uint8_t subfunction;
	uint8_t rsp_buf[8];
	uint16_t rid;
	// uint16_t rid_n;
	bool_t find_rid;

    subfunction = UDS_GET_SUB_FUNCTION (msg_buf[1]);
	rid = ((uint16_t)msg_buf[2]) << 8;
	rid |= msg_buf[3];

	find_rid = FALSE;
	// for (rid_n = 0; rid_n < RTCTRL_NUM; rid_n++)
	// {
	// 	if (rtctrl_list[rid_n].rid == rid)
	// 	{
	// 		find_rid = TRUE;
	// 		break;
	// 	}
	// }

	rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_31);
	rsp_buf[1] = msg_buf[1];
	rsp_buf[2] = msg_buf[2];
	rsp_buf[3] = msg_buf[3];
    switch (subfunction)
	{
		case UDS_ROUTINE_CTRL_START:
		    if (find_rid == TRUE)
			{
                // if (rtctrl_list[rid_n].rtst == UDS_RT_ST_RUNNING)
				if(1)
				{
                    uds_negative_rsp (SID_31,NRC_REQUEST_SEQUENCE_ERROR);
				}
				else
				{
					// rtctrl_list[rid_n].init_routine ();
					uds_positive_rsp (rsp_buf,4);
				}
			}
			else
			{
				uds_negative_rsp (SID_31,NRC_REQUEST_OUT_OF_RANGE);
			}
		    break;
		case UDS_ROUTINE_CTRL_STOP:
		    if (find_rid == TRUE)
			{
                // if (rtctrl_list[rid_n].rtst == UDS_RT_ST_IDLE)
				if(1)
				{
                    uds_negative_rsp (SID_31,NRC_REQUEST_SEQUENCE_ERROR);
				}
				else
				{
					// rtctrl_list[rid_n].stop_routine ();
					uds_positive_rsp (rsp_buf,4);
				}
			}
			else
			{
				uds_negative_rsp (SID_31,NRC_REQUEST_OUT_OF_RANGE);
			}
		    break;
		case UDS_ROUTINE_CTRL_REQUEST_RESULT:
		    if (find_rid == TRUE)
			{
                // if (rtctrl_list[rid_n].rtst == UDS_RT_ST_IDLE)
				if(1)
				{
                    uds_negative_rsp (SID_31,NRC_REQUEST_SEQUENCE_ERROR);
				}
				else
				{
					// rsp_buf[4] = (uint8_t)rtctrl_list[rid_n].rtst;
				}
			}
			else
			{
				uds_negative_rsp (SID_31,NRC_REQUEST_OUT_OF_RANGE);
			}
		    break;
		default:
		    uds_negative_rsp (SID_31, NRC_SUBFUNCTION_NOT_SUPPORTED);
		    break;
	}
}


/****************EOF****************/
