
/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "aic_profiler.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdatomic.h>

#if defined(__linux__)
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(KERNEL_RTTHREAD)
#include <aic_core.h>
#endif

#define AIC_PROFILER_IRQ_ID           0
#define AIC_PROFILER_TICK_PER_SEC_MAX 1000000

#ifndef AIC_PROFILER_BUF_SIZE
#define AIC_PROFILER_BUF_SIZE         (512 * 1024)
#endif

#ifndef AIC_PROFILER_OUTPUT_FILENAME
#define AIC_PROFILER_OUTPUT_FILENAME "/udisk/profiler.systrace"
#endif

#ifndef AIC_PROFILER_IRQ_MODE
#define AIC_PROFILER_IRQ_MODE 0
#endif

#if defined __linux__ || defined KERNEL_RTTHREAD
    #define AIC_PROFILER_MUTEX_INIT   pthread_mutex_init(&profiler_ctx->mutex, NULL)
    #define AIC_PROFILER_MUTEX_DEINIT pthread_mutex_destroy(&profiler_ctx->mutex)
    #define AIC_PROFILER_MUTEX_LOCK   pthread_mutex_lock(&profiler_ctx->mutex)
    #define AIC_PROFILER_MUTEX_UNLOCK pthread_mutex_unlock(&profiler_ctx->mutex)
#else
    #define AIC_PROFILER_MUTEX_INIT
    #define AIC_PROFILER_MUTEX_DEINIT
    #define AIC_PROFILER_MUTEX_LOCK
    #define AIC_PROFILER_MUTEX_UNLOCK
#endif

typedef struct {
    char tag;          /**< The tag of the profiler item */
    uint32_t tick;     /**< The tick value of the profiler item */
    const char * func; /**< A pointer to the function associated with the profiler item */
    int tid;           /**< The thread ID of the profiler item */
    int cpu;         /**< The CPU ID of the profiler item */
} aic_profiler_item_t;

typedef struct _aic_profiler_ctx_t {
    aic_profiler_item_t * item_arr[2]; /**< Pointer to an array of profiler items */
    uint32_t item_num;        /**< Number of profiler items in the array */
    _Atomic uint32_t cur_index[2];    /**< Index of the current profiler item */
    _Atomic uint32_t online_buf;
    _Atomic uint32_t offline_buf;
    aic_profiler_config_t config;   /**< Configuration for the built-in profiler */
    bool enable;                           /**< Whether the built-in profiler is enabled */
    FILE *file;
#if defined __linux__ || defined KERNEL_RTTHREAD
    pthread_mutex_t mutex;                      /**< Mutex to protect the built-in profiler */
    pthread_t flush_thread;
    pthread_cond_t cond;
#endif
    bool thread_running;
    _Atomic bool flush_pending;
    uint32_t drop_count;
} aic_profiler_ctx_t;

static void * malloc_zeroed(size_t size);
static void force_systrace_suffix(char *filename, size_t max_len);
static void format_profiler_item(char *buf, size_t buf_size, const aic_profiler_item_t *item, uint32_t tick_per_sec);
static void flush_items(aic_profiler_ctx_t *ctx, uint32_t buf_index, uint32_t item_count);
static void flush_no_lock(void);

static uint32_t default_tick_get_cb(void);
static void default_flush_cb(const char * buf);
static int default_tid_get_cb(void);
static int default_cpu_get_cb(void);

static aic_profiler_ctx_t *profiler_ctx;

#if defined __linux__ || defined KERNEL_RTTHREAD
static void * flush_thread_func(void * arg)
{
    aic_profiler_ctx_t * ctx = (aic_profiler_ctx_t *)arg;

    while (ctx->thread_running) {
        pthread_mutex_lock(&ctx->mutex);
        pthread_cond_wait(&ctx->cond, &ctx->mutex);

        bool flush_pending = atomic_load(&ctx->flush_pending);
        if (flush_pending) {
            uint32_t flush_buf = atomic_load(&ctx->offline_buf);
            uint32_t item_count = atomic_load(&ctx->cur_index[flush_buf]);

            flush_items(ctx, flush_buf, item_count);

            atomic_store(&ctx->flush_pending, false);
            atomic_store(&ctx->cur_index[flush_buf], 0);
        }

        pthread_mutex_unlock(&ctx->mutex);
    }
    return NULL;
}
#endif

void aic_profiler_config_init(aic_profiler_config_t * config)
{
    if (!config)
        return;

    memset(config, 0, sizeof(aic_profiler_config_t));
    strncpy(config->filename, AIC_PROFILER_OUTPUT_FILENAME, AIC_PROFILER_STR_MAX_LEN);
#if AIC_PROFILER_IRQ_MODE
    config->irq_mode = 1;
#endif
    config->buf_size = AIC_PROFILER_BUF_SIZE;
    config->tick_per_sec = 1000000;
    config->tick_get_cb = default_tick_get_cb;
    config->flush_cb = default_flush_cb;
    config->tid_get_cb = default_tid_get_cb;
    config->cpu_get_cb = default_cpu_get_cb;
}

void aic_profiler_init(const aic_profiler_config_t * config)
{
    if(config == NULL || config->tick_get_cb == NULL) {
        printf("Invalid config or tick_get_cb\n");
        return;
    }

    uint32_t num = config->buf_size / sizeof(aic_profiler_item_t);
    if(num == 0) {
        printf("buf_size must > %d\n", (int)sizeof(aic_profiler_item_t));
        return;
    }

    if(config->tick_per_sec == 0 || config->tick_per_sec > AIC_PROFILER_TICK_PER_SEC_MAX) {
        printf("tick_per_sec range must be between 1~%d\n", AIC_PROFILER_TICK_PER_SEC_MAX);
        return;
    }

    /*Free the old item_arr memory*/
    if(profiler_ctx) {
        aic_profiler_uninit();
    }

    profiler_ctx = malloc_zeroed(sizeof(aic_profiler_ctx_t));
    if (!profiler_ctx) {
        printf("malloc failed for profiler_ctx\n");
        return;
    }

    char file_name[AIC_PROFILER_STR_MAX_LEN] = {0};
    snprintf(file_name, AIC_PROFILER_STR_MAX_LEN, "%s", config->filename);
    force_systrace_suffix(file_name, sizeof(file_name));
    snprintf(profiler_ctx->config.filename, AIC_PROFILER_STR_MAX_LEN, "%s", file_name);
    profiler_ctx->file = fopen(profiler_ctx->config.filename, "w");
    if (!profiler_ctx->file) {
        printf("Failed to open log file, file: %s\n", profiler_ctx->config.filename);
    }
    printf("Note: Finally, upload the generated file %s to website <https://ui.perfetto.dev/> for viewing.\n", profiler_ctx->config.filename);
#if defined(KERNEL_RTTHREAD)
    profiler_ctx->item_arr[0] = aicos_malloc(MEM_CMA, num * sizeof(aic_profiler_item_t));
#else
    profiler_ctx->item_arr[0] = malloc(num * sizeof(aic_profiler_item_t));
#endif
    if(profiler_ctx->item_arr[0] == NULL) {
        free(profiler_ctx);
        profiler_ctx = NULL;
        printf("malloc failed for item_arr\n");
        return;
    }

    if (config->irq_mode) {
    #if defined(KERNEL_RTTHREAD)
        profiler_ctx->item_arr[1] = aicos_malloc(MEM_CMA, num * sizeof(aic_profiler_item_t));
    #else
        profiler_ctx->item_arr[1] = malloc(num * sizeof(aic_profiler_item_t));
    #endif
        if(profiler_ctx->item_arr[1] == NULL) {
#if defined(KERNEL_RTTHREAD)
            aicos_free(MEM_CMA, profiler_ctx->item_arr[0]);
#else
            free(profiler_ctx->item_arr[0]);
#endif
            free(profiler_ctx);
            profiler_ctx = NULL;
            printf("malloc failed for item_arr\n");
            return;
        }
    }

    AIC_PROFILER_MUTEX_INIT;
    profiler_ctx->item_num = num;
    profiler_ctx->config = *config;

    if(profiler_ctx->config.flush_cb) {
        /* add profiler header for perfetto */
        profiler_ctx->config.flush_cb("# tracer: nop\n");
        profiler_ctx->config.flush_cb("#\n");
    }

    aic_profiler_set_enable(true);

#if defined __linux__ || defined KERNEL_RTTHREAD
    if (config->irq_mode) {
        pthread_cond_init(&profiler_ctx->cond, NULL);
        profiler_ctx->thread_running = true;
        pthread_create(&profiler_ctx->flush_thread, NULL, flush_thread_func, profiler_ctx);
    }
#endif
}

void aic_profiler_uninit(void)
{
    if (!profiler_ctx)
        return;

    if (profiler_ctx->file) {
        AIC_PROFILER_MUTEX_LOCK;
        flush_no_lock();
        AIC_PROFILER_MUTEX_UNLOCK;

        fclose(profiler_ctx->file);
        profiler_ctx->file = NULL;
    }

    AIC_PROFILER_MUTEX_DEINIT;
#if defined __linux__ || defined KERNEL_RTTHREAD
    if (profiler_ctx->config.irq_mode) {
        profiler_ctx->thread_running = false;
        pthread_cond_signal(&profiler_ctx->cond);
        pthread_join(profiler_ctx->flush_thread, NULL);
        pthread_cond_destroy(&profiler_ctx->cond);
    }
#endif

#if defined(KERNEL_RTTHREAD)
    aicos_free(MEM_CMA, profiler_ctx->item_arr[0]);
    if (profiler_ctx->config.irq_mode) {
        aicos_free(MEM_CMA, profiler_ctx->item_arr[1]);
    }
#else
    free(profiler_ctx->item_arr[0]);
    if (profiler_ctx->config.irq_mode) {
        free(profiler_ctx->item_arr[1]);
    }
#endif
    free(profiler_ctx);
    profiler_ctx = NULL;
}

void aic_profiler_set_enable(bool enable)
{
    if(!profiler_ctx) {
        return;
    }

    profiler_ctx->enable = enable;
}

void aic_profiler_flush(void)
{
    if (!profiler_ctx)
        return;

    if (profiler_ctx->config.irq_mode) {
#if defined __linux__ || defined KERNEL_RTTHREAD
        while (atomic_load(&profiler_ctx->flush_pending)) {
            usleep(1000);
        }
#endif
    }

    AIC_PROFILER_MUTEX_LOCK;
    flush_no_lock();
    AIC_PROFILER_MUTEX_UNLOCK;
}

void aic_profiler_write(const char * func, char tag)
{
    if (!func || !profiler_ctx)
        return;

    if(!profiler_ctx->enable) {
        return;
    }

    AIC_PROFILER_MUTEX_LOCK;

    uint32_t online_buf = profiler_ctx->online_buf;
    uint32_t cur_index = profiler_ctx->cur_index[online_buf];
    if(cur_index >= profiler_ctx->item_num) {
        flush_no_lock();
    }

    aic_profiler_item_t * item = &profiler_ctx->item_arr[online_buf][cur_index];
    item->func = func;
    item->tag = tag;
    item->tick = profiler_ctx->config.tick_get_cb();

#if defined __linux__ || defined KERNEL_RTTHREAD
    item->tid = profiler_ctx->config.tid_get_cb();
    item->cpu = profiler_ctx->config.cpu_get_cb();
#endif

    profiler_ctx->cur_index[online_buf]++;

    AIC_PROFILER_MUTEX_UNLOCK;
}

void aic_profiler_irq_write(const char * func, char tag)
{
    if (!func || !tag || !profiler_ctx || !profiler_ctx->enable) {
        return;
    }

    uint32_t online_buf = atomic_load(&profiler_ctx->online_buf);
    uint32_t cur_index = atomic_load(&profiler_ctx->cur_index[online_buf]);
    if (cur_index >= profiler_ctx->item_num) {
        uint32_t offline = 1 - online_buf;
        uint32_t offline_index = atomic_load(&profiler_ctx->cur_index[offline]);
        if (offline_index != 0) {
            atomic_fetch_add(&profiler_ctx->drop_count, 1);
            return;
        }

        atomic_store(&profiler_ctx->flush_pending, true);
        atomic_store(&profiler_ctx->offline_buf, online_buf);
        atomic_store(&profiler_ctx->online_buf, offline);
#if defined __linux__ || defined KERNEL_RTTHREAD
        pthread_cond_signal(&profiler_ctx->cond);
#endif
        online_buf = offline;
        cur_index = 0;
    }

    aic_profiler_item_t * item = &profiler_ctx->item_arr[online_buf][cur_index];
    item->func = func;
    item->tag = tag;
    item->tick = profiler_ctx->config.tick_get_cb();
    item->tid = AIC_PROFILER_IRQ_ID;
    item->cpu = AIC_PROFILER_IRQ_ID;

    atomic_fetch_add(&profiler_ctx->cur_index[online_buf], 1);
}

uint32_t aic_profiler_get_drop_count(void)
{
    return profiler_ctx->drop_count;
}
/**********************
 *   STATIC FUNCTIONS
 **********************/
static void * malloc_zeroed(size_t size)
{
    void * ptr = malloc(size);
    if(ptr != NULL) {
        memset(ptr, 0, size);
    }
    return ptr;
}

static void force_systrace_suffix(char *filename, size_t max_len)
{
    char *last_slash = strrchr(filename, '/');
#ifdef _WIN32
    char *last_backslash = strrchr(filename, '\\');
    if (last_backslash > last_slash) {
        last_slash = last_backslash;
    }
#endif

    char *basename = (last_slash != NULL) ? last_slash + 1 : filename;

    const char *suffix = ".systrace";
    size_t basename_len = strlen(basename);
    size_t suffix_len = strlen(suffix);

    if (basename_len >= suffix_len &&
        strcmp(basename + basename_len - suffix_len, suffix) == 0) {
        printf("Filename already has .systrace suffix: %s\n", filename);
        return;
    }

    char *dot = strrchr(basename, '.');
    if (dot != NULL) {
        *dot = '\0';
    }

    if (strlen(filename) + suffix_len < max_len) {
        strcat(filename, suffix);
        printf("Forced filename to have .systrace suffix: %s\n", filename);
    } else {
        printf("Warning: Filename too long, cannot add .systrace suffix: %s\n", filename);
    }
}

static void format_profiler_item(char *buf, size_t buf_size, const aic_profiler_item_t *item, uint32_t tick_per_sec)
{
#if defined __linux__ || defined KERNEL_RTTHREAD
    uint32_t sec = item->tick / tick_per_sec;
    uint32_t usec = (item->tick % tick_per_sec) * (AIC_PROFILER_TICK_PER_SEC_MAX / tick_per_sec);
    snprintf(buf, buf_size,
             "   AIC-%d [%d] %" PRIu32 ".%06" PRIu32 ": tracing_mark_write: %c|1|%s\n",
             item->tid,
             item->cpu,
             sec,
             usec,
             item->tag,
             item->func);
#else
    uint32_t sec = item->tick / tick_per_sec;
    uint32_t usec = (item->tick % tick_per_sec) * (AIC_PROFILER_TICK_PER_SEC_MAX / tick_per_sec);
    snprintf(buf, buf_size,
             "   AIC-1 [0] %" PRIu32 ".%06" PRIu32 ": tracing_mark_write: %c|1|%s\n",
             sec,
             usec,
             item->tag,
             item->func);
#endif
}

static void flush_items(aic_profiler_ctx_t *ctx, uint32_t buf_index, uint32_t item_count)
{
    if (item_count > 0) {
        char buf[AIC_PROFILER_STR_MAX_LEN];
        uint32_t tick_per_sec = ctx->config.tick_per_sec;

        for (uint32_t i = 0; i < item_count; i++) {
            aic_profiler_item_t * item = &ctx->item_arr[buf_index][i];
            format_profiler_item(buf, sizeof(buf), item, tick_per_sec);
            ctx->config.flush_cb(buf);
        }
    }
}

static void flush_no_lock(void)
{
    if(!profiler_ctx->config.flush_cb) {
        printf("flush_cb is not registered");
        return;
    }

    uint32_t online_buf = profiler_ctx->online_buf;
    uint32_t cur_index = profiler_ctx->cur_index[online_buf];

    flush_items(profiler_ctx, online_buf, cur_index);
    profiler_ctx->cur_index[online_buf] = 0;
}

static uint32_t default_tick_get_cb(void)
{
#if defined(__linux__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)ts.tv_sec * 1000000UL + ts.tv_nsec / 1000;
#elif defined(KERNEL_RTTHREAD)
    return (uint32_t)aic_get_time_us();
#endif
}

static void default_flush_cb(const char * buf)
{
    if (profiler_ctx->file) {
        fprintf(profiler_ctx->file, "%s", buf);
    } else {
        printf("%s\n", buf);
    }
}

static int default_tid_get_cb(void)
{
#if defined(__linux__)
    return (int)syscall(SYS_gettid);
#elif defined KERNEL_RTTHREAD
    return (int)((int)rt_thread_self() & 0xFFFFFFFF);
#else
    return 1;
#endif
}

static int default_cpu_get_cb(void)
{
#if defined __linux__
    int cpu_id = 0;
    syscall(SYS_getcpu, &cpu_id, NULL);
    return cpu_id;
#elif defined KERNEL_RTTHREAD
    return 1;
#else
    return 1;
#endif
}
