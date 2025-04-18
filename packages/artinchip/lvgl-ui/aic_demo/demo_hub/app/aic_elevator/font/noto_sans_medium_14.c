/*******************************************************************************
 * Size: 14 px
 * Bpp: 4
 * Opts: --bpp 4 --size 14 --no-compress --font NotoSansSChinese-Medium.ttf --symbols QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm : 0987654321-星期一二三四五主页面 --format lvgl -o noto_sans_medium_14.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef NOTO_SANS_MEDIUM_14
#define NOTO_SANS_MEDIUM_14 1
#endif

#if NOTO_SANS_MEDIUM_14

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+002D "-" */
    0x0, 0x0, 0x5, 0xff, 0xf5, 0x13, 0x33, 0x10,

    /* U+0030 "0" */
    0x0, 0x9f, 0xe9, 0x0, 0x9, 0xf7, 0x7f, 0x80,
    0xf, 0x80, 0x9, 0xf0, 0x3f, 0x40, 0x5, 0xf3,
    0x5f, 0x30, 0x3, 0xf4, 0x5f, 0x30, 0x3, 0xf4,
    0x3f, 0x50, 0x5, 0xf3, 0xf, 0x90, 0x9, 0xf0,
    0x8, 0xf6, 0x7f, 0x80, 0x0, 0x9f, 0xe9, 0x0,

    /* U+0031 "1" */
    0x17, 0xdf, 0x10, 0x4, 0xce, 0xf1, 0x0, 0x0,
    0x8f, 0x10, 0x0, 0x8, 0xf1, 0x0, 0x0, 0x8f,
    0x10, 0x0, 0x8, 0xf1, 0x0, 0x0, 0x8f, 0x10,
    0x0, 0x8, 0xf1, 0x0, 0x45, 0xbf, 0x65, 0xc,
    0xff, 0xff, 0xf1,

    /* U+0032 "2" */
    0x4, 0xcf, 0xd7, 0x0, 0x3f, 0x95, 0xaf, 0x60,
    0x2, 0x0, 0xe, 0xb0, 0x0, 0x0, 0xd, 0xb0,
    0x0, 0x0, 0x2f, 0x60, 0x0, 0x0, 0xcd, 0x0,
    0x0, 0xa, 0xf2, 0x0, 0x0, 0x9f, 0x40, 0x0,
    0xb, 0xf9, 0x66, 0x61, 0x6f, 0xff, 0xff, 0xf4,

    /* U+0033 "3" */
    0x4, 0xcf, 0xe9, 0x0, 0x1d, 0x95, 0x8f, 0x90,
    0x0, 0x0, 0xd, 0xc0, 0x0, 0x1, 0x6f, 0x70,
    0x0, 0x7f, 0xf8, 0x0, 0x0, 0x14, 0x8f, 0x80,
    0x0, 0x0, 0x8, 0xf1, 0x2, 0x0, 0x8, 0xf1,
    0x6f, 0x95, 0x8f, 0xc0, 0x6, 0xdf, 0xe9, 0x10,

    /* U+0034 "4" */
    0x0, 0x1, 0xef, 0x40, 0x0, 0xa, 0xef, 0x40,
    0x0, 0x4f, 0x7f, 0x40, 0x1, 0xe9, 0x4f, 0x40,
    0xa, 0xd0, 0x4f, 0x40, 0x4f, 0x40, 0x4f, 0x40,
    0xbf, 0xff, 0xff, 0xf9, 0x23, 0x33, 0x6f, 0x61,
    0x0, 0x0, 0x4f, 0x40, 0x0, 0x0, 0x4f, 0x40,

    /* U+0035 "5" */
    0x8, 0xff, 0xff, 0xb0, 0x9, 0xe6, 0x66, 0x40,
    0xa, 0xc0, 0x0, 0x0, 0xb, 0xee, 0xea, 0x10,
    0x6, 0x83, 0x6f, 0xb0, 0x0, 0x0, 0x9, 0xf1,
    0x0, 0x0, 0x7, 0xf2, 0x3, 0x0, 0xa, 0xf0,
    0x6f, 0x96, 0x9f, 0x80, 0x6, 0xdf, 0xe7, 0x0,

    /* U+0036 "6" */
    0x0, 0x4c, 0xfd, 0x70, 0x4, 0xfa, 0x59, 0xc0,
    0xc, 0xa0, 0x0, 0x0, 0x1f, 0x6b, 0xfc, 0x30,
    0x3f, 0xe6, 0x4c, 0xe0, 0x3f, 0x60, 0x4, 0xf4,
    0x2f, 0x60, 0x2, 0xf5, 0xe, 0xb0, 0x5, 0xf3,
    0x6, 0xf8, 0x5d, 0xc0, 0x0, 0x6d, 0xfa, 0x10,

    /* U+0037 "7" */
    0x5f, 0xff, 0xff, 0xf5, 0x16, 0x66, 0x6c, 0xe1,
    0x0, 0x0, 0x3f, 0x30, 0x0, 0x0, 0xca, 0x0,
    0x0, 0x3, 0xf3, 0x0, 0x0, 0x9, 0xe0, 0x0,
    0x0, 0xd, 0xb0, 0x0, 0x0, 0xf, 0x80, 0x0,
    0x0, 0x2f, 0x70, 0x0, 0x0, 0x3f, 0x60, 0x0,

    /* U+0038 "8" */
    0x0, 0x9e, 0xea, 0x10, 0x9, 0xe4, 0x4d, 0xa0,
    0xd, 0x90, 0x7, 0xe0, 0xa, 0xe2, 0xa, 0xb0,
    0x0, 0xcf, 0xbd, 0x10, 0x7, 0xd6, 0xcf, 0x60,
    0x2f, 0x30, 0x8, 0xf2, 0x5f, 0x20, 0x3, 0xf4,
    0x1f, 0xc4, 0x4c, 0xe1, 0x3, 0xbf, 0xeb, 0x20,

    /* U+0039 "9" */
    0x2, 0xbf, 0xd6, 0x0, 0xd, 0xc5, 0x8f, 0x60,
    0x4f, 0x30, 0xa, 0xe0, 0x5f, 0x20, 0x5, 0xf2,
    0x2f, 0x90, 0x2c, 0xf3, 0x6, 0xff, 0xe8, 0xf2,
    0x0, 0x2, 0x7, 0xf0, 0x0, 0x0, 0xc, 0xb0,
    0xd, 0x96, 0xcf, 0x30, 0x7, 0xef, 0xc4, 0x0,

    /* U+003A ":" */
    0xbd, 0xe, 0xf1, 0x12, 0x0, 0x0, 0x0, 0x1,
    0x20, 0xef, 0x1b, 0xd0,

    /* U+0041 "A" */
    0x0, 0xc, 0xf7, 0x0, 0x0, 0x1, 0xfc, 0xc0,
    0x0, 0x0, 0x6e, 0x4f, 0x20, 0x0, 0xc, 0xa0,
    0xf7, 0x0, 0x1, 0xf5, 0xb, 0xd0, 0x0, 0x7f,
    0x10, 0x7f, 0x20, 0xc, 0xff, 0xff, 0xf8, 0x2,
    0xf8, 0x33, 0x3d, 0xd0, 0x7f, 0x20, 0x0, 0x7f,
    0x3d, 0xd0, 0x0, 0x2, 0xf8,

    /* U+0042 "B" */
    0xaf, 0xff, 0xeb, 0x30, 0xaf, 0x44, 0x6e, 0xe0,
    0xaf, 0x0, 0x7, 0xf2, 0xaf, 0x0, 0x3c, 0xd0,
    0xaf, 0xff, 0xff, 0x40, 0xaf, 0x33, 0x5b, 0xf4,
    0xaf, 0x0, 0x0, 0xea, 0xaf, 0x0, 0x0, 0xfa,
    0xaf, 0x44, 0x6c, 0xf4, 0xaf, 0xff, 0xeb, 0x40,

    /* U+0043 "C" */
    0x0, 0x19, 0xef, 0xd5, 0x0, 0x1d, 0xfa, 0x7a,
    0xf2, 0x9, 0xf4, 0x0, 0x1, 0x0, 0xfb, 0x0,
    0x0, 0x0, 0x2f, 0x80, 0x0, 0x0, 0x2, 0xf8,
    0x0, 0x0, 0x0, 0xf, 0xb0, 0x0, 0x0, 0x0,
    0xaf, 0x40, 0x0, 0x30, 0x1, 0xef, 0xa7, 0xaf,
    0x50, 0x1, 0x9e, 0xfd, 0x60,

    /* U+0044 "D" */
    0xaf, 0xff, 0xd8, 0x0, 0xaf, 0x56, 0xaf, 0xd0,
    0xaf, 0x0, 0x6, 0xf8, 0xaf, 0x0, 0x0, 0xed,
    0xaf, 0x0, 0x0, 0xbf, 0xaf, 0x0, 0x0, 0xbf,
    0xaf, 0x0, 0x0, 0xed, 0xaf, 0x0, 0x7, 0xf7,
    0xaf, 0x56, 0xbf, 0xc0, 0xaf, 0xff, 0xd8, 0x0,

    /* U+0045 "E" */
    0xaf, 0xff, 0xff, 0x7a, 0xf6, 0x66, 0x62, 0xaf,
    0x0, 0x0, 0xa, 0xf1, 0x11, 0x10, 0xaf, 0xff,
    0xfc, 0xa, 0xf4, 0x44, 0x30, 0xaf, 0x0, 0x0,
    0xa, 0xf0, 0x0, 0x0, 0xaf, 0x66, 0x66, 0x3a,
    0xff, 0xff, 0xf9,

    /* U+0046 "F" */
    0xaf, 0xff, 0xff, 0x7a, 0xf6, 0x66, 0x62, 0xaf,
    0x0, 0x0, 0xa, 0xf1, 0x11, 0x10, 0xaf, 0xff,
    0xfc, 0xa, 0xf4, 0x44, 0x30, 0xaf, 0x0, 0x0,
    0xa, 0xf0, 0x0, 0x0, 0xaf, 0x0, 0x0, 0xa,
    0xf0, 0x0, 0x0,

    /* U+0047 "G" */
    0x0, 0x8, 0xdf, 0xe8, 0x0, 0x1d, 0xfa, 0x78,
    0xe5, 0x9, 0xf5, 0x0, 0x0, 0x0, 0xfb, 0x0,
    0x1, 0x11, 0x2f, 0x80, 0xb, 0xff, 0xd2, 0xf8,
    0x0, 0x24, 0xbd, 0xf, 0xb0, 0x0, 0xa, 0xd0,
    0xaf, 0x40, 0x0, 0xad, 0x1, 0xef, 0xa7, 0x9f,
    0xc0, 0x1, 0x8d, 0xfe, 0x91,

    /* U+0048 "H" */
    0xaf, 0x0, 0x0, 0x9f, 0xa, 0xf0, 0x0, 0x9,
    0xf0, 0xaf, 0x0, 0x0, 0x9f, 0xa, 0xf2, 0x22,
    0x2a, 0xf0, 0xaf, 0xff, 0xff, 0xff, 0xa, 0xf5,
    0x55, 0x5b, 0xf0, 0xaf, 0x0, 0x0, 0x9f, 0xa,
    0xf0, 0x0, 0x9, 0xf0, 0xaf, 0x0, 0x0, 0x9f,
    0xa, 0xf0, 0x0, 0x9, 0xf0,

    /* U+0049 "I" */
    0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
    0xaf, 0xaf,

    /* U+004A "J" */
    0x0, 0x0, 0x3f, 0x60, 0x0, 0x3, 0xf6, 0x0,
    0x0, 0x3f, 0x60, 0x0, 0x3, 0xf6, 0x0, 0x0,
    0x3f, 0x60, 0x0, 0x3, 0xf6, 0x0, 0x0, 0x3f,
    0x60, 0x40, 0x5, 0xf4, 0x5f, 0xa7, 0xee, 0x0,
    0x7e, 0xfc, 0x30,

    /* U+004B "K" */
    0xaf, 0x0, 0x7, 0xf5, 0xa, 0xf0, 0x4, 0xf8,
    0x0, 0xaf, 0x2, 0xfa, 0x0, 0xa, 0xf1, 0xdd,
    0x0, 0x0, 0xaf, 0xcf, 0xe0, 0x0, 0xa, 0xff,
    0x7f, 0x80, 0x0, 0xaf, 0x60, 0x9f, 0x20, 0xa,
    0xf0, 0x1, 0xec, 0x0, 0xaf, 0x0, 0x6, 0xf5,
    0xa, 0xf0, 0x0, 0xd, 0xe1,

    /* U+004C "L" */
    0xaf, 0x0, 0x0, 0xa, 0xf0, 0x0, 0x0, 0xaf,
    0x0, 0x0, 0xa, 0xf0, 0x0, 0x0, 0xaf, 0x0,
    0x0, 0xa, 0xf0, 0x0, 0x0, 0xaf, 0x0, 0x0,
    0xa, 0xf0, 0x0, 0x0, 0xaf, 0x66, 0x66, 0x2a,
    0xff, 0xff, 0xf5,

    /* U+004D "M" */
    0xaf, 0x60, 0x0, 0xc, 0xf4, 0xaf, 0xc0, 0x0,
    0x2f, 0xf4, 0xac, 0xf2, 0x0, 0x8b, 0xf4, 0xac,
    0xb8, 0x0, 0xe7, 0xf4, 0xac, 0x5e, 0x4, 0xe3,
    0xf4, 0xad, 0xf, 0x49, 0x93, 0xf4, 0xad, 0xa,
    0x9e, 0x33, 0xf4, 0xad, 0x4, 0xfd, 0x3, 0xf4,
    0xad, 0x0, 0xe7, 0x3, 0xf4, 0xad, 0x0, 0x0,
    0x3, 0xf4,

    /* U+004E "N" */
    0xaf, 0x40, 0x0, 0xae, 0xaf, 0xd0, 0x0, 0xae,
    0xac, 0xd6, 0x0, 0xae, 0xad, 0x5e, 0x0, 0xae,
    0xad, 0xc, 0x80, 0xae, 0xae, 0x3, 0xf1, 0x9e,
    0xae, 0x0, 0xb9, 0x8e, 0xae, 0x0, 0x2f, 0x9e,
    0xae, 0x0, 0x9, 0xfe, 0xae, 0x0, 0x1, 0xfe,

    /* U+004F "O" */
    0x0, 0x1a, 0xef, 0xd6, 0x0, 0x2, 0xef, 0x97,
    0xcf, 0x90, 0xa, 0xf3, 0x0, 0xa, 0xf3, 0xf,
    0xb0, 0x0, 0x2, 0xf9, 0x2f, 0x80, 0x0, 0x0,
    0xfb, 0x2f, 0x80, 0x0, 0x0, 0xfb, 0xf, 0xb0,
    0x0, 0x2, 0xf9, 0xa, 0xf3, 0x0, 0xa, 0xf3,
    0x1, 0xef, 0x97, 0xcf, 0x90, 0x0, 0x19, 0xef,
    0xd6, 0x0,

    /* U+0050 "P" */
    0xaf, 0xff, 0xea, 0x20, 0xaf, 0x55, 0x7e, 0xe1,
    0xaf, 0x0, 0x5, 0xf4, 0xaf, 0x0, 0x4, 0xf5,
    0xaf, 0x11, 0x3c, 0xf1, 0xaf, 0xff, 0xfd, 0x40,
    0xaf, 0x43, 0x20, 0x0, 0xaf, 0x0, 0x0, 0x0,
    0xaf, 0x0, 0x0, 0x0, 0xaf, 0x0, 0x0, 0x0,

    /* U+0051 "Q" */
    0x0, 0x19, 0xef, 0xc6, 0x0, 0x0, 0x1e, 0xf9,
    0x7c, 0xf8, 0x0, 0x9, 0xf4, 0x0, 0xb, 0xf2,
    0x0, 0xfc, 0x0, 0x0, 0x3f, 0x80, 0x2f, 0x90,
    0x0, 0x0, 0xfa, 0x2, 0xf7, 0x0, 0x0, 0xe,
    0xb0, 0xf, 0x90, 0x0, 0x0, 0xf9, 0x0, 0xdd,
    0x0, 0x0, 0x4f, 0x60, 0x6, 0xf6, 0x0, 0xc,
    0xf0, 0x0, 0xb, 0xfb, 0x9e, 0xf5, 0x0, 0x0,
    0x5, 0xcf, 0xb2, 0x0, 0x0, 0x0, 0x1, 0xed,
    0x41, 0x20, 0x0, 0x0, 0x2, 0xcf, 0xff, 0x0,
    0x0, 0x0, 0x0, 0x23, 0x20,

    /* U+0052 "R" */
    0xaf, 0xff, 0xeb, 0x30, 0xaf, 0x55, 0x7d, 0xe1,
    0xaf, 0x0, 0x4, 0xf5, 0xaf, 0x0, 0x3, 0xf6,
    0xaf, 0x11, 0x3c, 0xf2, 0xaf, 0xff, 0xfe, 0x50,
    0xaf, 0x45, 0xf9, 0x0, 0xaf, 0x0, 0x9f, 0x30,
    0xaf, 0x0, 0xe, 0xd0, 0xaf, 0x0, 0x5, 0xf7,

    /* U+0053 "S" */
    0x0, 0x8d, 0xfd, 0x70, 0x9, 0xfa, 0x7a, 0xf5,
    0xd, 0xd0, 0x0, 0x10, 0xb, 0xf5, 0x0, 0x0,
    0x1, 0xcf, 0xd7, 0x10, 0x0, 0x3, 0xaf, 0xe3,
    0x0, 0x0, 0x2, 0xec, 0x5, 0x0, 0x0, 0xdd,
    0x2f, 0xe8, 0x7b, 0xf7, 0x2, 0xae, 0xfd, 0x60,

    /* U+0054 "T" */
    0x9f, 0xff, 0xff, 0xff, 0x23, 0x66, 0xbf, 0x76,
    0x60, 0x0, 0x8, 0xf1, 0x0, 0x0, 0x0, 0x8f,
    0x10, 0x0, 0x0, 0x8, 0xf1, 0x0, 0x0, 0x0,
    0x8f, 0x10, 0x0, 0x0, 0x8, 0xf1, 0x0, 0x0,
    0x0, 0x8f, 0x10, 0x0, 0x0, 0x8, 0xf1, 0x0,
    0x0, 0x0, 0x8f, 0x10, 0x0,

    /* U+0055 "U" */
    0xbe, 0x0, 0x0, 0x9f, 0xbe, 0x0, 0x0, 0x9f,
    0xbe, 0x0, 0x0, 0x9f, 0xbe, 0x0, 0x0, 0x9f,
    0xbe, 0x0, 0x0, 0x9f, 0xbe, 0x0, 0x0, 0x9f,
    0xaf, 0x0, 0x0, 0xbe, 0x7f, 0x50, 0x1, 0xfb,
    0x1e, 0xf9, 0x8d, 0xf4, 0x2, 0xbf, 0xfc, 0x50,

    /* U+0056 "V" */
    0xd, 0xd0, 0x0, 0x7, 0xf2, 0x8, 0xf2, 0x0,
    0xc, 0xd0, 0x3, 0xf6, 0x0, 0xf, 0x80, 0x0,
    0xeb, 0x0, 0x5f, 0x30, 0x0, 0x9f, 0x0, 0xae,
    0x0, 0x0, 0x4f, 0x40, 0xe9, 0x0, 0x0, 0xe,
    0x93, 0xf4, 0x0, 0x0, 0x9, 0xd7, 0xe0, 0x0,
    0x0, 0x4, 0xfe, 0x90, 0x0, 0x0, 0x0, 0xef,
    0x40, 0x0,

    /* U+0057 "W" */
    0x9f, 0x10, 0x8, 0xf2, 0x0, 0x7f, 0x16, 0xf4,
    0x0, 0xcf, 0x60, 0xa, 0xe0, 0x2f, 0x70, 0x1f,
    0xca, 0x0, 0xda, 0x0, 0xfa, 0x5, 0xf7, 0xe0,
    0xf, 0x70, 0xb, 0xd0, 0x9b, 0x3f, 0x23, 0xf4,
    0x0, 0x8f, 0xd, 0x70, 0xf6, 0x6f, 0x10, 0x5,
    0xf3, 0xf3, 0xb, 0xa9, 0xd0, 0x0, 0x1f, 0x9f,
    0x0, 0x7d, 0xba, 0x0, 0x0, 0xef, 0xb0, 0x3,
    0xff, 0x70, 0x0, 0xa, 0xf7, 0x0, 0xf, 0xf4,
    0x0,

    /* U+0058 "X" */
    0x4f, 0x70, 0x1, 0xfa, 0x0, 0xbf, 0x10, 0x8f,
    0x20, 0x2, 0xf8, 0x1f, 0x80, 0x0, 0x9, 0xfa,
    0xe0, 0x0, 0x0, 0x1f, 0xf6, 0x0, 0x0, 0x3,
    0xff, 0x80, 0x0, 0x0, 0xcc, 0x9f, 0x20, 0x0,
    0x5f, 0x41, 0xfa, 0x0, 0xe, 0xb0, 0x7, 0xf4,
    0x7, 0xf3, 0x0, 0xe, 0xd0,

    /* U+0059 "Y" */
    0xc, 0xe0, 0x0, 0x2f, 0x80, 0x4f, 0x50, 0x9,
    0xf1, 0x0, 0xcd, 0x1, 0xf8, 0x0, 0x4, 0xf4,
    0x7f, 0x10, 0x0, 0xc, 0xbe, 0x80, 0x0, 0x0,
    0x4f, 0xf1, 0x0, 0x0, 0x0, 0xfa, 0x0, 0x0,
    0x0, 0xf, 0xa0, 0x0, 0x0, 0x0, 0xfa, 0x0,
    0x0, 0x0, 0xf, 0xa0, 0x0,

    /* U+005A "Z" */
    0xd, 0xff, 0xff, 0xfd, 0x5, 0x66, 0x69, 0xf7,
    0x0, 0x0, 0xe, 0xc0, 0x0, 0x0, 0x9f, 0x20,
    0x0, 0x4, 0xf7, 0x0, 0x0, 0x1e, 0xc0, 0x0,
    0x0, 0xaf, 0x20, 0x0, 0x5, 0xf6, 0x0, 0x0,
    0x1e, 0xe6, 0x66, 0x65, 0x5f, 0xff, 0xff, 0xfe,

    /* U+0061 "a" */
    0x3, 0xae, 0xfa, 0x10, 0xab, 0x67, 0xf9, 0x0,
    0x0, 0xb, 0xe0, 0x5, 0xad, 0xff, 0xb, 0xf8,
    0x3a, 0xf2, 0xf6, 0x0, 0xaf, 0x1f, 0xa2, 0x6f,
    0xf0, 0x6e, 0xea, 0x8f,

    /* U+0062 "b" */
    0xcd, 0x0, 0x0, 0x0, 0xcd, 0x0, 0x0, 0x0,
    0xcc, 0x0, 0x0, 0x0, 0xcc, 0x7e, 0xf9, 0x0,
    0xcf, 0xc6, 0xaf, 0x80, 0xcd, 0x0, 0xd, 0xe0,
    0xcd, 0x0, 0x9, 0xf1, 0xcd, 0x0, 0x9, 0xf0,
    0xcd, 0x0, 0xd, 0xd0, 0xcf, 0x73, 0x9f, 0x60,
    0xca, 0xaf, 0xe6, 0x0,

    /* U+0063 "c" */
    0x0, 0x6d, 0xfc, 0x30, 0x8f, 0xb6, 0xa4, 0x1f,
    0xb0, 0x0, 0x4, 0xf6, 0x0, 0x0, 0x4f, 0x60,
    0x0, 0x1, 0xfb, 0x0, 0x0, 0x9, 0xfa, 0x6a,
    0x80, 0x8, 0xef, 0xc3,

    /* U+0064 "d" */
    0x0, 0x0, 0x0, 0xf9, 0x0, 0x0, 0x0, 0xf9,
    0x0, 0x0, 0x0, 0xf9, 0x0, 0x8e, 0xe8, 0xf9,
    0x9, 0xfa, 0x6b, 0xf9, 0x1f, 0xb0, 0x0, 0xf9,
    0x3f, 0x60, 0x0, 0xf9, 0x3f, 0x60, 0x0, 0xf9,
    0x1f, 0x90, 0x0, 0xf9, 0xa, 0xf6, 0x3a, 0xf9,
    0x1, 0xaf, 0xe8, 0xc9,

    /* U+0065 "e" */
    0x0, 0x7e, 0xfb, 0x20, 0x8, 0xf7, 0x5c, 0xd0,
    0x1f, 0x60, 0x2, 0xf3, 0x4f, 0xff, 0xff, 0xf4,
    0x4f, 0x62, 0x22, 0x20, 0x1f, 0xa0, 0x0, 0x0,
    0x8, 0xfa, 0x57, 0x90, 0x0, 0x7d, 0xfd, 0x60,

    /* U+0066 "f" */
    0x0, 0xaf, 0xe2, 0x6, 0xf8, 0x60, 0x8, 0xf1,
    0x0, 0x8f, 0xff, 0xa0, 0x2a, 0xf6, 0x30, 0x8,
    0xf1, 0x0, 0x8, 0xf1, 0x0, 0x8, 0xf1, 0x0,
    0x8, 0xf1, 0x0, 0x8, 0xf1, 0x0, 0x8, 0xf1,
    0x0,

    /* U+0067 "g" */
    0x1, 0xae, 0xff, 0xfc, 0xc, 0xd3, 0x4f, 0xb4,
    0xf, 0x70, 0xa, 0xb0, 0xd, 0xc0, 0x1e, 0x90,
    0x4, 0xff, 0xfb, 0x10, 0xc, 0x80, 0x0, 0x0,
    0xd, 0xc5, 0x54, 0x10, 0x7, 0xff, 0xff, 0xf7,
    0x2f, 0x30, 0x1, 0xcd, 0x3f, 0x72, 0x16, 0xf8,
    0x6, 0xdf, 0xec, 0x50,

    /* U+0068 "h" */
    0xcd, 0x0, 0x0, 0xc, 0xd0, 0x0, 0x0, 0xcc,
    0x0, 0x0, 0xc, 0xc5, 0xdf, 0x90, 0xcf, 0xd7,
    0xbf, 0x5c, 0xe0, 0x1, 0xf8, 0xcd, 0x0, 0xf,
    0x9c, 0xd0, 0x0, 0xf9, 0xcd, 0x0, 0xf, 0x9c,
    0xd0, 0x0, 0xf9, 0xcd, 0x0, 0xf, 0x90,

    /* U+0069 "i" */
    0xcd, 0xb, 0xc0, 0x0, 0xc, 0xd0, 0xcd, 0xc,
    0xd0, 0xcd, 0xc, 0xd0, 0xcd, 0xc, 0xd0, 0xcd,
    0x0,

    /* U+006A "j" */
    0x0, 0xcd, 0x0, 0xb, 0xc0, 0x0, 0x0, 0x0,
    0xc, 0xd0, 0x0, 0xcd, 0x0, 0xc, 0xd0, 0x0,
    0xcd, 0x0, 0xc, 0xd0, 0x0, 0xcd, 0x0, 0xc,
    0xd0, 0x0, 0xcd, 0x0, 0xc, 0xc0, 0x25, 0xfa,
    0x7, 0xfc, 0x20,

    /* U+006B "k" */
    0xcc, 0x0, 0x0, 0xc, 0xc0, 0x0, 0x0, 0xcc,
    0x0, 0x0, 0xc, 0xc0, 0x8, 0xf3, 0xcc, 0x5,
    0xf6, 0xc, 0xc2, 0xf9, 0x0, 0xcd, 0xdf, 0x30,
    0xc, 0xfe, 0xdc, 0x0, 0xcf, 0x34, 0xf6, 0xc,
    0xc0, 0xb, 0xe1, 0xcc, 0x0, 0x2f, 0x90,

    /* U+006C "l" */
    0xcd, 0xc, 0xd0, 0xcd, 0xc, 0xd0, 0xcd, 0xc,
    0xd0, 0xcd, 0xc, 0xd0, 0xcd, 0xb, 0xe2, 0x5f,
    0x90,

    /* U+006D "m" */
    0xc9, 0x7e, 0xf9, 0x1a, 0xfd, 0x30, 0xcf, 0xa4,
    0xbf, 0xe7, 0x4e, 0xc0, 0xcd, 0x0, 0x3f, 0x80,
    0x9, 0xf0, 0xcd, 0x0, 0x2f, 0x70, 0x8, 0xf1,
    0xcd, 0x0, 0x2f, 0x70, 0x8, 0xf1, 0xcd, 0x0,
    0x2f, 0x70, 0x8, 0xf1, 0xcd, 0x0, 0x2f, 0x70,
    0x8, 0xf1, 0xcd, 0x0, 0x2f, 0x70, 0x8, 0xf1,

    /* U+006E "n" */
    0xc9, 0x7e, 0xf9, 0xc, 0xfb, 0x49, 0xf5, 0xcd,
    0x0, 0x1f, 0x8c, 0xd0, 0x0, 0xf9, 0xcd, 0x0,
    0xf, 0x9c, 0xd0, 0x0, 0xf9, 0xcd, 0x0, 0xf,
    0x9c, 0xd0, 0x0, 0xf9,

    /* U+006F "o" */
    0x0, 0x7e, 0xfc, 0x30, 0x8, 0xf9, 0x6d, 0xf2,
    0x1f, 0xa0, 0x1, 0xfa, 0x4f, 0x60, 0x0, 0xcd,
    0x4f, 0x60, 0x0, 0xcd, 0x1f, 0xa0, 0x1, 0xfa,
    0x8, 0xf9, 0x6d, 0xf2, 0x0, 0x7e, 0xfc, 0x30,

    /* U+0070 "p" */
    0xca, 0x8e, 0xf9, 0x0, 0xcf, 0x93, 0x7f, 0x80,
    0xcd, 0x0, 0xc, 0xe0, 0xcd, 0x0, 0x9, 0xf1,
    0xcd, 0x0, 0xa, 0xf0, 0xcd, 0x0, 0xe, 0xd0,
    0xcf, 0xa6, 0xbf, 0x60, 0xcd, 0x9e, 0xe6, 0x0,
    0xcc, 0x0, 0x0, 0x0, 0xcd, 0x0, 0x0, 0x0,
    0xcd, 0x0, 0x0, 0x0,

    /* U+0071 "q" */
    0x0, 0x8e, 0xe8, 0xb9, 0x9, 0xfa, 0x6b, 0xf9,
    0x1f, 0xb0, 0x0, 0xf9, 0x3f, 0x60, 0x0, 0xf9,
    0x3f, 0x60, 0x0, 0xf9, 0x1f, 0xa0, 0x0, 0xf9,
    0xa, 0xf9, 0x6d, 0xf9, 0x1, 0xaf, 0xe6, 0xf9,
    0x0, 0x0, 0x0, 0xf9, 0x0, 0x0, 0x0, 0xf9,
    0x0, 0x0, 0x0, 0xf9,

    /* U+0072 "r" */
    0x0, 0x0, 0xc, 0x97, 0xea, 0xce, 0xd5, 0x2c,
    0xf1, 0x0, 0xcd, 0x0, 0xc, 0xd0, 0x0, 0xcd,
    0x0, 0xc, 0xd0, 0x0, 0xcd, 0x0, 0x0,

    /* U+0073 "s" */
    0x4, 0xdf, 0xd6, 0x0, 0xfb, 0x47, 0x80, 0x2f,
    0x70, 0x0, 0x0, 0x9f, 0xc6, 0x0, 0x0, 0x3a,
    0xfb, 0x0, 0x0, 0x6, 0xf3, 0x4d, 0x64, 0xbf,
    0x0, 0x8e, 0xfc, 0x40,

    /* U+0074 "t" */
    0x5, 0xf1, 0x0, 0x6, 0xf1, 0x0, 0xaf, 0xff,
    0xf1, 0x2a, 0xf5, 0x40, 0x8, 0xf1, 0x0, 0x8,
    0xf1, 0x0, 0x8, 0xf1, 0x0, 0x7, 0xf1, 0x0,
    0x5, 0xf9, 0x60, 0x0, 0x9f, 0xe2,

    /* U+0075 "u" */
    0xeb, 0x0, 0x2f, 0x7e, 0xb0, 0x2, 0xf7, 0xeb,
    0x0, 0x2f, 0x7e, 0xb0, 0x2, 0xf7, 0xdb, 0x0,
    0x2f, 0x7d, 0xc0, 0x3, 0xf7, 0x9f, 0x65, 0xdf,
    0x71, 0xcf, 0xc3, 0xe7,

    /* U+0076 "v" */
    0xaf, 0x0, 0x4, 0xf4, 0x4f, 0x40, 0x9, 0xe0,
    0xf, 0x90, 0xe, 0x90, 0xa, 0xe0, 0x3f, 0x40,
    0x4, 0xf3, 0x8e, 0x0, 0x0, 0xe7, 0xc9, 0x0,
    0x0, 0x9d, 0xf4, 0x0, 0x0, 0x4f, 0xe0, 0x0,

    /* U+0077 "w" */
    0x7f, 0x10, 0xf, 0xb0, 0x6, 0xf1, 0x3f, 0x50,
    0x4f, 0xf0, 0xa, 0xd0, 0xf, 0x90, 0x8a, 0xf3,
    0xd, 0x90, 0xb, 0xd0, 0xc6, 0xd7, 0x1f, 0x50,
    0x7, 0xf1, 0xf2, 0x9b, 0x5f, 0x10, 0x3, 0xf7,
    0xe0, 0x5f, 0x8d, 0x0, 0x0, 0xee, 0xa0, 0x1f,
    0xe9, 0x0, 0x0, 0xaf, 0x70, 0xd, 0xf5, 0x0,

    /* U+0078 "x" */
    0x4f, 0x60, 0xe, 0xa0, 0xb, 0xe0, 0x7f, 0x20,
    0x2, 0xf9, 0xe8, 0x0, 0x0, 0x8f, 0xe0, 0x0,
    0x0, 0xbf, 0xe0, 0x0, 0x4, 0xf5, 0xf9, 0x0,
    0xd, 0xb0, 0x8f, 0x30, 0x7f, 0x20, 0xd, 0xc0,

    /* U+0079 "y" */
    0x9f, 0x0, 0x4, 0xf4, 0x3f, 0x50, 0x9, 0xe0,
    0xd, 0xb0, 0xe, 0x90, 0x6, 0xf1, 0x3f, 0x30,
    0x1, 0xf6, 0x8e, 0x0, 0x0, 0xab, 0xc8, 0x0,
    0x0, 0x3f, 0xf3, 0x0, 0x0, 0xd, 0xd0, 0x0,
    0x0, 0x1f, 0x70, 0x0, 0x5, 0xce, 0x10, 0x0,
    0x4f, 0xc3, 0x0, 0x0,

    /* U+007A "z" */
    0x1f, 0xff, 0xff, 0x50, 0x55, 0x5e, 0xe0, 0x0,
    0x7, 0xf4, 0x0, 0x2, 0xfa, 0x0, 0x0, 0xce,
    0x10, 0x0, 0x6f, 0x50, 0x0, 0x2f, 0xd5, 0x55,
    0x27, 0xff, 0xff, 0xf7,

    /* U+4E00 "一" */
    0x2, 0x22, 0x22, 0x22, 0x22, 0x22, 0x21, 0x2f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0x4, 0x44,
    0x44, 0x44, 0x44, 0x44, 0x42,

    /* U+4E09 "三" */
    0x1, 0x55, 0x55, 0x55, 0x55, 0x55, 0x10, 0x3f,
    0xff, 0xff, 0xff, 0xff, 0xf3, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x5, 0xff, 0xff, 0xff, 0xff, 0x20, 0x0,
    0x26, 0x66, 0x66, 0x66, 0x60, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x22, 0x22, 0x22, 0x22, 0x22,
    0x22, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0,
    0x22, 0x22, 0x22, 0x22, 0x22, 0x22,

    /* U+4E3B "主" */
    0x0, 0x0, 0x4, 0x70, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x5, 0xfb, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x4f, 0x70, 0x0, 0x0, 0x8, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x80, 0x2, 0x33, 0x33, 0xcc,
    0x33, 0x33, 0x10, 0x0, 0x0, 0x0, 0xbb, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0xbb, 0x0, 0x0,
    0x0, 0x0, 0xdf, 0xff, 0xff, 0xff, 0xfe, 0x0,
    0x0, 0x34, 0x44, 0xcc, 0x44, 0x44, 0x0, 0x0,
    0x0, 0x0, 0xbb, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0xbb, 0x0, 0x0, 0x0, 0x2d, 0xdd, 0xdd,
    0xff, 0xdd, 0xdd, 0xd3, 0x16, 0x66, 0x66, 0x66,
    0x66, 0x66, 0x61,

    /* U+4E8C "二" */
    0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0,
    0x77, 0x77, 0x77, 0x77, 0x77, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x18, 0x88, 0x88, 0x88, 0x88, 0x88, 0x81, 0x2f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf2,

    /* U+4E94 "五" */
    0x1, 0x55, 0x55, 0x55, 0x55, 0x55, 0x10, 0x4,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x30, 0x0, 0x0,
    0x8, 0xe0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xa,
    0xc0, 0x0, 0x0, 0x0, 0x0, 0x25, 0x5d, 0xc5,
    0x55, 0x40, 0x0, 0x0, 0x8f, 0xff, 0xff, 0xff,
    0xd0, 0x0, 0x0, 0x0, 0x2f, 0x40, 0x9, 0xc0,
    0x0, 0x0, 0x0, 0x5f, 0x10, 0xb, 0xa0, 0x0,
    0x0, 0x0, 0x8e, 0x0, 0xc, 0x90, 0x0, 0x0,
    0x0, 0xbb, 0x0, 0xe, 0x70, 0x0, 0x2f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xf3, 0x5, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x51,

    /* U+56DB "四" */
    0x55, 0x55, 0x55, 0x55, 0x55, 0x52, 0xef, 0xef,
    0xfe, 0xff, 0xee, 0xf7, 0xe6, 0x5, 0xe0, 0x4e,
    0x0, 0xe7, 0xe6, 0x6, 0xd0, 0x4e, 0x0, 0xe7,
    0xe6, 0x8, 0xb0, 0x4e, 0x0, 0xe7, 0xe6, 0xd,
    0x80, 0x4e, 0x0, 0xe7, 0xe6, 0x7f, 0x30, 0x2f,
    0xcc, 0xf7, 0xeb, 0xf9, 0x0, 0x3, 0x44, 0xf7,
    0xe6, 0x40, 0x0, 0x0, 0x0, 0xe7, 0xe9, 0x66,
    0x66, 0x66, 0x66, 0xf7, 0xee, 0xee, 0xee, 0xee,
    0xee, 0xf7, 0xc5, 0x0, 0x0, 0x0, 0x0, 0xb5,

    /* U+661F "星" */
    0x0, 0xaf, 0xff, 0xff, 0xff, 0xfa, 0x0, 0x0,
    0xaa, 0x11, 0x11, 0x11, 0xba, 0x0, 0x0, 0xaf,
    0xee, 0xee, 0xee, 0xfa, 0x0, 0x0, 0xaa, 0x11,
    0x11, 0x11, 0xba, 0x0, 0x0, 0xaf, 0xff, 0xff,
    0xff, 0xfa, 0x0, 0x0, 0x3e, 0x30, 0x67, 0x0,
    0x0, 0x0, 0x1, 0xef, 0xcc, 0xee, 0xcc, 0xcc,
    0x20, 0x1e, 0xb2, 0x22, 0xbb, 0x22, 0x22, 0x0,
    0x3, 0x6e, 0xee, 0xff, 0xee, 0xe8, 0x0, 0x0,
    0x1, 0x11, 0xab, 0x11, 0x11, 0x0, 0x1, 0x11,
    0x11, 0xab, 0x11, 0x11, 0x10, 0x1f, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xf1,

    /* U+671F "期" */
    0x0, 0xe4, 0x8, 0xa0, 0x0, 0x0, 0x0, 0x0,
    0xe4, 0x8, 0xa0, 0xcf, 0xff, 0xf3, 0xf, 0xff,
    0xff, 0xfa, 0xc9, 0x33, 0xf3, 0x2, 0xe6, 0x29,
    0xb1, 0xc7, 0x1, 0xf3, 0x0, 0xee, 0xde, 0xa0,
    0xcf, 0xff, 0xf3, 0x0, 0xe5, 0x19, 0xa0, 0xc8,
    0x12, 0xf3, 0x0, 0xee, 0xde, 0xa0, 0xc7, 0x1,
    0xf3, 0x0, 0xe6, 0x3a, 0xa0, 0xcf, 0xff, 0xf3,
    0x3, 0xe6, 0x3a, 0xb1, 0xeb, 0x99, 0xf3, 0x3f,
    0xff, 0xff, 0xf9, 0xf3, 0x1, 0xf3, 0x0, 0x87,
    0x1c, 0x23, 0xf0, 0x1, 0xf3, 0x6, 0xf2, 0x9,
    0xd9, 0xc0, 0x34, 0xf2, 0xc, 0x50, 0x0, 0x4a,
    0x60, 0xaf, 0xb0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0,

    /* U+9762 "面" */
    0x2f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf3, 0x3,
    0x33, 0x33, 0xf9, 0x33, 0x33, 0x30, 0x0, 0x11,
    0x13, 0xf5, 0x11, 0x11, 0x0, 0x8, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x90, 0x8, 0xb1, 0x9a, 0x11,
    0xc7, 0x1c, 0x90, 0x8, 0xb0, 0x8e, 0xdd, 0xf6,
    0xc, 0x90, 0x8, 0xb0, 0x8b, 0x33, 0xd6, 0xc,
    0x90, 0x8, 0xb0, 0x8a, 0x11, 0xc6, 0xc, 0x90,
    0x8, 0xb0, 0x8f, 0xff, 0xf6, 0xc, 0x90, 0x8,
    0xb0, 0x8a, 0x0, 0xc6, 0xc, 0x90, 0x8, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x90, 0x8, 0xc4, 0x44,
    0x44, 0x44, 0x4d, 0x90,

    /* U+9875 "页" */
    0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x3,
    0x33, 0x34, 0xf7, 0x33, 0x33, 0x30, 0x0, 0x24,
    0x47, 0xf4, 0x44, 0x43, 0x0, 0x0, 0xaf, 0xff,
    0xff, 0xff, 0xfd, 0x0, 0x0, 0xaa, 0x0, 0x33,
    0x0, 0x8d, 0x0, 0x0, 0xaa, 0x0, 0xab, 0x0,
    0x8d, 0x0, 0x0, 0xaa, 0x0, 0xab, 0x0, 0x8d,
    0x0, 0x0, 0xaa, 0x0, 0xaa, 0x0, 0x8d, 0x0,
    0x0, 0xaa, 0x0, 0xe7, 0x20, 0x8d, 0x0, 0x0,
    0x22, 0x1c, 0xe3, 0xfb, 0x52, 0x0, 0x0, 0x49,
    0xfc, 0x20, 0x19, 0xfc, 0x30, 0xe, 0xfa, 0x40,
    0x0, 0x0, 0x2a, 0xc0, 0x1, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 50, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 80, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 8, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 48, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 83, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 203, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 243, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 283, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 363, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 403, .adv_w = 67, .box_w = 3, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 415, .adv_w = 139, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 460, .adv_w = 149, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 500, .adv_w = 145, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 545, .adv_w = 157, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 585, .adv_w = 134, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 620, .adv_w = 127, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 655, .adv_w = 157, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 700, .adv_w = 166, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 745, .adv_w = 69, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 755, .adv_w = 123, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 790, .adv_w = 149, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 835, .adv_w = 125, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 870, .adv_w = 186, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 920, .adv_w = 164, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 960, .adv_w = 169, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1010, .adv_w = 145, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1050, .adv_w = 169, .box_w = 11, .box_h = 14, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 1127, .adv_w = 147, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1167, .adv_w = 136, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1207, .adv_w = 137, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1252, .adv_w = 164, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1292, .adv_w = 133, .box_w = 10, .box_h = 10, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1342, .adv_w = 200, .box_w = 13, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1407, .adv_w = 134, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1452, .adv_w = 124, .box_w = 9, .box_h = 10, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1497, .adv_w = 136, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1537, .adv_w = 129, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1565, .adv_w = 141, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1609, .adv_w = 116, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1637, .adv_w = 141, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1681, .adv_w = 127, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1713, .adv_w = 77, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1746, .adv_w = 129, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1790, .adv_w = 139, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1829, .adv_w = 64, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1846, .adv_w = 65, .box_w = 5, .box_h = 14, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 1881, .adv_w = 129, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1920, .adv_w = 67, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1937, .adv_w = 211, .box_w = 12, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1985, .adv_w = 140, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2013, .adv_w = 138, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2045, .adv_w = 141, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2089, .adv_w = 141, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2133, .adv_w = 92, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2156, .adv_w = 108, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2184, .adv_w = 89, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2214, .adv_w = 139, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2242, .adv_w = 122, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2274, .adv_w = 185, .box_w = 12, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2322, .adv_w = 118, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2354, .adv_w = 122, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2398, .adv_w = 110, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2426, .adv_w = 224, .box_w = 14, .box_h = 3, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 2447, .adv_w = 224, .box_w = 13, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 2525, .adv_w = 224, .box_w = 14, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 2616, .adv_w = 224, .box_w = 14, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2686, .adv_w = 224, .box_w = 14, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 2770, .adv_w = 224, .box_w = 12, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 2842, .adv_w = 224, .box_w = 14, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 2926, .adv_w = 224, .box_w = 14, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 3024, .adv_w = 224, .box_w = 14, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 3108, .adv_w = 224, .box_w = 14, .box_h = 13, .ofs_x = 0, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0xd
};

static const uint16_t unicode_list_4[] = {
    0x0, 0x9, 0x3b, 0x8c, 0x94, 0x8db, 0x181f, 0x191f,
    0x4962, 0x4a75
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 14, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 2, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    },
    {
        .range_start = 48, .range_length = 11, .glyph_id_start = 3,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 65, .range_length = 26, .glyph_id_start = 14,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 97, .range_length = 26, .glyph_id_start = 40,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 19968, .range_length = 19062, .glyph_id_start = 66,
        .unicode_list = unicode_list_4, .glyph_id_ofs_list = NULL, .list_length = 10, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 2, 3, 4,
    5, 6, 7, 8, 9, 0, 0, 10,
    11, 12, 0, 0, 6, 13, 6, 14,
    15, 16, 17, 18, 19, 20, 21, 22,
    23, 24, 25, 0, 26, 27, 28, 29,
    0, 0, 30, 31, 29, 29, 24, 24,
    32, 33, 34, 35, 32, 36, 37, 38,
    39, 40, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 2, 3, 0,
    4, 0, 0, 0, 4, 0, 0, 5,
    0, 0, 0, 0, 4, 0, 4, 0,
    6, 7, 8, 9, 10, 11, 12, 13,
    14, 0, 15, 15, 15, 16, 17, 0,
    0, 18, 0, 0, 19, 19, 15, 19,
    15, 19, 20, 21, 22, 23, 24, 25,
    26, 27, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, 0, 0, 0, -5, -5, -13, 0,
    -5, -2, -8, -17, -5, 0, 0, 0,
    0, 0, 0, 0, -6, 0, -2, 0,
    -4, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -7, 0,
    0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, -2, 0, 0, -15, -4, -4, 0,
    0, -4, -2, 5, 0, -2, 0, 0,
    0, 5, -4, -1, -2, -1, -1, -2,
    0, 0, 0, 0, 0, -4, -4, -6,
    0, -2, -1, -1, -4, -1, 0, 0,
    0, 0, 0, 0, 0, -2, 0, -4,
    -2, -2, -4, 0, -7, 0, 0, -8,
    -2, -6, -4, -4, -1, -1, -1, -2,
    -2, 0, 0, 0, -5, 0, 0, 0,
    -8, -2, -4, -2, 0, -4, 0, 0,
    0, -2, 0, -11, 0, -6, 0, -2,
    -1, -5, -5, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -4,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, 0, -6, 0,
    -2, 0, -9, -2, 0, 0, 0, -10,
    -2, -34, -6, 0, 0, 0, 0, -6,
    -1, -8, -9, -4, 0, -6, 0, -5,
    -6, -2, -4, -6, -5, -7, -5, -9,
    0, 0, -1, 0, 0, 0, -5, 0,
    -4, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -10,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -9, 0,
    -2, -6, 0, -3, -5, -4, -4, -2,
    0, -5, 0, 0, -2, 0, 0, -2,
    0, 0, -10, -3, -6, -5, -5, -6,
    -4, -13, 0, 0, -7, 0, -6, -30,
    -8, -19, -14, 0, -20, 0, 0, -3,
    -4, -1, 0, 0, 0, -6, -2, -10,
    -9, 0, -10, 0, -8, 0, -13, 0,
    -37, -2, -5, 0, 0, 0, -6, -2,
    -17, -10, -6, 0, -6, 0, 0, -2,
    0, 0, 0, 0, -4, 0, -5, -8,
    0, -1, 0, -4, -3, -4, 0, 0,
    1, -1, -1, -2, -1, -2, 0, -1,
    0, 0, 0, 0, 0, 0, 0, -2,
    0, -2, 4, 0, 0, 0, -4, -4,
    -5, 0, 0, 0, 0, -4, 0, 0,
    0, 0, 0, 0, 0, 0, -6, 0,
    0, 0, 0, 0, -1, -18, -5, -10,
    -6, -32, -9, 0, 0, 0, 0, -5,
    -4, -13, -18, -17, -4, -18, 0, -11,
    -15, -4, -11, -8, -8, -9, -8, -18,
    0, 0, -4, 0, -13, -2, 0, 0,
    -1, 0, -2, -4, 0, -1, 0, 0,
    -2, 0, 0, -1, 0, 0, 0, 0,
    -2, 0, 0, -5, 0, -4, -2, -18,
    -2, 0, -2, 2, 0, 0, 0, -5,
    -6, -4, 0, -5, 0, -5, -4, 0,
    -8, -2, -2, -4, -2, -6, -2, 0,
    0, -1, -15, -1, 0, 0, 0, 0,
    0, 0, -1, -4, 0, 0, -2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, -8, 0, 0, -5, -1, -4, -5,
    -2, 0, 0, 0, 0, 0, -2, -2,
    -5, 0, 0, 0, 0, -5, -2, -5,
    -4, -2, -5, -4, -17, -7, -4, -4,
    -25, -4, -4, -2, 0, 0, 0, 0,
    -6, -18, -11, 0, -15, 0, -10, -11,
    -9, -9, -4, -6, -9, -4, -13, -8,
    0, -1, -5, -8, -8, 0, -2, -1,
    -1, 0, -4, -4, -4, -5, -5, -3,
    0, 0, 0, -4, -5, -4, -4, -6,
    -4, 0, 0, 0, 0, 0, 0, 0,
    -6, 0, -5, -1, 0, -6, 0, 0,
    0, 0, 0, 0, 0, 0, -4, 0,
    0, 0, 0, 0, 0, 1, 0, 0,
    0, 0, 0, -15, 0, -5, 0, -1,
    -12, 0, -3, 0, -1, 0, 0, 0,
    -1, -5, 0, -1, -1, -5, -1, -2,
    -6, 0, 0, 0, 0, 0, -5, 0,
    -4, 0, 0, -6, 0, -2, -6, 0,
    -2, 0, 0, 0, -2, 0, 1, 1,
    1, 1, 0, 2, 0, 0, 0, -5,
    -5, -6, 0, -5, -2, 0, -7, 0,
    -3, 0, -1, -2, 0, 0, 0, -2,
    0, 0, 0, -2, 0, 0, -4, 0,
    0, 0, 0, 0, 11, 0, 16, 11,
    8, 14, 0, -5, -2, 0, -3, -2,
    0, -1, 0, 0, 2, 0, -1, 0,
    -4, 0, 0, 0, 0, 0, 0, -7,
    0, 0, 0, 0, -4, 0, -4, -4,
    0, 0, 10, 0, 0, 0, 0, -1,
    -1, 0, 3, -4, 0, 0, 0, 0,
    0, 0, -6, 0, -2, 0, 0, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -14,
    2, 0, 0, 0, 0, -10, 0, 0,
    0, 0, -3, 0, -2, -5, 0, -2,
    -2, 0, 0, -6, -4, 0, 0, -2,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, -4, 0, 0,
    -8, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -6, 5, -4, 0, -15, 0, 0, 0,
    0, 0, 0, 0, -2, -6, -2, 0,
    -2, 0, 0, -1, 0, 0, 5, 4,
    0, 5, -1, 2, 0, 0, 0, 0,
    0, -6, 0, -2, 0, 0, -5, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    0, 0, 0, 0, 0, 0, -7, 3,
    0, 0, 0, 0, -4, 0, 0, 0,
    0, -1, 0, -5, -4, 0, -2, 0,
    0, -2, -5, 0, 0, 0, -4, 0,
    0, -2, 0, -3, 0, -10, 0, -5,
    0, -2, 0, 0, -4, -2, -5, -1,
    0, 0, -2, 0, 0, 0, 0, 0,
    0, 0, 0, -6, 0, 0, -1, 0,
    -8, 0, -6, 0, -2, 0, -4, -6,
    0, -2, -1, 0, 0, -2, 0, 0,
    0, 0, 0, 0, 0, 0, -5, -4,
    0, 0, -2, 0, -1, -8, 0, -4,
    0, -2, -9, 0, -2, -5, 0, 0,
    0, 0, 0, -6, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, -10, 0,
    -5, 0, -1, 0, -1, -2, 0, -5,
    -1, 0, 0, -2, 0, 0, 0, 0,
    0, 0, -4, 0, -6, -5, 0, 0,
    0, 0, 0, -6, 0, 0, 0, 0,
    -6, 0, -5, -2, 0, -1, 0, 0,
    0, 0, -2, -1, 0, 0, -1, 0
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 40,
    .right_class_cnt     = 27,
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 5,
    .bpp = 4,
    .kern_classes = 1,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};

extern const lv_font_t noto_sans_medium_14;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t noto_sans_medium_14 = {
#else
lv_font_t noto_sans_medium_14 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 16,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &noto_sans_medium_14,
#endif
    .user_data = NULL,
};



#endif /*#if NOTO_SANS_MEDIUM_14*/

