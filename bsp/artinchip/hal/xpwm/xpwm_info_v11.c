/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#include "aic_core.h"
#include "aic_hal_clk.h"
#include "hal_xpwm.h"

struct aic_xpwm_arg xpwm_pdata[] = {
#ifdef AIC_USING_XPWM0
    {
        .id = 0,
        .dma_id = DMA_ID_XPWM0,
        .base = XPWM0_BASE,
        .irq = XPWM0_IRQn,
        .clk = CLK_XPWM0,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_XPWM0_TB_CLK_RATE,
        .xpwm_mode = AIC_XPWM0_XPWM_MODE,
    },
#endif
#ifdef AIC_USING_XPWM1
    {
        .id = 1,
        .dma_id = DMA_ID_XPWM1,
        .base = XPWM1_BASE,
        .irq = XPWM1_IRQn,
        .clk = CLK_XPWM1,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_XPWM1_TB_CLK_RATE,
        .xpwm_mode = AIC_XPWM1_XPWM_MODE,
    },
#endif
#ifdef AIC_USING_XPWM2
    {
        .id = 2,
        .dma_id = DMA_ID_XPWM2,
        .base = XPWM2_BASE,
        .irq = XPWM2_IRQn,
        .clk = CLK_XPWM2,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_XPWM2_TB_CLK_RATE,
        .xpwm_mode = AIC_XPWM2_XPWM_MODE,
    },
#endif
#ifdef AIC_USING_XPWM3
    {
        .id = 3,
        .dma_id = DMA_ID_XPWM3,
        .base = XPWM3_BASE,
        .irq = XPWM3_IRQn,
        .clk = CLK_XPWM3,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_XPWM3_TB_CLK_RATE,
        .xpwm_mode = AIC_XPWM3_XPWM_MODE,
    },
#endif
#ifdef AIC_USING_XPWM4
    {
        .id = 4,
        .dma_id = DMA_ID_XPWM4,
        .base = XPWM4_BASE,
        .irq = XPWM4_IRQn,
        .clk = CLK_XPWM4,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_XPWM4_TB_CLK_RATE,
        .xpwm_mode = AIC_XPWM4_XPWM_MODE,
    },
#endif
#ifdef AIC_USING_XPWM5
    {
        .id = 5,
        .dma_id = DMA_ID_XPWM5,
        .base = XPWM5_BASE,
        .irq = XPWM5_IRQn,
        .clk = CLK_XPWM5,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_XPWM5_TB_CLK_RATE,
        .xpwm_mode = AIC_XPWM5_XPWM_MODE,
    },
#endif
#ifdef AIC_USING_XPWM6
    {
        .id = 6,
        .dma_id = DMA_ID_XPWM6,
        .base = XPWM6_BASE,
        .irq = XPWM6_IRQn,
        .clk = CLK_XPWM6,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_XPWM6_TB_CLK_RATE,
        .xpwm_mode = AIC_XPWM6_XPWM_MODE,
    },
#endif
#ifdef AIC_USING_XPWM7
    {
        .id = 7,
        .dma_id = DMA_ID_XPWM7,
        .base = XPWM7_BASE,
        .irq = XPWM7_IRQn,
        .clk = CLK_XPWM7,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_XPWM7_TB_CLK_RATE,
        .xpwm_mode = AIC_XPWM7_XPWM_MODE,
    },
#endif
#ifdef AIC_USING_R_XPWM0
    {
        .id = 4,
        .dma_id = DMA_ID_XPWM0,
        .base = R_XPWM0_BASE,
        .irq = R_XPWM0_IRQn,
        .clk = CLK_XPWM0,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_R_XPWM0_TB_CLK_RATE,
        .xpwm_mode = AIC_R_XPWM0_XPWM_MODE,
    },
#endif
#ifdef AIC_USING_R_XPWM1
    {
        .id = 5,
        .dma_id = DMA_ID_XPWM1,
        .base = R_XPWM1_BASE,
        .irq = R_XPWM1_IRQn,
        .clk = CLK_XPWM1,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_R_XPWM1_TB_CLK_RATE,
        .xpwm_mode = AIC_R_XPWM1_XPWM_MODE,
    },
#endif
#ifdef AIC_USING_R_XPWM2
    {
        .id = 6,
        .dma_id = DMA_ID_XPWM2,
        .base = R_XPWM2_BASE,
        .irq = R_XPWM2_IRQn,
        .clk = CLK_XPWM2,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_R_XPWM2_TB_CLK_RATE,
        .xpwm_mode = AIC_R_XPWM2_XPWM_MODE,
    },
#endif
#ifdef AIC_USING_R_XPWM3
    {
        .id = 7,
        .dma_id = DMA_ID_XPWM3,
        .base = R_XPWM3_BASE,
        .irq = R_XPWM3_IRQn,
        .clk = CLK_XPWM3,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_R_XPWM3_TB_CLK_RATE,
        .xpwm_mode = AIC_R_XPWM3_XPWM_MODE,
    },
#endif
#ifdef AIC_USING_R_PWM0
    {
        .id = 8,
        .base = R_PWM0_BASE,
        .irq = R_PWM0_IRQn,
        .clk = CLK_PWM0,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_R_PWM0_TB_CLK_RATE,
        .xpwm_mode = 0,
    },
#endif
#ifdef AIC_USING_R_PWM1
    {
        .id = 9,
        .base = R_PWM1_BASE,
        .irq = R_PWM1_IRQn,
        .clk = CLK_PWM1,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_R_PWM1_TB_CLK_RATE,
        .xpwm_mode = 0,
    },
#endif
#ifdef AIC_USING_R_PWM2
    {
        .id = 10,
        .base = R_PWM2_BASE,
        .irq = R_PWM2_IRQn,
        .clk = CLK_PWM2,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_R_PWM2_TB_CLK_RATE,
        .xpwm_mode = 0,
    },
#endif
#ifdef AIC_USING_R_PWM3
    {
        .id = 11,
        .base = R_PWM3_BASE,
        .irq = R_PWM3_IRQn,
        .clk = CLK_PWM3,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .tb_clk_rate = AIC_R_PWM3_TB_CLK_RATE,
        .xpwm_mode = 0,
    },
#endif
};

const int xpwm_pdata_size = ARRAY_SIZE(xpwm_pdata);


