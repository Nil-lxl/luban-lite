/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
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
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
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
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
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
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
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
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
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
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
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
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
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
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
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
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = AIC_XPWM7_XPWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM0
    {
        .id = 8,
        .base = PWM0_BASE,
        .irq = PWM0_IRQn,
        .clk = CLK_PWM0,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM1
    {
        .id = 9,
        .base = PWM1_BASE,
        .irq = PWM1_IRQn,
        .clk = CLK_PWM1,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM2
    {
        .id = 10,
        .base = PWM2_BASE,
        .irq = PWM2_IRQn,
        .clk = CLK_PWM2,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM3
    {
        .id = 11,
        .base = PWM3_BASE,
        .irq = PWM3_IRQn,
        .clk = CLK_PWM3,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM4
    {
        .id = 12,
        .base = PWM4_BASE,
        .irq = PWM4_IRQn,
        .clk = CLK_PWM4,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM5
    {
        .id = 13,
        .base = PWM5_BASE,
        .irq = PWM5_IRQn,
        .clk = CLK_PWM5,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM6
    {
        .id = 14,
        .base = PWM6_BASE,
        .irq = PWM6_IRQn,
        .clk = CLK_PWM6,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM7
    {
        .id = 15,
        .base = PWM7_BASE,
        .irq = PWM7_IRQn,
        .clk = CLK_PWM7,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM8
    {
        .id = 16,
        .base = PWM8_BASE,
        .irq = PWM8_IRQn,
        .clk = CLK_PWM8,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM9
    {
        .id = 17,
        .base = PWM9_BASE,
        .irq = PWM9_IRQn,
        .clk = CLK_PWM9,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM10
    {
        .id = 18,
        .base = PWM10_BASE,
        .irq = PWM10_IRQn,
        .clk = CLK_PWM10,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM11
    {
        .id = 19,
        .base = PWM11_BASE,
        .irq = PWM11_IRQn,
        .clk = CLK_PWM11,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM12
    {
        .id = 20,
        .base = PWM12_BASE,
        .irq = PWM12_IRQn,
        .clk = CLK_PWM12,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM13
    {
        .id = 21,
        .base = PWM13_BASE,
        .irq = PWM13_IRQn,
        .clk = CLK_PWM13,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM14
    {
        .id = 22,
        .base = PWM14_BASE,
        .irq = PWM14_IRQn,
        .clk = CLK_PWM14,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
#ifdef AIC_USING_PWM15
    {
        .id = 23,
        .base = PWM15_BASE,
        .irq = PWM15_IRQn,
        .clk = CLK_PWM15,
        .def_level = 0,
        .polarity = XPWM_POLARITY_NORMAL,
        .set_clk_rate = XPWM_CLK_RATE,
        .tb_clk_rate = XPWM_TB_CLK_RATE,
        .xpwm_mode = XPWM_PWM_MODE,
    },
#endif
};

const int xpwm_pdata_size = ARRAY_SIZE(xpwm_pdata);


