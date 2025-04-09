/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

struct aic_tsen_ch aic_tsen_chs[] = {
#ifdef AIC_USING_TSEN_CPU
    {
        .id = 0,
        .available = 1,
        .name = "tsen_cpu",
        .mode = AIC_TSEN0_MODE,
        .soft_mode = AIC_TSEN0_SOFT_MODE,
        .hta_enable = 0,
        .lta_enable = 0,
        .otp_enable = 0,
#ifndef CONFIG_FPGA_BOARD_ARTINCHIP
        .slope  = -1134,
        .offset = 2439001,
#endif
    },
#endif
#ifdef AIC_USING_TSEN_GPAI
    {
        .id = 1,
        .available = 1,
        .name = "tsen_gpai",
        .mode = AIC_TSEN1_MODE,
        .soft_mode = AIC_TSEN1_SOFT_MODE,
        .hta_enable = 0,
        .lta_enable = 0,
        .otp_enable = 0,
#ifndef CONFIG_FPGA_BOARD_ARTINCHIP
        .slope  = -1139,
        .offset = 2450566,
#endif
    }
#endif
};
