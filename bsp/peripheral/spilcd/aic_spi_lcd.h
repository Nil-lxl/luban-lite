/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Huahui.mai <huahui.mai@artinchip.com>
 */

#ifndef AIC_SPI_LCD_H
#define AIC_SPI_LCD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <drv_qspi.h>

struct qspi_disp_priv {
    struct spi_disp_config *spi_disp;

    struct {
        unsigned int cmd_content;
        unsigned int flush_content;
        unsigned int qspi_lines;
    } instruction;

    unsigned int data_lines;
};

struct qspi_cfg {
    bool disp_mode;
    const struct qspi_disp_priv *priv;
};

typedef struct {
    const char *bus_name;
    const char *dev_name;
    unsigned int mode;
    unsigned int max_hz;

    const char *rs_pin; /* Register select pin, also known as D/C (Data/Command) pin */

    unsigned int width;
    unsigned int height;
    unsigned int bus_width; /* SPI： 1， QSPI: 4 */

    const struct qspi_cfg *qspi_cfg;
} aic_spi_lcd_cfg_t;

struct aic_spi_lcd_dev;

typedef struct aic_spi_lcd_dev aic_spi_lcd_dev_t;

struct spi_ops {
    int (*init)(aic_spi_lcd_dev_t *spi_dev);
    int (*command)(aic_spi_lcd_dev_t *spi_dev, unsigned int cmd, unsigned int len, const u8 *data);
    int (*flush)(aic_spi_lcd_dev_t *spi_dev, u8 *data, unsigned int len);
    void (*wait_completion)(aic_spi_lcd_dev_t *spi_dev);
};

struct aic_spi_lcd_dev {
    const aic_spi_lcd_cfg_t *config;
    const struct spi_ops *ops;
    const struct qspi_cfg *qspi_cfg;

    void *dev;

    int rs_pin;
};

/**
 * aic_spi_lcd_init - Initialize the SPI LCD device
 * @config: Pointer to the configuration structure containing initialization parameters.
 *
 * This function allocates and initializes an SPI LCD device based on the provided
 * configuration. It sets up GPIO pins, selects the appropriate SPI/QSPI operations,
 * and calls the corresponding initialization routine.
 *
 * Return: Pointer to the initialized device structure on success, NULL on failure.
 */
aic_spi_lcd_dev_t *aic_spi_lcd_init(const aic_spi_lcd_cfg_t *config);

/**
 * aic_spi_lcd_write_cmd - Send initialization commands to the LCD screen
 * @spi_dev: Pointer to the SPI LCD device structure.
 * @cmd: Command to be sent to the LCD controller.
 * @len: Length of the data payload associated with the command.
 * @data: Pointer to the data payload (can be NULL if no data is required).
 *
 * This function sends a command along with optional data to the LCD controller
 * for initialization purposes. The command transmission is performed in synchronous mode.
 *
 * Return: Number of bytes successfully sent on success, a negative value on failure.
 */
int aic_spi_lcd_write_cmd(aic_spi_lcd_dev_t *spi_dev, unsigned int cmd, unsigned int len, const u8 *data);

/**
 * aic_spi_lcd_flush - Flush framebuffer data to the LCD
 * @spi_dev: Pointer to the SPI LCD device structure.
 * @data: Pointer to the framebuffer data to be sent to the LCD.
 * @len: Size of the data in bytes.
 *
 * This function transfers framebuffer data to the LCD for display. The framebuffer
 * data is transmitted asynchronously and does not block. To wait for the transfer
 * to complete, call the aic_spi_lcd_wait_completion() function.
 *
 * Before sending the data, a 0x2C command is automatically inserted.
 *
 * Return: Number of bytes successfully sent on success, a negative value on failure.
 */
int aic_spi_lcd_flush(aic_spi_lcd_dev_t *spi_dev, u8 *data, unsigned int len);

/**
 * aic_spi_lcd_config_win - Configure the LCD window
 * @spi_dev: Pointer to the SPI LCD device structure.
 * @x_start: Start column index of the window.
 * @x_end: End column index of the window.
 * @y_start: Start row index of the window.
 * @y_end: End row index of the window.
 *
 * This function configures the Column Address Register (0x2A) and Row Address
 * Register (0x2B) of the LCD controller to define the active display window.
 * It sets the horizontal and vertical boundaries for partial screen updates,
 * enabling efficient rendering of specific regions.
 *
 * Return: 0 on success, a negative value on failure.
 */
int aic_spi_lcd_config_win(aic_spi_lcd_dev_t *spi_dev,
                           unsigned int x_start, unsigned int x_end,
                           unsigned int y_start, unsigned int y_end);

/**
 * aic_spi_lcd_flush_win - Flush a specific window of screen data to the LCD
 * @spi_dev: Pointer to the SPI LCD device structure.
 * @x_start: Start column index of the window.
 * @x_end: End column index of the window.
 * @y_start: Start row index of the window.
 * @y_end: End row index of the window.
 * @data: Pointer to the framebuffer data to be sent to the LCD.
 * @len: Size of the data in bytes.
 *
 * This function transfers a specific window of framebuffer data to the LCD for display.
 * It allows partial screen updates by specifying the region to be refreshed.
 * The data is transmitted asynchronously and does not block. To wait for the transfer
 * to complete, call the aic_spi_lcd_wait_completion() function.
 *
 * Return: Number of bytes successfully sent on success, a negative value on failure.
 */
int aic_spi_lcd_flush_win(aic_spi_lcd_dev_t *spi_dev,
                          unsigned int x_start, unsigned int x_end,
                          unsigned int y_start, unsigned int y_end,
                          u8 *data, unsigned int len);

/**
 * aic_spi_lcd_wait_completion - Wait for SPI data transmission to complete
 * @spi_dev: Pointer to the SPI LCD device structure.
 *
 * This function waits for the SPI controller to finish data transmission. It should
 * be called after flush interface. This waiting operation is blocking.
 */
void aic_spi_lcd_wait_completion(aic_spi_lcd_dev_t *spi_dev);

/**
 * aic_spi_lcd_draw_rgb565_swap - Perform byte swapping on RGB565 pixel data
 * @buf: Pointer to the framebuffer containing RGB565 pixel data.
 * @buf_size_px: Number of pixels in the buffer (width * height).
 *
 * This function swaps the byte order of RGB565 pixel data in the buffer to
 * match the endianness expected by the LCD controller. It processes data in
 * 32-bit chunks for efficiency and handles any remaining pixels individually.
 */
void aic_spi_lcd_draw_rgb565_swap(void *buf, u32 buf_size_px);

/**
 * aic_spi_lcd_backlight_gpio_enable - Enable or disable LCD backlight via GPIO
 * @gpio_name: Name of the GPIO pin used for backlight control.(e.g., "PE.12")
 * @low_active: Indicates whether the backlight is active-low (true) or active-high (false).
 * @enable: Boolean flag to enable (true) or disable (false) the backlight.
 *
 * This function controls the LCD backlight by configuring and setting the specified
 * GPIO pin. The polarity of the output is determined by the @low_active parameter.
 */
void aic_spi_lcd_backlight_gpio_enable(const char *gpio_name, bool low_active, bool enable);

/**
 * aic_spi_lcd_backlight_pwm_enable - Enable or disable LCD backlight via PWM
 * @channel: PWM channel number used for backlight control.
 * @level: Brightness level (0-100) to set for the PWM output.
 * @enable: Boolean flag to enable (true) or disable (false) the backlight.
 *
 * This function controls the LCD backlight using PWM. When enabled, it sets the
 * specified brightness level on the given PWM channel. When disabled, it turns
 * off the PWM output for the specified channel.
 */
void aic_spi_lcd_backlight_pwm_enable(int channel, int level, bool enable);

#define aic_spi_lcd_write_seq(spi_dev, cmd, seq...)            \
    do {                                                       \
        static const u8 d[] = { seq };                         \
        aic_spi_lcd_write_cmd(spi_dev, cmd, ARRAY_SIZE(d), d); \
    } while(0);

#endif // AIC_SPI_LCD_H
