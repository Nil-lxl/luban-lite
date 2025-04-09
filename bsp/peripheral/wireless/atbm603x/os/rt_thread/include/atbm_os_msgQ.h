#ifndef ATBM_OS_MSGQ_H
#define ATBM_OS_MSGQ_H

typedef struct{
    rt_mq_t mq;
}atbm_os_msgq;

atbm_int8 atbm_os_MsgQ_Create(atbm_os_msgq *pmsgQ, atbm_void *pstack, atbm_uint32 item_size, atbm_uint32 item_num);
atbm_int8 atbm_os_MsgQ_Delete(atbm_os_msgq *pmsgQ);
atbm_int8 atbm_os_MsgQ_Recv(atbm_os_msgq *pmsgQ, atbm_void *pbuf, atbm_uint32 val, int timeout);
atbm_int8 atbm_os_MsgQ_Send(atbm_os_msgq *pmsgQ, atbm_void *pbuf, atbm_uint32 val, int timeout);

#endif /* ATBM_OS_MSGQ_H */

