/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <rtconfig.h>
#include <rtdevice.h>
#include <drv_qspi.h>
#include <aic_log.h>
#include "spi_flash_sfud.h"
#include <string.h>

#if defined(RT_USING_SFUD)

#define SPI_FLASH_DEVICE_NAME "qspi01"

#define USING_NOR_FLASH_DEV_NAME "norflash0"

#ifndef RT_SFUD_DEFAULT_SPI_CFG

#ifndef RT_SFUD_SPI_MAX_HZ
#define RT_SFUD_SPI_MAX_HZ 50000000
#endif

/* read the JEDEC SFDP command must run at 50 MHz or less */
#define RT_SFUD_DEFAULT_SPI_CFG                  \
{                                                \
    .mode = RT_SPI_MODE_0 | RT_SPI_MSB,          \
    .data_width = 8,                             \
    .max_hz = RT_SFUD_SPI_MAX_HZ,                \
}
#endif /* RT_SFUD_DEFAULT_SPI_CFG */

#ifdef SFUD_USING_QSPI
#define RT_SFUD_DEFAULT_QSPI_CFG                 \
{                                                \
    RT_SFUD_DEFAULT_SPI_CFG,                     \
    .medium_size = 0x800000,                     \
    .ddr_mode = 0,                               \
    .qspi_dl_width = 4,                          \
}
#endif /* SFUD_USING_QSPI */

int rt_hw_spi_flash_with_sfud_init(void)
{
    struct rt_spi_configuration cfg = RT_SFUD_DEFAULT_SPI_CFG;
    rt_err_t ret;
    rt_spi_flash_device_t flash_dev;

    ret = aic_qspi_bus_attach_device("qspi0", SPI_FLASH_DEVICE_NAME, 0,
                                     AIC_QSPI0_BUS_WIDTH,
                                     RT_NULL, RT_NULL);
    if (ret < 0) {
        pr_err("attach qspi device failed.\n");
        return RT_ERROR;
    }
#if defined(AIC_USING_QSPI1) && defined(AIC_QSPI1_DEVICE_SPINOR)
    ret = aic_qspi_bus_attach_device("qspi1", "qspi11", 0,
                                     AIC_QSPI1_BUS_WIDTH,
                                     RT_NULL, RT_NULL);
    if (ret < 0) {
        pr_err("attach qspi device failed.\n");
        return RT_ERROR;
    }
#endif
#ifndef SFUD_USING_QSPI
    cfg.max_hz = AIC_QSPI0_DEVICE_SPINOR_FREQ;
    flash_dev = rt_sfud_flash_probe_ex(USING_NOR_FLASH_DEV_NAME,
                                       SPI_FLASH_DEVICE_NAME, &cfg, RT_NULL);
#if defined(AIC_USING_QSPI1) && defined(AIC_QSPI1_DEVICE_SPINOR)
    cfg.max_hz = AIC_QSPI1_DEVICE_SPINOR_FREQ;
    flash_dev = rt_sfud_flash_probe_ex("norflash1",
                                       "qspi11", &cfg, RT_NULL);
#endif
#else
    struct rt_qspi_configuration qspi_cfg = RT_SFUD_DEFAULT_QSPI_CFG;

    qspi_cfg.parent.max_hz = AIC_QSPI0_DEVICE_SPINOR_FREQ;
    flash_dev = rt_sfud_flash_probe_ex(USING_NOR_FLASH_DEV_NAME,
                                       SPI_FLASH_DEVICE_NAME, &cfg, &qspi_cfg);
#if defined(AIC_USING_QSPI1) && defined(AIC_QSPI1_DEVICE_SPINOR)
    qspi_cfg.parent.max_hz = AIC_QSPI1_DEVICE_SPINOR_FREQ;
    flash_dev = rt_sfud_flash_probe_ex("norflash1",
                                       "qspi11", &cfg, &qspi_cfg);
#endif
#endif
    if (flash_dev == RT_NULL) {
        pr_err("sfud flash probe  failed.\n");
        return RT_ERROR;
    };

    return RT_EOK;
}
void sfud_log_debug(const char *file, const long line, const char *fmt, ...)
{
    va_list args;
    char log_buf[RT_CONSOLEBUF_SIZE];
    int head_len;

    snprintf(log_buf, RT_CONSOLEBUF_SIZE, "[D] %s()%ld ", file, line);
    head_len = strlen(log_buf);
    va_start(args, fmt);
    vsnprintf(log_buf + head_len, sizeof(log_buf) - head_len - 1, fmt, args);
    va_end(args);
    puts(log_buf);
}

void sfud_log_info(const char *fmt, ...)
{
    va_list args;
    char log_buf[RT_CONSOLEBUF_SIZE];
    int head_len;

    snprintf(log_buf, RT_CONSOLEBUF_SIZE, "[I] ");
    head_len = strlen(log_buf);
    va_start(args, fmt);
    vsnprintf(log_buf + head_len, sizeof(log_buf) - head_len - 1, fmt, args);
    va_end(args);
    puts(log_buf);
}

INIT_PREV_EXPORT(rt_hw_spi_flash_with_sfud_init);
#endif
