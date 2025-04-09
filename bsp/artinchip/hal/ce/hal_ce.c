/*
 * Copyright (C) 2020-2024 Artinchip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <aic_log.h>
#include <hal_ce.h>

#define SECURE_SRAM_ADDR    (0x1000)
#define CE_REG_ICR          (0x000)
#define CE_REG_ISR          (0x004)
#define CE_REG_TAR          (0x008)
#define CE_REG_TCR          (0x00C)
#define CE_REG_TSR          (0x010)
#define CE_REG_TER          (0x014)
#define CE_REG_VER          (0xFFC)

int hal_crypto_init(void)
{
    int ret = 0;

    ret = hal_clk_enable(CLK_CE);
    if (ret < 0) {
        hal_log_err("Failed to enable CE clk.\n");
        return -EFAULT;
    }

    ret = hal_clk_enable_deassertrst(CLK_CE);
    if (ret < 0) {
        hal_log_err("Failed to reset CE deassert.\n");
        return -EFAULT;
    }

    writel(0x7, CE_BASE + CE_REG_ICR);
    /* Clear interrupt status */
    writel(0xf, CE_BASE + CE_REG_ISR);
    writel(0xffffffff, CE_BASE + CE_REG_TER);

    return 0;
}

int hal_crypto_deinit(void)
{
    hal_clk_disable(CLK_CE);
    hal_clk_disable_assertrst(CLK_CE);

    return 0;
}

void hal_crypto_dump_reg(void)
{
    hal_log_info("0x00 ICR 0x%08x\n", readl(CE_BASE + CE_REG_ICR));
    hal_log_info("0x04 ISR 0x%08x\n", readl(CE_BASE + CE_REG_ISR));
    hal_log_info("0x08 TAR 0x%08x\n", readl(CE_BASE + CE_REG_TAR));
    hal_log_info("0x0c TCR 0x%08x\n", readl(CE_BASE + CE_REG_TCR));
    hal_log_info("0x10 TSR 0x%08x\n", readl(CE_BASE + CE_REG_TSR));
    hal_log_info("0x14 TER 0x%08x\n", readl(CE_BASE + CE_REG_TER));
}

static inline s32 hal_crypto_start(struct crypto_task *task)
{
    writel((unsigned long)task, CE_BASE + CE_REG_TAR);
    writel((task->alg.alg_tag) | (1UL << 31), CE_BASE + CE_REG_TCR);
    return 0;
}

bool  hal_crypto_is_start(void)
{
    return ((readl(CE_BASE + CE_REG_TCR) & (1UL << 31)) == 0);
}

s32 hal_crypto_start_symm(struct crypto_task *task)
{
    return hal_crypto_start(task);
}

s32 hal_crypto_start_asym(struct crypto_task *task)
{
    return hal_crypto_start(task);
}

s32 hal_crypto_start_hash(struct crypto_task *task)
{
    return hal_crypto_start(task);
}

u32 hal_crypto_poll_finish(u32 alg_unit)
{
    u32 reg_val;

    /* Interrupt should be disabled, so here check and wait tmo */
    reg_val = readl(CE_BASE + CE_REG_ISR);

    return (reg_val & (0x01 << alg_unit));
}

void hal_crypto_irq_enable(u32 alg_unit)
{
    u32 reg_val;

    reg_val = readl(CE_BASE + CE_REG_ICR);
    reg_val |= (0x01 << alg_unit);
    writel(reg_val, CE_BASE + CE_REG_ICR);
}

void hal_crypto_irq_disable(u32 alg_unit)
{
    u32 reg_val;

    reg_val = readl(CE_BASE + CE_REG_ICR);
    reg_val &= ~(0x01 << alg_unit);
    writel(reg_val, CE_BASE + CE_REG_ICR);
}

void hal_crypto_pending_clear(u32 alg_unit)
{
    u32 reg_val;

    reg_val = readl(CE_BASE + CE_REG_ISR);
    if ((reg_val & (0x01 << alg_unit)) == (0x01 << alg_unit)) {
        reg_val &= ~(0x0f);
        reg_val |= (0x01 << alg_unit);
    }
    writel(reg_val, CE_BASE + CE_REG_ISR);
}

u32 hal_crypto_get_err(u32 alg_unit)
{
    return ((readl(CE_BASE + CE_REG_TER) >> (8 * alg_unit)) & 0xFF);
}

s32 hal_crypto_bignum_byteswap(u8 *bn, u32 len)
{
    u32 i, j;
    u8 val;

    if (len == 0)
        return (-1);

    i = 0;
    j = len - 1;

    while (i < j) {
        val = bn[i];
        bn[i] = bn[j];
        bn[j] = val;
        i++;
        j--;
    }
    return 0;
}

/* Big Number from little-endian to big-endian */
s32 hal_crypto_bignum_le2be(u8 *src, u32 slen, u8 *dst, u32 dlen)
{
    int i;

    memset(dst, 0, dlen);
    for (i = 0; i < slen && i < dlen; i++)
        dst[dlen - 1 - i] = src[i];

    return 0;
}

/* Big Number from big-endian to litte-endian */
s32 hal_crypto_bignum_be2le(u8 *src, u32 slen, u8 *dst, u32 dlen)
{
    int i;

    memset(dst, 0, dlen);
    for (i = 0; i < slen && i < dlen; i++)
        dst[i] = src[slen - 1 - i];

    return 0;
}
#define is_aes(alg) (((alg) & 0xF0) == 0)
#define is_des(alg) ((((alg) & 0xF0) == 0x10) || (((alg) & 0xF0) == 0x20))
#define is_hash(alg) (((alg) & 0xF0) == 0x40)
#define is_trng(alg) (((alg) & 0xF0) == 0x50)

void hal_crypto_dump_task(struct crypto_task *task, int len)
{
	u32 i, count;

	count = len / sizeof(struct crypto_task);
	for (i = 0; i < count; i++) {
		pr_err("task:               0x%08lx\n", (unsigned long)task);
		if (is_aes(task->alg.alg_tag)) {
			pr_err("  alg.alg_tag:      %08x\n",
			       task->alg.aes_ecb.alg_tag);
			pr_err("  alg.direction:    %u\n",
			       task->alg.aes_ecb.direction);
			pr_err("  alg.key_siz:      %u\n",
			       task->alg.aes_ecb.key_siz);
			pr_err("  alg.key_src:      %u\n",
			       task->alg.aes_ecb.key_src);
			pr_err("  alg.key_addr:     %08x\n",
			       task->alg.aes_ecb.key_addr);
			if (task->alg.alg_tag == ALG_AES_CBC)
				pr_err("  alg.iv_addr:      %08x\n",
				       task->alg.aes_cbc.iv_addr);

			if (task->alg.alg_tag == ALG_AES_CTR) {
				pr_err("  alg.ctr_in:       %08x\n",
				       task->alg.aes_ctr.ctr_in_addr);
				pr_err("  alg.ctr_out:      %08x\n",
				       task->alg.aes_ctr.ctr_out_addr);
			}
			if (task->alg.alg_tag == ALG_AES_CTS)
				pr_err("  alg.iv_addr:      %08x\n",
				       task->alg.aes_cts.iv_addr);

			if (task->alg.alg_tag == ALG_AES_XTS)
				pr_err("  alg.tweak_addr:   %08x\n",
				       task->alg.aes_xts.tweak_addr);

			pr_err("  data.in_addr      %08x\n",
			       task->data.in_addr);
			pr_err("  data.in_len       %u\n",
			       task->data.in_len);
			pr_err("  data.out_addr     %08x\n",
			       task->data.out_addr);
			pr_err("  data.out_len      %u\n",
			       task->data.out_len);
		}
		if (is_des(task->alg.alg_tag)) {
			pr_err("  alg.alg_tag:      %08x\n",
			       task->alg.des_ecb.alg_tag);
			pr_err("  alg.direction:    %u\n",
			       task->alg.des_ecb.direction);
			pr_err("  alg.key_siz:      %u\n",
			       task->alg.des_ecb.key_siz);
			pr_err("  alg.key_src:      %u\n",
			       task->alg.des_ecb.key_src);
			pr_err("  alg.key_addr:     %08x\n",
			       task->alg.des_ecb.key_addr);
			if ((task->alg.alg_tag == ALG_DES_CBC) ||
			    (task->alg.alg_tag == ALG_TDES_CBC))
				pr_err("  alg.iv_addr:      %08x\n",
				       task->alg.des_cbc.iv_addr);

			pr_err("  data.in_addr      %08x\n",
			       task->data.in_addr);
			pr_err("  data.in_len       %u\n",
			       task->data.in_len);
			pr_err("  data.out_addr     %08x\n",
			       task->data.out_addr);
			pr_err("  data.out_len      %u\n",
			       task->data.out_len);
		}
		if (task->alg.alg_tag == ALG_RSA) {
			pr_err("  alg.alg_tag:      %08x\n",
			       task->alg.rsa.alg_tag);
			pr_err("  alg.op_siz:       %u\n",
			       task->alg.rsa.op_siz);
			pr_err("  alg.m_addr:       %08x\n",
			       task->alg.rsa.m_addr);
			pr_err("  alg.d_e_addr:     %08x\n",
			       task->alg.rsa.d_e_addr);

			pr_err("  data.in_addr      %08x\n",
			       task->data.in_addr);
			pr_err("  data.in_len       %u\n",
			       task->data.in_len);
			pr_err("  data.out_addr     %08x\n",
			       task->data.out_addr);
			pr_err("  data.out_len      %u\n",
			       task->data.out_len);
		}
		if (is_hash(task->alg.alg_tag)) {
			pr_err("  alg.alg_tag:      %08x\n",
			       task->alg.hmac.alg_tag);
			pr_err("  alg.iv_mode:      %u\n",
			       task->alg.hmac.iv_mode);
			pr_err("  alg.iv_addr:      %08x\n",
			       task->alg.hmac.iv_addr);
			pr_err("  alg.key_addr:     %08x\n",
			       task->alg.hmac.hmac_key_addr);

			pr_err("  data.first        %u\n",
			       task->data.first_flag);
			pr_err("  data.last         %u\n",
			       task->data.last_flag);
			pr_err("  data.total_byte   %u\n",
			       task->data.total_bytelen);
			pr_err("  data.in_addr      %08x\n",
			       task->data.in_addr);
			pr_err("  data.in_len       %u\n",
			       task->data.in_len);
			pr_err("  data.out_addr     %08x\n",
			       task->data.out_addr);
			pr_err("  data.out_len      %u\n",
			       task->data.out_len);
		}
		if (is_trng(task->alg.alg_tag)) {
			pr_err("  alg.alg_tag:      %08x\n",
			       task->alg.trng.alg_tag);
			pr_err("  data.in_len       %u\n",
			       task->data.in_len);
			pr_err("  data.out_addr     %08x\n",
			       task->data.out_addr);
			pr_err("  data.out_len      %u\n",
			       task->data.out_len);
		}
		pr_err("  next:             %08x\n\n", task->next);
		task++;
	}
}
