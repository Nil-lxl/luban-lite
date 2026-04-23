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

struct aic_dvp g_dvp = {0};
static u32 g_dvp_full_cnt[DVP_MAX_CH_NUM] = {0};
static bool g_dvp_resumed[DVP_MAX_CH_NUM] = {false};

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
    g_dvp_resumed[ch] = true;
    hal_dvp_enable_int(&g_dvp.cfg, ch, 1);
}

int aic_dvp_req_buf(char *buf, u32 size, struct vin_video_buf *vbuf, u32 ch)
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

    return vin_vb_req_buf(&g_dvp.ch[ch].queue, buf, size, vbuf);
}

int aic_dvp_q_buf(u32 index, u32 ch)
{
    if (index >= g_dvp.ch[ch].queue.num_buffers) {
        pr_err("Invalid index %d for channel %d\n", index, ch);
        return -EINVAL;
    }

    return vin_vb_q_buf(&g_dvp.ch[ch].queue, index);
}

int aic_dvp_dq_buf(u32 *pindex, u32 ch)
{
    if (pindex == NULL) {
        pr_err("Invalid parameter\n");
        return -EINVAL;
    }

    return vin_vb_dq_buf(&g_dvp.ch[ch].queue, pindex);
}

/* Return: 0, error; > 0, the elapse time in ms unit */
u32 aic_dvp_get_timestamp(u32 index, u32 ch)
{
    if (index >= g_dvp.ch[ch].queue.num_buffers) {
        pr_err("Invalid index out of range: %d\n", index);
        return 0;
    }

    return vin_vb_get_timestamp(&g_dvp.ch[ch].queue, index);
}

static int aic_dvp_buf_reload(struct aic_dvp *dvp, struct vb_buffer *buf, u32 ch)
{
    buf->hw_using = 1;
    pr_debug("[%u] Set %d buf 0x%x-0x%x to register\n", aic_get_time_ms(), buf->index,
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
    struct vb_buffer *cur_buf = NULL;

    if (list_empty(&dvp->ch[ch].active_list)) {
        pr_err("No buf available!\n");
        return 0;
    }

    cur_buf = list_first_entry(&dvp->ch[ch].active_list, struct vb_buffer, active_entry);
    pr_debug("cur: index %d, dvp_using %d\n",
             cur_buf->index, cur_buf->hw_using);
    if (BUF_IS_INVALID(cur_buf->index)) {
        pr_err("Invalid buf %d\n", cur_buf->index);
        return -1;
    }

    pr_debug("Add offset %d of cur buf %d", dvp->cfg.stride[0], cur_buf->index);

#ifdef DVP_SFIELD_MODE
    hal_dvp_update_buf_addr(cur_buf->planes[0].buf, cur_buf->planes[1].buf, ch,
                            dvp->cfg.sizeimage[0] / 2, dvp->cfg.sizeimage[1] / 2);
#else
    hal_dvp_update_buf_addr(cur_buf->planes[0].buf, cur_buf->planes[1].buf, ch,
                            dvp->cfg.stride[0], dvp->cfg.stride[0]);
#endif
    dvp->ch[ch].sequence++;
    return 0;
}

static int aic_dvp_frame_done(struct aic_dvp *dvp, int err, u32 ch)
{
    static bool need_skip[DVP_MAX_CH_NUM] = {false};
    struct vb_buffer *cur_buf = NULL;

    if (need_skip[ch]) {
        need_skip[ch] = false;
        return 0;
    }

    if (list_empty(&dvp->ch[ch].active_list)) {
#ifndef AIC_DVP_IGNORE_LOSS
        pr_err("No buf available!\n");
#endif
        return 0;
    }

    cur_buf = list_first_entry(&dvp->ch[ch].active_list, struct vb_buffer, active_entry);
    pr_debug("[%u] cur: index %d, hw_using %d, err %d\n\n", aic_get_time_ms(),
             cur_buf->index, cur_buf->hw_using, err);
    if (BUF_IS_INVALID(cur_buf->index)) {
        pr_err("Invalid buf %d\n", cur_buf->index);
        return -1;
    }

    /* If cur_buf is a new one queued, DVP should use it first */
    if (!cur_buf->hw_using) {
        pr_debug("Buf %d is free just now\n", cur_buf->index);
        aic_dvp_buf_reload(dvp, cur_buf, ch);
        dvp->ch[ch].sequence++;
        need_skip[ch] = true;
        return 0;
    }

    /* Release the current buffer from DVP driver */
    list_del(&cur_buf->active_entry);
    aic_dvp_buf_mark_done(dvp, cur_buf, dvp->ch[ch].sequence, err);

    if (!g_dvp.ch[ch].streaming)
        aicos_sem_give(g_dvp.ch[ch].finished);

    return 0;
}

static int aic_dvp_update_addr(struct aic_dvp *dvp, u32 ch)
{
    struct vb_buffer *cur_buf;
    struct vb_buffer *next_buf;

    if (!dvp->ch[ch].streaming)
        return 0;

    if (list_empty(&dvp->ch[ch].active_list)) {
#ifndef AIC_DVP_IGNORE_LOSS
        pr_warn("No buf available!\n");
#endif
        return -1;
    }

    cur_buf = list_first_entry(&dvp->ch[ch].active_list, struct vb_buffer, active_entry);
    pr_debug("cur: index %d, hw_using %d\n", cur_buf->index, cur_buf->hw_using);
    if (BUF_IS_INVALID(cur_buf->index)) {
        pr_err("Cur buf %d is invalid\n", cur_buf->index);
        return -1;
    }

    if (cur_buf == list_last_entry(&dvp->ch[ch].active_list, struct vb_buffer,
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
        aic_dvp_buf_reload(dvp, next_buf, ch);
        dvp->ch[ch].sequence++;
    } else {
        /* This should not happened! */
        if (!dvp->cfg.interlaced)
            pr_debug("[%u] Weird! DVP is using two buf %d & %d!\n",
                     aic_get_time_ms(), cur_buf->index, next_buf->index);
        return -1;
    }

    return 0;
}

static void aic_dvp_buf_queue(struct vb_buffer *vb)
{
    pr_debug("Queue buf %d\n", vb->index);

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

    pr_debug("Starting capture\n");

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

static int aic_dvp_wait_irq_sta_set(u32 ch, u32 flag, u32 timeout_ms)
{
    u32 wait_time = 0;
    u32 check_interval = 10;

    while (wait_time < timeout_ms) {
        u32 reg_val = hal_dvp_irq_sta_get(ch);

        if (reg_val & flag) {
            return reg_val & flag;
        }

        aicos_msleep(check_interval);
        wait_time += check_interval;
    }

    return 0;
}

static void aic_dvp_wait_streaming(struct aic_dvp *dvp, u32 ch)
{
    if (!dvp->ch[ch].streaming)
        return;

    dvp->ch[ch].streaming = 0;
    pr_debug("Wait streaming done\n");
    if (aicos_sem_take(dvp->ch[ch].finished, 200) < 0)
        pr_warn("Wait for stop streaming timeout!\n");
}

static void aic_dvp_stop_streaming(struct vb_queue *q)
{
    struct aic_dvp *dvp = &g_dvp;
    int ch = q->ch;

    pr_debug("Stopping capture\n");

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

static irqreturn_t aic_dvp_isr(int irq, void *data)
{
    static u32 recv_first_field = 0;
    struct aic_dvp *dvp = &g_dvp;
    u32 ch = (u32)(long)data;
    u32 sta, err = 0;

    sta = hal_dvp_clr_int(ch);
    pr_debug("[%u] IRQ status 0x%x, sequence %d\n", aic_get_time_ms(),
             sta, dvp->ch[ch].sequence);

    if (sta & DVP_IRQ_STA_BUF_FULL) {
        g_dvp_full_cnt[ch]++;
        /* should tag the buf error, so APP can ignore it */
        err = 1;
        pr_warn("DVP FIFO is full! Count %d (0x%x)\n", g_dvp_full_cnt[ch], sta);
    } else if (sta & DVP_IRQ_STA_XY_CODE_ERR) {
        err = 1;
        pr_warn("DVP checksum has error! (0x%x)\n", sta);
        hal_dvp_clr_fifo();
        return IRQ_HANDLED;
    }

    if (sta & DVP_IRQ_EN_FRAME_DONE) {
        if (err)
            hal_dvp_clr_fifo();

        if (g_dvp_resumed[ch]) {
            hal_dvp_clr_fifo();
            hal_dvp_clr_fifo();
            g_dvp_resumed[ch] = false;
        }

        if (dvp->cfg.interlaced) {
            /* If the first field is a bottom field, ignore it */
            if (!recv_first_field && hal_dvp_is_bottom_field(ch)) {
                pr_info("The first is bottom field - ignored\n");
                hal_dvp_clr_fifo();
                recv_first_field = 1;
                return IRQ_HANDLED;
            }

            if (hal_dvp_is_top_field(ch)) {
                recv_first_field = 1;
#ifdef DVP_SFIELD_MODE
            } else {
                /* Ignore the bottom field */
                return IRQ_HANDLED;
            }
        }
#else
                return IRQ_HANDLED;
            }
        }
#endif

        aic_dvp_frame_done(dvp, err, ch);
    }

    if (sta & DVP_IRQ_STA_HNUM) {
        if (dvp->cfg.interlaced) {
            hal_dvp_get_current_xy(ch);

            if (hal_dvp_is_top_field(ch)) {
                aic_dvp_top_field_done(dvp, err, ch);
                recv_first_field = 1;
                return IRQ_HANDLED;
            }

            /* If the first field is a bottom field, ignore it */
            if (!recv_first_field) {
                pr_debug("The first is bottom field - ignore\n");
                return IRQ_HANDLED;
            }
        }

        aic_dvp_update_addr(dvp, ch);
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

int aic_dvp_probe(u32 ch)
{
    int ret = 0;

    ret = aicos_request_irq(DVP_IRQn, aic_dvp_isr, 0, "AIC_DVP_NAME", (void *)(long)ch);
    if (ret < 0) {
        pr_err("Failed to request DVP IRQ\n");
        return -1;
    }

    memset(&g_dvp, 0, sizeof(struct aic_dvp));
    INIT_LIST_HEAD(&g_dvp.ch[ch].active_list);

    return ret;
}

int aic_dvp_vb_init(u32 ch)
{
    mpp_vin_sel_ch(ch);

    if (vin_vb_init(&g_dvp.ch[ch].queue, &aic_dvp_vb_ops))
        return -1;

    INIT_LIST_HEAD(&g_dvp.ch[ch].active_list);
    if (!g_dvp.ch[ch].finished)
        g_dvp.ch[ch].finished = aicos_sem_create(0);

    return 0;
}

void aic_dvp_vb_deinit(u32 ch)
{
    vin_vb_deinit(&g_dvp.ch[ch].queue);
}

int aic_dvp_open(u32 ch)
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

    /* Channel 0 and channel 1 process the input of the 0th data separately */
    if (ch == DVP_CH0)
        hal_dvp_ch_index_config(DVP_CH_INDEX_0, DVP_CH_INDEX_1);

    if (ch == DVP_CH1)
        hal_dvp_ch_index_config(DVP_CH_INDEX_1, DVP_CH_INDEX_0);

    /* Select single channel mode */
    hal_dvp_channel_sel(SINGLE_CHANNEL);

    /* Initialize the data counter for the specified channel */
    g_dvp_full_cnt[ch] = 0;
    return 0;
}

int aic_dvp_close(u32 ch)
{
    int ret = 0, dvp_sta = 0;
    dvp_sta = aic_dvp_wait_irq_sta_set(ch, DVP_IRQ_STA_CLOSE_STA, 100);

    if (g_dvp.ch[ch].streaming)
        aic_dvp_wait_streaming(&g_dvp, ch);

    hal_dvp_enable(&g_dvp.cfg, 0);

    if (dvp_sta) {
        ret = hal_clk_disable_assertrst(CLK_DVP);
        if (ret < 0) {
            pr_err("DVP reset disable failed!\n");
            return -1;
        }
    }

    if (g_dvp_full_cnt[ch])
        pr_info("DVP FIFO full happened %d times\n", g_dvp_full_cnt[ch]);

    return 0;
}

void cmd_dvp_vb_info(int argc, char **argv, u32 ch)
{
    struct vb_buffer *vb = NULL;

    vin_vb_show_info(&g_dvp.ch[ch].queue);

    printf("Active list  : [");
    list_for_each_entry(vb, &g_dvp.ch[ch].active_list, active_entry)
        printf("%d%s", vb->index, vb->hw_using ? "* " : " ");
    printf("]\n");
}
MSH_CMD_EXPORT_ALIAS(cmd_dvp_vb_info, vbinfo, Show VB status);
