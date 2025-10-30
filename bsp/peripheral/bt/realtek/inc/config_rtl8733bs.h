/**
 *******************************************************************************
 * Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
 *******************************************************************************
 * @file rtl8733BS_config.c
 * @brief RTL8733BS firmware and configuration
 * @details
 * @author Luke_lu
 * @version https://jira.realtek.com/browse/BTGPSQC-13272
 *                  UART Over HCI/ FW Estimate Profile:
 *                  rtl8723f_mp_chip_bt40_fw_asic_rom_patch_linux_uart_0xDDB7_72F4_220225_1606_new.dll
 *                  8733BS C-CUT
 * @date 2023-02-02
 */

#ifndef __CONFIG_RTL8733BS_H__
#define __CONFIG_RTL8733BS_H__

#include <stdint.h>

//The configuration item for setting the MAC address must be at the end of the config.
static unsigned char config_rtl8733bs[] =
{
    0x55, 0xab, 0x23, 0x87,
    0x24, 0x00,
    0x0c, 0x00, 0x10,
    0x02, 0x80, 0x92, 0x04, 0x50, 0xc5, 0xea, 0x19,
    0xe1, 0x1b, 0xfd, 0xaf, 0x5f, 0x01, 0xa4, 0x0b,
    0x8d, 0x00, 0x01, 0xfa,
    0x8f, 0x00, 0x01, 0xbf,
    0x30, 0x00, 0x06,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66
};

#endif
