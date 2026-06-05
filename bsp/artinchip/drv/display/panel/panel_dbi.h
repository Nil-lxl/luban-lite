/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PANEL_DBI_H_
#define _PANEL_DBI_H_

#include <aic_hal_dbi.h>

#include "panel_com.h"

int mipi_dbi_dcs_send(struct aic_panel *panel, const u8 *data, size_t size);

#define panel_dbi_dcs_send_seq(panel, seq...) do {                  \
        static const u8 d[] = { seq };                              \
        int ret;                                                    \
        ret = mipi_dbi_dcs_send(panel, d, ARRAY_SIZE(d));           \
        if (ret < 0)                                                \
            return ret;                                             \
    } while (0)

int panel_dbi_default_enable(struct aic_panel *panel);

int panel_dbi_commands_execute(struct aic_panel *panel,
                    struct panel_dbi_commands *commands);

#endif /* _PANEL_DBI_H_ */
