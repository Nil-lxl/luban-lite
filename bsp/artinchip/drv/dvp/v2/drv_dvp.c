/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: ArtInChip <ArtInChip@artinchip.com>
 */

#include <string.h>

#include "aic_core.h"
#include "aic_list.h"
#include "aic_hal_clk.h"
#include "mpp_types.h"

#include "hal_dvp.h"
#include "drv_dvp.h"

#define DVP_CH0 0
#define DVP_CH1 1

#define DVP_CH_INDEX_0 0
#define DVP_CH_INDEX_1 1

#define SINGLE_CHANNEL  0
#define DUAL_CHANNEL    1

#define DVP_FIRST_BUF       0
#define BUF_IS_INVALID(index)   (((index) < 0) || ((index) >= VIN_MAX_BUF_NUM))

static struct aic_dvp g_dvp = {0};

static const struct {
    u32 fmt;
    enum dvp_input_yuv_seq dvp;
} aic_dvp_in_fmt[] = {
    {MEDIA_BUS_FMT_Y8_1X8,    0},
    {MEDIA_BUS_FMT_YUYV8_2X8, DVP_YUV_DATA_SEQ_YUYV},
    {MEDIA_BUS_FMT_YVYU8_2X8, DVP_YUV_DATA_SEQ_YVYU},
    {MEDIA_BUS_FMT_UYVY8_2X8, DVP_YUV_DATA_SEQ_UYVY},
    {MEDIA_BUS_FMT_VYUY8_2X8, DVP_YUV_DATA_SEQ_VYUY},
};

static const struct {
    enum mpp_pixel_format pixelformat;
    enum dvp_output dvp;
} aic_dvp_out_fmt[] = {
    {MPP_FMT_NV16, DVP_OUT_YUV422_COMBINED_NV16},
    {MPP_FMT_NV12, DVP_OUT_YUV420_COMBINED_NV12},
    {MPP_FMT_YUV400, DVP_OUT_Y_ONLY}
};

static int aic_dvp_out_fmt_valid(u32 pixelformat)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(aic_dvp_out_fmt); i++) {
        if (aic_dvp_out_fmt[i].pixelformat == pixelformat)
            return aic_dvp_out_fmt[i].dvp;
    }
    pr_err("Invalid pixelformat: 0x%x\n", pixelformat);
    return -1;
}

static int aic_dvp_in_fmt_valid(u32 fmt)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(aic_dvp_in_fmt); i++) {
        if (aic_dvp_in_fmt[i].fmt == fmt)
            return aic_dvp_in_fmt[i].dvp;
    }

    pr_err("Invalid input format: 0x%x\n", fmt);
    return -1;
}

int aic_dvp_set_in_fmt(struct mpp_video_fmt *fmt)
{
    int ret = 0;
    struct aic_dvp_config *cfg = &g_dvp.cfg;

    ret = aic_dvp_in_fmt_valid(fmt->code);
    if (ret < 0)
        return -EINVAL;
    cfg->input_seq = (enum dvp_input_yuv_seq)ret;

    if (fmt->bus_type == MEDIA_BUS_BT656)
        cfg->input = DVP_IN_BT656;
    else if (fmt->bus_type == MEDIA_BUS_PARALLEL)
        cfg->input = DVP_IN_YUV422;
    else
        cfg->input = DVP_IN_RAW;

#ifdef AIC_USING_CAMERA_OV5640
    /* Should inverse the HSYNC signal of OV5640 */
    if (fmt->flags & MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH)
        cfg->flags = (fmt->flags & ~MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH)
                        | MEDIA_SIGNAL_HSYNC_ACTIVE_LOW;
    else
        cfg->flags = (fmt->flags & ~MEDIA_SIGNAL_HSYNC_ACTIVE_LOW)
                        | MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH;
#else
    cfg->flags = fmt->flags;
#endif
    cfg->mux = fmt->mux;

    if (fmt->flags & MEDIA_SIGNAL_INTERLACED_MODE)
        cfg->interlaced = 1;

    return 0;
}

static void _aic_dvp_try_fmt(struct dvp_out_fmt *pix)
{
    int ret;
    unsigned int i;

    ret = aic_dvp_out_fmt_valid(pix->pixelformat);
    if (ret < 0)
        return;

    pix->num_planes = DVP_PLANE_NUM;
    for (i = 0; i < DVP_PLANE_NUM; i++) {
        pix->plane_fmt[i].bytesperline = ALIGN_UP(pix->width, 8);
        pix->plane_fmt[i].sizeimage = ALIGN_UP(pix->plane_fmt[i].bytesperline * pix->height, 8);

        if (i > 0) {
            if (ret == DVP_OUT_YUV420_COMBINED_NV12) {
                pix->plane_fmt[i].sizeimage >>= 1;
            } else if ((g_dvp.cfg.input == DVP_IN_RAW) || (ret == DVP_OUT_Y_ONLY)) {
                pix->plane_fmt[i].bytesperline = 0;
                pix->plane_fmt[i].sizeimage = 0;
            }
        }
    }
}

int aic_dvp_set_out_fmt(struct dvp_out_fmt *fmt)
{
    int i, ret = 0;

    _aic_dvp_try_fmt(fmt);
    g_dvp.fmt = *fmt;

    /* Save the configuration for DVP controller */
    ret = aic_dvp_out_fmt_valid(g_dvp.fmt.pixelformat);
    if (ret < 0)
        return -1;
    g_dvp.cfg.output = ret;
    g_dvp.cfg.width = g_dvp.fmt.width;
    g_dvp.cfg.height = g_dvp.fmt.height;
    g_dvp.cfg.crop_x = g_dvp.fmt.crop_x;
    g_dvp.cfg.crop_y = g_dvp.fmt.crop_y;
    g_dvp.cfg.stitch_mode = fmt->stitch_mode;

    for (i = 0; i < DVP_PLANE_NUM; i++) {
        g_dvp.cfg.stride[i] = g_dvp.fmt.plane_fmt[i].bytesperline;
        g_dvp.cfg.sizeimage[i] = g_dvp.fmt.plane_fmt[i].sizeimage;
    }
    return 0;
}

int aic_dvp_stream_on(u32 ch)
{
    return vin_vb_stream_on(&g_dvp.ch[ch].queue);
}

int aic_dvp_stream_off(u32 ch)
{
    int ret = 0;

    ret = vin_vb_stream_off(&g_dvp.ch[ch].queue);

    INIT_LIST_HEAD(&g_dvp.ch[ch].active_list);

    return ret;
}

void aic_dvp_stream_pause(u32 ch)
{
    hal_dvp_enable_int(&g_dvp.cfg, ch, 0);
}

void aic_dvp_stream_resume(u32 ch)
{
    hal_dvp_clr_fifo();
    hal_dvp_clr_int(ch);
    g_dvp.ch[ch].resumed = true;
    hal_dvp_enable_int(&g_dvp.cfg, ch, 1);
}

#define PLANE_NUM       vbuf->num_planes
#define VBUF_NUM        vbuf->num_buffers
#define PLANE_S_SIZE    sizeof(struct vin_video_plane)
#define VBUF_S_SIZE     (PLANE_S_SIZE * vbuf->num_planes)

/* Split the plane for stitch mode */
static void aic_dvp_split_plane(struct vin_video_buf *vbuf,
                                enum mpp_stitch_mode stitch,
                                struct aic_dvp_config *cfg)
{
    struct vin_video_plane planes1[VIN_MAX_BUF_NUM] = {0};
    int i;

    /**
    * Buffer layout for stitch mode:
    *
    *    MPP_STITCH_V_MODE              MPP_STITCH_H_MODE
    *    +-----------+                  +-----------+-----------+
    *    | CH0       |                  | CH0       | CH1       | Y
    *    +-----------+ Y                +-----------+-----------+
    *    | CH1       |                  | CH0       | CH1       | UV
    *    +-----------+                  +-----------+-----------+
    *    | CH0       |
    *    +-----------+ UV
    *    | CH1       |
    *    +-----------+
    */

    if (stitch == MPP_STITCH_V_MODE) {
        for (i = 0; i < PLANE_NUM * VBUF_NUM; i++) {
            planes1[i].buf = vbuf->planes[i].buf + vbuf->planes[i].len;
            planes1[i].len = vbuf->planes[i].len;
        }
    }

    if (stitch == MPP_STITCH_H_MODE) {
        for (i = 0; i < PLANE_NUM * VBUF_NUM; i++) {
            planes1[i].buf = vbuf->planes[i].buf + cfg->width;
            planes1[i].len = vbuf->planes[i].len;
        }
    }
    memcpy(&vbuf->planes[PLANE_NUM * VBUF_NUM], planes1, VBUF_S_SIZE * VBUF_NUM);
}

#ifdef AIC_DVP_NO_SIGNAL_PATTERN
static void dvp_fill_no_signal_pattern(struct vin_video_buf *vbuf,
                                       struct aic_dvp_config *cfg, bool stitch)
{
    u32 nf = vbuf->num_buffers;
    u32 width  = cfg->width;
    u32 height = cfg->height;
    u32 block_height = 100;
    u32 r, c, i, p;
    u8 *y, *uv;

    if (aic_dvp_sfield_mode())
        block_height /= 2;

    if (stitch)
        nf *= 2;

    for (i = 0; i < nf; i++) {
        /* Y plane: checkerboard */
        y = (u8 *)(ptr_t)vbuf->planes[i * vbuf->num_planes].buf;
        if (y && vbuf->planes[i * vbuf->num_planes].len) {
            for (r = 0; r < height; r++)
                for (c = 0; c < width; c++)
                    y[r * width + c] = ((r / block_height + c / 100) & 1) ? 0x1C : 0xA1;
            aicos_dcache_clean_invalid_range(y, vbuf->planes[i * vbuf->num_planes].len);
        }

        /* UV plane: neutral gray (0x80) */
        for (p = 1; p < vbuf->num_planes; p++) {
            uv = (u8 *)(ptr_t)vbuf->planes[i * vbuf->num_planes + p].buf;
            if (uv && vbuf->planes[i * vbuf->num_planes + p].len) {
                memset(uv, 0x80, vbuf->planes[i * vbuf->num_planes + p].len);
                aicos_dcache_clean_invalid_range(uv, vbuf->planes[i * vbuf->num_planes + p].len);
            }
        }
    }
}
#endif

int aic_dvp_req_buf(char *buf, u32 size, struct vin_video_buf *vbuf, u32 ch)
{
    struct aic_dvp_config *cfg = &g_dvp.cfg;
    enum mpp_stitch_mode stitch = cfg->stitch_mode;
    int i, ret = 0;

    if (!vbuf || !buf || size == 0) {
        pr_err("[DVP%d] Invalid parameter\n", ch);
        return -1;
    }

    vbuf->num_planes = DVP_PLANE_NUM;
    for (i = 0; i < PLANE_NUM; i++)
        vbuf->planes[i].len = cfg->sizeimage[i];

    if (MPP_IS_STITCH(stitch)) {
        if (ch > 0)
            return vin_vb_req_stitch_buf(&g_dvp.ch[ch].queue, vbuf);

        /* Request the buffer with the double size */
        for (i = 0; i < PLANE_NUM; i++)
            vbuf->planes[i].len *= 2;
    }

    ret = vin_vb_req_buf(&g_dvp.ch[ch].queue, buf, size, vbuf);
    if (ret < 0)
        return ret;

    if (MPP_IS_STITCH(stitch)) { /* It's surely CH0 */
        /* Revert the size of all planes */
        for (i = 0; i < PLANE_NUM * VBUF_NUM; i++)
            vbuf->planes[i].len /= 2;

        aic_dvp_split_plane(vbuf, stitch, cfg);
    }

#ifdef AIC_DVP_NO_SIGNAL_PATTERN
    dvp_fill_no_signal_pattern(vbuf, cfg, MPP_IS_STITCH(stitch));
#endif

    return ret;
}

int aic_dvp_q_buf(u32 index, u32 ch)
{
    if (index >= g_dvp.ch[ch].queue.num_buffers) {
        pr_err("[DVP%d] Invalid index %d\n", ch, index);
        return -EINVAL;
    }

    return vin_vb_q_buf(&g_dvp.ch[ch].queue, index);
}

int aic_dvp_dq_buf(u32 *pindex, u32 ch)
{
    if (pindex == NULL) {
        pr_err("[DVP%d] Invalid parameter\n", ch);
        return -EINVAL;
    }

    return vin_vb_dq_buf(&g_dvp.ch[ch].queue, pindex);
}

/* Return: 0, error; > 0, the elapse time in ms unit */
u32 aic_dvp_get_timestamp(u32 index, u32 ch)
{
    if (index >= g_dvp.ch[ch].queue.num_buffers) {
        pr_err("[DVP%d] Invalid index out of range: %d\n", ch, index);
        return 0;
    }

    return vin_vb_get_timestamp(&g_dvp.ch[ch].queue, index);
}

static int aic_dvp_buf_reload(struct aic_dvp *dvp, struct vb_buffer *buf, u32 ch)
{
    buf->hw_using = 1;
    pr_debug("[DVP%d] Set buf%d 0x%x-0x%x to register\n", ch, buf->index,
             (long)buf->planes[0].buf, (long)buf->planes[1].buf);

    if (buf->planes[1].length)
        hal_dvp_update_buf_addr(buf->planes[0].buf, buf->planes[1].buf, ch, 0, 0);
    else
        hal_dvp_update_buf_addr(buf->planes[0].buf, 0, ch, 0, 0);
    return 0;
}

static void aic_dvp_buf_mark_done(struct aic_dvp *dvp,
                                  struct vb_buffer *vb,
                                  unsigned int sequence, u32 err)
{
    if (err)
        vin_vb_buffer_done(vb, VB_BUF_STATE_ERROR);
    else
        vin_vb_buffer_done(vb, VB_BUF_STATE_DONE);
    vb->hw_using = 0;
}

static int aic_dvp_top_field_done(struct aic_dvp *dvp, u32 err, u32 ch)
{
    struct aic_dvp_ch *dvp_ch = &g_dvp.ch[ch];
    struct vb_buffer *cur_buf = NULL;

    dvp_ch->update_cnt++;
    if (list_empty(&dvp_ch->active_list)) {
        pr_err("[DVP%d] No buf available!\n", ch);
        return 0;
    }

    cur_buf = list_first_entry(&dvp_ch->active_list, struct vb_buffer, active_entry);
    pr_debug("[DVP%d] cur: index %d, dvp_using %d\n", ch,
             cur_buf->index, cur_buf->hw_using);
    if (BUF_IS_INVALID(cur_buf->index)) {
        pr_err("[DVP%d] Invalid buf %d\n", ch, cur_buf->index);
        return -1;
    }

    pr_debug("[DVP%d] Add offset %d of cur buf %d\n", ch, dvp->cfg.stride[0], cur_buf->index);

#ifdef DVP_SFIELD_MODE
    hal_dvp_update_buf_addr(cur_buf->planes[0].buf, cur_buf->planes[1].buf, ch,
                            dvp->cfg.sizeimage[0] / 2, dvp->cfg.sizeimage[1] / 2);
#else
    hal_dvp_update_buf_addr(cur_buf->planes[0].buf, cur_buf->planes[1].buf, ch,
                            dvp->cfg.stride[0], dvp->cfg.stride[0]);
#endif
    dvp_ch->sequence++;
    return 0;
}

static int aic_dvp_frame_done(struct aic_dvp *dvp, int err, u32 ch)
{
    static bool need_skip[DVP_MAX_CH_NUM] = {false};
    struct aic_dvp_ch *dvp_ch = &g_dvp.ch[ch];
    struct vb_buffer *cur_buf = NULL;

    dvp_ch->frame_cnt++;
    if (need_skip[ch]) {
        need_skip[ch] = false;
        return 0;
    }

    if (list_empty(&dvp_ch->active_list)) {
#ifndef AIC_DVP_IGNORE_LOSS
        pr_err("No buf available!\n");
#endif
        return 0;
    }

    cur_buf = list_first_entry(&dvp_ch->active_list, struct vb_buffer, active_entry);
    pr_debug("[DVP%d] cur: index %d, hw_using %d, err %d\n\n", ch,
             cur_buf->index, cur_buf->hw_using, err);
    if (BUF_IS_INVALID(cur_buf->index)) {
        pr_err("[DVP%d] Invalid buf %d\n", ch, cur_buf->index);
        return -1;
    }

    /* If cur_buf is a new one queued, DVP should use it first */
    if (!cur_buf->hw_using) {
        pr_debug("[DVP%d] Buf %d is free just now\n", ch, cur_buf->index);
        aic_dvp_buf_reload(dvp, cur_buf, ch);
        dvp_ch->sequence++;
        need_skip[ch] = true;
        return 0;
    }

    /* Release the current buffer from DVP driver */
    list_del(&cur_buf->active_entry);
    aic_dvp_buf_mark_done(dvp, cur_buf, dvp_ch->sequence, err);

    return 0;
}

static int aic_dvp_update_addr(struct aic_dvp *dvp, u32 ch)
{
    struct aic_dvp_ch *dvp_ch = &g_dvp.ch[ch];
    struct vb_buffer *cur_buf;
    struct vb_buffer *next_buf;

    dvp_ch->update_cnt++;
    if (!dvp_ch->streaming)
        return 0;

    if (list_empty(&dvp_ch->active_list)) {
#ifndef AIC_DVP_IGNORE_LOSS
        pr_warn("[DVP%d] No buf available!\n", ch);
#endif
        return -1;
    }

    cur_buf = list_first_entry(&dvp_ch->active_list, struct vb_buffer, active_entry);
    pr_debug("[DVP%d] cur: index %d, hw_using %d\n", ch, cur_buf->index, cur_buf->hw_using);
    if (BUF_IS_INVALID(cur_buf->index)) {
        pr_err("[DVP%d] Cur buf %d is invalid\n", ch, cur_buf->index);
        return -1;
    }

    if (cur_buf == list_last_entry(&dvp_ch->active_list, struct vb_buffer,
                                   active_entry)) {
#ifndef AIC_DVP_IGNORE_LOSS
        pr_warn("[DVP%d] It's the last buf!\n", ch);
#endif
        return 0;
    }

    next_buf = list_next_entry(cur_buf, active_entry);
    if (!next_buf || BUF_IS_INVALID(next_buf->index)) {
        pr_err("[DVP%d] Next buf is invalid\n", ch);
        return -1;
    }
    pr_debug("[DVP%d] Next: index %d, hw_using %d\n",
             ch, next_buf->index, next_buf->hw_using);

    /* DVP can use the next buf as output. */
    if (!next_buf->hw_using) {
        aic_dvp_buf_reload(dvp, next_buf, ch);
        dvp->ch[ch].sequence++;
    } else {
        /* This should not happened! */
        if (!dvp->cfg.interlaced)
            pr_debug("[DVP%d] Weird! DVP is using two buf %d & %d!\n",
                      ch, cur_buf->index, next_buf->index);
        return -1;
    }

    return 0;
}

static void aic_dvp_buf_queue(struct vb_buffer *vb)
{
    pr_debug("[DVP%d] Queue buf %d\n", vb->queue->ch, vb->index);

    list_add_tail(&vb->active_entry, &g_dvp.ch[vb->queue->ch].active_list);
    vb->hw_using = 0;
}

static void aic_dvp_reclaim_all_buffers(struct aic_dvp *dvp,
                                        enum vb_buffer_state state,
                                        u32 ch)
{
    struct vb_buffer *vb, *node;

    rt_base_t level = rt_hw_interrupt_disable();

    list_for_each_entry_safe(vb, node, &dvp->ch[ch].active_list, active_entry) {
        vin_vb_buffer_done(vb, state);
        list_del(&vb->active_entry);
    }

    rt_hw_interrupt_enable(level);
}

static int aic_dvp_start_streaming(struct vb_queue *q)
{
    struct aic_dvp *dvp = &g_dvp;
    struct vb_buffer *vb;
    int ch = q->ch;
    int ret = 0;

    pr_debug("[DVP%d] Starting capture\n", ch);
    dvp->ch[ch].sequence = 0;
    hal_dvp_field_tag_clr(ch);

    hal_dvp_set_cfg(&dvp->cfg, ch);
    hal_dvp_set_pol(dvp->cfg.flags, ch);
    hal_dvp_record_mode(ch);

    if (g_dvp.fmt.frame_offset)
        hal_dvp_set_frame_offset(g_dvp.fmt.frame_offset, ch);

    hal_dvp_clr_int(ch);
    hal_dvp_enable_int(&dvp->cfg, ch, 1);

    /* Prepare our active_uffers in hardware */
    vb = list_first_entry(&dvp->ch[ch].active_list, struct vb_buffer, active_entry);
    ret = aic_dvp_buf_reload(dvp, vb, ch);
    if (ret)
        goto err_disable_pipeline;

    hal_dvp_capture_start(ch);
    hal_dvp_update_ctl(ch);

    dvp->ch[ch].streaming = 1;
    return 0;

err_disable_pipeline:
    aic_dvp_reclaim_all_buffers(dvp, VB_BUF_STATE_QUEUED, ch);
    return ret;
}

static void aic_dvp_wait_streaming(struct aic_dvp *dvp, u32 ch)
{
    if (!dvp->ch[ch].streaming)
        return;

    dvp->ch[ch].streaming = 0;
    pr_debug("[DVP%d] Wait streaming done\n", ch);
    if (aicos_sem_take(dvp->ch[ch].finished, 200) < 0)
        pr_warn("Wait for stop streaming timeout!\n");
}

static void aic_dvp_stop_streaming(struct vb_queue *q)
{
    struct aic_dvp *dvp = &g_dvp;
    int ch = q->ch;

    pr_debug("[DVP%d] Stopping capture\n", ch);
    hal_dvp_capture_stop(ch);
    aic_dvp_wait_streaming(dvp, ch);
    hal_dvp_enable_int(&dvp->cfg, ch, 0);
    hal_dvp_update_ctl(ch);

    /* Release all active buffers */
    aic_dvp_reclaim_all_buffers(dvp, VB_BUF_STATE_DONE, ch);
}

static const struct vb_ops aic_dvp_vb_ops = {
    .buf_queue          = aic_dvp_buf_queue,
    .start_streaming    = aic_dvp_start_streaming,
    .stop_streaming     = aic_dvp_stop_streaming,
};

static void aic_dvp_ch_isr(u32 ch, u32 sta)
{
    struct aic_dvp *dvp = &g_dvp;
    struct aic_dvp_config *dvp_cfg = &dvp->cfg;
    struct aic_dvp_ch *dvp_ch = &dvp->ch[ch];
    static u32 all_ch_done = 0;
    u32 err = 0;

    pr_debug("[DVP%d] IRQ status 0x%x, sequence %d\n",
             ch, sta, dvp_ch->sequence);

    if ((sta & DVP_IRQ_STA_CLOSE_DONE) && !dvp_ch->streaming) {
        aicos_sem_give(dvp_ch->finished);
        return;
    }

    if (sta & DVP_IRQ_STA_BUF_FULL) {
        dvp_ch->full_cnt++;
        /* should tag the buf error, so APP can ignore it */
        err = 1;
        pr_warn("[DVP%d] FIFO is full! Count %d (0x%x)\n", ch, dvp_ch->full_cnt, sta);
    } else if (sta & DVP_IRQ_STA_XY_CODE_ERR) {
        err = 1;
        pr_warn("[DVP%d] checksum has error! (0x%x)\n", ch, sta);
        hal_dvp_clr_fifo();
        return;
    }

    if (sta & DVP_IRQ_EN_FRAME_DONE) {
        if (err)
            hal_dvp_clr_fifo();

        if (dvp_ch->resumed) {
            hal_dvp_clr_fifo();
            hal_dvp_clr_fifo();
            dvp_ch->resumed = false;
        }

        if (dvp_cfg->interlaced) {
            /* If the first field is a bottom field, ignore it */
            if (!dvp_ch->recv_first_field && hal_dvp_is_bottom_field(ch)) {
                pr_info("[DVP%d] The first is bottom field - ignored\n", ch);
                hal_dvp_clr_fifo();
                dvp_ch->recv_first_field = true;
                return;
            }

            if (hal_dvp_is_top_field(ch)) {
                dvp_ch->recv_first_field = true;
#ifdef DVP_SFIELD_MODE
            } else {
                /* Ignore the bottom field */
                return;
            }
        }
#else
                return;
            }
        }
#endif

        if (MPP_IS_STITCH(dvp_cfg->stitch_mode)) {
            all_ch_done |= BIT(ch);
            if ((ch != 0) && (all_ch_done == DVP_CH_IRQ_ALL_PENDING))
                all_ch_done &= ~BIT(0); // SO, wait for next CH0 done

            /* Only consider frame done when CH0 is done */
            if ((all_ch_done == DVP_CH_IRQ_ALL_PENDING) && (ch == 0)) {
                for (int i = DVP_MAX_CH_NUM - 1; i >= 0; i--)
                    aic_dvp_frame_done(dvp, err, i);
                all_ch_done = 0;
            }
        } else {
            aic_dvp_frame_done(dvp, err, ch);
        }
    }

    if (sta & DVP_IRQ_STA_HNUM) {
        if (dvp_cfg->interlaced) {
            hal_dvp_get_current_xy(ch);

            if (hal_dvp_is_top_field(ch)) {
                aic_dvp_top_field_done(dvp, err, ch);
                dvp_ch->recv_first_field = true;
                return;
            }

            /* If the first field is a bottom field, ignore it */
            if (!dvp_ch->recv_first_field) {
                pr_debug("The first is bottom field - ignore\n");
                return;
            }
        }

        aic_dvp_update_addr(dvp, ch);
    }
}

static irqreturn_t aic_dvp_isr(int irq, void *data)
{
    s32 ch, pending, sta;

    pending = hal_dvp_ch_irq_sta_get();
    if (pending)
        pr_debug("DVP IRQ channel pending: 0x%x\n", pending);

    for (ch = DVP_MAX_CH_NUM - 1; ch >= 0; ch--) {
        sta = hal_dvp_clr_int(ch);

        if (!sta)
            continue;

        if (!g_dvp.ch[ch].available) {
            pr_debug("[DVP%d] IRQ is unavailable! sta 0x%x\n", ch, sta);
            continue;
        }

        if (!hal_dvp_int_is_enabled(ch)) {
            pr_warn("[DVP%d] IRQ is not enabled! sta 0x%x\n", ch, sta);
            continue;
        }

        if (pending & BIT(ch))
            aic_dvp_ch_isr(ch, sta);
    }

    return IRQ_HANDLED;
}

bool aic_dvp_sfield_mode(void)
{
#ifdef DVP_SFIELD_MODE
    if (g_dvp.cfg.interlaced)
        return true;
    else
        return false;
#else
    return false;
#endif
}

int aic_dvp_probe(void)
{
    int ret = 0;

    ret = aicos_request_irq(DVP_IRQn, aic_dvp_isr, 0, AIC_DVP_NAME, NULL);
    if (ret < 0) {
        pr_err("Failed to request DVP IRQ\n");
        return -1;
    }

    return ret;
}

int aic_dvp_vb_init(u32 ch)
{
    struct aic_dvp_ch *dvp_ch = &g_dvp.ch[ch];

    if (vin_vb_init(&dvp_ch->queue, &aic_dvp_vb_ops))
        return -1;

    dvp_ch->queue.ch = ch;
    INIT_LIST_HEAD(&dvp_ch->active_list);
    if (!dvp_ch->finished)
        dvp_ch->finished = aicos_sem_create(0);

    dvp_ch->available = true;
    return 0;
}

void aic_dvp_vb_deinit(u32 ch)
{
    struct aic_dvp_ch *dvp_ch = &g_dvp.ch[ch];

    if (!dvp_ch->available) {
        pr_err("[DVP%d] VB is not initialized\n", ch);
        return;
    }

    vin_vb_deinit(&dvp_ch->queue);
    dvp_ch->available = false;

    if (dvp_ch->finished) {
        aicos_sem_delete(dvp_ch->finished);
        dvp_ch->finished = NULL;
    }
}

int aic_dvp_open(void)
{
    int ret = 0;

    if (hal_clk_is_enabled(CLK_DVP)) {
        pr_debug("DVP has been enabled\n");
        return 0;
    }

    ret = hal_clk_set_freq(CLK_DVP, AIC_DVP_CLK_RATE);
    if (ret < 0) {
        pr_err("Failed to set DVP clk %d\n", AIC_DVP_CLK_RATE);
        return -1;
    }

    ret = hal_clk_enable_deassertrst(CLK_DVP);
    if (ret < 0) {
        pr_err("DVP reset enable failed!\n");
        return -1;
    }

    hal_dvp_qos_cfg(AIC_DVP_QOS_HIGH, AIC_DVP_QOS_LOW, 0x100, 0x80);
    hal_dvp_enable(&g_dvp.cfg, 1);

    memset(g_dvp.ch, 0, sizeof(struct aic_dvp_ch) * DVP_MAX_CH_NUM);
    g_dvp.inited = true;
    return 0;
}

int aic_dvp_close(void)
{
    struct aic_dvp_ch *dvp_ch = g_dvp.ch;
    int ret = 0;
    u32 i;

    for (i = 0, dvp_ch = g_dvp.ch; i < DVP_MAX_CH_NUM; i++, dvp_ch++) {
        if (dvp_ch->available && dvp_ch->streaming)
            aic_dvp_wait_streaming(&g_dvp, i);
    }

    hal_dvp_enable(&g_dvp.cfg, 0);

    ret = hal_clk_disable_assertrst(CLK_DVP);
    if (ret < 0) {
        pr_err("DVP reset disable failed!\n");
        return -1;
    }

    for (i = 0, dvp_ch = g_dvp.ch; i < DVP_MAX_CH_NUM; i++, dvp_ch++) {
        if (dvp_ch->full_cnt)
            pr_info("[DVP%d] FIFO full happened %d times\n", i, dvp_ch->full_cnt);
    }

    g_dvp.inited = false;
    return 0;
}

void cmd_dvp_vb_info(int argc, char **argv)
{
    struct aic_dvp_ch *dvp_ch = g_dvp.ch;
    struct vb_buffer *vb = NULL;
    u32 i;

    printf("\nDVP Driver info:\n");
    printf("===============================================\n");
    printf("No. Streaming Sequence Frame    Update     Full\n");
    printf("--- --------- -------- -------- -------- ------\n");

    for (i = 0, dvp_ch = g_dvp.ch; i < DVP_MAX_CH_NUM; i++, dvp_ch++) {
        if (!dvp_ch->available)
            continue;
        printf("%3d %9s %8d %8d %8d %6d\n", i, dvp_ch->streaming ? "yes" : "no",
               dvp_ch->sequence, dvp_ch->frame_cnt, dvp_ch->update_cnt,
               dvp_ch->full_cnt);
    }
    printf("===============================================\n");

    for (i = 0, dvp_ch = g_dvp.ch; i < DVP_MAX_CH_NUM; i++, dvp_ch++) {
        if (!dvp_ch->available)
            continue;

        printf("[DVP%d] VB info:\n", i);
        vin_vb_show_info(&dvp_ch->queue);

        printf("Active list  : [");
        list_for_each_entry(vb, &dvp_ch->active_list, active_entry)
            printf("%d%s", vb->index, vb->hw_using ? "* " : " ");
        printf("]\n");
    }
    printf("\n");
}
MSH_CMD_EXPORT_ALIAS(cmd_dvp_vb_info, vbinfo, Show VB status);

#ifdef RT_USING_PM
static int aic_dvp_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct aic_dvp *dvp_dev = (struct aic_dvp *)device->user_data;
    struct aic_dvp_ch *dvp_ch = dvp_dev->ch;
    u32 i;

    if (!dvp_dev->inited) {
        pr_debug("DVP is not initialized\n");
        return 0;
    }

    for (i = 0; i < DVP_MAX_CH_NUM; i++, dvp_ch++) {
        if (dvp_ch->available && dvp_ch->streaming) {
            pr_err("[DVP%d] is streaming, cannot suspend\n", i);
            return -EBUSY;
        }
    }

    switch (mode)
    {
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
#ifdef AIC_PM_DRV_V15
        hal_clk_disable_assertrst(CLK_DVP);
#else
        hal_clk_disable(CLK_DVP);
#endif
        LOG_I("DVP suspend\n");
        break;
    case PM_SLEEP_MODE_IDLE:
    default:
        break;
    }

    return 0;
}

static void aic_dvp_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct aic_dvp *dvp_dev = (struct aic_dvp *)device->user_data;
    int ret = 0;

    if (!dvp_dev->inited) {
        pr_debug("DVP is not initialized\n");
        return;
    }

    switch (mode)
    {
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        ret = hal_clk_set_freq(CLK_DVP, AIC_DVP_CLK_RATE);
        if (ret < 0) {
            pr_err("Failed to set DVP clk %d\n", AIC_DVP_CLK_RATE);
            break;
        }
#ifdef AIC_PM_DRV_V15
        hal_clk_enable_deassertrst(CLK_DVP);
#else
        hal_clk_enable(CLK_DVP);
#endif

        hal_dvp_qos_cfg(AIC_DVP_QOS_HIGH, AIC_DVP_QOS_LOW, 0x100, 0x80);
        hal_dvp_enable(&dvp_dev->cfg, 1);
        LOG_I("DVP resume\n");
        break;
    case PM_SLEEP_MODE_IDLE:
    default:
        break;
    }
}

static struct rt_device_pm_ops aic_dvp_pm_ops =
{
    SET_DEVICE_PM_OPS(aic_dvp_suspend, aic_dvp_resume)
    NULL,
};
#endif /* RT_USING_PM */

static int drv_dvp_init(void)
{
    rt_err_t ret = RT_EOK;

#ifdef RT_USING_DEVICE_OPS
    g_dvp.dev.ops = RT_NULL;
#endif
    g_dvp.dev.type = RT_Device_Class_Miscellaneous;

    ret = rt_device_register(&g_dvp.dev, AIC_DVP_NAME, 0);
    if (ret != RT_EOK) {
        pr_err("Failed to register DVP device\n");
        return ret;
    }
    g_dvp.dev.user_data = &g_dvp;

#ifdef RT_USING_PM
    rt_pm_device_register(&g_dvp.dev, &aic_dvp_pm_ops);
#endif

    return 0;
}
INIT_PREV_EXPORT(drv_dvp_init);
