/*
 * Copyright (c) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * XS9950 4-Channel Video Decoder Register Definitions
 * Reference: XS9950 User Register Manual
 */

#ifndef __XS9950_REG_H__
#define __XS9950_REG_H__

#include "aic_core.h"

/* I2C & device identification */
#define XS9950_I2C_SLAVE_ID 0x30   /* I2C slave address (from manual 4.1.4) */
#define XS9950_CHIP_ID      0x9950 /* Chip ID (custom, needs adjustment based on actual) */
#define XS9950_REG_ADDR_LEN 2      /* I2C register address length (16bit) */
#define XS9950_DEVICE_ID_H  0x40f0
#define XS9950_DEVICE_ID_L  0x40f1

/* ========================================================================
 * XS9950 Status Register Query (Manual Section 2.3.3.2, Offset 0x00~0x0B)
 * All registers in this range are Read-Only (RO).
 * ======================================================================== */

/* Status register offsets */
#define XS9950_REG_VIDEO_STATUS_1       0x00  /* VIDEO_STATUS_REGISTER_1_CHX */
#define XS9950_REG_HD_VIDEO_STD_RB      0x01  /* HD_VIDEO_STANDARD_READBACK_CHX */
#define XS9950_REG_SD_VIDEO_STD_RB      0x02  /* SD_VIDEO_STANDARD_READBACK_CHX */
#define XS9950_REG_VSYNC_STATUS_2       0x03  /* VSYNC_STATUS_REGISTER_2_CHX */
#define XS9950_REG_SIGNAL_LOSS_LSB      0x04  /* SIGNAL_LOSS_FACT_LSB_CHX */
#define XS9950_REG_SIGNAL_LOSS_MSB      0x05  /* SIGNAL_LOSS_FACT_MSB_CHX */
#define XS9950_REG_SYNC_DEPTH_LSB       0x06  /* SYNC_DEPTH_LSB_CHX */
#define XS9950_REG_SYNC_DEPTH_MSB       0x07  /* SYNC_DEPTH_MSB_CHX */
#define XS9950_REG_HD_STATUS            0x08  /* HD_STATUS_CHX */
#define XS9950_REG_SD_STATUS            0x09  /* SD_STATUS_CHX */
#define XS9950_REG_HD_STD               0x0A  /* HD_STD_CHX */
#define XS9950_REG_SD_STD               0x0B  /* SD_STD_CHX */

/* VIDEO_STATUS_REGISTER_1 (0x00) bit definitions */
#define XS9950_STS1_HD_SD              BIT(7) /* 1: HD, 0: SD */
#define XS9950_STS1_SD_BURST_DETECT    BIT(6) /* SD color sub-carrier detected */
#define XS9950_STS1_FREE_RUN           BIT(4) /* 1: no video / non-standard signal */
#define XS9950_STS1_HSPLL_LOCKED_FE    BIT(3) /* Front-end HSYNC PLL locked */
#define XS9950_STS1_HSPLL_LOCKED_BE    BIT(2) /* Back-end HSYNC PLL locked */
#define XS9950_STS1_VSYNC_LOCKED       BIT(1) /* Frame/field sync locked */
#define XS9950_STS1_COLOR_KILL         BIT(0) /* 1: no color (color kill) */

/* HD_STATUS (0x08) bit definitions */
#define XS9950_HD_STS_SYNC_DEPTH_MSB_MASK  0xC0 /* [7:6] HD sync depth high 2 bits */
#define XS9950_HD_STS_FREE_RUN             BIT(4) /* 1: no video / non-standard */
#define XS9950_HD_STS_HSPLL_LOCKED_FE      BIT(3) /* Front-end HSYNC PLL locked */
#define XS9950_HD_STS_HSPLL_LOCKED_BE      BIT(2) /* Back-end HSYNC PLL locked */
#define XS9950_HD_STS_VSYNC_LOCKED         BIT(1) /* Frame sync locked */
#define XS9950_HD_STS_COLOR_KILL           BIT(0) /* 1: no color */

/* SD_STATUS (0x09) bit definitions */
#define XS9950_SD_STS_SYNC_DEPTH_MSB_MASK  0xC0 /* [7:6] SD sync depth high 2 bits */
#define XS9950_SD_STS_FREE_RUN             BIT(4) /* 1: no video / non-standard */
#define XS9950_SD_STS_HSPLL_LOCKED_FE      BIT(3) /* Front-end HSYNC PLL locked */
#define XS9950_SD_STS_HSPLL_LOCKED_BE      BIT(2) /* Back-end HSYNC PLL locked */
#define XS9950_SD_STS_VSYNC_LOCKED         BIT(1) /* Frame sync locked */
#define XS9950_SD_STS_COLOR_KILL           BIT(0) /* 1: no color */

/* VSYNC_STATUS_REGISTER_2 (0x03) bit definitions */
#define XS9950_VSYNC2_PEDESTAL          BIT(1) /* 1: has pedestal level */
#define XS9950_VSYNC2_FIFTY_HZ          BIT(0) /* 1: 50Hz, 0: 60Hz (SD mode) */

/* HD_VIDEO_STANDARD_READBACK (0x01) - Bit[7:6]: standard type */
#define XS9950_HD_STD_TYPE_MASK        0xC0
#define XS9950_HD_STD_TYPE_HDCVI       0x00  /* 00: HDCVI */
#define XS9950_HD_STD_TYPE_ASTD        0x40  /* 01: AHD */
#define XS9950_HD_STD_TYPE_TSTD        0x80  /* 10: TVI */
#define XS9950_HD_STD_TYPE_RSVD        0xC0  /* 11: reserved */
#define XS9950_HD_STD_FORMAT_MASK      0x3F  /* Bit[5:0]: video format */
#define XS9950_HD_STD_NO_SIGNAL        0xFF  /* No HD video input */

/* SD_VIDEO_STANDARD_READBACK (0x02) - Bit[3:0] */
#define XS9950_SD_STD_MASK             0x0F
#define XS9950_SD_STD_NO_SIGNAL        0x0F  /* No SD video input */
#define XS9950_SD_STD_NTSC_JM          0x00
#define XS9950_SD_STD_NTSC_443         0x01
#define XS9950_SD_STD_PAL_M            0x02
#define XS9950_SD_STD_PAL_60           0x03
#define XS9950_SD_STD_PAL_CN           0x04
#define XS9950_SD_STD_PAL_BGHID        0x05

/**
 * @brief XS9950 channel status info (registers 0x00~0x0B)
 */
struct xs9950_ch_status {
    /* 0x00: VIDEO_STATUS_REGISTER_1 */
    u8 is_hd;              /* 1: HD, 0: SD */
    u8 sd_burst_detected;  /* SD color sub-carrier detected */
    u8 free_run;           /* 1: no video or non-standard signal */
    u8 hspll_locked_fe;    /* Front-end HSYNC PLL locked */
    u8 hspll_locked_be;    /* Back-end HSYNC PLL locked */
    u8 vsync_locked;       /* Frame/field sync locked */
    u8 color_kill;         /* 1: no color (color kill) */

    /* 0x01: HD_VIDEO_STANDARD_READBACK */
    u8 hd_std_raw;         /* Raw register value (0xFF = no HD signal) */
    u8 hd_std_type;        /* Bit[7:6]: 00=HDCVI, 01=AHD, 10=TVI */
    u8 hd_std_format;      /* Bit[5:0]: video format code */

    /* 0x02: SD_VIDEO_STANDARD_READBACK */
    u8 sd_std_raw;         /* Raw register value (0x0F = no SD signal) */

    /* 0x03: VSYNC_STATUS_REGISTER_2 */
    u8 has_pedestal;       /* 1: has pedestal level */
    u8 is_fifty_hz;        /* 1: 50Hz, 0: 60Hz (SD mode) */

    /* 0x04~0x05: SIGNAL_LOSS_FACT */
    u16 signal_loss;       /* Channel signal attenuation (16-bit) */

    /* 0x06~0x07: SYNC_DEPTH */
    u16 sync_depth;        /* Sync level depth (10-bit: [1:0]<<8 | [7:0]) */

    /* 0x08: HD_STATUS */
    u16 hd_sync_depth;     /* HD sync depth (10-bit: [7:6]<<8 | 0x0C) */
    u8 hd_free_run;        /* HD: no video signal */
    u8 hd_hspll_locked_fe; /* HD front-end PLL locked */
    u8 hd_hspll_locked_be; /* HD back-end PLL locked */
    u8 hd_vsync_locked;    /* HD frame sync locked */
    u8 hd_color_kill;      /* HD no color */

    /* 0x09: SD_STATUS */
    u16 sd_sync_depth;     /* SD sync depth (10-bit: [7:6]<<8 | 0x0D) */
    u8 sd_free_run;        /* SD: no video signal */
    u8 sd_hspll_locked_fe; /* SD front-end PLL locked */
    u8 sd_hspll_locked_be; /* SD back-end PLL locked */
    u8 sd_vsync_locked;    /* SD frame sync locked */
    u8 sd_color_kill;      /* SD no color */

    /* 0x0A: HD_STD */
    u8 hd_std_debug;       /* HD standard debug value (same format as 0x01) */

    /* 0x0B: SD_STD */
    u8 sd_std_debug;       /* SD standard debug value (same format as 0x02) */
};

#endif /* __XS9950_REG_H__ */
