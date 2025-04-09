/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include "sys_met_hub.h"
#include "lv_port_disp.h"

#ifdef LPKG_USING_CPU_USAGE
#include "cpu_usage.h"
#endif

sys_met_hub* sys_met_hub_create(void)
{
    sys_met_hub* p = lv_mem_alloc(sizeof(sys_met_hub));
    if (p == NULL)
        return NULL;
    memset(p, 0, sizeof(sys_met_hub));
    return p;
}

int sys_met_hub_get_info(sys_met_hub* info)
{
    if (info == NULL)
        return -1;

    info->draw_fps = 0;
    info->cpu_usage = 0.0;
    info->mem_usage = 0.0;

    info->draw_fps = fbdev_draw_fps();

#ifdef RT_USING_MEMHEAP
    extern long get_mem_used(void);
    info->mem_usage = ((float)(get_mem_used())) / (1024.0 * 1024.0);
#endif

#ifdef LPKG_USING_CPU_USAGE
    info->cpu_usage = cpu_load_average();
#endif
    return 0;
}

void sys_met_hub_delete(sys_met_hub* info)
{
    if (info)
        lv_mem_free(info);
}
