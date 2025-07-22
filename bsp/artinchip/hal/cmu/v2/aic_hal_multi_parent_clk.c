/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_clk.h"

#define to_clk_multi_parent(_hw) \
    container_of(_hw, struct aic_clk_multi_parent_cfg, comm)

static int clk_multi_parent_enable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_multi_parent_cfg *mod = to_clk_multi_parent(comm_cfg);
    u32 i, val;

    if (!mod->table_gates) {
        hal_log_err("%s does not have valid table_gates\n", comm_cfg->name);
        return -EINVAL;
    }

    if (mod->table_gates[0] < 0)
        return 0;

    val = readl(cmu_reg(mod->offset_reg));

    for (i = 0; i < mod->num_gates; i++) {
        val |= (1 << mod->table_gates[i]);
        writel(val, cmu_reg(mod->offset_reg));
    }

    return 0;
}

static void clk_multi_parent_disable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_multi_parent_cfg *mod = to_clk_multi_parent(comm_cfg);
    u32 i, val;

    if (!mod->table_gates) {
        hal_log_err("%s does not have valid table_gates\n", comm_cfg->name);
        return;
    }

    if (mod->table_gates[0] < 0)
        return;

    val = readl(cmu_reg(mod->offset_reg));

    for (i = mod->num_gates - 1; i >= 0; i--) {
        val &= ~(1 << mod->table_gates[i]);
        writel(val, cmu_reg(mod->offset_reg));
    }
}

static int clk_multi_parent_enable_and_deassert_rst(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_multi_parent_cfg *mod = to_clk_multi_parent(comm_cfg);
    u32 val;
    s32 ret;

    ret = clk_multi_parent_enable(comm_cfg);
    if (ret)
        return ret;

    aicos_udelay(30);

    /* deassert rst */
    val = readl(cmu_reg(mod->offset_reg));
    val |= (1 << MOD_RSTN);
    writel(val, cmu_reg(mod->offset_reg));

    aicos_udelay(30);

    return 0;
}

static void clk_multi_parent_disable_and_assert_rst(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_multi_parent_cfg *mod = to_clk_multi_parent(comm_cfg);
    u32 val;

    /* assert rst */
    val = readl(cmu_reg(mod->offset_reg));
    val &= ~(1 << MOD_RSTN);
    writel(val, cmu_reg(mod->offset_reg));

    aicos_udelay(30);

    clk_multi_parent_disable(comm_cfg);

    aicos_udelay(30);
}

static int clk_multi_parent_mod_is_enable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_multi_parent_cfg *mod = to_clk_multi_parent(comm_cfg);
    u32 val, i, ret = 1;

    if (!mod->table_gates) {
        hal_log_err("%s does not have valid table_gates\n", comm_cfg->name);
        return -EINVAL;
    }

    if (mod->table_gates[0] < 0)
        return ret;

    val = readl(cmu_reg(mod->offset_reg));

    for (i = 0;  i < mod->num_gates; i++) {
        ret &= ((val & (1 << mod->table_gates[i])) >> mod->table_gates[i]);
        if (!ret)
            return ret;
    }

    return ret;
}

static unsigned long clk_multi_parent_mod_recalc_rate(struct aic_clk_comm_cfg *comm_cfg,
                                                      unsigned long parent_rate)
{
    unsigned long rate, val, div = 0, div_mask, parent_index = 0;
    struct aic_clk_multi_parent_cfg *mod = to_clk_multi_parent(comm_cfg);

    if (!mod->table_div) {
        hal_log_err("%s does not have valid table_div\n", comm_cfg->name);
        return parent_rate;
    }

    parent_index = (readl(cmu_reg(mod->offset_reg)) >> mod->mux_bit) & mod->mux_mask;

    if (parent_index >= mod->num_parents) {
        hal_log_err("%s parent clock index error!\n", comm_cfg->name);
        return parent_rate;
    }

    if (mod->num_parents != mod->num_div) {
        hal_log_err("%s parent number is not equal to divider number!\n", comm_cfg->name);
        return parent_rate;
    }

    if (mod->table_div[parent_index].shift < 0) {
        rate = parent_rate / mod->table_div[parent_index].wd.div;
    } else {
        div_mask = (1 << mod->table_div[parent_index].wd.width) - 1;
        val = readl(cmu_reg(mod->offset_reg));
        div = (val >> mod->table_div[parent_index].shift) & div_mask;
        rate = parent_rate / (div + 1);
    }

#ifdef FPGA_BOARD_ARTINCHIP
    rate = fpga_board_rate[mod->id];
#endif
    return rate;
}

static void try_best_divider(u32 rate, u32 parent_rate, u32 max_div, u32 *div)
{
    u32 tmp, i, min_delta = U32_MAX, best_div = 0;

    for (i = 1; i <= max_div; i++) {
        tmp = i * rate;
        if (parent_rate == tmp) {
            best_div = i;
            goto __out;
        }

        if (abs(parent_rate - tmp) < min_delta) {
            min_delta = abs(parent_rate - tmp);
            best_div = i;
        }
    }

__out:
    *div = best_div;
}

static long clk_multi_parent_mod_round_rate(struct aic_clk_comm_cfg *comm_cfg,
                                            unsigned long rate,
                                            unsigned long *prate)
{
    u32 rrate, parent_rate, parent_index;
    u32 div = 0, div_max;
    struct aic_clk_multi_parent_cfg *mod = to_clk_multi_parent(comm_cfg);

    if (!mod->table_div) {
        hal_log_err("%s does not have valid table_div\n", comm_cfg->name);
        return *prate;
    }

    parent_rate = *prate;
    parent_index = (readl(cmu_reg(mod->offset_reg)) >> mod->mux_bit) & mod->mux_mask;

    if (parent_index >= mod->num_parents) {
        hal_log_err("%s parent clock index error!\n", comm_cfg->name);
        return parent_rate;
    }

    if (mod->num_parents != mod->num_div) {
        hal_log_err("%s parent number is not equal to divider number!\n", comm_cfg->name);
        return parent_rate;
    }

    if (mod->table_div[parent_index].shift < 0) {
        rrate = parent_rate / mod->table_div[parent_index].wd.div;
    } else {
        div_max = 1 << mod->table_div[parent_index].wd.width;
        try_best_divider(rate, parent_rate, div_max, &div);
        rrate = parent_rate / div;
    }

#ifdef FPGA_BOARD_ARTINCHIP
    rrate = fpga_board_rate[mod->id];
#endif
    return rrate;
}

static int clk_multi_parent_mod_set_rate(struct aic_clk_comm_cfg *comm_cfg,
                                         unsigned long rate,
                                         unsigned long parent_rate)
{
    struct aic_clk_multi_parent_cfg *mod = to_clk_multi_parent(comm_cfg);
    u32 val, parent_index;
    u32 div = 0, div_max, div_mask;

    if (!mod->table_div){
        hal_log_err("%s does not have valid table_div\n", comm_cfg->name);
        return -EINVAL;
    }

    parent_index = (readl(cmu_reg(mod->offset_reg)) >> mod->mux_bit) & mod->mux_mask;

    if (parent_index >= mod->num_parents) {
        hal_log_err("%s parent clock index error!\n", comm_cfg->name);
        return -EINVAL;
    }

    if (mod->num_parents != mod->num_div) {
        hal_log_err("%s parent number is not equal to divider number!\n", comm_cfg->name);
        return -EINVAL;
    }

    if (mod->table_div[parent_index].shift < 0)
        return 0;

    div_max = 1 << mod->table_div[parent_index].wd.width;
    try_best_divider(rate, parent_rate, div_max, &div);
    div_mask = div_max - 1;

    val = readl(cmu_reg(mod->offset_reg));
    val &= ~(div_mask << mod->table_div[parent_index].shift);
    val |= (div - 1) << mod->table_div[parent_index].shift;
    writel(val, cmu_reg(mod->offset_reg));

    return 0;
}

static unsigned int clk_multi_parent_mod_get_parent(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_multi_parent_cfg *mod = to_clk_multi_parent(comm_cfg);
    u32 parent_index = (readl(cmu_reg(mod->offset_reg)) >> mod->mux_bit) & mod->mux_mask;

    if (parent_index >= mod->num_parents) {
        hal_log_err("%s parent clock index error!\n", comm_cfg->name);
        return -EINVAL;
    }

    if (mod->num_parents != mod->num_div) {
        hal_log_err("%s parent number is not equal to divider number!\n", comm_cfg->name);
        return -EINVAL;
    }

    return mod->parent_ids[parent_index];
}

static int clk_multi_parent_mod_set_parent(struct aic_clk_comm_cfg *comm_cfg,
                                           unsigned int parent_id)
{
    struct aic_clk_multi_parent_cfg *mod = to_clk_multi_parent(comm_cfg);
    u32 i, val, parent_index = 0xFFFF;

    for (i = 0; i < mod->num_parents; i++) {
        if (parent_id == mod->parent_ids[i]) {
            parent_index = i;
            break;
        }
    }

    if (parent_index == 0xFFFF) {
        return -EINVAL;
    }

    val = readl(cmu_reg(mod->offset_reg));
    val &= ~(mod->mux_mask << mod->mux_bit);
    val |= parent_index << mod->mux_bit;
    writel(val, cmu_reg(mod->offset_reg));

    return 0;
}

const struct aic_clk_ops aic_clk_multi_parent_ops = {
    .enable      = clk_multi_parent_enable,
    .disable     = clk_multi_parent_disable,
    .is_enabled  = clk_multi_parent_mod_is_enable,
    .recalc_rate = clk_multi_parent_mod_recalc_rate,
    .round_rate  = clk_multi_parent_mod_round_rate,
    .set_rate    = clk_multi_parent_mod_set_rate,
    .set_parent  = clk_multi_parent_mod_set_parent,
    .get_parent  = clk_multi_parent_mod_get_parent,
    .enable_clk_deassert_rst = clk_multi_parent_enable_and_deassert_rst,
    .disable_clk_assert_rst  = clk_multi_parent_disable_and_assert_rst,
};
