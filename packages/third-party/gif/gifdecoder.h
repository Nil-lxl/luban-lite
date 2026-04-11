/**
* gif lib,copy from lvgl.
*
*/

#ifndef _GIF_DECODER_H_
#define _GIF_DECODER_H_

#include <stdint.h>
#include "aic_core.h"

#define PRINTF printf
//img mem
#define GIF_IMG_ALLOC(size) aicos_malloc(MEM_CMA,size)
#define GIF_IMG_FREE(ptr) aicos_free(MEM_CMA,ptr)
#define GIF_MEM_ALLOC  malloc
#define GIF_MEM_FREE  free
#define GIF_MEM_REALLOC realloc

typedef union {
    struct {
        uint16_t blue : 5;
        uint16_t green : 6;
        uint16_t red : 5;
    } ch;
    uint16_t full;
} lv_color16_t;

typedef union {
    struct {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
        uint8_t alpha;
    } ch;
    uint32_t full;
} lv_color32_t;

#if GIF_COLOR_FORMAT == 0
    #define GIF_COLOR_DEPTH 32
#elif GIF_COLOR_FORMAT == 14
    #define GIF_COLOR_DEPTH 16
#endif

#define _GIF_CONCAT3(x, y, z) x ## y ## z
#define GIF_CONCAT3(x, y, z) _GIF_CONCAT3(x, y, z)
#define _GIF_CONCAT(x, y) x ## y
#define GIF_CONCAT(x, y) _GIF_CONCAT(x, y)

typedef GIF_CONCAT3(lv_color, GIF_COLOR_DEPTH, _t) gif_color_t;

#define _GIF_COLOR_ZERO_INITIALIZER16  {{0x00, 0x00, 0x00}}
#define GIF_COLOR_MAKE16(r8, g8, b8) {{(uint8_t)((b8 >> 3) & 0x1FU), (uint8_t)((g8 >> 2) & 0x3FU), (uint8_t)((r8 >> 3) & 0x1FU)}}

# define _GIF_COLOR_ZERO_INITIALIZER32  {{0x00, 0x00, 0x00, 0x00}}
# define GIF_COLOR_MAKE32(r8, g8, b8) {{b8, g8, r8, 0xff}} /*Fix 0xff alpha*/

#define GIF_COLOR_MAKE(r8, g8, b8) GIF_CONCAT(GIF_COLOR_MAKE, GIF_COLOR_DEPTH)(r8, g8, b8)
#define _GIF_COLOR_MAKE_TYPE_HELPER (gif_color_t)

static inline gif_color_t gif_color_make(uint8_t r, uint8_t g, uint8_t b)
{
    return (gif_color_t) GIF_COLOR_MAKE(r, g, b);
}

typedef struct gd_palette {
    int size;
    uint8_t colors[0x100 * 3];
} gd_palette;

typedef struct gd_gce {
    uint16_t delay;
    uint8_t tindex;
    uint8_t disposal;
    int input;
    int transparency;
} gd_gce;

typedef struct gd_gif {
    int32_t fd;
    const char * data;
    uint8_t is_file;
    uint32_t f_rw_p;
    int32_t anim_start;
    uint16_t width, height;
    uint16_t depth;
    uint16_t loop_count;
    gd_gce gce;
    gd_palette *palette;
    gd_palette lct, gct;
    void (*plain_text)(
        struct gd_gif *gif, uint16_t tx, uint16_t ty,
        uint16_t tw, uint16_t th, uint8_t cw, uint8_t ch,
        uint8_t fg, uint8_t bg
    );
    void (*comment)(struct gd_gif *gif);
    void (*application)(struct gd_gif *gif, char id[8], char auth[3]);
    uint16_t fx, fy, fw, fh;
    uint8_t bgindex;
    uint8_t *canvas, *frame;
} gd_gif;

gd_gif * gifdec_open_gif_file(const char *fname);
gd_gif * gifdec_open_gif_data(const void *data);
void gifdec_render_frame(gd_gif *gif, uint8_t *buffer);
int gifdec_get_frame(gd_gif *gif);
void gifdec_rewind(gd_gif *gif);
void gifdec_close_gif(gd_gif *gif);

#endif /* _GIF_DECODER_H_ */
