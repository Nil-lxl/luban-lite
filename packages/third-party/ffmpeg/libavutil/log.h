/*
 * copyright (c) 2006 Michael Niedermayer <michaelni@gmx.at>
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

#ifndef AVUTIL_LOG_H
#define AVUTIL_LOG_H

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>


#define AV_LOGL_DEFAULT AV_LOG_INFO

/**
 * Print no output.
 */
#define AV_LOG_QUIET    -8

/**
 * Something went really wrong and we will crash now.
 */
#define AV_LOG_PANIC     0

/**
 * Something went wrong and recovery is not possible.
 * For example, no header was found for a format which depends
 * on headers or an illegal combination of parameters is used.
 */
#define AV_LOG_FATAL     8

/**
 * Something went wrong and cannot losslessly be recovered.
 * However, not all future data is affected.
 */
#define AV_LOG_ERROR    16

/**
 * Something somehow does not look correct. This may or may not
 * lead to problems. An example would be the use of '-vstrict -2'.
 */
#define AV_LOG_WARNING  24

/**
 * Standard information.
 */
#define AV_LOG_INFO     32

/**
 * Detailed information.
 */
#define AV_LOG_VERBOSE  40

/**
 * Stuff which is only useful for libav* developers.
 */
#define AV_LOG_DEBUG    48

/**
 * Extremely verbose debugging, useful for libav* development.
 */
#define AV_LOG_TRACE    56

#define AV_LOG_MAX_OFFSET (AV_LOG_TRACE - AV_LOG_QUIET)


/*avoid  redefine warning */
#ifdef AV_LOG_TAG
#undef AV_LOG_TAG
#define AV_LOG_TAG "av"
#else
#define AV_LOG_TAG "av"
#endif

#define AV_LOG_QUIET_STR    "quiet  "
#define AV_LOG_PANIC_STR    "panic  "
#define AV_TAG_FATAL_STR    "fatal  "
#define AV_TAG_ERROR_STR    "error  "
#define AV_TAG_WARNING_STR  "warning"
#define AV_TAG_INFO_STR     "info   "
#define AV_TAG_DEBUG_STR    "debug  "
#define AV_TAG_VERBOSE_STR  "verbose"

#define av_print(level, tag, fmt, arg...) ({                                              \
    int _l = level;                                                                       \
    if (_l <= AV_LOGL_DEFAULT)                                                            \
        printf("%s: %s <%s:%d>: " fmt "\n", tag, AV_LOG_TAG, __FUNCTION__, __LINE__, ##arg); \
})


#define av_loge(fmt, arg...) av_print(AV_LOG_ERROR, AV_TAG_ERROR_STR, "\033[40;31m"fmt"\033[0m", ##arg)
#define av_logw(fmt, arg...) av_print(AV_LOG_WARNING, AV_TAG_WARNING_STR, "\033[40;33m"fmt"\033[0m", ##arg)
#define av_logi(fmt, arg...) av_print(AV_LOG_INFO, AV_TAG_INFO_STR, "\033[40;32m"fmt"\033[0m", ##arg)
#define av_logd(fmt, arg...) av_print(AV_LOG_DEBUG, AV_TAG_DEBUG_STR, fmt, ##arg)
#define av_logv(fmt, arg...) av_print(AV_LOG_VERBOSE, AV_TAG_VERBOSE_STR, fmt, ##arg)

 #define av_log(avcl, level, fmt, arg...) ({                                              \
    int _l = level;                                                                       \
    if (_l <= AV_LOG_ERROR)                                                               \
        av_loge(fmt, ##arg);                                                              \
    else if (_l <= AV_LOG_WARNING)                                                        \
        av_logw(fmt, ##arg);                                                              \
    else if (_l <= AV_LOG_INFO)                                                           \
        av_logi(fmt, ##arg);                                                              \
    else if (_l <= AV_LOG_DEBUG)                                                          \
        av_logd(fmt, ##arg);                                                              \
    else                                                                                  \
        av_logv(fmt, ##arg);                                                              \
})

//void av_log(void* avcl, int level, const char *fmt, ...);

#define ff_tlog(ctx, ...) av_log(ctx, AV_LOG_TRACE, __VA_ARGS__)
#define ff_dlog(ctx, ...) av_log(ctx, AV_LOG_DEBUG, __VA_ARGS__)


#endif














