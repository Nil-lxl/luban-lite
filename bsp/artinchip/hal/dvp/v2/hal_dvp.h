/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: ArtInChip <ArtInChip@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_DVP_H_
#define _ARTINCHIP_HAL_DVP_H_

#include "mpp_types.h"

#define DVP_MAX_CH_NUM		    2
#define DVP_PLANE_NUM           2
#define DVP_DEMUX_CH0_ID        0
#define DVP_DEMUX_CH1_ID        1
#define DVP_MAX_HEIGHT          4096U
#define DVP_MAX_WIDTH           4096U
#define DVP_SFIELD_MODE

#define DVP_CTL                         0x000
#define DVP_CH_REF_ID                   0x004
#define DVP_CAP_CTL                     0x008
#define DVP_CH_IRQ_STA                  0x00C

#define DVP_DLL_PHA_CFG2                0x034
#define DVP_QOS_CFG                     0x1B8
#define DVP_AXI_CFG                     0x1BC

#define DVP_CH_BASE(ch)                 (0x100 * (ch))

#define DVP_IRQ_EN(ch)                  (DVP_CH_BASE(ch) + 0x100)
#define DVP_IRQ_STA(ch)                 (DVP_CH_BASE(ch) + 0x104)
#define DVP_IRQ_CFG(ch)                 (DVP_CH_BASE(ch) + 0x108)
#define DVP_IN_CFG(ch)                  (DVP_CH_BASE(ch) + 0x10C)
#define DVP_IN_HOR_SIZE(ch)             (DVP_CH_BASE(ch) + 0x110)
#define DVP_IN_VER_SIZE(ch)             (DVP_CH_BASE(ch) + 0x114)
#define DVP_OUT_HOR_SIZE(ch)            (DVP_CH_BASE(ch) + 0x120)
#define DVP_OUT_VER_SIZE(ch)            (DVP_CH_BASE(ch) + 0x128)
#define DVP_OUT_FRA_NUM(ch)             (DVP_CH_BASE(ch) + 0x130)
#define DVP_OUT_CUR_FRA(ch)             (DVP_CH_BASE(ch) + 0x134)
#define DVP_OUT_CTL(ch)                 (DVP_CH_BASE(ch) + 0x138)
#define DVP_OUT_UPDATE_CTL(ch)          (DVP_CH_BASE(ch) + 0x13C)
#define DVP_OUT_ADDR_BUF0(ch)           (DVP_CH_BASE(ch) + 0x140)
#define DVP_OUT_ADDR_BUF1(ch)           (DVP_CH_BASE(ch) + 0x144)
#define DVP_OUT_READ_ADDR0(ch)          (DVP_CH_BASE(ch) + 0x148)
#define DVP_OUT_READ_ADDR1(ch)          (DVP_CH_BASE(ch) + 0x14C)
#define DVP_OUT_LINE_STRIDE0(ch)        (DVP_CH_BASE(ch) + 0x150)
#define DVP_OUT_LINE_STRIDE1(ch)        (DVP_CH_BASE(ch) + 0x154)
#define DVP_OUT_ADDR_BUF0_SHA(ch)       (DVP_CH_BASE(ch) + 0x158)
#define DVP_OUT_ADDR_BUF1_SHA(ch)       (DVP_CH_BASE(ch) + 0x15C)
#define DVP_OUT_LINE_STRIDE_SHA(ch)     (DVP_CH_BASE(ch) + 0x160)

#define DVP_HIST_RESULT(ch, n)          (DVP_CH_BASE(ch) + 0x1C0 + (n) * 4)

#define DVP_DEBUG_SEL               0xFF0
#define DVP_VER                     0xFFC

#define DVP_CH_IRQ_ALL_PENDING      0x3

#define DVP_CTL_AXI_INTI_THR(v)     ((v) << 24)
#define DVP_CTL_AXI_INTI_THR_MASK   GENMASK(31, 24)
#define DVP_CTL_AXI_RESP_EN         BIT(23)
#define DVP_CTL_IN_CLK_POL          BIT(22)
#define DVP_CTL_CAPTURE_IFULL_EN    BIT(21)
#define DVP_CTL_CLR_MODE            BIT(19)
#define DVP_CTL_CLK_GATE            BIT(18)
#define DVP_CTL_HIST_EN             BIT(17)
#define DVP_CTL_DEMUX_EN            BIT(16)
#define DVP_CTL_OUT_FMT(v)          ((v) << 12)
#define DVP_CTL_OUT_FMT_MASK        GENMASK(14, 12)
#define DVP_CTL_DDR_EN              BIT(11)
#define DVP_CTL_RAW_IN_SEQ          BIT(10)
#define DVP_CTL_IN_SEQ(v)           ((v) << 8)
#define DVP_CTL_IN_SEQ_MASK         GENMASK(9, 8)
#define DVP_CTL_IN_FMT(v)           ((v) << 4)
#define DVP_CTL_IN_FMT_MASK         GENMASK(6, 4)
#define DVP_CTL_VIDEO_CONTINUE_EN   BIT(3)
#define DVP_CTL_DROP_FRAME_EN       BIT(2)
#define DVP_CTL_CLR                 BIT(1)
#define DVP_CTL_EN                  BIT(0)

#define DVP_CH_REF_ID1_INDEX        BIT(16)
#define DVP_CH_REF_ID0_INDEX        BIT(12)
#define DVP_CH_REF_ID1_CFG(v)       ((v) << 8)
#define DVP_CH_REF_ID1_MASK         GENMASK(11, 8)
#define DVP_CH_REF_ID0_CFG(v)       ((v) << 4)
#define DVP_CH_REF_ID0_MASK         GENMASK(7, 4)
#define DVP_CH_REF_ID_EN            BIT(0)

#define DVP_CAP_CTL_PATH_SEL        BIT(0)
#define DVP_CAP_CTL_CHAIN_SEL(v)    ((v) << 4)
#define DVP_CAP_CTL_CHAIN_SEL_MASK  GENMASK(8, 4)

#define DVP_CH1_IRQ_STA             BIT(1)
#define DVP_CH0_IRQ_STA             BIT(0)

#define DVP_IRQ_EN_UPDATE_DONE      BIT(7)
#define DVP_IRQ_EN_XY_CODE_ERR      BIT(6)
#define DVP_IRQ_EN_IN_VER_CHG       BIT(5)
#define DVP_IRQ_EN_IN_HOR_CHG       BIT(4)
#define DVP_IRQ_EN_BUF_FULL         BIT(3)
#define DVP_IRQ_EN_HNUM             BIT(2)
#define DVP_IRQ_EN_FRAME_DONE       BIT(1)
#define DVP_IRQ_EN_CAP_DONE         BIT(0)

#define DVP_IRQ_STA_CLOSE_DONE      BIT(8)
#define DVP_IRQ_STA_UPDATE_DONE     BIT(7)
#define DVP_IRQ_STA_XY_CODE_ERR     BIT(6)
#define DVP_IRQ_STA_IN_VER_CHG      BIT(5)
#define DVP_IRQ_STA_IN_HOR_CHG      BIT(4)
#define DVP_IRQ_STA_BUF_FULL        BIT(3)
#define DVP_IRQ_STA_HNUM            BIT(2)
#define DVP_IRQ_STA_FRAME_DONE      BIT(1)
#define DVP_IRQ_STA_CAP_DONE        BIT(0)

#define DVP_IRQ_CFG_HNUM_MASK       GENMASK(30, 16)
#define DVP_IRQ_CFG_HNUM_SHIFT      16

#define DVP_IN_CFG_FILED_POL_ACTIVE_LOW         BIT(3)
#define DVP_IN_CFG_VSYNC_POL_FALLING            BIT(2)
#define DVP_IN_CFG_HREF_POL_ACTIVE_HIGH         BIT(1)
#define DVP_IN_CFG_PCLK_POL_FALLING             BIT(0)

/* The field definition of IN_HOR_SIZE */
#define DVP_IN_HOR_SIZE_IN_HOR_MASK         GENMASK(30, 16)
#define DVP_IN_HOR_SIZE_IN_HOR_SHIFT        (16)
#define DVP_IN_HOR_SIZE_XY_CODE_ERR_MASK    GENMASK(15, 8)
#define DVP_IN_HOR_SIZE_XY_CODE_ERR_SHIFT   (8)
#define DVP_IN_HOR_SIZE_XY_CODE_MASK        GENMASK(7, 0)
#define DVP_IN_HOR_SIZE_XY_CODE_SHIFT       (0)
#define DVP_IN_HOR_SIZE_XY_CODE_F           BIT(6)

/* The field definition of DVP_IN_VER_SIZE */
#define DVP_IN_VER_SIZE_IN_VER_MASK             GENMASK(30, 16)
#define DVP_IN_VER_SIZE_IN_VER_SHIFT            (16)
#define DVP_IN_VER_SIZE_CURR_FILED              BIT(15)
#define DVP_IN_VER_SIZE_CURR_VER_MASK           GENMASK(14, 0)
#define DVP_IN_VER_SIZE_CURR_VER_SHIFT          (0)

#define DVP_OUT_HOR_NUM_RAW(w)          (((w) - 1) << 16)
#define DVP_OUT_HOR_BEG_RAW(x)          (ALIGN_DOWN((x), 2))
#define DVP_OUT_HOR_NUM(w)              (((w) * 2 - 1) << 16)
#define DVP_OUT_HOR_BEG(x)              (ALIGN_DOWN((x), 2) * 2)

#define DVP_OUT_VER_NUM(h)              (((h) - 1) << 16)
#define DVP_OUT_VER_BEG(y)              (y)

#define DVP_OUT_ADDR_BUF(ch, plane)	(plane ? DVP_OUT_ADDR_BUF1(ch) \
						: DVP_OUT_ADDR_BUF0(ch))

#define DVP_OUT_FRA_NUM_MASK            GENMASK(13, 0)

#define DVP_OUT_CTL_CAP_OFF_IMMEDIATELY BIT(1)
#define DVP_OUT_CTL_CAP_ON              BIT(0)

#define DVP_QOS_CUSTOM              BIT(26)
#define DVP_QOS_INC_THR_MASK        GENMASK(25, 17)
#define DVP_QOS_INC_THR_SHIFT       17
#define DVP_QOS_DEC_THR_MASK        GENMASK(16, 8)
#define DVP_QOS_DEC_THR_SHIFT       8
#define DVP_QOS_HIGH_MASK           GENMASK(7, 4)
#define DVP_QOS_HIGH_SHIFT          4
#define DVP_QOS_LOW_MASK            GENMASK(3, 0)

enum dvp_input {
    DVP_IN_RAW      = 0,
    DVP_IN_YUV422   = 1,
    DVP_IN_BT656    = 2,
};

enum dvp_output {
    DVP_OUT_RAW_PASSTHROUGH         = 0,
    DVP_OUT_YUV422_COMBINED_NV16    = 1,
    DVP_OUT_YUV420_COMBINED_NV12    = 2,
    DVP_OUT_Y_ONLY                  = 3,
};

enum dvp_input_yuv_seq {
    DVP_YUV_DATA_SEQ_YUYV   = 0,
    DVP_YUV_DATA_SEQ_YVYU   = 1,
    DVP_YUV_DATA_SEQ_UYVY   = 2,
    DVP_YUV_DATA_SEQ_VYUY   = 3,
};

enum dvp_capture_mode {
    DVP_CAPTURE_PICTURE = 0,
    DVP_CAPTURE_VIDEO = 1
};

enum dvp_subdev_pads {
    DVP_SUBDEV_SINK = 0,
    DVP_SUBDEV_SOURCE,
    DVP_SUBDEV_PAD_NUM,
};

/* Save the configuration information for DVP controller. */
struct aic_dvp_config {
    /* Input format */
    enum dvp_input          input;
    enum dvp_input_yuv_seq  input_seq;
    u32                     flags;
    u32                     interlaced;
    u32                     mux;

    /* Output format */
    enum dvp_output output;
    u32             width;
    u32             height;
    u32             crop_x;
    u32             crop_y;
    u32             stride[DVP_PLANE_NUM];
    u32             sizeimage[DVP_PLANE_NUM];

    /* Stitch mode configuration */
    enum mpp_stitch_mode stitch_mode;
};

/* Some API of register, Defined in hal_dvp.c */
void hal_dvp_enable(struct aic_dvp_config *cfg, int enable);
void hal_dvp_demux_en(int enable);
void hal_dvp_clr_mode(void);
void hal_dvp_hist_en(void);
void hal_dvp_ch_id_cfg(u32 id0, u32 id1);
u32 hal_dvp_ch_irq_sta_get(void);
u32 hal_dvp_irq_sta_get(u32 ch);

void hal_dvp_capture_start(u32 ch);
void hal_dvp_capture_stop(u32 ch);
void hal_dvp_clr_fifo(void);
int  hal_dvp_clr_int(u32 ch);
bool hal_dvp_int_is_enabled(u32 ch);
void hal_dvp_enable_int(struct aic_dvp_config *cfg, u32 ch, int enable);
void hal_dvp_set_pol(u32 flags, u32 ch);
void hal_dvp_set_cfg(struct aic_dvp_config *cfg, u32 ch);
void hal_dvp_update_buf_addr(dma_addr_t y, dma_addr_t uv, u32 ch, u32 y_offset, u32 uv_offset);
void hal_dvp_update_ctl(u32 ch);
void hal_dvp_record_mode(u32 ch);
void hal_dvp_qos_cfg(u32 high, u32 low, u32 inc_thd, u32 dec_thd);

u32 hal_dvp_get_current_xy(u32 ch);
u32 hal_dvp_is_top_field(u32 ch);
u32 hal_dvp_is_bottom_field(u32 ch);
void hal_dvp_field_tag_clr(u32 ch);
void hal_dvp_set_frame_offset(u32 num, u32 ch);

#endif /* _ARTINCHIP_HAL_DVP_V2_H_ */
