/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#ifndef __ABSYSTEM_OS_H__
#define __ABSYSTEM_OS_H__

#ifdef __cplusplus
extern "C" {
#endif

void aic_set_upgrade_status(char *file_name);
int aic_ota_status_update(void);
int aic_upgrade_end(void);
int aic_get_rodata_to_mount(char *target_rodata);
int aic_get_data_to_mount(char *target_data);

#ifdef __cplusplus
}
#endif

#endif
