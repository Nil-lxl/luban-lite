/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "SID36_TransferData.h"
#include "service_cfg.h"
#include "uds_def.h"

/******************************************************************************
* 函数名称: bool_t service_36_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 检查 36 服务数据长度是否合法
* 输入参数: uint16_t msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: TRUE: 合法; FALSE: 非法
* 其它说明: 无
******************************************************************************/
bool_t service_36_check_len(const uint8_t *msg_buf, uint16_t msg_dlc)
{
    bool_t ret = FALSE;

    ret = TRUE;

    return ret;
}

/******************************************************************************
* 函数名称: void service_36_TransferData(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 36 服务 - 数据传输
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
FILE *transfer_file = NULL;
static uint8_t expected_block_counter = 1;

void service_36_TransferData(const uint8_t *msg_buf, uint16_t msg_dlc)
{
    uint8_t rsp_buf[8];
    if (msg_buf[0] == 0x36) {
        uint8_t block_number = msg_buf[1];
        uint16_t data_length = msg_dlc - 2;

        const uint8_t *data_start = msg_buf + 2;

        if (block_number != expected_block_counter) {
            printf("Error: Block number mismatch! Expected %d, got %d\n", expected_block_counter,
                   block_number);

            rsp_buf[0] = 0x7F;
            rsp_buf[1] = SID_36;
            rsp_buf[2] = 0x24;
            uds_positive_rsp(rsp_buf, 3);
            return;
        }

        if (transfer_file == NULL) {
            transfer_file = fopen(uds_output_file_path, "wb");
            if (transfer_file == NULL) {
                printf("Error: Failed to open file %s for writing\n", uds_output_file_path);

                rsp_buf[0] = 0x7F;
                rsp_buf[1] = SID_36;
                rsp_buf[2] = 0x70; // NRC: UploadDownloadNotAccepted
                uds_positive_rsp(rsp_buf, 3);
                return;
            }
            printf("File %s opened for writing\n", uds_output_file_path);
        }

        size_t written = fwrite(data_start, 1, data_length, transfer_file);
        if (written != data_length) {
            // printf("Error: File write failed! Expected %d, wrote %d\n", data_length, written);

            rsp_buf[0] = 0x7F;
            rsp_buf[1] = SID_36;
            rsp_buf[2] = 0x72; // NRC: GeneralProgrammingFailure
            uds_positive_rsp(rsp_buf, 3);
            return;
        }

        fflush(transfer_file);
        expected_block_counter = (expected_block_counter % 255) + 1;
        printf("Successfully wrote %d bytes to %s\n", data_length, uds_output_file_path);
        rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_36);
        rsp_buf[1] = block_number;
    } else {
        uint8_t bn = 0;
        bn = msg_buf[1];
        rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_36);
        rsp_buf[1] = bn;
    }
    uds_positive_rsp(rsp_buf, 2);
}

/****************EOF****************/
