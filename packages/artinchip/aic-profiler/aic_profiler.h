/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define AIC_PROFILER_STR_MAX_LEN 256

#define AIC_PROFILER_BEGIN_TAG(tag)  aic_profiler_write((tag), 'B')
#define AIC_PROFILER_END_TAG(tag)    aic_profiler_write((tag), 'E')
#define AIC_PROFILER_BEGIN           AIC_PROFILER_BEGIN_TAG(__func__)
#define AIC_PROFILER_END             AIC_PROFILER_END_TAG(__func__)

#define AIC_PROFILER_IRQ_BEGIN_TAG(tag)    aic_profiler_irq_write((tag), 'B')
#define AIC_PROFILER_IRQ_END_TAG(tag)      aic_profiler_irq_write((tag), 'E')
#define AIC_PROFILER_IRQ_BEGIN             AIC_PROFILER_IRQ_BEGIN_TAG(__func__)
#define AIC_PROFILER_IRQ_END               AIC_PROFILER_IRQ_END_TAG(__func__)

typedef struct {
    char filename[AIC_PROFILER_STR_MAX_LEN];
    bool irq_mode;
    size_t buf_size;                    /**< The size of the buffer used for profiling data */
    uint32_t tick_per_sec;              /**< The number of ticks per second */
    uint32_t (*tick_get_cb)(void);      /**< Callback function to get the current tick count */
    void (*flush_cb)(const char * buf); /**< Callback function to flush the profiling data */
    int (*tid_get_cb)(void);            /**< Callback function to get the current thread ID */
    int (*cpu_get_cb)(void);            /**< Callback function to get the current CPU */
} aic_profiler_config_t;

void aic_profiler_config_init(aic_profiler_config_t * config);

void aic_profiler_init(const aic_profiler_config_t * config);

void aic_profiler_uninit(void);

void aic_profiler_set_enable(bool enable);

void aic_profiler_flush(void);

void aic_profiler_write(const char * func, char tag);

void aic_profiler_irq_write(const char * func, char tag);

uint32_t aic_profiler_get_drop_count(void);
#ifdef __cplusplus
} /*extern "C"*/
#endif
