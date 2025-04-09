/*
 * Copyright (c) 2024, ArtInChip Technology CO.,LTD. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Chen JunLong <junlong.chen@artinchip.com>
 */
#include <string.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <aic_log.h>
#include <hal_dce.h>
#include "aic_hal_clk.h"

int hal_dce_init(void)
{
    int ret = 0;

    ret = hal_clk_enable(CLK_DCE);
    if (ret < 0) {
        hal_log_err("Failed to enable DCE clk.\n");
        return -EFAULT;
    }

    ret = hal_clk_enable_deassertrst(CLK_DCE);
    if (ret < 0) {
        hal_log_err("Failed to reset DCE deassert.\n");
        return -EFAULT;
    }
    return 0;
}

void hal_dce_deinit(void)
{
    hal_clk_disable(CLK_DCE);
    hal_clk_disable_assertrst(CLK_DCE);
}

void hal_dce_checksum_start(u8 *data, u32 len)
{
	writel((u32)data, DCE_ADDR_REG);
	writel(len, DCE_LEN_REG);
	writel(DCE_ALG_SUM, DCE_CFG_REG);
	writel(0x1, DCE_CTL_REG);
}

u32 hal_dce_checksum_wait(void)
{
	int ret = 0, cnt = 0;
	u32 val;

	while (1) {
		val = readl(DCE_ISR_REG);
		if (val & DCE_SUM_FINISH_MSK) {
			ret = 0;
			break;
		}
		if (val & DCE_ERR_ALL_MSK) {
			ret = DCE_CALC_ERR;
			break;
		}
		cnt++;
		if (cnt >= DCE_WAIT_CNT) {
			ret = DCE_CALC_TMO;
			break;
		}
		aic_udelay(100);
	}
	/* Clear all status */
	writel(0xFFFFFFFF, DCE_ISR_REG);
	return ret;
}

u32 hal_dce_checksum_result(void)
{
	return readl(DCE_SUM_RST_REG);
}

/*
 * param    u32 out_bit_in_word:OUTPUT_BIT_IN_WORD_REV 0 or 1
 * param    u32 input_bit_in_byte:INPUT_BIT_IN_BYTE_REV 0 or 1
 * param    u32 input_bit_in_word:INPUT_BIT_IN_WORD_REV 0 or 1
 * param    u32 input_byte_in_word:INPUT_BYTE_IN_WORD_REV 0 or 1
 * return   0 success   other failed
 */
void hal_dce_crc32_cfg(u32 out_bit_in_word, u32 input_bit_in_byte,
        u32 input_bit_in_word, u32 input_byte_in_word)
{
    u32 val = 0;

    if (out_bit_in_word)
        val |= OUTPUT_BIT_IN_WORD_REV;
    if (input_bit_in_byte)
        val |= INPUT_BIT_IN_BYTE_REV;
    if (input_bit_in_word)
        val |= INPUT_BIT_IN_WORD_REV;
    if (input_byte_in_word)
        val |= INPUT_BYTE_IN_WORD_REV;
    writel(val, DCE_CRC_CFG_REG);
}

void hal_dce_crc32_xor_val(u32 val)
{
	writel(val, DCE_CRC_XOROUT_REG);
}

void hal_dce_crc32_start(u32 crc, u8 *data, u32 len)
{
    if (crc)
        writel(crc, DCE_CRC_INIT_REG);
	writel((u32)data, DCE_ADDR_REG);
	writel(len, DCE_LEN_REG);
	writel(DCE_ALG_CRC, DCE_CFG_REG);
	writel(0x1, DCE_CTL_REG);
}

int hal_dce_crc32_wait(void)
{
	int ret = 0, cnt = 0;
	u32 val;

	while (1) {
		val = readl(DCE_ISR_REG);
		if (val & DCE_CRC_FINISH_MSK) {
			ret = 0;
			break;
		}
		if (val & DCE_ERR_ALL_MSK) {
			ret = DCE_CALC_ERR;
			break;
		}
		cnt++;
		if (cnt >= DCE_WAIT_CNT) {
			ret = DCE_CALC_TMO;
			break;
		}
		aic_udelay(100);
	}
	/* Clear all status */
	writel(0xFFFFFFFF, DCE_ISR_REG);
	return ret;
}

u32 hal_dce_crc32_result(void)
{
	return readl(DCE_CRC_RST_REG);
}

u32 hal_get_version(void)
{
    return readl(DCE_GET_VERSION_REG);
}
