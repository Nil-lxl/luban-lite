/*
 * Copyright (C) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "aic_ui.h"

#define USE_TEMPLATE_DEMO

void vscode_ui_init(void)
{
#ifdef USE_BASE_DEMO
    void base_ui_init();
    base_ui_init();
#endif

#ifdef USE_TEMPLATE_DEMO
    extern void template_ui_init();
    template_ui_init();
#endif
}

void ui_init(void)
{
    vscode_ui_init();
}
