/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "aic_core.h"

#include "bouncebuf.h"

#define BOUNCE_BUF_ALIGN     CACHE_LINE_SIZE

#define debug   pr_debug

static int bounce_buffer_addr_aligned(struct bounce_buffer *state)
{
    const ulong align_mask = BOUNCE_BUF_ALIGN - 1;

    /* Check if start is aligned */
    if ((ulong)state->user_buffer & align_mask) {
        pr_debug("Unaligned buffer address 0x%x\n", (u32)(ptr_t)state->user_buffer);
        return 0;
    }

    /* Check if length is aligned */
    if (state->len != state->len_aligned) {
        pr_debug("Unaligned buffer length %zu\n", state->len);
        return 0;
    }

    /* Aligned */
    return 1;
}

int bounce_buffer_start_extalign(struct bounce_buffer *state, void *data,
                                 size_t len, unsigned int flags,
                                 size_t alignment,
                                 int (*addr_is_aligned)(struct bounce_buffer *state))
{
    state->user_buffer = data;
    state->bounce_buffer = data;
    state->len = len;
    state->len_aligned = roundup(len, alignment);
    state->flags = flags;

    if (!addr_is_aligned(state)) {
        state->bounce_buffer = aicos_malloc_align(0, state->len_aligned, alignment);
        if (!state->bounce_buffer)
            return -1;

        if (state->flags & GEN_BB_READ)
            memcpy(state->bounce_buffer, state->user_buffer,
                   state->len);
    }

    /*
     * Flush data to RAM so DMA reads can pick it up,
     * and any CPU writebacks don't race with DMA writes
     */
    aicos_dcache_clean_invalid_range((unsigned long *)state->bounce_buffer,
                                     state->len_aligned);

    return 0;
}

int bounce_buffer_start(struct bounce_buffer *state, void *data,
                        size_t len, unsigned int flags)
{
    return bounce_buffer_start_extalign(state, data, len, flags,
                                        BOUNCE_BUF_ALIGN,
                                        bounce_buffer_addr_aligned);
}

int bounce_buffer_stop(struct bounce_buffer *state)
{
    if (state->flags & GEN_BB_WRITE) {
        aicos_dma_sync();
        /* Invalidate cache so that CPU can see any newly DMA'd data */
        aicos_dcache_invalid_range((unsigned long *)state->bounce_buffer,
                                   state->len_aligned);
    }

    if (state->bounce_buffer == state->user_buffer)
        return 0;

    if (state->flags & GEN_BB_WRITE)
        memcpy(state->user_buffer, state->bounce_buffer, state->len);

    aicos_free_align(0, state->bounce_buffer);

    return 0;
}
