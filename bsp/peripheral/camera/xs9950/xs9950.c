/*
 * Copyright (c) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#define LOG_TAG     "xs9950"
#include <string.h>
#include <getopt.h>
#include <drivers/i2c.h>
#include <drivers/pin.h>
#include "aic_core.h"
#include "aic_hal_clk.h"
#include "mpp_types.h"
#include "mpp_img_size.h"
#include "mpp_vin.h"
#include "drv_camera.h"
#include "camera_inner.h"

#define DRV_NAME            LOG_TAG
#define DEFAULT_FORMAT      HD720P25
#define DEFAULT_MEDIA_CODE  MEDIA_BUS_FMT_UYVY8_2X8
#define DEFAULT_BUS_TYPE    MEDIA_BUS_BT656         // Default BT656 output
#define DEFAULT_WIDTH       1280
#define DEFAULT_HEIGHT      720
#define DEFAULT_IS_HDCCTV   1                       // 0: CVBS; 1: HDCCTV
#define DEFAULT_VIN_CH      VIN1
#define XS9950_I2C_SLAVE_ID 0x30                    // I2C slave address (from manual 4.1.4)
#define XS9950_CHIP_ID      0x9950                  // Chip ID (custom, needs adjustment based on actual)
#define XS9950_REG_ADDR_LEN 2                       // I2C register address length (16bit)
// #define XS9950_INTERRUPT

// Video channel enum (supports 4-channel input, manual 3.1.3.7)
enum tp_vin_ch {
    VIN1 = 0,  // AFE_VINA
    VIN2 = 1,  // AFE_VINB
    VIN3 = 2,  // AFE_VINC
    VIN4 = 3,  // AFE_VIND
};

// Video standard enum (manual 2.3)
enum tp_std {
    STD_CVBS,    // SD CVBS
    STD_HDCCTV,  // HD HDCCTV
};

// Video format enum (manual Table 2-1, Table A-1)
enum tp_fmt {
    // Standard Definition
    CVBS_PAL,    // PAL-D1
    CVBS_NTSC,   // NTSC-D1
    CVBS_960H_P, // 960H-PAL
    CVBS_960H_N, // 960H-NTSC
    // High Definition
    HD720P25,    // 720P25
    HD720P30,    // 720P30
    HD720P50,    // 720P50
    HD720P60,    // 720P60
    HD960P25,    // 960P25
    HD960P30,    // 960P30
    FHD1080P15,  // 1080P15
    FHD1080P25,  // 1080P25
    FHD1080P30,  // 1080P30
};

// BT656 configuration struct (manual 4.4)
struct xs9950_bt656_cfg {
    u8 mode;      // 0: standard 8bit; 1: netra mode; 2: shengmai mode
    u8 edge;      // 0: single edge; 1: dual edge
    u8 enable;    // BT656 enable flag
};

// Device struct
struct xs9950_dev {
    struct rt_device dev;
    struct rt_i2c_bus_device *i2c;
    u32 pwdn_pin;
    u32 irq_pin;  // New interrupt pin
    struct mpp_video_fmt fmt;
    enum tp_vin_ch curr_ch;
    enum tp_fmt curr_fmt;
    enum tp_std curr_std;
    struct xs9950_bt656_cfg bt656_cfg;
    bool on;
    bool streaming;
    u8 irq_status;  // Interrupt status (bit0: video loss; bit1: MIPI error; bit2: control info received)
};

static struct xs9950_dev g_xs_dev = {0};

/**
 * @brief I2C write register (16bit address)
 * @param reg Register address
 * @param val Write value
 * @return 0 success, -1 failure
 */
static int xs9950_write_reg(u16 reg, u8 val)
{
    struct rt_i2c_msg msgs[2];
    u8 reg_buf[2];

    reg_buf[0] = (reg >> 8) & 0xFF; // Register address high byte
    reg_buf[1] = reg & 0xFF;        // Register address low byte

    msgs[0].addr  = XS9950_I2C_SLAVE_ID;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = reg_buf;
    msgs[0].len   = 2;

    msgs[1].addr  = XS9950_I2C_SLAVE_ID;
    msgs[1].flags = RT_I2C_WR | RT_I2C_NO_START;
    msgs[1].buf   = &val;
    msgs[1].len   = 1;

    if (rt_i2c_transfer(g_xs_dev.i2c, msgs, 2) != 2) {
        LOG_E("%s: reg=0x%x, val=0x%x write failed", __func__, reg, val);
        return -1;
    }

    return 0;
}

/**
 * @brief I2C read register (16bit address)
 * @param reg Register address
 * @return Read value, return 0xFF on failure
 */
static unsigned char xs9950_read_reg(u16 reg)
{
    struct rt_i2c_msg msgs[2];
    u8 reg_buf[2];
    u8 val = 0xFF;

    reg_buf[0] = (reg >> 8) & 0xFF; // Register address high byte
    reg_buf[1] = reg & 0xFF;        // Register address low byte

    msgs[0].addr  = XS9950_I2C_SLAVE_ID;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = reg_buf;
    msgs[0].len   = 2;

    msgs[1].addr  = XS9950_I2C_SLAVE_ID;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf   = &val;
    msgs[1].len   = 1;

    if (rt_i2c_transfer(g_xs_dev.i2c, msgs, 2) != 2) {
        LOG_E("%s: reg=0x%x read failed", __func__, reg);
        return 0xFF;
    }

    return val;
}

/**
 * @brief Set video resolution (new: associate format with resolution)
 */
static void xs9950_set_resolution(enum tp_fmt fmt)
{
    u16 h_active, v_active;

    switch (fmt) {
        case CVBS_PAL:      h_active=720;  v_active=576; break;
        case CVBS_NTSC:     h_active=720;  v_active=480; break;
        case CVBS_960H_P:   h_active=960;  v_active=576; break;
        case CVBS_960H_N:   h_active=960;  v_active=480; break;
        case HD720P25:      h_active=1280; v_active=720; break;
        case HD720P30:
        case HD720P50:
        case HD720P60:      h_active=1280; v_active=720; break;
        case HD960P25:
        case HD960P30:      h_active=1280; v_active=960; break;
        case FHD1080P15:
        case FHD1080P25:
        case FHD1080P30:    h_active=1920; v_active=1080; break;
        default:            h_active=1280; v_active=720; // Default 720P
    }

    // Configure horizontal/vertical active pixel registers (manual 4.2.2)
    xs9950_write_reg(0x4310, (h_active >> 8) & 0xFF);  // H_ACTIVE[15:8]
    xs9950_write_reg(0x4311, h_active & 0xFF);         // H_ACTIVE[7:0]
    xs9950_write_reg(0x4312, (v_active >> 8) & 0xFF);  // V_ACTIVE[15:8]
    xs9950_write_reg(0x4313, v_active & 0xFF);         // V_ACTIVE[7:0]

    // Update device format information
    g_xs_dev.fmt.width = h_active;
    g_xs_dev.fmt.height = v_active;
    LOG_I("Set resolution: %dx%d", h_active, v_active);
}

/**
 * @brief BT656 interface initialization (manual 4.4.2)
 */
static void xs9950_bt656_init(void)
{
    xs9950_write_reg(0x4300, 0x05);
    xs9950_write_reg(0x4300, 0x15);
    xs9950_write_reg(0x4080, 0x07);
    xs9950_write_reg(0x4119, 0x01);
    xs9950_write_reg(0x0803, 0x00);
    xs9950_write_reg(0x4020, 0x00);
    xs9950_write_reg(0x080e, 0x00);
    xs9950_write_reg(0x080e, 0x20);
    xs9950_write_reg(0x080e, 0x28);
    xs9950_write_reg(0x4020, 0x03);
    xs9950_write_reg(0x0803, 0x0f);
    xs9950_write_reg(0x0100, 0x35);
    xs9950_write_reg(0x0104, 0x48);
    xs9950_write_reg(0x0300, 0x3f);
    xs9950_write_reg(0x0105, 0xe1);
    xs9950_write_reg(0x0101, 0x42);
    xs9950_write_reg(0x0102, 0x40);
    xs9950_write_reg(0x0116, 0x3c);
    xs9950_write_reg(0x0117, 0x23);
    xs9950_write_reg(0x0333, 0x09);
    xs9950_write_reg(0x0337, 0xd9);
    xs9950_write_reg(0x0338, 0x0a);
    xs9950_write_reg(0x01bf, 0x4e);
    xs9950_write_reg(0x010e, 0x78);
    xs9950_write_reg(0x010f, 0x92);
    xs9950_write_reg(0x0110, 0x70);
    xs9950_write_reg(0x0111, 0x40);
    xs9950_write_reg(0x01e1, 0xff);
    xs9950_write_reg(0x0314, 0x66);
    xs9950_write_reg(0x0130, 0x10);
    xs9950_write_reg(0x0315, 0x23);
    xs9950_write_reg(0x0b64, 0x02);
    xs9950_write_reg(0x01e2, 0x03);
    xs9950_write_reg(0x0b55, 0x80);
    xs9950_write_reg(0x0b56, 0x00);
    xs9950_write_reg(0x0b59, 0x04);
    xs9950_write_reg(0x0b5a, 0x01);
    xs9950_write_reg(0x0b5c, 0x07);
    xs9950_write_reg(0x0b5e, 0x05);
    xs9950_write_reg(0x0b4b, 0x10);
    xs9950_write_reg(0x0b4e, 0x05);
    xs9950_write_reg(0x0b51, 0x21);
    xs9950_write_reg(0x0b30, 0xbc);
    xs9950_write_reg(0x0b31, 0x19);
    xs9950_write_reg(0x0b15, 0x03);
    xs9950_write_reg(0x0b16, 0x03);
    xs9950_write_reg(0x0b17, 0x03);
    xs9950_write_reg(0x0b07, 0x03);
    xs9950_write_reg(0x0b08, 0x05);
    xs9950_write_reg(0x0b1a, 0x10);
    xs9950_write_reg(0x0158, 0x03);
    xs9950_write_reg(0x0a88, 0x20);
    xs9950_write_reg(0x0a61, 0x09);
    xs9950_write_reg(0x0a62, 0x00);
    xs9950_write_reg(0x0a63, 0x0e);
    xs9950_write_reg(0x0a64, 0x00);
    xs9950_write_reg(0x0a65, 0xfc);
    xs9950_write_reg(0x0a67, 0xe5);
    xs9950_write_reg(0x0a69, 0xef);
    xs9950_write_reg(0x0a6b, 0x1b);
    xs9950_write_reg(0x0a6d, 0x2f);
    xs9950_write_reg(0x0a6f, 0x00);
    xs9950_write_reg(0x0a71, 0xc2);
    xs9950_write_reg(0x0a72, 0xff);
    xs9950_write_reg(0x0a73, 0xd0);
    xs9950_write_reg(0x0a74, 0xff);
    xs9950_write_reg(0x0a75, 0x29);
    xs9950_write_reg(0x0a77, 0x57);
    xs9950_write_reg(0x0a78, 0x00);
    xs9950_write_reg(0x0a79, 0x10);
    xs9950_write_reg(0x0a7a, 0x00);
    xs9950_write_reg(0x0a7b, 0xaa);
    xs9950_write_reg(0x0a7d, 0xb2);
    xs9950_write_reg(0x0a7f, 0x24);
    xs9950_write_reg(0x0a80, 0x00);
    xs9950_write_reg(0x0a81, 0x69);
    xs9950_write_reg(0x0a82, 0x00);
    xs9950_write_reg(0x0802, 0x02);
    xs9950_write_reg(0x0501, 0x81);
    xs9950_write_reg(0x0b74, 0xfc);
    xs9950_write_reg(0x01dc, 0x01);
    xs9950_write_reg(0x0804, 0x04);
    xs9950_write_reg(0x4018, 0x01);
    xs9950_write_reg(0x0b56, 0x01);
    xs9950_write_reg(0x0b73, 0x02);
    xs9950_write_reg(0x4210, 0x0c);
    xs9950_write_reg(0x420b, 0x2f);
    xs9950_write_reg(0x0504, 0x89);// bit[4:7] free_run color, value range: [0, 8]
    xs9950_write_reg(0x0507, 0x1b);// bit[5] 0: rising edge capture mode; 1: up/down edge capture mode;
    xs9950_write_reg(0x0503, 0x00);
    xs9950_write_reg(0x0502, 0x00);// bit4 bt data0-7 inverse, changed to data7-0
    xs9950_write_reg(0x015a, 0x00);
    xs9950_write_reg(0x015b, 0x24);
    xs9950_write_reg(0x015c, 0x80);
    xs9950_write_reg(0x015d, 0x16);
    xs9950_write_reg(0x015e, 0xd0);
    xs9950_write_reg(0x015f, 0x02);
    xs9950_write_reg(0x0160, 0xee);
    xs9950_write_reg(0x0161, 0x02);
    xs9950_write_reg(0x0165, 0x00);
    xs9950_write_reg(0x0166, 0x0f);
    xs9950_write_reg(0x4030, 0x15);
    xs9950_write_reg(0x4134, 0x0a);// Increase BT output driving capability from 0x6 to 0xa
    xs9950_write_reg(0x0803, 0x0f);
    xs9950_write_reg(0x4412, 0x01);
    xs9950_write_reg(0x0803, 0x1f);
    xs9950_write_reg(0x10e3, 0x04);
    xs9950_write_reg(0x10eb, 0xfd);
    xs9950_write_reg(0x0800, 0x07);
    xs9950_write_reg(0x0805, 0x07);
    xs9950_write_reg(0x4200, 0x02);

    // 720p@25fps
    xs9950_write_reg(0x010c, 0x00);
    xs9950_write_reg(0x0800, 0x07);
    xs9950_write_reg(0x0805, 0x07);
    xs9950_write_reg(0x0800, 0x07);
    xs9950_write_reg(0x0800, 0x05);
    xs9950_write_reg(0x0805, 0x05);
    xs9950_write_reg(0x0b50, 0x08);
    xs9950_write_reg(0x0e08, 0x00);
    xs9950_write_reg(0x010d, 0x00);
    xs9950_write_reg(0x010c, 0x01);
    xs9950_write_reg(0x0121, 0x5a);
    xs9950_write_reg(0x0130, 0x10);
    xs9950_write_reg(0x01a9, 0x00);
    xs9950_write_reg(0x01aa, 0x04);
    xs9950_write_reg(0x0156, 0x00);
    xs9950_write_reg(0x0157, 0x08);
    xs9950_write_reg(0x0105, 0xe1);
    xs9950_write_reg(0x0101, 0x42);
    xs9950_write_reg(0x0102, 0x40);
    xs9950_write_reg(0x0116, 0x3c);
    xs9950_write_reg(0x0117, 0x23);
    xs9950_write_reg(0x012d, 0x3f);
    xs9950_write_reg(0x012f, 0x8c);
    xs9950_write_reg(0x01e2, 0x03);
    xs9950_write_reg(0x420b, 0x21);
    xs9950_write_reg(0x0106, 0x80);
    xs9950_write_reg(0x0107, 0x00);
    xs9950_write_reg(0x0108, 0x80);
    xs9950_write_reg(0x0109, 0x00);
    xs9950_write_reg(0x010a, 0x88);
    xs9950_write_reg(0x010a, 0x08);
    xs9950_write_reg(0x010b, 0x00);
    xs9950_write_reg(0x010b, 0x00);
    xs9950_write_reg(0x011d, 0x17);
    xs9950_write_reg(0x0e08, 0x01);
    xs9950_write_reg(0x4201, 0x00);
    xs9950_write_reg(0x4203, 0x00);
    xs9950_write_reg(0x4202, 0x00);
    xs9950_write_reg(0x4204, 0x00);
    xs9950_write_reg(0x0147, 0x00);
    xs9950_write_reg(0x0102, 0x40);
    xs9950_write_reg(0x0105, 0xe1);
    xs9950_write_reg(0x0108, 0x80);
    xs9950_write_reg(0x0a60, 0x04);
    xs9950_write_reg(0x0a5c, 0xae);
    xs9950_write_reg(0x0a5d, 0xd7);
    xs9950_write_reg(0x0a5e, 0xc7);
    xs9950_write_reg(0x0a5f, 0x31);
    xs9950_write_reg(0x01a9, 0x00);
    xs9950_write_reg(0x01aa, 0x04);
    xs9950_write_reg(0x0503, 0x00);
    xs9950_write_reg(0x015a, 0x00);
    xs9950_write_reg(0x015b, 0x24);
    xs9950_write_reg(0x015c, 0x80);
    xs9950_write_reg(0x015d, 0x16);
    xs9950_write_reg(0x015e, 0xd0);
    xs9950_write_reg(0x015f, 0x02);
    xs9950_write_reg(0x0160, 0xee);
    xs9950_write_reg(0x0161, 0x02);
    xs9950_write_reg(0x0165, 0x00);
    xs9950_write_reg(0x0166, 0x0f);
    LOG_I("BT656 init done: mode=standard 8bit");
}

/**
 * @brief AFE front-end initialization (manual 4.7)
 * @param ch Video channel
 */
static void xs9950_afe_init(enum tp_vin_ch ch)
{
    // 1. AFE power enable (new: manual 4.7.1)
    xs9950_write_reg(0x470A, 0x01);    // AFE power domain enable
    rt_thread_mdelay(1);

    // 2. AFE clock configuration (new: manual 4.7.2)
    xs9950_write_reg(0x4704, 0x02);    // Clock source selection (24MHz)
    xs9950_write_reg(0x4705, 0x01);    // Clock division (24MHz/2=12MHz)

    // 3. Channel selection (manual 4.2.1, register 0x4200)
    u8 ch_val = 0;
    switch (ch) {
        case VIN1: ch_val = 0x01; break;
        case VIN2: ch_val = 0x00; break;
        case VIN3: ch_val = 0x02; break;
        case VIN4: ch_val = 0x03; break;
        default: ch_val = 0x01;
    }
    xs9950_write_reg(0x4200, (xs9950_read_reg(0x4200) & 0xFC) | ch_val);

    // 2. Configure EQ (signal equalization, compensate transmission attenuation)
    xs9950_write_reg(0x4700, 0x03);    // EQ strength configuration (medium)
    // 3. Configure Clamp (STC mode)
    xs9950_write_reg(0x4701, 0x00);    // STC Clamp enable
    // 4. Configure LPF (anti-aliasing filter)
    xs9950_write_reg(0x4702, 0x02);    // LPF bandwidth configuration
    // 5. Enable signal short-circuit detection
    xs9950_write_reg(0x4703, 0x01);    // Short-circuit detection enable

    LOG_I("AFE init done: channel=%d", ch);
}

/**
 * @brief Video decoder initialization (manual 4.2)
 * @param fmt Video format
 * @param std Video standard
 */
static void xs9950_decoder_init(enum tp_fmt fmt, enum tp_std std)
{
    // 1. Format identification configuration (auto recognition)
    xs9950_write_reg(0x4300, 0x01);    // Enable auto format recognition

    // 2. Frequency offset compensation enable (solve color bias, manual 4.2.3.3)
    xs9950_write_reg(0x4301, 0x01);    // Frequency offset compensation enable

    // 3. Edge enhancement configuration (15 levels adjustable, manual 4.2.4)
    xs9950_write_reg(0x4302, 0x08);    // Sharpness level 8 (medium)

    // 4. Image parameter configuration (brightness, contrast default values)
    xs9950_write_reg(0x4303, 0x80);    // Brightness default
    xs9950_write_reg(0x4304, 0x80);    // Contrast default
    xs9950_write_reg(0x4305, 0x80);    // Saturation default

    // 5. Set resolution (new: associate format with resolution)
    xs9950_set_resolution(fmt);

    LOG_I("Decoder init done: fmt=%d, std=%d", fmt, std);
}

/**
 * @brief XS9950 sensor initialization
 * @param ch Video channel (VIN1~VIN4)
 * @param fmt Video format
 * @param std Video standard
 */
static void xs9950_sensor_init(enum tp_vin_ch ch, enum tp_fmt fmt, enum tp_std std)
{
    g_xs_dev.curr_ch = ch;
    g_xs_dev.curr_fmt = fmt;
    g_xs_dev.curr_std = std;

    // 1. Global reset
    xs9950_write_reg(0x0000, 0x01);    // Software reset
    rt_thread_mdelay(2);
    xs9950_write_reg(0x0000, 0x00);    // Reset release
    rt_thread_mdelay(20);  // Extend reset stabilization time

    // 2. AFE front-end initialization
    xs9950_afe_init(g_xs_dev.curr_ch);

    // 3. Video decoder initialization
    xs9950_decoder_init(g_xs_dev.curr_fmt, g_xs_dev.curr_std);

    // 4. Output interface initialization
    xs9950_bt656_init();

    // 5. Enable video output
    xs9950_write_reg(0x4000, 0x01);    // Global video enable
    LOG_I("XS9950 sensor init done: ch=%d, fmt=%d, std=%d", ch, fmt, std);
}

/**
 * @brief Chip ID check
 * @return 0 success, -1 failure
 */
#define XS9950_DEVICE_ID_1 0x40f0
#define XS9950_DEVICE_ID_0 0x40f1
static int xs9950_chipid_check(struct xs9950_dev *sensor)
{
    u8 id_h = 0, id_l = 0;
    id_h = xs9950_read_reg(XS9950_DEVICE_ID_1);      // Chip ID high 8 bits
    id_l = xs9950_read_reg(XS9950_DEVICE_ID_0);      // Chip ID low 8 bits
    if ((id_h << 8 | id_l) != XS9950_CHIP_ID) {
        LOG_E("Invalid Chip ID: 0x%02x%02x (expect 0x%04x)", id_h, id_l, XS9950_CHIP_ID);
        return -1;
    }
    LOG_I("Chip ID check pass: 0x%04x", XS9950_CHIP_ID);
    return 0;
}

/**
 * @brief Power on
 */
static void xs9950_power_on(struct xs9950_dev *sensor)
{
    if (sensor->on)
        return;
    camera_pin_set_high(sensor->pwdn_pin);  // PWDn high level power on
    rt_thread_mdelay(20);  // Power-on stabilization delay
    LOG_I("XS9950 power on");
    sensor->on = true;
}

/**
 * @brief Power off
 */
static void xs9950_power_off(struct xs9950_dev *sensor)
{
    if (!sensor->on)
        return;
    camera_pin_set_low(sensor->pwdn_pin);   // PWDn low level power off
    LOG_I("XS9950 power off");
    sensor->on = false;
}

/**
 * @brief Device initialization
 */
static rt_err_t xs9950_init(rt_device_t dev)
{
    struct xs9950_dev *sensor = (struct xs9950_dev *)dev;
    struct mpp_video_fmt *fmt = &sensor->fmt;

    // 1. Get I2C bus
    sensor->i2c = camera_i2c_get();
    if (!sensor->i2c) {
        LOG_E("Get I2C bus failed");
        return -RT_EINVAL;
    }

    // 2. Get PWDn pin
    sensor->pwdn_pin = camera_pwdn_pin_get();
    if (!sensor->pwdn_pin) {
        LOG_E("Get PWDn pin failed");
        return -RT_EINVAL;
    }
#ifdef XS9950_INTERRUPT
    sensor->irq_pin = camera_irq_pin_get();  // New interrupt pin acquisition
    if (!sensor->irq_pin) {
        LOG_W("Get IRQ pin failed, interrupt disabled");
    }
#endif
    // 3. Default configuration initialization
    fmt->code = DEFAULT_MEDIA_CODE;
    fmt->width = DEFAULT_WIDTH;
    fmt->height = DEFAULT_HEIGHT;
    fmt->bus_type = DEFAULT_BUS_TYPE;
    fmt->flags = MEDIA_SIGNAL_FIELD_ACTIVE_HIGH |
                 MEDIA_SIGNAL_VSYNC_ACTIVE_LOW |
                 MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH |
                 MEDIA_SIGNAL_PCLK_SAMPLE_FALLING;

    // 4. BT656 default configuration (enable according to bus type)
    sensor->bt656_cfg.mode = 0;
    sensor->bt656_cfg.edge = 0;
    sensor->bt656_cfg.enable = (DEFAULT_BUS_TYPE == MEDIA_BUS_BT656) ? 1 : 0;

    LOG_I("XS9950 device init done");
    return RT_EOK;
}

/**
 * @brief Device open
 */
static rt_err_t xs9950_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct xs9950_dev *sensor = (struct xs9950_dev *)dev;

    if (sensor->on)
        return RT_EOK;

    // 1. Power on
    xs9950_power_on(sensor);

    // 2. Chip ID check
    if (xs9950_chipid_check(sensor) != 0) {
        xs9950_power_off(sensor);
        return -RT_ERROR;
    }

    // 3. Sensor initialization
    xs9950_sensor_init(DEFAULT_VIN_CH, DEFAULT_FORMAT, DEFAULT_IS_HDCCTV);
#ifdef XS9950_INTERRUPT
    // 4. Register interrupt (fix: enable interrupt)
    if (sensor->irq_pin) {
        rt_pin_attach_irq(sensor->irq_pin, PIN_IRQ_MODE_FALLING, xs9950_irq_handler, &g_xs_dev);
        rt_pin_irq_enable(sensor->irq_pin, PIN_IRQ_ENABLE);
        LOG_I("IRQ enabled on pin %d", sensor->irq_pin);
    }
#endif
    sensor->streaming = true;
    LOG_I("XS9950 device open done");
    return RT_EOK;
}

/**
 * @brief Device close
 */
static rt_err_t xs9950_close(rt_device_t dev)
{
    struct xs9950_dev *sensor = (struct xs9950_dev *)dev;

    if (!sensor->on)
        return -RT_ERROR;

    // 1. Stop data stream
    xs9950_write_reg(0x4000, 0x00);    // Turn off video output
    sensor->streaming = false;
#ifdef XS9950_INTERRUPT
    // 2. Disable interrupt
    if (sensor->irq_pin) {
        rt_pin_irq_enable(sensor->irq_pin, PIN_IRQ_DISABLE);
        rt_pin_detach_irq(sensor->irq_pin);
    }
#endif
    // 3. Power off
    xs9950_power_off(sensor);

    LOG_I("XS9950 device close done");
    return RT_EOK;
}

/**
 * @brief Get video format
 */
static int xs9950_get_fmt(rt_device_t dev, struct mpp_video_fmt *cfg)
{
    struct xs9950_dev *sensor = (struct xs9950_dev *)dev;
    cfg->code = sensor->fmt.code;
    cfg->width = sensor->fmt.width;
    cfg->height = sensor->fmt.height;
    cfg->flags = sensor->fmt.flags;
    cfg->bus_type = sensor->fmt.bus_type;
    return RT_EOK;
}

#ifdef XS9950_INTERRUPT
/**
 * @brief Interrupt handler (example: video loss interrupt)
 */
static void xs9950_irq_handler(void *args)
{
    struct xs9950_dev *sensor = (struct xs9950_dev *)args;
    u8 irq_reg = xs9950_read_reg(0xE22);  // Interrupt status register

    // Video loss interrupt (bit0)
    if (irq_reg & 0x01) {
        sensor->irq_status |= 0x01;
        LOG_W("Video loss interrupt detected");
        xs9950_write_reg(0xE22, 0x01);  // Clear interrupt
    }

    // MIPI error interrupt (bit1)
    if (irq_reg & 0x02) {
        sensor->irq_status |= 0x02;
        LOG_W("MIPI error interrupt detected");
        xs9950_write_reg(0xE22, 0x02);  // Clear interrupt
    }
}
#endif

/**
 * @brief Device control interface
 */
static rt_err_t xs9950_control(rt_device_t dev, int cmd, void *args)
{
    struct xs9950_dev *sensor = (struct xs9950_dev *)dev;

    switch (cmd) {
        case CAMERA_CMD_START:
            xs9950_write_reg(0x4000, 0x01);
            sensor->streaming = true;
            return RT_EOK;
        case CAMERA_CMD_STOP:
            xs9950_write_reg(0x4000, 0x00);
            sensor->streaming = false;
            return RT_EOK;
        case CAMERA_CMD_GET_FMT:
            return xs9950_get_fmt(dev, (struct mpp_video_fmt *)args);
#ifdef XS9950_INTERRUPT
        case CAMERA_CMD_SET_CH:  // Switch video channel
            if (args && *(u8 *)args < 4) {
                xs9950_sensor_init(*(u8 *)args, sensor->curr_fmt, sensor->curr_std);
                return RT_EOK;
            }
            return -RT_EINVAL;
        case CAMERA_CMD_GET_IRQ_STATUS:  // Get interrupt status
            if (args) {
                *(u8 *)args = sensor->irq_status;
                sensor->irq_status = 0;  // Clear status
                return RT_EOK;
            }
            return -RT_EINVAL;
#endif
        default:
            LOG_I("Unsupported cmd: 0x%x", cmd);
            return -RT_EINVAL;
    }
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops xs9950_ops = {
    .init = xs9950_init,
    .open = xs9950_open,
    .close = xs9950_close,
    .control = xs9950_control,
};
#endif

/**
 * @brief Driver registration
 */
int rt_hw_xs9950_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_xs_dev.dev.ops = &xs9950_ops;
#else
    g_xs_dev.dev.init = xs9950_init;
    g_xs_dev.dev.open = xs9950_open;
    g_xs_dev.dev.close = xs9950_close;
    g_xs_dev.dev.control = xs9950_control;
#endif

    g_xs_dev.dev.type = RT_Device_Class_CAMERA;
    rt_device_register(&g_xs_dev.dev, CAMERA_DEV_NAME, 0);
#ifdef XS9950_INTERRUPT
    // Register interrupt (assuming IRQ pin is configured)
    rt_pin_attach_irq(camera_irq_pin_get(), PIN_IRQ_MODE_FALLING, xs9950_irq_handler, &g_xs_dev);
    rt_pin_irq_enable(camera_irq_pin_get(), PIN_IRQ_ENABLE);
#endif
    LOG_I("XS9950 driver register done");
    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_xs9950_init);
