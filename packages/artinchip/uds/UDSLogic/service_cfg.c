/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include "service_cfg.h"
#include "uds_def.h"

#include "SID10_SessionControl.h"
#include "SID11_EcuReset.h"
#include "SID27_SecurityAccess.h"
#include "SID28_CommunicationControl.h"
#include "SID3E_TesterPresent.h"
#include "SID85_ControlDTCSetting.h"
#include "SID22_ReadDataByIdentifier.h"
#include "SID2E_WriteDataByIdentifier.h"
#include "SID14_ClearDiagnosticInformation.h"
#include "SID19_ReadDTCInformation.h"
#include "SID31_RoutineControl.h"
#include "SID34_RequestDownload.h"
#include "SID36_TransferData.h"
#include "SID37_RequestTransferExit.h"


// 服务配置表
const uds_service_t uds_service_list[SID_NUM]  =
{
    /* SID   服务处理函数                            长度是否合法            是否支持默认会话
                                                                                   是否支持编程会话
                                                                                          是否支持扩展会话
                                                                                                 是否支持功能寻址
                                                                                                        是否支持肯定响应抑制
                                                                                                               安全访问等级 */
    {SID_10, service_10_SessionControl,             service_10_check_len,  TRUE,   TRUE,  TRUE,  TRUE,  TRUE,  UDS_SA_NON},
    {SID_11, service_11_EcuReset,                   service_11_check_len,  TRUE,   TRUE,  TRUE,  TRUE,  TRUE,  UDS_SA_NON},
    {SID_27, service_27_SecurityAccess,             service_27_check_len,  TRUE,   TRUE,  TRUE,  FALSE, TRUE,  UDS_SA_NON},
    {SID_28, service_28_CommunicationControl,       service_28_check_len,  TRUE,   TRUE,  TRUE,  TRUE,  TRUE,  UDS_SA_NON},
    {SID_3E, service_3E_TesterPresent,              service_3E_check_len,  TRUE,   TRUE,  TRUE,  TRUE,  TRUE,  UDS_SA_NON},
    {SID_85, service_85_ControlDTCSetting,          service_85_check_len,  TRUE,   TRUE,  TRUE,  TRUE,  TRUE,  UDS_SA_NON},
    {SID_22, service_22_ReadDataByIdentifier,       service_22_check_len,  TRUE,   TRUE,  TRUE,  TRUE,  FALSE, UDS_SA_NON},
    {SID_2E, service_2E_WriteDataByIdentifier,      service_2E_check_len,  FALSE,  TRUE,  TRUE,  TRUE,  FALSE, UDS_SA_LV1},
    {SID_14, service_14_ClearDiagnosticInformation, service_14_check_len,  TRUE,   TRUE,  TRUE,  TRUE,  FALSE, UDS_SA_NON},
    {SID_19, service_19_ReadDTCInformation,         service_19_check_len,  TRUE,   TRUE,  TRUE,  TRUE,  FALSE, UDS_SA_NON},
    {SID_31, service_31_RoutineControl,             service_31_check_len,  TRUE,   TRUE,  TRUE,  FALSE, TRUE,  UDS_SA_LV1},
    {SID_34, service_34_RequestDownload,            service_34_check_len,  FALSE,  TRUE,  FALSE, FALSE, FALSE, UDS_SA_LV1},
    {SID_36, service_36_TransferData,               service_36_check_len,  FALSE,  TRUE,  FALSE, FALSE, FALSE, UDS_SA_LV1},
    {SID_37, service_37_RequestTransferExit,        service_37_check_len,  FALSE,  TRUE,  FALSE, FALSE, FALSE, UDS_SA_LV1},
};


/****************EOF****************/
