/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _XIP_RAMCODE_D12X_BIN_H_
#define _XIP_RAMCODE_D12X_BIN_H_

#define XIP_RAMCODE_LINK_ADDRESS 0x40000000

const unsigned char *aic_xip_get_ramcode_address(void);

unsigned int aic_xip_get_ramcode_size(void);

#endif //_XIP_RAMCODE_D12X_BIN_H_
