/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <string.h>

#include "aic_core.h"
#include "aic_list.h"
#include "aic_hal_clk.h"
#include "mpp_types.h"

#include "hal_dvp.h"
#include "drv_dvp.h"

#define DVP_FIRST_BUF       0
#define BUF_IS_INVALID(index)   (((index) < 0) || ((index) >= VIN_MAX_BUF_NUM))

struct aic_dvp g_dvp = {0};
static u32 g_dvp_full_cnt = 0;

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
    {MPP_FMT_YUV400, DVP_OUT_RAW_PASSTHROUGH}
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
        if ((ret == DVP_OUT_YUV420_COMBINED_NV12) && (i > 0))
            pix->plane_fmt[i].sizeimage >>= 1;
        if ((g_dvp.cfg.input == DVP_IN_RAW) && (i > 0)) {
            pix->plane_fmt[i].bytesperline = 0;
            pix->plane_fmt[i].sizeimage = 0;
        }
    }
}

int aic_dvp_set_out_fmt(struct dvp_out_fmt *fmt)
{
    int i;

    _aic_dvp_try_fmt(fmt);
    g_dvp.fmt = *fmt;

    /* Save the configuration for DVP controller */
    g_dvp.cfg.output = aic_dvp_out_fmt_valid(g_dvp.fmt.pixelformat);
    g_dvp.cfg.width = g_dvp.fmt.width;
    g_dvp.cfg.height = g_dvp.fmt.height;
    for (i = 0; i < DVP_PLANE_NUM; i++) {
        g_dvp.cfg.stride[i] = g_dvp.fmt.plane_fmt[i].bytesperline;
        g_dvp.cfg.sizeimage[i] = g_dvp.fmt.plane_fmt[i].sizeimage;
    }
    return 0;
}

int aic_dvp_stream_on(void)
{
    return vin_vb_stream_on(&g_dvp.queue);
}

int aic_dvp_stream_off(void)
{
    int ret = 0;

    ret = vin_vb_stream_off(&g_dvp.queue);

    INIT_LIST_HEAD(&g_dvp.active_list);

    return ret;
}

int aic_dvp_req_buf(char *buf, u32 size, struct vin_video_buf *vbuf)
{
    struct aic_dvp_config *cfg = &g_dvp.cfg;
    int i;

    if (!vbuf) {
        pr_err("Invalid parameter\n");
        return -1;
    }

    memset(vbuf, 0, sizeof(struct vin_video_buf));
    vbuf->num_planes = DVP_PLANE_NUM;
    for (i = 0; i < DVP_PLANE_NUM; i++)
        vbuf->planes[i].len = cfg->sizeimage[i];

    return vin_vb_req_buf(&g_dvp.queue, buf, size, vbuf);
}

int aic_dvp_q_buf(u32 index)
{
    if (index >= g_dvp.queue.num_buffers) {
        pr_err("Invalid index out of range: %d\n", index);
        return -EINVAL;
    }

    return vin_vb_q_buf(&g_dvp.queue, index);
}

int aic_dvp_dq_buf(u32 *pindex)
{
    if (pindex == NULL) {
        pr_err("Invalid parameter\n");
        return -EINVAL;
    }

    return vin_vb_dq_buf(&g_dvp.queue, pindex);
}

static int aic_dvp_buf_reload(struct aic_dvp *dvp, struct vb_buffer *buf)
{
    buf->hw_using = 1;
    pr_debug("Set %d buf 0x%x-0x%x to register\n", buf->index,
             (long)buf->planes[0].buf, (long)buf->planes[1].buf);
    hal_dvp_update_buf_addr(buf->planes[0].buf, buf->planes[1].buf, 0);
    hal_dvp_update_ctl();
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

static int aic_dvp_top_field_done(struct aic_dvp *dvp, u32 err)
{
    struct vb_buffer *cur_buf = NULL;

    if (list_empty(&dvp->active_list)) {
        pr_err("No buf available!\n");
        return 0;
    }

    cur_buf = list_first_entry(&dvp->active_list, struct vb_buffer, active_entry);
    pr_debug("cur: index %d, dvp_using %d\n",
             cur_buf->vb.vb2_buf.index, cur_buf->dvp_using);
    if (BUF_IS_INVALID(cur_buf->index)) {
        pr_err("Invalid buf %d\n", cur_buf->index);
        return -1;
    }

    pr_debug("Add offset %d of cur buf %d", dvp->cfg.stride[0], cur_buf->index);

    hal_dvp_update_buf_addr(cur_buf->planes[0].buf, cur_buf->planes[1].buf,
                            dvp->cfg.stride[0]);
    hal_dvp_update_ctl();
    dvp->sequence++;
    return 0;
}

static int aic_dvp_frame_done(struct aic_dvp *dvp, int err)
{
    struct vb_buffer *cur_buf = NULL;

    if (list_empty(&dvp->active_list)) {
#ifndef AIC_DVP_IGNORE_LOSS
        pr_err("No buf available!\n");
#endif
        return 0;
    }

    cur_buf = list_first_entry(&dvp->active_list, struct vb_buffer, active_entry);
    pr_debug("cur: index %d, hw_using %d, err %d\n",
             cur_buf->index, cur_buf->hw_using, err);
    if (BUF_IS_INVALID(cur_buf->index)) {
        pr_err("Invalid buf %d\n", cur_buf->index);
        return -1;
    }

    /* If cur_buf is a new one queued, DVP should use it first */
    if (!cur_buf->hw_using) {
        pr_debug("Good! Buf %d is free again\n", cur_buf->index);
        aic_dvp_buf_reload(dvp, cur_buf);
        dvp->sequence++;
        return 0;
    }

    /* Release the current buffer from DVP driver */
    list_del(&cur_buf->active_entry);
    aic_dvp_buf_mark_done(dvp, cur_buf, dvp->sequence, err);

    return 0;
}

static int aic_dvp_update_addr(struct aic_dvp *dvp)
{
    struct vb_buffer *cur_buf;
    struct vb_buffer *next_buf;

    if (!dvp->streaming)
        return 0;

    if (list_empty(&dvp->active_list)) {
#ifndef AIC_DVP_IGNORE_LOSS
        pr_warn("No buf available!\n");
#endif
        return -1;
    }

    cur_buf = list_first_entry(&dvp->active_list, struct vb_buffer, active_entry);
    pr_debug("cur: index %d, hw_using %d\n", cur_buf->index, cur_buf->hw_using);
    if (BUF_IS_INVALID(cur_buf->index)) {
        pr_err("Cur buf %d is invalid\n", cur_buf->index);
        return -1;
    }

    if (!cur_buf->hw_using) {
        aic_dvp_buf_reload(dvp, cur_buf);
        dvp->sequence++;
        return 0;
    }

    if (cur_buf == list_last_entry(&dvp->active_list, struct vb_buffer,
                                   active_entry)) {
#ifndef AIC_DVP_IGNORE_LOSS
        pr_warn("It's the last buf!\n");
#endif
        return 0;
    }

    next_buf = list_next_entry(cur_buf, active_entry);
    if (!next_buf || BUF_IS_INVALID(next_buf->index)) {
        pr_err("Next buf is invalid\n");
        return -1;
    }
    pr_debug("Next: index %d, hw_using %d\n",
             next_buf->index, next_buf->hw_using);

    /* DVP can use the next buf as output. */
    if (!next_buf->hw_using) {
        aic_dvp_buf_reload(dvp, next_buf);
        dvp->sequence++;
    } else {
        /* This should not happened! */
        pr_warn("Weird! DVP is using two buf %d & %d!\n",
                cur_buf->index, next_buf->index);
        return -1;
    }

    return 0;
}

static void aic_dvp_buf_queue(struct vb_buffer *vb)
{
    pr_debug("Queue buf %d\n", vb->index);

    list_add_tail(&vb->active_entry, &g_dvp.active_list);
    vb->hw_using = 0;
}

static void aic_dvp_reclaim_all_buffers(struct aic_dvp *dvp,
                                        enum vb_buffer_state state)
{
    struct vb_buffer *vb, *node;

    rt_base_t level = rt_hw_interrupt_disable();

    list_for_each_entry_safe(vb, node, &dvp->active_list, active_entry) {
        vin_vb_buffer_done(vb, state);
        list_del(&vb->active_entry);
    }

    rt_hw_interrupt_enable(level);
}

static int aic_dvp_start_streaming(struct vb_queue *q)
{
    struct vb_buffer *vb;
    int ret = 0;
    struct aic_dvp *dvp = &g_dvp;

    pr_debug("Starting capture\n");

    dvp->sequence = 0;
    hal_dvp_field_tag_clr();

    hal_dvp_set_cfg(&dvp->cfg);
    hal_dvp_set_pol(dvp->cfg.flags);
    hal_dvp_record_mode();

    hal_dvp_clr_int();
    hal_dvp_enable_int(&dvp->cfg, 1);

    /* Prepare our active_uffers in hardware */
    vb = list_first_entry(&dvp->active_list, struct vb_buffer, active_entry);
    ret = aic_dvp_buf_reload(dvp, vb);
    if (ret)
        goto err_disable_pipeline;

    hal_dvp_capture_start();
    hal_dvp_update_ctl();

    dvp->streaming = 1;
    return 0;

err_disable_pipeline:
    aic_dvp_reclaim_all_buffers(dvp, VB_BUF_STATE_QUEUED);
    return ret;
}

static void aic_dvp_stop_streaming(struct vb_queue *q)
{
    struct aic_dvp *dvp = &g_dvp;

    pr_debug("Stopping capture\n");

    hal_dvp_capture_stop();
    hal_dvp_enable_int(&dvp->cfg, 0);
    hal_dvp_update_ctl();

    /* Release all active buffers */
    aic_dvp_reclaim_all_buffers(dvp, VB_BUF_STATE_ERROR);
    dvp->streaming = 0;
}

static const struct vb_ops aic_dvp_vb_ops = {
    .buf_queue          = aic_dvp_buf_queue,
    .start_streaming    = aic_dvp_start_streaming,
    .stop_streaming     = aic_dvp_stop_streaming,
};

static irqreturn_t aic_dvp_isr(int irq, void *data)
{
    struct aic_dvp *dvp = &g_dvp;
    u32 sta, err = 0;
    static u32 recv_first_field = 0;

    sta = hal_dvp_clr_int();
    pr_debug("IRQ status 0x%x, sequence %d\n", sta, dvp->sequence);

    if (sta & DVP_IRQ_STA_BUF_FULL) {
        g_dvp_full_cnt++;
        /* should tag the buf error, so APP can ignore it */
        err = 1;
        pr_debug("DVP FIFO is full! Count %d (0x%x)\n", g_dvp_full_cnt, sta);
    } else if (sta & DVP_IRQ_STA_XY_CODE_ERR) {
        err = 1;
        pr_warn("DVP checksum has error! (0x%x)\n", sta);
        hal_dvp_clr_fifo();
        return IRQ_HANDLED;
    }

    if (sta & DVP_IRQ_EN_FRAME_DONE) {
        if (err)
            hal_dvp_clr_fifo();

        if (dvp->cfg.interlaced) {
            /* If the first field is a bottom field, ignore it */
            if (!recv_first_field && hal_dvp_is_bottom_field()) {
                pr_info("The first is bottom field - ignored\n");
                hal_dvp_clr_fifo();
                recv_first_field = 1;
                return IRQ_HANDLED;
            }

            if (hal_dvp_is_top_field()) {
                recv_first_field = 1;
                return IRQ_HANDLED;
            }
        }

        aic_dvp_frame_done(dvp, err);
    }

    if (sta & DVP_IRQ_STA_HNUM) {
        if (dvp->cfg.interlaced) {
            hal_dvp_get_current_xy();

            if (hal_dvp_is_top_field()) {
                aic_dvp_top_field_done(dvp, err);
                recv_first_field = 1;
                return IRQ_HANDLED;
            }

            /* If the first field is a bottom field, ignore it */
            if (!recv_first_field) {
                pr_debug("The first is bottom field - ignore\n");
                return IRQ_HANDLED;
            }
        }

        aic_dvp_update_addr(dvp);
    }

    return IRQ_HANDLED;
}

int aic_dvp_probe(void)
{
    int ret = 0;

    ret = aicos_request_irq(DVP_IRQn, aic_dvp_isr, 0, "AIC_DVP_NAME", NULL);
    if (ret < 0) {
        pr_err("Failed to request DVP IRQ\n");
        return -1;
    }

    memset(&g_dvp, 0, sizeof(struct aic_dvp));
    INIT_LIST_HEAD(&g_dvp.active_list);

    return ret;
}

int aic_dvp_vb_init(void)
{
    if (vin_vb_init(&g_dvp.queue, &aic_dvp_vb_ops))
        return -1;

    INIT_LIST_HEAD(&g_dvp.active_list);

    return 0;
}

void aic_dvp_vb_deinit(void)
{
    vin_vb_deinit(&g_dvp.queue);
}

int aic_dvp_open(void)
{
    int ret = 0;

    if (hal_clk_is_enabled(CLK_DVP)) {
        pr_debug("DVP has been enabled\n");
        return 0;
    }

    ret = hal_clk_enable(CLK_DVP);
    if (ret < 0) {
        pr_err("DVP clk enable failed!\n");
        return -1;
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

    g_dvp_full_cnt = 0;
    return 0;
}

int aic_dvp_close(void)
{
    int ret = 0;

    hal_dvp_enable(&g_dvp.cfg, 0);

    ret = hal_clk_disable_assertrst(CLK_DVP);
    if (ret < 0) {
        pr_err("DVP reset disable failed!\n");
        return -1;
    }

    ret = hal_clk_disable(CLK_DVP);
    if (ret < 0) {
        pr_err("DVP clk disable failed!\n");
        return -1;
    }

    if (g_dvp_full_cnt)
        pr_info("DVP FIFO full happened %d times\n", g_dvp_full_cnt);

    return 0;
}
