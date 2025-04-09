/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <aic_hal.h>
#include <aic_hal_rgb.h>

#include "drv_fb.h"

struct aic_rgb_comp
{
    void *regs;

    struct aic_panel *panel;
    u64 sclk_rate;
    u64 pll_disp_rate;
};
static struct aic_rgb_comp *g_aic_rgb_comp;

#ifdef AIC_DISP_PQ_TOOL
AIC_PQ_TOOL_PINMUX_CONFIG(disp_pinmux_config);
AIC_PQ_TOOL_SET_DISP_PINMUX_FOPS(disp_pinmux_config)
#endif

static struct aic_rgb_comp *aic_rgb_request_drvdata(void)
{
    return g_aic_rgb_comp;
}

static void aic_rgb_release_drvdata(void)
{

}

#ifdef AIC_DISP_RGB_DRV_V12
struct rgb_bits_valid {
    u32 mode;
    u32 format;
    u32 bits_valid;
};

static const struct rgb_bits_valid rgb_valid[] = {
    {
        .mode       = PRGB,
        .format     = PRGB_24BIT,
        .bits_valid = 0x777,
    },
    {
        .mode       = PRGB,
        .format     = PRGB_18BIT_LD,
        .bits_valid = 0x555,
    },
    {
        .mode       = PRGB,
        .format     = PRGB_18BIT_HD,
        .bits_valid = 0x555,
    },
    {
        .mode       = PRGB,
        .format     = PRGB_16BIT_LD,
        .bits_valid = 0x454,
    },
    {
        .mode       = PRGB,
        .format     = PRGB_16BIT_HD,
        .bits_valid = 0x454,
    },
    {
        .mode       = SRGB,
        .format     = SRGB_8BIT,
        .bits_valid = 0x777,
    },
    {
        .mode       = SRGB,
        .format     = SRGB_6BIT,
        .bits_valid = 0x555,
    },
};

static int aic_rgb_data_valid(u32 mode, u32 format, u32 *bits_valid)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(rgb_valid); ++i)
        if (rgb_valid[i].mode == mode && rgb_valid[i].format == format) {
            *bits_valid = rgb_valid[i].bits_valid;
            return 0;
        }

    return -EINVAL;
}
#endif

static int aic_rgb_clk_enable(void)
{
    struct aic_rgb_comp *comp = aic_rgb_request_drvdata();
    u32 pixclk = comp->panel->timings->pixelclock;

    hal_clk_set_freq(CLK_PLL_FRA2, comp->pll_disp_rate);
    hal_clk_set_rate(CLK_SCLK, comp->sclk_rate, comp->pll_disp_rate);
    hal_clk_set_rate(CLK_PIX, pixclk, comp->sclk_rate);

    hal_clk_enable(CLK_PLL_FRA2);
    hal_clk_enable(CLK_SCLK);
    hal_clk_enable(CLK_RGB);

    hal_reset_deassert(RESET_RGB);

    aic_rgb_release_drvdata();
    return 0;
}

static int aic_rgb_clk_disable(void)
{
    hal_reset_assert(RESET_RGB);

    hal_clk_disable(CLK_RGB);
    hal_clk_disable(CLK_SCLK);

    return 0;
}

static void aic_rgb_swap(void)
{
    struct aic_rgb_comp *comp = aic_rgb_request_drvdata();
    struct panel_rgb *rgb = comp->panel->rgb;
    u32 bits_valid = 0;

    if (rgb->data_mirror) {
        reg_set_bits(comp->regs + RGB_DATA_SEQ_SEL,
            RGB_DATA_OUT_SEL_MASK, RGB_DATA_OUT_SEL(7));

#ifdef AIC_DISP_RGB_DRV_V12
        if (aic_rgb_data_valid(rgb->mode, rgb->format, &bits_valid))
            pr_err("rgb data mirror failed, disable data mirror\n");
#endif
        reg_set_bits(comp->regs + RGB_DATA_SEQ_SEL,
            RGB_DATA_OUT_SEL_VALID_MASK, RGB_DATA_OUT_SEL_VALID(bits_valid));
    }

    if (rgb->data_order)
        reg_write(comp->regs + RGB_DATA_SEL, rgb->data_order);

    if (rgb->clock_phase)
        reg_set_bits(comp->regs + RGB_CLK_CTL,
            CKO_PHASE_SEL_MASK,
            CKO_PHASE_SEL(rgb->clock_phase));

    aic_rgb_release_drvdata();
}

static int aic_rgb_enable(void)
{
    struct aic_rgb_comp *comp = aic_rgb_request_drvdata();
    struct panel_rgb *rgb = comp->panel->rgb;

    reg_set_bits(comp->regs + RGB_LCD_CTL,
            RGB_LCD_CTL_MODE_MASK,
            RGB_LCD_CTL_MODE(rgb->mode));

    switch (rgb->mode) {
    case PRGB:
        reg_set_bits(comp->regs + RGB_LCD_CTL,
                RGB_LCD_CTL_PRGB_MODE_MASK,
                RGB_LCD_CTL_PRGB_MODE(rgb->format));
        aic_rgb_swap();
        break;
    case SRGB:
        if (rgb->format)
            reg_set_bit(comp->regs + RGB_LCD_CTL,
                    RGB_LCD_CTL_SRGB_MODE);
        aic_rgb_swap();
        break;
    default:
        pr_err("Invalid mode %d\n", rgb->mode);
        break;
    }

    reg_set_bit(comp->regs + RGB_LCD_CTL, RGB_LCD_CTL_EN);
    aic_rgb_release_drvdata();

    return 0;
}

static int aic_rgb_disable(void)
{
    struct aic_rgb_comp *comp = aic_rgb_request_drvdata();

    reg_clr_bit(comp->regs + RGB_LCD_CTL, RGB_LCD_CTL_EN);
    aic_rgb_release_drvdata();
    return 0;
}

static int aic_rgb_attach_panel(struct aic_panel *panel)
{
    struct aic_rgb_comp *comp = aic_rgb_request_drvdata();
    u32 pixclk = panel->timings->pixelclock;
    struct panel_rgb *rgb = panel->rgb;
    u64 pll_disp_rate = 0;
    int i = 0;

    comp->panel = panel;

    if (rgb->mode == PRGB)
#ifdef AIC_DISP_RGB_DRV_V10
        comp->sclk_rate = pixclk * 4;
#else
        comp->sclk_rate = pixclk * 2;
#endif
    else if (rgb->mode == SRGB)
#ifdef AIC_DISP_RGB_DRV_V10
        comp->sclk_rate = pixclk * 12;
#else
        comp->sclk_rate = pixclk * 6;
#endif

    pll_disp_rate = comp->sclk_rate;
    while (pll_disp_rate < PLL_DISP_FREQ_MIN)
    {
        pll_disp_rate = comp->sclk_rate * (2 << i);
        i++;
    }
    comp->pll_disp_rate = pll_disp_rate;

    aic_rgb_release_drvdata();
    return 0;
}

struct di_funcs aic_rgb_func = {
    .clk_enable = aic_rgb_clk_enable,
    .clk_disable = aic_rgb_clk_disable,
    .enable = aic_rgb_enable,
    .disable = aic_rgb_disable,
    .attach_panel = aic_rgb_attach_panel,
};

static int aic_rgb_probe(void)
{
    struct aic_rgb_comp *comp;

    comp = aicos_malloc(0, sizeof(*comp));
    if (!comp)
    {
        pr_err("alloc rgb comp failed\n");
        return -ENOMEM;
    }

    memset(comp, 0, sizeof(*comp));

    comp->regs = (void *)LCD_BASE;
    g_aic_rgb_comp = comp;

#ifdef AIC_DISP_PQ_TOOL
    AIC_PQ_SET_DSIP_PINMUX;
#endif

    return 0;
}

static void aic_rgb_remove(void)
{

}

struct platform_driver artinchip_rgb_driver = {
    .name = "artinchip-rgb",
    .component_type = AIC_RGB_COM,
    .probe = aic_rgb_probe,
    .remove = aic_rgb_remove,
    .di_funcs = &aic_rgb_func,
};

