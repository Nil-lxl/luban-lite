/*
 * Copyright (c) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#define LOG_TAG "xs9950"
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
#include "xs9950_reg.h"

#define DRV_NAME            LOG_TAG
#define DEFAULT_FORMAT      HD720P25
#define DEFAULT_MEDIA_CODE  MEDIA_BUS_FMT_UYVY8_2X8
#define DEFAULT_BUS_TYPE    MEDIA_BUS_BT656
#define DEFAULT_VIN_CH      VIN1

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

static inline enum tp_std xs9950_fmt_to_std(enum tp_fmt fmt)
{
    if (fmt <= CVBS_960H_N)
        return STD_CVBS;
    return STD_HDCCTV;
}

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
    volatile u8 irq_status; // Interrupt status (bit0: video loss; bit1: MIPI error; bit2: control info received)
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

static void xs9950_wait_lock(void)
{
    u32 timeout = 100, cnt = 0;
    u8 video_loss = 0;

    while (1) {
        rt_thread_mdelay(10);

        video_loss = xs9950_read_reg(XS9950_REG_VIDEO_STATUS_1) & XS9950_STS1_FREE_RUN;
        if (!video_loss)
            break;

        cnt++;
        if (cnt > timeout) {
            LOG_E("Wait video source timeout!");
            return;
        }
    }
    rt_thread_mdelay(300);
}

/**
 * @brief Query XS9950 channel status from registers 0x00~0x0B
 * @param status Output status structure pointer
 * @return 0 on success, -1 on I2C error
 */
static int xs9950_query_status(struct xs9950_ch_status *status)
{
    u8 reg_val;

    if (!status)
        return -1;

    memset(status, 0, sizeof(*status));

    /* 0x00: VIDEO_STATUS_REGISTER_1 */
    reg_val = xs9950_read_reg(XS9950_REG_VIDEO_STATUS_1);
    if (reg_val == 0xFF) {
        /* Read might still return 0xFF on valid data for this register,
         * but we proceed and let caller check individual fields */
    }
    status->is_hd             = !!(reg_val & XS9950_STS1_HD_SD);
    status->sd_burst_detected = !!(reg_val & XS9950_STS1_SD_BURST_DETECT);
    status->free_run          = !!(reg_val & XS9950_STS1_FREE_RUN);
    status->hspll_locked_fe   = !!(reg_val & XS9950_STS1_HSPLL_LOCKED_FE);
    status->hspll_locked_be   = !!(reg_val & XS9950_STS1_HSPLL_LOCKED_BE);
    status->vsync_locked      = !!(reg_val & XS9950_STS1_VSYNC_LOCKED);
    status->color_kill        = !!(reg_val & XS9950_STS1_COLOR_KILL);

    /* 0x01: HD_VIDEO_STANDARD_READBACK */
    status->hd_std_raw    = xs9950_read_reg(XS9950_REG_HD_VIDEO_STD_RB);
    status->hd_std_type   = status->hd_std_raw & XS9950_HD_STD_TYPE_MASK;
    status->hd_std_format = status->hd_std_raw & XS9950_HD_STD_FORMAT_MASK;

    /* 0x02: SD_VIDEO_STANDARD_READBACK */
    status->sd_std_raw = xs9950_read_reg(XS9950_REG_SD_VIDEO_STD_RB) & XS9950_SD_STD_MASK;

    /* 0x03: VSYNC_STATUS_REGISTER_2 */
    reg_val = xs9950_read_reg(XS9950_REG_VSYNC_STATUS_2);
    status->has_pedestal = !!(reg_val & XS9950_VSYNC2_PEDESTAL);
    status->is_fifty_hz  = !!(reg_val & XS9950_VSYNC2_FIFTY_HZ);

    /* 0x04~0x05: SIGNAL_LOSS_FACT (16-bit: MSB<<8 | LSB) */
    status->signal_loss = ((u16)xs9950_read_reg(XS9950_REG_SIGNAL_LOSS_MSB) << 8) |
                          xs9950_read_reg(XS9950_REG_SIGNAL_LOSS_LSB);

    /* 0x06~0x07: SYNC_DEPTH (10-bit: [1:0]<<8 | [7:0]) */
    status->sync_depth = (((u16)(xs9950_read_reg(XS9950_REG_SYNC_DEPTH_MSB) & 0x03)) << 8) |
                         xs9950_read_reg(XS9950_REG_SYNC_DEPTH_LSB);

    /* 0x08: HD_STATUS */
    reg_val = xs9950_read_reg(XS9950_REG_HD_STATUS);
    status->hd_sync_depth     = ((u16)((reg_val & XS9950_HD_STS_SYNC_DEPTH_MSB_MASK) >> 6)) << 8;
    status->hd_free_run       = !!(reg_val & XS9950_HD_STS_FREE_RUN);
    status->hd_hspll_locked_fe = !!(reg_val & XS9950_HD_STS_HSPLL_LOCKED_FE);
    status->hd_hspll_locked_be = !!(reg_val & XS9950_HD_STS_HSPLL_LOCKED_BE);
    status->hd_vsync_locked   = !!(reg_val & XS9950_HD_STS_VSYNC_LOCKED);
    status->hd_color_kill     = !!(reg_val & XS9950_HD_STS_COLOR_KILL);

    /* 0x09: SD_STATUS */
    reg_val = xs9950_read_reg(XS9950_REG_SD_STATUS);
    status->sd_sync_depth     = ((u16)((reg_val & XS9950_SD_STS_SYNC_DEPTH_MSB_MASK) >> 6)) << 8;
    status->sd_free_run       = !!(reg_val & XS9950_SD_STS_FREE_RUN);
    status->sd_hspll_locked_fe = !!(reg_val & XS9950_SD_STS_HSPLL_LOCKED_FE);
    status->sd_hspll_locked_be = !!(reg_val & XS9950_SD_STS_HSPLL_LOCKED_BE);
    status->sd_vsync_locked   = !!(reg_val & XS9950_SD_STS_VSYNC_LOCKED);
    status->sd_color_kill     = !!(reg_val & XS9950_SD_STS_COLOR_KILL);

    /* 0x0A: HD_STD (debug standard readback) */
    status->hd_std_debug = xs9950_read_reg(XS9950_REG_HD_STD);

    /* 0x0B: SD_STD (debug standard readback) */
    status->sd_std_debug = xs9950_read_reg(XS9950_REG_SD_STD) & XS9950_SD_STD_MASK;

    return 0;
}

/**
 * @brief Print XS9950 channel status for debugging
 */
static void xs9950_print_status(const struct xs9950_ch_status *s)
{
    static const char *hd_std_type_str[] = { "HDCVI", "AHD", "TVI", "Rsvd" };
    static const char *sd_std_str[] = {
        "NTSC-JM", "NTSC-443", "PAL-M", "PAL-60", "PAL-CN", "PAL-BGHID",
        "Rsvd6", "Rsvd7", "Rsvd8", "Rsvd9", "RsvdA", "RsvdB",
        "RsvdC", "RsvdD", "RsvdE", "No Signal"
    };

    LOG_I("---------------- XS9950 Status -----------------");
    LOG_I("Video mode: %s", s->is_hd ? "HD" : "SD");
    LOG_I("Free run  : %s", s->free_run ? "YES (no signal)" : "NO");
    LOG_I("Vsync lock: %s", s->vsync_locked ? "LOCKED" : "UNLOCKED");
    LOG_I("PLL FE/BE : %s/%s",
          s->hspll_locked_fe ? "LOCKED" : "UNLOCKED",
          s->hspll_locked_be ? "LOCKED" : "UNLOCKED");
    LOG_I("Color kill: %s", s->color_kill ? "YES (no color)" : "NO");
    LOG_I("SD burst  : %s", s->sd_burst_detected ? "detected" : "not detected");

    if (s->is_hd) {
        LOG_I("HD standard: %s (type=0x%02X, fmt=0x%02X)",
              hd_std_type_str[(s->hd_std_type >> 6) & 0x03],
              s->hd_std_type, s->hd_std_format);
        LOG_I("HD free run : %s", s->hd_free_run ? "YES" : "NO");
        LOG_I("HD PLL FE/BE: %s/%s",
              s->hd_hspll_locked_fe ? "LOCKED" : "UNLOCKED",
              s->hd_hspll_locked_be ? "LOCKED" : "UNLOCKED");
        LOG_I("HD vsync    : %s", s->hd_vsync_locked ? "LOCKED" : "UNLOCKED");
        LOG_I("HD color kill: %s", s->hd_color_kill ? "YES" : "NO");
    } else {
        LOG_I("SD standard: %s", sd_std_str[s->sd_std_raw & 0x0F]);
        LOG_I("Pedestal   : %s", s->has_pedestal ? "YES" : "NO");
        LOG_I("Field rate : %s", s->is_fifty_hz ? "50Hz" : "60Hz");
        LOG_I("SD free run : %s", s->sd_free_run ? "YES" : "NO");
        LOG_I("SD PLL FE/BE: %s/%s",
              s->sd_hspll_locked_fe ? "LOCKED" : "UNLOCKED",
              s->sd_hspll_locked_be ? "LOCKED" : "UNLOCKED");
        LOG_I("SD vsync    : %s", s->sd_vsync_locked ? "LOCKED" : "UNLOCKED");
        LOG_I("SD color kill: %s", s->sd_color_kill ? "YES" : "NO");
    }

    LOG_I("Signal loss : %u", s->signal_loss);
    LOG_I("Sync depth  : %u (SD %u, HD %u)", s->sync_depth,
          s->sd_sync_depth, s->hd_sync_depth);
    LOG_I("------------------------------------------------");
}

static void xs9950_status(int argc, char **argv)
{
    struct xs9950_ch_status status = {0};

    if (xs9950_query_status(&status) != 0) {
        LOG_E("Failed to query XS9950 status");
        return;
    }
    xs9950_print_status(&status);
}
#ifdef RT_USING_FINSH
#include <finsh.h>
MSH_CMD_EXPORT(xs9950_status, Query XS9950 channel status 0x00-0x0B);
#endif

static int xs9950_set_contrast(struct xs9950_dev *sensor, u32 percent)
{
    u8 val = PERCENT_TO_INT(0, 255, percent);

    return xs9950_write_reg(0x0106, val);
}

static int xs9950_set_brightness(struct xs9950_dev *sensor, u32 percent)
{
    s8 val = PERCENT_TO_INT((s8)0x80, (s8)0x7F, percent);

    return xs9950_write_reg(0x0107, val);
}

static int xs9950_set_saturation(struct xs9950_dev *sensor, u32 percent)
{
    u8 val = PERCENT_TO_INT(0, 255, percent);

    return xs9950_write_reg(0x0108, val);
}

static int xs9950_set_hue(struct xs9950_dev *sensor, u32 percent)
{
    s8 val = PERCENT_TO_INT((s8)0x80, (s8)0x7F, percent);

    return xs9950_write_reg(0x0109, val);
}

static void xs9950_adjust_color(struct xs9950_dev *sensor, enum tp_fmt fmt)
{
    switch (fmt) {
    case HD720P25:
        xs9950_set_hue(sensor, 40);
        xs9950_set_saturation(sensor, 70);
        break;
    default:
        break;
    }
}

static void xs9950_adjust_blank(enum tp_fmt fmt)
{
    switch (fmt) {
    case HD720P25:
        xs9950_write_reg(0x010a, 0x80);
        break;
    default:
        break;
    }
}

/**
 * @brief Set video resolution (new: associate format with resolution)
 */
static void xs9950_set_resolution(enum tp_fmt fmt)
{
    u16 h_active, v_active;

    switch (fmt) {
        case CVBS_PAL:
            h_active = 720;
            v_active = 576;
            break;
        case CVBS_NTSC:
            h_active = 720;
            v_active = 480;
            break;
        case CVBS_960H_P:
            h_active = 960;
            v_active = 576;
            break;
        case CVBS_960H_N:
            h_active = 960;
            v_active = 480;
            break;
        case HD720P25:
            h_active = 1280;
            v_active = 720;
            break;
        case HD720P30:
        case HD720P50:
        case HD720P60:
            h_active = 1280;
            v_active = 720;
            break;
        case HD960P25:
        case HD960P30:
            h_active = 1280;
            v_active = 960;
            break;
        case FHD1080P15:
        case FHD1080P25:
        case FHD1080P30:
            h_active = 1920;
            v_active = 1080;
            break;
        default:
            h_active = 1280;
            v_active = 720; // Default 720P
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

static void xs9950_bt656_init_sk(void)
{
    // enVoClkEdge = NI_VO_CLK_EDGE_RISING;  // Rising edge sampling
    // vo_clk = 148.5MHz
    // xs9950_write_reg(0x4135, 0x02);  // Default: BT656 D0-D7 and VO_HS/VS IO level, bit1=0:1.8V, bit1=1:3.3V
    // xs9950_write_reg(0x101F, 0x00);  // Default: bit4=0: VCCA18_33 is 1.8V, bit4=1: VCCA18_33 is 3.3V
    // xs9950_write_reg(0x4107, 0x40);  // Default: bit0=0: iic_rst_ls3v is 1.8V, bit0=1: iic_rst_ls3v is 3.3V
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
    xs9950_write_reg(0x0158, 0x01);
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
    xs9950_write_reg(0x0504, 0x89); // bit[4:7] free_run color, value range: [0, 8]
    xs9950_write_reg(0x0507, 0x0b); // bit[5] 0: rising edge capture; 1: up/down edge capture
    xs9950_write_reg(0x0503, 0x00);
    xs9950_write_reg(0x0502, 0x00); // bit4 bt data0-7 inverse, changed to data7-0
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
    xs9950_write_reg(0x4134, 0x0a); // Increase BT output driving capability 0x6 -> 0xa
    xs9950_write_reg(0x0803, 0x0f);
    xs9950_write_reg(0x4412, 0x01);
    xs9950_write_reg(0x0803, 0x1f);
    xs9950_write_reg(0x10e3, 0x04);
    xs9950_write_reg(0x10eb, 0xfd);
    xs9950_write_reg(0x0800, 0x07);
    xs9950_write_reg(0x0805, 0x07);
    xs9950_write_reg(0x01c4, 0x11);
    xs9950_write_reg(0x01ce, 0x01);
    // xs9950_write_reg(0x4200, 0x02); // 0x4200=2: VINA, 0x4200=0: VINB, 0x4200=4: VINC, 0x4200=6: VIND
    // xs9950_write_reg(0x0111, 0x68);  // Adjust these 3 registers to increase clarity and saturation
    // xs9950_write_reg(0x4202, 0x0e);
    // xs9950_write_reg(0x4203, 0x04);
    // xs9950_write_reg(0x0100, 0x38);
    // xs9950_write_reg(0x01ce, 0x00);  // Adjust auto exposure
}

static void xs9950_bt656_fmt_sk(enum tp_fmt fmt)
{
    unsigned char sta_0507;
    if ((HD720P25 == fmt) || (HD720P30 == fmt) || (HD720P50 == fmt) || (HD720P60 == fmt))
    {
        xs9950_write_reg(0x060b, 0x00);
        xs9950_write_reg(0x0627, 0x14);
        xs9950_write_reg(0x010c, 0x00);
        xs9950_write_reg(0x0800, 0x05);
        xs9950_write_reg(0x0805, 0x05);
        if (HD720P60 == fmt)
        {
            xs9950_write_reg(0x0b50, 0x08);
            xs9950_write_reg(0x0b4e, 0x3f);
            xs9950_write_reg(0x0b50, 0x3f);
            xs9950_write_reg(0x0b51, 0x52);
            xs9950_write_reg(0x4201, 0x00);
            xs9950_write_reg(0x4203, 0x00);
            xs9950_write_reg(0x4202, 0x00);
            xs9950_write_reg(0x4204, 0x00);
            xs9950_write_reg(0x0b4e, 0x06);
            xs9950_write_reg(0x0b50, 0x07);
            xs9950_write_reg(0x0b51, 0x21);
        }
        else
        {
            xs9950_write_reg(0x0b50, 0x08);
        }
        xs9950_write_reg(0x0e08, 0x00);
        if (HD720P25 == fmt)
            xs9950_write_reg(0x010d, 0x40);
        else if (HD720P30 == fmt)
            xs9950_write_reg(0x010d, 0x41);
        else if (HD720P50 == fmt)
            xs9950_write_reg(0x010d, 0x42);
        else if (HD720P60 == fmt)
            xs9950_write_reg(0x010d, 0x43);
        xs9950_write_reg(0x010c, 0x01);
        xs9950_write_reg(0x0121, 0x6a);
        xs9950_write_reg(0x0122, 0x5b);
        xs9950_write_reg(0x0130, 0x10);
        xs9950_write_reg(0x01a9, 0x00);
        xs9950_write_reg(0x01aa, 0x04);
        xs9950_write_reg(0x0156, 0x00);
        xs9950_write_reg(0x0157, 0x08);
        xs9950_write_reg(0x0105, 0xe1); // For AHD: if color saturation is abnormal, toggle bit5 of 0x105
        xs9950_write_reg(0x0101, 0x42);
        xs9950_write_reg(0x0102, 0x40);
        xs9950_write_reg(0x0116, 0x3c);
        xs9950_write_reg(0x0117, 0x23); // For AHD: if color saturation is abnormal, enable 0x105 bit5 and fine-tune 0x117
        xs9950_write_reg(0x01e2, 0x03);
        xs9950_write_reg(0x420b, 0x2f); //clamp
        xs9950_write_reg(0x0100, 0x38);
        xs9950_write_reg(0x0106, 0x80);
        xs9950_write_reg(0x0107, 0x00);
        xs9950_write_reg(0x0108, 0x80);
        xs9950_write_reg(0x0109, 0x00);
        if (HD720P25 == fmt)
            xs9950_write_reg(0x010a, 0x20);
        else
            xs9950_write_reg(0x010a, 0x1a);
        if (HD720P60 == fmt)
            xs9950_write_reg(0x010b, 0x80);
        else
            xs9950_write_reg(0x010b, 0x00);
        xs9950_write_reg(0x011d, 0x17);
        xs9950_write_reg(0x0e08, 0x01);
        xs9950_write_reg(0x0a60, 0x04);
        if (HD720P25 == fmt)
        {
            xs9950_write_reg(0x0a5c, 0xf6);
            xs9950_write_reg(0x0a5d, 0xc0);
            xs9950_write_reg(0x0a5e, 0x2d);
            xs9950_write_reg(0x0a5f, 0x1b);
        }
        else if (HD720P30 == fmt)
        {
            xs9950_write_reg(0x0a5c, 0x16);
            xs9950_write_reg(0x0a5d, 0x00);
            xs9950_write_reg(0x0a5e, 0x1f);
            xs9950_write_reg(0x0a5f, 0x1b);
        }
        else if (HD720P50 == fmt)
        {
            xs9950_write_reg(0x0a5c, 0xb4);
            xs9950_write_reg(0x0a5d, 0x80);
            xs9950_write_reg(0x0a5e, 0xef);
            xs9950_write_reg(0x0a5f, 0x38);
        }
        else if (HD720P60 == fmt)
        {
            xs9950_write_reg(0x0a5c, 0x4e);
            xs9950_write_reg(0x0a5d, 0xeb);
            xs9950_write_reg(0x0a5e, 0xe8);
            xs9950_write_reg(0x0a5f, 0x38);
        }
        xs9950_write_reg(0x0156, 0x50);
        xs9950_write_reg(0x0157, 0x07);
        xs9950_write_reg(0x0156, 0x00);
        xs9950_write_reg(0x0157, 0x08);
        xs9950_write_reg(0x0158, 0x01);
        if (HD720P25 == fmt)
            xs9950_write_reg(0x0503, 0x00);
        else if (HD720P30 == fmt)
            xs9950_write_reg(0x0503, 0x01);
        else if (HD720P50 == fmt)
            xs9950_write_reg(0x0503, 0x02);
        else if (HD720P60 == fmt)
            xs9950_write_reg(0x0503, 0x03);
        xs9950_write_reg(0x015a, 0x8b);
        xs9950_write_reg(0x015b, 0x0e);
        if (HD720P25 == fmt)
        {
            xs9950_write_reg(0x015c, 0x80);
            xs9950_write_reg(0x015d, 0x16);
        }
        else if (HD720P30 == fmt)
        {
            xs9950_write_reg(0x015c, 0xc0);
            xs9950_write_reg(0x015d, 0x12);
        }
        else if (HD720P50 == fmt)
        {
            xs9950_write_reg(0x015c, 0x40);
            xs9950_write_reg(0x015d, 0x0b);
        }
        else if (HD720P60 == fmt)
        {
            xs9950_write_reg(0x015c, 0x60);
            xs9950_write_reg(0x015d, 0x09);
        }
        xs9950_write_reg(0x015e, 0xd0);
        xs9950_write_reg(0x015f, 0x02);
        if ((HD720P25 == fmt) || (HD720P30 == fmt) || (HD720P50 == fmt))
        {
            xs9950_write_reg(0x0160, 0xee);
            xs9950_write_reg(0x0161, 0x02);
        }
        if (HD720P25 == fmt)
        {
            xs9950_write_reg(0x0165, 0x40);
            xs9950_write_reg(0x0166, 0x0f);
        }
        else if (HD720P30 == fmt)
        {
            xs9950_write_reg(0x0165, 0x41);
            xs9950_write_reg(0x0166, 0x0f);
            xs9950_write_reg(0x0147, 0x00);
            xs9950_write_reg(0x0147, 0x00);
        }
        else if (HD720P50 == fmt)
        {
            xs9950_write_reg(0x0165, 0x42);
            xs9950_write_reg(0x0166, 0x0f);
        }
        if ((HD720P25 == fmt) || (HD720P30 == fmt))
        {
            xs9950_write_reg(0x0A00, 0xFD);
            xs9950_write_reg(0x0A01, 0xFF);
            xs9950_write_reg(0x0A02, 0x00);
            xs9950_write_reg(0x0A03, 0x00);
            xs9950_write_reg(0x0A04, 0x04);
            xs9950_write_reg(0x0A05, 0x00);
            xs9950_write_reg(0x0A06, 0x01);
            xs9950_write_reg(0x0A07, 0x00);
            xs9950_write_reg(0x0A08, 0xFB);
            xs9950_write_reg(0x0A09, 0xFF);
            xs9950_write_reg(0x0A0A, 0xFE);
            xs9950_write_reg(0x0A0B, 0xFF);
            xs9950_write_reg(0x0A0C, 0x07);
            xs9950_write_reg(0x0A0D, 0x00);
            xs9950_write_reg(0x0A0E, 0x03);
            xs9950_write_reg(0x0A0F, 0x00);
            xs9950_write_reg(0x0A10, 0xF7);
            xs9950_write_reg(0x0A11, 0xFF);
            xs9950_write_reg(0x0A12, 0xFA);
            xs9950_write_reg(0x0A13, 0xFF);
            xs9950_write_reg(0x0A14, 0x0B);
            xs9950_write_reg(0x0A15, 0x00);
            xs9950_write_reg(0x0A16, 0x0A);
            xs9950_write_reg(0x0A17, 0x00);
            xs9950_write_reg(0x0A18, 0xF3);
            xs9950_write_reg(0x0A19, 0xFF);
            xs9950_write_reg(0x0A1A, 0xF1);
            xs9950_write_reg(0x0A1B, 0xFF);
            xs9950_write_reg(0x0A1C, 0x0F);
            xs9950_write_reg(0x0A1D, 0x00);
            xs9950_write_reg(0x0A1E, 0x18);
            xs9950_write_reg(0x0A1F, 0x00);
            xs9950_write_reg(0x0A20, 0xEE);
            xs9950_write_reg(0x0A21, 0xFF);
            xs9950_write_reg(0x0A22, 0xDC);
            xs9950_write_reg(0x0A23, 0xFF);
            xs9950_write_reg(0x0A24, 0x14);
            xs9950_write_reg(0x0A25, 0x00);
            xs9950_write_reg(0x0A26, 0x39);
            xs9950_write_reg(0x0A27, 0x00);
            xs9950_write_reg(0x0A28, 0xEB);
            xs9950_write_reg(0x0A29, 0xFF);
            xs9950_write_reg(0x0A2A, 0x98);
            xs9950_write_reg(0x0A2B, 0xFF);
            xs9950_write_reg(0x0A2C, 0x16);
            xs9950_write_reg(0x0A2D, 0x00);
            xs9950_write_reg(0x0A2E, 0x45);
            xs9950_write_reg(0x0A2F, 0x01);
            xs9950_write_reg(0x0A30, 0xEA);
            xs9950_write_reg(0x0A31, 0x01);
        }
        else if ((HD720P50 == fmt) || (HD720P60 == fmt))
        {
            xs9950_write_reg(0x0A00, 0x00);
            xs9950_write_reg(0x0A01, 0x00);
            xs9950_write_reg(0x0A02, 0xFE);
            xs9950_write_reg(0x0A03, 0xFF);
            xs9950_write_reg(0x0A04, 0x04);
            xs9950_write_reg(0x0A05, 0x00);
            xs9950_write_reg(0x0A06, 0xFD);
            xs9950_write_reg(0x0A07, 0xFF);
            xs9950_write_reg(0x0A08, 0x00);
            xs9950_write_reg(0x0A09, 0x00);
            xs9950_write_reg(0x0A0A, 0x04);
            xs9950_write_reg(0x0A0B, 0x00);
            xs9950_write_reg(0x0A0C, 0xF9);
            xs9950_write_reg(0x0A0D, 0xFF);
            xs9950_write_reg(0x0A0E, 0x06);
            xs9950_write_reg(0x0A0F, 0x00);
            xs9950_write_reg(0x0A10, 0x00);
            xs9950_write_reg(0x0A11, 0x00);
            xs9950_write_reg(0x0A12, 0xF8);
            xs9950_write_reg(0x0A13, 0xFF);
            xs9950_write_reg(0x0A14, 0x0E);
            xs9950_write_reg(0x0A15, 0x00);
            xs9950_write_reg(0x0A16, 0xF5);
            xs9950_write_reg(0x0A17, 0xFF);
            xs9950_write_reg(0x0A18, 0x00);
            xs9950_write_reg(0x0A19, 0x00);
            xs9950_write_reg(0x0A1A, 0x0F);
            xs9950_write_reg(0x0A1B, 0x00);
            xs9950_write_reg(0x0A1C, 0xE7);
            xs9950_write_reg(0x0A1D, 0xFF);
            xs9950_write_reg(0x0A1E, 0x14);
            xs9950_write_reg(0x0A1F, 0x00);
            xs9950_write_reg(0x0A20, 0x00);
            xs9950_write_reg(0x0A21, 0x00);
            xs9950_write_reg(0x0A22, 0xE3);
            xs9950_write_reg(0x0A23, 0xFF);
            xs9950_write_reg(0x0A24, 0x31);
            xs9950_write_reg(0x0A25, 0x00);
            xs9950_write_reg(0x0A26, 0xD5);
            xs9950_write_reg(0x0A27, 0xFF);
            xs9950_write_reg(0x0A28, 0x00);
            xs9950_write_reg(0x0A29, 0x00);
            xs9950_write_reg(0x0A2A, 0x4B);
            xs9950_write_reg(0x0A2B, 0x00);
            xs9950_write_reg(0x0A2C, 0x5F);
            xs9950_write_reg(0x0A2D, 0xFF);
            xs9950_write_reg(0x0A2E, 0xE6);
            xs9950_write_reg(0x0A2F, 0x00);
            xs9950_write_reg(0x0A30, 0x00);
            xs9950_write_reg(0x0A31, 0x03);
        }
        xs9950_write_reg(0x0A60, 0x01);
        if ((HD720P25 == fmt) || (HD720P30 == fmt))
        {
            xs9950_write_reg(0x0336, 0x7e);
            xs9950_write_reg(0x033b, 0x03);
            /* Additional config for non-standard AHD720 signals */
            xs9950_write_reg(0x0AA7, 0x7A);
            xs9950_write_reg(0x0AA8, 0x18);
            xs9950_write_reg(0x0AA9, 0xC0);
            xs9950_write_reg(0x0AAA, 0x01);
            xs9950_write_reg(0x0AAB, 0xC2);
            xs9950_write_reg(0x0AAC, 0x01);
            xs9950_write_reg(0x0AAD, 0x80);
            xs9950_write_reg(0x0AAE, 0x43);
            xs9950_write_reg(0x0AAF, 0x00);
            xs9950_write_reg(0x0AB0, 0x70);
            xs9950_write_reg(0x0AB1, 0x00);
            xs9950_write_reg(0x0AB2, 0x1B);
            xs9950_write_reg(0x0A88, 0x30);
        }
    }
    else if ((CVBS_PAL == fmt) || (CVBS_960H_P == fmt) || (CVBS_NTSC == fmt) || (CVBS_960H_N == fmt))
    {
        xs9950_write_reg(0x0e08, 0x00);
        xs9950_write_reg(0x0102, 0x40);
        xs9950_write_reg(0x0105, 0xe1);
        xs9950_write_reg(0x0108, 0x80);
        xs9950_write_reg(0x080d, 0x00);
        xs9950_write_reg(0x0158, 0x01);
        xs9950_write_reg(0x0a60, 0x00);
        xs9950_write_reg(0x0a88, 0x20);
        xs9950_write_reg(0x0121, 0x5a);
        xs9950_write_reg(0x0122, 0x4b);
        xs9950_write_reg(0x0125, 0x73);
        xs9950_write_reg(0x010c, 0x00);
        xs9950_write_reg(0x420b, 0x2f); //clamp
        xs9950_write_reg(0x0100, 0x38);
        xs9950_write_reg(0x0a60, 0x00);
        xs9950_write_reg(0x0803, 0x1f);
        xs9950_write_reg(0x080e, 0x1f);
        xs9950_write_reg(0x0803, 0x1f);
        xs9950_write_reg(0x080e, 0x3f);
        xs9950_write_reg(0x080e, 0x3f);
        xs9950_write_reg(0x0e08, 0x01);
        xs9950_write_reg(0x0800, 0x04);
        xs9950_write_reg(0x0805, 0x07);
        xs9950_write_reg(0x0800, 0x04);
        xs9950_write_reg(0x0800, 0x06);
        xs9950_write_reg(0x0805, 0x0e);
        xs9950_write_reg(0x0b50, 0x08);
        xs9950_write_reg(0x0e08, 0x00);
        xs9950_write_reg(0x010c, 0x00);
        xs9950_write_reg(0x0305, 0xe1);
        xs9950_write_reg(0x033b, 0x02);
        xs9950_write_reg(0x0511, 0x00);
        xs9950_write_reg(0x0158, 0x03);
        xs9950_write_reg(0x0a60, 0x00);
        xs9950_write_reg(0x0a88, 0x20);
        xs9950_write_reg(0x0121, 0x5a);
        xs9950_write_reg(0x0122, 0x4b);
        xs9950_write_reg(0x0125, 0x73);
        xs9950_write_reg(0x0126, 0x4c);
        xs9950_write_reg(0x0505, 0x00);
        xs9950_write_reg(0x0506, 0x00);
        xs9950_write_reg(0x0106, 0x80);
        xs9950_write_reg(0x0107, 0x00);
        xs9950_write_reg(0x0108, 0x80);
        xs9950_write_reg(0x0109, 0x00);
        if ((CVBS_PAL == fmt) || (CVBS_960H_P == fmt))
        {
            xs9950_write_reg(0x010a, 0x04);
            xs9950_write_reg(0x010a, 0x04);
        }
        else
        {
            xs9950_write_reg(0x010a, 0x12);
            xs9950_write_reg(0x010a, 0x12);
        }
        xs9950_write_reg(0x010b, 0x02);
        xs9950_write_reg(0x010b, 0x02);
        xs9950_write_reg(0x033a, 0x00);
        xs9950_write_reg(0x0e08, 0x01);
        xs9950_write_reg(0x0102, 0x40);
        xs9950_write_reg(0x0105, 0xe1);
        xs9950_write_reg(0x0108, 0x80);
        xs9950_write_reg(0x0156, 0x00);
        xs9950_write_reg(0x0157, 0x00);
        xs9950_write_reg(0x0156, 0x00);
        xs9950_write_reg(0x0157, 0x00);
        sta_0507 = xs9950_read_reg(0x0507);
        if ((CVBS_PAL == fmt) || (CVBS_NTSC == fmt))
            sta_0507 &= ~(0x01 << 4); // bit4, 0:720H; 1:960H;
        else
            sta_0507 |= (0x01 << 4); // bit4, 0:720H; 1:960H;
        xs9950_write_reg(0x0507, sta_0507);
        if ((CVBS_PAL == fmt) || (CVBS_960H_P == fmt))
            xs9950_write_reg(0x0503, 0x48);
        else
            xs9950_write_reg(0x0503, 0x60);
        xs9950_write_reg(0x015a, 0xc0);
        xs9950_write_reg(0x015b, 0x03);
        if ((CVBS_PAL == fmt) || (CVBS_960H_P == fmt))
        {
            xs9950_write_reg(0x015c, 0x00);
            xs9950_write_reg(0x015d, 0x36);
            xs9950_write_reg(0x015e, 0x20);
            xs9950_write_reg(0x015f, 0x01);
        }
        else
        {
            xs9950_write_reg(0x015c, 0x49);
            xs9950_write_reg(0x015d, 0x00);
            xs9950_write_reg(0x015e, 0xf0);
            xs9950_write_reg(0x015f, 0x00);
        }
        xs9950_write_reg(0x0160, 0x39);
        xs9950_write_reg(0x0161, 0x01);
        xs9950_write_reg(0x0165, 0xff);
        if ((CVBS_PAL == fmt) || (CVBS_960H_P == fmt))
            xs9950_write_reg(0x0166, 0x05);
        else
            xs9950_write_reg(0x0166, 0x00);
        /* Additional config */
        if ((CVBS_PAL == fmt) || (CVBS_960H_P == fmt))
        {
            xs9950_write_reg(0x0336, 0xde);
            xs9950_write_reg(0x033B, 0x02);
            xs9950_write_reg(0x0316, 0x48);
        }
        else
        {
            xs9950_write_reg(0x0316, 0x48);
            xs9950_write_reg(0x0336, 0xde);
            xs9950_write_reg(0x0337, 0x01);
        }
    }
    else if ((FHD1080P25 == fmt) || (FHD1080P30 == fmt))
    {
        xs9950_write_reg(0x060b, 0x00);
        xs9950_write_reg(0x0627, 0x14);
        xs9950_write_reg(0x010c, 0x00);
        xs9950_write_reg(0x0800, 0x05);
        xs9950_write_reg(0x0805, 0x05);
        xs9950_write_reg(0x0b50, 0x08);
        xs9950_write_reg(0x0e08, 0x00);
        if (FHD1080P25 == fmt)
            xs9950_write_reg(0x010d, 0x44);
        else
            xs9950_write_reg(0x010d, 0x45);
        xs9950_write_reg(0x010c, 0x01);
        xs9950_write_reg(0x0121, 0x6a);
        xs9950_write_reg(0x0122, 0x5b);
        xs9950_write_reg(0x0130, 0x10);
        xs9950_write_reg(0x01a9, 0x00);
        xs9950_write_reg(0x01aa, 0x04);
        xs9950_write_reg(0x0156, 0x00);
        xs9950_write_reg(0x0157, 0x08);
        xs9950_write_reg(0x0105, 0xe1); // For AHD: if color saturation is abnormal, toggle bit5 of 0x105
        xs9950_write_reg(0x0101, 0x42);
        xs9950_write_reg(0x0102, 0x40);
        xs9950_write_reg(0x0116, 0x3c);
        xs9950_write_reg(0x0117, 0x23); // For AHD: if color saturation is abnormal, enable 0x105 bit5 and fine-tune 0x117
        xs9950_write_reg(0x01e2, 0x03);
        xs9950_write_reg(0x420b, 0x2f); //clamp
        xs9950_write_reg(0x0100, 0x38);
        xs9950_write_reg(0x0106, 0x80);
        xs9950_write_reg(0x0107, 0x00);
        xs9950_write_reg(0x0108, 0x80);
        xs9950_write_reg(0x0109, 0x00);
        xs9950_write_reg(0x010a, 0x1b);
        xs9950_write_reg(0x010b, 0x01);
        xs9950_write_reg(0x011d, 0x17);
        xs9950_write_reg(0x0e08, 0x01);
        xs9950_write_reg(0x0a60, 0x04);
        if (FHD1080P25 == fmt)
        {
            xs9950_write_reg(0x0a5c, 0x56);
            xs9950_write_reg(0x0a5d, 0x00);
            xs9950_write_reg(0x0a5e, 0xe7);
        }
        else
        {
            xs9950_write_reg(0x0a5c, 0x3c);
            xs9950_write_reg(0x0a5d, 0x10);
            xs9950_write_reg(0x0a5e, 0xec);
        }
        xs9950_write_reg(0x0a5f, 0x38);
        xs9950_write_reg(0x0156, 0x50);
        xs9950_write_reg(0x0157, 0x07);
        xs9950_write_reg(0x0156, 0x00);
        xs9950_write_reg(0x0157, 0x08);
        xs9950_write_reg(0x0158, 0x01);
        if (FHD1080P25 == fmt)
            xs9950_write_reg(0x0503, 0x04);
        else
            xs9950_write_reg(0x0503, 0x05);
        xs9950_write_reg(0x015a, 0xd1);
        xs9950_write_reg(0x015b, 0x15);
        if (FHD1080P25 == fmt)
        {
            xs9950_write_reg(0x015c, 0x00);
            xs9950_write_reg(0x015d, 0x0f);
        }
        else
        {
            xs9950_write_reg(0x015c, 0x80);
            xs9950_write_reg(0x015d, 0x0c);
        }
        xs9950_write_reg(0x015e, 0x38);
        xs9950_write_reg(0x015f, 0x04);
        xs9950_write_reg(0x0160, 0x65);
        xs9950_write_reg(0x0161, 0x04);
        if (FHD1080P25 == fmt)
            xs9950_write_reg(0x0165, 0x44);
        else
            xs9950_write_reg(0x0165, 0x45);
        xs9950_write_reg(0x0166, 0x0f);
        xs9950_write_reg(0x0A00, 0x00);
        xs9950_write_reg(0x0A01, 0x00);
        xs9950_write_reg(0x0A02, 0xFE);
        xs9950_write_reg(0x0A03, 0xFF);
        xs9950_write_reg(0x0A04, 0x04);
        xs9950_write_reg(0x0A05, 0x00);
        xs9950_write_reg(0x0A06, 0xFD);
        xs9950_write_reg(0x0A07, 0xFF);
        xs9950_write_reg(0x0A08, 0x00);
        xs9950_write_reg(0x0A09, 0x00);
        xs9950_write_reg(0x0A0A, 0x04);
        xs9950_write_reg(0x0A0B, 0x00);
        xs9950_write_reg(0x0A0C, 0xF9);
        xs9950_write_reg(0x0A0D, 0xFF);
        xs9950_write_reg(0x0A0E, 0x06);
        xs9950_write_reg(0x0A0F, 0x00);
        xs9950_write_reg(0x0A10, 0x00);
        xs9950_write_reg(0x0A11, 0x00);
        xs9950_write_reg(0x0A12, 0xF8);
        xs9950_write_reg(0x0A13, 0xFF);
        xs9950_write_reg(0x0A14, 0x0E);
        xs9950_write_reg(0x0A15, 0x00);
        xs9950_write_reg(0x0A16, 0xF5);
        xs9950_write_reg(0x0A17, 0xFF);
        xs9950_write_reg(0x0A18, 0x00);
        xs9950_write_reg(0x0A19, 0x00);
        xs9950_write_reg(0x0A1A, 0x0F);
        xs9950_write_reg(0x0A1B, 0x00);
        xs9950_write_reg(0x0A1C, 0xE7);
        xs9950_write_reg(0x0A1D, 0xFF);
        xs9950_write_reg(0x0A1E, 0x14);
        xs9950_write_reg(0x0A1F, 0x00);
        xs9950_write_reg(0x0A20, 0x00);
        xs9950_write_reg(0x0A21, 0x00);
        xs9950_write_reg(0x0A22, 0xE3);
        xs9950_write_reg(0x0A23, 0xFF);
        xs9950_write_reg(0x0A24, 0x31);
        xs9950_write_reg(0x0A25, 0x00);
        xs9950_write_reg(0x0A26, 0xD5);
        xs9950_write_reg(0x0A27, 0xFF);
        xs9950_write_reg(0x0A28, 0x00);
        xs9950_write_reg(0x0A29, 0x00);
        xs9950_write_reg(0x0A2A, 0x4B);
        xs9950_write_reg(0x0A2B, 0x00);
        xs9950_write_reg(0x0A2C, 0x5F);
        xs9950_write_reg(0x0A2D, 0xFF);
        xs9950_write_reg(0x0A2E, 0xE6);
        xs9950_write_reg(0x0A2F, 0x00);
        xs9950_write_reg(0x0A30, 0x00);
        xs9950_write_reg(0x0A31, 0x03);
        xs9950_write_reg(0x0A60, 0x01);
    }
    else
    {
        printf("(%d)unknown fmt\n", fmt);
    }
}

static void xs9950_regbt656_sk(enum tp_vin_ch ch, enum tp_fmt fmt, enum tp_std std)
{
    unsigned char sta_4303;

    xs9950_write_reg(0x4300, 0x5);
    xs9950_write_reg(0x4300, 0x15);
    rt_thread_mdelay(1);
    sta_4303 = xs9950_read_reg(0x4303);
    if (0x2 == sta_4303)
    {
        rt_thread_mdelay(1);
    }
    else
    {
        xs9950_write_reg(0x4300, 0x5);
        xs9950_write_reg(0x4300, 0x15);
        rt_thread_mdelay(1);
    }
    xs9950_write_reg(0x4080, 0x07);
    xs9950_write_reg(0x4119, 0x1);
    xs9950_write_reg(0x0803, 0x00);
    xs9950_write_reg(0x4020, 0x00);
    xs9950_write_reg(0x080E, 0x00);
    xs9950_write_reg(0x080E, 0x20);
    xs9950_write_reg(0x080E, 0x28);
    xs9950_write_reg(0x4020, 0x03);
    xs9950_write_reg(0x0803, 0x1f);
    xs9950_write_reg(0x0100, 0x35);
    xs9950_write_reg(0x0104, 0x48);
    xs9950_write_reg(0x0300, 0x3f);
    xs9950_write_reg(0x0105, 0xe1);
    xs9950_write_reg(0x0101, 0x42);
    xs9950_write_reg(0x0102, 0x40);
    xs9950_write_reg(0x0116, 0x3c);
    xs9950_write_reg(0x0117, 0x23);
    xs9950_write_reg(0x0333, 0x23);
    xs9950_write_reg(0x0336, 0x9e);
    xs9950_write_reg(0x0337, 0xd9);
    xs9950_write_reg(0x0338, 0x0a);
    xs9950_write_reg(0x01BF, 0x4e);
    xs9950_write_reg(0x010E, 0x78);
    xs9950_write_reg(0x010F, 0x92);
    xs9950_write_reg(0x0110, 0x70);
    xs9950_write_reg(0x0111, 0x40);
    xs9950_write_reg(0x0314, 0x66);
    xs9950_write_reg(0x0130, 0x10);
    xs9950_write_reg(0x0315, 0x23);
    xs9950_write_reg(0x0B64, 0x2);
    xs9950_write_reg(0x01E2, 0x03);
    xs9950_write_reg(0x0B55, 0x80);
    xs9950_write_reg(0x0B56, 0x0);
    xs9950_write_reg(0x0B59, 0x4);
    xs9950_write_reg(0x0B5A, 0x1);
    xs9950_write_reg(0x0B5C, 0x7);
    xs9950_write_reg(0x0B5E, 0x5);
    xs9950_write_reg(0x0B4B, 0x10);
    xs9950_write_reg(0x0B4E, 0x5);
    xs9950_write_reg(0x0B51, 0x21);
    xs9950_write_reg(0x0B30, 0xBC);
    xs9950_write_reg(0x0B31, 0x19);
    xs9950_write_reg(0x0B15, 0x3);
    xs9950_write_reg(0x0B16, 0x3);
    xs9950_write_reg(0x0B17, 0x3);
    xs9950_write_reg(0x0B07, 0x3);
    xs9950_write_reg(0x0B08, 0x5);
    xs9950_write_reg(0x0B1A, 0x10);
    xs9950_write_reg(0x0158, 0x3);
    xs9950_write_reg(0x0A88, 0x20);
    xs9950_write_reg(0x0A61, 0x09);
    xs9950_write_reg(0x0A62, 0x00);
    xs9950_write_reg(0x0A63, 0x0e);
    xs9950_write_reg(0x0A64, 0x00);
    xs9950_write_reg(0x0A65, 0xfc);
    xs9950_write_reg(0x0A67, 0xe5);
    xs9950_write_reg(0x0A69, 0xef);
    xs9950_write_reg(0x0A6B, 0x1b);
    xs9950_write_reg(0x0A6D, 0x2f);
    xs9950_write_reg(0x0A6F, 0x00);
    xs9950_write_reg(0x0A71, 0xc2);
    xs9950_write_reg(0x0A72, 0xff);
    xs9950_write_reg(0x0A73, 0xd0);
    xs9950_write_reg(0x0A74, 0xff);
    xs9950_write_reg(0x0A75, 0x29);
    xs9950_write_reg(0x0A77, 0x57);
    xs9950_write_reg(0x0A78, 0x00);
    xs9950_write_reg(0x0A79, 0x10);
    xs9950_write_reg(0x0A7A, 0x00);
    xs9950_write_reg(0x0A7B, 0xaa);
    xs9950_write_reg(0x0A7D, 0xb2);
    xs9950_write_reg(0x0A7F, 0x24);
    xs9950_write_reg(0x0A80, 0x00);
    xs9950_write_reg(0x0A81, 0x69);
    xs9950_write_reg(0x0A82, 0x00);
    xs9950_write_reg(0x0802, 0x2);
    xs9950_write_reg(0x0501, 0x81);
    xs9950_write_reg(0x0502, 0x00);
    xs9950_write_reg(0x0B74, 0xFC);
    xs9950_write_reg(0x01DC, 0x01);
    xs9950_write_reg(0x0804, 0x04);
    xs9950_write_reg(0x4018, 0x01);
    xs9950_write_reg(0x0B56, 0x1);
    xs9950_write_reg(0x0B73, 0x2);
    xs9950_write_reg(0x4210, 0xC);
    xs9950_write_reg(0x420B, 0x2F);
    xs9950_write_reg(0x4030, 0x15);
    xs9950_write_reg(0x4134, 0xa);
    xs9950_write_reg(0x0803, 0xf);
    xs9950_write_reg(0x4412, 0x1);
    rt_thread_mdelay(1);
    xs9950_write_reg(0x0803, 0x1f);
    xs9950_write_reg(0x10E3, 0x04);
    xs9950_write_reg(0x10EB, 0xfd);
    xs9950_write_reg(0x0800, 0x7);
    xs9950_write_reg(0x0805, 0x7);
    rt_thread_mdelay(1);
    xs9950_bt656_init_sk();

    xs9950_bt656_fmt_sk(fmt);
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
    xs9950_write_reg(0x4704, 0x02); // Clock source selection (24MHz)
    xs9950_write_reg(0x4705, 0x01); // Clock division (24MHz/2=12MHz)

    // 3. Channel selection (manual 4.2.1, register 0x4200)
    // 0x4200: bit[2:1]=channel, bit[0]=power (0=on, 1=down)
    // VINA=0x02, VINB=0x00, VINC=0x04, VIND=0x06
    u8 ch_val = 0;
    switch (ch) {
    case VIN1:
        ch_val = 0x02;
        break;
    case VIN2:
        ch_val = 0x00;
        break;
    case VIN3:
        ch_val = 0x04;
        break;
    case VIN4:
        ch_val = 0x06;
        break;
    default:
        ch_val = 0x02;
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
 * @brief XS9950 sensor initialization
 * @param ch Video channel (VIN1~VIN4)
 * @param fmt Video format (std is derived from fmt)
 */
static void xs9950_sensor_init(struct xs9950_dev *sensor, enum tp_vin_ch ch, enum tp_fmt fmt)
{
    enum tp_std std = xs9950_fmt_to_std(fmt);

    sensor->curr_ch = ch;
    sensor->curr_fmt = fmt;
    sensor->curr_std = std;

    // 1. Global reset
    xs9950_write_reg(0x0000, 0x01);    // Software reset
    rt_thread_mdelay(2);
    xs9950_write_reg(0x0000, 0x00);    // Reset release
    rt_thread_mdelay(20);  // Extend reset stabilization time

    // 2. AFE front-end initialization
    xs9950_afe_init(ch);

    // 3. BT656 + format initialization (combined: base + sk + format-specific)
    xs9950_regbt656_sk(ch, fmt, std);

    // 4. Set resolution
    xs9950_set_resolution(fmt);

    // 5. Enable video output
    xs9950_write_reg(0x4000, 0x01); // Global video enable

    // 6. Adjust blank for some special formats
    xs9950_adjust_blank(fmt);

    // 7. Adjust color for some special formats
    xs9950_adjust_color(sensor, fmt);

    // auto detect
    xs9950_write_reg(0x010c, 0x00);
    LOG_I("XS9950 sensor init done: ch=%d, fmt=%d, std=%d", ch, fmt, std);
}

/**
 * @brief Chip ID check
 * @return 0 success, -1 failure
 */
static int xs9950_chipid_check(struct xs9950_dev *sensor)
{
    u8 id_h = 0, id_l = 0;
    id_h = xs9950_read_reg(XS9950_DEVICE_ID_H);      // Chip ID high 8 bits
    id_l = xs9950_read_reg(XS9950_DEVICE_ID_L);      // Chip ID low 8 bits
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
    sensor->irq_pin = camera_irq_pin_get(); // New interrupt pin acquisition
    if (!sensor->irq_pin) {
        LOG_W("Get IRQ pin failed, interrupt disabled");
    }
#endif
    // 3. Default configuration initialization
    fmt->code = DEFAULT_MEDIA_CODE;
    fmt->bus_type = DEFAULT_BUS_TYPE;
    fmt->flags = MEDIA_SIGNAL_FIELD_ACTIVE_HIGH |
                 MEDIA_SIGNAL_VSYNC_ACTIVE_LOW |
                 MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH |
                 MEDIA_SIGNAL_PCLK_SAMPLE_FALLING;

    if ((DEFAULT_FORMAT == CVBS_PAL) || (DEFAULT_FORMAT == CVBS_NTSC)) {
        LOG_I("Interlace mode enabled");
        sensor->fmt.flags &= ~MEDIA_SIGNAL_FIELD_ACTIVE_HIGH;
        sensor->fmt.flags |= MEDIA_SIGNAL_FIELD_ACTIVE_LOW | MEDIA_SIGNAL_INTERLACED_MODE;
    }

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
    xs9950_sensor_init(sensor, DEFAULT_VIN_CH, DEFAULT_FORMAT);

#ifdef XS9950_INTERRUPT
    // 4. Register interrupt (fix: enable interrupt)
    if (sensor->irq_pin) {
        rt_pin_attach_irq(sensor->irq_pin, PIN_IRQ_MODE_FALLING, xs9950_irq_handler, &g_xs_dev);
        rt_pin_irq_enable(sensor->irq_pin, PIN_IRQ_ENABLE);
        LOG_I("IRQ enabled on pin %d", sensor->irq_pin);
    }
#endif
    sensor->streaming = true;

    xs9950_wait_lock();
    xs9950_status(0, NULL);

    LOG_D("XS9950 device open done");
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
        // TODO: MUST not call rt_i2c_transfer() in ISR
        xs9950_write_reg(0xE22, 0x01); // Clear interrupt
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
    case CAMERA_CMD_SET_CONTRAST:
        return xs9950_set_contrast(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_BRIGHTNESS:
        return xs9950_set_brightness(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_HUE:
        return xs9950_set_hue(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_SATURATION:
        return xs9950_set_saturation(sensor, *(u32 *)args);
#ifdef XS9950_INTERRUPT
    case CAMERA_CMD_SET_CH: // Switch video channel
        if (args && *(u8 *)args < 4) {
            xs9950_sensor_init(sensor, *(u8 *)args, sensor->curr_fmt);
            return RT_EOK;
        }
        return -RT_EINVAL;
    case CAMERA_CMD_GET_IRQ_STATUS: // Get interrupt status
        if (args) {
            *(u8 *)args = sensor->irq_status;
            sensor->irq_status = 0; // Clear status
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

#ifdef RT_USING_PM
static int xs9950_pm_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    return RT_EOK;
}

static void xs9950_pm_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct xs9950_dev *sensor = (struct xs9950_dev *)device;

    switch (mode) {
    case PM_SLEEP_MODE_LIGHT:
        break;
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        sensor->pwdn_pin = camera_pwdn_pin_get();
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops xs9950_pm_ops =
{
    SET_DEVICE_PM_OPS(xs9950_pm_suspend, xs9950_pm_resume)
    NULL,
};
#endif /* RT_USING_PM */

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

#ifdef RT_USING_PM
    rt_pm_device_register(&g_xs_dev.dev, &xs9950_pm_ops);
#endif

    LOG_I("XS9950 driver register done");
    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_xs9950_init);
