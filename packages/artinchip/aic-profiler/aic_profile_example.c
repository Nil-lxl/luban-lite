/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "aic_profiler.h"

static void aic_profiler_usage_1(void)
{
    int i = 100000;
    AIC_PROFILER_BEGIN;
    while (i--) {;};
    AIC_PROFILER_END;
}

static void aic_profiler_usage_2(void)
{
    int j = 50000;
    AIC_PROFILER_BEGIN_TAG("50000");
    while (j--) {;};
    AIC_PROFILER_END_TAG("50000");
}

/* can be used with multiple layers of nesting */
static void aic_profiler_usage_3(void)
{
    int i = 50000, j = 10000;

    AIC_PROFILER_BEGIN;
    while (i--) {;};
    AIC_PROFILER_BEGIN_TAG("10000");
    while (j--) {;};
    AIC_PROFILER_END_TAG("10000");
    AIC_PROFILER_END;
}

static int cmd_aic_profiler_test(int argc, char **argv)
{
    aic_profiler_config_t config = {0};

    aic_profiler_config_init(&config);
    aic_profiler_init(&config);

    aic_profiler_usage_1();
    aic_profiler_usage_2();
    aic_profiler_usage_3();

    aic_profiler_flush();
    aic_profiler_uninit();
}

#if defined(RT_USING_FINSH)
MSH_CMD_EXPORT_ALIAS(cmd_aic_profiler_test, aic_profiler_test, aic profiler test);
#elif defined(AIC_CONSOLE_BARE_DRV)
CONSOLE_CMD(aic_profiler_test, cmd_aic_profiler_test,  "aic profiler test");
#endif
