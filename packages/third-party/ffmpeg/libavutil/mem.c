/*
 * default memory allocator for libavutil
 * Copyright (c) 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * default memory allocator for libavutil
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "mem.h"

void *av_malloc(size_t size)
{
    void *ptr;
    if (size > INT_MAX)
        return NULL;
    ptr = malloc(size);

    return ptr;
}

void *av_mallocz(size_t size)
{
    void *ptr;

    ptr = av_malloc(size);
    if (!ptr)
        return NULL;
    memset(ptr, 0, size);
    return ptr;
}

void *av_realloc(void *ptr, size_t size)
{
    if (size > INT_MAX)
        return NULL;

    return realloc(ptr, size);
}

void *av_realloc_f(void *ptr, size_t nelem, size_t elsize)
{
    size_t size;
    void *r;

    if (av_size_mult(elsize, nelem, &size)) {
        av_free(ptr);
        return NULL;
    }
    r = av_realloc(ptr, size);
    if (!r)
        av_free(ptr);
    return r;
}


int av_reallocp(void *ptr, size_t size)
{
    void *val;

    if (!size) {
        av_freep(ptr);
        return 0;
    }

    memcpy(&val, ptr, sizeof(val));
    val = av_realloc(val, size);

    if (!val) {
        av_freep(ptr);
        return -1;
    }

    memcpy(ptr, &val, sizeof(val));
    return 0;
}


void *av_malloc_array(size_t nmemb, size_t size)
{
    size_t result;
    if (av_size_mult(nmemb, size, &result) < 0)
        return NULL;
    return av_malloc(result);
}

void *av_mallocz_array(size_t nmemb, size_t size)
{
    size_t result;
    if (av_size_mult(nmemb, size, &result) < 0)
        return NULL;
    return av_mallocz(result);
}

void *av_realloc_array(void *ptr, size_t nmemb, size_t size)
{
    size_t result;
    if (av_size_mult(nmemb, size, &result) < 0)
        return NULL;
    return av_realloc(ptr, result);
}

int av_reallocp_array(void *ptr, size_t nmemb, size_t size)
{
    void *val;

    memcpy(&val, ptr, sizeof(val));
    val = av_realloc_f(val, nmemb, size);
    memcpy(ptr, &val, sizeof(val));
    if (!val && nmemb && size)
        return -1;

    return 0;
}

void av_free(void *ptr)
{
    free(ptr);
}

void av_freep(void *arg)
{
    void *val;

    memcpy(&val, arg, sizeof(val));
    memcpy(arg, &(void *){ NULL }, sizeof(val));
    av_free(val);
}
