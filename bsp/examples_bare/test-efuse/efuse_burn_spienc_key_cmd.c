/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <efuse.h>
#include <console.h>
#include <aic_utils.h>
#include "spi_aes_key.h"

int write_efuse(char *msg, u32 offset, const void *val, u32 size)
{
#if defined(AIC_SID_BURN_SIMULATED)
    printf("eFuse %s:\n", msg);
    hexdump((unsigned char *)val, size, 1);
    return size;
#else
    return efuse_program(offset, val, size);
#endif
}

int burn_brom_spienc_bit(void)
{
    u32 offset = 0xFFFF, val;
    int ret;

#if defined(AIC_CHIP_D12X)
    offset = 0x4;
    val = 0;
    val |= (1 << 28); // SPIENC boot bit for brom
#elif defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    offset = 0x38;
    val = 0;
    val |= (1 << 16); // Secure boot bit for brom
    val |= (1 << 19); // SPIENC boot bit for brom
#endif
    ret = write_efuse("brom enable spienc secure bit", offset, (const void *)&val, 4);
    if (ret <= 0) {
        printf("Write BROM SPIENC bit error\n");
        return -1;
    }

    return 0;
}

int check_brom_spienc_bit(void)
{
    u32 offset = 0xFFFF, val, mskval = 0;
    int ret;

#if defined(AIC_CHIP_D12X)
    offset = 4;
    mskval = 0;
    mskval |= (1 << 28); // SPIENC boot bit for brom
#elif defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    offset = 0x38;
    mskval = 0;
    mskval |= (1 << 16); // Secure boot bit for brom
    mskval |= (1 << 19); // SPIENC boot bit for brom
#endif
    ret = efuse_read(offset, (void *)&val, 4);
    if (ret <= 0) {
        printf("Read secure bit efuse error.\n");
        return -1;
    }
    if ((val & mskval) == mskval) {
        printf("BROM SPIENC is ENABLED\n");
    } else {
        printf("BROM SPIENC is NOT enabled\n");
    }

    return 0;
}

int burn_jtag_lock_bit(void)
{
    u32 offset = 0xFFFF, val;
    int ret;

#if defined(AIC_CHIP_D12X)
    offset = 4;
    val = 0;
    val |= (1 << 24); // JTAG LOCK
#elif defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    offset = 0x38;
    val = 0;
    val |= (1 << 0); // JTAG LOCK
#endif
    ret = write_efuse("jtag lock bit", offset, (const void *)&val, 4);
    if (ret <= 0) {
        printf("Write JTAG LOCK bit error\n");
        return -1;
    }

    return 0;
}

int check_jtag_lock_bit(void)
{
    u32 offset = 0xFFFF, val, mskval = 0;
    int ret;

#if defined(AIC_CHIP_D12X)
    offset = 4;
    mskval = 0;
    mskval |= (1 << 24); // JTAG LOCK
#elif defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    offset = 0x38;
    mskval = 0;
    mskval |= (1 << 0); // JTAG LOCK
#endif
    ret = efuse_read(offset, (void *)&val, 4);
    if (ret <= 0) {
        printf("Read secure bit efuse error.\n");
        return -1;
    }
    if ((val & mskval) == mskval) {
        printf("JTAG LOCK   is ENABLED\n");
    } else {
        printf("JTAG LOCK   is NOT enabled\n");
    }

    return 0;
}

int burn_spienc_key(void)
{
    u32 offset = 0xFFFF;
    int ret;

#if defined(AIC_CHIP_D12X)
    offset = 0x20;
#elif defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    offset = 0xA0;
#endif
    ret = write_efuse("spi_aes.key", offset, (const void *)spi_aes_key, spi_aes_key_len);
    if (ret <= 0) {
        printf("Write SPI ENC AES key error.\n");
        return -1;
    }

    return 0;
}

int check_spienc_key(void)
{
    u32 offset = 0xFFFF;
    u8 data[256];
    int ret;

#if defined(AIC_CHIP_D12X)
    offset = 0x20;
#elif defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    offset = 0xA0;
#endif
    ret = efuse_read(offset, (void *)data, 16);
    if (ret <= 0) {
        printf("Read efuse error.\n");
        return -1;
    }
    printf("SPI ENC KEY:\n");
    hexdump(data, 16, 1);

    return 0;
}

int burn_spienc_nonce(void)
{
#if defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    u32 offset;
    int ret;

    offset = 0xB0;
    ret = write_efuse("spi_nonce.key", offset, (const void *)spi_nonce_key, spi_nonce_key_len);
    if (ret <= 0) {
        printf("Write SPI ENC NONCE key error.\n");
        return -1;
    }
#endif
    return 0;
}

int check_spienc_nonce(void)
{
#if defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    u32 offset;
    u8 data[256];
    int ret;

    offset = 0xB0;
    ret = efuse_read(offset, (void *)data, 8);
    if (ret <= 0) {
        printf("Read efuse error.\n");
        return -1;
    }
    printf("SPI ENC NONCE:\n");
    hexdump(data, 8, 1);

#endif
    return 0;
}

#if defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
int burn_spienc_rotpk(void)
{
    u32 offset = 0xFFFF;
    int ret;

#if defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    offset = 0x40;
#endif
    ret = write_efuse("rotpk.bin", offset, (const void *)rotpk_bin, rotpk_bin_len);
    if (ret <= 0) {
        printf("Write SPI ENC ROTPK error.\n");
        return -1;
    }

    return 0;
}

int check_spienc_rotpk(void)
{
    u32 offset = 0xFFFF;
    u8 data[256];
    int ret;

#if defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    offset = 0x40;
#endif
    ret = efuse_read(offset, (void *)data, 16);
    if (ret <= 0) {
        printf("Read efuse error.\n");
        return -1;
    }
    printf("ROTPK:\n");
    hexdump(data, 16, 1);

    return 0;
}
#endif

int burn_spienc_key_read_write_disable_bits(void)
{
#if defined(AIC_CHIP_D12X)
    u32 offset, val;
    int ret;

    offset = 0;
    val = 0;
    val = 0x0F000F00; // SPIENC Key Read/Write disable
    ret = write_efuse("spienc key r/w dis", offset, (const void *)&val, 4);
    if (ret <= 0) {
        printf("Write r/w disable bit efuse error.\n");
        return -1;
    }
#elif defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    u32 offset, val;
    int ret;

    // SPIENC KEY and NONCE
    offset = 0x4;
    val = 0;
    val = 0x00003F00; // SPIENC Key and Nonce Read disable
    ret = write_efuse("spienc key/nonce r dis", offset, (const void *)&val, 4);
    if (ret <= 0) {
        printf("Write r/w disable bit efuse error.\n");
        return -1;
    }

    //  ROTPK
    offset = 0x8;
    val = 0;
    val = 0x000F0000; // ROTPK Write disable
    ret = write_efuse("rotpk w dis", offset, (const void *)&val, 4);
    if (ret <= 0) {
        printf("Write r/w disable bit efuse error.\n");
        return -1;
    }
    // SPIENC KEY and NONCE
    offset = 0xC;
    val = 0;
    val = 0x00003F00; // SPIENC Key Write disable
    ret = write_efuse("spienc key/nonce w dis", offset, (const void *)&val, 4);
    if (ret <= 0) {
        printf("Write r/w disable bit efuse error.\n");
        return -1;
    }
#endif

    return 0;
}


int check_spienc_key_read_write_disable_bits(void)
{
#if defined(AIC_CHIP_D12X)
    u32 offset, val, mskval;
    int ret;

    offset = 0;
    mskval = 0xF00;
    ret = efuse_read(offset, (void *)&val, 4);
    if (ret <= 0) {
        printf("Read r/w disable bit efuse error.\n");
        return -1;
    }

    if ((val & mskval) == mskval)
        printf("SPI ENC Key is read DISABLED\n");
    else
        printf("SPI ENC Key is NOT read disabled\n");
    if (((val>>16) & 0xF00) == 0xF00)
        printf("SPI ENC Key is write DISABLED\n");
    else
        printf("SPI ENC Key is NOT write disabled\n");
#elif defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    u32 offset, val, mskval;
    int ret;

    offset = 0x4;
    mskval = 0x00003F00;
    ret = efuse_read(offset, (void *)&val, 4);
    if (ret <= 0) {
        printf("Read read disable bit efuse error.\n");
        return -1;
    }

    if ((val & mskval) == mskval)
        printf("SPI ENC Key is read DISABLED\n");
    else
        printf("SPI ENC Key is NOT read disabled\n");

    offset = 0x8;
    mskval = 0x000F0000;
    ret = efuse_read(offset, (void *)&val, 4);
    if (ret <= 0) {
        printf("Read write disable bit efuse error.\n");
        return -1;
    }

    if ((val & mskval) == mskval)
        printf("SPI ENC ROTPK is write DISABLED\n");
    else
        printf("SPI ENC ROTPK is NOT write disabled\n");

    offset = 0xC;
    mskval = 0x00003F00;
    ret = efuse_read(offset, (void *)&val, 4);
    if (ret <= 0) {
        printf("Read write disable bit efuse error.\n");
        return -1;
    }

    if ((val & mskval) == mskval)
        printf("SPI ENC Key is write DISABLED\n");
    else
        printf("SPI ENC Key is NOT write disabled\n");
#endif

    return 0;
}


int cmd_efuse_do_spienc(int argc, char **argv)
{
    int ret;

    efuse_init();
    efuse_write_enable();

#if defined(AIC_CHIP_D12X) || defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    ret = burn_brom_spienc_bit();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }

    ret = burn_spienc_key();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }

    ret = burn_spienc_nonce();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }

#if defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    ret = burn_spienc_rotpk();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }
#endif

    ret = burn_spienc_key_read_write_disable_bits();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }

    ret = burn_jtag_lock_bit();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }
#endif

    ret = check_brom_spienc_bit();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }

    ret = check_jtag_lock_bit();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }
    ret = check_spienc_key();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }

    ret = check_spienc_nonce();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }

#if defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    ret = check_spienc_rotpk();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }
#endif

    ret = check_spienc_key_read_write_disable_bits();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }

    efuse_write_disable();
    printf("\n");
    printf("Write SPI ENC eFuse done.\n");
#if defined(AIC_SID_BURN_SIMULATED)
    printf("WARNING: This is a dry run to check the eFuse content, key is not burn to eFuse yet.\n");
#endif
#if !defined(AIC_SID_CONTINUE_BOOT_BURN_AFTER)
    while (1)
        continue;
#endif
    return 0;
}

CONSOLE_CMD(efuse_spienc, cmd_efuse_do_spienc, "eFuse test example");
