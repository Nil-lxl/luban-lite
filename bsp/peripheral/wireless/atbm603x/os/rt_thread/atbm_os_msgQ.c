
/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#include "atbm_hal.h"
#include "atbm_os_msgQ.h"


atbm_int8 atbm_os_MsgQ_Create(atbm_os_msgq *pmsgQ, atbm_void *pstack, atbm_uint32 item_size, atbm_uint32 item_num)
{
    pmsgQ->mq=rt_mq_create("wifi_msgq",item_size,item_num,RT_IPC_FLAG_FIFO);
	if(!pmsgQ->mq){
		ATBM_ERROR("err\n");
		return -1;
	}
	return 0;
}

atbm_int8 atbm_os_MsgQ_Delete(atbm_os_msgq *pmsgQ)
{
	return rt_mq_delete(pmsgQ->mq);
}

atbm_int8 atbm_os_MsgQ_Recv(atbm_os_msgq *pmsgQ, atbm_void *pbuf, atbm_uint32 val, int timeout)
{
    return rt_mq_recv(pmsgQ->mq, pbuf, val,timeout);
}


atbm_int8 atbm_os_MsgQ_Send(atbm_os_msgq *pmsgQ, atbm_void *pbuf, atbm_uint32 val, int timeout)
{
    return rt_mq_send_wait(pmsgQ->mq, pbuf,val,timeout);
}
