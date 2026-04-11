/*
 * Copyright (c) 2024, ArtInChip Technology CO.,LTD. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Chen JunLong <junlong.chen@artinchip.com>
 */
#ifndef _AIC_HAL_DCE_H_
#define _AIC_HAL_DCE_H_

#include <aic_common.h>
#include <aic_soc.h>

#define DCE_CTL_REG             (DCE_BASE + 0x000)
#define DCE_CFG_REG             (DCE_BASE + 0x004)
#define DCE_IRQ_REG             (DCE_BASE + 0x008)
#define DCE_ISR_REG             (DCE_BASE + 0x00C)
#define DCE_ADDR_REG            (DCE_BASE + 0x010)
#define DCE_LEN_REG             (DCE_BASE + 0x014)
#define DCE_RST_REG             (DCE_BASE + 0x040)
#define DCE_CRC_CFG_REG         (DCE_BASE + 0x080)
#define DCE_CRC_INIT_REG        (DCE_BASE + 0x084)
#define DCE_CRC_XOROUT_REG      (DCE_BASE + 0x088)
#define DCE_CRC_RST_REG         (DCE_BASE + 0x0C0)
#define DCE_SUM_RST_REG         (DCE_BASE + 0x140)
#define DCE_GET_VERSION_REG     (DCE_BASE + 0xFFC)

#define DCE_WAIT_CNT 10000
#define DCE_CALC_OK 0
#define DCE_CALC_ERR -1
#define DCE_CALC_TMO -2

#define DCE_ALG_CRC        0x1
#define DCE_ALG_SUM        0x2

#define DCE_ERR_ALL_MSK  0x07000000
#define DCE_CRC_FINISH_MSK 0x01
#define DCE_SUM_FINISH_MSK 0x02

#define OUTPUT_BIT_IN_WORD_REV  (0x1 << 0)
#define INPUT_BIT_IN_BYTE_REV   (0x1 << 1)
#define INPUT_BIT_IN_WORD_REV   (0x1 << 2)
#define INPUT_BYTE_IN_WORD_REV  (0x1 << 3)

int hal_dce_init(void);
void hal_dce_deinit(void);
void hal_dce_checksum_start(u8 *data, u32 len);
u32 hal_dce_checksum_wait(void);
u32 hal_dce_checksum_result(void);
void hal_dce_crc32_cfg(u32 out_bit_in_word, u32 input_bit_in_byte,
        u32 input_bit_in_word, u32 input_byte_in_word);
void hal_dce_crc32_xor_val(u32 val);
void hal_dce_crc32_start(u32 crc, u8 *data, u32 len);
int hal_dce_crc32_wait(void);
u32 hal_dce_crc32_result(void);
u32 hal_get_version(void);

#endif
