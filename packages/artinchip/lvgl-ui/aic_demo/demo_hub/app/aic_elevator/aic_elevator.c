#include "aic_ui.h"
#include "lvgl.h"
#include "aic_lv_ffmpeg.h"

#define RUNNING_DIR_DOWN 0
#define RUNNING_DIR_UP   1

#define BYTE_ALIGN(x, byte) (((x) + ((byte) - 1))&(~((byte) - 1)))

#define GET_HOURS(seconds)   ((seconds) / (60 * 60))
#define GET_MINUTES(seconds) (((seconds) % (60 * 60)) / 60)
#define GET_SECONDS(seconds) ((seconds) % 60)

extern lv_obj_t *com_imgbtn_comp(lv_obj_t *parent, void *img_src, void *pressed_img_src);

extern lv_obj_t *nonto_sans_media_label_comp(lv_obj_t *parent, uint16_t weight);
extern lv_obj_t *nonto_sans_regular_label_comp(lv_obj_t *parent, uint16_t weight);

static void aic_elevator_ui_impl(lv_obj_t *parent);
static void aic_elevator_player_impl(lv_obj_t *parent);

static void ui_aic_elevator_cb(lv_event_t *e);
static void ui_switch_run_dir_cb(lv_event_t *e);

static void time_update_cb(lv_timer_t * timer);
static void floor_update_cb(lv_timer_t * timer);

static char *num_to_imgnum_path(int num);

static int adaptive_width(int width);
static int adaptive_height(int height);

static lv_obj_t *floor_dir;
static lv_obj_t *floor_num;

static lv_timer_t *time_timer;
static lv_timer_t *floor_timer;

static long seconds = 60 * 60 * 8;
static int run_dir = RUNNING_DIR_UP;
static int run_num = 0;

extern void fb_dev_layer_reset(void);
extern void fb_dev_layer_only_video(void);

lv_obj_t *aic_elevator_ui_init(void)
{
    lv_obj_t *aic_elevator_ui = lv_obj_create(NULL);
    lv_obj_clear_flag(aic_elevator_ui, LV_OBJ_FLAG_SCROLLABLE);

#ifdef AIC_CHIP_D12X
    return aic_elevator_ui;
#endif

    aic_elevator_ui_impl(aic_elevator_ui);
    aic_elevator_player_impl(aic_elevator_ui);

    lv_obj_add_event_cb(aic_elevator_ui, ui_aic_elevator_cb, LV_EVENT_ALL, NULL);

    return aic_elevator_ui;
}

static void aic_elevator_ui_impl(lv_obj_t *parent)
{
    lv_obj_t *bg = lv_img_create(parent);
    lv_img_set_src(bg, LVGL_PATH(elevator/bg.png));

    lv_obj_t *floor_bg_1 = com_imgbtn_comp(parent, LVGL_PATH(elevator/floor.png), LVGL_PATH(elevator/floor_press.png));
    lv_obj_set_pos(floor_bg_1, adaptive_width(8), adaptive_height(60));
    floor_dir = lv_img_create(floor_bg_1);
    if (run_dir == RUNNING_DIR_DOWN)
        lv_img_set_src(floor_dir, LVGL_PATH(elevator/down.png));
    else
        lv_img_set_src(floor_dir, LVGL_PATH(elevator/up.png));
    lv_obj_center(floor_dir);

    lv_obj_t *floor_bg_2 = com_imgbtn_comp(parent, LVGL_PATH(elevator/floor.png), LVGL_PATH(elevator/floor_press.png));
    lv_img_set_src(floor_bg_2, LVGL_PATH(elevator/floor.png));
    lv_obj_set_pos(floor_bg_2, adaptive_width(8), adaptive_height(307));
    floor_num = lv_img_create(floor_bg_2);
    lv_img_set_src(floor_num, num_to_imgnum_path(run_num));
    lv_obj_center(floor_num);

    lv_obj_t *welcome_img_label = lv_img_create(parent);
    lv_img_set_src(welcome_img_label, LVGL_PATH(elevator/welcome.png));
    lv_obj_align(welcome_img_label, LV_ALIGN_BOTTOM_MID, adaptive_width(0), -adaptive_height(15));

    lv_obj_t *data_img_label = lv_img_create(parent);
    lv_img_set_src(data_img_label, LVGL_PATH(elevator/date.png));
    lv_obj_align(data_img_label, LV_ALIGN_TOP_RIGHT, -adaptive_width(10), adaptive_height(0));

    int font_weight = 36;
    if (LV_HOR_RES == 480)
        font_weight = 14;
    lv_obj_t *time_label = nonto_sans_media_label_comp(parent, font_weight);
    lv_label_set_text_fmt(time_label, "%02ld:%02ld:%02ld", GET_HOURS(seconds), GET_MINUTES(seconds), GET_SECONDS(seconds));
    lv_obj_align(time_label, LV_ALIGN_DEFAULT, adaptive_width(736), adaptive_height(18));

    lv_obj_add_event_cb(floor_bg_1, ui_switch_run_dir_cb, LV_EVENT_ALL, NULL);

    time_timer = lv_timer_create(time_update_cb, 1000 , time_label);
    floor_timer = lv_timer_create(floor_update_cb, 1000 , NULL);
}

static void aic_elevator_player_impl(lv_obj_t *parent)
{
    lv_obj_t *ffmpeg = lv_ffmpeg_player_create(parent);
    lv_obj_set_size(ffmpeg, adaptive_width(715), adaptive_height(485));
    lv_obj_set_pos(ffmpeg, adaptive_width(300), adaptive_height(62));
    lv_ffmpeg_player_set_src(ffmpeg, LVGL_PATH_ORI(common/elevator_mjpg.mp4));
    lv_ffmpeg_player_set_auto_restart(ffmpeg, true);
    lv_ffmpeg_player_set_cmd(ffmpeg, LV_FFMPEG_PLAYER_CMD_START);
}

static void ui_aic_elevator_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *elevator_scr = (lv_obj_t *)lv_event_get_current_target(e);

    if (code == LV_EVENT_SCREEN_UNLOAD_START) {
        lv_obj_clean(elevator_scr); /* pre-release memory to avoid issues of insufficient memory when loading hte next screen */
        lv_timer_del(time_timer);
        lv_timer_del(floor_timer);
        fb_dev_layer_reset();
    }
    if (code == LV_EVENT_DELETE) {;}
    if (code == LV_EVENT_SCREEN_LOADED) {
        fb_dev_layer_only_video();
    }
}

static void ui_switch_run_dir_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        run_dir = run_dir == RUNNING_DIR_DOWN ? RUNNING_DIR_UP: RUNNING_DIR_DOWN;
        if (run_dir == RUNNING_DIR_DOWN)
            lv_img_set_src(floor_dir, LVGL_PATH(elevator/down.png));
        else
            lv_img_set_src(floor_dir, LVGL_PATH(elevator/up.png));
    }
}

static void time_update_cb(lv_timer_t * timer)
{
    lv_obj_t *label = (lv_obj_t *)timer->user_data;

    seconds++;
    lv_label_set_text_fmt(label, "%02ld:%02ld:%02ld", GET_HOURS(seconds), GET_MINUTES(seconds), GET_SECONDS(seconds));
}

static void floor_update_cb(lv_timer_t * timer)
{
    if (run_dir == RUNNING_DIR_UP) {
        run_num++;
        run_num = run_num > 9 ? 9 : run_num;
    } else {
        run_num--;
        run_num = run_num < 0 ? 0 : run_num;
    }

    if (run_num == 9) {
        run_dir = RUNNING_DIR_DOWN;
    } else if (run_num == 0) {
        run_dir = RUNNING_DIR_UP;
    }

    if (run_dir == RUNNING_DIR_DOWN)
        lv_img_set_src(floor_dir, LVGL_PATH(elevator/down.png));
    else
        lv_img_set_src(floor_dir, LVGL_PATH(elevator/up.png));

    lv_img_set_src(floor_num, num_to_imgnum_path(run_num));
}

/* 0 ~ 9 */
static char *num_to_imgnum_path(int num)
{
    static char *path_list[] = {
        LVGL_PATH(elevator/num_0.png),
        LVGL_PATH(elevator/num_1.png),
        LVGL_PATH(elevator/num_2.png),
        LVGL_PATH(elevator/num_3.png),
        LVGL_PATH(elevator/num_4.png),
        LVGL_PATH(elevator/num_5.png),
        LVGL_PATH(elevator/num_6.png),
        LVGL_PATH(elevator/num_7.png),
        LVGL_PATH(elevator/num_8.png),
        LVGL_PATH(elevator/num_9.png)
    };

    return path_list[num];
}

static int adaptive_width(int width)
{
    float src_width = LV_HOR_RES;
    int align_width = (int)((src_width / 1024.0) * width);
    return BYTE_ALIGN(align_width, 2);
}

static int adaptive_height(int height)
{
    float scr_height = LV_VER_RES;
    int align_height = (int)((scr_height / 600.0) * height);
    return BYTE_ALIGN(align_height, 2);
}
