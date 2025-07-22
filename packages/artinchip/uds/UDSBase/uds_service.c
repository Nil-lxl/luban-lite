/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "service_cfg.h"
#include <rtthread.h>
#include "uds_def.h"
#include "SID10_SessionControl.h"
#include "SID27_SecurityAccess.h"

// UDS 应用层相关定时计数器
static uint32_t uds_timer[UDS_TIMER_CNT] = {0};

// 安全访问种子匹配错误次数
extern uint8_t uds_fsa_cnt;

// 肯定响应抑制标志，在收到的服务中若带有子功能，其子功能的最高位 bit7 表示肯定响应抑制位
// 当其置 1 时，则表示不需要回复肯定响应，只进行服务处理即可
static bool_t ssp_flg;

// 服务配置表
extern const uds_service_t uds_service_list[SID_NUM];

/******************************************************************************
* 函数名称: void uds_timer_start(uds_timer_t num)
* 功能说明: 启动应用层定时器
* 输入参数: uds_timer_t num              --定时器
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void uds_timer_start(uds_timer_t num)
{
	// 检查参数合法性
	if (num >= UDS_TIMER_CNT) return;

	// 启动 FSA 定时器
	if (num == UDS_TIMER_FSA)
        uds_timer[UDS_TIMER_FSA] = TIMEOUT_FSA + 1;

	// 启动 S3server 定时器
	if (num == UDS_TIMER_S3server)
	    uds_timer[UDS_TIMER_S3server] = TIMEOUT_S3server + 1;
}

/******************************************************************************
* 函数名称: static void uds_timer_stop (uds_timer_t num)
* 功能说明: 关闭应用层定时器
* 输入参数: uds_timer_t num              --定时器
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
static void uds_timer_stop(uds_timer_t num)
{
	// 检查参数合法性
	if (num >= UDS_TIMER_CNT) return;

	// 计数值清 0，表示关闭定时器
    uds_timer[num] = 0;
}

/******************************************************************************
* 函数名称: static int uds_timer_run(uds_timer_t num)
* 功能说明: 定时器计数运行
* 输入参数: uds_timer_t num              --定时器
* 输出参数: 无
* 函数返回: 0: 定时器已经被关闭; -1: 超时发生; 1: 定时器正在计时运行
* 其它说明: 该函数需要被 1ms 周期调用
******************************************************************************/
static int uds_timer_run(uds_timer_t num)
{
	// 检查参数合法性
	if (num >= UDS_TIMER_CNT) return 0;

	// 如果计数值为 0，表示定时器已经关闭，不再工作
    if (uds_timer[num] == 0)
	{
        return 0;						// 返回 0，定时器已经被关闭
	}
	// 如果计数值为 1，表示定时器超时已发生
	else if (uds_timer[num] == 1)
	{
		uds_timer[num] = 0;				// 关闭定时器
	    return -1;						// 返回 -1，发生超时
	}
	// 其余情况则表示定时器正在运行
	else
	{
		uds_timer[num]--;				// 计数值 -1
	    return 1;						// 返回 1，定时器正在计时运行
	}
}

/******************************************************************************
* 函数名称: int uds_timer_chk(uds_timer_t num)
* 功能说明: 检查定时器状态
* 输入参数: uds_timer_t num              --定时器
* 输出参数: 无
* 函数返回: 0: 定时器已停止运行;  1: 定时器正在计时运行
* 其它说明: 无
******************************************************************************/
int uds_timer_chk(uds_timer_t num)
{
	// 检查参数合法性
	if (num >= UDS_TIMER_CNT) return 0;

	// 如果定时器计数值 > 0,表示定时器正在工作，否则表示定时器已停止工作
	if (uds_timer[num] > 0)
	    return 1;
	else
		return 0;
}

/******************************************************************************
* 函数名称: static void uds_no_response(void)
* 功能说明: 无响应
* 输入参数: 无
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
static void uds_no_response(void)
{
	return;
}

/******************************************************************************
* 函数名称: void uds_negative_rsp(uint8_t sid, uds_nrc_em rsp_nrc)
* 功能说明: 否定响应
* 输入参数: uint8_t sid              	--服务 ID
    　　　　uds_nrc_em rsp_nrc          --否定响应具体原因
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void uds_negative_rsp(uint8_t sid, uds_negative_response_code_t rsp_nrc)
{
	uint8_t temp_buf[8] = {0};

	// 如果不是正确接收请求消息，等待响应，其它情况都属于否定响应，这时候需要开启 S3server 定时器
    if (rsp_nrc != NRC_SERVICE_BUSY)
        uds_timer_start(UDS_TIMER_S3server);

	// 在功能寻址下， 0x11,0x12,0x31,0x7E,0x7F 这几种否定响应是不需要回复的
    if (g_tatype == N_TATYPE_FUNCTIONAL)
	{
		if (NRC_SERVICE_NOT_SUPPORTED == rsp_nrc
		 || NRC_SUBFUNCTION_NOT_SUPPORTED == rsp_nrc
		 || NRC_REQUEST_OUT_OF_RANGE == rsp_nrc
		 || NRC_SUBFUNCTION_NOT_SUPPORTED_IN_ACTIVE_SESSION == rsp_nrc
		 || NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION == rsp_nrc)
		    return;
	}
	
	// 否定响应回复
	temp_buf[0] = NEGATIVE_RSP;
	temp_buf[1] = sid;
	temp_buf[2] = rsp_nrc;
	network_send_udsmsg(temp_buf, 3);
	return;
}

/******************************************************************************
* 函数名称: void uds_positive_rsp(uint8_t* data, uint16_t len)
* 功能说明: 肯定响应
* 输入参数: uint8_t* data             	--正响应回复数据首地址
    　　　　uint16_t len          		--正响应回复数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void uds_positive_rsp(uint8_t* data, uint16_t len)
{
	// 启动 S3server 定时器
	uds_timer_start (UDS_TIMER_S3server);
	
	// 肯定响应抑制标志，在收到的服务中若带有子功能，其子功能的最高位 bit7 表示肯定响应抑制位
	// 当其置 1 时，则表示不需要回复肯定响应
	if (ssp_flg == TRUE) return;

	// 调用网络层提供的数据发送接口将数据发送出去
	network_send_udsmsg(data, len);
    return;
}

/******************************************************************************
* 函数名称: static void uds_dataff_indication (uint16_t msg_dlc)
* 功能说明: 首帧指示服务处理
* 输入参数: n_result_t n_result			--处理结果
* 输出参数: 无
* 函数返回: 无
* 其它说明: TP 层接收到首帧后将会调用该函数
******************************************************************************/
static void uds_dataff_indication (n_result_t n_result)
{
	(void)n_result;
	// 关闭 S3server 定时器
	uds_timer_stop(UDS_TIMER_S3server);
}

/******************************************************************************
* 函数名称: static void uds_data_confirm(n_result_t n_result)
* 功能说明: 确认服务处理
* 输入参数: n_result_t n_result		--处理结果
* 输出参数: 无
* 函数返回: 无
* 其它说明: TP 层接收到错误帧或者一些异常超时时将会调用该函数
******************************************************************************/
static void uds_data_confirm(n_result_t n_result)
{
	(void)n_result;
	// 启动 S3server 定时器
	uds_timer_start(UDS_TIMER_S3server);
}

/******************************************************************************
* 函数名称: static void uds_data_indication (uint8_t* msg_buf, uint16_t msg_dlc, n_result_t n_result)
* 功能说明: 指示服务处理
* 输入参数: 无
* 输出参数: 无
* 函数返回: 无
* 其它说明: 在 n_result == N_OK 的情况下，TP 层通过调用该函数将一包完整的有效数据传递过来
******************************************************************************/
static void uds_data_indication (uint8_t* msg_buf, uint16_t msg_dlc, n_result_t n_result)
{
	uint8_t i;
	uint8_t sid;
	uint8_t ssp;

	// 关闭 S3server 定时器
	uds_timer_stop(UDS_TIMER_S3server);

	// 如果 TP 接收到的帧数据异常，则重启 S3server 定时器并退出
    if (n_result != N_OK)
	{
		uds_timer_start(UDS_TIMER_S3server);
	    return;
	}

	// 第一个字节区作为标识区分不同的服务
    sid = msg_buf[0];
	rt_kprintf("SID%d",sid);
	
	// 肯定响应抑制位，在收到的服务中若带有子功能，其子功能的最高位 bit7 表示肯定响应抑制位
	// 当其置 1 时，则表示不需要回复肯定响应，只执行即可
	ssp = UDS_GET_SUB_FUNCTION_SUPPRESS_POSRSP(msg_buf[1]);

    for (i = 0; i < SID_NUM; i++)
	{
		if(sid != uds_service_list[i].uds_sid)
			continue;

		// 检查是否支持功能寻址
		if(N_TATYPE_FUNCTIONAL == g_tatype && FALSE == uds_service_list[i].fun_spt)
		{
			uds_no_response();
			return;
		}

		// 检查会话状态是否支持
		if((UDS_SESSION_STD == get_current_session() && FALSE == uds_service_list[i].std_spt)
		    || (UDS_SESSION_PROG == get_current_session() && FALSE == uds_service_list[i].prog_spt)
			|| (UDS_SESSION_EXT == get_current_session() && FALSE == uds_service_list[i].ext_spt))
		{
			uds_negative_rsp(sid, NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
			return;
		}

		// 检查安全访问等级
		if(get_current_sa_lv() < uds_service_list[i].uds_sa)
		{
			uds_negative_rsp(sid, NRC_SECURITY_ACCESS_DENIED);
			return;
		}

		// 检查数据长度是否合法
		if(FALSE == uds_service_list[i].check_len(msg_buf, msg_dlc))
		{
			uds_negative_rsp(sid, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT);
			return;
		}

		// 肯定响应抑制标志，在收到的服务中若带有子功能，其子功能的最高位 bit7 表示肯定响应抑制位
		// 当其置 1 时，则表示不需要回复肯定响应，只进行服务处理即可
		if (uds_service_list[i].ssp_spt == TRUE && ssp == 0x01)
			ssp_flg = TRUE;
		else
			ssp_flg = FALSE;

		// 执行服务处理函数
		uds_service_list[i].uds_service(msg_buf, msg_dlc);
		
		return;
	}

	// 程序运行到这里说明没找到对应的服务 ID
	uds_negative_rsp(sid, NRC_SERVICE_NOT_SUPPORTED);
}

/******************************************************************************
* 函数名称: void service_task(void)
* 功能说明: 应用层任务处理
* 输入参数: 无
* 输出参数: 无
* 函数返回: 无
* 其它说明: 该函数需要被 1ms 周期调用
******************************************************************************/
void service_task(void)
{
	// 如果 S3server 定时器超时，复位当前会话状态和安全访问等级
	if (uds_timer_run(UDS_TIMER_S3server) < 0)
	{
		set_current_session(UDS_SESSION_STD);
		set_current_sa_lv(UDS_SA_NON);
	}

	// 如果 FSA 定时器超时，安全访问种子匹配错误次数清 0
	if (uds_timer_run(UDS_TIMER_FSA) < 0)
	{
        uds_fsa_cnt = 0;
	}
}

/******************************************************************************
* 函数名称: int service_init(void)
* 功能说明: 初始化
* 输入参数: 无
* 输出参数: 无
* 函数返回: 0: OK; -1: ERR
* 其它说明: 向 TP 层注册一些接口函数
******************************************************************************/
int service_init(void)
{
    nt_usdata_t usdata = {0};

    usdata.ffindication = uds_dataff_indication;
    usdata.indication = uds_data_indication;
    usdata.confirm = uds_data_confirm;

    return network_reg(&usdata);
}

/****************EOF****************/
