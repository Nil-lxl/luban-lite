#include <rtthread.h>
#include "bt_api.h"

struct bt_debug_cmd {
    const char *cmd;
    const char *exp;
    int (*entry)(void);
};

typedef struct {
    u16 left_click;
    s16 x_diff;
    s16 y_diff;
}hid_mouse_action_t;

static hid_mouse_action_t mouse_action_para = {0};

typedef enum {
    DIR_UP = 0,
    DIR_LEFTUP,
    DIR_LEFT,
    DIR_LEFTDOWN,
    DIR_DOWN,
    DIR_RIGHTDOWN,
    DIR_RIGHT,
    DIR_RIGHTUP,

    DIR_MAX,
} DIR;

void bt_hid_mouse_action_send(hid_mouse_action_t *p)
{
    bt_hid_set_cursor_xy(p->left_click, p->x_diff, p->y_diff);
}

typedef struct {
    s16 x_diff;
    s16 y_diff;
}xy_diff_t;

static xy_diff_t DIR_XY[] = {
    {0, -20},
    {-20, -20},
    {-20, 0},
    {-20, 20},
    {0, 20},
    {20, 20},
    {20, 0},
    {20, -20},
};

#define ANDRIOD     0

void bt_hid_mouse_swipe_simulate_demo(DIR dir)
{
    memset(&mouse_action_para, 0x00, sizeof(hid_mouse_action_t));

#if 1//ANDRIOD
    //set left button triggered event
    mouse_action_para.left_click = 1;
    //点击屏幕的坐标（实际位置减去初始位置得到的变化量）
    mouse_action_para.x_diff = -540;
    mouse_action_para.y_diff = -1200;

    bt_hid_mouse_action_send(&mouse_action_para);
    msleep(20);
#else
    bt_hid_set_cursor_xy(1, 0, 0);
    msleep(20);
    for (int k =0; k < 5; k++) {
        bt_hid_set_cursor_xy(0, -20, -50);
        msleep(20);
    }
    bt_hid_set_cursor_xy(2, 0, 0);
    msleep(1000);
    mouse_action_para.left_click = 1;//5;
#endif

    mouse_action_para.x_diff = ANDRIOD ? DIR_XY[dir].x_diff : DIR_XY[dir].x_diff ;
    mouse_action_para.y_diff = ANDRIOD ? DIR_XY[dir].y_diff : DIR_XY[dir].y_diff ;

    printf("\tx_diff == %d\n\ty_diff == %d\n", mouse_action_para.x_diff, mouse_action_para.y_diff);

    //上报坐标变化量
    for (int i = 0; i < 5; ++i) {
        bt_hid_mouse_action_send(&mouse_action_para);
        msleep(20);
    }

//松开按键
    mouse_action_para.left_click = 2;//ANDRIOD ? 2 : 6;
    mouse_action_para.x_diff = 0;
    mouse_action_para.y_diff = 0;
    bt_hid_mouse_action_send(&mouse_action_para);
    msleep(5);

//重置光标回屏幕的右下角（screen_x_max，screen_y_max ）
    // bt_hid_reset_cursor_pos();
#if ANDRIOD
    bt_hid_set_cursor_pos(1080, 2400); //安卓机
    bt_hid_reset_cursor_pos();
#else
    bt_hid_set_cursor_pos(1179, 2556); //苹果机
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556); //苹果机
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556); //苹果机
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556); //苹果机
    msleep(20);
    bt_hid_reset_cursor_pos();
#endif
}

void oblique(int x, int y)
{
    bt_hid_set_cursor_xy(1, 0, 0);
    msleep(20);
    for (int i =0; i < 5; i ++) {
        bt_hid_set_cursor_xy(1, x, y);
        msleep(20);
    }
    bt_hid_set_cursor_xy(2, 0, 0);
    msleep(20);
}

static int test_touch_every_dir_demo(void)
{
    // 先设置光标到右下角
#if ANDRIOD
    bt_hid_set_cursor_pos(1080, 2400); //安卓机
#else
    bt_hid_set_cursor_pos(1179, 2556); //苹果机
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556); //苹果机
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556); //苹果机
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556); //苹果机
    msleep(20);
#endif
    bt_hid_reset_cursor_pos();

    msleep(1000);

    printf("starting mouse test...\n");

    for (int j = 0; j < 100; j ++) {
        printf("test round %d\n", j);
        for (int i = 0; i < DIR_MAX; i++) {
            bt_hid_mouse_swipe_simulate_demo(i);
            msleep(1000);
        }
    }

    return 0;
}

static int move_cursor()
{
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_reset_cursor_pos();
    msleep(1000);

    printf("20 times\n");

    bt_hid_set_cursor_xy(5, 0, 0);
    msleep(20);
    for (int k =0; k < 20; k++) {
        bt_hid_set_cursor_xy(5, -25, -50);
        msleep(20);
    }
    bt_hid_set_cursor_xy(6, 0, 0);

    msleep(3000);

    /*---------------*/
    printf("10 times\n");
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_reset_cursor_pos();
    msleep(1000);

    bt_hid_set_cursor_xy(5, 0, 0);
    msleep(20);
    for (int k =0; k < 10; k++) {
        bt_hid_set_cursor_xy(5, -50, -100);
        msleep(20);
    }
    bt_hid_set_cursor_xy(6, 0, 0);

    msleep(3000);

    /*---------------*/
    printf("5 times\n");
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_reset_cursor_pos();
    msleep(1000);

    bt_hid_set_cursor_xy(5, 0, 0);
    msleep(20);
    for (int k =0; k < 5; k++) {
        bt_hid_set_cursor_xy(5, -100, -200);
        msleep(20);
    }
    bt_hid_set_cursor_xy(6, 0, 0);

    // msleep(3000);
    /*---------------*/
    printf("5 times\n");
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_set_cursor_pos(1179, 2556);
    msleep(20);
    bt_hid_reset_cursor_pos();
    msleep(1000);

    bt_hid_set_cursor_xy(1, 0, 0);
    msleep(30);
    for (int k =0; k < 5; k++) {
        bt_hid_set_cursor_xy(0, -20, -20);
        msleep(30);
    }
    bt_hid_set_cursor_xy(2, 0, 0);

    return 0;
}
static struct bt_debug_cmd cmd[] =
{
    /* vol opt */
    {"vol+", "vol+", bt_vol_add},
    {"vol-", "vol-", bt_vol_dec},
    {"mute", "mute", bt_vol_mute},
    {"unmute", "unmute", bt_vol_unmute},

    /* phone call */
    {"phone_break", "phone break", bt_phone_break},
    {"phone_reject", "phone reject", bt_phone_reject},
    {"phone_recall", "phone recall", bt_phone_recall},
    {"phone_incoming", "phone incoming", bt_phone_incoming},
    {"phone_break", "phone break", bt_phone_break},

    /* music */
    {"play", "music play", bt_audio_play},
    {"pause", "music pause", bt_audio_pause},
    {"stop", "audio stop", bt_audio_stop},
    {"prev", "audio prev", bt_audio_prev},
    {"next", "audio next", bt_audio_next},

    {"hid_disconn", "HID disconnected", bt_hid_disconnect},
    {"hid_conn", "HID connect", bt_hid_connect},
    {"hid_mouse", "HID mouse", bt_hid_mouse_test},
    {"reset", "reset", bt_hid_reset_cursor_pos},

    {"test", "test touch every dir", test_touch_every_dir_demo},
    {"move", "move cursor", move_cursor}

};

static void bt_cmd(int argc, char *argv[])
{
    if (argc < 2)
        return ;

    if ((argc == 4) && (strcmp(argv[1], "click") == 0) ) {
        u16 x=0,y=0;
        x = strtoul(argv[2], NULL, 0);
        y = strtoul(argv[3], NULL, 0);
        bt_hid_set_cursor_pos(x, y);
        bt_hid_reset_cursor_pos();
        return ;
    }

    if ((argc == 3) && (strcmp(argv[1], "touch") == 0) ) {
        if (strcmp(argv[2], "left") == 0)
            bt_hid_mouse_swipe_simulate_demo(DIR_LEFT);//touch_left();
        else if (strcmp(argv[2], "right") == 0)
            bt_hid_mouse_swipe_simulate_demo(DIR_RIGHT);//touch_right();
        else if (strcmp(argv[2], "up") == 0)
            bt_hid_mouse_swipe_simulate_demo(DIR_UP);//touch_up();
        else if (strcmp(argv[2], "down") == 0)
            bt_hid_mouse_swipe_simulate_demo(DIR_DOWN);//touch_down();
        else if (strcmp(argv[2], "right_down") == 0)
            oblique(10, 20);
        else if (strcmp(argv[2], "right_up") == 0)
            oblique(10, -20);
        else if (strcmp(argv[2], "left_down") == 0)
            oblique(-10, 20);
        else
            oblique(-10, -20);
        return ;
    }


    for (int i = 0; i < sizeof(cmd) / sizeof(cmd[0]); i++) {
        if (strcmp(argv[1], cmd[i].cmd) == 0) {
            cmd[i].entry();
            return ;
        }
    }

    for (int i = 0; i < sizeof(cmd) / sizeof(cmd[0]); i++) {
        printf("btcmd [%s] : %s\n", cmd[i].cmd, cmd[i].exp);
    }
}

MSH_CMD_EXPORT_ALIAS(bt_cmd, bt_cmd, debug bt command);

// INIT_DEVICE_EXPORT(bt_init);
