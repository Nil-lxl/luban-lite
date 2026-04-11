/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "player_backend_ops.h"
#include "aic_ui.h"

#if LV_USE_AIC_SIMULATOR == 0
#include <string.h>
#include "mpp_decoder.h"
#include "mpp_ge.h"
#include "aic_player.h"

#include "frame_allocator.h"
#include "backend_common.h"

#define PNG_SIGNATURE_SIZE 8
#define PNG_IHDR_CHINK_SIZE 25
#define PNG_FCTL_CHUNK_SIZE 38
#define PNG_IEND_CHUNK_SIZE 12
#define IS_FFMPEG_CONVERTED_CHUNK(chunk) ((chunk) && (chunk)->length == 6)

/**********************
 *      DEFINITIONS
 **********************/
#define PNG_DUMPING_IMAGE 0

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    APNG_BLEND_OP_SOURCE = 0,
    APNG_BLEND_OP_OVER = 1,
} APNG_Blend_Op;

typedef enum {
    APNG_DISPOSE_OP_NONE = 0,
    APNG_DISPOSE_OP_BACKGROUND = 1,
    APNG_DISPOSE_OP_PREVIOUS = 2,
} APNG_Dispose_Op;

typedef struct {
    uint32_t length;
    uint8_t type[4];
    uint8_t *data;
    uint32_t crc;
} PNG_Chunk;

typedef struct {
    uint32_t serial_number;
    uint32_t width;
    uint32_t height;
    uint32_t x;
    uint32_t y;
    uint16_t delay_num;
    uint16_t delay_den;
    uint8_t blend_op;
    uint8_t dispose_op;
} fcTL_Chunk;

typedef struct {
    uint8_t has_alpha;
    uint8_t has_blend_over;
    uint32_t *frame_chunks_length;
    uint32_t max_chunk_size;
} Private_Chunk_dcTL;

typedef struct {
    uint32_t stride;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint8_t *data;
} PNG_Bitmap;

struct png_player_ctx {
    /* Memory management */
    struct frame_allocator allocator;
    uint8_t allocated_frame_count;

    struct av_media_info media_info;

    /* File handling */
    lv_fs_file_t file;

    /* Image information */
    PNG_Bitmap bitmap;

    /* Playback control */
    uint32_t last_frame_time;
    uint32_t status;
    float playback_rate;

    /* Frame management */
    uint32_t num_frames;
    uint32_t num_plays;
    uint32_t frame_count;
    uint32_t frame_duration;
    uint32_t first_frame_pos;

    /* Decoder related */
    struct mpp_decoder *dec;
    uint32_t package_size;
    uint8_t *addr[2];
    uint8_t *align_addr[2];
    uint8_t *image_data;
    void *image_src;
    struct mpp_buf buf; /* only used by v8 for drawing images */

    uint8_t is_normal_png;

    /* PNG chunks */
    PNG_Chunk ihdr;
    Private_Chunk_dcTL dcTL;
    PNG_Chunk *aux_chunks; // Auxiliary blocks before the first IDAT (PLTE/tRNS, etc.)
    uint32_t aux_chunks_num;
    uint32_t aux_chunks_size;
};

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void *png_player_backend_create(void);
static void png_player_backend_destroy(void *ctx);
static lv_res_t png_player_backend_set_src(void *ctx, const char *src);
static lv_res_t png_player_backend_control(void *ctx, player_cmd_t cmd, void *data);

/* PNG parsing functions */
static uint32_t png_get_be32(const unsigned char *buffer);
static uint16_t png_get_be16(const unsigned char *buffer);
static uint32_t png_to_be32(uint32_t host_int);
static long png_read_chunk(struct png_player_ctx *ctx, PNG_Chunk *chunk);
static size_t png_write_chunk(uint8_t *dest, PNG_Chunk *chunk);
static void png_parse_dctl(Private_Chunk_dcTL *dcTL, PNG_Chunk *chunk);
static void png_parse_actl(uint32_t *num_frames, uint32_t *num_plays, PNG_Chunk *chunk);
static void png_parse_fctl(fcTL_Chunk *fcTL, PNG_Chunk *chunk);

static lv_res_t png_read_fctl_chunk(struct png_player_ctx *ctx, fcTL_Chunk *fcTL_chunk, PNG_Chunk *chunk);
static lv_res_t png_peek_idats_chunk_header(struct png_player_ctx *ctx, PNG_Chunk *chunk);

static bool png_should_cache_auxiliary_chunk(const char *chunk_type);
static void png_scan_apng_header(struct png_player_ctx *ctx);

/* Frame processing functions */
static lv_res_t process_normal_png_frame(struct png_player_ctx *ctx);
static lv_res_t process_apng_frame(struct png_player_ctx *ctx, uint32_t cur_frame);
static lv_res_t png_create_decoder(struct png_player_ctx *ctx);
static void png_destroy_decoder(struct png_player_ctx *ctx);
static int png_blend_bitmaps_software(PNG_Bitmap *dst, PNG_Bitmap *src, uint32_t x, uint32_t y, uint8_t blend_op);
static int png_blend_bitmaps_hardware(PNG_Bitmap *dst, PNG_Bitmap *src, uint32_t x, uint32_t y, uint8_t blend_op);
static int png_blend_bitmaps(PNG_Bitmap *dst, PNG_Bitmap *src, uint32_t x, uint32_t y, uint8_t blend_op);
static void png_calc_blend_dimensions(int dst_dim, int src_dim, int pos, int *actual_dim);
static void png_player_cleanup(struct png_player_ctx *ctx);

/* Utility functions */
static void png_update_image_desc(struct png_player_ctx *ctx);
static uint32_t png_get_package_size(struct png_player_ctx *ctx);

/* Player command handlers */
static lv_res_t player_handle_seek(void *ctx, uint64_t time);
static lv_res_t player_handle_get_frame(void *ctx);

/* Memory allocation functions */
static int alloc_player_frame_buffer(struct frame_allocator *p, struct mpp_frame *frame,
                              int width, int height, enum mpp_pixel_format format);
static int free_player_frame_buffer(struct frame_allocator *p, struct mpp_frame *frame);
static int close_allocator(struct frame_allocator *p);

/* auxiliary functions */
static void convert_to_lvgl_path(const char *path, char *lv_path, uint16_t max_len);
#if PNG_DUMPING_IMAGE
static void png_debug_save_frame(uint8_t *data, uint32_t len, uint16_t frame_index, const char* suffix, char *path);
static void png_debug_save_bmp_frame(PNG_Bitmap *bitmap, uint16_t frame_index, char *path);
#endif

static struct alloc_ops frame_buffer_alloc_ops = {
    .alloc_frame_buffer = alloc_player_frame_buffer,
    .free_frame_buffer = free_player_frame_buffer,
    .close_allocator = close_allocator,
};

const player_backend_ops_t png_backend_ops_template = {
    .name = "apng",
    .create = png_player_backend_create,
    .destroy = png_player_backend_destroy,
    .set_src = png_player_backend_set_src,
    .control = png_player_backend_control,
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
const player_backend_ops_t *png_backend_get_template(void)
{
    return &png_backend_ops_template;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void *png_player_backend_create(void)
{
    player_backend_ops_t *ops = lv_malloc(sizeof(player_backend_ops_t));
    if (!ops) {
        LV_LOG_ERROR("Failed to allocate player backend ops");
        return NULL;
    }

    const player_backend_ops_t *template = png_backend_get_template();
    memcpy(ops, template, sizeof(player_backend_ops_t));

    struct png_player_ctx *png_ctx = lv_malloc_zeroed(sizeof(struct png_player_ctx));
    if (!png_ctx) {
        lv_free(ops);
        LV_LOG_ERROR("Failed to allocate PNG player context");
        return NULL;
    }

    ops->ctx = png_ctx;
    png_ctx->status = PLAYER_STATUS_IDLE;
    png_ctx->is_normal_png = true;
    png_ctx->playback_rate = 1.0f;

    return ops;
}

static void png_player_backend_destroy(void *ctx)
{
    player_backend_ops_t *ops = (player_backend_ops_t *)ctx;
    struct png_player_ctx *png_ctx = ops->ctx;

    if (!ops || !png_ctx) {
        LV_LOG_ERROR("Invalid ops or png_ctx");
        return;
    }

    png_player_cleanup(png_ctx);

    lv_free(png_ctx);
    lv_free(ops);

    return;
}

static lv_res_t png_player_backend_set_src(void *ctx, const char *src)
{
    player_backend_ops_t *ops = (player_backend_ops_t *)ctx;
    uint8_t read_sig[PNG_SIGNATURE_SIZE] = {0};
    struct png_player_ctx *png_ctx = ops->ctx;
    char lvgl_path[256] = {0};
    lv_res_t res = LV_RES_OK;
    PNG_Bitmap bitmap = {0};
    uint32_t size = 0;

    /* cleanup old resources */
    png_player_cleanup(png_ctx);

    /* Convert path to LVGL format */
    convert_to_lvgl_path(src, lvgl_path, sizeof(lvgl_path));

    /* Open the file */
    res = lv_fs_open(&png_ctx->file, lvgl_path, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) {
        LV_LOG_ERROR("Failed to open PNG file '%s': error code %d", lvgl_path, res);
        goto set_src_err;
    }

    /* Verify PNG signature */
    lv_fs_read(&png_ctx->file, read_sig, PNG_SIGNATURE_SIZE, &size);
    if (size != PNG_SIGNATURE_SIZE ||
        memcmp(read_sig, "\x89PNG\r\n\x1a\n", PNG_SIGNATURE_SIZE) != 0) {
        LV_LOG_ERROR("Invalid PNG signature");
        goto set_src_err;
    }

    /* Read IHDR chunk */
    if (png_read_chunk(png_ctx, &png_ctx->ihdr) < 0) {
        LV_LOG_ERROR("Failed to read IHDR chunk");
        goto set_src_err;
    }

    png_scan_apng_header(png_ctx);
    if (png_ctx->is_normal_png == true) {
        LV_LOG_WARN("APNG is not AIC's proprietary format, assuming normal PNG");
    }

    /* Allocate image source structure */
    png_ctx->image_src = lv_malloc(sizeof(lv_image_dsc_t));
    if (!png_ctx->image_src) {
        LV_LOG_ERROR("Failed to allocate image descriptor");
        goto set_src_err;
    }

    /* Parse image dimensions */
    png_ctx->bitmap.format = MPP_FMT_ARGB_8888;
    png_ctx->bitmap.width = png_get_be32(png_ctx->ihdr.data);
    png_ctx->bitmap.height = png_get_be32(png_ctx->ihdr.data + 4);
    if (png_ctx->is_normal_png == false) {
        png_ctx->bitmap.format = png_ctx->dcTL.has_alpha ? MPP_FMT_ARGB_8888 : MPP_FMT_RGB_888;
    }
    png_ctx->bitmap.stride = backend_align_stride(png_ctx->bitmap.width, png_ctx->bitmap.format);

    /* Setup decoder */
    size = png_ctx->bitmap.stride * png_ctx->bitmap.height + 256;
    png_ctx->addr[0] = aicos_malloc_try_cma(size);
    if (!png_ctx->addr[0]) {
        LV_LOG_ERROR("malloc png buffer[0] failed, size = %d", (int)size);
        goto set_src_err;
    }
    aicos_dcache_clean_invalid_range((unsigned long *)((unsigned long)png_ctx->addr[0]), size);
    if (png_ctx->is_normal_png == false) {
        if (png_ctx->dcTL.has_blend_over == 1) {
            png_ctx->addr[1] = aicos_malloc_try_cma(size);
            if (!png_ctx->addr[1]) {
                LV_LOG_ERROR("malloc png buffer[1] failed, size = %d", (int)size);
                goto set_src_err;
            }
            aicos_dcache_clean_invalid_range((unsigned long *)((unsigned long)png_ctx->addr[1]), size);
        }
    }

    png_ctx->align_addr[0] = (uint8_t *)ALIGN_UP((uintptr_t)png_ctx->addr[0], 16);
    png_ctx->align_addr[1] = (uint8_t *)ALIGN_UP((uintptr_t)png_ctx->addr[1], 16);

    png_ctx->bitmap.data = png_ctx->align_addr[0];
    png_ctx->image_data = (uint8_t *)png_ctx->align_addr[0];

    /* draw buffer is black */
    for (int i = 0; i < 2; i++) {
        memcpy(&bitmap, &png_ctx->bitmap, sizeof(PNG_Bitmap));
        bitmap.data = png_ctx->align_addr[i];
        if (png_ctx->align_addr[i] == NULL)
            break;
    }

    png_ctx->package_size = png_get_package_size(png_ctx);

    png_update_image_desc(png_ctx);

    backend_draw_color_buffer((lv_image_dsc_t *)png_ctx->image_src, 0xff000000);

    png_ctx->media_info.has_video = 1;
    png_ctx->media_info.video_stream.width = png_ctx->bitmap.width;
    png_ctx->media_info.video_stream.height = png_ctx->bitmap.height;

    png_ctx->status = PLAYER_STATUS_READY;

    LV_LOG_INFO("bitmap: w = %d, h = %d, format = %d, stride = %d, addr[0] = %p, addr[1] = %p, packet size = %d",
                 (int)png_ctx->bitmap.width, (int)png_ctx->bitmap.height,
                 (int)png_ctx->bitmap.format, (int)png_ctx->bitmap.stride,
                 png_ctx->addr[0], png_ctx->addr[1],
                 (int)png_ctx->package_size);
    return LV_RES_OK;

set_src_err:
    png_player_cleanup(png_ctx);

    return LV_RES_INV;
}

static lv_res_t png_player_backend_control(void *ctx, player_cmd_t cmd, void *data)
{
    player_backend_ops_t *ops = (player_backend_ops_t *)ctx;
    struct png_player_ctx *png_ctx = ops->ctx;
    lv_res_t res = LV_RES_OK;

    switch(cmd) {
        case PLAYER_CMD_START:
            png_ctx->status = PLAYER_STATUS_RUNNING;
            break;
        case PLAYER_CMD_PAUSE:
            png_ctx->status = PLAYER_STATUS_PAUSE;
            break;
        case PLAYER_CMD_RESUME:
            png_ctx->status = PLAYER_STATUS_RUNNING;
            break;
        case PLAYER_CMD_PLAY_END:
            if (!data) {
                LV_LOG_WARN("PLAYER_CMD_PLAY_END: data is NULL");
                return LV_RES_INV;
            }
            *(bool *)data = (png_ctx->status == PLAYER_STATUS_END);
            break;
        case PLAYER_CMD_SET_PLAY_TIME:
            if (!data) {
                LV_LOG_WARN("PLAYER_CMD_SET_PLAY_TIME: data is NULL");
                return LV_RES_INV;
            }
            res = player_handle_seek(ctx, *(uint64_t *)data);
            break;
        case PLAYER_CMD_GET_FRAME:
            return player_handle_get_frame(ctx);
        case PLAYER_CMD_GET_IMAGE_SRC:
            if (!data) {
                LV_LOG_WARN("PLAYER_CMD_GET_IMAGE_SRC: data is NULL");
                return LV_RES_INV;
            }
            if (!png_ctx->image_src) {
                LV_LOG_WARN("PLAYER_CMD_GET_IMAGE_SRC: image_src is NULL");
                return LV_RES_INV;
            }
            *(void **)data = png_ctx->image_src;
            break;
       case PLAYER_CMD_GET_MEDIA_INFO:
            if (!data) {
                LV_LOG_WARN("PLAYER_CMD_GET_IMAGE_SRC: data is NULL");
                return LV_RES_INV;
            }
            memcpy(data, &png_ctx->media_info, sizeof(struct av_media_info));
            res = LV_RES_OK;
            break;
        case PLAYER_CMD_SET_PLAYBACK_RATE:
            if (!data) {
                LV_LOG_WARN("PLAYER_CMD_SET_PLAYBACK_RATE: data is NULL");
                return LV_RES_INV;
            }
            float rate = *(float *)data;
            if (rate < 0.1f || rate > 10.0f) {
                LV_LOG_ERROR("Invalid playback rate: %.2f (valid range: 0.1 to 10.0)", rate);
                return LV_RES_INV;
            }
            png_ctx->playback_rate = rate;
            LV_LOG_INFO("Playback rate set to: %.2fx", rate);
            res = LV_RES_OK;
            break;
        default:
            LV_LOG_ERROR("Unsupported command: %d", cmd);
            res = LV_RES_INV;
            break;
    }

    return res;
}

/* PNG parsing functions */
static uint32_t png_get_be32(const unsigned char *buffer)
{
    return (((unsigned)buffer[0] << 24u) |
            ((unsigned)buffer[1] << 16u) |
            ((unsigned)buffer[2] << 8u) |
            (unsigned)buffer[3]);
}

static uint16_t png_get_be16(const unsigned char *buffer)
{
    return ((uint16_t)buffer[0] << 8) | (uint16_t)buffer[1];
}

static uint32_t png_to_be32(uint32_t host_int)
{
    const uint32_t test_value = 0x01020304;
    const uint8_t *test_bytes = (const uint8_t *)&test_value;
    const bool is_little_endian = (test_bytes[0] == 0x04);

    if (is_little_endian)
    {
        return ((host_int >> 24) & 0xFF) |
               ((host_int >> 8) & 0xFF00) |
               ((host_int << 8) & 0xFF0000) |
               ((host_int << 24) & 0xFF000000);
    }
    return host_int;
}

static long png_read_chunk(struct png_player_ctx *ctx, PNG_Chunk *chunk)
{
    uint8_t buf[4];
    uint32_t start_pos = 0;
    uint32_t read_size = 0;
    lv_fs_res_t res = LV_FS_RES_OK;

    /* Get current position */
    lv_fs_tell(&ctx->file, &start_pos);
    if (res != LV_FS_RES_OK) {
        LV_LOG_ERROR("Failed to get file position");
        return -1;
    }

    /* Read chunk length */
    res = lv_fs_read(&ctx->file, buf, 4, &read_size);
    if (res != LV_FS_RES_OK || read_size != 4) {
        LV_LOG_ERROR("Failed to read chunk length");
        return -1;
    }
    chunk->length = png_get_be32(buf);

    /* Read chunk type */
    res = lv_fs_read(&ctx->file, chunk->type, 4, &read_size);
    if (res != LV_FS_RES_OK || read_size != 4) {
        LV_LOG_ERROR("Failed to read chunk type");
        return -1;
    }

    /* Read chunk data */
    if (chunk->length > 0) {
        chunk->data = lv_malloc(chunk->length);
        if (!chunk->data) {
            LV_LOG_ERROR("Failed to allocate %d bytes for chunk data", (int)chunk->length);
            return -1;
        }
        res = lv_fs_read(&ctx->file, chunk->data, chunk->length, &read_size);
        if (res != LV_FS_RES_OK || read_size != chunk->length) {
            LV_LOG_ERROR("Failed to read chunk data");
            lv_free(chunk->data);
            chunk->data = NULL;
            return -1;
        }
    }

    /* Read CRC */
    res = lv_fs_read(&ctx->file, buf, 4, &read_size);
    if (res != LV_FS_RES_OK || read_size != 4) {
        LV_LOG_ERROR("Failed to read chunk CRC");
        if (chunk->data) lv_free(chunk->data);
        return -1;
    }
    chunk->crc = png_get_be32(buf);

    return start_pos;
}

static size_t png_write_chunk(uint8_t *dest, PNG_Chunk *chunk)
{
    uint32_t buffer;

    buffer = png_to_be32(chunk->length);
    memcpy(dest, &buffer, 4);

    memcpy(dest + 4, chunk->type, 4);

    if (chunk->length != 0)
        memcpy(dest + 8, chunk->data, chunk->length);

    buffer = png_to_be32(chunk->crc);
    memcpy(dest + 8 + chunk->length, &buffer, 4);

    return 12 + chunk->length;
}

static void png_parse_dctl(Private_Chunk_dcTL *dcTL, PNG_Chunk *chunk)
{
    uint32_t frame_num = 0;

    dcTL->has_alpha = chunk->data[0];
    dcTL->has_blend_over = chunk->data[1];

    if (IS_FFMPEG_CONVERTED_CHUNK(chunk)) {
        dcTL->max_chunk_size = png_get_be32(&chunk->data[2]);
        return;
    } else {
        frame_num = (chunk->length - 2) / 4;
        dcTL->frame_chunks_length = lv_malloc(frame_num * sizeof(uint32_t));
        LV_LOG_INFO("dcTL: frame_num = %d, has_transparent_color = %d, blend mode = %d", (int)frame_num, (int)chunk->data[0], (int)chunk->data[1]);
        for (int i = 0; i < frame_num; i++) {
            dcTL->frame_chunks_length[i] = png_get_be32(&chunk->data[2 + 4 * i]);
            //LV_LOG_INFO("dcTL[%d].chunks_length = %d", i, (int)dcTL->frame_chunks_length[i]);
        }
    }
}

static void png_parse_actl(uint32_t *num_frames, uint32_t *num_plays, PNG_Chunk *chunk)
{
    *num_frames = png_get_be32(chunk->data);
    *num_plays = png_get_be32(chunk->data + 4);
    LV_LOG_INFO("acTL: num_frames = %d, num_plays = %d", (int)*num_frames, (int)*num_plays);
}

static void png_parse_fctl(fcTL_Chunk *fcTL, PNG_Chunk *chunk)
{
    fcTL->serial_number = png_get_be32(chunk->data);
    fcTL->width = png_get_be32(chunk->data + 4);
    fcTL->height = png_get_be32(chunk->data + 8);
    fcTL->x = png_get_be32(chunk->data + 12);
    fcTL->y = png_get_be32(chunk->data + 16);
    fcTL->delay_num = png_get_be16(chunk->data + 20);
    fcTL->delay_den = png_get_be16(chunk->data + 22);
    fcTL->dispose_op = (APNG_Dispose_Op)chunk->data[24];
    fcTL->blend_op = (APNG_Blend_Op)chunk->data[25];
    // LV_LOG_INFO("fcTL: serial_number = %d, width = %d, height = %d, x= %d, y = %d, delay_num = %d, delay_den = %d, dispose_op = %d, blend_op = %d",
    //              (int)fcTL->serial_number, (int)fcTL->width, (int)fcTL->height, (int)fcTL->x, (int)fcTL->y, (int)fcTL->delay_num, (int)fcTL->delay_den, (int)fcTL->dispose_op, (int)fcTL->blend_op);
}

static bool png_should_cache_auxiliary_chunk(const char *chunk_type)
{
    const char *cacheable[] = {"PLTE", "tRNS", "gAMA", "cHRM", "bKGD", "sRGB", "iCCP", "pHYs"};
    for (size_t i = 0; i < sizeof(cacheable) / sizeof(cacheable[0]); i++)
    {
        if (memcmp(chunk_type, cacheable[i], 4) == 0)
        {
            return true;
        }
    }
    return false;
}

static void png_scan_apng_header(struct png_player_ctx *ctx)
{
    PNG_Chunk *new_aux = NULL;
    PNG_Chunk chunk = {0};
    long chunk_pos = -1;

    while (1)
    {
        chunk_pos = png_read_chunk(ctx, &chunk);
        if (chunk_pos < 0) {
            LV_LOG_ERROR("Failed to read chunk");
            goto cleanup_aux_chunks;
        }

        if (memcmp(chunk.type, "IEND", 4) == 0) {
            lv_free(chunk.data);
            break;
        }

        if (png_should_cache_auxiliary_chunk((const char *)chunk.type)) {
            new_aux = lv_realloc(ctx->aux_chunks, (ctx->aux_chunks_num + 1) * sizeof(PNG_Chunk));
            if (!new_aux)
            {
                LV_LOG_ERROR("Cant't alloc aux chunk, size = %d", (int)((ctx->aux_chunks_num + 1) * sizeof(PNG_Chunk)));
                lv_free(chunk.data);
                goto cleanup_aux_chunks;
                return;
            }

            ctx->aux_chunks = new_aux;
            ctx->aux_chunks[ctx->aux_chunks_num++] = chunk;
            ctx->aux_chunks_size += (12 + chunk.length);
            continue;
        }

        if (memcmp(chunk.type, "acTL", 4) == 0) {
            png_parse_actl(&ctx->num_frames, &ctx->num_plays, &chunk);
        } else if (memcmp(chunk.type, "dcTL", 4) == 0) {
            png_parse_dctl(&ctx->dcTL, &chunk);
            ctx->is_normal_png = false;
        } else if (memcmp(chunk.type, "fcTL", 4) == 0) {
            ctx->first_frame_pos = chunk_pos;
            if (lv_fs_seek(&ctx->file, ctx->first_frame_pos, LV_FS_SEEK_SET) != LV_FS_RES_OK) {
                LV_LOG_ERROR("seek to %d fail", (int)ctx->first_frame_pos);
            }
            lv_free(chunk.data);
            return;
        } else if (memcmp(chunk.type, "IDAT", 4) == 0) {
            lv_free(chunk.data);
            return;
        } else {
            LV_LOG_ERROR("read chunk type failed: %s", chunk.type);
            lv_free(chunk.data);
            return;
        }
        lv_free(chunk.data);
    }

    return;
cleanup_aux_chunks:
    for (uint32_t i = 0; i < ctx->aux_chunks_num; i++) {
        lv_free(ctx->aux_chunks[i].data);
    }
    lv_free(ctx->aux_chunks);
    ctx->aux_chunks = NULL;
    ctx->aux_chunks_num = 0;
}

/* Frame processing functions */
static lv_res_t process_normal_png_frame(struct png_player_ctx *ctx)
{
    struct mpp_packet packet = {0};
    struct mpp_frame frame = {0};
    uint32_t bytes_read = 0;

    if (png_create_decoder(ctx) != LV_RES_OK) {
        LV_LOG_ERROR("Failed to create APNG file data");
        return LV_RES_INV;
    }

    mpp_decoder_get_packet(ctx->dec, &packet, ctx->package_size);
    packet.size = ctx->package_size;
    packet.flag = PACKET_FLAG_EOS;

    lv_fs_seek(&ctx->file, 0, LV_FS_SEEK_SET);
    lv_fs_read(&ctx->file, packet.data, ctx->package_size, &bytes_read);
    if (bytes_read != ctx->package_size) {
        LV_LOG_ERROR("Failed to read PNG file data, size = %d", (int)ctx->package_size);
        mpp_decoder_put_packet(ctx->dec, &packet);
        goto png_frame_ret;
    }

    mpp_decoder_put_packet(ctx->dec, &packet);

    if (mpp_decoder_decode(ctx->dec) < 0) {
        LV_LOG_ERROR("PNG frame decode failed: frame=%d, width=%d, height=%d, format=%d",
             (int)ctx->frame_count, (int)ctx->bitmap.width, (int)ctx->bitmap.height, (int)ctx->bitmap.format);
        mpp_decoder_put_packet(ctx->dec, &packet);
        goto png_frame_ret;
    }

    mpp_decoder_get_frame(ctx->dec, &frame);

    mpp_decoder_put_frame(ctx->dec, &frame);

png_frame_ret:
    png_destroy_decoder(ctx);

    ctx->frame_duration = 0xffffffff;
    ctx->status = PLAYER_CMD_STOP;

    return LV_RES_OK;
}

static lv_res_t png_read_fctl_chunk(struct png_player_ctx *ctx, fcTL_Chunk *fcTL_chunk, PNG_Chunk *chunk)
{
    long chunk_pos = -1;

    chunk_pos = png_read_chunk(ctx, chunk);
    if (chunk_pos < 0) {
        LV_LOG_ERROR("Failed to read chunk");
        return LV_RES_INV;
    }

    if (memcmp(chunk->type, "fcTL", 4) == 0) {
        png_parse_fctl(fcTL_chunk, chunk);
    } else {
        return LV_RES_INV;
    }

    return LV_RES_OK;
}

static lv_res_t png_peek_idats_chunk_header(struct png_player_ctx *ctx, PNG_Chunk *chunk)
{
    lv_fs_res_t res = LV_FS_RES_OK;
    uint32_t start_pos = 0;
    uint32_t read_size = 0;
    uint8_t buf[4] = {0};

    /* Get current position */
    lv_fs_tell(&ctx->file, &start_pos);
    if (res != LV_FS_RES_OK) {
        LV_LOG_ERROR("Failed to get file position");
        return LV_RES_INV;
    }

    /* Read chunk length */
    res = lv_fs_read(&ctx->file, buf, 4, &read_size);
    if (res != LV_FS_RES_OK || read_size != 4) {
        LV_LOG_ERROR("Failed to read chunk length");
        return LV_RES_INV;
    }
    chunk->length = png_get_be32(buf);

    /* Read chunk type */
    res = lv_fs_read(&ctx->file, chunk->type, 4, &read_size);
    if (res != LV_FS_RES_OK || read_size != 4) {
        LV_LOG_ERROR("Failed to read chunk type");
        return LV_RES_INV;
    }

    return LV_RES_OK;
}

static lv_res_t process_apng_frame(struct png_player_ctx *ctx, uint32_t cur_frame)
{
    struct mpp_packet packet = {0};
    uint8_t *packet_buffer = NULL;
    struct mpp_frame frame = {0};
    PNG_Chunk *aux_chunk = NULL;
    fcTL_Chunk fcTL_data = {0};
    uint32_t IDATs_length = 0;
    uint32_t package_size = 0;
    uint32_t bytes_read = 0;
    int get_packet_ret = -1;
    PNG_Chunk IDATs = {0};
    PNG_Chunk fcTL = {0};
    PNG_Chunk IEND = {0};
    uint32_t offset = 0;

    if (cur_frame == 0) {
        png_destroy_decoder(ctx);
        if (png_create_decoder(ctx) != LV_RES_OK) {
            LV_LOG_ERROR("Failed to create APNG file data");
            return LV_RES_INV;
        }
    }

    if (png_read_fctl_chunk(ctx, &fcTL_data, &fcTL) != LV_RES_OK) {
        LV_LOG_ERROR("Failed to get fctl");
        goto failed_get_apng_frame;
    }

    if (png_peek_idats_chunk_header(ctx, &IDATs) != LV_RES_OK) {
        LV_LOG_ERROR("Failed to get idat");
        goto failed_get_apng_frame;
    }

    /* get package */
    package_size = cur_frame == 0 ?
                    PNG_SIGNATURE_SIZE + PNG_IHDR_CHINK_SIZE + ctx->aux_chunks_size + PNG_FCTL_CHUNK_SIZE + (12 + IDATs.length) + PNG_IEND_CHUNK_SIZE:
                    PNG_FCTL_CHUNK_SIZE + (12 + IDATs.length); // fcTL + fdAT
    get_packet_ret = mpp_decoder_get_packet(ctx->dec, &packet, package_size);
    if (get_packet_ret != 0) {
        LV_LOG_ERROR("decoder get packet failed, size = %d", (int)package_size);
        goto failed_get_apng_frame;
    }
    packet.size = package_size;
    packet.flag = PACKET_FLAG_EOS;
    packet_buffer = packet.data;

    /* build png package */
    if (cur_frame == 0) {
        memcpy(packet_buffer, "\x89PNG\r\n\x1a\n", PNG_SIGNATURE_SIZE);  // header
        offset += PNG_SIGNATURE_SIZE;

        offset += png_write_chunk(packet_buffer + offset, &ctx->ihdr); // ihdr

        // aux chunk
        for (int i = 0; i < ctx->aux_chunks_num; i++) {
            aux_chunk = &ctx->aux_chunks[i];
            offset += png_write_chunk(packet_buffer + offset, aux_chunk);
        }
    }

    offset += png_write_chunk(packet_buffer + offset, &fcTL); // fctl

    // idats
    IDATs_length = png_to_be32(IDATs.length);
    memcpy(packet_buffer + offset, &IDATs_length, 4);
    offset += 4;
    memcpy(packet_buffer + offset, IDATs.type, 4);
    offset += 4;
    lv_fs_read(&ctx->file, packet_buffer + offset, IDATs.length + 4, &bytes_read);
    if (bytes_read != IDATs.length + 4) {
        LV_LOG_ERROR("Read IDATs data failed");
        goto failed_get_apng_frame;
    }
    offset += bytes_read;

    if (cur_frame == 0) {
        memcpy(IEND.type, "IEND", 4);
        IEND.length = 0;

        offset += png_write_chunk(packet_buffer + offset, &IEND); // iend
    }

    lv_free(fcTL.data);

#if PNG_DUMPING_IMAGE
    png_debug_save_frame(packet.data, packet.size, cur_frame, "png", "/sdcard/frame");
#endif

    mpp_decoder_put_packet(ctx->dec, &packet);
    if (mpp_decoder_decode(ctx->dec) < 0) {   // return packet and then decode
        LV_LOG_ERROR("Failed to decode APNG frame");
        goto failed_get_apng_frame;
    }
    mpp_decoder_get_frame(ctx->dec, &frame);

#if PNG_DUMPING_IMAGE
    PNG_Bitmap decoded = {
        .data = (uint8_t *)(uintptr_t)frame.buf.phy_addr[0],
        .width = fcTL_data.width,
        .height = fcTL_data.height,
        .stride = frame.buf.stride[0],
        .format = frame.buf.format
    };
    png_debug_save_bmp_frame(&decoded, cur_frame, "/sdcard/bmp_decode");
#endif
    if (ctx->dcTL.has_blend_over == 0)
        mpp_decoder_put_frame(ctx->dec, &frame);
    else if (ctx->dcTL.has_blend_over == 1 && cur_frame != 0)
        mpp_decoder_put_frame(ctx->dec, &frame);
    if (cur_frame > 0 && ctx->dcTL.has_blend_over == 1) {
        PNG_Bitmap src = {
            .data = (uint8_t *)(uintptr_t)frame.buf.phy_addr[0],
            .width = fcTL_data.width,
            .height = fcTL_data.height,
            .stride = frame.buf.stride[0],
            .format = frame.buf.format
        };

        if (png_blend_bitmaps(&ctx->bitmap, &src, fcTL_data.x, fcTL_data.y, fcTL_data.blend_op) < 0) {
            LV_LOG_ERROR("Failed to blend APNG frame");
            return LV_RES_INV;
        }
    }

    int den = fcTL_data.delay_den > 0 ? fcTL_data.delay_den : 100;
    ctx->frame_duration = (fcTL_data.delay_num * 1000 + den / 2) / den;

#if PNG_DUMPING_IMAGE
    png_debug_save_bmp_frame(&ctx->bitmap, cur_frame, "/sdcard/bmp_render");
#endif
    return LV_RES_OK;

failed_get_apng_frame:
    if (get_packet_ret == 0)
        mpp_decoder_put_packet(ctx->dec, &packet);

    if (fcTL.data)
        lv_free(fcTL.data);
    ctx->frame_duration = 0xffffffff;
    ctx->status = PLAYER_CMD_STOP;
    return LV_RES_INV;
}

static lv_res_t png_create_decoder(struct png_player_ctx *ctx)
{
    struct decode_config config = {0};

    ctx->dec = mpp_decoder_create(MPP_CODEC_VIDEO_DECODER_PNG);
    if (!ctx->dec) {
        LV_LOG_ERROR("Failed to create PNG decoder");
        return LV_RES_INV;
    }

    config.bitstream_buffer_size = ALIGN_UP(ctx->package_size, 1024);
    config.extra_frame_num = 0;
    config.packet_count = 1;
    config.pix_fmt = ctx->bitmap.format;
    if (ctx->dcTL.has_blend_over == 1) {
        config.extra_frame_num = 1;
    }

    ctx->allocated_frame_count = 0;
    ctx->allocator.ops = &frame_buffer_alloc_ops;
    mpp_decoder_control(ctx->dec, MPP_DEC_INIT_CMD_SET_EXT_FRAME_ALLOCATOR, (void*)&ctx->allocator);
    mpp_decoder_init(ctx->dec, &config);

    return LV_RES_OK;
}

static void png_destroy_decoder(struct png_player_ctx *ctx)
{
    if (ctx->dec)
        mpp_decoder_destory(ctx->dec);
    ctx->dec = NULL;
}

static void png_calc_blend_dimensions(int dst_dim, int src_dim, int pos, int *actual_dim)
{
    if (pos >= dst_dim || src_dim <= 0) {
        *actual_dim = 0;
        return;
    }
    *actual_dim = (pos + src_dim > dst_dim) ? (dst_dim - pos) : src_dim;
}

static int png_blend_bitmaps_software(PNG_Bitmap *dst, PNG_Bitmap *src, uint32_t x, uint32_t y, uint8_t blend_op)
{
    int actual_w = 0, actual_h = 0;
    int size = 0;
    png_calc_blend_dimensions(dst->width, src->width, x, &actual_w);
    png_calc_blend_dimensions(dst->height, src->height, y, &actual_h);

    if (actual_w <= 0 || actual_h <= 0) {
        LV_LOG_WARN("Invalid blending dimensions");
        return -1;
    }

    uint8_t byte_pixel = 4;
    if (dst->format == MPP_FMT_RGB_888 && src->format == MPP_FMT_RGB_888) {
        byte_pixel = 3;
    }
    uint8_t *dst_pixels = dst->data + y * dst->stride + x * byte_pixel;
    const uint8_t *src_pixels = src->data + y * dst->stride + x * byte_pixel;

    for (int row = 0; row < actual_h; row++) {
        uint8_t *dst_row = dst_pixels + row * dst->stride;
        const uint8_t *src_row = src_pixels + row * src->stride;

        for (int col = 0; col < actual_w; col++) {
            uint8_t *dst_pixel = dst_row + col * 4;
            const uint8_t *src_pixel = src_row + col * 4;

            if (blend_op == APNG_BLEND_OP_SOURCE) {
                memcpy(dst_pixel, src_pixel, 4);
            } else { // APNG_BLEND_OP_OVER
                const uint8_t src_alpha = src_pixel[3];
                if (src_alpha == 0xFF) {
                    memcpy(dst_pixel, src_pixel, 4);
                } else if (src_alpha != 0) {
                    // result = (src * src_alpha) + (dst * (1 - src_alpha))
                    const uint16_t inv_src_alpha = 255 - src_alpha;
                    for (int i = 0; i < 3; i++) {
                        dst_pixel[i] = (uint8_t)(
                            (src_pixel[i] * src_alpha + dst_pixel[i] * inv_src_alpha) / 255
                        );
                    }
                    // Alpha channel：src_alpha + dst_alpha * (1 - src_alpha)
                    dst_pixel[3] = (uint8_t)(
                        src_alpha + (dst_pixel[3] * inv_src_alpha) / 255
                    );
                }
            }
        }
    }

    size = dst->stride * dst->height;
    aicos_dcache_clean_invalid_range((unsigned long *)((unsigned long)dst->data), size);
    aicos_dcache_clean_invalid_range((unsigned long *)((unsigned long)src->data), size);
    return 0;
}

static int png_blend_bitmaps_hardware(PNG_Bitmap *dst, PNG_Bitmap *src, uint32_t x, uint32_t y, uint8_t blend_op)
{
    struct mpp_ge *ge = NULL;
    struct ge_bitblt blt = {0};
    int ret = 0;

    uint32_t src_width = dst->width;
    uint32_t dst_width = dst->height;
    blt.src_buf.buf_type = MPP_PHY_ADDR;
    blt.src_buf.phy_addr[0] = (unsigned int)(intptr_t)src->data;
    blt.src_buf.stride[0] = src->stride;
    blt.src_buf.size.width = src_width;
    blt.src_buf.size.height = dst_width;
    blt.src_buf.format = src->format;
    blt.src_buf.crop_en = 1;
    blt.src_buf.crop.x = x;
    blt.src_buf.crop.y = y;
    blt.src_buf.crop.width = src->width;
    blt.src_buf.crop.height = src->height;

    uint32_t dst_crop_width = src->width;
    uint32_t dst_crop_height = src->height;
    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = (unsigned int)(intptr_t)dst->data;
    blt.dst_buf.stride[0] = dst->stride;
    blt.dst_buf.size.width = dst->width;
    blt.dst_buf.size.height = dst->height;
    blt.dst_buf.format = dst->format;
    blt.dst_buf.crop_en = 1;
    blt.dst_buf.crop.x = x;
    blt.dst_buf.crop.y = y;
    blt.dst_buf.crop.width = dst_crop_width;
    blt.dst_buf.crop.height = dst_crop_height;

    if (blend_op == APNG_BLEND_OP_SOURCE) {
        blt.ctrl.alpha_en = 0;
    } else {
        blt.ctrl.alpha_en = 1;
        blt.ctrl.alpha_rules = GE_PD_NONE;
    }

    ge = mpp_ge_open();
    if (!ge) {
        LV_LOG_ERROR("open ge device error\n");
        return -1;
    }

    ret = mpp_ge_bitblt(ge, &blt);
    if (ret < 0) {
        LV_LOG_ERROR("ge bitble fail\n");
    }

    ret = mpp_ge_emit(ge);
    if (ret < 0) {
        LV_LOG_ERROR("ge emit fail\n");
    }

    ret = mpp_ge_sync(ge);
    if (ret < 0) {
        LV_LOG_ERROR("ge sync fail\n");
    }

    mpp_ge_close(ge);

    return 0;
}

static int png_blend_bitmaps(PNG_Bitmap *dst, PNG_Bitmap *src, uint32_t x, uint32_t y, uint8_t blend_op)
{
    if (!dst || !src || !dst->data || !src->data) {
        LV_LOG_ERROR("Invalid bitmap parameters");
        return -1;
    }

    if (src->height * src->stride < 1000) {
        return png_blend_bitmaps_software(dst, src, x, y, blend_op);
    }
    return png_blend_bitmaps_hardware(dst, src, x, y, blend_op);
}

static void png_player_cleanup(struct png_player_ctx *png_ctx)
{
    lv_fs_close(&png_ctx->file);

    for (int i = 0; i < png_ctx->aux_chunks_num; i++)
    {
        lv_free(png_ctx->aux_chunks[i].data);
    }

    if (png_ctx->aux_chunks)
    {
        lv_free(png_ctx->aux_chunks);
    }

    for (int i = 0; i < 2; i++) {
        if (png_ctx->addr[i]) {
            aicos_free(MEM_CMA, (void*)(unsigned long)png_ctx->addr[i]);
        }
    }

    if (png_ctx->dec)
        mpp_decoder_destory(png_ctx->dec);

    if (png_ctx->ihdr.data)
        lv_free(png_ctx->ihdr.data);

    if (png_ctx->dcTL.frame_chunks_length)
        lv_free(png_ctx->dcTL.frame_chunks_length);

    if (png_ctx->image_src)
        lv_free(png_ctx->image_src);

    memset(png_ctx, 0, sizeof(struct png_player_ctx));
    png_ctx->status = PLAYER_STATUS_IDLE;
    png_ctx->is_normal_png = true;
}

/* Player command handlers */
static lv_res_t player_handle_seek(void *ctx, uint64_t time)
{
    if (time != 0) {
        LV_LOG_ERROR("Seeking not supported for PNG files");
        return LV_RES_INV;
    }

    player_backend_ops_t *ops = (player_backend_ops_t *)ctx;
    struct png_player_ctx *png_ctx = ops->ctx;

    if (png_ctx->is_normal_png == true) {
        return LV_RES_INV;
    }

    if (lv_fs_seek(&png_ctx->file, png_ctx->first_frame_pos, LV_FS_SEEK_SET) != LV_FS_RES_OK) {
        LV_LOG_ERROR("seek to %d fail", (int)png_ctx->first_frame_pos);
        return LV_RES_INV;
    }

    png_ctx->last_frame_time = 0;
    png_ctx->status = PLAYER_STATUS_RUNNING;
    png_ctx->frame_count = 0;

    LV_LOG_INFO("Seeking to the first frame, pos = %d", (int)png_ctx->first_frame_pos);

    return LV_RES_OK;
}

static lv_res_t player_handle_get_frame(void *ctx)
{
    player_backend_ops_t *ops = (player_backend_ops_t *)ctx;
    struct png_player_ctx *png_ctx = ops->ctx;
    uint32_t cur_time = lv_tick_get();

    if (png_ctx->status != PLAYER_STATUS_RUNNING) {
        return LV_RES_INV;
    }

    /* Calculate adjusted frame duration based on playback rate */
    uint32_t adjusted_duration = png_ctx->frame_duration;
    if (png_ctx->playback_rate > 0.0f) {
        adjusted_duration = (uint32_t)(png_ctx->frame_duration / png_ctx->playback_rate);
        if (adjusted_duration < 1) {
            adjusted_duration = 1;
        }
    }

    if (cur_time - png_ctx->last_frame_time < adjusted_duration) {
        return LV_RES_INV;
    }

    if (png_ctx->is_normal_png) {
        return process_normal_png_frame(png_ctx);
    } else {
        if (png_ctx->frame_count == png_ctx->num_frames) {
            png_ctx->status = PLAYER_STATUS_END;
            return LV_RES_INV;
        }
        if (process_apng_frame(png_ctx, png_ctx->frame_count) != LV_RES_OK) {
            return LV_RES_INV;
        }
    }

    /* Update frame counter */
    png_ctx->frame_count++;

    /* Update next frame time */
    png_ctx->last_frame_time = lv_tick_get();

    return LV_RES_OK;
}

/* Memory allocation functions */
static int alloc_player_frame_buffer(struct frame_allocator *p, struct mpp_frame *frame,
                              int width, int height, enum mpp_pixel_format format)
{
    struct png_player_ctx *ctx = (struct png_player_ctx *)p;
    int alloc_buffer_times = ctx->allocated_frame_count;
    if (ctx->align_addr[alloc_buffer_times] == 0) {
        return 0;
    }

    frame->buf.format = ctx->bitmap.format;
    frame->buf.size.width = ctx->bitmap.width;
    frame->buf.size.height = ctx->bitmap.height;
    frame->buf.stride[0] = ctx->bitmap.stride;
    frame->buf.buf_type = MPP_PHY_ADDR;
    frame->buf.phy_addr[0] = (unsigned long)ctx->align_addr[alloc_buffer_times];
    ctx->allocated_frame_count++;

    LV_LOG_INFO("alloc buffer: w = %d, h = %d, stride = %d, fmt = %d, data = %p",
                 (int)ctx->bitmap.width, (int)ctx->bitmap.height,
                 (int)ctx->bitmap.stride, (int)ctx->bitmap.format, ctx->align_addr[alloc_buffer_times]);
    return 0;
}

static int free_player_frame_buffer(struct frame_allocator *p, struct mpp_frame *frame)
{
    /* No special cleanup needed for PNG frames */
    return 0;
}

static int close_allocator(struct frame_allocator *p)
{
    /* No special cleanup needed for allocator */
    return 0;
}

/* Utility functions */
static void png_update_image_desc(struct png_player_ctx *ctx)
{
    /* Setup LVGL image descriptor */
    lv_image_dsc_t *image_dst = (lv_image_dsc_t *)ctx->image_src;
#if LVGL_VERSION_MAJOR == 8
    ctx->buf.buf_type = MPP_PHY_ADDR;
    ctx->buf.size.width = ctx->bitmap.width;
    ctx->buf.size.height = ctx->bitmap.height;
    ctx->buf.format = ctx->bitmap.format;
    ctx->buf.stride[0] = ctx->bitmap.stride;
    ctx->buf.phy_addr[0] = (unsigned int)(uintptr_t)ctx->image_data;

    image_dst->header.always_zero = 0;
    image_dst->header.w = ctx->bitmap.width;
    image_dst->header.h = ctx->bitmap.height;
    image_dst->header.cf = LV_IMG_CF_RESERVED_16;
    image_dst->data = (uint8_t *)&ctx->buf;
#elif LVGL_VERSION_MAJOR == 9
    image_dst->header.w = ctx->bitmap.width;
    image_dst->header.h = ctx->bitmap.height;
    image_dst->header.cf = backend_fmt_mpp_to_lv(ctx->bitmap.format);
    image_dst->header.stride = ctx->bitmap.stride;
    image_dst->header.flags = 0;
    image_dst->header.magic = LV_IMAGE_HEADER_MAGIC;
    image_dst->data = ctx->image_data;
    image_dst->data_size = ctx->bitmap.stride * ctx->bitmap.height;
#endif
}

static uint32_t png_get_package_size(struct png_player_ctx *ctx)
{
    uint32_t size = 0;
    uint32_t max_size = 0;
    uint32_t package_size = 0;

    if (ctx->is_normal_png == true) {
        lv_fs_seek(&ctx->file, 0, LV_FS_SEEK_END);
        lv_fs_tell(&ctx->file, &package_size);
        lv_fs_seek(&ctx->file, 0, LV_FS_SEEK_SET);
    } else {
        package_size += ctx->aux_chunks_size + 256;
        if (ctx->dcTL.max_chunk_size != 0) {
            package_size += ctx->dcTL.max_chunk_size;
            return package_size;
        }

        for (int i = 0; i < ctx->num_frames; i++) {
            size = 8 + 25 + ctx->dcTL.frame_chunks_length[i] + 12;
            if (max_size < size)
                max_size = size;
        }
        package_size = max_size;
    }
    return package_size;
}

static void convert_to_lvgl_path(const char *path, char *lv_path, uint16_t max_len)
{
    char prefix[10] = {0};
    snprintf(prefix, sizeof(prefix), "%c:", LV_FS_POSIX_LETTER);
    if (strncmp(path, prefix, strlen(prefix)) != 0) {
        snprintf(lv_path, max_len, "%s%s", prefix, path);
    } else {
        snprintf(lv_path, max_len, "%s", path);
    }
    lv_path[max_len -1] = '\0';
}

#if PNG_DUMPING_IMAGE
#include <sys/stat.h>
#include <unistd.h>

static void create_directory_if_not_exists(const char *path)
{
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        char tmp[256] = {0};
        char *p = NULL;
        size_t len;

        snprintf(tmp, sizeof(tmp), "%s", path);
        len = strlen(tmp);
        if (tmp[len - 1] == '/')
            tmp[len - 1] = 0;

        for (p = tmp + 1; *p; p++) {
            if (*p == '/') {
                *p = 0;
                if (stat(tmp, &st) == -1) {
                    mkdir(tmp, 0755);
                }
                *p = '/';
            }
        }

        if (stat(tmp, &st) == -1) {
            mkdir(tmp, 0755);
        }
    }
}

static void png_debug_save_frame(uint8_t *data, uint32_t len, uint16_t frame_index, const char* suffix, char *path)
{
    lv_fs_file_t file = {0};
    char lvgl_path[256] = {0};
    char full_path[256] = {0};
    char filename[32] = {0};
    lv_fs_res_t res = LV_FS_RES_OK;

    if (!data || !path) {
        LV_LOG_ERROR("Invalid parameters: data=%p, path=%p", data, path);
        return;
    }

    if (len == 0) {
        LV_LOG_WARN("Frame data length is zero, frame_index=%d", (int)frame_index);
        return;
    }

    create_directory_if_not_exists(path);

    if (suffix == NULL) {
        suffix = "bin";
    }

    snprintf(filename, sizeof(filename), "frame_%04d.%s", frame_index, suffix);

    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

    convert_to_lvgl_path((const char *)full_path, lvgl_path, sizeof(lvgl_path));

    res = lv_fs_open(&file, lvgl_path, LV_FS_MODE_WR);
    if (res != LV_FS_RES_OK) {
        LV_LOG_ERROR("Failed to open file for writing: path=%s, error=%d", lvgl_path, res);
        return;
    }

    uint32_t written = 0;
    res = lv_fs_write(&file, data, len, &written);
    if (res != LV_FS_RES_OK || written != len) {
        LV_LOG_ERROR("Failed to write frame data: path=%s, error=%d", lvgl_path, res);
        lv_fs_close(&file);
        return;
    }

    lv_fs_close(&file);

    LV_LOG_INFO("save %s file success: path=%s", suffix, lvgl_path);
}

static int lv_fmt_to_mpp_fmt(lv_color_format_t cf)
{
    switch(cf) {
    case LV_COLOR_FORMAT_RGB565: return MPP_FMT_RGB_565;
    case LV_COLOR_FORMAT_RGB888: return MPP_FMT_RGB_888;
    case LV_COLOR_FORMAT_ARGB8888: return MPP_FMT_ARGB_8888;
    default:
        LV_LOG_ERROR("unsupported format:%d", (int)cf);
        return -1;
    }
}

static int mpp_fmt_to_lv_fmt(enum mpp_pixel_format cf)
{
    switch(cf) {
    case MPP_FMT_RGB_565: return LV_COLOR_FORMAT_RGB565;
    case MPP_FMT_RGB_888: return LV_COLOR_FORMAT_RGB888;
    case MPP_FMT_ARGB_8888: return LV_COLOR_FORMAT_ARGB8888;
    default:
        LV_LOG_ERROR("unsupported format:%d", (int)cf);
        return -1;
    }
}

static int lv_fmt_to_bpp(lv_color_format_t cf)
{
    switch(cf) {
    case LV_COLOR_FORMAT_RGB565:    return 16;
    case LV_COLOR_FORMAT_RGB888:    return 24;
    case LV_COLOR_FORMAT_ARGB8888:  return 32;
    default:
        LV_LOG_ERROR("unsupported format:%d", (int)cf);
        return -1;
    }
}

static int lv_image_save_as_bmp(const char* path, lv_image_dsc_t *dsc)
{
    lv_fs_res_t res = LV_FS_RES_OK;
    uint8_t file_header[14] = {0};
    uint8_t info_header[40] = {0};
    uint32_t pixel_data_size = 0;
    lv_fs_file_t file = {0};
    int bytes_per_pixel = 0;
    uint32_t file_size = 0;
    uint32_t written = 0;
    int is_rgb565 = 0;
    int row_size = 0;
    int bpp = 0;

    LV_UNUSED(lv_fmt_to_mpp_fmt);

    bpp = lv_fmt_to_bpp(dsc->header.cf);
    if (bpp < 0) {
        LV_LOG_ERROR("unsupported format:%d", (int)dsc->header.cf);
        return -1;
    }

    is_rgb565 = (dsc->header.cf == LV_COLOR_FORMAT_RGB565);
    bytes_per_pixel = bpp / 8;
    row_size = (dsc->header.w * bytes_per_pixel + 3) & ~3;
    pixel_data_size = dsc->header.h * (uint32_t)row_size;
    file_size = 54 + pixel_data_size;

    file_header[0] = 'B';
    file_header[1] = 'M';
    file_header[2] = (uint8_t)(file_size);
    file_header[3] = (uint8_t)(file_size >> 8);
    file_header[4] = (uint8_t)(file_size >> 16);
    file_header[5] = (uint8_t)(file_size >> 24);
    file_header[10] = 54;

    info_header[0] = 40;  // header size
    info_header[4] = (uint8_t)(dsc->header.w);
    info_header[5] = (uint8_t)(dsc->header.w >> 8);
    info_header[6] = (uint8_t)(dsc->header.w >> 16);
    info_header[7] = (uint8_t)(dsc->header.w >> 24);
    info_header[8] = (uint8_t)(dsc->header.h);
    info_header[9] = (uint8_t)(dsc->header.h >> 8);
    info_header[10] = (uint8_t)(dsc->header.h >> 16);
    info_header[11] = (uint8_t)(dsc->header.h >> 24);
    info_header[12] = 1;  // planes
    info_header[13] = 0;
    info_header[14] = (uint8_t)(bpp);
    info_header[15] = (uint8_t)(bpp >> 8);
    info_header[16] = is_rgb565 ? 3 : 0;  // compression: BI_RGB
    info_header[17] = is_rgb565 ? 0 : 0;
    info_header[18] = 0;
    info_header[19] = 0;
    info_header[20] = (uint8_t)(pixel_data_size);
    info_header[21] = (uint8_t)(pixel_data_size >> 8);
    info_header[22] = (uint8_t)(pixel_data_size >> 16);
    info_header[23] = (uint8_t)(pixel_data_size >> 24);

    res = lv_fs_open(&file, path, LV_FS_MODE_WR);
    if (res != LV_FS_RES_OK) {
        LV_LOG_ERROR("Failed to open file for writing: path=%s, error=%d", path, res);
        return -1;
    }

    res = lv_fs_write(&file, file_header, 14, &written);
    if (res != LV_FS_RES_OK || written != 14) {
        LV_LOG_ERROR("Failed to write frame data: path=%s, error=%d", path, res);
        lv_fs_close(&file);
        return -1;
    }

    res = lv_fs_write(&file, info_header, 40, &written);
    if (res != LV_FS_RES_OK || written != 40) {
        LV_LOG_ERROR("Failed to write BMP info header: path=%s, error=%d", path, res);
        lv_fs_close(&file);
        return -1;
    }

    int row_bytes = dsc->header.w * bytes_per_pixel;
    int padding = (4 - (row_bytes % 4)) % 4;
    uint8_t *row_buffer = lv_malloc(row_bytes + padding);

    for (int y = dsc->header.h - 1; y >= 0; y--) {
        uint8_t *src_row = (uint8_t *)dsc->data + y * dsc->header.stride;

        memcpy(row_buffer, src_row, row_bytes);
        memset(row_buffer + row_bytes, 0, padding);

        res = lv_fs_write(&file, row_buffer, row_bytes + padding, &written);
        if (res != LV_FS_RES_OK || written != row_bytes + padding) {
            LV_LOG_ERROR("Failed to write BMP row data");
            break;
        }
    }
    lv_free(row_buffer);

    lv_fs_close(&file);

    LV_LOG_INFO("save bmp file success: path=%s", path);

    return 0;
}

static void png_debug_save_bmp_frame(PNG_Bitmap *bitmap, uint16_t frame_index, char *path)
{
    lv_image_dsc_t bmp_dsc = {0};
    char lvgl_path[256] = {0};
    char full_path[256] = {0};
    char filename[32] = {0};
    int ret = 0;

    if (!bitmap || !path) {
        LV_LOG_ERROR("Invalid parameters: data=%p, path=%p", bitmap, path);
        return;
    }

    create_directory_if_not_exists(path);

    bmp_dsc.data = bitmap->data;
    bmp_dsc.header.w = bitmap->width;
    bmp_dsc.header.h = bitmap->height;
    bmp_dsc.header.stride = bitmap->stride;
    bmp_dsc.header.cf = mpp_fmt_to_lv_fmt(bitmap->format);
    bmp_dsc.data_size = bitmap->stride * bitmap->height;

    snprintf(filename, sizeof(filename), "frame_%04d.bmp", frame_index);
    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);
    convert_to_lvgl_path((const char *)full_path, lvgl_path, sizeof(lvgl_path));

    ret = lv_image_save_as_bmp(lvgl_path, &bmp_dsc);
    if (ret < 0) {
        LV_LOG_ERROR("create bmp file error\n");
    }
}
#endif
#endif /* LV_USE_AIC_SIMULATOR == 0 */
