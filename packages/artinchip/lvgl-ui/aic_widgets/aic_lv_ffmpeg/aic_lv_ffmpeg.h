/**
 * lv__ffmpeg is compatible with the following formats on the luban and luban-lite platforms:
 *
 * Container Format: Supports MP4 container, mjpeg(D13x), H264(D21x) raw stream, and MP3 container.
 *
 * Transport Protocol: Supports local files.
 *
 * Video Decoding: Supports mjpeg(D13x), H264(D21x).

 * Audio Decoding: Supports MP3 and AAC.

 * This is because ffmpeg is implemented to interface with aic_player, so it only supports formats that are supported by aic_player.
 *
*/
/**
 * @file aic_lv_ffmpeg.h
 *
 */
#ifndef LV_AIC_FFMPEG_H
#define LV_AIC_FFMPEG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#if LV_USE_FFMPEG == 0 && defined(AIC_MPP_PLAYER_INTERFACE)
#include "aic_player.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
struct ffmpeg_context_s;

extern const lv_obj_class_t lv_ffmpeg_player_class;

typedef struct {
#if LVGL_VERSION_MAJOR == 8
    lv_img_t img;
#else
    lv_image_t img;
#endif
    lv_timer_t * timer;
    lv_img_dsc_t imgdsc;
    bool auto_restart;
    bool keep_last_frame;
    struct ffmpeg_context_s * ffmpeg_ctx;
} lv_ffmpeg_player_t;

typedef enum {
    LV_FFMPEG_PLAYER_CMD_START,
    LV_FFMPEG_PLAYER_CMD_STOP,
    LV_FFMPEG_PLAYER_CMD_PAUSE,
    LV_FFMPEG_PLAYER_CMD_RESUME,
    /* the following extension commands are only supported by aic */
    LV_FFMPEG_PLAYER_CMD_PLAY_END_EX,       /* data type is bool * */
    LV_FFMPEG_PLAYER_CMD_GET_MEDIA_INFO_EX, /* data type is struct av_media_info * */
    LV_FFMPEG_PLAYER_CMD_SET_VOLUME_EX,     /* data type is s32 *, rang: 0 ~ 100 */
    LV_FFMPEG_PLAYER_CMD_GET_VOLUME_EX,     /* data type is s32 *, rang: 0 ~ 100 */
    LV_FFMPEG_PLAYER_CMD_SET_PLAY_TIME_EX,  /* data type is u64 *, unite: microsecond */
    LV_FFMPEG_PLAYER_CMD_GET_PLAY_TIME_EX,  /* data type is u64 *, unite: microsecond */
    LV_FFMPEG_PLAYED_CMD_KEEP_LAST_FRAME_EX, /* data is NULL, caching last frame on multiple src switches avoids black screen */
    _LV_FFMPEG_PLAYER_CMD_LAST
} lv_ffmpeg_player_cmd_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Register FFMPEG image decoder
 */
void lv_ffmpeg_init(void);

/**
 * Get the number of frames contained in the file
 * @param path image or video file name
 * @return Number of frames, less than 0 means failed
 */
int lv_ffmpeg_get_frame_num(const char * path);

/**
 * Create ffmpeg_player object
 * @param parent pointer to an object, it will be the parent of the new player
 * @return pointer to the created ffmpeg_player
 */
lv_obj_t * lv_ffmpeg_player_create(lv_obj_t * parent);

/**
 * Set the path of the file to be played
 * @param obj pointer to a ffmpeg_player object
 * @param path video file path
 * @return LV_RES_OK: no error; LV_RES_INV: can't get the info.
 */
lv_res_t lv_ffmpeg_player_set_src(lv_obj_t * obj, const char * path);

/**
 * Set command control video player
 * @param obj pointer to a ffmpeg_player object
 * @param cmd control commands
 */
void lv_ffmpeg_player_set_cmd(lv_obj_t * obj, lv_ffmpeg_player_cmd_t cmd);

/**
 * Set the video to automatically replay
 * @param obj pointer to a ffmpeg_player object
 * @param en true: enable the auto restart
 */
void lv_ffmpeg_player_set_auto_restart(lv_obj_t * obj, bool en);

/*=====================
 * Expansion functions
 *====================*/
/**
 * Set command control video player, expansion commands can be used
 * @param obj pointer to a ffmpeg_player object
 * @param cmd control commands
 * @param data set or read data
 */
void lv_ffmpeg_player_set_cmd_ex(lv_obj_t * obj, lv_ffmpeg_player_cmd_t cmd, void *data);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_FFMPEG*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_AIC_FFMPEG_H*/
