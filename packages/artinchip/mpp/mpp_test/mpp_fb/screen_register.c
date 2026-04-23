/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <rtconfig.h>
#ifdef RT_USING_FINSH
#include <rthw.h>
#include <rtthread.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <aic_core.h>

#include <aic_hal.h>
#include <aic_hal_de.h>
#include <aic_drv_de.h>
#include <aic_hal_dsi.h>
#include <aic_hal_dbi.h>
#include <mipi_display.h>

#include "mpp_fb.h"

struct display_cxt {
    aicos_sem_t sem;
    void *regs; /* display interface base register */
    unsigned int screen_reg;
    bool ready;
};

static struct display_cxt g_ctx = {0};

static long long int str2int(char *_str)
{
    if (_str == NULL) {
        pr_err("The string is empty!\n");
        return -1;
    }

    if (strncmp(_str, "0x", 2))
        return atoi(_str);
    else
        return strtoll(_str, NULL, 16);
}

static void usage(char *app)
{
    printf("Usage: %s [Options]\n", app);
    printf("\tmodify a screen register during the vsync \n");
    printf("\tMIPI-DBI interface: modify only once; MIPI-DSI interface: read each frame \n");
    printf("\t-r, --register \n");
    printf("\t-u, --usage \n");
    printf("\n");
    printf("Example: %s -r 0x04 \n", app);
}

#if defined(AIC_DISP_MIPI_DSI)
/* DE VSYNC INTERRUPT context */
void de_vsync_callback(void *data)
{
    struct display_cxt *ctx = data;

    void *regs = ctx->regs;
    u32 val = ctx->screen_reg;

    dsi_cmd_wr(regs, MIPI_DSI_DCS_READ, 0, (u8[]){ val }, 1);
    aicos_sem_give(ctx->sem);
}

static void read_register_thread(void *data)
{
    struct display_cxt *ctx = data;
    void *regs = ctx->regs;
    u32 val = 0;

    while (1) {
        aicos_sem_take(ctx->sem, AICOS_WAIT_FOREVER);

        aic_mdelay(8);
        val = readl(regs + DSI_GEN_PD_CFG);
        printf("mipi read %d\n", val);
        // TODO: check status or changes read regs
    }
}

static int screen_register_start(unsigned int reg)
{
    struct display_cxt *ctx = &g_ctx;
    aicos_thread_t thid = NULL;

    ctx->sem = aicos_sem_create(0);
    if (!ctx->sem) {
        printf("create display sem failed\n");
        return -1;
    }

    ctx->regs = (void *)MIPI_DSI_BASE;
    ctx->screen_reg = reg;
    ctx->ready = true;

    /* MIPI-DSI read each frame */
    thid = aicos_thread_create("disp", 4096, 0, read_register_thread, ctx);
    if (thid == NULL)
        printf("Failed to create display thread\n");
    else
        de_register_vsync_cb(de_vsync_callback, ctx);

    return 0;
}
#elif defined(AIC_DISP_MIPI_DBI)
/* DE VSYNC INTERRUPT context */
void de_vsync_callback(void *data)
{
    struct display_cxt *ctx = data;

    if (ctx->ready) {
        void *regs = ctx->regs;

        /* MIPI-DBI interface must reset before send cmd */
        hal_reset_assert(RESET_DBI);
        aic_udelay(2);
        hal_reset_deassert(RESET_DBI);

        aic_udelay(2);
        extern int aic_dbi_enable(void);
        aic_dbi_enable();

        /* default modify screen register 0x2a */
        spi_cmd_wr(regs, 0x2a, 4, (u8[]){ 0x0, 0x0, 0x0, 0x20 });
        ctx->ready = false;
    }
}

static int screen_register_start(unsigned int reg)
{
    struct display_cxt *ctx = &g_ctx;

    ctx->regs = (void *)DBI_BASE;
    ctx->screen_reg = reg;
    ctx->ready = true;

    de_register_vsync_cb(de_vsync_callback, ctx);

    /* just send cmd once */
    while (ctx->ready) {};
    de_unregister_vsync_cb();

    return 0;
}
#else
static int screen_register_start(unsigned int reg)
{
    (void)reg;
    (void)g_ctx;
    printf("Invalid display interface\n");
    return 0;
}
#endif

static int screen_register_test(int argc, char **argv)
{
    int c, reg = 0x09, ret = 0;

    const char sopts[] = "r:u";
    const struct option lopts[] = {
        {"register",    required_argument, NULL, 'r'},
        {"usage",             no_argument, NULL, 'u'},
        {0, 0, 0, 0}
    };

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'u':
            usage(argv[0]);
            return 0;
        case 'r':
        {
            reg = str2int(optarg);
            break;
        }
        default:
            pr_err("Invalid parameter: %#x\n", ret);
            usage(argv[0]);
            return 0;
        }
    }

    screen_register_start(reg);
    return 0;
}

MSH_CMD_EXPORT_ALIAS(screen_register_test, screen_register,
        test screen register value);
#endif /* RT_USING_FINSH */
