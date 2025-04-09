/*
 * WMA compatible decoder
 * Copyright (c) 2002 The FFmpeg Project
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
 * WMA compatible decoder.
 * This decoder handles Microsoft Windows Media Audio data, versions 1 & 2.
 * WMA v1 is identified by audio format 0x160 in Microsoft media files
 * (ASF/AVI/WAV). WMA v2 is identified by audio format 0x161.
 *
 * To use this decoder, a calling application must supply the extra data
 * bytes provided with the WMA data. These are the extra, codec-specific
 * bytes at the end of a WAVEFORMATEX data structure. Transmit these bytes
 * to the decoder using the extradata[_size] fields in AVCodecContext. There
 * should be 4 extra bytes for v1 data and 6 extra bytes for v2 data.
 */

#include "avcodec.h"
#include "common.h"
#include "dsputil.h"
#include "get_bits.h"
#include "intmath.h"
#include "log.h"

/* size of blocks */
#define BLOCK_MIN_BITS 7
#define BLOCK_MAX_BITS 11
#define BLOCK_MAX_SIZE (1 << BLOCK_MAX_BITS)

#define BLOCK_NB_SIZES (BLOCK_MAX_BITS - BLOCK_MIN_BITS + 1)

/* XXX: find exact max size */
#define HIGH_BAND_MAX_SIZE 16

#define NB_LSP_COEFS 10

/* XXX: is it a suitable value ? */
#define MAX_CODED_SUPERFRAME_SIZE 16384

#define MAX_CHANNELS 2

#define NOISE_TAB_SIZE 8192

#define LSP_POW_BITS 7

#define VLCBITS 9
#define VLCMAX ((22 + VLCBITS - 1) / VLCBITS)

#define EXPVLCBITS 8
#define EXPMAX ((19 + EXPVLCBITS - 1) / EXPVLCBITS)

#define HGAINVLCBITS 9
#define HGAINMAX ((13 + HGAINVLCBITS - 1) / HGAINVLCBITS)

int fixed32_sqrt(int x)
{
    int m, y, b;
    int t;
    m = 0x40000000;
    y = 0;
    while (m != 0) {
        b = y | m;
        y = y >> 1;

        t = (int)((x | ~(x - b)) >> 31);
        x = x - (b & t);
        y = y | (m & t);

        m = (m >> 2);
    }
    return y << 8;
}

typedef struct WMADecodeContext {
    GetBitContext gb;

    int sample_rate;

    int nb_channels;

    int bit_rate;

    int version; /* 1 = 0x160 (WMAV1), 2 = 0x161 (WMAV2) */

    int block_align;

    int use_bit_reservoir;

    int use_variable_block_len;

    int use_exp_vlc; /* exponent coding: 0 = lsp, 1 = vlc + delta */

    int use_noise_coding; /* true if perceptual noise is added */

    int byte_offset_bits;

    VLC exp_vlc;

    int exponent_sizes[BLOCK_NB_SIZES];

    uint16_t exponent_bands[BLOCK_NB_SIZES][25];

    int high_band_start[BLOCK_NB_SIZES]; /* index of first coef in high band */

    int coefs_start; /* first coded coef */

    int coefs_end[BLOCK_NB_SIZES]; /* max number of coded coefficients */

    int exponent_high_sizes[BLOCK_NB_SIZES];

    int exponent_high_bands[BLOCK_NB_SIZES][HIGH_BAND_MAX_SIZE];

    VLC hgain_vlc;

    int high_band_coded[MAX_CHANNELS][HIGH_BAND_MAX_SIZE];

    int high_band_values[MAX_CHANNELS][HIGH_BAND_MAX_SIZE];

    VLC coef_vlc[2];

    uint16_t *run_table[2];

    uint16_t *level_table[2];

    int frame_len; /* frame length in samples */

    int frame_len_bits; /* frame_len = 1 << frame_len_bits */

    int nb_block_sizes; /* number of block sizes */

    int reset_block_lengths;

    int block_len_bits; /* log2 of current block length */

    int next_block_len_bits; /* log2 of next block length */

    int prev_block_len_bits; /* log2 of prev block length */

    int block_len; /* block length in samples */

    int block_num; /* block number in current frame */

    int block_pos; /* current position in frame */

    uint8_t ms_stereo; /* true if mid/side stereo mode */

    uint8_t channel_coded[MAX_CHANNELS]; /* true if channel is coded */

    DECLARE_ALIGNED_16(int, exponents[MAX_CHANNELS][BLOCK_MAX_SIZE]);

    int max_exponent[MAX_CHANNELS];

    int16_t coefs1[MAX_CHANNELS][BLOCK_MAX_SIZE];

    DECLARE_ALIGNED_16(int, coefs[MAX_CHANNELS][BLOCK_MAX_SIZE]);

    MDCTContext mdct_ctx[BLOCK_NB_SIZES];

    int *windows[BLOCK_NB_SIZES];
    int window[BLOCK_MAX_SIZE * 2];

    DECLARE_ALIGNED_16(long long, mdct_tmp[BLOCK_MAX_SIZE]); /* temporary storage for imdct */

    DECLARE_ALIGNED_16(int, frame_out[MAX_CHANNELS][BLOCK_MAX_SIZE * 2]);

    uint8_t last_superframe[MAX_CODED_SUPERFRAME_SIZE + 4]; /* padding added */

    int last_bitoffset;

    int last_superframe_len;

    int noise_table[NOISE_TAB_SIZE];

    int noise_index;

    int noise_mult; /* XXX: suppress that and integrate it in the noise array */

    int lsp_cos_table[BLOCK_MAX_SIZE];

    DECLARE_ALIGNED_16(long long, output[BLOCK_MAX_SIZE * 2]);
    DECLARE_ALIGNED_16(int, output1[BLOCK_MAX_SIZE * 2]);
#ifdef TRACE
    int frame_count;
#endif

} WMADecodeContext;

static void wma_lsp_to_curve_init(WMADecodeContext *s, int frame_len);

#include "wmadata.h"

#ifdef TRACE
static void dump_shorts(const char *name, const short *tab, int n)
{
    int i;

    printf("%s[%d]:\n", name, n);
    for (i = 0; i < n; i++) {
        if ((i & 7) == 0)
            printf("%4d: ", i);
        printf(" %5d.0", tab[i]);
        if ((i & 7) == 7)
            printf("\n");
    }
}

static void dump_floats(const char *name, int prec, const float *tab, int n)
{
    int i;

    printf("%s[%d]:\n", name, n);
    for (i = 0; i < n; i++) {
        if ((i & 7) == 0)
            printf("%4d: ", i);
        printf(" %8.*f", prec, tab[i]);
        if ((i & 7) == 7)
            printf("\n");
    }
    if ((i & 7) != 0)
        printf("\n");
}
#endif

/* XXX: use same run/length optimization as mpeg decoders */
static void init_coef_vlc(VLC *vlc,
                          uint16_t **prun_table, uint16_t **plevel_table,
                          const CoefVLCTable *vlc_table)
{
    int n = vlc_table->n;
    const uint8_t *table_bits = vlc_table->huffbits;
    const uint32_t *table_codes = vlc_table->huffcodes;
    const uint16_t *levels_table = vlc_table->levels;
    uint16_t *run_table, *level_table;
    const uint16_t *p;
    int i, l, j, level;

    init_vlc(vlc, VLCBITS, n, table_bits, 1, 1, table_codes, 4, 4, 0);

    run_table = av_malloc(n * sizeof(uint16_t));
    level_table = av_malloc(n * sizeof(uint16_t));
    p = levels_table;
    i = 2;
    level = 1;
    while (i < n) {
        l = *p++;
        for (j = 0; j < l; j++) {
            run_table[i] = j;
            level_table[i] = level;
            i++;
        }
        level++;
    }
    *prun_table = run_table;
    *plevel_table = level_table;
}

int g_pow1div16[256];
long long g_pow1div20[256];

int g_pow1div20copy[256];
int g_lsp_codebook[NB_LSP_COEFS][16];

static int wma_decode_init(AVCodecContext *avctx)
{
    WMADecodeContext *s = avctx->priv_data;
    int i, flags1, flags2;
    int *window_copy;
    uint8_t *extradata;
    float bps1, high_freq;
    volatile float bps;
    int sample_rate1;
    int coef_vlc_table;

    int index;

    // HDRB
    for (index = 0; index < 256; index++) {
        float fTemp = pow(10, index * (1.0 / 16.0));
        g_pow1div16[index] = float2fp(fTemp);
    }
    for (index = 0; index < 256; index++) {
        int j;
        long long multcopy2;

        g_pow1div20[index] = 73531;
        multcopy2 = 73531 >> 8;

        for (j = 1; j < index; j++) {
            g_pow1div20[index] = g_pow1div20[index] >> 8;
            g_pow1div20[index] *= multcopy2;
        }
        g_pow1div20[index] = (g_pow1div20[index] << 16);
    }

    for (index = 0; index < 256; index++) {
        float fTemp = pow(10, index * 0.05);
        g_pow1div20copy[index] = float2fp(fTemp);
        g_pow1div20copy[index] = g_pow1div20copy[index] >> 8;
    }
    for (index = 0; index < NB_LSP_COEFS; index++) {
        int j;
        for (j = 0; j < 16; j++)
            g_lsp_codebook[index][j] = float2fp(lsp_codebook[index][j]);
    }

    /* HDRB */
    s->sample_rate = avctx->sample_rate;
    s->nb_channels = avctx->channels;
    s->bit_rate = avctx->bit_rate;
    s->block_align = avctx->block_align;

    if (avctx->codec->id == AV_CODEC_ID_WMAV1) {
        s->version = 1;
    } else {
        s->version = 2;
    }

    /* extract flag infos */
    flags1 = 0;
    flags2 = 0;
    extradata = avctx->extradata;
    if (s->version == 1 && avctx->extradata_size >= 4) {
        flags1 = extradata[0] | (extradata[1] << 8);
        flags2 = extradata[2] | (extradata[3] << 8);
    } else if (s->version == 2 && avctx->extradata_size >= 6) {
        flags1 = extradata[0] | (extradata[1] << 8) |
                 (extradata[2] << 16) | (extradata[3] << 24);
        flags2 = extradata[4] | (extradata[5] << 8);
    }
    s->use_exp_vlc = flags2 & 0x0001;
    s->use_bit_reservoir = flags2 & 0x0002;
    s->use_variable_block_len = flags2 & 0x0004;

    /* compute MDCT block size */
    if (s->sample_rate <= 16000) {
        s->frame_len_bits = 9;
    } else if (s->sample_rate <= 22050 ||
               (s->sample_rate <= 32000 && s->version == 1)) {
        s->frame_len_bits = 10;
    } else {
        s->frame_len_bits = 11;
    }
    s->frame_len = 1 << s->frame_len_bits;
    if (s->use_variable_block_len) {
        int nb_max, nb;
        nb = ((flags2 >> 3) & 3) + 1;
        if ((s->bit_rate / s->nb_channels) >= 32000)
            nb += 2;
        nb_max = s->frame_len_bits - BLOCK_MIN_BITS;
        if (nb > nb_max)
            nb = nb_max;
        s->nb_block_sizes = nb + 1;
    } else {
        s->nb_block_sizes = 1;
    }

    /* init rate dependant parameters */
    s->use_noise_coding = 1;
    high_freq = s->sample_rate * 0.5;

    /* if version 2, then the rates are normalized */
    sample_rate1 = s->sample_rate;
    if (s->version == 2) {
        if (sample_rate1 >= 44100)
            sample_rate1 = 44100;
        else if (sample_rate1 >= 22050)
            sample_rate1 = 22050;
        else if (sample_rate1 >= 16000)
            sample_rate1 = 16000;
        else if (sample_rate1 >= 11025)
            sample_rate1 = 11025;
        else if (sample_rate1 >= 8000)
            sample_rate1 = 8000;
    }

    bps = (float)s->bit_rate / (float)(s->nb_channels * s->sample_rate);
    s->byte_offset_bits = av_log2((int)(bps * s->frame_len / 8.0 + 0.5)) + 2;

    /* compute high frequency value and choose if noise coding should
       be activated */
    bps1 = bps;
    if (s->nb_channels == 2)
        bps1 = bps * 1.6;
    if (sample_rate1 == 44100) {
        if (bps1 >= 0.61)
            s->use_noise_coding = 0;
        else
            high_freq = high_freq * 0.4;
    } else if (sample_rate1 == 22050) {
        if (bps1 >= 1.16)
            s->use_noise_coding = 0;
        else if (bps1 >= 0.72)
            high_freq = high_freq * 0.7;
        else
            high_freq = high_freq * 0.6;
    } else if (sample_rate1 == 16000) {
        if (bps > 0.5)
            high_freq = high_freq * 0.5;
        else
            high_freq = high_freq * 0.3;
    } else if (sample_rate1 == 11025) {
        high_freq = high_freq * 0.7;
    } else if (sample_rate1 == 8000) {
        if (bps <= 0.625) {
            high_freq = high_freq * 0.5;
        } else if (bps > 0.75) {
            s->use_noise_coding = 0;
        } else {
            high_freq = high_freq * 0.65;
        }
    } else {
        if (bps >= 0.8) {
            high_freq = high_freq * 0.75;
        } else if (bps >= 0.6) {
            high_freq = high_freq * 0.6;
        } else {
            high_freq = high_freq * 0.5;
        }
    }
    av_log(NULL, AV_LOG_INFO, "flags1=0x%x flags2=0x%x\n", flags1, flags2);
    av_log(NULL, AV_LOG_INFO, "version=%d channels=%d sample_rate=%d bitrate=%d block_align=%d\n",
           s->version, s->nb_channels, s->sample_rate, s->bit_rate,
           s->block_align);
    av_log(NULL, AV_LOG_INFO, "bps=%f bps1=%f high_freq=%f bitoffset=%d\n",
           bps, bps1, high_freq, s->byte_offset_bits);
    av_log(NULL, AV_LOG_INFO, "use_noise_coding=%d use_exp_vlc=%d nb_block_sizes=%d "
           "use_bit_reservoir=%d\n",
           s->use_noise_coding, s->use_exp_vlc, s->nb_block_sizes, s->use_bit_reservoir);

    /* compute the scale factor band sizes for each MDCT block size */
    {
        int a, b, pos, lpos, k, block_len, i, j, n;
        const uint8_t *table;

        if (s->version == 1) {
            s->coefs_start = 3;
        } else {
            s->coefs_start = 0;
        }
        for (k = 0; k < s->nb_block_sizes; k++) {
            block_len = s->frame_len >> k;

            if (s->version == 1) {
                lpos = 0;
                for (i = 0; i < 25; i++) {
                    a = wma_critical_freqs[i];
                    b = s->sample_rate;
                    pos = ((block_len * 2 * a) + (b >> 1)) / b;
                    if (pos > block_len)
                        pos = block_len;
                    s->exponent_bands[0][i] = pos - lpos;
                    if (pos >= block_len) {
                        i++;
                        break;
                    }
                    lpos = pos;
                }
                s->exponent_sizes[0] = i;
            } else {
                /* hardcoded tables */
                table = NULL;
                a = s->frame_len_bits - BLOCK_MIN_BITS - k;
                if (a < 3) {
                    if (s->sample_rate >= 44100)
                        table = exponent_band_44100[a];
                    else if (s->sample_rate >= 32000)
                        table = exponent_band_32000[a];
                    else if (s->sample_rate >= 22050)
                        table = exponent_band_22050[a];
                }
                if (table) {
                    n = *table++;
                    for (i = 0; i < n; i++)
                        s->exponent_bands[k][i] = table[i];
                    s->exponent_sizes[k] = n;
                } else {
                    j = 0;
                    lpos = 0;
                    for (i = 0; i < 25; i++) {
                        a = wma_critical_freqs[i];
                        b = s->sample_rate;
                        pos = ((block_len * 2 * a) + (b << 1)) / (4 * b);
                        pos <<= 2;
                        if (pos > block_len)
                            pos = block_len;
                        if (pos > lpos)
                            s->exponent_bands[k][j++] = pos - lpos;
                        if (pos >= block_len)
                            break;
                        lpos = pos;
                    }
                    s->exponent_sizes[k] = j;
                }
            }

            /* max number of coefs */
            s->coefs_end[k] = (s->frame_len - ((s->frame_len * 9) / 100)) >> k;
            /* high freq computation */
            s->high_band_start[k] = (int)((block_len * 2 * high_freq) /
                                              s->sample_rate +
                                          0.5);
            n = s->exponent_sizes[k];
            j = 0;
            pos = 0;
            for (i = 0; i < n; i++) {
                int start, end;
                start = pos;
                pos += s->exponent_bands[k][i];
                end = pos;
                if (start < s->high_band_start[k])
                    start = s->high_band_start[k];
                if (end > s->coefs_end[k])
                    end = s->coefs_end[k];
                if (end > start) {
                    s->exponent_high_bands[k][j++] = end - start;
                }
            }
            s->exponent_high_sizes[k] = j;
#if 0
            av_log(NULL,  AV_LOG_INFO, "%5d: coefs_end=%d high_band_start=%d nb_high_bands=%d: ",
                  s->frame_len >> k,
                  s->coefs_end[k],
                  s->high_band_start[k],
                  s->exponent_high_sizes[k]);
            for(j=0;j<s->exponent_high_sizes[k];j++)
                printf(" %d", s->exponent_high_bands[k][j]);
            printf("\n");
#endif
        }
    }

#ifdef TRACE
    {
        int i, j;
        for (i = 0; i < s->nb_block_sizes; i++) {
            printf("%5d: n=%2d:",
                   s->frame_len >> i,
                   s->exponent_sizes[i]);
            for (j = 0; j < s->exponent_sizes[i]; j++)
                printf(" %d", s->exponent_bands[i][j]);
            printf("\n");
        }
    }
#endif

    /* init MDCT */
    for (i = 0; i < s->nb_block_sizes; i++)
        ff_mdct_init(&s->mdct_ctx[i], s->frame_len_bits - i + 1, 1);

    /* init MDCT windows : simple sinus window */
    for (i = 0; i < s->nb_block_sizes; i++) {
        int n, j;
        float alpha;
        n = 1 << (s->frame_len_bits - i);
        window_copy = av_malloc(sizeof(int) * n);
        alpha = M_PI / (2.0 * n);
        for (j = 0; j < n; j++) {
            float tmp = sin((j + 0.5) * alpha);
            window_copy[n - j - 1] = float2fp(tmp);
        }
        s->windows[i] = (window_copy);
    }

    s->reset_block_lengths = 1;

    if (s->use_noise_coding) {

        /* init the noise generator */
        if (s->use_exp_vlc)
            s->noise_mult = float2fp(0.02);
        else
            s->noise_mult = float2fp(0.04);

#ifdef TRACE
        for (i = 0; i < NOISE_TAB_SIZE; i++) {
            // float temp = 1.0 * s->noise_mult;
            s->noise_table[i] = s->noise_mult
        }
#else
        {
            unsigned int seed;
            float norm;
            seed = 1;
            norm = (1.0 / (float)(1LL << 31)) * sqrt(3) * fp2float(s->noise_mult);
            for (i = 0; i < NOISE_TAB_SIZE; i++) {
                float fTemp;
                seed = seed * 314159 + 1;
                fTemp = (float)((int)seed) * norm;
                s->noise_table[i] = float2fp(fTemp);
            }
        }
#endif
        init_vlc(&s->hgain_vlc, HGAINVLCBITS, sizeof(hgain_huffbits),
                 hgain_huffbits, 1, 1,
                 hgain_huffcodes, 2, 2, 0);
    }

    if (s->use_exp_vlc) {
        init_vlc(&s->exp_vlc, EXPVLCBITS, sizeof(scale_huffbits),
                 scale_huffbits, 1, 1,
                 scale_huffcodes, 4, 4, 0);
    } else {
        wma_lsp_to_curve_init(s, s->frame_len);
    }

    /* choose the VLC tables for the coefficients */
    coef_vlc_table = 2;
    if (s->sample_rate >= 32000) {
        if (bps1 < 0.72)
            coef_vlc_table = 0;
        else if (bps1 < 1.16)
            coef_vlc_table = 1;
    }

    init_coef_vlc(&s->coef_vlc[0], &s->run_table[0], &s->level_table[0],
                  &coef_vlcs[coef_vlc_table * 2]);
    init_coef_vlc(&s->coef_vlc[1], &s->run_table[1], &s->level_table[1],
                  &coef_vlcs[coef_vlc_table * 2 + 1]);
    return 0;
}

/* interpolate values for a bigger or smaller block. The block must
   have multiple sizes */
static void interpolate_array(int *scale, int old_size, int new_size)
{
    int i, j, jincr, k;
    int v;
    if (new_size > old_size) {
        jincr = new_size / old_size;
        j = new_size;
        for (i = old_size - 1; i >= 0; i--) {
            v = scale[i];
            k = jincr;
            do {
                scale[--j] = v;
            } while (--k);
        }
    } else if (new_size < old_size) {
        j = 0;
        jincr = old_size / new_size;
        for (i = 0; i < new_size; i++) {
            scale[i] = scale[j];
            j += jincr;
        }
    }
}

static void wma_lsp_to_curve_init(WMADecodeContext *s, int frame_len)
{
    float wdel;
    int i;

    wdel = M_PI / frame_len;
    for (i = 0; i < frame_len; i++) {
        float float_temp;
        float_temp = 2.0f * cos(wdel * i);
        s->lsp_cos_table[i] = float2fp(float_temp);
    }
}

/* NOTE: We use the same code as Vorbis here */
/* XXX: optimize it further with SSE/3Dnow */
static void wma_lsp_to_curve(WMADecodeContext *s,
                             int *out, int *val_max_ptr,
                             int n, int *lsp)
{
    int i, j;
    int p_copy, q_copy, w_copy, v_copy, val_max_copy;
    long long myTempQ, myTempP;
    val_max_copy = 0;

    for (i = 0; i < n; i++) {
        int temp16, tmp16;
        p_copy = 32768;
        q_copy = p_copy;

        w_copy = s->lsp_cos_table[i];

        myTempQ = q_copy;
        myTempP = p_copy;

        for (j = 1; j < NB_LSP_COEFS; j += 2) {
            temp16 = w_copy - (lsp[j - 1]);
            tmp16 = w_copy - (lsp[j]);

            myTempQ = myTempQ * (long)(long)temp16;
            myTempQ = (myTempQ >> 16);

            myTempP = myTempP * (long)(long)tmp16;
            myTempP = (myTempP >> 16);
        }
        p_copy = myTempP;
        q_copy = myTempQ;

        myTempP = 131072 - w_copy;
        myTempP = p_copy * myTempP;
        myTempP = myTempP >> 16;
        myTempP = (myTempP) * (long)(long)(p_copy);
        myTempP = myTempP >> 16;
        p_copy = myTempP;

        myTempQ = (131072 + w_copy);
        myTempQ = (long long)q_copy * myTempQ;
        myTempQ = myTempQ >> 16;
        myTempQ = myTempQ * (long)(long)(q_copy);
        myTempQ = myTempQ >> 16;
        q_copy = myTempQ;

        v_copy = p_copy + q_copy;

        if (v_copy) {
            v_copy = 0x80000000 / v_copy;
            v_copy = v_copy << 1;
            v_copy = fixed32_sqrt(v_copy);
            v_copy = fixed32_sqrt(v_copy);
        }

        if (v_copy > val_max_copy)
            val_max_copy = v_copy;

        out[i] = v_copy;
    }
    *val_max_ptr = (val_max_copy);
}

/* decode exponents coded with LSP coefficients (same idea as Vorbis) */
static void decode_exp_lsp(WMADecodeContext *s, int ch)
{
    int lsp_coefs[NB_LSP_COEFS];
    int val, i;

    for (i = 0; i < NB_LSP_COEFS; i++) {
        if (i == 0 || i >= 8)
            val = get_bits(&s->gb, 3);
        else
            val = get_bits(&s->gb, 4);

        lsp_coefs[i] = (g_lsp_codebook[i][val]);
    }

    wma_lsp_to_curve(s, s->exponents[ch], &s->max_exponent[ch],
                     s->block_len, lsp_coefs);
}

/* decode exponents coded with VLC codes */
static int decode_exp_vlc(WMADecodeContext *s, int ch)
{
    int last_exp, n, code;
    const uint16_t *ptr, *band_ptr;

    int v_copy, *q_copy, max_scale_copy, *q_end_copy;

    band_ptr = s->exponent_bands[s->frame_len_bits - s->block_len_bits];
    ptr = band_ptr;
    q_copy = s->exponents[ch];
    q_end_copy = q_copy + s->block_len;

    max_scale_copy = 0;
    if (s->version == 1) {
        last_exp = get_bits(&s->gb, 5) + 10;

        v_copy = g_pow1div16[last_exp];

        max_scale_copy = v_copy;

        n = *ptr++;
        do {
            *q_copy++ = v_copy;
        } while (--n);
    }

    last_exp = 36;

    while (q_copy < q_end_copy) {
        code = get_vlc2(&s->gb, s->exp_vlc.table, EXPVLCBITS, EXPMAX);
        if (code < 0)
            return -1;

        last_exp += code - 60;

        v_copy = g_pow1div16[last_exp];

        if (v_copy > max_scale_copy)
            max_scale_copy = v_copy;

        n = *ptr++;
        do {
            *q_copy++ = v_copy;
        } while (--n);
    }
    s->max_exponent[ch] = max_scale_copy;

    return 0;
}

/* return 0 if OK. return 1 if last block of frame. return -1 if
   unrecorrable error. */
static int wma_decode_block(WMADecodeContext *s)
{
    int n, v, a, ch, code, bsize;
    int coef_nb_bits, total_gain, parse_exponents;
    int *window = s->window;
// XXX: FIXME!! there's a bug somewhere which makes this mandatory under altivec
#ifdef HAVE_ALTIVEC
    volatile int nb_coefs[MAX_CHANNELS] __attribute__((aligned(16)));
#else
    int nb_coefs[MAX_CHANNELS];
#endif
    int mdct_norm_copy;
#ifdef TRACE
    av_log(NULL, AV_LOG_INFO, "***decode_block: %d:%d\n", s->frame_count - 1, s->block_num);
#endif

    /* compute current block length */
    if (s->use_variable_block_len) {
        n = av_log2(s->nb_block_sizes - 1) + 1;

        if (s->reset_block_lengths) {
            s->reset_block_lengths = 0;
            v = get_bits(&s->gb, n);
            if (v >= s->nb_block_sizes)
                return -1;
            s->prev_block_len_bits = s->frame_len_bits - v;
            v = get_bits(&s->gb, n);
            if (v >= s->nb_block_sizes)
                return -1;
            s->block_len_bits = s->frame_len_bits - v;
        } else {
            /* update block lengths */
            s->prev_block_len_bits = s->block_len_bits;
            s->block_len_bits = s->next_block_len_bits;
        }
        v = get_bits(&s->gb, n);
        if (v >= s->nb_block_sizes)
            return -1;
        s->next_block_len_bits = s->frame_len_bits - v;
    } else {
        /* fixed block len */
        s->next_block_len_bits = s->frame_len_bits;
        s->prev_block_len_bits = s->frame_len_bits;
        s->block_len_bits = s->frame_len_bits;
    }

    /* now check if the block length is coherent with the frame length */
    s->block_len = 1 << s->block_len_bits;
    if ((s->block_pos + s->block_len) > s->frame_len)
        return -1;

    if (s->nb_channels == 2) {
        s->ms_stereo = get_bits(&s->gb, 1);
    }
    v = 0;
    for (ch = 0; ch < s->nb_channels; ch++) {
        a = get_bits(&s->gb, 1);
        s->channel_coded[ch] = a;
        v |= a;
    }
    /* if no channel coded, no need to go further */
    /* XXX: fix potential framing problems */
    if (!v)
        goto next;

    bsize = s->frame_len_bits - s->block_len_bits;

    /* read total gain and extract corresponding number of bits for
       coef escape coding */
    total_gain = 1;
    for (;;) {
        a = get_bits(&s->gb, 7);
        total_gain += a;
        if (a != 127)
            break;
    }

    if (total_gain < 15)
        coef_nb_bits = 13;
    else if (total_gain < 32)
        coef_nb_bits = 12;
    else if (total_gain < 40)
        coef_nb_bits = 11;
    else if (total_gain < 45)
        coef_nb_bits = 10;
    else
        coef_nb_bits = 9;

    /* compute number of coefficients */
    n = s->coefs_end[bsize] - s->coefs_start;
    for (ch = 0; ch < s->nb_channels; ch++)
        nb_coefs[ch] = n;

    /* complex coding */
    if (s->use_noise_coding) {
        for (ch = 0; ch < s->nb_channels; ch++) {
            if (s->channel_coded[ch]) {
                int i, n, a;
                n = s->exponent_high_sizes[bsize];
                for (i = 0; i < n; i++) {
                    a = get_bits(&s->gb, 1);
                    s->high_band_coded[ch][i] = a;
                    /* if noise coding, the coefficients are not transmitted */
                    if (a)
                        nb_coefs[ch] -= s->exponent_high_bands[bsize][i];
                }
            }
        }
        for (ch = 0; ch < s->nb_channels; ch++) {
            if (s->channel_coded[ch]) {
                int i, n, val, code;

                n = s->exponent_high_sizes[bsize];
                val = (int)0x80000000;
                for (i = 0; i < n; i++) {
                    if (s->high_band_coded[ch][i]) {
                        if (val == (int)0x80000000) {
                            val = get_bits(&s->gb, 7) - 19;
                        } else {
                            code = get_vlc2(&s->gb, s->hgain_vlc.table, HGAINVLCBITS, HGAINMAX);
                            if (code < 0)
                                return -1;
                            val += code - 18;
                        }
                        s->high_band_values[ch][i] = val;
                    }
                }
            }
        }
    }

    /* exposant can be interpolated in short blocks. */
    parse_exponents = 1;
    if (s->block_len_bits != s->frame_len_bits) {
        parse_exponents = get_bits(&s->gb, 1);
    }

    if (parse_exponents) {
        for (ch = 0; ch < s->nb_channels; ch++) {
            if (s->channel_coded[ch]) {
                if (s->use_exp_vlc) {
                    if (decode_exp_vlc(s, ch) < 0) // HD1-HD5.wma
                        return -1;
                } else { // HD7.wma HD6.wma
                    decode_exp_lsp(s, ch);
                }
            }
        }
    } else {
        for (ch = 0; ch < s->nb_channels; ch++) {
            if (s->channel_coded[ch]) {
                interpolate_array(s->exponents[ch], 1 << s->prev_block_len_bits,
                                  s->block_len);
            }
        }
    }

    /* parse spectral coefficients : just RLE encoding */
    for (ch = 0; ch < s->nb_channels; ch++) {
        if (s->channel_coded[ch]) {
            VLC *coef_vlc;
            int level, run, sign, tindex;
            int16_t *ptr, *eptr;
            const int16_t *level_table, *run_table;

            /* special VLC tables are used for ms stereo because
               there is potentially less energy there */
            tindex = (ch == 1 && s->ms_stereo);
            coef_vlc = &s->coef_vlc[tindex];
            run_table = (const int16_t *)s->run_table[tindex];
            level_table = (const int16_t *)s->level_table[tindex];
            /* XXX: optimize */
            ptr = &s->coefs1[ch][0];
            eptr = ptr + nb_coefs[ch];
            memset(ptr, 0, s->block_len * sizeof(int16_t));
            for (;;) {
                code = get_vlc2(&s->gb, coef_vlc->table, VLCBITS, VLCMAX);
                if (code < 0)
                    return -1;
                if (code == 1) {
                    /* EOB */
                    break;
                } else if (code == 0) {
                    /* escape */
                    level = get_bits(&s->gb, coef_nb_bits);
                    /* NOTE: this is rather suboptimal. reading
                       block_len_bits would be better */
                    run = get_bits(&s->gb, s->frame_len_bits);
                } else {
                    /* normal code */
                    run = run_table[code];
                    level = level_table[code];
                }
                sign = get_bits(&s->gb, 1);
                if (!sign)
                    level = -level;
                ptr += run;
                if (ptr >= eptr)
                    return -1;
                *ptr++ = level;
                /* NOTE: EOB can be omitted */
                if (ptr >= eptr)
                    break;
            }
        }
        if (s->version == 1 && s->nb_channels >= 2) {
            align_get_bits(&s->gb);
        }
    }

    /* normalize */
    {
        int n4_copy = (s->block_len);
        n4_copy = (n4_copy << 13);
        mdct_norm_copy = 0x40000000 / (n4_copy);

        if (s->version == 1) {
            mdct_norm_copy = (mdct_norm_copy >> 8) * (fixed32_sqrt(n4_copy) >> 8);
        }
    }

    /* finally compute the MDCT coefficients */
    for (ch = 0; ch < s->nb_channels; ch++) {
        if (s->channel_coded[ch]) {
            int16_t *coefs1;
            int *coefs_copy, *exponents_ptr, *exp_ptr_copy;
            int exponents_copy;

            long long multcopy1; // HDRB

            int max_copy;
            int mult_copy;
            int mult1_copy;
            int nosie_copy;

            int i, j, n, n1, last_high_band;

            int exp_power[HIGH_BAND_MAX_SIZE];

            coefs1 = s->coefs1[ch];
            exponents_ptr = s->exponents[ch];

            max_copy = (s->max_exponent[ch]);
            multcopy1 = g_pow1div20[total_gain];
            multcopy1 = (multcopy1) / max_copy;

            mult_copy = (multcopy1 >> 16);
            mult_copy = mult_copy * (mdct_norm_copy);

            coefs_copy = s->coefs[ch];

            if (s->use_noise_coding) // HD3.wma HD5.wma HD6.wma HD7.wma
            {
                mult1_copy = mult_copy;

                for (i = 0; i < s->coefs_start; i++) {

                    long long value64;

                    nosie_copy = (s->noise_table[s->noise_index]);

                    exponents_copy = (*exponents_ptr++);

                    value64 = ((long long)(nosie_copy)) * ((long long)(*exponents_ptr++)) * ((long long)(mult_copy));
                    *coefs_copy++ = ((value64) / 0x0000000100000000);
                    s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                }

                n1 = s->exponent_high_sizes[bsize];

                /* compute power of high bands */
                exp_ptr_copy = exponents_ptr +
                               s->high_band_start[bsize] -
                               s->coefs_start;
                last_high_band = 0; /* avoid warning */

                for (j = 0; j < n1; j++) {

                    n = s->exponent_high_bands[s->frame_len_bits - s->block_len_bits][j];

                    if (s->high_band_coded[ch][j]) {
                        int e2_copy1, e2_copy, v_copy; // HDRB
                        e2_copy = 0;
                        e2_copy1 = 0;

                        for (i = 0; i < n; i++) {
                            v_copy = (exp_ptr_copy[i]);
                            v_copy = (v_copy >> 2);
                            v_copy = (v_copy) * (v_copy);
                            v_copy = v_copy >> 12;

                            e2_copy += v_copy;
                        }

                        e2_copy = e2_copy / n;
                        exp_power[j] = (e2_copy);

                        last_high_band = j;
                        (void)e2_copy1;
                    }

                    exp_ptr_copy += n;
                }
                /* main freqs and high freqs */
                for (j = -1; j < n1; j++) {
                    if (j < 0) {
                        n = s->high_band_start[bsize] - s->coefs_start;
                    } else {
                        n = s->exponent_high_bands[s->frame_len_bits - s->block_len_bits][j];
                    }

                    if (j >= 0 && s->high_band_coded[ch][j]) //  HD7.wma
                    {
                        int fpTemp1;
                        int fpTemp2;

                        mult1_copy = 0x1000;
                        if (exp_power[last_high_band])
                            mult1_copy = (exp_power[j] << 12) / exp_power[last_high_band];

                        mult1_copy = mult1_copy << 4;

                        mult1_copy = fixed32_sqrt(mult1_copy);

                        mult1_copy = (mult1_copy >> 8) * (g_pow1div20copy[s->high_band_values[ch][j]]);

                        fpTemp1 = ((s->max_exponent[ch]));
                        fpTemp2 = (s->noise_mult);
                        fpTemp1 = (fpTemp1 >> 8) * fpTemp2;
                        fpTemp1 = (fpTemp1 >> 8);

                        mult1_copy = mult_copy / fpTemp1;

                        mult1_copy = (mult1_copy)*mdct_norm_copy;

                        for (i = 0; i < n; i++) // HD7.wma //  n[48 167]
                        {
                            nosie_copy = (s->noise_table[s->noise_index]);
                            s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                            nosie_copy = (nosie_copy) * (mult1_copy);
                            nosie_copy = nosie_copy / (0x00010000);

                            nosie_copy = (nosie_copy) * (*exponents_ptr++);
                            nosie_copy = nosie_copy / (0x00010000);
                            *coefs_copy++ = (nosie_copy);
                        }
                    } else { // HD3.wma HD5.wma HD6.wma
                        for (i = 0; i < n; i++) {
                            long long value64;

                            nosie_copy = (s->noise_table[s->noise_index]);
                            nosie_copy += ((*coefs1++) * (0x00010000));

                            value64 = ((long long)(nosie_copy)) * ((long long)(*exponents_ptr++)) * ((long long)(mult_copy));

                            *coefs_copy++ = ((value64) / 0x0000000100000000);

                            s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                        }
                    }
                }

                /* very high freqs : noise */
                n = s->block_len - s->coefs_end[bsize];
                exponents_copy = (exponents_ptr[-1]);

                mult1_copy = (mult_copy) * (exponents_copy >> 8);
                mult1_copy = (mult1_copy >> 8);

                for (i = 0; i < n; i++) {
                    nosie_copy = (s->noise_table[s->noise_index]);
                    *coefs_copy++ = (nosie_copy >> 4) * (mult1_copy >> 12);
                    s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                }
            } else { // HD1.wma HD2.wma HD4.wma
                for (i = 0; i < s->coefs_start; i++)
                    *coefs_copy++ = 0;
                n = nb_coefs[ch];

                for (i = 0; i < n; i++) {
                    exponents_copy = (exponents_ptr[i] / 0x00000100) * coefs1[i];

                    *coefs_copy++ = (exponents_copy / 0x00000100) * mult_copy;
                }

                n = s->block_len - s->coefs_end[bsize];

                for (i = 0; i < n; i++)
                    *coefs_copy++ = 0;
            }
            (void)exponents_copy;
        }
    }

#ifdef TRACE
    for (ch = 0; ch < s->nb_channels; ch++) {
        if (s->channel_coded[ch]) {
            dump_floats("exponents", 3, s->exponents[ch], s->block_len);
            dump_floats("coefs", 1, s->coefs[ch], s->block_len);
        }
    }
#endif

    if (s->ms_stereo && s->channel_coded[1]) {
        int a, b;
        int i;

        /* nominal case for ms stereo: we do it before mdct */
        /* no need to optimize this case because it should almost
           never happen */
        if (!s->channel_coded[0]) {
            av_log(NULL, AV_LOG_INFO, "rare ms-stereo case happened\n");
            memset(s->coefs[0], 0, sizeof(int) * s->block_len);
            s->channel_coded[0] = 1;
        }

        for (i = 0; i < s->block_len; i++) {
            a = s->coefs[0][i];
            b = s->coefs[1][i];
            s->coefs[0][i] = a + b;
            s->coefs[1][i] = a - b;
        }
    }

    /* XXX: merge with output */
    {
        int i, next_block_len, block_len, prev_block_len, n;
        int *wptr_copy;

        block_len = s->block_len;
        prev_block_len = 1 << s->prev_block_len_bits;
        next_block_len = 1 << s->next_block_len_bits;

        /* right part */
        wptr_copy = window + block_len;
        if (block_len <= next_block_len) {
            for (i = 0; i < block_len; i++)
                *wptr_copy++ = s->windows[bsize][i];
        } else {
            /* overlap */
            n = (block_len / 2) - (next_block_len / 2);
            for (i = 0; i < n; i++)
                *wptr_copy++ = 0x00010000;

            for (i = 0; i < next_block_len; i++)
                *wptr_copy++ = s->windows[s->frame_len_bits - s->next_block_len_bits][i];

            for (i = 0; i < n; i++)
                *wptr_copy++ = 0;
        }

        /* left part */
        wptr_copy = window + block_len;
        if (block_len <= prev_block_len) {
            for (i = 0; i < block_len; i++)
                *--wptr_copy = s->windows[bsize][i];
        } else {
            /* overlap */
            n = (block_len / 2) - (prev_block_len / 2);

            for (i = 0; i < n; i++)
                *--wptr_copy = 0x00010000;
            for (i = 0; i < prev_block_len; i++)
                *--wptr_copy = s->windows[s->frame_len_bits - s->prev_block_len_bits][i];

            for (i = 0; i < n; i++)
                *--wptr_copy = 0;
        }
    }

    for (ch = 0; ch < s->nb_channels; ch++) {
        if (s->channel_coded[ch]) {
            long long *output = s->output;
            int *output1 = s->output1;
            int *ptr_copy;
            int i, n4, index, n;

            n = s->block_len;
            n4 = s->block_len / 2;
            ff_imdct_calc(&s->mdct_ctx[bsize], (FFTSample *)output,
                          (FFTSample *)s->coefs[ch], (FFTSample *)s->mdct_tmp);

            /* XXX: optimize all that by build the window and
               multipying/adding at the same time */
            /* multiply by the window */
            for (i = 0; i < n * 2; i++) {

                output[i] = (output[i] >> 34) * (window[i]);

                output1[i] = (output[i]);
            }

            /* add in the frame */
            index = (s->frame_len / 2) + s->block_pos - n4;
            ptr_copy = &s->frame_out[ch][index];

            for (i = 0; i < n * 2; i++) {
                *ptr_copy += (output1[i]);
                ptr_copy++;
            }

            /* specific fast case for ms-stereo : add to second
               channel if it is not coded */
            if (s->ms_stereo && !s->channel_coded[1]) {
                ptr_copy = &s->frame_out[1][index];
                for (i = 0; i < n * 2; i++) {
                    *ptr_copy += (output1[i]);
                    ptr_copy++;
                }
            }
        }
    }

next:
    /* update block number */
    s->block_num++;
    s->block_pos += s->block_len;
    if (s->block_pos >= s->frame_len)
        return 1;
    else
        return 0;
}

/* decode a frame of frame_len samples */
static int wma_decode_frame(WMADecodeContext *s, int16_t *samples)
{
    int ret, i, n, a, ch, incr;
    int16_t *ptr;
    // float *iptr;
    int *iptr_copy;
#ifdef TRACE
    av_log(NULL, AV_LOG_INFO, "***decode_frame: %d size=%d\n", s->frame_count++, s->frame_len);
#endif

    /* read each block */
    s->block_num = 0;
    s->block_pos = 0;
    for (;;) {
        ret = wma_decode_block(s);
        if (ret < 0)
            return -1;
        if (ret)
            break;
    }

    /* convert frame to integer */
    n = s->frame_len;
    incr = s->nb_channels;
    for (ch = 0; ch < s->nb_channels; ch++) {
        ptr = samples + ch;
        iptr_copy = s->frame_out[ch];

        for (i = 0; i < n; i++) {
            a = ((*iptr_copy) >> 14);
            iptr_copy++;

            if (a > 32767)
                a = 32767;
            else if (a < -32768)
                a = -32768;
            *ptr = (int16_t)a;
            ptr += incr;
        }
        /* prepare for next block */
        memmove(&s->frame_out[ch][0], &s->frame_out[ch][s->frame_len],
                s->frame_len * sizeof(int));
        /* XXX: suppress this */
        memset(&s->frame_out[ch][s->frame_len], 0,
               s->frame_len * sizeof(int));
    }

#ifdef TRACE
    dump_shorts("samples", samples, n * s->nb_channels);
#endif
    return 0;
}

static int wma_decode_superframe(AVCodecContext *avctx,
                                 void *frame_data, int *got_frame_ptr,
                                 AVPacket *avpkt)
{
    AVFrame *frame = frame_data;
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    WMADecodeContext *s = avctx->priv_data;
    int nb_frames, bit_offset, i, pos, len;
    uint8_t *q;
    int16_t *samples;
    void *data = frame->data[0];

    av_log(NULL, AV_LOG_DEBUG, "***decode_superframe:\n");

    if (buf_size == 0) {
        s->last_superframe_len = 0;
        *got_frame_ptr = 0;
        return 0;
    }

    samples = data;

    init_get_bits(&s->gb, buf, buf_size * 8);

    if (s->use_bit_reservoir) {
        /* read super frame header */
        get_bits(&s->gb, 4); /* super frame index */
        nb_frames = get_bits(&s->gb, 4) - 1;

        bit_offset = get_bits(&s->gb, s->byte_offset_bits + 3);

        if (s->last_superframe_len > 0) {
            /* add bit_offset bits to last frame */
            if ((s->last_superframe_len + ((bit_offset + 7) >> 3)) >
                MAX_CODED_SUPERFRAME_SIZE)
                goto fail;
            q = s->last_superframe + s->last_superframe_len;
            len = bit_offset;
            while (len > 0) {
                *q++ = (get_bits)(&s->gb, 8);
                len -= 8;
            }
            if (len > 0) {
                *q++ = (get_bits)(&s->gb, len) << (8 - len);
            }

            /* XXX: bit_offset bits into last frame */
            init_get_bits(&s->gb, s->last_superframe, MAX_CODED_SUPERFRAME_SIZE * 8);
            /* skip unused bits */
            if (s->last_bitoffset > 0)
                skip_bits(&s->gb, s->last_bitoffset);
            /* this frame is stored in the last superframe and in the current one */
            if (wma_decode_frame(s, samples) < 0)
                goto fail;
            samples += s->nb_channels * s->frame_len;
        }

        /* read each frame starting from bit_offset */
        pos = bit_offset + 4 + 4 + s->byte_offset_bits + 3;
        init_get_bits(&s->gb, buf + (pos >> 3), (MAX_CODED_SUPERFRAME_SIZE - (pos >> 3)) * 8);
        len = pos & 7;
        if (len > 0)
            skip_bits(&s->gb, len);

        s->reset_block_lengths = 1;
        for (i = 0; i < nb_frames; i++) {
            if (wma_decode_frame(s, samples) < 0)
                goto fail;
            samples += s->nb_channels * s->frame_len;
        }

        /* we copy the end of the frame in the last frame buffer */
        pos = get_bits_count(&s->gb) + ((bit_offset + 4 + 4 + s->byte_offset_bits + 3) & ~7);
        s->last_bitoffset = pos & 7;
        pos >>= 3;
        len = buf_size - pos;
        if (len > MAX_CODED_SUPERFRAME_SIZE || len < 0) {
            goto fail;
        }
        s->last_superframe_len = len;
        memcpy(s->last_superframe, buf + pos, len);
    } else {
        /* single frame decode */
        if (wma_decode_frame(s, samples) < 0)
            goto fail;
        samples += s->nb_channels * s->frame_len;
    }
    frame->frame_size = (int8_t *)samples - (int8_t *)data;
    *got_frame_ptr = 1;
    return s->block_align;
fail:
    /* when error, we reset the bit reservoir */
    s->last_superframe_len = 0;
    *got_frame_ptr = 0;
    return -1;
}

static int wma_decode_end(AVCodecContext *avctx)
{
    WMADecodeContext *s = avctx->priv_data;
    int i;

    for (i = 0; i < s->nb_block_sizes; i++)
        ff_mdct_end(&s->mdct_ctx[i]);
    for (i = 0; i < s->nb_block_sizes; i++)
        av_free(s->windows[i]);

    if (s->use_exp_vlc) {
        ff_free_vlc(&s->exp_vlc);
    }
    if (s->use_noise_coding) {
        ff_free_vlc(&s->hgain_vlc);
    }
    for (i = 0; i < 2; i++) {
        ff_free_vlc(&s->coef_vlc[i]);
        av_free(s->run_table[i]);
        av_free(s->level_table[i]);
    }

    return 0;
}

AVCodec ff_wmav1_decoder = {
    .name = "wmav1",
    .type = AVMEDIA_TYPE_AUDIO,
    .id = AV_CODEC_ID_WMAV1,
    .priv_data_size = sizeof(WMADecodeContext),
    .init = wma_decode_init,
    .close = wma_decode_end,
    .decode = wma_decode_superframe,
};

AVCodec ff_wmav2_decoder = {
    .name = "wmav2",
    .type = AVMEDIA_TYPE_AUDIO,
    .id = AV_CODEC_ID_WMAV2,
    .priv_data_size = sizeof(WMADecodeContext),
    .init = wma_decode_init,
    .close = wma_decode_end,
    .decode = wma_decode_superframe,
};
