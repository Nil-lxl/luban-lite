/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Huahui.mai <huahui.mai@artinchip.com>
 */

#include <rtdef.h>
#include "aic_spi_lcd.h"

static void panel_st77916_enable(aic_spi_lcd_dev_t *dev)
{
    aic_spi_lcd_write_seq(dev, 0xF0, 0x28);
    aic_spi_lcd_write_seq(dev, 0xF2, 0x28);
    aic_spi_lcd_write_seq(dev, 0x73, 0xF0);
    aic_spi_lcd_write_seq(dev, 0x7C, 0xD1);
    aic_spi_lcd_write_seq(dev, 0x83, 0xE0);
    aic_spi_lcd_write_seq(dev, 0x84, 0x61);
    aic_spi_lcd_write_seq(dev, 0xF2, 0x82);
    aic_spi_lcd_write_seq(dev, 0xF0, 0x00);
    aic_spi_lcd_write_seq(dev, 0xF0, 0x01);
    aic_spi_lcd_write_seq(dev, 0xF1, 0x01);
    aic_spi_lcd_write_seq(dev, 0xB0, 0x69);
    aic_spi_lcd_write_seq(dev, 0xB1, 0x4A);
    aic_spi_lcd_write_seq(dev, 0xB2, 0x2F);
    aic_spi_lcd_write_seq(dev, 0xB4, 0x69);
    aic_spi_lcd_write_seq(dev, 0xB5, 0x45);
    aic_spi_lcd_write_seq(dev, 0xB6, 0xAB);
    aic_spi_lcd_write_seq(dev, 0xB7, 0x41);
    aic_spi_lcd_write_seq(dev, 0xB8, 0x86);
    aic_spi_lcd_write_seq(dev, 0xBA, 0x00);
    aic_spi_lcd_write_seq(dev, 0xBB, 0x08);
    aic_spi_lcd_write_seq(dev, 0xBC, 0x08);
    aic_spi_lcd_write_seq(dev, 0xBD, 0x00);
    aic_spi_lcd_write_seq(dev, 0xC0, 0x80);
    aic_spi_lcd_write_seq(dev, 0xC1, 0x10);
    aic_spi_lcd_write_seq(dev, 0xC2, 0x37);
    aic_spi_lcd_write_seq(dev, 0xC3, 0x80);
    aic_spi_lcd_write_seq(dev, 0xC4, 0x10);
    aic_spi_lcd_write_seq(dev, 0xC5, 0x37);
    aic_spi_lcd_write_seq(dev, 0xC6, 0xA9);
    aic_spi_lcd_write_seq(dev, 0xC7, 0x41);
    aic_spi_lcd_write_seq(dev, 0xC8, 0x01);
    aic_spi_lcd_write_seq(dev, 0xC9, 0xA9);
    aic_spi_lcd_write_seq(dev, 0xCA, 0x41);
    aic_spi_lcd_write_seq(dev, 0xCB, 0x01);
    aic_spi_lcd_write_seq(dev, 0xD0, 0x91);
    aic_spi_lcd_write_seq(dev, 0xD1, 0x68);
    aic_spi_lcd_write_seq(dev, 0xD2, 0x68);
    aic_spi_lcd_write_seq(dev, 0xF5, 0x00, 0xA5);
    aic_spi_lcd_write_seq(dev, 0xF1, 0x10);
    aic_spi_lcd_write_seq(dev, 0xF0, 0x00);
    aic_spi_lcd_write_seq(dev, 0xF0, 0x02);
    aic_spi_lcd_write_seq(dev, 0xE0, 0xF0, 0x10, 0x18, 0x0D, 0x0C, 0x38, 0x3E, 0x44, 0x51, 0x39, 0x15, 0x15, 0x30, 0x34);
    aic_spi_lcd_write_seq(dev, 0xE1, 0xF0, 0x0F, 0x17, 0x0D, 0x0B, 0x07, 0x3E, 0x33, 0x51, 0x39, 0x15, 0x15, 0x30, 0x34);
    aic_spi_lcd_write_seq(dev, 0xF0, 0x10);
    aic_spi_lcd_write_seq(dev, 0xF3, 0x10);
    aic_spi_lcd_write_seq(dev, 0xE0, 0x08);
    aic_spi_lcd_write_seq(dev, 0xE1, 0x00);
    aic_spi_lcd_write_seq(dev, 0xE2, 0x00);
    aic_spi_lcd_write_seq(dev, 0xE3, 0x00);
    aic_spi_lcd_write_seq(dev, 0xE4, 0xE0);
    aic_spi_lcd_write_seq(dev, 0xE5, 0x06);
    aic_spi_lcd_write_seq(dev, 0xE6, 0x21);
    aic_spi_lcd_write_seq(dev, 0xE7, 0x03);
    aic_spi_lcd_write_seq(dev, 0xE8, 0x05);
    aic_spi_lcd_write_seq(dev, 0xE9, 0x02);
    aic_spi_lcd_write_seq(dev, 0xEA, 0xE9);
    aic_spi_lcd_write_seq(dev, 0xEB, 0x00);
    aic_spi_lcd_write_seq(dev, 0xEC, 0x00);
    aic_spi_lcd_write_seq(dev, 0xED, 0x14);
    aic_spi_lcd_write_seq(dev, 0xEE, 0xFF);
    aic_spi_lcd_write_seq(dev, 0xEF, 0x00);
    aic_spi_lcd_write_seq(dev, 0xF8, 0xFF);
    aic_spi_lcd_write_seq(dev, 0xF9, 0x00);
    aic_spi_lcd_write_seq(dev, 0xFA, 0x00);
    aic_spi_lcd_write_seq(dev, 0xFB, 0x30);
    aic_spi_lcd_write_seq(dev, 0xFC, 0x00);
    aic_spi_lcd_write_seq(dev, 0xFD, 0x00);
    aic_spi_lcd_write_seq(dev, 0xFE, 0x00);
    aic_spi_lcd_write_seq(dev, 0xFF, 0x00);
    aic_spi_lcd_write_seq(dev, 0x60, 0x40);
    aic_spi_lcd_write_seq(dev, 0x61, 0x05);
    aic_spi_lcd_write_seq(dev, 0x62, 0x00);
    aic_spi_lcd_write_seq(dev, 0x63, 0x42);
    aic_spi_lcd_write_seq(dev, 0x64, 0xDA);
    aic_spi_lcd_write_seq(dev, 0x65, 0x00);
    aic_spi_lcd_write_seq(dev, 0x66, 0x00);
    aic_spi_lcd_write_seq(dev, 0x67, 0x00);
    aic_spi_lcd_write_seq(dev, 0x68, 0x00);
    aic_spi_lcd_write_seq(dev, 0x69, 0x00);
    aic_spi_lcd_write_seq(dev, 0x6A, 0x00);
    aic_spi_lcd_write_seq(dev, 0x6B, 0x00);
    aic_spi_lcd_write_seq(dev, 0x70, 0x40);
    aic_spi_lcd_write_seq(dev, 0x71, 0x04);
    aic_spi_lcd_write_seq(dev, 0x72, 0x00);
    aic_spi_lcd_write_seq(dev, 0x73, 0x42);
    aic_spi_lcd_write_seq(dev, 0x74, 0xD9);
    aic_spi_lcd_write_seq(dev, 0x75, 0x00);
    aic_spi_lcd_write_seq(dev, 0x76, 0x00);
    aic_spi_lcd_write_seq(dev, 0x77, 0x00);
    aic_spi_lcd_write_seq(dev, 0x78, 0x00);
    aic_spi_lcd_write_seq(dev, 0x79, 0x00);
    aic_spi_lcd_write_seq(dev, 0x7A, 0x00);
    aic_spi_lcd_write_seq(dev, 0x7B, 0x00);
    aic_spi_lcd_write_seq(dev, 0x80, 0x48);
    aic_spi_lcd_write_seq(dev, 0x81, 0x00);
    aic_spi_lcd_write_seq(dev, 0x82, 0x07);
    aic_spi_lcd_write_seq(dev, 0x83, 0x02);
    aic_spi_lcd_write_seq(dev, 0x84, 0xD7);
    aic_spi_lcd_write_seq(dev, 0x85, 0x04);
    aic_spi_lcd_write_seq(dev, 0x86, 0x00);
    aic_spi_lcd_write_seq(dev, 0x87, 0x00);
    aic_spi_lcd_write_seq(dev, 0x88, 0x48);
    aic_spi_lcd_write_seq(dev, 0x89, 0x00);
    aic_spi_lcd_write_seq(dev, 0x8A, 0x09);
    aic_spi_lcd_write_seq(dev, 0x8B, 0x02);
    aic_spi_lcd_write_seq(dev, 0x8C, 0xD9);
    aic_spi_lcd_write_seq(dev, 0x8D, 0x04);
    aic_spi_lcd_write_seq(dev, 0x8E, 0x00);
    aic_spi_lcd_write_seq(dev, 0x8F, 0x00);
    aic_spi_lcd_write_seq(dev, 0x90, 0x48);
    aic_spi_lcd_write_seq(dev, 0x91, 0x00);
    aic_spi_lcd_write_seq(dev, 0x92, 0x0B);
    aic_spi_lcd_write_seq(dev, 0x93, 0x02);
    aic_spi_lcd_write_seq(dev, 0x94, 0xDB);
    aic_spi_lcd_write_seq(dev, 0x95, 0x04);
    aic_spi_lcd_write_seq(dev, 0x96, 0x00);
    aic_spi_lcd_write_seq(dev, 0x97, 0x00);
    aic_spi_lcd_write_seq(dev, 0x98, 0x48);
    aic_spi_lcd_write_seq(dev, 0x99, 0x00);
    aic_spi_lcd_write_seq(dev, 0x9A, 0x0D);
    aic_spi_lcd_write_seq(dev, 0x9B, 0x02);
    aic_spi_lcd_write_seq(dev, 0x9C, 0xDD);
    aic_spi_lcd_write_seq(dev, 0x9D, 0x04);
    aic_spi_lcd_write_seq(dev, 0x9E, 0x00);
    aic_spi_lcd_write_seq(dev, 0x9F, 0x00);
    aic_spi_lcd_write_seq(dev, 0xA0, 0x48);
    aic_spi_lcd_write_seq(dev, 0xA1, 0x00);
    aic_spi_lcd_write_seq(dev, 0xA2, 0x06);
    aic_spi_lcd_write_seq(dev, 0xA3, 0x02);
    aic_spi_lcd_write_seq(dev, 0xA4, 0xD6);
    aic_spi_lcd_write_seq(dev, 0xA5, 0x04);
    aic_spi_lcd_write_seq(dev, 0xA6, 0x00);
    aic_spi_lcd_write_seq(dev, 0xA7, 0x00);
    aic_spi_lcd_write_seq(dev, 0xA8, 0x48);
    aic_spi_lcd_write_seq(dev, 0xA9, 0x00);
    aic_spi_lcd_write_seq(dev, 0xAA, 0x08);
    aic_spi_lcd_write_seq(dev, 0xAB, 0x02);
    aic_spi_lcd_write_seq(dev, 0xAC, 0xD8);
    aic_spi_lcd_write_seq(dev, 0xAD, 0x04);
    aic_spi_lcd_write_seq(dev, 0xAE, 0x00);
    aic_spi_lcd_write_seq(dev, 0xAF, 0x00);
    aic_spi_lcd_write_seq(dev, 0xB0, 0x48);
    aic_spi_lcd_write_seq(dev, 0xB1, 0x00);
    aic_spi_lcd_write_seq(dev, 0xB2, 0x0A);
    aic_spi_lcd_write_seq(dev, 0xB3, 0x02);
    aic_spi_lcd_write_seq(dev, 0xB4, 0xDA);
    aic_spi_lcd_write_seq(dev, 0xB5, 0x04);
    aic_spi_lcd_write_seq(dev, 0xB6, 0x00);
    aic_spi_lcd_write_seq(dev, 0xB7, 0x00);
    aic_spi_lcd_write_seq(dev, 0xB8, 0x48);
    aic_spi_lcd_write_seq(dev, 0xB9, 0x00);
    aic_spi_lcd_write_seq(dev, 0xBA, 0x0C);
    aic_spi_lcd_write_seq(dev, 0xBB, 0x02);
    aic_spi_lcd_write_seq(dev, 0xBC, 0xDC);
    aic_spi_lcd_write_seq(dev, 0xBD, 0x04);
    aic_spi_lcd_write_seq(dev, 0xBE, 0x00);
    aic_spi_lcd_write_seq(dev, 0xBF, 0x00);
    aic_spi_lcd_write_seq(dev, 0xC0, 0x10);
    aic_spi_lcd_write_seq(dev, 0xC1, 0x47);
    aic_spi_lcd_write_seq(dev, 0xC2, 0x56);
    aic_spi_lcd_write_seq(dev, 0xC3, 0x65);
    aic_spi_lcd_write_seq(dev, 0xC4, 0x74);
    aic_spi_lcd_write_seq(dev, 0xC5, 0x88);
    aic_spi_lcd_write_seq(dev, 0xC6, 0x99);
    aic_spi_lcd_write_seq(dev, 0xC7, 0x01);
    aic_spi_lcd_write_seq(dev, 0xC8, 0xBB);
    aic_spi_lcd_write_seq(dev, 0xC9, 0xAA);
    aic_spi_lcd_write_seq(dev, 0xD0, 0x10);
    aic_spi_lcd_write_seq(dev, 0xD1, 0x47);
    aic_spi_lcd_write_seq(dev, 0xD2, 0x56);
    aic_spi_lcd_write_seq(dev, 0xD3, 0x65);
    aic_spi_lcd_write_seq(dev, 0xD4, 0x74);
    aic_spi_lcd_write_seq(dev, 0xD5, 0x88);
    aic_spi_lcd_write_seq(dev, 0xD6, 0x99);
    aic_spi_lcd_write_seq(dev, 0xD7, 0x01);
    aic_spi_lcd_write_seq(dev, 0xD8, 0xBB);
    aic_spi_lcd_write_seq(dev, 0xD9, 0xAA);
    aic_spi_lcd_write_seq(dev, 0xF3, 0x01);
    aic_spi_lcd_write_seq(dev, 0xF0, 0x00);
    aic_spi_lcd_write_seq(dev, 0x35, 0x00);
    aic_spi_lcd_write_seq(dev, 0x3A, 0x55);
    aic_spi_lcd_write_seq(dev, 0x21);

    aic_spi_lcd_write_seq(dev, 0x2A, 0x00, 0x00, 0x01, 0x67);
    aic_spi_lcd_write_seq(dev, 0x2B, 0x00, 0x00, 0x01, 0x67);

    aic_spi_lcd_write_seq(dev, 0x11);
    rt_thread_mdelay(120);
    aic_spi_lcd_write_seq(dev, 0x29);
}

static const struct qspi_disp_priv st77916 = {
    .instruction = {
        .cmd_content = 0x02002C00,
        .flush_content = 0x32002C00,
        .qspi_lines = 1,
    },
    .data_lines = 4,
};

static const struct qspi_cfg qspi = {
    .disp_mode = false,
    .priv = &st77916,
};

#ifdef RT_USING_FINSH
static int cmd_spi_st77916(int argc, char **argv)
{
    aic_spi_lcd_cfg_t config = {
        .bus_name = "qspi1",
        .dev_name = "qspi1dev",
        .max_hz = 25000000,
        .mode = RT_SPI_MODE_0 | RT_SPI_MSB,
        .qspi_cfg = &qspi,
    };

    aic_spi_lcd_dev_t *spi_dev = aic_spi_lcd_init(&config);
    if (!spi_dev) {
        printf("Failed to initialize SPI dev\n");
        return -1;
    }

    panel_st77916_enable(spi_dev);

    unsigned int fb_size = 360 * 360 * 2;

    u8 *buffer = aicos_malloc_align(MEM_CMA, fb_size, CACHE_LINE_SIZE);
    if (!buffer) {
        printf("Failed to allocate buffer\n");
        return -1;
    }
    memset(buffer, 0xFF, fb_size);
    aicos_dcache_clean_invalid_range((ulong *)buffer, (ulong)ALIGN_UP(fb_size, CACHE_LINE_SIZE));

    aic_spi_lcd_flush(spi_dev, buffer, fb_size);
    aic_spi_lcd_wait_completion(spi_dev);
    printf("Flushed full screen with white (0xFF)\n");

    /* enable backlight */
    aic_spi_lcd_backlight_gpio_enable("PE.12", false, true);

    aicos_mdelay(2000);

    memset(buffer, 0x00, fb_size);
    aicos_dcache_clean_invalid_range((ulong *)buffer, (ulong)ALIGN_UP(fb_size, CACHE_LINE_SIZE));

    /*
     * Fill the framebuffer with white (0x00) and flush a specific window region.
     * The window is defined by coordinates (100, 100) to (200, 200).
     * This verifies partial-screen update functionality.
     */
    aic_spi_lcd_flush_win(spi_dev, 100, 200, 100, 200, buffer, 100 * 100 * 2);
    aic_spi_lcd_wait_completion(spi_dev);
    printf("Flushed window region (100,100)-(200,200) with black (0x00)\n");

    aicos_mdelay(2000);

    /*
     * Configure the window to cover the entire screen and flush with white (0x00).
     * This demonstrates full-screen update using window configuration.
     *
     * Alternatively, aic_spi_lcd_flush_win() can be used directly for the same effect:
     * aic_spi_lcd_flush_win(spi_dev, 0, 0, 360, 360, buffer, fb_size);
     */
    aic_spi_lcd_config_win(spi_dev, 0, 360, 0, 360);
    aic_spi_lcd_flush(spi_dev, buffer, fb_size);
    aic_spi_lcd_wait_completion(spi_dev);
    printf("Flushed full screen with black (0x00)\n");

    aicos_free_align(MEM_CMA, buffer);

    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_spi_st77916, spi_st77916, Test panel_st77916);
#endif // defined(RT_USING_FINSH)
