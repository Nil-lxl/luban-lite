/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "uds_def.h"
#include <rtthread.h>
void print_uds_response(uint32_t id, uint8_t *data, uint8_t len)
{
    if (len == 0)
        return;

    // determine response type
    uint8_t is_positive = (data[0] == 0x40 + ((data[1] >> 4) & 0x0F));

    rt_kprintf("\n[UDS] RESPONSE: ID=0x%03X LEN=%d DATA: ", id, len);

    // print data
    for (int i = 0; i < len; i++) {
        if (is_positive) {
            rt_kprintf("%02X ", data[i]);
        } else {
            rt_kprintf("%02X ", data[i]);
        }
    }

    // Add explanation and description
    if (len >= 2) {
        rt_kprintf("\n[UDS] ");
        if (is_positive) {
            rt_kprintf("POSITIVE ");
            switch (data[1]) {
                case 0x01:
                    rt_kprintf("(Default Session)");
                    break;
                case 0x02:
                    rt_kprintf("(Programming Session)");
                    break;
                case 0x03:
                    rt_kprintf("(Extended Session)");
                    break;
                case 0xF1:
                    rt_kprintf("(Read DID)");
                    break;
                default:
                    rt_kprintf("(Service 0x%02X)", data[1] - 0x40);
            }
        } else {
            rt_kprintf("NEGATIVE ");
            rt_kprintf("(NRC: 0x%02X)", data[2]);
        }
    }
    rt_kprintf("\n");
}
