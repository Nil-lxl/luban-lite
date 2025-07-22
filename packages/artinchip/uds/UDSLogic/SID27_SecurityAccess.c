/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "SID27_SecurityAccess.h"
#include "service_cfg.h"
#include "uds_def.h"
#include <stdlib.h>
#include <time.h>

#define UNLOCKKEY					0x00000000
#define UNLOCKSEED					0x00000000
#define UNDEFINESEED				0xFFFFFFFF
#define SEEDMASK					0x80000000
#define SHIFTBIT					1
#define ALGORITHMASK				0x42303131

#define UDS_SEED_LENGTH                   (0x04)
#define UDS_REQUEST_SEED                  (0x01)
#define UDS_SEND_KEY                      (0x02)
#define UDS_FAS_MAX_TIMES                 (0x02)  /* failed security access */

static uint8_t req_seed = 0;				// 接收到请求种子标志
static uint8_t org_seed_buf[UDS_SEED_LENGTH];


// 当前安全访问等级
static uds_sa_lv curr_sa = UDS_SA_NON;

// 安全访问种子匹配错误次数
uint8_t uds_fsa_cnt = 0;

/******************************************************************************
* 函数名称: void set_current_sa_lv(uds_sa_lv level)
* 功能说明: 设置当前安全访问等级
* 输入参数: uds_sa_lv level        	--安全访问等级
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void set_current_sa_lv(uds_sa_lv level)
{
	curr_sa = level;
}


/******************************************************************************
* 函数名称: uds_session_t get_current_sa_lv(void)
* 功能说明: 获取当前安全访问等级
* 输入参数: 无
* 输出参数: 无
* 函数返回: 当前安全访问等级
* 其它说明: 无
******************************************************************************/
uds_sa_lv get_current_sa_lv(void)
{
    return curr_sa;
}


/******************************************************************************
* 函数名称: static uint8_t rand_u8 (void)
* 功能说明: 获取随机数
* 输入参数: 无
* 输出参数: 无
* 函数返回: 8 位随机数
* 其它说明: 无
******************************************************************************/
static uint8_t rand_u8(void)
{
    static uint8_t initialized = 0;
    if (!initialized) {
        srand(time(NULL)); 
        initialized = 1;
    }
    return rand() % 0xFF;
}


/******************************************************************************
* 函数名称: static uint32_t seedTOKey(uint32_t seed)
* 功能说明: 安全访问算法
* 输入参数: uint32_t seed		--种子
* 输出参数: 无
* 函数返回: key 值
* 其它说明: 该算法需根据实际需求而定
******************************************************************************/
static uint32_t seedTOKey(uint32_t seed)
{
	return (~seed);
}


/******************************************************************************
* 函数名称: int uds_security_access(uint8_t* key_buf, uint8_t* seed_buf)
* 功能说明: 比较自己根据种子 seed 计算的 key 值与接收到的 key 值是否一致
* 输入参数: uint8_t* key_buf		--接收到的 key
    　　　　uint8_t* seed_buf		--种子
* 输出参数: 无
* 函数返回: 0: 一致; -1: 不一致
* 其它说明: 无
******************************************************************************/
int uds_security_access(uint8_t* key_buf, uint8_t* seed_buf)
{
	uint32_t key = 0;
	uint32_t seed = 0;
    
	key = (key_buf[0] << 24) |  (key_buf[1] << 16) |  (key_buf[2] << 8) |  key_buf[3];
	seed = (seed_buf[0] << 24) |  (seed_buf[1] << 16) |  (seed_buf[2] << 8) |  seed_buf[3];

	if (key == seedTOKey(seed))
	    return 0;
	else
	    return -1;
}


/******************************************************************************
* 函数名称: bool_t service_27_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 27 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_27_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
{
	bool_t ret = FALSE;
	uint8_t subfunction;

	subfunction = UDS_GET_SUB_FUNCTION(msg_buf[1]);
	
	if ((UDS_REQUEST_SEED == subfunction && 2 == msg_dlc)
		|| (UDS_SEND_KEY == subfunction && 6 == msg_dlc))

	{
		ret = TRUE;
	}

	return ret;
}


/******************************************************************************
* 函数名称: void service_27_SecurityAccess(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 27 服务 - 安全访问
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_27_SecurityAccess(const uint8_t* msg_buf, uint16_t msg_dlc)
{
    uint8_t subfunction;
	uint8_t rsp_buf[8];
	uint16_t i;

	subfunction = UDS_GET_SUB_FUNCTION(msg_buf[1]);

	switch (subfunction)
	{
		case UDS_REQUEST_SEED:	// 请求种子
		{
			// 锁定时间要求不能因模块断电被清零，这里暂未实现掉电保存的功能
			if (uds_timer_chk(UDS_TIMER_FSA) > 0)
			{
				uds_negative_rsp(SID_27, NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED);
				break;
			}
			req_seed = 1;
			rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_27);
			rsp_buf[1] = subfunction;
			for (i = 0; i < UDS_SEED_LENGTH; i++) 
			{
				// ECU 在已经解锁的情况下，如果再次收到请求种子，则返回种子 0x00000000
				if (curr_sa == UDS_SA_LV1)
					org_seed_buf[i] = 0;
				else
				    org_seed_buf[i] = rand_u8();
				rsp_buf[2+i] = org_seed_buf[i];
			}
			uds_positive_rsp (rsp_buf, UDS_SEED_LENGTH+2);
			break;
		}
		case UDS_SEND_KEY:		// 发送密钥
		{
			// 在发送秘钥前必须先请求种子
			if (req_seed == 0)
			{
				uds_negative_rsp(SID_27, NRC_REQUEST_SEQUENCE_ERROR);
				break;
			}
			req_seed = 0;

			// 判断发送过来的密钥和自己计算的密钥是否一致
			if (!uds_security_access((uint8_t *)&msg_buf[2], org_seed_buf))
			{
				rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_27);
				rsp_buf[1] = subfunction;
				uds_positive_rsp (rsp_buf,2);
				set_current_sa_lv(UDS_SA_LV1);
			}
			else
			{
				uds_fsa_cnt++;
				if (uds_fsa_cnt >= UDS_FAS_MAX_TIMES) 
				{
					// 密钥尝试次数超过限值
					uds_timer_start (UDS_TIMER_FSA); // 锁定时间要求不能因模块断电被清零，这里暂未实现掉电保存的功能
					uds_negative_rsp (SID_27, NRC_EXCEEDED_NUMBER_OF_ATTEMPTS);
				} else 
				{
					// 密钥无效
					uds_negative_rsp (SID_27, NRC_INVALID_KEY);
				}
			}
			break;
		}
		default:
			uds_negative_rsp (SID_27, NRC_SUBFUNCTION_NOT_SUPPORTED);
			break;
	}
}


/****************EOF****************/
