/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BLUFI_PROTOCOL_H__
#define __BLUFI_PROTOCOL_H__

#include "blufi_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLUFI_GREAT_VER 0x01 //Version + Subversion
#define BLUFI_SUB_VER   0x03 //Version + Subversion
#define BLUFI_VERSION   ((BLUFI_GREAT_VER << 8) | BLUFI_SUB_VER) //Version + Subversion

//function declare
void blufi_protocol_handler(uint8_t type, uint8_t *data, int len);

#ifdef __cplusplus
}
#endif

#endif  /* __BLUFI_PROTOCOL_H__ */
