/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#ifndef __OTA_H__
#define __OTA_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * OTA upgrade has two caches
 * OTA unpacking requires one cache because the cpio header info may be split into two transfer,
 * determine by OTA_HEAD_LEN
 * Burning requires a cache because burning requires address and length align,
 * determine by OTA_BURN_BUFF_LEN
 */
#define OTA_BURN_BUFF_LEN (2048 * 2)
#define OTA_BURN_LEN      2048
#define OTA_HEAD_LEN      (2048 * 2)
#define OTA_BUFF_LEN      2048
#define OTA_CPIO_INFO_LEN 512

int ota_init(void);
void ota_deinit(void);
int ota_shard_download_fun(char *buffer, int length);

#ifdef __cplusplus
}
#endif

#endif
