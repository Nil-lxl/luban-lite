/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <string.h>

#include "aic_core.h"

#include "mpp_vin_vb.h"

static const struct vb_ops *g_vb_ops = NULL;

static const char *vb_state_name(enum vb_buffer_state s)
{
    static const char * const state_names[] = {
        [VB_BUF_STATE_DEQUEUED] = "Dequeued",
        [VB_BUF_STATE_QUEUED] = "Queued",
        [VB_BUF_STATE_ACTIVE] = "Active",
        [VB_BUF_STATE_DONE] = "Done",
        [VB_BUF_STATE_ERROR] = "Error",
    };

    if ((unsigned int)(s) < ARRAY_SIZE(state_names))
        return state_names[s];
    return "unknown";
}

int vin_vb_req_buf(struct vb_queue *q,
                   char *buf, u32 size, struct vin_video_buf *vbuf)
{
    u32 i, j, one_buf_size = 0, offset = 0;
    struct vin_video_plane *cur_plane = NULL;

    for (i = 0; i < vbuf->num_planes; i++)
        one_buf_size += vbuf->planes[i].len;

    if (one_buf_size * 2 > size) {
        pr_err("The buffer size is too small: %d/%d\n", size, one_buf_size);
        return -1;
    }

    for (i = 0; i < size/one_buf_size; i++) {
        if (i >= VIN_MAX_BUF_NUM)
            break;

        cur_plane = &vbuf->planes[i * vbuf->num_planes];
        for (j = 0; j < vbuf->num_planes; j++) {
            cur_plane->buf = (dma_addr_t)(ptr_t)&buf[offset];
            cur_plane->len = vbuf->planes[j].len;

            q->bufs[i].planes[j].buf = cur_plane->buf;
            q->bufs[i].planes[j].length = cur_plane->len;
            q->bufs[i].planes[j].bytesused = cur_plane->len;

            offset += cur_plane->len;
            cur_plane++;
        }
        q->bufs[i].index = i;
        q->bufs[i].queue = q;
        q->bufs[i].num_planes = vbuf->num_planes;
        q->bufs[i].state = VB_BUF_STATE_DEQUEUED;
    }
    q->num_buffers = i;
    vbuf->num_buffers = i;
    pr_debug("Requested %d buffers(size %d)\n", i, one_buf_size);
    return 0;
}

/* Enqueue a vb_buffer in driver for processing */
static void __enqueue_in_driver(struct vb_buffer *vb)
{
    if (vb->state == VB_BUF_STATE_ACTIVE)
        return;

    vb->state = VB_BUF_STATE_ACTIVE;
    if (g_vb_ops->buf_queue)
        g_vb_ops->buf_queue(vb);
}

int vin_vb_q_buf(struct vb_queue *q, u32 index)
{
    struct vb_buffer *vb = NULL;
    rt_base_t level = 0;

    if (q->error) {
        pr_err("Fatal error occurred on queue\n");
        return -EIO;
    }

    if (!g_vb_ops) {
        pr_err("Should init VideoBuf ops first!\n");
        return -EPERM;
    }

    vb = &q->bufs[index];
    if (vb->state != VB_BUF_STATE_DEQUEUED) {
        pr_err("buffer %d state is invalid: %s\n",
               vb->index, vb_state_name(vb->state));
        return -EINVAL;
    }

    /* Add to the queued buffers list. It will stay on it until dequeued */
    level = rt_hw_interrupt_disable();
    list_add_tail(&vb->queued_entry, &q->queued_list);
    q->queued_count++;
    vb->state = VB_BUF_STATE_QUEUED;

    /*
     * If already streaming, give the buffer to driver for processing.
     * If not, the buffer will be given to driver on next streamon.
     */
    __enqueue_in_driver(vb);
    q->owned_by_drv_count++;
    rt_hw_interrupt_enable(level);

    if (q->streaming)
        vin_vb_stream_on(q);

    pr_debug("Qbuf buffer %d [%d]\n", vb->index, q->queued_count);
    return 0;
}

static int __wait_for_done_vb(struct vb_queue *q)
{
    if (!q->streaming) {
        pr_err("streaming off, will not wait for buffers\n");
        return -EINVAL;
    }

    if (q->error) {
        pr_err("Queue in error state, will not wait for buffers\n");
        return -EIO;
    }

    if (aicos_sem_take(q->done, 60000) < 0)  /* Wait 1 min */
        return -EBUSY;

    if (!list_empty(&q->done_list)) {
        /* Found a buffer that we were waiting for.*/
        pr_debug("done_list is not empty after tried %d times!\n", i + 1);
        return 0;
    }

    return -EBUSY;
}

int vin_vb_dq_buf(struct vb_queue *q, u32 *pindex)
{
    int ret = 0;
    struct vb_buffer *vb = NULL;
    rt_base_t level = 0;

    ret = __wait_for_done_vb(q);
    if (ret)
        return ret;

    level = rt_hw_interrupt_disable();

    vb = list_first_entry(&q->done_list, struct vb_buffer, done_entry);
    list_del(&vb->done_entry);

    pr_debug("DQ buf %d [%d], state: %s\n",
             vb->index, q->queued_count, vb_state_name(vb->state));
    switch (vb->state) {
    case VB_BUF_STATE_DONE:
        break;
    case VB_BUF_STATE_ERROR:
        pr_err("Get done buffer %d with errors\n", vb->index);
        break;
    default:
        pr_err("Buffer %d state: %s\n", vb->index, vb_state_name(vb->state));
        return -EINVAL;
    }

    *pindex = vb->index;

    /* Remove from videobuf queue */
    list_del(&vb->queued_entry);
    q->queued_count--;

    rt_hw_interrupt_enable(level);

    vb->state = VB_BUF_STATE_DEQUEUED;
    return 0;
}

void vin_vb_buffer_done(struct vb_buffer *vb, enum vb_buffer_state state)
{
    struct vb_queue *q = vb->queue;

    if (vb->state != VB_BUF_STATE_ACTIVE) {
        pr_warn("Invalid buffer %d state: %s\n",
                vb->index, vb_state_name(vb->state));
        return;
    }

    if (state != VB_BUF_STATE_DONE &&
            state != VB_BUF_STATE_ERROR &&
            state != VB_BUF_STATE_QUEUED) {
        pr_warn("Change buffer %d state to %s\n",
                vb->index, vb_state_name(state));
        state = VB_BUF_STATE_ERROR;
    }

    pr_debug("Buffer %d done, state: %s\n\n", vb->index, vb_state_name(state));

    if (state == VB_BUF_STATE_ERROR) {
        /* The active vb just happened some error, so ignore the data in buf.
           The DVP driver should reuse it */
        vb->state = VB_BUF_STATE_QUEUED;
        __enqueue_in_driver(vb);
        return;
    }

    if (state == VB_BUF_STATE_QUEUED) {
        /* Reclaim the buf from DVP driver to queued_list */
        vb->state = VB_BUF_STATE_QUEUED;
    } else {
        /* Add the buffer to the done buffers list */
        list_add_tail(&vb->done_entry, &q->done_list);
        aicos_sem_give(q->done);
        vb->state = state;
    }

    q->owned_by_drv_count--;
}

int vin_vb_stream_on(struct vb_queue *q)
{
    struct vb_buffer *vb;
    int ret;

    if (q->streaming)
        return 0;

    if (!q->num_buffers) {
        pr_err("no buffers have been allocated\n");
        return -EINVAL;
    }

    if (!g_vb_ops) {
        pr_err("Should init VideoBuf ops first!\n");
        return -EPERM;
    }

    /* If any buffers were queued before streamon, pass them to driver */
    list_for_each_entry(vb, &q->queued_list, queued_entry)
        __enqueue_in_driver(vb);

    /* Tell the driver to start streaming */
    q->streaming = 1;
    ret = g_vb_ops->start_streaming(q);
    if (!ret)
        return 0;

    pr_err("driver refused to start streaming\n");

    /* The buffers should be returned to vb from driver */
    if (q->owned_by_drv_count) {
        u32  i;

        /* Forcefully reclaim buffers */
        for (i = 0; i < q->num_buffers; ++i) {
            vb = &q->bufs[i];
            if (vb->state == VB_BUF_STATE_ACTIVE)
                vin_vb_buffer_done(vb, VB_BUF_STATE_QUEUED);
        }

        /* Must be zero now */
        if (q->owned_by_drv_count)
            pr_warn("Driver did hold %d buf\n", q->owned_by_drv_count);
    }

    if (!list_empty(&q->done_list))
        pr_warn("done_list is not empty!\n");

    return ret;
}

/* Removes all queued buffers from driver's queue and
 * all buffers queued by user from videobuf's queue */
int vin_vb_stream_off(struct vb_queue *q)
{
    u32 i;

    if (!g_vb_ops) {
        pr_err("Should init VideoBuf ops first!\n");
        return -EPERM;
    }

    /* Tell driver to stop all transactions and release all queued buffers. */
    if (g_vb_ops->stop_streaming)
        g_vb_ops->stop_streaming(q);

    if (q->owned_by_drv_count) {
        pr_warn("owned_by_drv_count is %d\n", q->owned_by_drv_count);

        for (i = 0; i < q->num_buffers; ++i)
            if (q->bufs[i].state == VB_BUF_STATE_ACTIVE) {
                pr_warn("stop_streaming operation is leaving buf %d in active state\n", i);
                vin_vb_buffer_done(&q->bufs[i], VB_BUF_STATE_ERROR);
            }

        /* Must be zero now */
        if (q->owned_by_drv_count)
            pr_warn("owned_by_drv_count should be 0, but %d\n",
                    q->owned_by_drv_count);
    }

    q->streaming = 0;
    q->queued_count = 0;
    q->error = 0;

    /* Remove all buffers from videobuf's list... */
    INIT_LIST_HEAD(&q->queued_list);

    /* ...and done list;
     * user will not receive any buffers it has not already dequeued */
    INIT_LIST_HEAD(&q->done_list);

    q->owned_by_drv_count = 0;

    /* Reinitialize all buffers for next use. */
    for (i = 0; i < q->num_buffers; ++i)
        q->bufs[i].state = VB_BUF_STATE_DEQUEUED;

    aicos_sem_reset(q->done, 0);

    pr_debug("Stop steaming successfully\n");
    return 0;
}

int vin_vb_init(struct vb_queue *q, const struct vb_ops *ops)
{
    if (!q || !ops) {
        pr_err("Invalid parameter: q %#lx, ops %#lx\n", (long)q, (long)ops);
        return -1;
    }

    memset(q, 0, sizeof(struct vb_queue));
    INIT_LIST_HEAD(&q->queued_list);
    INIT_LIST_HEAD(&q->done_list);

    q->done = aicos_sem_create(0);
    g_vb_ops = ops;
    return 0;
}

void vin_vb_deinit(struct vb_queue *q)
{
    if (!q) {
        pr_err("Invalid parameter: q %#lx\n", (long)q);
        return;
    }

    aicos_sem_delete(q->done);
    g_vb_ops = NULL;
}

void vin_vb_show_info(struct vb_queue *q)
{
    struct vb_buffer *vb = NULL;

    printf("In VIN buffer queue: total %d\n", q->num_buffers);
    printf("Stream State : %s\n", q->streaming ? "StreamOn" : "StreamOff");
    printf("Buf has error: %d\n", q->error);

    printf("Queued list  : %d [", q->queued_count);
    list_for_each_entry(vb, &q->queued_list, queued_entry)
        printf("%d ", vb->index);
    printf("]\n");

    printf("Done list    : [");
    list_for_each_entry(vb, &q->done_list, done_entry)
        printf("%d ", vb->index);
    printf("]\n");

    printf("Owned by drv : %d\n", q->owned_by_drv_count);
}
