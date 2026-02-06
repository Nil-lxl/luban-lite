/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _XIP_RAMCODE_H_
#define _XIP_RAMCODE_H_

#if defined(AIC_CHIP_D13X)
#include "xip_ramcode_d13x_bin.h"
#endif
#if defined(AIC_CHIP_D12X)
#include "xip_ramcode_d12x_bin.h"
#endif
#include "aic_core.h"
#define XIP_RAMCODE_API_INIT     (XIP_RAMCODE_LINK_ADDRESS + 0x0)
#define XIP_RAMCODE_API_READ     (XIP_RAMCODE_LINK_ADDRESS + 0x10)
#define XIP_RAMCODE_API_WRITE    (XIP_RAMCODE_LINK_ADDRESS + 0x20)
#define XIP_RAMCODE_API_ERASE    (XIP_RAMCODE_LINK_ADDRESS + 0x30)

/*
 * Move xip_ramcode.bin from "rodata" to destination SRAM: XIP_RAMCODE_LINK_ADDRESS
 *
 * This API should be called before the rest xip_ramcode API.
 */
static inline void aic_xip_ramcode_prepare(void)
{
	const unsigned char *ramcode;
	unsigned int ramcode_size;
	void *dest = (void *)XIP_RAMCODE_LINK_ADDRESS;

	ramcode = aic_xip_get_ramcode_address();
	ramcode_size = aic_xip_get_ramcode_size();

	memcpy(dest, ramcode, ramcode_size);
	aicos_dcache_clean_range((void *)dest, ramcode_size);
	aicos_icache_invalid();
}

/*
 * Initalize xip_ramcode's hardware setting.
 */
static inline int aic_xip_ramcode_init(uint32_t addr_4bmode, uint32_t spienc)
{
	int (*init)(uint32_t addr_4bmode, uint32_t spienc);

	init = (void *)XIP_RAMCODE_API_INIT;
	return init(addr_4bmode, spienc);
}

static inline int aic_xip_ramcode_spinor_read(uint32_t start, uint32_t size, uint8_t *output)
{
	int (*read)(uint32_t start, uint32_t size, uint8_t *output);

	read = (void *)XIP_RAMCODE_API_READ;
	return read(start, size, output);
}

static inline int aic_xip_ramcode_spinor_write(uint32_t start, uint32_t size, uint8_t *input)
{
	int (*write)(uint32_t start, uint32_t size, uint8_t *input);

	write = (void *)XIP_RAMCODE_API_WRITE;
	return write(start, size, input);
}

static inline int aic_xip_ramcode_spinor_erase(uint32_t start, uint32_t size)
{
	int (*erase)(uint32_t start, uint32_t size);

	erase = (void *)XIP_RAMCODE_API_ERASE;
	return erase(start, size);
}
#endif
