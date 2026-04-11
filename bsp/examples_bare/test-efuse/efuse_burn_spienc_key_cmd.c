/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
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
#include <aic_crc32.h>
#include "spi_aes_key.h"
#if defined(LPKG_USING_DFS_ELMFAT)
#include <dfs_file.h>
#include <dfs_elm.h>
#endif
#include <mmc.h>
#include <block_dev.h>

static uint32_t crc32_val = 0;

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

#if !defined(AIC_SID_BURN_DEBUG_MODE)
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
#endif

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

    if (spi_aes_key_len != 16) {
        printf("SPI ENC AES key length is not equal 16 bytes.\n ");
        return -1;
    }

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
    crc32_val = crc32(crc32_val, data, 16);
    printf("SPI ENC KEY crc32 value:0x%x\n", (u32)crc32(0, data, 16));

    return 0;
}

int burn_spienc_nonce(void)
{
#if defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    u32 offset;
    int ret;

    offset = 0xB0;
    if (spi_nonce_key_len != 8) {
        printf("SPI ENC NONCE key length is not equal 8 bytes.\n ");
        return -1;
    }

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
    crc32_val = crc32(crc32_val, data, 8);
    printf("SPI ENC NONCE crc32 value:0x%x\n", (u32)crc32(0, data, 8));

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
    if (rotpk_bin_len != 16) {
        printf("ROTPK bin length is not equal 16 bytes.\n ");
        return -1;
    }

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
    crc32_val = crc32(crc32_val, data, 16);
    printf("ROTPK crc32 value:0x%x\n", (u32)crc32(0, data, 16));
    printf("ROTPK:\n");
    hexdump(data, 16, 1);

    return 0;
}
#endif

#if !defined(AIC_SID_BURN_DEBUG_MODE)
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
#endif

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

int save_crc32_to_sdcard(void)
{
    int ret = 0;
#if defined(LPKG_USING_DFS_ELMFAT) && defined(AICUPG_SDCARD_ENABLE)
    struct dfs_fd fd = { 0 };
    char *devname = NULL, *file_buf = NULL;
    char str_chipid[64] = { 0 }, chipid[16] = { 0 };
    int i, length = 0;

    dfs_init();
    elm_init();

    ret = mmc_init(1);
    if (ret) {
        printf("sdmc 1 init failed.\n");
        return ret;
    }

    devname = block_get_name_by_id(1);

    if (dfs_mount(devname, "/", "elm", 0, DEVICE_TYPE_SDMC_DISK) < 0) {
        pr_err("Failed to mount %s with FatFS\n", devname);
        ret = -1;
        goto err2;
    } else {
        pr_info("mount %s ok\n", devname);
    }

    file_buf = (char *)aicos_malloc_align(0, 2048, CACHE_LINE_SIZE);
    if (!file_buf) {
        pr_err("Error, malloc buf failed.\n");
        ret = -1;
        goto err2;
    }
    memset((void *)file_buf, 0, 2048);

    if (dfs_file_open(&fd, "burn.log", O_CREAT | O_WRONLY | O_APPEND) < 0) {
        pr_err("Open burn.log failed.\n");
        ret = -1;
        goto err2;
    }

    if (efuse_read_chip_id(chipid)) {
        pr_err("Read chipid failed.\n");
        ret = -1;
        goto err1;
    }

    for (i = 0; i < sizeof(chipid); i++) {
        sprintf(&str_chipid[2 * i], "%02x", chipid[i]);
    }
    sprintf(file_buf, "chipid:%s, crc32:0x%x\n", str_chipid, (u32)crc32_val);

    length = dfs_file_write(&fd, file_buf, strlen(file_buf));
    if (length != strlen(file_buf)) {
        pr_err("Write file data failed, errno=%d\n", length);
        ret = -1;
        goto err1;
    }

err1:
    dfs_file_close(&fd);

err2:
    if (file_buf)
        aicos_free_align(0, file_buf);
#endif

    return ret;
}


int cmd_efuse_do_spienc(int argc, char **argv)
{
    int ret;
    crc32_val = 0;

    efuse_init();
    efuse_write_enable();

#if defined(AIC_CHIP_D12X) || defined(AIC_CHIP_D13X) || defined(AIC_CHIP_D21X) || defined(AIC_CHIP_G73X)
    ret = burn_spienc_key();
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

    ret = burn_spienc_nonce();
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
    ret = burn_spienc_rotpk();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }

    ret = check_spienc_rotpk();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }
#endif

    ret = burn_brom_spienc_bit();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }

    ret = check_brom_spienc_bit();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }

#if !defined(AIC_SID_BURN_DEBUG_MODE)
    ret = burn_spienc_key_read_write_disable_bits();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }

    ret = check_spienc_key_read_write_disable_bits();
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

    ret = check_jtag_lock_bit();
    if (ret) {
        efuse_write_disable();
        printf("Error\n");
        return -1;
    }
#endif
#endif

    efuse_write_disable();
    printf("\n");
    printf("Write SPI ENC eFuse done.\n");
    printf("All key crc32 value are 0x%x.\n", (u32)crc32_val);
    save_crc32_to_sdcard();
#if defined(AIC_SID_BURN_DEBUG_MODE)
    printf("WARNING: The debug mode, the key is visible to the CPU.\n");
#endif
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
