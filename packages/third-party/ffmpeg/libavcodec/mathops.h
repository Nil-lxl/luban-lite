/*
 * simple math operations
 * Copyright (c) 2001, 2002 Fabrice Bellard
 * Copyright (c) 2006 Michael Niedermayer <michaelni@gmx.at> et al
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
#ifndef AVCODEC_MATHOPS_H
#define AVCODEC_MATHOPS_H

#include <stdint.h>

#include "libavutil/common.h"
#include "libavutil/reverse.h"
#include "mathops.h"

#define MAX_NEG_CROP 1024

extern const uint32_t ff_inverse[257];

/* generic implementation */

#ifndef MUL64
#   define MUL64(a,b) ((int64_t)(a) * (int64_t)(b))
#endif

#ifndef MULL
#   define MULL(a,b,s) (MUL64(a, b) >> (s))
#endif

#ifndef MULH
static inline int MULH(int a, int b){
    return MUL64(a, b) >> 32;
}
#endif

#ifndef UMULH
static inline unsigned UMULH(unsigned a, unsigned b){
    return ((uint64_t)(a) * (uint64_t)(b))>>32;
}
#endif

#ifndef MAC64
#   define MAC64(d, a, b) ((d) += MUL64(a, b))
#endif

#ifndef MLS64
#   define MLS64(d, a, b) ((d) -= MUL64(a, b))
#endif

/* signed 16x16 -> 32 multiply add accumulate */
#ifndef MAC16
#   define MAC16(rt, ra, rb) rt += (ra) * (rb)
#endif

/* signed 16x16 -> 32 multiply */
#ifndef MUL16
#   define MUL16(ra, rb) ((ra) * (rb))
#endif

#ifndef MLS16
#   define MLS16(rt, ra, rb) ((rt) -= (ra) * (rb))
#endif

/* median of 3 */
#ifndef mid_pred
#define mid_pred mid_pred
static inline const int mid_pred(int a, int b, int c)
{
    if(a>b){
        if(c>b){
            if(c>a) b=a;
            else    b=c;
        }
    }else{
        if(b>c){
            if(c>a) b=c;
            else    b=a;
        }
    }
    return b;
}
#endif

#ifndef median4
#define median4 median4
static inline const int median4(int a, int b, int c, int d)
{
    if (a < b) {
        if (c < d) return (FFMIN(b, d) + FFMAX(a, c)) / 2;
        else       return (FFMIN(b, c) + FFMAX(a, d)) / 2;
    } else {
        if (c < d) return (FFMIN(a, d) + FFMAX(b, c)) / 2;
        else       return (FFMIN(a, c) + FFMAX(b, d)) / 2;
    }
}
#endif

#ifndef sign_extend
static inline const int sign_extend(int val, unsigned bits)
{
    unsigned shift = 8 * sizeof(int) - bits;
    union { unsigned u; int s; } v = { (unsigned) val << shift };
    return v.s >> shift;
}
#endif

#ifndef zero_extend
static inline const unsigned zero_extend(unsigned val, unsigned bits)
{
    return (val << ((8 * sizeof(int)) - bits)) >> ((8 * sizeof(int)) - bits);
}
#endif

#ifndef COPY3_IF_LT
#define COPY3_IF_LT(x, y, a, b, c, d)\
if ((y) < (x)) {\
    (x) = (y);\
    (a) = (b);\
    (c) = (d);\
}
#endif

#ifndef MASK_ABS
#define MASK_ABS(mask, level) do {              \
        mask  = level >> 31;                    \
        level = (level ^ mask) - mask;          \
    } while (0)
#endif

#ifndef NEG_SSR32
#   define NEG_SSR32(a,s) ((( int32_t)(a))>>(32-(s)))
#endif

#ifndef NEG_USR32
#   define NEG_USR32(a,s) (((uint32_t)(a))>>(32-(s)))
#endif

#if HAVE_BIGENDIAN
# ifndef PACK_2U8
#   define PACK_2U8(a,b)     (((a) <<  8) | (b))
# endif
# ifndef PACK_4U8
#   define PACK_4U8(a,b,c,d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))
# endif
# ifndef PACK_2U16
#   define PACK_2U16(a,b)    (((a) << 16) | (b))
# endif
#else
# ifndef PACK_2U8
#   define PACK_2U8(a,b)     (((b) <<  8) | (a))
# endif
# ifndef PACK_4U2
#   define PACK_4U8(a,b,c,d) (((d) << 24) | ((c) << 16) | ((b) << 8) | (a))
# endif
# ifndef PACK_2U16
#   define PACK_2U16(a,b)    (((b) << 16) | (a))
# endif
#endif

#ifndef PACK_2S8
#   define PACK_2S8(a,b)     PACK_2U8((a)&255, (b)&255)
#endif
#ifndef PACK_4S8
#   define PACK_4S8(a,b,c,d) PACK_4U8((a)&255, (b)&255, (c)&255, (d)&255)
#endif
#ifndef PACK_2S16
#   define PACK_2S16(a,b)    PACK_2U16((a)&0xffff, (b)&0xffff)
#endif

#ifndef FASTDIV
#   define FASTDIV(a,b) ((uint32_t)((((uint64_t)a) * ff_inverse[b]) >> 32))
#endif /* FASTDIV */


static inline const float ff_sqrf(float a)
{
    return a*a;
}

static inline int8_t ff_u8_to_s8(uint8_t a)
{
    union {
        uint8_t u8;
        int8_t  s8;
    } b;
    b.u8 = a;
    return b.s8;
}

static inline uint32_t bitswap_32(uint32_t x)
{
    return (uint32_t)ff_reverse[ x        & 0xFF] << 24 |
           (uint32_t)ff_reverse[(x >> 8)  & 0xFF] << 16 |
           (uint32_t)ff_reverse[(x >> 16) & 0xFF] << 8  |
           (uint32_t)ff_reverse[ x >> 24];
}

#endif /* AVCODEC_MATHOPS_H */
