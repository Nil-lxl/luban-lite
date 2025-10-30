/*
 * Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  author: <qi.xu@artinchip.com>
 *  Desc: mov
 */
#define LOG_TAG "mov"

#include <stdlib.h>
#include <inttypes.h>
#include "aic_stream.h"
#include "mov.h"
#include "mov_tags.h"
#include "mpp_mem.h"
#include "mpp_log.h"

/* those functions parse an atom */
/* links atom IDs to parse functions */
struct mov_parse_table {
    uint32_t type;
    int (*parse)(struct aic_mov_parser *ctx, struct mov_atom atom);
};

static int mov_read_default(struct aic_mov_parser *c, struct mov_atom atom);

static int64_t get_time(int64_t pts, int64_t st_time_scale)
{
    return pts * TIME_BASE / st_time_scale;
}

static int mov_read_moov(struct aic_mov_parser *c, struct mov_atom atom)
{
    int ret;

    if (c->find_moov) {
        loge("find duplicated moov atom. skipped it");
        aic_stream_skip(c->stream, atom.size);
        return 0;
    }

    if ((ret = mov_read_default(c, atom)) < 0)
        return ret;
    c->find_moov = 1;
    return 0;
}

static int mov_read_mvhd(struct aic_mov_parser *c, struct mov_atom atom)
{
    int i;
    int version = aic_stream_r8(c->stream);
    aic_stream_rb24(c->stream);

    if (version == 1) {
        aic_stream_rb64(c->stream); // create time
        aic_stream_rb64(c->stream); // modify time
    } else {
        aic_stream_rb32(c->stream); // create time
        aic_stream_rb32(c->stream); // modify time
    }

    c->time_scale = aic_stream_rb32(c->stream);
    if (c->time_scale <= 0) {
        loge("Invalid mvhd time scale %d, default to 1", c->time_scale);
        c->time_scale = 1;
    }

    c->duration = (version == 1) ? aic_stream_rb64(c->stream) : aic_stream_rb32(c->stream);

    aic_stream_rb32(c->stream); // preferred scale
    aic_stream_rb16(c->stream); /* preferred volume */

    aic_stream_skip(c->stream, 10); /* reserved */

    /* movie display matrix, store it in main context and use it later on */
    for (i=0; i<3; i++) {
        c->movie_display_matrix[i][0] = aic_stream_rb32(c->stream); // 16.16 fixed point
        c->movie_display_matrix[i][1] = aic_stream_rb32(c->stream); // 16.16 fixed point
        c->movie_display_matrix[i][2] = aic_stream_rb32(c->stream); //  2.30 fixed point
    }

    aic_stream_rb32(c->stream); /* preview time */
    aic_stream_rb32(c->stream); /* preview duration */
    aic_stream_rb32(c->stream); /* poster time */
    aic_stream_rb32(c->stream); /* selection time */
    aic_stream_rb32(c->stream); /* selection duration */
    aic_stream_rb32(c->stream); /* current time */
    aic_stream_rb32(c->stream); /* next track ID */

    return 0;
}

static struct mov_stream_ctx *new_stream(struct aic_mov_parser *c)
{
    struct mov_stream_ctx *sc;

    sc = (struct mov_stream_ctx *)mpp_alloc(sizeof(struct mov_stream_ctx));
    if (sc == NULL) {
        return NULL;
    }
    memset(sc, 0, sizeof(struct mov_stream_ctx));
    sc->type = MPP_MEDIA_TYPE_UNKNOWN;
    sc->index = c->nb_streams;
    return sc;
}

static void mov_build_index(struct aic_mov_parser *c, struct mov_stream_ctx *st)
{

}

static int mov_codec_id(struct mov_stream_ctx *st, uint32_t format)
{
    int id = aic_codec_get_id(mov_audio_tags, format);

    if (st->type == MPP_MEDIA_TYPE_VIDEO)
        id = aic_codec_get_id(mov_video_tags, format);

    return id;
}

static int mov_read_trak(struct aic_mov_parser *c, struct mov_atom atom)
{
    int ret;
    struct mov_stream_ctx *st = new_stream(c);

    c->streams[c->nb_streams++] = st;

    if ((ret = mov_read_default(c, atom) < 0)) {
        loge("read trak error");
        return ret;
    }

    // build index
    mov_build_index(c, st);

    return 0;
}

static int mov_read_tkhd(struct aic_mov_parser *c, struct mov_atom atom)
{
    int i;
    int width;
    int height;
    int version;
    struct mov_stream_ctx *st;

    if (c->nb_streams < 1)
        return 0;

    st = c->streams[c->nb_streams - 1];
    version = aic_stream_r8(c->stream);
    aic_stream_rb24(c->stream);

    if (version == 1) {
        aic_stream_rb64(c->stream);
        aic_stream_rb64(c->stream);
    } else {
        aic_stream_rb32(c->stream);
        aic_stream_rb32(c->stream);
    }

    st->id = aic_stream_rb32(c->stream);
    aic_stream_rb32(c->stream); /* reserved */

    /* highlevel (considering edits) duration in movie timebase */
    (version == 1) ? aic_stream_rb64(c->stream) : aic_stream_rb32(c->stream);
    aic_stream_rb32(c->stream); /* reserved */
    aic_stream_rb32(c->stream); /* reserved */

    aic_stream_rb16(c->stream); /* layer */
    aic_stream_rb16(c->stream); /* alternate group */
    aic_stream_rb16(c->stream); /* volume */
    aic_stream_rb16(c->stream); /* reserved */

    // read in the display matrix (outlined in ISO 14496-12, Section 6.2.2)
    // they're kept in fixed point format through all calculations
    // save u,v,z to store the whole matrix in the AV_PKT_DATA_DISPLAYMATRIX
    // side data, but the scale factor is not needed to calculate aspect ratio

    for (i = 0; i < 3; i++) {
        aic_stream_rb32(c->stream);   // 16.16 fixed point
        aic_stream_rb32(c->stream);   // 16.16 fixed point
        aic_stream_rb32(c->stream);   //  2.30 fixed point
    }


    width = aic_stream_rb32(c->stream);    // 16.16 fixed point track width
    height = aic_stream_rb32(c->stream);   // 16.16 fixed point track height
    st->width = width >> 16;
    st->height = height >> 16;

    return 0;
}

static int mov_read_hdlr(struct aic_mov_parser *c, struct mov_atom atom)
{
    uint32_t type;
    int64_t title_size;
    struct mov_stream_ctx *st;

    aic_stream_r8(c->stream);
    aic_stream_rb24(c->stream);

    aic_stream_rl32(c->stream);
    type = aic_stream_rl32(c->stream);

    st = c->streams[c->nb_streams-1];

    if (type == MKTAG('v','i','d','e'))
        st->type = MPP_MEDIA_TYPE_VIDEO;
    else if (type == MKTAG('s','o','u','n'))
        st->type = MPP_MEDIA_TYPE_AUDIO;

    aic_stream_rb32(c->stream); /* component  manufacture */
    aic_stream_rb32(c->stream); /* component flags */
    aic_stream_rb32(c->stream); /* component flags mask */

    title_size = atom.size - 24;
    if (title_size > 0) {
        aic_stream_seek(c->stream, title_size, SEEK_CUR);
    }

    return 0;
}

static int mov_read_stsd_video(struct aic_mov_parser *c, struct mov_stream_ctx *st)
{
    uint32_t tmp, bit_depth;

    aic_stream_rb16(c->stream); /* version */
    aic_stream_rb16(c->stream); /* revision level */
    aic_stream_rl32(c->stream);
    aic_stream_rb32(c->stream); /* temporal quality */
    aic_stream_rb32(c->stream); /* spatial quality */

    st->width = aic_stream_rb16(c->stream);
    st->height = aic_stream_rb16(c->stream);

    aic_stream_rb32(c->stream); /* horiz resolution */
    aic_stream_rb32(c->stream); /* vert resolution */
    aic_stream_rb32(c->stream); /* data size, always 0 */
    aic_stream_rb16(c->stream); /* frames per samples */

    aic_stream_skip(c->stream, 32); /* compress name*/

    tmp = aic_stream_rb16(c->stream); /* bit depth */
    bit_depth = tmp & 0x1f;
    //grayscale = tmp & 0x20;

    aic_stream_rb16(c->stream);
    if ((bit_depth == 1 || bit_depth == 2 || bit_depth == 4 || bit_depth == 8)) {
        logw("video is pallet");
    }

    return 0;
}

static int mov_read_stsd_audio(struct aic_mov_parser *c, struct mov_stream_ctx *st)
{
    uint16_t version = aic_stream_rb16(c->stream);

    aic_stream_rb16(c->stream); /* revision level */
    aic_stream_rl32(c->stream); /* vendor */

    st->channels = aic_stream_rb16(c->stream);
    st->bits_per_sample = aic_stream_rb16(c->stream);

    aic_stream_rb16(c->stream); /* compress id */
    aic_stream_rb16(c->stream); /* packet size = 0 */

    st->sample_rate = (aic_stream_rb32(c->stream) >> 16);

    if (!c->isom) {
        if (version == 1) {
            aic_stream_rb32(c->stream); /* bytes per packet */
            st->samples_per_frame = aic_stream_rb32(c->stream); /* sample per frame */
            st->bytes_per_frame = aic_stream_rb32(c->stream); /* bytes per frame */
            aic_stream_rb32(c->stream); /* bytes per sample */
        } else if (version == 2) {
            aic_stream_rb32(c->stream); /* sizeof struct only */

            union int_float64{
                            long long i;
                            double f;
                        };
                        union int_float64 v;
            v.i = aic_stream_rb64(c->stream);
            st->sample_rate = v.f;
            st->channels = aic_stream_rb32(c->stream);
            aic_stream_rb32(c->stream);
            st->bits_per_sample = aic_stream_rb32(c->stream);

            aic_stream_rb32(c->stream); /* lpcm format specific flag */
            st->bytes_per_frame   = aic_stream_rb32(c->stream);
            st->samples_per_frame = aic_stream_rb32(c->stream);
        }
    }

    if (st->format == 0) {
        if (st->bits_per_sample == 8) {
            st->id = mov_codec_id(st, MKTAG('r','a','w',' '));
        } else if (st->bits_per_sample == 16) {
            st->id = mov_codec_id(st, MKTAG('t','w','o','s'));
        }
    }

    st->sample_size = (st->bits_per_sample >> 3) * st->channels;
    return 0;
}

static int mov_read_stsd_entries(struct aic_mov_parser *c, int entries)
{
    int i;
    struct mov_stream_ctx *st = c->streams[c->nb_streams-1];

    for (i=0; i<entries; i++) {
        struct mov_atom a = {0x64737473, 0};
        int ret;
        int64_t start_pos = aic_stream_tell(c->stream);
        int64_t size = aic_stream_rb32(c->stream);
        uint32_t format = aic_stream_rl32(c->stream);

        if (size >= 16) {
            aic_stream_rb32(c->stream);
            aic_stream_rb16(c->stream);
            aic_stream_rb16(c->stream);
        } else if (size <= 7) {
            loge("invalid size %"PRId64" in stsd", size);
            return -1;
        }

        st->id = mov_codec_id(st, format);
        st->format = format;

        if(st->id == CODEC_ID_NONE) {
            printf("skip invalid stsd format:%"PRIx32".\n", format);
            aic_stream_skip(c->stream, size - 16);
            continue;
        }

        if (st->type == MPP_MEDIA_TYPE_VIDEO) {
            mov_read_stsd_video(c, st);
        } else if (st->type == MPP_MEDIA_TYPE_AUDIO) {
            mov_read_stsd_audio(c, st);
        }

        /* read extra atoms at the end(avcC ...)*/
        a.size = size - (aic_stream_tell(c->stream) - start_pos);
        if (a.size > 8) {
            if ((ret = mov_read_default(c, a)) < 0)
                return ret;
        } else if (a.size > 0) {
            aic_stream_skip(c->stream, a.size);
        }

        st->stsd_count ++;
    }

    return 0;
}

static int mov_read_stsd(struct aic_mov_parser *c, struct mov_atom atom)
{
    int entries;
    struct mov_stream_ctx *st;

    if (c->nb_streams < 1)
        return 0;

    st = c->streams[c->nb_streams-1];
    st->stsd_version = aic_stream_r8(c->stream);
    aic_stream_rb24(c->stream);
    entries = aic_stream_rb32(c->stream);

    /* Each entry contains a size (4 bytes) and format (4 bytes). */
    if (entries <= 0 || entries > atom.size / 8 || entries > 1024) {
        loge("invalid STSD entries %d", entries);
        return -1;
    }

    if (st->extra_data) {
        loge("Duplicate stsd found in this track.");
        return -1;
        }

    if (entries > 1) {
        logw("entries(%d) > 1", entries);
    }

    return mov_read_stsd_entries(c, entries);
}

/*
'stss' - sample duration
*/
static int mov_read_stts(struct aic_mov_parser *c, struct mov_atom atom)
{
    struct mov_stream_ctx *st = c->streams[c->nb_streams-1];
    unsigned int entries;
    unsigned int i;
    int64_t duration = 0;
    int64_t total_sample_count = 0;

    if (c->nb_streams < 1)
        return 0;

    aic_stream_r8(c->stream);   /* version */
    aic_stream_rb24(c->stream); /* flags */
    entries = aic_stream_rb32(c->stream);

    st->stts_count = entries;
    st->stts_offset = aic_stream_tell(c->stream);

    for (i=0; i<entries; i++) {
        int sample_duration;
        unsigned int sample_count;

        sample_count = aic_stream_rb32(c->stream);
        sample_duration = aic_stream_rb32(c->stream);

        duration += (int64_t)sample_duration*(uint64_t)sample_count;
        total_sample_count += sample_count;
    }

    st->nb_frames = total_sample_count;
    st->duration = get_time(duration, st->time_scale);

    return 0;
}

/*
'stsc' -   Sample-to-Chunk Atoms
The sample-to-chunk atom contains a table that maps
samples to chunks in the media data stream
*/
static int mov_read_stsc(struct aic_mov_parser *c, struct mov_atom atom)
{
    unsigned int entries;

    if (c->nb_streams < 1)
        return 0;

    struct mov_stream_ctx *st = c->streams[c->nb_streams-1];

    aic_stream_r8(c->stream);
    aic_stream_rb24(c->stream);
    entries = aic_stream_rb32(c->stream);

    if (entries*12+4 > atom.size) {
        loge("stsc size error");
        return -1;
    }

    if (!entries)
        return 0;

    st->stsc_count = entries;
    st->stsc_offset = aic_stream_tell(c->stream);

    return 0;
}

/*
'stsz' - Sample Size Atoms
specify the size of each sample in the media
*/
static int mov_read_stsz(struct aic_mov_parser *c, struct mov_atom atom)
{
    unsigned int entries;
    unsigned int sample_size;

    if (c->nb_streams < 1)
        return 0;

    struct mov_stream_ctx *st = c->streams[c->nb_streams-1];

    aic_stream_r8(c->stream);
    aic_stream_rb24(c->stream);

    sample_size = aic_stream_rb32(c->stream);
    if (!st->sample_size) /* do not overwrite value computed in stsd */
        st->sample_size = sample_size;
    st->stsz_sample_size = sample_size;

    entries = aic_stream_rb32(c->stream);
    if (!entries)
        return 0;

    st->stsz_count = entries;
    st->stsz_offset = aic_stream_tell(c->stream);

    return 0;
}

/*
'stco' -  Chunk Offset Atoms
Chunk offset atoms identify the location of each chunk of data in the media's data stream
*/
static int mov_read_stco(struct aic_mov_parser *c, struct mov_atom atom)
{
    unsigned int entries;

    if (c->nb_streams < 1)
        return 0;

    struct mov_stream_ctx *st = c->streams[c->nb_streams-1];

    aic_stream_r8(c->stream);
    aic_stream_rb24(c->stream);
    entries = aic_stream_rb32(c->stream);

    if (!entries) {
        logw("stco entries is 0");
        return 0;
    }

    st->stco_count = entries;
    st->stco_offset = aic_stream_tell(c->stream);

    return 0;
}

static void update_dts_shift(struct mov_stream_ctx *st, int duration)
{
    if (duration > 0) {
        if (duration == INT_MIN) {
            duration ++;
        }
        st->dts_shift = MPP_MAX(st->dts_shift, -duration);
    }
}

// ctts: composition time
static int mov_read_ctts(struct aic_mov_parser *c, struct mov_atom atom)
{
    unsigned int i, entries;
    int total_count = 0;

    if (c->nb_streams < 1)
        return 0;

    struct mov_stream_ctx *st = c->streams[c->nb_streams-1];

    aic_stream_r8(c->stream);
    aic_stream_rb24(c->stream);
    entries = aic_stream_rb32(c->stream);

    if (!entries) {
        logw("ctts entries is 0");
        return 0;
    }

    if (st->ctts_data) {
        logw("duplicated ctts data");
        mpp_free(st->ctts_data);
        st->ctts_data = NULL;
    }

    st->ctts_count = 0;
    st->ctts_data = mpp_alloc(entries*sizeof(*st->ctts_data));
    if (st->ctts_data == NULL)
        return -1;

    for (i=0; i<entries; i++) {
        int count = aic_stream_rb32(c->stream);
        int duration = aic_stream_rb32(c->stream);

        total_count += count;

        if (count <= 0) {
            logi("ignore ctts entry");
            continue;
        }

        st->ctts_data[i].count = count;
        st->ctts_data[i].duration = duration;
        st->ctts_count ++;

        if (i+2 < entries) {
            update_dts_shift(st, duration);
        }
    }

    // if st->ctts_data[i].count > 1, we should relloc a large buffer for ctts
    if (total_count != st->ctts_count) {
        struct mov_stts *ctts_data_old = st->ctts_data;
        int ctts_idx = 0;
        int j;

        st->ctts_data = mpp_alloc(total_count*sizeof(*st->ctts_data));
        for (i=0; i<entries; i++) {
            for(j=0; j<ctts_data_old[i].count; j++) {
                st->ctts_data[ctts_idx].count = 1;
                st->ctts_data[ctts_idx].duration = ctts_data_old[i].duration;
                ctts_idx ++;
            }
        }

        mpp_free(ctts_data_old);
        st->ctts_count = total_count;
    }

    logi("dts shift: %d", st->dts_shift);

    return 0;
}

// key frames
static int mov_read_stss(struct aic_mov_parser *c, struct mov_atom atom)
{
    unsigned int i, entries;

    if (c->nb_streams < 1)
        return 0;

    struct mov_stream_ctx *st = c->streams[c->nb_streams-1];

    aic_stream_r8(c->stream);
    aic_stream_rb24(c->stream);
    entries = aic_stream_rb32(c->stream);

    if (!entries) {
        logw("stts entries is 0");
        return 0;
    }

    if (st->keyframes) {
        logw("duplicated stss data");
        mpp_free(st->keyframes);
        st->keyframes = NULL;
    }

    st->keyframe_count = entries;
    st->keyframes = mpp_alloc(entries*sizeof(*st->keyframes));

    for (i=0; i<entries; i++) {
        st->keyframes[i] = aic_stream_rb32(c->stream);
    }

    return 0;
}

static int mov_read_mdhd(struct aic_mov_parser *c, struct mov_atom atom)
{
    int version;
    struct mov_stream_ctx *st;
    if (c->nb_streams < 1)
        return 0;
    st = c->streams[c->nb_streams-1];

    if (st->time_scale) {
        loge("multi mdhd?");
        return -1;
    }

    version = aic_stream_r8(c->stream);
    aic_stream_rb24(c->stream);
    if (version > 1) {
        aic_stream_rb64(c->stream);
        aic_stream_rb64(c->stream);
    } else {
        aic_stream_rb32(c->stream);
        aic_stream_rb32(c->stream);
    }

    st->time_scale = aic_stream_rb32(c->stream);
    if (st->time_scale <= 0) {
        st->time_scale = c->time_scale;
    }

    st->duration = (version == 1) ? aic_stream_rb64(c->stream) : aic_stream_rb32(c->stream);

    aic_stream_rb16(c->stream);
    aic_stream_rb16(c->stream);

    return 0;
}

static int mov_read_glbl(struct aic_mov_parser *c, struct mov_atom atom)
{
    struct mov_stream_ctx *st;

    if (c->nb_streams < 1)
        return 0;
    st = c->streams[c->nb_streams-1];

    st->extra_data_size = atom.size;
    st->extra_data = (unsigned char*)mpp_alloc(atom.size);
    if (st->extra_data == NULL)
        return -1;
    memset(st->extra_data, 0, st->extra_data_size);

    aic_stream_read(c->stream, st->extra_data, st->extra_data_size);

    if (atom.type == MKTAG('h','v','c','C')) {
        loge("not support hevc");
        return -1;
    }

    return 0;
}

static int mov_read_ftyp(struct aic_mov_parser *c, struct mov_atom atom)
{
    uint8_t type[5] = {0};

    aic_stream_read(c->stream, type, 4);
    if (strcmp((char*)type, "qt  "))
        c->isom = 1;

    aic_stream_rb32(c->stream); // minor version

    return 0;
}

static int mov_read_mdat(struct aic_mov_parser *c, struct mov_atom atom)
{
    if (atom.size == 0)
        return 0;
    c->find_mdat = 1;
    return 0;
}

static int mp4_read_desc(struct aic_stream *s, int *tag)
{
    int len = 0;
    *tag = aic_stream_r8(s);

    int count = 4;
    while (count--) {
        int c = aic_stream_r8(s);
        len = (len << 7) | (c & 0x7f);
        if (!(c & 0x80))
            break;
    }
    return len;
}

#define MP4ESDescrTag 			0x03
#define MP4DecConfigDescrTag		0x04
#define MP4DecSpecificDescrTag		0x05
static int mp4_read_dec_config_descr(struct aic_mov_parser *c, struct mov_stream_ctx *st)
{
    enum CodecID codec_id;
    int len, tag;

    int object_type_id = aic_stream_r8(c->stream);
    aic_stream_r8(c->stream);
    aic_stream_rb24(c->stream);
    aic_stream_rb32(c->stream);
    aic_stream_rb32(c->stream); // avg bitrate

    codec_id = aic_codec_get_id(mp4_obj_type, object_type_id);
    if (codec_id)
        st->id = codec_id;

    len = mp4_read_desc(c->stream, &tag);
    if (tag == MP4DecSpecificDescrTag) {
        if (len > (1<<30))
            return -1;

        st->extra_data_size = len;
        st->extra_data = (unsigned char*)mpp_alloc(len);
        if (st->extra_data == NULL)
            return -1;
        memset(st->extra_data, 0, st->extra_data_size);

        aic_stream_read(c->stream, st->extra_data, st->extra_data_size);
    }

    return 0;
}

static int mov_read_esds(struct aic_mov_parser *c, struct mov_atom atom)
{
    struct mov_stream_ctx *st;
    int tag;
    int ret = 0;

    if (c->nb_streams < 1)
        return 0;
    st = c->streams[c->nb_streams-1];
    aic_stream_rb32(c->stream);

    mp4_read_desc(c->stream, &tag);
    if (tag == MP4ESDescrTag) {
        aic_stream_rb24(c->stream);
    } else {
        aic_stream_rb16(c->stream);
    }

    mp4_read_desc(c->stream, &tag);
    if (tag == MP4DecConfigDescrTag) {
        mp4_read_dec_config_descr(c, st);
    }

    return ret;
}

static const struct mov_parse_table mov_default_parse_table[] = {
    { MKTAG('c','o','6','4'), mov_read_stco },
    { MKTAG('c','t','t','s'), mov_read_ctts }, /* composition time to sample */
    { MKTAG('d','i','n','f'), mov_read_default },
    { MKTAG('e','d','t','s'), mov_read_default },
    { MKTAG('f','t','y','p'), mov_read_ftyp },
    { MKTAG('g','l','b','l'), mov_read_glbl },
    { MKTAG('h','d','l','r'), mov_read_hdlr },
    { MKTAG('m','d','a','t'), mov_read_mdat },
    { MKTAG('m','d','h','d'), mov_read_mdhd },
    { MKTAG('m','d','i','a'), mov_read_default },
    { MKTAG('m','i','n','f'), mov_read_default },
    { MKTAG('m','o','o','v'), mov_read_moov },
    { MKTAG('m','v','e','x'), mov_read_default },
    { MKTAG('m','v','h','d'), mov_read_mvhd },
    { MKTAG('a','v','c','C'), mov_read_glbl },
    { MKTAG('s','t','b','l'), mov_read_default },
    { MKTAG('s','t','c','o'), mov_read_stco },
    { MKTAG('s','t','s','c'), mov_read_stsc },
    { MKTAG('s','t','s','d'), mov_read_stsd }, /* sample description */
    { MKTAG('s','t','s','s'), mov_read_stss }, /* sync sample */
    { MKTAG('s','t','s','z'), mov_read_stsz }, /* sample size */
    { MKTAG('s','t','t','s'), mov_read_stts },
    { MKTAG('t','k','h','d'), mov_read_tkhd }, /* track header */
    { MKTAG('t','r','a','k'), mov_read_trak },
    { MKTAG('t','r','a','f'), mov_read_default },
    { MKTAG('t','r','e','f'), mov_read_default },
    { MKTAG('u','d','t','a'), mov_read_default },
    //{ MKTAG('c','m','o','v'), mov_read_cmov },
    { MKTAG('h','v','c','C'), mov_read_glbl },
    { MKTAG('e','s','d','s'), mov_read_esds },
    { 0, NULL }
};

static int mov_read_default(struct aic_mov_parser *c, struct mov_atom atom)
{
    int i = 0;
    int64_t total_size = 0;
    struct mov_atom a;

    if (c->atom_depth > 10) {
        loge("atom too deeply nested");
        return -1;
    }
    c->atom_depth ++;

    if (atom.size < 0)
        atom.size = INT64_MAX;
    while (total_size <= (atom.size - 8)) {
        int (*parse)(struct aic_mov_parser*, struct mov_atom) = NULL;
        a.size = atom.size;
        a.type = 0;
        if (atom.size >= 8) {
            a.size = aic_stream_rb32(c->stream);
            a.type = aic_stream_rl32(c->stream);

            total_size += 8;
            if (a.size == 1 && total_size + 8 <= atom.size) {
                // 64 bit extended size
                a.size = aic_stream_rb64(c->stream) - 8;
                total_size += 8;
            }
        }
        // logi("atom type:%c%c%c%c, parent:%c%c%c%c, sz:" "%"PRId64"%"PRId64"%"PRId64"",
        // 	(a.type)&0xff, (a.type>>8)&0xff,
        // 	(a.type>>16)&0xff, (a.type>>24)&0xff,
        // 	(atom.type)&0xff, (atom.type>>8)&0xff,
        // 	(atom.type>>16)&0xff, (atom.type>>24)&0xff,
        // 	a.size, total_size, atom.size);

        if (a.size == 0) {
            a.size = atom.size - total_size + 8;
        }
        if (a.size < 0)
            break;
        a.size -= 8;
        if (a.size < 0)
            break;
        if (atom.size - total_size < a.size)
            a.size = atom.size - total_size;

        for (i=0; i<mov_default_parse_table[i].type; i++) {
            if (mov_default_parse_table[i].type == a.type) {
                parse = mov_default_parse_table[i].parse;
                break;
            }
        }

        if (!parse) {
            aic_stream_skip(c->stream, a.size);
        } else {
            int64_t start_pos = aic_stream_tell(c->stream);
            int64_t left;
            int err = parse(c, a);
            if (err < 0) {
                c->atom_depth --;
                return err;
            }

            if (c->find_moov && c->find_mdat) {
                c->atom_depth --;
                return 0;
            }

            left = a.size - aic_stream_tell(c->stream) + start_pos;
            if (left > 0) {
                /* skip garbage at atom end */
                aic_stream_skip(c->stream, left);
            } else if (left < 0) {
                loge("overread end of atom %"PRIx32"", a.type);
                aic_stream_seek(c->stream, left, SEEK_CUR);
            }
        }

        total_size += a.size;
    }

    if (total_size < atom.size && atom.size < 0x7ffff)
        aic_stream_skip(c->stream, atom.size - total_size);

    c->atom_depth --;
    return 0;
}

static int read_offset_rb32(struct aic_stream *stream, int offset)
{
    aic_stream_seek(stream, offset, SEEK_SET);
    return aic_stream_rb32(stream);
}

static int update_dts(struct aic_mov_parser *c, struct mov_stream_ctx *st)
{
    int stts_sample_count;
    int stts_sample_duration;
    int64_t diff;

    st->stts_idx = 0;
    while (1) {
        int stts_offset = st->stts_offset + st->stts_idx * 8;

        stts_sample_count    = read_offset_rb32(c->stream, stts_offset);
        stts_sample_duration = read_offset_rb32(c->stream, stts_offset+4);
        st->stts_samples_acc += stts_sample_count;
        st->stts_duration_acc += stts_sample_count * stts_sample_duration;
        st->stts_sample_duration = stts_sample_duration;

        // find the sample presentation time
        if (st->cur_sample_idx < st->stts_samples_acc)
            break;

        st->stts_idx ++;
    }


    diff = (int64_t)st->stts_sample_duration * (st->stts_samples_acc - st->cur_sample_idx);
    if (st->stts_duration_acc < diff)
        st->cur_dts = 0;
    else
        st->cur_dts = st->stts_duration_acc - diff;

    return 0;
}

static int update_stsc(struct aic_mov_parser *c, struct mov_stream_ctx *st)
{
    if (st->stsc_idx < st->stsc_count &&st->chunk_idx >= st->stsc_end) {
        int offset = st->stsc_offset + st->stsc_idx * 12;
        st->stsc_first = read_offset_rb32(c->stream, offset) - 1;
        st->stsc_samples = read_offset_rb32(c->stream, offset + 4);
        st->chunk_sample_idx = 0; // s new sample idx
        st->stsc_idx ++;

        // the next entry of stsc atom
        if (st->stsc_idx >= st->stsc_count) {
            st->stsc_end = INT_MAX;
        } else {
            st->stsc_end = read_offset_rb32(c->stream, offset + 12) - 1;
        }

        // skip the redundance stsc table
        if (st->stsc_end == st->stsc_first) {
            int offset = st->stsc_offset + st->stsc_idx * 12;
            st->stsc_first = read_offset_rb32(c->stream, offset) - 1;
            st->stsc_samples = read_offset_rb32(c->stream, offset + 4);
            st->chunk_sample_idx = 0; // s new sample idx
            st->stsc_idx ++;

            // the next entry of stsc atom
            if (st->stsc_idx >= st->stsc_count) {
                st->stsc_end = INT_MAX;
            } else {
                st->stsc_end = read_offset_rb32(c->stream, offset + 12) - 1;
            }
        }
    }

    return 0;
}

static int parse_sample_offset(struct aic_mov_parser *c, struct mov_stream_ctx *st)
{
    unsigned int chunk_offset = read_offset_rb32(c->stream, st->stco_offset + st->stco_idx*4);
    unsigned int cur_sample_offset = chunk_offset + st->chunk_sample_size;

    return cur_sample_offset;
}

static int parse_sample_size(struct aic_mov_parser *c, struct mov_stream_ctx *st)
{
    unsigned int cur_sample_size = 0;
    if (st->stsz_sample_size) {
        cur_sample_size = st->stsz_sample_size;
    } else {
        int stsz_offset = st->stsz_offset + st->stsz_idx*4;
        cur_sample_size = read_offset_rb32(c->stream, stsz_offset);
        st->stsz_idx++;
    }
    st->chunk_sample_size += cur_sample_size;

    return cur_sample_size;
}

static int parse_sample_duration(struct aic_mov_parser *c, struct mov_stream_ctx *st)
{
    if (st->cur_sample_idx >= st->stts_samples_acc) {
        int stts_sample_count;
        int stts_sample_duration;
        while (1) {
            int stts_offset = st->stts_offset + st->stts_idx * 8;

            stts_sample_count    = read_offset_rb32(c->stream, stts_offset);
            stts_sample_duration = read_offset_rb32(c->stream, stts_offset+4);
            st->stts_samples_acc += stts_sample_count;
            st->stts_duration_acc += stts_sample_count * stts_sample_duration;
            st->stts_sample_duration = stts_sample_duration;
            st->stts_idx ++;

            // find the sample presentation time
            if (st->cur_sample_idx < st->stts_samples_acc)
                break;
        }
    }

    unsigned int diff = st->stts_sample_duration * (st->stts_samples_acc - st->cur_sample_idx);
    if (st->stts_duration_acc < diff)
        st->cur_dts = 0;
    else
        st->cur_dts = st->stts_duration_acc - diff;

    if (st->type == MPP_MEDIA_TYPE_VIDEO) {
        if (st->ctts_data && st->ctts_idx < st->ctts_count) {
            st->cur_dts += st->dts_shift + st->ctts_data[st->ctts_idx].duration;

            st->ctts_sample ++;
            if ((st->ctts_index < st->ctts_count) &&
                (st->ctts_data[st->ctts_index].count == st->ctts_sample) ) {
                st->ctts_index ++;
                st->ctts_sample = 0;
            }
        }
    }

    st->cur_sample_duration = st->stts_sample_duration;
    return st->cur_dts;
}

static int update_stco(struct aic_mov_parser *c, struct mov_stream_ctx *st)
{
    st->chunk_sample_idx ++;
    if (st->chunk_sample_idx >= st->stsc_samples) {
        st->chunk_sample_idx = 0;
        st->stco_idx++;
        st->chunk_idx++;
        st->chunk_sample_size = 0;
    }

    return 0;
}

static int find_next_sample(struct aic_mov_parser *c, struct mov_stream_ctx **st, struct index_entry *sample)
{
    int i;
    int64_t best_dts = INT64_MAX;
    int64_t time[4] = {0};
    int cur_st_idx = -1;

    // find the sample with the smallest dts from every streams
    for (i=0; i<c->nb_streams; i++) {
        struct mov_stream_ctx *cur_st = c->streams[i];
        if (cur_st->cur_sample_idx == cur_st->nb_frames)
            continue;

        time[i] = cur_st->cur_dts * 1000LL / cur_st->time_scale;

        if (time[i] < best_dts) {
            best_dts = time[i];
            cur_st_idx = i;
        }
    }

    if (cur_st_idx == -1) {
        logd("eos");
        return -1;
    }

    for (i=0; i<c->nb_streams; i++) {
        if (time[cur_st_idx] < best_dts) {
            cur_st_idx = i;
        }
    }
    *st = c->streams[cur_st_idx];

    update_stsc(c, *st);

    sample->pos = parse_sample_offset(c, *st);

    sample->size = parse_sample_size(c, *st);
    sample->timestamp =  parse_sample_duration(c, *st);
    update_stco(c, *st);


    if (*st) {
        (*st)->cur_sample_idx ++;
    }

    return 0;
}

int mov_peek_packet(struct aic_mov_parser *c, struct aic_parser_packet *pkt)
{
    struct mov_stream_ctx *st = NULL;


    if (find_next_sample(c, &st, &c->cur_sample)) {
        // eos
        return PARSER_EOS;
    }

    pkt->size = c->cur_sample.size;
    pkt->type = st->type;

    if (st->cur_sample_idx == st->nb_frames) {
        // eos now
        logi("[%s:%d] this stream eos", __FUNCTION__, __LINE__);
        pkt->flag = PACKET_EOS;
    }

    pkt->pts = get_time(c->cur_sample.timestamp, st->time_scale);

    if (st->ctts_data && st->ctts_index < st->ctts_count) {
        pkt->pts = c->cur_sample.timestamp + st->dts_shift + st->ctts_data[st->cur_sample_idx - 1].duration;

        pkt->pts = get_time(pkt->pts, st->time_scale);
    }

    return 0;
}

int mov_close(struct aic_mov_parser *c)
{
    //int i, j;
    int i;

    for (i=0; i<c->nb_streams; i++) {
        struct mov_stream_ctx *st = c->streams[i];

        if (st->ctts_data)
            mpp_free(st->ctts_data);
        if (st->keyframes)
            mpp_free(st->keyframes);
        if (st->extra_data)
            mpp_free(st->extra_data);
        if (st->index_entries)
            mpp_free(st->index_entries);

        mpp_free(st);
    }

    return 0;
}

int mov_read_header(struct aic_mov_parser *c)
{
    int err;
    struct mov_atom atom = {0, 0};

    atom.size = aic_stream_size(c->stream);
    do {
        err = mov_read_default(c, atom);
        if(err < 0) {
            loge("error reading header");
            return -1;
        }
    } while (!c->find_moov);

    if(!c->find_moov) {
        loge("moov atom not find");
        return -1;
    }

    return 0;
}

int find_index_by_pts(struct mov_stream_ctx *st,s64 pts)
{
    int i,index = 0;
    int64_t min = INT64_MAX;
    int64_t sample_pts;
    int64_t diff;
    struct index_entry *cur_sample = NULL;

    for (i = 0; i < st->nb_index_entries; i++) {
        cur_sample = &st->index_entries[i];
        sample_pts = get_time(cur_sample->timestamp, st->time_scale);
        diff = MPP_ABS(pts,sample_pts);
        if (diff < min) {
            min = diff;
            index = i;
        }
    }

    return index;
}

int find_key_frame(struct mov_stream_ctx *video_st, int sample_idx)
{
    int i;
    int min_dist = INT_MAX;
    int diff;
    int ret = 0;

    for (i = 0 ; i < video_st->keyframe_count; i++) {
        diff = MPP_ABS(sample_idx, video_st->keyframes[i]);
        if (diff < min_dist) {
            min_dist = diff;
            ret = video_st->keyframes[i] - 1;
        }

        if ((video_st->keyframes[i] - sample_idx) >= min_dist)
            break;
    }

    return ret;
}

// found the sample of video stream near time_us
static int time_to_sample(struct aic_mov_parser *c, struct mov_stream_ctx *st, int64_t time_us)
{
    int stts_offset = st->stts_offset;
    int sample_count;
    int sample_duration;
    int64_t duration_acc = 0;
    int sample_count_acc = 0;
    int stts_idx = 0;
    int64_t seek_time = (int64_t)st->time_scale * time_us / 1000000LL;

    while (1) {
        stts_offset = st->stts_offset + stts_idx*8;
        sample_count = read_offset_rb32(c->stream, stts_offset);
        sample_duration = read_offset_rb32(c->stream, stts_offset + 4);
        duration_acc += (int64_t)sample_count * (int64_t)sample_duration;
        sample_count_acc += sample_count;

        if (seek_time < duration_acc || stts_idx > st->stts_count)
            break;
        stts_idx++;
    }

    if (sample_duration == 0) {
        logw("Careful: sample_duration=0, set seek sample idx = 0");
        return 0;
    }

    return sample_count_acc - (duration_acc - seek_time) / sample_duration;
}

static int adjust_ctts_info(struct mov_stream_ctx *video_st)
{
    int time_sample;
    int i;
    if (video_st->ctts_data) {
        time_sample = 0;
        int next = 0;
        for(i=0; i<video_st->ctts_count; i++)
        {
            next = time_sample + video_st->ctts_data[i].count;
            if(next > video_st->cur_sample_idx)
            {
                video_st->ctts_index = i;
                video_st->ctts_sample = video_st->cur_sample_idx - time_sample;
                break;
            }
            time_sample = next;
        }
    }

    return 0;
}

static int update_video_stsz(struct aic_mov_parser *c, struct mov_stream_ctx *st,
    int sample_idx, int sample_in_chunk_ost)
{
    int i;
    int chunk_first_sample_ost;
    st->cur_sample_idx = st->stsz_idx = sample_idx;
    st->chunk_sample_size = 0;

    chunk_first_sample_ost = (sample_idx - sample_in_chunk_ost)<<2;
    for (i=0; i<sample_in_chunk_ost; i++) {
        st->chunk_sample_size += read_offset_rb32(c->stream,
            st->stsz_offset + chunk_first_sample_ost + i*4);
    }
    return 0;
}

//update video sample info: stsc_idx  stco_idx stsz_idx chunk_sample_size   chunk_sample_idx    sample_idx
static int seek_sample(struct aic_mov_parser *c, struct mov_stream_ctx *st, int sample_idx)
{
    int stsc_offset = 0;
    int stsc_first = 0;
    int stsc_samples;
    int stsc_next;
    int samples_in_chunk = 0;
    int stsc_samples_acc = 0;
    int prev_stsc_samples_acc;
    int sample_in_chunk_offset;

    st->stsc_idx = 0;
    stsc_first = read_offset_rb32(c->stream, st->stsc_offset);

    while (1) {
        stsc_offset = st->stsc_offset + st->stsc_idx * 12;
        stsc_samples = read_offset_rb32(c->stream, stsc_offset+4);
        stsc_next = read_offset_rb32(c->stream, stsc_offset+12);

        if (st->stsc_idx + 1 >= st->stsc_count) {
            stsc_next = INT_MAX;
            samples_in_chunk = INT_MAX;
        } else {
            samples_in_chunk = (stsc_next - stsc_first) * stsc_samples;
        }
        prev_stsc_samples_acc = stsc_samples_acc;
        stsc_samples_acc += samples_in_chunk;
        st->stsc_idx ++;
        if ((st->stsc_idx > st->stco_count) || (sample_idx <= stsc_samples_acc - 1))
            break;
        stsc_first = stsc_next;
    }

    st->stsc_samples = stsc_samples;
    st->stsc_end = stsc_next - 1;

    sample_in_chunk_offset = sample_idx - prev_stsc_samples_acc;
    st->stco_idx = stsc_first + (sample_in_chunk_offset / stsc_samples) -1;
    st->chunk_idx = st->stco_idx;
    sample_in_chunk_offset = sample_in_chunk_offset - (st->stco_idx + 1 - stsc_first) * stsc_samples;
    st->chunk_sample_idx = sample_in_chunk_offset;

    update_video_stsz(c, st, sample_idx, sample_in_chunk_offset);
    update_dts(c, st);

    return 0;
}

int mov_seek_packet(struct aic_mov_parser *c, s64 pts)
{
    int i;
    struct mov_stream_ctx *cur_st = NULL;
    struct mov_stream_ctx *video_st = NULL;
    struct mov_stream_ctx *audio_st = NULL;
    int sample_idx = 0;

    for (i = 0; i< c->nb_streams ;i++) {
        cur_st = c->streams[i];
        if ((!video_st) && (cur_st->type == MPP_MEDIA_TYPE_VIDEO)) {//only support first video stream
            video_st = c->streams[i];
        } else if ((!audio_st) && (cur_st->type == MPP_MEDIA_TYPE_AUDIO)) {//only support first audio stream
            audio_st = c->streams[i];
        }
    }

    if (video_st && (pts > (video_st->duration*1000))) {
        printf("pts: %lld, duration: %"PRId64", not seek\n", pts, video_st->duration*1000);
        return 0;
    }

    if (audio_st && (pts > (audio_st->duration*1000))) {
        printf("pts: %lld, duration: %"PRId64", not seek\n", pts, audio_st->duration);
        return 0;
    }

    if (video_st) {
        sample_idx = time_to_sample(c, video_st, pts);
        if(video_st->id == CODEC_ID_MJPEG) {
            video_st->cur_sample_idx = sample_idx > video_st->nb_frames - 1?
                video_st->nb_frames - 1 : sample_idx;
        } else {
            if (!video_st->keyframes) {
                loge("no keyframes\n");
                return -1;
            }
            video_st->cur_sample_idx = find_key_frame(video_st, sample_idx);
        }

        // adjust ctts index
        adjust_ctts_info(video_st);

        seek_sample(c, video_st, video_st->cur_sample_idx);
    }

    if (audio_st) {
        int64_t audio_pts = pts;
        if (video_st)
            audio_pts = get_time(video_st->cur_dts, video_st->time_scale);

        audio_st->cur_sample_idx = time_to_sample(c, audio_st, audio_pts);
        audio_st->cur_sample_idx = audio_st->cur_sample_idx > audio_st->nb_frames -1 ?
            audio_st->nb_frames - 1 : audio_st->cur_sample_idx;
        seek_sample(c, audio_st, audio_st->cur_sample_idx);
    }

    return 0;
}
